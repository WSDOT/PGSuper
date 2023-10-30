Interface Shear {#tg_interface_shear}
======================================
Interface shear transfer is computed using the methods outlined in LRFD 5.7.4 (*pre-2017: 5.8.4*). Cacluations are carried out on a per unit of girder length basis.

## Deck Systems
The following assumptions are made about the permanent net compressive force normal to the shear plane (P<sub>c</sub>) for Cast-In-Place (CIP) reinforced concrete decks and Stay-In-Place (SIP) Deck Panel systems.

The permanent net compressive force normal to the shear plane (P<sub>c</sub>) can be conservatively neglected as described in LRFD C5.7.4.3. This option can be set in the Project Criteria.

For CIP deck systems, P<sub>c</sub>, if used, is computed using the tributary width of the deck slab. 

For SIP deck systems, P<sub>c</sub>, if used, is computed using only the amount of cast in place deck concrete directly above the area of the interface surface not supporting the precast panels. This reduced area is assumed to be the only area participating in the shear transfer.   

## Interface Shear Width
The area of concrete considered to be engaged in interface shear transfer is defined by LRFD Eq. 5.7.4.3-6 as A<sub>cv</sub> = b<sub>vi</sub>L<sub>vi</sub> 

where

b<sub>vi</sub> = interface width considered to be engaged in shear transfer  
L<sub>vi</sub> = interface length considered to be engaged in shear transfer  

The interface shear analysis is carried out on a per unit length basis. Therefore, L<sub>vi</sub> is taken to be 1 unit. 

The interface shear width, b<sub>vi</sub> is taken to be the sum of all mating surface widths, w<sub>ms</sub>, in the girder section and the reductions discussed below. 

> Mating surfaces are those surfaces on the top of the girder that mate with the deck.  
> Typically, an I-Beam has a single mating surface and U-Beams have two. 

### Cast-in-Place Deck Reductions
The definition of the girder in the Girder Library has a parameter, "Total interface shear width reduction", b<sub>vir</sub>. This parameter is typically used to reduce the interface width because of intentionally smooth finish and bond breaker at the extremities of top flanges to facilitate deck removal. The reduction is deducted from the sum of the mating surface widths. The interface shear width is

b<sub>vi</sub> = w<sub>ms</sub> - b<sub>vir</sub>.

### Stay-in-Place Panel Reductions
For SIP deck panels, the panel support width, w<sub>dp</sub>, is deducted from both sides of each mating surface except for the exterior side of exterior girders. 

Consider an interior U-Beam with two mating surfaces (top flanges). It is assumed that there is a panel on each side of the girder spanning to the adjacent girders and a panel spanning the webs (3 panels in total total supported by the girder). A total of 4 panel support widths are deducted from the sum of the top flange widths. The total interface shear width reduction is not applied. The interface shear width is  

b<sub>vi</sub> = w<sub>ms</sub> - 4w<sub>dp</sub>.

For an exterior girder, there is not a panel on the exterior side of the beam. The deck overhang is cast full depth. In this case, the one-half of total interface shear with reduction is applied in addition to the panel support with reductions. The interface shear width is  

b<sub>vi</sub> = w<sub>ms</sub> - 3w<sub>dp</sub> - b<sub>vir</sub>/2.

## Shear Stress at Slab/Girder Interface
The shear stress at the slab/girder interface can be computed by either the LRFD approximate equation v<sub>ui</sub> = V<sub>u</sub>/(b<sub>vi</sub>d<sub>v</sub>) or by the classical shear flow equation v<sub>ui</sub> = (V<sub>u</sub>Q)/(I<sub>x</sub>b<sub>vi</sub>).

