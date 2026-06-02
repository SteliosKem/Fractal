# Fractal

A multiplatform compiler for **Fractal** — a statically-typed, object-oriented
systems language that compiles to native x86_64 assembly via NASM.

Fractal ships with **Sequence**, its built-in build system / project manager.

---

## Status

Early development. The lexer, parser, semantic analyzer, IR code generator,
and Intel assembly emitter are all in place. Currently targets:

- `x86_64-intel-win` — Windows x86_64
- `x86_64-intel-mac` — macOS x86_64

Several language features are parsed but not yet fully lowered (classes,
floating-point arithmetic, strings, arrays). See [Language Reference](#language-reference)
for the current support matrix.

---

## Building the compiler

### Prerequisites

- CMake 3.12 or newer
- A C++20 compiler (Clang, GCC, or MSVC)
- Ninja (recommended) or Make
- For running compiled output: `nasm` and `gcc` on `PATH`

On macOS:

```sh
brew install cmake ninja nasm
```

On Debian / Ubuntu:

```sh
sudo apt install cmake ninja-build clang nasm gcc
```

### Build

From the project root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The compiler binary is produced at `build/Fractal/Fractal`.

If you prefer Make over Ninja, omit `-G Ninja`.

### Platform-specific presets

`CMakePresets.json` provides ready-made configurations:

| Preset                                | Host    | Notes                            |
| ------------------------------------- | ------- | -------------------------------- |
| `x64-debug` / `x64-release`           | Windows | MSVC, Ninja                      |
| `x86-debug` / `x86-release`           | Windows | MSVC, Ninja, 32-bit              |
| `Clang 17.0.0 arm64-apple-darwin24.5.0` | macOS | `/usr/bin/clang++`               |
| `gcc`                                 | Linux   | `/usr/bin/g++`                   |

Use them with `cmake --preset <name>`.

---

## Command-line usage

```
Fractal -h | --help                Show usage
Fractal create <project_name>      Create a new Fractal project in the cwd
Fractal build                      Build the project in the cwd
Fractal -f <file>                  Compile a single .frc source file
Fractal -f <file> -o <out>         Compile a single file to a custom output dir
```

### Quick start — your first project

```sh
mkdir hello && cd hello
Fractal create hello
Fractal build
./build/intermediate/hello   # or .exe on Windows
```

`Fractal create` lays down:

```
hello/
├── build_config.json
├── src/
│   └── hello.frc        # the main file (same name as project)
└── build/               # populated on `Fractal build`
```

The file `src/<project>.frc` is the **main file** — statements outside the
`<define>...<!define>` block run at program start, equivalent to a `main()`.

---

## Project configuration

`build_config.json` lives at the project root:

```json
{
    "Name":         "hello",
    "SourcePath":   "src",
    "BuildPath":    "build",
    "Architecture": "x86_64-intel-mac"
}
```

| Field          | Description                                                 |
| -------------- | ----------------------------------------------------------- |
| `Name`         | Project name. The main source file must be `<Name>.frc`.    |
| `SourcePath`   | Directory holding `.frc` sources, relative to project root. |
| `BuildPath`    | Output directory for intermediate `.asm`/`.o` and binaries. |
| `Architecture` | `x86_64-intel-win` or `x86_64-intel-mac`.                   |

---

## Language reference

### Sample

```frc
/* Statements above <define> are not allowed.
   Definitions go inside <define>...<!define>.
   Statements below <!define> form the program's main body. */

<define>

fn add(a: i32, b: i32): i32 {
    return a + b;
}

@external
fn printf(fmt: i32): i32;

<!define>

let result: i32 = add(2, 3);

let p: (i32) = &result;     // (T) is a pointer to T
let v:  i32  = @p;           // @ dereferences a pointer

if v == 5 => {
    printf(v);
} else {
    printf(0);
}
```

### Source files

- Extension: `.frc`
- Encoding: UTF-8
- Single-line comments: `// ...`
- Multi-line comments: `/* ... */`

### File structure

Each source file has two sections:

```
<define>
    // definitions: fn, let, const, class, decorated definitions
<!define>
    // statements: executed top-to-bottom when the file is "called"
```

For the project's main file, the statements section acts as `main()`.

### Types

| Keyword       | Meaning                          | Codegen status |
| ------------- | -------------------------------- | -------------- |
| `i8` / `i16`  | 8/16-bit signed integers         | parsed; widened to i32 |
| `i32`         | 32-bit signed integer            | full           |
| `i64`         | 64-bit signed integer            | full           |
| `u32` / `u64` | unsigned integers                | parsed         |
| `f32` / `f64` | floating-point                   | parsed         |
| `bool`        | boolean                          | parsed (treated as i32) |
| `null`        | unit type                        | parsed         |
| `(T)`         | pointer to `T`                   | full           |
| `[T]`         | array of `T`                     | parsed         |
| user-defined  | classes                          | parsed         |

### Definitions

```frc
// Variables
let   name: i32 = 5;        // mutable; type may be inferred if initializer present
const PI:  f32  = 3.14;     // immutable

// Functions
fn foo(x: i32, y: i32): i32 {
    return x + y;
}

// Classes
class Point {
    public  x: i32;
    public  y: i32;
    private id: i32;
}

// Decorators
@external                   // links against a symbol provided elsewhere (libc, etc.)
fn printf(fmt: i32): i32;

@internal                   // marks a definition as module-private
fn helper(): i32 { return 0; }
```

### Statements

```frc
// Compound
{ stmt1; stmt2; }

// Control flow — note the `=>` after if/while conditions
if cond => stmt else stmt
while cond => stmt
loop stmt                   // infinite loop
break;
continue;

// Return / expression
return expr;
expr;
```

### Expressions

| Category       | Operators                                                  |
| -------------- | ---------------------------------------------------------- |
| Arithmetic     | `+`  `-`  `*`  `/`  `%`                                    |
| Comparison     | `==`  `!=`  `<`  `>`  `<=`  `>=`                           |
| Logical        | `and`  `or`  `!`                                           |
| Bitwise        | `&`  `\|`  `^`  `~`                                        |
| Assignment     | `=`  `+=`  `-=`  `*=`  `/=`                                |
| Member access  | `.` (value)  `->` (through pointer)                        |
| Address / Deref | `&expr` (address-of)  `@expr` (dereference)               |
| Grouping       | `(expr)`                                                   |
| Calls          | `name(arg, arg, ...)`                                      |
| Literals       | `42`  `3.14`  `"string"`  `'c'`  `true`  `false`  `null`   |
| Arrays         | `[1, 2, 3]`                                                |

Operator precedence (highest first): member access → unary → `* /` → `+ -` →
relational → equality → `and` → `or` → assignment.

---

## How it works

The compiler is a classical multi-pass pipeline:

```
.frc source
   │
   ▼
Lexer            Fractal/Lexer/         tokens
   │
   ▼
Parser           Fractal/Parser/        AST (definitions + statements)
   │
   ▼
SemanticAnalyzer Fractal/Analysis/      typed AST, scope resolution, casts
   │
   ▼
CodeGenerator    Fractal/CodeGeneration/ Intel-style IR (3-address-ish)
   │
   ▼
IntelCodeEmission Fractal/CodeEmission/ NASM-syntax x86_64 assembly
   │
   ▼
nasm + gcc       (shelled out by Sequence)
   │
   ▼
native executable
```

### Sequence (build system)

`Sequence` (`Fractal/Sequence/`) is the project layer that:

1. Reads `build_config.json`
2. Drives lexer → parser → analyzer → codegen → emitter
3. Writes `<BuildPath>/intermediate/<Name>.asm`
4. Shells out to `nasm -f {elf64|macho64}` then `gcc` to assemble and link

---

## Repository layout

```
Fractal-1/
├── CMakeLists.txt
├── CMakePresets.json
├── Fractal/                      # compiler source
│   ├── Fractal.cpp               # CLI entry point
│   ├── Common.h, Utilities.*     # shared helpers
│   ├── Lexer/                    # tokens, lexer, language types
│   ├── Parser/                   # AST nodes, Pratt parser
│   ├── Analysis/                 # semantic analyzer
│   ├── CodeGeneration/           # IR + code generator
│   ├── CodeEmission/             # NASM assembly emitter
│   ├── Sequence/                 # project / build orchestration
│   └── Error/                    # error reporting
└── Test/
    ├── build_config.json         # example project config
    └── src/test.frc              # example source
```

---

## License

MIT — see [LICENSE](LICENSE).
