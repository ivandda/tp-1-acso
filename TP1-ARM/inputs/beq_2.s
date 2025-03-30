    // beq.s - Branch if Equal test
    // Set up registers so that the condition is met
    movz    X1, 0x1       // X1 = 1
    movz    X2, 0x1       // X2 = 1, so X1 == X2
    cmp     X1, X2        // Compare X1 and X2; flags updated
    beq     equal_label   // Branch should be taken since X1 == X2

    // This part should be skipped if BEQ works correctly.
    movz    X0, 0x0       // X0 = 0 (if branch not taken, error)
    b       end

equal_label:
    movz    X0, 0xdead    // X0 = 0xdead to indicate branch taken

end:
    hlt   0             // Halt the simulator
