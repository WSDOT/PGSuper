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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PoiMgr.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsPoiMgr
****************************************************************************/



// Predicate STL objects.

class SamePlace
{
public:
   SamePlace(const pgsPointOfInterest& poi,Float64 tol) : m_Poi(poi), m_Tol(tol) {}

   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_Poi.GetSegmentKey() == other.GetSegmentKey() &&
           IsZero( m_Poi.GetDistFromStart() - other.GetDistFromStart(), m_Tol ) )
      {
         return true;
      }

      return false;
   }

   const pgsPointOfInterest& GetPoi() const { return m_Poi; }

private:
   const pgsPointOfInterest& m_Poi;
   Float64 m_Tol;
};

class ExactlySame
{
public:
   ExactlySame(const pgsPointOfInterest& poi,Float64 tol) : m_SamePlace(poi,tol) {}
   
   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_SamePlace.operator()(other) )
      {
         // poi are at the same place, do they have the same attributes?
         if (!other.HasAttribute(m_SamePlace.GetPoi().GetAttributes()))
            return false;

         return true;
      }

      return false;
   }

private:
   SamePlace m_SamePlace;
};

class SameAttribute
{
public:
   SameAttribute(PoiAttributeType targetAttribute,PoiAttributeType exceptAttribute) : m_TargetAttribute(targetAttribute), m_ExceptAttribute(exceptAttribute) {}

   bool operator()(const pgsPointOfInterest& other) const
   {
      // return true if the poi has the target attribute, except if it has the exception attribute
      if ( other.HasAttribute(m_TargetAttribute) && !other.HasAttribute(m_ExceptAttribute) )
      {
         return true;
      }

      return false;
   }

private:
   PoiAttributeType m_TargetAttribute;
   PoiAttributeType m_ExceptAttribute;
};

class FindByID
{
public:
   FindByID(PoiIDType id) : m_ID(id) {}
   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_ID == other.GetID() )
      {
         return true;
      }

      return false;
   }

private:
   PoiIDType m_ID;
};

class NotFindPoi
{
public:
   NotFindPoi(const CSegmentKey& segmentKey) {m_GroupIdx = segmentKey.groupIndex; m_GirderIdx = segmentKey.girderIndex; m_SegmentIdx = segmentKey.segmentIndex;}
   bool operator()(const pgsPointOfInterest& other) const
   {
      // returning true causes the poi to be excluded from the container
      // return false for those poi that match the search criteria... return false for the poi we want to keep

      const CSegmentKey& segmentKey = other.GetSegmentKey();
      GroupIndexType grpIdx = segmentKey.groupIndex;
      GirderIndexType gdrIdx = segmentKey.girderIndex;

      if ( (m_GroupIdx   == INVALID_INDEX || m_GroupIdx   == grpIdx) &&  // group matches -AND-
           (m_GirderIdx  == INVALID_INDEX || m_GirderIdx  == gdrIdx) &&  // girder matches -AND-
           (m_SegmentIdx == INVALID_INDEX || m_SegmentIdx == segmentKey.segmentIndex) // segment matches
         )
      {
         // we want to keep the POI
         return false;
      }

      return true;
   }

private:
   GroupIndexType   m_GroupIdx;
   GirderIndexType  m_GirderIdx;
   SegmentIndexType m_SegmentIdx;
};

PoiIDType pgsPoiMgr::ms_NextID = 0;


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPoiMgr::pgsPoiMgr()
{
   m_Tolerance = ::ConvertToSysUnits( 1.0, unitMeasure::Millimeter );
   pgsPointOfInterest::SetTolerance( m_Tolerance );
}

pgsPoiMgr::pgsPoiMgr(const pgsPoiMgr& rOther)
{
   MakeCopy(rOther);
}

pgsPoiMgr::~pgsPoiMgr()
{
}

//======================== OPERATORS  =======================================
pgsPoiMgr& pgsPoiMgr::operator= (const pgsPoiMgr& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
PoiIDType pgsPoiMgr::AddPointOfInterest(const pgsPointOfInterest& poi)
{
   // first see if we have an existing poi at this location. Just merge attributes into existing if we do
   if ( poi.CanMerge() )
   {
      std::vector<pgsPointOfInterest>::iterator iter(m_Poi.begin());
      std::vector<pgsPointOfInterest>::iterator end(m_Poi.end());
      for ( ; iter != end; iter++ )
      {
         pgsPointOfInterest& curpoi = *iter;
         if ( curpoi.AtSamePlace(poi) )
         {
            curpoi.MergeAttributes(poi);
            return curpoi.m_ID; // no need to resort vector
         }
      }
   }

   PoiIDType id = poi.m_ID;
   if ( id == INVALID_ID )
   {
      // assert if we are about to roll over the id
      ATLASSERT(ms_NextID != MAX_ID);
      id = ms_NextID++;
   }

   // Don't copy poi and put it in because this is a very highly utilized function and copies don't come cheap
#if defined _DEBUG
   std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(m_Poi.begin(),m_Poi.end(), FindByID(id) ) );
   ATLASSERT(found == m_Poi.end());
#endif
   m_Poi.push_back(poi);
   m_Poi.back().m_ID = id;

   std::sort(m_Poi.begin(),m_Poi.end());

   ASSERTVALID;

   return id;
}

bool pgsPoiMgr::RemovePointOfInterest(const pgsPointOfInterest& poi,bool bConsiderAttributes)
{
   std::vector<pgsPointOfInterest>::iterator begin(m_Poi.begin());
   std::vector<pgsPointOfInterest>::iterator end(m_Poi.end());
   std::vector<pgsPointOfInterest>::iterator found;
   if ( bConsiderAttributes )
      found = std::find_if(begin, end, ExactlySame(poi,m_Tolerance) );
   else
      found = std::find_if(begin, end, SamePlace(poi,m_Tolerance) );

   if ( found != end )
   {
      m_Poi.erase(found);
      std::sort(m_Poi.begin(),m_Poi.end());
      return true;
   }

   ASSERTVALID;

   return false;
}

void pgsPoiMgr::RemovePointsOfInterest(PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute)
{
   std::vector<pgsPointOfInterest>::iterator newEnd( std::remove_if(m_Poi.begin(),m_Poi.end(),SameAttribute(targetAttribute,exceptionAttribute)) );
   m_Poi.erase(newEnd,m_Poi.end());

   ASSERTVALID;
}

void pgsPoiMgr::RemoveAll()
{
   m_Poi.clear();

   ASSERTVALID;
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart) 
{
   pgsPointOfInterest poi(segmentKey,distFromStart);
   std::vector<pgsPointOfInterest>::const_iterator begin(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(begin, end, SamePlace(poi,m_Tolerance) ) );
   if ( found != end )
      return (*found);

   return poi;
}

pgsPointOfInterest pgsPoiMgr::GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart) 
{
   // get the poi just for this span/girder
   std::vector<pgsPointOfInterest> vPOI;
   std::vector<pgsPointOfInterest>::const_iterator iter(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSegmentKey() == segmentKey)
         vPOI.push_back(poi);
   }

   if ( vPOI.empty() )
      return pgsPointOfInterest();

   std::vector<pgsPointOfInterest>::const_iterator iter1(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator iter2(iter1+1);
   std::vector<pgsPointOfInterest>::const_iterator end2(vPOI.end());
   for ( ; iter2 != end2; iter1++, iter2++ )
   {
      const pgsPointOfInterest& prevPOI = *iter1;
      const pgsPointOfInterest& nextPOI = *iter2;

      ATLASSERT( prevPOI.GetSegmentKey() == segmentKey);
      ATLASSERT( nextPOI.GetSegmentKey() == segmentKey);

      if ( InRange(prevPOI.GetDistFromStart(),distFromStart,nextPOI.GetDistFromStart()) )
      {
         Float64 d1 = distFromStart - prevPOI.GetDistFromStart();
         Float64 d2 = nextPOI.GetDistFromStart() - distFromStart;

         if ( d1 < d2 )
            return prevPOI;
         else
            return nextPOI;
      }
   }

   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(PoiIDType id) const
{
   std::vector<pgsPointOfInterest>::const_iterator begin(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(begin, end, FindByID(id) ) );
   if ( found != end )
      return (*found);

   return pgsPointOfInterest();
}

void pgsPoiMgr::GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const
{
   if ( mode == POIMGR_AND )
      AndFind(segmentKey,attrib,pPois);
   else
      OrFind(segmentKey,attrib,pPois);
}

std::vector<pgsPointOfInterest> pgsPoiMgr::GetPointsOfInterest(const CSegmentKey& segmentKey) const
{
   std::vector<pgsPointOfInterest> copy_poi;

   // This has to be the most poorly named function in the stl. 
   // (which really should be called copy_if_not)

   std::remove_copy_if(m_Poi.begin(), m_Poi.end(), std::back_inserter(copy_poi), NotFindPoi(segmentKey));

   return copy_poi;
}

bool pgsPoiMgr::ReplacePointOfInterest(PoiIDType ID,const pgsPointOfInterest& poi)
{
   std::vector<pgsPointOfInterest>::iterator begin(m_Poi.begin());
   std::vector<pgsPointOfInterest>::iterator end(m_Poi.end());
   std::vector<pgsPointOfInterest>::iterator found(std::find_if(begin, end, FindByID(ID) ) );
   if ( found == end )
      return false;


   *found = poi;
   (*found).m_ID = ID;

   return true;
}

//======================== ACCESS     =======================================
void pgsPoiMgr::SetTolerance(Float64 tol)
{
   m_Tolerance = tol;
   pgsPointOfInterest::SetTolerance( m_Tolerance );
}

Float64 pgsPoiMgr::GetTolerance() const
{
   return m_Tolerance;
}

//======================== INQUIRY    =======================================
CollectionIndexType pgsPoiMgr::GetPointOfInterestCount() const
{
   return m_Poi.size();
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsPoiMgr::MakeCopy(const pgsPoiMgr& rOther)
{
   // Add copy code here...
   m_Poi       = rOther.m_Poi;
   m_Tolerance = rOther.m_Tolerance;

   ASSERTVALID;
}

void pgsPoiMgr::MakeAssignment(const pgsPoiMgr& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsPoiMgr::GetTenthPointPOIs(PoiAttributeType reference,const CSegmentKey& segmentKey,std::vector<pgsPointOfInterest>* pPois) const
{
   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator iter(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSegmentKey() == segmentKey && poi.IsTenthPoint(reference) )
      {
         pPois->push_back( poi );
      }
   }
}

void pgsPoiMgr::GetPointsOfInterestInRange(const CSegmentKey& segmentKey,Float64 xMin,Float64 xMax,std::vector<pgsPointOfInterest>* pPois) const
{
   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator iter(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSegmentKey() == segmentKey && ::InRange(xMin,poi.GetDistFromStart(),xMax) )
      {
         pPois->push_back( poi );
      }
   }
}


void pgsPoiMgr::AndFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator iter(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( AndFind(poi,segmentKey,attrib) ) 
      {
         // poi has desired attributes - keep it
         pPois->push_back(poi);
      }
   }
}

bool pgsPoiMgr::AndFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same segment, then
   // it check if each flag in attrib is set. If it is, the corrosponding flag in poi must
   // also be set, otherwise, the test is irrelavent (and always passes as indicated by the true)
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);
   PoiAttributeType poiReference = poi.GetReference();
   bool bPoiReference = (targetReference == 0 ? true : sysFlags<PoiAttributeType>::IsSet(poiReference,targetReference));
   if ( segmentKey == poi.GetSegmentKey() &&
        bPoiReference &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_0L)              ? poi.IsTenthPoint(poiReference) == 1   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_1L)              ? poi.IsTenthPoint(poiReference) == 2   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_2L)              ? poi.IsTenthPoint(poiReference) == 3   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_3L)              ? poi.IsTenthPoint(poiReference) == 4   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_4L)              ? poi.IsTenthPoint(poiReference) == 5   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_5L)              ? poi.IsTenthPoint(poiReference) == 6   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_6L)              ? poi.IsTenthPoint(poiReference) == 7   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_7L)              ? poi.IsTenthPoint(poiReference) == 8   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_8L)              ? poi.IsTenthPoint(poiReference) == 9   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_9L)              ? poi.IsTenthPoint(poiReference) == 10  : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_10L)             ? poi.IsTenthPoint(poiReference) == 11  : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)  ? poi.HasAttribute(POI_CRITSECTSHEAR1) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)  ? poi.HasAttribute(POI_CRITSECTSHEAR2) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)    ? poi.IsHarpingPoint()            : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT)       ? poi.HasAttribute(POI_PICKPOINT) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT)       ? poi.HasAttribute(POI_BUNKPOINT) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)        ? poi.IsConcentratedLoad()        : true) &&
	    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_MIDSPAN)         ? poi.IsMidSpan(poiReference)                 : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_H)               ? poi.IsAtH(poiReference)                     : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_15H)             ? poi.IsAt15H(poiReference)                   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)          ? poi.HasAttribute(POI_PSXFER) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DECKBARCUTOFF)   ? poi.HasAttribute(POI_DECKBARCUTOFF) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARCUTOFF)   ? poi.HasAttribute(POI_BARCUTOFF) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARDEVELOP)   ? poi.HasAttribute(POI_BARDEVELOP) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_RIGHTFACE) ? poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_LEFTFACE) ? poi.HasAttribute(POI_SECTCHANGE_LEFTFACE) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_TRANSITION) ? poi.HasAttribute(POI_SECTCHANGE_TRANSITION) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)   ? poi.HasAttribute(POI_FACEOFSUPPORT) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_PIER) ? poi.HasAttribute(POI_INTERMEDIATE_PIER) : true ) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PIER)   ? poi.HasAttribute(POI_PIER) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_STIRRUP_ZONE)   ? poi.HasAttribute(POI_STIRRUP_ZONE) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_TEMPSUPPORT)   ? poi.HasAttribute(POI_TEMPSUPPORT) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CLOSURE)   ? poi.HasAttribute(POI_CLOSURE) : true)
      )
   {
      // This poi matches the selection criteria. Add it to the vector
      return true;
   }
   return false;
}

void pgsPoiMgr::OrFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   pPois->clear();

   std::vector<pgsPointOfInterest>::const_iterator poiIter(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(m_Poi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;

      if ( OrFind(poi,segmentKey,attrib) )
      {
         // poi has target attributes - keep it
         pPois->push_back(poi);
      }
   }
}

bool pgsPoiMgr::OrFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same segment, then
   // it check if each flag in attrib is set. If it is, the corrosponding flag in the poi must
   // be set, otherwise, the test is irrelavent (and always passes as indicated by the true)
   if ( segmentKey != poi.GetSegmentKey() )
      return false;

   if ( attrib == 0 )
      return true; // always match if we don't care what the attributes are

   PoiAttributeType poiReference = poi.GetReference();
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   if ( targetReference != 0 )
   {
      // the attribute we are looking for has at least one poi reference...
      // the poi reference on the subject POI must match on at least one of the target reference types
      if ( (sysFlags<PoiAttributeType>::IsSet(targetReference,POI_RELEASED_SEGMENT) && !sysFlags<PoiAttributeType>::IsSet(poiReference,POI_RELEASED_SEGMENT)) ||
           (sysFlags<PoiAttributeType>::IsSet(targetReference,POI_LIFT_SEGMENT)     && !sysFlags<PoiAttributeType>::IsSet(poiReference,POI_LIFT_SEGMENT))     ||
           (sysFlags<PoiAttributeType>::IsSet(targetReference,POI_STORAGE_SEGMENT)  && !sysFlags<PoiAttributeType>::IsSet(poiReference,POI_STORAGE_SEGMENT))  ||
           (sysFlags<PoiAttributeType>::IsSet(targetReference,POI_HAUL_SEGMENT)     && !sysFlags<PoiAttributeType>::IsSet(poiReference,POI_HAUL_SEGMENT))     ||
           (sysFlags<PoiAttributeType>::IsSet(targetReference,POI_ERECTED_SEGMENT)  && !sysFlags<PoiAttributeType>::IsSet(poiReference,POI_ERECTED_SEGMENT))  ||
           (sysFlags<PoiAttributeType>::IsSet(targetReference,POI_GIRDER)           && !sysFlags<PoiAttributeType>::IsSet(poiReference,POI_GIRDER))
         )
      {
         // No matches...
         return false;
      }
   }

   if (
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_0L)              ? poi.IsTenthPoint(poiReference) == 1   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_1L)              ? poi.IsTenthPoint(poiReference) == 2   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_2L)              ? poi.IsTenthPoint(poiReference) == 3   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_3L)              ? poi.IsTenthPoint(poiReference) == 4   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_4L)              ? poi.IsTenthPoint(poiReference) == 5   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_5L)              ? poi.IsTenthPoint(poiReference) == 6   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_6L)              ? poi.IsTenthPoint(poiReference) == 7   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_7L)              ? poi.IsTenthPoint(poiReference) == 8   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_8L)              ? poi.IsTenthPoint(poiReference) == 9   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_9L)              ? poi.IsTenthPoint(poiReference) == 10  : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_10L)             ? poi.IsTenthPoint(poiReference) == 11  : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)  ? poi.HasAttribute(POI_CRITSECTSHEAR1) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)  ? poi.HasAttribute(POI_CRITSECTSHEAR2) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)    ? poi.IsHarpingPoint()            : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT)       ? poi.HasAttribute(POI_PICKPOINT) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT)       ? poi.HasAttribute(POI_BUNKPOINT) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)        ? poi.IsConcentratedLoad()        : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_MIDSPAN)         ? poi.IsMidSpan(poiReference)                 : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_H)               ? poi.IsAtH(poiReference)                     : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_15H)             ? poi.IsAt15H(poiReference)                   : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DEBOND)          ? poi.HasAttribute(POI_DEBOND) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)          ? poi.HasAttribute(POI_PSXFER) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DECKBARCUTOFF)   ? poi.HasAttribute(POI_DECKBARCUTOFF) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARCUTOFF)   ? poi.HasAttribute(POI_BARCUTOFF) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARDEVELOP)   ? poi.HasAttribute(POI_BARDEVELOP) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_RIGHTFACE)      ? poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_LEFTFACE)      ? poi.HasAttribute(POI_SECTCHANGE_LEFTFACE) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_TRANSITION)      ? poi.HasAttribute(POI_SECTCHANGE_TRANSITION) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)   ? poi.HasAttribute(POI_FACEOFSUPPORT) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_PIER) ? poi.HasAttribute(POI_INTERMEDIATE_PIER) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PIER)   ? poi.HasAttribute(POI_PIER) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_STIRRUP_ZONE)   ? poi.HasAttribute(POI_STIRRUP_ZONE) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_TEMPSUPPORT)   ? poi.HasAttribute(POI_TEMPSUPPORT) : false) ||
    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CLOSURE)   ? poi.HasAttribute(POI_CLOSURE) : false)
    )
   {
      // This poi matches the selection criteria. Add it to the vector
      return true;
   }

   return false;
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsPoiMgr::AssertValid() const
{
   if ( m_Tolerance < 0 )
      return false;

   std::vector<pgsPointOfInterest>::const_iterator i(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;
      if ( poi.m_ID < 0 )
         return false;
   }

   return true;
}

void pgsPoiMgr::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsPoiMgr" << endl;
   os << "m_Tolerance = " << m_Tolerance << endl;

   std::vector<pgsPointOfInterest>::const_iterator i(m_Poi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_Poi.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;
//      poi.Dump(os);
   }
}
#endif // _DEBUG
