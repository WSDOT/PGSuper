General {#ug_dialogs_segment_details_general}
==============================================
Define the general attributes of a precast segment.

Events
--------
Select the construction events for this segment.

Item | Description
-----|-----------
Construction | Select the event when the segment is constructed
Erection | Select the event when the segment is erected

Section Variation
--------------------
Define variations in the segment depth.

Item | Description
-----|----------------
Variation Type | Select a segment profile variation type from the drop down list. If the only option in the list is "None" this girder types does not support profile variations.
Length | Enter the length of the variation
Height | Enter the overall height of the segment at the end of the variation
Bottom Flange Depth | If checked, the bottom flange depth is also variable. Enter the depth of the bottom flange at the ends of the variations.

Precast segments can also have end blocks.

Item | Description
-----|-------------
Length | Length of the constant width portion of the end block
Transition Length | Length of the transition from the end block section to the typical section
Width | The width of the end block at the end of the girder

See @ref ug_girder_modeling_defining_a_girder in the @ref user_guide for more details about defining variations in the segment depth.

Precast Segment Concrete Properties
-------------------------------------
Define the properties of the segment concrete.

Item | Description
-----|----------
f'ci/f'c | Enter the concrete strength
Eci/Ec | When checked, enter the modulus of elasticity, otherwise it is computed
[More Properties...] | Press to define additional details.