///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <EAF\EAFViewController.h>

// {4BC86416-257D-41E8-B4EC-A2BD3AD33050}
DEFINE_GUID(IID_IBridgeModelViewController,
0x4bc86416, 0x257d, 0x41e8, 0xb4, 0xec, 0xa2, 0xbd, 0x3a, 0xd3, 0x30, 0x50);
struct __declspec(uuid("{4BC86416-257D-41E8-B4EC-A2BD3AD33050}")) IBridgeModelViewController;

interface IBridgeModelViewController : IEAFViewController
{
   enum ViewMode { Bridge, Alignment };
   virtual void GetGroupRange(GroupIndexType* pStartGroupIdx, GroupIndexType* pEndGroupIdx) const = 0;
   virtual void SetGroupRange(GroupIndexType startGroupIdx, GroupIndexType endGroupIdx) = 0;
   virtual Float64 GetCutStation() const = 0;
   virtual void SetCutStation(Float64 station) = 0;
   virtual void SetViewMode(ViewMode mode) = 0;
   virtual ViewMode GetViewMode() const = 0;

   virtual void NorthUp(bool bNorthUp) = 0;
   virtual bool NorthUp() const = 0;
   virtual void ShowLabels(bool bShowLabels) = 0;
   virtual bool ShowLabels() const = 0;
   virtual void ShowDimensions(bool bShowDimensions) = 0;
   virtual bool ShowDimensions() const = 0;
   virtual void ShowBridge(bool bShowBridge) = 0;
   virtual bool ShowBridge() const = 0;
   virtual void Schematic(bool bSchematic) = 0;
   virtual bool Schematic() const = 0;
};
