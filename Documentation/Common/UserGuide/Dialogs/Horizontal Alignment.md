Horizontal Alignment {#ug_dialogs_horizontal_alignment}
==============================================
Define the horizontal alignment of the roadway. The horizontal alignment is defined by a sequence of horizontal curves.

The back tangent of the first curve and the forward tangent of the last curve are project backwards and forwards along the alignment.

Alignment Definition
----------------------

Item | Description
-----|------------
Initial Direction | Enter the direction of the alignment as a bearing in the direction of increasing station. For alignments with horizontal curves, enter the back tangent of the first curve.
Horizontal Curves | Add a row to the table for each horizontal curve in the alignment. 
[Add] | Add a horizontal curve
[Remove] | Remove the selected horizontal curves
[Sort] | Sort the horizontal curves by PI Station

### Horizontal Curve Parameters ###

Horizontal curves are defined in sequence. The forward tangent of the previous curve is taken to be the back tangent of the next curve. The initial direction is used as the back tangent of the first curve. Leave the curve table empty for a straight alignment.

Item | Description
-----|----------
PI Station | Station at the intersection of the back and forward tangents to the curve
Forward Tangent or Delta | Enter the forward tangent or the overall delta angle of the curve
Radius | Enter the curve radius
Entry Spiral | Enter the length of the entry spiral or 0.0 if there is not an entry spiral
Exit Spiral | Enter the length of the exit spiral or 0.0 if there is not an entry spiral

NOTE: A PI angle point can be modeled by defining a horizontal curve with a Radius, Entry Spiral Length and Exit Spiral Length of zero.

Horizontal Alignment Control Point
---------------------------------------
The horizontal alignment control point defined the Northing and Easting of a specific point on the alignment.


Item | Description
-----|----------
Station | Station where the horizontal control point is defined.
East (X) | The East/X value of the control point
North (Y) | The North/Y value of the control point

> NOTE: Coordinates at the start and end of the girders and the points of bearing are given in the Bridge Geometry Report. The horizontal control point defines the coordinate system.

