.text
// Test writing to XZR
mov X1, #5
mov X31, #10      // Should be ignored, XZR stays 0
cmp X31, X1       // Comparing 0 with 5, should set N=1, Z=0
blt xzr_works     // Should branch

xzr_fails:
mov X10, #0       // XZR test failed
HLT 0

xzr_works:
// Test CMP using XZR as destination (alias for SUBS)
mov X2, #0
// This is actually CMP instruction under the hood:
subs XZR, X2, #0  // Should set Z=1
beq cmp_works     // Should branch

cmp_fails:
mov X10, #0       // CMP test failed
HLT 0

cmp_works:
mov X10, #1       // All tests passed
HLT 0