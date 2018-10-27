Bearing Description {#ug_dialogs_bearings}
==============================================
Bearing information can be described uniformly or uniquely for different locations on the bridge. The following options are available:
* Same for the entire bridge
* Uniquely for each bearing line (in Pier or Span Description dialogs)
* Uniquely at the each end of every girder 

Bearing Data
------------
![](BearingDescription.png)


Item | Description
-----|----------
Bearing Shape | Select the shape of the bearing. Bearings can be rectangular or round. At this time, this setting is for graphical presentation purposes only
Number of Bearings | Enter the number of bearings at each girder
Spacing | If the number of bearings is more than one, enter the distance between bearings. Note that a Status Center Warning will be issued if the bearing layout is wider than the bottom width of the section.
Length or Diameter | For rectangular bearings, enter the length of the bearing measured along the girder CL. For round bearings, enter the diameter.
Width | Enter the width (measured transversly to girder) of the rectangular bearing.
Height | Enter the vertical height of the bearing
Recess Height | Enter the vertical distance that the bearing is cut into the girder. This is measured at the CL of bearing
Recess Length | Enter the length of the recess cut into the girder as measured along the girder C.L.
Sole Plate | Enter the vertical height of the sole plate attached to the bottom of the girder. The sole plate will be recessed into the girder if a recess is specified.

> NOTE: The bearing length is used to compute the critical section for shear, which is located at one-half of the bearing length from the C.L. bearing.

> TIP: Use the bearing dimensions to define the temporary supports (often oak blocks) during construction for continuous and integral piers. The girder setting elevation will be computed and reported as the bearing seat elevation.