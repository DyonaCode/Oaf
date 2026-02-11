# Runtime and Standard Library Reference

## Runtime Modules (C)

### Core

- `src/Runtime/core`
  - runtime bootstrap/shutdown
  - context wiring

### Error Handling

- `src/Runtime/error`
  - runtime error model
  - stack trace collection and formatting
  - propagation/recovery primitives

### Memory

- `src/Runtime/memory`
  - default, arena, pool, temporary allocators
  - ownership/lifetime helpers
  - bounds/null safety and leak detection
  - optional cycle collection support

### Concurrency

- `src/Runtime/concurrency`
  - lightweight scheduler + work stealing
  - channels
  - mutex/condition variable wrappers
  - atomic operations

### Types

- `src/Runtime/types`
  - runtime type information
  - reflection helpers
  - interface dispatch

### FFI

- `src/Runtime/ffi`
  - foreign type descriptors
  - marshalling/unmarshalling
  - dynamic call support (libffi optional)
  - callback registry and trampolines

## Standard Library Modules

### Collections

- `array` (`OafArray`)
- `list` (`OafList`)
- `dict` (`OafDict`, separate chaining)
- `set` (`OafSet`)

### Algorithms

- sort
- binary search
- reverse
- partition

### IO

- file open/read/write/seek/tell/flush/close
- stream abstraction wrappers

### Text

- mutable string builder utilities
- append/trim/case conversion
- formatting helpers

### Serialization

- byte buffer writer/reader
- integer/float/string encoding helpers

### Advanced Concurrency

- thread pool
- async futures (`await` style)
- parallel for/map/reduce helpers
