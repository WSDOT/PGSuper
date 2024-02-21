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
#include "CLSID.h"
#include "resource.h"
#include <EAF\EAFAppPlugin.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF\EAFAppPluginDocumentationImpl.h>

class CLibraryAppPlugin;

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};

   afx_msg void OnManagePlugins();

   CLibraryAppPlugin* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

class ATL_NO_VTABLE CLibraryAppPlugin : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CLibraryAppPlugin, &CLSID_LibraryAppPlugin>,
   public IEAFAppPlugin,
   public IEAFCommandCallback
{
public:
   CLibraryAppPlugin()
   {
      m_MyCmdTarget.m_pMyAppPlugin = this;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_LIBAPPPLUGIN)

BEGIN_COM_MAP(CLibraryAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CLibraryAppPlugin)
END_CONNECTION_POINT_MAP()

   CMyCmdTarget m_MyCmdTarget;

   void ManagePlugins();

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent);
   virtual void Terminate();
   virtual void IntegrateWithUI(BOOL bIntegrate);
   virtual std::vector<CEAFDocTemplate*> CreateDocTemplates();
   virtual HMENU GetSharedMenuHandle();
   virtual CString GetName();
   virtual CString GetDocumentationSetName();
   virtual CString GetDocumentationURL();
   virtual CString GetDocumentationMapFile();
   virtual void LoadDocumentationMap();
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL);

// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
   virtual BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const;

private:
   CEAFAppPluginDocumentationImpl m_DocumentationImpl;
};


OBJECT_ENTRY_AUTO(__uuidof(LibraryAppPlugin), CLibraryAppPlugin)
