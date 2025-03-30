    // subs_imm_edge.s
    movz    X1, 5            // X1 = 5
    // 5 - 5 = 0; expect FLAG_Z to be set.
    subs    X0, X1, 5        
    
    // 5 - 10 = -5; expect FLAG_N to be set.
    subs    X2, X1, 10       

    hlt 0
