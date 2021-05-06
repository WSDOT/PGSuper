Anchor Set and Seating Losses {#tg_anchor_set}
==============================
After tendons are stressed they are anchored with a conical wedge system. The tendon retracts when it is released and pulls the wedges into the anchorage device. This forces the wedges together and locks the tendon in place. 

The tendon stress after seating losses is show below
![](TendonStress.png)

The stress loss due to seating is computed using an iterative process. 
1. Estimate the length of the region affected by the tendon retraction, \f$ L_{set} \f$.
2. Compute the stress in the tendon at \f$ L_{set}, f_a \f$.
3. Compute the retraction of the tendon \f$ \Delta_{set} =  \frac{1}{E_p} \displaystyle\int_{}^{}  2(f_{px}-f_{a})\, dx\ \f$.
4. Revise the estimate of \f$ L_{set} \f$ and repeat until the computed \f$ \Delta_{set} \f$ is equal to the specified anchor set.

The figure below illustrates the parameters used to compute \f$ \Delta_{set} \f$.
![](AnchorSet.png)

The seating loss is computed as \f$ \Delta f_{pA} = 2 ( f_{pj} - f_{a} ) \f$.

