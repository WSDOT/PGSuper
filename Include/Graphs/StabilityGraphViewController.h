///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// {1A3AE4C5-98BC-4323-A4F7-9CDA6CFD9BF6}
DEFINE_GUID(IID_IStabilityGraphViewController ,
   0x1a3ae4c5, 0x98bc, 0x4323, 0xa4, 0xf7, 0x9c, 0xda, 0x6c, 0xfd, 0x9b, 0xf6);
struct __declspec(uuid("{1A3AE4C5-98BC-4323-A4F7-9CDA6CFD9BF6}")) IStabilityGraphViewController;

interface IStabilityGraphViewController : IEAFViewController
{
   enum ViewMode { Lifting, Hauling };
   virtual void SelectSegment(const CSegmentKey& segmentKey) = 0;
   virtual const CSegmentKey& GetSegment() const = 0;

   virtual void SetViewMode(ViewMode mode) = 0;
   virtual ViewMode GetViewMode() const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;
};
