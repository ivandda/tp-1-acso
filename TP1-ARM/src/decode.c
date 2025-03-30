#include "decode.h"
#include <stdio.h>
#include <string.h>

// Instructions patterns 
// Ordered from "bigger" to "smaller" masks to avoid false positives
// The order of the patterns is important, as the first match will be used
const InstructionPattern patterns[] = {
    // System instructions (29 bits)
    {0xFFFFFC1F, 0xD4400000, HLT, "HLT"},
    {0xFFFFFC1F, 0xD61F0000, BR, "BR"},
    
    // Register operations with specific flags (21 bits)
    {0xFFE0FC1F, 0xEB00001F, CMP_REG, "CMP Register"},
    
    // Register data processing (18 bits)
    {0xFFE0FC00, 0xEB000000, SUBS_REG, "SUBS Register"},
    {0xFFE0FC00, 0xAB000000, ADDS_REG, "ADDS Register"},
    {0xFFE0FC00, 0x8B000000, ADD_REG, "ADD Register"},
    {0xFFE0FC00, 0xEA000000, ANDS_REG, "ANDS Register"},
    {0xFFE0FC00, 0xCA000000, EOR_REG, "EOR Register"},
    {0xFFE0FC00, 0xAA000000, ORR_REG, "ORR Register"},
    {0xFFE0FC00, 0x9B000000, MUL, "MUL"},
    
    // Immediate operations with specific flags (13 bits)
    {0xFF80001F, 0xF100001F, CMP_IMM, "CMP Immediate"},
    
    // Memory operations (12 bits)
    {0xFFC00000, 0xF8000000, STUR, "STUR"},
    {0xFFC00000, 0x38000000, STURB, "STURB"},
    {0xFFC00000, 0x78000000, STURH, "STURH"},
    {0xFFC00000, 0xF8400000, LDUR, "LDUR"},
    {0xFFC00000, 0x38400000, LDURB, "LDURB"},
    {0xFFC00000, 0x78400000, LDURH, "LDURH"},
    
    // Data processing with shifts (12 bits)
    {0xFFC00000, 0xD3400000, LSL_IMM, "LSL Immediate"},
    {0xFFC00000, 0xD3800000, LSR_IMM, "LSR Immediate"},
    
    // Immediate data processing (9 bits)
    {0xFF800000, 0xF1000000, SUBS_IMM, "SUBS Immediate"},
    {0xFF800000, 0xB1000000, ADDS_IMM, "ADDS Immediate"},
    {0xFF800000, 0x91000000, ADD_IMM, "ADD Immediate"},
    {0xFF800000, 0xD2800000, MOVZ, "MOVZ"},
    
    // Branch operations (9-6 bits)
    {0xFF000010, 0x54000000, B_COND, "B.cond"},
    {0x7F000000, 0x34000000, CBZ, "CBZ"},
    {0x7F000000, 0x35000000, CBNZ, "CBNZ"},
    {0xFC000000, 0x14000000, B, "B"}
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
            
            d.type = patterns[i].type;
            printf("Detected Instruction: %s\n", patterns[i].name);
            
            // Extract fields based on instruction type
            switch (d.type) {
                case ADDS_IMM:
                case SUBS_IMM:
                case ADD_IMM:
                case ADD_REG:
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
                case ADDS_REG:
                case CMP_REG:
                case ANDS_REG:
                case EOR_REG:
                case ORR_REG:
                case MUL:
                    extract_register_fields(instruction, &d);
                    break;

                case B:
                extract_b_fields(instruction, &d);
                break;
                case BR:
                    extract_br_fields(instruction, &d);
                    break;
                case CBZ:
                case CBNZ:
                extract_cb_fields(instruction, &d);
                break;
            }

            if (d.type == B_COND) {
                extract_bcond_fields(instruction, &d);
                
                switch (d.cond) {
                    case 0x0: d.type = BEQ; break; // Z == 1
                    case 0x1: d.type = BNE; break; // Z == 0
                    case 0xa: d.type = BGE; break; // N == V (but V=0 => N=0 => X1 >= X2)
                    case 0xb: d.type = BLT; break; // N != V (V=0 => N=1 => X1 < X2)
                    case 0xc: d.type = BGT; break; // Z==0 && N==V => (Z=0 && N=0 => X1 > X2)
                    case 0xd: d.type = BLE; break; // Z==1 || N!=V => (Z=1 || N=1 => X1 <= X2)
                    default:
                        printf("Unsupported condition code: 0x%x\n", d.cond);
                        d.type = UNKNOWN;
                        break;
                }
            }
            
            return d;
        }
    }
    
    d.type = UNKNOWN;
    printf("Unknown instruction\n");
    return d;
}

// Field extraction functions (to be called after matching a pattern)

void extract_immediate_fields(uint32_t instruction, DecodedInstruction* d) {
    //     Las instrucciones LSL y LSR, y como mencionamos en el punto 4 ADDS y SUBS usan shift. Para todas las demás instrucciones, pueden asumir que el shift (o shift amount [shamt]) es cero.
        
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
    
    void extract_bcond_fields(uint32_t instruction, DecodedInstruction* d) {
        // Extract the 4-bit condition from bits [3:0]
        d->cond = instruction & 0xF;
        
        // Extract the 19-bit signed immediate from bits [23:5]
        int32_t imm19 = (instruction >> 5) & 0x7FFFF;
        
        // Sign-extend if necessary
        if (imm19 & (1 << 18)) {
            imm19 |= ~((1 << 19) - 1);
        }
        
        // Each B.cond offset is left-shifted by 2
        imm19 <<= 2;
        d->imm = imm19;
    }
    
    // Extract branch fields for B instruction
    void extract_b_fields(uint32_t instruction, DecodedInstruction* d) {
        // Extract imm26 from bits [25:0]
        int32_t imm26 = instruction & 0x3FFFFFF;
        
        // Sign-extend if necessary (if bit 25 is set)
        if (imm26 & 0x2000000) {
            imm26 |= 0xFC000000;  // Extend with 1s
        }
        
        // Shift left by 2 bits (multiply by 4) for word alignment
        imm26 <<= 2;
        
        d->imm = imm26;
    }
    
    // Extract CBZ/CBNZ fields
    void extract_cb_fields(uint32_t instruction, DecodedInstruction* d) {
        d->rd = instruction & 0x1F;
        
        // Extract imm19 from bits [23:5]
        int32_t imm19 = (instruction >> 5) & 0x7FFFF;
        
        // Sign-extend if necessary (if bit 18 is set)
        if (imm19 & 0x40000) {
            imm19 |= 0xFFF80000;  // Extend with 1s
        }
        
        // Shift left by 2 bits for word alignment
        imm19 <<= 2;
        
        d->imm = imm19;
    }
    
    void extract_br_fields(uint32_t instruction, DecodedInstruction* d) {
        // Extract the register number from bits [4:0]
        d->rn = instruction & 0x1F;
    }