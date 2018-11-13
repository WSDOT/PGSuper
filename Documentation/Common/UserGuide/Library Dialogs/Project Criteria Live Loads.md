Live Loads {#ug_library_dialogs_project_criteria_live_loads}
==============================================
Define project criteria for live loads.

HL93 Live Load
---------------
Check the box to include the dual design tandem and lane load as described in LRFD C3.6.1.3.1. The design tandem is sometimes refered to as the "low boy" vehicle.

Live Load Distribution Factors
-------------------------------
This option allows you to select the default method for computing live load distribution factors and related parameters.

Item | Description
-----|----------------
Method | Select the method for computing live load distribution factors. Options are AASHTO LRFD Specifications,  WSDOT Bridge Design Manual, or TxDOT Bridge Design Manual.
Use (Number of Lanes)/(Number of Beams) as the Lower Limit for All Distribution Factors | When checked the live load distribution factor will be not be taken less than (Number of Lanes)/(Number of Beams).of Beams).
Maximum Angular Deviation Between Girders | The equations in LRFD 4.6.2.2 may be used to compute distribution factors as long as "Beams are parallel and have approximately the same stiffness". However, no guidance is given as to when girders are not parallel. Enter the maximum allowable angular deviation between girder where you consider girders to be "parallel". 
Maximum Girder Stiffness Ratio | The equations in LRFD 4.6.2.2 may be used to compute distribution factors as long as "Beams are parallel and have approximately the same stiffness". However, no definition is given for "approximately the same stiffness". Enter the maximum percentage that Ix can vary between girders in a span.
Location to Measure Girder Spacing For Distribution Factors | Enter the fractional distance, measured from the ends of a span, for the location(s) where girder spacing is to be measured in order to determine the value of S and de for use in the live load distribution factor equations.
Use rigid method... | When checked, the rigid method, as described in LRFD 4.6.2.2.2d, will be used as the minimum value of live load distribution factors for moment in exterior beams. Starting with LRFD 7th Edition, 2014, the rigid method is only applicable to steel beam-slab bridges. However, some owner/agencies still require the use of this method.

> NOTES:
> * The location of maximum deck width will be used to determine the spacing used to compute the live load distribution factors.
> * Spacings are measured normal to the alignment at the station of the location determined above.
> * Most LRFD distribution factor methods use a single value (S) for girder spacing as a parameter. However, PGSuper allows spacing to vary (splay), if desired. If the spacing to the left and right side of the girder are different, PGSuper will use the average of the two adjacent spacings where a single value is needed. Note that the actual spacings are used when using the lever rule or statical method.
> * If the fractional distance is not 0.5, the width of the bridge is evaluated at two locations. If the width of the bridge is the same at both locations, the section that is nearest the start of the span will be used.

> TIP: You can forego the computation of live load distribution factors and enter your own values.
 
Pedestrian Live Load (LRFD 3.6.1.6)
------------------------------------
Define the pedestrian live load.

Item | Description
-----|-------------
Magnitude of Pedestrian Live Load | Enter the load
Minimum width of sidewalk to apply pedestrian live load | Enter the minimum width of sidewalk to apply pedestrian live load. The load will not be applied to narrower sidewalks.

> NOTE: Pedestrian loading will be applied only over the sidewalk width and will be distributed between the same girders as sidewalk dead loads.
