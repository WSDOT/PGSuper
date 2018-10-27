Longitudinal Reinforcement {#ug_dialogs_girder_details_longitudinal_reinforcement}
==============================================
Define the longitudinal reinforcement layout. 

> NOTE: Longitudinal reinforcement may be used in moment capacity and shear capacity computations. However, the options to use it must be enabled in the current Project Criteria. Refer to the @ref tg_longitudinal_reinforcement section in the @ref technical_guide for a discussion on how longitudinal rebars are used in analysis.

Item | Description
-----|---------
Reinforcement Material | Select the appropriate mild steel for longitudinal rebar
Reinforcement | Describe the reinforcement in the grid. See below for details.
[Insert Row] | Inserts a row into the grid
[Delete Row] | Deletes the selected rows from the grid
[Append Row] | Appends a row to the bottom of the grid
[Restore to Library Defaults] | Removes all reinforcement from the grid and copies the reinforcement from the girder definition.

Describing the Reinforcement
------------------------------
The reinforcement is described in the Reinforcement grid with the following parameters

Item | Description
-------|-------------
Measured From | Defines the datum for locating the start point of the reinforcing bar. Segments that extend beyond the end of the girder are truncated at the end of the girder. Full Length and Mid-Girder-End segments grow and shrink as the girder length changes. ![](LongRebarOptions.gif)
Distance from End | Defines the location of the start point of the reinforcing bar.
Bar Length | Defines the length of the bar
Girder Face | Rebar rows may be measured either from the top or bottom face of the girder.
Cover | Enter the distance between the specified (top or bottom) girder face to the outside nominal diameter for the specified bar.
Bar Size | Select from the available standard bar sizes that may be used for longitudinal rebar.
\# of Bars | Specify the number of bars to be placed in the row.
Spacing | Enter the center-to-center rebar spacing.
Anchored Left/Right | Bars measured as Full Length or from the Left and Right Ends with a Distance From End of 0.0 can be designated as Anchored. Anchored bars are assumed to be anchored into adjacent structural elements such as a pier or abutment diaphragm or a closure joint for spliced girder segments. Anchored bars are assumed to be fully developed at the face of the member as well as at the centerline of adjacent diaphragms.

All cells in the grid must be filled in. Blank cells are considered incomplete data.  

> NOTE: The assumption of anchorage is not verified by the software. The engineer has the responsibility to ensure anchorage can be achieved.
