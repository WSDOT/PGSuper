General Features {#general_features}
============
This tutorial highlights the major features of PGSuper. At times, we deviate from what might be considered "normal practice" to illustrate certain features. Please consider this tutorial a lesson on using PGSuper and not a lesson in precast girder design.

In this tutorial you will:

1. @subpage tutorial_general_new_project
2. @subpage tutorial_general_editing_the_bridge_model
3. @subpage tutorial_modeling_a_special_dead_load
4. @subpage tutorial_general_design_a_girder
5. @subpage tutorial_general_reviewing_results
6. @subpage tutorial_general_autocalc_mode
7. @subpage tutorial_general_getting_help
8. @subpage tutorial_general_wrapping_up

The Bridge
----------
One of the basic assumptions inherent in PGSuper is that you are analyzing and designing a real bridge and not just a girder. It is assumed that you have either preliminary plans or a final set of PS&E sitting in front of you. The input for PGSuper reflects the information you will find on a typical set of bridge plans.

The example bridge for this tutorial is the Angeline Road Overcrossing bridge near the City of Bonney Lake, WA. This is a single span bridge with a 106ft span and six W58G girders. The details are given below:

Item | Description
-----|---------
Alignment | S 49 08 03 E
Profile |  Station : 1+64.60<br>Elevation : 524.16<br>Slope : -4.20%
Crown Slope |  Left : -0.02 ft/ft <br>Right : -0.02 ft/ft
Pier 1 | Back of Pavement Seat : 1+80.66 <br> Skew : 11 00 00 R
Pier 2 | Back of Pavement Seat : 2+86.66 <br> Skew : 11 00 00 R
Girders | 6 - W58G @ 6ft
Prestressing | To Be Determined by Design
Slab |  Gross Depth : 7.5 in<br>Overhang : 3.25 ft (18.25 ft from CL Bridge to edge of deck)<br>Slab Offset (aka Haunch or "A" Dimension) : To Be Determined by Design<br>Wearing Surface (Sacrificial Depth) : 0.5 in
Materials | Girder : Concrete, 28 day Strength, To Be Determined by Design<br>Girder: Concrete, release strength, To Be Determined by Design<br>Deck : Concrete, 28 day Strength, 4 ksi<br>Strands : 0.6 in Grade 270 Low Relaxation
Special Loads | A 10 kip overhead sign structure is placed at mid-span.

> NOTE: "A" Dimension is a WSDOT term that denotes the distance from the top of the girder to the top of the slab, measured at the centerline of bearing. Other common terms are "Slab Offset" and "Haunch"

Your task is to design the girders for this structure. This tutorial will guide you through the major features of the PGSuper software as you design the girders for flexure. This tutorial is intended to demonstrate the capabilities and features of the software and does not necessarily prescribe step by step procedures for designing prestressed girder bridges.
