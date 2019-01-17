Live Load Deflection {#tg_live_load_deflection}
======================================
Live load deflections are computed using the loading specified in LRFD 3.6.1.3.2 and evaluated, at the designer's option, using the critieria specified in LRFD 2.5.2.6.2. 

In accordance with LRFD 2.5.2.6.2, all design lanes should be loaded and all supporting components should be assomed to deflect equally. Since girder line analysis is performed, stiffness and loading is proportioned to satisify LRFD 2.5.2.6.2. Live load is applied to a girder line with a stiffness equivalent to composite stiffness of the entire bridge structure and a live load distribution which results in equal distribution to all girders.

The girder line stiffness is taken as the stiffness of the entire bridge cross section at mid-span, which includes the deck, girders and barriers (if structurally continuous), divided by the number of girders. The slab haunch fillet and slab offset ("A" dimension) are ignored (the deck is placed directly atop the girders). 

> EI = (Bridge EI)/(Number of Girders)

The girder line loading is that of all lanes loaded equally divided to all the girders. The live load is modeled as a distribution factor multiplied by a single lane of live load. The distribution factor is the product of the multiple presence factor and the number of design lanes divided by the number of girders

> LLDF = (MPF)(Number of Lanes)/(Number of Girders)

> Live Load applied to girder line model = (LLDF)(Single Lane Live Load per LRFD 3.6.1.3.2)

