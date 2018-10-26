///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <IFace\Bridge.h>
#include "BarrierSidewalkLoadDistributionTool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// utility struct used to store distances between sw/b's and girders
struct SwBDist
{
   GirderIndexType m_Gdr;
   Float64         m_Distance;

   SwBDist(GirderIndexType gdr, Float64 distance):
   m_Gdr(gdr), m_Distance(distance)
   {;}
};

// function class to sort distances
struct SwBDistComparer
{
   pgsTypes::TrafficBarrierOrientation m_Orientation;

   bool operator() (SwBDist i,SwBDist j)
   { 
      if (IsEqual(i.m_Distance, j.m_Distance))
      {
         // Exterior-most girder wins
         if (m_Orientation==pgsTypes::tboLeft)
         {
            return i.m_Gdr > j.m_Gdr;
         }
         else
         {
           return i.m_Gdr < j.m_Gdr;
         }
      }
      else
      {
          return (i.m_Distance < j.m_Distance);
      }
  }

  SwBDistComparer(pgsTypes::TrafficBarrierOrientation orientation):
  m_Orientation(orientation)
  {;}
};

/****************************************************************************
CLASS
   pgsBarrierSidewalkLoadDistributionTool
****************************************************************************/
//////////////////////
pgsBarrierSidewalkLoadDistributionTool::pgsBarrierSidewalkLoadDistributionTool(SHARED_LOGFILE lf, IBridge* pBridge, IGirder* pGirder, IBarriers* pBarriers):
LOGFILE(lf),
m_pIBridge(pBridge),
m_pIGirder(pGirder),
m_pIBarriers(pBarriers),
m_Span(INVALID_INDEX),
m_DidCompute(false)
{
   m_GeomUtil.CoCreateInstance(CLSID_GeomUtil);

   m_MidSpanRefLine.CoCreateInstance(CLSID_Line2d);
   m_LeftSlabEdgeLine.CoCreateInstance(CLSID_Line2d);
   m_RightSlabEdgeLine.CoCreateInstance(CLSID_Line2d);
}

void pgsBarrierSidewalkLoadDistributionTool::Initialize(SpanIndexType span, pgsTypes::TrafficBarrierDistribution distType, GirderIndexType nMaxDistributed)
{
   m_Span = span;
   m_DistType = distType;
   m_nMaxDistributed = nMaxDistributed;

   m_SpanLoadFractionData.Init();
   m_DidCompute = false;
}

void pgsBarrierSidewalkLoadDistributionTool::GetTrafficBarrierLoadFraction(GirderIndexType gdrIdx,
                                                                           Float64* pFraExtLeft, Float64* pFraIntLeft,
                                                                           Float64* pFraExtRight,Float64* pFraIntRight)
{
   if (!m_DidCompute)
   {
      // Fill m_SpanLoadFractionData
      Compute();

      m_DidCompute = true;
   }

   if (gdrIdx>=0 && gdrIdx<m_SpanLoadFractionData.m_GirderLoadFractions.size())
   {
      const GlfData& rglf = m_SpanLoadFractionData.m_GirderLoadFractions[gdrIdx];

      *pFraExtLeft  = rglf.GMSWs[LeftExteriorBarrier]  ==0 ? 0 : (Float64)rglf.GMSWs[LeftExteriorBarrier]  / m_SpanLoadFractionData.m_GMSWsAppliedTo[LeftExteriorBarrier];
      *pFraIntLeft  = rglf.GMSWs[LeftInteriorBarrier]  ==0 ? 0 : (Float64)rglf.GMSWs[LeftInteriorBarrier]  / m_SpanLoadFractionData.m_GMSWsAppliedTo[LeftInteriorBarrier];
      *pFraExtRight = rglf.GMSWs[RightExteriorBarrier] ==0 ? 0 : (Float64)rglf.GMSWs[RightExteriorBarrier] / m_SpanLoadFractionData.m_GMSWsAppliedTo[RightExteriorBarrier];
      *pFraIntRight = rglf.GMSWs[RightInteriorBarrier] ==0 ? 0 : (Float64)rglf.GMSWs[RightInteriorBarrier] / m_SpanLoadFractionData.m_GMSWsAppliedTo[RightInteriorBarrier];
   }
   else
   {
      ATLASSERT(0); // girder is out of range, this should never happen.
      *pFraExtLeft  = 0.0;
      *pFraIntLeft  = 0.0;
      *pFraExtRight = 0.0;
      *pFraIntRight = 0.0;
   }
}

void pgsBarrierSidewalkLoadDistributionTool::GetSidewalkLoadFraction(GirderIndexType gdrIdx, Float64* pFraSwLeft, Float64* pFraSwRight)
{
   if (!m_DidCompute)
   {
      // Fill m_SpanLoadFractionData
      Compute();

      m_DidCompute = true;
   }

   if (gdrIdx>=0 && gdrIdx<m_SpanLoadFractionData.m_GirderLoadFractions.size())
   {
      const GlfData& rglf = m_SpanLoadFractionData.m_GirderLoadFractions[gdrIdx];

      *pFraSwLeft  = rglf.GMSWs[LeftSidewalk]  ==0 ? 0 : (Float64)rglf.GMSWs[LeftSidewalk]  / m_SpanLoadFractionData.m_GMSWsAppliedTo[LeftSidewalk];
      *pFraSwRight = rglf.GMSWs[RightSidewalk] ==0 ? 0 : (Float64)rglf.GMSWs[RightSidewalk] / m_SpanLoadFractionData.m_GMSWsAppliedTo[RightSidewalk];
   }
   else
   {
      ATLASSERT(0); // girder is out of range, this should never happen.
      *pFraSwLeft  = 0.0;
      *pFraSwRight  = 0.0;
   }
}


void pgsBarrierSidewalkLoadDistributionTool::Compute()
{
   ATLASSERT(m_Span!=INVALID_INDEX); // failed to call Initialize?

   // First build geometry model
   BuildGeometryModel();

   // Fill GlfData with zeros
   GirderIndexType nGirders = m_pIBridge->GetGirderCount(m_Span);
   m_SpanLoadFractionData.m_GirderLoadFractions.assign(nGirders, GlfData());

   // Get total GMSWs From geometry model
   m_SpanLoadFractionData.m_TotalGMSWs = m_GMSWInterSectionPoints.size();

   // Some needed logicals
   bool hasLeftSw   = m_pIBarriers->HasSidewalk(pgsTypes::tboLeft);
   bool hasRightSw  = m_pIBarriers->HasSidewalk(pgsTypes::tboRight);
   bool hasLeftInt  = m_pIBarriers->HasInteriorBarrier(pgsTypes::tboLeft);
   bool hasRightInt = m_pIBarriers->HasInteriorBarrier(pgsTypes::tboRight);

   // Compute load fractions for each barrier element
   // Left exterior barrier
   Float64 weight = m_pIBarriers->GetExteriorBarrierWeight(pgsTypes::tboLeft);
   if (weight>0.0)
   {
      Float64 cgOffset = m_pIBarriers->GetExteriorBarrierCgToDeckEdge(pgsTypes::tboLeft);

      ComputeBarrierLoadDistribution(pgsTypes::tboLeft, cgOffset, LeftExteriorBarrier, true);
   }

   // Left Sidewalk
   if(hasLeftSw)
   {
      // centerline sidewalk is between dead load edges
      Float64 intEdge, extEdge;
      m_pIBarriers->GetSidewalkDeadLoadEdges(pgsTypes::tboLeft, &intEdge, &extEdge);

      ComputeSidewalkLoadDistribution(pgsTypes::tboLeft, extEdge, intEdge, LeftSidewalk, hasRightSw);
   }

   // Left interior barrier
   if (hasLeftInt)
   {
      Float64 cgOffset = m_pIBarriers->GetInteriorBarrierCgToDeckEdge(pgsTypes::tboLeft);

      ComputeBarrierLoadDistribution(pgsTypes::tboLeft, cgOffset, LeftInteriorBarrier, hasRightInt);
   }

   // Right exterior barrier
   weight = m_pIBarriers->GetExteriorBarrierWeight(pgsTypes::tboRight);
   if (weight>0.0)
   {
      Float64 cgOffset = m_pIBarriers->GetExteriorBarrierCgToDeckEdge(pgsTypes::tboRight);

      ComputeBarrierLoadDistribution(pgsTypes::tboRight, cgOffset, RightExteriorBarrier, true);
   }

   // Right Sidewalk
   if(hasRightSw)
   {
      // centerline sidewalk is between dead load edges
      Float64 intEdge, extEdge;
      m_pIBarriers->GetSidewalkDeadLoadEdges(pgsTypes::tboRight, &intEdge, &extEdge);

      ComputeSidewalkLoadDistribution(pgsTypes::tboRight, extEdge, intEdge, RightSidewalk, hasLeftSw);
   }

   // Right interior barrier
   if (hasRightInt)
   {
      Float64 cgOffset = m_pIBarriers->GetInteriorBarrierCgToDeckEdge(pgsTypes::tboRight);

      ComputeBarrierLoadDistribution(pgsTypes::tboRight, cgOffset, RightInteriorBarrier, hasLeftInt);
   }
}

void pgsBarrierSidewalkLoadDistributionTool::ComputeBarrierLoadDistribution(pgsTypes::TrafficBarrierOrientation orientation, Float64 swBrOffset,
                                                                           BarrSwType barrswType, bool oppositeExists)
{
   // Loads can either be evenly distributed to all GMSW's, or to N nearest.
   // Check for easy case first
   if ( !DistributeBSWLoadEvenly(barrswType, oppositeExists) )
   {
      // Distribute to N nearest
      DistributeBSWLoadToNNearest(orientation, swBrOffset, barrswType);
   }
}

void pgsBarrierSidewalkLoadDistributionTool::ComputeSidewalkLoadDistribution(pgsTypes::TrafficBarrierOrientation orientation, Float64 extEdgeOffset, Float64 intEdgeOffset, BarrSwType barrswType, bool oppositeExists)
{
   // Loads can either be evenly distributed to all GMSW's, GMSW's under the sidewalk, or to N nearest.
   // Check for easy case first
   if ( !DistributeBSWLoadEvenly(barrswType, oppositeExists))
   {
      // See if we have more than m_nMaxDistributed GMSW's directly beneath the sidewalk
      if ( !DistributeSidewalkLoadUnderSw(orientation, extEdgeOffset, intEdgeOffset, barrswType) )
      {
         // Distribute to N nearest
         Float64 swOffset = (extEdgeOffset + intEdgeOffset)/2.0;
         DistributeBSWLoadToNNearest(orientation, swOffset, barrswType);
      }
   }
}

bool pgsBarrierSidewalkLoadDistributionTool::DistributeBSWLoadEvenly(BarrSwType barrswType, bool oppositeExists)
{
   // Check if loads are evenly distributed to all GMSW's - return true if so
   if (m_nMaxDistributed <= 0)
   {
      // Special case of no load distribution. Defaults take care of this
      return true;
   }
   else if ( (oppositeExists && m_SpanLoadFractionData.m_TotalGMSWs/2 < m_nMaxDistributed) || m_SpanLoadFractionData.m_TotalGMSWs < m_nMaxDistributed)
   {
      // Distribute load evenly to all GMSW's
      m_SpanLoadFractionData.m_GMSWsAppliedTo[barrswType] = m_SpanLoadFractionData.m_TotalGMSWs;

      // Intersection points contains element data for each girder, use it to build
      // even distributions for each girder
      GMSWInterSectionIterator it(m_GMSWInterSectionPoints.begin());
      GMSWInterSectionIterator it_end(m_GMSWInterSectionPoints.end());
      while(it!= it_end)
      {
         GirderIndexType igdr = it->m_Gdr; // Girder that GSMS lives on
         GlfData& rlfData = m_SpanLoadFractionData.m_GirderLoadFractions[igdr];

         rlfData.GMSWs[barrswType]++;

         it++;
      }

      return true;
   }
   else
   {
      return false;
   }
}

void pgsBarrierSidewalkLoadDistributionTool::DistributeBSWLoadToNNearest(pgsTypes::TrafficBarrierOrientation orientation, Float64 bSwOffset, BarrSwType barrswType)
{
   // Load is distributed to N nearest girders
   m_SpanLoadFractionData.m_GMSWsAppliedTo[barrswType] = m_nMaxDistributed;

   // Compute intersection with line tangent to deck along sidewalk or barrier and mid-span reference line
   CComPtr<ILine2d> barswLine;
   if (orientation == pgsTypes::tboLeft)
   {
      m_LeftSlabEdgeLine->Clone(&barswLine);
      barswLine->Offset(-bSwOffset);
   }
   else
   {
      m_RightSlabEdgeLine->Clone(&barswLine);
      barswLine->Offset(bSwOffset);
   }

   CComPtr<IPoint2d> barswIntersect;
   m_GeomUtil->LineLineIntersect(m_MidSpanRefLine, barswLine, &barswIntersect);

   // Compute distances between barrer/sidewalk and all GMSW intersections
   std::vector<SwBDist> distances;

   GMSWInterSectionIterator it(m_GMSWInterSectionPoints.begin());
   GMSWInterSectionIterator it_end(m_GMSWInterSectionPoints.end());
   while(it!= it_end)
   {
      Float64 dist;
      barswIntersect->DistanceEx(it->m_IntersectionPoint, &dist);

      distances.push_back(SwBDist(it->m_Gdr, dist));

      it++;
   }

   // We have distances. Now sort them to get N nearest GMSW's
   SwBDistComparer comp(orientation);
   std::sort(distances.begin(), distances.end(), comp);

   // Finally, add number of participating GMSW's N nearest
   ATLASSERT(distances.size()>m_nMaxDistributed); // logic break: if this happens, load should have been distributed evenly to all gmsw's

   IndexType nidxs = min(distances.size(), m_nMaxDistributed);
   for (IndexType idx=0; idx<nidxs; idx++)
   {
      GirderIndexType igdr = distances[idx].m_Gdr;

      GlfData& rlfdata = m_SpanLoadFractionData.m_GirderLoadFractions[igdr];

      rlfdata.GMSWs[barrswType]++;
   }
}

bool pgsBarrierSidewalkLoadDistributionTool::DistributeSidewalkLoadUnderSw(pgsTypes::TrafficBarrierOrientation orientation, Float64 extEdgeOffset, Float64 intEdgeOffset, BarrSwType barrswType)
{
   // Create a line tangent to the interior edge of the sidewalk
   Float64 sign;
   CComPtr<ILine2d> swLine;
   if (orientation == pgsTypes::tboLeft)
   {
      m_LeftSlabEdgeLine->Clone(&swLine);
      swLine->Offset(-intEdgeOffset);
      sign = -1.0;
   }
   else
   {
      m_RightSlabEdgeLine->Clone(&swLine);
      swLine->Offset(-intEdgeOffset);
      sign = 1.0;
   }

   // Any GMSW's outboard of the interior edge of the sidewalk are considered to be underneath the sidwalk.
   // This might be considered a bug if a girder is outboard of the exterior barrier and ends up picking up sw load? (to me, models should never be this way-rdp)
   std::vector<GirderIndexType> GMSWs_under_sidewalk;

   GMSWInterSectionIterator it(m_GMSWInterSectionPoints.begin());
   GMSWInterSectionIterator it_end(m_GMSWInterSectionPoints.end());
   while(it!= it_end)
   {
      Float64 dist;
      m_GeomUtil->ShortestDistanceToPoint(swLine, it->m_IntersectionPoint, &dist);

      dist *= sign; // compensate for left or right sidewalk

      if (dist >= 0.0)
      {
         GMSWs_under_sidewalk.push_back(it->m_Gdr);
      }

      it++;
   }

   // If we have more than m_nMaxDistributed GMSW's beneath the sidewalk, distribute the load evenly between them
   if (GMSWs_under_sidewalk.size() >= m_nMaxDistributed)
   {
      m_SpanLoadFractionData.m_GMSWsAppliedTo[barrswType] = GMSWs_under_sidewalk.size();

      std::vector<GirderIndexType>::iterator gsit(GMSWs_under_sidewalk.begin());
      std::vector<GirderIndexType>::iterator gsit_end(GMSWs_under_sidewalk.end());
      while(gsit!= gsit_end)
      {
         GirderIndexType igdr = *gsit;
         GlfData& rlfdata = m_SpanLoadFractionData.m_GirderLoadFractions[igdr];
         rlfdata.GMSWs[barrswType]++;

         gsit++;
      }

      return true; // GSMW's under sidewalk controlled
   }
   else
   {
      return false; // some other distribution method must control
   }
}

void pgsBarrierSidewalkLoadDistributionTool::BuildGeometryModel()
{
   // clear out any old data
   m_GMSWInterSectionPoints.clear();

   // Build geometry model data for current span so we can compute distances from sidewalks/barriers to girders
   // First build reference line at mid-span.
   // Use points intersecting the left and right edges of pier at mid-span
   Float64 startPierStation = m_pIBridge->GetPierStation((PierIndexType)m_Span);
   Float64 endPierStation   = m_pIBridge->GetPierStation((PierIndexType)m_Span+1);
   Float64 midSpanStation = (startPierStation + endPierStation)/2.0;

   CComPtr<IDirection> startDirection, endDirection; 
   m_pIBridge->GetPierDirection((PierIndexType)m_Span, &startDirection);
   m_pIBridge->GetPierDirection((PierIndexType)m_Span+1, &endDirection);

   Float64 startDirVal, endDirVal;
   startDirection->get_Value(&startDirVal);
   endDirection->get_Value(&endDirVal);

   Float64 midDirVal = (startDirVal + endDirVal)/2.0;

   CComPtr<IDirection> midDirection;
   midDirection.CoCreateInstance(CLSID_Direction);
   midDirection->put_Value(midDirVal);

   CComPtr<IPoint2d> leftSlabEdgePoint, rightSlabEdgePoint;
   m_pIBridge->GetLeftSlabEdgePoint(midSpanStation, midDirection, &leftSlabEdgePoint);
   m_pIBridge->GetRightSlabEdgePoint(midSpanStation, midDirection, &rightSlabEdgePoint);

   m_MidSpanRefLine->ThroughPoints(rightSlabEdgePoint, leftSlabEdgePoint);

   // Create tangent lines to deck by creating points slightly further along deck
   Float64 nextStation = midSpanStation + ::ConvertToSysUnits(1.0,unitMeasure::Feet);

   CComPtr<IPoint2d> leftSlabEdgePoint1, rightSlabEdgePoint1;
   m_pIBridge->GetLeftSlabEdgePoint(nextStation, midDirection, &leftSlabEdgePoint1);
   m_pIBridge->GetRightSlabEdgePoint(nextStation, midDirection, &rightSlabEdgePoint1);

   m_LeftSlabEdgeLine->ThroughPoints(leftSlabEdgePoint, leftSlabEdgePoint1);
   m_RightSlabEdgeLine->ThroughPoints(rightSlabEdgePoint, rightSlabEdgePoint1);

   // Next store intersection points between mid-span reference line and
   // all GMSSW's across span
   CComPtr<ILine2d> constrLine; // a reusable construction line
   constrLine.CoCreateInstance(CLSID_Line2d);

   GirderIndexType nGirders = m_pIBridge->GetGirderCount(m_Span);
   for (GirderIndexType igdr=0; igdr<nGirders; igdr++)
   {
      // Create line along girder CL
      CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
      m_pIGirder->GetGirderEndPoints(m_Span, igdr, &pntPier1, &pntEnd1, &pntBrg1, &pntBrg2, &pntEnd2, &pntPier2);

      constrLine->ThroughPoints(pntPier1, pntPier2);

      if ( m_DistType == pgsTypes::tbdGirder )
      {
         CComPtr<IPoint2d> inters;
         m_GeomUtil->LineLineIntersect(m_MidSpanRefLine, constrLine, &inters);

         m_GMSWInterSectionPoints.push_back( GMSWInterSectionPoint(igdr, 0, inters) );
      }
      else 
      {
         // need poi at mid girder
         Float64 gl = m_pIBridge->GetGirderLength(m_Span, igdr);
         const pgsPointOfInterest mid_poi(m_Span, igdr, gl/2.0);

         if ( m_DistType == pgsTypes::tbdMatingSurface )
         {
            // get intersections for each mating surface
            MatingSurfaceIndexType nMs = m_pIGirder->GetNumberOfMatingSurfaces(m_Span,igdr); 
            for (MatingSurfaceIndexType ims=0; ims<nMs; ims++)
            {
               Float64 offset = -1.0 * m_pIGirder->GetMatingSurfaceLocation(mid_poi, ims);

               constrLine->Offset(offset); // move from cl of gider to ms location

               CComPtr<IPoint2d> inters;
               m_GeomUtil->LineLineIntersect(m_MidSpanRefLine, constrLine, &inters);
               m_GMSWInterSectionPoints.push_back( GMSWInterSectionPoint(igdr, 0, inters) );

               constrLine->Offset(-offset); // move back to original location
            }
         }
         else if ( m_DistType == pgsTypes::tbdWebs )
         {
            // get intersections for each web
            WebIndexType nWb = m_pIGirder->GetNumberOfWebs(m_Span,igdr);
            for (MatingSurfaceIndexType iwb=0; iwb<nWb; iwb++)
            {
               Float64 offset = -1.0 * m_pIGirder->GetWebLocation(mid_poi, iwb);

               constrLine->Offset(offset); // move from cl of gider to ms location

               CComPtr<IPoint2d> inters;
               m_GeomUtil->LineLineIntersect(m_MidSpanRefLine, constrLine, &inters);
               m_GMSWInterSectionPoints.push_back( GMSWInterSectionPoint(igdr, 0, inters) );

               constrLine->Offset(-offset); // move back to original location
            }
         }
      }
   }
}
