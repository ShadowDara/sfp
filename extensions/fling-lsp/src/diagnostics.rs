use crate::lexer::Span;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Severity {
    Error,
    Warning,
    Info,
    Hint,
}

#[derive(Debug, Clone)]
pub struct Diag {
    pub span: Span,
    pub severity: Severity,
    pub message: String,
    /// Short machine-readable code, e.g. "B1", "B14", "syntax", ... used so
    /// tests / hover text can cross-reference the bug list in section 9 of
    /// the spec.
    pub code: Option<&'static str>,
}

impl Diag {
    pub fn error(span: Span, message: impl Into<String>) -> Self {
        Diag {
            span,
            severity: Severity::Error,
            message: message.into(),
            code: None,
        }
    }
    pub fn warning(span: Span, message: impl Into<String>) -> Self {
        Diag {
            span,
            severity: Severity::Warning,
            message: message.into(),
            code: None,
        }
    }
    pub fn info(span: Span, message: impl Into<String>) -> Self {
        Diag {
            span,
            severity: Severity::Info,
            message: message.into(),
            code: None,
        }
    }
    pub fn with_code(mut self, code: &'static str) -> Self {
        self.code = Some(code);
        self
    }
}
