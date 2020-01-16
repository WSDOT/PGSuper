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

#include <Graphing\EffectivePrestressGraphViewController.h>
#include "EffectivePrestressGraphController.h"

// {93AE9A2A-8266-4AB3-A542-F0ADE9ABDBCD}
DEFINE_GUID(CLSID_EffectivePrestressGraphViewController,
   0x93ae9a2a, 0x8266, 0x4ab3, 0xa5, 0x42, 0xf0, 0xad, 0xe9, 0xab, 0xdb, 0xcd);


class ATL_NO_VTABLE CEffectivePrestressGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CEffectivePrestressGraphViewController, &CLSID_EffectivePrestressGraphViewController>,
   public IEffectivePrestressGraphViewController
{
public:
   CEffectivePrestressGraphViewController();
   virtual ~CEffectivePrestressGraphViewController();

   void Init(CEffectivePrestressGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CEffectivePrestressGraphViewController)
      COM_INTERFACE_ENTRY(IEffectivePrestressGraphViewController)
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

   // IEffectivePrestressGraphViewController
public:
   virtual void GetIntervalRange(IntervalIndexType* pMin, IntervalIndexType* pMax) const override;
   virtual void SelectInterval(IntervalIndexType intervalIdx) override;
   virtual void SelectIntervals(const std::vector<IntervalIndexType>& vIntervals) override;
   virtual std::vector<IntervalIndexType> GetSelectedIntervals() const override;

   virtual void SelectGirder(const CGirderKey& girderKey) override;
   virtual const CGirderKey& GetGirder() const override;

   virtual void SetViewMode(IEffectivePrestressGraphViewController::ViewMode mode) override;
   virtual IEffectivePrestressGraphViewController::ViewMode GetViewMode() const override;

   virtual void SetStrandType(IEffectivePrestressGraphViewController::StrandType strandType) override;
   virtual IEffectivePrestressGraphViewController::StrandType GetStrandType() const override;

   virtual void SetDuct(IEffectivePrestressGraphViewController::DuctType ductType,DuctIndexType ductIdx) override;
   virtual IEffectivePrestressGraphViewController::DuctType GetDuctType() const override;
   virtual DuctIndexType GetDuct() const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

   virtual void ShowGirder(bool bShow) override;
   virtual bool ShowGirder() const override;

private:
   CEffectivePrestressGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
