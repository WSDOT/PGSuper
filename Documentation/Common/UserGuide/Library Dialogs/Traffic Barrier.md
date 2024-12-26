Traffic Barrier {#ug_library_dialogs_traffic_barriers}
==============================================
This dialog is used to defined the cross section of the traffic barrier.

Item | Description
-----|------------
Entry Name | Each traffic barrier entry must have a unique name. The name may contain any characters and can be up to 32 characters in length. The name can be changed by selecting this entry in the Library Editor window and selecting *Edit > Rename Entry*
Barrier | Use the grid to input the coordinates that make up the barrier shape. The origin of the coordinate system is taken to be at the top edge of the deck (or top of girder if no deck is present) as shown in the schematic. The coordinates define a barrier on the left side of the bridge, looking ahead on station.
[Add] | Add a point to the grid
[Delete] | Remove a point from the grid
[Move Up] | Move the selected point upward in the grid
[Move Down] | Move the selected point downward in the grid 
[View] | View the traffic barrier shape
Curb Offset | Offset from the face of barrier to the nominal curb line. 
Weight | The weight of the barrier can be defined by an explicit value or computed based on the cross sectional area of the barrier. The deck material properties will be used for barrier density.
Ec | Modulus of elasticity of the barrier concrete.
Barrier is Structurally Continuous | Check this box if the barrier is to be considered structurally continuous per LRFD 2.5.2.6.2 and 4.6.2.6.1. Note that if the barrier is used as an interior barrier it is not considered when computing effective flange width (4.6.2.6.1).


