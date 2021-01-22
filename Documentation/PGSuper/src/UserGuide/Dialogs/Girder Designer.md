Girder Designer {#ug_dialogs_girder_designer}
==============================================
The Girder Designer feature is used to "take an educated guess" at an initial design for a selected girder in your bridge. 

> NOTE: A successful design attempt does not guarantee compliance with all Project Criteria or LRFD Specification requirements. Always review the Spec Check Report.

Select Girder(s) to be Designed
-------------------------------
Two options are available for selecting girders: 
1. Design a single girder: Use the drop down lists for the span and girder to select the girder that will be designed.
2. Design multiple girders: Click [Select Girders] to bring up the multiple girder selection window and select the desired girders.

Design Options
--------------
Check the boxes in this section to select a design option.

Option | Description
-------|------------
Design For Flexure | Designs the girders for flexure. Flexure design can be refined with the Haunch Geometry and  Concrete Strength options.
Haunch Geometry<sup>*</sup> | Haunch geometry can be preserved during the design, or it can be designed. When the design option is enabled the Girder Designer will attempt to find the optimal slab offset for the selected girder(s). If the haunch load is distributed parabolically, the Girder Designer will also change the assumed excess camber to match the computed excess camber.
Concrete Strength | When Design for minimum concrete strength is selected the Girder Designer will attempt to determine the minimum concrete strength for release and the specified 28 day strength. When Preserve concrete strength during design is selected the Girder Designer will not change the concrete strength.
Design for Shear | Designs the selected girders for shear
Start with Current Stirrup Layout | If this option is selected the Girder Designer will first perform a specification check on the currently input stirrup layout. If the check passes, the Girder Designer will not modify the current stirrup layout. If the specification check fails, the Girder Designer will attempt to modify the layout of the stirrups by moving the stirrup zone boundaries. If this option is not selected, the Girder Designer will design the stirrups using the design control parameters from the Shear Design tab for the girder library entry.

> NOTES:
> * Haunch geometry design is not available if there is no slab or if slab offset design is disabled in the Project Criteria.
> * If multiple girders are selected for haunch geometry design, the girder designer will attempt a separate design for each girder, which may result in unique haunch geometry values for each girder. You, the engineer, are then responsible for inputting the optimal slab offset and assumed excess camber values for all girders in the bridge.

> TIP: If your PGSuper Administrator has defined standard stirrup configurations for your girders (for example WSDOT and TxDOT), you generally **WILL NOT** want the Girder Designer to design for shear.

Refer to @ref ug_design in the @ref user_guide for more information about designing girders.  

