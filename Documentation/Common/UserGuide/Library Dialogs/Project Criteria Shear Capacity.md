Shear Capacity {#ug_library_dialogs_project_criteria_shear_capacity}
==============================================
Define project criteria for shear capacity analysis.

Shear Capacity
---------------
Select the method for computing shear capacity.

Limit net longitudinal strain in the section at the centroid of the tension reinforcement (es) to positive values (LRFD 5.7.3.4.2). When this option is checked, the strain computed by LRFD Equation 5.7.3.4.2-4 (pre-2017: 5.8.3.4.2-4) is taken to be zero if the computed value is negative.

Enter the coefficient for computing the modulus of rupture, which is used for determining the cracking moment.

Enter the residual tensile strength of UHPC concrete. This parameter is used to compute the shear contribution of fibers in the concrete matrix.

Resistance Factors (LRFD 5.5.4.2)
----------------------------------
### Conventional Construction (LRFD 5.5.4.2) ###
Enter the normal, lightweight, and UHPC concrete resistance factors.
Note the LRFD 8th Edition (2017) added separate resistance factors for dedonded and fully bonded sections.

### Closure Joints (LRFD 5.5.4.2 and 5.12.3.4.2d (pre-2017: 5.14.1.3.2d)) ###
Enter the normal and lightweight concrete resistance factors.

Maximum Spacing of Transverse Reinforcement (LRFD 5.7.2.6 (pre-2017: 5.8.2.7))
----------------------------------------------------------
Enter parameters for LRFD equations 5.7.2.6-1 and 5.7.2.6-2. These parameters define the maximum stirrup spacing as a fraction of the shear depth, dv, and a maximum value.

Longitudinal Reinforcement for Shear
-------------------------------------
Select a method for computing longitudinal reinforcement for shear capacity. The longitudinal mild reinforcement in the girder will be included in the analysis if the box is checked.

Horizontal Interface Shear
--------------------------
Select a method for computing horizontal interface shear.

Define the maximum spacing of interface shear connectors for LRFD 5.7.4.5 (*pre-2017: 5.8.4.2*).

Use the deck weight for the permanent net compressive force normal to the shear plane. Check this box to use the weight of the deck as the normal force, Pc, for use in LRFD Eq'n 5.7.4.3-3 (*pre-2017: Eq'n 5.8.4.1-3*). LRFD C5.7.4.3 (*pre-2017: C5.8.4.1*) states that it is conservative to neglect Pc if it is compressive.