///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <PgsExt\GirderPointOfInterest.h>

template <class T>
class TPoiKey
{
public:
   TPoiKey(T subkey,GroupIndexType grpIdx,GirderIndexType gdrIdx,Float64 distFromStart) :
      m_Subkey(subkey), m_Poi(grpIdx,gdrIdx,distFromStart) 
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

   const pgsPointOfInterest& GetPoi() const {return m_Poi;}
private:
   T                  m_Subkey;
   pgsPointOfInterest m_Poi;
};

typedef TPoiKey<IntervalIndexType> PoiIntervalKey;
typedef TPoiKey<PoiIDType> PoiIDKey;



template <class T>
class TSSMbrKey
{
public:
   TSSMbrKey(T subkey,GirderIDType gdrID,SegmentIndexType segIdx,Float64 distFromStartOfSegment) :
      m_Subkey(subkey), m_SSMbrID(gdrID), m_SegmentIdx(segIdx), m_DistFromStartOfSegment(distFromStartOfSegment) 
      {}

   TSSMbrKey(const TSSMbrKey& rOther)
   {
      m_Subkey        = rOther.m_Subkey;
      m_SSMbrID       = rOther.m_SSMbrID;
      m_SegmentIdx    = rOther.m_SegmentIdx;
      m_DistFromStartOfSegment = rOther.m_DistFromStartOfSegment;
   }

   TSSMbrKey& operator=(const TSSMbrKey& rOther)
   {
      m_Subkey        = rOther.m_Subkey;
      m_SSMbrID       = rOther.m_SSMbrID;
      m_SegmentIdx    = rOther.m_SegmentIdx;
      m_DistFromStartOfSegment = rOther.m_DistFromStartOfSegment;
      return *this;
   }

   bool operator<(const TSSMbrKey& rOther) const
   {
      // this operator is just for container sorting. It has nothing to do with
      // temporal order or geometric order of POIs
      if (m_Subkey         < rOther.m_Subkey         ) return true;
      if (rOther.m_Subkey  < m_Subkey                ) return false;

      if ( m_SSMbrID < rOther.m_SSMbrID ) return true;
      if ( rOther.m_SSMbrID < m_SSMbrID ) return false;

      if ( m_SegmentIdx < rOther.m_SegmentIdx ) return true;
      if ( rOther.m_SegmentIdx < m_SegmentIdx ) return false;

      if ( m_DistFromStartOfSegment < rOther.m_DistFromStartOfSegment ) return true;
      if ( rOther.m_DistFromStartOfSegment < m_DistFromStartOfSegment ) return false;

      return false;
   }
private:
   T                  m_Subkey;
   GirderIDType m_SSMbrID;
   SegmentIndexType m_SegmentIdx;
   Float64 m_DistFromStartOfSegment;
};

typedef TSSMbrKey<IntervalIndexType> SSMbrIntervalKey;

#endif // INCLUDED_POIKEY_H_