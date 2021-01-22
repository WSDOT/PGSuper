Connections {#ug_dialogs_pier_description_connections}
==============================================
Define the connection boundary conditions and geometry at a pier.

> NOTE: "Back" refers to the back on station side and "Ahead" refers to the ahead on station side of the abutment/pier.

Boundary Condition
------------------
Use the drop down list to select a connection boundary condition.

Item | Description
-----|-------------------------
Hinge | Idealized "knife edge" support. Vertical and horizontal support.
Roller | Idealized "roller" support. Vertical support only.
Continuous | Vertical and horizontal support. No moment connection with pier. Used for bridges without decks. Continuity occurs prior to placement of superimposed dead loads.
Continuous after deck placement | Vertical and horizontal support. No moment connection with pier. Continuity occurs prior to placement of superimposed dead loads.
Continuous before deck placement | Vertical and horizontal support. No moment connection with pier. Continuity occurs before placement of deck concrete.
Integral | Vertical, horizontal, and rotational support. Full moment connection with pier. Used for bridges without decks. Continuity occurs prior to placement of superimposed dead loads.
Integral after deck placement | Vertical, horizontal, and rotational support. Full moment connection with pier. Continuity occurs prior to placement of superimposed dead loads.
Integral before deck placement | Vertical, horizontal, and rotational support. Full moment connection with pier. Continuity occurs before placement of deck concrete.
Hinged on back side; Integral on ahead side | Hinged on back side of pier and integral on ahead side. See Hinge and Integral boundary conditions.
Hinged on back side; Integral on ahead side after deck placement | Hinged on back side of pier and integral on ahead side. See Hinge and Integral after deck placement boundary conditions.
Hinged on back side; Integral on ahead side before deck placement | Hinged on back side of pier and integral on ahead side. See Hinge and Integral before deck placement boundary conditions.
Integral on back side; Hinged on ahead side | Integral on back side of pier and hinged on ahead side. See Integral and Hinge boundary conditions.
Integral on back side after deck placement; Hinged on ahead side | Integral on back side of pier and hinged on ahead side. See Integral after deck placement and Hinge boundary conditions.
Integral on back side before deck placement; Hinged on ahead side | Integral on back side of pier and hinged on ahead side. See Integral before deck placement and Hinge boundary conditions.

### Girder Connection Properties ###
Define the geometry of the connection. Press [Copy Connection Geometry from Library] to copy pre-defined connection geometry from the library.

Define the geometric parameters that locate the centerline of bearing and ends of girders. The schematic image illustrates how the dimensions are measured.

Item | Description
-----|---------------
Bearing Offset | Distance from the abutment/pier line to the centerline of bearing line. Use the drop down list to define how this dimension is measured.
End Distance | Distance from the centerline of bearing line to the nearest end of the girder. Use the drop down list to define how this dimension is measured.

> NOTE: The bearing offset cannot be measured along the centerline of girder when girder spacing is measured at the centerline bearing. In the general case, girders are not parallel and this will result in a unique bearing location relative to the centerline pier and there would not be a single, continuous bearing line through the bearing points for all girders.

### Pier Diaphragm Dimensions ###

Defines the geometry of a cast in place diaphragm.

Item | Description
-----|------------
Diaphragm Height | Height of the diaphragm. Enter the keyword "Compute" to have this value computed based on the girder size and connection geometry.
Diaphragm Width | Cross sectional width of the diaphragm. Enter the keyword "Compute" to have this value computed based on the girder size and connection geometry.
Load Application | Use the drop down list to define how the weight of the diaphragm is applied to the model.
Distance from CL Bearing to C.G. of Diaphragm | If "Apply weight of diaphragm to girder" is selected for the load application option, this parameter defines the distance from the CL Bearing to the center of gravity of the diaphragm. This is the moment arm for the diaphragm dead load. If the moment arm exceeds the girder end distance, an equivalent force and moment will be applied at the end of the segment.

> NOTE: See @ref tg_computing_pier_diaphragm_dimensions for more information about automatically computed diaphragm dimensions.

### Face of Support ###
The face of support for various shear releated computations is based on the connection and diaphragm geometry as well as the boundary conditions. See @ref tg_face_of_support for more information.

