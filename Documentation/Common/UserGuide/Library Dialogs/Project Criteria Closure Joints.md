Closure Joints {#ug_library_dialogs_project_criteria_closure_joints}
==============================================
Define project criteria for specification compliance checking of cast in place closure joints used with spliced girders.

> NOTE: These parameters are used by PGSplice only.

The parameters on this tab are allowable stress limits. They can be set to match or deviate from AASHTO.

Stress Limits for Temporary Stresses (LRFD 5.9.2.3.1 and 5.12.3.4.2d) (*pre-2017: 5.9.4.1 and 5.14.1.3.2d*)
-----------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.9.2.3.1 and 5.12.3.4.2d

Item | Description
-----|--------------
Compressive Stress | Enter the coefficient for the allowable compressive stress.
Tensile Stress in the Precompressed Tensile Zone - In areas without minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.
Tensile Stress in the Precompressed Tensile Zone - In areas with minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.
Tensile Stress in other areas - In areas without minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.
Tensile Stress in other areas - In areas with minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.

> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for discussion describing how longitudinal reinforcement in the girder factors into the determination of the allowable tensile stress.

Stress Limits at Service Limit States. LRFD 5.9.2.3.2 and 5.12.3.4.2d (*pre-2017: 5.9.4.2 and 5.14.1.3.2d*)
-----------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.9.2.3.2 and 5.12.3.4.2d.

Item | Description
-----|--------------
Compressive Stress (Effective Prestress + Permanent Loads) | Enter the coefficient for the allowable compressive stress.
Compressive Stress (Effective Prestress + Permanent Loads + Transient Loads) | Enter the coefficient for the allowable compressive stress.
Tensile Stress (Service I)(Effective Prestress + Permanent Loads, Final Stress without Live Load) | This option is enabled from the Precast Elements tab. Refer to the documentation at @ref ug_library_dialogs_project_criteria_prestressed_elements for details. Check the "But not greater than" box to establish an upper limit on the tensile stress limit.
Tensile Stress in the Precompressed Tensile Zone - In areas without minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.
Tensile Stress in the Precompressed Tensile Zone - In areas with minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.
Tensile Stress in other areas - In areas without minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.
Tensile Stress in other areas - In areas with minimum bonded auxiliary reinforcement | Enter the coefficient for the allowable tensile stress.

Allowable Concrete Stress at Fatigue Limit State (LRFD 5.5.3.1)
---------------------------------------------------------------
The information in this section allows you to deviate from LRFD 5.5.3.1.

Item | Description
-----|--------------
Fatigue I plus one-half the sum of effective prestress and permanent loads | Enter the coefficient for the allowable compressive stress.
