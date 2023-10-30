Analysis Results Graph {#ug_analysis_results_graph}
==============================================
The Analysis Results Graph presents analysis results in traditional engineering graph formats including moment, shear, deflection, and stress diagrams. There are two analysis results views, allowing you to view individual results before erection and after erection.

The distinction of before and after erection isn't significant for pretensioned girder bridges (PGSuper) but is very meaningful for spliced girder bridges (PGSplice).

Results can be presented for several different loadings for a specified analysis interval, or they can be shown for a specified loading as it changes over multiple analysis intervals.

The graph controller, found on the left side of the graph window, is used to select graphs. The main graph window consists of a title, legend, and the graphical representation of analysis results.

Graph Controller Item | Description
-----|-------------
Plot Mode | Select the plot mode. Select Plot by Interval to plot results for various loadings during a specific interval. Select Plot by Loading to plot results for a specific loading over multiple intervals.
Span | Select a span or All Spans (PGSuper)
Group | Select a group or All Groups (PGSplice)
Segment | Select a segment
Girder | Select a girder, or girder line if All Spans/Groups is selected
Plot mode | Select Plot by Loading or Plot by Interval. Plot by Loading plots results for multiple loading conditions for a specified analysis interval. Plot by Interval plots results for a single loading across multiple analysis intervals.
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
Include Elevation Adjustments | When checked, elevation adjustments are added to computed deflections (PGSplice)
Structural Analysis Method | Select the structural analysis method (PGSuper)
Include Unrecoverable Deflection | When checked, unrecoverable deflection from prior to erection are summed with deflections occuring at and after erection. Precamber (PGSuper), and pretensioning are treated as unrecoverable deflections.
Export Graph Data | Data from graphs can be exported to Microsoft Excel or CSV files by clicking on the [Export Graph Data] button. See @ref ug_exporting_graph_data for more information.

> NOTE: Analysis results for the Lifting and Hauling Intervals are for plumb girder subjected to self-weight and prestress loading only.

Unrecoverable Deflection at Erection
-----------------------------------------------------------------------------
Pretensioned elements are moved through a variety of support conditions from initial prestress release, storage, lifting, hauling, and erection. The deflected shape due to girder self weight changes when the boundary conditions change. Over time, the elastic modulus of the concrete increases and some deflections cannot be fully recovered when support conditions change. Pretensioning and creep also create unrecoverable deflections.

For this reason, precast elements (girders/segments) are not perfectly straight when they are erected. The analysis results graph shows the unrecoverable deflections at the time of erection.

As an example, consider the dead load deflection of a simple span beam. The beam is initially supported near its ends and deflection is \f$ \Delta = \frac{5wL^4}{384E_{ci}I}\f$. Overtime, the elastic modulus increases from \f$E_{ci}\f$ to \f$E_c\f$. If the beam was then completely unloaded by an equal and opposite load to the self-weight, deflection does not return to zero because of the change in the elastic modulus. The unrecoverable deflection is \f$ \frac{5wL^4}{384E_{ci}I} - \frac{5wL^4}{384E_cI}\f$. When the deflection at erection is computed using \f$E_c\f$, the unrecoverable deflection must be included to get the true deflection.

See @ref tg_deflections in the @ref technical_guide for more information.
