Geometry - Vertical Design {#tg_vertical_geometry}
======================================
The ultimate goal in bridge vertical geometric design is for Finished Deck elevations to match Design Roadway elevations within a given tolerance at all locations along the roadway at some specified time during service. This specified time is called the **Roadway Geometry Control Event** defined in the Timeline, and it will be discussed further throughout chapters in this section.

![](VerticalGeometry.gif)

Design of the haunch is an integral part of vertical geometric design. Haunches may be specified for beam-slab bridges and all bridge types with composite decks, composite overlays, or structural overlays.

> For no-deck structures, the analysis is much simpler since there is no haunch. Refer to documentation on explicit haunch depth input below with the understanding that haunch depth is zero.

Definitions
--------------------------
- **GPGL** – Elevation of roadway surface along CL girder line.
- **Segment Chord Line** – Chord along top surface of segments ending at ends of segments
- **Geometry Control Event (GCE)** – Timeline event where adjusted segment chord elevations are defined. The GCE occurs typically at the time when the bridge is open to traffic but may be project or agency dependent.
- **SCD (Segment Chord Datum)** – Discrete location along a segment where GPGL and top of slab elevation are forced to be coincident at the time of the Geometry Control Event. Thus, top of segment elevations can be directly computed at SCD’s using the GPGL elevation and haunch + slab depths.
- **Profile Chord** – Chord lines along CL’s of each segment that intersect the GPGL elevation at SCD locations (not shown in figure).

![](ExplicitHaunch.png)

Vertical Geometry Basic Concepts
------------------------------------
* @subpage tg_vertical_geometry_basics
* @subpage tg_slab_offset
* @subpage tg_vertical_geometry_deflection_mapping

Types of Haunch Input
----------------------
There are two types of haunch input, and there are stark differences in how elevation computations are performed. The sections below describe each in detail.
* @subpage tg_slab_offset_simplified 
* @subpage tg_haunch_explicit

> Notes:
> - Prior to Version 8.0 of the software, the Slab Offset ("A") input method was the only way to decribe the haunch in both PGSuper and PGSplice. Version 8.0 added an additional new, more robust, *Explicit Haunch Depth* input method in both PGSuper and PGSplice. The simplified Slab Offset input method was removed from PGSplice because of limitations described in @ref tg_slab_offset_simplified. 
> .
> - Explicit haunch input is the now the only way to input haunch depths in PGSplice.
> .
> - Slab Offset data in PGSplice files created before Version 8.0 will be converted to the new explicit haunch format. Haunch loads and composite section properties should change very little, if at all, after  conversion. However, haunch and finished roadway elevation results for geometric computations will be different (e.g., haunch depth will vary linearly along segments using the "A" dimension at ends to specify the depths). Excercise caution when loading older files.
