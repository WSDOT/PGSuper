Bridge Input Tab {#ui_bridge_input_tab}
===================
This tab contains bridge-level data related to the project.

> Note: All data on this tab must be filled in prior to visiting other tabs.

General Information
===================
The General Information group contains textual data used to describe the current project in TOGA reports. None of this data affects the analysis.

Item       | Description
-----------|-------------
Bridge     | Name of the Bridge
Bridge ID  |  TxDOT Bridge Identification Number
Job Number |  CSJ
Engineer   |  -
Company    | -
Comments   | Any additional text to be included in report
 

Design Information
==================
This group contains bridge-level design information

Item       | Description
-----------|-------------
Span No    | Span where the beam is located. (textual information only)
Beam No    | Beam number. Generally numbered from left to right facing up-station. (textual information only)
Beam Type  | Type of beam in question. Note that changing beam type will reset the strand data on the Girder Input tab to Standard Strand Fill with zero strands.
Span Length | Length of span as measured between bearing centerlines in feet. Note that the overall girder length is determined by the default connection type used by TOGA for the girder in question. This may not reflect the plan or as-fabricated girder length.
Beam Spacing | CL to CL spacing of beams in feet.
Slab Thickness | Equivalent slab thickness in inches used to compute composite section properties and weight of slab. Slab density is assumed to be 150 pcf.
Relative Humidity | In % at bridge location
LLDF (Moment) | Live load distribution factor for moment
LLDF (Shear) | Live load distribution factor for shear
 

Material Properties
===================
This group contains bridge-level material properties.

Item       | Description
-----------|-------------
Ec, Slab | Modulus of elasticity of slab concrete. Default is 5000.0 KSI. 
Ec, Beam | Modulus of elasticity of beam concrete for final condition in KSI. Default is 5000.0 KSI.
f'c, Slab | Final concrete strength of slab concrete. Default is 4.0 KSI.
 

Design Data
===========
This group contains the results from the analysis of the original per-plan beam design.

Item       | Description
-----------|-------------
*ft, Design Compressive Stress, Top CL | Beam top fiber stress at centerline, due to total external load. (Service I, Final Stage) in KSI
*fb, Design Tensile Stress, Bottom CL | Beam bottom fiber stress at centerline, due to total external load. A negative sign {-} must be entered for tension. (Service III, Final Stage) in KSI
Mu, Required Ultimate Moment Capacity | Ultimate moment capacity required in kip-ft
 
> \* Note: Tension is negative

Optional Uniform Design Loads {#uniform_design_load}
=============================
This group contains equivalent uniform design loads to be applied to the girder in the associated stages/load combinations. Default is 0.0 kip/ft. Input is not required

Item       | Description
-----------|-------------
W non-comp, DC | Uniform dead load, in addition to weight of beam and slab, applied to non-composite section in the DC load case. (e.g. diaphragms) in kip/ft
W comp, DC | Uniform dead load, applied to composite section in the DC load case (e.g., rail) in kip/ft
W Overlay | Uniform dead load, applied to composite section in the DW load case after all losses in kip/ft. This is equivalent to a future overlay in PGSuper.
 

Analysis Options
==================
This group contains various options.

Item       | Description
-----------|-------------
Project Criteria | Select the Project Criteria library entry to be used for the analysis. Normally, this will be the most recent version of the TxDOT specification.
 
