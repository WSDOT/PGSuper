Temporary Strands {#ug_library_dialogs_girder_temporary_strands}
==============================================
Temporary strands are straight strands typically placed in the top flange of the girder to control stresses associated with long overhangs required for stability during handling. Temporary strands can be:

* Pre-tensioned along with the permanent strands
* Post-tensioned immediately before lifting the girder from the casting bed
* Post-tensioned immediately before transportation of the girder to the job site

Temporary strands are assumed to be bonded at the ends of the girder and debonded along the remainder of the girder. Temporary strands are assumed to be removed once the girder has been erected and properly stabilized. Once removed, the temporary strands are no longer included in the analysis.

> NOTE: The bonded portion of pre-tensioned temporary strands are considered to be removed, even though they remain bonded to the concrete.

Guidance on the use of temporary strands is available in the AASHTO LRFD Bridge Design Specifications, 9th Edition, 2020, Article 5.9.4.5

Temporary Strand Grid
The information in the temporary strand grid defines the possible locations and fill order of temporary strands.

Item     | Description
---------|------------
Xt,Yt    | Location of a temporary strand position measured from the top center of the girder
[Delete] | Deletes the selected strand position from the grid
[Append] | Adds a new strand position at the bottom of the girder
[Insert] | Inserts a new strand position at the currently selected row in the grid

The following rules apply:
* X values must be positive or zero. If a positive X value is given, two strands are added for the fill location: one strand at (X,Y) and another at (-X,Y). This enforces the requirement that all prestressing must be symmetric about the Y axis.
* Strands are added to the section in the given Fill # sequence.
* Strands must be located inside of the girder cross section.

