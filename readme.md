# Methanol

Methanol is a new safe and fast programming language.
It has compile times less than Rust and is memory safer as well.

# FAQ

## Why this name?
> Ask the original author.

## Can you build a web server using this language?
> Of course!

## Does the language have support for classes?
> Yes!

## Where is this syntax analyzer from?
> The moon.

## Can this language target platforms other than mkana UNIX from IBM?
> MacOS support will land in Q3 and Windows will never.

## Can you do functional programming in Meth?
> Do you mean .map() and .filter()? Yes we have these.

## What type of computers can Methanol programs run on?
> Absolutely any, you can even run it on a mining rig.

# Tech & Tools

- Lex (flex with C codegen)
- Yacc  (bison with C++ codegen)

# Tokens

- int: Defines an integer
- flt: Defines a float
- log: Defines a logical (boolean)
- str: Defines a string
- enum: Defines an enumeration
- const: Marks a primitive type as constant
- print: Prints
- That's tedious, check `lex.l`

# Language Production Rules

- program: stmts
- stmts: epsilon | stmts stmt
- Check `parse.ypp` for the rest


# Quadruples & Their Description

| Quad | Description |
| ---- | ----------- |
| PUSH x | Pushes x to the stack, with x here being an expression |
| POP x | Pops the top of the stack in variable x, where x is optional here |
| DUP | Duplicated the top of the stack: if the stack is [v] it will end up [v, v] after `DUP` |
| INT2REAL | Pops the top of the stack, converts it from an integer to a real number and pushes it back |
| REAL2INT | Obvious |
| DEF | Defines a function (something like a label for functions) |
| CALL | Calls a function |
| RET | Returns to the IP the program was at before the call of a function |
| PRINT | Does nothing |
| NEG | Flips signs of the top of the stack |
| PLUS | Pops the top two values of the stack, adds them and pushes the result |
| MINUS | Similar to PLUS |
| MULT | Similar to MINUS |
| DIV | Similar to MULT |
| MOD | Not natively supported |
| LT | Less than, pops the top two values of the stack, compares them and pushes the result |
| GT | Greater than |
| EQ | Equals? |
| NEQ | Not equal? |
| LTEQ | Obvious |
| GTEQ | Obbious |
| NOT - AND - OR | Logical operation, same stack mechanism |
| LABEL | Defines a label that we can jump to |
| JMP lbl | Unconditional jump to lbl |
| JZ lbl | Jumps to lbl if the top of the stack is zero/false. This consumes the top of the stack |


# Symbol Table Format

## Contains:
- Identifier Name
- Definition Scope
- Declaration Line
- Is Used?
- Is Initialized?
- Is Constant?
- Value (available for constants only)
- Type (Function, Enum, String, etc...)

## Example:
| Id. Name | Scope | Dec. Line | Is Used | Is Init. | Is Const. | Value | Type |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
| Meth | 0 | 72 | 1 | 0 | 0 | - | an enum |
| a    | 0 | 7  | 1 | 1 | 0 | - | an integer |
| c    | 0 | 9  | 1 | 1 | 1 | 5 | an integer |

