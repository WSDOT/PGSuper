Segment Tendons {#ug_dialogs_segment_tendons}
==============================================
Precast segments can have plant installed tendons. These tendons, refered to as segment tendons, are isolated to individual segments are not spliced with tendons in other segments after erection.

Segment tendons may be used in conjunction with prestressing strands.

> NOTE: Be careful to ensure longitindal reinforcement, pretension strands, and post-tensioning ducts do not conflict. Geometric conflicts are not automatically detected.

Item | Description
-----|----------------
Tendon Grid | Define individual tendons in the grid. See below for details.
[Add] | Add a tendon
[Remove] | Remove tendon in the selected row
Duct Material | Select the duct material. Used for evaluation of the provisions of LRFD 5.4.6.1. This setting does not effect the duct friction factor or wobble coefficient. Select *Project > Losses* to edit duct friction factors.
Strand Size | Select the tendon strand size
Installation Method | Select the strand installation method. Used to evalute the provisions of LRFD 5.4.6.2.
Time of Installation | Select the time when the tendons are tensioned and grouted. Tendons can be installed after prestress release (before initial lifting), after moving the segment into storage (before time elapses during storage), or before hauling (at the end of storage).

Tendon Grid
Item | Description
-----|----------------
Type | Select a duct type from the drop down list. The duct types are defined in the Ducts library
Shape | Select the shape of the duct. Segment tendons are limited to a straight duct or a simple parabola.
# Strands | Enter the number of strands in the tendon
Pjack | Check the box to enter a jacking force, otherwise the maximum jacking force is used
Jacking End | Select the live end for tendon jacking. End orientation is determined based on orienation of the segment in the final structure.
Left End | Enter the elevation of the tendon (Y) at the left end of the segment. Use the drop down list to select the datum (top or bottom of segment).
Middle | Enter the elevation of the tendon (Y) at the middle of the segment. Use the drop down list to select the datum (top or bottom of segment). The middle point is not used for Straight tendons.
Right End | Enter the elevation of the tendon (Y) at the right end of the segment. Use the drop down list to select the datum (top or bottom of segment).

> NOTE: The segment height at the left end, middle, and right end are listed to assist you in selecting the correct tendon elevation.
