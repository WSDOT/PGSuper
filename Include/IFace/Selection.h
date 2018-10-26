///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include <WbflTypes.h>
#include <PgsExt\Keys.h>
#include <PgsExt\Selection.h>

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
   virtual CSelection GetSelection() = 0;
   virtual void ClearSelection() = 0;

   virtual PierIndexType GetSelectedPier() = 0;
   virtual SpanIndexType GetSelectedSpan() = 0;
   virtual CGirderKey GetSelectedGirder() = 0;
   virtual CSegmentKey GetSelectedSegment() = 0;
   virtual CClosureKey GetSelectedClosureJoint() = 0;
   virtual SupportIDType GetSelectedTemporarySupport() = 0;
   virtual bool IsDeckSelected() = 0;
   virtual bool IsAlignmentSelected() = 0;

   virtual void SelectPier(PierIndexType pierIdx) = 0;
   virtual void SelectSpan(SpanIndexType spanIdx) = 0;
   virtual void SelectGirder(const CGirderKey& girderKey) = 0;
   virtual void SelectSegment(const CSegmentKey& segmentKey) = 0;
   virtual void SelectClosureJoint(const CClosureKey& closureKey) = 0;
   virtual void SelectTemporarySupport(SupportIDType tsID) = 0;
   virtual void SelectDeck() = 0;
   virtual void SelectAlignment() = 0;

   virtual Float64 GetSectionCutStation() = 0; // bridge model view section cut station
};
