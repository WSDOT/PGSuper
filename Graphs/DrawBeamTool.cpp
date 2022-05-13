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
#include <Graphs\DrawBeamTool.h>

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
   m_dwStyle = DBS_ERECTED_SEGMENTS_ONLY;
   m_MinAspectRatio = 1.0;
}

CDrawBeamTool::~CDrawBeamTool()
{
}

void CDrawBeamTool::SetStyle(DWORD dwStyle)
{
   m_dwStyle = dwStyle;
}

DWORD CDrawBeamTool::GetStyle() const
{
   return m_dwStyle;
}

void CDrawBeamTool::SetMinAspectRatio(Float64 ratio)
{
   m_MinAspectRatio = ratio;
}

Float64 CDrawBeamTool::GetMinAspectRatio() const
{
   return m_MinAspectRatio;
}

void CDrawBeamTool::DrawBeam(IBroker* pBroker,CDC* pDC, const WBFL::Graphing::PointMapper& mapper,WBFL::Units::PhysicalConverter* pUnitConverter,IntervalIndexType firstPlottingIntervalIdx,IntervalIndexType lastPlottingIntervalIdx,const CGirderKey& girderKey,Float64 beamShift)
{
   m_pBroker = pBroker;
   m_pUnitConverter = pUnitConverter;

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IGirder,pIGirder);
   GET_IFACE(IPointOfInterest,pPoi);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);

   IntervalIndexType firstErectedSegmentIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);

   IntervalIndexType intervalIdx = lastPlottingIntervalIdx;

   // we are drawing the beam in it's permanent configuration if the current interval is at the interval
   // when the first segment is erected or later, EXCEPT, if this is an interval in which hauling is occuring
   // when hauling is occuring, we only draw the segments that are being hauled during the interval
   bool bIsPermanentInterval = (firstErectedSegmentIntervalIdx <= intervalIdx) ? true : false;
   bool bIsHaulingInterval = (firstPlottingIntervalIdx == lastPlottingIntervalIdx ? pIntervals->IsHaulSegmentInterval(intervalIdx) : false);
   if (bIsHaulingInterval)
   {
      bIsPermanentInterval = false;
   }

   // Get maximum beam depth along girder. Assume max height occurs at a pier
   GET_IFACE(ISectionProperties, pSectProp);
   Float64 beamDepth = 0.0;
   Float64 beamLength = 0.0;
   for (const auto& thisGirderKey : vGirderKeys)
   {
      beamLength += pBridge->GetGirderLength(thisGirderKey);

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

         const std::vector<const CPierData2*> piers = pSegment->GetPiers();
         for (const auto& pPier : piers)
         {
            Float64 segdep = pSectProp->GetSegmentHeightAtPier(segmentKey, pPier->GetIndex());
            beamDepth = max(beamDepth, segdep);
         }
      }
   }

   Float64 beamAspectRatio = beamLength / beamDepth;

   // if beam length/depth is smaller than min we need to scale down Y to bring it into range
   m_YScaleFac = 1.0;
   if (beamAspectRatio < m_MinAspectRatio)
   {
      m_YScaleFac = beamAspectRatio / m_MinAspectRatio;
   }

   m_SupportSize = GetSupportSize(pDC);

   //
   // Draw the segments
   //
   for(const auto& thisGirderKey : vGirderKeys)
   {
      // deal with girder index when there are different number of girders in each group
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

         //
         // Draw Segment
         //
         DrawSegment(beamShift, intervalIdx, bIsHaulingInterval, segmentKey, pIntervals, pIGirder, pPoi, mapper, pDC);

         //
         // Draw Closure Joint
         //
         if ( segIdx < nSegments-1 )
         {
            CClosureKey closureKey(segmentKey);
            DrawClosureJoint(beamShift, intervalIdx, closureKey, pIntervals, pIGirder, pPoi, mapper, pDC);
         }

         //
         // Draw Supports
         //
         if (!bIsPermanentInterval)
         {
            DrawSegmentEndSupport(beamShift, intervalIdx, bIsHaulingInterval, segmentKey, pgsTypes::metStart, pIntervals, pPoi, mapper, pDC);
            DrawSegmentEndSupport(beamShift, intervalIdx, bIsHaulingInterval, segmentKey, pgsTypes::metEnd, pIntervals, pPoi, mapper, pDC);
         }
      } // end of segment loop

      //
      // Draw the tendons for all segments
      //
      CSegmentKey segmentKey(thisGirderKey,ALL_SEGMENTS);

      DrawTendons(beamShift,intervalIdx,segmentKey,mapper,pDC);

      //
      // Draw the permanent piers
      //
      if (bIsPermanentInterval)
      {
         PierIndexType startPierIdx = pBridgeDesc->GetGirderGroup(vGirderKeys.front().groupIndex)->GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx = pBridgeDesc->GetGirderGroup(vGirderKeys.back().groupIndex)->GetPierIndex(pgsTypes::metEnd);
         for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
         {
            DrawPier(beamShift, intervalIdx, girderKey, pierIdx, mapper, pDC);
         }

         GET_IFACE(ITempSupport, pTempSupport);
         std::vector<SupportIndexType> vTS = pTempSupport->GetTemporarySupports(girderKey.groupIndex);
         for (auto tsIdx : vTS)
         {
            IntervalIndexType erectionTempSupportIntervalIdx = pIntervals->GetTemporarySupportErectionInterval(tsIdx);
            IntervalIndexType removeTempSupportIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);

            if (erectionTempSupportIntervalIdx <= intervalIdx && intervalIdx < removeTempSupportIntervalIdx)
            {
               // only draw temporary support if it has been erected and not yet removed
               DrawTemporarySupport(beamShift, intervalIdx, girderKey, tsIdx, mapper, pDC);
            }
         }
      }
   } // end of group loop
}

void CDrawBeamTool::DrawBeamSegment(IBroker* pBroker,CDC* pDC,const WBFL::Graphing::PointMapper& mapper,WBFL::Units::PhysicalConverter* pUnitConverter,IntervalIndexType firstPlottingIntervalIdx,IntervalIndexType lastPlottingIntervalIdx,const CSegmentKey& segmentKey,Float64 beamShift)
{
   m_pBroker = pBroker;
   m_pUnitConverter = pUnitConverter;

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pIGirder);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE_NOCHECK(ISectionProperties,pSectProp);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   IntervalIndexType intervalIdx = lastPlottingIntervalIdx;

   IntervalIndexType erectedSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType haulingSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   bool bIsHaulingInterval = intervalIdx == haulingSegmentIntervalIdx;

   bool bIsPermanentInterval = erectedSegmentIntervalIdx <= intervalIdx;

   Float64 beamLength = pBridge->GetSegmentLength(segmentKey);

   // Get maximum beam depth along girder. First assume max height occurs at a pier. If no pier, get at mid-segment
   Float64 beamDepth = 0.0;
   const CPrecastSegmentData* pSegment = pBridgeDesc->GetSegment(segmentKey);
   const std::vector<const CPierData2*> piers = pSegment->GetPiers();
   if (!piers.empty())
   {
      for (const auto& pPier : piers)
      {
         Float64 segdep = pSectProp->GetSegmentHeightAtPier(segmentKey,pPier->GetIndex());
         beamDepth = max(beamDepth,segdep);
      }
   }
   else
   {
      beamDepth = pSectProp->GetSegmentHeight(pSegment, beamLength/2.0);
   }

   // if beam length/depth is smaller than min we need to scale down Y to bring it into range
   Float64 beamAspectRatio = beamLength / beamDepth;
   m_YScaleFac = 1.0;
   if (beamAspectRatio < m_MinAspectRatio)
   {
      m_YScaleFac = beamAspectRatio / m_MinAspectRatio;
   }

   m_SupportSize = GetSupportSize(pDC);

   //
   // Draw Segment
   //
   DrawSegment(beamShift,intervalIdx,bIsHaulingInterval,segmentKey,pIntervals,pIGirder,pPoi,mapper,pDC);

   //
   // Draw the tendons
   //
   DrawTendons(beamShift,intervalIdx,segmentKey,mapper,pDC);

   //
   // Draw Supports or piers
   //
   CGirderKey girderKey(segmentKey.groupIndex,segmentKey.girderIndex);

   if (!bIsPermanentInterval)
   {
      DrawSegmentEndSupport(beamShift,intervalIdx,bIsHaulingInterval,segmentKey,pgsTypes::metStart,pIntervals,pPoi,mapper,pDC);
      DrawSegmentEndSupport(beamShift,intervalIdx,bIsHaulingInterval,segmentKey,pgsTypes::metEnd,pIntervals,pPoi,mapper,pDC);
   }
   else 
   {
      const std::vector<const CPierData2*> piers = pSegment->GetPiers();
      for (const auto& pPier : piers)
      {
         DrawPier(beamShift,intervalIdx,girderKey,pPier->GetIndex(),mapper,pDC);
      }

      const std::vector<const CTemporarySupportData*> tempsupports = pSegment->GetTemporarySupports();
      for (const auto& tempsupport : tempsupports)
      {
         SupportIndexType tsIdx = tempsupport->GetIndex();

         IntervalIndexType erectionTempSupportIntervalIdx = pIntervals->GetTemporarySupportErectionInterval(tsIdx);
         IntervalIndexType removeTempSupportIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);

         if (erectionTempSupportIntervalIdx <= intervalIdx && intervalIdx < removeTempSupportIntervalIdx)
         {
            // only draw temporary support if it has been erected and not yet removed
            DrawTemporarySupport(beamShift,intervalIdx,girderKey,tsIdx,mapper,pDC);
         }
      }
   } 
}

CSize CDrawBeamTool::GetSupportSize(CDC* pDC) const
{
#pragma Reminder("UPDATE: printing sizes should be based on printer properties")
   if (pDC->IsPrinting())
   {
      return CSize(20, 20);
   }
   else
   {
      return CSize(5, 5);
   }
}

WBFL::Graphing::PointMapper CDrawBeamTool::CreatePointMapperAtGraphZero(const WBFL::Graphing::PointMapper& graphMapper) const
{
   //
   // re-configure mapper so beam height draws with same scale as beam length
   //
   WBFL::Graphing::PointMapper mapper(graphMapper);

   // get device point at World (0,0)
   LONG x, y;
   mapper.WPtoDP(0, 0, &x, &y);

   // Get world extents and world origin
   WBFL::Graphing::Size wExt = mapper.GetWorldExt();
   WBFL::Graphing::Point wOrg = mapper.GetWorldOrg();

   // get device extents and device origin
   LONG dx, dy;
   mapper.GetDeviceExt(&dx, &dy);

   LONG dox, doy;
   mapper.GetDeviceOrg(&dox, &doy);

   // compute a new device origin in the y direction
   doy = (LONG)Round(y - (0.0 - wOrg.Y()) * ((Float64)(dx) / -wExt.Dx()));

   // change the y scaling (use same scaling as X direction)
   mapper.SetWorldExt(wExt.Dx(), wExt.Dx());
   mapper.SetDeviceExt(dx, dx);

   // change the device origin
   mapper.SetDeviceOrg(dox, doy);

   return mapper;
}

Float64 CDrawBeamTool::ComputeGirderShift(const CGirderKey& girderKey)
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

Float64 CDrawBeamTool::ComputeSegmentShift(const CSegmentKey& segmentKey)
{
   GET_IFACE(IPointOfInterest, pPoi);
   pgsPointOfInterest poi(segmentKey, 0.0); // poi at start of segment
   Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   poi.SetSegmentKey(CSegmentKey(segmentKey/*demotes segmentKey to a girder key*/, 0 /*first segment*/));
   Float64 Xstart = pPoi->ConvertPoiToGirderlineCoordinate(poi); // poi at start of first segment

   Float64 shift = Xgl - Xstart;
   return shift;
}

void CDrawBeamTool::DrawSegment(Float64 beamShift, IntervalIndexType intervalIdx, bool bIsHaulingInterval, const CSegmentKey& segmentKey, IIntervals*pIntervals, IGirder* pIGirder, IPointOfInterest* pPoi, const WBFL::Graphing::PointMapper& mapper, CDC* pDC)
{
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType firstErectedSegmentIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);

   if (intervalIdx < releaseIntervalIdx)
   {
      // the prestress has not been released yet so the segment doesn't exist...
      return;
   }

   CBrush segment_brush(SEGMENT_FILL_COLOR);
   CPen segment_pen(PS_SOLID, 1, SEGMENT_BORDER_COLOR);
   CBrush ghost_brush(SEGMENT_FILL_GHOST_COLOR);

   // use regular color if the current interval is before the first segment is erected (the current interval is
   // when the segments are individual pieces at release, storage, lifting, hauling, or some early timestep) or
   // if the segment is erected
   CPen* pOldPen = pDC->SelectObject(&segment_pen);
   CBrush* pOldBrush = (intervalIdx < firstErectedSegmentIntervalIdx || erectionIntervalIdx <= intervalIdx) ? pDC->SelectObject(&segment_brush) : pDC->SelectObject(&ghost_brush);

   // Get segment profile, don't include shape in the closure joint region.
   // Coordinates are in the girder path coordinate system.
   CComPtr<IShape> shape;
   pIGirder->GetSegmentProfile(segmentKey, false/*exclude closure joint*/, &shape);

   // map the points into the logical space of the graph
   CComPtr<IPoint2dCollection> points;
   shape->get_PolyPoints(&points); // these points are in girder path coordinates

   CollectionIndexType nPoints;
   points->get_Count(&nPoints);

   CPoint* pnts = new CPoint[nPoints];
   for (CollectionIndexType idx = 0; idx < nPoints; idx++)
   {
      CComPtr<IPoint2d> pnt;
      points->get_Item(idx, &pnt);

      Float64 x, y;
      pnt->Location(&x, &y);

      Float64 Xgl = pPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(segmentKey, x);

      Float64 X = m_pUnitConverter->Convert(Xgl + beamShift);
      Float64 Y = m_pUnitConverter->Convert(y); // use the X-converter so the height of the beam is scaled the same as the length
      Y *= m_YScaleFac;

      mapper.WPtoDP(X, Y, &pnts[idx].x, &pnts[idx].y);
   }

   pDC->Polygon(pnts, (int)nPoints);
   delete[] pnts;


   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}

void CDrawBeamTool::DrawClosureJoint(Float64 beamShift, IntervalIndexType intervalIdx, const CClosureKey& closureKey, IIntervals* pIntervals, IGirder* pIGirder, IPointOfInterest* pPoi, const WBFL::Graphing::PointMapper& mapper, CDC* pDC)
{
   if (intervalIdx < pIntervals->GetCastClosureJointInterval(closureKey))
   {
      // closure has not been cast yet, nothing to draw
      return;
   }

   IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

   CBrush closure_brush(CLOSURE_FILL_COLOR);
   CPen closure_pen(PS_SOLID, 1, CLOSURE_BORDER_COLOR);
   CBrush ghost_brush(SEGMENT_FILL_GHOST_COLOR);

   CPen* pOldPen = pDC->SelectObject(&closure_pen);
   CBrush* pOldBrush = (compositeClosureIntervalIdx <= intervalIdx) ? pDC->SelectObject(&closure_brush) : pDC->SelectObject(&ghost_brush);

   CComPtr<IShape> shape;
   pIGirder->GetClosureJointProfile(closureKey, &shape);

   CComPtr<IPoint2dCollection> points;
   shape->get_PolyPoints(&points); // these points are in girder path coordinates

   CollectionIndexType nPoints;
   points->get_Count(&nPoints);

   CPoint* pnts = new CPoint[nPoints];
   for (CollectionIndexType idx = 0; idx < nPoints; idx++)
   {
      CComPtr<IPoint2d> pnt;
      points->get_Item(idx, &pnt);

      Float64 x, y;
      pnt->Location(&x, &y);

      Float64 Xgl = pPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(closureKey, x);

      Float64 X = m_pUnitConverter->Convert(Xgl + beamShift);
      Float64 Y = m_pUnitConverter->Convert(y); // use the X-converter so the height of the beam is scaled the same as the length
      Y *= m_YScaleFac;

      mapper.WPtoDP(X, Y, &pnts[idx].x, &pnts[idx].y);
   }

   pDC->Polygon(pnts, (int)nPoints);
   delete[] pnts;

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}


void CDrawBeamTool::DrawSegmentEndSupport(Float64 beamShift, IntervalIndexType intervalIdx, bool bIsHaulingInterval, const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType, IIntervals* pIntervals, IPointOfInterest* pPoi, const WBFL::Graphing::PointMapper& mapper, CDC* pDC)
{
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   if ((intervalIdx < releaseIntervalIdx)
      ||
      (bIsHaulingInterval && intervalIdx != pIntervals->GetHaulSegmentInterval(segmentKey) && sysFlags<DWORD>::IsSet(m_dwStyle, DBS_HAULED_SEGMENTS_ONLY))
      )
   {
      // the prestress has not been released yet so the segment doesn't exist... so there aren't any supports
      // -OR-
      // segment hauling occurs during the current interval and the current segment is not being hauled in this interval
      return;
   }

   // Draw support at end of segment
   IntervalIndexType liftingIntervalIdx  = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx  = pIntervals->GetHaulSegmentInterval(segmentKey);

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
   else
   {
      poiReference = POI_HAUL_SEGMENT;

      // this method draws segment supports during before erection intervals
      // if this assert fires, we are calling this for an erection or later interval
      ATLASSERT(intervalIdx < pIntervals->GetErectSegmentInterval(segmentKey));
   }

   PoiAttributeType attribute = (endType == pgsTypes::metStart ? POI_0L : POI_10L);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, poiReference | attribute, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiCLBrg(vPoi.front());

   Float64 Xg = pPoi->ConvertPoiToGirderlineCoordinate(poiCLBrg);
   Float64 X = m_pUnitConverter->Convert(Xg+beamShift);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 sectionHeight = pSectProp->GetHg(pIntervals->GetPrestressReleaseInterval(segmentKey),poiCLBrg);
   Float64 H = m_pUnitConverter->Convert(sectionHeight);
   H *= m_YScaleFac;

   CPoint p;
   mapper.WPtoDP(X,-H,&p.x,&p.y);

   CBrush brush(TS_FILL_COLOR);
   CPen pen(PS_SOLID, 1, TS_BORDER_COLOR);

   CPen* pOldPen = pDC->SelectObject(&pen);
   CBrush* pOldBrush = pDC->SelectObject(&brush);

   if (endType == pgsTypes::metStart)
   {
      DrawHinge(p, pDC);
   }
   else
   {
      DrawRoller(p, pDC);
   }

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}

void CDrawBeamTool::DrawPier(Float64 beamShift,IntervalIndexType intervalIdx,const CGirderKey& girderKey,PierIndexType pierIdx, const WBFL::Graphing::PointMapper& mapper, CDC* pDC)
{
   CPoint p = GetPierPoint(beamShift, intervalIdx, girderKey, pierIdx, mapper);

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

   CBrush pier_brush(PIER_FILL_COLOR);
   CBrush* pOldBrush = pDC->SelectObject(&pier_brush);

   CPen pier_pen(PS_SOLID,1,PIER_BORDER_COLOR);
   CPen*   pOldPen = pDC->SelectObject(&pier_pen);

   if ( pPier->IsBoundaryPier() )
   {
      GET_IFACE(IIntervals,pIntervals);

      pgsTypes::BoundaryConditionType boundaryConditionType = pPier->GetBoundaryConditionType();
      PierIndexType pierIdx = pPier->GetIndex();

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
      IntervalIndexType erectFirstSegmentIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);

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
            DrawRoller(p, pDC);
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

CPoint CDrawBeamTool::GetPierPoint(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, PierIndexType pierIdx, const WBFL::Graphing::PointMapper& mapper)
{
   GET_IFACE(IBridge, pBridge);
   CSegmentKey backSegmentKey, aheadSegmentKey;
   pBridge->GetSegmentsAtPier(pierIdx, girderKey.girderIndex, &backSegmentKey, &aheadSegmentKey);

   if (aheadSegmentKey.segmentIndex == INVALID_INDEX)
   {
      // there isn't an ahead side segment. this could be for one of two reasons
      // 1) a segment is continuous over this pier (expect backSegmentKey.segmentIndex == INVALID_INDEX)
      // 2) pierIdx is at the end of the bridge
      if (backSegmentKey.segmentIndex == INVALID_INDEX)
      {
         // get the straddling segment
         aheadSegmentKey = pBridge->GetSegmentAtPier(pierIdx, girderKey);
      }
      else
      {
         // use the back side segment since there isn't an ahead side segment
         aheadSegmentKey = backSegmentKey;
      }
   }

   ASSERT_SEGMENT_KEY(aheadSegmentKey);

   GET_IFACE(ISectionProperties, pSectProp);
   Float64 sectionHeight = pSectProp->GetSegmentHeightAtPier(aheadSegmentKey, pierIdx);

   GET_IFACE(IPointOfInterest, pPoi);
   Float64 Xs; // location of pier in segment coordinates
   if (pBridge->IsAbutment(pierIdx))
   {
      PoiList vPoi;
      pPoi->GetPointsOfInterest(aheadSegmentKey, POI_ERECTED_SEGMENT | POI_0L | POI_10L, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      Xs = (pierIdx == 0 ? vPoi.front().get().GetDistFromStart() : vPoi.back().get().GetDistFromStart());
   }
   else
   {
      bool bResult = pBridge->GetPierLocation(pierIdx, aheadSegmentKey, &Xs);
      ATLASSERT(bResult == true);
   }

   Float64 Xgl = pPoi->ConvertSegmentCoordinateToGirderlineCoordinate(aheadSegmentKey, Xs);

   CPoint p;
   Float64 X = m_pUnitConverter->Convert(Xgl + beamShift);
   Float64 H = m_pUnitConverter->Convert(sectionHeight);
   H *= m_YScaleFac;

   mapper.WPtoDP(X, -H, &p.x, &p.y);

   return p;
}

void CDrawBeamTool::DrawTemporarySupport(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, SupportIndexType tsIdx, const WBFL::Graphing::PointMapper& mapper, CDC* pDC)
{
   CPoint p = GetTemporarySupportPoint(beamShift, intervalIdx, girderKey, tsIdx, mapper);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetTemporarySupportErectionInterval(tsIdx);

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

   if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
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

      DrawStrongBack(girderKey,pTS,p,mapper,pDC);

      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
   }
}

CPoint CDrawBeamTool::GetTemporarySupportPoint(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, SupportIndexType tsIdx, const WBFL::Graphing::PointMapper& mapper)
{
   GET_IFACE(IBridge, pBridge);
   CSegmentKey backSegmentKey, aheadSegmentKey;
   pBridge->GetSegmentsAtTemporarySupport(girderKey.girderIndex, tsIdx, &backSegmentKey, &aheadSegmentKey);
   ATLASSERT(backSegmentKey.segmentIndex != INVALID_INDEX && aheadSegmentKey.segmentIndex != INVALID_INDEX);

   CSegmentKey segmentKey;
   segmentKey = (backSegmentKey.segmentIndex == INVALID_INDEX) ? aheadSegmentKey : backSegmentKey;

   GET_IFACE(ISectionProperties, pSectProp);
   Float64 sectionHeight = pSectProp->GetSegmentHeightAtTemporarySupport(segmentKey, tsIdx);

   GET_IFACE(IPointOfInterest, pPoi);
   Float64 Xgp = pBridge->GetTemporarySupportLocation(tsIdx, segmentKey.girderIndex);
   Float64 Xgl = pPoi->ConvertGirderPathCoordinateToGirderlineCoordinate(segmentKey, Xgp);

   CPoint p;
   Float64 X = m_pUnitConverter->Convert(Xgl + beamShift);
   Float64 H = m_pUnitConverter->Convert(sectionHeight);
   H *= m_YScaleFac;

   mapper.WPtoDP(X, -H, &p.x, &p.y);

   return p;
}

void CDrawBeamTool::DrawStrongBack(const CGirderKey& girderKey,const CTemporarySupportData* pTS,const CPoint& p,const WBFL::Graphing::PointMapper& mapper,CDC* pDC)
{
   const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(girderKey.girderIndex);
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
   H *= m_YScaleFac;

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

void CDrawBeamTool::DrawTendons(Float64 beamShift,IntervalIndexType intervalIdx, const CSegmentKey& rSegmentKey, const WBFL::Graphing::PointMapper& mapper,CDC* pDC)
{
   GET_IFACE_NOCHECK(ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE(IGirderTendonGeometry,pGirderTendonGeometry);
   GET_IFACE_NOCHECK(IIntervals,pIntervals); // only gets used if there are tendons

   CGirderKey girderKey(rSegmentKey.groupIndex,rSegmentKey.girderIndex);
   Float64 tendonShift = ComputeGirderShift(girderKey);

   SegmentIndexType nSegments;
   if (rSegmentKey.segmentIndex == ALL_SEGMENTS)
   {
      GET_IFACE(IBridge,pBridge);
      nSegments = pBridge->GetSegmentCount(girderKey);
   }
   else
   {
      nSegments = 1;
   }

   CSegmentKey mySegmentKey(rSegmentKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      if (rSegmentKey.segmentIndex == ALL_SEGMENTS)
      {
         mySegmentKey.segmentIndex = segIdx;
      }

      IntervalIndexType ptIntervalIdx = pIntervals->GetStressSegmentTendonInterval(mySegmentKey);
      if (intervalIdx < ptIntervalIdx)
      {
         continue; // don't draw if not yet installed
      }

      DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(mySegmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         CComPtr<IPoint2dCollection> points;
         pSegmentTendonGeometry->GetDuctCenterline(mySegmentKey, ductIdx, &points);

         Float64 segment_tendon_shift = ComputeSegmentShift(mySegmentKey);

         COLORREF color = SEGMENT_TENDON_LINE_COLOR;

         CPen pen(PS_SOLID, 1, color);
         CPen* pOldPen = pDC->SelectObject(&pen);

         IndexType nPoints;
         points->get_Count(&nPoints);
         CPoint* polyPnts = new CPoint[nPoints];
         for (IndexType pntIdx = 0; pntIdx < nPoints; pntIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(pntIdx, &point);

            Float64 x, y;
            point->Location(&x, &y);

            Float64 WX = m_pUnitConverter->Convert(x + beamShift + tendonShift + segment_tendon_shift);
            Float64 WY = m_pUnitConverter->Convert(y);
            WY *= m_YScaleFac;

            LONG DX, DY;
            mapper.WPtoDP(WX, WY, &DX, &DY);

            polyPnts[pntIdx].x = DX;
            polyPnts[pntIdx].y = DY;
         }

         pDC->Polyline(polyPnts, (int)nPoints);
         delete[] polyPnts;

         pDC->SelectObject(pOldPen);
      }
   }

   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      IntervalIndexType ptIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);
      if ( intervalIdx < ptIntervalIdx )
      {
         continue; // don't draw if not yet installed
      }

      CComPtr<IPoint2dCollection> points;
      pGirderTendonGeometry->GetDuctCenterline(girderKey, ductIdx, &points);

      COLORREF color = GIRDER_TENDON_LINE_COLOR;

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
         WY *= m_YScaleFac;

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
