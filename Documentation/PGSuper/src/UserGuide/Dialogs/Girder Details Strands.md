Strands {#ug_dialogs_girder_details_strands}
==============================================
Define the prestressing strand arrangement.

Prestressing Strand Type
------------------------

Item | Description
-----|-----------
Permanent | Use the drop down list to select the material and size of permanent strands
Grit impregnated epoxy coating | Check if the strand has grit impregnated epoxy coating. See @ref tg_epoxy_coated_strands in the @ref technical_guide for more information.
Temporary | Use the drop down list to select the material and size of temporary strands. Temporary strands are never grit impregnated epoxy coated.

Strand Details
---------------

Item | Description
-----|-------------
Adjustable Strands | Select the adjustable strand type
Specify Filling of Strands Using | Select the method for filling the strands grid defined in the girder library entry associated with this girder.
[Select Strands...] | Open the Select Strands window
[Define Strands...] | Open the Define Strands window
Number of Straight Strands | Enter the number of straight strands. Check the Calculating Jacking Force box and enter a value, otherwise the maximum jacking force will assumed.
Number of Harped Strands | Enter the number of straight strands. Check the Calculating Jacking Force box and enter a value, otherwise the maximum jacking force will assumed.
Number of Temporary Strands | Enter the number of temporary strands. Check the Calculating Jacking Force box and enter a value, otherwise the maximum jacking force will assumed.
Temporary Installation | Use the drop down list to select when the temporary strands are installed.

Vertical Location of Harped Strands
-----------------------------------
Enter the location of the harped strands at the ends of the girder and the harping point.

Use the drop down list to select the method of measuring the harped strand location.

Enter the location.

> NOTE: If you hold the CTRL key while selecting a measurement type, the strand location will be converted to the selected measurement type.

Strand Filling Options
----------------------
Four options are available for defining the location of the prestressing strands.

Option | Description
-------|-------------
Total Number of Permanent Strands | When this option is used, both straight and adjustable strands (harped or straight-web) are filled sequentially using the global strand fill order (Fill #) defined by the Girder Library entry strand grid.
Number of Straight and Number Of Harped (or Number of Straight and Adjustable) | The number of straight and adjustable strands are defined independently. Straight and adjustable strand locations are filled sequentially using their relative orders defined in the Girder Library strand grid.
Direct Selection of Strand Locations | This option allows you to fill the strand locations defined in the Girder Library strand grid in any order you like. Press [Select Strands...] to open the @subpage ug_dialogs_girder_select_strands window.
Direct Input of Strand Locations | This option allows you to ignore the strand locations defined in the Girder Library strand grid and to define strands at any location in the cross section. Press [Define Strands...] to open the @subpage ug_dialogs_girder_define_strands window.

Temporary strands are filled either sequentially or using direct selection depending on the option selected above.

