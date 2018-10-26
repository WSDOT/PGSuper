Loads {#ug_library_dialogs_project_criteria_loads}
==============================================
Define project criteria for loads.

Distribution of Railing System (Barriers, Sidewalk, and Pedestrian) Loads
--------------------------------------------------------------------------
Enter the maximum number of girders, webs, or mating surfaces to distribute the railing system loads. The railing system loads include the dead load of the exterior traffic barrier, sidewalk, and combination (interior) railing as well as pedestrian live load on sidewalk. 

For a more detailed discussion on the distribution of railing system loads, see the @ref tg_structural_analysis_models topic in the @ref technical_guide.

A mating surface is the contact point between a girder and the bridge deck. Examples are:

Girder Type | Number of Webs | Number of Mating Surfaces
-----------|-------------------|-------------------------
I-Beam |  1 |  1
U-Beam |  2 | 2
Box Beam |  2  | 1
Voided Slab |  0 |  1
Double Tee |  2 |  1
 
Pedestrian Live Load (LRFD 3.6.1.6)
------------------------------------
Define the pedestrian live load.

Item | Description
-----|-------------
Magnitude of Pedestrian Live Load | Enter the load
Minimum width of sidewalk to apply pedestrian live load | Enter the minimum width of sidewalk to apply pedestrian live load. The load will not be applied to narrower sidewalks.

> NOTE: Pedestrian loading will be applied only over the sidewalk width and will be distributed between the same girders as sidewalk dead loads.

Distribution of Overlay Dead Load
----------------------------------
Define the method for distributing overlay dead load to the girders. LRFD 4.6.2.2.1 states "permanent loads of and on the deck may be distributed uniformly among the beams and/or stringers". However, many agencies prefer to distribute overlay loads using tributary area. Select your preference here.

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

> NOTES:
> * The location of maximum deck width will be used to determine the spacing used to compute the live load distribution factors.
> * Spacings are measured normal to the alignment at the station of the location determined above.
> * Most LRFD distribution factor methods use a single value (S) for girder spacing as a parameter. However, PGSuper allows spacing to vary (splay), if desired. If the spacing to the left and right side of the girder are different, PGSuper will use the average of the two adjacent spacings where a single value is needed. Note that the actual spacings are used when using the lever rule or statical method.
> * If the fractional distance is not 0.5, the width of the bridge is evaluated at two locations. If the width of the bridge is the same at both locations, the section that is nearest the start of the span will be used.

> TIP: You can forego the computation of live load distribution factors and enter your own values.

Haunch Load
-----------
Select the method for computing the distributed load from the haunch. 
Method | Description
-----|----------------
Assume zero excess camber | The top of the girder is assumed to be straight (zero camber) for purposes of computing the haunch dead load. This is the method used by WSDOT and the only method available for spliced girders.
Excess camber defined by parabola (PGSuper only) | The top of the girder is assumed to follow a parabolic curve defined by the slab offsets at the ends of the girder, and the user-input Fillet dimension. A specification check is performed to insure that the excess camber predicted by the parabola is within tolerance of the computed excess camber.

#### Allowable Tolerance Between User-input and Computed Excess Camber ####
Enter a tolerance for the parabolic case. The tolerance will be used to compare the haunch depth predicted by the user-input fillet to the haunch depth predicted using the computed excess camber at mid-span. A spec check fail results if user-input haunch depth used to apply haunch load is not within tolerance. Make sure to enter a reasonable value for the tolerance (i.e., a value near zero will make it practically impossible to pass the spec check).

> TIP: Refer to the Slab Haunch loading section of @ref tg_structural_analysis_models for detailed information about how the slab offset and fillet dimensions are used for computing the haunch loading.