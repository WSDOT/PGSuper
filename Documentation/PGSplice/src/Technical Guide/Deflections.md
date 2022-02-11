Deflections {#tg_deflections}
======================================

Mapping of Permanent Segment Deformations from Segment Storage to Bridge Model Deflections
======================================

Girder Segments are not perfectly straight elements when placed at erection. They are shaped with permanent unrecoverable deformations caused by prestressing forces, creep, shrinkage, relaxation, girder hardening (increase in modulus of elasticity during storage), and precamber.

These unrecoverable deformations must be accounted for when segments are placed into PGSplice’s flat bridge analysis model (aka, the LBAM). The flat analysis model assumes that segments are initially perfectly straight with zero applied external forces prior to loading. Segments are placed on simple supports at the time of segment erection, so the second assumption is correct,
but the affect to deflections must be computed by summing unrecoverable deflections to flat model deflections to determine total deflections in the bridge. This is done by establishing key locations along a segment where unrecoverable deflections are equal to LBAM deflections.

**Definition:  Erection Deflection Datum (EDD)** – Discrete locations on a segment where unrecoverable (permanent) deflection of the segment is mapped into the flat LBAM model at time of erection. Unrecoverable deflection between EDD points is measured relative to the EDD line.

For example, in the top of Figure below a segment is supported at its ends during storage in the casting yard. Hence, the unrecoverable segment deflection is zero at the segment’s ends. The segment is then hauled to the site and erected at different bearing support locations, which become the new locations of zero deflection (aka, the EDD’s). The deflected shape is translated into the new position at erection assuming a rigid body translation. Post-erection deflections computed from the flat bridge model analysis (aka, the LBAM) can then be added to these unrecoverable deflections to determine total deflections in the bridge model.

If, for example, the left end of the right adjacent segment (shown in the bottom of the figure) is fully supported by the left segment, then the unrecoverable deflection of the supported segment is mapped to the deflection at the supporting segment’s strongback bearings as shown.

![](image001.png)

The sketches below show more examples how unrecoverable permanent segment deflection is mapped at the EDD locations. This pre-erection permanent deflection is then summed with subsequent LBAM deflections from the time of segment erection onward.

![](image002.png")

EDD datum locations are always at erection supports (pier, tower, or strongback), and there can be 2, and only 2, datum locations on a segment. Any additional supports on the segment are redundant, and their elevation at time of erection is adjusted based on the unrecoverable deflection at their location.

***Unrecoverable Deflections Due to Change in Modulus During Storage***

Changes in support locations between storage and erection induce changes in dead load deflections that are superimposed on the unrecoverable dead load deflection. For example: a segment may be supported 5 ft inboard from its ends during storage causing a dead load deflection for this configuration. During storage the concrete ages and the modulus of elasticity increases. When the segment is moved to its erected configuration, some, but not all of the initial dead load deflection is recovered. E.g., the erected configuration may be supported 1 ft inboard from the end. This additional deflection in the erected configuration due to the change in support location is superimposed with the unrecoverable deflection, resulting in the total dead load deflection in the erected configuration.

Rules for Locations of Erection Deflection Datums (EDD’s)
==========================================================
The locations of EDD’s are computed automatically by the program using the following rules.

Legend:

![](image003.png)

1) Rules are executed based on support conditions at erection time of segment in question

2) If a segment is supported by 2 or more permanent piers, then both EDD locations are at the two outermost permanent piers ![](image004.png)

3) Else If segment supported by 1 permanent pier:
  3a) If zero erection towers. Place EDD at pier and strongback at other end ![](image005.png)

  3b) Else If 1 erection tower, or multiple towers all on same side of pier
     3bi) If tower(s) on side of pier with adjacent segment with free end, place EDD at outermost tower. Allow tower adjustment if free to move 
	 ![](image006.png) 
	 ![](image007.png) 
	 ![](image008.png)

	3bii) Else if tower on side of pier with a supporting adjacent segment, Place EDD at strongback, or tower if tower at closure 
	![](image009.png) 
	![](image010.png)
	
  3c) Else if more than 1 towers, and towers on both sides of pier: Place single EDD at Pier, allow outermost towers to be adjustable in co-dependent seesaw motion. Any redundant towers go along for the ride. Segment rotation is independent of adjacent segment BC’s. ![](image011.png)

4) Else If zero permanent piers:
	4a)If zero towers: Place EDDs at strongbacks ![](image012.png)
    4b) Else if 1 tower:
       4bi) If both ends of segment dependent on adjacent segments, place EDD’s at strongbacks and adjust tower elevation per unrecoverable deflection ![](image013.png)
       4bii) If only one end dependent on adjacent segment, place EDD at tower and at supporting strongback ![](image014.png)
    4c) If 2+ towers: 
       4ci) If adjacent segments at both ends free, place EDDs at outermost towers ![](image015.png)
       4cii) Else if both ends dependent on adjacent segments. Place EDD’s on strongbacks, adjust tower elevations ![](image016.png)
       4ciii) Else if one end free and other dependent. Place EDD at strongback at dependent end, and other EDD at closest tower to free end ![](image017.png)

5) Additional rules for Erection Deflection Datums:
   5a) Deflections at free ends of drop in segments are made to match deflection of adjacent supporting segment
   5b) A mismatch in end deflections can occur when segments with fully constrained ends are placed adjacently. This mismatch must be checked if post-tensioning through joint
 ![](image018.png) 
 ![](image019.png)


Deflections and Elevation Adjustment
======================================
Deflections are measured relative to a zero datum line. The displacement of spliced girders can be modified by applying elevation adjustments to segment ends at temporary supports. Consider a single span bridge comprised of three segments. The segments are supported by the abutments and two erection towers. The segment ends can be raised at the erection towers to compensate for the downward deflection of the girder. The adjustment is defined as the Elevation Adjustment and is a rigid body displacement of the girder segments.
 ![](ElevationAdjustment.png)

The total displacement of the girder is the sum of the deflections and the rigid body displacements.