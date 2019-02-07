///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <Graphing\DeflectionHistoryGraphViewController.h>
#include "DeflectionHistoryGraphController.h"

// {0059152C-86FD-4533-815B-6DC1AEF8E42A}
DEFINE_GUID(CLSID_DeflectionHistoryGraphViewController,
   0x59152c, 0x86fd, 0x4533, 0x81, 0x5b, 0x6d, 0xc1, 0xae, 0xf8, 0xe4, 0x2a);

class ATL_NO_VTABLE CDeflectionHistoryGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CDeflectionHistoryGraphViewController, &CLSID_DeflectionHistoryGraphViewController>,
   public IDeflectionHistoryGraphViewController
{
public:
   CDeflectionHistoryGraphViewController();
   virtual ~CDeflectionHistoryGraphViewController();

   void Init(CDeflectionHistoryGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CDeflectionHistoryGraphViewController)
      COM_INTERFACE_ENTRY(IDeflectionHistoryGraphViewController)
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

   // IDeflectionHistoryGraphViewController
public:
   virtual void SelectLocation(const pgsPointOfInterest& poi) override;
   virtual const pgsPointOfInterest& GetLocation() const override;

   virtual void SetXAxisType(IDeflectionHistoryGraphViewController::XAxisType type) override;
   virtual IDeflectionHistoryGraphViewController::XAxisType GetXAxisType() const override;

   virtual void IncludeElevationAdjustment(bool bAdjust) override;
   virtual bool IncludeElevationAdjustment() const override;

   virtual void IncludePrecamber(bool bInclude) override;
   virtual bool IncludePrecamber() const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

private:
   CDeflectionHistoryGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
