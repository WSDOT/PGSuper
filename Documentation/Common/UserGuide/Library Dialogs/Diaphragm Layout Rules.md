Diaphragm Layout Rules {#ug_library_dialogs_diaphragm_layout_rules}
==============================================

Item | Description
----|---------
Description | Enter a descriptive name for the rule
Construction | Select how the diaphragm is constructed. Choices are Cast-in-Place or Precast.
Location | Enter the location of the diaphragm relative to other bridge components. Choices are External (between girders) and Internal (between interior webs)
Create diaphragms when the girder length is | Enter the minimum and maximum span lengths for which the rule applies.
Diaphragm weight | Select the method that defines how the diaphragm weight is determined. Choices are Compute weight of diaphragm based on Width, Height, and Web Spacing or Input diaphragm weight
Height | Enter the height of the diaphragm
Thickness | Enter the thickness of the diaphragm
Weight | Enter the weight of the diaphragm
Diaphragm location is measured as a | Select an option that describes how the diaphragm location is defined
Diaphragm location is measured from | Select a datum for locating the diaphragm
Diaphragm location | Enter the location of the diaphragm

Diaphragm Dimensions
--------------------
These dimensions are used to compute the weight of the diaphragm based on the girder spacing (or internal girder void size) and the weight density of the deck concrete.

> NOTE: The diaphragm length is measured from center to center of the exterior webs of the girders. The ends of the diaphragms are not sloped to match the shape of slope webs, like those found in U girders.

Location of Diaphragms 
-----------------------
Diaphragms can be located using fractional measure or a fixed distance from; girder ends, span ends, or from mid-span.

If a diaphragm is not located at mid-span, two diaphragms are created. One is created at the specified location and the other at the opposite end of the span.

