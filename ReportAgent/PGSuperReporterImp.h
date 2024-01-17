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

// PGSuperReporterImp.h : Declaration of the CPGSuperReporterImp

#pragma once

#include "CLSID.h"

#include "resource.h"       // main symbols

#include "ReporterBase.h"

#include <IFace\Project.h>
#include <IFace\ReportOptions.h>

#include <EAF\EAFInterfaceCache.h>

#include <memory>

#include <map>

class rptReport;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperReporterImp
class ATL_NO_VTABLE CPGSuperReporterImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSuperReporterImp, &CLSID_PGSuperReportAgent>,
   public CReporterBase,
	public IConnectionPointContainerImpl<CPGSuperReporterImp>,
   public IAgentEx,
   public IReportOptions,
   public ISpecificationEventSink
{
public:
	CPGSuperReporterImp()
	{
      m_pBroker = 0;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPER_REPORTER)

BEGIN_COM_MAP(CPGSuperReporterImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
   COM_INTERFACE_ENTRY(IReportOptions)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperReporterImp)
END_CONNECTION_POINT_MAP()


// IAgent
public:
	STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker) override;
   STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

protected:
   // CReporterBase implementation
   virtual WBFL::Reporting::TitlePageBuilder* CreateTitlePageBuilder(LPCTSTR strName,bool bFullVersion=true) override;

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged() override;
   virtual HRESULT OnAnalysisTypeChanged() override;

// IReportOptions
      virtual bool IncludeSpanAndGirder4Pois(const CGirderKey& rKey) override;

private:
   DECLARE_EAF_AGENT_DATA;

   DWORD m_dwSpecCookie;

   HRESULT InitReportBuilders();
};
