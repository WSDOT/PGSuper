Bearings {#ug_library_dialogs_project_criteria_bearings}
==============================================
Define project criteria related to bearings

Tapered Sole Plates [LRFD 14.8.2]
---------------
Check this option of the requirements for tapered sole plates are to be evaluated. If the inclination of the underside of the girder exceeds the specified threshold a warning will be added to the the specification check.

Enter the inclination of the underside of the girder to the horizontal threshold, in radians.

Reactions and Rotations
----------------------------------
Include dynamic load allowance (IM) [LRFD 14.4.1]. Check this option to include impact in the reactions and rotations reported for Bearing Design Parameters.

Combined Shear Strain [Method B]
----------------------------------

Uncheck the option to use signed contributions, including signed rotations. Opposing contributions can cancel; verify that the selected combination is appropriate for the project. The 1.75 amplification factor applies to cyclic strain contributions with either option.

Turning off absolute summation displays a warning dialog referencing AASHTO LRFD Commentary C14.7.5.3.3. The Status Center also displays a warning while Method B bearing checks use signed contributions, because opposing rotations can produce artificially low combined strains.
