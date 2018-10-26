///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\GirderPointOfInterest.h>
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


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   // Copy constructor
   pgsPoiMgr(const pgsPoiMgr& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPoiMgr();

   //------------------------------------------------------------------------
   // Assignment operator
   pgsPoiMgr& operator = (const pgsPoiMgr& rOther);

   //------------------------------------------------------------------------
   // Adds a point of interest. Returns its unique identifier.
   // Returns INVALID_ID if the POI could not be added.
   PoiIDType AddPointOfInterest(const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Removes a point of interest. If bConsiderAttributes is true, removes the poi
   // that exactly matches the specified poi, otherwise it removes the poi if just the
   // locations match.
   bool RemovePointOfInterest(const pgsPointOfInterest& poi,bool bConsiderAttributes=true);

   // Removes all POIs that have the target attribute, except if it contains the exception attribute
   // in which case it is not removed
   void RemovePointsOfInterest(PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute=0);

   //------------------------------------------------------------------------
   // Removes all points of interest.
   void RemoveAll();

   //------------------------------------------------------------------------
   // Returns the point of interest at the specified location. If not found,
   // returns a default poi
   pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the point of interest nearest to the specified location. A default POI is
   // returned if a poi could not be found
   pgsPointOfInterest GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the point of interest at the specified location or a default if not found. 
   pgsPointOfInterest GetPointOfInterest(PoiIDType id) const;

   //------------------------------------------------------------------------
   // Returns a vector of pointers to Points of Interest that have the
   // specified attributes.
   void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const;

   //------------------------------------------------------------------------
   // Gets all the POIs on the specified segment. ALL_GROUPS, ALL_GIRDERS, ALL_SEGMENTS can be used
   std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey) const;

   //------------------------------------------------------------------------
   void GetTenthPointPOIs(PoiAttributeType reference,const CSegmentKey& segmentKey,std::vector<pgsPointOfInterest>* pPois) const;

   // Gets all the points of interest on the specified section with distance from start between xMin and xMax
   void GetPointsOfInterestInRange(const CSegmentKey& segmentKey,Float64 xMin,Float64 xMax,std::vector<pgsPointOfInterest>* pPois) const;


   //------------------------------------------------------------------------
   // Replaces a previously defined point of interest
   bool ReplacePointOfInterest(PoiIDType ID,const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Sets the tolerance for eliminating duplicate points of interest.  If two
   // points of interest, on the same segment, are with this tolerance of each
   // other,  they are considered to be at the same point.  These std::vector<pgsPointOfInterest>'s are merged,
   // maintaining the attributes of both std::vector<pgsPointOfInterest>. 
   //
   // If MergePOI is set to false, duplicate points are not merged.
   //
   // Changing the tolerance does not effect previously stored points of interest.
   void SetTolerance(Float64 tol);

   //------------------------------------------------------------------------
   // Returns the std::vector<pgsPointOfInterest> tolerance.
   Float64 GetTolerance() const;

   //------------------------------------------------------------------------
   // Returns the number of points of interest.
   CollectionIndexType GetPointOfInterestCount() const;

protected:
   void MakeCopy(const pgsPoiMgr& rOther);
   virtual void MakeAssignment(const pgsPoiMgr& rOther);

private:
   static PoiIDType ms_NextID;
   std::vector<pgsPointOfInterest> m_Poi;
   Float64 m_Tolerance;


   void AndFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const;
   bool AndFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const;
   void OrFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const;
   bool OrFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const;

public:
#if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
#endif // _DEBUG
};
