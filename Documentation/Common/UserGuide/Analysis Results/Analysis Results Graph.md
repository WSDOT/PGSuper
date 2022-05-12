Analysis Results Graph {#ug_analysis_results_graph}
==============================================
The Analysis Results Graph presents analysis results in traditional engineering graph formats including moment, shear, deflection, and stress diagrams. There are two analysis results views, allowing you to view individual segment results prior to erection, and girder responses in the bridge after erection.

Results can be presented for several different loadings for a specified analysis interval, or they can be shown for a specified loading as it changes over multiple analysis intervals.

The graph controller, found on the left side of the graph window, is used to select graphs. The main graph window consists of a title, legend, and the graphical representation of analysis results.

Graph Controller Item | Description
-----|-------------
Plot Mode | Select the plot mode. Select Plot by Interval to plot results for various loadings during a specific interval. Select Plot by Loading to plot results for a specific loading over multiple intervals.
Span | Select a span or All Spans
Girder | Select a girder, or girder line if All Spans is selected
Interval/Loading list | If the plot mode is Plot by Interval, select an interval, otherwise select a loading
Results Type | Select an analysis results type to plot such as moment, shear, deflection or stress.
Graph Item List | Select one or more items from the graph item list. If the plot mode is Plot by Interval, select loadings, otherwise select intervals. Hold [CTRL] to select multiple, non-sequential, items. Hold [SHIFT] and select the first and last items in a range
Show Grid | Toggles the grid 
Show Girder | Toggles the girder
Incremental/Cumulative | Plot incremental results (incremental change in results occurring during the selected interval) or cumulative results (sum of incremental results in this and all preceding intervals).
Top Girder | Toggles stress plot for top of girder
Bottom Girder | Toggles stress plot for bottom of girder
Top Deck | Toggles stress plot for top of deck
Bottom Deck | Toggles stress plot for bottom of deck
Include Elevation Adjustments | When checked, elevation adjustments are added to computed deflections (PGSplice only)
Structural Analysis Method | Select the structural analysis method
Include Unrecoverable Deflections Prior to Erection | When checked, unrecoverable girder deflections from prior to erection are summed with responses after erection.

**Note that the temporary support Elevation Adjustment feature has been disabled for this version of the program due to bugs that will be addressed in the next version. Any existing adjustments from previous versions of the program have been set to zero. If adjustments are needed, you will need to compute them by hand.**

> NOTE: Analysis results for the Lifting and Hauling Intervals are for plumb girder subjected to self-weight and prestress loading only.

**Exporting Results Data**: Data from graphs can be exported to a file by clicking on the *Export Graph Data* button. See @ref ug_exporting_graph_data for more information.

***Unrecoverable Deflections at Erection Due to Change in Modulus During Storage***

Girder Segments are not perfectly straight elements when placed at erection. They are shaped with permanent unrecoverable deformations caused by prestressing forces, creep, shrinkage, relaxation, girder hardening (increase in modulus of elasticity during storage), and pre-camber. The analysis results graph allows you to view the different aspects of unrecoverable deflections at the time of erection.

See @ref tg_deflections for more information.
