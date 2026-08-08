//! Bindeglied zwischen dem Samfile-LSP und der `fling-lsp`-Library.
//! Nimmt reinen Fling-Quelltext (bereits aus der `FLING`-Section extrahiert,
//! also `SectionInfo::content`) entgegen und liefert Diagnostics/Hover in
//! tower-lsp-Typen, mit den Zeilennummern bereits auf das Gesamtdokument
//! umgerechnet (`line_offset` = `section.start_line + 1`).

use tower_lsp::lsp_types::{Diagnostic, DiagnosticSeverity, NumberOrString, Position, Range};

use fling_lsp::analysis::{self, SimpleType, Use};
use fling_lsp::diagnostics::{Diag, Severity};
use fling_lsp::lexer::{self, Pos, Span};
use fling_lsp::parser;

fn whole_span(text: &str) -> Span {
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
    Span::new(Pos::default(), Pos { line, col })
}

fn span_to_range(span: Span, line_offset: u32) -> Range {
    Range {
        start: Position {
            line: span.start.line + line_offset,
            character: span.start.col,
        },
        end: Position {
            line: span.end.line + line_offset,
            character: span.end.col,
        },
    }
}

fn span_contains(span: Span, pos: Pos) -> bool {
    (pos.line, pos.col) >= (span.start.line, span.start.col)
        && (pos.line, pos.col) <= (span.end.line, span.end.col)
}

fn severity_to_lsp(s: Severity) -> DiagnosticSeverity {
    match s {
        Severity::Error => DiagnosticSeverity::ERROR,
        Severity::Warning => DiagnosticSeverity::WARNING,
        Severity::Info => DiagnosticSeverity::INFORMATION,
        Severity::Hint => DiagnosticSeverity::HINT,
    }
}

fn diag_to_lsp(d: &Diag, line_offset: u32) -> Diagnostic {
    Diagnostic {
        range: span_to_range(d.span, line_offset),
        severity: Some(severity_to_lsp(d.severity)),
        code: d.code.map(|c| NumberOrString::String(c.to_string())),
        source: Some("fling".into()),
        message: d.message.clone(),
        ..Default::default()
    }
}

/// Läuft die komplette Lex/Parse/Analyse-Pipeline für den Inhalt einer
/// `FLING`-Section (`SectionInfo::content`) und gibt fertige LSP-Diagnostics
/// zurück, mit Zeilen bereits um `line_offset` verschoben.
pub fn fling_diagnostics(section_text: &str, line_offset: u32) -> Vec<Diagnostic> {
    let lex = lexer::tokenize(section_text);
    let (program, mut diags) = parser::parse(&lex.tokens);

    for w in &lex.warnings {
        diags.push(Diag::warning(w.span, w.message.clone()).with_code("lex"));
    }
    if let Some(f) = &lex.fatal {
        diags.push(Diag::error(f.span, f.message.clone()).with_code("lex-fatal"));
    }

    let file_span = whole_span(section_text);
    let a = analysis::analyze(&program, file_span);
    diags.extend(a.diags);

    diags.iter().map(|d| diag_to_lsp(d, line_offset)).collect()
}

/// Hover für eine Position *innerhalb* der Section (0-basiert, relativ zum
/// Section-Text, d.h. schon um `line_offset` zurückgerechnet). Gibt bei
/// Treffer `(Range im Gesamtdokument, Markdown-Text)` zurück.
pub fn fling_hover(
    section_text: &str,
    local_pos: Position,
    line_offset: u32,
) -> Option<(Range, String)> {
    let lex = lexer::tokenize(section_text);
    let (program, _diags) = parser::parse(&lex.tokens);
    let file_span = whole_span(section_text);
    let a = analysis::analyze(&program, file_span);

    let pos = Pos {
        line: local_pos.line,
        col: local_pos.character,
    };

    // 1. Operator-Hover (%, /, +, Vergleiche).
    for oh in &a.op_hovers {
        if span_contains(oh.span, pos) {
            return Some((span_to_range(oh.span, line_offset), oh.text.clone()));
        }
    }

    // 2. Identifier-Hover (Deklarationen und Verwendungen); schmalste Span gewinnt.
    let mut best: Option<(&Use, u32)> = None;
    for u in &a.uses {
        if span_contains(u.span, pos) {
            let width = u.span.end.col.saturating_sub(u.span.start.col)
                + 1000 * u.span.end.line.saturating_sub(u.span.start.line);
            if best.map(|(_, w)| width < w).unwrap_or(true) {
                best = Some((u, width));
            }
        }
    }
    let (u, _) = best?;
    let sym_id = u.symbol?;
    let sym = &a.symbols[sym_id];

    let mut value = format!("**`{}`**: {}", sym.name, sym.kind.label());
    if sym.simple_type != SimpleType::Unknown {
        value.push_str(&format!("\n\n*(vermuteter Typ: {})*", sym.simple_type.label()));
    }
    if let Some(keys) = &sym.object_keys {
        if !keys.is_empty() {
            value.push_str(&format!("\n\nBekannte Properties: `{}`", keys.join("`, `")));
        }
    }

    Some((span_to_range(u.span, line_offset), value))
}
