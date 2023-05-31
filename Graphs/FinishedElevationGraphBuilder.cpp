///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include <Graphs/FinishedElevationGraphBuilder.h>
#include <Graphs/DrawBeamTool.h>
#include <Graphs/ExportGraphXYTool.h>
#include "FinishedElevationGraphController.h"
#include "FinishedElevationGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <Units\UnitValueNumericalFormatTools.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <IFace\Alignment.h>
#include <IFace\EditByUI.h>
#include <IFace\AnalysisResults.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <MFCTools\MFCTools.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int PenWeight = GRAPH_PEN_WEIGHT;
static const int PglWeight = GRAPH_PEN_WEIGHT + 1; // heavier weight for PGL

BEGIN_MESSAGE_MAP(CFinishedElevationGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()

CFinishedElevationGraphBuilder::CFinishedElevationGraphBuilder() :
   CGirderGraphBuilderBase(),
   m_bShowPGL(TRUE),
   m_bShowFinishedDeck(FALSE),
   m_bShowFinishedDeckBottom(FALSE),
   m_bShowFinishedGirderBottom(FALSE),
   m_bShowFinishedGirderTop(FALSE),
   m_ShowGirderChord(FALSE),
   m_GraphType(gtElevations),
   m_GraphSide(gsCenterLine),
   m_ShowHaunchDepth(FALSE),
   m_Show10thPoints(FALSE),
   m_ShowElevationTolerance(FALSE)
{
   SetName(_T("Finished Elevations"));
   
   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_FINISHED_ELEVATION_VIEW);
}

CFinishedElevationGraphBuilder::~CFinishedElevationGraphBuilder()
{
}

BOOL CFinishedElevationGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != nullptr);
   return m_pGraphController->Create(pParent,IDD_FINISHED_ELEVATION_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

void CFinishedElevationGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CFinishedElevationGraphViewController>* pController;
   CComObject<CFinishedElevationGraphViewController>::CreateInstance(&pController);
   pController->Init((CFinishedElevationGraphController*)m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CFinishedElevationGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CFinishedElevationGraphBuilder>(*this);
}

void CFinishedElevationGraphBuilder::UpdateXAxis()
{
   __super::UpdateXAxis();
   m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
}

void CFinishedElevationGraphBuilder::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }

   m_Graph.YAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);
   m_Graph.PinYAxisAtZero(false);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   if (gtElevationDifferential == m_GraphType)
   {
      const WBFL::Units::LengthData& lengthUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new WBFL::Units::LengthTool(lengthUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);

      m_Graph.SetYAxisTitle(std::_tstring(_T("Elevation Differential (") + m_pYFormat->UnitTag() + _T(")")).c_str());
   }
   else
   {
      const WBFL::Units::LengthData& lengthUnit = pDisplayUnits->GetSpanLengthUnit();
      m_pYFormat = new WBFL::Units::LengthTool(lengthUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);

      m_Graph.SetYAxisTitle(std::_tstring(_T("Elevation (") + m_pYFormat->UnitTag() + _T(")")).c_str());
   }
}

bool CFinishedElevationGraphBuilder::HandleDoubleClick(UINT nFlags,CPoint point)
{
   GET_IFACE_NOCHECK(IBridge,pBridge);
   if (pgsTypes::sdtNone == pBridge->GetDeckType())
   {
      return true;
   }
   else
   {
      GET_IFACE(IEditByUI,pEditByUI);
      return pEditByUI->EditHaunch();
   }
}

CGirderGraphControllerBase* CFinishedElevationGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CFinishedElevationGraphController;
}

bool CFinishedElevationGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   __super::UpdateNow();

   // Update graph properties
   GroupIndexType    grpIdx      = m_pGraphController->GetGirderGroup();
   GirderIndexType   gdrIdx      = m_pGraphController->GetGirder();
   UpdateGraphTitle(grpIdx,gdrIdx);

   UpdateYAxis();

   // get data to graph
   UpdateGraphData(grpIdx,gdrIdx);

   return true;
}

void CFinishedElevationGraphBuilder::UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx)
{
   if (gtElevationDifferential == m_GraphType)
   {
      m_Graph.SetTitle(_T("Elevation Differentials from PGL"));
   }
   else
   {
      m_Graph.SetTitle(_T("Design and Finished Roadway Elevations"));
   }

   CString strGraphSubTitle;
   strGraphSubTitle.Format(_T("Group %d Girder %s"), LABEL_GROUP(grpIdx), LABEL_GIRDER(gdrIdx));
   if (gsLeftEdge == m_GraphSide)
   {
      strGraphSubTitle += _T(" - along Left Top Edge of Girder");
   }
   else if (gsCenterLine == m_GraphSide)
   {
      strGraphSubTitle += _T(" - along Centerline of Girder");
   }
   else
   {
      strGraphSubTitle += _T(" - along Right Top Edge of Girder");
   }

   m_Graph.SetSubtitle(strGraphSubTitle);
}

class MatchPoiOffSegment
{
public:
   MatchPoiOffSegment(IPointOfInterest* pIPointOfInterest) : m_pIPointOfInterest(pIPointOfInterest) {}
   bool operator()(const pgsPointOfInterest& poi) const
   {
      return m_pIPointOfInterest->IsOffSegment(poi);
   }

   IPointOfInterest* m_pIPointOfInterest;
};

void CFinishedElevationGraphBuilder::UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx)
{
   GET_IFACE_NOCHECK(IBridge,pBridge);
   GET_IFACE_NOCHECK(IGirder,pGirder);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE_NOCHECK(ISectionProperties,pSectProps);

   IntervalIndexType geomCtrlInterval = pIntervals->GetGeometryControlInterval();

   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pPoi);

   PoiList vPoi;

   if (Show10thPoints())
   {
      GroupIndexType startGrp,endGrp;
      if (grpIdx == ALL_GROUPS)
      {
         startGrp = 0;
         endGrp = pBridge->GetGirderGroupCount()-1;
      }
      else
      {
         startGrp = grpIdx;
         endGrp = grpIdx;
      }

      for (GroupIndexType iGrp = startGrp; iGrp <= endGrp; iGrp++)
      {
         SegmentIndexType numSegs = pBridge->GetSegmentCount(iGrp,gdrIdx);
         for (SegmentIndexType iSeg = 0; iSeg < numSegs; iSeg++)
         {
            CSegmentKey segmentKey(iGrp, gdrIdx, iSeg);

            // Remove first and last elements at segment since they are at support locations (not 10th points), and then add segment ends
            PoiList vPoi2;
            pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS,&vPoi2);
            vPoi2.pop_back();
            vPoi2.erase(vPoi2.begin());

            vPoi.insert(vPoi.end(),vPoi2.begin(), vPoi2.end());

            pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
         }
      }
   }
   else
   {
      CSegmentKey segmentKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS,&vPoi);
      pPoi->GetPointsOfInterest(segmentKey,POI_CLOSURE,&vPoi);
      pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
      pPoi->GetPointsOfInterest(segmentKey,POI_SUPPORTS,&vPoi);
      // We can't compute accurate elevations within closure joints. Get rid of any pois off segments
      vPoi.erase(std::remove_if(vPoi.begin(),vPoi.end(),MatchPoiOffSegment(pPoi)),std::end(vPoi));
   }
   pPoi->SortPoiList(&vPoi); // sorts and removes duplicates

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);

   // Tackle simpler stuff first
   // Most graphs will want the PGL, so just store it
   GET_IFACE(IRoadway,pRoadway);
   std::vector<Float64> PglElevations;
   PglElevations.reserve(vPoi.size());
   for (auto poi : vPoi)
   {
      Float64 station,offset;
      pBridge->GetStationAndOffset(poi,&station,&offset);

      Float64 deltaOffset(0.0);
      if (m_GraphSide != gsCenterLine)
      {
         Float64 leftEdge,rightEdge;
         pGirder->GetTopWidth(poi,&leftEdge,&rightEdge);
         deltaOffset = m_GraphSide == gsLeftEdge ? leftEdge : -1.0 * rightEdge;
      }

      Float64 elev = pRoadway->GetElevation(station,offset+deltaOffset);
      PglElevations.push_back(elev);
   }

   if (m_bShowPGL)
   {
      CString strRWLabel(_T("Design Roadway (PGL)"));
      IndexType dataRwSeries = m_Graph.CreateDataSeries(strRWLabel,PS_SOLID,PglWeight,BLUE);

      auto iter(PglElevations.begin());
      auto end(PglElevations.end());
      auto xIter(xVals.begin());
      for (; iter != end; iter++,xIter++)
      {
         Float64 elev = *iter;
         Float64 X = *xIter;
         AddGraphPoint(dataRwSeries,X,elev);
      }
   }

   WBFL::Graphing::GraphColor graphColor;
   if (m_ShowGirderChord)
   {
      CString strGCLabel(_T("Girder CL Top Chord"));
      IndexType dataGcSeries = m_Graph.CreateDataSeries(strGCLabel,PS_DASH,PenWeight,GREEN);
      auto xIter(xVals.begin());
      auto pglIter(PglElevations.begin());
      for (auto poi : vPoi)
      {
         // Chord is always at CL girder
         Float64 girder_chord_elevation = pGirder->GetTopGirderChordElevation(poi);
         Float64 X = *xIter++;

         if (gtElevationDifferential == m_GraphType)
         {
            Float64 pglElev = *pglIter++;
            AddGraphPoint(dataGcSeries,X,girder_chord_elevation-pglElev);
         }
         else
         {
            AddGraphPoint(dataGcSeries,X,girder_chord_elevation);
         }
      }
   }

   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();

   if (ShowElevationTolerance() && gtElevationDifferential == m_GraphType && haunchInputType != pgsTypes::hidACamber)
   {
      CString strGCLabel(_T("Elevation Tolerance"));
      IndexType dataGcSeriesPos = m_Graph.CreateDataSeries(strGCLabel,PS_DASH,PglWeight,BLUE);
      IndexType dataGcSeriesNeg = m_Graph.CreateDataSeries(_T(""),PS_DASH,PglWeight,BLUE);

      GET_IFACE(ILibrary,pLib);
      GET_IFACE(ISpecification,pSpec);
      std::_tstring spec_name = pSpec->GetSpecification();
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
      Float64 tolerance = pSpecEntry->GetFinishedElevationTolerance();

      auto xIter(xVals.begin());
      for (auto poi : vPoi)
      {
         AddGraphPoint(dataGcSeriesPos,*xIter, tolerance);
         AddGraphPoint(dataGcSeriesNeg,*xIter++, -tolerance);
      }
   }

   // Don't compute values unless needed
   std::vector<IntervalIndexType> vIntervals = ((CMultiIntervalGirderGraphControllerBase*)m_pGraphController)->GetSelectedIntervals();

   if ((m_bShowFinishedDeck || m_bShowFinishedDeckBottom || m_bShowFinishedGirderBottom || m_bShowFinishedGirderTop || ShowHaunchDepth())
       && !vIntervals.empty())
   {
      IntervalIndexType deckCastInterval = pIntervals->GetLastCastDeckInterval();

      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

      GET_IFACE_NOCHECK(IDeformedGirderGeometry,pDeformedGirderGeometry);
      if (deckType == pgsTypes::sdtNone)
      {
         // No deck
         for (auto intervalIdx : vIntervals)
         {
            // Need this data for all possible graphs
            std::vector<Float64> finishedDeckElevations;
            finishedDeckElevations.reserve(vPoi.size());
            for (const auto& poi : vPoi)
            {
               Float64 lftElev,clElev,rgtElev;
               pDeformedGirderGeometry->GetFinishedElevation(poi,nullptr,&lftElev,&clElev,&rgtElev);

               if (m_GraphSide == gsCenterLine)
               {
                  finishedDeckElevations.push_back(clElev);
               }
               else if (m_GraphSide == gsLeftEdge)
               {
                  finishedDeckElevations.push_back(lftElev);
               }
               else
               {
                  finishedDeckElevations.push_back(rgtElev);
               }
            }

            if (m_bShowFinishedDeck)
            {
               COLORREF color = graphColor.GetColor(intervalIdx+36);

               CString strLabel;
               strLabel.Format(_T("Finished Deck - Interval %d"),LABEL_INTERVAL(intervalIdx));
               IndexType fdDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

               auto iter(finishedDeckElevations.begin());
               auto end(finishedDeckElevations.end());
               auto xIter(xVals.begin());
               auto pglIter(PglElevations.begin());
               for (; iter != end; iter++,xIter++)
               {
                  Float64 x = *xIter;
                  Float64 elev = *iter;
                  if (gtElevationDifferential == m_GraphType)
                  {
                     Float64 pglElev = *pglIter++;
                     AddGraphPoint(fdDataSeries,x,elev - pglElev);
                  }
                  else
                  {
                     AddGraphPoint(fdDataSeries,x,elev);
                  }
               }
            }

            if (m_bShowFinishedDeckBottom)
            {
               ATLASSERT(0); // there is no deck
            }

            if (m_bShowFinishedGirderTop)
            {
               ATLASSERT(0); // there is no deck
            }

            if (m_bShowFinishedGirderBottom)
            {
               COLORREF color = graphColor.GetColor(intervalIdx + 252);

               CString strLabel;
               strLabel.Format(_T("Bottom of Girder - Interval %d"),LABEL_INTERVAL(intervalIdx));
               IndexType fdbDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

               auto iter(vPoi.begin());
               auto end(vPoi.end());
               auto xIter(xVals.begin());
               auto fditer(finishedDeckElevations.begin());
               auto pglIter(PglElevations.begin());
               for (; iter != end; iter++,xIter++,fditer++)
               {
                  const pgsPointOfInterest& poi = *iter;
                  Float64 x = *xIter;
                  Float64 fdelev = *fditer;
                  Float64 gdrDepth = pGirder->GetHeight(poi);
                  Float64 elev = fdelev - gdrDepth;

                  if (gtElevationDifferential == m_GraphType)
                  {
                     Float64 pglElev = *pglIter++;
                     AddGraphPoint(fdbDataSeries,x,elev - pglElev);
                  }
                  else
                  {
                     AddGraphPoint(fdbDataSeries,x,elev);
                  }
               }
            }
         } // next interval
      }
      else if (haunchInputType == pgsTypes::hidACamber)
      {
         // Haunch defined using "A"
         ATLASSERT(vIntervals.front() == geomCtrlInterval && vIntervals.size()==1); // only interval we can deal with for "A" input
         IntervalIndexType intervalIdx = geomCtrlInterval;

         if (m_bShowFinishedDeck)
         {
            // Height of finished deck always matches the PGL for "A" case
            COLORREF color = graphColor.GetColor(intervalIdx);

            CString strLabel;
            strLabel.Format(_T("Finished Deck"));
            IndexType fdDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

            auto xIter(xVals.begin());
            for (auto pglIter=PglElevations.begin(); pglIter != PglElevations.end(); pglIter++,xIter++)
            {
               Float64 pgl = *pglIter;
               Float64 x = *xIter;

               if (gtElevationDifferential == m_GraphType)
               {
                  AddGraphPoint(fdDataSeries,x,0.0);
               }
               else
               {
                  AddGraphPoint(fdDataSeries,x,pgl);
               }
            }
         }

         // top of overlay is at finished deck elev if it's applied before the gce
         Float64 overlay = pBridge->GetOverlayDepth(geomCtrlInterval);

         std::vector<Float64> DeckBottomElevations;
         DeckBottomElevations.reserve(vPoi.size());
         auto iterp(vPoi.begin());
         auto endp(vPoi.end());
         auto pglIter(PglElevations.begin());
         for (; iterp != endp; iterp++,pglIter++)
         {
            Float64 tdeck = pBridge->GetGrossSlabDepth(*iterp);
            Float64 botelev = *pglIter - tdeck - overlay;
            DeckBottomElevations.push_back(botelev);
         }

         if (m_bShowFinishedDeckBottom)
         {
            COLORREF color = graphColor.GetColor(intervalIdx + 72);

            CString strLabel(_T("Bottom of Finished Deck"));
            IndexType fdDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

            auto dbiter(DeckBottomElevations.begin());
            auto dbend(DeckBottomElevations.end());
            auto xIter(xVals.begin());
            auto pglIter(PglElevations.begin());
            for (; dbiter != dbend; dbiter++,xIter++,pglIter++)
            {
               Float64 x = *xIter;
               Float64 elevBotDeck = *dbiter;
               if (gtElevationDifferential == m_GraphType)
               {
                  Float64 pglElev = *pglIter;
                  AddGraphPoint(fdDataSeries,x,elevBotDeck - pglElev);
               }
               else
               {
                  AddGraphPoint(fdDataSeries,x,elevBotDeck);
               }
            }
         }

         std::vector<Float64> TopGirderElevations;
         TopGirderElevations.reserve(vPoi.size());
         for (const auto& poi : vPoi)
         {
            Float64 eLeft,eCenter,eRight;
            pDeformedGirderGeometry->GetTopGirderElevation(poi,nullptr,&eLeft,&eCenter,&eRight);
            if (m_GraphSide == gsCenterLine)
            {
               TopGirderElevations.push_back(eCenter);
            }
            else if (m_GraphSide == gsLeftEdge)
            {
               TopGirderElevations.push_back(eLeft);
            }
            else
            {
               TopGirderElevations.push_back(eRight);
            }
         }

         std::vector<Float64> HaunchDepths;
         HaunchDepths.reserve(vPoi.size());
         auto dbiter(DeckBottomElevations.begin());
         auto dbend(DeckBottomElevations.end());
         auto tgIter(TopGirderElevations.begin());
         for (; dbiter != dbend; dbiter++,tgIter++)
         {
            Float64 deckBottom = *dbiter;
            Float64 topGirder = *tgIter;
            Float64 haunch = deckBottom - topGirder;
            HaunchDepths.push_back( haunch );
         }

         if(ShowHaunchDepth())
         {
            CString strGCLabel(_T("Haunch Depth")); // always at CL
            IndexType dataGcSeries = m_Graph.CreateDataSeries(strGCLabel,PS_DASHDOT,PenWeight,SALMON4);
            auto xIter(xVals.begin());
            for (auto haunch_depth : HaunchDepths)
            {
               AddGraphPoint(dataGcSeries,*xIter++,haunch_depth);
            }
         }

         if (m_bShowFinishedGirderTop || m_bShowFinishedGirderBottom)
         {
            if (m_bShowFinishedGirderTop)
            {
               COLORREF color = graphColor.GetColor(intervalIdx + 108);

               CString strLabel;
               strLabel.Format(_T("Top of Girder - Interval %d"),LABEL_INTERVAL(intervalIdx));
               IndexType fdbDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

               auto xIter(xVals.begin());
               auto pglIter(PglElevations.begin());
               auto tgiter(TopGirderElevations.begin());
               auto tgend(TopGirderElevations.end());
               for (; tgiter != tgend; tgiter++,xIter++)
               {
                  Float64 x = *xIter;
                  Float64 elev = *tgiter;
                  if (gtElevationDifferential == m_GraphType)
                  {
                     Float64 pglElev = *pglIter++;
                     AddGraphPoint(fdbDataSeries,x,elev - pglElev);
                  }
                  else
                  {
                     AddGraphPoint(fdbDataSeries,x,elev);
                  }
               }
            }

            if (m_bShowFinishedGirderBottom)
            {
               COLORREF color = graphColor.GetColor(intervalIdx + 144);

               CString strLabel;
               strLabel.Format(_T("Bottom of Girder - Interval %d"),LABEL_INTERVAL(intervalIdx));
               IndexType fdbDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

               auto iter(vPoi.begin());
               auto end(vPoi.end());
               auto xIter(xVals.begin());
               auto tgiter(TopGirderElevations.begin());
               auto pglIter(PglElevations.begin());
               for (; iter != end; iter++,xIter++,tgiter++)
               {
                  const pgsPointOfInterest& poi = *iter;
                  Float64 x = *xIter;
                  Float64 tgelev = *tgiter;

                  Float64 gdrDepth = pGirder->GetHeight(poi);
                  Float64 elev = tgelev - gdrDepth;

                  if (gtElevationDifferential == m_GraphType)
                  {
                     Float64 pglElev = *pglIter++;
                     AddGraphPoint(fdbDataSeries,x,elev - pglElev);
                  }
                  else
                  {
                     AddGraphPoint(fdbDataSeries,x,elev);
                  }
               }
            }
         }
      }
      else
      {
         // Direct haunch input. For this case, we build elevations from top of girder up/down

         if (ShowHaunchDepth()) // not interval-dependent
         {
            CString strGCLabel(_T("Haunch Depth")); // always at CL
            IndexType dataGcSeries = m_Graph.CreateDataSeries(strGCLabel,PS_DASHDOT,PenWeight,SALMON4);
            auto xIter(xVals.begin());
            for (auto poi : vPoi)
            {
               Float64 haunch_depth = pSectProps->GetStructuralHaunchDepth(poi,pgsTypes::hspDetailedDescription);
               AddGraphPoint(dataGcSeries,*xIter++,haunch_depth);
            }
         }

         for (auto intervalIdx : vIntervals)
         {
            // Need this data for all possible graphs
            std::vector<Float64> topGirderElevations;
            topGirderElevations.reserve(vPoi.size());
            std::vector<Float64> haunchDepths;
            haunchDepths.reserve(vPoi.size());
            for (const auto& poi : vPoi)
            {
               Float64 lftHaunch,clHaunch,rgtHaunch;
               Float64 finished_elevation_cl = pDeformedGirderGeometry->GetFinishedElevation(poi,intervalIdx,&lftHaunch,&clHaunch,&rgtHaunch);

               Float64 tgLeft,tgCL,tgRight;
               pDeformedGirderGeometry->GetTopGirderElevationEx(poi,intervalIdx,nullptr,&tgLeft,&tgCL,&tgRight);

               if (m_GraphSide == gsCenterLine)
               {
                  topGirderElevations.push_back(tgCL);
                  haunchDepths.push_back(clHaunch);
               }
               else if (m_GraphSide == gsLeftEdge)
               {
                  topGirderElevations.push_back(tgLeft);
                  haunchDepths.push_back(lftHaunch);
               }
               else
               {
                  topGirderElevations.push_back(tgRight);
                  haunchDepths.push_back(rgtHaunch);
               }
            }

            if (m_bShowFinishedDeck || m_bShowFinishedDeckBottom)
            {
               int penType = intervalIdx >= deckCastInterval ? PS_SOLID : PS_DOT; // show "shadow" deck if drawn before deck is cast

               if (m_bShowFinishedDeck) // top
               {
                  Float64 overlayDepth = pBridge->GetOverlayDepth(geomCtrlInterval); // must add overlay from GCE, not current interval

                  COLORREF color = graphColor.GetColor(intervalIdx);

                  CString strLabel;
                  strLabel.Format(_T("Finished Deck - Interval %d"),LABEL_INTERVAL(intervalIdx));
                  IndexType fdDataSeries = m_Graph.CreateDataSeries(strLabel,penType,PenWeight,color);

                  auto iter(topGirderElevations.begin());
                  auto end(topGirderElevations.end());
                  auto poiIter(vPoi.begin());
                  auto xIter(xVals.begin());
                  auto hdIter(haunchDepths.begin());
                  auto pglIter(PglElevations.begin());
                  for (; iter != end; iter++,poiIter++,xIter++,hdIter++)
                  {
                     Float64 x = *xIter;
                     Float64 tgelev = *iter;
                     Float64 haunchDepth = *hdIter;
                     const auto& poi = *poiIter;
                  Float64 deckDepth = pBridge->GetGrossSlabDepth(poi);
                     Float64 elev = tgelev + haunchDepth + deckDepth + overlayDepth;
                     if (gtElevationDifferential == m_GraphType)
                     {
                        Float64 pglElev = *pglIter++;
                        AddGraphPoint(fdDataSeries,x,elev - pglElev);
                     }
                     else
                     {
                        AddGraphPoint(fdDataSeries,x,elev);
                     }
                  }
               }

               if (m_bShowFinishedDeckBottom)
               {
                  COLORREF color = graphColor.GetColor(intervalIdx + 144);

                  CString strLabel;
                  strLabel.Format(_T("Finished Deck Bottom - Interval %d"),LABEL_INTERVAL(intervalIdx));
                  IndexType fdbDataSeries = m_Graph.CreateDataSeries(strLabel,penType,PenWeight,color);

                  auto iter(topGirderElevations.begin());
                  auto end(topGirderElevations.end());
                  auto poiIter(vPoi.begin());
                  auto xIter(xVals.begin());
                  auto hdIter(haunchDepths.begin());
                  auto pglIter(PglElevations.begin());
                  for (; iter != end; iter++,poiIter++,xIter++,hdIter++)
                  {
                     Float64 x = *xIter;
                     Float64 tgelev = *iter;
                     Float64 haunchDepth = *hdIter;
                     const auto& poi = *poiIter;
                     Float64 elev = tgelev + haunchDepth;
                     if (gtElevationDifferential == m_GraphType)
                     {
                        Float64 pglElev = *pglIter++;
                        AddGraphPoint(fdbDataSeries,x,elev - pglElev);
                     }
                     else
                     {
                        AddGraphPoint(fdbDataSeries,x,elev);
                     }
                  }
               }
            }

            if (m_bShowFinishedGirderTop)
            {
               COLORREF color = graphColor.GetColor(intervalIdx+180);

               CString strLabel;
               strLabel.Format(_T("Top of Girder - Interval %d"),LABEL_INTERVAL(intervalIdx));
               IndexType fdbDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

               auto iter(topGirderElevations.begin());
               auto end(topGirderElevations.end());
               auto xIter(xVals.begin());
               auto pglIter(PglElevations.begin());
               for (; iter != end; iter++,xIter++)
               {
                  Float64 elev = *iter;
                  Float64 x = *xIter;

                  if (gtElevationDifferential == m_GraphType)
                  {
                     Float64 pglElev = *pglIter++;
                     AddGraphPoint(fdbDataSeries,x, elev - pglElev);
                  }
                  else
                  {
                     AddGraphPoint(fdbDataSeries,x,elev);
                  }
               }
            }

            if (m_bShowFinishedGirderBottom)
            {
               COLORREF color = graphColor.GetColor(intervalIdx+216);

               CString strLabel;
               strLabel.Format(_T("Bottom of Girder - Interval %d"),LABEL_INTERVAL(intervalIdx));
               IndexType fdbDataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,PenWeight,color);

               auto iter(vPoi.begin());
               auto end(vPoi.end());
               auto xIter(xVals.begin());
               auto tgiter(topGirderElevations.begin());
               auto pglIter(PglElevations.begin());
               for (; iter != end; iter++,xIter++,tgiter++)
               {
                  const pgsPointOfInterest& poi = *iter;
                  Float64 x = *xIter;
                  Float64 tgelev = *tgiter;
                  Float64 gdrDepth = pGirder->GetHeight(poi);
                  Float64 elev = tgelev - gdrDepth;

                  if (gtElevationDifferential == m_GraphType)
                  {
                     Float64 pglElev = *pglIter++;
                     AddGraphPoint(fdbDataSeries,x, elev - pglElev);
                  }
                  else
                  {
                     AddGraphPoint(fdbDataSeries,x,elev);
                  }
               }
            }
         } // next interval
      }
   }
}


void CFinishedElevationGraphBuilder::GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx)
{
   CFinishedElevationGraphController* pMyGraphController = (CFinishedElevationGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());
   if (0 < vIntervals.size())
   {
      *pFirstIntervalIdx = vIntervals.front();
      *pLastIntervalIdx = vIntervals.back();
   }
   else
   {
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetLastCastDeckInterval();
      *pFirstIntervalIdx = intervalIdx;
      *pLastIntervalIdx = *pFirstIntervalIdx;
   }
}

void CFinishedElevationGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph,rstrDefaultFileName);
}

void CFinishedElevationGraphBuilder::ShowFinishedDeck(BOOL show)
{
   m_bShowFinishedDeck = show;
}

BOOL CFinishedElevationGraphBuilder::ShowFinishedDeck() const
{
   return m_bShowFinishedDeck;
}

void CFinishedElevationGraphBuilder::ShowFinishedDeckBottom(BOOL show)
{
   m_bShowFinishedDeckBottom = show;
}

BOOL CFinishedElevationGraphBuilder::ShowFinishedDeckBottom() const
{
   return m_bShowFinishedDeckBottom;
}

void CFinishedElevationGraphBuilder::ShowPGL(BOOL show)
{
   m_bShowPGL = show;
}

BOOL CFinishedElevationGraphBuilder::ShowPGL() const
{
   return m_bShowPGL;
}

void CFinishedElevationGraphBuilder::ShowFinishedGirderBottom(BOOL show)
{
   m_bShowFinishedGirderBottom = show;
}

BOOL CFinishedElevationGraphBuilder::ShowFinishedGirderBottom() const
{
   return m_bShowFinishedGirderBottom;
}

void CFinishedElevationGraphBuilder::ShowFinishedGirderTop(BOOL show)
{
   m_bShowFinishedGirderTop = show;
}

BOOL CFinishedElevationGraphBuilder::ShowFinishedGirderTop() const
{
   return m_bShowFinishedGirderTop;
}

void CFinishedElevationGraphBuilder::ShowGirderChord(BOOL show)
{
   m_ShowGirderChord = show;
}

BOOL CFinishedElevationGraphBuilder::ShowGirderChord() const
{
   return m_ShowGirderChord;
}

void CFinishedElevationGraphBuilder::ShowHaunchDepth(BOOL show)
{
   m_ShowHaunchDepth = show;
}

BOOL CFinishedElevationGraphBuilder::ShowHaunchDepth() const
{
   if (gtElevationDifferential == m_GraphType)
   {
      return m_ShowHaunchDepth;
   }
   else
   {
      return FALSE;
   }
}

void CFinishedElevationGraphBuilder::Show10thPoints(BOOL show)
{
   m_Show10thPoints = show;
}

BOOL CFinishedElevationGraphBuilder::Show10thPoints() const
{
   return m_Show10thPoints;
}

void CFinishedElevationGraphBuilder::ShowElevationTolerance(BOOL show)
{
   m_ShowElevationTolerance = show;
}

BOOL CFinishedElevationGraphBuilder::ShowElevationTolerance() const
{
   return m_ShowElevationTolerance;
}

void CFinishedElevationGraphBuilder::SetGraphType(GraphType type)
{
   m_GraphType = type;
}

CFinishedElevationGraphBuilder::GraphType CFinishedElevationGraphBuilder::GetGraphType() const
{
   return m_GraphType;
}

void CFinishedElevationGraphBuilder::SetGraphSide(GraphSide side)
{
   m_GraphSide = side;
}

CFinishedElevationGraphBuilder::GraphSide CFinishedElevationGraphBuilder::GetGraphSide() const
{
   return m_GraphSide;
}
