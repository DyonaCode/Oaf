using System.Text;
using Oaf.Frontend.Compiler.AST;
using Oaf.Frontend.Compiler.Diagnostics;
using Oaf.Frontend.Compiler.Lexer;

namespace Oaf.Frontend.Compiler.Parser;

public sealed class Parser
{
    private readonly IReadOnlyList<Token> _tokens;
    private readonly Stack<HashSet<string>> _declaredVariables = new();
    private int _position;

    public Parser(string source)
    {
        var lexer = new Lexer.Lexer(source);
        _tokens = lexer.Lex().Where(token => token.Kind != TokenKind.BadToken).ToList();
        Diagnostics.AddRange(lexer.Diagnostics.Diagnostics);
        _declaredVariables.Push(new HashSet<string>(StringComparer.Ordinal));
    }

    public DiagnosticBag Diagnostics { get; } = new();

    public CompilationUnitSyntax ParseCompilationUnit()
    {
        var statements = new List<StatementSyntax>();

        while (Current.Kind != TokenKind.EndOfFileToken)
        {
            var start = _position;
            var statement = ParseStatement();
            statements.Add(statement);

            if (_position != start)
            {
                continue;
            }

            Diagnostics.ReportParserError("Parser did not make progress. Recovering to next statement boundary.", Current);
            Synchronize();

            if (_position == start)
            {
                NextToken();
            }
        }

        var span = statements.Count > 0 ? statements[0].Span : SourceSpan.Unknown;
        return new CompilationUnitSyntax(statements, span);
    }

    private StatementSyntax ParseStatement()
    {
        return Current.Kind switch
        {
            TokenKind.OpenBraceToken => ParseBraceBlockStatement(),
            TokenKind.ModuleKeyword => ParseModuleDeclarationStatement(),
            TokenKind.ImportKeyword => ParseImportStatement(),
            TokenKind.IfKeyword => ParseIfStatement(),
            TokenKind.LoopKeyword => ParseLoopStatement(isParallel: false),
            TokenKind.ParalloopKeyword => ParseLoopStatement(isParallel: true),
            TokenKind.ReturnKeyword => ParseReturnStatement(),
            TokenKind.ThrowKeyword => ParseThrowStatement(),
            TokenKind.GcKeyword => ParseGcStatement(),
            TokenKind.FluxKeyword => ParseFluxVariableDeclarationStatement(),
            TokenKind.BreakKeyword => ParseBreakStatement(),
            TokenKind.ContinueKeyword => ParseContinueStatement(),
            TokenKind.PublicKeyword => ParsePublicDeclarationStatement(),
            TokenKind.StructKeyword => ParseStructDeclarationStatement(),
            TokenKind.ClassKeyword => ParseClassDeclarationStatement(),
            TokenKind.EnumKeyword => ParseEnumDeclarationStatement(),
            _ => ParseFallbackStatement()
        };
    }

    private StatementSyntax ParseFallbackStatement()
    {
        if (IsJotStatementAtCurrent())
        {
            return ParseJotStatement();
        }

        if (IsTypedVariableDeclarationAtCurrent())
        {
            return ParseTypedVariableDeclarationStatement(isMutable: false, fallbackSpan: null);
        }

        if (IsIndexedAssignmentAtCurrent())
        {
            return ParseIndexedAssignmentStatement();
        }

        if (TryGetQualifiedIdentifierLengthAtCurrent(out var qualifiedLength))
        {
            var assignmentOperator = Peek(qualifiedLength).Kind;
            if (assignmentOperator == TokenKind.EqualsToken)
            {
                if (qualifiedLength == 1 && IsVariableDeclared(Current.Text))
                {
                    return ParseAssignmentStatement();
                }

                if (qualifiedLength == 1)
                {
                    return ParseInferredVariableDeclarationStatement(isMutable: false, fallbackSpan: null);
                }

                return ParseAssignmentStatement();
            }

            if (IsAssignmentOperator(assignmentOperator))
            {
                return ParseAssignmentStatement();
            }
        }

        return ParseExpressionOrMatchStatement();
    }

    private StatementSyntax ParsePublicDeclarationStatement()
    {
        var publicToken = Match(TokenKind.PublicKeyword, "Expected 'public'.");
        return Current.Kind switch
        {
            TokenKind.ClassKeyword => ParseClassDeclarationStatement(),
            TokenKind.StructKeyword => ParseStructDeclarationStatement(),
            TokenKind.EnumKeyword => ParseEnumDeclarationStatement(),
            _ => ParseInvalidPublicStatement(publicToken)
        };
    }

    private StatementSyntax ParseInvalidPublicStatement(Token publicToken)
    {
        Diagnostics.ReportParserError("Expected 'class', 'struct', or 'enum' after 'public'.", Current);
        var expression = ParseExpression();
        Match(TokenKind.SemicolonToken, "Expected ';' after expression statement.");
        return new ExpressionStatementSyntax(expression, SpanFrom(publicToken));
    }

    private StatementSyntax ParseModuleDeclarationStatement()
    {
        var moduleToken = Match(TokenKind.ModuleKeyword, "Expected 'module'.");
        var (moduleName, _) = ParseQualifiedIdentifier("Expected module name.");
        Match(TokenKind.SemicolonToken, "Expected ';' after module declaration.");
        return new ModuleDeclarationStatementSyntax(moduleName, SpanFrom(moduleToken));
    }

    private StatementSyntax ParseImportStatement()
    {
        var importToken = Match(TokenKind.ImportKeyword, "Expected 'import'.");
        string moduleName;
        if (Current.Kind == TokenKind.StringToken && Current.Value is string importPath)
        {
            moduleName = importPath;
            NextToken();
        }
        else
        {
            var parsed = ParseQualifiedIdentifier("Expected module name.");
            moduleName = parsed.Name;
        }

        Match(TokenKind.SemicolonToken, "Expected ';' after import statement.");
        return new ImportStatementSyntax(moduleName, SpanFrom(importToken));
    }

    private StatementSyntax ParseFluxVariableDeclarationStatement()
    {
        var flux = Match(TokenKind.FluxKeyword, "Expected 'flux'.");
        var fluxSpan = SpanFrom(flux);

        if (IsTypedVariableDeclarationAtCurrent())
        {
            return ParseTypedVariableDeclarationStatement(isMutable: true, fallbackSpan: fluxSpan);
        }

        return ParseInferredVariableDeclarationStatement(isMutable: true, fallbackSpan: fluxSpan);
    }

    private StatementSyntax ParseStructDeclarationStatement()
    {
        var keyword = Match(TokenKind.StructKeyword, "Expected 'struct'.");
        var name = Match(TokenKind.IdentifierToken, "Expected type name for struct declaration.").Text;
        var typeParameters = ParseTypeParameterList();
        var fields = ParseFieldList();
        Match(TokenKind.SemicolonToken, "Expected ';' after struct declaration.");
        return new StructDeclarationStatementSyntax(name, typeParameters, fields, SpanFrom(keyword));
    }

    private StatementSyntax ParseClassDeclarationStatement()
    {
        var keyword = Match(TokenKind.ClassKeyword, "Expected 'class'.");
        var name = Match(TokenKind.IdentifierToken, "Expected type name for class declaration.").Text;
        var typeParameters = ParseTypeParameterList();

        IReadOnlyList<FieldDeclarationSyntax> fields = [];
        if (Current.Kind == TokenKind.OpenBracketToken)
        {
            fields = ParseFieldList();
        }

        ParseOptionalClassModifiers();
        Match(TokenKind.SemicolonToken, "Expected ';' after class declaration.");
        return new ClassDeclarationStatementSyntax(name, typeParameters, fields, SpanFrom(keyword));
    }

    private void ParseOptionalClassModifiers()
    {
        if (!IsClassModifierToken(Current.Kind))
        {
            return;
        }

        while (true)
        {
            if (!IsClassModifierToken(Current.Kind))
            {
                Diagnostics.ReportParserError("Expected class modifier identifier.", Current);
                break;
            }

            NextToken();
            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }
    }

    private StatementSyntax ParseEnumDeclarationStatement()
    {
        var keyword = Match(TokenKind.EnumKeyword, "Expected 'enum'.");
        var name = Match(TokenKind.IdentifierToken, "Expected type name for enum declaration.").Text;
        var typeParameters = ParseTypeParameterList();
        Match(TokenKind.FatArrowToken, "Expected '=>' in enum declaration.");

        var variants = new List<EnumVariantSyntax>();
        while (Current.Kind != TokenKind.SemicolonToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            var variantToken = Match(TokenKind.IdentifierToken, "Expected enum variant name.");
            TypeReferenceSyntax? payloadType = null;

            if (Current.Kind == TokenKind.OpenParenToken)
            {
                NextToken();
                payloadType = ParseTypeReference();
                Match(TokenKind.CloseParenToken, "Expected ')' after enum variant payload type.");
            }

            variants.Add(new EnumVariantSyntax(variantToken.Text, payloadType, SpanFrom(variantToken)));

            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }

        Match(TokenKind.SemicolonToken, "Expected ';' after enum declaration.");
        return new EnumDeclarationStatementSyntax(name, typeParameters, variants, SpanFrom(keyword));
    }

    private IReadOnlyList<string> ParseTypeParameterList()
    {
        if (Current.Kind != TokenKind.LessToken)
        {
            return [];
        }

        Match(TokenKind.LessToken, "Expected '<' to start type parameter list.");
        var parameters = new List<string>();

        while (Current.Kind != TokenKind.GreaterToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            var parameter = Match(TokenKind.IdentifierToken, "Expected generic type parameter name.").Text;
            parameters.Add(parameter);

            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }

        Match(TokenKind.GreaterToken, "Expected '>' to close type parameter list.");
        return parameters;
    }

    private IReadOnlyList<FieldDeclarationSyntax> ParseFieldList()
    {
        Match(TokenKind.OpenBracketToken, "Expected '[' to start field list.");
        var fields = new List<FieldDeclarationSyntax>();

        while (Current.Kind != TokenKind.CloseBracketToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            var fieldType = ParseTypeReference();
            var fieldNameToken = Match(TokenKind.IdentifierToken, "Expected field name.");
            fields.Add(new FieldDeclarationSyntax(fieldType, fieldNameToken.Text, SpanFrom(fieldNameToken)));

            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }

        Match(TokenKind.CloseBracketToken, "Expected ']' to close field list.");
        return fields;
    }

    private StatementSyntax ParseBraceBlockStatement()
    {
        var openBrace = Match(TokenKind.OpenBraceToken, "Expected '{' to start block.");
        EnterVariableScope();

        var statements = new List<StatementSyntax>();
        while (Current.Kind != TokenKind.CloseBraceToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            var start = _position;
            statements.Add(ParseStatement());

            if (_position == start)
            {
                Synchronize();
            }
        }

        Match(TokenKind.CloseBraceToken, "Expected '}' to close block.");
        ExitVariableScope();
        return new BlockStatementSyntax(statements, SpanFrom(openBrace));
    }

    private StatementSyntax ParseIfStatement()
    {
        var ifToken = Match(TokenKind.IfKeyword, "Expected 'if'.");
        var condition = ParseCommaSeparatedConditionExpression();
        Match(TokenKind.FatArrowToken, "Expected '=>' after if condition.");

        var thenStatement = ParseArrowBody(stopOnElseArrow: true);

        StatementSyntax? elseStatement = null;
        if (Current.Kind == TokenKind.ArrowToken)
        {
            NextToken();
            elseStatement = ParseArrowBody(stopOnElseArrow: false);
        }

        ConsumeOptionalLegacyBlockTerminator();
        return new IfStatementSyntax(condition, thenStatement, elseStatement, SpanFrom(ifToken));
    }

    private ExpressionSyntax ParseCommaSeparatedConditionExpression()
    {
        var condition = ParseExpression();
        while (Current.Kind == TokenKind.CommaToken)
        {
            var commaToken = Match(TokenKind.CommaToken, "Expected ',' in condition list.");
            var nextCondition = ParseExpression();
            condition = new BinaryExpressionSyntax(condition, TokenKind.DoubleAmpersandToken, nextCondition, SpanFrom(commaToken));
        }

        return condition;
    }

    private StatementSyntax ParseLoopStatement(bool isParallel)
    {
        var keyword = isParallel
            ? Match(TokenKind.ParalloopKeyword, "Expected 'paralloop'.")
            : Match(TokenKind.LoopKeyword, "Expected 'loop'.");

        var iteratorOrCondition = ParseExpression();

        string? iterationVariable = null;
        if (Current.Kind == TokenKind.CommaToken)
        {
            NextToken();
            iterationVariable = Match(TokenKind.IdentifierToken, "Expected identifier after ',' in loop declaration.").Text;
        }

        Match(TokenKind.FatArrowToken, "Expected '=>' in loop statement.");
        EnterVariableScope();
        if (!string.IsNullOrWhiteSpace(iterationVariable))
        {
            DeclareVariable(iterationVariable);
        }

        var body = ParseArrowBody(stopOnElseArrow: false);
        ExitVariableScope();
        ConsumeOptionalLegacyBlockTerminator();

        return new LoopStatementSyntax(isParallel, iteratorOrCondition, iterationVariable, body, SpanFrom(keyword));
    }

    private StatementSyntax ParseArrowBody(bool stopOnElseArrow)
    {
        if (Current.Kind == TokenKind.OpenBraceToken)
        {
            return ParseBraceBlockStatement();
        }

        if (stopOnElseArrow && Current.Kind == TokenKind.ArrowToken)
        {
            Diagnostics.ReportParserError("Expected statement before '->' in if body.", Current);
            return new BlockStatementSyntax([], SourceSpan.Unknown);
        }

        var start = _position;
        var statement = ParseStatement();
        if (_position == start)
        {
            Diagnostics.ReportParserError("Unable to parse statement inside block-like body.", Current);
            Synchronize();
            if (_position == start)
            {
                NextToken();
            }

            return new BlockStatementSyntax([], SourceSpan.Unknown);
        }

        return statement;
    }

    private void ConsumeOptionalLegacyBlockTerminator()
    {
        if (Current.Kind != TokenKind.SemicolonToken || Peek(1).Kind != TokenKind.SemicolonToken)
        {
            return;
        }

        NextToken();
        NextToken();
    }

    private StatementSyntax ParseInferredVariableDeclarationStatement(bool isMutable, SourceSpan? fallbackSpan)
    {
        var identifierToken = Match(TokenKind.IdentifierToken, "Expected identifier in variable declaration.");
        Match(TokenKind.EqualsToken, "Expected '=' in variable declaration.");
        var initializer = ParseExpression();
        Match(TokenKind.SemicolonToken, "Expected ';' after variable declaration.");
        DeclareVariable(identifierToken.Text);

        return new VariableDeclarationStatementSyntax(
            identifierToken.Text,
            isMutable,
            initializer,
            fallbackSpan ?? SpanFrom(identifierToken));
    }

    private StatementSyntax ParseTypedVariableDeclarationStatement(bool isMutable, SourceSpan? fallbackSpan)
    {
        var declaredType = ParseTypeReference();
        var identifierToken = Match(TokenKind.IdentifierToken, "Expected identifier in variable declaration.");
        Match(TokenKind.EqualsToken, "Expected '=' in variable declaration.");
        var initializer = ParseExpression();
        Match(TokenKind.SemicolonToken, "Expected ';' after variable declaration.");
        DeclareVariable(identifierToken.Text);

        return new VariableDeclarationStatementSyntax(
            identifierToken.Text,
            isMutable,
            initializer,
            fallbackSpan ?? declaredType.Span,
            declaredType);
    }

    private TypeReferenceSyntax ParseTypeReference()
    {
        if (Current.Kind == TokenKind.OpenBracketToken)
        {
            var openBracket = Match(TokenKind.OpenBracketToken, "Expected '[' to start array type.");
            var elementType = ParseTypeReference();
            Match(TokenKind.CloseBracketToken, "Expected ']' to close array type.");
            return new TypeReferenceSyntax("array", [elementType], SpanFrom(openBracket));
        }

        var (name, nameSpan) = ParseQualifiedIdentifier("Expected type name.");

        if (Current.Kind != TokenKind.LessToken)
        {
            return new TypeReferenceSyntax(name, [], nameSpan);
        }

        Match(TokenKind.LessToken, "Expected '<' to start type argument list.");
        var typeArguments = new List<TypeReferenceSyntax>();

        while (Current.Kind != TokenKind.GreaterToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            typeArguments.Add(ParseTypeReference());

            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }

        Match(TokenKind.GreaterToken, "Expected '>' to close type argument list.");
        return new TypeReferenceSyntax(name, typeArguments, nameSpan);
    }

    private StatementSyntax ParseAssignmentStatement()
    {
        var (identifier, span) = ParseQualifiedIdentifier("Expected identifier on assignment left side.");
        var assignmentToken = NextToken();

        if (!IsAssignmentOperator(assignmentToken.Kind))
        {
            Diagnostics.ReportParserError("Expected assignment operator.", assignmentToken);
            assignmentToken = new Token(TokenKind.EqualsToken, "=", null, assignmentToken.Line, assignmentToken.Column);
        }

        var expression = ParseExpression();
        Match(TokenKind.SemicolonToken, "Expected ';' after assignment.");

        return new AssignmentStatementSyntax(identifier, assignmentToken.Kind, expression, span);
    }

    private StatementSyntax ParseIndexedAssignmentStatement()
    {
        var target = ParsePrimaryExpression();
        var assignmentToken = NextToken();

        if (!IsAssignmentOperator(assignmentToken.Kind))
        {
            Diagnostics.ReportParserError("Expected assignment operator.", assignmentToken);
            assignmentToken = new Token(TokenKind.EqualsToken, "=", null, assignmentToken.Line, assignmentToken.Column);
        }

        var expression = ParseExpression();
        Match(TokenKind.SemicolonToken, "Expected ';' after assignment.");
        return new IndexedAssignmentStatementSyntax(target, assignmentToken.Kind, expression, target.Span);
    }

    private StatementSyntax ParseExpressionOrMatchStatement()
    {
        var expression = ParseExpression();
        if (Current.Kind == TokenKind.MatchKeyword)
        {
            return ParseMatchStatement(expression);
        }

        Match(TokenKind.SemicolonToken, "Expected ';' after expression statement.");
        return new ExpressionStatementSyntax(expression, expression.Span);
    }

    private StatementSyntax ParseMatchStatement(ExpressionSyntax expression)
    {
        var matchToken = Match(TokenKind.MatchKeyword, "Expected 'match'.");
        Match(TokenKind.FatArrowToken, "Expected '=>' after match keyword.");

        var arms = new List<MatchArmSyntax>();
        while (Current.Kind != TokenKind.EndOfFileToken
               && !(Current.Kind == TokenKind.SemicolonToken && Peek(1).Kind == TokenKind.SemicolonToken))
        {
            var armStart = Current;
            ExpressionSyntax? pattern = null;
            if (Current.Kind == TokenKind.ArrowToken)
            {
                NextToken();
            }
            else
            {
                pattern = ParseExpression();
                Match(TokenKind.ArrowToken, "Expected '->' after match pattern.");
            }

            var body = ParseArrowBody(stopOnElseArrow: false);
            arms.Add(new MatchArmSyntax(pattern, body, pattern?.Span ?? SpanFrom(armStart)));
        }

        if (arms.Count == 0)
        {
            Diagnostics.ReportParserError("Match statement requires at least one arm.", matchToken);
        }

        if (Current.Kind == TokenKind.SemicolonToken && Peek(1).Kind == TokenKind.SemicolonToken)
        {
            NextToken();
            NextToken();
        }
        else
        {
            Diagnostics.ReportParserError("Expected ';;' to close match statement.", Current);
        }

        return new MatchStatementSyntax(expression, arms, expression.Span);
    }

    private StatementSyntax ParseReturnStatement()
    {
        var returnToken = Match(TokenKind.ReturnKeyword, "Expected 'return'.");

        ExpressionSyntax? expression = null;
        if (Current.Kind != TokenKind.SemicolonToken)
        {
            expression = ParseExpression();
        }

        Match(TokenKind.SemicolonToken, "Expected ';' after return statement.");
        return new ReturnStatementSyntax(expression, SpanFrom(returnToken));
    }

    private StatementSyntax ParseThrowStatement()
    {
        var throwToken = Match(TokenKind.ThrowKeyword, "Expected 'throw'.");
        ExpressionSyntax? errorExpression = null;
        ExpressionSyntax? detailExpression = null;

        if (Current.Kind != TokenKind.SemicolonToken)
        {
            errorExpression = ParseExpression();
            if (Current.Kind == TokenKind.CommaToken)
            {
                NextToken();
                detailExpression = ParseExpression();
            }
        }

        Match(TokenKind.SemicolonToken, "Expected ';' after throw statement.");
        return new ThrowStatementSyntax(errorExpression, detailExpression, SpanFrom(throwToken));
    }

    private StatementSyntax ParseBreakStatement()
    {
        var breakToken = Match(TokenKind.BreakKeyword, "Expected 'break'.");
        Match(TokenKind.SemicolonToken, "Expected ';' after break.");
        return new BreakStatementSyntax(SpanFrom(breakToken));
    }

    private StatementSyntax ParseContinueStatement()
    {
        var continueToken = Match(TokenKind.ContinueKeyword, "Expected 'continue'.");
        Match(TokenKind.SemicolonToken, "Expected ';' after continue.");
        return new ContinueStatementSyntax(SpanFrom(continueToken));
    }

    private StatementSyntax ParseGcStatement()
    {
        var gcToken = Match(TokenKind.GcKeyword, "Expected 'gc'.");
        Match(TokenKind.FatArrowToken, "Expected '=>' after 'gc'.");
        var body = ParseArrowBody(stopOnElseArrow: false);
        ConsumeOptionalLegacyBlockTerminator();
        return new GcStatementSyntax(body, SpanFrom(gcToken));
    }

    private StatementSyntax ParseJotStatement()
    {
        var jotToken = Match(TokenKind.IdentifierToken, "Expected 'Jot'.");
        Match(TokenKind.OpenParenToken, "Expected '(' after 'Jot'.");
        var expression = ParseExpression();
        Match(TokenKind.CloseParenToken, "Expected ')' after Jot argument.");
        Match(TokenKind.SemicolonToken, "Expected ';' after Jot statement.");
        return new JotStatementSyntax(expression, SpanFrom(jotToken));
    }

    private ExpressionSyntax ParseExpression()
    {
        return ParseBinaryExpression();
    }

    private ExpressionSyntax ParseBinaryExpression(int parentPrecedence = 0)
    {
        var left = ParseUnaryExpression();

        while (true)
        {
            var precedence = GetBinaryOperatorPrecedence(Current.Kind);
            if (precedence == 0 || precedence <= parentPrecedence)
            {
                break;
            }

            var operatorToken = NextToken();
            var right = ParseBinaryExpression(precedence);
            left = new BinaryExpressionSyntax(left, operatorToken.Kind, right, SpanFrom(operatorToken));
        }

        return left;
    }

    private ExpressionSyntax ParseUnaryExpression()
    {
        if (IsCastExpressionAtCurrent())
        {
            var openParen = Match(TokenKind.OpenParenToken, "Expected '(' for cast expression.");
            var targetType = ParseTypeReference();
            Match(TokenKind.CloseParenToken, "Expected ')' after cast type.");
            var expression = ParseUnaryExpression();
            return new CastExpressionSyntax(targetType, expression, SpanFrom(openParen));
        }

        var unaryPrecedence = GetUnaryOperatorPrecedence(Current.Kind);
        if (unaryPrecedence != 0)
        {
            var operatorToken = NextToken();
            var operand = ParseUnaryExpression();
            return new UnaryExpressionSyntax(operatorToken.Kind, operand, SpanFrom(operatorToken));
        }

        return ParsePrimaryExpression();
    }

    private ExpressionSyntax ParsePrimaryExpression()
    {
        var expression = ParsePrimaryAtomExpression();
        while (Current.Kind == TokenKind.OpenBracketToken)
        {
            var openBracket = Match(TokenKind.OpenBracketToken, "Expected '[' for indexing.");
            var index = ParseExpression();
            Match(TokenKind.CloseBracketToken, "Expected ']' after index expression.");
            expression = new IndexExpressionSyntax(expression, index, SpanFrom(openBracket));
        }

        return expression;
    }

    private ExpressionSyntax ParsePrimaryAtomExpression()
    {
        if (Current.Kind == TokenKind.OpenParenToken)
        {
            var open = NextToken();
            var expression = ParseExpression();
            Match(TokenKind.CloseParenToken, "Expected ')' after expression.");
            return new ParenthesizedExpressionSyntax(expression, SpanFrom(open));
        }

        if (Current.Kind == TokenKind.OpenBracketToken)
        {
            return ParseArrayLiteralExpression();
        }

        if (Current.Kind is TokenKind.NumberToken or TokenKind.StringToken or TokenKind.CharToken or TokenKind.TrueKeyword or TokenKind.FalseKeyword)
        {
            var token = NextToken();
            return new LiteralExpressionSyntax(token.Value, SpanFrom(token));
        }

        if (Current.Kind == TokenKind.IdentifierToken)
        {
            if (IsConstructorExpressionAtCurrent())
            {
                return ParseConstructorExpression();
            }

            var (identifier, span) = ParseQualifiedIdentifier("Expected identifier.");
            return new NameExpressionSyntax(identifier, span);
        }

        var unexpected = NextToken();
        Diagnostics.ReportParserError("Expected expression.", unexpected);
        return new LiteralExpressionSyntax(null, SpanFrom(unexpected));
    }

    private ExpressionSyntax ParseArrayLiteralExpression()
    {
        var openBracket = Match(TokenKind.OpenBracketToken, "Expected '[' to start array literal.");
        var elements = new List<ExpressionSyntax>();

        while (Current.Kind != TokenKind.CloseBracketToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            elements.Add(ParseExpression());
            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }

        Match(TokenKind.CloseBracketToken, "Expected ']' to close array literal.");
        return new ArrayLiteralExpressionSyntax(elements, SpanFrom(openBracket));
    }

    private ExpressionSyntax ParseConstructorExpression()
    {
        var targetType = ParseTypeReference();
        Match(TokenKind.OpenBracketToken, "Expected '[' to start constructor argument list.");
        var arguments = new List<ExpressionSyntax>();

        while (Current.Kind != TokenKind.CloseBracketToken && Current.Kind != TokenKind.EndOfFileToken)
        {
            arguments.Add(ParseExpression());
            if (Current.Kind != TokenKind.CommaToken)
            {
                break;
            }

            NextToken();
        }

        Match(TokenKind.CloseBracketToken, "Expected ']' to close constructor argument list.");
        return new ConstructorExpressionSyntax(targetType, arguments, targetType.Span);
    }

    private bool IsTypedVariableDeclarationAtCurrent()
    {
        var cursor = _position;
        if (!TrySkipTypeReference(ref cursor))
        {
            return false;
        }

        if (PeekAbsolute(cursor).Kind != TokenKind.IdentifierToken)
        {
            return false;
        }

        cursor++;
        return PeekAbsolute(cursor).Kind == TokenKind.EqualsToken;
    }

    private bool IsCastExpressionAtCurrent()
    {
        if (Current.Kind != TokenKind.OpenParenToken)
        {
            return false;
        }

        var cursor = _position + 1;
        if (!TrySkipTypeReference(ref cursor))
        {
            return false;
        }

        if (PeekAbsolute(cursor).Kind != TokenKind.CloseParenToken)
        {
            return false;
        }

        cursor++;
        return IsExpressionStart(PeekAbsolute(cursor).Kind);
    }

    private bool IsConstructorExpressionAtCurrent()
    {
        if (Current.Kind != TokenKind.IdentifierToken)
        {
            return false;
        }

        var text = Current.Text ?? string.Empty;
        if (text.Length == 0 || !char.IsUpper(text[0]))
        {
            return false;
        }

        var cursor = _position;
        if (!TrySkipTypeReference(ref cursor))
        {
            return false;
        }

        return PeekAbsolute(cursor).Kind == TokenKind.OpenBracketToken;
    }

    private bool TrySkipTypeReference(ref int cursor)
    {
        if (PeekAbsolute(cursor).Kind == TokenKind.OpenBracketToken)
        {
            cursor++;
            if (!TrySkipTypeReference(ref cursor))
            {
                return false;
            }

            if (PeekAbsolute(cursor).Kind != TokenKind.CloseBracketToken)
            {
                return false;
            }

            cursor++;
            return true;
        }

        if (PeekAbsolute(cursor).Kind != TokenKind.IdentifierToken)
        {
            return false;
        }

        cursor++;
        while (PeekAbsolute(cursor).Kind == TokenKind.DotToken && PeekAbsolute(cursor + 1).Kind == TokenKind.IdentifierToken)
        {
            cursor += 2;
        }

        if (PeekAbsolute(cursor).Kind != TokenKind.LessToken)
        {
            return true;
        }

        cursor++;
        if (!TrySkipTypeReference(ref cursor))
        {
            return false;
        }

        while (PeekAbsolute(cursor).Kind == TokenKind.CommaToken)
        {
            cursor++;
            if (!TrySkipTypeReference(ref cursor))
            {
                return false;
            }
        }

        if (PeekAbsolute(cursor).Kind != TokenKind.GreaterToken)
        {
            return false;
        }

        cursor++;
        return true;
    }

    private void Synchronize()
    {
        while (Current.Kind != TokenKind.EndOfFileToken)
        {
            if (Current.Kind == TokenKind.SemicolonToken)
            {
                NextToken();
                return;
            }

            if (IsStatementStart(Current.Kind))
            {
                return;
            }

            NextToken();
        }
    }

    private bool IsStatementStart(TokenKind kind)
    {
        return kind is TokenKind.ModuleKeyword
            or TokenKind.ImportKeyword
            or TokenKind.IfKeyword
            or TokenKind.LoopKeyword
            or TokenKind.ParalloopKeyword
            or TokenKind.ReturnKeyword
            or TokenKind.ThrowKeyword
            or TokenKind.GcKeyword
            or TokenKind.FluxKeyword
            or TokenKind.BreakKeyword
            or TokenKind.ContinueKeyword
            or TokenKind.PublicKeyword
            or TokenKind.StructKeyword
            or TokenKind.ClassKeyword
            or TokenKind.EnumKeyword
            or TokenKind.IdentifierToken
            or TokenKind.OpenBraceToken;
    }

    private bool TryGetQualifiedIdentifierLengthAtCurrent(out int tokenLength)
    {
        tokenLength = 0;
        if (Current.Kind != TokenKind.IdentifierToken)
        {
            return false;
        }

        tokenLength = 1;
        while (Peek(tokenLength).Kind == TokenKind.DotToken && Peek(tokenLength + 1).Kind == TokenKind.IdentifierToken)
        {
            tokenLength += 2;
        }

        return true;
    }

    private (string Name, SourceSpan Span) ParseQualifiedIdentifier(string errorMessage)
    {
        var first = Match(TokenKind.IdentifierToken, errorMessage);
        var builder = new StringBuilder(first.Text);

        while (Current.Kind == TokenKind.DotToken && Peek(1).Kind == TokenKind.IdentifierToken)
        {
            NextToken(); // dot
            var segment = Match(TokenKind.IdentifierToken, "Expected identifier after '.'.");
            builder.Append('.');
            builder.Append(segment.Text);
        }

        return (builder.ToString(), SpanFrom(first));
    }

    private bool IsExpressionStart(TokenKind kind)
    {
        return kind is TokenKind.NumberToken
            or TokenKind.StringToken
            or TokenKind.CharToken
            or TokenKind.TrueKeyword
            or TokenKind.FalseKeyword
            or TokenKind.IdentifierToken
            or TokenKind.OpenBracketToken
            or TokenKind.OpenParenToken
            or TokenKind.PlusToken
            or TokenKind.MinusToken
            or TokenKind.BangToken
            or TokenKind.TildeToken;
    }

    private bool IsJotStatementAtCurrent()
    {
        return Current.Kind == TokenKind.IdentifierToken
               && string.Equals(Current.Text, "Jot", StringComparison.OrdinalIgnoreCase)
               && Peek(1).Kind == TokenKind.OpenParenToken;
    }

    private bool IsIndexedAssignmentAtCurrent()
    {
        if (Current.Kind != TokenKind.IdentifierToken)
        {
            return false;
        }

        var cursor = _position;
        if (!TrySkipQualifiedIdentifier(ref cursor))
        {
            return false;
        }

        if (PeekAbsolute(cursor).Kind != TokenKind.OpenBracketToken)
        {
            return false;
        }

        while (PeekAbsolute(cursor).Kind == TokenKind.OpenBracketToken)
        {
            if (!TrySkipBracketExpression(ref cursor))
            {
                return false;
            }
        }

        return IsAssignmentOperator(PeekAbsolute(cursor).Kind);
    }

    private bool TrySkipQualifiedIdentifier(ref int cursor)
    {
        if (PeekAbsolute(cursor).Kind != TokenKind.IdentifierToken)
        {
            return false;
        }

        cursor++;
        while (PeekAbsolute(cursor).Kind == TokenKind.DotToken && PeekAbsolute(cursor + 1).Kind == TokenKind.IdentifierToken)
        {
            cursor += 2;
        }

        return true;
    }

    private bool TrySkipBracketExpression(ref int cursor)
    {
        if (PeekAbsolute(cursor).Kind != TokenKind.OpenBracketToken)
        {
            return false;
        }

        var depth = 0;
        while (true)
        {
            var kind = PeekAbsolute(cursor).Kind;
            if (kind == TokenKind.EndOfFileToken)
            {
                return false;
            }

            if (kind == TokenKind.OpenBracketToken)
            {
                depth++;
            }
            else if (kind == TokenKind.CloseBracketToken)
            {
                depth--;
                if (depth == 0)
                {
                    cursor++;
                    return true;
                }
            }

            cursor++;
        }
    }

    private void EnterVariableScope()
    {
        _declaredVariables.Push(new HashSet<string>(StringComparer.Ordinal));
    }

    private void ExitVariableScope()
    {
        if (_declaredVariables.Count <= 1)
        {
            return;
        }

        _declaredVariables.Pop();
    }

    private void DeclareVariable(string name)
    {
        _declaredVariables.Peek().Add(name);
    }

    private bool IsVariableDeclared(string name)
    {
        foreach (var scope in _declaredVariables)
        {
            if (scope.Contains(name))
            {
                return true;
            }
        }

        return false;
    }

    private bool IsAssignmentOperator(TokenKind kind)
    {
        return kind is TokenKind.EqualsToken
            or TokenKind.PlusEqualsToken
            or TokenKind.MinusEqualsToken
            or TokenKind.StarEqualsToken
            or TokenKind.SlashEqualsToken;
    }

    private static bool IsClassModifierToken(TokenKind kind)
    {
        return kind is TokenKind.IdentifierToken
            or TokenKind.PublicKeyword
            or TokenKind.GcKeyword;
    }

    private int GetUnaryOperatorPrecedence(TokenKind kind)
    {
        return kind switch
        {
            TokenKind.PlusToken or TokenKind.MinusToken or TokenKind.BangToken or TokenKind.TildeToken => 8,
            _ => 0
        };
    }

    private int GetBinaryOperatorPrecedence(TokenKind kind)
    {
        return kind switch
        {
            TokenKind.StarToken or TokenKind.SlashToken or TokenKind.PercentToken or TokenKind.RootToken => 7,
            TokenKind.PlusToken or TokenKind.MinusToken => 6,
            TokenKind.ShiftLeftToken or TokenKind.ShiftRightToken or TokenKind.UnsignedShiftLeftToken or TokenKind.UnsignedShiftRightToken => 5,
            TokenKind.LessToken or TokenKind.LessOrEqualsToken or TokenKind.GreaterToken or TokenKind.GreaterOrEqualsToken => 4,
            TokenKind.EqualsEqualsToken or TokenKind.BangEqualsToken => 3,
            TokenKind.AmpersandToken or TokenKind.BangAmpersandToken or TokenKind.CaretToken or TokenKind.CaretAmpersandToken => 2,
            TokenKind.PipeToken or TokenKind.BangPipeToken or TokenKind.DoubleAmpersandToken or TokenKind.DoublePipeToken => 1,
            _ => 0
        };
    }

    private Token Match(TokenKind kind, string message)
    {
        if (Current.Kind == kind)
        {
            return NextToken();
        }

        Diagnostics.ReportParserError(message, Current);
        return new Token(kind, string.Empty, null, Current.Line, Current.Column);
    }

    private Token NextToken()
    {
        var current = Current;
        _position++;
        return current;
    }

    private static SourceSpan SpanFrom(Token token)
    {
        return new SourceSpan(token.Line, token.Column, Math.Max(token.Text.Length, 1));
    }

    private Token Current => Peek(0);

    private Token Peek(int offset)
    {
        return PeekAbsolute(_position + offset);
    }

    private Token PeekAbsolute(int index)
    {
        if (index >= _tokens.Count)
        {
            return _tokens[^1];
        }

        return _tokens[index];
    }
}
