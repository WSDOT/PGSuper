///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
   bool BuildBridgeGeometryModel(const CBridgeDescription2* pBridgeDesc,ICogoModel* pCogoModel,IAlignment* pAlignment,IBridgeGeometry* pBridgeGeometry);

protected:
   bool LayoutPiers(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutTemporarySupports(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutUniformGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutGeneralGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);
   bool LayoutDiaphragmLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry);

   void ResolveGirderSpacingAtPier(IAlignment* pAlignment,Float64 alignmentOffset,const CPierData2* pPier,pgsTypes::PierFaceType pierFace,IPoint2dCollection** ppPoints,IBridgeGeometry* pBridgeGeometry);
   void ResolveGirderSpacingAtTempSupport(IAlignment* pAlignment,Float64 alignmentOffset,const CTemporarySupportData* pTS,IPoint2dCollection** ppPoints,IBridgeGeometry* pBridgeGeometry);

   // Resolves girder spacing from however it is input into an array of distances measured from the
   // alignment to the girder line, measured along the support direction.
   void ResolveGirderSpacing(IAlignment* pAlignment,Float64 alignmentOffset,Float64 supportStation,IDirection* pSupportDirection,const CGirderSpacing2* pSpacing,IPoint2dCollection** ppPoints,IBridgeGeometry* pBridgeGeometry);

   void GetPierID(const CPrecastSegmentData* pSegment,PierIDType* pStartID,PierIDType* pEndID);
   void GetPierSkewAngle(IAlignment* pAlignment,const CPierData2* pPier,IAngle** ppSkew);

   const CGirderGroupData* GetGirderGroup(const CGirderSpacing2* pSpacing);
   Float64 GetGirderWidth(const CSplicedGirderData* pGirder);

   CogoObjectID m_AlignmentID;
};