///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#define IDH_STRUCTURAL_ANALYSIS                                   3792
#ifndef INCLUDED_HELPTOPICS_HH_
#define INCLUDED_HELPTOPICS_HH_


#define IDH_DIALOG_TOOLBARS       100
#define IDH_DIALOG_REPORT         101
#define IDH_DIALOG_PROPERTIES     102
#define IDH_DIALOG_COPYGDRPROPERTIES 103
#define IDH_DIALOG_DESIGNGIRDER   104
#define IDH_DIALOG_DESIGNCOMPLETE 105
#define IDH_DIALOG_UNITS          106
#define IDH_DIALOG_DESIGNCRITERIA 107
#define IDH_DIALOG_ENVIRONMENT    108
#define IDH_DIALOG_LIBENTRYCONFLICT 109
#define IDH_DIALOG_SECTIONCUT     110
#define IDH_DIALOG_LOADMODIFIERS  111
#define IDH_DIALOG_LIBIMPORTENTRYCONFLICT 112
#define IDH_DIALOG_DESIGNDETAILS   113
#define IDH_LIVELOAD_DIALOG			114
#define IDH_DECK_CONDITION         115
#define IDH_GIRDER_CONDITION       116
#define IDH_MULTIGIRDER_REPORT         117

#define IDH_EFFECTIVEFLANGEWIDTH 118


#define IDH_DIALOG_BRIDGEANALYSISREPORT 119

#define IDH_ALIGNMENT_HORIZONTAL		1000
#define IDH_ALIGNMENT_PROFILE			1001
#define IDH_ALIGNMENT_SUPERELEVATION	1002

#define IDH_BRIDGEDESC_GENERAL      1020
#define IDH_BRIDGEDESC_FRAMING      1021
#define IDH_BRIDGEDESC_RAILING      1022
#define IDH_BRIDGEDESC_DECKDETAILS	1023
#define IDH_BRIDGEDESC_DECKREBAR	1024

#define IDH_GIRDERWIZ_SELECT    2000
#define IDH_GIRDERWIZ_GENERAL   2001
#define IDH_GIRDERWIZ_PRESTRESS 2002
#define IDH_GIRDERWIZ_DEBOND    2003
#define IDH_GIRDERWIZ_SHEARDESC 2004
#define IDH_GIRDERWIZ_REBAR     2005
#define IDH_GIRDERWIZ_LIFTING   2006

#define IDH_PIERDETAILS_GENERAL         2100
#define IDH_PIERDETAILS_CONNECTIONS     2101
#define IDH_PIERDETAILS_GIRDERSPACING   2102

#define IDH_SPANDETAILS_GENERAL			2110
#define IDH_SPANDETAILS_CONNECTIONS		2111
#define IDH_SPANDETAILS_GIRDERSPACING	2112

#define IDH_SETTINGS_USERINFO      3000
#define IDH_SETTINGS_FILELOCATIONS 3001

#define IDH_BRIDGEVIEW_SECTION   3500
#define IDH_BRIDGEVIEW_PLAN      3501

#define IDH_GIRDERVIEW_SECTION   3600
#define IDH_GIRDERVIEW_ELEV      3601

#define IDH_CONFIGURE_PGSUPER	 3699

// library editor dialogs
#define IDH_PGSUPER_LIBRARY_EDITOR                                3700

#define IDH_PGSUPER_LIBRARY_DIALOGS                               3701
#define IDH_CONCRETE_ENTRY_DIALOG                                 3702
#define IDH_GIRDER_CONNECTION_DIALOG                              3703
#define IDH_GIRDER_TEMPLATE_EDITING_DIALOG                        3704
#define IDH_GIRDER_DIMENSIONS_TAB                                 3705
#define IDH_TEMPORARY_STRANDS_TAB                                 3706
#define IDH_HARPED_STRANDS_TAB                                    3707
#define IDH_LONGITUDINAL_REINFORCEMENT_TAB                        3708
#define IDH_TRANSVERSE_REINFORCEMENT_TAB                          3709
#define IDH_HARPING_POINTS_TAB                                    3710
#define IDH_DIAPHRAGM_LAYOUT_DIALOG                               3711
#define IDH_TRAFFIC_BARRIER_DIALOG                                3712
#define IDH_SPECIFICATION_ENTRY_DIALOG                            3714
#define IDH_SPECIFICATION_DESCRIPTION_TAB                         3715
#define IDH_CASTING_YARD_TAB                                      3716
#define IDH_HAULING_AND_ERECTION_TAB                              3717
#define IDH_BRIDGE_SITE_TAB                                       3718
#define IDH_LATERAL_STABILITY_OF_LONG_PRESTRESSED_CONCRETE_BEAMS  3719
#define IDH_SPEC_LIFTING                                          3720
#define IDH_SPEC_LOSSES                                           3721
#define IDH_SPEC_CREEP                                            3722
#define IDH_SPEC_STRAND                                           3723
#define IDH_BRIDGESITE_1_TAB                                      3724
#define IDH_BRIDGESITE_2_TAB                                      3725
#define IDH_COPY_CONCRETE                                         3726
#define IDH_EDIT_LOADS                                            3727
#define IDH_EDIT_POINT_LOADS                                      3728
#define IDH_EDIT_DISTRIBUTED_LOADS                                3729
#define IDH_DISTRIBUTION_FACTORS                                  3730
#define IDH_SPEC_DESIGN                                           3732
#define IDH_SPEC_LIMITS                                           3733
#define IDH_SPEC_LOADFACTORS                                      3734
#define IDH_SHEAR_TAB                                             3735
#define IDH_MOMENT_TAB                                            3736
#define IDH_DEFLECTIONS_TAB                                       3737
#define IDH_DEBONDING_TAB                                         3738
#define IDH_EDIT_MOMENT_LOADS                                     3739
#define IDH_GIRDER_DEBOND_CRITERIA                                3740

#define IDH_BRIDGE_VIEW                                           3750
#define IDH_GIRDER_VIEW                                           3751
#define IDH_REPORT_VIEW                                           3752
#define IDH_ANALYSIS_RESULTS_VIEW                                 3753
#define IDH_STATUS_CENTER_VIEW                                    3754
#define IDH_STABILITY_VIEW                                        3755

#define IDH_SELECT_LIVELOAD									      3790

#define IDH_CONCRETE_DETAILS                                      3791
#define IDH_STRUCTURAL_ANALYSIS                                   3792
#define IDH_GIRDER_CONNECTION_ERROR                               3793

#define IDH_RATING_GENERAL_TAB                                    3800
#define IDH_RATING_DESIGN_TAB                                     3801
#define IDH_RATING_LEGAL_TAB                                      3802
#define IDH_RATING_PERMIT_TAB                                     3803

#define IDH_PLUGINS                                               3810

#define IDH_GENERATE_STRANDS                                      3900

#define IDH_LOAD_RATING_CRITERIA                                  3910
#define IDH_LIVE_LOAD_FACTORS                                     3911

#define IDH_CONSTRUCTION_LOADS                                    3920
#define IDH_FILL_DISTRIBUTION_FACTORS                             3921

#endif // INCLUDED_HELPTOPICS_HH_
