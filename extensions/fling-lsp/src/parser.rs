//! Recursive-descent parser for Fling, following the EBNF grammar and
//! precedence table in spec section 3.
//!
//! Unlike the C++ reference implementation (which calls `exit(1)` on the
//! first fatal syntax error), this parser always tries to recover and
//! produce a best-effort AST plus a list of diagnostics, since an LSP needs
//! to keep working on a file that's mid-edit and therefore often invalid.

use crate::ast::*;
use crate::diagnostics::Diag;
use crate::lexer::{Pos, Span, Token, TokenKind};

pub struct Parser<'a> {
    tokens: &'a [Token],
    pos: usize,
    pub diags: Vec<Diag>,
}

impl<'a> Parser<'a> {
    pub fn new(tokens: &'a [Token]) -> Self {
        Parser {
            tokens,
            pos: 0,
            diags: Vec::new(),
        }
    }

    pub fn parse_program(mut self) -> (Program, Vec<Diag>) {
        let mut body = Vec::new();
        while !self.is_eof() {
            let before = self.pos;
            let stmt = self.parse_stmt();
            body.push(stmt);
            if self.pos == before {
                // Safety net: guarantee forward progress.
                self.advance();
            }
        }
        (Program { body }, self.diags)
    }

    // ---- token helpers -------------------------------------------------

    fn cur(&self) -> &Token {
        &self.tokens[self.pos.min(self.tokens.len() - 1)]
    }

    fn peek_kind(&self) -> &TokenKind {
        &self.cur().kind
    }

    fn is_eof(&self) -> bool {
        matches!(self.peek_kind(), TokenKind::Eof)
    }

    fn advance(&mut self) -> Token {
        let t = self.cur().clone();
        if !self.is_eof() {
            self.pos += 1;
        }
        t
    }

    fn check(&self, kind: &TokenKind) -> bool {
        std::mem::discriminant(self.peek_kind()) == std::mem::discriminant(kind)
    }

    fn matches(&mut self, kind: &TokenKind) -> bool {
        if self.check(kind) {
            self.advance();
            true
        } else {
            false
        }
    }

    fn expect(&mut self, kind: TokenKind, ctx: &str) -> Option<Token> {
        if self.check(&kind) {
            Some(self.advance())
        } else {
            let tok = self.cur().clone();
            self.diags.push(
                Diag::error(
                    tok.span,
                    format!(
                        "expected {} {ctx}, found {}",
                        kind, tok.kind
                    ),
                )
                .with_code("syntax"),
            );
            None
        }
    }

    /// Skip tokens until we hit something that plausibly starts a new
    /// statement, so a single error doesn't cascade into hundreds.
    /// Currently unused (recovery is handled locally at each call site
    /// instead), kept as a utility for future, coarser recovery needs.
    #[allow(dead_code)]
    fn synchronize(&mut self) {
        while !self.is_eof() {
            match self.peek_kind() {
                TokenKind::Semicolon => {
                    self.advance();
                    return;
                }
                TokenKind::Let
                | TokenKind::Const
                | TokenKind::Fn
                | TokenKind::If
                | TokenKind::While
                | TokenKind::CloseCurly => return,
                _ => {
                    self.advance();
                }
            }
        }
    }

    // ---- statements ------------------------------------------------------

    fn parse_stmt(&mut self) -> Stmt {
        match self.peek_kind() {
            TokenKind::Let | TokenKind::Const => self.parse_var_decl(),
            TokenKind::Fn => self.parse_fn_decl(),
            TokenKind::If => self.parse_if_stmt(),
            TokenKind::While => self.parse_while_stmt(),
            _ => self.parse_expr_stmt(),
        }
    }

    fn parse_block(&mut self) -> (Vec<Stmt>, Span) {
        let open = self.expect(TokenKind::OpenCurly, "to start a block");
        let start = open.as_ref().map(|t| t.span.start).unwrap_or(self.cur().span.start);
        let mut body = Vec::new();
        while !self.is_eof() && !self.check(&TokenKind::CloseCurly) {
            let before = self.pos;
            body.push(self.parse_stmt());
            if self.pos == before {
                self.advance();
            }
        }
        let close = self.expect(TokenKind::CloseCurly, "to close block");
        let end = close.map(|t| t.span.end).unwrap_or(self.cur().span.end);
        (body, Span::new(start, end))
    }

    fn parse_var_decl(&mut self) -> Stmt {
        let kw = self.advance(); // 'let' or 'const'
        let constant = matches!(kw.kind, TokenKind::Const);
        let start = kw.span.start;

        let ident_tok = self.expect(
            TokenKind::Identifier(String::new()),
            "as the declared variable name",
        );
        let (identifier, identifier_span) = match ident_tok {
            Some(t) => {
                if let TokenKind::Identifier(name) = t.kind {
                    (name, t.span)
                } else {
                    (String::new(), t.span)
                }
            }
            None => (String::new(), kw.span),
        };

        let mut value = None;
        if self.matches(&TokenKind::Equals) {
            value = Some(self.parse_expression());
        } else if constant {
            self.diags.push(
                Diag::error(
                    identifier_span,
                    "'const' declaration requires an initializer (e.g. `const x = 1;`)",
                )
                .with_code("const-no-init"),
            );
        }

        let has_semicolon = self.check(&TokenKind::Semicolon);
        let end = if has_semicolon {
            self.advance().span.end
        } else {
            let tok = self.cur().clone();
            self.diags.push(
                Diag::error(
                    tok.span,
                    format!(
                        "expected ';' after variable declaration, found {}",
                        tok.kind
                    ),
                )
                .with_code("missing-semicolon"),
            );
            tok.span.start
        };

        Stmt::VarDecl(VarDecl {
            constant,
            identifier,
            identifier_span,
            value,
            span: Span::new(start, end),
            has_semicolon,
        })
    }

    fn parse_fn_decl(&mut self) -> Stmt {
        let kw = self.advance(); // 'fn'
        let start = kw.span.start;

        let name_tok = self.expect(TokenKind::Identifier(String::new()), "as the function name");
        let (name, name_span) = match name_tok {
            Some(t) => {
                if let TokenKind::Identifier(n) = t.kind {
                    (n, t.span)
                } else {
                    (String::new(), t.span)
                }
            }
            None => (String::new(), kw.span),
        };

        self.expect(TokenKind::OpenParen, "to start the parameter list");
        let mut parameters = Vec::new();
        if !self.check(&TokenKind::CloseParen) {
            loop {
                let p = self.expect(TokenKind::Identifier(String::new()), "as a parameter name");
                if let Some(t) = p {
                    if let TokenKind::Identifier(n) = t.kind {
                        parameters.push((n, t.span));
                    }
                }
                if !self.matches(&TokenKind::Comma) {
                    break;
                }
            }
        }
        self.expect(TokenKind::CloseParen, "to close the parameter list");

        let (body, body_span) = self.parse_block();
        let end = body_span.end;

        Stmt::FunctionDecl(FunctionDecl {
            name,
            name_span,
            parameters,
            body,
            span: Span::new(start, end),
            body_span,
        })
    }

    fn parse_if_stmt(&mut self) -> Stmt {
        let kw = self.advance(); // 'if'
        let start = kw.span.start;
        let condition = self.parse_expression();
        let (then_branch, then_span) = self.parse_block();
        let mut end = then_span.end;

        let mut else_branch = None;
        let mut else_span = None;
        if self.matches(&TokenKind::Else) {
            if self.check(&TokenKind::If) {
                self.diags.push(
                    Diag::error(
                        self.cur().span,
                        "'else if' is not valid Fling syntax -- the grammar only allows `else { ... }`. \
                         Write `else { if ... { ... } }` instead (the reference parser expects '{' right after 'else').",
                    )
                    .with_code("no-else-if"),
                );
                let inner = self.parse_if_stmt();
                let span = inner.span();
                else_branch = Some(vec![inner]);
                else_span = Some(span);
                end = span.end;
            } else {
                let (body, span) = self.parse_block();
                end = span.end;
                else_branch = Some(body);
                else_span = Some(span);
            }
        }

        Stmt::If(IfStmt {
            condition,
            then_branch,
            then_span,
            else_branch,
            else_span,
            span: Span::new(start, end),
        })
    }

    fn parse_while_stmt(&mut self) -> Stmt {
        let kw = self.advance(); // 'while'
        let start = kw.span.start;
        let condition = self.parse_expression();
        let (body, body_span) = self.parse_block();
        Stmt::While(WhileStmt {
            condition,
            body,
            body_span,
            span: Span::new(start, body_span.end),
        })
    }

    fn parse_expr_stmt(&mut self) -> Stmt {
        let start_tok = self.cur().clone();
        if matches!(start_tok.kind, TokenKind::Semicolon) {
            // A lone ';' with nothing before it: same "superfluous semicolon"
            // family of bug (B14) -- the reference parser chokes on it.
            self.diags.push(
                Diag::warning(
                    start_tok.span,
                    "unexpected ';' -- Fling only expects ';' after 'let'/'const' declarations",
                )
                .with_code("B14"),
            );
            self.advance();
            return Stmt::Error(start_tok.span);
        }
        let expr = self.parse_expression();
        let mut span = expr.span();
        let mut trailing_semicolon = None;
        if self.check(&TokenKind::Semicolon) {
            let tok = self.advance();
            self.diags.push(
                Diag::warning(
                    tok.span,
                    "superfluous ';' after expression statement -- the reference interpreter does \
                     not consume this token and treats it as a syntax error on the next statement (bug B14); remove it",
                )
                .with_code("B14"),
            );
            trailing_semicolon = Some(tok.span);
            span = Span::new(span.start, tok.span.end);
        }
        Stmt::ExprStmt(ExprStmt {
            expr,
            span,
            trailing_semicolon,
        })
    }

    // ---- expressions (precedence climbing, spec 3.4) --------------------

    fn parse_expression(&mut self) -> Expr {
        self.parse_assignment()
    }

    fn parse_assignment(&mut self) -> Expr {
        let left = self.parse_logical();
        if self.check(&TokenKind::Equals) {
            self.advance();
            let value = self.parse_assignment(); // right-associative
            let span = Span::new(left.span().start, value.span().end);
            return Expr::Assignment {
                target: Box::new(left),
                value: Box::new(value),
                span,
            };
        }
        left
    }

    fn parse_logical(&mut self) -> Expr {
        let mut left = self.parse_comparison();
        loop {
            let op = match self.peek_kind() {
                TokenKind::AndAnd => LogicalOp::And,
                TokenKind::OrOr => LogicalOp::Or,
                _ => break,
            };
            let op_span = self.advance().span;
            let right = self.parse_comparison();
            let span = Span::new(left.span().start, right.span().end);
            left = Expr::Logical {
                left: Box::new(left),
                op,
                op_span,
                right: Box::new(right),
                span,
            };
        }
        left
    }

    fn parse_comparison(&mut self) -> Expr {
        let mut left = self.parse_additive();
        loop {
            let op = match self.peek_kind() {
                TokenKind::EqEq => BinaryOp::EqEq,
                TokenKind::NotEq => BinaryOp::NotEq,
                TokenKind::Lt => BinaryOp::Lt,
                TokenKind::Gt => BinaryOp::Gt,
                TokenKind::Le => BinaryOp::Le,
                TokenKind::Ge => BinaryOp::Ge,
                _ => break,
            };
            let op_span = self.advance().span;
            let right = self.parse_additive();
            let span = Span::new(left.span().start, right.span().end);
            left = Expr::Binary {
                left: Box::new(left),
                op,
                op_span,
                right: Box::new(right),
                span,
            };
        }
        left
    }

    fn parse_additive(&mut self) -> Expr {
        let mut left = self.parse_multiplicative();
        loop {
            let op = match self.peek_kind() {
                TokenKind::Plus => BinaryOp::Add,
                TokenKind::Minus => BinaryOp::Sub,
                _ => break,
            };
            let op_span = self.advance().span;
            let right = self.parse_multiplicative();
            let span = Span::new(left.span().start, right.span().end);
            left = Expr::Binary {
                left: Box::new(left),
                op,
                op_span,
                right: Box::new(right),
                span,
            };
        }
        left
    }

    fn parse_multiplicative(&mut self) -> Expr {
        let mut left = self.parse_unary();
        loop {
            let op = match self.peek_kind() {
                TokenKind::Star => BinaryOp::Mul,
                TokenKind::Slash => BinaryOp::Div,
                TokenKind::Percent => BinaryOp::Mod,
                _ => break,
            };
            let op_span = self.advance().span;
            let right = self.parse_unary();
            let span = Span::new(left.span().start, right.span().end);
            left = Expr::Binary {
                left: Box::new(left),
                op,
                op_span,
                right: Box::new(right),
                span,
            };
        }
        left
    }

    fn parse_unary(&mut self) -> Expr {
        let op = match self.peek_kind() {
            TokenKind::Bang => Some(UnaryOp::Not),
            TokenKind::Minus => Some(UnaryOp::Neg),
            _ => None,
        };
        if let Some(op) = op {
            let op_span = self.advance().span;
            let operand = self.parse_unary();
            let span = Span::new(op_span.start, operand.span().end);
            return Expr::Unary {
                op,
                op_span,
                operand: Box::new(operand),
                span,
            };
        }
        self.parse_call_member()
    }

    fn parse_call_member(&mut self) -> Expr {
        let mut expr = self.parse_member();
        if self.check(&TokenKind::OpenParen) {
            // First call, then keep wrapping further immediate `(...)`
            // chains (currying: f()()), but NOT further `.`/`[...]` access
            // -- that's the "no member access after call" limitation
            // (spec 3.5, bug B13).
            while self.check(&TokenKind::OpenParen) {
                let (args, args_end) = self.parse_args();
                let call_span = Span::new(expr.span().start, args_end);
                expr = Expr::Call {
                    callee: Box::new(expr),
                    args,
                    span: call_span,
                };
            }
            // If a '.' or '[' follows a call, that's unsupported (B13): flag
            // it, but still consume it into the AST (as a Member node) so a
            // single mistake doesn't cascade into a second, confusing
            // "unexpected token" error on the next statement.
            if self.check(&TokenKind::Dot) || self.check(&TokenKind::OpenSquare) {
                let tok = self.cur().clone();
                self.diags.push(
                    Diag::error(
                        tok.span,
                        "member/index access after a function call is not supported by Fling \
                         (e.g. `foo().bar` or `foo()[0]`); only member access *before* the call \
                         works, such as `obj.method()` (bug B13)",
                    )
                    .with_code("B13"),
                );
                expr = self.parse_member_chain(expr);
            }
        }
        expr
    }

    fn parse_args(&mut self) -> (Vec<Expr>, Pos) {
        self.expect(TokenKind::OpenParen, "to start argument list");
        let mut args = Vec::new();
        if !self.check(&TokenKind::CloseParen) {
            loop {
                args.push(self.parse_assignment());
                if !self.matches(&TokenKind::Comma) {
                    break;
                }
            }
        }
        let close = self.expect(TokenKind::CloseParen, "to close argument list");
        let end = close.map(|t| t.span.end).unwrap_or(self.cur().span.end);
        (args, end)
    }

    fn parse_member(&mut self) -> Expr {
        let expr = self.parse_primary();
        self.parse_member_chain(expr)
    }

    fn parse_member_chain(&mut self, mut expr: Expr) -> Expr {
        loop {
            if self.check(&TokenKind::Dot) {
                let dot_span = self.advance().span;
                if let TokenKind::Number(_) = self.peek_kind() {
                    // `3.14`-shaped input: the lexer tokenizes this as
                    // Number Dot Number with no float support (bug B2).
                    let ntok = self.cur().clone();
                    self.diags.push(
                        Diag::error(
                            Span::new(dot_span.start, ntok.span.end),
                            "decimal-point number literals are not supported: the lexer tokenizes \
                             this as separate integer tokens split by '.', and the parser then \
                             expects an identifier (for member access) after '.', not a number (bug B2)",
                        )
                        .with_code("B2"),
                    );
                    self.advance();
                    let full_span = Span::new(expr.span().start, ntok.span.end);
                    expr = Expr::Error(full_span);
                    continue;
                }
                let prop_tok = self.expect(
                    TokenKind::Identifier(String::new()),
                    "after '.' (member access only supports identifiers)",
                );
                let (name, span) = match prop_tok {
                    Some(t) => {
                        if let TokenKind::Identifier(n) = t.kind {
                            (n, t.span)
                        } else {
                            (String::new(), t.span)
                        }
                    }
                    None => (String::new(), self.cur().span),
                };
                let property = Expr::Identifier { name, span };
                let full_span = Span::new(expr.span().start, span.end);
                expr = Expr::Member {
                    object: Box::new(expr),
                    property: Box::new(property),
                    computed: false,
                    span: full_span,
                };
            } else if self.check(&TokenKind::OpenSquare) {
                self.advance();
                let index = self.parse_expression();
                let close = self.expect(TokenKind::CloseSquare, "to close computed member access");
                let end = close.map(|t| t.span.end).unwrap_or(index.span().end);
                let full_span = Span::new(expr.span().start, end);
                expr = Expr::Member {
                    object: Box::new(expr),
                    property: Box::new(index),
                    computed: true,
                    span: full_span,
                };
            } else {
                break;
            }
        }
        expr
    }

    fn parse_primary(&mut self) -> Expr {
        let tok = self.cur().clone();
        match &tok.kind {
            TokenKind::Identifier(name) => {
                self.advance();
                Expr::Identifier {
                    name: name.clone(),
                    span: tok.span,
                }
            }
            TokenKind::Number(n) => {
                self.advance();
                Expr::NumericLiteral {
                    value: n.clone(),
                    span: tok.span,
                }
            }
            TokenKind::String(s) => {
                self.advance();
                Expr::StringLiteral {
                    value: s.clone(),
                    span: tok.span,
                }
            }
            TokenKind::OpenParen => {
                self.advance();
                let expr = self.parse_expression();
                self.expect(TokenKind::CloseParen, "to close parenthesized expression");
                expr
            }
            TokenKind::OpenSquare => self.parse_array_literal(),
            TokenKind::OpenCurly => self.parse_object_literal(),
            _ => {
                self.diags.push(
                    Diag::error(tok.span, format!("unexpected {} in expression", tok.kind))
                        .with_code("syntax"),
                );
                if !self.is_eof() {
                    self.advance();
                }
                Expr::Error(tok.span)
            }
        }
    }

    fn parse_array_literal(&mut self) -> Expr {
        let open = self.advance(); // '['
        let start = open.span.start;
        let mut elements = Vec::new();
        while !self.is_eof() && !self.check(&TokenKind::CloseSquare) {
            elements.push(self.parse_expression());
            if self.check(&TokenKind::Comma) {
                self.advance();
            } else {
                break;
            }
        }
        let close = self.expect(TokenKind::CloseSquare, "to close array literal");
        let end = close.map(|t| t.span.end).unwrap_or(self.cur().span.end);
        Expr::ArrayLiteral {
            elements,
            span: Span::new(start, end),
        }
    }

    fn parse_object_literal(&mut self) -> Expr {
        let open = self.advance(); // '{'
        let start = open.span.start;
        let mut properties = Vec::new();
        // Reference parser tolerates leading/extra commas.
        while self.check(&TokenKind::Comma) {
            self.advance();
        }
        while !self.is_eof() && !self.check(&TokenKind::CloseCurly) {
            let key_tok = self.expect(TokenKind::Identifier(String::new()), "as an object property key");
            let (key, key_span) = match key_tok {
                Some(t) => {
                    if let TokenKind::Identifier(n) = t.kind {
                        (n, t.span)
                    } else {
                        (String::new(), t.span)
                    }
                }
                None => break,
            };
            let mut value = None;
            let mut end = key_span.end;
            if self.matches(&TokenKind::Colon) {
                let v = self.parse_expression();
                end = v.span().end;
                value = Some(v);
            }
            properties.push(Property {
                key,
                key_span,
                value,
                span: Span::new(key_span.start, end),
            });
            if self.check(&TokenKind::Comma) {
                self.advance();
                while self.check(&TokenKind::Comma) {
                    self.advance();
                }
            } else {
                break;
            }
        }
        let close = self.expect(TokenKind::CloseCurly, "to close object literal");
        let end = close.map(|t| t.span.end).unwrap_or(self.cur().span.end);
        Expr::ObjectLiteral {
            properties,
            span: Span::new(start, end),
        }
    }
}

/// Convenience wrapper: tokenize + parse in one go is done by the caller
/// (main/server) so lexer warnings can be merged in too. This just exposes
/// the entry point.
pub fn parse(tokens: &[Token]) -> (Program, Vec<Diag>) {
    let p = Parser::new(tokens);
    let (prog, diags) = p.parse_program();
    (prog, diags)
}
