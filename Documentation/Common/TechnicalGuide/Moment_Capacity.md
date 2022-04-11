Moment Capacity {#tg_moment_capacity}
======================================
Moment capacity is computed using a non-linear strain-compatibility methodology. Non-linear material models are used for concrete materials, prestressing strands, and mild reinforcement.

A non linear approach is used in lieu of the AASHTO approaches because the equations provided in Section 5.6.3.1.1 are not applicable to all situations (see C5.6.3.1.1), the AASHTO equations neglect the contribution of the top flange of I-Beams, and the research done in Reference 1 (below) has shown that for T-beams with different concrete strengths in the flange and web, the traditional equivalent rectangular concrete compressive stress distribution does not provide a reliable estimate of flexural strength.

Theory
----------
The basic theory and assumptions for the strain compatibility analysis are:
1. The strength design of flexural members is based on satisfaction of applicable conditions of equilibrium and compatibility of strains.
2. Strain in the reinforcement and concrete is assumed to be directly proportional to the distance from the neutral axis.
3. The maximum useable strain at the extreme concrete compression fiber is 0.003.
4. The maximum useable strain in prestressing strand is 0.035 as defined in ASTM A416.
5. For positive moment analysis; the effects of non-prestressed (mild steel) reinforcement are included in the analysis at the user's option. The area of girder rebar is adjusted for lack of development. Deck rebar is ignored
6. For negative moment analysis; the effects of non-prestressed (mild steel) Deck reinforcement are included in the analysis. Rebar in the girder is included at the user's option.
7. The tensile strength of concrete is neglected.
8. The non-linear concrete stress-strain relationship is taken from Reference 2.
9. The material model of stress-strain behavior for low relaxation and stress relieved prestressing strands utilizes the "power formula" taken from the PCI Bridge Design Manual.
10. The distance c from the fiber of maximum strain to the neutral axis is measured in a direction perpendicular to that axis.

NOTE: Mild steel reinforcement is only used in the capacity analysis if permitted by the Project Criteria. For bridge systems without a cast in place deck or overlay, the mild reinforcement in the girder is used in the negative moment capacity analysis, regardless of the settings in the Project Criteria.

Solution Method
----------------
The strain compatibility analysis is carried out by dividing the beam section into slices, computing the strain at the centroid of each slice, and computing the stress at the centroid. The stress field is then integrated over the girder section to determine the resultant axial force and moment. This process is repeated for different locations of the neutral axis until equilibrium of internal and external forces is acheived.

At the conclusion of the analysis, the strain in the reinforcement is compared to the minimum required elongation specified in the applicable ASTM. If the strain in the reinforcement exceeds the minimum required elogation the moment capacity is over predicted. The strain at the level of the reinforcement is fixed at the minimum elongation and the capacity analysis is repeated. The resulting strain in the concrete will be less than 0.003 and the rectangular stress distribution approximation from the AASHTO specifications cannot be used. This provides further justification for the strain-compatibility solution.

Ultimate moment capacity is computed relative to the condition of zero strain in the non-prestressed materials in the cross section. The following flow chart illustrates the high level procedure for this method:

![](MomentCapacityFlowchart.jpg)

The figure below shows the equilibrium state from which the nominal moment capacity is obtained:

![](Equilibrium.gif)

 

The strain <span style="font-family:Symbol">e</span><sub>2</sub> is the strain in the prestressing steel at the effective level of prestress. The strain <span style="font-family:Symbol">e</span><sub>1</sub> is the strain associated with the flexure of the cross section. The resultant compressive force, C, acts at the centroid of the compression region. The tensile force T, acts at the location of the resultant of the strand forces.

> NOTE: <i>d<sub>e</sub></i> is the "effective depth from the extreme compression fiber to the centroid of the tensile force in the tensile reinforcement". Strands that are further away from the extreme compression fibers are strained more that those strands that are closer. This results in the centroid of the resultant tensile force being lower in the cross section than the geometric centroid of the prestressing strands.

Cross Section Geometry and Materials Used for Calculating Capacities
---------------------------------------------------------------------
The cross sectional geometry used for calculating the moment capacity of a girder is the same as that used for calculating section properties (Refer to @ref tg_section_properties ). Mild steel longitudinal reinforcement in the girder and in the deck is not considered in the capacity calculation.

Strain in the Prestressing Steel
--------------------------------
The initial strain in the prestressing steel, <span style="font-family:Symbol">e</span><sub>2</sub>, is computed as <i>f<sub>pe</sub>/E<sub>ps</sub></i>.

Stress in the Prestressing Strand
--------------------------------
As mentioned above, the stress/strain material model for the prestressing steel is computed using the "power method" where  <span style="font-family:Symbol">e</span><sub>ps</sub> =  <span style="font-family:Symbol">e</span><sub>1</sub> + <span style="font-family:Symbol">e</span><sub>2</sub>. The equation, and a graph, for Grade 270 steel are given below.

![](PowerFormula.gif)

For sections where strand are not fully developed, the stress determined by the power formula is reduced as specified in AASHTO LRFD 5.9.4.3.2.

Stress in the Mild Reinforcement
----------------------------------
The stress/strain material model for mild reinforcement is the traditional elastic-perfectly plastic model.

> NOTE: mild reinforcement is only used in the capacity analysis if permitted by the Project Criteria.
> NOTE: for bridge systems without a cast in place deck or overlay, the mild reinforcement in the girder is used in the negative moment capacity analysis, regardless of the settings in the Project Criteria.

Stress in the Concrete
-------------------------------
The stress in the concrete, f'c, is determined from the formulas shown below (see Reference 2):
![](ConcreteFunction.png)


Maximum Reinforcement Limits
----------------------------
> NOTE: The information in this section is only applicable to PGSuper projects with the Design Criteria based on the AASHTO LRFD Bridge Design Specifications, 3rd Edition with 2005 interim provisions and earlier.

The maximum amount of reinforcement is limited such that c/d<sub>e</sub> <= 0.42. When this ratio is exceeded, the maximum design moment capacity is computed in accordance with LRFD C5.7.3.3.1.

There is a sudden drop in usable capacity when c/d<sub>e</sub> transitions from just below 0.42 to just over 0.42. Furthermore, the equations in LRFD C5.7.3.3.1 do not adequately account for the different concrete materials in the deck and girder. WSDOT uses a variable capacity reduction factor method for dealing with this situation.

If the design criteria in this program is set to use the WSDOT BDM method of computing the capacity of over-reinforced sections, the nominal capacity will be computed using the strain compatibility analysis as described above. Then a reduced capacity reduction factor will be computed by

Concrete Strength at Continuity Diaphragms
------------------------------------------
LRFD 5.12.3.3.10 (*pre-2017: 5.14.1.4.10*) allows the moment capacity of continuity diaphragms to be based on the strength of the concrete in the precast girders. The reason for this is explained in C5.12.3.3.10. The use of the increased concrete strength is permitted because the continuity diaphragm concrete between girder ends is confined by the girder and the continuity diaphragm extends beyond the girders. 

The girder concrete strength is used in the computation of moment capacity at continity diaphragms when the width of the diaphragm exceeds the distance between the ends of girders on either side of the pier by 5% and when girders on both sides of the pier are essentially aligned. 
The girders are considered to be essentially aligned when the distance between the points where the centerline of the girder, projected, intersects the centerline pier is less than 5% of the bottom flange width.

![](DiaphragmConfinement.png)

References
------------
1. [Stephen J. Seguirant P.E. Richard Brice P.E, Bijan Khaleghi, Ph.D., P.E., "FLEXURAL STRENGTH OF REINFORCED AND PRESTRESSED CONCRETE T-BEAMS", PCI Journal Jan/Feb 2005.](https://www.pci.org/PCI/Publications/PCI_Journal/Issues/2005/January-February/Flexural_Strength_of_Reinforced_and_Prestressed_Concrete_T-Beams.aspx)
2. Collins, M.P., and Mitchell, D., Prestressed Concrete Structures, Prentice-Hall, Inc., A Division of Simon & Schuster, Englewood Cliffs, New Jersey, 07632, 1991, pp. 61-65. 
