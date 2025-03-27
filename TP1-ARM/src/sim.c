#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
// 138


// typedef enum {
//     ADDS_IMM,
//     HLT,
//     SUBS_REG,
//     SUBS_IMM,
//     UNKNOWN,
// } InstructionType;


// DecodedInstruction decode_instruction(uint32_t instruction) {
//     DecodedInstruction d;
//     memset(&d, 0, sizeof(d));

//     int isImmediate = (instruction >> 22) & 1; // bit 22
//     printf("Immediate: %d\n", isImmediate);

//     uint32_t opcode = (instruction >> 21) & 0x7FF; //look at bits [31:21]

//     switch(opcode) {
//         case 0x588:  // 0x588 => ADDS Immediate
//             d.type = ADDS_IMM;
//             printf("ADDS Immediate\n");
//             break;
//         case 0x6A2:  // 0x6A2 => HLT
//             d.type = HLT;
//             printf("HLT\n");
//             break;
//         // case 0x784:  // here we should detect that it is inmediate annd then use => 0xF1
//         //     d.type = SUBS_IMM;
//         //     printf("SUBS Immediate\n");
//         //     break;
//         case 0x785:  // Suppose your assembler yields this for "subs Xd, Xn, Xm"
//             d.type = SUBS_REG;
//             printf("SUBS Register\n");
//             break;
//         default:
//             d.type = UNKNOWN;
//             printf("Unknown\n");
//             break;
//     }

//     // Now decode common fields (some instructions might ignore them):
//     d.rd = instruction & 0x1F;             // bits [4:0]
//     d.rn = (instruction >> 5) & 0x1F;      // bits [9:5]
//     int imm12 = (instruction >> 10) & 0xFFF;   // bits [21:10]
//     int shiftBits = (instruction >> 22) & 0x3; // bits [23:22]

//     // If it's an immediate instruction with a shift:
//     if (shiftBits == 0b01) {
//         imm12 <<= 12; // e.g. shift left 12 bits
//     }
//     d.imm = imm12;  // store in the struct

//     // If you also need Rm for a register form, you might do something like:
//     // d.rm = (instruction >> 16) & 0x1F; // depends on the exact encoding

//     return d;
// }

// DecodedInstruction decode_instruction(uint32_t instruction) {
//     // We only work with the 64-bit variant of the instruction set.
//     // So the first most significant bit is always 1.(the sf bit)

//     DecodedInstruction d;
//     memset(&d, 0, sizeof(d));

//     // First extract the top byte to determine instruction category
//     // This are the first 8 bits of the instruction
//     uint32_t topByte = (instruction >> 24) & 0xFF;
    
//     // For system instructions like HLT
//     if (topByte == 0xD4) {
//         d.type = HLT;
//         printf("HLT\n");
//     }
//     // Data processing immediate (ADDS/SUBS immediate)
//     else if ((topByte & 0xF0) == 0xF0) {
//         // Check specific bits for immediate operations
//         uint32_t op31_29 = (instruction >> 29) & 0x7; // bits [31:29]
//         uint32_t op25_24 = (instruction >> 24) & 0x3; // bits [25:24]
        
//         if (op31_29 == 0x7 && op25_24 == 0x1) {
//             // This is SUBS immediate
//             d.type = SUBS_IMM;
//             printf("SUBS Immediate\n");
//         } 
//         else if (op31_29 == 0x2 && op25_24 == 0x1) {
//             // This is ADDS immediate
//             d.type = ADDS_IMM;
//             printf("ADDS Immediate\n");
//         }
//     }
    
//     // Data processing register (ADDS/SUBS register)
//     else if ((topByte & 0xFE) == 0xAB) {
//         // This range is for register-based operations
//         uint32_t op31_21 = (instruction >> 21) & 0x7FF;
        
//         if (op31_21 == 0x785) {
//             d.type = SUBS_REG;
//             printf("SUBS Register\n");
//         }
//         // Add other register operations here
//     }
//     else {
//         d.type = UNKNOWN;
//         printf("Unknown\n");
//     }

//     // Now decode common fields (some instructions might ignore them):
//     d.rd = instruction & 0x1F;             // bits [4:0]
//     d.rn = (instruction >> 5) & 0x1F;      // bits [9:5]
    
//     // For immediate instructions
//     if (d.type == SUBS_IMM || d.type == ADDS_IMM) {
//         int imm12 = (instruction >> 10) & 0xFFF;   // bits [21:10]
//         int shift = (instruction >> 22) & 0x3;    // bits [23:22]
        
//         if (shift == 0x1) {
//             imm12 <<= 12; // LSL #12
//         }
//         d.imm = imm12;
//     }
    
//     // For register form instructions
//     if (d.type == SUBS_REG) {
//         d.rm = (instruction >> 16) & 0x1F; // bits [20:16]
//         // Handle shift type and amount if needed
//     }

//     return d;
// }


// First, expand your instruction type enum to include all required types
typedef enum {
    ADDS_IMM,
    ADDS_REG,
    SUBS_IMM,
    SUBS_REG,
    HLT,
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
    CMP_IMM,
    CMP_REG,
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
    d->rd = instruction & 0x1F;
    d->rn = (instruction >> 5) & 0x1F;
    int imm12 = (instruction >> 10) & 0xFFF;
    int shift = (instruction >> 22) & 0x3;
    
    if (shift == 0x1) {
        d->imm = ((int64_t)imm12) << 12;
    } else {
        d->imm = imm12;
    }
    d->shift = shift;
}

void extract_register_fields(uint32_t instruction, DecodedInstruction* d) {
    d->rd = instruction & 0x1F;
    d->rn = (instruction >> 5) & 0x1F;
    d->rm = (instruction >> 16) & 0x1F;
}

// The pattern table - ordered from most specific to least specific
const InstructionPattern patterns[] = {
    // System instructions
    {0xFFFFFC1F, 0xD4400000, HLT, "HLT"},
    
    // Data processing immediate
    {0xFF800000, 0xF1000000, SUBS_IMM, "SUBS Immediate"},
    {0xFF800000, 0xB1000000, ADDS_IMM, "ADDS Immediate"},
    {0xFF800000, 0x91000000, ADD_IMM, "ADD Immediate"},
    {0xFF800000, 0xD1000000, SUB_IMM, "SUB Immediate"},
    {0xFF800000, 0xF1000000, CMP_IMM, "CMP Immediate"}, // Same as SUBS but rd=XZR
    
    // Data processing register
    {0xFFE0FC00, 0xAB000000, ADDS_REG, "ADDS Register"},
    {0xFFE0FC00, 0xEB000000, SUBS_REG, "SUBS Register"},
    {0xFFE0FC00, 0x8B000000, ADD_REG, "ADD Register"},
    {0xFFE0FC00, 0xCB000000, SUB_REG, "SUB Register"},
    {0xFFE0FC00, 0xEB000000, CMP_REG, "CMP Register"}, // Same as SUBS but rd=XZR
    
    // Logical operations
    {0xFFE0FC00, 0xEA000000, ANDS_REG, "ANDS Register"},
    {0xFFE0FC00, 0xCA000000, EOR_REG, "EOR Register"},
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
    DecodedInstruction d;
    memset(&d, 0, sizeof(d));
    
    // Try to match against each pattern
    for (int i = 0; i < PATTERN_COUNT; i++) {
        if ((instruction & patterns[i].mask) == patterns[i].value) {
            d.type = patterns[i].type;
            printf("%s\n", patterns[i].name);
            
            // Extract fields based on instruction type
            switch (d.type) {
                case ADDS_IMM:
                case SUBS_IMM:
                case ADD_IMM:
                case SUB_IMM:
                case CMP_IMM:
                case MOVZ:
                case LSL_IMM:
                case LSR_IMM:
                    extract_immediate_fields(instruction, &d);
                    break;
                    
                case ADDS_REG:
                case SUBS_REG:
                case ADD_REG:
                case SUB_REG:
                case CMP_REG:
                case ANDS_REG:
                case EOR_REG:
                case ORR_REG:
                case MUL:
                    extract_register_fields(instruction, &d);
                    break;
                    
                // Handle special cases
                case B:
                case BEQ:
                case BNE:
                    // Extract branch offset
                    d.imm = ((instruction & 0x3FFFFFF) << 2); 
                    // Sign extend if needed
                    if (d.imm & 0x8000000) d.imm |= 0xFFFFFFFF0000000;
                    break;
                    
                // More special cases as needed
            }
            
            // Special handling for conditions in B.cond
            if (d.type == B_COND) {
                uint32_t cond = instruction & 0xF;
                // Map condition code to specific enum
                // BEQ = 0x0, BNE = 0x1, etc.
            }
            
            return d;
        }
    }
    
    // No match found
    d.type = UNKNOWN;
    printf("Unknown instruction\n");
    return d;
}

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
            printf("Executing ADDS Immediate\n");
            int64_t result = CURRENT_STATE.REGS[d.rn] + d.imm;
            NEXT_STATE.REGS[d.rd] = result;
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result < 0);
            break;
        }

        case HLT: {
            printf("Executing HLT\n");
            RUN_BIT = 0; // Stop execution
            break;
        }

        case SUBS_IMM: {
            // Use d.imm
            int64_t lhs = CURRENT_STATE.REGS[d.rn];
            int64_t rhs = d.imm; 
            int64_t result = lhs - rhs;
            NEXT_STATE.REGS[d.rd] = result;
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result < 0);
            break;
        }

        case SUBS_REG: {
            // Use d.rm
            int64_t lhs = CURRENT_STATE.REGS[d.rn];
            int64_t rhs = CURRENT_STATE.REGS[d.rm];
            int64_t result = lhs - rhs;
            break;
        }
        

        // Add more cases: SUBS_IMM, ADD_IMM, B, etc.
        // Each will interpret d’s fields as needed.

        default:
            printf("Unknown Instruction\n");
            break;
    }

    // The shell’s cycle() will do: CURRENT_STATE = NEXT_STATE;
    printf("Instruction processed\n\n");
}