Connections {#ug_dialogs_pier_description_connections}
==============================================
Define the connection geometry at this pier.

Support Geometry
-----------------
Define the geometry of the connection.

Item | Description
-----|-----------
Segment Connection | Select the segment connection type from the drop down list
Installation Event | Select the event when this closure joint is installed. See NOTE below. 
Bearing Offset | Distance from the pier line to the centerline bearing. Use the drop down list to select how this parameter is measured.
End Distance | Locates the end of the segments. Use the drop down list to select how this parameter is measured
Support Width | Width of the support at the centerline bearing. Measured along the centerline of the segment. 

> NOTE: All closure joints at piers are installed during the same construction event. Changing the installation event for this closure joint changes it for all closure joints at this support. 

Diaphragm
------------
Define the cross section of the diaphragm at this pier.

Item | Description
-----|-----------
Width | Enter the width of the diaphragm, normal to the pier or enter the keyword "Compute" to compute the width based on the connection geometry.
Height | Enter the height of the diaphragm or enter the keyword "Compute" to compute the height based on the connection geometry.