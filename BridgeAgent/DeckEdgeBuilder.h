///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <PsgLib\BridgeDescription2.h>
#include <PsgLib\DeckPoint.h>

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
   IndexType m_DeckPointIdx;
   IndexType m_nDeckPoints;

   CComPtr<IPoint2d> m_LeftLinearTransitionStartPoint;
   CComPtr<IPoint2d> m_RightLinearTransitionStartPoint;

   CComPtr<IPath> m_LeftPath, m_RightPath;
   CComPtr<ICubicSpline> m_LeftSpline, m_RightSpline;
   CComPtr<IPath> m_LeftSubPath, m_RightSubPath;

   // data for building the parallel sub-path
   Float64 m_LeftStartStation, m_RightStartStation;
   Float64 m_LeftOffset, m_RightOffset;

   void NextDeckPoint();
   void GetDeckEdges(IPath** ppLeftEdgePath,IPath** ppRightEdgePath);
   
   int BeginSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline** ppSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft);
   int Spline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition);
   int EndSpline(IPath* pPath,IPoint2d* pPoint,ICubicSpline* pSpline,pgsTypes::DeckPointTransitionType transition,bool bLeft);

   int BeginParallel(Float64 station,Float64 offset,Float64* pStartStation,Float64* pOffset,pgsTypes::DeckPointTransitionType transition);
   int EndParallel(IPath* pPath,Float64 startStation,Float64 endStation,Float64 offset,pgsTypes::DeckPointTransitionType transition);

   int BeginLinearTransition(IPath* pPath, IPoint2d* pPoint, IPoint2d** ppBeginPoint,pgsTypes::DeckPointTransitionType transition);
   int EndLinearTransition(IPath* pPath,IPoint2d* pPoint,IPoint2d* pBeginPoint,pgsTypes::DeckPointTransitionType transition);

   Float64 GetAlignmentOffset(Float64 station);
};

#endif // INCLUDED_DECKEDGEBUILDER_H_