Analysis Results {#ug_analysis_results}
==============================================
In this section you will learn how to view analysis results. Analysis results are available from a variety of graph views and reports. 

Simply open graph and report windows to get analysis results. This is part of the _Bridge-centric_ user interface. The basic idea is this: keep the engineer focused on the engineering problem and not the details of the software. You are using this software because you need to know specific information about your structure. The _Bridge-centric_ interface allows you to stay focused on your goal.

> NOTE: This section assumes that AutoCalc mode is enabled. See @ref ug_autocalc_mode for information.

Understanding Analysis Results
------------------------------
Before learning how to access the analysis results, we need to take a few minutes to understand how analysis results are presented. 

The [construction sequence](@ref ug_construction_sequence) for your bridge is defined in the Timeline Manager. The timeline consists of Construction Events that you define. Each construction event defines the Construction Activities that take place during the event, such as "Construct Segments" and "Cast Closure Joints". 

The construction activities have predefined steps. For example, in the Construct Segments activity the following steps take place:
1. Tension the strand
2. Cast the segment concrete
3. Wait for concrete to cure
4. Release the prestressing
5. Lift the segment and move it to storage
6. Wait to ship the segment to the bridge site.

Erecting a segment consists of:
1. Ship the segment to the bridge site
2. Erect the segment onto its supporting elements such as abutments, piers, and temporary supports
3. Cast diaphragms
4. Remove temporary strands

The steps in the construction activities relate to Analysis Intervals. The Analysis Intervals represent the intervals of time over which changes occur. For example, while the segment is in storage, creep and shrinkage are happening. Boundary conditions change during lifting and shipping so there are new states of stresses to analyze. The structural system changes when tendons are stressed and the girder lifts off of temporary supports.

The name of the Analysis Intervals are generated from your Construction Event names and describe the steps of the construction activities to which they are related. 

See @ref ug_construction_sequence for more information about Construction Events.
 
Graphical Results
-----------------
Many graphical results options are available. Select *View > Graphs > graph* to view graphical results. The available graphs are:

Graph | Description
-----------|------------
@subpage ug_analysis_results_graph | Traditional engineering graphs for moment, shear, deflection, rotation, stress, and reactions. Note that these are separated into two views: 1) Individual segment results prior to erection and 2) In the full bridge after girder erections have taken place.
@subpage ug_concrete_properties_graph | Graphical representation of various concrete properties
@subpage ug_deflection_history_graph | Change in deflection at a point over time
@subpage ug_finished_elevations_graph | Elevations of deformed roadway and structure
@subpage ug_effective_prestress_graph | Changes in effective prestress by position and time
@subpage ug_girder_properties_graph | Graphical representation of various girder properties such as area, moment of inertia, and section modulus
@subpage ug_girder_stability_graph | Graphs stability factors of safety as a function of support location for lifting and hauling
@subpage ug_stress_history_graph | Change in stress at a point over time

**Exporting Graph Data**: Data from this graph can be exported to a file by clicking on the *Export Graph Data* button. See @subpage ug_exporting_graph_data for more information.

> NOTE: Extensions can add more graph types. Refer to extension documentation for more specific information.

Reporting Results
------------------
There are many different types of reports. The reports focus on different aspects of your bridge model and analysis results.
Some common reports are:

Report | Description
-------|------------
Details Report | Provides a detailed accounting of all bridge and girder description parameters, project criteria, loads, structural analysis results, specification compliance checking results, and in-depth details of the calculations performed such as prestress loss and camber predications.
Time Step Details Report | Provides a detailed accounting of the time-step analysis
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
