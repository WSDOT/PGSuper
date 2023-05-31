Finished Elevations Graph {#ug_finished_elevations_graph}
==============================================
Finished Elevations results can be presented for multiple analysis intervals.

The graph controller, found on the left side of the graph window, is used to select graphs. The main graph window consists of a title, legend, and the graphical representation of analysis results.

Graph Controller Item | Description
-----|-------------
Span | Select a span or All Spans (PGSuper)
Group | Select a group or All Groups (PGSplice)
Segment | Select a segment
Girder | Select a girder, or an entire girder line if All Spans/Groups is selected
Graph Type | Select to plot elevations directly, or to plot elevation differentials from the PGL.
Show Grid | Toggles the grid 
Show Girder | Toggles the girder
1/10th Points Only | Plot at 1/10th points only, or all locations along the selected girder. 
Show Design Roadway | Plot the design profile grade PGL along the girder
Show Finished Top of Deck | Plot the top of deformed finished deck. This can be compared with the PGL.
Show Finished Bottom of Deck | Plot the bottom of deformed finished deck. This is the top of the haunch if a haunch exists
Show Top of Deflected Girder | Toggles plot for top of girder
Show Bottom of Deflected Girder | Toggles plot for bottom of girder. Useful for seeing top of bearing elevations
Show Girder Top Chord | Toggle plot of girder chord lines
Show Haunch Depths | (Only for differential view). Overlay plot of haunch depths with other data.
Edit Haunch | Bridge up the Edit Haunch dialog. Haunch can be modified here and then results are plotted directly
Export Graph Data | Data from graphs can be exported to Microsoft Excel or CSV files by clicking on the [Export Graph Data] button. See @ref ug_exporting_graph_data for more information.
Fill Haunch Input with Computed Values... | (Explicit haunch input only) Brings up dialog enabling automated design of direct haunch depths. Dialog also can add a scalar value to current haunch depths.

See @ref tg_vertical_geometry in the Technical Guide for more information about vertical geometry design and haunch options.

> Note: Only the *Open to Traffic* interval is available when Slab Offset haunch input, or non-Time Step losses are selected. Finished elevations cannot be computed for these conditions at other intervals.
