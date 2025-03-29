.text
// Test if Z flag persists correctly
mov X1, #5
mov X2, #5
cmp X1, X2        // Sets Z=1
mov X3, #10       // Should not affect Z flag
beq equal_path    // Should branch
b fail            // Should not reach here

equal_path:
// Test if N flag is set correctly
mov X1, #5
mov X2, #10
cmp X1, X2        // Sets N=1 (if signed comparison)
blt less_path     // Should branch
b fail            // Should not reach here

less_path:
// Test if correct flags are used in BLE
mov X1, #10
mov X2, #10
cmp X1, X2        // Sets Z=1, N=0
ble le_path       // Should branch (Z=1)
b fail            // Should not reach here

le_path:
mov X10, #1       // Success
HLT 0

fail:
mov X10, #0       // Failure
HLT 0