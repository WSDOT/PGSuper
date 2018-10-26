Connections {#ug_dialogs_pier_description_connections}
==============================================
Define the connection boundary conditions and geometry at a pier.

> NOTE: "Back" refers to the back on station side and "Ahead" refers to the ahead on station side of the abutment/pier.

Boundary Condition
------------------
Use the drop down list to select a connection boundary condition.

Connection Definition
---------------------
Define the geometry of the connection. Press [Copy Connection Geometry from Library] to copy pre-defined connection geometry from the library.

### Girder Connection Properties ###
Define the geometric parameters that locate the centerline of bearing and ends of girders. The schematic image illustrates how the dimensions are measured.

Item | Description
-----|---------------
Bearing Offset | Distance from the abutment/pier line to the centerline of bearing line. Use the drop down list to define how this dimension is measured.
End Distance | Distance from the centerline of bearing line to the nearest end of the girder. Use the drop down list to define how this dimension is measured.
Support Width | Width of the support (bearing). This dimension is always measured along the girder. The support area is always centered on the centerline bearing and half the support width is used to define the face of support for various shear related computations.

Diaphragm
-----------
Defines the geometry of a cast in place diaphragm.

Item | Description
-----|------------
Diaphragm Height | Height of the diaphragm. Enter the keyword "Compute" to have this value computed based on the girder size and connection geometry.
Diaphragm Width | Cross sectional width of the diaphragm. Enter the keyword "Compute" to have this value computed based on the girder size and connection geometry.
Load Application | Use the drop down list to define how the weight of the diaphragm is applied to the model.
Distance from Abutment/Pier Line to C.G. of Diaphragm | If "Apply weight of diaphragm to girder" is selected for the load application option, this parameter defines the distance from the Abutment/Pier line to the center of gravity of the diaphragm.
