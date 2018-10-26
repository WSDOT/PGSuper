Splitting Zone Checks {#splitting_zone_checks}
======================================
The reinforcement in the splitting/bursting zone is checked in accordance with LRFD 5.10.10.1. The area of shear reinforcement in the splitting zone is computed as

Av = SUMMATION(Number of Legs)(Bar Size)(Length)/(Spacing)

Length is the length of the stirrups zone that is contained within the splitting zone.

Example

Zone |  Start (ft) | End (ft) | Bar | # Legs | Spacing (in)
-----|-------------|----------|-----|--------|--------
1    |  0.00       |  1.00    | #5  |  2     | 6
2    |  1.00       |  4.00    | #5  |  2     | 9


Zone Length = 2.00 ft

Av = 2(0.31 5.10.10.1)(1.00 ft)/(0.5 ft) + 2(0.31 in2)(1.00 ft)/(0.75 ft) = 2.066 in<sup>2</sup>

> NOTE: WSDOT standard practice deviates from the LRFD specifications as outlined a [June 2001 Design Memorandum](http://www.wsdot.wa.gov/eesc/bridge/designmemos/08-2001.htm)

