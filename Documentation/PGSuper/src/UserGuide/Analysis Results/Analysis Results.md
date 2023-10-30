Analysis Results {#ug_analysis_results}
==============================================
In this section you will learn how to view analysis results. Analysis results are available from a variety of graph views and reports. 

Simply open graph and report windows to get analysis results. This is part of the _Bridge-centric_ user interface. The basic idea is this; keep the engineer focused on the engineering problem and not the details of the software. You are using this software because you need to know specific information about your structure. The _Bridge-centric_ interface allows you to stay focused on your goal.

> NOTE: This section assumes that AutoCalc mode is enabled. See @ref ug_autocalc_mode for information.

Understanding Analysis Results
------------------------------
Before learning how to access the analysis results, we need to take a few minutes to understand how analysis results are presented. 

The bridge modeling follows the typical construction sequence of a precast-prestressed girder bridge. We refer to this as the construction timeline. The timeline consists of predefined Construction Events. Each construction event defines the Construction Activities that take place during the event, such as "Construct Girder" and "Cast Deck".

The construction activities have predefined steps. For example, in the Construct Girders Activity the following steps take place:
1. Tension the strand
2. Cast the girder concrete
3. Wait for concrete to cure
4. Release the prestressing
5. Lift the girder and move it to storage
6. Wait to ship the girder to the bridge site.

Erecting the girders consists of:
1. Ship the girders to the bridge site
2. Erect the girders onto their permanent bearings
3. Remove temporary strands
4. Cast diaphragms

The steps in the construction activities relate to Analysis Intervals. The Analysis Intervals represent the intervals of time over which changes occur. For example, while the girder is in storage, creep and shrinkage are happening. Boundary conditions change during lifting and shipping so there are new states of stresses to analyze. The structal system changes when continuity is achieved between spans.

The analysis interval names describe the construction activities to which they are related. 

See @ref ug_bridge_modeling for more information about Construction Events.

The analysis results are presented based on the Analysis Intervals. The intervals for a typical precast-prestressed girder bridge are:

Interval | Name | Description
---------|------|-------------
1        | Stress Strands | Prestressing strands are stressed 
2        | Prestress Release (Casting Yard) | The prestressing force is imparted onto the girder
3        | Lift Girders | The precast girders are lifted from the casting bed
4        | Place girders into storage | The precast girders are placed into storage and are supported at their storage support locations
5        | Haul Girders | The precast girders are hauled to the bridge site
6        | Erect Girders | The precast girders are erected onto the substructure at the bridge site.
7        | Remove temporary strands | Temporary strands are removed
8        | Cast Deck (Bridge Site 1) | The deck and diaphragms are cast and carried by the non-composite girder section
9        | Install Railing System and Overlay (Bridge Site 2) | The railing system and overlay are have been installed on the composite girder section. This is final without live load
10       | Open to Traffic (Bridge Site 3) | The bridge is open to traffic. This is final with live load.

> NOTE: The intervals are always numbered sequentially and some intervals may not be applicable to your bridge model. The intervals in your project may not exactly match the list above.

> NOTE: The interval names contain the old stage names in parentheses.

 
Graphical Results
-----------------
Many graphical results options are available. Select *View > Graphs > graph* to view graphical results. The available graphs are:

Graph | Description
-----------|------------
@subpage ug_analysis_results_graph | Traditional engineering graphs for moment, shear, deflection, rotation, stress, and reactions.  Note that these are separated into two views: 1) Individual girder results prior to erection and 2) In the full bridge after girder erections have taken place.
@subpage ug_concrete_properties_graph | Graphical representation of various concrete properties
@subpage ug_deflection_history_graph | Change in deflection at a point over time
@subpage ug_finished_elevations_graph | Elevations of deformed roadway and structure
@subpage ug_effective_prestress_graph | Changes in effective prestress by position and time
@subpage ug_girder_properties_graph | Graphical representation of various girder properties such as area, moment of inertia, and section modulus
@subpage ug_girder_stability_graph | Graphs stability factors of safety as a function of support location for lifting and hauling
@subpage ug_stress_history_graph | Change in stress at a point over time

**Exporting Results Data**: Data from graphs can be exported to a file by clicking on the *Export Graph Data* button. See @subpage ug_exporting_graph_data for more information.

> NOTE: Extensions can add more graph types. Refer to extension documentation for more specific information.

Reporting Results
------------------
There are many different types of reports. The reports focus on different aspects of your bridge model and analysis results.
Some common reports are:

Report | Description
-------|------------
Details Report | Provides a detailed account of all bridge and girder description parameters, project criteria, loads, structural analysis results, specification compliance checking results, and in-depth details of the calculations performed such as prestress loss and camber predications.
Spec. Check Report | Lists the results of specification compliance checking
Bridge Geometry Report | Lists geometric information about your project including horizontal curve details, vertical curve details, and framing details.

To create a report:
1. Select *View > Reports* and select a report. This will open the Report Options window for that report. 
2. Enter the options for the report you are creating. Reports often require a span and girder number as well as a list of chapters. Scroll through the list of chapters and place a check mark next to the ones you want reported. You can select several chapters at a time using the standard Ctrl+Click and Shift+Click mouse operations.
3. Press [OK] to create the report. The Report View is displayed when the report has been created.

> TIP: Quickly display a report by right-clicking on girder in the Bridge View and choosing a report from the context menu.

Once a report has been created, its contents can be edited by press [Edit] in the top left corner of the Report View. This will open the Report Options window and you can change the reporting parameters.

Scroll through the report to review its content.

> TIP: Click the right mouse button over the Report View window to activate the context menu. The context menu has many options such as a Table of Contents for quick navigation and a search feature.
