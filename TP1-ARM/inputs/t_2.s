// Modified test file that only uses MOVZ with hw=0
// Tests comparison and branching instructions

// Initialize with values within hw=0 range
movz X1, #0x0       // X1 = 0
movz X2, #0x1       // X2 = 1

// Test negative case (X1-X2 should be negative)
subs X3, X1, X2     // X3 = X1-X2, sets N=1 since result is negative
blt negative        // Should branch to negative because N=1

positive:
movz X10, #0        // Incorrect path
b end

negative:
// Test zero case
movz X4, #0         
movz X5, #0
cmp X4, X5          // Compare equal values, sets Z=1
beq equal           // Should branch to equal because Z=1

not_equal:
movz X10, #0        // Incorrect path
b end

equal:
movz X10, #1        // Success path - should reach here
b end

end:
HLT #0