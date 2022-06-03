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
Elevation Adjustment | This feature has been disabled for this version of the program

**Note that the temporary support elevation adjustment feature has been disabled for this version of the program due to bugs that will be addressed in the next version. Any existing adjustments from previous versions of the program have been set to zero. If adjustments are needed, you will need to compute them by hand.**