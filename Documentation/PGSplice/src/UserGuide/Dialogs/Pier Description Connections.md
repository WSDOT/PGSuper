Connections {#ug_dialogs_pier_description_connections}
==============================================
Define the segment connection and support geometry for this pier.

Segment Connection
------------------
The segment connection describes how the precast segment(s) are connected at the pier. The connection type defines the boundary conditions for structural analysis. The installation event for closure joints is also defined if applicable.

Item | Description
-----|------------
Segment Connection | Select the segment connection type from the drop down list.
Installation Event | Select the event when this closure joint is installed. See NOTE below. 

When two segments are joined at a pier, the connection has a closure joint. Contiuous connections are used when the girder is continuous over the pier and there is not a moment connection to the pier. Integral connections are used when the girder and pier have moment connectivity.

Connection | Description
-----------|------------
Continous Closure Joint | Segments are joined at the pier with a cast-in-place closure joint. The girder is continuous over the pier.
Integral Closure Joint | Segments are joined at the pier with a cast-in-place closure joint. The girder is integral with the pier.
Continuous Segment | A single segment spanning over the pier. The girder is continuous over the pier after the pier diaphragm is cast.
Integral Segment | A single segment spanning over the pier. The girder is integral with the pier after the pier diaphragm is cast.

See @ref tg_structural_analysis_models in the @ref technical_guide for more information on boundary conditions and structural analysis.

> NOTE: All closure joints at piers are installed during the same construction event. Changing the installation event for this closure joint changes it for all closure joints at this support. 

Support Geometry
-----------------
Define the geometry of the connection.

Item | Description
-----|-----------
Bearing Offset | Distance from the pier line to the centerline bearing. Use the drop down list to select how this parameter is measured.
End Distance | Locates the end of the segments. Use the drop down list to select how this parameter is measured

> NOTE: The bearing offset cannot be measured along the centerline of girder when girder spacing is measured at the centerline bearing. In the general case, girders are not parallel and this will result in a unique bearing location relative to the centerline pier and there would not be a single, continuous bearing line through the bearing points for all girders.

Diaphragm
------------
Define the cross section of the diaphragm at this pier.

Item | Description
-----|-----------
Width | Enter the width of the diaphragm, normal to the pier or enter the keyword "Compute" to compute the width based on the connection geometry.
Height | Enter the height of the diaphragm or enter the keyword "Compute" to compute the height based on the connection geometry.

See @ref tg_computing_pier_diaphragm_dimensions in the @ref technical_guide for more information.

Face of Support
-------------------
The face of support for various shear releated computations is based on the connection and diaphragm geometry as well as the boundary conditions. See @ref tg_face_of_support for more information.
