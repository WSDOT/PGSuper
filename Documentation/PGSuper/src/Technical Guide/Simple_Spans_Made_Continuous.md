Simple Spans Made Continuous {#tg_simple_spans_made_continuous}
======================================
Three methods of structural analysis for precast prestressed girder brides composed of simple spans made continuous are available. Bridges may be designed as a series of simple spans, simple spans made continuous, or for the envelope of these two conditions.

Guidance and requirements for the design of continuous bridges is given in LRFD 5.12.3.3 (*pre-2017: 5.14.1.4*)

Simple Spans and Envelope Analysis
--------------------------------------
When the Simple Span and Envelope analysis option is used, the requirements of LRFD 5.12.3.3 are not required to be satisfied unless the age of the precast concrete is less than 28 days at the time continuity is established (see LRFD 5.12.3.3 (*pre-2017: 5.14.1.4*)). Note that the 28-day requirement was added with the AASHTO LRFD BDS 10th Edition, 2024 and is not explicitly checked by this software.


Simple Spans Made Continuous
----------------------------
When the Simple Spans Made Continuous analysis option is used, the requirements of LRFD 5.12.3.3 (*pre-2017: 5.14.1.4*) are required to be satisfied. The sections below describe how this software addresses the AASHTO  requirements.

### 5.12.3.3.2 (pre-2017: 5.14.1.4.2)Restraint Moments ###
Restraint moments are not computed. Restraint moments can be computed by any method deemed appropriate by the engineer and modeled in PGSuper with user defined concentrated moments.

### 5.12.3.3.4 (pre-2017: 5.14.1.4.4) Age of Girder When Continuity is Established ###
No assumptions are made about the age of the girder when continuity is established. You'll have to take this into account when determining restraint moments.

### 5.12.3.3.5 (pre-2017: 5.14.1.4.5) Degree of Continuity at Various Limit States - LRFD 9th Edition and Earlier ###
For bridges with fully effective connections, continuity can be accounted for in the Strength and Service Limit States. For partially effective connections, continuity can only be accounted for in the Strength Limit State.

A connection is considered fully effective if the calculated stress at the bottom of the continuity diaphragm for the combination of superimposed permanent loads, creep, shrinkage, and 50 percent live load is compressive. This is consistent with the first bullet paragraph in LRFD 5.12.3.3.5.

### 5.12.3.3.5 Service Limit State - LRFD 10th Edition and Later ###
The evaluation of the 36.0 ksi limit on crack concrete reinforcement is not evaluated by this software.

### 5.12.3.3.6 (pre-2017: 5.14.1.4.6) Service Limit State - LRFD 9th Edition and Earlier ###
The top of the precast girders at interior supports are designed as reinforced concrete members at the strength limit state. In this case, the stress limits for the service limit state do not apply to the top of the girders near interior supports.

### 5.12.3.3.6 Strength Limit - LRFD 10th Edition and Later ###
Negative moment capacity is evaluated at interior supports at the strength limit state.

### 5.12.3.3.7 (pre-2017: 5.14.1.4.7) Strength Limit State - LRFD 9th Edition and Earlier ###
Negative moment capacity is evaluated at interior supports at the strength limit state.

### 5.12.3.3.7 Negative Moment Connections - LRFD 10th Edition and Later ###
Negative moment capacity is evaluated at interior supports at the strength limit state. Connection is issued to be through deck reinforcement or girder reinforcement for bridges without decks. Reinforcing detailing requirements are not checked by this software.

### 5.12.3.3.8 (pre-2017: 5.14.1.4.8) ###
This provision is not evaluated by this software.

