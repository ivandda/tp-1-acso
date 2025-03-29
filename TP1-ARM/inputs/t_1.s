.text
// Test when equal (should not branch)
mov X1, #5
mov X2, #5
cmp X1, X2      // Sets Z=1
bne fail        // Should not branch
// Test when not equal (should branch)
mov X1, #5
mov X2, #10
cmp X1, X2      // Sets Z=0
bne success     // Should branch

fail:
mov X10, #1     // Failure indicator
HLT 0

success:
mov X10, #0     // Success indicator
HLT 0