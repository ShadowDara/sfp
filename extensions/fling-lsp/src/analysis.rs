//! Static semantic analysis for Fling (spec section 11.1-11.4).
//!
//! Builds a scope/symbol table that mirrors the reference interpreter's
//! `Environment` model (spec 6.2): every `while` loop body and every
//! function call gets its own scope, but `if`/`else` blocks do **not** get
//! their own scope (bug B8) -- declarations inside them land directly in
//! the enclosing scope, which this analyzer reproduces so it doesn't flag
//! false positives (or miss real ones).
//!
//! Since the reference interpreter never raises real exceptions (spec
//! section 8), this module is the *only* source of "hard" diagnostics for
//! the things that would otherwise silently evaluate to `Null`.

use std::collections::HashMap;

use crate::ast::*;
use crate::diagnostics::Diag;
use crate::lexer::Span;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SymbolKind {
    Let,
    Const,
    Param,
    Function,
    /// Built-in global (`true`, `false`, `null`, `print`).
    Global,
}

impl SymbolKind {
    pub fn label(&self) -> &'static str {
        match self {
            SymbolKind::Let => "let variable",
            SymbolKind::Const => "const variable",
            SymbolKind::Param => "function parameter",
            SymbolKind::Function => "function",
            SymbolKind::Global => "built-in global",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SimpleType {
    Number,
    String,
    Boolean,
    Null,
    Array,
    Object,
    Function,
    Unknown,
}

impl SimpleType {
    pub fn label(&self) -> &'static str {
        match self {
            SimpleType::Number => "Number",
            SimpleType::String => "String",
            SimpleType::Boolean => "Boolean",
            SimpleType::Null => "Null",
            SimpleType::Array => "Array",
            SimpleType::Object => "Object",
            SimpleType::Function => "Function",
            SimpleType::Unknown => "unknown",
        }
    }
}

pub type SymbolId = usize;
pub type ScopeId = usize;

#[derive(Debug, Clone)]
pub struct Symbol {
    pub id: SymbolId,
    pub name: String,
    pub kind: SymbolKind,
    pub decl_span: Span,
    pub scope: ScopeId,
    pub simple_type: SimpleType,
    pub object_keys: Option<Vec<String>>,
    pub params: Option<Vec<String>>,
    pub fn_body_span: Option<Span>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ScopeKind {
    Global,
    Function,
    While,
}

#[derive(Debug, Clone)]
pub struct Scope {
    pub parent: Option<ScopeId>,
    pub kind: ScopeKind,
    pub symbols: HashMap<String, SymbolId>,
}

#[derive(Debug, Clone)]
pub struct Use {
    pub span: Span,
    pub symbol: Option<SymbolId>,
    pub is_write: bool,
    pub is_decl: bool,
}

#[derive(Debug, Clone)]
pub struct OpHover {
    pub span: Span,
    pub text: String,
}

pub struct Analysis {
    pub symbols: Vec<Symbol>,
    pub scopes: Vec<Scope>,
    pub diags: Vec<Diag>,
    pub uses: Vec<Use>,
    /// (span, scope) pairs for every block-like container, used to find the
    /// innermost scope at a given cursor position (for completion).
    pub containers: Vec<(Span, ScopeId)>,
    pub op_hovers: Vec<OpHover>,
}

pub fn analyze(program: &Program, file_span: Span) -> Analysis {
    let mut a = Analyzer {
        symbols: Vec::new(),
        scopes: Vec::new(),
        diags: Vec::new(),
        uses: Vec::new(),
        containers: Vec::new(),
        op_hovers: Vec::new(),
    };

    let global = a.new_scope(None, ScopeKind::Global);
    a.predeclare_global(global);

    a.declare_block(&program.body, global);
    a.containers.push((file_span, global));
    a.analyze_block(&program.body, global);

    Analysis {
        symbols: a.symbols,
        scopes: a.scopes,
        diags: a.diags,
        uses: a.uses,
        containers: a.containers,
        op_hovers: a.op_hovers,
    }
}

struct Analyzer {
    symbols: Vec<Symbol>,
    scopes: Vec<Scope>,
    diags: Vec<Diag>,
    uses: Vec<Use>,
    containers: Vec<(Span, ScopeId)>,
    op_hovers: Vec<OpHover>,
}

impl Analyzer {
    fn new_scope(&mut self, parent: Option<ScopeId>, kind: ScopeKind) -> ScopeId {
        self.scopes.push(Scope {
            parent,
            kind,
            symbols: HashMap::new(),
        });
        self.scopes.len() - 1
    }

    fn add_symbol(
        &mut self,
        name: String,
        kind: SymbolKind,
        decl_span: Span,
        scope: ScopeId,
    ) -> SymbolId {
        let id = self.symbols.len();
        self.symbols.push(Symbol {
            id,
            name,
            kind,
            decl_span,
            scope,
            simple_type: SimpleType::Unknown,
            object_keys: None,
            params: None,
            fn_body_span: None,
        });
        id
    }

    fn predeclare_global(&mut self, scope: ScopeId) {
        let zero = Span::default();
        for (name, kind, ty) in [
            ("true", SymbolKind::Global, SimpleType::Boolean),
            ("false", SymbolKind::Global, SimpleType::Boolean),
            ("null", SymbolKind::Global, SimpleType::Null),
            ("print", SymbolKind::Global, SimpleType::Function),
        ] {
            let id = self.add_symbol(name.to_string(), kind, zero, scope);
            self.symbols[id].simple_type = ty;
            self.scopes[scope].symbols.insert(name.to_string(), id);
        }
    }

    fn resolve(&self, name: &str, scope: ScopeId) -> Option<SymbolId> {
        let mut cur = Some(scope);
        while let Some(s) = cur {
            if let Some(id) = self.scopes[s].symbols.get(name) {
                return Some(*id);
            }
            cur = self.scopes[s].parent;
        }
        None
    }

    // ---- declaration collection (crosses if/else, stops at while/fn) ----

    fn declare_block(&mut self, stmts: &[Stmt], scope: ScopeId) {
        for stmt in stmts {
            match stmt {
                Stmt::VarDecl(d) => self.declare_var(d, scope),
                Stmt::FunctionDecl(d) => self.declare_fn(d, scope),
                Stmt::If(s) => {
                    self.declare_block(&s.then_branch, scope);
                    if let Some(else_b) = &s.else_branch {
                        self.declare_block(else_b, scope);
                    }
                }
                Stmt::While(_) | Stmt::ExprStmt(_) | Stmt::Error(_) => {}
            }
        }
    }

    fn declare_var(&mut self, d: &VarDecl, scope: ScopeId) {
        if d.identifier.is_empty() {
            return; // already reported as a parse error
        }
        if self.scopes[scope].symbols.contains_key(&d.identifier) {
            self.diags.push(
                Diag::error(
                    d.identifier_span,
                    format!(
                        "cannot redeclare variable '{}' -- it already exists in this scope \
                         (the reference interpreter refuses the redeclaration and silently \
                         evaluates it to Null instead of erroring)",
                        d.identifier
                    ),
                )
                .with_code("redeclare"),
            );
            return;
        }
        let kind = if d.constant {
            SymbolKind::Const
        } else {
            SymbolKind::Let
        };
        let id = self.add_symbol(d.identifier.clone(), kind, d.identifier_span, scope);
        self.scopes[scope].symbols.insert(d.identifier.clone(), id);
        self.uses.push(Use {
            span: d.identifier_span,
            symbol: Some(id),
            is_write: true,
            is_decl: true,
        });
    }

    fn declare_fn(&mut self, d: &FunctionDecl, scope: ScopeId) {
        if d.name.is_empty() {
            return;
        }
        if self.scopes[scope].symbols.contains_key(&d.name) {
            self.diags.push(
                Diag::error(
                    d.name_span,
                    format!(
                        "cannot redeclare '{}' in this scope -- a variable or function with \
                         that name already exists here",
                        d.name
                    ),
                )
                .with_code("redeclare"),
            );
            return;
        }
        let id = self.add_symbol(d.name.clone(), SymbolKind::Function, d.name_span, scope);
        self.symbols[id].simple_type = SimpleType::Function;
        self.scopes[scope].symbols.insert(d.name.clone(), id);
        self.uses.push(Use {
            span: d.name_span,
            symbol: Some(id),
            is_write: true,
            is_decl: true,
        });
    }

    // ---- analysis / use-resolution pass ---------------------------------

    fn analyze_block(&mut self, stmts: &[Stmt], scope: ScopeId) {
        for stmt in stmts {
            match stmt {
                Stmt::VarDecl(d) => {
                    if let Some(value) = &d.value {
                        self.analyze_expr(value, scope);
                        if !d.identifier.is_empty() {
                            if let Some(&id) = self.scopes[scope].symbols.get(&d.identifier) {
                                let ty = self.infer_type(value, scope);
                                self.symbols[id].simple_type = ty;
                                if let Expr::ObjectLiteral { properties, .. } = value {
                                    self.symbols[id].object_keys = Some(
                                        properties.iter().map(|p| p.key.clone()).collect(),
                                    );
                                }
                            }
                        }
                    }
                }
                Stmt::FunctionDecl(d) => {
                    let sym_id = self.scopes[scope].symbols.get(&d.name).copied();
                    let child = self.new_scope(Some(scope), ScopeKind::Function);
                    for (pname, pspan) in &d.parameters {
                        if pname.is_empty() {
                            continue;
                        }
                        if self.scopes[child].symbols.contains_key(pname) {
                            self.diags.push(
                                Diag::error(
                                    *pspan,
                                    format!("duplicate parameter name '{pname}'"),
                                )
                                .with_code("redeclare"),
                            );
                            continue;
                        }
                        let pid = self.add_symbol(pname.clone(), SymbolKind::Param, *pspan, child);
                        self.scopes[child].symbols.insert(pname.clone(), pid);
                        self.uses.push(Use {
                            span: *pspan,
                            symbol: Some(pid),
                            is_write: true,
                            is_decl: true,
                        });
                    }
                    self.declare_block(&d.body, child);
                    self.containers.push((d.body_span, child));
                    self.analyze_block(&d.body, child);
                    if let Some(id) = sym_id {
                        self.symbols[id].params =
                            Some(d.parameters.iter().map(|(n, _)| n.clone()).collect());
                        self.symbols[id].fn_body_span = Some(d.body_span);
                    }
                }
                Stmt::If(s) => {
                    self.analyze_expr(&s.condition, scope);
                    self.containers.push((s.then_span, scope));
                    self.analyze_block(&s.then_branch, scope);
                    if let Some(else_b) = &s.else_branch {
                        if let Some(espan) = s.else_span {
                            self.containers.push((espan, scope));
                        }
                        self.analyze_block(else_b, scope);
                    }
                }
                Stmt::While(s) => {
                    self.analyze_expr(&s.condition, scope);
                    let child = self.new_scope(Some(scope), ScopeKind::While);
                    self.declare_block(&s.body, child);
                    self.containers.push((s.body_span, child));
                    self.analyze_block(&s.body, child);
                }
                Stmt::ExprStmt(s) => {
                    self.analyze_expr(&s.expr, scope);
                }
                Stmt::Error(_) => {}
            }
        }
    }

    fn analyze_expr(&mut self, expr: &Expr, scope: ScopeId) {
        match expr {
            Expr::Identifier { name, span } => {
                let sym = self.resolve(name, scope);
                self.uses.push(Use {
                    span: *span,
                    symbol: sym,
                    is_write: false,
                    is_decl: false,
                });
                if sym.is_none() {
                    self.diags.push(
                        Diag::error(
                            *span,
                            format!(
                                "undeclared variable or function '{name}' -- the reference \
                                 interpreter does not throw here either, it just prints \
                                 \"Variable not found\" and evaluates this to Null"
                            ),
                        )
                        .with_code("undeclared"),
                    );
                }
            }
            Expr::NumericLiteral { .. } | Expr::StringLiteral { .. } | Expr::Error(_) => {}
            Expr::ArrayLiteral { elements, .. } => {
                for e in elements {
                    self.analyze_expr(e, scope);
                }
            }
            Expr::ObjectLiteral { properties, .. } => {
                for p in properties {
                    match &p.value {
                        Some(v) => self.analyze_expr(v, scope),
                        None => {
                            // Shorthand `{ key }` resolves `key` as a
                            // variable *at runtime*, not parse time (spec 5).
                            let sym = self.resolve(&p.key, scope);
                            self.uses.push(Use {
                                span: p.key_span,
                                symbol: sym,
                                is_write: false,
                                is_decl: false,
                            });
                            if sym.is_none() {
                                self.diags.push(
                                    Diag::error(
                                        p.key_span,
                                        format!(
                                            "undeclared variable '{}' used as shorthand object \
                                             property -- `{{ {} }}` needs a variable named '{}' \
                                             in scope",
                                            p.key, p.key, p.key
                                        ),
                                    )
                                    .with_code("undeclared"),
                                );
                            }
                        }
                    }
                }
            }
            Expr::Unary { operand, .. } => self.analyze_expr(operand, scope),
            Expr::Logical { left, right, .. } => {
                self.analyze_expr(left, scope);
                self.analyze_expr(right, scope);
            }
            Expr::Binary {
                left,
                op,
                op_span,
                right,
                ..
            } => {
                self.analyze_expr(left, scope);
                self.analyze_expr(right, scope);

                let hover_text = match op {
                    BinaryOp::Mod => Some(
                        "**`%` (bug B1):** the reference interpreter swaps its operands -- \
                         `a % b` actually computes `b % a`. `5 % 2` evaluates to `2`, not `1`."
                            .to_string(),
                    ),
                    BinaryOp::Div => Some(
                        "**`/`:** division by zero returns `0` in the reference interpreter \
                         instead of raising an error or producing `Infinity`/`NaN` (bug B12)."
                            .to_string(),
                    ),
                    BinaryOp::Add => Some(
                        "**`+`:** only defined for `Number + Number`. There is **no** string \
                         concatenation -- `\"a\" + \"b\"` silently evaluates to `Null` (bug B3)."
                            .to_string(),
                    ),
                    BinaryOp::EqEq | BinaryOp::NotEq => Some(
                        "**`==`/`!=`:** type-safe -- values of different runtime types are \
                         never equal. `Object`, `Array`, and function values have no defined \
                         equality and always compare as `false` (`!=` as `true`)."
                            .to_string(),
                    ),
                    BinaryOp::Lt | BinaryOp::Gt | BinaryOp::Le | BinaryOp::Ge => Some(
                        "**Relational operator:** works directly on the numeric value with no \
                         type check -- comparing non-numbers (e.g. strings) silently compares \
                         as `0 < 0` instead of erroring."
                            .to_string(),
                    ),
                    _ => None,
                };
                if let Some(text) = hover_text {
                    self.op_hovers.push(OpHover {
                        span: *op_span,
                        text,
                    });
                }

                if *op == BinaryOp::Add {
                    let lt = self.infer_type(left, scope);
                    let rt = self.infer_type(right, scope);
                    if lt == SimpleType::String && rt == SimpleType::String {
                        self.diags.push(
                            Diag::warning(
                                expr.span(),
                                "'+' does not concatenate strings in Fling -- this expression \
                                 evaluates to Null at runtime (bug B3)",
                            )
                            .with_code("B3"),
                        );
                    }
                }
            }
            Expr::Assignment { target, value, .. } => {
                self.analyze_expr(value, scope);
                match target.as_ref() {
                    Expr::Identifier { name, span } => {
                        let sym = self.resolve(name, scope);
                        self.uses.push(Use {
                            span: *span,
                            symbol: sym,
                            is_write: true,
                            is_decl: false,
                        });
                        match sym {
                            None => {
                                self.diags.push(
                                    Diag::error(
                                        *span,
                                        format!("assignment to undeclared variable '{name}'"),
                                    )
                                    .with_code("undeclared"),
                                );
                            }
                            Some(id) => {
                                if self.symbols[id].kind == SymbolKind::Const {
                                    self.diags.push(
                                        Diag::error(
                                            *span,
                                            format!(
                                                "cannot assign to const variable '{name}' -- the \
                                                 reference interpreter refuses the assignment and \
                                                 the whole expression evaluates to Null instead"
                                            ),
                                        )
                                        .with_code("assign-const"),
                                    );
                                }
                            }
                        }
                    }
                    other => {
                        self.diags.push(
                            Diag::error(
                                other.span(),
                                "assignment target must be a simple variable name -- assigning to \
                                 a member/index expression (`obj.x = ...`, `arr[0] = ...`) is \
                                 syntactically accepted but is undefined behavior in the reference \
                                 interpreter, which blindly casts the target to a plain identifier \
                                 (bug B6)",
                            )
                            .with_code("B6"),
                        );
                        self.analyze_expr(other, scope);
                    }
                }
            }
            Expr::Call { callee, args, span } => {
                self.analyze_expr(callee, scope);
                for a in args {
                    self.analyze_expr(a, scope);
                }
                if let Expr::Identifier { name, .. } = callee.as_ref() {
                    if let Some(id) = self.resolve(name, scope) {
                        if let Some(params) = self.symbols[id].params.clone() {
                            if params.len() != args.len() {
                                self.diags.push(
                                    Diag::warning(
                                        *span,
                                        format!(
                                            "'{}' expects {} argument{}, but {} {} passed -- \
                                             Fling does not check arity at runtime, so missing \
                                             arguments become Null and extra ones are silently \
                                             ignored",
                                            name,
                                            params.len(),
                                            if params.len() == 1 { "" } else { "s" },
                                            args.len(),
                                            if args.len() == 1 { "is" } else { "are" }
                                        ),
                                    )
                                    .with_code("arity"),
                                );
                            }
                        }
                    }
                }
            }
            Expr::Member {
                object,
                property,
                computed,
                span,
            } => {
                self.analyze_expr(object, scope);
                let obj_ty = self.infer_type(object, scope);
                if *computed {
                    self.analyze_expr(property, scope);
                    match obj_ty {
                        SimpleType::Object => {
                            self.diags.push(
                                Diag::warning(
                                    *span,
                                    "computed object property access (`obj[key]`) is broken in \
                                     the reference interpreter -- the key gets turned into its \
                                     full debug string instead of the plain value, so this will \
                                     essentially always evaluate to Null. Use `obj.key` instead \
                                     (bug B4)",
                                )
                                .with_code("B4"),
                            );
                        }
                        SimpleType::String => {
                            self.diags.push(
                                Diag::error(
                                    *span,
                                    "indexing into a string (`str[i]`) is undefined behavior in \
                                     the reference interpreter (it wrongly casts the index to an \
                                     identifier pointer). Only `.length` is supported on strings \
                                     (bug B5)",
                                )
                                .with_code("B5"),
                            );
                        }
                        _ => {}
                    }
                } else if let Expr::Identifier { name, .. } = property.as_ref() {
                    if matches!(obj_ty, SimpleType::Array | SimpleType::String) && name != "length"
                    {
                        self.diags.push(
                            Diag::info(
                                *span,
                                format!(
                                    "only `.length` is supported on {}s in Fling -- `.{}` will \
                                     evaluate to Null",
                                    obj_ty.label().to_lowercase(),
                                    name
                                ),
                            )
                            .with_code("member-unsupported"),
                        );
                    }
                }
            }
        }
    }

    /// Very small best-effort static type inference: only looks at literal
    /// syntax and previously-inferred variable types, no control-flow or
    /// reassignment tracking. Good enough for the B3/B4/B5 heuristics.
    fn infer_type(&self, expr: &Expr, scope: ScopeId) -> SimpleType {
        match expr {
            Expr::NumericLiteral { .. } => SimpleType::Number,
            Expr::StringLiteral { .. } => SimpleType::String,
            Expr::ArrayLiteral { .. } => SimpleType::Array,
            Expr::ObjectLiteral { .. } => SimpleType::Object,
            Expr::Unary {
                op: UnaryOp::Not, ..
            } => SimpleType::Boolean,
            Expr::Unary {
                op: UnaryOp::Neg, ..
            } => SimpleType::Number,
            Expr::Logical { .. } => SimpleType::Boolean,
            Expr::Binary { op, .. } => match op {
                BinaryOp::EqEq
                | BinaryOp::NotEq
                | BinaryOp::Lt
                | BinaryOp::Gt
                | BinaryOp::Le
                | BinaryOp::Ge => SimpleType::Boolean,
                _ => SimpleType::Unknown,
            },
            Expr::Identifier { name, .. } => self
                .resolve(name, scope)
                .map(|id| self.symbols[id].simple_type)
                .unwrap_or(SimpleType::Unknown),
            _ => SimpleType::Unknown,
        }
    }
}

/// Finds the innermost container span that contains `pos`, returning its
/// scope id. Falls back to `default_scope` (usually the global scope) if
/// nothing matches.
pub fn scope_at(containers: &[(Span, ScopeId)], pos: crate::lexer::Pos, default_scope: ScopeId) -> ScopeId {
    let mut best: Option<(Span, ScopeId)> = None;
    for &(span, scope) in containers {
        if contains(span, pos) {
            let smaller = match best {
                None => true,
                Some((b, _)) => size(span) <= size(b),
            };
            if smaller {
                best = Some((span, scope));
            }
        }
    }
    best.map(|(_, s)| s).unwrap_or(default_scope)
}

fn contains(span: Span, pos: crate::lexer::Pos) -> bool {
    let after_start = (pos.line, pos.col) >= (span.start.line, span.start.col);
    let before_end = (pos.line, pos.col) <= (span.end.line, span.end.col);
    after_start && before_end
}

fn size(span: Span) -> (u32, u32) {
    let lines = span.end.line.saturating_sub(span.start.line);
    (lines, span.end.col.saturating_sub(span.start.col))
}
