using OafLang.Frontend.Compiler.AST;
using OafLang.Frontend.Compiler.CodeGen;
using OafLang.Frontend.Compiler.CodeGen.Bytecode;
using OafLang.Frontend.Compiler.Diagnostics;
using OafLang.Frontend.Compiler.Symbols;

namespace OafLang.Frontend.Compiler.Driver;

public sealed class CompilationResult
{
    public CompilationResult(
        CompilationUnitSyntax syntaxTree,
        IReadOnlyList<Diagnostic> diagnostics,
        SymbolTable symbols,
        IrModule irModule,
        BytecodeProgram bytecodeProgram)
    {
        SyntaxTree = syntaxTree;
        Diagnostics = diagnostics;
        Symbols = symbols;
        IrModule = irModule;
        BytecodeProgram = bytecodeProgram;
    }

    public CompilationUnitSyntax SyntaxTree { get; }

    public IReadOnlyList<Diagnostic> Diagnostics { get; }

    public SymbolTable Symbols { get; }

    public IrModule IrModule { get; }

    public BytecodeProgram BytecodeProgram { get; }

    public bool Success => Diagnostics.All(d => d.Severity != DiagnosticSeverity.Error);
}
