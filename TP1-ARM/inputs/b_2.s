// branch_edge.s
// Set up equality condition
movz    X1, 5
movz    X2, 5
cmp     X1, X2         // Should set Z flag.
beq     label_equal    // Branch should be taken.
movz    X0, 0          // If not taken, error indicator.
b       end
label_equal:
movz    X0, 0xdead     // Correct branch indicator.

// Test BNE: set registers to different values.
movz    X3, 5
movz    X4, 10
cmp     X3, X4         // Not equal.
bne     label_notequal // Should branch.
movz    X5, 0          // Error if not branched.
label_notequal:
movz    X5, 0xbeef     // Correct indicator.

// Test an unconditional branch (B) that jumps backward.
b       branch_back
movz    X6, 0          // This should be skipped.
branch_back:
movz    X6, 1          // Branch target.

hlt 0
