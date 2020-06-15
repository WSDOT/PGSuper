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

Item                         | Description                                                                                                                     
-----------------------------|---------------------------------------------------------------------------------------------------------------------------------
 Adjustable Strands          | Select the adjustable strand type                                                                                               
 Strand definition type      | Select the method for defining strands. (See Strand Definition Types below)                                                
 [Select Strands...]         | Open the Select Strands window                                                                                                  
 [Define Strand Rows...]     | Open the Define Strand Rows window                                                                                              
 [Define Individual Rows...] | Open the Define Individual Strands window                                                                                       
 Number of Straight Strands  | Enter the number of straight strands. Check the Pjack box and enter a value, otherwise the maximum jacking force will assumed.  
 Number of Harped Strands    | Enter the number of straight strands. Check the Pjack box and enter a value, otherwise the maximum jacking force will assumed.  
 Number of Temporary Strands | Enter the number of temporary strands. Check the Pjack box and enter a value, otherwise the maximum jacking force will assumed. 
 Temporary Installation      | Use the drop down list to select when the temporary strands are installed.                                                      

Vertical Location of Harped Strands
-----------------------------------
Enter the location of the harped strands at the ends of the girder and the harping point.

Use the drop down list to select the method of measuring the harped strand location.

Enter the location.

> NOTE: If you hold the CTRL key while selecting a measurement type, the strand location will be converted to the selected measurement type.

Strand Definition Types
----------------------
There are several options for defining prestressing strands.

Option | Description
-------|-------------
Number of Permanent Strands | When this option is used, both straight and adjustable strands (harped or straight-web) are filled sequentially using the global strand fill order (Fill #) defined by the Girder Library permanent strand grid.
Number of Straight and Harped Strands (or Number of Straight and Adjustable Strands) | The number of straight and harped/adjustable strands are defined independently. Straight and harped/adjustable strand locations are filled sequentially using their relative orders defined in the Girder Library permanent strand grid.
Strand Locations | This option allows you to fill the strand locations defined in the Girder Library permanent strand grid in any order you like. Press [Select Strands...] to open the @subpage ug_dialogs_girder_select_strands window.
Strand Rows | This option allows you define prestressing strands in horizontal rows. The strand positions defined in the Girder Library are ignored. Press [Define Strand Rows...] to open the @subpage ug_dialogs_girder_define_strand_rows window.
Individual Strands | This option allows you define each prestressing strand individually. Asymmetric strand layouts can be modeled with this option. The strand positions defined in the Girder Library are ignored. Press [Define Individual Strands...] to open the @subpage ug_dialogs_girder_define_individual_strands window. LRFD 9th Edition 5.9.4.3.3, Requirment D is not evaluated when this option is selected.

