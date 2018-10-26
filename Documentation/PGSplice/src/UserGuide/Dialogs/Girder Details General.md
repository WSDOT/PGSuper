General {#ug_dialogs_girder_details_general}
==============================================
Define the details of a spliced girder.

Segments
----------
Click on the hyperlink to toggle between using the same segment type for each girder individually or a single segment type for the entire bridge.

When the segment type is defined for each girder individually, select the segment type from the drop down list. The segment types in the list are taken from the Girder library.

The segment grid lists the segments and closure joints that make up the girder. Press [Edit] to edit the details of these components.


Ducts/Tendons
--------------
Define the geometry of the post-tensioning ducts and the tendons

Item | Description
-----|-------------
Duct Grid | Grid listing the general parameters of the ducts and tendons. See Duct Grid below for details.
Duct Material | Select the duct material. Used for evaluation the provisions of LRFD 5.4.6.1
Strand Size | Select the tendon strand size
Installation Method | Select the strand installation method. Used to evaluate the provisions of LRFD 5.4.6.2

### Duct Grid ###
Enter the following information in the duct grid. The first two items define the duct geometry. The remainder of the items define the tendon and the jacking force parameters.

Item | Description
-----|--------------
Type | Select a duct type from the drop down list. The duct types are defined in the Ducts library
Shape | Select the shape of the duct. Press [Edit] to edit the details of the duct geometry.
# Strands | Enter the number of strands in the tendon
Pjack | Check the box to enter a jacking force, otherwise the maximum jacking force is used
Jacking End | Select the live end for tendon jacking.
Event | Select the event when the tendon is stressed


Slab Offsets
---------------
Click on the hyperlink to toggle between defining the slab offset for each girder individually or a single slab offset for the entire bridge.

When the slab offset is defined for each girder individually, enter the slab offset dimension for each pier supporting this girder.

Condition Factor
-----------------
Select the condition of the girder from the drop down list. If "Other" is selected, enter the condition factor. 