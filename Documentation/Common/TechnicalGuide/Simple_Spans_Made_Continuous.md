Simple Spans Made Continuous {#simple_spans_made_continuous}
======================================
PGSuper offers three methods of structural analysis for precast prestressed girder brides composed of simple spans made continuous. Bridges may be designed as a series of simple spans, simple spans made continuous, or for the envelope of these two conditions.

Guidance and requirements for the design of continuous bridges is given in LRFD 5.14.1.4

## Simple Spans and Envelope Analysis
When the Simple Span and Envelope analysis option is used, the requirements of LRFD 5.14.1.4 are not required to be satisfied (see LRFD 5.14.1.4.1)

## Simple Spans Made Continuous
When the Simple Spans Made Continuous analysis option is used, the requirements of LRFD 5.14.1.4 are required to be satisfied. 

### 5.14.1.4.2 Restraint Moments
PGSuper does not compute restrain moments. Restraint moments can be computed by any method deemed appropriate by the engineer and modeled in PGSuper with user defined concentrated moments. (See Chapter 5 of the PGSuper User's Guide for details)

### 5.14.1.4.4 Age of Girder When Continuity is Established
PGSuper does not make any assumptions about the age of the girder when continuity is established.

### 5.14.1.4.5 Degree of Continuity at Various Limit States
For bridges with fully effective connections continuity can be accounted for in the Strength and Service Limit States. For partially effective connections, continuity can only be accounted for in the Strength Limit State.

PGSuper will consider a connection as fully effective if the calculated stress at the bottom of the continuity diaphragm for the combination of superimposed permanent loads, creep, shrinkage, and 50 percent live load is compressive. This is consistent with the first bullet paragraph in LRFD 5.14.1.4.5.

### 5.14.1.4.6 Service Limit State
PGSuper assumes that the top of the precast girders at interior supports are designed as reinforced concrete members at the strength limit state. In this case, the stress limits for the service limit state do not apply to the top of the girders near interior supports.

### 5.14.1.4.7 Strength Limit State
PGSuper evaluates the negative moment capacity at interior supports

### 5.14.1.4.8-10
PGSuper does not evaluate these provisions.

