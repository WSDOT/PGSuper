General {#ug_library_dialogs_concrete_general}
==============================================
Define general information about the concrete. 

Parameter | Description
----------|------------
Type | Specifies the type of concrete. Concrete can be Normal, Lightweight, or UHPC.
Strength, f'c | This is the strength for the material as defined by most common specifications. This strength is normally defined as the stress value at a strain of 0.003 at 28 days.
Unit Weight | This is the unit weight of plain concrete. This value is typically used when computing modulus of elasticity. If the number in this input field turns red, the value does not conform to the unit weight limits defined for the concrete type in LRFD 5.2.
Unit Weight with Reinforcement | This is the unit weight including an allowance for reinforcement. This value is used when computing dead load.
Mod. Elasticity, Ec | Select the check box if you want to override the computation of modulus of elasticity and enter your own value. If the check box is unchecked, Ec will be computed based on f'c and Unit Weight using the application equations for the selected concrete model (AAAHSTO LRFD, ACI 209R-92, or CEB-FIP).
Max. Aggregate Size | Enter the maximum dimension of the concrete aggregate. This value is used to check minimum reinforcement spacing requirements per LRFD 5.10.3.1.2.

> NOTE: Prior to AASHTO LRFD 7th Edition with 2016 interim provisions, lightweight concrete was classified as All Lightweight and Sand Lightweight. If you select All Lightweight or Sand Lightweight and your Project Criteria is based on AASHTO LRFD 7th Edition with 2016 interim provisions or later, your concrete will be considered to be Lightweight.

> NOTE: UHPC concrete is an experimental feature. See @ref tg_uhpc in the Technical Guide for more information


