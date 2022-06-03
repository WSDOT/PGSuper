Moment Capacity {#ug_library_dialogs_project_criteria_moment_capacity}
==============================================
Define project criteria for computing moment capacity.

> TIP: Refer to the @ref tg_moment_capacity in the technical guide for more information about how moment capacities are computed.

Moment Capacity
----------------

Item | Description
-----|-------------
Include pretensioned strand in negative moment capacity calculations | When checked, the pretensioned strands will be included in the negative moment capacity analysis. Typically the strands are neglected, however for cantilevered girders, the strands may provide significant capacity and should be included in the analysis.
Include girder longitudinal mild reinforcement in capacity calculations | When checked, the mild steel longitudinal reinforcement in the girder will be included in the capacity analysis. Prestressing strand is always included the capacity analysis. For bridges without a cast-in-place deck or overlay, this setting is ignored and mild steel longitudinal reinforcement in the girder will always be included in the negative moment capacity analysis. Without a deck structure, the girder is the only source of reinforcement for negative moment capacity.
Consider reinforcement strain limits in capacity analysis | When checked, the reinforcement strain in the reinforcing steel and prestressed reinforcement are evaluated against the material specifications referenced by AASHTO LRFD 5.4.3 and 5.4.4, respectively. See additional discussion below.
Number of "slices" for strain compatibility analysis | Enter the number of elements the composite girder cross section is divided into for the strain compatibility analysis. The number of slices must be between 10 and 100. The larger numbers provide more accurate results but take longer to compute. The actual number of slices used in the strain compatibility analysis will differ from this value. Slices that cross the neutral axis will be subdivided. Slices for reinforcement will overlap concrete slices.
Modulus of rupture for cracking moment (LRFD 5.4.2.6, 5.6.3.3 (*pre-2017: 5.7.3.3.2*)) | Enter the coefficient for the modulus of rupture for computing cracking moment.

Resistance Factors (LRFD 5.5.4.2)
---------------------
Define the resistance factors

### Conventional Construction (LRFD 5.5.4.2) ###
Define the resistance factors for conventional construction for normal weight, lightweight concrete, and UHPC.

### Closure Joint (LRFD 5.5.4.2 and 5.12.3.4.2d (*pre-2017: 5.14.1.3.2d*)) ###
Define the resistance factors for cast-in-place closure joints for normal weight, lightweight concrete, and UHPC.

Negative Moment Capacity
------------------------
Deck longitudinal reinforcement provides all tensile section forces in the negative moment region of the structure. This option specifies whether negative moments due to non-composite loads (such as girder and deck dead load) are included in Mu. Some bridge owners have a policy to consider only moments due to superimposed dead loads when evaluating negative moments. You will see the effect of this setting in the Strenght I Deck Moment in the Moments, Shears, and Reactions chapter of the Details Report as well as the ultimate moment, Mu, in the negative moment specification checks.


Reinforcement strain limits
---------------------------
**Reinforcing Steel**

AASHTO LRFD 5.4.3 requires that reinforcing steel conform to the material standards specifed in Article 9.2 of the AASHTO LRFD Bridge Construction Specifications. Article 9.2.1 of the construction specifications requires uncoated reinforcing conform to one of the following specifications:
  * AASHTO M31M/M31 (ASTM A615/A615M)
  * AASHTO M322M/M322 (ASTM A996/A996M)
  * ASTM A706/A706M
  * ASTM A1035/A1035M (See Article 9.2.4)
  * and others not currently supported by PGSuper/PGSplice
  
The minimum required elongation of these materials range between 6% and 12% depending on Grade and Bar Size.

**Prestressing Strand**

AASHTO LRFD 5.4.4 requires that prestressing strand conform to the material strandard AASHTO M203/M203M (ASTM A416/A416M). The minimum required elongation is 3.5%.

**Moment Capacity**

The moment capacity analysis first assumes crushing of the extreme compression fiber at a strain of 0.003 controls the capacity of the section. The strain in the reinforcement is assumed to be adequate, i.e. the reinforcement does not debond or pull out from the surrounding concrete matrix and it does not fracture. 

When the reinforcement strain limits are considered in the moment capacity analysis the strain in the reinforcement is compared to the minimum elongation provided in the material specification. If the strain exceeds the minimum elongation the reinforcement, fracture of the reinforcement becomes the controlling condition of the section capacity and the concrete crushing assumption is no longer valid. The moment capacity analysis is then repeated using the reinforcement strain as the controlling condition. The resulting strain in the extreme compression fiber will be less than 0.003.

The Moment Capacity Details chapter in the Details Report provides a notation that indicates which element of the cross section controlled the capacity. The Moment Capacity Details Report provides the complete details of the strain compatibility analysis.
