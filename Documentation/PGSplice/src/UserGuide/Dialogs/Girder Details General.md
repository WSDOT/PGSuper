General {#ug_dialogs_spliced_girder_general}
==============================================
Define the details of a spliced girder.

Segments
----------
From the drop down list, select if using the same segment type for each girder individually or a single segment type for the entire bridge.

When the segment type is defined for each girder individually, select the segment type from the drop down list. The segment types in the list are taken from the Girder library.

The segment grid lists the segments and closure joints that make up the girder. Press [Edit] to edit the details of these components.


Field Installed Tendons
--------------
Define the geometry of the post-tensioning ducts for field installed girder tendons

> NOTE: Girder tendons differ from @ref ug_dialogs_segment_tendons

> NOTE: Be careful to ensure longitindal reinforcement, pretension strands, and post-tensioning ducts do not conflict. Geometric conflicts are not automatically detected.

Item | Description
-----|-------------
Duct Grid | Grid listing the general parameters of the ducts and tendons. See Duct Grid below for details.
Duct Material | Select the duct material. Used for evaluation the provisions of LRFD 5.4.6.1. This setting does not effect the duct friction factor or wobble coefficient. Select Project > Losses to edit duct friction factors.
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

Condition Factor
-----------------
Select the condition of the girder from the drop down list. If "Other" is selected, enter the condition factor. 