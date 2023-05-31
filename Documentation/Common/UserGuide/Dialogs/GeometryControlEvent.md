Roadway Geometry Control {#ug_dialogs_geometry_control}
==============================================
Define a geometry control activity.

Roadway geometry control activities are used to define milestone events for the bridge vertical geometry analysis and checking. There are three different types of events:

Activity | Description
-----|--------------
The Roadway Geometry Control Event (GCE) | This activity defines when bridge elevations are synchronized to roadway elevations and typically occurs at time first service. Elevation specification checks are performed, and elevations are reported. *There can be only one GCE in the timeline.*
Specification Checking Events | Perform finished roadway elevations specification checks and report finished elevations.
Reporting Events | Report finished elevations only

> Notes:
> - There can be only one Geometry Control Event
> - Spec checking and reporting activities can occur in multiple events, but can only be added to events after deck placement.
> - Top of deck elevations will change over the life of the structure due to time-dependent effects. This activity enables reporting or spec checking to monitor differences between computed top of deck elevation verses the design roadway profile over time.

Learn about how the Roadway Geometry Control Event (GCE) effects vertical geometry analysis in @ref tg_vertical_geometry.
