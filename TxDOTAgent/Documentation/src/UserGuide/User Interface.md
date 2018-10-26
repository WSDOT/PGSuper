User Interface Guide {#ui}
===================

This section defines the user interface (UI) and data used in the TOGA UI. 

Basic Usage
------------
TOGA is a simple Form View application consisting of a @subpage ui_main_menu and four tabbed pages. After creating a new project, using TOGA a basic four-step process:

1. Enter general bridge information on the @subpage ui_bridge_input_tab.
2. Enter girder-specific design information on the @subpage ui_girder_input_tab.
3. View both Original and Fabricator Optional girder designs on the @subpage ui_girder_view_tab. This step is optional, but helpful.
4. Click on the @subpage ui_analysis_report_tab to review results and print the long, or short form analysis report.

Advanced Usage
--------------
All TOGA analyses use simplified, limited input data to generate a PGSuper model behind the scenes. If questions arise about the assumptions made in TOGA you can use the "Export PGSuper Model" command from the main menu to save the generated PGSuper model. Then use the full power of PGSuper to examine all input data, assumptions, and results. Refer to the section [Generation of the PGSuper Model](@ref model_generation) in the Techinical Guide for details.

 
Other UI Elements
-----------------
* @subpage ui_select_strands_dialog
