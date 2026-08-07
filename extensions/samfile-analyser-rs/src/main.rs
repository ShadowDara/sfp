use std::collections::HashMap;
use std::sync::Arc;

use tokio::sync::RwLock;
use tower_lsp::jsonrpc::Result;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer, LspService, Server};

#[derive(Default)]
struct DocumentData {
    text: String,
}

struct Backend {
    client: Client,
    documents: Arc<RwLock<HashMap<String, DocumentData>>>,
}

impl Backend {
    fn parse_defines(text: &str) -> HashMap<String, String> {
        let mut defs = HashMap::new();

        for line in text.lines() {
            let line = line.trim();

            if let Some(rest) = line.strip_prefix("#define ") {
                let mut parts = rest.splitn(2, ' ');

                if let (Some(name), Some(value)) = (parts.next(), parts.next()) {
                    defs.insert(name.to_string(), value.trim().to_string());
                }
            }
        }

        defs
    }

    fn word_at(text: &str, pos: Position) -> Option<String> {
        let line = text.lines().nth(pos.line as usize)?;

        let chars: Vec<char> = line.chars().collect();

        let col = pos.character as usize;
        if col >= chars.len() {
            return None;
        }

        let mut start = col;
        while start > 0 {
            let c = chars[start - 1];
            if c.is_alphanumeric() || c == '_' {
                start -= 1;
            } else {
                break;
            }
        }

        let mut end = col;
        while end < chars.len() {
            let c = chars[end];
            if c.is_alphanumeric() || c == '_' {
                end += 1;
            } else {
                break;
            }
        }

        Some(chars[start..end].iter().collect())
    }
}

#[tower_lsp::async_trait]
impl LanguageServer for Backend {
    async fn initialize(&self, _: InitializeParams) -> Result<InitializeResult> {
        eprintln!("INITIALIZE CALLED");

        Ok(InitializeResult {
            capabilities: ServerCapabilities {
                text_document_sync: Some(TextDocumentSyncCapability::Kind(
                    TextDocumentSyncKind::FULL,
                )),
                hover_provider: Some(HoverProviderCapability::Simple(true)),
                semantic_tokens_provider: Some(
                    SemanticTokensServerCapabilities::SemanticTokensOptions(
                        SemanticTokensOptions {
                            legend: SemanticTokensLegend {
                                token_types: vec![SemanticTokenType::MACRO],
                                token_modifiers: vec![],
                            },

                            full: Some(SemanticTokensFullOptions::Bool(true)),

                            range: None,
                            work_done_progress_options: Default::default(),
                        },
                    ),
                ),

                ..Default::default()
            },
            ..Default::default()
        })
    }

    async fn semantic_tokens_full(
        &self,
        params: SemanticTokensParams,
    ) -> Result<Option<SemanticTokensResult>> {
        let docs = self.documents.read().await;

        let uri = params.text_document.uri.to_string();

        let doc = match docs.get(&uri) {
            Some(doc) => doc,
            None => return Ok(None),
        };

        let mut tokens = Vec::new();

        for (line_idx, line) in doc.text.lines().enumerate() {
            if line.trim().starts_with("#define") {
                let parts: Vec<&str> = line.split_whitespace().collect();

                if parts.len() >= 2 {
                    let name = parts[1];

                    let start = line.find(name).unwrap();

                    tokens.push(SemanticToken {
                        delta_line: line_idx as u32,
                        delta_start: start as u32,
                        length: name.len() as u32,
                        token_type: 0,
                        token_modifiers_bitset: 0,
                    });
                }
            }
        }

        Ok(Some(SemanticTokensResult::Tokens(SemanticTokens {
            result_id: None,
            data: tokens,
        })))
    }

    async fn initialized(&self, _: InitializedParams) {
        self.client
            .log_message(MessageType::INFO, "Samfile LSP started")
            .await;
    }

    async fn shutdown(&self) -> Result<()> {
        Ok(())
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        let mut docs = self.documents.write().await;

        docs.insert(
            params.text_document.uri.to_string(),
            DocumentData {
                text: params.text_document.text,
            },
        );
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let mut docs = self.documents.write().await;

        if let Some(doc) = docs.get_mut(params.text_document.uri.as_str()) {
            if let Some(change) = params.content_changes.first() {
                doc.text = change.text.clone();
            }
        }
    }

    async fn hover(&self, params: HoverParams) -> Result<Option<Hover>> {
        let docs = self.documents.read().await;

        let uri = params
            .text_document_position_params
            .text_document
            .uri
            .to_string();

        let doc = match docs.get(&uri) {
            Some(doc) => doc,
            None => return Ok(None),
        };

        let pos = params.text_document_position_params.position;

        let word = match Self::word_at(&doc.text, pos) {
            Some(word) => word,
            None => return Ok(None),
        };

        let defines = Self::parse_defines(&doc.text);

        if let Some(value) = defines.get(&word) {
            return Ok(Some(Hover {
                contents: HoverContents::Markup(MarkupContent {
                    kind: MarkupKind::Markdown,
                    value: format!("### Macro: {}\n\n```text\n{} -> {}\n```", word, word, value),
                }),
                range: None,
            }));
        }

        Ok(None)
    }
}

#[tokio::main]
async fn main() {
    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    let (service, socket) = LspService::new(|client| Backend {
        client,
        documents: Arc::new(RwLock::new(HashMap::new())),
    });

    Server::new(stdin, stdout, socket).serve(service).await;
}
