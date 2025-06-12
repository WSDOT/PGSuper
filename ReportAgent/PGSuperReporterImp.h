///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "ReporterBase.h"

#include <IFace\Project.h>
#include <IFace\ReportOptions.h>



#include <memory>

#include <map>


#include <Reporting\ReporterEvents.h>
#include "CPReportAgent.h"



class rptReport;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperReporterImp
class CPGSuperReporterImp : public CReporterBase,
   public IReportOptions,
   public ISpecificationEventSink,
   public CProxyIReporterEventSink<CPGSuperReporterImp>
{
public:
	CPGSuperReporterImp()
	{
   }


// IAgent
public:
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const;

protected:
   // CReporterBase implementation
   virtual WBFL::Reporting::TitlePageBuilder* CreateTitlePageBuilder(LPCTSTR strName,bool bFullVersion=true) override;

// ISpecificationEventSink
public:
   HRESULT OnSpecificationChanged() override;
   HRESULT OnAnalysisTypeChanged() override;

// IReportOptions
   bool IncludeSpanAndGirder4Pois(const CGirderKey& rKey) override;

private:
   EAF_DECLARE_AGENT_DATA;

   IDType m_dwSpecCookie;

   HRESULT InitReportBuilders();
};
