Bearing Shear Deformation {#tg_bearing_shear_deformation}
======================================

In PGSuper, bearing shear deformation is determined by computing the horizontal displacement of the bottom of the girder at the bearing location from a point of longitudinal fixity in the superstructure. This displacement is found by computing the factored thermal movement range combined with the movements caused by creep, shrinkage, and relaxation. For this calculation, it is assumed that the pier supporting the bearing is rigid and there is no slip between the bearing and girder. 

Point of Longitidunal Fixity
----------------------------
In PGSuper, points of longitudinal fixity in the superstructure are assumed to be at supports that have fixed longitudinal translation. If all supports are defined with free longitudinal translation (e.g. all rollers), then the point of longitudinal fixity is taken to be at the first pier, ahead on station, that is at or adjacent to the center of the bridge (where the center of the bridge is either at the central pier for girders with even numbers of spans, or at midspan of the central span for girders with odd numbers of spans).

>NOTE: The above computation of the location of the point of longitudinal fixity can be used for girders with even numbers of spans. For girders with odd spans, it is recommended to take the point of longitudinal fixity to be at midspan of the central span, then compute the shear deformation by hand.

### Factored Thermal Movement
-----------------------------

The following formula is used to compute the factored thermal movement:

$\Delta_{\text{thermal}} = \Delta_0 \times \alpha \times L_{pf} \times (T_{\max} - T_{\min})$

where

$\Delta_0$ = thermal movement factor

$\alpha$ = coefficient of thermal expansion

$L_{pf}$ = distance from bearing to adjacent point of longitudinal fixity

$(T_{\max} - T_{\min})$ = temperature range based on Procedure A (Article 3.12)

### Creep, Shrinkage and Relaxation
------------------------------------

For refined loss methods, the contribution of shear deformation provided by time-dependent effects are accounted for by computing the shortening of the bottom flange due to change in length of prestressing steel caused by creep, shrinkage, and relaxation using the following formula (PCI BDM Eq. 10.8.3.8.2-6):

$\Delta_s = -\Delta L_{bf} = -\frac{\left(1 + \dfrac{e_p y_b}{r ^ 2} \right)}{\left(1 + \dfrac{e_p ^ 2}{r ^ 2} \right)} \left(\Delta L_{ten} \right)$

For the timestep method, bottom-flange shortening is computed using the following formula which sums the contributions of strain-induced deformations over the discrete segments along the length of the girder from the point of longitudinal fixity:

$\Delta_s = \sum_{i=1}^{n} \epsilon_{\text{avg},i} \cdot \Delta x_i$

where

$\epsilon_{\text{avg}}$ = average strain for each segment determined using the midpoint rule, which approximates the strain at the midpoint of the segment as representative value for that segment.

$\Delta x$ = distance between the consecutive points of interest along the girder which is multiplied by the average strain to calculate the deformation contribution from that segment.










