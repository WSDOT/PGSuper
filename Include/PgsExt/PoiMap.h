///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_POIMAP_H_
#define INCLUDED_POIMAP_H_
#include <PgsExt\PgsExtExp.h>

#include <map>
#include <PgsExt\PointOfInterest.h>

// Private class used by the analysis agent.
// Maps Product and Analysis model poi's
class PGSEXTCLASS pgsPoiMap
{
public:
   pgsPoiMap();
   pgsPoiMap(const pgsPoiMap& rOther);
   pgsPoiMap& operator=(const pgsPoiMap& rOther);

   void AddMap(const pgsPointOfInterest& poi,PoiIDType modelPoi);
   void Clear();
   PoiIDType GetProductPoi(PoiIDType modelPoi) const;
   PoiIDType GetModelPoi(const pgsPointOfInterest& productPoi) const;
   std::vector<PoiIDType> GetModelPois() const;

private:
   std::map<pgsPointOfInterest,PoiIDType> m_Map; // key = product POI;
};

#endif // INCLUDED_POIMAP_H_