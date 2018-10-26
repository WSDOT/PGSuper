Vehicular Live Load {#appendix_b_vehicular_live_load}
==============================================
Vehicular Live Load library entiries define specialized vehicles for live load analysis. These vehicles can be used for design and load ratings.

Item | Description
----|-----
Entry Name | Each entry must have a unique name. The name may contain any characters and can be up to 32 characters in length. The name can be changed by selecting this entry in the Library Editor window and selecting *Edit > Rename Entry*
Usage | Select how this live load is to be used for analysis. Options are: <ol><li>Use for all actions at all locations</li><li>Use only for negative moments between points of contraflexure and interior pier reactions</li><li>Use only for negative moments and interior pier reactions</li></ol>
Load Type | Select the type of live load model. <table><tr><th></th><th>Description</th></tr><tr><td>Truck Only</td><td>This loading consists only of a truck</td></tr><tr><td>Lane Load Only</td><td>This loading consists only of a lane load</td></tr><tr><td>Sum of Lane Load and Truck</td><td>The response of the lane load and truck are added together</td></tr><tr><td>Envelope of Lane Load and Truck</td><td> The response of the lane load and truck are enveloped</td></tr></table>
Lane Load | Input the magnitude of the lane load and the span length for which it will be applied. If any span length, measured as the pier-to-pier distance, exceeds the threshold value, the lane load will be applied. A span length of zero will cause the lane load to always be applied.
Truck Configuration | Describe the truck configuration. Trucks are defined as a series of axle weights and spacing. Use the [Add] and [Delete] buttons to define the axles. Spacing is measured as the distance between axles. A single axle spacing can be optional designated as a variable spacing. For a variable spacing, input the axle spacing as min-max (e.g. 14-30). 
Neglect axles that do not contributed to the maximum load effect under consideration | When this box is checked, axles not contributing to the maximum load effect under consideration are neglected. This is typically done with notional loads such as HL-93 and NRL.


