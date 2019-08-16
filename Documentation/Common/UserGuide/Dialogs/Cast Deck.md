Cast Deck {#ug_dialogs_cast_deck}
==============================================
Define properties for the deck casting activity

Select the deck placement method.
Option | Description
-----|---------------
Continuous placement | The entire deck is placed in one continuous operation
Staged placement | The deck is placed in stages defined by deck regions and the casting sequence

Define the timing parameters
Item | Description
-----|---------------
Age of concrete when continuity is achieved | Enter the age of the concrete when it has sufficient strength to be consider composite with the girders
Curing Duration | Enter the duration of active curing of the concrete. Shrinkage strains are assumed to begin when curing ends.
Time between casting | For staged placement, enter the timing between the start of each placement

Define the placement regions
When Stage placement is selected, the deck concrete is placed in regions in the specified sequence. Regions with the same sequence number are placed at the same time. The general layout of regions is automatically determined based on the structure framing. Regions roughly correspond to the positive and negative moment areas. The specific boundaries of the regions are defined by the engineer. The negative moment regions are defined by boundaries relative to the intermediate piers. Positive moment regions are located between the negative moment regions in the vicinity of the middle of spans. Region boundaries can be normal to the alignment or parallel to a pier.

Item | Description
-----|---------------
Region | A number identifying a placement region
Span | Span number and pier to pier span length, based on layout stationing
Pier | Pier number and location of region boundary measured from the centerline pier. Region boundaries are parallel to the pier and are located by an absolute distance or a fraction of the length of the adjacent span
Back | Distance from the centerline of pier to region boundary on the back side of the pier
Ahead | Distance from the centerline of pier to the region boundary on the ahead side of the pier
Sequence | The placement sequence number. Any positive integer can be used as the sequence number. Regions with lower sequence numbers are placed before regions with higher sequence numbers. Sequence numbers do not need to be consecutive
Region boundaries are | Select from the drop down list how the deck casting region boundaries are defined.

> NOTE: User defined loads applied in the same construction event as the Cast Deck activity will be applied concurrently with the first casting in the casting sequence.


