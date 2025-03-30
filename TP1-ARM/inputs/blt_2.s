    // blt.s - Branch if Less Than test
    // Set up registers so that X1 < X2
    movz    X1, 0x1       // X1 = 1
    movz    X2, 0x2       // X2 = 2, so X1 < X2
    cmp     X1, X2        // Compare X1 and X2; N flag should be set if less
    blt     less_label    // Branch if X1 < X2

    // If branch is not taken (error)
    movz    X0, 0x0       
    b       end

less_label:
    movz    X0, 0xbeef    // X0 = 0xbeef indicates branch taken

end:
    hlt   0
