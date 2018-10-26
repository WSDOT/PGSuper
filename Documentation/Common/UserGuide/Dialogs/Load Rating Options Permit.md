Permit {#ug_dialogs_load_rating_options_permit}
==============================================
Define the load rating parameters for Permit level ratings

Live Loads
----------

Item | Description
-----|------------
Live Load Vehicles | Select the live load models to be included in the load rating calculations for Routing/Annual Permit and Special/Limited Crossing Permit Vehicles. If no live loads are selected a load rating will not be performed.
Dynamic Load Allowances | Input the truck and lane dynamic load allowance factors.
Permit Type | Select the permit type for the Special/Limited Crossing Permit evaluation.

Limit States
-------------
Enter the load factors for the Strength II, Service I, and Service III limit states. Enter the keyword "Compute" if the live load factors are to be computed based on the requirements of the rating criteria.

Analysis Controls
----------------

Item | Description
-----|------------
Rate for Service III Stresses | Check the box if Service III tension stresses are to be included in the load rating.
Allowable Tension | Enter the allowable tension stress coefficient.
Check reinforcement yielding | Reinforcement is evaluating for yielding at the Service I limit state when checked. See MBE 6A.5.4.2.2b for guidance.
Allowable Stress | Enter the allowable yield stress coefficient.
Rate for Shear | Check the box if shear is to be included in the load rating.

> NOTE: Load rating for Service III tension stresses is a WSDOT requirement (See Chapter 13 of the WSDOT Bridge Design Manual). This is not a required in the AASHTO MBE.
