Deck Reinforcement {#ug_dialogs_bridge_description_deck_reinforcement}
==============================================
The reinforcement in the bridge deck must be defined to properly perform engineering evaluations of the bridge.

Longitudinal deck reinforcement is modeled as Primary Reinforcement and Supplemental Reinforcement. The primary reinforcement runs the full length of the bridge. The supplemental reinforcement is placed over the piers to supplement the primary reinforcement in resisting negative moments.

Transverse deck reinforcement is not modeled.

Reinforcement is modeled with a combination of individual bars and a lump sum quantity. These values are additive. The total area of reinforcement is computed as

As = (Area of individual bar/spacing + lump sum area per unit length) X (Tributary width of deck)

Deck Reinforcement
------------------

Item | Description
-----|--------------
Reinforcement | Use the drop down list to select the reinforcing steel used in the deck.
Top Mat Cover | Enter the nominal cover for the top mat of reinforcing. Cover is measured from the top of the deck to the top of individual bars and to the center of lump sum reinforcement.
Bottom Mat Cover | Enter the nominal cover for the bottom mat of reinforcing. The bottom mat cover is measured from the bottom (underside) of the deck to the bottom of individual bars and to the center of lump sum reinforcement.

Primary Reinforcement
---------------------
The primary longitudinal reinforcement runs the full length of the bridge. The reinforcement can be described with a bar size and spacing plus a lump sum area per unit length transversely. If both bar spacing and lump sum inputs are provided, they are added together for the total amount of reinforcement. For example, if the top mat is defined with #6 bars at 18" and 0.18 in<sup>2</sup>/ft, the total reinforcement will be 0.44in<sup>2</sup>/1.5ft + 0.18in<sup>2</sup>/ft = 0.473in<sup>2</sup>/ft.

Item | Description
-----|----------------
Top Mat | Define the top mat of reinforcement. Use the drop down list to select a bar size and enter the bar spacing. Additionally, enter a lump sum area of reinforcement.
Bottom Mat | Define the bottom mat of reinforcement. Use the drop down list to select a bar size and enter the bar spacing. Additionally, enter a lump sum area of reinforcement.

Supplemental Reinforcement
-------------------------
The supplemental reinforcement is described similarly to the primary reinforcement, however a grid is used so multiple reinforcements be defined at the bridge piers. The total supplemental reinforcement is the sum of the lump sum reinforcement and bar size and spacing reinforcement.

Item | Description
-----|--------------
Pier | Select a pier
Mat | Select a reinforcement mat
Bar | Select a bar size
Spacing | Enter the bar spacing
As | Enter the lump sum area of reinforcement
Left Cutoff | Enter the distance to the bar cutoff on the left side of the Pier Line.
Right Cutoff | Enter the distance to the bar cutoff on the right side of the Pier Line.

> NOTE: The total reinforcement at a section is equal to the sum of the primary and supplement reinforcement within the tributary width of a girder.

> NOTE: For all Primary Reinforcement and Supplemental Reinforcement described using a lump sum value, the reinforcement is considered to be developed along the full reinforcement length. However: for Supplemental Reinforcement bars described using a bar size and spacing, the area of the bar is reduced for partial development at the ends of the bars.

 

