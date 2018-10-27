Define Strands {#ug_dialogs_girder_define_strands}
==============================================
Ignore the strand locations defined in the Girder Library and define the strand locations for this girder.

Item | Description
-----|----------------
Strand Grid | Strand rows are defined in this grid. See below for details.
[Add] | Add a row to the strand grid
[Remove] | Remove the selected row from the strand grid
Permanent Strands | Select the permanent strand size and type from the drop down list. If the permanent strands are epoxy coated, check the "Grit impregnated epoxy coating" box. See @ref tg_epoxy_coated_strands in the @ref technical_guide for more information.
Temporary Strands | Select the temporary strand size and type from the drop down list. Temporary strands are never epoxy coated.
Pjack | For each strand type, the maximum jacking force is shown. Check the box to enter a different jacking force.

Strand Grid
------------
Strands are defined in horizontal rows. Multiple rows can be defined at the same elevation in the girder section. You must ensure the clear distance between strands is acceptable. The information in the strand row grid is:

Item | Description
-----|-----------
Spacing, S1 | Spacing between the two strands that straddle the vertical centerline of the girder. Enter a value of zero if there is a strand on the vertical centerline.
Spacing, S2 | Spacing between strands in a row.
Type | Use the drop down list to select the strand type.
Left End, Y | Elevation of the strand row at the left end of the girder.
Left End, Face | Use the drop down list to select the face of the girder Y is measured from.
Left Harp Pt, Dist | Enter the distance from the left end of the girder to the first strand deviation point. Use the drop down list to select the measurement unit.
Left Harp Pt, X | Enter the location of the left harp point from the left end of the girder. Use the drop down list to select the measurement unit.
Left Harp Pt, Y | Elevation of the strand row at the left harp point of the girder.
Left Harp Pt, Face | Use the drop down list to select the face of the girder Y is measured from.
Right Harp Pt, X | Enter the location of the right harp point from the left end of the girder. Use the drop down list to select the measurement unit.
Right Harp Pt, Dist | Enter the distance from the left end of the girder to the last strand deviation point. Use the drop down list to select the measurement unit.
Right Harp Pt, Y | Elevation of the strand row at the right harp point of the girder.
Right Harp Pt, Face | Use the drop down list to select the face of the girder Y is measured from.
Right End, Y | Elevation of the strand row at the right end of the girder.
Right End, Face | Use the drop down list to select the face of the girder Y is measured from.
Ext. Strands, Left | Check if the strands are extended at the left end of the girder.
Ext. Strands, Right | Check if the strands are extended at the right end of the girder.
Debond, Left | Check if the strands are debonded at the left end of the girder. Enter the debond length.
Debond, Right | Check if the strands are debonded at the right end of the girder. Enter the debond length.

A girder can have up to four harp points. The images below illustate the two typical harp point layouts.

![](TwoPointHarpedStrands.png)(Typical two point harped strand. Left Harp Pt Dist = 0%, Right Harp Pt Dist = 100%)
<br>
<br>
![](FourPointHarpedStrands.png)(Typical three point harped strand. This strand arrangement could be used if the left end of the girder were a cantilever. Another strand deviation could be added at the right end of the girder by changing the Right Harp Pt Dist value)
