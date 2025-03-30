    // cmp_edge.s
    movz    X1, 10
    movz    X2, 10
    // CMP immediate: X1 - 10 = 0 → Zero flag should be set.
    cmp     X1, 10        
    
    // CMP register: X1 - X2 = 0 → Zero flag should be set.
    cmp     X1, X2        
    
    movz    X3, 15
    // CMP immediate: 10 - 15 = -5 → Negative flag should be set.
    cmp     X1, 15        

    hlt 0
