#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
// 138

// define masks for the different sizes
#define MASK_8 0xFF000000
#define FIRST_9 0xFF800000 //1111 1111 1000 0000 0000 0000 0000 0000
#define MASK_16 0xFFFF0000
#define MASK_32 0xFFFFFFFF

typedef enum {
    ADDS_IMM, // ok
    ADDS_REG, // ok
    
    SUBS_IMM, // ok
    
    SUBS_REG,
    HLT,

    CMP_IMM,
    CMP_REG,
    
    ANDS_REG,
    EOR_REG,
    ORR_REG,
    B,
    BR,
    BEQ,
    BNE,
    BGT,
    BLT,
    BGE,
    BLE,
    LSL_IMM,
    LSR_IMM,
    STUR,
    STURB,
    STURH,
    LDUR,
    LDURB,
    LDURH,
    MOVZ,
    ADD_IMM,
    ADD_REG,
    MUL,
    CBZ,
    CBNZ,
    
    SUB_IMM,
    SUB_REG,
    B_COND,
    
    UNKNOWN
} InstructionType;

typedef struct {
    InstructionType type;  // Instruction type
    uint32_t instruction;  // Original instruction

    int rd; // destination register
    int rn; // first operand register
    int rm; // second operand register (if applicable)
    int64_t imm; // immediate value (if applicable)
    int shift; // shift amount (if applicable)

    // int updates_flags;         // Whether instruction updates flags (1) or not (0)
    // int cond_code;             // For branch conditions (EQ=0, NE=1, etc.)
    // int mem_size;              // Memory operation size in bytes (1, 2, 4, 8)
    // int64_t branch_target;     // Calculated absolute address for branches

} DecodedInstruction;

// Pattern structure definition
typedef struct {
    uint32_t mask;          // Bits that matter for identification
    uint32_t value;         // Expected values for those bits
    InstructionType type;   // The instruction type this pattern represents
    const char* name;       // Human-readable name for debugging
} InstructionPattern;

void show_instruction(DecodedInstruction d) {
    printf("Decoded Instruction: ");
    printf(" | rd: %d", d.rd);
    printf(" | rn: %d", d.rn);
    printf(" | rm: %d", d.rm);
    printf(" | imm: %ld", d.imm);
    printf(" | shift: %d", d.shift);
    printf("\n");
}

// Field extraction functions (to be called after matching a pattern)
void extract_immediate_fields(uint32_t instruction, DecodedInstruction* d) {
//     Las instrucciones LSL y LSR, y como mencionamos en el punto 4 ADDS y SUBS usan
// shift. Para todas las demás instrucciones, pueden asumir que el shift (o shift amount
// [shamt]) es cero.
    
    d->rd = instruction & 0x1F; // [4:0]
    d->rn = (instruction >> 5) & 0x1F; // [9:5]
    
    int imm12 = (instruction >> 10) & 0xFFF; // [21:10]
    int shift = (instruction >> 22) & 0x3; //[23:22]
    
    if (shift == 0x1) {
        d->imm = ((int64_t)imm12) << 12; // LSL #12
    } else {
        d->imm = imm12;
    }
    d->shift = shift;
}

void extract_register_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F; // [4:0]
    d->rn = (instruction >> 5) & 0x1F; // [9:5]
    d->rm = (instruction >> 16) & 0x1F; // [20:16]
}

void extract_movz_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F;

    uint16_t imm16 = (instruction >> 5) & 0xFFFF; //[20:5]

    uint32_t hw = (instruction >> 21) & 0x3; //[22:21]
    
    if (hw != 0) {
        // just in case, i am only implementing hw=0
        printf("Warning: MOVZ with hw != 0 not supported\n");
    }

    d->imm = imm16;
    d->shift = 0; // always cero
}

void extract_shift_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F;                   // bits [4:0]
    d->rn = (instruction >> 5) & 0x1F;           // bits [9:5]
    
    // For LSL/LSR
    uint32_t immr = (instruction >> 16) & 0x3F;  // bits [21:16]
    
    // For LSL or LSR based on the opcode pattern
    if ((instruction & 0xFFC00000) == 0xD3400000) {
        d->imm = (64 - immr) % 64;
    } 
    else if ((instruction & 0xFFC00000) == 0xD3800000) {
        d->imm = immr;
    }
    else {
        d->imm = (instruction >> 10) & 0x3F;  //[15:10]
    }
    
    d->shift = 0;
    
    printf("Extracted shift amount: %ld\n", d->imm);
}

void extract_memory_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F;                // bits [4:0]
    d->rn = (instruction >> 5) & 0x1F;         // bits [9:5]
    int imm9 = (instruction >> 12) & 0x1FF;    // bits [20:12]
    
    // Sign extend imm9 (if bit 8 is set, fill bits 9-63 with 1s)
    if (imm9 & 0x100) {
        imm9 |= ~0x1FF;  // Sign extension
    }
    
    d->imm = imm9;
}

// Instructions patterns 
// Ordered from most specific to least specific to prevent false positives
const InstructionPattern patterns[] = {
    // System instructions
    {0xFFFFFC1F, 0xD4400000, HLT, "HLT"},

    // CMP
    {0xFF80001F, 0xF100001F, CMP_IMM, "CMP Immediate"},     // SUBS with rd=XZR
    {0xFFE0FC1F, 0xEB00001F, CMP_REG, "CMP Register"},    // SUBS with rd=XZR
    
    // Data processing immediate
    {0xFF800000, 0xF1000000, SUBS_IMM, "SUBS Immediate"},   //1111 0001 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFF800000, 0xB1000000, ADDS_IMM, "ADDS Immediate"},   // 1011 0001 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFF800000, 0x91000000, ADD_IMM, "ADD Immediate"},     // 1001 0001 0000 XXXX XXXX XXXX XXXX XXXX
    // {0xFF800000, 0xD1000000, SUB_IMM, "SUB Immediate"},     // 1101 0001 0000 XXXX XXXX XXXX XXXX XXXX
    // {0xFF800000, 0xF1000000, CMP_IMM, "CMP Immediate"},     // Same as SUBS but rd=XZR -> 1111 0001 0000 XXXX XXXX XXXX XXXX 0000
    
    // Data processing register
    {0xFFE0FC00, 0xAB000000, ADDS_REG, "ADDS Register"}, //
    {0xFFE0FC00, 0xEB000000, SUBS_REG, "SUBS Register"},
    {0xFFE0FC00, 0x8B000000, ADD_REG, "ADD Register"},
    // {0xFFE0FC00, 0xCB000000, SUB_REG, "SUB Register"},
    {0xFFE0FC00, 0xEB000000, CMP_REG, "CMP Register"}, // Same as SUBS but rd=XZR
    
    // Logical operations
    {0xFFE0FC00, 0xEA000000, ANDS_REG, "ANDS Register"},
    {0xFFE0FC00, 0xCA000000, EOR_REG, "EOR Register"}, // 1100 1010 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFFE0FC00, 0xAA000000, ORR_REG, "ORR Register"},
    
    // Branches
    {0xFC000000, 0x14000000, B, "B"},
    {0xFFFFFC1F, 0xD61F0000, BR, "BR"},
    {0xFF000010, 0x54000000, B_COND, "B.cond"}, // Additional check needed for condition
    {0x7F000000, 0x34000000, CBZ, "CBZ"},
    {0x7F000000, 0x35000000, CBNZ, "CBNZ"},
    
    // Memory operations
    {0xFFC00000, 0xF8000000, STUR, "STUR"},
    {0xFFC00000, 0x38000000, STURB, "STURB"},
    {0xFFC00000, 0x78000000, STURH, "STURH"},
    {0xFFC00000, 0xF8400000, LDUR, "LDUR"},
    {0xFFC00000, 0x38400000, LDURB, "LDURB"},
    {0xFFC00000, 0x78400000, LDURH, "LDURH"},
    
    // Data processing miscellaneous
    {0xFF800000, 0xD2800000, MOVZ, "MOVZ"},
    {0xFFE0FC00, 0x9B000000, MUL, "MUL"},
    {0xFFC00000, 0xD3400000, LSL_IMM, "LSL Immediate"},
    {0xFFC00000, 0xD3800000, LSR_IMM, "LSR Immediate"} //this can be the wrong code
};

const int PATTERN_COUNT = sizeof(patterns) / sizeof(patterns[0]);

DecodedInstruction decode_instruction(uint32_t instruction) {
    
    // Initialize a decoded instruction with ceros
    DecodedInstruction d;
    memset(&d, 0, sizeof(d));

    d.instruction = instruction;

    // Extract rd for special cases e.g CMP
    int rd = instruction & 0x1F;
    
    // Try to match against each pattern
    for (int i = 0; i < PATTERN_COUNT; i++) {
        if ((instruction & patterns[i].mask) == patterns[i].value) {
            
            // //special case for CMP
            // if (patterns[i].type == SUBS_IMM && rd == 31) {
            //     d.type = CMP_IMM;
            //     printf("CMP Immediate (detected via rd=XZR)\n");
            // }
            
            // else if (patterns[i].type == SUBS_REG && rd == 31) {
            //     d.type = CMP_REG;
            //     printf("CMP Register (detected via rd=XZR)\n");
            // }
            
            // else {
            //     d.type = patterns[i].type;
            //     printf("Detected Instruction: %s\n", patterns[i].name);
            // }

            d.type = patterns[i].type;
            printf("Detected Instruction: %s\n", patterns[i].name);
            
            // Extract fields based on instruction type
            switch (d.type) {
                case ADDS_IMM:
                case SUBS_IMM:
                case ADD_IMM:
                case SUB_IMM:
                case CMP_IMM:
                    extract_immediate_fields(instruction, &d);
                    break;
                
                case MOVZ:
                    extract_movz_fields(instruction, &d);
                    break;
                
                case LSL_IMM:
                case LSR_IMM:
                    extract_shift_fields(instruction, &d);
                    break;
                
                case STUR:
                case STURB:
                case STURH:
                case LDUR:
                case LDURB:
                case LDURH:
                    extract_memory_fields(instruction, &d);
                    break;
                    
                case SUBS_REG:
                case SUB_REG:
                case ADDS_REG:
                case ADD_REG:
                case CMP_REG:
                case ANDS_REG:
                case EOR_REG:
                case ORR_REG:
                case MUL:
                    extract_register_fields(instruction, &d);
                    break;
            }
            
            // Special handling for conditions in B.cond
            if (d.type == B_COND) {
                // 1) Extract the 4-bit condition from bits [3:0]
                uint32_t cond = instruction & 0xF;
            
                // 2) Extract the 19-bit signed immediate from bits [23:5]
                //    and sign-extend it, then shift left by 2.
                int32_t imm19 = (instruction >> 5) & 0x7FFFF; // 19 bits
                // sign bit is bit 18 of imm19
                if (imm19 & (1 << 18)) {
                    // sign-extend: fill upper bits with 1
                    imm19 |= ~((1 << 19) - 1);
                }
                // Each B.cond offset is left-shifted by 2
                imm19 <<= 2;
                d.imm = imm19;
            
                // 3) Map cond to your InstructionType
                switch (cond) {
                    case 0x0: d.type = BEQ; break; // Z == 1
                    case 0x1: d.type = BNE; break; // Z == 0
                    case 0xa: d.type = BGE; break; // N == V (but V=0 => N=0 => X1 >= X2)
                    case 0xb: d.type = BLT; break; // N != V (V=0 => N=1 => X1 < X2)
                    case 0xc: d.type = BGT; break; // Z==0 && N==V => (Z=0 && N=0 => X1 > X2)
                    case 0xd: d.type = BLE; break; // Z==1 || N!=V => (Z=1 || N=1 => X1 <= X2)
                    default:
                        // If you don't plan to handle other cond codes, mark as unknown
                        // or keep d.type = B_COND. Up to you.
                        d.type = UNKNOWN;
                        break;
                }
            }
            
            
            return d;
        }
    }
    
    // No match found
    d.type = UNKNOWN;
    printf("Unknown instruction\n");
    return d;
}

void update_flags(int64_t result, int updateFlags) {
    if (updateFlags) {
        NEXT_STATE.FLAG_Z = (result == 0);    // Zero flag - set if result is zero
        NEXT_STATE.FLAG_N = (result < 0);     // Negative flag - set if result is negative
        
        // Note: C and V flags are assumed to be 0 for all operations per requirements
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
    // ADD Rd, Rn, imm - adds the immediate to Rn and stores in Rd
    int64_t result = CURRENT_STATE.REGS[d.rn] + d.imm;
    NEXT_STATE.REGS[d.rd] = result;
    // Regular ADD does not update flags (use 0 as second parameter)
    update_flags(result, 0);  // 0 means don't update flags
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


void process_instruction() {
    printf("-------------------------- Processing instruction --------------------------\n\n");

    // Fetch
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    // printf("Instruction: 0x%08X\n", instruction);
    //in binary
    printf("Instruction in binary: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (instruction >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");

    //Decode
    DecodedInstruction d = decode_instruction(instruction);
    show_instruction(d);

    // Increment PC by default (some instructions might override)
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    // Execute
    switch (d.type) {
        case ADDS_IMM: {
            adds_imm(d);
            break;
        }

        case ADDS_REG: {
            adds_reg(d);
            break;
        }

        case SUBS_IMM: {
            subs_imm(d);
            break;
        }

        case SUBS_REG: {
            subs_reg(d);
            break;
        }

        case ANDS_REG: {
            ands_reg(d);
            break;
        }

        case CMP_IMM: {
            cmp_imm(d);
            break;
        }
        case CMP_REG: {
            cmp_reg(d);
            break;
        }

        case HLT: {
            hlt();
            break;
        }

        case EOR_REG: {
            eor_reg(d);
            break;
        }

        case ORR_REG: {
            orr_reg(d);
            break;
        }

        case MOVZ: {
            movz(d);
            break;
        }

        case STUR: {
            stur(d);
            break;
        }
        case STURB: {
            sturb(d);
            break;
        }
        case STURH: {
            sturh(d);
            break;
        }
        case LSL_IMM: {
            lsl_imm(d);
            break;
        }
        case LSR_IMM: {
            lsr_imm(d);
            break;
        }
        case LDUR: {
            ldur(d);
            break;
        }
        case LDURH: {
            ldurh(d);
            break;
        }
        case LDURB: {
            ldurb(d);
            break;
        }

        case BEQ: {
            beq(d);
            break;
        }

        case BNE: {
            bne(d);
            break;
        }

        case BGT: {
            bgt(d);
            break;
        }

        case BLT: {
            blt(d);
            break;
        }

        case BGE: {
            bge(d);
            break;
        }

        case BLE: {
            ble(d);
            break;
        } 

        default:
            break;
    }
}