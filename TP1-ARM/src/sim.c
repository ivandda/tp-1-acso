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
    ADDS_IMM,
    ADDS_REG,
    
    SUBS_IMM,
    
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
    InstructionType type;  // e.g. ADDS_IMM, HLT, etc.

    int rd;     // bits [4:0]
    int rn;     // bits [9:5]
    int rm;     // bits [20:16] or wherever Rm lives
    int64_t imm; // immediate if needed
    int shift;  // shift amount if needed

} DecodedInstruction;

// Pattern structure definition
typedef struct {
    uint32_t mask;          // Bits that matter for identification
    uint32_t value;         // Expected values for those bits
    InstructionType type;   // The instruction type this pattern represents
    const char* name;       // Human-readable name for debugging
} InstructionPattern;

// Field extraction functions (to be called after matching a pattern)
void extract_immediate_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F; // (0x1F = 00011111) == [4:0]
    d->rn = (instruction >> 5) & 0x1F; // 5 Rshift + (0x1F = 00011111) == [9:5]
    int imm12 = (instruction >> 10) & 0xFFF; // 12 Rshift + (0xFFF = 111111111111) == [21:10]
    int shift = (instruction >> 22) & 0x3; // 22 Rshift + (0x3 = 11) == [23:22]
    
    if (shift == 0x1) {
        d->imm = ((int64_t)imm12) << 12; // LSL #12
    } else {
        d->imm = imm12;
    }
    d->shift = shift;
}

void extract_register_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F; // (0x1F = 00011111) == [4:0]
    d->rn = (instruction >> 5) & 0x1F; // 5 Rshift + (0x1F = 00011111) == [9:5]
    d->rm = (instruction >> 16) & 0x1F; // 16 Rshift + (0x1F = 00011111) == [20:16]
}

void extract_movz_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F;

    uint16_t imm16 = (instruction >> 5) & 0xFFFF; //[20:5]

    uint32_t hw = (instruction >> 21) & 0x3; //[22:21]
    
    if (hw != 0) {
        printf("Warning: MOVZ with hw != 0 not supported\n");
    }

    d->imm = imm16;
    d->shift = 0; // always cero
}

// The pattern table - ordered from most specific to least specific
const InstructionPattern patterns[] = {
    // System instructions
    {0xFFFFFC1F, 0xD4400000, HLT, "HLT"},
    
    // Data processing immediate
    {0xFF800000, 0xF1000000, SUBS_IMM, "SUBS Immediate"},   //1111 0001 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFF800000, 0xB1000000, ADDS_IMM, "ADDS Immediate"},   // 1011 0001 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFF800000, 0x91000000, ADD_IMM, "ADD Immediate"},     // 1001 0001 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFF800000, 0xD1000000, SUB_IMM, "SUB Immediate"},     // 1101 0001 0000 XXXX XXXX XXXX XXXX XXXX
    {0xFF800000, 0xF1000000, CMP_IMM, "CMP Immediate"},     // Same as SUBS but rd=XZR -> 1111 0001 0000 XXXX XXXX XXXX XXXX 0000
    
    // Data processing register
    {0xFFE0FC00, 0xAB000000, ADDS_REG, "ADDS Register"},
    {0xFFE0FC00, 0xEB000000, SUBS_REG, "SUBS Register"},
    {0xFFE0FC00, 0x8B000000, ADD_REG, "ADD Register"},
    {0xFFE0FC00, 0xCB000000, SUB_REG, "SUB Register"},
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
    {0xFFC00000, 0xD3400000, LSR_IMM, "LSR Immediate"}
};

const int PATTERN_COUNT = sizeof(patterns) / sizeof(patterns[0]);

DecodedInstruction decode_instruction(uint32_t instruction) {
    
    // Initialize a decoded instruction with ceros
    DecodedInstruction d;
    memset(&d, 0, sizeof(d));

    // Extract rd for special cases e.g CMP
    int rd = instruction & 0x1F;
    
    // Try to match against each pattern
    for (int i = 0; i < PATTERN_COUNT; i++) {
        if ((instruction & patterns[i].mask) == patterns[i].value) {
            // d.type = patterns[i].type;
            // printf("%s\n", patterns[i].name);
            if (patterns[i].type == SUBS_IMM && rd == 31) {
                d.type = CMP_IMM;
                printf("CMP Immediate (detected via rd=XZR)\n");
            }
            else if (patterns[i].type == SUBS_REG && rd == 31) {
                d.type = CMP_REG;
                printf("CMP Register (detected via rd=XZR)\n");
            }
            else {
                d.type = patterns[i].type;
                printf("%s\n", patterns[i].name);
            }
            
            // Extract fields based on instruction type
            switch (d.type) {
                case ADDS_IMM:
                case SUBS_IMM:
                case ADD_IMM:
                case SUB_IMM:
                case CMP_IMM:
                case MOVZ:
                    extract_movz_fields(instruction, &d);
                    break;
                case LSL_IMM:
                case LSR_IMM:
                    extract_immediate_fields(instruction, &d);
                    break;
                    
                case SUBS_REG:
                    // printf("d.rd: %d\n", d.rd);
                    // if(d.rd == 31) {
                    //     d.type = CMP_REG;
                    //     extract_register_fields(instruction, &d);
                    //     printf("Detected special case CMP_REG\n");
                    // };
                case SUB_REG:
                case ADDS_REG:
                case ADD_REG:
                // case CMP_REG:
                case ANDS_REG:
                case EOR_REG:
                case ORR_REG:
                case MUL:
                    extract_register_fields(instruction, &d);
                    break;
                    
                
                //     // Handle special cases
                // case B:
                // case BEQ:
                // case BNE:
                //     // Extract branch offset
                //     d.imm = ((instruction & 0x3FFFFFF) << 2); 
                //     // Sign extend if needed
                //     if (d.imm & 0x8000000) d.imm |= 0xFFFFFFFF0000000;
                //     break;
                    
                // // More special cases as needed
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
    // CMP Rd, imm - subtracts the immediate from Rd and updates flags
    int64_t result = CURRENT_STATE.REGS[d.rd] - d.imm;
    update_flags(result, 1);
}

void cmp_reg(DecodedInstruction d) {
    printf("Executing CMP_REG\n");
    // CMP Rd, Rm - subtracts Rm from Rd and updates flags
    int64_t result = CURRENT_STATE.REGS[d.rd] - CURRENT_STATE.REGS[d.rm];
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


void add_imm(DecodedInstruction d) {
    printf("Executing ADD_IMM\n");
    // ADD Rd, Rn, imm - adds the immediate to Rn and stores in Rd
    int64_t result = CURRENT_STATE.REGS[d.rn] + d.imm;
    NEXT_STATE.REGS[d.rd] = result;
    // Regular ADD does not update flags (use 0 as second parameter)
    update_flags(result, 0);  // 0 means don't update flags
}



// void beq(DecodedInstruction d) {
//     printf("Executing BEQ\n");
//     // BEQ imm - branch if equal
//     if (CURRENT_STATE.FLAG_Z) {
//         NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
//     }
// }

void process_instruction() {
    printf("-------------------------- Processing instruction --------------------------\n\n");

    // Fetch
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    printf("Instruction: 0x%08X\n", instruction);
    //in binary
    printf("Instruction in binary: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (instruction >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");

    //Decode
    DecodedInstruction d = decode_instruction(instruction);

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

        case BEQ:
        printf("Detected special B.cond case BEQ\n");
        printf("Executing BEQ\n");
        // Branch if Z == 1
        if (CURRENT_STATE.FLAG_Z == 1) {
            NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
        }
        break;

        case BNE:
            printf("Detected special B.cond case BNE\n");
            printf("Executing BNE\n");
            // Branch if Z == 0
            if (CURRENT_STATE.FLAG_Z == 0) {
                NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
            }
            break;

        case BGT:
            printf("Detected special B.cond case BGT\n");
            printf("Executing BGT\n");
            // Branch if (Z == 0 && N == 0) under your assumption C=V=0
            if (CURRENT_STATE.FLAG_Z == 0 && CURRENT_STATE.FLAG_N == 0) {
                NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
            }
            break;

        case BLT:
            printf("Detected special B.cond case BLT\n");
            printf("Executing BLT\n");
            // Branch if N == 1
            if (CURRENT_STATE.FLAG_N == 1) {
                NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
            }
            break;

        case BGE:
            printf("Detected special B.cond case BGE\n");
            printf("Executing BGE\n");
            // Branch if N == 0
            if (CURRENT_STATE.FLAG_N == 0) {
                NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
            }
            break;

        case BLE:
            printf("Detected special B.cond case BLE\n");
            printf("Executing BLE\n");
            // Branch if Z == 1 || N == 1
            if (CURRENT_STATE.FLAG_Z == 1 || CURRENT_STATE.FLAG_N == 1) {
                NEXT_STATE.PC = CURRENT_STATE.PC + d.imm;
            }
            break;        

        // Add more cases: SUBS_IMM, ADD_IMM, B, etc.
        // Each will interpret d’s fields as needed.

        default:
            printf("Unknown Instruction to execute\n");
            break;
    }

    // The shell’s cycle() will do: CURRENT_STATE = NEXT_STATE;
    printf("Instruction processed\n\n");
}