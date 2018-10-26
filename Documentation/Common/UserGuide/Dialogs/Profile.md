Profile {#ug_dialogs_profile}
==============================================
The roadway profile is defined by a series of grade breaks (PVI) and vertical curves.

The profile begins with a reference point before the start of the bridge and then is defined by a sequence of vertical curves.

The entry grade of the first vertical curve and the exit grade of the last vertical curve are projected backwards and forwards along the profile.

Item | Description 
-----|------------
Station | Station of the reference point
Elevation | Profile elevation at the reference point
Grade | Profile grade at the reference point in the direction of increasing station
Vertical Curves | The vertical curves for this profile
[Add] | Adds a vertical curve
[Remove] | Deletes the selected vertical curves
[Sort] | Sorts the vertical curves based on PVI Station

Vertical curves are defined by the following parameters:
Item | Description
-----|-------------
PVI Station | Point of Vertical Intersection station
Exit Grade | The profile grade at the exit point of the vertical curve
L1 | Distance from the beginning of the vertical curve to the PVI, or the total length of the vertical curve if L2 is left blank
L2 | Distance from the PVI to the end of the vertical curve.

> NOTE: The PVI of the first vertical curve can be used as the Reference Point

> NOTE: The exit grade of the previous curve is taken to be the entry grade of the next curve.

> NOTE: The total length of the vertical curve is L1 + L2. 

> NOTE: For symmetrical vertical curves, enter the total curve length as L1 and leave L2 blank.

> NOTE: Leave L1 and L2 blank to define a point of vertical intersection (PVI) for a grade break.

> NOTE: While grade breaks do occur, they are not common. When a grade break is modeled, a warning will be put in the Status Center telling you that a zero-length vertical curve has been modeled. If you intended to model a grade break then no action is necessary. If you meant to model a vertical curve, then this status item will alert you to the input error.
