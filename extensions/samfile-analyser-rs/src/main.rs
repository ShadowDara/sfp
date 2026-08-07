use std::collections::HashMap;
use std::sync::Arc;

use tokio::sync::RwLock;
use tower_lsp::jsonrpc::Result;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer, LspService, Server};

mod bridge;

#[derive(Debug, Clone, Copy)]
pub enum ParserMode {
    Default,
    Version2,
    Unknown,
}

#[derive(Default)]
struct DocumentData {
    text: String,
}

struct Backend {
    client: Client,
    documents: Arc<RwLock<HashMap<String, DocumentData>>>,
}

impl Backend {
    async fn log(&self, msg: impl Into<String>) {
        let msg = msg.into();

        eprintln!("[SAMFILE LSP] {}", msg);

        self.client.log_message(MessageType::INFO, msg).await;
    }

    fn detect_parser_mode(text: &str) -> ParserMode {
        let defines = Self::parse_defines(text);

        match defines.get("VERSION") {
            None => ParserMode::Default,

            Some(version) => match version.as_str() {
                "0" => ParserMode::Default,
                "2" => ParserMode::Version2,
                _ => ParserMode::Unknown,
            },
        }
    }

    fn parse_defines(text: &str) -> HashMap<String, String> {
        let mut defs = HashMap::new();

        for line in text.lines() {
            let line = line.trim();

            if let Some(rest) = line.strip_prefix("#define") {
                let parts: Vec<&str> = rest.split_whitespace().collect();

                if parts.len() >= 2 {
                    defs.insert(parts[0].to_string(), parts[1..].join(" "));
                }
            }
        }

        defs
    }

    fn parse_document(text: &str) -> ParserMode {
        return Self::detect_parser_mode(text);
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
    async fn initialize(&self, params: InitializeParams) -> Result<InitializeResult> {
        self.log(format!("INITIALIZE CALLED client={:?}", params.client_info))
            .await;

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
                                // token types
                                token_types: vec![
                                    SemanticTokenType::MACRO,
                                    SemanticTokenType::COMMENT,
                                ],
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
        self.log(format!(
            "SEMANTIC TOKENS REQUEST {}",
            params.text_document.uri
        ))
        .await;

        let docs = self.documents.read().await;

        let uri = params.text_document.uri.to_string();

        let doc = match docs.get(&uri) {
            Some(doc) => doc,
            None => return Ok(None),
        };

        let mode = Self::parse_document(&doc.text);

        self.log(format!("parser mode: {:?}", mode)).await;

        if !matches!(mode, ParserMode::Default) {
            return Ok(None);
        }

        let mut tokens = Vec::new();

        let mut last_line = 0;
        let mut last_start = 0;

        for (line_idx, line) in doc.text.lines().enumerate() {
            let trimmed = line.trim_start();

            let mut add_token = |start: usize, length: usize, token_type: u32| {
                let delta_line = line_idx as u32 - last_line;

                let delta_start = if delta_line == 0 {
                    start as u32 - last_start
                } else {
                    start as u32
                };

                tokens.push(SemanticToken {
                    delta_line,
                    delta_start,
                    length: length as u32,
                    token_type,
                    token_modifiers_bitset: 0,
                });

                last_line = line_idx as u32;
                last_start = start as u32;
            };

            // # Kommentar
            if trimmed.starts_with('#') && !trimmed.starts_with("#define") {
                let start = line.find('#').unwrap();

                add_token(start, line.len() - start, 1);

                continue;
            }

            // // Kommentar
            if let Some(start) = line.find("//") {
                add_token(start, line.len() - start, 1);

                continue;
            }

            // #define NAME
            if trimmed.starts_with("#define") {
                let parts: Vec<&str> = line.split_whitespace().collect();

                if parts.len() >= 2 {
                    let name = parts[1];

                    if let Some(start) = line.find(name) {
                        add_token(start, name.len(), 0);
                    }
                }
            }
        }

        self.log(format!("generated {} semantic tokens", tokens.len()))
            .await;

        Ok(Some(SemanticTokensResult::Tokens(SemanticTokens {
            result_id: None,
            data: tokens,
        })))
    }

    async fn initialized(&self, _: InitializedParams) {
        self.log("Samfile LSP started").await;
    }

    async fn shutdown(&self) -> Result<()> {
        Ok(())
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        let uri = params.text_document.uri.to_string();

        self.log(format!(
            "OPEN {} ({} chars)",
            uri,
            params.text_document.text.len()
        ))
        .await;

        let mut docs = self.documents.write().await;

        docs.insert(
            uri,
            DocumentData {
                text: params.text_document.text,
            },
        );
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri.to_string();

        self.log(format!("CHANGE {}", uri)).await;

        let mut docs = self.documents.write().await;

        if let Some(doc) = docs.get_mut(&uri) {
            if let Some(change) = params.content_changes.first() {
                self.log(format!("new document size={} chars", change.text.len()))
                    .await;

                doc.text = change.text.clone();
            }
        } else {
            self.log("CHANGE received for unknown document").await;
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

        let mode = Self::parse_document(&doc.text);

        self.log(format!("parser mode: {:?}", mode)).await;

        if !matches!(mode, ParserMode::Default) {
            return Ok(None);
        }

        let pos = params.text_document_position_params.position;

        let word = match Self::word_at(&doc.text, pos) {
            Some(word) => word,
            None => return Ok(None),
        };

        self.log(format!("HOVER word={}", word)).await;

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

        self.log(format!("no define found for {}", word)).await;

        Ok(None)
    }
}

#[tokio::main]
async fn main() {
    // Never stdout benutzen bei LSP
    eprintln!("Samfile LSP starting ...");

    let text = bridge::ffi::hello_cpp();

    eprintln!("{}", text);

    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    let (service, socket) = LspService::new(|client| Backend {
        client,
        documents: Arc::new(RwLock::new(HashMap::new())),
    });

    Server::new(stdin, stdout, socket).serve(service).await;
}
