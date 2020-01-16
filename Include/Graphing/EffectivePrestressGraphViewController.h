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

// {DAAC0486-0E9B-49B5-A41E-E43FF097531B}
DEFINE_GUID(IID_IEffectivePrestressGraphViewController,
   0xdaac0486, 0xe9b, 0x49b5, 0xa4, 0x1e, 0xe4, 0x3f, 0xf0, 0x97, 0x53, 0x1b);
struct __declspec(uuid("{DAAC0486-0E9B-49B5-A41E-E43FF097531B}")) IEffectivePrestressGraphViewController;

interface IEffectivePrestressGraphViewController : IEAFViewController
{
   enum ViewMode { Stress, Force};
   enum StrandType {Permanent, Temporary};
   enum DuctType { Segment, Girder };

   virtual void GetIntervalRange(IntervalIndexType* pMin, IntervalIndexType* pMax) const = 0;

   virtual void SelectInterval(IntervalIndexType intervalIdx) = 0;
   virtual void SelectIntervals(const std::vector<IntervalIndexType>& vIntervals) = 0;
   virtual std::vector<IntervalIndexType> GetSelectedIntervals() const = 0;

   virtual void SelectGirder(const CGirderKey& girderKey) = 0;
   virtual const CGirderKey& GetGirder() const = 0;

   virtual void SetViewMode(ViewMode mode) = 0;
   virtual ViewMode GetViewMode() const = 0;

   virtual void SetStrandType(StrandType strandType) = 0;
   virtual StrandType GetStrandType() const = 0;

   // set the duct index to INVALID_INDEX to show effective prestress of pretensioned strands
   virtual void SetDuct(DuctType ductType,DuctIndexType ductIdx) = 0;
   virtual DuctType GetDuctType() const = 0;
   virtual DuctIndexType GetDuct() const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;

   virtual void ShowGirder(bool bShow) = 0;
   virtual bool ShowGirder() const = 0;
};
