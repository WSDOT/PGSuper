///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

void CDeckEdgeBuilder::BuildDeckEdges(const CBridgeDescription* pBridgeDesc,ICogoEngine* pCogoEngine,IAlignment* pAlignment,
                                      IPath** ppLeftEdgePath,IPath** ppRightEdgePath)
{
   m_pBridgeDesc = pBridgeDesc;
   m_CogoEngine = pCogoEngine;
   m_Alignment = pAlignment;

   const CDeckDescription* pDeck = m_pBridgeDesc->GetDeckDescription();
   m_DeckPoints = pDeck->DeckEdgePoints;

   double startPierStation = m_pBridgeDesc->GetPier(0)->GetStation();
   double lastPierStation  = m_pBridgeDesc->GetPier(m_pBridgeDesc->GetPierCount()-1)->GetStation();

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

   // start state
   m_LeftEdgeState  = -1;
   m_RightEdgeState = -1;

   m_LeftPath.CoCreateInstance(CLSID_Path);
   m_RightPath.CoCreateInstance(CLSID_Path);

   m_nDeckPoints = m_DeckPoints.size();
   for ( m_DeckPointIdx = 0; m_DeckPointIdx < m_nDeckPoints; m_DeckPointIdx++ )
      NextDeckPoint();

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
   m_Alignment->Normal(CComVariant(deckPoint.Station),&alignmentNormal);

   double left_offset;
   double right_offset;

   if ( deckPoint.MeasurementType == pgsTypes::omtAlignment )
   {
      // deck edge is measured from the alignment
      left_offset  = -deckPoint.LeftEdge;
      right_offset =  deckPoint.RightEdge;
   }
   else
   {
      // deck edge is measured from the CL bridge. compute the offsets from the alignment
      double alignment_offset = m_pBridgeDesc->GetAlignmentOffset();
      left_offset  = alignment_offset - deckPoint.LeftEdge;
      right_offset = alignment_offset + deckPoint.RightEdge;
   }

   CComPtr<IPoint2d> left_point, right_point;
   m_Alignment->LocatePoint(CComVariant(deckPoint.Station),omtAlongDirection, left_offset, CComVariant(alignmentNormal),&left_point);
   m_Alignment->LocatePoint(CComVariant(deckPoint.Station),omtAlongDirection, right_offset,CComVariant(alignmentNormal),&right_point);

   if ( m_LeftEdgeState < 0 )
   {
      if ( deckPoint.LeftTransitionType == pgsTypes::dptLinear )
      {
         m_LeftEdgeState = LinearTransition(m_LeftPath,left_point,deckPoint.LeftTransitionType);
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
      if ( deckPoint.LeftTransitionType != pgsTypes::dptLinear )
      {
         // end linear transition
         m_LeftEdgeState = LinearTransition(m_LeftPath,left_point,deckPoint.LeftTransitionType);
      }

      // state change
      if ( m_LeftEdgeState == pgsTypes::dptLinear )
      {
         m_LeftEdgeState = LinearTransition(m_LeftPath,left_point,deckPoint.LeftTransitionType);
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
         // moving into a linear transtions. 
         // do nothing
      }
      else if ( m_LeftEdgeState == pgsTypes::dptParallel )
      {
         // moving into a parallel transition state
         m_LeftEdgeState = BeginParallel(deckPoint.Station,left_offset,&m_LeftStartStation,&m_LeftOffset,deckPoint.LeftTransitionType);
      }
      else if ( m_LeftEdgeState == pgsTypes::dptSpline )
      {
         // still in a spline state, capture the splien point
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
         // moving into a linear transtions. 
         // do nothing
      }
      else if ( m_LeftEdgeState == pgsTypes::dptSpline )
      {
         // begining a splien state
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
         m_RightEdgeState = LinearTransition(m_RightPath,right_point,deckPoint.RightTransitionType);
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
      if ( deckPoint.RightTransitionType != pgsTypes::dptLinear )
      {
         // end linear transition
         m_RightEdgeState = LinearTransition(m_RightPath,right_point,deckPoint.RightTransitionType);
      }

      // state change
      if ( m_RightEdgeState == pgsTypes::dptLinear )
      {
         m_RightEdgeState = LinearTransition(m_RightPath,right_point,deckPoint.RightTransitionType);
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
         // moving into a linear transtions. 
         // do nothing
      }
      else if ( m_RightEdgeState == pgsTypes::dptParallel )
      {
         // moving into a parallel transition state
         m_RightEdgeState = BeginParallel(deckPoint.Station,right_offset,&m_RightStartStation,&m_RightOffset,deckPoint.RightTransitionType);
      }
      else if ( m_RightEdgeState == pgsTypes::dptSpline )
      {
         // still in a spline state, capture the splien point
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
         // moving into a linear transtions. 
         // do nothing
      }
      else if ( m_RightEdgeState == pgsTypes::dptSpline )
      {
         // begining a splien state
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

pgsTypes::DeckPointTransitionType CDeckEdgeBuilder::BeginSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline** ppSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft)
{
   ATLASSERT( *ppSpline == NULL );

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
      const CDeckPoint& prevDeckPoint = m_DeckPoints[m_DeckPointIdx];
      prevTransition = (bLeft ? prevDeckPoint.LeftTransitionType : prevDeckPoint.RightTransitionType);
   }

   CComPtr<IDirection> tangent;
   if ( m_DeckPointIdx == 0 || prevTransition == pgsTypes::dptParallel )
   {
      // the deck edge description starts with a curve so make the tangent parallel the alignment
      m_Alignment->Bearing(CComVariant(deckPoint.Station),&tangent);
   }
   else
   {
      // compute a tangent direction using the last point and the current point
      CollectionIndexType nPathElements;
      pPath->get_Count(&nPathElements);
      CComPtr<IPathElement> path_element;
      pPath->get_Item(nPathElements-1,&path_element);

      PathElementType type;
      path_element->get_Type(&type);

      CComPtr<IUnknown> punk;
      path_element->get_Value(&punk);

      if ( type == petPoint )
      {
         CComQIPtr<IPoint2d> prev_point(punk);

         CComQIPtr<IMeasure2> measure(m_CogoEngine);
         measure->Direction(prev_point, pPoint, &tangent);
      }
      else if ( type == petHorzCurve )
      {
         CComQIPtr<IHorzCurve> hc(punk);
         hc->get_FwdTangentBrg(&tangent);
      }
      else if ( type == petLineSegment )
      {
         CComQIPtr<IMeasure2> measure(m_CogoEngine);
         CComPtr<IPoint2d> p1,p2;
         CComQIPtr<ILineSegment2d> ls(punk);
         ls->get_StartPoint(&p1);
         ls->get_EndPoint(&p2);
         measure->Direction(p1,p2,&tangent);
      }
      else
      {
         ATLASSERT(false); // should never get here
      }
   }
   (*ppSpline)->put_StartDirection(CComVariant(tangent));

   return transition;
}

pgsTypes::DeckPointTransitionType CDeckEdgeBuilder::Spline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition)
{
   ATLASSERT(pSpline != NULL );
   pSpline->AddPointEx(pPoint);
   return transition;
}

pgsTypes::DeckPointTransitionType CDeckEdgeBuilder::EndSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft)
{
   ATLASSERT(pSpline != NULL );

   pSpline->AddPointEx(pPoint);

   CDeckPoint deckPoint = m_DeckPoints[m_DeckPointIdx];

   CComPtr<IDirection> tangent;
   // get the bearing at the end of the spline
   if ( m_DeckPointIdx == m_nDeckPoints-1 || transition == pgsTypes::dptParallel )
   {
      // this is the last point or the transition to next is parallel
      // make the curve tangent parallel the alignment
      m_Alignment->Bearing(CComVariant(deckPoint.Station),&tangent);
   }
   else
   {
      // get the next point and use it with the current point to determine a bearing
      const CDeckPoint& nextDeckPoint = m_DeckPoints[m_DeckPointIdx+1];

      // Locate the deck edge points in X-Y space
      CComPtr<IDirection> nextAlignmentNormal;
      m_Alignment->Normal(CComVariant(nextDeckPoint.Station),&nextAlignmentNormal);

      double offset  = (bLeft ? -nextDeckPoint.LeftEdge : nextDeckPoint.RightEdge);

      CComPtr<IPoint2d> next_point;
      m_Alignment->LocatePoint(CComVariant(nextDeckPoint.Station), omtAlongDirection, offset, CComVariant(nextAlignmentNormal),&next_point);

      CComQIPtr<IMeasure2> measure(m_CogoEngine);
      measure->Direction(pPoint, next_point, &tangent);
   }
   pSpline->put_EndDirection(CComVariant(tangent));

   pPath->AddEx(pSpline);

   pSpline = NULL;

   return transition;
}

pgsTypes::DeckPointTransitionType CDeckEdgeBuilder::BeginParallel(double station,double offset,double* pStartStation,double* pOffset,pgsTypes::DeckPointTransitionType transition)
{
   *pStartStation = station;
   *pOffset = offset;
   return transition;
}

pgsTypes::DeckPointTransitionType CDeckEdgeBuilder::EndParallel(IPath* pPath,double startStation,double endStation,double offset,pgsTypes::DeckPointTransitionType transition)
{
   CComPtr<IPath> subPath;
   m_Alignment->CreateSubPath(CComVariant(startStation),CComVariant(endStation),&subPath);

   CComPtr<IPath> edgeSubPath;
   subPath->CreateParallelPath(offset,&edgeSubPath);

   CollectionIndexType nPathElements;
   edgeSubPath->get_Count(&nPathElements);
   for ( CollectionIndexType i = 0; i < nPathElements; i++ )
   {
      CComPtr<IPathElement> pe;
      edgeSubPath->get_Item(i,&pe);

      pPath->Add(pe);
   }

   return transition;
}

pgsTypes::DeckPointTransitionType CDeckEdgeBuilder::LinearTransition(IPath* pPath,IPoint2d* pPoint,pgsTypes::DeckPointTransitionType transition)
{
   pPath->AddEx(pPoint);
   return transition;
}

double CDeckEdgeBuilder::GetAlignmentOffset(double station)
{
   return m_pBridgeDesc->GetAlignmentOffset();
}
