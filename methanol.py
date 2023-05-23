import os
import sys
import subprocess


def is_expr(expr):
    """Returns True if the given string is an immediate value, False otherwise."""
    return (
        # Strings, negative numbers, and fractions.
        expr[0] in ['"', '-', '.'] or
        # Booleans.
        expr in ["true", "false"] or
        # Numbers.
        expr[0].isdigit()
    )

def to_expr(expr):
    """Converts a string representing an immediate value to a pythonic object."""
    # Strings
    if expr[0] == '"':
        return expr[1:-1]
    # True and False
    elif expr in ["true", "false"]:
        return {"true": True, "false": False}[expr]
    # Floats
    elif '.' in expr:
        return float(expr)
    # Integers
    else:
        return int(expr)

def panic(msg):
    print("Error: " + msg)
    exit(1)

def main(file):
    # Build the compiler if it doesn't exist.
    if not os.path.exists("compiler.exe"):
        subprocess.run(["bash", "build.sh"]).check_returncode()

    # Run the compiler.
    subprocess.run(["./compiler.exe", file]).check_returncode()

    # Remove the symbol table file.
    os.remove(file + ".sym")

    # Load the program.
    program = [line.strip()  for line in open(file + ".quad").readlines()]

    # Initialize the VM.
    stack = []          # The stack of the VM.
    variables = {}      # Runtime variables.
    labels = {}         # For labels and functions.
    index_stack = []    # For CALL and RET.

    # First, find all the labels and functions.
    for index, line in enumerate(program):
        if line.startswith(("LABEL", "DEF")):
            labels[line.split()[1][:-1]] = index

    # Run the program.
    index = 0
    while True:
        line = program[index]
        if line == "":
            pass
        elif line.startswith("/*"):
            pass
        elif line.startswith(("LABEL", "DEF")):
            pass
        elif line == "INT2REAL":
            stack.append(float(stack.pop()))
        elif line == "REAL2INT":
            stack.append(int(stack.pop()))
        elif line == "POP":
            stack.pop()
        elif line == "PRINT":
            print(stack.pop())
        elif line.startswith("PUSH"):
            to_push = line.split(maxsplit=1)[1]
            if is_expr(to_push):
                stack.append(to_expr(to_push))
            else:
                if variables.get(to_push) is None:
                    panic("Variable " + to_push + " is being used without being initialized.")
                stack.append(variables[to_push])
        elif line.startswith("DUP"):
            stack.append(stack[-1])
        elif line.startswith("POP"):
            variables[line.split()[1]] = stack.pop()
        elif line == "PLUS":
            stack.append(stack.pop() + stack.pop())
        # NOTE(MINUS, DIV, LT, GT, ...): stack[-2] is the first operand & stack[-1] is the second. Popping happens in reverse order.
        elif line == "MINUS":
            stack.append(-stack.pop() + stack.pop())
        elif line == "MULT":
            stack.append(stack.pop() * stack.pop())
        elif line == "DIV":
            second = stack.pop()
            first = stack.pop()
            if second == 0:
                panic("Division by zero.")
            # Note that both operands gonna be of the same type anyways (int or float).
            if isinstance(first, int):
                stack.append(first // second)
            else:
                stack.append(first / second)
        elif line == "NEG":
            stack.append(-stack.pop())
        elif line == "LT":
            stack.append(stack.pop() > stack.pop())
        elif line == "GT":
            stack.append(stack.pop() < stack.pop())
        elif line == "LTEQ":
            stack.append(stack.pop() >= stack.pop())
        elif line == "GTEQ":
            stack.append(stack.pop() <= stack.pop())
        elif line == "EQ":
            stack.append(stack.pop() == stack.pop())
        elif line == "NEQ":
            stack.append(stack.pop() != stack.pop())
        elif line == "AND":
            stack.append(stack.pop() and stack.pop())
        elif line == "OR":
            stack.append(stack.pop() or stack.pop())
        elif line == "NOT":
            stack.append(not stack.pop())
        elif line.startswith("JMP"):
            index = labels[line.split()[1]]
        elif line.startswith("JZ"):
            if stack.pop() == 0:
                index = labels[line.split()[1]]
        elif line.startswith("CALL"):
            index_stack.append(index)
            index = labels[line.split()[1]]
        elif line == "RET":
            index = index_stack.pop()
        else:
            panic("Invalid instruction: " + line)

        index += 1
        if index >= len(program):
            break


if __name__ == "__main__":
    main(sys.argv[1])