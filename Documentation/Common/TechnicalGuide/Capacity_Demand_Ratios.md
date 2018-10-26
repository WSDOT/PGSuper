Capacity/Demand Ratios {#tg_capacity_demand_ratios}
======================================
Capacity/Demand (C/D) ratios are reported in several places in PGSuper (examples are stresses, moment capacity,...) The rules for reporting the ratios are consistent throughout, and are as follows:

* C/D is only computed when the algebraic signs of Capacity and Demand are the same 
* C/D's are reported using two decimal places in the form "(2.22)"
* When C/D exceeds 10.0, it is reported as "(10+)"
* When Demand exceeds Capacity the specification check is always reported as FAIL. A tolerance is never applied to make C/D equal 1.0. 
* For the case of zero capacity (e.g., zero allowable tension) C/D is not reported. 
* When Demand is 0.0 and Capacity is non-zero, C/D is reported as infinity. 
* For the case of sign reversals, C/D is reported as "(-)". For example, C is positive moment capacity and D is a negative moment.
 
