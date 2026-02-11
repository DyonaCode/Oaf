# Coding Standards

## C#

- Use 4-space indentation.
- Prefer explicit types over `var` unless the type is obvious from constructor usage.
- Keep classes focused and small; one responsibility per class.
- Keep parser/lexer diagnostics actionable and deterministic.

## Error Handling

- Report diagnostics instead of throwing for source errors.
- Throw only for internal invariants that indicate a compiler bug.

## Testing

- Add parser and lexer tests for every new syntax feature.
- Add type checker tests for every new semantic rule.

## Source Layout

- `Lexer/` contains tokenization logic and token definitions.
- `Parser/` contains recursive descent parser logic.
- `AST/` contains syntax node models and AST utilities.
- `TypeChecker/` contains semantic checks and symbol resolution.
