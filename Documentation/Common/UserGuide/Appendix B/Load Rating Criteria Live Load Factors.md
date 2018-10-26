Live Load Factors {#appendix_b_load_rating_criteria_live_load_factors}
==============================================
This tab is used to define live load factors for the various load rating cases. The live load factors model is based on a generalization of Table 6A.4.5.4.2a-1 of the AASHTO MBE. The title of the tab indicates the load rating case. The load rating cases are:

Load Rating Case/Tab Title | Description
---------------------------|------------------
Design - Inventory | Define live load factors for the design, inventory load rating (MBE 6A.4.3.2.2)
Design - Operating | Define live load factors for the design, operating load rating (MBE 6A.4.3.2.2)
Legal - Routine | Define live load factors for the legal load rating for routine commercial traffic (MBE 6A.4.4.2.3a)
Legal - Special | Define live load factors for the legal load rating for specialized hauling vehicles (MBE 6A.4.4.2.3b)
Permit - Routine | Define live load factors for the permit load rating for routine or annual permits (MBE 6A.4.5.4.2a)
Permit - Special, Single trip with escort | Define live load factors for the permit load rating for special single trip permits when the live load is escorted with no other vehicles on the bridge (MBE 6A.4.5.4.2a)
Permit - Special, Single trip with traffic | Define live load factors for the permit load rating for special single trip permits mixed with other vehicular traffic on the bridge (MBE 6A.4.5.4.2a)
Permit - Special, Multiple trips with traffic | Define live load factors for permit load rating for multiple trip permits mixed with other vehicular traffic on the bridg e( MBE 6A.4.5.4.2a)

All of the live load factors defined in the AASHTO MBE can be modeled by selecting the appropreate model type. The model types are:

Live Load Factor Model Type | Description
----------------------------|----------------------
Single Value | A single value is used for the live load factor. This would be applicable to the design case. See MBE Table 6A.4.3.2.2-1
Stepped | One live load factor is used when the ADTT is below a threshold value, and a different live load factors is used when the ADTT exceeds the threshold value. An additional live load factor is available for cases when the ADTT is unknown.
Linear | The live load factor varies linearly between two ADTT values. The live load factor is constant for ADTT's that are less than and greater than these ADTTs. An additional live load factor is available for cases when the ADTT is unknown. 
Bilinear | Similar to the Linear model except that there are three ADTT control values. See MBE Table 6A.4.4.2.3a-1 and Table 6A.4.4.2.3b-1.
Bilinear with vehicle weight | Same as the Bilinear model except there is a secondary interpolatation for vehicle weight. See MBE Table 6A.4.5.4.2a-1.


Use the following parameters to define the live load factor model.

Item | Description
----|-------
Live Load Factor Model | Select a live load factor model from the list.
ADTT | Live load factors for up to three ADTT thresholds can be defined. Enter the ADTT thresholds for one directional average daily truck traffic. ADTT = Unknown is defines live load factors when the ADTT is unknown.
gLL | Live load factor. Live load factors are defined for Service and Strength Limit States.
Permit Weight Ratio | Enter GVW/AL thresholds where GVW = Gross Vehicle Weight, and AL = front axle to rear axle length using only the axles on the bridge. See MBE Table 6A.4.5.4.2a-1 for additional information.
Allow live load factors to be modified | When this is checked, the user can input live load factors that override these factors, otherwise the live load factors are always computed in accordance with this live load model.
Interpolation | Select the method for interpolating live load factors for the input ADTT. Live load factors can be linearly interpolated between ADTT or rounded up to the next highest ADTT.

