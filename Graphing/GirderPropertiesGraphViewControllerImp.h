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

#include <Graphing\GirderPropertiesGraphViewController.h>
#include "GirderPropertiesGraphController.h"

// {739F92BF-2651-4AE8-B892-906AFD60173E}
DEFINE_GUID(CLSID_GirderPropertiesGraphViewController ,
   0x739f92bf, 0x2651, 0x4ae8, 0xb8, 0x92, 0x90, 0x6a, 0xfd, 0x60, 0x17, 0x3e);

class ATL_NO_VTABLE CGirderPropertiesGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CGirderPropertiesGraphViewController, &CLSID_GirderPropertiesGraphViewController>,
   public IGirderPropertiesGraphViewController
{
public:
   CGirderPropertiesGraphViewController();
   virtual ~CGirderPropertiesGraphViewController();

   void Init(CGirderPropertiesGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CGirderPropertiesGraphViewController)
      COM_INTERFACE_ENTRY(IGirderPropertiesGraphViewController)
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

   // IGirderPropertiesGraphViewController
public:
   virtual bool SetPropertyType(CGirderPropertiesGraphBuilder::PropertyType propertyType) override;
   virtual CGirderPropertiesGraphBuilder::PropertyType GetPropertyType() const override;
   virtual bool IsInvariantProperty(CGirderPropertiesGraphBuilder::PropertyType propertyType) const override;

   virtual bool SetSectionPropertyType(pgsTypes::SectionPropertyType type) override;
   virtual pgsTypes::SectionPropertyType GetSectionPropertyType() const override;

   virtual void SelectGirder(const CGirderKey& girderKey) override;
   virtual const CGirderKey& GetGirder() const override;

   virtual void SetInterval(IntervalIndexType intervalIdx) override;
   virtual IntervalIndexType GetInterval() const override;
   virtual IntervalIndexType GetFirstInterval() const override;
   virtual IntervalIndexType GetLastInterval() const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

   virtual void ShowGirder(bool bShow) override;
   virtual bool ShowGirder() const override;

private:
   CGirderPropertiesGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
