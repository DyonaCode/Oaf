# Getting Started

## Prerequisites

- .NET SDK 10+
- CMake 3.25+
- C compiler (Clang/GCC) for runtime smoke targets

## Build and Verify

```bash
dotnet build
dotnet run -- --self-test
cmake -S . -B out/cmake-build
cmake --build out/cmake-build
```

## Run a Program

Inline source:

```bash
dotnet run -- "flux x = 2; return x + 3;" --run-bytecode
```

From file:

```bash
dotnet run -- ./examples/basics/01_hello_and_return.oaf --run-bytecode
```

## Inspect Compiler Stages

```bash
dotnet run -- ./examples/basics/02_control_flow_if_else.oaf --ast
dotnet run -- ./examples/basics/02_control_flow_if_else.oaf --ir
dotnet run -- ./examples/basics/02_control_flow_if_else.oaf --bytecode
```

## Use Built-in Tooling

```bash
dotnet run -- --pkg-init packages.txt
dotnet run -- --pkg-add stdlib.core@1.0.0 packages.txt
dotnet run -- --pkg-install packages.txt

dotnet run -- --gen-docs ./examples/basics/01_hello_and_return.oaf --out ./docs/generated/hello.md

dotnet run -- --format ./examples/basics/01_hello_and_return.oaf --write

dotnet run -- --benchmark 200
```
