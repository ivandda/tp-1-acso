#include "execute.h"
#include "shell.h"
#include "utils.h"
#include <stdio.h>

void update_flags(int64_t result, int updateFlags) {
    if (updateFlags) {
        NEXT_STATE.FLAG_Z = (result == 0);    // Zero flag
        NEXT_STATE.FLAG_N = (result < 0);     // Negative flag
        
        // In this case C and V flags are assumed to be 0 for all operations per requirements
    }
}


void adds_imm(DecodedInstruction d) {
    printf("Executing ADDS_IMM\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] + d.imm;
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 1);
}

void adds_reg(DecodedInstruction d) {
    printf("Executing ADDS_REG\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] + CURRENT_STATE.REGS[d.rm];
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 1);
}

void subs_imm(DecodedInstruction d) {
    printf("Executing SUBS_IMM\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] - d.imm;
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 1);
}

void subs_reg(DecodedInstruction d) {
    printf("Executing SUBS_REG\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] - CURRENT_STATE.REGS[d.rm];
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 1);
}

void hlt() {
    printf("Executing HLT\n");
    RUN_BIT = 0;
}

void cmp_imm(DecodedInstruction d) {
    printf("Executing CMP_IMM\n");
    // CMP Rn, imm - subtracts the immediate from Rn and updates flags
    int64_t result = CURRENT_STATE.REGS[d.rn] - d.imm;
    update_flags(result, 1);
}

void cmp_reg(DecodedInstruction d) {
    printf("Executing CMP_REG\n");
    // CMP Rn, Rm - subtracts Rm from Rn and updates flags
    int64_t result = CURRENT_STATE.REGS[d.rn] - CURRENT_STATE.REGS[d.rm];
    update_flags(result, 1);
}

void ands_reg(DecodedInstruction d) {
    printf("Executing ANDS_REG\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] & CURRENT_STATE.REGS[d.rm];
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 1);
}

void eor_reg(DecodedInstruction d) {
    printf("Executing EOR_REG\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] ^ CURRENT_STATE.REGS[d.rm];
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 0);
}

void orr_reg(DecodedInstruction d) {
    printf("Executing ORR_REG\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] | CURRENT_STATE.REGS[d.rm];
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 0);
}

void movz(DecodedInstruction d) {
    printf("Executing MOVZ\n");
    NEXT_STATE.REGS[d.rd] = d.imm;
}

void stur(DecodedInstruction d) {
    printf("Executing STUR\n");
    //stur X1, [X2, #0x10] (descripción: M[X2 + 0x10] = X1)
    
    // Calculate memory address: base register + immediate offset
    uint64_t address = CURRENT_STATE.REGS[d.rn] + d.imm;
    
    // Store full 64-bit value from Xn to memory
    // Since mem_write_32 only writes 32 bits at a time, we need two operations
    
    // Write low 32 bits
    uint32_t lower_word = (uint32_t)(CURRENT_STATE.REGS[d.rd] & 0xFFFFFFFF);
    mem_write_32(address, lower_word);
    
    // Write high 32 bits
    uint32_t upper_word = (uint32_t)((CURRENT_STATE.REGS[d.rd] >> 32) & 0xFFFFFFFF);
    mem_write_32(address + 4, upper_word);
    
    printf("Stored 0x%lx at address 0x%lx\n", CURRENT_STATE.REGS[d.rd], address);
}


void sturh(DecodedInstruction d) {
    printf("Executing STURH\n");
    
    // Calculate memory address
    uint64_t address = CURRENT_STATE.REGS[d.rn] + d.imm;
    
    // Read the current memory value so we can preserve other halfwords
    uint32_t current_value = mem_read_32(address);
    
    // Extract halfword position within the word (0-1)
    int halfword_position = (address & 0x3) >> 1;
    
    // Extract the lowest halfword of the register to store
    uint16_t halfword_to_store = CURRENT_STATE.REGS[d.rd] & 0xFFFF;
    
    // Clear the target halfword in current value and set new halfword
    uint32_t halfword_mask = 0xFFFF << (halfword_position * 16);
    uint32_t new_value = (current_value & ~halfword_mask) | 
                        (halfword_to_store << (halfword_position * 16));
    
    // Write back to memory
    mem_write_32(address & ~0x3, new_value);
    
    printf("Stored halfword 0x%x at address 0x%lx\n", halfword_to_store, address);
}

void sturb(DecodedInstruction d) {
    printf("Executing STURB\n");
    
    // Calculate memory address
    uint64_t address = CURRENT_STATE.REGS[d.rn] + d.imm;
    
    // Read the current memory value so we can preserve other bytes
    uint32_t current_value = mem_read_32(address);
    
    // Extract byte position within the word (0-3)
    int byte_position = address & 0x3;
    
    // Extract the lowest byte of the register to store
    uint8_t byte_to_store = CURRENT_STATE.REGS[d.rd] & 0xFF;
    
    // Clear the target byte in current value and set new byte
    uint32_t byte_mask = 0xFF << (byte_position * 8);
    uint32_t new_value = (current_value & ~byte_mask) | 
                         (byte_to_store << (byte_position * 8));
    
    // Write back to memory
    mem_write_32(address & ~0x3, new_value);
    
    printf("Stored byte 0x%x at address 0x%lx\n", byte_to_store, address);
}

void lsl_imm(DecodedInstruction d) {
    printf("Executing LSL_IMM\n");
    
    int shift_amount = d.imm;
    
    int64_t result = CURRENT_STATE.REGS[d.rn] << shift_amount;
    NEXT_STATE.REGS[d.rd] = result;
    
    printf("X%d = X%d << %d = 0x%lx\n", d.rd, d.rn, shift_amount, result);
}


void lsr_imm(DecodedInstruction d) {
    printf("Executing LSR_IMM\n");
    
    int shift_amount = d.imm;
    
    // Use unsigned right shift to ensure zero-filling from left
    uint64_t unsigned_value = (uint64_t)CURRENT_STATE.REGS[d.rn];
    uint64_t result = unsigned_value >> shift_amount;
    
    NEXT_STATE.REGS[d.rd] = (int64_t)result;
    printf("X%d = X%d >> %d = 0x%lx\n", d.rd, d.rn, shift_amount, (int64_t)result);
}


void ldur(DecodedInstruction d) {
    printf("Executing LDUR\n");
    
    uint64_t address = CURRENT_STATE.REGS[d.rn] + d.imm;
    
    // Load 64-bit value from memory (two 32-bit reads)
    uint32_t lower_word = mem_read_32(address);
    uint32_t upper_word = mem_read_32(address + 4);
    
    // Combine into 64-bit result - use unsigned to avoid sign extension
    uint64_t result = ((uint64_t)upper_word << 32) | (uint64_t)lower_word;
    
    NEXT_STATE.REGS[d.rd] = result;
    
    printf("X%d = Memory[0x%lx] = 0x%lx\n", d.rd, address, result);
}

void ldurh(DecodedInstruction d) {
    printf("Executing LDURH\n");
    
    // Calculate memory address
    uint64_t address = CURRENT_STATE.REGS[d.rn] + d.imm;
    
    // Read the 32-bit word containing our halfword
    uint32_t word = mem_read_32(address & ~0x3);
    
    // Extract halfword position within the word (0 or 1)
    int halfword_position = (address & 0x2) >> 1;
    
    // Extract the halfword (16 bits)
    uint16_t halfword = (word >> (halfword_position * 16)) & 0xFFFF;
    
    // Zero-extend to 64 bits (48 zeros followed by 16 bits)
    int64_t result = halfword;
    
    // Store in destination register
    NEXT_STATE.REGS[d.rd] = result;
    
    printf("X%d = Zero-extend(Memory[0x%lx](15:0)) = 0x%lx\n", d.rd, address, result);
}

void ldurb(DecodedInstruction d) {
    printf("Executing LDURB\n");
    
    // Calculate memory address
    uint64_t address = CURRENT_STATE.REGS[d.rn] + d.imm;
    
    // Read the 32-bit word containing our byte
    uint32_t word = mem_read_32(address & ~0x3);
    
    // Extract byte position within the word (0-3)
    int byte_position = address & 0x3;
    
    // Extract the byte (8 bits)
    uint8_t byte_value = (word >> (byte_position * 8)) & 0xFF;
    
    // Zero-extend to 64 bits (56 zeros followed by 8 bits)
    int64_t result = byte_value;
    
    // Store in destination register
    NEXT_STATE.REGS[d.rd] = result;
    
    printf("X%d = Zero-extend(Memory[0x%lx](7:0)) = 0x%lx\n", d.rd, address, result);
}


void add_imm(DecodedInstruction d) {
    printf("Executing ADD_IMM\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] + d.imm;
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 0);
}

void add_reg(DecodedInstruction d) {
    printf("Executing ADD_REG\n");
    int64_t result = CURRENT_STATE.REGS[d.rn] + CURRENT_STATE.REGS[d.rm];
    NEXT_STATE.REGS[d.rd] = result;
    update_flags(result, 0);
}

void beq(DecodedInstruction d) {
    printf("Executing BEQ\n");
    // Branch if Z == 1
    if (CURRENT_STATE.FLAG_Z == 1) {
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    }
}

void bne(DecodedInstruction d) {
    printf("Executing BNE\n");
    // Branch if Z == 0
    if (CURRENT_STATE.FLAG_Z == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    }
}

void bgt(DecodedInstruction d) {
    printf("Executing BGT\n");
    // Branch if (Z == 0 && N == 0) under your assumption C=V=0
    if (CURRENT_STATE.FLAG_Z == 0 && CURRENT_STATE.FLAG_N == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    }
}

void blt(DecodedInstruction d) {
    printf("Executing BLT\n");
    // Branch if N == 1
    if (CURRENT_STATE.FLAG_N == 1) {
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    }
}

void bge(DecodedInstruction d) {
    printf("Executing BGE\n");
    // Branch if N == 0
    if (CURRENT_STATE.FLAG_N == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    }
}

void ble(DecodedInstruction d) {
    printf("Executing BLE\n");
    // Branch if Z == 1 || N == 1
    if (CURRENT_STATE.FLAG_Z == 1 || CURRENT_STATE.FLAG_N == 1) {
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    }
}

void b(DecodedInstruction d) {
    printf("Executing B\n");
    
    // Update PC with the PC-relative branch target
    NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
    
    printf("Branching to PC + %ld = 0x%lx\n", d.imm, NEXT_STATE.PC);
}

void br(DecodedInstruction d) {
    printf("Executing BR\n");
    
    // Extract Rn register (bits [9:5]) if not already done
    if (d.rn == 0 && ((d.instruction >> 5) & 0x1F != 0)) {
        d.rn = (d.instruction >> 5) & 0x1F;
    }
    
    // Jump to address in register Rn
    NEXT_STATE.PC = CURRENT_STATE.REGS[d.rn];
    
    printf("Branching to address in X%d = 0x%lx\n", d.rn, NEXT_STATE.PC);
}

void mul(DecodedInstruction d) {
    printf("Executing MUL\n");
    
    // Multiply values from Rn and Rm registers
    int64_t result = CURRENT_STATE.REGS[d.rn] * CURRENT_STATE.REGS[d.rm];
    
    // Store result in Rd register
    NEXT_STATE.REGS[d.rd] = result;
    
    printf("X%d = X%d * X%d = %ld\n", d.rd, d.rn, d.rm, result);
}

void cbz(DecodedInstruction d) {
    printf("Executing CBZ\n");

    if (CURRENT_STATE.REGS[d.rd] == 0) {
        // Branch to PC-relative address
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
        printf("X%d is zero, branching to PC + %ld = 0x%lx\n", d.rd, d.imm, NEXT_STATE.PC);
    } else {
        printf("X%d is not zero (%ld), not branching\n", d.rd, CURRENT_STATE.REGS[d.rd]);
    }
}

void cbnz(DecodedInstruction d) {
    printf("Executing CBNZ\n");
    
    if (CURRENT_STATE.REGS[d.rd] != 0) {
        // Branch to PC-relative address
        NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
        printf("X%d is not zero (%ld), branching to PC + %ld = 0x%lx\n", 
               d.rd, CURRENT_STATE.REGS[d.rd], d.imm, NEXT_STATE.PC);
    } else {
        printf("X%d is zero, not branching\n", d.rd);
    }
}