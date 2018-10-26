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
#pragma once

#include <PGSuperTypes.h>
#include <PgsExt\PointOfInterest.h>

#define POIFIND_AND   1  // Find POIs that have all of the specified attributes
#define POIFIND_OR    2  // Find POIs that have at least one of the specified attributes


/*****************************************************************************
INTERFACE
   IPointOfInterest

DESCRIPTION
   Interface for getting points of interest
*****************************************************************************/
// {32574861-444E-46dc-A12B-904A083E68EC}
DEFINE_GUID(IID_IPointOfInterest, 
0x32574861, 0x444e, 0x46dc, 0xa1, 0x2b, 0x90, 0x4a, 0x8, 0x3e, 0x68, 0xec);
interface IPointOfInterest : public IUnknown
{
   // Returns all points of interest for a segment
   // If the segment index is ALL_SEGMENTS, returns pois for all segments in the specified girder
   // If the girder index is ALL_GIRDERS, returns the pois for the specified segments in all girders in the specified group
   // If the group index is ALL_GROUPS, returns the pois for the specified girder and segment all girder groups in the bridge
   // Critical section POI will only be included if they were previously computed
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey) = 0;

   // Returns all points of interest for a segment that include the attrib attributes
   // The use of ALL_GROUPS, ALL_GIRDERS, and ALL_SEGMENTS is permitted
   // Use this for with an attribute flag for critical sections if you absolutely must get critical section POI in this request
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;

   // Returns a poi with a specified ID
   virtual pgsPointOfInterest GetPointOfInterest(PoiIDType poiID) = 0;

   // Returns a point of interest at the specified distance from the start of the segment. If a poi
   // has been, or can be, defined at the location, returns an actual poi. Otherwise a poi is created 
   // (but not stored) and it will have an ID of INVALID_ID. NOTE: poiDistFromStartOfSegment is NOT in 
   // Segment Coordinates. Use ConvertSegmentCoordinateToPoi if the distance you have is in segment coordinates.
   virtual pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 poiDistFromStartOfSegment) = 0;

   // Returns the poi that is nearst to the specified location on a segment
   virtual pgsPointOfInterest GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 poiDistFromStartOfSegment) = 0;

   // Returns all of the 10th point points of interest for a segment
   virtual std::vector<pgsPointOfInterest> GetSegmentTenthPointPOIs(PoiAttributeType reference,const CSegmentKey& segmentKey) = 0;

   // Gets the critical section for shear points of interest
   virtual std::vector<pgsPointOfInterest> GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey) = 0;
   virtual std::vector<pgsPointOfInterest> GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey,const GDRCONFIG& config) = 0;

   // Returns a point of interest that is located at the specified distance from the start face of a girder
   // This is the companion function to GetDistanceAlongGirder.
   virtual pgsPointOfInterest GetPointOfInterest(const CGirderKey& girderKey,Float64 poiDistFromStartOfGirder) = 0;

   // Returns a point of interest that is located distFromStartOfSpan measured from the CL Pier at the start of spanIdx
   // and measured along the centerline of gdrIdx
   virtual pgsPointOfInterest GetPointOfInterest(const CGirderKey& girderKey,SpanIndexType spanIdx,Float64 distFromStartOfSpan) = 0;

   // Returns a point of interest that is located at the intersection of a girder and a pier
   virtual pgsPointOfInterest GetPointOfInterest(const CGirderKey& girderKey,PierIndexType pierIdx) = 0;


   // Returns the points of interest located in the range xLeft-xRight around the given POI.
   // The returned vector of POI does not include POIs that are in adjacent segments
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterestInRange(Float64 xLeft,const pgsPointOfInterest& poi,Float64 xRight) = 0;

   // Returns the index of the span where this poi is located
   virtual SpanIndexType GetSpan(const pgsPointOfInterest& poi) = 0;

   // Converts a POI to a location in the segment coordinate system
   virtual Float64 ConvertPoiToSegmentCoordinate(const pgsPointOfInterest& poi) = 0;

   // Converts a segment coordinate to a POI
   virtual pgsPointOfInterest ConvertSegmentCoordinateToPoi(const CSegmentKey& segmentKey,Float64 Xs) = 0;

   // Converts a segment coordinate to a POI coordinate
   virtual Float64 ConvertSegmentCoordinateToPoiCoordinate(const CSegmentKey& segmentKey,Float64 Xs) = 0;

   // Converts a POI coordinate to a segment coordinate
   virtual Float64 ConvertPoiCoordinateToSegmentCoordinate(const CSegmentKey& segmentKey,Float64 Xpoi) = 0;

   // Converts a POI to a location in the girder path coordinate system
   virtual Float64 ConvertPoiToGirderPathCoordinate(const pgsPointOfInterest& poi) = 0;

   // Converts a girder path coordinate to a POI
   virtual pgsPointOfInterest ConvertGirderPathCoordinateToPoi(const CGirderKey& girderKey,Float64 Xgp) = 0;

   // Converts a POI to a location in the girder coordinate system
   virtual Float64 ConvertPoiToGirderCoordinate(const pgsPointOfInterest& poi) = 0;

   // Converts a girder coordinate to a POI
   virtual pgsPointOfInterest ConvertGirderCoordinateToPoi(const CGirderKey& girderKey,Float64 Xg) = 0;

   // Converts a girder coordinate to a girder path coordinate
   virtual Float64 ConvertGirderCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float64 distFromStartOfGirder) = 0;

   // Converts a girder path coordinate to a girder coordinate
   virtual Float64 ConvertGirderPathCoordinateToGirderCoordinate(const CGirderKey& girderKey,Float64 Xg) = 0;

   // Removes all poi from the vector that have the specified attribute unless the poi also has the
   // exception attribute, in which case it is retained
   virtual void RemovePointsOfInterest(std::vector<pgsPointOfInterest>& vPoi,PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute=0) = 0;
};

/*****************************************************************************
INTERFACE
   IGirderLiftingPointsOfInterest

   Interface to points of interest for lifting

DESCRIPTION
   Interface to points of interest for lifting
*****************************************************************************/
// non-COM version
interface IGirderLiftingDesignPointsOfInterest
{
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType attrib=0,Uint32 mode = POIFIND_AND) = 0;
};

// {19EA189E-E5F4-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_IGirderLiftingPointsOfInterest, 
0x19ea189e, 0xe5f4, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderLiftingPointsOfInterest : IUnknown, IGirderLiftingDesignPointsOfInterest
{
   virtual std::vector<pgsPointOfInterest> GetLiftingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_AND) = 0;
};

/*****************************************************************************
INTERFACE
   IGirderHaulingPointsOfInterest

   Interface to points of interest for Hauling

DESCRIPTION
   Interface to points of interest for Hauling
*****************************************************************************/
// non-COM version
interface IGirderHaulingDesignPointsOfInterest
{
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib=0,Uint32 mode = POIFIND_AND) = 0;
};

// {E6A0E250-E5F4-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_IGirderHaulingPointsOfInterest, 
0xe6a0e250, 0xe5f4, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderHaulingPointsOfInterest : IUnknown, IGirderHaulingDesignPointsOfInterest
{
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_AND) = 0;
   virtual Float64 GetMinimumOverhang(const CSegmentKey& segmentKey) = 0;
};
