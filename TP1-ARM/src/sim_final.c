#include "decode.h"
#include "execute.h"
#include "utils.h"
#include "shell.h"
#include <stdio.h>


void process_instruction() {
    printf("-------------------------- Processing instruction --------------------------\n\n");
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    DecodedInstruction d = decode_instruction(instruction);
    show_instruction_in_binary(d);
    show_instruction(d);

    // In some cases (e.g. branches), the PC is updated in the instruction itself
    // But this is the default behavior
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    switch (d.type) {
        case ADDS_IMM: adds_imm(d); break;
        case ADDS_REG: adds_reg(d); break;
        case SUBS_IMM: subs_imm(d); break;
        case SUBS_REG: subs_reg(d); break;
        case ANDS_REG: ands_reg(d); break;
        case CMP_IMM: cmp_imm(d); break;
        case CMP_REG: cmp_reg(d); break;
        case HLT: hlt(); break;
        case EOR_REG: eor_reg(d); break;
        case ORR_REG: orr_reg(d); break;
        case MOVZ: movz(d); break;
        case STUR: stur(d); break;
        case STURB: sturb(d); break;
        case STURH: sturh(d); break;
        case LSL_IMM: lsl_imm(d); break;
        case LSR_IMM: lsr_imm(d); break;
        case LDUR: ldur(d); break;
        case LDURH: ldurh(d); break;
        case LDURB: ldurb(d); break;
        case BEQ: beq(d); break;
        case BNE: bne(d); break;
        case BGT: bgt(d); break;
        case BLT: blt(d); break;
        case BGE: bge(d); break;
        case BLE: ble(d); break;

        case B: b(d); break;
        case BR: br(d); break;
        case ADD_IMM: add_imm(d); break;
        case ADD_REG: add_reg(d); break;
        case MUL: mul(d); break;
        case CBZ: cbz(d); break;
        case CBNZ: cbnz(d); break;
        default: break;
    }

    CURRENT_STATE.REGS[31] = 0;
}
