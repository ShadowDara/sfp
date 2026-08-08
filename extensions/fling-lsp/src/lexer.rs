//! Lexer for Fling, reproducing the *actual* behaviour of the C++ reference
//! implementation described in `fling-language-spec.md` (section 2), including
//! its quirks (no underscores in identifiers, no decimal points, no string
//! escapes, single `&`/`|` are skipped with a warning, unknown characters
//! abort tokenization entirely).

use std::fmt;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Pos {
    /// 0-based line, matching LSP conventions.
    pub line: u32,
    /// 0-based column (character offset), matching LSP conventions.
    pub col: u32,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Span {
    pub start: Pos,
    pub end: Pos,
}

impl Span {
    pub fn new(start: Pos, end: Pos) -> Self {
        Span { start, end }
    }
    pub fn point(p: Pos) -> Self {
        Span { start: p, end: p }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum TokenKind {
    // Keywords
    Let,
    Const,
    Fn,
    If,
    Else,
    While,

    // Literals / identifiers
    Identifier(String),
    Number(String), // digit-sequence only, no decimal point (spec 2.5)
    String(String),

    // Punctuation
    OpenParen,
    CloseParen,
    OpenCurly,
    CloseCurly,
    OpenSquare,
    CloseSquare,
    Comma,
    Dot,
    Colon,
    Semicolon,

    // Operators
    Equals,   // =
    EqEq,     // ==
    NotEq,    // !=
    Lt,       // <
    Gt,       // >
    Le,       // <=
    Ge,       // >=
    Plus,     // +
    Minus,    // -
    Star,     // *
    Slash,    // /
    Percent,  // %
    Bang,     // ! (unary not)
    AndAnd,   // &&
    OrOr,     // ||

    Eof,
}

impl fmt::Display for TokenKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use TokenKind::*;
        match self {
            Let => write!(f, "'let'"),
            Const => write!(f, "'const'"),
            Fn => write!(f, "'fn'"),
            If => write!(f, "'if'"),
            Else => write!(f, "'else'"),
            While => write!(f, "'while'"),
            Identifier(s) => write!(f, "identifier '{s}'"),
            Number(s) => write!(f, "number '{s}'"),
            String(s) => write!(f, "string \"{s}\""),
            OpenParen => write!(f, "'('"),
            CloseParen => write!(f, "')'"),
            OpenCurly => write!(f, "'{{'"),
            CloseCurly => write!(f, "'}}'"),
            OpenSquare => write!(f, "'['"),
            CloseSquare => write!(f, "']'"),
            Comma => write!(f, "','"),
            Dot => write!(f, "'.'"),
            Colon => write!(f, "':'"),
            Semicolon => write!(f, "';'"),
            Equals => write!(f, "'='"),
            EqEq => write!(f, "'=='"),
            NotEq => write!(f, "'!='"),
            Lt => write!(f, "'<'"),
            Gt => write!(f, "'>'"),
            Le => write!(f, "'<='"),
            Ge => write!(f, "'>='"),
            Plus => write!(f, "'+'"),
            Minus => write!(f, "'-'"),
            Star => write!(f, "'*'"),
            Slash => write!(f, "'/'"),
            Percent => write!(f, "'%'"),
            Bang => write!(f, "'!'"),
            AndAnd => write!(f, "'&&'"),
            OrOr => write!(f, "'||'"),
            Eof => write!(f, "end of file"),
        }
    }
}

#[derive(Debug, Clone)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub struct LexWarning {
    pub message: String,
    pub span: Span,
}

/// Result of tokenizing: either a full token stream, or a fatal error that
/// mirrors the reference lexer's behaviour of discarding the rest of the
/// source on an unrecognized character.
pub struct LexResult {
    pub tokens: Vec<Token>,
    pub warnings: Vec<LexWarning>,
    /// Set if an unrecognized character was hit (spec 2.6: the whole rest of
    /// the source is discarded by the reference lexer at that point).
    pub fatal: Option<LexWarning>,
}

pub fn tokenize(src: &str) -> LexResult {
    let chars: Vec<char> = src.chars().collect();
    let mut i = 0usize;
    let mut line: u32 = 0;
    let mut col: u32 = 0;
    let mut tokens = Vec::new();
    let mut warnings = Vec::new();
    let mut fatal = None;

    let advance = |i: &mut usize, line: &mut u32, col: &mut u32, chars: &[char]| -> char {
        let c = chars[*i];
        *i += 1;
        if c == '\n' {
            *line += 1;
            *col = 0;
        } else {
            *col += 1;
        }
        c
    };

    while i < chars.len() {
        let start_pos = Pos { line, col };
        let c = chars[i];

        // Whitespace (space, tab) - skippable
        if c == ' ' || c == '\t' {
            advance(&mut i, &mut line, &mut col, &chars);
            continue;
        }
        // Newlines: no syntactic meaning, just tracked for position.
        if c == '\n' || c == '\r' {
            advance(&mut i, &mut line, &mut col, &chars);
            continue;
        }

        // Comments: # ... or // ...
        if c == '#' {
            while i < chars.len() && chars[i] != '\n' {
                advance(&mut i, &mut line, &mut col, &chars);
            }
            continue;
        }
        if c == '/' && i + 1 < chars.len() && chars[i + 1] == '/' {
            while i < chars.len() && chars[i] != '\n' {
                advance(&mut i, &mut line, &mut col, &chars);
            }
            continue;
        }

        // Identifiers / keywords: must start with alpha, ASCII only, no '_'.
        if c.is_ascii_alphabetic() {
            let mut s = String::new();
            while i < chars.len() && chars[i].is_ascii_alphanumeric() {
                s.push(advance(&mut i, &mut line, &mut col, &chars));
            }
            let end_pos = Pos { line, col };
            let span = Span::new(start_pos, end_pos);
            let kind = match s.as_str() {
                "let" => TokenKind::Let,
                "const" => TokenKind::Const,
                "fn" => TokenKind::Fn,
                "if" => TokenKind::If,
                "else" => TokenKind::Else,
                "while" => TokenKind::While,
                _ => TokenKind::Identifier(s),
            };
            tokens.push(Token { kind, span });
            continue;
        }

        // Numbers: digit sequences only (no decimal point support).
        if c.is_ascii_digit() {
            let mut s = String::new();
            while i < chars.len() && chars[i].is_ascii_digit() {
                s.push(advance(&mut i, &mut line, &mut col, &chars));
            }
            let end_pos = Pos { line, col };
            tokens.push(Token {
                kind: TokenKind::Number(s),
                span: Span::new(start_pos, end_pos),
            });
            continue;
        }

        // Strings: "..." with NO escape handling. A `"` always ends the
        // string immediately; unterminated strings run to EOF.
        if c == '"' {
            advance(&mut i, &mut line, &mut col, &chars); // consume opening quote
            let mut s = String::new();
            while i < chars.len() && chars[i] != '"' {
                s.push(advance(&mut i, &mut line, &mut col, &chars));
            }
            if i < chars.len() {
                advance(&mut i, &mut line, &mut col, &chars); // consume closing quote
            } else {
                warnings.push(LexWarning {
                    message: "unterminated string literal (runs to end of file)".into(),
                    span: Span::new(start_pos, Pos { line, col }),
                });
            }
            let end_pos = Pos { line, col };
            tokens.push(Token {
                kind: TokenKind::String(s),
                span: Span::new(start_pos, end_pos),
            });
            continue;
        }

        // Two-character operators first.
        let two: Option<(char, char, TokenKind)> = if i + 1 < chars.len() {
            Some((c, chars[i + 1], TokenKind::Eof))
        } else {
            None
        };
        if let Some((a, b, _)) = two {
            let kind = match (a, b) {
                ('=', '=') => Some(TokenKind::EqEq),
                ('!', '=') => Some(TokenKind::NotEq),
                ('<', '=') => Some(TokenKind::Le),
                ('>', '=') => Some(TokenKind::Ge),
                ('&', '&') => Some(TokenKind::AndAnd),
                ('|', '|') => Some(TokenKind::OrOr),
                _ => None,
            };
            if let Some(kind) = kind {
                advance(&mut i, &mut line, &mut col, &chars);
                advance(&mut i, &mut line, &mut col, &chars);
                let end_pos = Pos { line, col };
                tokens.push(Token {
                    kind,
                    span: Span::new(start_pos, end_pos),
                });
                continue;
            }
        }

        // Single-character punctuation / operators.
        let single = match c {
            '(' => Some(TokenKind::OpenParen),
            ')' => Some(TokenKind::CloseParen),
            '{' => Some(TokenKind::OpenCurly),
            '}' => Some(TokenKind::CloseCurly),
            '[' => Some(TokenKind::OpenSquare),
            ']' => Some(TokenKind::CloseSquare),
            ',' => Some(TokenKind::Comma),
            '.' => Some(TokenKind::Dot),
            ':' => Some(TokenKind::Colon),
            ';' => Some(TokenKind::Semicolon),
            '=' => Some(TokenKind::Equals),
            '<' => Some(TokenKind::Lt),
            '>' => Some(TokenKind::Gt),
            '+' => Some(TokenKind::Plus),
            '-' => Some(TokenKind::Minus),
            '*' => Some(TokenKind::Star),
            '/' => Some(TokenKind::Slash),
            '%' => Some(TokenKind::Percent),
            '!' => Some(TokenKind::Bang),
            _ => None,
        };
        if let Some(kind) = single {
            advance(&mut i, &mut line, &mut col, &chars);
            let end_pos = Pos { line, col };
            tokens.push(Token {
                kind,
                span: Span::new(start_pos, end_pos),
            });
            continue;
        }

        // Single & or | (not doubled): reference lexer warns and *skips*
        // the character silently, producing no token (spec 2.6).
        if c == '&' || c == '|' {
            advance(&mut i, &mut line, &mut col, &chars);
            let end_pos = Pos { line, col };
            warnings.push(LexWarning {
                message: format!(
                    "unexpected '{c}' (bitwise operators are not supported by Fling; did you mean '{c}{c}'?)"
                ),
                span: Span::new(start_pos, end_pos),
            });
            continue;
        }

        // Unknown character: reference lexer discards the ENTIRE rest of the
        // source at this point. We mirror that for diagnostic purposes but
        // keep whatever tokens we already collected so the rest of the LSP
        // can still do something useful with the prefix.
        let end_pos = Pos {
            line,
            col: col + 1,
        };
        fatal = Some(LexWarning {
            message: format!(
                "unrecognized character '{c}' in source; the reference interpreter discards the rest of the file at this point"
            ),
            span: Span::new(start_pos, end_pos),
        });
        break;
    }

    let eof_pos = Pos { line, col };
    tokens.push(Token {
        kind: TokenKind::Eof,
        span: Span::point(eof_pos),
    });

    LexResult {
        tokens,
        warnings,
        fatal,
    }
}
