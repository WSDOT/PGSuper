///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <PgsExt\ReportPointOfInterest.h>
#include <set>

// Search Modes
#define POIMGR_AND  1  // must match all attributes
#define POIMGR_OR   2  // must match at least one attribute

/*****************************************************************************
CLASS 
   pgsPoiMgr

   Point of interest manager.  Objects of this class manage points of
   interest.


DESCRIPTION
   Point of interest manager.  Objects of this class manage points of
   interest.  Management includes storage and retreival, and elimination of
   duplicates.

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsPoiMgr
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   pgsPoiMgr();

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPoiMgr();

   //------------------------------------------------------------------------
   // Adds a point of interest. Returns its unique identifier.
   // Returns INVALID_ID if the POI could not be added.
   PoiIDType AddPointOfInterest(const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Removes a point of interest
   bool RemovePointOfInterest(const pgsPointOfInterest& poi);

   // Removes a point of interest that has the specified ID. Returns true if successful.
   bool RemovePointOfInterest(PoiIDType poiID);

   // Removes all POIs that have the target attribute, except if it contains the exception attribute
   // in which case it is not removed
   void RemovePointsOfInterest(PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute=0);

   //------------------------------------------------------------------------
   // Removes all points of interest.
   void RemoveAll();

   //------------------------------------------------------------------------
   // caches the ID of the start and end poi of a post-tensioning duct
   void AddDuctBoundary(const CGirderKey& girderKey, DuctIndexType ductIdx, PoiIDType startPoiID, PoiIDType endPoiID);
   void GetDuctBoundary(const CGirderKey& girderKey, DuctIndexType ductIdx, PoiIDType* pStartPoiID, PoiIDType* pEndPoiID) const;

   //------------------------------------------------------------------------
   pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey, Float64 Xs) const;

   //------------------------------------------------------------------------
   // Returns the point of interest nearest to the specified location. A default POI is
   // returned if a poi could not be found
   const pgsPointOfInterest& GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs) const;

   const pgsPointOfInterest& GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIMGR_OR) const;
   const pgsPointOfInterest& GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIMGR_OR) const;

   //------------------------------------------------------------------------
   // Returns the point of interest at the specified location or a default if not found. 
   const pgsPointOfInterest& GetPointOfInterest(PoiIDType id) const;

   //------------------------------------------------------------------------
   // Returns a vector of pointers to Points of Interest that have the
   // specified attributes.
   void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode, PoiList* pPois) const;

   //------------------------------------------------------------------------
   // Gets all the POIs on the specified segment. ALL_GROUPS, ALL_GIRDERS, ALL_SEGMENTS can be used
   void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiList* pPoiList) const;

   //------------------------------------------------------------------------
   // Merges two Poi lists into a single list, sorting and removing duplicate entries
   void MergePoiLists(const PoiList& list1, const PoiList& list2,PoiList* pPoiList) const;

   //------------------------------------------------------------------------
   // Sorts a Poi list, removing any duplicate entries
   void SortPoiList(PoiList* pPoiList) const;

   //------------------------------------------------------------------------
   void GetTenthPointPOIs(PoiAttributeType reference,const CSegmentKey& segmentKey, PoiList* pPois) const;

   //------------------------------------------------------------------------
   // Gets all the points of interest on the specified section with distance from start between xMin and xMax
   void GetPointsOfInterestInRange(const CSegmentKey& segmentKey,Float64 xMin,Float64 xMax, PoiList* pPois) const;


   //------------------------------------------------------------------------
   // Replaces a previously defined point of interest
   bool ReplacePointOfInterest(PoiIDType ID,const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Returns the number of points of interest.
   CollectionIndexType GetPointOfInterestCount() const;

private:
   // make these private so we can't have copy or assignment
   // may want it in the future, but for now, we don't want to bother
   // copying the internal data structure
   pgsPoiMgr(const pgsPoiMgr& rOther) = delete;
   pgsPoiMgr& operator=(const pgsPoiMgr& rOther) = delete;

   static PoiIDType ms_NextID;

   pgsPointOfInterest m_DefaultPoi;

   // Since PoiList (see PointOfInterest.h) is a container of references to pgsPointOfInterest objects
   // we must store pointers to POIs... Consider this... if a function is holding a PoiList and
   // another thread causes a new POI to be added to the POI manager and the PoiContainer must be
   // resized, the POIs in the container get copied... The function holding the PoiList now holds
   // references to POIs that no longer exist. If we instead hold pointers, only the pointers
   // move when a container resizes but the actual POI remains
   typedef std::vector<std::unique_ptr<pgsPointOfInterest>> PoiContainer; // because of unique_ptr, containers of this type are not copy-able
   mutable std::map<CSegmentKey,PoiContainer> m_PoiData;
   const PoiContainer& GetPoiContainer(const CSegmentKey& segmentKey) const;
   PoiContainer& GetPoiContainer(const CSegmentKey& segmentKey);

   void AndFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,PoiList* pPois) const;
   bool AndFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const;
   bool AndAttributeEvaluation(const pgsPointOfInterest& poi,PoiAttributeType attrib) const;
   void OrFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,PoiList* pPois) const;
   bool OrFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const;
   bool OrAttributeEvaluation(const pgsPointOfInterest& poi,PoiAttributeType attrib) const;

   struct DuctBoundaryRecord
   {
      CGirderKey girderKey;
      DuctIndexType ductIdx;
      PoiIDType startID;
      PoiIDType endID;

      DuctBoundaryRecord(const CGirderKey& girderKey, DuctIndexType ductIdx, PoiIDType startID, PoiIDType endID) :
         girderKey(girderKey), ductIdx(ductIdx), startID(startID), endID(endID)
      {
      };

      bool operator<(const DuctBoundaryRecord& other) const
      {
         if (girderKey < other.girderKey)
            return true;

         if (other.girderKey < girderKey)
            return false;

         if (ductIdx < other.ductIdx)
            return true;

         return false;
      }
   };
   std::set<DuctBoundaryRecord> m_DuctBoundaries;
};
