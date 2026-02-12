# CLI Reference

## Core SDK Commands

- `oaf`
  Run `./main.oaf` when present, otherwise print usage.
- `oaf --version`
  Print the active CLI version.
- `oaf version`
  List installed versions and show the active one.
- `oaf version <version>`
  Switch the active local version.
- `oaf run <file-or-source> [-r vm|native]`
  Compile and execute source.
- `oaf build <file-or-source> [-o <output-path>] [-r vm|native]`
  Build artifacts (default flow: bytecode artifact).
- `oaf publish <file-or-source> [-o <output-path>] [-r native]`
  Publish native executable output.
- `oaf clean [-o <path>]`
  Remove build output.
- `oaf --self-test`
  Run managed self-test suite.

Useful examples:

```bash
oaf run ./examples/basics/01_hello_and_return.oaf -r vm
oaf build ./examples/basics/01_hello_and_return.oaf -o ./out/oaf
oaf publish ./examples/applications/01_sum_accumulator.oaf -o ./out/oaf-publish
oaf clean -o ./out/oaf
```

Optional inspection flags for compile/run commands:

- `--ast`
- `--ir`
- `--bytecode`
- `--run-bytecode`

## Package Manager

- `--pkg-init [manifestPath]`
- `--pkg-add <name@version> [manifestPath]`
- `--pkg-remove <name> [manifestPath]`
- `--pkg-install [manifestPath]`
- `--pkg-verify [manifestPath]`
- `add [package] <name@version> [manifestPath]` (alias of `--pkg-add`)
- `remove [package] <name> [manifestPath]` (alias of `--pkg-remove`)
- `restore [manifestPath]` (alias of `--pkg-install`)
- `verify [manifestPath]` (alias of `--pkg-verify`)

Source-backed installs:

- Optional `packages.sources` next to `packages.txt` (or `OAF_PACKAGE_INDEX` env var) points to one or more JSON package indexes.
- Index schema:

```json
{
  "source": "localrepo",
  "packages": [
    {
      "name": "core.math",
      "version": "1.0.0",
      "artifact": "./core.math-1.0.0.oafpkg",
      "sha256": "...",
      "dependencies": ["core.runtime@^1.0.0", "core.text@>=2.0.0 <3.0.0"]
    }
  ]
}
```

- `--pkg-install` verifies artifact SHA256 and extracts `.zip`/`.nupkg`/`.oafpkg` into `.oaf/packages/<name>/<version>/content`.
- `packages.txt` selectors can be exact or ranges (`1.2.3`, `^1.2.0`, `~2.4.0`, `>=1.0.0 <2.0.0`, `1.3.*`).
- Resolver includes transitive dependencies and fails install on version conflicts.
- Package module files must match their content-relative module path (`content/pkg/math.oaf` must declare `module pkg.math;`).
- `oaf run`, `oaf build`, `oaf publish`, and direct compile mode compose only explicitly imported package modules (including transitive imports) from nearest `packages.lock` and `.oaf/packages/**/content`.

## Documentation Generator

- `--gen-docs <file-or-directory> [--out <outputPath>]`

## Formatter

- `--format <file-or-source> [--check] [--write]`
  - default: print formatted output
  - `--check`: non-zero exit if formatting changes are needed
  - `--write`: write formatted output back to file

## Benchmarks

Run benchmark suite:

```bash
oaf --benchmark 200 --max-mean-ratio 5.0 --fail-on-regression
```

Run kernel benchmarks:

```bash
oaf --benchmark-kernels --iterations 5 --sum-n 5000000 --prime-n 30000 --matrix-n 48
oaf --benchmark-kernels --native --iterations 5 --sum-n 5000000 --prime-n 30000 --matrix-n 48
oaf --benchmark-kernels --tiered --iterations 5 --sum-n 5000000 --prime-n 30000 --matrix-n 48
```

Native comparison script:

```bash
./scripts/benchmark/run_c_rust_benchmarks.sh --iterations 5 --oaf-mode both --oaf-cli oaf
```
