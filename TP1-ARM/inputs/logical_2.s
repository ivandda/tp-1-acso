    // logical_edge.s
    movz    X1, 0xFFFF      // Lower 16 bits are ones.
    movz    X2, 0x0F0F      // A repeating pattern.
    
    // ANDS: (0xFFFF & 0x0F0F) = 0x0F0F
    ands    X0, X1, X2      
    
    // EOR: (0xFFFF ^ 0x0F0F)
    eor     X3, X1, X2      
    
    // ORR: (0xFFFF | 0x0F0F) should still be 0xFFFF.
    orr     X4, X1, X2      

    hlt 0
