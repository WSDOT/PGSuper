Prestressed Elements {#ug_library_dialogs_project_criteria_prestressed_elements}
==============================================
Define project criteria for specification compliance checking of prestressed elements.

The parameters on this tab are allowable stress limits. They can be set to match or deviate from AASHTO.

Stress Limits for Temporary Stresses before Losses (LRFD 5.9.2.3.1 (*pre-2017: 5.9.4.1*) )
-----------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.9.2.3.1.

Item | Description
-----|--------------
Compressive Stress | Enter the coefficient for the allowable compressive stress.
Tensile Stress - In areas other than the precompressed tensile zone and without bonded reinforcement | Enter the coefficient for the allowable tensile stress. Check the "But not greater than" box to estabilish an upper limit on the allowable tensile stress.
Tensile Stress - In areas with sufficient bonded reinforcement | Enter the coefficient for the allowable tensile stress.

> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for discussion describing how longitudinal reinforcement in the girder factors into the determination of the allowable tensile stress.

Stress Limits at Service Limit States after Losses (LRFD 5.9.2.3.2 (*pre-2017: 5.9.4.2*))
-----------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.9.2.3.2

Item | Description
-----|--------------
Compressive Stress (Effective Prestress + Permanent Loads) | Enter the coefficient for the allowable compressive stress.
Compressive Stress (Effective Prestress + Permanent Loads + Transient Loads) | Enter the coefficient for the allowable compressive stress.
Tensile Stress (Service I)(Effective Prestress + Permanent Loads = Final Stress without Live Load) | When checked, the girder stress for the final condition, without live load, will be check for the Service I limit state combination. Enter the coefficient for the allowable tensile stress. Check the "But not greater than" box to establish an upper limit on the allowable tensile stress.
Tensile Stress (Service III)(Effective Prestress + Permanent Loads + Transient Loads) | Enter the coefficient for the tensile stress for "Not worse than moderate corrosion conditions" and "Severe corrosive conditions". Check the "But not greater than" box to establish an upper limit on the allowable tensile stress.

> NOTE: Some bridge owners limit the girder tensile stress for final tension without live load, however, this is not an AASHTO requirement.

> NOTE: The corrosion condition is defined in the Environmental parameters of a project.

Allowable Concrete Stress at Fatigue Limit State (LRFD 5.5.3.1)
---------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.5.3.1.

Item | Description
-----|--------------
Fatigue I plus one-half the sum of effective prestress and permanent loads | Enter the coefficient for the allowable compressive stress.

Stress Limits for Temporary Loading Conditions (PGSuper Only)
-------------------------------------------------------------
Some bridge owners limit the girder stresses during intermediate, temporary loading conditions. These parameters are only used for PGSuper projects. 

> NOTE: The LRFD specifications require evaluation of stresses during temporary loading conditions for spliced girder bridges. The stress limitations are based on LRFD 5.9.2.3.1 and 5.9.2.3.2 (*pre-2017: 5.9.4.1 and 5.9.4.2*). Therefore, PGSplice does not use these limitations.

Check the "Evaluate stress limits for temporary loading conditions" to cause stress to be evaluated immediately after temporary strand removal and immediately after deck placement.

### Stress Limits immediately after Temporary Strand Removal ###
Parameters for evaluating girder stresses immediately after temporary strands are removed.

Item | Description
-----|----------------
Compressive Stress | Enter the coefficient for the allowable compressive stress
Tensile stress - In areas other than the precompressed tensile zone | Enter the coefficient for the allowable tensile stress. Check the "But not greater than" box to establish an upper limit on the allowable tensile stress.
Tensile stress - In areas with sufficient bonded reinforcement | Enter the coefficient for the allowable tensile stress.


> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for discussion describing how longitudinal reinforcement in the girder factors into the determination of the allowable tensile stress.


### Stress Limits immediately after Deck Placement ###
Parameters for evaluating girder stresses immediately after deck placement.

Item | Description
-----|----------------
Compressive Stress | Enter the coefficient for the allowable compressive stress
Tensile stress | Enter the coefficient for the allowable tensile stress. Check the "But not greater than" box to establish an upper limit on the allowable tensile stress.
