///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// TxDOTAgentImp.cpp : Implementation of CTxDOTAgentImp

#include "stdafx.h"
#include "TxDOTAgentImp.h"

#include <IFace\StatusCenter.h>
#include <IReportManager.h>

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>

#include "TexasIBNSChapterBuilder.h"
#include "TexasGirderSummaryChapterBuilder.h"
#include "TexasPrestressSummaryChapterBuilder.h"
#include "TexasCamberAndDeflectionChapterBuilder.h"
#include "TexasStressChecksChapterBuilder.h"
#include "TexasMomentCapacityChapterBuilder.h"
#include "TexasShearChapterBuilder.h"


#include "TxDOTCadWriter.h"


DECLARE_LOGFILE;

// CTxDOTAgentImp

/////////////////////////////////////////////////////////////////////////////
// IAgentEx
//
STDMETHODIMP CTxDOTAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_ITxDOTCadExport, this );
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::Init()
{
   CREATE_LOGFILE("TxDOTAgent");

   AGENT_INIT;

   // We are going to add new reports to PGSuper. In order to do this, the agent that implements
   // IReportManager must be loaded. We have no way of knowing if that agent is loaded before
   // or after us. Request the broker call our Init2 function after all registered agents
   // are loaded
   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CTxDOTAgentImp::Init2()
{
   // Register our reports
   GET_IFACE(IReportManager,pRptMgr);

   //
   // Create report spec builders
   //

   boost::shared_ptr<CReportSpecificationBuilder> pSpanGirderRptSpecBuilder( new CSpanGirderReportSpecificationBuilder(m_pBroker) );


   // Texas Girder Schedule - use compacted title page
   CReportBuilder* pRptBuilder = new CReportBuilder("TxDOT Girder Schedule Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasIBNSChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Texas Summary report
   pRptBuilder = new CReportBuilder("TxDOT Summary Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasGirderSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasPrestressSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasStressChecksChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasMomentCapacityChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasShearChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_TxDOTAgent;
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::Reset()
{
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::ShutDown()
{
   AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;
   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ITxDOTCadExport
int CTxDOTAgentImp::WriteCADDataToFile(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, TxDOTCadExportFormatType format, bool designSucceeded)
{
   return TxDOT_WriteCADDataToFile(fp,pBroker,span, gdr, format, designSucceeded);
}

int CTxDOTAgentImp::WriteDistributionFactorsToFile(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr)
{
   return TxDOT_WriteDistributionFactorsToFile(fp,pBroker,span,gdr);
}
