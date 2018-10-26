///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <Reporting\CrackedSectionDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\CrackedSection.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCrackedSectionDetailsChapterBuilder
****************************************************************************/

void write_cracked_section_table(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             bool bIncludeNegMoment);

CCrackedSectionDetailsChapterBuilder::CCrackedSectionDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CCrackedSectionDetailsChapterBuilder::GetName() const
{
   return TEXT("Cracked Section Analysis Details");
}

rptChapter* CCrackedSectionDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType gdr;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      gdr = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      gdr = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      gdr  = ALL_GIRDERS;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   bool bRateRoutine = pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine);
   bool bRateSpecial = pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special);

   if ( !bRateRoutine && !bRateSpecial )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Cracked section analysis not performed") << rptNewLine;
      return pChapter;
   }
   else
   {
      if ( !pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) && !pRatingSpec->RateForStress(pgsTypes::lrPermit_Special) )
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << _T("Cracked section analysis not performed") << rptNewLine;
         return pChapter;
      }
   }

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << Sub2(_T("Y"),_T("t")) << _T(" = Distance from top of section to neutral axis") << rptNewLine;
   *pPara << Sub2(_T("Y"),_T("b")) << _T(" = Distance from bottom of section to neutral axis") << rptNewLine;
   *pPara << Sub2(_T("I"),_T("cr")) << _T(" = Moment of inertia of cracked section about neutral axis") << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi;

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType firstGirderIdx = min(nGirders-1,(gdr == ALL_GIRDERS ? 0 : gdr));
      GirderIndexType lastGirderIdx  = min(nGirders,  (gdr == ALL_GIRDERS ? nGirders : firstGirderIdx + 1));

      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx < lastGirderIdx; gdrIdx++ )
      {
         rptParagraph* pPara;
         if ( span == ALL_SPANS || gdr == ALL_GIRDERS )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx);
            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         vPoi = pIPOI->GetPointsOfInterest( spanIdx,gdrIdx, pgsTypes::BridgeSite3, POI_FLEXURECAPACITY | POI_SHEAR, POIFIND_OR );
         write_cracked_section_table(pBroker,pDisplayUnits, spanIdx,gdrIdx, vPoi, pChapter, pBridge->ProcessNegativeMoments(spanIdx));
      }
   }

   return pChapter;
}

CChapterBuilder* CCrackedSectionDetailsChapterBuilder::Clone() const
{
   return new CCrackedSectionDetailsChapterBuilder;
}

void write_cracked_section_table(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             bool bIncludeNegMoment)
{
   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   // Setup the table
   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(bIncludeNegMoment ? 7 : 4,_T(""));

   *pPara << table << rptNewLine;

   ColumnIndexType col = 0;

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   table->SetNumberOfHeaderRows(2);

   table->SetRowSpan(0,0,2);
   table->SetRowSpan(1,0,SKIP_CELL);
   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table->SetColumnSpan(0,1,3);
   table->SetColumnSpan(0,2,SKIP_CELL);
   table->SetColumnSpan(0,3,SKIP_CELL);
   (*table)(0,1) << _T("Positive Moment");
   (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(1,col++) << COLHDR(Sub2(_T("I"),_T("cr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

   if ( bIncludeNegMoment )
   {
      table->SetColumnSpan(0,4,3);
      table->SetColumnSpan(0,5,SKIP_CELL);
      table->SetColumnSpan(0,6,SKIP_CELL);
      (*table)(0,4) << _T("Negative Moment");
      (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(1,col++) << COLHDR(Sub2(_T("I"),_T("cr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_i,    pDisplayUnits->GetMomentOfInertiaUnit(),       false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ICrackedSection,pCrackedSection);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      CRACKEDSECTIONDETAILS csd;
      pCrackedSection->GetCrackedSectionDetails(poi,true,&csd);

      Float64 h = pSectProp->GetHg(pgsTypes::BridgeSite3,poi);

      Float64 Yt = csd.c;
      Float64 Yb = h - Yt;

      col = 0;

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,col++) << dim.SetValue( Yt );
      (*table)(row,col++) << dim.SetValue( Yb );
      (*table)(row,col++) << mom_i.SetValue( csd.Icr );

      if ( bIncludeNegMoment )
      {
         pCrackedSection->GetCrackedSectionDetails(poi,false,&csd);
         Yt = csd.c;
         Yb = h - Yt;
         (*table)(row,col++) << dim.SetValue( Yt );
         (*table)(row,col++) << dim.SetValue( Yb );
         (*table)(row,col++) << mom_i.SetValue( csd.Icr );
      }

      row++;
   }
}
