#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"

typedef enum {
    ADDS_IMM,
    HLT,
    UNKNOWN,
} optcodeType;


// // Function to get the instruction name based on the opcode
// const char* get_instruction_name(uint32_t opcode) {
//     switch (opcode) {
//         case 0x588:  // For ADDS Immediate (from sample 0xB1000540)
//             return "ADDS Immediate";
//         case 0x6A2:  // For HLT (from sample 0xD4400000)
//             return "HLT";
//         // Add more cases for other opcodes as needed
//         default:
//             return "Unknown";
//     }
// }

const optcodeType get_optocode_form_instruction(uint32_t instruction) {
    uint32_t opcode = (instruction >> 21) & 0x7FF; // Extract bits [31:21]
    switch (opcode) {
        case 0x588:  // For ADDS Immediate (from sample 0xB1000540)
            return ADDS_IMM;
        case 0x6A2:  // For HLT (from sample 0xD4400000)
            return HLT;
        // Add more cases for other opcodes as needed
        default:
            return UNKNOWN;
    }
}

const char* get_instruction_name(optcodeType opcode) {
    switch (opcode) {
        case ADDS_IMM:
            return "ADDS Immediate";
        case HLT:
            return "HLT";
        default:
            return "Unknown";
    }
}

// void process_instruction() {
//     /* execute one instruction here. You should use CURRENT_STATE and modify
//      * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
//      * access memory. */
    
//     // 1. Fetch instruction from memory
//     uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
//     printf("Instruction: 0x%08X\n", instruction);
    
//     // 2. Increment PC (next instruction)
//     NEXT_STATE.PC = CURRENT_STATE.PC + 4;

//     // 3. Decode the instruction
//     uint32_t opcode = (instruction >> 21) & 0x7FF; // Extract bits [31:21]
//     printf("Opcode: 0x%03X\n", opcode);
//     printf("Instruction Name: %s\n", get_instruction_name(opcode));
    
//     // Extract additional fields common to several instructions
//     int rd = instruction & 0x1F;               // Destination register (bits [4:0])
//     int rn = (instruction >> 5) & 0x1F;          // First operand register (bits [9:5])
//     int imm12 = (instruction >> 10) & 0xFFF;     // Immediate value (bits [21:10])
//     int shift = (instruction >> 22) & 0x3;       // Shift (bits [23:22])

//     // Apply shift if necessary (for immediate instructions)
//     if (shift == 0b01) {
//         imm12 <<= 12; // Shift imm12 left by 12 bits
//     }

//     // 4. Execute the instruction based on the opcode
//     if (opcode == 0x588) { // ADDS Immediate (detected opcode 0x588)
//         printf("Detected ADDS Immediate opcode\n");
//         NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm12;

//         // Update flags: Zero flag and Negative flag
//         NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[rd] == 0);
//         NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[rd] < 0);
//     }
//     else if (instruction == 0xD4400000) { // HLT (Halt)
//         printf("Detected HLT opcode\n");
//         RUN_BIT = 0; // Stop execution
//     }
//     // You can add more instructions here...
// }

void process_instruction(){
    printf("-------------------------- Processing instruction --------------------------\n\n");
    // 1. Fetch instruction from memory
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    printf("Instruction: 0x%08X\n", instruction);
    
    // 2. Increment PC (next instruction)
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    // 3. Decode the instruction
    optcodeType opcode = get_optocode_form_instruction(instruction);
    printf("Instruction Name: %s\n", get_instruction_name(opcode));
    
    // Extract additional fields common to several instructions
    int rd = instruction & 0x1F;               // Destination register (bits [4:0])
    int rn = (instruction >> 5) & 0x1F;          // First operand register (bits [9:5])
    int imm12 = (instruction >> 10) & 0xFFF;     // Immediate value (bits [21:10])
    int shift = (instruction >> 22) & 0x3;       // Shift (bits [23:22])

    // Apply shift if necessary (for immediate instructions)
    if (shift == 0b01) {
        imm12 <<= 12; // Shift imm12 left by 12 bits
    }

    // 4. Execute the instruction based on the opcode
    if (opcode == ADDS_IMM) { // ADDS Immediate (detected opcode 0x588)
        printf("Executing ADDS Immediate\n");
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm12;

        // Update flags: Zero flag and Negative flag
        NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[rd] == 0);
        NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[rd] < 0);
    }
    else if (opcode == HLT) { // HLT (Halt)
        printf("Executing HLT\n");
        RUN_BIT = 0; // Stop execution
    }
    // You can add more instructions here...
    printf("Instruction processed\n\n");
}