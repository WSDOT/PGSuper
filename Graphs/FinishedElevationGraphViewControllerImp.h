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

#include <Graphs\FinishedElevationGraphViewController.h>
#include "FinishedElevationGraphController.h"

// {808FF666-2A87-494B-82D7-584EC11376DD}
DEFINE_GUID(CLSID_FinishedElevationGraphViewController,
   0x808ff666,0x2a87,0x494b,0x82,0xd7,0x58,0x4e,0xc1,0x13,0x76,0xdd);

class ATL_NO_VTABLE CFinishedElevationGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CFinishedElevationGraphViewController, &CLSID_FinishedElevationGraphViewController>,
   public IFinishedElevationGraphViewController
{
public:
   CFinishedElevationGraphViewController();
   virtual ~CFinishedElevationGraphViewController();

   void Init(CFinishedElevationGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CFinishedElevationGraphViewController)
      COM_INTERFACE_ENTRY(IFinishedElevationGraphViewController)
      COM_INTERFACE_ENTRY(IEAFViewController)
   END_COM_MAP()

   // IEAFViewController
public:
   virtual bool IsOpen() const override;

   // Closes the view
   virtual void Close() override;

   // implement later....
   //virtual void Move(INT x, INT y) = 0;
   //virtual void Size(INT x, INT y) = 0;
   //virtual void GetSize() = 0;
   //virtual void GetPosition() = 0;
   //virtual void GetState() = 0;
   virtual void Minimize() override;
   virtual void Maximize() override;
   virtual void Restore() override;

   // IFinishedElevationGraphViewController
public:
   virtual void GetIntervalRange(IntervalIndexType* pMin, IntervalIndexType* pMax) const override;
   virtual void SelectInterval(IntervalIndexType intervalIdx) override;
   virtual void SelectIntervals(const std::vector<IntervalIndexType>& vIntervals) override;
   virtual std::vector<IntervalIndexType> GetSelectedIntervals() const override;

   virtual void SelectGirder(const CGirderKey& girderKey) override;
   virtual const CGirderKey& GetGirder() const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

   virtual void ShowGirder(bool bShow) override;
   virtual bool ShowGirder() const override;

private:
   CFinishedElevationGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
