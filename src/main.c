#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"
#include "array.h"
#include <stdint.h>

#define TAPE_SIZE 30000

typedef enum
{
    OP_ADD = '+',
    OP_SUB = '-',
    OP_MOVE_RIGHT = '>',
    OP_MOVE_LEFT = '<',
    OP_OUTPUT = '.',
    OP_INPUT = ',',
    OP_JUMP_ZERO = '[',
    OP_JUMP_NONZERO = ']'
} Opcode;

typedef struct
{
    Opcode opcode;
    size_t operand;
} Instruction;

typedef struct
{
    Instruction* items;
    size_t count;
    size_t capacity;
} Instructions;

typedef struct
{
    char* cmds;
    const char* path;
    size_t count;
    size_t pos;
} Scanner;

typedef struct {
    size_t instruction;
    size_t position;
} Addr;

typedef struct {
    Addr* items;
    size_t count;
    size_t capacity;
} Addrs;

bool is_valid_token(char ch) {
    const char* cmds = "+-<>,.[]";
    return strchr(cmds, ch) != NULL;
}

char scanner_next(Scanner* scanner) {
    while (scanner->count > 0 && scanner->pos < scanner->count && !is_valid_token(scanner->cmds[scanner->pos])) {
        scanner->pos++;
    }
    if (scanner->pos >= scanner->count)
        return 0;
    return scanner->cmds[scanner->pos++];
}

bool interpret(Instructions* instructions) {
    uint8_t tape[TAPE_SIZE] = { 0 };
    size_t ptr = 0;

    for (size_t i = 0; i < instructions->count; i++) {
        Instruction instr = instructions->items[i];

        switch (instr.opcode) {
        case OP_ADD:
            tape[ptr] += instr.operand;
            break;

        case OP_OUTPUT:
            for (size_t j = 0; j < instr.operand; j++) {
                putchar(tape[ptr]);
            }
            break;
        case OP_SUB:
            tape[ptr] -= instr.operand;
            break;

        case OP_MOVE_RIGHT:
            if (instr.operand >= TAPE_SIZE - ptr) {
                fprintf(stderr, "Tape overflow\n");
                return false;
            }
            ptr += instr.operand;
            break;

        case OP_MOVE_LEFT:
            if (instr.operand > ptr) {
                fprintf(stderr, "Tape underflow\n");
                return false;
            }
            ptr -= instr.operand;
            break;
        case OP_INPUT: {
            int ch = getchar();
            tape[ptr] = ch == EOF ? 0 : (uint8_t)ch;
            break;
        }

        default:
            break;
        }
    }
    return true;
}

void usage(const char* program) {
    fprintf(stderr, "Usage: %s [--jit] <input.bf>\n", program);
}

bool parse(Scanner* scanner, Instructions* instructions) {
    bool success = true;
    char token = scanner_next(scanner);

    Addrs addrs = { 0 };

    while (token) {
        if (token == '+' || token == '-' ||
            token == '>' || token == '<' ||
            token == '.') {
            size_t count = 1;
            char tmp = scanner_next(scanner);

            while (token == tmp) {
                count++;
                tmp = scanner_next(scanner);
            }

            Instruction instr = {
                .opcode = token,
                .operand = count
            };

            ARRAY_PUSH(instructions, instr);
            token = tmp;
        }
        else if (token == ',') {
            Instruction instr = {
                .opcode = OP_INPUT,
                .operand = 1
            };

            ARRAY_PUSH(instructions, instr);
            token = scanner_next(scanner);
        }
        else if (token == '[') {
            Addr addr = {
                .instruction = instructions->count,
                .position = scanner->pos - 1
            };

            Instruction instr = {
                .opcode = OP_JUMP_ZERO,
                .operand = 0
            };

            ARRAY_PUSH(instructions, instr);
            ARRAY_PUSH(&addrs, addr);

            token = scanner_next(scanner);
        }
        else if (token == ']') {
            if (addrs.count == 0) {
                fprintf(stderr, "%s [%zu]: ERROR: Unbalanced bracket\n",
                    scanner->path, scanner->pos - 1);
                success = false;
                break;
            }

            Addr addr = addrs.items[--addrs.count];

            Instruction instr = {
                .opcode = OP_JUMP_NONZERO,
                .operand = addr.instruction + 1
            };

            ARRAY_PUSH(instructions, instr);

            instructions->items[addr.instruction].operand = instructions->count;

            token = scanner_next(scanner);
        }

        else {
            token = scanner_next(scanner);
        }
    }
    if (addrs.count > 0) {
        Addr addr = addrs.items[addrs.count - 1];

        fprintf(stderr, "%s [%zu]: ERROR: Unbalanced bracket\n",
            scanner->path, addr.position);

        success = false;
    }

    ARRAY_FREE(&addrs);
    return success;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        usage(argv[0]);
        return 1;
    }
    const char* path = argv[1];
    char* content = read_entire_file(path);
    if (content == NULL) {
        fprintf(stderr, "ERROR: Cannot read file: %s\n", path);
        return 1;
    }
    Scanner scanner = { 0 };
    scanner.cmds = content;
    scanner.count = strlen(scanner.cmds);
    scanner.path = path;

    Instructions instructions = { 0 };
    ARRAY_INIT(&instructions);

    // dump_instructions(&instructions);
    int result = parse(&scanner, &instructions) && interpret(&instructions) ? 0 : 1;

    ARRAY_FREE(&instructions);

    free(scanner.cmds);

    return result;
}