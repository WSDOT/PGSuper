Design Live Loads {#ug_dialogs_design_live_loads}
==============================================
Design Live loads are defined for three categories of limit states. For each category of limit states the selected live loads are enveloped to determine the live load response.

The limit state categories are:

Category | Limit States Evaluated
---------|-----------------------
Design | Service I, Service IA (for LRFD 4th Edition with 2008 interims and earlier), Service III, Strength I
Fatigue | Fatigue I
Permit | Strength II

For each limit state category, check the boxes in the list to select the Vehicular Live Loads to be included in the analysis. The HL-93, Fatigue, Single-Unit SHVs, Notational Rating Load (NRL), and AASHTO Legal Loads are pre-defined. All other live loads are defined in the Vehicular Live Load library.

> NOTE: The live loads selected for the Design Limit States are used for the Design (Inventory and Operating) load rating analysis.

> NOTE: Live loads for legal and permit load ratings are defined in the @ref ug_dialogs_load_rating_options window.

Dynamic Load Allowance (Impact)
-------------------------------------
Truck and Lane load dynamic load allowance factors (impact factors) are defined as a percentage of the live load response. Live load responses always include the dynamic load allowance.

Application of Pedestrian Live Loads to Sidewalks
-------------------------------------------------
If a sidewalk of adequate width is present, pedestrian live loads are applied to those girders, mating surfaces, or webs loaded by the sidewalk dead load. The pedestrian live load is then combined with the vehicular live loads for the selected limit state. The possible options are:

1. Apply only vehicular live loads (ignore pedestrian loads all together)
2. Envelope pedestrian live loads with vehicular live loads. For this option, vehicular live loads are applied to all girders and enveloped with the application of pedestrian live loads. A multiple presence factor of 1.0 is used for the pedestrian live load. 
3. Apply pedestrian live loads concurrently with vehicular live loads. For this option, vehicular live loads are applied to all girders and summed with the application of pedestrian live loads.

Notes
------
* If you do not select any live loads for the Strength II category, then Strength II responses will not be computed.
* The optional deflection vehicle as defined by 3.6.1.3.2 is a part of the HL-93 response envelope. You must select HL-93 in order to activate the deflection vehicle.
* Refer to @ref ug_loading in the @ref user_guide for more information about load generation.
* Pedestrian loads will not be applied if the sidewalk width is less than the minimum width specified in the Project Criteria.
* When pedestrian live loads are applied, dead loads from the sidewalk and interior barriers are applied as if traffic is allowed to mount the sidewalk.

