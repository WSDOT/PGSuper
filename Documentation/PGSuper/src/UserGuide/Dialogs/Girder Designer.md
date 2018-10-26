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
-------|-------------
Design For Flexure | Designs the girders for flexure
Don't Change Slab Offset ("A" Dimension) | This option is only used if Design For Flexure is checked. If the Slab Offset is considered in design (as set in the Project Criteria), the optimal slab offset for the selected girders will be designed. Uncheck this option if you do not want the Girder Designer to alter the slab offset. 
Design for Shear | Designs the selected girders for shear
Start with Current Stirrups Layout | If this option is selected the Girder Designer will first perform a specification check on the currently input stirrup layout. If the check passes, the Girder Designer will not modify the current stirrup layout. If the specification check fails, the Girder Designer will attempt to modify the layout of the stirrups by moving the stirrup zone boundaries. If this option is not selected, the Girder Designer will design the stirrups using the design control parameters from the Shear Design tab for the girder library entry.

> NOTE: Slab offset ("A" Dimension) design is not available if there is no slab, if slab offset design is disabled in the Project Criteria, or if multiple girders are selected for design.

> TIP: If your PGSuper Administrator has defined standard stirrup configurations for your girders you generally **WILL NOT** want the Girder Designer to design for shear.

> NOTE: Refer to @ref ug_design in the @ref user_guide for more information about designing girders.  

