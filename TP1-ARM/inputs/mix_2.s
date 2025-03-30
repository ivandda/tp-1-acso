// complex_mix.s
movz    X1, 10
movz    X2, 20
add     X3, X1, X2        // X3 = 30
subs    X4, X3, 15        // X4 = 15
cmp     X4, 15            // Should set Z flag (15 - 15 = 0)
beq     label_success
movz    X0, 0             // Error if branch not taken
b       end
label_success:
movz    X0, 0xBEEF        // Indicator of success

// Test a backward branch loop: decrement X5 until zero.
movz    X5, 3
loop:
cmp     X5, 0
beq     end_loop
subs    X5, X5, 1
b       loop
end_loop:
hlt 0
