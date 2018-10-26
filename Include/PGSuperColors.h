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

#ifndef INCLUDED_COLORS_H_
#define INCLUDED_COLORS_H_

#include <Colors.h>

#define GIRDER_BORDER_COLOR GREY50
#define GIRDER_FILL_COLOR   GREY70

#define GIRDER_FILL_COLOR_ADJACENT   GREY86
#define GIRDER_BORDER_COLOR_ADJACENT GREY66

#define ALIGNMENT_COLOR       MEDIUMBLUE
#define CLBRIDGE_COLOR        BROWN

#define PIER_BORDER_COLOR     POWDERBLUE
#define PIER_FILL_COLOR       ALICEBLUE

#define VOID_BORDER_COLOR     GIRDER_BORDER_COLOR
#define BARRIER_BORDER_COLOR  GREY30
#define BARRIER_FILL_COLOR    GREY50
#define DECK_BORDER_COLOR     GREY40
#define DECK_FILL_COLOR       GREY60
#define STRAND_BORDER_COLOR   BLACK
#define STRAND_FILL_COLOR     BLACK
#define DEBOND_FILL_COLOR     RED
#define CUT_COLOR             MEDIUMBLUE
#define REBAR_COLOR           CYAN4
#define OVERLAY_COLOR         BLACK
#define FUTURE_OVERLAY_COLOR  LIGHTSLATEGRAY
#define STIRRUP_COLOR         SLATEGRAY2

#define DIAPHRAGM_BORDER_COLOR       GIRDER_BORDER_COLOR
#define DIAPHRAGM_FILL_COLOR         GIRDER_FILL_COLOR

#define HYPERLINK_COLOR       ::GetSysColor(COLOR_HOTLIGHT)

#define DC_COLOR   RED3
#define DW_COLOR   GREEN3
#define LLIM_COLOR GOLD3

#define SELECTED_OBJECT_LINE_COLOR RED4
#define SELECTED_OBJECT_FILL_COLOR BLUE

#endif // INCLUDED_COLORS_H_