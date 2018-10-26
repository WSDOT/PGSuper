Load Rating {#ug_load_rating}
==============================================
Load rating is an analysis used to determine the load carrying capacity of an existing bridge. Beginning October 1, 2010, all new and total replacement bridges designed by the LRFD Specifications must be rated by LRFR methods.

From FHWA Memorandum dated October 30, 2006 (http://www.fhwa.dot.gov/BRIDGE/nbis/103006.cfm), _For bridges and total replacement bridges designed by LRFD Specifications using HL-93, after October 1, 2010 Items 63, 64, 65, and 66 are to be computed and reported to the NBI as a RF based on LRFR methods using HL-93 loading_.

> NOTE: The MBE 2010 interim provisions, section C6A.1.1 defer the load rating requirements if a structure is designed using LRFD and the HL-93 loading until changes to the structure occur that would reduce the inventory rating below the design load level.

Load rating analysis of the bridge superstructure can be performed in accordance with Section 6, Part A of the AASHTO Manual for Bridge Evaluation.

> TIP: The XBRate application can be used to load rate reinforced concrete cross beams

> TIP: MBE Section 6, Part A, is commonly referred to as the "LRFR" rating.

Load rating calculations can be performed for:
* Inventory
* Operating
* Legal Loads with Routine Commercial Traffic
* Legal Loads with Specialized Hauling Vehicles
* Permit Loads for routine or annual permits
* Permit Loads for special or limited crossing permits

Rating factors can be computed for positive moment, negative moment, shear, and flexural stresses.

If needed, a load posting analysis is automatically performed for legal load ratings.

Load factors, limit state factors, dynamic load allowances, allowable stresses, condition factors, and system factors can be controlled by the bridge owner and the engineer.

Girder Line Load Rating
-----------------------
Load ratings are performed for a girder line. When a project template is configured with the required rating options, performing a load rating is as simple as creating a load rating report. This section will describe all the steps necessary to configure the rating options and perform a load rating.

### Load Rating Options ###
Load rating options control the type of ratings that are performed, live loads, load factors, and allowable stress limits.

To edit the Load Rating Options:
1. Select *Project > Load Rating Options*
2. On the General tab, define the general rating options including the load rating criteria, ADTT, and the type of load rating to be performed.
3. On the Design tab, define the rating options for the design loads including the load factors.
4. On the Legal tab, define the rating options for a legal load rating including the legal live loads and load factors.
5. On the Permit tab, define the rating options for a permit load rating including the permit live loads and load factors.

> NOTE: Specialized vehicles for legal and permit ratings can be defined in the Vehicular Live Load library.

### Load Rating Results ###
A load rating is automatically performed when a Load Rating report is created. Refer to @ref ug_analysis_results for more information regarding analysis results and reports. 

The load rating report lists the controlling rating factors and provides all the detailed supporting computations.
