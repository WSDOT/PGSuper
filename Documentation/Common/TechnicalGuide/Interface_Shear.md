Interface Shear {#interface_shear}
======================================
PGSuper calculates interface shear transfer resistance using the methods outlined in LRFD 5.8.4.

## Definitions
The term A<sub>vf</sub> is used inconsistently in the LRFD specifications. In some cases A<sub>vf</sub> is treated as the area of shear reinforcement crossing the shear plane, and in other cases it is treated as reinforcement area/length along the member. To avoid this confusion we make the following definition:

A<sub>vf</sub> = Total area of shear reinforcement crossing the shear plane.

a<sub>vf</sub> = Area of shear reinforcement per unit length along the girder.  

Hence, LRFD Equation 5.8.4.4-1 would be written as:

a<sub>vf</sub> >= 0.35 b<sub>v</sub> / f<sub>y</sub> (SI)

a<sub>vf</sub> >= 0.05 b<sub>v</sub> / f<sub>y</sub> (US)

## Deck Systems
PGSuper supports Cast-In-Place (CIP) reinforced concrete decks and Stay-In-Place (SIP) Deck Panel systems. The following assumptions are made about the permanent net compressive force normal to the shear plane (P<sub>c</sub>) and area of concrete engaged in shear transfer (A<sub>cv</sub>).

For CIP deck systems, P<sub>c</sub> is computed using the tributary width of the deck slab. The entire top flange(s) of the girder is assumed to participate in the shear transfer.

For SIP deck systems, P<sub>c</sub> is computed using only the amount of cast in place deck concrete directly above the area of the top flange not supporting the precast panels. This reduced area is assomed to be the only area participating in the shear transfer.  

## Shear Stress at Deck/Slab Interface
The shear stress at the deck slab interface can be computed by either the LRFD approximate equation v = V/bd or by the classical shear flow equation v = VQ/Ib.
