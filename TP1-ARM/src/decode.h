#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>

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
    
    // SUB_IMM,
    // SUB_REG,
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

    int cond;             // For branch conditions (EQ=0, NE=1, etc.)

} DecodedInstruction;


typedef struct {
    uint32_t mask;          // Bits that matter for identification
    uint32_t value;         // Expected values for those bits
    InstructionType type;   // The instruction type this pattern represents
    const char* name;       // Human-readable name for debugging
} InstructionPattern;


DecodedInstruction decode_instruction(uint32_t instruction);
void extract_immediate_fields(uint32_t instruction, DecodedInstruction* d);
void extract_register_fields(uint32_t instruction, DecodedInstruction* d);
void extract_movz_fields(uint32_t instruction, DecodedInstruction* d);
void extract_shift_fields(uint32_t instruction, DecodedInstruction* d);
void extract_memory_fields(uint32_t instruction, DecodedInstruction* d);
void extract_bcond_fields(uint32_t instruction, DecodedInstruction* d);
void extract_b_fields(uint32_t instruction, DecodedInstruction* d);
void extract_cb_fields(uint32_t instruction, DecodedInstruction* d);
void extract_br_fields(uint32_t instruction, DecodedInstruction* d);


#endif