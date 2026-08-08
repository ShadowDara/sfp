use std::env;
use std::fs;

use fling_lsp::{analysis, diagnostics, diagnostics::Severity, lexer, parser};

fn main() {
    let path = env::args().nth(1).expect("usage: check <file.fling>");
    let text = fs::read_to_string(&path).expect("read file");

    let lex = lexer::tokenize(&text);
    let (program, mut diags) = parser::parse(&lex.tokens);
    for w in &lex.warnings {
        diags.push(diagnostics::Diag::warning(w.span, w.message.clone()));
    }
    if let Some(f) = &lex.fatal {
        diags.push(diagnostics::Diag::error(f.span, f.message.clone()));
    }

    let mut end = lexer::Pos::default();
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
    end.line = line;
    end.col = col;
    let file_span = lexer::Span::new(lexer::Pos::default(), end);

    let a = analysis::analyze(&program, file_span);
    diags.extend(a.diags.clone());

    diags.sort_by_key(|d| (d.span.start.line, d.span.start.col));

    for d in &diags {
        let sev = match d.severity {
            Severity::Error => "ERROR",
            Severity::Warning => "WARN ",
            Severity::Info => "INFO ",
            Severity::Hint => "HINT ",
        };
        println!(
            "{}:{}-{}:{} [{}]{} {}",
            d.span.start.line + 1,
            d.span.start.col + 1,
            d.span.end.line + 1,
            d.span.end.col + 1,
            sev,
            d.code.map(|c| format!(" ({c})")).unwrap_or_default(),
            d.message
        );
    }
    println!("\n{} diagnostics total", diags.len());
}
