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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLFem2d.h>
#include <WBFLCore.h>
#include <PgsExt\PointOfInterest.h>
#include "PoiMap.h"

class PGSEXTCLASS pgsGirderModelFactory
{
public:
   pgsGirderModelFactory(void);
   ~pgsGirderModelFactory(void);

   static void CreateGirderModel(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel,pgsPoiMap* pPoiMap);

   // searches the fem model of a girder to find the member ID and distance from start of member for
   // a location measured from the start of the girder
   static void FindMember(IFem2dModel* pModel,Float64 distFromStartOfModel,MemberIDType* pMbrID,Float64* pDistFromStartOfMbr);

   static PoiIDType AddPointOfInterest(IFem2dModel* pModel,const pgsPointOfInterest& poi);
   static std::vector<PoiIDType> AddPointsOfInterest(IFem2dModel* pModel,const std::vector<pgsPointOfInterest>& vPOI);

private:
   static PoiIDType ms_FemModelPoiID;
};
