Select Strands {#ug_dialogs_girder_select_strands}
==============================================
Define strand locations in the girder by specifying any location in the Girder Library strand grid. 

Select Strands
--------------

The strand grid lists all possible strand locations in the girder. The schematic to the right shows the positions of the strands in the girder section. 

Click the check box in the "Select" column to add or remove a strand. You can also click on the strand locations in the picture.

Several types of strands are available depending on the current girder library entry:
* S  - Straight permanent strand
* S-DB - Straight, debondable permanent strand
* H - Harped strand
* A-S - Straight adjustable strand
* T - Temporary strand

Refer to the User Guide section on strands for more information about strand types.

The numbering that is shown in the grid and the schematic represents the position of possible strand locations, not the actual strand numbers.

Click the check box in the "Debond" column to debond a strand. Enter the debond length measured from the end of the girder in the "Debond Length" column. If the debonding is the same at both ends of the girder, check the "Symmetrically Debonding" box beneath the grid.

> NOTE: Debonding is only available if permanent straight strands have been designated as "debondable" in the girder library.

Straight strands that are not debonded may be designated as extended by checking the box in the Extend Left and Extend Right columns.

> NOTE: Strands may only be extended if the Allow Extended Strand Strands option is enabled in the Project Criteria.

> NOTE: Extended strands are considered 100% developed over their entire length in the Strength limit states.

Vertical Adjustment of Adjustable (Harped or Straight-web) Strands
-------------------------------------------------------------------------
Select a value in the drop-down boxes to adjust the vertical location of Adjustable Strands in the girder section. Note that the entire adjustable strand group is adjusted as a unit (i.e., individual strands cannot be adjusted independently). 

The availability and allowable ranges for strand adjustment are specified in the girder library entry's Permanent Strands tab. Adjustments are only available if adjustable strands exist and adjustment is allowed by the library entry.

> NOTE: The drop boxes only allow selection of locations based on the design adjustment increment specified in the girder library entry. If another value is needed, real number values can be entered on the Strands tab of the Girder Details dialog.

> TIP: adding or removing adjustable strands can affect the allowable ranges of strand adjustment. Values will be forced into the allowable range if strands are added to make the current value invalid.

View Settings
--------------
Select whether to view the section at the ends of the girder, or at mid-span (between harping points)

Click on the check box to show or hide strand numbers. This may be helpful if the view becomes congested.

