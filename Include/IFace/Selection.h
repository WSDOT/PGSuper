///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-2004
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#include <WbflTypes.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   ISelection

   Interface for interrogating the UI for the current selection

DESCRIPTION
   Interface for interrogating the UI for the current selection
*****************************************************************************/
// {A37216C6-E800-4ac9-899D-2518407E081C}
DEFINE_GUID(IID_ISelection, 
0xa37216c6, 0xe800, 0x4ac9, 0x89, 0x9d, 0x25, 0x18, 0x40, 0x7e, 0x8, 0x1c);
interface ISelection : IUnknown
{
   virtual PierIndexType GetPierIdx() = 0;
   virtual SpanIndexType GetSpanIdx() = 0;
   virtual GirderIndexType GetGirderIdx() = 0;
   virtual void SelectPier(PierIndexType pierIdx) = 0;
   virtual void SelectSpan(SpanIndexType spanIdx) = 0;
   virtual void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetSectionCutStation() = 0; // bridge model view section cut station
};


struct CSelection
{
public:
   enum Type { None, Pier, Span, Girder, Deck, Alignment } Type;
   SpanIndexType SpanIdx;
   PierIndexType PierIdx;
   GirderIndexType GirderIdx;
};


/*****************************************************************************
INTERFACE
   ISelectionEx

   Interface for interrogating the UI for the current selection

DESCRIPTION
   Interface for interrogating the UI for the current selection.

   ISelection is obsolute, use ISelectionEx
*****************************************************************************/
// {7CDC51A1-57DF-43bc-A828-5933AC369766}
DEFINE_GUID(IID_ISelectionEx, 
0x7cdc51a1, 0x57df, 0x43bc, 0xa8, 0x28, 0x59, 0x33, 0xac, 0x36, 0x97, 0x66);
interface ISelectionEx : ISelection
{
   // returns information about the current selection
   virtual CSelection GetSelection() = 0;

   virtual void SelectDeck() = 0;
   virtual void SelectAlignment() = 0;
   virtual void ClearSelection() = 0;
};
