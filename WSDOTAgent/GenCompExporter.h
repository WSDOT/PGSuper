// GenCompExporter.h : Declaration of the CGenCompExporter

#pragma once

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGenCompExporter
class ATL_NO_VTABLE CGenCompExporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CGenCompExporter, &CLSID_GenCompExporter>,
   public IPGSDataExporter
{
public:
	CGenCompExporter()
	{
	}

   HRESULT FinalConstruct();

   CBitmap m_Bitmap;

DECLARE_REGISTRY_RESOURCEID(IDR_GENCOMPEXPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGenCompExporter)
   COM_INTERFACE_ENTRY(IPGSDataExporter)
END_COM_MAP()

// IPGSDataExporter
public:
   STDMETHOD(Init)(UINT nCmdID);
   STDMETHOD(GetMenuText)(/*[out,retval]*/BSTR*  bstrText);
   STDMETHOD(GetBitmapHandle)(/*[out]*/HBITMAP* phBmp);
   STDMETHOD(GetCommandHintText)(BSTR*  bstrText);
   STDMETHOD(Export)(/*[in]*/IBroker* pBroker);
};

OBJECT_ENTRY_AUTO(__uuidof(GenCompExporter), CGenCompExporter)
