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

#include <Graphs\ConcretePropertiesGraphViewController.h>
#include "ConcretePropertyGraphController.h"

// {B82962D3-7E98-4018-A289-D2ABFF387C2F}
DEFINE_GUID(CLSID_ConcretePropertiesGraphViewController,
   0xb82962d3, 0x7e98, 0x4018, 0xa2, 0x89, 0xd2, 0xab, 0xff, 0x38, 0x7c, 0x2f);


class ATL_NO_VTABLE CConcretePropertiesGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CConcretePropertiesGraphViewController, &CLSID_ConcretePropertiesGraphViewController>,
   public IConcretePropertiesGraphViewController
{
public:
   CConcretePropertiesGraphViewController();
   virtual ~CConcretePropertiesGraphViewController();

   void Init(CConcretePropertyGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CConcretePropertiesGraphViewController)
      COM_INTERFACE_ENTRY(IConcretePropertiesGraphViewController)
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

   // IConcretePropertiesGraphViewController
public:
   virtual void SetGraphType(IConcretePropertiesGraphViewController::GraphType type) override;
   virtual IConcretePropertiesGraphViewController::GraphType GetGraphType() const override;
   virtual void SetElementType(IConcretePropertiesGraphViewController::ElementType type) override;
   virtual IConcretePropertiesGraphViewController::ElementType GetElementType() const override;
   virtual void SetXAxisType(IConcretePropertiesGraphViewController::XAxisType type) override;
   virtual IConcretePropertiesGraphViewController::XAxisType GetXAxisType() const override;

   virtual void SetSegment(const CSegmentKey& segmentKey) override;
   virtual const CSegmentKey& GetSegment() const override;
   virtual void SetClosureJoint(const CClosureKey& closureKey) override;
   virtual const CClosureKey& GetClosureJoint() const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

private:
   CConcretePropertyGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
