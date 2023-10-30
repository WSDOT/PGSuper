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
Precamber | Enter the amount of precamber to build into the girder during fabrication. Precamber is the amount of camber that is build into the form work, not the total camber. This is sometimes referred to as formed camber. See below regarding precamber and deflections.
Top Width | Select a top width type and enter the top width dimensions. This option is only available when the Spacing Type specifed in the Bridge Description window, General tab, is set to *Adjacent girders with unique joint spacing and top flange width for each span*. Top width types are Symmetric (the top flange is symmetric about the centerline of the girder), Centered CG (the length of the top flange on the left and right side of the girder centerline are automatically computed such that the center of gravity of the girder is coincident with the girder centerline), and Asymmetric (the top flange is asymmetric and defined by unique left and right overhangs measured from the girder centerline).
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

Haunch Geometry
---------------
Define the geometry of the slab haunch

> A complete description of vertical bridge geometry, haunch design, and haunch input methods can be found at @ref tg_vertical_geometry

Different options are shown depending on the selected haunch input method

###"A" and Assumed Excess Camber Method###

Item | Description
-----|-------------
Slab Offset ("A" Dimension) | Enter the slab offset dimension for the girder.
Assumed Excess Camber <sup>+</sup>  | Enter the excess camber that is used to define the parabolic haunch depth used for computing composite section properties and/or haunch dead load. Note that this option is only available if the parabolic option is selected for computing composite section properties or haunch loading in the current Project Criteria library entry.

###Explicit Haunch Depth input method###

Item | Description
-----|-------------
Haunch Depth | Haunch depth is shown when applicable

Condition and Rating
---------------------
Select the condition of the girder from the drop down list. If "Other" is selected, enter the condition factor.

Precamber and Deflections
--------------------------
Precamber is not a deflection in the sense of linear elastic structural response to loads. However, precamber is important in the overall shape of the deflected beam. Precamber is treated as an unrecoverable deflections in graphs and reports. See @ref ug_analysis_results_graph and @ref tg_deflections for more information about unrecoverable deflection.