///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_BRIDGEEDITORSETTINGS_H_
#define INCLUDED_BRIDGEEDITORSETTINGS_H_

// Bridge Model Editor
//================================================
// all possible settings for bridge model editor;
//       plan view
#define IDB_PV_LABEL_PIERS      ((DWORD)0x00000001)
#define IDB_PV_LABEL_ALIGNMENT  ((DWORD)0x00000002)
#define IDB_PV_LABEL_GIRDERS    ((DWORD)0x00000004)
#define IDB_PV_LABEL_TICKMARKS  ((DWORD)0x00000008)
#define IDB_PV_SHOW_TICKMARKS   ((DWORD)0x00000010)
#define IDB_PV_DRAW_ISOTROPIC   ((DWORD)0x00000020)
#define IDB_PV_DRAW_TO_SCALE    ((DWORD)0x00000040)
#define IDB_PV_NORTH_UP         ((DWORD)0x00000080)
#define IDB_CS_LABEL_GIRDERS    ((DWORD)0x00000100)
#define IDB_CS_SHOW_DIMENSIONS  ((DWORD)0x00000200)
#define IDB_CS_DRAW_ISOTROPIC   ((DWORD)0x00000400)
#define IDB_CS_DRAW_TO_SCALE    ((DWORD)0x00000800)
#define IDB_PV_LABEL_BEARINGS   ((DWORD)0x00001000)
#define IDB_CS_DRAW_RW_CS       ((DWORD)0x00002000)

// #define                   ((DWORD)0x00001000)


// Alignment/Profile
// ======================================
#define IDA_AP_NORTH_UP         ((DWORD)0x00000001)
#define IDA_AP_DRAW_BRIDGE      ((DWORD)0x00000002) // draw bridge on alignment
#define IDP_AP_DRAW_BRIDGE      ((DWORD)0x00000004) // draw bridge on profile
#define IDP_AP_DRAW_ISOTROPIC   ((DWORD)0x00000008) // draw isotroipic or anisotripc

// Girder Model Editor
// ======================================
// all possible settings for girder model editor. don't change these values... they are stored
// in the registry. each time the view is opened, the settings are restored from the registry.
// changing their values would change the settings themselves
//       section view
#define IDG_SV_SHOW_STRANDS     ((DWORD)0x00000001)
#define IDG_SV_SHOW_PS_CG       ((DWORD)0x00000002)
#define IDG_SV_SHOW_DIMENSIONS  ((DWORD)0x00000004)
#define IDG_SV_DRAW_ISOTROPIC   ((DWORD)0x00000010)
#define IDG_SV_DRAW_TO_SCALE    ((DWORD)0x00000020)
#define IDG_SV_SHOW_LONG_REINF  ((DWORD)0x00000040)
#define IDG_SV_SYNC_GIRDER      ((DWORD)0x00000080)

// elevation view
#define IDG_EV_SHOW_STRANDS     ((DWORD)0x00000100)
#define IDG_EV_SHOW_PS_CG       ((DWORD)0x00000200)
#define IDG_EV_SHOW_DIMENSIONS  ((DWORD)0x00000400)
#define IDG_EV_DRAW_ISOTROPIC   ((DWORD)0x00000800)
#define IDG_EV_DRAW_TO_SCALE    ((DWORD)0x00001000)
#define IDG_EV_SHOW_STIRRUPS    ((DWORD)0x00002000)
#define IDG_EV_SHOW_LONG_REINF  ((DWORD)0x00004000)
#define IDG_EV_SHOW_LOADS       ((DWORD)0x00008000)
#define IDG_EV_SHOW_LEGEND      ((DWORD)0x00010000)

#define IDG_SV_GIRDER_CG        ((DWORD)0x00020000)
#define IDG_EV_GIRDER_CG        ((DWORD)0x00040000)
#define IDG_SV_PROPERTIES       ((DWORD)0x00080000)

#endif