Live Load {#tg_live_load}
======================================
In addition to supporting the HL93 live load as defined in LRFD 3.6.1.3, you can apply your own live loads and envelopes of live loads. Live loads can be defined for three different load categories:

1. Design Limit States
    * Service I
    * Service III
    * Strength I 

2. Fatigue Limit States
    * Service IA/Fatigue I

3. Permit Limit State
    * Strength II

The responses for the vehicles defined in each category are enveloped and these envelopes are used to compute the associated limit state responses.

> NOTE: Multiple lane distribution factors are used for both design limit states and permit limit states. We know that some agencies use single-lane distribution factors and others combine permit and design responses in accordance with LRFD 4.6.2.2.5. Currently the only work-around for single-lane factors is to specify user-input distribution factors. No work-around currently exists for LRFD 4.6.2.2.5. **If you would like to add direct support for LRFD 4.6.2.2.5, please consider joining our development team**

