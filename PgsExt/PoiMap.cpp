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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PoiMap.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsPoiMap::pgsPoiMap()
{
}

pgsPoiMap::pgsPoiMap(const pgsPoiMap& rOther)
{
   m_Map = rOther.m_Map;
}

pgsPoiMap& pgsPoiMap::operator=(const pgsPoiMap& rOther)
{
   if ( this != &rOther )
   {
      m_Map = rOther.m_Map;
   }

   return *this;
}

void pgsPoiMap::AddMap(const pgsPointOfInterest& poi,PoiIDType modelPoi)
{
   PoiIDType poiID = poi.GetID();
   if ( poiID != INVALID_ID )
   {
      m_Map.insert( std::make_pair(poiID,modelPoi) );
   }
}

void pgsPoiMap::Clear()
{
   m_Map.clear();
}

PoiIDType pgsPoiMap::GetModelPoi(const pgsPointOfInterest& productPoi) const
{
   std::map<PoiIDType,PoiIDType>::const_iterator found( m_Map.find( productPoi.GetID() ) );
   if ( found == m_Map.end() )
   {
      return INVALID_ID;
   }

   return found->second;
}

std::vector<PoiIDType> pgsPoiMap::GetModelPois() const
{
   std::vector<PoiIDType> poi;

   std::map<PoiIDType,PoiIDType>::const_iterator i(m_Map.begin());
   std::map<PoiIDType,PoiIDType>::const_iterator end(m_Map.begin());
   for ( ; i != end; i++ )
   {
      poi.push_back( (*i).second );
   }

   return poi;
}
