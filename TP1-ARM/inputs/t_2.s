.text
// Test comparing with maximum positive value
movz X1, #0xFFFF, LSL 48
movz X2, #0xFFFF, LSL 32
orr X1, X1, X2
movz X2, #0xFFFF, LSL 16
orr X1, X1, X2
movz X2, #0xFFFF
orr X1, X1, X2    // X1 now holds 0xFFFFFFFFFFFFFFFF

cmp X1, #0        // Compare with 0, should set N=1, Z=0
blt negative      // Should branch

positive:
mov X10, #0       // Incorrect path
HLT 0

negative:
// Test comparing with zero
mov X2, #0
cmp X2, X2        // Should set Z=1
beq equal         // Should branch

not_equal:
mov X10, #0       // Incorrect path
HLT 0

equal:
mov X10, #1       // Success
HLT 0