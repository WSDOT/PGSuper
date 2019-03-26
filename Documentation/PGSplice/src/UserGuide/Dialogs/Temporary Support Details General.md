General {#ug_dialogs_temporary_support_general}
==============================================
Describe the general layout of a temporary support

Item | Description
------|-----------------
Type | Select the temporary support type
Station | Enter the location of the temporary support line
Orientation | Enter the orientation of the temporary support line
Erection Event | Select the event when the temporary support is erected
Removal Event | Select the event when the temporary support is removed

Erection tower temporary supports provide a vertical reaction load path to the ground. Strong back temporary supports connect adjacent segments.

> NOTE: Erection towers provided a vertical load path to the ground for both tension and compression forces. When post-tensioning is applied, the girder may have the tendency to "lift off" the erection town. A hold down (tension) reaction will be producted in the erection tower. Review the erection tower reactions and make sure they are removed at the appropreate stage in the construction timeline.

> NOTE: Strong back temporary supports are essentially hinges. Be sure your model is geometrically stable.

If segments are connected at this temporary support, then slab offsets and an elevation adjustment can be defined.

Item | Description
------|-----------------
Slab Offset Type | Use the drop down list to select a slab offset type
Slab Offset | Distance from top of cast slab to top of precast segment at centerline bearing
Elevaton Adjustment | Enter the elevation adjustment

The slab offset and elevation adjustment define the segment elevation at erection. The elevation is taken to be the profile grade elevation reducted by the slab offset and increased by the elevation adjustment.

The elevation adjustment is typically used to raise or lower the ends of a segment to accomodate deflections that will occur after erection. Such deflections may occur from post-tensioning, temporary support removal, and installation of external loads.
