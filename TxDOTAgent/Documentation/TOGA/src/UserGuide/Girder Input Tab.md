Girder Input Tab {#ui_girder_input_tab}
===================
This tab defines prestressing and concrete strength design parameters for the Original Girder Design and the Fabricator Optional Girder Design.

> Note: This tab can not be accessed until all data on the Bridge Input tab has been filled out correctly.

Original Girder Design, and Fabricator Optional Girder Design
================================================================
The format for input data for both beams is identical. Hence, we will only treat one case.

Prestressing Strand Type
=========================
Enter the strand material grade, type and strand size for the given girder

Girder Material Properties
===========================

Item | Description
-----|-------------
f'ci |  Input the release strength, f'ci, for the girder concrete in KSI
f'c | This is the crushing strength, or final compressive strength, for the material as defined by most common specifications in KSI. This strength is normally defined as the stress value at a strain of 0.003 at 28 days.
 

Strand Fill Information
=======================
Strands may be filled using the standard fill sequence, a row-by-row non-standard definition, or by direct selection of strand locations. Use the combo box to select the fill type. The number of strand information and available rows are populated automatically for the current beam type.

Strand locations may be viewed on the [Girder View Tab](@ref ui_girder_view_tab)  once all input data on this tab has been validated.

Standard Sequential Strand Fill
-------------------------------
This is the simplest way to define strand fill. If the Standard Sequential Strand Fill option is selected, strands are filled in the order defined by the TxDOT standard precast design plan sheets for the selected girder type (e.g., IBND sheets). For example, as quoted below:

\"<i>For depressed strand designed beams, strands shall be located as low as possible on the 2" grid system unless a Non-Standard Strand Pattern is indicated. Fill row "2", then row "4", then row "6", etc., beginning each row in the "A" position and working outward until the required number of strands is reached. All strands in the "A" position shall be depressed, maintaining the 2" spacing so that, at the beam ends, the upper two strands are in the position shown in the table.</i>\"

The actual fill order is defined in the TxDOT PGSuper Master library. 


Item | Description
-----|-------------
Total No. Strands | Select the total number of permanent (straight + depressed strands) in the section
Girder Bottom to Topmost Strand (To) | Distance between the bottom of the girder and the top most row of depressed strands at the girder ends.
Compute Eccentricity | Click this button to view the strand eccentricity for current strand data
 

Non-Standard Strand Fill with Depressed Strands
-----------------------------------------------
This option is available only if the current girder contains depressed strands. If the Non-Standard Strand Fill with Depressed Strands is selected, strands may be filled using a row-by-row description. Select the number of strands to be placed in each row using the available locations listed in the combo boxes.

The strand row locations are measured from the bottom of the girder. Available rows and number of strands per row are based on the permanent strand locations defined in PGSuper's girder library entry for the given girder type. The fill sequence in each row is defined by the relative fill order within the library entry. The row locations cannot be edited. 

Two input grids of strand rows are presented, one representing strand locations at the girder ends, and one representing strand locations at the centerline between the hold-down points. Note that the number of straight strands and number of harped strands at both locations must match in order for the fill data to be valid.

Non-Standard Direct Strand Fill of Straight Strands
----------------------------------------------------
Select this option to fill straight strands non-sequentially by directly selecting the individual strand locations to be filled.

> Note: Beam types with depressed strands will be modified as follows for the direct fill option:

* All depressed strand locations are converted to straight, non-debondable.
* Straight strand locations are made debondable.

### Also See ###
* @subpage ui_select_strands_dialog

