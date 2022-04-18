///////////////////////////////////////////////////////////////////////
// IEPluginExample
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

// PGSuperDataImporter.h : Declaration of the CPGSuperDataImporter

#pragma once

#include <Plugins\PGSuperIEPlugin.h>
#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CPGSuperDataImporter
class ATL_NO_VTABLE CPGSuperDataImporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSuperDataImporter, &CLSID_PGSuperDataImporter>,
   public IPGSDataImporter
{
public:
	CPGSuperDataImporter()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERDATAIMPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPGSuperDataImporter)
	COM_INTERFACE_ENTRY(IPGSDataImporter)
END_COM_MAP()

// IPGSDataImporter
public:
   STDMETHOD(Init)(UINT nCmdID) override;
   STDMETHOD(GetMenuText)(/*[out,retval]*/BSTR*  bstrText) const override;
   STDMETHOD(GetBitmapHandle)(/*[out]*/HBITMAP* phBmp) const override;
   STDMETHOD(GetCommandHintText)(BSTR*  bstrText) const override;
   STDMETHOD(Import)(/*[in]*/IBroker* pBroker) override;
};
