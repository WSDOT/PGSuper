Cast Deck {#ug_dialogs_cast_deck}
==============================================
Define properties for the deck casting activity

Select the deck placement method.
Option | Description
-----|--------------
Continuous placement | The entire deck is placed in one continuous operation
Staged placement | The deck is placed in stages defined by deck regions and the casting sequence

Define the timing parameters
Item | Description
-----|------------
Total curing duration | Enter the total curing duration. Concrete is assumed to be fully cured when it has sufficient strength to be consider composite with the girders
Active curing Duration | Enter the duration of active curing of the concrete. Shrinkage strains are assumed to begin when active curing ends.
Time between casting | For staged placement, enter the timing between the start of each placement

Closure Joints
--------------
A closure joint cast in the same event as the deck casting is assumed to be cast at the same time as the deck for continuous placement. For staged deck placement, the casting of all the closure joints in the event can be cast at the same time as any one of the deck casting regions. Use the drop down list to select the deck casting region that the closure joints are cast along with.

> NOTE: The precast segments become a continuous girder when the total curing duration of the closure joint is complete. Closure joints must be cast and fully cured before PT tendons can be stressed.

Define the placement regions
-----------------------------
When Staged placement is selected, the deck concrete is placed in regions in the specified sequence. Regions with the same sequence number are placed at the same time. The general layout of regions is automatically determined based on the structure framing. Regions roughly correspond to the positive and negative moment areas. The specific boundaries of the regions are defined by the engineer. The negative moment regions are defined by boundaries relative to the intermediate piers. Positive moment regions are located between the negative moment regions in the vicinity of the middle of spans. Region boundaries can be normal to the alignment or parallel to a pier.

The deck casting regions are roughly divided into positive and negative moment regions. The boundaries of these regions are specified in the Back and Ahead fields of the grid.

Item | Description
-----|------------
Region | A number identifying a placement region
Type | A mnemonic identifying the region type; +M for positive moment, -M for negative moment.
Span | Span number and pier to pier span length, based on layout stationing
Pier | Pier number and location of region boundary measured from the centerline pier. Region boundaries are parallel to the pier and are located by an absolute distance or a fraction of the length of the adjacent span
Back | Distance from the centerline of pier to region boundary on the back side of the pier
Ahead | Distance from the centerline of pier to the region boundary on the ahead side of the pier
Sequence | The placement sequence number. Any positive integer can be used as the sequence number. Regions with lower sequence numbers are placed before regions with higher sequence numbers. Sequence numbers do not need to be consecutive
Region boundaries are | Select from the drop down list how the deck casting region boundaries are defined.

> NOTE: User defined loads applied in the same construction event as the Cast Deck activity will be applied concurrently with the first casting in the casting sequence.

> NOTE: The total curing duration of the deck region defines when the deck becomes composite with the precast girder segments and closure joint. The deck does not create continuity between precast segments.


