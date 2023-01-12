///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "GirderModelViewController.h"
#include "GirderModelChildFrame.h"

// {1964372A-C399-4C11-B834-65B49B23C6AE}
DEFINE_GUID(CLSID_GirderModelViewController ,
   0x1964372a, 0xc399, 0x4c11, 0xb8, 0x34, 0x65, 0xb4, 0x9b, 0x23, 0xc6, 0xae);


class ATL_NO_VTABLE CGirderModelViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CGirderModelViewController, &CLSID_GirderModelViewController>,
   public IGirderModelViewController
{
public:
   CGirderModelViewController();
   virtual ~CGirderModelViewController();

   void Init(CGirderModelChildFrame* pFrame, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CGirderModelViewController)
      COM_INTERFACE_ENTRY(IGirderModelViewController)
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

   // IGirderModelViewController
public:
   virtual void SelectGirder(const CGirderKey& girderKey) override;
   virtual const CGirderKey& GetGirder() const override;

   virtual bool SyncWithBridgeModelView() const override;
   virtual void SyncWithBridgeModelView(bool bSync) override;

   virtual EventIndexType GetEvent() const override;
   virtual bool SetEvent(EventIndexType eventIdx) override;

   virtual Float64 GetCutLocation() const override;
   virtual void CutAt(Float64 Xg) override;
   virtual void CutAtNext() override;
   virtual void CutAtPrev() override;
   virtual void GetCutRange(Float64* pMin,Float64* pMax) const override;

   virtual void ShowStrands(bool bShow) override;
   virtual bool ShowStrands() const override;

   virtual void ShowStrandCG(bool bShow) override;
   virtual bool ShowStrandCG() const override;

   virtual void ShowCG(bool bShow) override;
   virtual bool ShowCG() const override;

   virtual void ShowSectionProperties(bool bShow) override;
   virtual bool ShowSectionProperties() const override;

   virtual void ShowDimensions(bool bShow) override;
   virtual bool ShowDimensions() const override;

   virtual void ShowLongitudinalReinforcement(bool bShow) override;
   virtual bool ShowLongitudinalReinforcement() const override;

   virtual void ShowTransverseReinforcement(bool bShow) override;
   virtual bool ShowTransverseReinforcement() const override;

   virtual void ShowLoads(bool bShow) override;
   virtual bool ShowLoads() const override;

   virtual void Schematic(bool bSchematic) override;
   virtual bool Schematic() const override;

private:
   CGirderModelChildFrame* m_pFrame;
   CComPtr<IEAFViewController> m_pStdController;
};

