Live Load Distribution Factors {#tg_lldf}
======================================

LRFD 4.6.2.2 addresses the topic of distribution of live loads to beam-slab structures. This is certainly one of the most contentious and difficult to understand sections of the Specifications, and is an area where many agencies deviate and use their own modifications to the Specifications; WSDOT and TxDOT included. 

WSDOT and TxDOT Modifications to Distribution Factors
------------------------------------------------------
The WSDOT and TxDOT modifications to the AASHTO LRFD for computing live load distribution factors are discussed in @subpage tg_lldf_wsdot and @subpage tg_lldf_txdot.

Interpretation Issues
---------------------
In this section we discuss some interpretations of ambiguous parts of Section 4.6.2.2. It is our hope that future versions of the Specification will clarify these issues.

### Lever Rule Inheritance ###
There are several equations for exterior beams or for shear, that refer to the equation for interior beams, or moment; which result in use of the lever rule for the case referred to. We interpret that if the beam referred to's distribution factor is computed using the lever rule, then compute the factor using the lever rule for the beam in question.

For example, the equation for the shear, multiple lane distribution factor equation for exterior type K beams is g<sub>exterior</sub>=eg<sub>interior</sub>; however if S>16.0,  g<sub>interior</sub> will be computed using the lever rule. For this case, PGSuper will compute g<sub>exterior</sub> using the lever rule for the exterior beam directly and ignore the e factor. Similar cases exist for shear equations that are dependent on results from moment equations.

### Exterior Shear in Adjacent U Beams and Voided Slabs (type f, g) ###
This is similar to the lever rule inheritance case above. For beam types f and g, the distribution factor equations for shear in exterior beams is g<sub>exterior</sub>=eg<sub>interior</sub>. However, if I or J exceed the range of applicability, the moment equation will be used to compute ginterior. The question is whether to compute e based on the shear equation, or e based on exterior moment. Our interpretation is: if I or J is out of range, use the exterior moment equation directly to compute the exterior beam distribution factor for shear.

### Live Load Distribution for Reactions, Deflections, and Rotations ###
Live load reactions, deflections, and rotations are destributed uniformly to all girders in a span. The distribution factor is the product of the multiple presence factor and the number of design lanes divided by the number of girders

> LLDF = (MPF)(Number of Lanes)/(Number of Girders)

> Live Load Reaction, Deflection, or Rotation per girder = (LLDF)(Single Lane Live Load Reaction, Deflection, or Rotation per LRFD 3.6.1.2)

