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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\DrawBeamTool.h>

#include <PgsExt\ClosurePourData.h>

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PGSuperColors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDrawBeamTool::CDrawBeamTool()
{
}

CDrawBeamTool::~CDrawBeamTool()
{
}

void CDrawBeamTool::DrawBeam(IBroker* pBroker,CDC* pDC,Float64 graphStartOffset,grlibPointMapper mapper,arvPhysicalConverter* pUnitConverter,IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   m_pBroker = pBroker;
   m_pUnitConverter = pUnitConverter;
   m_GraphStartOffset = graphStartOffset;
   m_GirderKey = girderKey;

   GET_IFACE(IIntervals,pIntervals);

   IntervalIndexType castSegmentInterval = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IGirderSegment,pGirderSegment);
   GET_IFACE(IClosurePour,pClosurePour);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IPointOfInterest,pPoi);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType groupIdx      = girderKey.groupIndex;
   GroupIndexType startGroupIdx = (groupIdx == ALL_GROUPS ? 0 : groupIdx);
   GroupIndexType nGroups       = (groupIdx == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount() : 1);
   GroupIndexType endGroupIdx   = startGroupIdx + nGroups - 1;

   PierIndexType startPierIdx = pBridgeDesc->GetGirderGroup(startGroupIdx)->GetPier(pgsTypes::metStart)->GetIndex();

   GirderIndexType girder = girderKey.girderIndex;

   //
   // re-configure mapper so beam height draws with same scale as beam length
   //

   // get device point at World (0,0)
   LONG x,y;
   mapper.WPtoDP(0,0,&x,&y);

   // Get world extents and world origin
   gpSize2d wExt  = mapper.GetWorldExt();
   gpPoint2d wOrg = mapper.GetWorldOrg();
   
   // get device extents and device origin
   LONG dx,dy;
   mapper.GetDeviceExt(&dx,&dy);

   LONG dox,doy;
   mapper.GetDeviceOrg(&dox,&doy);

   // compute a new device origin in the y direction
   doy = (LONG)Round(y - (0.0 - wOrg.Y())*((Float64)(dx)/-wExt.Dx()));

   // change the y scaling (use same scaling as X direction)
   mapper.SetWorldExt(wExt.Dx(),wExt.Dx());
   mapper.SetDeviceExt(dx,dx);

   // change the device origin
   mapper.SetDeviceOrg(dox,doy);

   // beam will now draw to scale

   //
   // define brushes and pens
   //
   CBrush segment_brush(SEGMENT_FILL_COLOR);
   CPen segment_pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush ghost_brush(SEGMENT_FILL_GHOST_COLOR);

   CBrush closure_brush(CLOSURE_FILL_COLOR);
   CPen closure_pen(PS_SOLID,1,CLOSURE_BORDER_COLOR);


   CBrush* pOldBrush = pDC->SelectObject(&segment_brush);
   CPen* pOldPen     = pDC->SelectObject(&segment_pen);

#pragma Reminder("UPDATE: printing sizes should be based on printer properties")
   if (pDC->IsPrinting())
      m_SupportSize = CSize(20,20);
   else
      m_SupportSize = CSize(5,5);


   // distance from origin of girder coordinate system to origin of graph
   // origin of graph is at the centerline of bearing of the first segment drawn
   Float64 segToGraphCoordinateAdjustment = m_GraphStartOffset;
   Float64 gdrPathToGraphCoordinateAdjustment = m_GraphStartOffset;

   EventIndexType eventIdx = pIntervals->GetStartEvent(intervalIdx);
   for ( GroupIndexType grpIdx = 0; grpIdx < startGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = min(girder,nGirders-1);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey(grpIdx,gdrIdx);
      gdrPathToGraphCoordinateAdjustment += pBridge->GetGirderLayoutLength(girderKey);
   }

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      // deal with girder index when there are different number of girders in each group
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = min(girder,nGirders-1);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey(grpIdx,gdrIdx);

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
         SegmentIDType segID = pSegment->GetID();

         //
         // Draw Segment
         //

         // Get segment profile, don't include shape in the closure pour region.
         // Coordinates are in the girder path coordinate system.
         CComPtr<IShape> shape;
         pGirderSegment->GetSegmentProfile(segmentKey,false,&shape);

         // map the points into the logical space of the graph
         CComPtr<IPoint2dCollection> points;
         shape->get_PolyPoints(&points); // these points are in girder path coordinates

         CollectionIndexType nPoints;
         points->get_Count(&nPoints);

         CPoint* pnts = new CPoint[nPoints];
         for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
         {
            CComPtr<IPoint2d> pnt;
            points->get_Item(idx,&pnt);

            Float64 x,y;
            pnt->Location(&x,&y);

            Float64 X = m_pUnitConverter->Convert(x+gdrPathToGraphCoordinateAdjustment);
            Float64 Y = m_pUnitConverter->Convert(y); // use the X-converter so the height of the beam is scaled the same as the length

            mapper.WPtoDP(X,Y,&pnts[idx].x,&pnts[idx].y);
         }

         if ( pTimelineMgr->IsSegmentErected(segID,eventIdx) )
         {
            pDC->SelectObject( &segment_brush );
         }
         else
         {
            pDC->SelectObject( &ghost_brush );
         }

         pDC->SelectObject(&segment_pen);
         pDC->Polygon(pnts,(int)nPoints);
         delete[] pnts;

         //
         // Draw Closure Pour
         //
         if ( segIdx < nSegments-1 && pIntervals->GetCastClosurePourInterval(segmentKey) <= intervalIdx )
         {
            CComPtr<IShape> shape;
            pClosurePour->GetClosurePourProfile(segmentKey,&shape);

            CComPtr<IPoint2dCollection> points;
            shape->get_PolyPoints(&points); // these points are in girder path coordinates

            CollectionIndexType nPoints;
            points->get_Count(&nPoints);

            CPoint* pnts = new CPoint[nPoints];
            for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
            {
               CComPtr<IPoint2d> pnt;
               points->get_Item(idx,&pnt);

               Float64 x,y;
               pnt->Location(&x,&y);

               Float64 X = m_pUnitConverter->Convert(x+gdrPathToGraphCoordinateAdjustment);
               Float64 Y = m_pUnitConverter->Convert(y); // use the X-converter so the height of the beam is scaled the same as the length

               mapper.WPtoDP(X,Y,&pnts[idx].x,&pnts[idx].y);
            }

            if ( pTimelineMgr->GetCastClosurePourEventIndex(segID) <= eventIdx )
            {
               pDC->SelectObject( &closure_brush );
            }
            else
            {
               pDC->SelectObject( &ghost_brush );
            }

            pDC->SelectObject( &closure_pen );
            pDC->Polygon(pnts,(int)nPoints);
            delete[] pnts;
         }

         //
         // Draw Supports
         //
         DrawSegmentEndSupport(segToGraphCoordinateAdjustment,pSegment,pgsTypes::metStart,intervalIdx,pTimelineMgr,mapper,pDC);
         DrawSegmentEndSupport(segToGraphCoordinateAdjustment,pSegment,pgsTypes::metEnd,intervalIdx,pTimelineMgr,mapper,pDC);
         DrawIntermediatePier(gdrPathToGraphCoordinateAdjustment,pSegment,intervalIdx,pTimelineMgr,mapper,pDC);
         DrawIntermediateTemporarySupport(gdrPathToGraphCoordinateAdjustment,pSegment,intervalIdx,pTimelineMgr,mapper,pDC);

         segToGraphCoordinateAdjustment += pBridge->GetSegmentLayoutLength(segmentKey);
      } // end of segment loop

      gdrPathToGraphCoordinateAdjustment += pBridge->GetGirderLayoutLength(girderKey);
   } // end of group loop

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

void CDrawBeamTool::DrawSegmentEndSupport(Float64 segToGraphCoordinateAdjustment,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,IntervalIndexType intervalIdx,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC)
{
   // Draws the support icon at one end of a segment

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);

   const CClosurePourData* pClosure = (endType == pgsTypes::metStart ? pSegment->GetLeftClosure() : pSegment->GetRightClosure());
   const CPierData2* pPier = NULL;
   const CTemporarySupportData* pTS = NULL;

   if ( pClosure )
   {
      pPier = pClosure->GetPier();
      pTS   = pClosure->GetTemporarySupport();
   }
   else
   {
      const CSpanData2* pSpan = pSegment->GetSpan(endType);
      pPier = (endType == pgsTypes::metStart ? pSpan->GetPrevPier() : pSpan->GetNextPier());
   }

   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   SegmentIndexType nSegments = pSegment->GetGirder()->GetSegmentCount();

   // Check to see if this segment ends at a pier and a new group starts
   // on the other side of the pier or the segment starts at a pier and
   // a previous group ends on the other side of the pier
   if ( (segmentKey.segmentIndex == nSegments-1 && // last segment in the girder
         endType == pgsTypes::metEnd            && // support at right end of segment
         pPier->GetNextGirderGroup() != NULL)      // not the last group
         &&                                     // AND
         (compositeDeckIntervalIdx <= intervalIdx &&
         pPier && pPier->IsContinuous())

         || // - OR -

         (segmentKey.segmentIndex == 0 && // first segment in the girder
         endType == pgsTypes::metStart && // support at left end of the segment
         pPier->GetPrevGirderGroup() != NULL) // not the first group
         && // AND
         (compositeDeckIntervalIdx <= intervalIdx &&
         pPier && pPier->IsContinuous())

         || // - OR -

         (
         compositeDeckIntervalIdx <= intervalIdx &&
         pPier && pPier->IsInteriorPier()
         )
      )
   {
      // It does!... draw a single pier at the CL Pier
      PierIndexType pierIdx = pPier->GetIndex();

      GET_IFACE(ISectionProperties,pSectProp);
      Float64 sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);

      GET_IFACE(IBridge,pBridge);
      Float64 x; // location of pier in segment coordinates
      bool bResult = pBridge->GetPierLocation(pierIdx,segmentKey,&x);
      ATLASSERT(bResult == true);

      // Bridge is continuous in this interval so draw pier symbol at CL Pier
      CPoint p;
      Float64 X = m_pUnitConverter->Convert(x + segToGraphCoordinateAdjustment);
      Float64 H = m_pUnitConverter->Convert(sectionHeight);
      mapper.WPtoDP(X,-H,&p.x,&p.y);

      DrawPier(intervalIdx,pPier,pTimelineMgr,p,pDC);

      Float64 brgOffset = 0;
      Float64 endDist = 0;
      if ( endType == pgsTypes::metStart )
      {
         brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         endDist   = pBridge->GetSegmentStartEndDistance(segmentKey);
      }
      else
      {
         brgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
         endDist   = pBridge->GetSegmentEndEndDistance(segmentKey);
      }
      Float64 left_offset = brgOffset - endDist;

      if ( endType == pgsTypes::metStart )
      {
         CSegmentKey prevSegmentKey(segmentKey);
         GirderIndexType nGirdersPrevGroup = pPier->GetPrevGirderGroup()->GetGirderCount();
         if ( nGirdersPrevGroup <= segmentKey.girderIndex )
            prevSegmentKey.girderIndex = nGirdersPrevGroup-1;

         brgOffset = pBridge->GetSegmentEndBearingOffset(prevSegmentKey);
         endDist   = pBridge->GetSegmentEndEndDistance(prevSegmentKey);
      }
      else
      {
         CSegmentKey nextSegmentKey(segmentKey);
         GirderIndexType nGirdersNextGroup = pPier->GetNextGirderGroup()->GetGirderCount();
         if ( nGirdersNextGroup <= segmentKey.girderIndex )
            nextSegmentKey.girderIndex = nGirdersNextGroup-1;

         brgOffset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
         endDist   = pBridge->GetSegmentStartEndDistance(nextSegmentKey);
      }
      Float64 right_offset = brgOffset - endDist;

      left_offset  = m_pUnitConverter->Convert(left_offset);
      right_offset = m_pUnitConverter->Convert(right_offset);

      CPoint points[4];
      mapper.WPtoDP(X-left_offset, -H,&points[0].x,&points[0].y);
      mapper.WPtoDP(X-left_offset,  0,&points[1].x,&points[1].y);
      mapper.WPtoDP(X+right_offset, 0,&points[2].x,&points[2].y);
      mapper.WPtoDP(X+right_offset,-H,&points[3].x,&points[3].y);

      CBrush pier_brush(CLOSURE_FILL_COLOR);
      CPen pier_pen(PS_SOLID,1,CLOSURE_BORDER_COLOR);
      CBrush* pOldBrush = pDC->SelectObject(&pier_brush);
      CPen*   pOldPen = pDC->SelectObject(&pier_pen);

      pDC->Polygon(points,4);

      pDC->SelectObject(pOldPen);
      pDC->SelectObject(pOldBrush);

      return;
   }

   // Draw support at end of segment

   PoiAttributeType poiReference = (pIntervals->GetErectSegmentInterval(segmentKey) <= intervalIdx ? POI_ERECTED_SEGMENT : POI_RELEASED_SEGMENT);

   GET_IFACE(IPointOfInterest,pIPoi);
   PoiAttributeType attribute = (endType == pgsTypes::metStart ? POI_0L : POI_10L);
   std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,poiReference | attribute,POIFIND_AND));
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiCLBrg(vPoi.front());

   Float64 Xs = pIPoi->ConvertPoiToSegmentCoordinate(poiCLBrg);
   Xs += segToGraphCoordinateAdjustment;

   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   if ( intervalIdx == storageIntervalIdx )
   {
      Float64 left_support, right_support;
      GET_IFACE(IGirderSegment,pGirderSegment);
      pGirderSegment->GetSegmentStorageSupportLocations(segmentKey,&left_support,&right_support);

      Xs += (endType == pgsTypes::metStart ? left_support : -right_support);
   }

   Float64 X = m_pUnitConverter->Convert(Xs);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 sectionHeight = pSectProp->GetHg(pIntervals->GetPrestressReleaseInterval(segmentKey),poiCLBrg);
   Float64 H = m_pUnitConverter->Convert(sectionHeight);

   CPoint p;
   mapper.WPtoDP(X,-H,&p.x,&p.y);

   if ( pPier )
   {
      DrawPier(intervalIdx,pPier,pTimelineMgr,p,pDC);
   }
   else
   {
      DrawTemporarySupport(intervalIdx,pTS,pTimelineMgr,p,mapper,pDC);
   }
}

void CDrawBeamTool::DrawIntermediatePier(Float64 gdrPathToGraphCoordinateAdjustment,const CPrecastSegmentData* pSegment,IntervalIndexType intervalIdx,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC)
{
   GET_IFACE(IIntervals,pIntervals);
   if ( intervalIdx < pIntervals->GetFirstErectedSegmentInterval() )
      return; 

   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   if ( pStartSpan == pEndSpan )
      return; // no intermediate pier

   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);

   const CSegmentKey& segmentKey( pSegment->GetSegmentKey() );

   CBrush pier_brush(PIER_FILL_COLOR);
   CPen pier_pen(PS_SOLID,1,PIER_BORDER_COLOR);

   CBrush* pOldBrush = pDC->SelectObject(&pier_brush);
   CPen* pOldPen    = pDC->SelectObject(&pier_pen);

   const CSpanData2* pSpan = pStartSpan;
   while ( pSpan != pEndSpan )
   {
      const CPierData2* pPier = pSpan->GetNextPier();
      PierIndexType pierIdx = pPier->GetIndex();

      EventIndexType erectionEventIdx = pTimelineMgr->GetPierErectionEventIndex(pierIdx);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetInterval(erectionEventIdx);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // pier is not erected in this interval... move to the next span
         pSpan = pPier->GetNextSpan();
         continue;
      }


      Float64 sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);

      Float64 x = pBridge->GetPierLocation(pierIdx,segmentKey.girderIndex);

      // Bridge is continuous in this interval so draw pier symbol at CL Pier
      CPoint p;
      Float64 X = m_pUnitConverter->Convert(x + gdrPathToGraphCoordinateAdjustment);
      Float64 H = m_pUnitConverter->Convert(sectionHeight);
      mapper.WPtoDP(X,-H,&p.x,&p.y);

      DrawPier(intervalIdx,pPier,pTimelineMgr,p,pDC);

      pSpan = pPier->GetNextSpan();
   }

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}

void CDrawBeamTool::DrawIntermediateTemporarySupport(Float64 gdrPathToGraphCoordinateAdjustment,const CPrecastSegmentData* pSegment,IntervalIndexType intervalIdx,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC)
{
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   std::vector<const CTemporarySupportData*> tempSupports(pStartSpan->GetTemporarySupports());
   std::vector<const CTemporarySupportData*> endTempSupports(pEndSpan->GetTemporarySupports());
   tempSupports.insert(tempSupports.begin(),endTempSupports.begin(),endTempSupports.end());

   if ( tempSupports.size() == 0 )
      return; // no temporary supports

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);

   Float64 segment_start_station, segment_end_station;
   pSegment->GetStations(segment_start_station,segment_end_station);
   std::vector<const CTemporarySupportData*>::iterator iter(tempSupports.begin());
   std::vector<const CTemporarySupportData*>::iterator iterEnd(tempSupports.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CTemporarySupportData* pTS = *iter;
      Float64 ts_station = pTS->GetStation();
      if ( ::IsEqual(segment_start_station,ts_station) || ::IsEqual(segment_end_station,ts_station) )
         continue; // temporary support display objects already created when creating DO's at ends of segment

      if ( ::InRange(segment_start_station,ts_station,segment_end_station) )
      {
         EventIndexType erectionEventIdx, removalEventIdx;
         pTimelineMgr->GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removalEventIdx);
         IntervalIndexType removalIntervalIdx = pIntervals->GetInterval(removalEventIdx);
         if ( removalIntervalIdx <= intervalIdx )
            return; // don't draw temporary support if it has been removed

         GET_IFACE(ISectionProperties,pSectProp);

         CSegmentKey segmentKey(pStartSpan->GetBridgeDescription()->GetGirderGroup(pStartSpan)->GetIndex(),
                                pSegment->GetGirder()->GetIndex(),
                                pSegment->GetIndex());

         Float64 x = pBridge->GetTemporarySupportLocation(pTS->GetIndex(),segmentKey.girderIndex);
         Float64 sectionHeight = pSectProp->GetSegmentHeightAtTemporarySupport(segmentKey,pTS->GetIndex());

         CPoint p;
         Float64 X = m_pUnitConverter->Convert(x+gdrPathToGraphCoordinateAdjustment);
         Float64 H = m_pUnitConverter->Convert(sectionHeight);
         mapper.WPtoDP(X,-H,&p.x,&p.y);

         DrawTemporarySupport(intervalIdx,pTS,pTimelineMgr,p,mapper,pDC);
      }
   }
}

void CDrawBeamTool::DrawPier(IntervalIndexType intervalIdx,const CPierData2* pPier,const CTimelineManager* pTimelineMgr,const CPoint& p,CDC* pDC)
{
   CBrush pier_brush(PIER_FILL_COLOR);
   CPen pier_pen(PS_SOLID,1,PIER_BORDER_COLOR);
   CBrush* pOldBrush = pDC->SelectObject(&pier_brush);
   CPen*   pOldPen = pDC->SelectObject(&pier_pen);

   GET_IFACE(IIntervals,pIntervals);
   if ( pPier->IsBoundaryPier() )
   {
      pgsTypes::PierConnectionType connectionType = pPier->GetPierConnectionType();
      PierIndexType pierIdx = pPier->GetIndex();

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      IntervalIndexType erectFirstSegmentIntervalIdx = pIntervals->GetFirstErectedSegmentInterval();

      GET_IFACE(IBridge,pBridge);
      EventIndexType leftContinuityEventIdx, rightContinuityEventIdx;
      pBridge->GetContinuityEventIndex(pierIdx,&leftContinuityEventIdx,&rightContinuityEventIdx);

      IntervalIndexType leftContinuityIntervalIdx  = pIntervals->GetInterval(leftContinuityEventIdx);
      IntervalIndexType rightContinuityIntervalIdx = pIntervals->GetInterval(rightContinuityEventIdx);

      if ( intervalIdx == erectFirstSegmentIntervalIdx )
      {
         connectionType = pgsTypes::Hinge;
      }

      if ( connectionType == pgsTypes::Roller )
      {
         DrawRoller(p,pDC);
      }
      else if ( connectionType == pgsTypes::Hinge )
      {
         DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::ContinuousBeforeDeck )
      {
         if ( leftContinuityIntervalIdx <= intervalIdx )
            DrawContinuous(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::ContinuousAfterDeck )
      {
         if ( castDeckIntervalIdx < intervalIdx )
            DrawContinuous(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::IntegralBeforeDeck )
      {
         if ( leftContinuityIntervalIdx <= intervalIdx )
            DrawIntegral(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::IntegralAfterDeck )
      {
         if ( castDeckIntervalIdx < intervalIdx )
            DrawIntegral(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::IntegralBeforeDeckHingeBack )
      {
         if ( rightContinuityIntervalIdx <= intervalIdx )
            DrawIntegralHingeBack(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::IntegralAfterDeckHingeBack )
      {
         if ( castDeckIntervalIdx < intervalIdx)
            DrawIntegralHingeBack(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::IntegralBeforeDeckHingeAhead )
      {
         if ( leftContinuityIntervalIdx <= intervalIdx )
            DrawIntegralHingeAhead(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::IntegralAfterDeckHingeAhead )
      {
         if ( castDeckIntervalIdx < intervalIdx )
            DrawIntegralHingeAhead(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else
      {
         ATLASSERT(false); // is there a new connection type?
      }
   }
   else
   {
      const CClosurePourData* pClosurePour = pPier->GetClosurePour(0);
      ATLASSERT(pClosurePour);
      CClosureKey closureKey(pClosurePour->GetClosureKey());
      IntervalIndexType closurePourIntervalIdx = pIntervals->GetCastClosurePourInterval(closureKey);

      pgsTypes::PierSegmentConnectionType connectionType = pPier->GetSegmentConnectionType();
      if ( connectionType == pgsTypes::psctContinousClosurePour )
      {
         if ( closurePourIntervalIdx < intervalIdx )
            DrawContinuous(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::psctIntegralClosurePour )
      {
         if ( closurePourIntervalIdx < intervalIdx )
            DrawIntegral(p,pDC);
         else
            DrawHinge(p,pDC);
      }
      else if ( connectionType == pgsTypes::psctContinuousSegment )
      {
         DrawContinuous(p,pDC);
      }
      else if ( connectionType == pgsTypes::psctIntegralSegment )
      {
         DrawIntegral(p,pDC);
      }
   }

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

void CDrawBeamTool::DrawTemporarySupport(IntervalIndexType intervalIdx,const CTemporarySupportData* pTS,const CTimelineManager* pTimelineMgr,const CPoint& p,const grlibPointMapper& mapper,CDC* pDC)
{
   EventIndexType erectionEventIdx, removalEventIdx;
   pTimelineMgr->GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removalEventIdx);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetInterval(erectionEventIdx);
   IntervalIndexType removalIntervalIdx  = pIntervals->GetInterval(removalEventIdx);

   if ( removalIntervalIdx <= intervalIdx )
      return; // temporary support has been removed

   if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
   {
      CBrush ts_brush(intervalIdx < erectionIntervalIdx ? TS_FILL_GHOST_COLOR : TS_FILL_COLOR);
      CPen ts_pen(PS_SOLID,1,TS_BORDER_COLOR);
      CBrush* pOldBrush = pDC->SelectObject(&ts_brush);
      CPen*   pOldPen = pDC->SelectObject(&ts_pen);

      DrawHinge(p,pDC);

      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
   }
   else
   {
      CBrush sb_brush(intervalIdx < erectionIntervalIdx ? SB_FILL_GHOST_COLOR : SB_FILL_COLOR);
      CPen sb_pen(PS_SOLID,2,intervalIdx < erectionIntervalIdx ? SB_BORDER_GHOST_COLOR : SB_BORDER_COLOR);
      CBrush* pOldBrush = pDC->SelectObject(&sb_brush);
      CPen*   pOldPen = pDC->SelectObject(&sb_pen);

      DrawStrongBack(pTS,p,mapper,pDC);

      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
   }
}

void CDrawBeamTool::DrawStrongBack(const CTemporarySupportData* pTS,const CPoint& p,const grlibPointMapper& mapper,CDC* pDC)
{
   const CClosurePourData* pClosurePour = pTS->GetClosurePour(m_GirderKey.girderIndex);
   ATLASSERT(pClosurePour != NULL);

   const CPrecastSegmentData* pLeftSegment  = pClosurePour->GetLeftSegment();
   const CPrecastSegmentData* pRightSegment = pClosurePour->GetRightSegment();

   const CSegmentKey& leftSegmentKey(pLeftSegment->GetSegmentKey());
   const CSegmentKey& rightSegmentKey(pRightSegment->GetSegmentKey());

   GET_IFACE(IBridge,pBridge);
   Float64 left_brg_offset  = pBridge->GetSegmentEndBearingOffset(leftSegmentKey);
   Float64 right_brg_offset = pBridge->GetSegmentStartBearingOffset(rightSegmentKey);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 sectionHeight = pSectProp->GetSegmentHeightAtTemporarySupport(leftSegmentKey,pTS->GetIndex());
   Float64 H = m_pUnitConverter->Convert(sectionHeight);

   CPoint hp0,hp1;
   mapper.WPtoDP(0,0,&hp0.x,&hp0.y);
   mapper.WPtoDP(left_brg_offset + right_brg_offset,H,&hp1.x,&hp1.y);

   LONG dx = hp1.x-hp0.x;
   LONG dy = hp1.y-hp0.y;

   pDC->MoveTo(p.x-dx/2,p.y);
   pDC->LineTo(p.x-dx/2,p.y+dy);
   pDC->LineTo(p.x+dx/2,p.y+dy);
   pDC->LineTo(p.x+dx/2,p.y);
}

void CDrawBeamTool::DrawRoller(CPoint p,CDC* pDC)
{
   CRect circle;
   circle.left = p.x;
   circle.right = p.x;
   circle.top = p.y + m_SupportSize.cy;
   circle.bottom = p.y + m_SupportSize.cy;
   circle.InflateRect(m_SupportSize);
   pDC->Ellipse(circle);
}

void CDrawBeamTool::DrawHinge(CPoint p,CDC* pDC)
{
   DrawContinuous(p,pDC);
}

void CDrawBeamTool::DrawContinuous(CPoint p,CDC* pDC)
{
   CPoint polyPnts[3];
   polyPnts[0] = p;

   polyPnts[1].x = polyPnts[0].x -   m_SupportSize.cx;
   polyPnts[1].y = polyPnts[0].y + 2*m_SupportSize.cy;

   polyPnts[2].x = polyPnts[0].x +   m_SupportSize.cx;
   polyPnts[2].y = polyPnts[0].y + 2*m_SupportSize.cy;

   pDC->Polygon(polyPnts,3);
}

void CDrawBeamTool::DrawIntegral(CPoint p,CDC* pDC)
{
   CRect box;
   box.left = p.x;
   box.right = p.x;
   box.top = p.y + m_SupportSize.cy;
   box.bottom = p.y + m_SupportSize.cy;
   box.InflateRect(m_SupportSize);
   pDC->Rectangle(box);
}

void CDrawBeamTool::DrawIntegralHingeBack(CPoint p,CDC* pDC)
{
   DrawIntegral(p,pDC);
   p.x -= m_SupportSize.cx;
   p.y -= m_SupportSize.cy;
   DrawRoller(p,pDC);
}

void CDrawBeamTool::DrawIntegralHingeAhead(CPoint p,CDC* pDC)
{
   DrawIntegral(p,pDC);
   p.x += m_SupportSize.cx;
   p.y -= m_SupportSize.cy;
   DrawRoller(p,pDC);
}
