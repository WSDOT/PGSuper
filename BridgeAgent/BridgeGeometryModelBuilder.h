///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#pragma once

#include <PgsExt\BridgeDescription2.h>

// Collection to store girder orientation angle and distance the girder layout line is shifted normal to itself to account 
// for the work point location and orientation angle.
struct GirderOrientationShift
{
   Float64 m_Orientation;
   Float64 m_LayoutLineShift; // distance normal to girder layout line

   GirderOrientationShift() :
      m_Orientation(0.0), m_LayoutLineShift(0.0)
   {
      ;
   }

   GirderOrientationShift(Float64 orientation, Float64 shift) :
      m_Orientation(orientation), m_LayoutLineShift(shift)
   {
      ;
   }
};
typedef std::map<CSegmentKey, GirderOrientationShift>  GirderOrientationCollection;

///////////////////////////////////////////////////////////////////////////////
// class CBridgeGeometryModelBuilder
//
// This is a utility class that creates Bridge Geometry models from the bridge description
//
// Using a factory class takes some of the code burden off of the bridge agent itself

class CBridgeGeometryModelBuilder
{
public:
   CBridgeGeometryModelBuilder();
   bool BuildBridgeGeometryModel(const CBridgeDescription2* pBridgeDesc,ICogoModel* pCogoModel,IAlignment* pAlignment,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& coll);

   static IDType AlignmentID;
   static IDType ProfileID;
   static IDType SurfaceID;

protected:
   bool LayoutPiers(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutTemporarySupports(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& coll);
   bool LayoutUniformGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& coll);
   bool LayoutGeneralGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& coll);
   bool LayoutDiaphragmLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);

   // Resolves segment spacing from however it is input into an array of distances measured from the
   // alignment to the girder line, measured along the support direction.
   void ResolveSegmentSpacing(IBridgeGeometry* pBridgeGeometry, Float64 alignmentOffset,const CPrecastSegmentData* pSegment,IPoint2dCollection** ppStartPoints,IPoint2dCollection** ppEndPoints);
   Float64 ComputeGirderOrientation(pgsTypes::GirderOrientationType orientType, const CSplicedGirderData* pGirder, IBridgeGeometry* pBridgeGeometry, IPoint2d* pStartPoint, IPoint2d* pEndPoint);

   Float64 GetSkewAngle(IAlignment* pAlignment,Float64 measureStation,IDirection* pMeasureDirection);
   Float64 GetLeftGirderOffset(IAlignment* pAlignment, Float64 alignmentOffset, Float64 measureStation, IDirection* pMeasureDirection, const CGirderSpacing2* pSpacing);
   void GetSpacingDataAtPier(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CPierData2* pPier,pgsTypes::PierFaceType pierFace,Float64* pMeasureStation,IDirection** ppMeasureDirection,IDirection** ppSupportDirection,const CGirderSpacing2** ppSpacing);
   void GetSpacingDataAtTempSupport(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CTemporarySupportData* pTS,Float64* pMeasureStation,IDirection** ppMeasureDirection,IDirection** ppSupportDirection,const CGirderSpacing2** ppSpacing);

   void ResolveSegmentSpacing(IBridgeGeometry* pBridgeGeometry,
                              Float64 alignmentOffset,
                              Float64 startMeasureStation,
                              IDirection* pStartMeasureDirection,
                              IDirection* pStartSupportDirection,
                              const CGirderSpacing2* pStartSpacing,
                              Float64 endMeasureStation,
                              IDirection* pEndMeasureDirection,
                              IDirection* pEndSupportDirection,
                              const CGirderSpacing2* pEndSpacing,
                              IPoint2dCollection** ppStartPoints,
                              IPoint2dCollection** ppEndPoints);

   void GetPierID(const CPrecastSegmentData* pSegment,PierIDType* pStartID,PierIDType* pEndID);
   void GetPierDirection(IAlignment* pAlignment,const CPierData2* pPier,IAngle** ppSkew,IDirection** ppDirection);
   void GetTempSupportDirection(IAlignment* pAlignment,const CTemporarySupportData* pTS,IAngle** ppSkew,IDirection** ppDirection);
   void GetSkewAndDirection(IAlignment* pAlignment,Float64 station,LPCTSTR strOrientation,IAngle** ppSkew,IDirection** ppDirection);

   const CGirderGroupData* GetGirderGroup(const CGirderSpacing2* pSpacing);
   void GetGirderWidth(const CSplicedGirderData* pGirder,pgsTypes::MemberEndType endType,Float64* pLeft,Float64* pRight);

   // Gets the pier line properties based on girder spacing, measured along the CL pier
   void GetPierLineProperties(const CBridgeDescription2* pBridgeDesc,const CGirderSpacing2* pSpacing,Float64 skewAngle,Float64* pWidth,Float64* pOffset);

};