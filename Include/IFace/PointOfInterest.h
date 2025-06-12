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
#pragma once

#include <PGSuperTypes.h>
#include <PsgLib\PointOfInterest.h>

#define POIFIND_AND   1  // Find POIs that have all of the specified attributes
#define POIFIND_OR    2  // Find POIs that have at least one of the specified attributes

interface IDirection;

/*****************************************************************************
INTERFACE
   IPointOfInterest

DESCRIPTION
   Interface for getting points of interest... 
   for methods where a container is passed in to be filled, the results
   are appended to the container. if you are reusing containers, be sure
   to clear the container (and possibly reserve space) before refilling it
*****************************************************************************/
// {32574861-444E-46dc-A12B-904A083E68EC}
DEFINE_GUID(IID_IPointOfInterest, 
0x32574861, 0x444e, 0x46dc, 0xa1, 0x2b, 0x90, 0x4a, 0x8, 0x3e, 0x68, 0xec);
class IPointOfInterest
{
public:
   // Returns all points of interest for a segment
   // If the segment index is ALL_SEGMENTS, returns pois for all segments in the specified girder
   // If the girder index is ALL_GIRDERS, returns the pois for the specified segments in all girders in the specified group
   // If the group index is ALL_GROUPS, returns the pois for the specified girder and segment all girder groups in the bridge
   // Critical section POI will only be included if they were previously computed
   virtual void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiList* pPoiList) const = 0;

   // Returns all points of interest along the specified line. It is assumed that the
   // line is more or less transverse to the bridge.
   virtual void GetPointsOfInterest(Float64 station,IDirection* pDirection, std::vector<pgsPointOfInterest>* pvPoi) const = 0;

   // Returns all points of interest for a segment that include the attrib attributes
   // The use of ALL_GROUPS, ALL_GIRDERS, and ALL_SEGMENTS is permitted
   virtual void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib, PoiList* pPoiList,Uint32 mode = POIFIND_OR) const = 0;

   virtual void GetPointsOfInterest(const CSpanKey& spanKey,PoiList* pPoiList) const = 0;
   virtual void GetPointsOfInterest(const CSpanKey& spanKey,PoiAttributeType attrib, PoiList* pPoiList, Uint32 mode = POIFIND_OR) const = 0;

   // Merges the two POI lists, eliminating any duplicates, and returns the sorted list
   virtual void MergePoiLists(const PoiList& list1, const PoiList& list2, PoiList* pPoiList) const = 0;

   // Sorts a POI list, eliminating any duplicates
   virtual void SortPoiList(PoiList* pPoiList) const = 0;

   // Returns a poi with a specified ID
   virtual const pgsPointOfInterest& GetPointOfInterest(PoiIDType poiID) const = 0;

   // Returns a point of interest at the specified distance from the start of the segment. If a poi
   // has been, or can be, defined at the location, returns an actual poi. Otherwise a poi is created 
   // (but not stored) and it will have an ID of INVALID_ID.
   virtual pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) const = 0;

   // Gets the point of interest where a line defined by station and direction intersects a segment
   // Returns true if the poi is found.
   virtual bool GetPointOfInterest(const CSegmentKey& segmentKey,Float64 station,IDirection* pDirection,pgsPointOfInterest* pPoi) const = 0;

   // Gets the point of interest where a line defined by station and direction intersects a girder.
   // if bProjectSegmentEnds is true and the line does not intersect a segment, nearest segment end is found
   // Returns true if the poi is found.
   virtual bool GetPointOfInterest(const CGirderKey& girderKey,Float64 station,IDirection* pDirection,bool bProjectSegmentEnds,pgsPointOfInterest* pPoi) const = 0;

   // Returns the poi that is nearest to the specified location on a segment
   virtual const pgsPointOfInterest& GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) const = 0;

   // Returns the previous/next poi with the specified attributes relative to the poi specified by ID.
   virtual const pgsPointOfInterest& GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR) const = 0;
   virtual const pgsPointOfInterest& GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR) const = 0;

   // Gets the critical section for shear points of interest
   virtual void GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey, PoiList* pPoiList) const = 0;
   virtual void GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey,const GDRCONFIG& config,std::vector<pgsPointOfInterest>* pvPoi) const = 0;

   // Returns a point of interest that is located at the intersection of a girder and a pier
   virtual pgsPointOfInterest GetPierPointOfInterest(const CGirderKey& girderKey,PierIndexType pierIdx) const = 0;

   // Returns a point of interest that is located at the intersection of a girder and a temporary support
   virtual pgsPointOfInterest GetTemporarySupportPointOfInterest(const CGirderKey& girderKey,SupportIndexType tsIdx) const = 0;

   // Returns the points of interest located in the range xLeft-xRight around the given POI.
   // The returned vector of POI does not include POIs that are in adjacent segments
   virtual void GetPointsOfInterestInRange(Float64 XpoiLeft, const pgsPointOfInterest& poi,
	   Float64 XpoiRight, PoiList* vPois) const = 0;

   // Returns the pier index associated with the poi. The poi must have POI_BOUNDARY_PIER attribute set.
   // If this POI is not associated with a pier, INVALID_INDEX is returned.
   virtual PierIndexType GetPier(const pgsPointOfInterest& poi) const = 0;

   // Gets the POI at the start and end of the specified PT duct
   virtual void GetDuctRange(const CGirderKey& girderKey, DuctIndexType ductIdx, const pgsPointOfInterest** ppStartPoi, const pgsPointOfInterest** ppEndPoi) const = 0;

   // takes a vector of poi that span multiple segments and breaks it up into a list of
   // vectors with each vector having poi that belong to a single segment
   virtual void GroupBySegment(const PoiList& vPoi,std::list<PoiList>* pList) const = 0;

   // takes a vector of poi that span multiple girders and breaks it up into a list of
   // vectors with each vector having poi that belong to a single girder
   virtual void GroupByGirder(const PoiList& vPoi,std::list<PoiList>* pList) const = 0;

   // returns a vector of all the different segment keys found in the vector of poi
   virtual void GetSegmentKeys(const PoiList& vPoi,std::vector<CSegmentKey>* pvSegments) const = 0;

   // returns a vector of all the different segment keys found in the vector of poi that belong to the specified girder
   virtual void GetSegmentKeys(const PoiList& vPoi,const CGirderKey& girderKey,std::vector<CSegmentKey>* pvSegments) const = 0;

   // returns a vector of all the different girders keys found in the vector of poi
   virtual void GetGirderKeys(const PoiList& vPoi,std::vector<CGirderKey>* pvGirders) const = 0;


   // Removes all poi from the vector that have the specified attribute unless the poi also has the
   // exception attribute, in which case it is retained
   virtual void RemovePointsOfInterest(PoiList& vPoi,PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute=0) const = 0;

   // Removes all point from the list that are not on a girder. A poi can be off the girder when
   // it is between groups at the CL pier of a continuous span. The poi is a span poi, but it 
   // is not on the girder
   virtual void RemovePointsOfInterestOffGirder(PoiList& vPoi) const = 0;

   // Returns true if the poi is in the closure joint. This includes points with the POI_CLOSURE attribute and
   // points that simply fall between the end face of segments. Returns false if the poi is not within
   // the closure joint or exactly on the end faces of the segment. pClosureKey will return the closure key
   // for the closure joint that the poi falls in.
   virtual bool IsInClosureJoint(const pgsPointOfInterest& poi,CClosureKey* pClosureKey) const = 0;

   // returns true if the poi is on/off the segment. This determination is made based solely on
   // the location of the poi and the length of the segment
   virtual bool IsOnSegment(const pgsPointOfInterest& poi) const = 0;
   virtual bool IsOffSegment(const pgsPointOfInterest& poi) const = 0;

   // returns true if the poi is on the girder. This determination is made based solely on
   // the location of the poi in girder coordinates and the and the length of the girder
   virtual bool IsOnGirder(const pgsPointOfInterest& poi) const = 0;

   // returns true if the poi is in an intermediate diaphragm at a boundary pier
   virtual bool IsInBoundaryPierDiaphragm(const pgsPointOfInterest& poi) const = 0;

   // returns true if the poi is in the zone between the nearest support
   // and the critical section for shear
   virtual bool IsInCriticalSectionZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState) const = 0;

   // returns the index of the deck casting region at the poi
   // returns INVALID_INDEX if there is not a deck region defined at that location
   virtual IndexType GetDeckCastingRegion(const pgsPointOfInterest& poi) const = 0;

   //////////////////////////////
   // Conversion Methods
   //////////////////////////////

   // Note: conversion methods that are commented out haven't been implemented yet.

   // pgsPointOfInterest::GetDistFromStart() is the POI coordinate in Segment Coordinates (no conversion methods needed)

   // Convert between POI and Segment Path Coordinates
   virtual Float64 ConvertPoiToSegmentPathCoordinate(const pgsPointOfInterest& poi) const = 0;
   virtual pgsPointOfInterest ConvertSegmentPathCoordinateToPoi(const CSegmentKey& segmentKey,Float64 Xsp) const = 0;

   // Convert between Segment and Segment Path Coordinates
   virtual Float64 ConvertSegmentPathCoordinateToSegmentCoordinate(const CSegmentKey& segmentKey,Float64 Xsp) const = 0;
   virtual Float64 ConvertSegmentCoordinateToSegmentPathCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const = 0;

   // Convert between Segment and Girder Coordinates
   virtual Float64 ConvertSegmentCoordinateToGirderCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const = 0;
   //virtual void ConvertGirderCoordinateToSegmentCoordinate(const CGirderKey& girderKey,Float64 Xg,CSegmentKey* pSegmentKey,Float64* pXs) const = 0;

   // Convert between Segment Path and Girder Path Coordinates
   virtual Float64 ConvertSegmentPathCoordinateToGirderPathCoordinate(const CSegmentKey& segmentKey,Float64 Xsp) const = 0;
   //virtual void ConvertGirderPathCoordinateToSegmentPathCoordinate(const CGirderKey& girderKey,Float64 Xgp,CSegmentKey* pSegmentKey,Float64* pXsp) const = 0;

   // Convert between Segment and Girderline Coordinates
   virtual Float64 ConvertSegmentCoordinateToGirderlineCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const = 0;
   //virtual void ConvertGirderlineCoordinateToSegmentCoordinate(const CGirderKey& girderKey,Float64 Xgl,CSegmentKey* pSegmentKey,Float64* pXs) const = 0;

   // Convert between POI and Girder Path Coordinates
   virtual Float64 ConvertPoiToGirderPathCoordinate(const pgsPointOfInterest& poi) const = 0;
   virtual pgsPointOfInterest ConvertGirderPathCoordinateToPoi(const CGirderKey& girderKey,Float64 Xgp) const = 0;

   // Convert between POI and Girder Coordinates
   virtual Float64 ConvertPoiToGirderCoordinate(const pgsPointOfInterest& poi) const = 0;
   virtual pgsPointOfInterest ConvertGirderCoordinateToPoi(const CGirderKey& girderKey,Float64 Xg) const = 0;

   // Convert between POI and Girderline Coordinates
   virtual Float64 ConvertPoiToGirderlineCoordinate(const pgsPointOfInterest& poi) const = 0;
   virtual pgsPointOfInterest ConvertGirderlineCoordinateToPoi(GirderIndexType gdrIdx,Float64 Xgl) const = 0;

   // Convert between Girder and Girder Path Coordinates
   virtual Float64 ConvertGirderCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float64 Xg) const = 0;
   virtual Float64 ConvertGirderPathCoordinateToGirderCoordinate(const CGirderKey& girderKey,Float64 Xgp) const = 0;

   // Convert between Girder Path and Girderline Coordinates
   virtual Float64 ConvertGirderPathCoordinateToGirderlineCoordinate(const CGirderKey& girderKey,Float64 Xgp) const = 0;
   //virtual Float64 ConvertGirderlineCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float46 Xgl) const = 0;

   // Converts between Span Point and Poi
   virtual pgsPointOfInterest ConvertSpanPointToPoi(const CSpanKey& spanKey,Float64 Xspan) const = 0;
   virtual void ConvertPoiToSpanPoint(const pgsPointOfInterest& poi,CSpanKey* pSpanKey,Float64* pXspan) const = 0;

   // Converts Span Point to Segment Coordiante
   virtual void ConvertSpanPointToSegmentCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXs) const = 0;
   virtual void ConvertSegmentCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xs,CSpanKey* pSpanKey,Float64* pXspan) const = 0;

   // Converts Span Point to Segment Path Coordiante
   virtual void ConvertSpanPointToSegmentPathCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXsp) const = 0;
   virtual void ConvertSegmentPathCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xsp,CSpanKey* pSpanKey,Float64* pXspan) const = 0;

   // Converts a Route Coordinate (station) to a Bridge Line Coordinate (distance from start of bridge)
   virtual Float64 ConvertRouteToBridgeLineCoordinate(Float64 station) const = 0;
   virtual Float64 ConvertBridgeLineToRouteCoordinate(Float64 Xb) const = 0;

   // Returns the Bridge Line Coordinate for a poi. (distance along the alignment from 
   // the start of the bridge to the station where a line normal to the alignment, 
   // passing through the POI intersects the alignment)
   virtual Float64 ConvertPoiToBridgeLineCoordinate(const pgsPointOfInterest& poi) const = 0;
};

/*****************************************************************************
INTERFACE
   ISegmentLiftingPointsOfInterest

   Interface to points of interest for lifting

DESCRIPTION
   Interface to points of interest for lifting
*****************************************************************************/
// non-COM version
class ISegmentLiftingDesignPointsOfInterest
{
public:
   // locations of points of interest
   virtual void GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType attrib, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode = POIFIND_OR) const = 0;
};

// {19EA189E-E5F4-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_ISegmentLiftingPointsOfInterest, 
0x19ea189e, 0xe5f4, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
class ISegmentLiftingPointsOfInterest : public ISegmentLiftingDesignPointsOfInterest
{
public:
   virtual void GetLiftingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,PoiList* pPoiList,Uint32 mode = POIFIND_OR) const = 0;
};

/*****************************************************************************
INTERFACE
   ISegmentHaulingPointsOfInterest

   Interface to points of interest for Hauling

DESCRIPTION
   Interface to points of interest for Hauling
*****************************************************************************/
// non-COM version
class ISegmentHaulingDesignPointsOfInterest
{
public:
   // locations of points of interest
   virtual void GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode = POIFIND_OR) const = 0;
};

// {E6A0E250-E5F4-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_ISegmentHaulingPointsOfInterest, 
0xe6a0e250, 0xe5f4, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
class ISegmentHaulingPointsOfInterest : public ISegmentHaulingDesignPointsOfInterest
{
public:
   // locations of points of interest
   virtual void GetHaulingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib, PoiList* pPoiList,Uint32 mode = POIFIND_OR) const = 0;
   virtual Float64 GetMinimumOverhang(const CSegmentKey& segmentKey) const = 0;
};
