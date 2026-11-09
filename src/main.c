#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"
#include "array.h"
#include <stdint.h>
#include <sys/mman.h>

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

typedef struct {
    uint8_t* items;
    size_t count;
    size_t capacity;
} Code;

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
    size_t pc = 0;

    while (pc < instructions->count) {
        Instruction instr = instructions->items[pc];

        switch (instr.opcode) {
        case OP_ADD:
            tape[ptr] += instr.operand;
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

        case OP_OUTPUT:
            for (size_t j = 0; j < instr.operand; j++) {
                putchar(tape[ptr]);
            }
            break;

        case OP_INPUT: {
            int ch = getchar();
            tape[ptr] = ch == EOF ? 0 : (uint8_t)ch;
            break;
        }

        case OP_JUMP_ZERO:
            if (tape[ptr] == 0) {
                pc = instr.operand;
                continue;
            }
            break;

        case OP_JUMP_NONZERO:
            if (tape[ptr] != 0) {
                pc = instr.operand;
                continue;
            }
            break;

        default:
            break;
        }

        pc++;
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

/**
 * uint8_t* tape,
 * int (*)(int) putchar,
 * int (*)(void) getchar
 * returns 0 if ok, 1 if Tape overflow or 2 when Tape underflow
 */
typedef int (*JitFn)(uint8_t*, int (*)(int), int (*)(void));

bool jit(Instructions* instructions) {
    Code code = { 0 };

#define EMIT(byte) ARRAY_PUSH(&code, (uint8_t)(byte))

#define EMIT32(value)                            \
    do {                                         \
        uint32_t v = (uint32_t)(value);          \
        for (int i = 0; i < 4; i++) {            \
            EMIT(v >> (i * 8));                  \
        }                                        \
    } while (0)

#define EMIT_EPILOGUE()                       \
    do {                                      \
        EMIT(0x41); EMIT(0x5F); /* pop r15 */ \
        EMIT(0x41); EMIT(0x5E); /* pop r14 */ \
        EMIT(0x41); EMIT(0x5D); /* pop r13 */ \
        EMIT(0x41); EMIT(0x5C); /* pop r12 */ \
        EMIT(0x5B);             /* pop rbx */ \
        EMIT(0xC3);             /* ret */     \
    } while (0)

#define EMIT_ERROR(status)                    \
        do {                                      \
            EMIT(0xB8);             /* mov eax */ \
            EMIT32(status);                       \
            EMIT_EPILOGUE();                      \
        } while (0)


    // Prologue
    EMIT(0x53);             // push rbx
    EMIT(0x41); EMIT(0x54); // push r12
    EMIT(0x41); EMIT(0x55); // push r13
    EMIT(0x41); EMIT(0x56); // push r14
    EMIT(0x41); EMIT(0x57); // push r15

    EMIT(0x48); EMIT(0x89); EMIT(0xFB); // mov rbx, rdi
    EMIT(0x49); EMIT(0x89); EMIT(0xF4); // mov r12, rsi
    EMIT(0x49); EMIT(0x89); EMIT(0xFD); // mov r13, rdi

    // mov r15, rdx
    EMIT(0x49);
    EMIT(0x89);
    EMIT(0xD7);

    // lea r14, [rdi + TAPE_SIZE]
    EMIT(0x4C);
    EMIT(0x8D);
    EMIT(0xB7);
    EMIT32(TAPE_SIZE);

    for (size_t i = 0; i < instructions->count; i++) {
        Instruction instr = instructions->items[i];

        switch (instr.opcode) {
        case OP_ADD:
            // add byte [rbx], imm8
            EMIT(0x80);
            EMIT(0x03);
            EMIT(instr.operand % 256);
            break;
        case OP_SUB:
            // sub byte [rbx], imm8
            EMIT(0x80);
            EMIT(0x2B);
            EMIT(instr.operand % 256);
            break;
        case OP_MOVE_RIGHT:
            if (instr.operand >= TAPE_SIZE) {
                EMIT_ERROR(1);
                break;
            }

            // lea rax, [rbx + imm32]
            EMIT(0x48);
            EMIT(0x8D);
            EMIT(0x83);
            EMIT32(instr.operand);

            // cmp rax, r14
            EMIT(0x4C);
            EMIT(0x39);
            EMIT(0xF0);

            // jb +15
            EMIT(0x72);
            EMIT(0x0F);

            EMIT_ERROR(1);

            // mov rbx, rax
            EMIT(0x48);
            EMIT(0x89);
            EMIT(0xC3);
            break;

        case OP_MOVE_LEFT:
            if (instr.operand >= TAPE_SIZE) {
                EMIT_ERROR(2);
                break;
            }

            // lea rax, [rbx - imm32]
            EMIT(0x48);
            EMIT(0x8D);
            EMIT(0x83);
            EMIT32((uint32_t)(-(int32_t)instr.operand));

            // cmp rax, r13
            EMIT(0x4C);
            EMIT(0x39);
            EMIT(0xE8);

            // jae +15
            EMIT(0x73);
            EMIT(0x0F);

            EMIT_ERROR(2);

            // mov rbx, rax
            EMIT(0x48);
            EMIT(0x89);
            EMIT(0xC3);
            break;

        case OP_OUTPUT:
            for (size_t j = 0; j < instr.operand; j++) {
                // movzx edi, byte [rbx]
                EMIT(0x0F);
                EMIT(0xB6);
                EMIT(0x3B);

                // call r12
                EMIT(0x41);
                EMIT(0xFF);
                EMIT(0xD4);
            }
            break;
        case OP_INPUT:
            // call r15
            EMIT(0x41);
            EMIT(0xFF);
            EMIT(0xD7);

            // cmp eax, -1 (EOF)
            EMIT(0x83);
            EMIT(0xF8);
            EMIT(0xFF);

            // jne +2
            EMIT(0x75);
            EMIT(0x02);

            // xor eax, eax
            EMIT(0x31);
            EMIT(0xC0);

            // mov byte [rbx], al
            EMIT(0x88);
            EMIT(0x03);
            break;
        default:
            fprintf(stderr, "Unsupported JIT opcode\n");
            ARRAY_FREE(&code);
            return false;
        }
    }

    // Return 0 (success)
    //xor eax, eax
    EMIT(0x31);
    EMIT(0xC0);

    // Epilogue
    EMIT_EPILOGUE();

#undef EMIT_ERROR
#undef EMIT_EPILOGUE
#undef EMIT32
#undef EMIT

    void* memory = mmap(
        NULL,
        code.count,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (memory == MAP_FAILED) {
        perror("mmap");
        ARRAY_FREE(&code);
        return false;
    }

    memcpy(memory, code.items, code.count);

    if (mprotect(memory, code.count, PROT_READ | PROT_EXEC) != 0) {
        perror("mprotect");
        munmap(memory, code.count);
        ARRAY_FREE(&code);
        return false;
    }

    uint8_t tape[TAPE_SIZE] = { 0 };

    JitFn fn;
    memcpy(&fn, &memory, sizeof(fn));
    int result = fn(tape, putchar, getchar);

    munmap(memory, code.count);
    ARRAY_FREE(&code);

    switch (result) {
    case 0:
        return true;
    case 1:
        fprintf(stderr, "Tape overflow\n");
        return false;
    case 2:
        fprintf(stderr, "Tape underflow\n");
        return false;
    default:
        fprintf(stderr, "Unknown JIT error: %d\n", result);
        return false;
    }
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
    //int result = parse(&scanner, &instructions) && interpret(&instructions) ? 0 : 1;
    int result = parse(&scanner, &instructions) &&
        jit(&instructions) ? 0 : 1;


    ARRAY_FREE(&instructions);

    free(scanner.cmds);

    return result;
}