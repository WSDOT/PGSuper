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
Define variations in the segment. Variations include depth, end blocks, and precamber

Item | Description
-----|----------------
Variation Type | Select a segment profile variation type from the drop down list. If the only option in the list is "None" this girder types does not support profile variations.
Length | Enter the length of the variation
Height | Enter the overall height of the segment at the end of the variation
Bottom Flange Depth | If checked, the bottom flange depth is also variable. Enter the depth of the bottom flange at the ends of the variations.
End Block Length | Length of the full end block excluding the transition
End Block Transition Length | Length of the transition from the end of the full end block to the normal cross section
End Block Width | Width of the end block

See @ref ug_girder_modeling_defining_a_girder in the @ref user_guide for more details about defining variations in the segment depth.

Precast Segment Concrete Properties
-------------------------------------
Define the properties of the segment concrete.

Item | Description
-----|----------
f'ci/f'c | Enter the concrete strength
Eci/Ec | When checked, enter the modulus of elasticity, otherwise it is computed
[More Properties...] | Press to define additional details.

Haunch Geometry
---------------
Define the geometry of the slab haunch

> A complete description of vertical bridge geometry, haunch design, and haunch input methods can be found at @ref tg_vertical_geometry

Item | Description
-----|-------------
Haunch Depth | Haunch depth at start and end of segment are shown when applicable

Web Thickening Over Interior Piers
--------------------
For spliced girder segments that span over an interior pier, the web can be thickened in the vicinity of the pier to carry the increased shear demand. This input section is only shown when the girder type supports web thickening and an interior permanent pier falls within the segment.

The thickened zone is symmetric about the pier centerline. The width increases linearly from zero over the taper zone, holds at the full thickening width through the full-width zone, then tapers back symmetrically on the other side.

Item | Description
-----|-------------
Web Thickening Width | The additional width added to each face of the web at the pier centerline (total web increase = 2 × this value)
Pier Thickening Length | Half-length of the constant full-width zone on each side of the pier centerline
Taper Zone Length | Half-length of the linear taper on each side of the full-width zone over which the web width transitions from the nominal value to the full thickening width
Interior Pier Location | Distance from the start of the segment to the centerline of the interior pier (informational, read-only)

The combined extent of the thickening zone (Pier Thickening Length + Taper Zone Length) must be less than the distance from the pier centerline to the nearer end of the segment.

> Web thickening is only available for spliced I-beam and spliced NU-beam girder types.
