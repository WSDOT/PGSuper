Design Outcome {#ug_dialogs_design_complete}
==============================================
After the Girder Designer has completed its design attempt, you are presented with the Design Outcome window. From here, you can review the outcome of the design attempt and choose whether you will accept the design.

Item | Description
-----|------------------
Design Outcome Report | This report tells you the outcome of the design attempt.
Apply the Slab Offset to... | This option is only available after slab offset designs, and allows you to select how the slab offset design will be applied to the bridge. Uncheck the box if you do not want to copy the design to any girders, or select which girders from/to to copy the design.
[Accept the Design] | Use this button to accept the design and modify your bridge model with the parameters determined during the design
[Reject the Design] | Use this button to reject the design. Your bridge model will not be changed
[Help] | Opens this help topic
[Print] | Prints the Design Outcome Report

If the Girder Designer was successful the basic design objectives were met. This doesn't mean that you are finished with the design of the girder. You must perform a comprehensive check to insure that the girder satisfies all specification provisions, and is optimized based on your own design criteria (minimum weight, cost, etc...). The Details Report and Spec Check Report contain comprehensive check information. It is possible that all criteria will not be satisfied. For example, if you instructed the Girder Designer to only design for flexure, it is likely that your girder will not satisfy shear capacity and detailing requirements. 

If the Girder Designer was not successful, a design that satisfies the design objectives was not found. This does not necessarily mean that the girder cannot be successfully designed. You can attempt to change the girder models in order to create a design that satisfies all requirements. Refer to @ref ug_design and @ref ug_girder_modeling in the @ref user_guide for more information.

> NOTE: If the Girder Designer was unsuccessful, the last guess will be reported in the "Proposed Design" column in the Design Outcome report. You can use this as a starting point for your manual changes.

> NOTE: Slab offset design outcomes are unique for individual girders (i.e., PGSuper does not perform a global slab offset design for the bridge). Hence, it is up to you to determine the slab offset design strategy for your entire bridge. 

> TIP: The Multi-Girder Haunch Geometry Summary report provides a quick way to view results for all slab offset values in your bridge.