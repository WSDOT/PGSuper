Dead Loads {#ug_library_dialogs_project_criteria_dead_loads}
==============================================
Define project criteria for dead loads.

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

> NOTE: If the number of girders, webs, or mating surfaces in your bridge is less than or equal to twice the number of girders, webs, or mating surfaces specified, the railing systems loads are distributed equally to all girders, webs, or mating surfaces.

Distribution of Overlay Dead Load
----------------------------------
Define the method for distributing overlay dead load to the girders. LRFD 4.6.2.2.1 states "permanent loads of and on the deck may be distributed uniformly among the beams and/or stringers". However, many agencies prefer to distribute overlay loads using tributary area. Select your preference here.
