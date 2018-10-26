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
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\PrestressLossTable.h>
#include "TexasPrestressSummaryChapterBuilder.h"

#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTexasPrestressSummaryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasPrestressSummaryChapterBuilder::CTexasPrestressSummaryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasPrestressSummaryChapterBuilder::GetName() const
{
   return TEXT("Prestress Force and Strand Stresses");
}

rptChapter* CTexasPrestressSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType span = pSpec->GetSpan();
   GirderIndexType girder = pSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << CPrestressLossTable().Build(pBroker,span,girder,pDisplayUnits) << rptNewLine;

   return pChapter;
}

CChapterBuilder* CTexasPrestressSummaryChapterBuilder::Clone() const
{
   return new CTexasPrestressSummaryChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
