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
#include <Reporting\SpanDataChapterBuilder.h>
#include <Reporting\StrandEccTable.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSpanDataChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpanDataChapterBuilder::CSpanDataChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpanDataChapterBuilder::GetName() const
{
   return TEXT("Span Data");
}

rptChapter* CSpanDataChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

   // Strand Eccentricity Table
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   CStrandEccTable ecc_table;
   *p << ecc_table.Build(pBroker,span,girder,pDisplayUnits) << rptNewLine;

   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   *p << "Eccentricities measured from neutral axis of non-composite section" << rptNewLine;
   *p << "Positive values indicate strands are below the neutral axis" << rptNewLine;
   *p << rptNewLine;


   p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2( pBroker, IBridge, pBridge );

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );
   *p << "Girder Length = " << length.SetValue( pBridge->GetGirderLength(span,girder) ) << rptNewLine;
   *p << "Span Length = " << length.SetValue( pBridge->GetSpanLength(span,girder) )<<" (CL Bearing to CL Bearing)" << rptNewLine;
   *p << "Left End Distance = " << length.SetValue( pBridge->GetGirderStartConnectionLength(span,girder) )<<" (Overhang, CL Bearing to End of Girder, Measured Along Girder)" << rptNewLine;
   *p << "Right End Distance = " << length.SetValue( pBridge->GetGirderEndConnectionLength(span,girder) )<<" (Overhang, CL Bearing to End of Girder, Measured Along Girder)" << rptNewLine;

   return pChapter;
}

CChapterBuilder* CSpanDataChapterBuilder::Clone() const
{
   return new CSpanDataChapterBuilder;
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
