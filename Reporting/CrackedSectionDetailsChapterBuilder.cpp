///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\CrackedSection.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>

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
                             const CGirderKey& girderKey,
                             const PoiList& vPoi,
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
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
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
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(thisGirderKey.groupIndex,&startSpanIdx,&endSpanIdx);
      bool bProcessNegativeMoments = false;
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         if ( pBridge->ProcessNegativeMoments(spanIdx) )
         {
            bProcessNegativeMoments = true;
            break;
         }
      }

      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);
      write_cracked_section_table(pBroker,pDisplayUnits, thisGirderKey, vPoi, pChapter, bProcessNegativeMoments);
   }

   return pChapter;
}

CChapterBuilder* CCrackedSectionDetailsChapterBuilder::Clone() const
{
   return new CCrackedSectionDetailsChapterBuilder;
}

void write_cracked_section_table(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             const CGirderKey& girderKey,
                             const PoiList& vPoi,
                             rptChapter* pChapter,
                             bool bIncludeNegMoment)
{
   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Setup the table
   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(bIncludeNegMoment ? 7 : 4,_T(""));

   *pPara << table << rptNewLine;

   ColumnIndexType col = 0;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   table->SetNumberOfHeaderRows(2);

   table->SetRowSpan(0,0,2);
   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table->SetColumnSpan(0,1,3);
   (*table)(0,1) << _T("Positive Moment");
   (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(1,col++) << COLHDR(Sub2(_T("I"),_T("cr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

   if ( bIncludeNegMoment )
   {
      table->SetColumnSpan(0,4,3);
      (*table)(0,4) << _T("Negative Moment");
      (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(1,col++) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(1,col++) << COLHDR(Sub2(_T("I"),_T("cr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_i,    pDisplayUnits->GetMomentOfInertiaUnit(),       false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ICrackedSection,pCrackedSection);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CRACKEDSECTIONDETAILS* pCSD = pCrackedSection->GetCrackedSectionDetails(poi,true);

      Float64 h = pSectProp->GetHg(liveLoadIntervalIdx,poi);

      Float64 Yt = pCSD->c;
      Float64 Yb = h - Yt;

      col = 0;

      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );
      (*table)(row,col++) << dim.SetValue( Yt );
      (*table)(row,col++) << dim.SetValue( Yb );
      (*table)(row,col++) << mom_i.SetValue( pCSD->Icr );

      if ( bIncludeNegMoment )
      {
         pCSD = pCrackedSection->GetCrackedSectionDetails(poi,false);
         Yt = pCSD->c;
         Yb = h - Yt;
         (*table)(row,col++) << dim.SetValue( Yt );
         (*table)(row,col++) << dim.SetValue( Yb );
         (*table)(row,col++) << mom_i.SetValue( pCSD->Icr );
      }

      row++;
   }
}
