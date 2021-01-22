Reactions {#tg_reactions}
======================================
Reactions are computed along indvidual girder lines for piers and bearings. These are known as Girder Line Pier Reactions and Girder Bearing Reactions. 

Reactions are listed in the Details Report in the Moments, Shears, and Reactions chapter. And, in the Pier Reactions report.

Structural analysis models are girder line models, as explained in @ref tg_structural_analysis_models. The reported reactions are those that result from the individual girder line analysis models.

Girder Line Pier Reactions
======================================
Girder line pier reactions are the vertical reactions at the support point for a pier. When a pier is described with a physical model, the reactions listed are the vertical force at the base of column caused by superstructure loads (they do not include the weight of the pier cap or columns). Moment reactions are not reported in the Details Report, however they are available in the Pier Reactions Report. 

### Dead Load Reactions
Dead load reactions are the simple span reactions from girder self-weight, slab, slab haunch, and all other dead loads applied before the deck becomes composite with the girders and spans become continuous at continuity diaphragms, and the continuous span reactions for superimposed dead loads such as the railing system and overlay. Dead load reactions are per girder values.

### Live Load Reactions
Live load reactions are the reactions from the structural frame model for an entire girder line that maximize the vertical force in a pier. This reaction is typically maximized by the HL-93 load case consisting of 90% of two design trucks spaced at least 50 ft apart and the design lane load. Girder line pier reactions for live load are per lane (they are not reduced with a live load distribution factor) including impact.

Girder Bearing Reactions
======================================
Girder bearing reactions are the reactions at the bearings. Girder bearing reactions are for loads on bearings from erection to the the time girders are made continuous with an adjacent span or integral with a pier and adjacent span. For simple bearing conditions, such as at abutments and expansion piers, girder bearing reactions are reported for all loads.

When there are multiple bearings at the end of the girder, the total bearing reaction is distributed equally to each bearing.

### Dead Load Reactions
Dead load reactions are the simple span reactions from girder self-weight, slab, slab haunch, railing system, overlay and all the other dead loads occuring during simple-span conditions. Dead load reactions are per girder values.

### Live Load Reactions
Live load reactions are the reactions from the structural frame model that maximize the vertical force in a bearing. This reaction is typically maximized by the HL-93 load case consisting of a design truck with the 32 kip axle located directly above the bearing and the design lane load. Live load values are per lane (they are not reduced with a live load distribution factor). Impact is included if it is specified in the Project Criteria.

Pier Reactions Report
======================================
The pier reactions report lists optimized pier reactions from each girder line model. These reactions include horizontal force (F<sub>x</sub>), vertical force (F<sub>y</sub>), and moment (M<sub>z</sub>). Live load reactions consist of the optimized (minimum and maximum value) F<sub>x</sub>, F<sub>y</sub>, and M<sub>z</sub> reactions and their corresponding force values. Live load reactions for the Design, Fatigue, and Load Rating conditions are reported.

Dead load reactions are per girder and live load reactions are per lane (they are not reduced with a live load distribution factor) including impact.

> NOTE: When a bridge model has a different number of girders in each span, the sum of the girder line pier reactions will be greater than the actual total pier reaction. Some girders are modeled in multiple girder lines as illustrated in @ref tg_structural_analysis_models.