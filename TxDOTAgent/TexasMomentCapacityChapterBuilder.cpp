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
#include <Reporting\ConstructabilityCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\MomentCapacityParagraphBuilder.h>
#include "TexasMomentCapacityChapterBuilder.h"

#include <IFace\DisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTexasMomentCapacityChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasMomentCapacityChapterBuilder::CTexasMomentCapacityChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasMomentCapacityChapterBuilder::GetName() const
{
   return TEXT("Moment Capacity");
}

rptChapter* CTexasMomentCapacityChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* p = new rptParagraph;
   bool bOverReinforced;
   *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;

   // The same code below is in serveral places:
#pragma Reminder("Redundant code to Determine whether to compute negative moment is all over PGSuper - This should be added to an interface once Rating is merged")
   bool bComputeNegativeMomentCapacity = false;

   // don't need to write out negative moment capacity if this is a simple span design
   // or if there isn't any continuity
   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( analysisType == pgsTypes::Continuous || analysisType == pgsTypes::Envelope )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);

      PierIndexType prev_pier = span;
      PierIndexType next_pier = prev_pier + 1;

      bool bContinuousAtPrevPier,bContinuousAtNextPier,bValue;
      pBridge->IsContinuousAtPier(prev_pier,&bValue,&bContinuousAtPrevPier);
      pBridge->IsContinuousAtPier(next_pier,&bContinuousAtNextPier,&bValue);

      bool bIntegralAtPrevPier,bIntegralAtNextPier;
      pBridge->IsIntegralAtPier(prev_pier,&bValue,&bIntegralAtPrevPier);
      pBridge->IsIntegralAtPier(next_pier,&bIntegralAtNextPier,&bValue);

      bComputeNegativeMomentCapacity = ( bContinuousAtPrevPier || bContinuousAtNextPier || bIntegralAtPrevPier || bIntegralAtNextPier );
   }

   if (bComputeNegativeMomentCapacity)
   {
      *p << rptNewLine;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,false,&bOverReinforced) << rptNewLine;
   }

   *pChapter << p;

   return pChapter;
}

CChapterBuilder* CTexasMomentCapacityChapterBuilder::Clone() const
{
   return new CTexasMomentCapacityChapterBuilder;
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
