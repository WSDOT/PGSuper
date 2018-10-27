General {#ug_dialogs_girder_details_general}
==============================================
Define general information about the girder

Events
--------
Select the construction events for this girder.

Item | Description
-----|-----------
Construction | Select the event when this girder is constructed
Erection | Select the event when this girder is erected

Girder Type
-----------
There are two drop down lists in this group. The first defines how girders are defined in the bridge.
If unique girders are used throughout the bridge, select the name of this girder from the second drop down list. 

Girder Modifiers
-----------------
These parameters modify the girder. Some of the items listed below may not be supported by your girder and will not be shown in the window.

Item | Description
-----|-----------
Precamber | Enter the amount of precamber to build into the girder during fabrication. 
Top Width | Enter the top width of the girder.
Top Flange Thickening | Select the method of top flange thickening and enter the amount of thickening.

Top flange thickening increases the thickness of the top flange at either the girder ends or middle of the girder, by the specified amount. The thickness of the top flange is then changed parabolically to its nominal value at the girder center or ends, respectively.

Girder Concrete Properties
--------------------------
Define the concrete properties.

Item | Description
-----|-----------
f'ci | Enter the concrete release strength
Eci  | When checked, enter the modulus of elasticity at release. Otherwise, the modulus of elasticity will be computed by the AASHTO equation.
f'c | Enter the design concrete strength
Ec  | When checked, enter the design modulus of elasticity. Otherwise, the modulus of elasticity will be computed by the AASHTO equation.
[More Properties...] | Press to define detailed concrete properties or select a pre-defined concrete from the library

Slab Offset ("A" Dimension)
----------------------------
Use the combo box to control how slab offsets are defined for the bridge.

Item | Description
-----|-----------
Start of Girder | Enter the slab offset at the start of the girder
End of Girder | Enter the slab offset at the end of the girder

Assumed Excess Camber
---------------------
Enter the excess camber that is used to define the parabolic haunch depth used for computing composite section properties and/or haunch dead load. Note that this option is only available if the parabolic option is selected for computing composite section properties or haunch loading in the current Project Criteria library entry.

Refer to  the Slab Haunch loading section of @ref tg_structural_analysis_models for more information. Refer to @ref tg_section_properties in the Technical Guide for detailed information about haunch depth is used when computing composite section properties.

Condition and Rating
---------------------
Select the condition of the girder from the drop down list. If "Other" is selected, enter the condition factor.
