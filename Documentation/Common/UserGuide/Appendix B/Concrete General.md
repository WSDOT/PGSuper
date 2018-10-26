General {#appendix_b_concrete_general}
==============================================
The Concrete Material Dialog allows you to edit the properties of concrete materials stored in the library. The properties that may be specified for a concrete entry are as follows:

Parameter | Description
----------|------------
Entry Name | Each concrete material entry must have a unique name. The name may contain any characters and can be up to 32 characters in length.
Type | Specifies the type of concrete. Concrete can be Normal weight, All Lightweight, or Sand Lightweight.
Strength, f'c | This is the strength for the material as defined by most common specifications. This strength is normally defined as the stress value at a strain of 0.003 at 28 days.
Unit Weight | This is the unit weight of the material which is used along with f'c to calculate the modulus of elasticity. If the number in this input field turns red, the value does not conform to the unit weight limits defined for the concrete type in LRFD 5.2.
Unit Weight with Reinforcement | This is the effective unit weight of the material and includes an allowance for reinforcement. This unit weight is used to compute self-weight.
Mod. Elasticity, Ec | Select the check box if you want to override the computation of modulus of elasticity and enter your own value. If the check box is unchecked, PGSuper will compute Ec based on f'c and the Unit Weight of the material using LRFD equation 5.4.2.4-1.
Averaging Factor, K1 | Correction factor for aggregate type in predicting average value. See [Concrete Properties](@ref concrete_properties) in the [Technical Guide](@ref technical_guide) for additional information.
Bounding Factor, K2 | Correction factor for aggregate type in predicting upper and lower bounds. See [Concrete Properties](@ref concrete_properties) in the [Technical Guide](@ref technical_guide) for additional information.
Creep | Creep of Concrete
Averaging Factor, K1 | Correction factor for aggregate type in predicting average value. See [Concrete Properties](@ref concrete_properties) in the [Technical Guide](@ref technical_guide) for additional information.
Bounding Factor, K2 | Correction factor for aggregate type in predicting upper and lower bounds. See [Concrete Properties](@ref concrete_properties) in the [Technical Guide](@ref technical_guide) for additional information.
Shrinkage | Shrinakge of Concrete
Averaging Factor, K1 | Correction factor for aggregate type in predicting average value. See [Concrete Properties](@ref concrete_properties) in the [Technical Guide](@ref technical_guide) for additional information.
Bounding Factor, K2 | Correction factor for aggregate type in predicting upper and lower bounds. See [Concrete Properties](@ref concrete_properties) in the [Technical Guide](@ref technical_guide) for additional information.
Max. Aggregate Size | Enter the maximum dimension of the concrete aggregate. This value is used to check minimum reinforcement spacing requirements per LRFD 5.10.3.1.2.
Agg. Splitting Strength, fct | If the aggregate splitting strength of a lightweight concrete is known, check the box and input the value.

