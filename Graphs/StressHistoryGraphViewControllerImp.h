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

#include <Graphs\StressHistoryGraphViewController.h>
#include "StressHistoryGraphController.h"

// {C504AEF2-A1DA-4740-92B4-FD576EFF43AB}
DEFINE_GUID(CLSID_StressHistoryGraphViewController,
   0xc504aef2, 0xa1da, 0x4740, 0x92, 0xb4, 0xfd, 0x57, 0x6e, 0xff, 0x43, 0xab);

class ATL_NO_VTABLE CStressHistoryGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CStressHistoryGraphViewController, &CLSID_StressHistoryGraphViewController>,
   public IStressHistoryGraphViewController
{
public:
   CStressHistoryGraphViewController();
   virtual ~CStressHistoryGraphViewController();

   void Init(CStressHistoryGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CStressHistoryGraphViewController)
      COM_INTERFACE_ENTRY(IStressHistoryGraphViewController)
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

   // IStressHistoryGraphViewController
public:
   virtual void SelectLocation(const pgsPointOfInterest& poi) override;
   virtual const pgsPointOfInterest& GetLocation() const override;

   virtual void SetXAxisType(IStressHistoryGraphViewController::XAxisType type) override;
   virtual IStressHistoryGraphViewController::XAxisType GetXAxisType() const override;

   virtual void Stresses(pgsTypes::StressLocation stressLocation, bool bEnable) override;
   virtual bool Stresses(pgsTypes::StressLocation stressLocation) const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

private:
   CStressHistoryGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
