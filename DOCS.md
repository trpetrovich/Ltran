# Syntax
Ltran's syntax is incredibly simple. The fundamental unit of computation is the procedure.

```
proc main do
    ret i64.0
corp
```

A procedure is opened with `proc`, then the name (`main` functions always begin execution), and `do`, terminated with `corp`. If you want a program to take parameters, you'd add `with`:

```
proc example with (setarg "a" i64) (setarg "b" i64) do
    ret (add (fetchli "a") (fetchli "b"))
corp
```

Then use the directive `setarg` to set your arguments as either an integer, i.e. `i64`, or a `string`. Currently, all integer types are `i64` by default. There is no `i32`, `i16`, or `i8`.

If you want to pipe the result of one function into an argument slot in another, use parentheses. Like in that function, `fetchli` returns the value of a variable by name. We operate that function on `a` and `b`, then pipe the result of both into `add`.

# Procedures

The current procedures and what they do are as follows:

* add: Adds the two arguments and returns the result.
* fadd: Adds two floating point numbers and returns the result.
* sub: Subtracts the rhs from the lhs and returns the result.
* fsub: Subtracts two floating point numbers and returns the result.
* mul: Multiplies the two arguments and returns the result.
* fmul: Multiplies two floating point numbers and returns the result.
* div: Divides the lhs by the rhs and returns the result.
* fdiv: Divides two floating point numbers and returns the result.
* mod: Divides the lhs by the rhs and returns the remainder.
* fmod: Divides two floating point numbers and returns the remainder.
* strptr: Returns a string's memory address which can be used as a pointer.
* fetchls: Returns a string argument by name.
* fetchli: Returns an integer argument by name.
* syscall: Performs a syscall with four arguments.
* fetch: Takes the name and type of a global variable, and then returns it.
* set: Takes the name and type of a global variable, and then assigns it, creating it if it does not exist.
* ret: Returns out of a procedure with the first argument.
