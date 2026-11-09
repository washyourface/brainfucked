# Brainfucked

A simple Brainfuck virtual machine written in C99, featuring a bytecode interpreter and a native x86-64 JIT compiler.  
## Features
- Support for all eight Brainfuck instructions (`+ - < > . , [ ]`)
- Bytecode interpreter
- Native x86-64 JIT compiler for Linux
- 30,000-byte memory tape with bounds checking
- Regression tests for both execution modes

## Building

Requires a C99 compiler and a Linux x86-64 environment.

```bash
make
```

## Usage

By default, the program runs using the bytecode interpreter.

**Interpreter:**

```bash
./bfckd tests/hello_world.bf
```

**JIT compiler:**

```bash
./bfckd --jit tests/hello_world.bf
```

**Input from stdin:**

```bash
echo "A" | ./bfckd --jit tests/input_basic.bf
```

Example of `tests/input_basic.bf` reading one byte from stdin and writing it to stdout:

```brainfuck
,.
```


## Tests

Run the regression tests using the interpreter:

```bash
./run_tests.sh
```

Run the same tests using the JIT:

```bash
./run_tests.sh --jit
```

## Benchmark

Both execution modes can be compared using [Hyperfine](https://github.com/sharkdp/hyperfine):

```bash
hyperfine --warmup 3 --runs 10 \
    './bfckd benchmark.bf' \
    './bfckd --jit benchmark.bf'
```

## TODO

- [ ] Add Windows x86-64 JIT support (Microsoft x64 ABI) using `VirtualAlloc` and `VirtualProtect` for executable memory management 
- [ ] Explore additional JIT optimizations

## Limitations

- The JIT currently supports Linux x86-64 only.
- Generated machine code is not persisted between executions.
- The JIT performs basic code generation without advanced optimization passes.
- The memory tape has a fixed size of 30,000 cells.