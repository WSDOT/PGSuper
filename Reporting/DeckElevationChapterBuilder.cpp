///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>


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
   return TEXT("Roadway Elevations");
}

rptChapter* CDeckElevationChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridge = pIBridgeDesc->GetBridgeDescription();
   if (pBridge->GetDeckDescription()->GetDeckType() == pgsTypes::sdtNone)
   {
      return BuildNoDeck(pRptSpec, level);
   }
   else
   {
      return BuildDeckOnGirder(pRptSpec, level);
   }

}


std::unique_ptr<WBFL::Reporting::ChapterBuilder> CDeckElevationChapterBuilder::Clone() const
{
   return std::make_unique<CDeckElevationChapterBuilder>();
}

rptChapter* CDeckElevationChapterBuilder::BuildDeckOnGirder(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
{
   auto pSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   auto pSGRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);

   CGirderKey girderKey;
   if (pSGRptSpec)
   {
      girderKey = pSGRptSpec->GetGirderKey();
   }

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Deck Elevations over Girder Webs") << rptNewLine;
   (*pChapter) << pPara;

   pPara = new rptParagraph();
   (*pChapter) << pPara;
   *pPara << _T("Web Offset is measured from and normal to the centerline girder at top of girder") << rptNewLine;
   *pPara << _T("Station, Offset, and Elev are given for 10th points between bearings along the girder webs") << rptNewLine;

   //
   // Bridge Elevation Table
   //

   INIT_UV_PROTOTYPE(rptLengthSectionValue, webDim, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), true);

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2(pBroker, IGirder, pGirder);

   RowIndexType row_step = 4; // number of rows reported for each web

   SpanIndexType nSpans = pBridge->GetSpanCount();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   SpanIndexType startSpanIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : pBridge->GetGirderGroupStartSpan(girderKey.groupIndex));
   SpanIndexType endSpanIdx = (girderKey.groupIndex == ALL_GROUPS ? nSpans - 1 : pBridge->GetGirderGroupEndSpan(girderKey.groupIndex));

   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(spanIdx) << std::endl;
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(14, os.str().c_str());
      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(1);
      pTable->SetNumberOfStripedRows(row_step); // stripe in bands of row_step

      ColumnIndexType col = 0;
      for (col = 0; col < pTable->GetNumberOfColumns(); col++)
      {
         pTable->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }

      col = 0;
      (*pTable)(0, col++) << _T("Girder");
      (*pTable)(0, col++) << _T("Web");
      (*pTable)(0, col++) << _T("");
      (*pTable)(0, col++) << _T("CL Brg");
      (*pTable)(0, col++) << Sub2(_T("0.1L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.2L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.3L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.4L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.5L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.6L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.7L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.8L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.9L"), _T("s"));
      (*pTable)(0, col++) << _T("CL Brg");

      RowIndexType row = pTable->GetNumberOfHeaderRows();
      col = 0;

      GirderIndexType nGirders = pBridge->GetGirderCountBySpan(spanIdx);
      GirderIndexType startGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType endGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : startGirderIdx);

      for (GirderIndexType gdrIdx = startGirderIdx; gdrIdx <= endGirderIdx; gdrIdx++)
      {
         CSpanKey spanKey(spanIdx, gdrIdx);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(spanKey, POI_SPAN | POI_TENTH_POINTS, &vPoi);
         ATLASSERT(vPoi.size() == 11);

         GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
         CSegmentKey segmentKey(grpIdx, gdrIdx, 0);

         MatingSurfaceIndexType nWebs = pGirder->GetMatingSurfaceCount(segmentKey);
         pTable->SetRowSpan(row, 0, Int16(row_step*nWebs));

         (*pTable)(row, 0) << LABEL_GIRDER(gdrIdx);

         for (MatingSurfaceIndexType web = 0; web < nWebs; web++)
         {
            col = 1;

            pTable->SetRowSpan(row, col, row_step);
            (*pTable)(row, col++) << MatingSurfaceIndexType(web + 1) << rptNewLine;

            (*pTable)(row, col) << Bold(_T("Web Offset (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
            (*pTable)(row + 1, col) << Bold(_T("Station"));
            (*pTable)(row + 2, col) << Bold(_T("Offset (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
            (*pTable)(row + 3, col) << Bold(_T("Elev (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));

            col++;

            for (const pgsPointOfInterest& poi : vPoi)
            {
               // offset to centerline of web, measured from centerline of girder
               // < 0 = left of centerline
               // in a nonprismatic member, web offset can  very with location
               Float64 webOffset = pGirder->GetMatingSurfaceLocation(poi, web);

               Float64 sta, offset;
               pBridge->GetStationAndOffset(poi, &sta, &offset);

               Float64 total_offset = offset + webOffset;
               Float64 elev = pAlignment->GetElevation(sta, total_offset);

               (*pTable)(row, col) << RPT_OFFSET(webOffset, dist);
               (*pTable)(row + 1, col) << rptRcStation(sta, &pDisplayUnits->GetStationFormat());
               (*pTable)(row + 2, col) << RPT_OFFSET(total_offset, dist);
               (*pTable)(row + 3, col) << dist.SetValue(elev);

               col++;
            }

            row += row_step;
         }
      }
   }

   return pChapter;
}

rptChapter* CDeckElevationChapterBuilder::BuildNoDeck(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   BuildNoDeckElevationContent(pChapter,pRptSpec,level);

   return pChapter;
}

void CDeckElevationChapterBuilder::BuildNoDeckElevationContent(rptChapter * pChapter,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   auto pSGRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);

   CGirderKey girderKey;
   if (pSGRptSpec)
   {
      girderKey = pSGRptSpec->GetGirderKey();
   }

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Design and Finished Elevations") << rptNewLine;
   (*pChapter) << pPara;

   pPara = new rptParagraph();
   (*pChapter) << pPara;
   *pPara << _T("Offsets are measured from and normal to the left edge, centerline (CL), and right edge of girder at top of girder.") << rptNewLine;
   *pPara << _T("Design elevations are the deck surface elevations along CL girder defined by the alignment, profile, and superelevations.") << rptNewLine;
   *pPara << _T("Finished elevations are the top CL of the finished girder, or overlay if applicable, including the effects of camber.") << rptNewLine;

   GET_IFACE2(pBroker,ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pISpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pISpec->GetSpecification().c_str());
   Float64 tolerance = pSpecEntry->GetFinishedElevationTolerance();
   
   INIT_UV_PROTOTYPE(rptLengthSectionValue, dim1, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, dim2, pDisplayUnits->GetComponentDimUnit(), true);
   *pPara << _T("Finished elevations in red text differ from the design elevation by more than ") << symbol(PLUS_MINUS) << dim1.SetValue(tolerance) << _T(" (") << symbol(PLUS_MINUS) << dim2.SetValue(tolerance) << _T(")") << rptNewLine;

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker,IBridgeDescription, pIBridgeDesc);
   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
   Float64 overlay = 0;
   if (pDeck->WearingSurface == pgsTypes::wstOverlay)
   {
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType geomCtrlInterval = pIntervals->GetGeometryControlInterval();

      overlay = pBridge->GetOverlayDepth(geomCtrlInterval);

      if (pDeck->bInputAsDepthAndDensity == false)
      {
         INIT_UV_PROTOTYPE(rptLengthSectionValue, olay, pDisplayUnits->GetComponentDimUnit(), true);
         *pPara << _T("Overlay is defined by weight. Overlay depth assumed to be ") << olay.SetValue(overlay) << _T(".") << rptNewLine;
      }
   }

   //
   // Bridge Elevation Table
   //

   INIT_UV_PROTOTYPE(rptLengthSectionValue, webDim, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), true);

   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2(pBroker, IGirder, pGirder);
   GET_IFACE2(pBroker, IGeometry, pGeometry);
   GET_IFACE2(pBroker, IDeformedGirderGeometry, pDeformedGirderGeometry);

   RowIndexType row_step = 4; // number of rows used for each location (left,center,right)

   SpanIndexType nSpans = pBridge->GetSpanCount();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   SpanIndexType startSpanIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : pBridge->GetGirderGroupStartSpan(girderKey.groupIndex));
   SpanIndexType endSpanIdx = (girderKey.groupIndex == ALL_GROUPS ? nSpans - 1 : pBridge->GetGirderGroupEndSpan(girderKey.groupIndex));


   const int Left = 0;
   const int Center = 1;
   const int Right = 2;

   CComPtr<IDirection> direction;
   direction.CoCreateInstance(CLSID_Direction);

   LPCTSTR lpszLocation[] = { _T("Left"),_T("Center"),_T("Right") };

   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
   {
      PierIndexType startPierIdx = (PierIndexType)spanIdx;
      PierIndexType endPierIdx = startPierIdx + 1;
      CComPtr<IDirection> brgStart, brgEnd;
      pBridge->GetPierDirection(startPierIdx, &brgStart);
      pBridge->GetPierDirection(endPierIdx, &brgEnd);

      Float64 startBearing, endBearing;
      brgStart->get_Value(&startBearing);
      brgEnd->get_Value(&endBearing);

      Float64 brgStep = (endBearing - startBearing) / 11; // 10th points so use 11
      Float64 bearing = startBearing;
      direction->put_Value(bearing);

      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(spanIdx) << std::endl;
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(14, os.str().c_str());
      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(1);
      pTable->SetNumberOfStripedRows(row_step); // stripe in bands of row_step

      ColumnIndexType col = 0;
      for (col = 0; col < 3; col++)
      {
         pTable->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }

      col = 0;
      (*pTable)(0, col++) << _T("Girder");
      
      (*pTable)(0, col) << _T("Location");
      pTable->SetColumnSpan(0, col, 2);
      col += 2;

      (*pTable)(0, col++) << _T("CL Brg");
      (*pTable)(0, col++) << Sub2(_T("0.1L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.2L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.3L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.4L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.5L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.6L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.7L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.8L"), _T("s"));
      (*pTable)(0, col++) << Sub2(_T("0.9L"), _T("s"));
      (*pTable)(0, col++) << _T("CL Brg");

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGirders = pBridge->GetGirderCountBySpan(spanIdx);
      GirderIndexType startGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType endGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : startGirderIdx);

      for (GirderIndexType gdrIdx = startGirderIdx; gdrIdx <= endGirderIdx; gdrIdx++)
      {
         CSpanKey spanKey(spanIdx, gdrIdx);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(spanKey, POI_SPAN | POI_TENTH_POINTS, &vPoi);
         ATLASSERT(vPoi.size() == 11);

         GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
         CSegmentKey segmentKey(grpIdx, gdrIdx, 0);

         CComPtr<IDirection> segmentNormal;
         pBridge->GetSegmentNormal(segmentKey,&segmentNormal);

         CComPtr<IAngle> skewAngle;
         direction->AngleBetween(segmentNormal, &skewAngle);

         Float64 angle;
         skewAngle->get_Value(&angle);

         Float64 cos_angle = cos(angle);


         col = 0;

         pTable->SetRowSpan(row, col, Int16(3 * row_step));
         (*pTable)(row, col) << LABEL_GIRDER(gdrIdx);

         for (int i = 0; i < 3; i++) // left, center, right
         {
            col = 1;
            pTable->SetRowSpan(row + i*row_step, col, row_step);
            (*pTable)(row + i*row_step, col) << lpszLocation[i] << rptNewLine;

            col = 2;

            (*pTable)(row + i*row_step + 0, col) << Bold(_T("Station"));
            (*pTable)(row + i*row_step + 1, col) << Bold(_T("Offset (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
            (*pTable)(row + i*row_step + 2, col) << Bold(_T("Design Elev (PGL) (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
            (*pTable)(row + i*row_step + 3, col) << Bold(_T("Finished Elev (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
         }

         col = 3;

         for ( const auto& poi : vPoi)
         {
            // get parameters for design elevation

            // distance left/right of CL girder, measured normal to CL girder
            Float64 edge_offset[3] = { 0,0,0 };
            pGirder->GetTopWidth(poi, &edge_offset[Left], &edge_offset[Right]);

            // CL Girder location
            CComPtr<IPoint2d> point_on_cl_girder;
            pBridge->GetPoint(poi, pgsTypes::pcLocal, &point_on_cl_girder);

            // compute location of left and right edge, using the direction of the "cut line"
            CComPtr<IPoint2d> point_on_left_edge;
            pGeometry->ByDistDir(point_on_cl_girder, edge_offset[Left] / cos_angle, CComVariant(direction), 0, &point_on_left_edge);

            CComPtr<IPoint2d> point_on_right_edge;
            pGeometry->ByDistDir(point_on_cl_girder, -edge_offset[Right] / cos_angle, CComVariant(direction), 0, &point_on_right_edge);

            // get station and offset of left, center, and right locations
            std::array<Float64,3> station, offset;
            pAlignment->GetStationAndOffset(pgsTypes::pcLocal, point_on_left_edge, &station[Left], &offset[Left]);
            pBridge->GetStationAndOffset(poi, &station[Center], &offset[Center]);
            pAlignment->GetStationAndOffset(pgsTypes::pcLocal, point_on_right_edge, &station[Right], &offset[Right]);

            // get parameters for finished elevation... for no deck, the finished elevation is the top of the girder
            std::array<Float64,3> finished_elevation;
            pDeformedGirderGeometry->GetFinishedElevation(poi, direction, &finished_elevation[Left], &finished_elevation[Center], &finished_elevation[Right]);

            for (int i = 0; i < 3; i++) // left, center, right
            {
               Float64 design_elevation = pAlignment->GetElevation(station[i], offset[i]);

               (*pTable)(row + i*row_step + 0, col) << rptRcStation(station[i], &pDisplayUnits->GetStationFormat());
               (*pTable)(row + i*row_step + 1, col) << RPT_OFFSET(offset[i], dist);
               (*pTable)(row + i*row_step + 2, col) << dist.SetValue(design_elevation);

               // if finished elevation differs from design_elevation by more than tolerance, use red text for the finished_elevation
               rptRiStyle::FontColor color = IsEqual(design_elevation, finished_elevation[i], tolerance) ? rptRiStyle::Black : rptRiStyle::Red;
               rptRcColor* pColor = new rptRcColor(color);
               (*pTable)(row + i*row_step + 3, col) << pColor << dist.SetValue(finished_elevation[i]) << color(Black);
            }

            col++;
            bearing += brgStep;
            direction->put_Value(bearing);
         } // next poi

         row += 3 * row_step;
      } // next girder
   } // next span
}
