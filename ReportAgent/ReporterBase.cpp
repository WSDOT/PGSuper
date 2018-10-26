///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// PGSpliceReporterImp.cpp : Implementation of CPGSpliceReporterImp
#include "stdafx.h"
#include "ReporterBase.h"

#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>

#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\DeckElevationChapterBuilder.h>
#include <Reporting\PierGeometryChapterBuilder.h>
#include <Reporting\GirderGeometryChapterBuilder.h>

#include <Reporting\IntervalChapterBuilder.h>
#include <Reporting\TendonGeometryChapterBuilder.h>
#include <Reporting\TimeStepParametersChapterBuilder.h>

#include <Reporting\PointOfInterestChapterBuilder.h>

#include <IReportManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CReporterBase::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

HRESULT CReporterBase::InitCommonReportBuilders()
{
   CreateBridgeGeometryReport();
   CreateStageByStageDetailsReport();
   CreatePointOfInterestReport();
   return S_OK;
}

void CReporterBase::CreateBridgeGeometryReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   
   boost::shared_ptr<CReportSpecificationBuilder> pBrokerRptSpecBuilder(  new CBrokerReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Bridge Geometry Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDeckElevationChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPierGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CGirderGeometryChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateStageByStageDetailsReport()
{
#pragma Reminder("UPDATE: this is a report for development purposes")
   // either flesh out this report or eliminate it

   GET_IFACE(IReportManager,pRptMgr);
   boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(  new CGirderReportSpecificationBuilder(m_pBroker,CGirderKey(0,0)) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Stage by Stage Details Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CIntervalChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTendonGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTimeStepParametersChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreatePointOfInterestReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(  new CGirderLineReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Points of Interest Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPointOfInterestChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}
