///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once
#define IDH_PGSLIBRARY                           99
#define IDH_PGSUPER                             100

#define IDH_BRIDGE_VIEW                         101
#define IDH_GIRDER_VIEW                         102
#define IDH_REPORT_VIEW                         103
#define IDH_ANALYSIS_RESULTS                    104
#define IDH_EFFECTIVE_PRESTRESS                 105
#define IDH_STABILITY_VIEW                      106
#define IDH_STRESS_HISTORY                      107
#define IDH_GIRDER_PROPERTIES                   108
#define IDH_CONCRETE_PROPERTIES                 109
#define IDH_DEFLECTION_HISTORY                  110
#define IDH_LIBRARY_VIEW            111
#define IDH_FINISHED_ELEVATION_VIEW             112
#define IDH_FILL_HAUNCH                         113

#define IDH_ALIGNMENT_HORIZONTAL		 200
#define IDH_ALIGNMENT_PROFILE			 201
#define IDH_ALIGNMENT_SUPERELEVATION 202

#define IDH_BRIDGEDESC_GENERAL       300
#define IDH_BRIDGEDESC_FRAMING       301
#define IDH_BRIDGEDESC_RAILING       302
#define IDH_BRIDGEDESC_DECKDETAILS	 303
#define IDH_BRIDGEDESC_DECKREBAR	    304
#define IDH_BRIDGEDESC_ENVIRONMENTAL 305

#define IDH_DIALOG_REPORT              401
#define IDH_DIALOG_PROPERTIES          402
#define IDH_DIALOG_COPYGDRPROPERTIES   403
#define IDH_DIALOG_DESIGNGIRDER        404
#define IDH_DIALOG_DESIGNCOMPLETE      405
#define IDH_DIALOG_DESIGNCRITERIA      407
#define IDH_DIALOG_ENVIRONMENT         408
#define IDH_DIALOG_LIBENTRYCONFLICT    409
#define IDH_DIALOG_SECTIONCUT          410
#define IDH_DIALOG_LOADMODIFIERS       411
#define IDH_DIALOG_LIBIMPORTENTRYCONFLICT 412
#define IDH_LIVELOAD_DIALOG			   414
#define IDH_MULTIGIRDER_REPORT         417
#define IDH_EDIT_HAUNCH                420
#define IDH_EDIT_BEARINGS              421
#define IDH_LOADRATING_REPORT          422
#define IDH_LOADRATING_SUMMARY_REPORT  423
#define IDH_DIALOG_COPYPIERPROPERTIES   424
#define IDH_DIALOG_COPYTEMPSUPPORTPROPERTIES   425
#define IDH_MOMENT_CAPACITY_DETAILS_REPORT  426
#define IDH_CRACKED_SECTION_DETAILS_REPORT  427
#define IDH_LOAD_FACTORS 428


#define IDH_GIRDERDETAILS_GENERAL           2001
#define IDH_GIRDERDETAILS_STRANDS           2002
#define IDH_GIRDERDETAILS_DEBOND            2003
#define IDH_GIRDERDETAILS_STRAND_EXTENSIONS 2004
#define IDH_GIRDERDETAILS_TRANSV_REBAR      2005
#define IDH_GIRDERDETAILS_ADDITIONAL_INTERFACE_SHEAR_REBAR 2006
#define IDH_GIRDERDETAILS_LONGIT_REBAR      2007
#define IDH_GIRDERDETAILS_TEMPORARY_CONDITIONS          2008

#define IDH_SEGMENTDETAILS_GENERAL            2010

#define IDH_PIERDETAILS_GENERAL         2100
#define IDH_PIERDETAILS_LAYOUT          2101
#define IDH_PIERDETAILS_CONNECTIONS     2102
#define IDH_PIERDETAILS_GIRDERSPACING   2103

#define IDH_CLOSUREJOINT_DETAILS_GENERAL  2105

#define IDH_TSDETAILS_GENERAL          2107
#define IDH_TSDETAILS_CONNECTION       2108
#define IDH_TSDETAILS_SPACING          2109

#define IDH_SPANDETAILS_GENERAL			2110
#define IDH_SPANDETAILS_GIRDERSPACING	2112

#define IDH_BRIDGEVIEW_PLAN      3500
#define IDH_BRIDGEVIEW_SECTION   3501
#define IDH_BRIDGEVIEW_ALIGNMENT 3502
#define IDH_BRIDGEVIEW_PROFILE   3503

#define IDH_GIRDERVIEW_SECTION   3600
#define IDH_GIRDERVIEW_ELEV      3601

#define IDH_CONFIGURE_PGSUPER	 3650
#define IDH_CONFIGURATION_SERVERS 3651
#define IDH_CONFIGURATION_SERVER_DEFINITION 3652

// library editor dialogs
#define IDH_GIRDER_CONNECTION_DIALOG            3703
#define IDH_GIRDER_DIMENSIONS                   3705
#define IDH_GIRDER_TEMPORARY_STRANDS            3706
#define IDH_GIRDER_PERMANENT_STRANDS            3707
#define IDH_GIRDER_LONGITUDINAL_REINFORCEMENT   3708
#define IDH_GIRDER_TRANSVERSE_REINFORCEMENT     3709
#define IDH_GIRDER_HARPING_POINTS               3710
#define IDH_GIRDER_DIAPHRAGMS                   3711
#define IDH_TRAFFIC_BARRIER_DIALOG              3712
#define IDH_PROJECT_CRITERIA_GENERAL            3715
#define IDH_PROJECT_CRITERIA_HAULING            3717
#define IDH_PROJECT_CRITERIA_DEAD_LOADS         3718
#define IDH_PROJECT_CRITERIA_LIVE_LOADS         3719
#define IDH_PROJECT_CRITERIA_LIFTING            3720
#define IDH_PROJECT_CRITERIA_LOSSES             3721
#define IDH_PROJECT_CRITERIA_CREEP              3722
#define IDH_PROJECT_CRITERIA_PRESTRESSING       3723
#define IDH_PROJECT_CRITERIA_PRESTRESS_ELEMENTS 3725
#define IDH_COPY_CONCRETE                       3726
#define IDH_EDIT_LOADS                          3727
#define IDH_EDIT_POINT_LOADS                    3728
#define IDH_EDIT_DISTRIBUTED_LOADS              3729
#define IDH_DISTRIBUTION_FACTORS                3730
#define IDH_PROJECT_CRITERIA_DESIGN             3732
#define IDH_PROJECT_CRITERIA_LIMITS             3733
#define IDH_PROJECT_CRITERIA_SHEAR_CAPACITY     3735
#define IDH_PROJECT_CRITERIA_MOMENT_CAPACITY    3736
#define IDH_EDIT_MOMENT_LOADS                   3739
#define IDH_GIRDER_FLEXURAL_DESIGN              3740
#define IDH_GIRDER_SHEAR_DESIGN                 3741
#define IDH_EFFECTIVE_FLANGE_WIDTH              3742
#define IDH_GIRDER_HAUNCH_AND_CAMBER            3743
#define IDH_PROJECT_CRITERIA_CLOSURE_JOINTS     3744
#define IDH_DUCT_DIALOG                         3745
#define IDH_TIMELINE_MANAGER                    3746
#define IDH_TIMELINE_EVENT                      3747
#define IDH_TIMELINE_CREATE_EVENT               3748
#define IDH_TIMESTEP_PROPERTIES                 3749
#define IDH_INSERT_SPAN                         3750
#define IDH_LINEAR_DUCT                         3751
#define IDH_PARABOLIC_DUCT                      3752
#define IDH_POST_TENSIONING                     3753
#define IDH_PRETENSIONING                       3754
#define IDH_SPLICED_GIRDER_GENERAL              3755
#define IDH_HAUL_TRUCK_DIALOG                   3756
#define IDH_PROJECT_CRITERIA_BEARINGS           3757
#define IDH_GEOMETRY_CONTOL_EVENT               3758

#define IDH_SELECT_LIVELOAD			  	         3790

#define IDH_STRUCTURAL_ANALYSIS                 3792

#define IDH_RATING_GENERAL_TAB                  3800
#define IDH_RATING_DESIGN_TAB                   3801
#define IDH_RATING_LEGAL_TAB                    3802
#define IDH_RATING_PERMIT_TAB                   3803

#define IDH_PLUGINS                             3810

#define IDH_FILE_COMPATIBILITY_WARNING          3811

#define IDH_GENERATE_STRANDS                    3900
#define IDH_STRAND_LOCATION                     3901

#define IDH_LOAD_RATING_CRITERIA                3910
#define IDH_LOAD_RATING_LIVE_LOAD_FACTORS       3911

#define IDH_CONSTRUCTION_LOADS                  3920
#define IDH_FILL_DISTRIBUTION_FACTORS           3921
#define IDH_GIRDER_DIRECT_STRAND_FILL           3922
#define IDH_GIRDER_STRAND_ROW_INPUT             3923
#define IDH_GIRDER_INDIVIDUAL_STRAND_INPUT      3924
#define IDH_SEGMENT_TENDON_INPUT                3925

#define IDH_CUSTOM_REPORT                       3950
#define IDH_FAVORITE_REPORT                     3951
#define IDH_CUSTOM_REPORT_DEFINITION            3952

#define IDH_DIAPHRAGM_LAYOUT_RULES     3999

#define IDH_CONCRETE_GENERAL           4000
#define IDH_CONCRETE_AASHTO            4001
#define IDH_CONCRETE_ACI               4002
#define IDH_CONCRETE_CEBFIP            4003
#define IDH_CONCRETE_PCIUHPC           4004
#define IDH_CONCRETE_UHPC          4005

#define IDH_ERECT_PIERS                5000
#define IDH_CONSTRUCT_SEGMENTS         5001
#define IDH_ERECT_SEGMENTS             5002
#define IDH_CAST_CLOSURE_JOINTS        5003
#define IDH_INSTALL_TENDONS            5004
#define IDH_CAST_DECK                  5005
#define IDH_CAST_LONGITUDINAL_JOINTS   5006
#define IDH_APPLY_LOADS                5007
#define IDH_REMOVE_TEMPORARY_SUPPORTS  5008
#define IDH_CAST_INTERMEDIATE_DIAPHRAGMS 5009

