Transverse Reinforcement {#ug_dialogs_girder_details_transverse_reinforcement}
==============================================
Define transverse (shear) reinforcement.

General Parameters
------------------

Item  | Description
------|-------------
Reinforcement Material | Select the appropriate mild steel for the transverse rebar
Make Zones Symmetrical about Mid-Girder | When checked, bar zones are symmetric about mid-girder. If zones are made symmetrical about the girder centerline, right-most zone data is either automatically extended or truncated to mid-girder such that the zones are always symmetrical. Similarly, if zones are not symmetrical, right-most zones are either extended or truncated such that the last zone extends to the right end of the girder.
Top flange is intentionally roughened for interface shear capacity | Check the box to indicate the top flange is intentionally roughened to an amplitude of 0.25 inch to help resist horizontal interface shear. For UHPC girders intentionally roughened is taken to mean the top flange is formed with keyed or fluted surface in accordance with the applicable structural design guidance.

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
Use Primary Bars for Splitting Resistance | Check the box to indicate that stirrups in the primary transverse reinforcement zones are to be utilized when evaluating splitting resistance at the girder ends (LRFD 5.9.4.4.1 (*pre-2017: 5.10.10.1*)).
[Insert Row] | Inserts a row into the grid
[Delete Row] | Removes a row from the grid

@subpage ug_dialogs_girder_details_additional_interface_shear_reinforcement may be defined by pressing [Additional horizontal interface shear reinforcement...]

Additional Reinforcement at Girder Ends
----------------------------------------

Item | Description 
-----|----------
Splitting Reinforcement | This data allows input for two identical zones placed at both ends of the girder to provide splitting resistance. These bars are used in addition to any splitting resistance provided by primary bars.
Bottom Flange Confinement Reinforcement | This data allows input for two identical zones placed at both ends of the girder to provide confinement resistance. These bars are used in addition to any primary confinement bars.

Copy From Library
---------------------------
Press button to replace all data on this tab with the data defined in the girder library for this girder.