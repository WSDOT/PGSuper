Structural Analysis Models {#tg_structural_analysis_models}
======================================
This section describes the analytical models used to calculate structural analysis results.

Model Geometry
------------------
Plane frame line models are used for structural analysis. The length of the spans in the analysis model are equal to the Centerline Bearing to Centerline Bearing length before continuity and Centerline Bearing to Centerline Pier after continuity. The centerlines of bearing are located using the connection dimensions.

![](AnalysisModel.png)

### Supporting Elements ###
Abutments are always considered to be supported by idealized elements such as pins or rollers. The mapping between the abutment boundary condition and the analysis model are:

Boundary Condition | Analysis Model
-------------------|---------------
Hinge | Modeled as an idealized knife-edge support. Supported in the global X and Y direction with no rotational restraint
Roller | Modeled as an idealized roller support. Supported in the global X direction with no transverse (Y) or rotational restraints.
Integral before deck placement | Modeled as a fully fixed connection. Supported in the X and Y transitaional directions and fully restrained against rotation.
Integral after deck placement | Modeled as a fully fixed connection. Supported in the X and Y transitaional directions and fully restrained against rotation.

Interior piers can be modeled with idealized supports or with substructure elements described by their physical attributes. The mapping of boundary condtion to analysis model is the same as for abutments. When a pier is modeled by its physical attriubutes, a column is modeled in the analysis model with either a fixed or pinned boundary condition. The connection boundary condition defines the connectivity between the superstruture and the substructure.

![](PhysicalPierModel.png)

In the figure above, the first analysis model shows the intermediate pier modeled with an integral connection. The intermediate pier is modeled by fully restraining rotation thereby transferring all moments directly into the support. The second analysis model shows the intermediate pier modeled as an integral pier. The base of the column is fixed and the top of the column has a moment connection with the superstructure. The column stiffness and sidesway effects are taking into account in this model.


The modeled column height is the average clear span height of all the columns in the pier plus one-half the superstructure depth. The cross sectional area and moment of inertia of the column is the total area and moment of inertia for the pier divided by the average number of girders framing into the pier.


Girderlines
--------------
The structural analysis models use the same girder in each span (e.g. Girder B from every span is used to create the model for the first interior girder line). When the number of girders per span is not equal the right exterior girder is used when a span has fewer girders then the girder line for which the model is being constructed. This is illustrated below.

![](GirderLines.gif)

In the illustration above, Spans 1 and 2 have 4 girders and Span 3 has 5 girders. Girder D from spans 1 and 2 are used in the line model for analyzing Girder E.

Loading
---------
The sections that follow describe loads are developed for the various analysis intervals.

### Interval 2: Prestress Release (Casting Yard) ###

![](CastingYardModel.png)

At prestress release, the precast element is supported at the locations defined in the girder description. For PGSuper models, the support location is always at the ends of the cast girder.

The self-weight load varies linearly between changes in cross sectional area. The image above is representitive of a precast element with end blocks.

w = A<sub>g</sub> <span style="font-family:Symbol">g</span><sub>c</sub> <br>
where <br>
A<sub>g</sub> = Gross cross sectional area <br>
<span style="font-family:Symbol">g</span><sub>c</sub> = Unit weight of concrete, including reinforcement

Some girder types, such as U-beams, may be defined with diaphragms that are cast with the girder. The dead load of these diaphragms are modeled as concentrated loads.

### Intervals 3 - 5 ###
Intervals 3 - 5 model the non-composite precast elements during Lifting, Storage, and Hauling. The loading conditions for this analysis intervals are the same as for Interval 2. However, the support location is that for lifting, storage, and hauling, respectively.

### Interval 6: Erect Girders ###
The precast elements have been erected onto the supporting elements of the bridge. They are supported at their specified bearing locations. The loading condition is similar to that Interval 2; however if the length of the cantilever in the analysis model is less than 1.1 times the depth of the non-composite girder section at the centerline bearing (H<sub>g</sub>), the cantilever is assumed to be a deep beam. In this case, the dead load of the cantilever is applied directly over the centerline of bearing as a concentrated load.

In the figure below, the cantilever on the left is greater than 1.1H<sub>g</sub> in length and it is treated as a flexural element. The cantilever on the right is less that 1.1H<sub>g</sub> and is treated as a deep beam where forces are transfered directly to the support through an internal compression strut.

![](ErectedGirderModel.png)

### Interval 7: Cast Deck (Bridge Site 1) ###
The self-weight of cast-in-place intermediate diaphragms as well as the cast deck, precast deck panels, and slab haunch are analyzed. The analysis model is the same as described for Interval 6.

#### Cast Deck Dead Load ####
The dead load of the deck is modeled as a series of linearly varying load segments. The magnitude of the load is taken to be w = A<sub>d</sub> <span style="font-family:Symbol">g</span><sub>c</sub>

where<br>
w = deck load at a specific location<br>
A<sub>d</sub> = cross sectional area of the deck
<span style="font-family:Symbol">g</span><sub>c</sub> = unit weight of deck concrete including reinforcement

For interior girders, A<sub>d</sub> = t<sub>slab</sub> w<sub>trib</sub>

where<br>
t<sub>slab</sub> = cast depth of the slab (gross depth for cast-in-place decks and cast decks for decks with stay-in-place deck panels)<br>
w<sub>trib</sub> = tributaty width of the deck

For exterior girders, A<sub>d</sub> depends on the shape of the cross section of the deck overhang

For bridges with parallel girders, the cast deck load is uniform for interior girders. The load is uniform for exterior girders if the edges of the deck are parallel to the girders and the girders are also parallel to each other.

#### Precast Deck Panel ####
The dead load of the precast deck panel is modeled as linearly load segments taken to be w = t<sub>panel</sub> w<sub>panel</sub> <span style="font-family:Symbol">g</span><sub>c</sub> 

where<br>
t<sub>panel</sub> = thickness of the deck panel<br>
w<sub>panel</sub> = width of the deck panel<br>
<span style="font-family:Symbol">g</span><sub>c</sub> = unit weight of deck concrete including reinforcement

When girder are parallel, the deck panel load is uniform.

#### Slab Haunch ####
The slab haunch load is modeled as a sequence of linear load segments. The basic load is taken to be w = D w<sub>tf</sub> <span style="font-family:Symbol">g</span><sub>c</sub>

where<br>
D = depth of the slab haunch<br>
w<sub>tf</sub> = total width of the all the top flanges for a girder<br>
<span style="font-family:Symbol">g</span><sub>c</sub> = unit weight of deck concrete including reinforcement<br>

The depth of the slab haunch is the distance from the bottom of the deck slab to the top of the girder. There are two methods for modeling the path of the top of the girder; straight and parabolic. The option is defined in the Project Criteria. The methods are as follows:

##### Option 1: Haunch Depth when Top of Girder is assumed to be a Straight Line  #####
The top of the girder is assumed to be straight (zero excess camber) for purposes of computing this dead load. The schematic below illustrates the depth of the haunch for load distribution for this case.

  ![](HaunchLoadLinear.png)

> NOTE: Modern girders have wide top flanges and as such the slab haunch load can be significant. This method of estimating the slab haunch load is conservative (unless the girder has negative excess camber) and it makes an allowance for the actual girder camber being less than predicted. With this approach, girders will not be under-designed if camber is less than predicted.

##### Option 2: Haunch Depth when Top of Girder is assumed to be a Parabola #####
The top of the girder is assumed to follow a parabolic curve defined by the slab offsets at the ends of the girder, and the Fillet dimension. The schematic below illustrates the depth of the haunch for this case.

  ![](HaunchLoadParabolic.png)

> NOTE: For this option it is the responsibility of the user to ensure that haunch dead load parabola defined by the slab offset and fillet dimensions closely matches the parabola defined by the excess camber computed by the program. If the parabolas do not match within tolerance, a specification check failure will occur indicating the haunch dead load was either over or under estimated. 
>     For complex bridge geometries it may be necessary to define a unique fillet and/or slab offset for each girder in the bridge in order to meet this requirement.
>     The Loading Details chapter in the Details report tabulates the haunch depth along the girder.

> NOTE: The shape of the slab haunch is governed, in part, by the girder camber which is a function of the concrete properties, prestressing, and loading conditions. At the onset of design, the girder camber is not known. As such, the slab haunch dead load must be estimated and refined manually. In other words, this software does not perform iterations to minimize varation between the slab haunch shape and the haunch dead load.
  
#### Intermediate Diaphragm Dead Load ####
Diaphragm loads are modeled as concentrated forces. The force for exterior girders is P = HW<span style="font-family:Symbol">g</span><sub>c</sub>(w<sub>trib</sub> - t<sub>web</sub>) and for interior girders P = 0.5HW<span style="font-family:Symbol">g</span><sub>c</sub>(S - t<sub>web</sub>)

where<br>
H = Height of the diaphragm <br>
W = Width of the diaphragm<br>
<span style="font-family:Symbol">g</span><sub>c</sub> = Unit weight of deck concrete including reinforcement<br>
w<sub>trib</sub> = tributary width of deck<br>
t<sub>web</sub> = total width of all webs in the section<br>
S = spacing between girders are the location of the diaphragm, measured along the centerline of the diaphragm<br>

#### Shear Key Dead Load ####
Shear key loads can be automatically generated **only for adjacently-spaced TxDOT Box girders**. To activate the load, specify the ShearKeyDepth variable in the Girder Library entry. 

* The shear key will fill the gap between girders from the specified shear key depth to the tops of the beams and will have the same unit weight as the slab concrete.
* The load is applied in a load case called "Shear Key" applied in Interval 7: Cast Deck (Bridge Site 1) and added to the DC combination.
* Structural stiffness properties of the shear key are not considered in any analyses.
* The width of the shear key will be variable if the joint spacing between girders is variable.
* The shear key load is applied between bearings (no loading on the cantilever portion of the girder).
* Results will only be displayed if a shear key is present


### Interval 8: Install Railing System and Overlay (Bridge Site 2) ###
At this analysis interval, the girders can be continuous over intermediate piers. For continuous or intergral connections, the centerline bearing supports locations are replaced with a single  support location at the centerline of the pier.

![](ContinuousModel.png)

#### Continuity Diaphragm ####
The dead load of continuity diaphragms are applied as concentrated loads at the centerline of the pier.

#### Railing System Loads ####
The dead load of the railing system is computed as the cross sectional area of the railing system times the unit weight of the material. Alternatively, a linear weight per length can be defined included in the railing definition.

Railing system loads consist of dead loads from the traffic barrier, sidewalk, and combination (interior) railing. These loads are distributed to a maximum of N "nearest" girders, mating surfaces, or webs of the bridge section.

A mating surface is the contact point between a girder and the bridge deck. Examples are:

Girder Type | Number of Webs | Number of Mating Surfaces
---------|-----------|------------
I-Beam |  1 | 1
U-Beam |  2 | 2
Box Beam |  2 |  1
Voided Slab |  0 |  1
Double Tee |  2 |  1

<h5>Exterior and Interior Traffic Barriers</h5>
The dead load of exterior and interior barriers are distributed as follows:  

_Distribute the weight of the barrier evenly to the N nearest girders, mating surfaces, or webs (GMSW's). Nearest distance is measured from the C.G. of the barrier in a bridge cross section taken at mid-span. For cases when the weight of a barrier can be distributed to either of two GMSW's that are equal distance left and right of the barrier C.G., and these GMSW's are furthest from the barrier, the load will be distributed to the exterior-most GMSW. If the span contains 2N or fewer GMSW's, the railing load will be distributed evenly to all GMSW's._

<h5>Sidewalks</h5>
The dead load of sidewalks are distributed using a similar method and using the same N value as barriers. However, if the sidewalk is wider than N GMSW's; the load will be distributed to all GMSW's lying directly beneath the sidewalk. Hence, the definition is a follows:

_Distribute the sidewalk weight and pedestrian live load evenly to the greater of: all girders, mating surfaces, or webs (GMSW's) lying directly under the sidewalk; or the N nearest GMSW's measured from centerline sidewalk using a bridge cross section taken at mid-span. For cases when the sidewalk weight can be distributed to either of two GMSW's that are equal distance left and right of C.L. sidewalk, and these GMSW's are furthest from the C.L. sidewalk, the load will be distributed to the exterior-most GMSW. If the span contains 2N or fewer GMSW's, the load will be distributed evenly to all GMSW's._

<h5>Pedestrian Live Load on Sidewalk</h5>
Pedestrian live load on sidewalk is distributed to the same girders, and in the same proportions, as the sidewalk dead load.

<h5>Barrier Load Distribution Example</h5>
Consider for example a bridge section with three U-beams. If the railing system loads are distributed over the 3 nearest girders, the exterior barrier dead load per girder would be 0.33(W<sub>left</sub> + W<sub>right</sub>).

Girder A |  Girder B |  Girder C
---------|-----------|------------
0.33W<sub>left</sub> + 0.33W<sub>right</sub> |  0.33W<sub>left</sub> + 0.33W<sub>right</sub> |  0.33W<sub>left</sub> + 0.33W<sub>right</sub>
 
However, if the loads are distributed over the 3 exterior webs or mating surfaces, the exterior barrier dead load would be:

<table>
<tr><th colspan=2>Girder A</th><th colspan=2>Girder B</th><th colspan=2>Girder C</th></tr>
<tr><td colspan=2>0.67W<sub>left</sub></td><td colspan=2>0.33W<sub>left</sub>+0.33W<sub>right</sub></td><td colspan=2>0.67W<sub>right</sub></td></tr>
<tr><th>Web 1</th><th>Web 2</th><th>Web 1</th><th>Web 2</th><th>Web 1</th><th>Web 2</th></tr>
<tr><td>0.33W<sub>left</sub></td><td>0.33W<sub>right</sub></td><td>0.33W<sub>left</sub></td><td>0.33W<sub>right</sub></td><td>0.33W<sub>left</sub></td><td>0.33W<sub>right</sub></td></tr>
</table>

#### Overlay ####
The self weight of the overlay can be distibuted uniformly to all girders as permitted by LRFD 4.6.2.2.1 or distributed to each girder based on its tributary area.



 