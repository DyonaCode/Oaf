using Oaf.Frontend.Compiler.CodeGen.Bytecode;
using Oaf.Frontend.Compiler.Driver;
using Oaf.Tests.Framework;

namespace Oaf.Tests.Integration;

public static class ExampleProgramsIntegrationTests
{
    public static IReadOnlyList<(string Name, Action Test)> GetTests()
    {
        return
        [
            ("all_example_programs_parse_and_typecheck", AllExampleProgramsParseAndTypecheck),
            ("all_example_programs_execute_in_bytecode_vm", AllExampleProgramsExecuteInBytecodeVm),
            ("spec_feature_coverage_example_executes_with_expected_output", SpecFeatureCoverageExampleExecutesWithExpectedOutput),
            ("optimized_expansion_example_executes_with_expected_output", OptimizedExpansionExampleExecutesWithExpectedOutput)
        ];
    }

    private static void AllExampleProgramsParseAndTypecheck()
    {
        var driver = new CompilerDriver(enableCompilationCache: false);
        foreach (var file in EnumerateExamplePrograms())
        {
            var source = File.ReadAllText(file);
            var result = driver.CompileSource(source);
            TestAssertions.True(result.Success, $"Expected example '{file}' to compile successfully.");
        }
    }

    private static void AllExampleProgramsExecuteInBytecodeVm()
    {
        var driver = new CompilerDriver(enableCompilationCache: false);
        var vm = new BytecodeVirtualMachine();

        foreach (var file in EnumerateExamplePrograms())
        {
            var source = File.ReadAllText(file);
            var result = driver.CompileSource(source);
            TestAssertions.True(result.Success, $"Expected example '{file}' to compile before execution.");

            var originalOut = Console.Out;
            var writer = new StringWriter();
            Console.SetOut(writer);

            BytecodeExecutionResult execution;
            try
            {
                execution = vm.Execute(result.BytecodeProgram);
            }
            finally
            {
                Console.SetOut(originalOut);
            }

            TestAssertions.True(execution.Success, $"Expected example '{file}' to execute successfully. {execution.ErrorMessage}");
        }
    }

    private static void SpecFeatureCoverageExampleExecutesWithExpectedOutput()
    {
        var examplesRoot = FindExamplesRoot();
        var coverageExamplePath = Path.Combine(examplesRoot, "applications", "04_spec_feature_coverage.oaf");
        TestAssertions.True(File.Exists(coverageExamplePath), $"Expected coverage example to exist at '{coverageExamplePath}'.");

        var source = File.ReadAllText(coverageExamplePath);
        var driver = new CompilerDriver(enableCompilationCache: false);
        var result = driver.CompileSource(source);
        TestAssertions.True(result.Success, $"Expected coverage example '{coverageExamplePath}' to compile.");

        var vm = new BytecodeVirtualMachine();
        var originalOut = Console.Out;
        var writer = new StringWriter();
        Console.SetOut(writer);

        BytecodeExecutionResult execution;
        try
        {
            execution = vm.Execute(result.BytecodeProgram);
        }
        finally
        {
            Console.SetOut(originalOut);
        }

        TestAssertions.True(execution.Success, $"Expected coverage example to execute successfully. {execution.ErrorMessage}");
        TestAssertions.Equal(119L, execution.ReturnValue as long? ?? Convert.ToInt64(execution.ReturnValue));

        var output = writer.ToString().Replace("\r\n", "\n", StringComparison.Ordinal);
        TestAssertions.True(output.Contains("spec_coverage\n", StringComparison.Ordinal), "Expected coverage banner output.");
        TestAssertions.True(output.Contains("mode=spec\n", StringComparison.Ordinal), "Expected string output.");
        TestAssertions.True(output.Contains("A\n", StringComparison.Ordinal), "Expected char output.");
        TestAssertions.True(output.Contains("36\n", StringComparison.Ordinal), "Expected class field output.");
        TestAssertions.True(output.Contains("2\n", StringComparison.Ordinal), "Expected struct/index scalar output.");
        TestAssertions.True(output.Contains("3\n", StringComparison.Ordinal), "Expected nested array index output.");
        TestAssertions.True(output.Contains("10\n", StringComparison.Ordinal), "Expected nested-loop coverage output.");
        TestAssertions.True(output.Contains("6\n", StringComparison.Ordinal), "Expected paralloop aggregate output.");
    }

    private static void OptimizedExpansionExampleExecutesWithExpectedOutput()
    {
        var examplesRoot = FindExamplesRoot();
        var optimizedExpansionPath = Path.Combine(examplesRoot, "applications", "05_optimized_expansion.oaf");
        TestAssertions.True(File.Exists(optimizedExpansionPath), $"Expected optimized expansion example to exist at '{optimizedExpansionPath}'.");

        var source = File.ReadAllText(optimizedExpansionPath);
        var driver = new CompilerDriver(enableCompilationCache: false);
        var result = driver.CompileSource(source);
        TestAssertions.True(result.Success, $"Expected optimized expansion example '{optimizedExpansionPath}' to compile.");

        var vm = new BytecodeVirtualMachine();
        var originalOut = Console.Out;
        var writer = new StringWriter();
        Console.SetOut(writer);

        BytecodeExecutionResult execution;
        try
        {
            execution = vm.Execute(result.BytecodeProgram);
        }
        finally
        {
            Console.SetOut(originalOut);
        }

        TestAssertions.True(execution.Success, $"Expected optimized expansion example to execute successfully. {execution.ErrorMessage}");
        TestAssertions.Equal(14L, execution.ReturnValue as long? ?? Convert.ToInt64(execution.ReturnValue));

        var output = writer.ToString().Replace("\r\n", "\n", StringComparison.Ordinal);
        TestAssertions.True(output.Contains("Final rectangle-id grid:\n", StringComparison.Ordinal), "Expected optimized expansion grid banner output.");
        TestAssertions.True(output.Contains("1 1 1 0 0\n", StringComparison.Ordinal), "Expected first grid row output.");
        TestAssertions.True(output.Contains("1 1 1 4 0\n", StringComparison.Ordinal), "Expected second grid row output.");
        TestAssertions.True(output.Contains("3 3 0 0 5\n", StringComparison.Ordinal), "Expected third grid row output.");
        TestAssertions.True(output.Contains("0 2 2 2 2\n", StringComparison.Ordinal), "Expected fourth grid row output.");
        TestAssertions.True(output.Contains("Total covered area:\n", StringComparison.Ordinal), "Expected optimized expansion area banner output.");
        TestAssertions.True(output.Contains("14\n", StringComparison.Ordinal), "Expected optimized expansion total area output.");
    }

    private static IReadOnlyList<string> EnumerateExamplePrograms()
    {
        var examplesRoot = FindExamplesRoot();
        return Directory.GetFiles(examplesRoot, "*.oaf", SearchOption.AllDirectories)
            .OrderBy(static path => path, StringComparer.Ordinal)
            .ToArray();
    }

    private static string FindExamplesRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            var candidate = Path.Combine(directory.FullName, "examples");
            if (Directory.Exists(candidate))
            {
                return candidate;
            }

            directory = directory.Parent;
        }

        throw new InvalidOperationException("Unable to locate examples directory.");
    }
}
