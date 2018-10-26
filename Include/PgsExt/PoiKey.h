///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#ifndef INCLUDED_POIKEY_H_
#define INCLUDED_POIKEY_H_

#include <PgsExt\PointOfInterest.h>

template <class T>
class TPoiKey
{
public:
   TPoiKey(T subkey,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) :
      m_Subkey(subkey), m_Poi(span,gdr,distFromStart) 
      {}

   TPoiKey(T subkey,const pgsPointOfInterest& poi) : 
      m_Subkey(subkey),m_Poi(poi)
      {}

   TPoiKey(const TPoiKey& rOther)
   {
      m_Subkey        = rOther.m_Subkey;
      m_Poi           = rOther.m_Poi;
   }

   TPoiKey& operator=(const TPoiKey& rOther)
   {
      m_Subkey        = rOther.m_Subkey;
      m_Poi           = rOther.m_Poi;
      return *this;
   }

   bool operator<(const TPoiKey& rOther) const
   {
      // this operator is just for container sorting. It has nothing to do with
      // temporal order or geometric order of POIs
      if (m_Subkey         < rOther.m_Subkey         ) return true;
      if (rOther.m_Subkey  < m_Subkey                ) return false;

      if ( m_Poi < rOther.m_Poi ) return true;
      if ( rOther.m_Poi < m_Poi ) return false;

      return false;
   }
private:
   T                  m_Subkey;
   pgsPointOfInterest m_Poi;
};

class PrestressSubKey
{
public:
   PrestressSubKey()
   {
      m_Stage = pgsTypes::AfterLosses;
      m_Strand = pgsTypes::Straight;
   }

   PrestressSubKey(pgsTypes::LossStage stage,pgsTypes::StrandType strand) :
      m_Stage(stage), m_Strand(strand)
      {
      }

   PrestressSubKey(const PrestressSubKey& rOther)
   {
      m_Stage = rOther.m_Stage;
      m_Strand = rOther.m_Strand;
   }

   bool operator<(const PrestressSubKey& rOther) const
   {
      if ( m_Stage < rOther.m_Stage ) return true;
      if ( rOther.m_Stage < m_Stage ) return false;

      if ( m_Strand < rOther.m_Strand ) return true;
      if ( rOther.m_Strand < m_Strand ) return false;

      return false;
   }

private:
   pgsTypes::LossStage m_Stage;
   pgsTypes::StrandType m_Strand;
};

typedef TPoiKey<pgsTypes::Stage> PoiStageKey;
typedef TPoiKey<pgsTypes::LossStage> PoiLossStageKey;
typedef TPoiKey<PrestressSubKey> PrestressPoiKey;
typedef TPoiKey<PoiIDType> PoiKey;

#endif // INCLUDED_POIKEY_H_