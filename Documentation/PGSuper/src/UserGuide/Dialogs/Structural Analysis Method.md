Structural Analysis Method {#ug_dialogs_structural_analysis_method}
==============================================
PGSuper supports three methods of structural analysis.

Use the drop down list to select a method of structural analysis.

Method | Description
------|---------------
Simple Span | Girders are assumed to be simple spans regardless of the connection boundary conditions
Simple Spans made Continuous | Girders are treated as simple spans until they are made continuous. Continuity can be achieved before or after deck placement as specified in the connection boundary conditions.
Envelope of Simple Span and Simple Spans made Continuous | Both of the previous analysis methods are used and their results enveloped. The controlling values are used.
 
> NOTE: The connection boundary conditions are defined as part of a bridge pier. See @ref ug_bridge_modeling of the @ref user_guide for details.

