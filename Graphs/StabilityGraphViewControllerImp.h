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

#include <Graphs\StabilityGraphViewController.h>
#include "StabilityGraphController.h"

// {E1785DBC-E140-43FE-9D35-6E7C89DC3FA4}
DEFINE_GUID(CLSID_StabilityGraphViewController,
   0xe1785dbc, 0xe140, 0x43fe, 0x9d, 0x35, 0x6e, 0x7c, 0x89, 0xdc, 0x3f, 0xa4);


class ATL_NO_VTABLE CStabilityGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CStabilityGraphViewController, &CLSID_StabilityGraphViewController>,
   public IStabilityGraphViewController
{
public:
   CStabilityGraphViewController();
   virtual ~CStabilityGraphViewController();

   void Init(CStabilityGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CStabilityGraphViewController)
      COM_INTERFACE_ENTRY(IStabilityGraphViewController)
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

   // IStabilityGraphViewController
public:
   virtual void SelectSegment(const CSegmentKey& segmentKey) override;
   virtual const CSegmentKey& GetSegment() const override;

   virtual void SetViewMode(IStabilityGraphViewController::ViewMode mode) override;
   virtual IStabilityGraphViewController::ViewMode GetViewMode() const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

private:
   CStabilityGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
