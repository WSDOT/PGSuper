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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PoiPairMap.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsPoiPairMap::pgsPoiPairMap()
{
}

pgsPoiPairMap::pgsPoiPairMap(const pgsPoiPairMap& rOther)
{
   m_Map = rOther.m_Map;
}

pgsPoiPairMap& pgsPoiPairMap::operator=(const pgsPoiPairMap& rOther)
{
   if ( this != &rOther )
   {
      m_Map = rOther.m_Map;
   }

   return *this;
}

void pgsPoiPairMap::AddMap(const pgsPointOfInterest& poi,PoiIDPairType modelPoi)
{
   PoiIDType poiID = poi.GetID();
   if ( poiID != INVALID_ID )
   {
      m_Map.insert( std::make_pair(poiID,modelPoi) );
   }
}

void pgsPoiPairMap::Clear()
{
   m_Map.clear();
}

PoiIDPairType pgsPoiPairMap::GetModelPoi(const pgsPointOfInterest& productPoi) const
{
   std::map<PoiIDType,PoiIDPairType>::const_iterator found( m_Map.find( productPoi.GetID() ) );
   if ( found == m_Map.end() )
   {
      return PoiIDPairType(INVALID_ID,INVALID_ID);
   }

   return found->second;
}

