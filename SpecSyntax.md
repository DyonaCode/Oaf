## Lexical Structure

### Tokens

- **Keywords**: `if`, `loop`, `paralloop`, `return`, `throw`, `catch`, `class`, `public`, `flux`, `enum`, `struct`, `match`, `actor`, `gc`, `base`, `recurse`, `break`, `continue`, `when`
- **Operators**:
    - Arithmetic: `+`, , , `^` (power), `/^` (root), `/`
    - Bitwise: `&&` (AND), `||` (OR), `^` (XOR), `^&` (XAND), `~` (complement)
    - Shift: `<<`, `>>`, `+<<` (unsigned left), `+>>` (unsigned right)
    - Logical: `==`, `!=`, `<`, `>`, `<=`, `>=`, `&` (AND), `|` (OR), `!|` (XOR), `!&` (XAND), `!` (NOT)
    - Assignment: `=`, `+=`, `=`, `=`, `/=`
    - Block/Flow: `=>` (block opening), `>` (branch/outcome), `;` (statement end), `;;` (block end)
    - Monadic: `<-` (bind)
- **Literals**:
    - Numeric: `42` (integer), `3.14` (float), `0b101` (binary), `0xFF` (hex)
    - String: `"standard string"`, `@"raw string"`, `$"{variable}"` (interpolated)
    - Character: `'A'`
    - Collection: `[1, 2, 3]` (array or list), `{key, value}` (dictionary entry)
- **Comments**:
    - Single line: `// comment` or `# comment`
    - Multi-line: `/#` to `#/`
    - Documentation: `@#` to `#@`

### Whitespace

- Whitespace is ignored except as token separators
- Indentation is not semantically significant

## Type Declarations

### Struct Definition

```
struct Point [int x, int y];
```

### Class Definition

```
class Person;
// Property declaration
[ string name, int age ]
// methods 
  Greet => return string $"Hello, {name}!";;
  
  
 class Person =>
 [ string name, int age] 
  Greet => return $"Hello, {name}!";
```

### Enum/ADT Definition

```
enum Option<T> => Some(T), None;
```

### Method Declaration

```
MethodName => Operations;
Test => x = 2 + 5;;

MethodName InputType Parameter => Operations;
Test int y => x = 2 + y;;

MethodName InputType Parameter -> ReturnType => Operations;
Test int input -> string => if input >= 5 => return "success"; -> return "failed";;;

```

### Method with Error Handling

```
MethodName InputType Parameter TryCatchKeyword => Operations;; ThrowKeyword "Error message";

MethodName InputType Parameter -> ReturnType TryCatchKeyword => Operations;; ThrowKeyword "Error message";
```

### Recursive Method

```
MethodName InputType Parameter RecurseKeyword => BaseCase ; RecurseCall;;

Sum List[int] items, int acc -> int recurse =>
  basecase items.Empty => return acc;
  recurse(items.Tail, acc + items.Head);;

```

## Variable Declarations

### Immutable Variable

```
count = 10;  // immutable
```

### Mutable Variable

```
flux total = 0;  // mutable
total += count;  // valid because total is mutable
```

## Control Flow

### If Statement

```
if condition => statement;;
```

### If-Else Statement

```
if condition => statement1; -> statement2;;
```

### Multiple Conditions (AND)

```
if condition1, condition2 => statement;;

```

### Pattern Matching (Type)

```
// if variant of switch  
if input =>
  1 -> return "one";
  2 -> return "two";
	  ->  return "bruh";;
	  
// match on value type like switch 
value match =>
  1 -> return "one";
  2 -> return "two";
  ->  return "bruh";;

// match on type 
Type match =>
 int i ->  HandleInteger(i);
 string s -> HandleString(s);
  -> HandleUnknown();;
 
// shorthand  
Type match =>
 int ->  HandleInteger;
 string -> HandleString;
 -> HandleUnknown;;
```

### Pattern Matching (Structure)

```
structValue match =>
 Point{x: 0, y: 0} -> "Origin";
 Point{x, y} when x == y -> "On diagonal";
 Point{x, y} -> $"At ({x}, {y})";;

```

### Pattern Matching (Collection)

```
list match =>
  [] -> "Empty";
  [x] -> $"Single element: {x}";
  [x, y, ..rest] -> $"Multiple elements";;
```

### Numeric Loop

```
loop 10 => print i;;  // i is implicitly declared
```

### Collection Iteration

```
loop items, item => print item;;
```

### While Loop

```
loop condition => statements;;
```

### Parallel Loop

```
paralloop items, item => process item;;
```

### Loop Control

```
loop condition =>
  if breakCondition => break;;
  if skipCondition => continue;;
  ;

```

### Monadic Binding

```
result = match =>
  a <- Get_value();
  b <- Compute(a);
  Ok(b * 2);;

```

## Error Handling

### Error Propagation

```
catch => Method()
  -> throw Exception;;
```

### Custom Exceptions

```
throw "OperationFailed", "Division by zero";
```

## Data Structures

### Arrays

```
[int] numbers = [1, 2, 3, 4, 5];
```

### Lists

```
List[string] names = ["Alice", "Bob", "Charlie"];
List*[int] uniqueNumbers = [1, 2, 3];  // HashSet semantics
```

### Dictionaries

```
Dict[int, string] idToName = [{1, "Alice"}, {2, "Bob"}];
Dict[string, int, bool] userData = [{"user1", 25, true}, {"user2", 30, false}];
```

## Concurrency

### Channels

```
channel = Channel<int>();
channel.Send(42);
received = channel.Receive();
```

### Actors

```
actor Counter =>
  state = 0;
  Handle "increment" -> state += 1;
  Handle "get" -> return state;;
```

### Parallel Operations

```
// Basic parallel iteration
paralloop items => Process(item);;

// Numeric range iteration
paralloop 10 => Console.WriteLine(i);;

// Indexed iteration
paralloop items, i => Console.WriteLine($"Item {i}: {items[i]}");;

// Order preservation
results = List[int]();
paralloop items, reordered: true =>
  results.Add(Transform(item));;

// Capture groups
paralloop items, capture: sum =>
  sum += Process(item);;;

// Performance parameters
paralloop items,
  minItems: 1000,  // Only parallelize if collection has at least 1000 items
  chunkSize: 100   // Process items in chunks of 100 for better locality
  => Process(item);;

// Direct collection operations
newList = list.ParallelMap(item => Transform(item));
filtered = list.ParallelFilter(item => Condition(item));
list.ParallelEach(item => ProcessItem(item));

```

## Memory Management

### Garbage Collection Control

```
// Class-level GC control
class ClassName GCoff;

// Method-level GC
MethodName Type param gc => statements;;

// Block-level GC
GC => statements;;

```

### Manual Memory Management

```
Allocate(size);
Deallocate(pointer);
```

## Module Organization

### Imports

```
import "github.com/user/package/v1"

```

### Class Modifiers

```
class ExposedClass public, gcoff;

```

## Program Entry Point

```
public class Program;
  Main => Console.WriteLine("Hello, World!");;

// With arguments
Main string[] args =>
  if args.Length > 0 => Console.WriteLine($"First argument: {args[0]}");;;

```

## Testing

```
test "Addition works correctly" =>
  result = Add(2, 3);
  assert result == 5;;
```