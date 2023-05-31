///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// {63F97992-2A1F-4DFF-B007-08BF9162BA2F}
DEFINE_GUID(IID_IFinishedElevationGraphViewController,
   0x63f97992,0x2a1f,0x4dff,0xb0,0x7,0x8,0xbf,0x91,0x62,0xba,0x2f);
struct __declspec(uuid("{63F97992-2A1F-4DFF-B007-08BF9162BA2F}")) IFinishedElevationGraphViewController;

interface IFinishedElevationGraphViewController : IEAFViewController
{
   virtual void GetIntervalRange(IntervalIndexType* pMin, IntervalIndexType* pMax) const = 0;

   virtual void SelectInterval(IntervalIndexType intervalIdx) = 0;
   virtual void SelectIntervals(const std::vector<IntervalIndexType>& vIntervals) = 0;
   virtual std::vector<IntervalIndexType> GetSelectedIntervals() const = 0;

   virtual void SelectGirder(const CGirderKey& girderKey) = 0;
   virtual const CGirderKey& GetGirder() const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;

   virtual void ShowGirder(bool bShow) = 0;
   virtual bool ShowGirder() const = 0;
};
