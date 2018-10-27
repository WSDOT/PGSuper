Anchor Set and Seating Losses {#tg_anchor_set}
==============================
After tendons are stressed they are anchored with a conical wedge system. The tendon retracts when it is released and pulls the wedges into the anchorage device. This forces the wedges together and locks the tendon in place. 

The tendon force after seating losses is show below
![](TendonStress.png)

The stress loss due to seating is computed using an iterative process. 
1. Estimate the length of the region affected by the tendon retraction, L<sub>set</sub>
2. Get the force in the tendon at L<sub>set</sub>, P<sub>a</sub>
3. Compute the length of the affected region as ![](LsetEquation.png)
4. Revise the estimate of L<sub>set</sub> and repeat until convergence

The figure below illustrates the parameters used to compute L<sub>set</sub>
![](AnchorSet.png)

The seating loss is computed as
![](AnchorSetEquation.png)
