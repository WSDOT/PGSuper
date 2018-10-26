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

#include <EAF\EAFDisplayUnits.h>
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

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* p = new rptParagraph;
   bool bOverReinforced;
   *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->ProcessNegativeMoments(span) )
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
