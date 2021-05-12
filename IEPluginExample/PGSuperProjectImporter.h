///////////////////////////////////////////////////////////////////////
// IEPluginExample
// Copyright © 1999-2021  Washington State Department of Transportation
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

// PGSuperProjectImporter.h : Declaration of the CPGSuperProjectImporter

#pragma once

#include <Plugins\PGSuperIEPlugin.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPGSuperProjectImporter
class ATL_NO_VTABLE CPGSuperProjectImporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSuperProjectImporter, &CLSID_PGSuperProjectImporter>,
   public IPGSProjectImporter
{
public:
	CPGSuperProjectImporter()
	{
	}

   HRESULT FinalConstruct();

   CBitmap m_Bitmap;

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERPROJECTIMPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPGSuperProjectImporter)
	COM_INTERFACE_ENTRY(IPGSProjectImporter)
END_COM_MAP()

// IPGSProjectImporter
public:
   STDMETHOD(GetItemText)(/*[out,retval]*/BSTR*  bstrText) const override;
   STDMETHOD(GetIcon)(/*[out]*/HICON* phIcon) const override;
   STDMETHOD(Import)(/*[in]*/IBroker* pBroker) override;

private:
   void BuildBridge(IBroker* pBroker);
   void SetSpecification(IBroker* pBroker);
   void InitGirderData(IBroker* pBroker);
};
