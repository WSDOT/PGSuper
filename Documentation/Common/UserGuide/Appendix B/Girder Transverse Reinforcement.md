Transverse Reinforcement {#appendix_b_girder_transverse_reinforcement}
==============================================
This tab allows you to edit the description of transverse (shear) reinforcement seed data for a girder. 

> Note that transverse rebar data from the library is copied to all girders when a girder type is first selected while editing the bridge model. The data can be subsequently overridden in the girder editor

General Parameters
------------------

Item  | Description
------|-------------
Reinforcement Material | Select the appropriate mild steel for the transverse rebar
Make Zones Symmetrical about Mid-Girder | When checked, bar zones are symmetric about mid-girder. If zones are made symmetrical about the girder centerline, right-most zone data is either automatically extended or truncated to mid-girder such that the zones are always symmetrical. Similarly, if zones are not symmetrical, right-most zones are either extended or truncated such that the last zone extends to the right end of the girder.
Top flange is intentionally roughened for interface shear capacity | Check the box to indicate the top flange is intentionally roughened to an amplitude of 0.25 inch to help resist horizontal interface shear. 

Primary Bars
------------------
The transverse reinforcement is described by zones, beginning at the left end of the girder. Transverse reinforcement can optionally be forced to be symmetrical about the center of the girder. Transverse reinforcement zones are described by their length, spacing and bar size.

Primary bar zones contain vertical shear reinforcement that can optionally be extended into the deck for horizontal interface shear, and also be used for confinement steel and splitting resistance. These bars typically represent the main stirrups in a girder.

Item | Description 
-----|------------
Zone | The zone number
Zone Length | Length of the zone
Bar Size | Size of the stirrup bar
Spacing | Spacing of the stirrup bars within the zone
\# of Vertical Legs | Number of vertical legs of stirrups in this zone
\# of Legs Extended into Deck | Number of stirrup legs extended into the deck.
Confinement Base Size | Size of confinement bars
Use Primary Bars for Splitting Resistance | Check the box to indicate that stirrups in the primary transverse reinforcement zones are to be utilized when evaluating splitting resistance at the girder ends (LRFD 5.10.10.1).
[Insert Row] | Inserts a row into the grid
[Delete Row] | Removes a row from the grid


Additional Horizontal Interface Shear Zones
---------------------------------------------
The bars in these zones are used to resist horizontal interface shear and are used in addition to any primary bars that have been extended into the deck. This zone layout is independent of the primary zones.

> NOTE: This input generally isn't used any more. In the LRFD 1st Edition 1994, 5.8.4, a minimum of four bars where required to cross the interface plane with the deck. It was common to add extra bars in the top flange of girders that had two bars extended into the deck, combined with two stirrup legs, to meet the four bar minimum. This requirement was removed in LRFD 2nd Edition 1998.
Additional Reinforcement at Girder Ends
----------------------------------------

Item | Description 
-----|----------
Splitting Reinforcement | This data allows input for two identical zones placed at both ends of the girder to provide splitting resistance. These bars are used in addition to any splitting resistance provided by primary bars.
Bottom Flange Confinement Reinforcement | This data allows input for two identical zones placed at both ends of the girder to provide confinement resistance. These bars are used in addition to any primary confinement  bars.

