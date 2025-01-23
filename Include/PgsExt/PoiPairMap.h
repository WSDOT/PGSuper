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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\PointOfInterest.h>
#include <map>

typedef std::pair<PoiIDType,PoiIDType> PoiIDPairType;

// Utility class to mape points of interest to model POI IDs.
// Model POI IDs can be any POI-type ID in any model. This
class PGSEXTCLASS pgsPoiPairMap
{
public:
   pgsPoiPairMap();
   pgsPoiPairMap(const pgsPoiPairMap& rOther);
   pgsPoiPairMap& operator=(const pgsPoiPairMap& rOther);

   void AddMap(const pgsPointOfInterest& poi,PoiIDPairType modelPoi);
   void Clear();

   PoiIDPairType GetModelPoi(const pgsPointOfInterest& productPoi) const;

private:
   std::map<PoiIDType, PoiIDPairType> m_Map; // key = product POI, value = left, right model POI;
};
