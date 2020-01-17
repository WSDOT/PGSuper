///////////////////////////////////////////////////////////////////////
// LibraryMgrPluginExample
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

// LibMgrDocPlugin.h : Declaration of the CLibMgrDocPlugin

#pragma once
#include "resource.h"       // main symbols

#include "LibraryMgrPluginExample_i.h"
#include <EAF\EAFDocumentPlugin.h>
#include <EAF\EAFUIintegration.h>
#include <EAF\EAFPluginPersist.h>


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

class CLibMgrDocPlugin;

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};
   
   void OnMyCommand();
   void OnUpdateMyCommand(CCmdUI* pCmdUI);
   void OnCreateView();

   CLibMgrDocPlugin* m_pMyDocPlugin;

   DECLARE_MESSAGE_MAP()
};

// CLibMgrDocPlugin

class ATL_NO_VTABLE CLibMgrDocPlugin :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CLibMgrDocPlugin, &CLSID_LibMgrDocPlugin>,
	public IEAFDocumentPlugin,
   public IEAFCommandCallback,
   public IEAFPluginPersist
{
public:
	CLibMgrDocPlugin()
	{
      m_MyCommandTarget.m_pMyDocPlugin = this;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LIBMGRDOCPLUGIN)


BEGIN_COM_MAP(CLibMgrDocPlugin)
	COM_INTERFACE_ENTRY(IEAFDocumentPlugin)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
   COM_INTERFACE_ENTRY(IEAFPluginPersist)
END_COM_MAP()

   CEAFDocument* m_pDoc;
   CMyCmdTarget m_MyCommandTarget;
   CBitmap m_bmpMenuItem;


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}


   void CreateMenus();
   void RemoveMenus();
   CEAFMenu* m_pPluginMenu;

   void RegisterViews();
   void UnregisterViews();
   void CreateView();
   long m_MyViewKey;

// IEAFDocumentPlugin
public:
   virtual BOOL Init(CEAFDocument* pParent) override;
   virtual BOOL IntagrateWithUI(BOOL bIntegrate) override;
   virtual void Terminate() override;
   virtual CString GetName() override;
   virtual CString GetDocumentationSetName() override;
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID,CString& strURL) override;

// IEAFCommandCallback
public:
   BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo) override;
   BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
   BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

// IEAFPluginPersist
public:
   virtual HRESULT Save(IStructuredSave* pStrSave) override;
   virtual HRESULT Load(IStructuredLoad* pStrLoad) override;
};

OBJECT_ENTRY_AUTO(__uuidof(LibMgrDocPlugin), CLibMgrDocPlugin)
