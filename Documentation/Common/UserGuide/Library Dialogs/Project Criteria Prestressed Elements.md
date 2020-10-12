Prestressed Elements {#ug_library_dialogs_project_criteria_prestressed_elements}
==============================================
Define project criteria for specification compliance checking of prestressed elements.

The parameters on this tab are stress limits. They can be set to match or deviate from AASHTO.

Stress Limits for Temporary Stresses before Losses (LRFD 5.9.2.3.1 (*pre-2017: 5.9.4.1*) )
-----------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.9.2.3.1.

Item | Description
-----|--------------
Compressive Stress | Enter the coefficient for the compressive stress limit.
Tensile Stress - In areas other than the precompressed tensile zone and without bonded reinforcement | Enter the coefficient for the tensile stress limit. Check the "But not greater than" box to estabilish an upper limit on the tensile stress limit.
Tensile Stress - In areas with sufficient bonded reinforcement | Enter the coefficient for the tensile stress limit.

> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for discussion describing how longitudinal reinforcement in the girder factors into the determination of the tensile stress limit.

Stress Limits at Service Limit States after Losses (LRFD 5.9.2.3.2 (*pre-2017: 5.9.4.2*))
-----------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.9.2.3.2

Item | Description
-----|--------------
Compressive Stress (Effective Prestress + Permanent Loads) | Enter the coefficient for the compressive stress limit.
Compressive Stress (Effective Prestress + Permanent Loads + Transient Loads) | Enter the coefficient for the compressive stress limit.
Tensile Stress (Service I)(Effective Prestress + Permanent Loads = Final Stress without Live Load) | When checked, the girder stress for the final condition, without live load, will be check for the Service I limit state combination. Enter the coefficient for the tensile stress limit. Check the "But not greater than" box to establish an upper limit on the tensile stress limit.
Tensile Stress (Service III)(Effective Prestress + Permanent Loads + Transient Loads) | Enter the coefficient for the tensile stress limit for "Not worse than moderate corrosion conditions" and "Severe corrosive conditions". Check the "But not greater than" box to establish an upper limit on the tensile stress limit.

> NOTE: Some bridge owners limit the girder tensile stress for final tension without live load, however, this is not an AASHTO requirement.

> NOTE: The corrosion condition is defined in the Environmental parameters of a project.

Allowable Concrete Stress at Fatigue Limit State (LRFD 5.5.3.1)
---------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.5.3.1.

Item | Description
-----|--------------
Fatigue I plus one-half the sum of effective prestress and permanent loads | Enter the coefficient for the compressive stress limit.

Stress Limits for Temporary Loading Conditions (PGSuper Only)
-------------------------------------------------------------
Some bridge owners limit the girder stresses during intermediate, temporary loading conditions. These parameters are only used for PGSuper projects. 

> NOTE: The LRFD specifications require evaluation of stresses during temporary loading conditions for spliced girder bridges. The stress limitations are based on LRFD 5.9.2.3.1 and 5.9.2.3.2 (*pre-2017: 5.9.4.1 and 5.9.4.2*). Therefore, PGSplice does not use these limitations.

Check the "Evaluate stress limits for temporary loading conditions" to cause stress to be evaluated immediately after temporary strand removal and immediately after deck placement.

### Stress Limits immediately after Temporary Strand Removal ###
Parameters for evaluating girder stresses immediately after temporary strands are removed.

Item | Description
-----|------------
Compressive Stress | Enter the coefficient for the compressive stress limit
Tensile stress - In areas other than the precompressed tensile zone | Enter the coefficient for the tensile stress limit. Check the "But not greater than" box to establish an upper limit on the tensile stress limit.
Tensile stress - In areas with sufficient bonded reinforcement | Enter the coefficient for the tensile stress limit.


> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for discussion describing how longitudinal reinforcement in the girder factors into the determination of the tensile stress limit.


### Stress Limits immediately after Deck Placement ###
Parameters for evaluating girder stresses immediately after deck placement.

Item | Description
-----|------------
Compressive Stress | Enter the coefficient for the compressive stress limit
Tensile stress | Enter the coefficient for the tensile stress limit. Check the "But not greater than" box to establish an upper limit on the tensile stress limit.

Principal Tensile Stress in Webs (LRFD 5.9.2.3.3)
---------------------------------------------------
These requirements were first introduced in the AASHTO LRFD Bridge Design Specifications, 8th Edition 2017. See @ref tg_principal_tensile_stress in the @ref technical_guide for more information.

Item | Description
-----|------------
Tensile Stress | Enter the coefficient for the tensile stress limit.
Check when f'c exceeds | Principal web stress requirements will only be checked for non-post tensioned beams when f'c is greater than or equal to the input value. Use the keyword "All" to ensure that principal web stresses are always checked. Post tensioned beams will always be checked.
Compute flexural shear stress... | Select the method for computing flexural shear stress in the webs. Options are AASHTO LRFD Equation 5.9.2.3.3-1 and WSDOT BDM/NCHRP Report 849 Equation 3.8. The AASHTO equation computes the shear stress by applying the Service III shear force to the composite section. The WSDOT/NCHRP equation computes shear stress by applying loading to the non-composite and composite sections as appropreate and summing the resulting stresses.
At sections where internal tendons cross... | Enter the number of outside duct diameters away from the centerline of a duct used to determine of the tendon is "near" the section being checked and the provisions of LRFD 5.7.2.1 are applied. LRFD 5.7.2.1 specifies reductions in effective web width in the precense of ducts.

