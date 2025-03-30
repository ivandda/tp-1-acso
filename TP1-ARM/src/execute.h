#ifndef EXECUTE_H
#define EXECUTE_H

#include "decode.h"

void adds_imm(DecodedInstruction d);
void adds_reg(DecodedInstruction d);
void subs_imm(DecodedInstruction d);
void subs_reg(DecodedInstruction d);
void hlt(void);
void cmp_imm(DecodedInstruction d);
void cmp_reg(DecodedInstruction d);
void ands_reg(DecodedInstruction d);
void eor_reg(DecodedInstruction d);
void orr_reg(DecodedInstruction d);
void movz(DecodedInstruction d);
void stur(DecodedInstruction d);
void sturh(DecodedInstruction d);
void sturb(DecodedInstruction d);
void lsl_imm(DecodedInstruction d);
void lsr_imm(DecodedInstruction d);
void ldur(DecodedInstruction d);
void ldurh(DecodedInstruction d);
void ldurb(DecodedInstruction d);
void add_imm(DecodedInstruction d);
void add_reg(DecodedInstruction d);
void beq(DecodedInstruction d);
void bne(DecodedInstruction d);
void bgt(DecodedInstruction d);
void blt(DecodedInstruction d);
void bge(DecodedInstruction d);
void ble(DecodedInstruction d);
void b(DecodedInstruction d);
void br(DecodedInstruction d);
void mul(DecodedInstruction d);
void cbz(DecodedInstruction d);
void cbnz(DecodedInstruction d);

void update_flags(int64_t result, int updateFlags);

#endif
