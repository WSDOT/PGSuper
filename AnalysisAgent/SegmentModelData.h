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
#pragma once

#include <PgsExt\PoiPairMap.h>
#include <WBFLFem2d.h>

class CSegmentModelData
{
public:
   CSegmentModelData();

   CSegmentModelData& operator=(const CSegmentModelData& other);

   CSegmentKey SegmentKey;
   IntervalIndexType IntervalIdx;
   Float64 Ec;
   CComPtr<IFem2dModel> Model;
   pgsPoiPairMap PoiMap;
   std::map<PoiIDType,LoadCaseIDType> UnitLoadIDMap; // maps product model POI ID to a FEM2D load case ID for a unit load at the corresponding poi in the Fem2d Model
   std::map<PoiIDType,LoadCaseIDType> UnitMomentIDMap; // maps product model POI ID to a FEM2D load case ID for a unit load at the corresponding poi in the Fem2d Model
   std::set<LoadCaseIDType> Loads; // keeps the ID of loads that have been applied to the model (except unit loads)
   std::map<std::_tstring,LoadCaseIDType> ExternalLoadMap; // maps externally created loads to FEM2D load case ID
};
