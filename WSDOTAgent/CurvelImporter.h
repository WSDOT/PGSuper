// CurvelImporter.h : Declaration of the CCurvelImporter

#pragma once

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CCurvelImporter
class ATL_NO_VTABLE CCurvelImporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCurvelImporter, &CLSID_CurvelImporter>,
   public IPGSuperDataImporter
{
public:
	CCurvelImporter()
	{
	}

   HRESULT FinalConstruct();
   CBitmap m_Bitmap;

DECLARE_REGISTRY_RESOURCEID(IDR_CURVELIMPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCurvelImporter)
	COM_INTERFACE_ENTRY(IPGSuperDataImporter)
END_COM_MAP()

// IPGSuperDataImporter
public:
   STDMETHOD(Init)(UINT nCmdID);
   STDMETHOD(GetMenuText)(/*[out,retval]*/BSTR*  bstrText);
   STDMETHOD(GetBitmapHandle)(/*[out]*/HBITMAP* phBmp);
   STDMETHOD(GetCommandHintText)(BSTR*  bstrText);
   STDMETHOD(Import)(/*[in]*/IBroker* pBroker);
};

OBJECT_ENTRY_AUTO(__uuidof(CurvelImporter), CCurvelImporter)
