///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_COLORS_H_
#define INCLUDED_COLORS_H_

#include <Colors.h>

#define SEGMENT_BORDER_COLOR GREY50
#define SEGMENT_FILL_COLOR   GREY70
#define SEGMENT_FILL_GHOST_COLOR GREY90 // used for girder segments that have not yet been erected

#define SEGMENT_BORDER_COLOR_ADJACENT GREY66
#define SEGMENT_FILL_COLOR_ADJACENT   GREY86

#define CLOSURE_BORDER_COLOR GREY38
#define CLOSURE_FILL_COLOR   GREY48

#define JOINT_BORDER_COLOR GREY48
#define JOINT_FILL_COLOR   GREY48

#define JOINT_BORDER_GHOST_COLOR GREY90
#define JOINT_FILL_GHOST_COLOR GREY90

#define JOINT_BORDER_COLOR_ADJACENT GREY86
#define JOINT_FILL_COLOR_ADJACENT   GREY86

#define STRAND_CG_COLOR_1 RED
#define STRAND_CG_COLOR_2 GREY78

#define SECTION_CG_COLOR_1 BLUE
#define SECTION_CG_COLOR_2 GREY78

#define ALIGNMENT_COLOR       MEDIUMBLUE
#define PROFILE_COLOR         ALIGNMENT_COLOR
#define BRIDGE_COLOR          BROWN
#define ALIGNMENT_LINE_WEIGHT 2
#define PROFILE_LINE_WEIGHT ALIGNMENT_LINE_WEIGHT
#define BRIDGELINE_LINE_WEIGHT 1
#define BRIDGE_LINE_WEIGHT  4

#define PIER_BORDER_COLOR     SADDLEBROWN
#define PIER_FILL_COLOR       SIENNA
#define CONNECTION_LABEL_COLOR SIENNA

#define TS_BORDER_COLOR     DARKGREEN
#define TS_FILL_COLOR       GREEN
#define TS_FILL_GHOST_COLOR LIGHTGREEN
#define TS_LABEL_COLOR      DARKGREEN

#define SB_BORDER_COLOR     ORANGERED
#define SB_BORDER_GHOST_COLOR     INDIANRED
#define SB_FILL_COLOR       ORANGERED
#define SB_FILL_GHOST_COLOR INDIANRED
#define SB_LABEL_COLOR      ORANGERED

#define VOID_BORDER_COLOR     SEGMENT_BORDER_COLOR
#define BARRIER_BORDER_COLOR  GREY30
#define BARRIER_FILL_COLOR    GREY50
#define DECK_BORDER_COLOR     GREY40
#define DECK_FILL_COLOR       GREY60
#define DECK_FILL_POS_MOMENT_REGION_COLOR GREY60
#define DECK_FILL_NEG_MOMENT_REGION_COLOR GREY65
#define NONSTRUCTURAL_DECK_BORDER_COLOR     GREY45
#define NONSTRUCTURAL_DECK_FILL_COLOR       GREY65
#define STRAND_BORDER_COLOR   BLACK
#define STRAND_FILL_COLOR     BLACK
#define STRAIGHT_FILL_COLOR   BLACK
#define DUCT_LINE_COLOR1      GREY30
#define DUCT_LINE_COLOR2      GREY60
#define TENDON_LINE_COLOR     DARKGREEN
#define TENDON_BORDER_COLOR   DECK_BORDER_COLOR
#define TENDON_FILL_COLOR     DECK_FILL_COLOR
#define DEBOND_FILL_COLOR     RED
#define HARPED_FILL_COLOR     GREEN4
#define HARPED_FILL_COLOR_NAME _T("Green")
#define TEMPORARY_FILL_COLOR  MEDIUMBLUE
#define EXTENDED_FILL_COLOR   DARKGOLDENROD
#define CUT_COLOR             MEDIUMBLUE
#define REBAR_COLOR           CYAN4
#define OVERLAY_COLOR         BLACK
#define FUTURE_OVERLAY_COLOR  LIGHTSLATEGRAY
#define STIRRUP_COLOR         SLATEGRAY2

#define DIAPHRAGM_BORDER_COLOR       SEGMENT_BORDER_COLOR
#define DIAPHRAGM_FILL_COLOR         SEGMENT_FILL_COLOR

#define HYPERLINK_COLOR       ::GetSysColor(COLOR_HOTLIGHT)

#define DC_COLOR   RED3
#define DW_COLOR   GREEN3
#define LLIM_COLOR GOLD3

#define SELECTED_OBJECT_LINE_COLOR RED4
#define SELECTED_OBJECT_FILL_COLOR BLUE

// background color for metafiles that are displayed in dialogs
#define METAFILE_BACKGROUND_COLOR WHITE

#define NOT_DEBONDABLE_FILL_COLOR GREY90
#define NOT_DEBONDABLE_FILL_COLOR_NAME _T("light grey")

#endif // INCLUDED_COLORS_H_