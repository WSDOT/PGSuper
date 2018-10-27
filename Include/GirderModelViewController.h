///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// {D4B9D328-92A5-43A0-BEEB-D5F498835A23}
DEFINE_GUID(IID_IGirderModelViewController ,
   0xd4b9d328, 0x92a5, 0x43a0, 0xbe, 0xeb, 0xd5, 0xf4, 0x98, 0x83, 0x5a, 0x23);
struct __declspec(uuid("{D4B9D328-92A5-43A0-BEEB-D5F498835A23}")) IGirderModelViewController;

interface IGirderModelViewController : IEAFViewController
{
   virtual bool SyncWithBridgeModelView() const = 0;
   virtual void SyncWithBridgeModelView(bool bSync) = 0;

   virtual void SelectGirder(const CGirderKey& girderKey) = 0;
   virtual const CGirderKey& GetGirder() const = 0;

   virtual EventIndexType GetEvent() const = 0;
   virtual bool SetEvent(EventIndexType eventIdx) = 0;

   // section cut
   virtual Float64 GetCutLocation() const = 0;
   virtual void CutAt(Float64 Xg) = 0;
   virtual void CutAtNext() = 0;
   virtual void CutAtPrev() = 0;
   virtual Float64 GetMinCutLocation() const = 0;
   virtual Float64 GetMaxCutLocation() const = 0;

   // View Settings
   virtual void ShowStrands(bool bShow) = 0;
   virtual bool ShowStrands() const = 0;

   virtual void ShowStrandCG(bool bShow) = 0;
   virtual bool ShowStrandCG() const = 0;

   virtual void ShowCG(bool bShow) = 0;
   virtual bool ShowCG() const = 0;

   virtual void ShowSectionProperties(bool bShow) = 0;
   virtual bool ShowSectionProperties() const = 0;

   virtual void ShowDimensions(bool bShow) = 0;
   virtual bool ShowDimensions() const = 0;

   virtual void ShowLongitudinalReinforcement(bool bShow) = 0;
   virtual bool ShowLongitudinalReinforcement() const = 0;

   virtual void ShowTransverseReinforcement(bool bShow) = 0;
   virtual bool ShowTransverseReinforcement() const = 0;

   virtual void ShowLoads(bool bShow) = 0;
   virtual bool ShowLoads() const = 0;

   virtual void Schematic(bool bSchematic) = 0;
   virtual bool Schematic() const = 0;
};
