namespace Oaf.Frontend.Compiler.AST;

public enum SyntaxNodeKind
{
    CompilationUnit,
    BlockStatement,
    ExpressionStatement,
    VariableDeclarationStatement,
    AssignmentStatement,
    IndexedAssignmentStatement,
    MatchStatement,
    ThrowStatement,
    GcStatement,
    ReturnStatement,
    IfStatement,
    LoopStatement,
    BreakStatement,
    ContinueStatement,
    JotStatement,
    ModuleDeclarationStatement,
    ImportStatement,

    StructDeclarationStatement,
    ClassDeclarationStatement,
    EnumDeclarationStatement,
    TypeReference,
    FieldDeclaration,
    EnumVariant,
    MatchArm,

    LiteralExpression,
    NameExpression,
    ConstructorExpression,
    ArrayLiteralExpression,
    IndexExpression,
    CastExpression,
    UnaryExpression,
    BinaryExpression,
    ParenthesizedExpression
}
