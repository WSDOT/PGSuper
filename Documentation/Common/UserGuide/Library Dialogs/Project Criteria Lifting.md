Lifting {#ug_library_dialogs_project_criteria_lifting}
==============================================
Define project criteria for allowable stresses and stability analysis during lifting.

Factors of Safety
------------------
Define the minimum factors of safety for the girder stability analysis.

Item | Description
-----|------------
Cracking | Enter the minimum factor of safety against cracking of the section during handling in the casting yard.
Failure | Enter the minimum factor of safety against failure of the section during handling in the casting yard.

> NOTE: Refer to the discussion Girder @ref tg_stability in the @ref technical_guide for more information about girder stability analysis and recommended factors of safety.

Modulus of Rupture for Cracking Moment
---------------------------------------
Define the coefficient for computing the modulus of rupture used for computing the cracking moment in the stability analysis.

Lifting Analysis Parameters
-------------------
Define lifting stability analysis parameters

Item | Description
-----|---------------
Impact Up/Down | Enter the factor in percent that dead load force effects are to be increased due to dynamic effects during lifting. The factor to be applied to the static load shall be taken as: (1 + IM/100). For example, a downward factor of 50.0 would increase vertical forces and stresses during lifting by 50%. An upward factor of 30.0 would decrease vertical forces and stresses during lifting by 30%. The factor must be between zero and 100.0.
Height of Pick Point Above Top of Girder | Enter the height of the rigid pick point of the top of the girder. Stability during lifting can be improved by increasing this value. The height can be increased by the use of gussets or other lifting devices. The lifting device must be rigid.
Lifting Loop Placement Tolerance | Enter the lateral tolerance for placement of lifting loops at the centerline of the top flange of the girder.
Max Girder Sweep Tolerance | Enter the maximum girder sweep tolerance expected for manufacturers in your area. The sweep tolerance is the horizontal deviation of the girder from end to end along the total length of the girder.
Min Angle of Inclination of Lifting Cable | Enter the minimum expected angle of inclination for the lifting cables as measured from the horizontal. Angled lifting cables induce compression and moment into the girder. This effect is accounted for in the lifting analysis.

Allowable Concrete Stresses
------------------------------

Item | Description
-----|----------------
Compressive Stress | Enter the coefficient for the allowable concrete compressive stress.
Tensile Stress - in areas without sufficient bonded reinforcement | Enter the coefficient for the allowable tensile stress. Check the "But not greater than" box to establish an upper limit on the allowable tensile stress.
Tensile Stress - in areas with sufficient bonded reinforcement | Enter the coefficient for the allowable tensile stress.

> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for a discussion describing how longitudinal reinforcement in the girder factors into the determination of the allowable tensile stress.
