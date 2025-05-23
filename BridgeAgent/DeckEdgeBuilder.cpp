///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "BridgeAgent.h"
#include "DeckEdgeBuilder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDeckEdgeBuilder::CDeckEdgeBuilder()
{
}

CDeckEdgeBuilder::~CDeckEdgeBuilder()
{
}

void CDeckEdgeBuilder::BuildDeckEdges(const CBridgeDescription2* pBridgeDesc,ICogoEngine* pCogoEngine,IAlignment* pAlignment,
                                      IPath** ppLeftEdgePath,IPath** ppRightEdgePath)
{
   m_pBridgeDesc = pBridgeDesc;
   m_CogoEngine  = pCogoEngine;
   m_Alignment   = pAlignment;

   const CDeckDescription2* pDeck = m_pBridgeDesc->GetDeckDescription();
   m_DeckPoints = pDeck->DeckEdgePoints;

   Float64 startPierStation = m_pBridgeDesc->GetPier(0)->GetStation();
   Float64 lastPierStation  = m_pBridgeDesc->GetPier(m_pBridgeDesc->GetPierCount()-1)->GetStation();

   if ( startPierStation < m_DeckPoints.front().Station )
   {
      // if the first deck point is after the start of the bridge,
      // create deck point that makes the deck edge parallel to the alignment
      // from the start of the bridge to the first deck point
      CDeckPoint dp;
      dp = m_DeckPoints.front();
      dp.Station = startPierStation;
      dp.LeftTransitionType  = pgsTypes::dptParallel;
      dp.RightTransitionType = pgsTypes::dptParallel;

      // insert before start of collection
      m_DeckPoints.insert(m_DeckPoints.begin(),dp);
   }

   if ( m_DeckPoints.back().Station < lastPierStation )
   {
      // if the last deck point is before the end of the bridge
      // create a deck point at the end of the bridge
      CDeckPoint dp;
      dp = m_DeckPoints.back();
      dp.Station = lastPierStation;
      dp.LeftTransitionType  = pgsTypes::dptLinear;
      dp.RightTransitionType = pgsTypes::dptLinear;

      // make the previous point have a parallel transition to the point at the end of the bridge
      m_DeckPoints.back().LeftTransitionType  = pgsTypes::dptParallel;
      m_DeckPoints.back().RightTransitionType = pgsTypes::dptParallel;

      m_DeckPoints.push_back(dp);
   }

   if (m_DeckPoints.size() < 2)
   {
      // we need a minimum of 2 points to defined the deck
      ASSERT(m_DeckPoints.size() == 1);
      CDeckPoint dp = m_DeckPoints.back();
      dp.Station += lastPierStation;
      dp.LeftTransitionType = pgsTypes::dptLinear;
      dp.RightTransitionType = pgsTypes::dptLinear;
      ASSERT(m_DeckPoints.back().Station < dp.Station); // don't want the deck points to be out of order or at the same location

      // make the previous point have a parallel transition to the point at the end of the bridge
      m_DeckPoints.back().LeftTransitionType = pgsTypes::dptParallel;
      m_DeckPoints.back().RightTransitionType = pgsTypes::dptParallel;

      m_DeckPoints.push_back(dp);
   }

   // start state
   m_LeftEdgeState  = -1;
   m_RightEdgeState = -1;

   m_LeftPath.CoCreateInstance(CLSID_Path);
   m_RightPath.CoCreateInstance(CLSID_Path);

   m_nDeckPoints = m_DeckPoints.size();
   for ( m_DeckPointIdx = 0; m_DeckPointIdx < m_nDeckPoints; m_DeckPointIdx++ )
   {
      NextDeckPoint();
   }

   GetDeckEdges(ppLeftEdgePath,ppRightEdgePath);

   m_LeftPath.Release();
   m_RightPath.Release();
}

void CDeckEdgeBuilder::NextDeckPoint()
{
   CDeckPoint deckPoint = m_DeckPoints[m_DeckPointIdx];
   pgsTypes::DeckPointTransitionType leftTransition  = deckPoint.LeftTransitionType;
   pgsTypes::DeckPointTransitionType rightTransition = deckPoint.RightTransitionType;

   // Locate the deck edge points in X-Y space
   CComPtr<IDirection> alignmentNormal;
   m_Alignment->GetNormal(CComVariant(deckPoint.Station),&alignmentNormal);

   Float64 left_offset;
   Float64 right_offset;

   if ( deckPoint.MeasurementType == pgsTypes::omtAlignment )
   {
      // deck edge is measured from the alignment
      left_offset  =  deckPoint.LeftEdge;
      right_offset = -deckPoint.RightEdge;
   }
   else
   {
      // deck edge is measured from the CL bridge. compute the offsets from the alignment
      Float64 alignment_offset = m_pBridgeDesc->GetAlignmentOffset();
      left_offset  =  deckPoint.LeftEdge - alignment_offset;
      right_offset = -deckPoint.RightEdge - alignment_offset;
   }

   CComPtr<IPoint2d> left_point, right_point;
   m_Alignment->LocatePoint(CComVariant(deckPoint.Station),omtAlongDirection, -left_offset, CComVariant(alignmentNormal),&left_point);
   m_Alignment->LocatePoint(CComVariant(deckPoint.Station),omtAlongDirection, -right_offset,CComVariant(alignmentNormal),&right_point);

   if ( m_LeftEdgeState < 0 )
   {
      if ( deckPoint.LeftTransitionType == pgsTypes::dptLinear )
      {
         m_LeftEdgeState = BeginLinearTransition(m_LeftPath,left_point, &m_LeftLinearTransitionStartPoint,deckPoint.LeftTransitionType);
      }
      else if ( deckPoint.LeftTransitionType == pgsTypes::dptSpline )
      {
         m_LeftEdgeState = BeginSpline(m_LeftPath,left_point,&m_LeftSpline,deckPoint.LeftTransitionType,true/*left*/);
      }
      else if ( deckPoint.LeftTransitionType == pgsTypes::dptParallel )
      {
         m_LeftEdgeState = BeginParallel(deckPoint.Station,left_offset,&m_LeftStartStation,&m_LeftOffset,deckPoint.LeftTransitionType);
      }
   }
   else if ( m_LeftEdgeState == pgsTypes::dptLinear )
   {
      // currently in linear transition state
      // end linear transition
      m_LeftEdgeState = EndLinearTransition(m_LeftPath,left_point, m_LeftLinearTransitionStartPoint,deckPoint.LeftTransitionType);
      m_LeftLinearTransitionStartPoint.Release();

      // state change
      if ( m_LeftEdgeState == pgsTypes::dptLinear )
      {
         m_LeftEdgeState = BeginLinearTransition(m_LeftPath,left_point, &m_LeftLinearTransitionStartPoint,deckPoint.LeftTransitionType);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptSpline )
      {
         m_LeftEdgeState = BeginSpline(m_LeftPath,left_point,&m_LeftSpline,deckPoint.LeftTransitionType,true/*left*/);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptParallel )
      {
         m_LeftEdgeState = BeginParallel(deckPoint.Station,left_offset,&m_LeftStartStation,&m_LeftOffset,deckPoint.LeftTransitionType);
      }
   }
   else if ( m_LeftEdgeState == pgsTypes::dptSpline )
   {
      // currently in spline state
      if ( deckPoint.LeftTransitionType != pgsTypes::dptSpline )
      {
         // end spline
         m_LeftEdgeState = EndSpline(m_LeftPath,left_point,m_LeftSpline,deckPoint.LeftTransitionType,true/*left*/);
      }

      if ( m_LeftEdgeState == pgsTypes::dptLinear )
      {
         // moving into a linear transitions. 
         m_LeftEdgeState = BeginLinearTransition(m_LeftPath, left_point, &m_LeftLinearTransitionStartPoint, deckPoint.LeftTransitionType);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptParallel )
      {
         // moving into a parallel transition state
         m_LeftEdgeState = BeginParallel(deckPoint.Station,left_offset,&m_LeftStartStation,&m_LeftOffset,deckPoint.LeftTransitionType);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptSpline )
      {
         // still in a spline state, capture the spline point
         m_LeftEdgeState = Spline(m_LeftPath,left_point,m_LeftSpline,deckPoint.LeftTransitionType);
      }
   }
   else if ( m_LeftEdgeState == pgsTypes::dptParallel )
   {
      // currently in parallel state
      if ( deckPoint.LeftTransitionType != pgsTypes::dptParallel )
      {
         m_LeftEdgeState = EndParallel(m_LeftPath,m_LeftStartStation,deckPoint.Station,m_LeftOffset,deckPoint.LeftTransitionType);
      }

      if ( m_LeftEdgeState == pgsTypes::dptLinear )
      {
         // moving into a linear transitions. 
         // capture the point at the end of the parallel section
         m_LeftEdgeState = BeginLinearTransition(m_LeftPath,left_point, &m_LeftLinearTransitionStartPoint,deckPoint.LeftTransitionType);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptSpline )
      {
         // beginning a spline state
         m_LeftEdgeState = BeginSpline(m_LeftPath,left_point,&m_LeftSpline,deckPoint.LeftTransitionType,true/*left*/);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptParallel )
      {
         // still in a parallel state. there is nothing to do
      }
   }


////////////   
   

   if ( m_RightEdgeState < 0 )
   {
      if ( deckPoint.RightTransitionType == pgsTypes::dptLinear )
      {
         m_RightEdgeState = BeginLinearTransition(m_RightPath,right_point, &m_RightLinearTransitionStartPoint,deckPoint.RightTransitionType);
      }
      else if ( deckPoint.RightTransitionType == pgsTypes::dptSpline )
      {
         m_RightEdgeState = BeginSpline(m_RightPath,right_point,&m_RightSpline,deckPoint.RightTransitionType,false/*right*/);
      }
      else if ( deckPoint.RightTransitionType == pgsTypes::dptParallel )
      {
         m_RightEdgeState = BeginParallel(deckPoint.Station,right_offset,&m_RightStartStation,&m_RightOffset,deckPoint.RightTransitionType);
      }
   }
   else if ( m_RightEdgeState == pgsTypes::dptLinear )
   {
      // currently in linear transition state
      // end linear transition
      m_RightEdgeState = EndLinearTransition(m_RightPath,right_point, m_RightLinearTransitionStartPoint,deckPoint.RightTransitionType);
      m_RightLinearTransitionStartPoint.Release();

      // state change
      if ( m_RightEdgeState == pgsTypes::dptLinear )
      {
         m_RightEdgeState = BeginLinearTransition(m_RightPath,right_point, &m_RightLinearTransitionStartPoint,deckPoint.RightTransitionType);
      }
      else if ( m_RightEdgeState == pgsTypes::dptSpline )
      {
         m_RightEdgeState = BeginSpline(m_RightPath,right_point,&m_RightSpline,deckPoint.RightTransitionType,false/*right*/);
      }
      else if ( m_RightEdgeState == pgsTypes::dptParallel )
      {
         m_RightEdgeState = BeginParallel(deckPoint.Station,right_offset,&m_RightStartStation,&m_RightOffset,deckPoint.RightTransitionType);
      }
   }
   else if ( m_RightEdgeState == pgsTypes::dptSpline )
   {
      // currently in spline state
      if ( deckPoint.RightTransitionType != pgsTypes::dptSpline )
      {
         // end spline
         m_RightEdgeState = EndSpline(m_RightPath,right_point,m_RightSpline,deckPoint.RightTransitionType,false/*right*/);
      }

      if ( m_RightEdgeState == pgsTypes::dptLinear )
      {
         // moving into a linear transitions. 
         m_RightEdgeState = BeginLinearTransition(m_RightPath, right_point, &m_RightLinearTransitionStartPoint, deckPoint.RightTransitionType);
      }
      else if ( m_RightEdgeState == pgsTypes::dptParallel )
      {
         // moving into a parallel transition state
         m_RightEdgeState = BeginParallel(deckPoint.Station,right_offset,&m_RightStartStation,&m_RightOffset,deckPoint.RightTransitionType);
      }
      else if ( m_RightEdgeState == pgsTypes::dptSpline )
      {
         // still in a spline state, capture the spline point
         m_RightEdgeState = Spline(m_RightPath,right_point,m_RightSpline,deckPoint.RightTransitionType);
      }
   }
   else if ( m_RightEdgeState == pgsTypes::dptParallel )
   {
      // currently in parallel state
      if ( deckPoint.RightTransitionType != pgsTypes::dptParallel )
      {
         m_RightEdgeState = EndParallel(m_RightPath,m_RightStartStation,deckPoint.Station,m_RightOffset,deckPoint.RightTransitionType);
      }

      if ( m_RightEdgeState == pgsTypes::dptLinear )
      {
         // moving into a linear transitions. 
         // capture the point at the end of the parallel section
         m_RightEdgeState = BeginLinearTransition(m_RightPath,right_point, &m_RightLinearTransitionStartPoint,deckPoint.RightTransitionType);
      }
      else if ( m_RightEdgeState == pgsTypes::dptSpline )
      {
         // beginning a spline state
         m_RightEdgeState = BeginSpline(m_RightPath,right_point,&m_RightSpline,deckPoint.RightTransitionType,false/*right*/);
      }
      else if ( m_RightEdgeState == pgsTypes::dptParallel )
      {
         // still in a parallel state. there is nothing to do
      }
   }
}

void CDeckEdgeBuilder::GetDeckEdges(IPath** ppLeftEdgePath,IPath** ppRightEdgePath)
{
   (*ppLeftEdgePath) = m_LeftPath;
   (*ppLeftEdgePath)->AddRef();

   (*ppRightEdgePath) = m_RightPath;
   (*ppRightEdgePath)->AddRef();
}

int CDeckEdgeBuilder::BeginSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline** ppSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft)
{
   ATLASSERT( *ppSpline == nullptr );

   CComPtr<ICubicSpline> spline;
   spline.CoCreateInstance(CLSID_CubicSpline);
   (*ppSpline) = spline;
   (*ppSpline)->AddRef();
   spline->AddPointEx(pPoint);

   const CDeckPoint& deckPoint = m_DeckPoints[m_DeckPointIdx];
   pgsTypes::DeckPointTransitionType prevTransition;
   if (m_DeckPointIdx == 0 )
   {
      prevTransition = pgsTypes::dptParallel;
   }
   else
   {
      const CDeckPoint& prevDeckPoint = m_DeckPoints[m_DeckPointIdx-1];
      prevTransition = (bLeft ? prevDeckPoint.LeftTransitionType : prevDeckPoint.RightTransitionType);
   }

   CComPtr<IDirection> tangent;
   if ( m_DeckPointIdx == 0 || prevTransition == pgsTypes::dptParallel )
   {
      // the deck edge description starts with a curve so make the tangent parallel the alignment
      m_Alignment->GetBearing(CComVariant(deckPoint.Station),&tangent);
   }
   else
   {
      // get the tangent direction from the path element before the start of the spline
      IndexType nPathElements;
      pPath->get_Count(&nPathElements);
      CComPtr<IPathElement> path_element;
      pPath->get_Item(nPathElements-1,&path_element);

      Float64 L;
      path_element->GetLength(&L);
      path_element->GetBearing(L, &tangent);
   }
   (*ppSpline)->put_StartDirection(CComVariant(tangent));

   return transition;
}

int CDeckEdgeBuilder::Spline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition)
{
   ATLASSERT(pSpline != nullptr );
   pSpline->AddPointEx(pPoint);
   return transition;
}

int CDeckEdgeBuilder::EndSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft)
{
   ATLASSERT(pSpline != nullptr );

   pSpline->AddPointEx(pPoint);

   CDeckPoint deckPoint = m_DeckPoints[m_DeckPointIdx];

   CComPtr<IDirection> tangent;
   // get the bearing at the end of the spline
   if ( m_DeckPointIdx == m_nDeckPoints-1 || transition == pgsTypes::dptParallel )
   {
      // this is the last point or the transition to next is parallel
      // make the curve tangent parallel the alignment
      m_Alignment->GetBearing(CComVariant(deckPoint.Station),&tangent);
   }
   else
   {
      // get the next point and use it with the current point to determine a bearing
      const CDeckPoint& nextDeckPoint = m_DeckPoints[m_DeckPointIdx+1];

      // Locate the deck edge points in X-Y space
      CComPtr<IDirection> nextAlignmentNormal;
      m_Alignment->GetNormal(CComVariant(nextDeckPoint.Station),&nextAlignmentNormal);

      // does alignment offset need to be considered if measured from alignment?
      Float64 offset = (bLeft ? nextDeckPoint.LeftEdge : -nextDeckPoint.RightEdge);

      CComPtr<IPoint2d> next_point;
      m_Alignment->LocatePoint(CComVariant(nextDeckPoint.Station), omtAlongDirection, -offset, CComVariant(nextAlignmentNormal),&next_point);

      CComQIPtr<IMeasure2> measure(m_CogoEngine);
      measure->Direction(pPoint, next_point, &tangent);
   }
   pSpline->put_EndDirection(CComVariant(tangent));

   CComQIPtr<IPathElement> pe(pSpline);
   pPath->Add(pe);

   pSpline = nullptr;

   return (m_DeckPointIdx == m_nDeckPoints - 1 ? -1 : transition);
}

int CDeckEdgeBuilder::BeginParallel(Float64 station,Float64 offset,Float64* pStartStation,Float64* pOffset,pgsTypes::DeckPointTransitionType transition)
{
   *pStartStation = station;
   *pOffset = offset;
   return transition;
}

int CDeckEdgeBuilder::EndParallel(IPath* pPath,Float64 startStation,Float64 endStation,Float64 offset,pgsTypes::DeckPointTransitionType transition)
{
   CComPtr<IPath> subPath;
   m_Alignment->CreateSubPath(CComVariant(startStation),CComVariant(endStation),&subPath);

   CComQIPtr<IPathElement> element(subPath);
   CComPtr<IPath> edgeSubPath;
   element->CreateOffsetPath(-offset,&edgeSubPath);

   IndexType nPathElements;
   edgeSubPath->get_Count(&nPathElements);
   for ( IndexType i = 0; i < nPathElements; i++ )
   {
      CComPtr<IPathElement> pe;
      edgeSubPath->get_Item(i,&pe);

      CComPtr<IPathElement> clone;
      pe->Clone(&clone);
      pPath->Add(clone);
   }

   return (m_DeckPointIdx == m_nDeckPoints - 1 ? -1 : transition);
}

int CDeckEdgeBuilder::BeginLinearTransition(IPath* pPath, IPoint2d* pPoint, IPoint2d** ppBeginPoint, pgsTypes::DeckPointTransitionType transition)
{
   (*ppBeginPoint) = pPoint;
   (*ppBeginPoint)->AddRef();
   return (m_DeckPointIdx == m_nDeckPoints - 1 ? -1 : transition);
}

int CDeckEdgeBuilder::EndLinearTransition(IPath* pPath,IPoint2d* pPoint,IPoint2d* pBeginPoint,pgsTypes::DeckPointTransitionType transition)
{
   CHECK(pPath);
   CHECK(pPoint);
   CHECK(pBeginPoint);
   CComPtr<IPathSegment> segment;
   segment.CoCreateInstance(CLSID_PathSegment);
   segment->ThroughPoints(pBeginPoint, pPoint);
   CComQIPtr<IPathElement> pe(segment);
   pPath->Add(pe);
   return (m_DeckPointIdx == m_nDeckPoints-1 ? -1 : transition);
}

Float64 CDeckEdgeBuilder::GetAlignmentOffset(Float64 station)
{
   return m_pBridgeDesc->GetAlignmentOffset();
}
