Permanent Strands {#ug_library_dialogs_girder_permanent_strands}
==============================================
This tab allows you to specify the possible locations of permanent prestressing strands. 

General Information
-------------------
The strand grid defines the possible locations of permanent strands in a girder and the order in which the strand positions are filled. Another way to think of this is a template for strand placement.  Actual strands are placed in a project by specifying the number of strands, or by individually selecting locations, to be filled in the template in the Strands Tab of the Girder Editing dialog.

There are two basic types of permanent strands. 
1. *Straight Permanent Strands* can be placed anywhere within a girder section and are straight along the entire length of the girder.
2. *Adjustable Strands* are permanent strands that have the unique capability to be vertically adjusted within the web region of a girder. Adjustable strands can only be placed in the girder's web(s). There are two types of Adjustable Strands: *Harped Strands* and *Adjustable Straight* strands. 
   * Harped strands are draped at the girders harping points. Harping (draping) is created by the difference in vertical adjustment between the strand group at the girder ends and harping points.
   * Adjustable Straight Strands are straight along the entire length of the girder. The entire group can be vertically adjusted. Adjustable Straight Strands located above the girder's upper kern location can be used by the automated design algorithm to alleviate girder end stresses.

> NOTE: Permanent strands are placed and tensioned at casting time and remain in the girder for its lifetime. Adjustable strands are bonded along the entire length of the girder. Individual straight strands can be debonded only if the "allow debonding" option is selected for the strands in question.

> NOTE: The flexural design algorithm does not attempt to position debonded strands to satisfy the requirements of LRFD 9th Edition 5.9.4.3.3 Requirements E, 4th bullet of Requirement I, 1st bullet of Requirement J, or 1st bullet of Requirement K, though these requirements are evaluated in the specification check. Careful selection of the debondable strands when defining strand grids will result in better design outcomes.

Adjustable Strand Settings
--------------------------

Item | Description
----|----
Adjustable Strand Type | The type of adjustable strands can be designated as Straight, Harped, or "Harped or Straight". If "Harped or Straight" is selected, the type of strand can be set by the end user when editing the girder.
Coerce Odd Number of Adjustable Strands | When selected, PGSuper will force (coerce) the highest (last in the fill order) pair of adjustable strands to alternate between a single strand at X=0.0, and two strands at the prescribed +/- X values. This allows a strand grid that contains only pairs of coordinates to place strands one at a time. This feature is uncommonly needed for most users, and is used on some WSDOT I-Beams or Ribbed Girders with an odd number of webs. 
Use Different Harped Locations at Girder Ends | This option allows you to define different locations of a harped strand at the ends of the girder and at the harping points. This option is most often used to describe the "fanned" harped bundles at harping points used on WSDOT I girders. 

> NOTE: All end strands in the web strand grid must have positive X values (i.e., filled in pairs) when the "Coerce Odd Number of Harped Strands" option is selected.

> NOTE: The "Use Different Harped Strand Locations at Girder Ends" option is not needed if the relative distance between harped strands is the same along the entire length of the girder (i.e., strands are always parallel). In this case, harping is achieved by adjusting the vertical offset of strand patterns at the girder ends and harping points.

Strand Grid (Potential Strand Locations)
-----------------------------------------
The strand grid lists the possible strand locations and fill order of permanent straight and adjustable strands in the girder. 

Item | Description
-----|-----------
Fill \# | Fill sequence number
Xb,Yb | Strand position at the harping point measured from the bottom center of the cross section
Type | Strand Type
Xt,Yb | Strand position at the ends of the girder measured from the top center of the cross section
[Insert] | Insert a strand position at the current location in the grid
[Append] | Append a strand position at the end of the fill sequence
[Edit] | Edit the strand position description. See @subpage ug_library_dialogs_strand_location.
[Delete] | Delete the selected strand positions
[Move Up] | Moves a selected strand position up in the fill sequence
[Move Down] | Moves a selected strand position down in the fill sequence
[Reverse Adjustable Strand Sequence] | Reverses the fill sequence of the adjustable strands
[@subpage ug_library_dialogs_generate_strands] | Activates a tool to generate a uniform layout of strand positions

> NOTE: Double click on a strand position to quickly edit its properties

> NOTE: The strand grid shows the strand positions on both sides of the vertical axis of the cross section

> NOTE: Strands listed as "Straight" are straight strands and cannot be debonded

> NOTE: Strands listed as "Straight-DB" are straight strands that can be debonded

Grid Status
-------------
The Grid Status region provides summary information about the strand grid. 

Item | Description
----|----------
\# Debondable Strands | Number of strand positions that have been designated as debondable
\# Straight Strands | Number of strand positions used for straight strands
\# Harped Strands | Number of strands positions used for harped strands
[View at Ends] | Press this button to see the strand positions at the end of the girder
[View at Mid-Girder] | Press this button to see the strand positions at the center of the girder

Vertical Adjustment of Adjustable Strands
------------------------------------------------------
The parameters defined in this group control the vertical range over which adjustable strands can be moved from their default position as defined in the strand grid as well as the adjustment increment used for design. Harped strands can be offset differently at the girder ends than at harping points, thus forming the harped drape. Adjustable Straight Strands use a single offset because the strands are straight and parallel to the top of the girder.

Item | Description
----|------
Allow | Check the box to allow vertical adjustment
Design Increment | In general, adjustable web strands can be adjusted continuously up and down within their adjustment limits through user-input adjustment values in the girder editing dialog. The design increment defines the step size to be used by the automated design algorithm when adjusting strand offsets. The increment must be less than or equal to the maximum adjustment value. If this value is zero, the design algorithm will make continuous adjustments within the offset limits.
Lower Strand Limit<BR>Upper Strand Limit | These values define the range of possible uppermost and lowermost adjustable strand Y locations. Adjustable strands may not lie above or below these limits. Limits can be measured downward from the top of the girder or upward from the bottom.
