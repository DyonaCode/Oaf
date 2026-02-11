using OafLang.Tests.Unit.Bytecode;
using OafLang.Tests.Unit.Benchmark;
using OafLang.Tests.Unit.CodeGen;
using OafLang.Tests.Integration;
using OafLang.Tests.Unit.Lexer;
using OafLang.Tests.Unit.Ownership;
using OafLang.Tests.Unit.Parser;
using OafLang.Tests.Unit.Tooling;
using OafLang.Tests.Unit.TypeChecker;

namespace OafLang.Tests;

public static class TestRunner
{
    public static int RunAll(TextWriter output)
    {
        var tests = new List<(string Name, Action Test)>();
        tests.AddRange(LexerTests.GetTests().Select(test => ($"lexer::{test.Name}", test.Test)));
        tests.AddRange(ParserTests.GetTests().Select(test => ($"parser::{test.Name}", test.Test)));
        tests.AddRange(TypeCheckerTests.GetTests().Select(test => ($"type_checker::{test.Name}", test.Test)));
        tests.AddRange(OwnershipTests.GetTests().Select(test => ($"ownership::{test.Name}", test.Test)));
        tests.AddRange(CodeGenTests.GetTests().Select(test => ($"codegen::{test.Name}", test.Test)));
        tests.AddRange(CompilerDriverPerformanceTests.GetTests().Select(test => ($"codegen::{test.Name}", test.Test)));
        tests.AddRange(BytecodeTests.GetTests().Select(test => ($"bytecode::{test.Name}", test.Test)));
        tests.AddRange(BenchmarkTests.GetTests().Select(test => ($"benchmark::{test.Name}", test.Test)));
        tests.AddRange(KernelBenchmarkTests.GetTests().Select(test => ($"benchmark::{test.Name}", test.Test)));
        tests.AddRange(PackageManagerTests.GetTests().Select(test => ($"tooling::{test.Name}", test.Test)));
        tests.AddRange(DocumentationGeneratorTests.GetTests().Select(test => ($"tooling::{test.Name}", test.Test)));
        tests.AddRange(FormatterTests.GetTests().Select(test => ($"tooling::{test.Name}", test.Test)));
        tests.AddRange(CompilerIntegrationTests.GetTests().Select(test => ($"integration::{test.Name}", test.Test)));
        tests.AddRange(ExampleProgramsIntegrationTests.GetTests().Select(test => ($"integration::{test.Name}", test.Test)));

        var failed = 0;

        foreach (var test in tests)
        {
            try
            {
                test.Test();
                output.WriteLine($"PASS {test.Name}");
            }
            catch (Exception ex)
            {
                failed++;
                output.WriteLine($"FAIL {test.Name}: {ex.Message}");
            }
        }

        output.WriteLine($"\nExecuted {tests.Count} tests, {failed} failed.");
        return failed;
    }
}
