//! tower-lsp `Backend` implementation: wires the lexer/parser/analysis
//! pipeline up to the LSP protocol (diagnostics, hover, go-to-definition,
//! find-references, completion).

use dashmap::DashMap;
use tower_lsp::jsonrpc::Result as RpcResult;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer};

use crate::analysis::{self, Analysis, SimpleType, SymbolKind};
use crate::ast::Program;
use crate::diagnostics::{Diag, Severity};
use crate::lexer::{self, Pos, Span};
use crate::parser;

pub struct Document {
    pub text: String,
    pub program: Program,
    pub analysis: Analysis,
    pub diags: Vec<Diag>,
}

pub struct Backend {
    pub client: Client,
    pub docs: DashMap<Url, Document>,
}

impl Backend {
    pub fn new(client: Client) -> Self {
        Backend {
            client,
            docs: DashMap::new(),
        }
    }

    fn analyze_text(text: &str) -> Document {
        let lex = lexer::tokenize(text);
        let (program, mut diags) = parser::parse(&lex.tokens);

        for w in &lex.warnings {
            diags.push(Diag::warning(w.span, w.message.clone()).with_code("lex"));
        }
        if let Some(f) = &lex.fatal {
            diags.push(Diag::error(f.span, f.message.clone()).with_code("lex-fatal"));
        }

        let file_span = whole_file_span(text);
        let analysis = analysis::analyze(&program, file_span);

        let mut all = diags;
        all.extend(analysis.diags.clone());

        Document {
            text: text.to_string(),
            program,
            analysis,
            diags: all,
        }
    }

    async fn publish(&self, uri: Url, doc: &Document) {
        let diags: Vec<Diagnostic> = doc
            .diags
            .iter()
            .map(|d| Diagnostic {
                range: span_to_range(d.span),
                severity: Some(match d.severity {
                    Severity::Error => DiagnosticSeverity::ERROR,
                    Severity::Warning => DiagnosticSeverity::WARNING,
                    Severity::Info => DiagnosticSeverity::INFORMATION,
                    Severity::Hint => DiagnosticSeverity::HINT,
                }),
                code: d.code.map(|c| NumberOrString::String(c.to_string())),
                source: Some("fling".into()),
                message: d.message.clone(),
                ..Default::default()
            })
            .collect();
        self.client.publish_diagnostics(uri, diags, None).await;
    }

    async fn on_change(&self, uri: Url, text: String) {
        let doc = Self::analyze_text(&text);
        self.publish(uri.clone(), &doc).await;
        self.docs.insert(uri, doc);
    }
}

fn whole_file_span(text: &str) -> Span {
    let mut line = 0u32;
    let mut col = 0u32;
    for c in text.chars() {
        if c == '\n' {
            line += 1;
            col = 0;
        } else {
            col += 1;
        }
    }
    Span::new(Pos { line: 0, col: 0 }, Pos { line, col })
}

fn span_to_range(span: Span) -> Range {
    Range {
        start: Position {
            line: span.start.line,
            character: span.start.col,
        },
        end: Position {
            line: span.end.line,
            character: span.end.col,
        },
    }
}

fn pos_from_lsp(p: Position) -> Pos {
    Pos {
        line: p.line,
        col: p.character,
    }
}

fn span_contains(span: Span, pos: Pos) -> bool {
    (pos.line, pos.col) >= (span.start.line, span.start.col)
        && (pos.line, pos.col) <= (span.end.line, span.end.col)
}

#[tower_lsp::async_trait]
impl LanguageServer for Backend {
    async fn initialize(&self, _: InitializeParams) -> RpcResult<InitializeResult> {
        Ok(InitializeResult {
            capabilities: ServerCapabilities {
                text_document_sync: Some(TextDocumentSyncCapability::Kind(
                    TextDocumentSyncKind::FULL,
                )),
                hover_provider: Some(HoverProviderCapability::Simple(true)),
                definition_provider: Some(OneOf::Left(true)),
                references_provider: Some(OneOf::Left(true)),
                completion_provider: Some(CompletionOptions {
                    trigger_characters: Some(vec![".".to_string()]),
                    ..Default::default()
                }),
                document_symbol_provider: Some(OneOf::Left(true)),
                ..Default::default()
            },
            server_info: Some(ServerInfo {
                name: "fling-lsp".into(),
                version: Some(env!("CARGO_PKG_VERSION").into()),
            }),
        })
    }

    async fn initialized(&self, _: InitializedParams) {
        self.client
            .log_message(MessageType::INFO, "fling-lsp initialized")
            .await;
    }

    async fn shutdown(&self) -> RpcResult<()> {
        Ok(())
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        self.on_change(params.text_document.uri, params.text_document.text)
            .await;
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        // FULL sync: the last change event contains the entire new text.
        if let Some(change) = params.content_changes.into_iter().last() {
            self.on_change(params.text_document.uri, change.text).await;
        }
    }

    async fn did_close(&self, params: DidCloseTextDocumentParams) {
        self.docs.remove(&params.text_document.uri);
        self.client
            .publish_diagnostics(params.text_document.uri, vec![], None)
            .await;
    }

    async fn did_save(&self, _: DidSaveTextDocumentParams) {}

    async fn hover(&self, params: HoverParams) -> RpcResult<Option<Hover>> {
        let uri = params.text_document_position_params.text_document.uri;
        let pos = pos_from_lsp(params.text_document_position_params.position);
        let Some(doc) = self.docs.get(&uri) else {
            return Ok(None);
        };

        // 1. Operator hovers (%, /, +, comparisons).
        for oh in &doc.analysis.op_hovers {
            if span_contains(oh.span, pos) {
                return Ok(Some(Hover {
                    contents: HoverContents::Markup(MarkupContent {
                        kind: MarkupKind::Markdown,
                        value: oh.text.clone(),
                    }),
                    range: Some(span_to_range(oh.span)),
                }));
            }
        }

        // 2. Identifier hovers (declarations and uses).
        let mut best: Option<(&crate::analysis::Use, u32)> = None;
        for u in &doc.analysis.uses {
            if span_contains(u.span, pos) {
                let width = u.span.end.col.saturating_sub(u.span.start.col)
                    + 1000 * u.span.end.line.saturating_sub(u.span.start.line);
                if best.map(|(_, w)| width < w).unwrap_or(true) {
                    best = Some((u, width));
                }
            }
        }
        if let Some((u, _)) = best {
            if let Some(sym_id) = u.symbol {
                let sym = &doc.analysis.symbols[sym_id];
                let mut value = String::new();
                match sym.kind {
                    SymbolKind::Function => {
                        let params = sym.params.clone().unwrap_or_default().join(", ");
                        value.push_str(&format!("```fling\nfn {}({params}) {{ ... }}\n```\n\n", sym.name));
                        value.push_str(
                            "Fling has no `return` keyword: the function's result is the value \
                             of its **last statement**, or -- if that's `Null` -- the value of a \
                             variable named `result` if one exists in its scope (spec 6.5).",
                        );
                    }
                    SymbolKind::Global => {
                        value.push_str(&format!("**`{}`** -- built-in global", sym.name));
                        if sym.name == "print" {
                            value.push_str(
                                "\n\nVariadic; prints all arguments space-separated to stdout. \
                                 Arrays print as `Can not print <array>` and objects print as \
                                 `<object>` -- neither is expanded.",
                            );
                        } else {
                            value.push_str(
                                "\n\nNot a keyword -- this is a predeclared constant in the \
                                 global scope and *can* be shadowed by an inner `let`/`const` \
                                 with the same name (bug B11).",
                            );
                        }
                    }
                    _ => {
                        value.push_str(&format!("**`{}`**: {}", sym.name, sym.kind.label()));
                        if sym.simple_type != SimpleType::Unknown {
                            value.push_str(&format!(
                                "\n\n*(usage suggests type: {})*",
                                sym.simple_type.label()
                            ));
                        }
                        if let Some(keys) = &sym.object_keys {
                            if !keys.is_empty() {
                                value.push_str(&format!(
                                    "\n\nKnown properties: `{}`",
                                    keys.join("`, `")
                                ));
                            }
                        }
                    }
                }
                return Ok(Some(Hover {
                    contents: HoverContents::Markup(MarkupContent {
                        kind: MarkupKind::Markdown,
                        value,
                    }),
                    range: Some(span_to_range(u.span)),
                }));
            }
        }

        Ok(None)
    }

    async fn goto_definition(
        &self,
        params: GotoDefinitionParams,
    ) -> RpcResult<Option<GotoDefinitionResponse>> {
        let uri = params.text_document_position_params.text_document.uri;
        let pos = pos_from_lsp(params.text_document_position_params.position);
        let Some(doc) = self.docs.get(&uri) else {
            return Ok(None);
        };
        for u in &doc.analysis.uses {
            if span_contains(u.span, pos) {
                if let Some(sym_id) = u.symbol {
                    let sym = &doc.analysis.symbols[sym_id];
                    if sym.decl_span == Span::default() {
                        // built-in global: no source location
                        return Ok(None);
                    }
                    let loc = Location {
                        uri: uri.clone(),
                        range: span_to_range(sym.decl_span),
                    };
                    return Ok(Some(GotoDefinitionResponse::Scalar(loc)));
                }
                return Ok(None);
            }
        }
        Ok(None)
    }

    async fn references(&self, params: ReferenceParams) -> RpcResult<Option<Vec<Location>>> {
        let uri = params.text_document_position.text_document.uri;
        let pos = pos_from_lsp(params.text_document_position.position);
        let Some(doc) = self.docs.get(&uri) else {
            return Ok(None);
        };
        let target = doc
            .analysis
            .uses
            .iter()
            .find(|u| span_contains(u.span, pos))
            .and_then(|u| u.symbol);
        let Some(target) = target else {
            return Ok(None);
        };
        let include_decl = params.context.include_declaration;
        let locs: Vec<Location> = doc
            .analysis
            .uses
            .iter()
            .filter(|u| u.symbol == Some(target) && (include_decl || !u.is_decl))
            .map(|u| Location {
                uri: uri.clone(),
                range: span_to_range(u.span),
            })
            .collect();
        Ok(Some(locs))
    }

    async fn completion(&self, params: CompletionParams) -> RpcResult<Option<CompletionResponse>> {
        let uri = params.text_document_position.text_document.uri;
        let pos = pos_from_lsp(params.text_document_position.position);
        let Some(doc) = self.docs.get(&uri) else {
            return Ok(None);
        };

        let dot_prefix = identifier_before_dot(&doc.text, pos);

        if let Some(name) = dot_prefix {
            let scope = analysis::scope_at(&doc.analysis.containers, pos, 0);
            let mut items = vec![CompletionItem {
                label: "length".into(),
                kind: Some(CompletionItemKind::PROPERTY),
                detail: Some("Array/String length (the only supported property)".into()),
                ..Default::default()
            }];
            let sym_id = doc
                .analysis
                .scopes
                .get(scope)
                .and_then(|_| resolve_in_chain(&doc.analysis, &name, scope));
            if let Some(id) = sym_id {
                if let Some(keys) = &doc.analysis.symbols[id].object_keys {
                    for k in keys {
                        items.push(CompletionItem {
                            label: k.clone(),
                            kind: Some(CompletionItemKind::FIELD),
                            detail: Some(format!("property of '{name}'")),
                            ..Default::default()
                        });
                    }
                }
            }
            return Ok(Some(CompletionResponse::Array(items)));
        }

        let mut items = Vec::new();
        for kw in ["let", "const", "fn", "if", "else", "while"] {
            items.push(CompletionItem {
                label: kw.into(),
                kind: Some(CompletionItemKind::KEYWORD),
                ..Default::default()
            });
        }

        let scope = analysis::scope_at(&doc.analysis.containers, pos, 0);
        let mut cur = Some(scope);
        let mut seen = std::collections::HashSet::new();
        while let Some(s) = cur {
            if let Some(scope_data) = doc.analysis.scopes.get(s) {
                for (name, &id) in &scope_data.symbols {
                    if !seen.insert(name.clone()) {
                        continue;
                    }
                    let sym = &doc.analysis.symbols[id];
                    let kind = match sym.kind {
                        SymbolKind::Function => CompletionItemKind::FUNCTION,
                        SymbolKind::Param => CompletionItemKind::VARIABLE,
                        SymbolKind::Const => CompletionItemKind::CONSTANT,
                        SymbolKind::Global => CompletionItemKind::CONSTANT,
                        SymbolKind::Let => CompletionItemKind::VARIABLE,
                    };
                    items.push(CompletionItem {
                        label: name.clone(),
                        kind: Some(kind),
                        detail: Some(sym.kind.label().to_string()),
                        ..Default::default()
                    });
                }
                cur = scope_data.parent;
            } else {
                break;
            }
        }

        Ok(Some(CompletionResponse::Array(items)))
    }

    async fn document_symbol(
        &self,
        params: DocumentSymbolParams,
    ) -> RpcResult<Option<DocumentSymbolResponse>> {
        let uri = params.text_document.uri;
        let Some(doc) = self.docs.get(&uri) else {
            return Ok(None);
        };
        #[allow(deprecated)]
        let symbols: Vec<SymbolInformation> = doc
            .analysis
            .symbols
            .iter()
            .filter(|s| !matches!(s.kind, SymbolKind::Global | SymbolKind::Param))
            .map(|s| SymbolInformation {
                name: s.name.clone(),
                kind: match s.kind {
                    SymbolKind::Function => SymbolKind_LSP::FUNCTION,
                    SymbolKind::Const => SymbolKind_LSP::CONSTANT,
                    _ => SymbolKind_LSP::VARIABLE,
                },
                tags: None,
                deprecated: None,
                location: Location {
                    uri: uri.clone(),
                    range: span_to_range(s.decl_span),
                },
                container_name: None,
            })
            .collect();
        Ok(Some(DocumentSymbolResponse::Flat(symbols)))
    }
}

// tower-lsp's SymbolKind clashes in name with our own analysis::SymbolKind;
// alias it locally for the document_symbol handler above.
#[allow(non_camel_case_types)]
type SymbolKind_LSP = tower_lsp::lsp_types::SymbolKind;

fn resolve_in_chain(analysis: &Analysis, name: &str, scope: analysis::ScopeId) -> Option<analysis::SymbolId> {
    let mut cur = Some(scope);
    while let Some(s) = cur {
        if let Some(id) = analysis.scopes[s].symbols.get(name) {
            return Some(*id);
        }
        cur = analysis.scopes[s].parent;
    }
    None
}

/// Looks backward from `pos` on its line to see whether the cursor directly
/// follows `<identifier>.`, returning the identifier if so.
fn identifier_before_dot(text: &str, pos: Pos) -> Option<String> {
    let line = text.lines().nth(pos.line as usize)?;
    let chars: Vec<char> = line.chars().collect();
    let col = (pos.col as usize).min(chars.len());
    if col == 0 || chars[col - 1] != '.' {
        return None;
    }
    let mut i = col - 1;
    let mut ident = String::new();
    while i > 0 && chars[i - 1].is_ascii_alphanumeric() {
        i -= 1;
        ident.insert(0, chars[i]);
    }
    if ident.is_empty() {
        None
    } else {
        Some(ident)
    }
}
