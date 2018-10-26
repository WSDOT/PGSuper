Advanced Geometry Features {#advanced_geometry}
============
This tutorial will highlight many of the advanced geometric modeling capabilities available in PGSuper. These capabilities include:

* Complex roadway geometry including multiple horizontal curves with transition spirals
* Complex alignment profiles including multiple, non-symmetric vertical curves
* Complex superelevation transitions including multiple crown point offsets
* Flared (non-parallel) girders
* Tapered deck edges with spline transitions
* Different girder spacing at each end of a span
* Different number of girders in each span
* Different girder spacing on either side of a pier
* Different types of girders within a bridge

Example Bridge 1
-----------------
The bridge we will be modeling has 3 - 80ft spans. 
* Abutment 1 is located at station 1+20.
* Span 1 has 5-WF58G girders at 6'-0\".
* Span 2 has 4-WF58G girders spaced at 8'-0\" at the centerline of pier 2 and 6'-0\" at the centerline of pier 3.
* Span 3 has 4-WF58G girders spaced at 6'-0\".

The bridge deck is 31 ft (16' on left and 15' on right of CL bridge) wide at pier 2 and 24.5 ft (12.5' on left and 12' on right of CL bridge) wide at pier 3. The deck is a constant width, and parallels the alignment between Abutment 1 and Pier 2 and also between Pier 3 and Abutment 4. The deck edge is a spline taper between pier 2 and 3.

The bridge is on a curved portion of the alignment. The back tangent is N 90 E, the PI station is 5+00 with a delta of 25 L and Radius of 2000 ft.

@subpage tutorial_advanced_example_1 Tutorial

Example Bridge 2
----------------
The bridge in this example is has 3 spans consisting of 7-W42G girders. The configuration is

### Alignment ###
Item | Description
-----|------------
Initial Direction| S 60 13 00 W
PI Station| 45+46.40
Delta| 98 23 00 R
Radius| 424.41 ft

### Profile ###
Item | Description
-----|------------
PVI | 47+75.00
PVI Elevation | 53.04
Entry Grade | 5.45%
Exit Grade | 0.5%
Length | 350 ft

### Superelevations ###

Item | Description
-----|------------
Station 47+33.70 | Left Slope = 0.1ft/ft, Right Slope = -0.1ft/ft
Station 49+33.70 | Left Slope = 0.02ft/ft, Right Slope = -0.02ft/ft

### Bridge ###

#### Abutment 1 ####
Location: Sta 47+40.99
Orientation: N 40 58 00 E
 
Between Girders | Spacing
---------------|------------
A-B | 5'-3\"
B-F | 6'-0\"
F-G | 5'-6\"
 
#### Pier 2 ####
Location: 47+84.73
Orientation: N 40 58 00 E
 
Between Girders | Spacing
---------------|------------
A-B | 6'-1.5\"
B-F | 6'-0\"
F-G | 6'-1.5\"
 
#### Pier 3 ####
Location: 48+53.21
Orientation: N 40 58 00 E
 
Between Girders | Spacing
---------------|------------
A-B | 5'-7.5\"
B-F | 6'-0\"
F-G | 5'-7.5\"
 
#### Abutment 4 ####
Location: 49+17.14
Orientation: N 31 28 00 E
 
Between Girders | Spacing
---------------|------------
A-B | 5'-7.5\"
B-F | 6'-9\"
F-G | 5'-7.5\"
 
The girder spacing is measured along the centerline of bearing at the abutments and along the centerline of pier at the interior piers.

The deck width is 35'-9" at Station 47+83.70 and linearly tapers-a width of 33'-9" at station 49+17.17.

@subpage tutorial_advanced_example_2 Tutorial
