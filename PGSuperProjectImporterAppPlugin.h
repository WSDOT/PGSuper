///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\PGSuperAppPlugin_i.h"
#include <EAF\EAFAppPlugin.h>
#include "PGSuperBaseAppPlugin.h"
#include "PGSuperAppPlugin\resource.h"


class CPGSuperProjectImporterAppPlugin;

class CProjectImportersCmdTarget : public CCmdTarget
{
public:
   CProjectImportersCmdTarget() {};

   afx_msg void OnConfigureProjectImporters();

   CPGSuperProjectImporterAppPlugin* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

class ATL_NO_VTABLE CPGSuperProjectImporterAppPlugin : 
   public CPGSuperBaseAppPlugin,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperProjectImporterAppPlugin, &CLSID_PGSuperProjectImporterAppPlugin>,
   public IEAFAppPlugin,
   public IEAFCommandCallback
{
public:
   CPGSuperProjectImporterAppPlugin()
   {
   }

   HRESULT FinalConstruct();
   void FinalRelease();

   virtual CString GetAppName() const { return CString("PGSuper"); }
   virtual CString GetTemplateFileExtension();

   virtual void SaveReportOptions();

BEGIN_COM_MAP(CPGSuperProjectImporterAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperProjectImporterAppPlugin)
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERPROJECTIMPORTERAPPPLUGIN)

   HMENU m_hMenuShared;
   CProjectImportersCmdTarget m_MyCmdTarget;

   void ConfigureProjectImporters();

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent);
   virtual void Terminate();
   virtual void IntegrateWithUI(BOOL bIntegrate);
   virtual std::vector<CEAFDocTemplate*> CreateDocTemplates();
   virtual HMENU GetSharedMenuHandle();
   virtual CString GetName();

// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
   virtual BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const;
};

OBJECT_ENTRY_AUTO(__uuidof(PGSuperProjectImporterAppPlugin), CPGSuperProjectImporterAppPlugin)
