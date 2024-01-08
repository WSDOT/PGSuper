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

#pragma once

#include <WbflTypes.h>
#include <PgsExt\Keys.h>
#include <PgsExt\Selection.h>

// {A37216C6-E800-4ac9-899D-2518407E081C}
DEFINE_GUID(IID_ISelection, 
0xa37216c6, 0xe800, 0x4ac9, 0x89, 0x9d, 0x25, 0x18, 0x40, 0x7e, 0x8, 0x1c);
struct __declspec(uuid("{A37216C6-E800-4ac9-899D-2518407E081C}")) ISelection;
/// @brief Interface for interrogating and controlling the UI for the current selection in the Bridge Model View
interface ISelection : IUnknown
{
   /// @brief Returns an object with information about the current select
   virtual CSelection GetSelection() = 0;

   /// @brief Clears the current selection
   virtual void ClearSelection() = 0;

   /// @brief Returns the index of the selected pier or INVALID_INDEX
   virtual PierIndexType GetSelectedPier() = 0;

   /// @brief Returns the index of the selected span or INVALID_INDEX
   virtual SpanIndexType GetSelectedSpan() = 0;

   /// @brief Returns the index of the selected girder or INVALID_INDEX
   virtual CGirderKey GetSelectedGirder() = 0;

   /// @brief Returns the index of the selected segment or INVALID_INDEX
   virtual CSegmentKey GetSelectedSegment() = 0;

   /// @brief Returns the index of the selected closure joint or INVALID_INDEX
   virtual CClosureKey GetSelectedClosureJoint() = 0;

   /// @brief Returns the index of the selected temporary or INVALID_INDEX
   virtual SupportIDType GetSelectedTemporarySupport() = 0;

   /// @brief Returns true if the deck is selected
   virtual bool IsDeckSelected() = 0;

   /// @brief Returns true if the alignment is selected
   virtual bool IsAlignmentSelected() = 0;

   /// @brief Returns true if the specified railing system object is selected
   virtual bool IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation) = 0;

   /// @brief Select the specified pier
   virtual void SelectPier(PierIndexType pierIdx) = 0;

   /// @brief Select the specified span
   virtual void SelectSpan(SpanIndexType spanIdx) = 0;

   /// @brief Select the specified girder
   virtual void SelectGirder(const CGirderKey& girderKey) = 0;

   /// @brief Select the specified segment
   virtual void SelectSegment(const CSegmentKey& segmentKey) = 0;

   /// @brief Select the specified closure joint
   virtual void SelectClosureJoint(const CClosureKey& closureKey) = 0;

   /// @brief Select the specified temporary support
   virtual void SelectTemporarySupport(SupportIDType tsID) = 0;

   /// @brief Select the deck
   virtual void SelectDeck() = 0;

   /// @brief Select the alignment
   virtual void SelectAlignment() = 0;

   /// @brief Select the specified railing system
   virtual void SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation) = 0;

   /// @brief Returns the location of the bridge model view section cut station
   virtual Float64 GetSectionCutStation() = 0;
};
