#include "utils.h"
#include <stdio.h>

void show_instruction(DecodedInstruction d) {
    printf("Decoded Instruction: ");
    printf(" | rd: %d", d.rd);
    printf(" | rn: %d", d.rn);
    printf(" | rm: %d", d.rm);
    printf(" | imm: %ld", d.imm);
    printf(" | shift: %d", d.shift);
    printf("\n");
}

void show_instruction_in_binary(DecodedInstruction d) {
    printf("Instruction in binary: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (d.instruction >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");
}