//! AST node definitions (spec section 4). Field names are the "cleaned up"
//! equivalents of the reference C++ implementation's typo'd names
//! (`FunctionDeckaration` -> `FunctionDecl`, `agrs` -> `args`,
//! `callculation_operator` -> `op`), as recommended by the spec.

use crate::lexer::Span;

#[derive(Debug, Clone)]
pub enum Stmt {
    VarDecl(VarDecl),
    FunctionDecl(FunctionDecl),
    If(IfStmt),
    While(WhileStmt),
    ExprStmt(ExprStmt),
    /// A statement that failed to parse; kept so the rest of the file can
    /// still be analyzed. Carries the span of the skipped region.
    Error(Span),
}

impl Stmt {
    pub fn span(&self) -> Span {
        match self {
            Stmt::VarDecl(d) => d.span,
            Stmt::FunctionDecl(d) => d.span,
            Stmt::If(s) => s.span,
            Stmt::While(s) => s.span,
            Stmt::ExprStmt(s) => s.span,
            Stmt::Error(s) => *s,
        }
    }
}

#[derive(Debug, Clone)]
pub struct VarDecl {
    pub constant: bool,
    pub identifier: String,
    pub identifier_span: Span,
    pub value: Option<Expr>,
    pub span: Span,
    /// True if the required trailing ';' was actually found.
    pub has_semicolon: bool,
}

#[derive(Debug, Clone)]
pub struct FunctionDecl {
    pub name: String,
    pub name_span: Span,
    pub parameters: Vec<(String, Span)>,
    pub body: Vec<Stmt>,
    pub span: Span,
    pub body_span: Span,
}

#[derive(Debug, Clone)]
pub struct IfStmt {
    pub condition: Expr,
    pub then_branch: Vec<Stmt>,
    pub then_span: Span,
    pub else_branch: Option<Vec<Stmt>>,
    pub else_span: Option<Span>,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub struct WhileStmt {
    pub condition: Expr,
    pub body: Vec<Stmt>,
    pub body_span: Span,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub struct ExprStmt {
    pub expr: Expr,
    pub span: Span,
    /// True if a (spec-invalid) trailing ';' followed this expression
    /// statement -- see spec 3.6 / bug B14.
    pub trailing_semicolon: Option<Span>,
}

#[derive(Debug, Clone)]
pub enum Expr {
    Assignment {
        target: Box<Expr>,
        value: Box<Expr>,
        span: Span,
    },
    Logical {
        left: Box<Expr>,
        op: LogicalOp,
        op_span: Span,
        right: Box<Expr>,
        span: Span,
    },
    Binary {
        left: Box<Expr>,
        op: BinaryOp,
        op_span: Span,
        right: Box<Expr>,
        span: Span,
    },
    Unary {
        op: UnaryOp,
        op_span: Span,
        operand: Box<Expr>,
        span: Span,
    },
    Call {
        callee: Box<Expr>,
        args: Vec<Expr>,
        span: Span,
    },
    Member {
        object: Box<Expr>,
        property: Box<Expr>,
        computed: bool,
        span: Span,
    },
    Identifier {
        name: String,
        span: Span,
    },
    NumericLiteral {
        value: String,
        span: Span,
    },
    StringLiteral {
        value: String,
        span: Span,
    },
    ArrayLiteral {
        elements: Vec<Expr>,
        span: Span,
    },
    ObjectLiteral {
        properties: Vec<Property>,
        span: Span,
    },
    Error(Span),
}

impl Expr {
    pub fn span(&self) -> Span {
        match self {
            Expr::Assignment { span, .. } => *span,
            Expr::Logical { span, .. } => *span,
            Expr::Binary { span, .. } => *span,
            Expr::Unary { span, .. } => *span,
            Expr::Call { span, .. } => *span,
            Expr::Member { span, .. } => *span,
            Expr::Identifier { span, .. } => *span,
            Expr::NumericLiteral { span, .. } => *span,
            Expr::StringLiteral { span, .. } => *span,
            Expr::ArrayLiteral { span, .. } => *span,
            Expr::ObjectLiteral { span, .. } => *span,
            Expr::Error(s) => *s,
        }
    }
}

#[derive(Debug, Clone)]
pub struct Property {
    pub key: String,
    pub key_span: Span,
    /// None => shorthand property `{ key }`.
    pub value: Option<Expr>,
    pub span: Span,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogicalOp {
    And,
    Or,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BinaryOp {
    EqEq,
    NotEq,
    Lt,
    Gt,
    Le,
    Ge,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
}

impl BinaryOp {
    pub fn as_str(&self) -> &'static str {
        match self {
            BinaryOp::EqEq => "==",
            BinaryOp::NotEq => "!=",
            BinaryOp::Lt => "<",
            BinaryOp::Gt => ">",
            BinaryOp::Le => "<=",
            BinaryOp::Ge => ">=",
            BinaryOp::Add => "+",
            BinaryOp::Sub => "-",
            BinaryOp::Mul => "*",
            BinaryOp::Div => "/",
            BinaryOp::Mod => "%",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UnaryOp {
    Not,
    Neg,
}

#[derive(Debug, Clone)]
pub struct Program {
    pub body: Vec<Stmt>,
}
