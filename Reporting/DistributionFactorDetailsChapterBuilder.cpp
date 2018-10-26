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

#include "StdAfx.h"
#include <Reporting\DistributionFactorDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>

#include <IFace\DisplayUnits.h>
#include <IFace\DistributionFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDistributionFactorDetailsChapterBuilder
****************************************************************************/


// free functions
rptRcTable* BuildDfTable(const lrfdILiveLoadDistributionFactor::DFResult& G1, const lrfdILiveLoadDistributionFactor::DFResult& G2, bool isExterior);
void FillRow(int row, rptRcTable* pTable, const std::string& rowtit, const lrfdILiveLoadDistributionFactor::DFResult& res, bool isExterior);


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDistributionFactorDetailsChapterBuilder::CDistributionFactorDetailsChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDistributionFactorDetailsChapterBuilder::GetName() const
{
   return TEXT("Live Load Distribution Factor Details");
}

rptChapter* CDistributionFactorDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   
   CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,span,gdr,pDisplayUnits);

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   pDistFact->ReportDistributionFactors(span,gdr,pChapter,pDisplayUnits);
   return pChapter;
}

CChapterBuilder* CDistributionFactorDetailsChapterBuilder::Clone() const
{
   return new CDistributionFactorDetailsChapterBuilder;
}
