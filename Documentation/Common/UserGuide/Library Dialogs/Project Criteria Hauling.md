Hauling {#ug_library_dialogs_project_criteria_hauling}
==============================================
Define project criteria for allowable stresses and stability analysis during hauling.

Hauling analysis can be done with the WSDOT or KDOT method. Use the drop down list to select the method

WSDOT Method
--------------

### Factors of Safety ###
Define the minimum factors of safety for the girder stability analysis.

Item | Description
-----|------------
Cracking | Enter the minimum factor of safety against cracking of the section during hauling to the bridge site.
Roll Over | Enter the minimum factor of safety against roll over of the hauling rig during hauling to the bridge site.

> NOTE: Refer to the discussion Girder @ref tg_stability in the @ref technical_guide for more information about girder stability analysis and recommended factors of safety.

### Modulus of Rupture for Cracking Moment ###
Define the coefficient for computing the modulus of rupture used for computing the cracking moment in the stability analysis.

### Hauling Analysis Parameters ###
Define hauling stability analysis parameters

Item | Description
-----|---------------
Impact Up/Down | Enter the factor in percent that dead load force effects are to be increased due to dynamic effects during hauling. The factor to be applied to the static load shall be taken as: (1 + IM/100). For example, a downward factor of 50.0 would increase vertical forces and stresses during hauling by 50%. An upward factor of 30.0 would decrease vertical forces and stresses during hauling by 30%. The factor must be between zero and 100.0.
Roll Stiffness of Truck Trailer | Enter the roll stiffness for truck trailers used to haul girders to the bridge site. This value is likely to be agency-specific, if not job-specific. Roll stiffness can be entered as a constant value or as a function of the girder weight. The heavier the girder, the more stiff and sturdy the truck will have to be. To define the roll stiffness as a function of the girder weight, enter the maximum weight per axle and the per axle stiffness. The roll stiffness will be determined by determining the number of axles required and multiplying that value by the per axle roll stiffness. A minimum roll stiffness is also required to represent the most common trucks used in your area.
Height of Girder Bottom Above Roadway | Enter the height of the bottom of the girder above the roadway for the standard truck for your agency.
Height of Roll Center of Truck Above Roadway | Enter the height of the roll center above the roadway for the standard truck for your agency.
Truck Width (C-C distance between dual tires) | Enter the width of the standard truck for your agency. The width of the truck is measured as the center to center distance between the left hand side and right hand side dual tires.
Max Distance Between Girder Supports | Enter the maximum distance between girder support locations (trailer length) for the standard truck for your agency.
Max Expected Roadway Superelevation | Enter the maximum expected slope of the roadway elevation to be encountered by the vehicle hauling the girder to the bridge site. This is a unitless slope value, so a six percent slope would be entered as 0.06
Sweep | Enter the girder sweep expected for manufacturers in your area. The sweep tolerance is the horizontal deviation of the girder from end to end along the total length of the girder. Sweep is modeled as an offet per unit length of girder plus and additional amount to account for time dependent changes to the initial sweep.
Support Placement Lateral Tolerance | Enter the lateral tolerance for placement of girder onto support points.
Increase Girder C.G. Height for Camber by | Enter the percentage to increase the height of the C.G. of the girder above the roll center to account for camber.
Maximum Girder Weight | Enter the maximum girder weight. This value will depend on the plant lift capabilities of your local precasters and the capabilities of the available trucking, rail, and barge equipment. Your organization should perform a study to determine a value appropriate in your area.  

KDOT Method
--------------

### Dynamic Load Factors ###
Dynamic "g" factors are applied to girder dead loads to simulate dynamic effects at girder hauling. Typically for KDOT, a dynamic factor of 3.0 is applied along cantilevers, and a factor of 1.0 is applied between the cantilever sections. The dynamic affect is only applied downward.

Item | Description
------|---------------
Along Cantilever | Enter the "g" factor to be applied to girder dead load along the cantilever (i.e., outside of bunk point support locations). This value must be greater than or equal to 1.0.
Between Cantilevers | Enter the "g" factor to be applied to girder dead load between cantilevers (i.e., interior of beam between bunk point support locations). This value must be greater than or equal to 1.0.


Allowable Concrete Stresses
------------------------------

Girder stresses can be computed either with or without consideration for the stability equilibrium angle. Use the drop down list to select an option.

Item | Description
-----|----------------
Compressive Stress - General | Enter the coefficient for the allowable concrete compressive stress for the general bending case (girder selfweight + prestress).
Compressive Stress - With lateral bending | Enter the coefficient for the allowable concrete compressive stress for the cases that include lateral bending due to girder tilt, wind and centrifugal force.
Tensile Stress - in areas without sufficient bonded reinforcement | Enter the coefficient for the allowable tensile stress. Check the "But not greater than" box to establish an upper limit on the allowable tensile stress.
Tensile Stress - in areas with sufficient bonded reinforcement | Enter the coefficient for the allowable tensile stress.

> NOTE: See @ref tg_longitudinal_reinforcement in the @ref technical_guide for a discussion describing how longitudinal reinforcement in the girder factors into the determination of the allowable tensile stress.
