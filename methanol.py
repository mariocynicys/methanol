import os
import sys
import shutil
import subprocess


def is_expr(expr):
    return (
        # Strings, negative numbers, and fractions.
        expr[0] in ['"', '-', '.'] or
        # Booleans.
        expr in ["true", "false"] or
        # Numbers.
        expr[0].isdigit()
    )

def boolean_expr(expr):
    if expr == "true":
        return True
    elif expr == "false":
        return False
    else:
        return float(expr)

def panic(msg):
    print("Error: " + msg)
    exit(1)

def main(file):
    # Build the compiler if it doesn't exist.
    if not os.path.exists("methanol.exe"):
        subprocess.run(["bash", "src/build.sh"])
        shutil.copyfile("src/methanol", "methanol.exe")
        os.chmod("methanol.exe", 0o755)

    # Run the compiler.
    subprocess.run(["./methanol.exe", file])

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
        elif line == "POP":
            stack.pop()
        elif line == "PRINT":
            print(stack.pop())
        elif line.startswith("PUSH"):
            to_push = line.split(maxsplit=1)[1]
            if is_expr(to_push):
                stack.append(to_push)
            else:
                if variables.get(to_push) is None:
                    panic("Variable " + to_push + " is being used without being initialized.")
                stack.append(variables[to_push])
        elif line.startswith("DUP"):
            stack.append(stack[-1])
        elif line.startswith("POP"):
            variables[line.split()[1]] = stack.pop()
        elif line == "PLUS":
            stack.append(float(stack.pop()) + float(stack.pop()))
        elif line == "MINUS":
            stack.append(float(stack.pop()) - float(stack.pop()))
        elif line == "MULT":
            stack.append(float(stack.pop()) * float(stack.pop()))
        elif line == "DIV":
            first = float(stack.pop())
            second = float(stack.pop())
            if second == 0:
                panic("Division by zero.")
            stack.append(first / second)
        elif line == "NEG":
            stack.append(-float(stack.pop()))
        elif line == "LT":
            stack.append(float(stack.pop()) < float(stack.pop()))
        elif line == "GT":
            stack.append(float(stack.pop()) > float(stack.pop()))
        elif line == "LTE":
            stack.append(float(stack.pop()) <= float(stack.pop()))
        elif line == "GTE":
            stack.append(float(stack.pop()) >= float(stack.pop()))
        elif line == "EQ":
            stack.append(stack.pop() == stack.pop())
        elif line == "NE":
            stack.append(stack.pop() != stack.pop())
        elif line == "AND":
            stack.append(bool(stack.pop()) and bool(stack.pop()))
        elif line == "OR":
            stack.append(bool(stack.pop()) or bool(stack.pop()))
        elif line == "NOT":
            stack.append(not bool(stack.pop()))
        elif line.startswith("JMP"):
            index = labels[line.split()[1]]
        elif line.startswith("JZ"):
            expr = boolean_expr(stack.pop())
            if expr == 0:
                index = labels[line.split()[1]]
        elif line.startswith("JNZ"):
            expr = boolean_expr(stack.pop())
            if expr != 0:
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