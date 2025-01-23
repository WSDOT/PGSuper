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

#ifndef INCLUDED_POIKEY_H_
#define INCLUDED_POIKEY_H_

#include <PgsExt\ReportPointOfInterest.h>


template <class P,class S>
class TKey2
{
public:
   TKey2(const P& primaryKey,const S& secondaryKey) :
      m_PrimaryKey(primaryKey), m_SecondaryKey(secondaryKey)
      {}

   TKey2(const TKey2& rOther)
   {
      m_PrimaryKey        = rOther.m_PrimaryKey;
      m_SecondaryKey      = rOther.m_SecondaryKey;
   }

   TKey2& operator=(const TKey2& rOther)
   {
      m_PrimaryKey        = rOther.m_Primary;
      m_SecondaryKey      = rOther.m_SecondaryKey;
      return *this;
   }

   bool operator<(const TKey2& rOther) const
   {
      if ( m_PrimaryKey < rOther.m_PrimaryKey ) return true;
      if ( rOther.m_PrimaryKey < m_PrimaryKey ) return false;

      if (m_SecondaryKey         < rOther.m_SecondaryKey         ) return true;
      if (rOther.m_SecondaryKey  < m_SecondaryKey                ) return false;

      return false;
   }

   bool operator==(const TKey2& rOther) const
   {
      if ( m_PrimaryKey != rOther.m_PrimaryKey )
      {
         return false;
      }

      if ( m_SecondaryKey != rOther.m_SecondaryKey )
      {
         return false;
      }

      return true;
   }

   const P& GetPrimaryKey() const {return m_PrimaryKey;}
   const S& GetSecondaryKey() const { return m_SecondaryKey; }

private:
   P m_PrimaryKey;
   S m_SecondaryKey;
};


template <class T>
class TPoiKey
{
public:
   TPoiKey(const CSegmentKey& segmentKey,Float64 Xpoi,T subkey) :
      m_Subkey(subkey), m_Poi(segmentKey,Xpoi) 
      {}

   TPoiKey(const pgsPointOfInterest& poi,T subkey) : 
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
      if ( m_Poi < rOther.m_Poi ) return true;
      if ( rOther.m_Poi < m_Poi ) return false;

      if (m_Subkey         < rOther.m_Subkey         ) return true;
      if (rOther.m_Subkey  < m_Subkey                ) return false;

      return false;
   }

   bool operator==(const TPoiKey& rOther) const
   {
      if ( m_Poi != rOther.m_Poi )
      {
         return false;
      }

      if ( m_Subkey != rOther.m_Subkey )
      {
         return false;
      }

      return true;
   }

   const pgsPointOfInterest& GetPoi() const {return m_Poi;}
   const T& GetSubKey() const { return m_Subkey; }

private:
   T                  m_Subkey;
   pgsPointOfInterest m_Poi;
};

class PrestressSubKey
{
public:
   PrestressSubKey()
   {
      m_IntervalIdx = INVALID_INDEX;
      m_Strand = pgsTypes::Straight;
   }

   PrestressSubKey(pgsTypes::StrandType strand,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) :
      m_Strand(strand), m_IntervalIdx(intervalIdx), m_IntervalTime(intervalTime)
      {
      }

   PrestressSubKey(const PrestressSubKey& rOther)
   {
      m_Strand = rOther.m_Strand;
      m_IntervalIdx = rOther.m_IntervalIdx;
      m_IntervalTime = rOther.m_IntervalTime;
   }

   bool operator<(const PrestressSubKey& rOther) const
   {
      if ( m_IntervalIdx < rOther.m_IntervalIdx ) return true;
      if ( rOther.m_IntervalIdx < m_IntervalIdx ) return false;

      if ( m_IntervalTime < rOther.m_IntervalTime ) return true;
      if ( rOther.m_IntervalTime < m_IntervalTime ) return false;

      if ( m_Strand < rOther.m_Strand ) return true;
      if ( rOther.m_Strand < m_Strand ) return false;

      return false;
   }

private:
   IntervalIndexType m_IntervalIdx;
   pgsTypes::IntervalTimeType m_IntervalTime;
   pgsTypes::StrandType m_Strand;
};

typedef TPoiKey<IntervalIndexType> PoiIntervalKey;
typedef TPoiKey<PoiIDType> PoiIDKey;
typedef TPoiKey<PrestressSubKey> PrestressPoiKey;



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