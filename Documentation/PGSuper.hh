///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#define IDH_PGSUPER                 100

#define IDH_BRIDGE_VIEW             101
#define IDH_GIRDER_VIEW             102
#define IDH_REPORT_VIEW             103
#define IDH_ANALYSIS_RESULTS        104
#define IDH_EFFECTIVE_PRESTRESS     105
#define IDH_STABILITY_VIEW          106
#define IDH_STRESS_HISTORY          107
#define IDH_GIRDER_PROPERTIES       108
#define IDH_CONCRETE_PROPERTIES     109
#define IDH_DEFLECTION_HISTORY      110
#define IDH_LIBRARY_VIEW            111

#define IDH_ALIGNMENT_HORIZONTAL		 200
#define IDH_ALIGNMENT_PROFILE			 201
#define IDH_ALIGNMENT_SUPERELEVATION 202

#define IDH_BRIDGEDESC_GENERAL       300
#define IDH_BRIDGEDESC_FRAMING       301
#define IDH_BRIDGEDESC_RAILING       302
#define IDH_BRIDGEDESC_DECKDETAILS	 303
#define IDH_BRIDGEDESC_DECKREBAR	    304
#define IDH_BRIDGEDESC_ENVIRONMENTAL 305

#define IDH_DIALOG_TOOLBARS            400
#define IDH_DIALOG_REPORT              401
#define IDH_DIALOG_PROPERTIES          402
#define IDH_DIALOG_COPYGDRPROPERTIES   403
#define IDH_DIALOG_DESIGNGIRDER        404
#define IDH_DIALOG_DESIGNCOMPLETE      405
#define IDH_DIALOG_UNITS               406
#define IDH_DIALOG_DESIGNCRITERIA      407
#define IDH_DIALOG_ENVIRONMENT         408
#define IDH_DIALOG_LIBENTRYCONFLICT    409
#define IDH_DIALOG_SECTIONCUT          410
#define IDH_DIALOG_LOADMODIFIERS       411
#define IDH_DIALOG_LIBIMPORTENTRYCONFLICT 412
#define IDH_DIALOG_DESIGNDETAILS       413
#define IDH_LIVELOAD_DIALOG			   414
#define IDH_DECK_CONDITION             415
#define IDH_GIRDER_CONDITION           416
#define IDH_MULTIGIRDER_REPORT         417

#define IDH_EFFECTIVEFLANGEWIDTH       418

#define IDH_DIALOG_BRIDGEANALYSISREPORT 419

#define IDH_EDIT_HAUNCH                420


#define IDH_GIRDERDETAILS_GENERAL   2001
#define IDH_GIRDERDETAILS_STRANDS 2002
#define IDH_GIRDERDETAILS_DEBOND    2003
#define IDH_GIRDERDETAILS_TRANSV_REBAR 2004
#define IDH_GIRDERDETAILS_LONGIT_REBAR     2005
#define IDH_GIRDERDETAILS_SUPPORTS   2006

#define IDH_PIERDETAILS_GENERAL         2100
#define IDH_PIERDETAILS_CONNECTIONS     2101
#define IDH_PIERDETAILS_GIRDERSPACING   2102
#define IDH_PIERDETAILS_LAYOUT          2103

#define IDH_SPANDETAILS_GENERAL			2110
#define IDH_SPANDETAILS_CONNECTIONS		2111
#define IDH_SPANDETAILS_GIRDERSPACING	2112

#define IDH_SETTINGS_USERINFO      3000
#define IDH_SETTINGS_FILELOCATIONS 3001

#define IDH_BRIDGEVIEW_PLAN      3500
#define IDH_BRIDGEVIEW_SECTION   3501
#define IDH_BRIDGEVIEW_ALIGNMENT 3502
#define IDH_BRIDGEVIEW_PROFILE   3503

#define IDH_GIRDERVIEW_SECTION   3600
#define IDH_GIRDERVIEW_ELEV      3601

#define IDH_CONFIGURE_PGSUPER	 3699

// library editor dialogs
#define IDH_PGSUPER_LIBRARY_DIALOGS                               3701
#define IDH_CONCRETE_ENTRY_DIALOG                                 3702
#define IDH_GIRDER_CONNECTION_DIALOG                              3703
#define IDH_GIRDER_TEMPLATE_EDITING_DIALOG                        3704
#define IDH_GIRDER_DIMENSIONS                                 3705
#define IDH_GIRDER_TEMPORARY_STRANDS                                 3706
#define IDH_GIRDER_PERMANENT_STRANDS                                    3707
#define IDH_GIRDER_LONGITUDINAL_REINFORCEMENT                        3708
#define IDH_GIRDER_TRANSVERSE_REINFORCEMENT                          3709
#define IDH_GIRDER_HARPING_POINTS                                    3710
#define IDH_GIRDER_DIAPHRAGMS                               3711
#define IDH_TRAFFIC_BARRIER_DIALOG                                3712
#define IDH_SPECIFICATION_ENTRY_DIALOG                            3714
#define IDH_PROJECT_CRITERIA_GENERAL                         3715
#define IDH_CASTING_YARD_TAB                                      3716
#define IDH_PROJECT_CRITERIA_HAULING                              3717
#define IDH_PROJECT_CRITERIA_LOADS                                       3718
#define IDH_LATERAL_STABILITY_OF_LONG_PRESTRESSED_CONCRETE_BEAMS  3719
#define IDH_PROJECT_CRITERIA_LIFTING                                          3720
#define IDH_PROJECT_CRITERIA_LOSSES                                           3721
#define IDH_PROJECT_CRITERIA_CREEP                                            3722
#define IDH_PROJECT_CRITERIA_PRESTRESSING                                           3723
#define IDH_BRIDGESITE_1_TAB                                      3724
#define IDH_PROJECT_CRITERIA_PRESTRESS_ELEMENTS                                      3725
#define IDH_COPY_CONCRETE                                         3726
#define IDH_EDIT_LOADS                                            3727
#define IDH_EDIT_POINT_LOADS                                      3728
#define IDH_EDIT_DISTRIBUTED_LOADS                                3729
#define IDH_DISTRIBUTION_FACTORS                                  3730
#define IDH_PROJECT_CRITERIA_DESIGN                                           3732
#define IDH_PROJECT_CRITERIA_LIMITS                                           3733
#define IDH_PROJECT_CRITERIA_SHEAR_CAPACITY                                             3735
#define IDH_PROJECT_CRITERIA_MOMENT_CAPACITY                                            3736
#define IDH_DEFLECTIONS_TAB                                       3737
#define IDH_DEBONDING_TAB                                         3738
#define IDH_EDIT_MOMENT_LOADS                                     3739
#define IDH_GIRDER_FLEXURAL_DESIGN                                3740
#define IDH_GIRDER_SHEAR_DESIGN                                     3741
#define IDH_EFFECTIVE_FLANGE_WIDTH                                3742
#define IDH_GIRDER_HAUNCH_AND_CAMBER                                     3743
#define IDH_PROJECT_CRITERIA_CLOSURE_JOINTS                       3744

#define IDH_SELECT_LIVELOAD			  	          3790

#define IDH_CONCRETE_DETAILS                                      3791
#define IDH_STRUCTURAL_ANALYSIS                                   3792
#define IDH_GIRDER_CONNECTION_ERROR                               3793

#define IDH_RATING_GENERAL_TAB                                    3800
#define IDH_RATING_DESIGN_TAB                                     3801
#define IDH_RATING_LEGAL_TAB                                      3802
#define IDH_RATING_PERMIT_TAB                                     3803

#define IDH_PLUGINS                                               3810

#define IDH_GENERATE_STRANDS                                      3900
#define IDH_STRAND_LOCATION                                       3901

#define IDH_LOAD_RATING_CRITERIA                                  3910
#define IDH_LOAD_RATING_LIVE_LOAD_FACTORS                                     3911

#define IDH_CONSTRUCTION_LOADS                                    3920
#define IDH_FILL_DISTRIBUTION_FACTORS                             3921
#define IDH_GIRDER_DIRECT_STRAND_FILL                             3922
#define IDH_GIRDER_DIRECST_STRAND_INPUT                           3923

#define IDH_CUSTOM_REPORT                                         3950
#define IDH_FAVORITE_REPORT                                       3951

#define IDH_DIAPHRAGM_LAYOUT_RULES     3999

#define IDH_CONCRETE_GENERAL           4000
#define IDH_CONCRETE_AASHTO            4001
#define IDH_CONCRETE_ACI               4002
#define IDH_CONCRETE_CEBFIP            4003

