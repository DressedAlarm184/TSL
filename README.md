# TapeStack Language

TSL (TapeStack Language) is an esoteric interpreted programming language. It uses both a traditional tape and user stack, along with 26 variables. All three data structures are unsigned short. TSL is turing-complete and contains many features that make it more user-friendly than other esoteric programming languages. TSL is also the transpiler target of the included TSLE (TSL Extended) compiler. It allows you to write TSL code without typing the single-character opcodes used by the language itself. It also includes niceties like naming functions and more.

## Dependencies

- C compiler with C99 support
- The Just command runner
- Python 3.10 or later (compiler only)
- GNU Readline (shell only)

## Building & Using

To build TSL, run `just build` at the root of the repository. Create a `programs` folder at the root too. Run `just run <file.tsl>` to run a TSL file. Run `just compile <source.tsl> <output.tsle>` to compile TSLE into TSL. The `run` and `compile` targets automatically operate on the `programs` folder at the root. 

## Example TSLE

```
set 5
while
	printf '%d\n'
	dec
endblock
printf 'Done!\n'
```

## Documentation

This is an esoteric programming language. There is no documentation. I don't feel like writing any. Read the source code and figure it out on your own. Good luck!
