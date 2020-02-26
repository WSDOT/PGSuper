Connection {#ug_dialogs_temporary_support_connection}
==============================================
Define the segment connection and support geometry for this temporary support

Segment Connection
-------------------
The segment connection describes how the precast segment(s) are connected at the temporary support. The connection type defines the boundary conditions for structural analysis. The installation event for closure joints is also defined if applicable.

Item | Description
-----|---------------
Segment Connection | Select the segment connection type from the drop down list
Installation Event | Select the event when this closure joint is installed. See NOTE below.

Segment connection types are:

Connection | Description
-----------|------------
Closure Joint      | Segments are joined at the temporary support with a cast-in-place closure joint.
Continuous Segment | A single segment spanning over the temporary support. This connection type cannot be used with Strongback temporary supports

See @ref tg_structural_analysis_models in the @ref technical_guide for more information on boundary conditions and structural analysis.

> NOTE: All closure joints at temporary supports are installed during the same construction event. Changing the installation event for this closure joint changes it for all closure joints at this support.


Support Geometry
-----------------
Define the geometry of the connection.

Item | Description
-----|-----------
Bearing Offset | Enter the distance from the temporary support line to the centerline bearing. Use the drop down list to define how this distance is measured.
End Distance | Enter the distance that defines the location of the end of the segment. Use the drop down list to define how this disance is measured
