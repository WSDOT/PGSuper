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

#ifndef INCLUDED_DECKEDGEBUILDER_H_
#define INCLUDED_DECKEDGEBUILDER_H_

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckPoint.h>

class CDeckEdgeBuilder
{
public:
   CDeckEdgeBuilder();
   ~CDeckEdgeBuilder();
   void BuildDeckEdges(const CBridgeDescription2* pBridgeDesc,ICogoEngine* pCogoEngine,IAlignment* pAlignment,IPath** ppLeftEdgePath,IPath** ppRightEdgePath);

private:
   const CBridgeDescription2* m_pBridgeDesc;
   int m_LeftEdgeState;
   int m_RightEdgeState;
   CComPtr<ICogoEngine> m_CogoEngine;
   CComPtr<IAlignment> m_Alignment;
   std::vector<CDeckPoint> m_DeckPoints;
   CollectionIndexType m_DeckPointIdx;
   CollectionIndexType m_nDeckPoints;

   CComPtr<IPath> m_LeftPath, m_RightPath;
   CComPtr<ICubicSpline> m_LeftSpline, m_RightSpline;
   CComPtr<IPath> m_LeftSubPath, m_RightSubPath;

   // data for building the parallel sub-path
   Float64 m_LeftStartStation, m_RightStartStation;
   Float64 m_LeftOffset, m_RightOffset;

   void NextDeckPoint();
   void GetDeckEdges(IPath** ppLeftEdgePath,IPath** ppRightEdgePath);
   
   pgsTypes::DeckPointTransitionType BeginSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline** ppSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft);
   pgsTypes::DeckPointTransitionType Spline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition);
   pgsTypes::DeckPointTransitionType EndSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft);

   pgsTypes::DeckPointTransitionType BeginParallel(Float64 station,Float64 offset,Float64* pStartStation,Float64* pOffset,pgsTypes::DeckPointTransitionType transition);
   pgsTypes::DeckPointTransitionType EndParallel(IPath* pPath,Float64 startStation,Float64 endStation,Float64 offset,pgsTypes::DeckPointTransitionType transition);

   pgsTypes::DeckPointTransitionType LinearTransition(IPath* pPath,IPoint2d* pPoint,pgsTypes::DeckPointTransitionType transition);

   Float64 GetAlignmentOffset(Float64 station);
};

#endif // INCLUDED_DECKEDGEBUILDER_H_