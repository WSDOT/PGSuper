Moment Capacity {#ug_library_dialogs_project_criteria_moment_capacity}
==============================================
Define project criteria for computing moment capacity.


Moment Capacity
----------------

Item | Description
-----|-------------
Include girder longitudinal mild reinforcement in capacity calculations | When checked, the mild steel longitudinal reinforcement in the girder will be included in the capacity analysis. Prestressing strand is always included the capacity analysis. For bridges without a cast-in-place deck or overlay, this setting is ignored and mild steel longitudinal reinforcement in the girder will always be included in the negative moment capacity analysis. Without a deck structure, the girder is the only source of reinforcement for negative moment capacity.
Modulus of rupture for cracking moment (LRFD 5.4.2.6, 5.6.3.3 (*pre-2017: 5.7.3.3.2*)) | Enter the coefficient for the modulus of rupture for computing cracking moment.

Resistance Factors (LRFD 5.5.4.2)
---------------------
Define the resistance factors

### Conventional Construction (LRFD 5.5.4.2) ###
Define the resistance factors for conventional construction for normal weight and lightweight concrete.

### Closure Joint (LRFD 5.5.4.2 and 5.12.3.4.2d (*pre-2017: 5.14.1.3.2d*)) ###
Define the resistance factors for cast-in-place closure joints for normal weight and lightweight concrete.

Negative Moment Capacity
------------------------
Deck longitudinal reinforcement provides all tensile section forces in the negative moment region of the structure. This option specifies whether negative moments due to non-composite loads (such as girder and deck dead load) are included in Mu. Some bridge owners have a policy to consider only moments due to superimposed dead loads when evaluating negative moments. You will see the effect of this setting in the Strenght I Deck Moment in the Moments, Shears, and Reactions chapter of the Details Report as well as the ultimate moment, Mu, in the negative moment specification checks.

> TIP: Refer to the @ref tg_moment_capacity in the technical guide for more information about how moment capacities are computed.

