Deck Geometry and Materials {#ug_dialogs_bridge_description_deck_geometry}
==============================================
The deck type select from the @ref ug_dialogs_bridge_description_general tab and the deck casting event are listed at the top of this tab. Use the drop down list to select the deck casting event (PGSplice only).

Cross Section
-------------
Define the cross section of the deck

Item | Description
-----|-------------
Gross Depth | Enter the gross depth of the cast-in-place slab
Cast Depth | Enter the cast depth of a deck build with stay-in-place precast deck panels
Overhang Edge Depth | Enter the depth of the deck at the edge of the overhang
Overhang Taper | Use the drop down list to select the shape of the deck overhang
Panel Depth | Depth of the stay-in-place precast deck panels
Panel Support Width | Support width of the stay-in-place precast deck panels

Plan (Edge of Deck)
-------------------
Define the edge of deck in plan view.

Item | Description
-----|-------------
Deck Edge Offset grid | Enter offsets to the left and right edge of the deck. (See below for more details)
[Add] | Add a deck edge offset point.
[Remove] | Remove the selected deck edge offset point.

### Deck Edge Offset Grid ###

Item | Description
-----|-------------
Station | Station where the deck edge point is defined
Measured From | Select the location from which the offsets are measured
Left Offset | Normal offset from the specified location to the left edge of the deck.
Right Offset | Normal offset from the specified location to the right edge of the deck.
Transition | Select a transition type from the drop down list.

The deck edge offset point transition defines how the edge of deck to the next.

Transition | Description
-----|-------------
Parallel | The edge of deck parallels the alignment and bridge line. The offset at the next point is exactly the same as the previous point
Linear | The edge of deck is a straight line between deck edge offset points.
Spline | The edge of deck is a cubic spline curve between deck edge offset points.


Haunch Geometry
---------------
Define the geometry of the slab haunch

Item | Description
-----|-------------
Haunch Shape | Select the shape of the slab haunch. This setting is for graphical presentation purposes only
Fillet | Enter the depth of the slab fillet
Slab offset type | Use the drop down list to select the method for defining the slab offset
Slab Offset ("A" Dimensions) | Enter the slab offset dimension for the bridge

Material
--------------
Define the deck material

Item | Description
-----|------------
f'c | Concrete strength
Ec | When checked, enter the modulus of elasticity. Otherwise, the modulus of elasticity is computed from the AASHTO equation
[More Properties...] | Press to enter more detailed information about the concrete or copy a predefined concrete from the library

Wearing Surface
-------------------
Define the bridge deck wearing surface

Item | Description
-----|-------------
Wearing Surface Type | Use the drop down list to select the wearing surface type
Installation | Use the drop down list to select the installation event (PGSplice only)
Sacrificial Depth | Enter the sacrificial depth of the deck
Overlay Weight | When selected, the overlay dead load is defined by a pressure load. Enter the pressure load
Overlay Depth | When selected, the overlay dead load is defined by the depth of the overlay and a unit weight.

Condition and Load Rating
-----------------------
Use the drop down list to enter the condition of the deck. If a condition of "Other" is selected, enter the condition factor.
