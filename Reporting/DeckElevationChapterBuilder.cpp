///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Reporting\DeckElevationChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>


#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDeckElevationChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDeckElevationChapterBuilder::CDeckElevationChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDeckElevationChapterBuilder::GetName() const
{
   return TEXT("Deck Elevations");
}

rptChapter* CDeckElevationChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CBrokerReportSpecification* pSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   CGirderReportSpecification* pSGRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CGirderKey girderKey;
   if ( pSGRptSpec )
   {
      girderKey = pSGRptSpec->GetGirderKey();
   }

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Deck Elevations over Girder Webs") << rptNewLine;
   (*pChapter) << pPara;

   pPara = new rptParagraph();
   (*pChapter) << pPara;
   *pPara << _T("Web Offset is measured from and normal to the centerline girder") << rptNewLine;
   *pPara << _T("Station, Offset, and Elev are given for 10th points between bearings along the girder webs") << rptNewLine;

   //
   // Bridge Elevation Table
   //

   INIT_UV_PROTOTYPE( rptLengthSectionValue, webDim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), true );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IPointOfInterest, pPOI);
   GET_IFACE2(pBroker, IGirder, pGirder );

   RowIndexType row_step = 4; // number of rows reported for each web

   SpanIndexType nSpans = pBridge->GetSpanCount();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   SpanIndexType startSpanIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : pBridge->GetGirderGroupStartSpan(girderKey.groupIndex));
   SpanIndexType endSpanIdx   = (girderKey.groupIndex == ALL_GROUPS ? nSpans-1 : pBridge->GetGirderGroupEndSpan(girderKey.groupIndex));

   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(spanIdx) << std::endl;
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(14,os.str().c_str());
      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(1);
      pTable->SetNumberOfStripedRows(row_step); // stripe in bands of row_step

      ColumnIndexType col = 0;
      for ( col = 0; col < pTable->GetNumberOfColumns(); col++ )
      {
         pTable->SetColumnStyle(col,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetStripeRowColumnStyle(col,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }

      col = 0;
      (*pTable)(0,col++) << _T("Girder");
      (*pTable)(0,col++) << _T("Web");
      (*pTable)(0,col++) << _T("");
      (*pTable)(0,col++) << _T("CL Brg");
      (*pTable)(0,col++) << Sub2(_T("0.1L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.2L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.3L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.4L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.5L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.6L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.7L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.8L"),_T("s"));
      (*pTable)(0,col++) << Sub2(_T("0.9L"),_T("s"));
      (*pTable)(0,col++) << _T("CL Brg");

      RowIndexType row = pTable->GetNumberOfHeaderRows();
      col = 0;

      GirderIndexType nGirders = pBridge->GetGirderCountBySpan(spanIdx);
      GirderIndexType startGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType endGirderIdx   = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : startGirderIdx);

      for ( GirderIndexType gdrIdx = startGirderIdx; gdrIdx <= endGirderIdx; gdrIdx++ )
      {
         CSpanKey spanKey(spanIdx,gdrIdx);

         std::vector<pgsPointOfInterest> vPoi( pPOI->GetPointsOfInterest(spanKey,POI_SPAN | POI_TENTH_POINTS) );
         ATLASSERT(vPoi.size() == 11);

         GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
         CSegmentKey segmentKey(grpIdx,gdrIdx,0);

         MatingSurfaceIndexType nWebs = pGirder->GetNumberOfMatingSurfaces(segmentKey);
         pTable->SetRowSpan(row,0,Int16(row_step*nWebs));

         RowIndexType r;
         for ( r = row+1; r < row+row_step*nWebs; r++ )
         {
            pTable->SetRowSpan(r,0,SKIP_CELL);
            pTable->SetRowSpan(r,1,SKIP_CELL);
         }

         (*pTable)(row,0) << LABEL_GIRDER(gdrIdx);

         for ( MatingSurfaceIndexType web = 0; web < nWebs; web++ )
         {
            col = 1;

            pTable->SetRowSpan(row,col,row_step);
            for ( r = row+1; r < row+row_step; r++ )
            {
               pTable->SetRowSpan(r,col,SKIP_CELL);
            }

            (*pTable)(row,col++) << MatingSurfaceIndexType(web+1) << rptNewLine;

            (*pTable)(row,col)   << Bold(_T("Web Offset (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag() ) << Bold(_T(")"));
            (*pTable)(row+1,col) << Bold(_T("Station"));
            (*pTable)(row+2,col) << Bold(_T("Offset (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag() ) << Bold(_T(")"));
            (*pTable)(row+3,col) << Bold(_T("Elev (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag() ) << Bold(_T(")"));
            
            col++;

            std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
            std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
            for ( ; iter != end; iter++ )
            {
               pgsPointOfInterest& poi = *iter;

               // offset to centerline of web, measured from centerline of girder
               // < 0 = left of centerline
               // in a nonprismatic member, web offset can  very with location
               Float64 webOffset = pGirder->GetMatingSurfaceLocation(poi,web);

               Float64 sta, offset;
               pBridge->GetStationAndOffset(poi,&sta,&offset);

               Float64 total_offset = offset + webOffset;
               Float64 elev = pAlignment->GetElevation(sta,total_offset);

               (*pTable)(row,col)   << RPT_OFFSET(webOffset,dist);
               (*pTable)(row+1,col) << rptRcStation(sta, &pDisplayUnits->GetStationFormat() );
               (*pTable)(row+2,col) << RPT_OFFSET(total_offset,dist);
               (*pTable)(row+3,col) << dist.SetValue(elev);

               col++;
            }

            row += row_step;
         }
      }
   }

   return pChapter;
}


CChapterBuilder* CDeckElevationChapterBuilder::Clone() const
{
   return new CDeckElevationChapterBuilder;
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
