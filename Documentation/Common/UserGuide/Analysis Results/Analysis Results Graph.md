Analysis Results Graph {#ug_analysis_results_graph}
==============================================
The Analysis Results Graph presents analysis results in traditional engineering graph formats including moment, shear, deflection, and stress diagrams.

Results can be presented for several different loadings for a specified analysis interval, or they can be shown for a specified loading as it changes over multiple analysis intervals.

The graph controller, found on the left side of the graph window, is used to select graphs. The main graph window consists of a title, legend, and the graphical representation of analysis results.

Graph Controller Item | Description
-----|-------------
Plot Mode | Select the plot mode. Select Plot by Interval to plot results for various loadings during a specific interval. Select Plot by Loading to plot results for a specific loading over multiple intervals.
Span | Select a span or All Spans
Girder | Select a girder, or girder line if All Spans is selected
Interval/Loading list | If the plot mode is Plot by Interval, select an interval, otherwise select a loading
Results Type | Select an analysis results type to plot such as moment, shear, deflection or stress.
Graph Item List | Select one or more items from the graph item list. If the plot mode is Plot by Interval, select loadings, otherwise select intervals. Hold [CTRL] to select multiple, nonsequential, items. Hold [SHIFT] and select the first and last items in a range
Show Grid | Toggles the grid 
Show Girder | Toggles the girder
Incremental/Cumulative | Plot incremental results (incremental change in results occuring during the selected interval) or cumulative results (sum of incremental results in this and all preceding intervals).
Top Girder | Toggles stress plot for top of girder
Bottom Girder | Toggles stress plot for bottom of girder
Top Deck | Toggles stress plot for top of deck
Bottom Deck | Toggles stress plot for bottom of deck
Include Elevation Adjustments | When checked, elevation adjustments are added to computed deflections (PGSplice only)
Structural Analysis Method | Select the structural analysis method

> NOTE: Analysis results for the Lifting and Hauling Intervals are for plumb girder subjected to self-weight and prestress loading only.
