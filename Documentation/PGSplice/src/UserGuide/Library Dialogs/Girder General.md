General {#ug_library_dialogs_girder_segment_general}
==============================================
The General Tab defines the basic girder type and the specific dimensions of the girder cross section.

Item | Description
----|-----
Girder Type | Select from the drop down list, the general type of girder cross section.
Girder Dimensions | Each of the girder dimensions are defined on the accompanying schematic. 
[View Section at Ends...] | Displays the basic girder section at the left end of the girder including the location of prestressing strands
[View Section at Mid-Span...] | Displays the basic girder section at the mid-point of the girder including the location of prestressing strands

Other Parameters
-------------------

Item | Description
----|-----
Drag Coefficient | Enter the drag coefficient for the bare girder. This parameter is used to determine wind pressure loading from wind speed for the lifting and hauling analysis. Typical values are 1.5 for U-beams and 2.2 for all other beams. (See University of Florida research http://ufdc.ufl.edu/UFE0045616/00001)
Report Bearing Elevations at Girder Edges | Click the check box to report bearing elevations at the outmost edges of the bottom of the girder. For multi-web girders this will be the outside edges of the outside webs. The information will be reported in the Bridge Geometry report and Details report.

Variable Depth Sections
------------------------
This input is shown for girder types that support variable depth sections. Variable depth sections are often used as pier segments. The specific variations in the section depth depend on the girder type. There will be descriptive text that lists the dimensions that can be altered by the user to modify the section depth. Check the "Section depth can vary" box to permit section depth variations.


> NOTE: PGSplice was written for bridges composed of typical prestressed girders. A clever user might be tempted to use these dimensions to create unique section shapes like Inverted T's, parallelograms, or even triangles. **Do not do this!** The program has not been tested for these types of shapes and may generate incorrect results.
