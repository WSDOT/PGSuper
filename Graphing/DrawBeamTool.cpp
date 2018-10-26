///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\ClosureJointData.h>

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>

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

void CDrawBeamTool::DrawBeam(IBroker* pBroker,CDC* pDC,grlibPointMapper mapper,arvPhysicalConverter* pUnitConverter,IntervalIndexType intervalIdx,const CGirderKey& girderKey,Float64 beamShift)
{
   m_pBroker = pBroker;
   m_pUnitConverter = pUnitConverter;

   GET_IFACE(IIntervals,pIntervals);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IGirder,pIGirder);
   GET_IFACE(IPointOfInterest,pPoi);

   GET_IFACE_NOCHECK(IBridge,pBridge); // only gets used if there are more than one segment per girder

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

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
   {
      m_SupportSize = CSize(20,20);
   }
   else
   {
      m_SupportSize = CSize(5,5);
   }

   //
   // Draw the girder
   //
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      // deal with girder index when there are different number of girders in each group
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girder,nGirders-1);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

      m_GirderKey.groupIndex = grpIdx;
      m_GirderKey.girderIndex = gdrIdx;

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
         SegmentIDType segID = pSegment->GetID();

         //
         // Draw Segment
         //

         // Get segment profile, don't include shape in the closure joint region.
         // Coordinates are in the girder path coordinate system.
         CComPtr<IShape> shape;
         pIGirder->GetSegmentProfile(segmentKey,false/*exclude closure joint*/,&shape);

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

            Float64 Xgl = pPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(segmentKey,x);

            Float64 X = m_pUnitConverter->Convert(Xgl+beamShift);
            Float64 Y = m_pUnitConverter->Convert(y); // use the X-converter so the height of the beam is scaled the same as the length

            mapper.WPtoDP(X,Y,&pnts[idx].x,&pnts[idx].y);
         }

         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

         if ( erectionIntervalIdx <= intervalIdx )
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
         // Draw Closure Joint
         //
         if ( segIdx < nSegments-1 && pIntervals->GetCastClosureJointInterval(segmentKey) <= intervalIdx )
         {
            CClosureKey closureKey(segmentKey);

            CComPtr<IShape> shape;
            pBridge->GetClosureJointProfile(closureKey,&shape);

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

               Float64 Xgl = pPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(segmentKey,x);

               Float64 X = m_pUnitConverter->Convert(Xgl+beamShift);
               Float64 Y = m_pUnitConverter->Convert(y); // use the X-converter so the height of the beam is scaled the same as the length

               mapper.WPtoDP(X,Y,&pnts[idx].x,&pnts[idx].y);
            }

            IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

            if ( compositeClosureIntervalIdx <= intervalIdx )
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
         DrawSegmentEndSupport(beamShift,pSegment,pgsTypes::metStart,intervalIdx,mapper,pDC);
         DrawSegmentEndSupport(beamShift,pSegment,pgsTypes::metEnd,intervalIdx,mapper,pDC);
         DrawIntermediatePier(beamShift,pSegment,intervalIdx,mapper,pDC);
         DrawIntermediateTemporarySupport(beamShift,pSegment,intervalIdx,mapper,pDC);
      } // end of segment loop

      Float64 tendonShift = ComputeShift(m_GirderKey);
      DrawTendons(beamShift,tendonShift,intervalIdx,mapper,pDC);
   } // end of group loop

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

Float64 CDrawBeamTool::ComputeShift(const CGirderKey& girderKey)
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.groupIndex == 0 )
   {
      // we are showing the first group so there isn't a shift
      return 0;
   }

   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest poi(CSegmentKey(girderKey,0),0.0);
   Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   poi.SetSegmentKey(CSegmentKey(0,girderKey.girderIndex,0));
   Float64 Xstart = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   Float64 shift = Xgl - Xstart;
   return shift;
}

void CDrawBeamTool::DrawSegmentEndSupport(Float64 beamShift,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,IntervalIndexType intervalIdx,const grlibPointMapper& mapper,CDC* pDC)
{
   // Draws the support icon at one end of a segment

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPointOfInterest,pIPoi);

   CSegmentKey segmentKey(pSegment->GetSegmentKey());

   const CClosureJointData* pClosure = (endType == pgsTypes::metStart ? pSegment->GetStartClosure() : pSegment->GetEndClosure());
   const CPierData2* pPier = nullptr;
   const CTemporarySupportData* pTS = nullptr;

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
         pPier->GetNextGirderGroup() != nullptr)      // not the last group
         &&                                     // AND
         (compositeDeckIntervalIdx <= intervalIdx &&
         pPier && pPier->IsContinuous())

         || // - OR -

         (segmentKey.segmentIndex == 0 && // first segment in the girder
         endType == pgsTypes::metStart && // support at left end of the segment
         pPier->GetPrevGirderGroup() != nullptr) // not the first group
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
      Float64 Xs; // location of pier in segment coordinates
      bool bResult = pBridge->GetPierLocation(pierIdx,segmentKey,&Xs);
      ATLASSERT(bResult == true);

      Float64 Xgl = pIPoi->ConvertSegmentCoordinateToGirderlineCoordinate(segmentKey,Xs);

      // Bridge is continuous in this interval so draw pier symbol at CL Pier
      CPoint p;
      Float64 X = m_pUnitConverter->Convert(Xgl+beamShift);
      Float64 H = m_pUnitConverter->Convert(sectionHeight);
      mapper.WPtoDP(X,-H,&p.x,&p.y);

      DrawPier(intervalIdx,pPier,p,pDC);

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

      if ( pPier->IsBoundaryPier() )
      {
         if ( endType == pgsTypes::metStart )
         {
            CSegmentKey prevSegmentKey(segmentKey.groupIndex-1,segmentKey.girderIndex,0);
            GirderIndexType nGirdersPrevGroup = pPier->GetPrevGirderGroup()->GetGirderCount();
            if ( nGirdersPrevGroup <= segmentKey.girderIndex )
            {
               prevSegmentKey.girderIndex = nGirdersPrevGroup-1;
            }

            prevSegmentKey.segmentIndex = pPier->GetPrevGirderGroup()->GetGirder(prevSegmentKey.girderIndex)->GetSegmentCount()-1;

            brgOffset = pBridge->GetSegmentEndBearingOffset(prevSegmentKey);
            endDist   = pBridge->GetSegmentEndEndDistance(prevSegmentKey);
         }
         else
         {
            CSegmentKey nextSegmentKey(segmentKey.groupIndex+1,segmentKey.girderIndex,0);
            GirderIndexType nGirdersNextGroup = pPier->GetNextGirderGroup()->GetGirderCount();
            if ( nGirdersNextGroup <= segmentKey.girderIndex )
            {
               nextSegmentKey.girderIndex = nGirdersNextGroup-1;
            }

            brgOffset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
            endDist   = pBridge->GetSegmentStartEndDistance(nextSegmentKey);
         }
      }
      else
      {
         if ( endType == pgsTypes::metStart )
         {
            CSegmentKey prevSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex-1);
            brgOffset = pBridge->GetSegmentEndBearingOffset(prevSegmentKey);
            endDist   = pBridge->GetSegmentEndEndDistance(prevSegmentKey);
         }
         else
         {
            CSegmentKey nextSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex+1);
            brgOffset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
            endDist   = pBridge->GetSegmentStartEndDistance(nextSegmentKey);
         }
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
   IntervalIndexType liftingIntervalIdx  = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx  = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   PoiAttributeType poiReference;
   if ( intervalIdx < liftingIntervalIdx )
   {
      poiReference = POI_RELEASED_SEGMENT;
   }
   else if ( liftingIntervalIdx <= intervalIdx && intervalIdx < storageIntervalIdx)
   {
      poiReference = POI_LIFT_SEGMENT;
   }
   else if ( storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx )
   {
      poiReference = POI_STORAGE_SEGMENT;
   }
   else if ( haulingIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx )
   {
      poiReference = POI_HAUL_SEGMENT;
   }
   else
   {
      ATLASSERT(erectionIntervalIdx <= intervalIdx);
      poiReference = POI_ERECTED_SEGMENT;
   }

   PoiAttributeType attribute = (endType == pgsTypes::metStart ? POI_0L : POI_10L);
   std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,poiReference | attribute,POIFIND_AND));
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiCLBrg(vPoi.front());

   Float64 Xg = pIPoi->ConvertPoiToGirderlineCoordinate(poiCLBrg);
   Float64 X = m_pUnitConverter->Convert(Xg+beamShift);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 sectionHeight = pSectProp->GetHg(pIntervals->GetPrestressReleaseInterval(segmentKey),poiCLBrg);
   Float64 H = m_pUnitConverter->Convert(sectionHeight);

   CPoint p;
   mapper.WPtoDP(X,-H,&p.x,&p.y);

   if ( pPier )
   {
      DrawPier(intervalIdx,pPier,p,pDC);
   }
   else
   {
      DrawTemporarySupport(intervalIdx,pTS,p,mapper,pDC);
   }
}

void CDrawBeamTool::DrawIntermediatePier(Float64 beamShift,const CPrecastSegmentData* pSegment,IntervalIndexType intervalIdx,const grlibPointMapper& mapper,CDC* pDC)
{
   const CSegmentKey& segmentKey( pSegment->GetSegmentKey() );

   GET_IFACE(IIntervals,pIntervals);
   if ( intervalIdx < pIntervals->GetFirstSegmentErectionInterval(segmentKey) )
   {
      return; 
   }

   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   if ( pStartSpan == pEndSpan )
   {
      return; // no intermediate pier
   }

   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pIPoi);

   CBrush pier_brush(PIER_FILL_COLOR);
   CPen pier_pen(PS_SOLID,1,PIER_BORDER_COLOR);

   CBrush* pOldBrush = pDC->SelectObject(&pier_brush);
   CPen* pOldPen     = pDC->SelectObject(&pier_pen);

   const CSpanData2* pSpan = pStartSpan;
   while ( pSpan != pEndSpan )
   {
      const CPierData2* pPier = pSpan->GetNextPier();
      PierIndexType pierIdx = pPier->GetIndex();

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectPierInterval(pierIdx);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // pier is not erected in this interval... move to the next span
         pSpan = pPier->GetNextSpan();
         continue;
      }


      Float64 sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);

      Float64 Xgp;
      VERIFY( pBridge->GetPierLocation(segmentKey,pierIdx,&Xgp) );
      Float64 Xgl = pIPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(segmentKey,Xgp);

      // Bridge is continuous in this interval so draw pier symbol at CL Pier
      CPoint p;
      Float64 X = m_pUnitConverter->Convert(Xgl + beamShift);
      Float64 H = m_pUnitConverter->Convert(sectionHeight);
      mapper.WPtoDP(X,-H,&p.x,&p.y);

      DrawPier(intervalIdx,pPier,p,pDC);

      pSpan = pPier->GetNextSpan();
   }

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}

void CDrawBeamTool::DrawIntermediateTemporarySupport(Float64 beamShift,const CPrecastSegmentData* pSegment,IntervalIndexType intervalIdx,const grlibPointMapper& mapper,CDC* pDC)
{
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   std::vector<const CTemporarySupportData*> tempSupports(pStartSpan->GetTemporarySupports());
   std::vector<const CTemporarySupportData*> endTempSupports(pEndSpan->GetTemporarySupports());
   tempSupports.insert(tempSupports.begin(),endTempSupports.begin(),endTempSupports.end());

   if ( tempSupports.size() == 0 )
   {
      return; // no temporary supports
   }

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

   // this interfaces only get used if there are temporary supports, and they have been erected and not yet removed
   GET_IFACE_NOCHECK(IIntervals,pIntervals);
   GET_IFACE_NOCHECK(IBridge,pBridge);
   GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
   GET_IFACE_NOCHECK(ISectionProperties,pSectProp);

   Float64 segment_start_station, segment_end_station;
   pSegment->GetStations(&segment_start_station,&segment_end_station);
   std::vector<const CTemporarySupportData*>::iterator iter(tempSupports.begin());
   std::vector<const CTemporarySupportData*>::iterator iterEnd(tempSupports.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CTemporarySupportData* pTS = *iter;
      Float64 ts_station = pTS->GetStation();
      if ( ::IsEqual(segment_start_station,ts_station) || ::IsEqual(segment_end_station,ts_station) )
      {
         continue; // temporary support display objects already created when creating DO's at ends of segment
      }

      if ( ::InRange(segment_start_station,ts_station,segment_end_station) )
      {
         SupportIndexType tsIdx = pTS->GetIndex();
         IntervalIndexType removalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);
         if ( removalIntervalIdx <= intervalIdx )
         {
            return; // don't draw temporary support if it has been removed
         }

         CSegmentKey segmentKey(pStartSpan->GetBridgeDescription()->GetGirderGroup(pStartSpan)->GetIndex(),
                                pSegment->GetGirder()->GetIndex(),
                                pSegment->GetIndex());

         Float64 Xgp = pBridge->GetTemporarySupportLocation(pTS->GetIndex(),segmentKey.girderIndex);
         Float64 Xgl = pPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(segmentKey,Xgp);

         Float64 sectionHeight = pSectProp->GetSegmentHeightAtTemporarySupport(segmentKey,pTS->GetIndex());

         CPoint p;
         Float64 X = m_pUnitConverter->Convert(Xgl + beamShift);
         Float64 H = m_pUnitConverter->Convert(sectionHeight);
         mapper.WPtoDP(X,-H,&p.x,&p.y);

         DrawTemporarySupport(intervalIdx,pTS,p,mapper,pDC);
      }
   }
}

void CDrawBeamTool::DrawPier(IntervalIndexType intervalIdx,const CPierData2* pPier,const CPoint& p,CDC* pDC)
{
   CBrush pier_brush(PIER_FILL_COLOR);
   CBrush* pOldBrush = pDC->SelectObject(&pier_brush);

   CPen pier_pen(PS_SOLID,1,PIER_BORDER_COLOR);
   CPen*   pOldPen = pDC->SelectObject(&pier_pen);

   if ( pPier->IsBoundaryPier() )
   {
      GET_IFACE(IIntervals,pIntervals);

      pgsTypes::BoundaryConditionType boundaryConditionType = pPier->GetBoundaryConditionType();
      PierIndexType pierIdx = pPier->GetIndex();

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      IntervalIndexType erectFirstSegmentIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(m_GirderKey);

      IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
      pIntervals->GetContinuityInterval(pierIdx,&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

      if ( intervalIdx == erectFirstSegmentIntervalIdx )
      {
         boundaryConditionType = pgsTypes::bctHinge;
      }

      if ( boundaryConditionType == pgsTypes::bctRoller )
      {
         DrawRoller(p,pDC);
      }
      else if ( boundaryConditionType == pgsTypes::bctHinge )
      {
         DrawHinge(p,pDC);
      }
      else if ( boundaryConditionType == pgsTypes::bctContinuousBeforeDeck )
      {
         if ( leftContinuityIntervalIdx <= intervalIdx )
         {
            DrawContinuous(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctContinuousAfterDeck )
      {
         if ( castDeckIntervalIdx < intervalIdx )
         {
            DrawContinuous(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralBeforeDeck )
      {
         if ( leftContinuityIntervalIdx <= intervalIdx )
         {
            DrawIntegral(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeck )
      {
         if ( castDeckIntervalIdx < intervalIdx )
         {
            DrawIntegral(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeBack )
      {
         if ( rightContinuityIntervalIdx <= intervalIdx )
         {
            DrawIntegralHingeBack(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeBack )
      {
         if ( castDeckIntervalIdx < intervalIdx)
         {
            DrawIntegralHingeBack(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeAhead )
      {
         if ( leftContinuityIntervalIdx <= intervalIdx )
         {
            DrawIntegralHingeAhead(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeAhead )
      {
         if ( castDeckIntervalIdx < intervalIdx )
         {
            DrawIntegralHingeAhead(p,pDC);
         }
         else
         {
            DrawHinge(p,pDC);
         }
      }
      else
      {
         ATLASSERT(false); // is there a new connection type?
      }
   }
   else
   {
      pgsTypes::PierSegmentConnectionType segmentConnectionType = pPier->GetSegmentConnectionType();
      if ( segmentConnectionType == pgsTypes::psctContinuousSegment )
      {
         DrawRoller(p,pDC);
      }
      else if ( segmentConnectionType == pgsTypes::psctIntegralSegment )
      {
         DrawIntegral(p,pDC);
      }
      else
      {
         const CClosureJointData* pClosureJoint = pPier->GetClosureJoint(0); // use gdrIdx = 0 because closure is the same for all girders
         ATLASSERT(pClosureJoint);
         CClosureKey closureKey(pClosureJoint->GetClosureKey());
      
         GET_IFACE(IIntervals,pIntervals);

         IntervalIndexType castClosureJointIntervalIdx = pIntervals->GetCastClosureJointInterval(closureKey);

         if ( segmentConnectionType == pgsTypes::psctContinousClosureJoint )
         {
            if ( castClosureJointIntervalIdx < intervalIdx )
            {
               if ( pPier->GetBoundaryConditionType() == pgsTypes::bctHinge )
               {
                  DrawHinge(p,pDC);
               }
               else
               {
                  ATLASSERT(pPier->GetBoundaryConditionType() == pgsTypes::bctRoller);
                  DrawRoller(p,pDC);
               }
            }
            else
            {
               DrawRoller(p,pDC);
            }
         }
         else if ( segmentConnectionType == pgsTypes::psctIntegralClosureJoint )
         {
            if ( castClosureJointIntervalIdx < intervalIdx )
            {
               DrawIntegral(p,pDC);
            }
            else
            {
               DrawRoller(p,pDC);
            }
         }
         else
         {
            ATLASSERT(false); // is there a new connectionType?
         }
      }
   }

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

void CDrawBeamTool::DrawTemporarySupport(IntervalIndexType intervalIdx,const CTemporarySupportData* pTS,const CPoint& p,const grlibPointMapper& mapper,CDC* pDC)
{
   SupportIndexType tsIdx = pTS->GetIndex();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetTemporarySupportErectionInterval(tsIdx);
   IntervalIndexType removalIntervalIdx  = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);

   if ( removalIntervalIdx <= intervalIdx )
   {
      return; // temporary support has been removed
   }

   IntervalIndexType erectFirstSegmentIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(m_GirderKey);

   if ( pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment && intervalIdx < erectFirstSegmentIntervalIdx )
   {
      // nothing to draw if interval is before the segments are erected and this temporary support isn't
      // at the end of a segment
      return;
   }

   if ( pTS->GetSupportType() == pgsTypes::ErectionTower || 
        intervalIdx < erectFirstSegmentIntervalIdx // draw as simple support if segments haven't been erected yet
       )
   {
      CBrush ts_brush(intervalIdx < erectionIntervalIdx ? TS_FILL_GHOST_COLOR : TS_FILL_COLOR);
      CPen ts_pen(PS_SOLID,1,TS_BORDER_COLOR);
      CBrush* pOldBrush = pDC->SelectObject(&ts_brush);
      CPen*   pOldPen = pDC->SelectObject(&ts_pen);

      DrawRoller(p,pDC);

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
   const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(m_GirderKey.girderIndex);
   ATLASSERT(pClosureJoint != nullptr);

   const CPrecastSegmentData* pLeftSegment  = pClosureJoint->GetLeftSegment();
   const CPrecastSegmentData* pRightSegment = pClosureJoint->GetRightSegment();

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

void CDrawBeamTool::DrawTendons(Float64 beamShift,Float64 tendonShift,IntervalIndexType intervalIdx,const grlibPointMapper& mapper,CDC* pDC)
{
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE_NOCHECK(IIntervals,pIntervals); // only gets used if there are tendons

   DuctIndexType nDucts = pTendonGeom->GetDuctCount(m_GirderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CComPtr<IPoint2dCollection> points;
      pTendonGeom->GetDuctCenterline(m_GirderKey,ductIdx,&points);

      IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(m_GirderKey,ductIdx);
      if ( intervalIdx < ptIntervalIdx )
      {
         continue; // don't draw if not yet installed
      }

      COLORREF color = TENDON_LINE_COLOR;

      CPen pen(PS_SOLID,1,color);
      CPen* pOldPen = pDC->SelectObject(&pen);

      IndexType nPoints;
      points->get_Count(&nPoints);
      CPoint* polyPnts = new CPoint[nPoints];
      for ( IndexType pntIdx = 0; pntIdx < nPoints; pntIdx++ )
      {
         CComPtr<IPoint2d> point;
         points->get_Item(pntIdx,&point);

         Float64 x,y;
         point->Location(&x,&y);

         Float64 WX = m_pUnitConverter->Convert(x+beamShift+tendonShift);
         Float64 WY = m_pUnitConverter->Convert(y);

         LONG DX, DY;
         mapper.WPtoDP(WX,WY,&DX,&DY);

         polyPnts[pntIdx].x = DX;
         polyPnts[pntIdx].y = DY;
      }

      pDC->Polyline(polyPnts,(int)nPoints);
      delete[] polyPnts;

      pDC->SelectObject(pOldPen);
   }
}
