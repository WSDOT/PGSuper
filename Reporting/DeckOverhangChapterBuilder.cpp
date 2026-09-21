///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
#include <Reporting\DeckOverhangChapterBuilder.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\Project.h>
#include <IFace\PointOfInterest.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <WBFLCogo.h>


// The overhang is taken as constant along a span when the spread between the largest and
// smallest value is within this tolerance. The tolerance is deliberately no finer than the
// precision the values are reported with, so that two values that print the same are both
// reported at mid-span.
static const Float64 g_OverhangTolerance = WBFL::Units::ConvertToSysUnits(0.001,WBFL::Units::Measure::Feet);

void deck_overhangs(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter);

/****************************************************************************
CLASS
   CDeckOverhangChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDeckOverhangChapterBuilder::CDeckOverhangChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDeckOverhangChapterBuilder::GetName() const
{
   return TEXT("Deck Overhangs");
}

rptChapter* CDeckOverhangChapterBuilder::Build(const std::shared_ptr<const WBFL::ReportMgr::ReportSpecification>& pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   auto pSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);

   auto pBroker = pSpec->GetBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   deck_overhangs(pBroker,pDisplayUnits,pChapter);

   return pChapter;
}


std::unique_ptr<WBFL::ReportMgr::ChapterBuilder> CDeckOverhangChapterBuilder::Clone() const
{
   return std::make_unique<CDeckOverhangChapterBuilder>();
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

void deck_overhangs(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter)
{
   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2_NOCHECK(pBroker, IRoadway,pAlignment);
   GET_IFACE2_NOCHECK(pBroker, IPointOfInterest,pPoi);

   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   if ( !pBridge->HasDeckOverhang() )
   {
      *pPara << _T("No deck overhang exists. This bridge does not have a deck and the exterior girders are solid sections, so the edge of deck does not reach past the exterior face of the exterior web.") << rptNewLine;
      return;
   }
   *pPara << _T("Overhang is the horizontal distance from the centerline of the exterior web of the exterior girder to the edge of deck. For sections with a single web, the centerline of the beam is the reference.") << rptNewLine;
   *pPara << _T("Values are the extremes found at tenth points between bearings. Station and offset locate the point on the edge of deck. Where the overhang is constant along a span, the value is reported at mid-span.") << rptNewLine;

   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      *pPara << _T("This bridge does not have a deck, so the top edge of the exterior girder is used as the edge of deck.") << rptNewLine;
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,  pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), false );

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(11,_T(""));
   (*pPara) << pTable << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetNumberOfStripedRows(4); // stripe in bands of one span

   ColumnIndexType col = 0;

   pTable->SetRowSpan(0, col, 2);
   (*pTable)(0,col++) << _T("Span");

   pTable->SetRowSpan(0, col, 2);
   (*pTable)(0,col++) << _T("Side");

   pTable->SetRowSpan(0, col, 2);
   (*pTable)(0,col++) << _T("Measured") << rptNewLine << _T("Normal To");

   pTable->SetColumnSpan(0, col, 4);
   (*pTable)(0, col) << _T("Minimum Overhang");
   (*pTable)(1, col++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Support"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(1, col++) << COLHDR(_T("Overhang"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(1, col++) << _T("Station");
   (*pTable)(1, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

   pTable->SetColumnSpan(0, col, 4);
   (*pTable)(0, col) << _T("Maximum Overhang");
   (*pTable)(1, col++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Support"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(1, col++) << COLHDR(_T("Overhang"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(1, col++) << _T("Station");
   (*pTable)(1, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

   bool bNoOverhang = false; // set when the 1/2 tw test is not satisfied anywhere in the bridge

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      pTable->SetRowSpan(row, 0, 4); // left and right, each measured two ways
      (*pTable)(row,0) << LABEL_SPAN(spanIdx);

      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::SideType side = (i == 0 ? pgsTypes::stLeft : pgsTypes::stRight);
         GirderIndexType gdrIdx = (side == pgsTypes::stLeft ? 0 : nGirders-1);

         CSpanKey spanKey(spanIdx,gdrIdx);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(spanKey, POI_SPAN | POI_TENTH_POINTS, &vPoi);
         ATLASSERT(vPoi.size() == 11);

         pTable->SetRowSpan(row, 1, 2); // both measurement directions
         (*pTable)(row,1) << (side == pgsTypes::stLeft ? _T("Left") : _T("Right"));

         for ( int j = 0; j < 2; j++ )
         {
            pgsTypes::DeckOverhangMeasurementType measure = (j == 0 ? pgsTypes::domtNormalToAlignment : pgsTypes::domtNormalToGirder);

            DeckOverhangDetails minDetails, maxDetails, midDetails;
            bool bFirst = true;
            for ( const pgsPointOfInterest& poi : vPoi )
            {
               DeckOverhangDetails details = pBridge->GetDeckOverhangDetails(poi,side,measure);

               if ( bFirst || details.Overhang < minDetails.Overhang )
               {
                  minDetails = details;
               }

               if ( bFirst || maxDetails.Overhang < details.Overhang )
               {
                  maxDetails = details;
               }

               if ( poi.IsMidSpan(POI_SPAN) )
               {
                  midDetails = details;
               }

               bFirst = false;
            }

            // when the overhang is constant the controlling location is arbitrary,
            // so report it at mid-span
            if ( midDetails.pntDeckEdge && maxDetails.Overhang - minDetails.Overhang < g_OverhangTolerance )
            {
               minDetails = midDetails;
               maxDetails = midDetails;
            }

            col = 2;
            (*pTable)(row,col++) << (measure == pgsTypes::domtNormalToAlignment ? _T("Alignment") : _T("Girder Line"));

            const DeckOverhangDetails* pDetails[2] = { &minDetails, &maxDetails };
            for ( int k = 0; k < 2; k++ )
            {
               Float64 station, offset;
               pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pDetails[k]->pntDeckEdge,&station,&offset);

               (*pTable)(row,col++) << location.SetValue(POI_SPAN, pDetails[k]->Poi);

               // a negative overhang means the edge of deck is inboard of the CL of the exterior web
               bool bNegative = (pDetails[k]->Overhang < 0);
               if ( bNegative )
               {
                  (*pTable)(row,col) << color(Red);
               }

               (*pTable)(row,col) << cogoPoint.SetValue(pDetails[k]->Overhang);

               if ( bNegative )
               {
                  (*pTable)(row,col) << color(Black);
               }
               if ( !pDetails[k]->bExists )
               {
                  (*pTable)(row,col) << Super(_T("*"));
                  bNoOverhang = true;
               }
               col++;

               (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
               (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            }

            row++;
         }
      }
   }

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   (*pChapter) << pPara;
   *pPara << _T("The centerline of the exterior web is taken at the top of the girder. This is the same reference the live load distribution factor computations use for ") << Sub2(_T("d"),_T("e")) << _T(".") << rptNewLine;

   if ( bNoOverhang )
   {
      *pPara << Super(_T("*")) << _T(" The edge of deck does not reach past the exterior face of the exterior web, so no overhang exists. The dimension to the centerline of the exterior web is reported for reference.") << rptNewLine;
   }
}
