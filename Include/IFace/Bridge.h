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

#ifndef INCLUDED_IFACE_BRIDGE_H_
#define INCLUDED_IFACE_BRIDGE_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
#include <vector>

#include <PGSuperTypes.h>
#include <Details.h>

#include <WBFLCore.h>
#include <WBFLTools.h>
#include <WBFLGeometry.h>

#include <PgsExt\LongRebarInstance.h>
#include <IFace\PointOfInterest.h>

///////////////////////////////////////////////////////////////////////////////
// NOTES:
// Slab Overhang = Distance from exterior girder to edge of slab
// Slab Edge Offset = Distance from Alignment to edge of slab
// Slab Offset = "A" Dimension

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;
class matPsStrand;
class matRebar;
class rptChapter;
class CPrecastSegmentData;
class CSplicedGirderData;

interface IRCBeam2Ex;
interface IEAFDisplayUnits;

interface IDirection;
interface IAngle;

interface IRebar;
interface IRebarSection;
interface IRebarSectionItem;
interface IRebarLayout;

// MISCELLANEOUS
//

struct IntermedateDiaphragm
{
   IntermedateDiaphragm() :
   m_bCompute(false),P(0),H(0),T(0),W(0),Location(0)
   {
   }
   bool m_bCompute; // if true, compuated based on H, T, and W, other use P
   Float64 P; // weight
   Float64 H; // height
   Float64 T; // thickness
   Float64 W; // width
   Float64 Location; // measured from left end of girder
};

struct SpaceBetweenGirder
{
   SpaceBetweenGirder() : firstGdrIdx(INVALID_INDEX),lastGdrIdx(INVALID_INDEX),spacing(0)
   {
   }
   GirderIndexType firstGdrIdx, lastGdrIdx;
   Float64 spacing;
};

/*****************************************************************************
INTERFACE
   IBridge

   Interface to get information about the bridge.

DESCRIPTION
   Interface to get information about the bridge.
*****************************************************************************/
// {3BB24886-677B-11d2-883A-006097C68A9C}
DEFINE_GUID(IID_IBridge, 
0x3bb24886, 0x677b, 0x11d2, 0x88, 0x3a, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IBridge : IUnknown
{
   // Returns the overall length of the bridge measured along the alignment between the first and last pier
   virtual Float64 GetLength() = 0;

   // Returns the span length between piers measured along the alignment
   virtual Float64 GetSpanLength(SpanIndexType spanIdx) = 0;

   // Returns the offset from the alignment to the bridge line
   virtual Float64 GetAlignmentOffset() = 0;

   // Returns number of spans
   virtual SpanIndexType GetSpanCount() = 0;

   // Returns number of permanent piers
   virtual PierIndexType GetPierCount() = 0;

   // Returns number of temporary supports
   virtual SupportIndexType GetTemporarySupportCount() = 0;

   // Returns number of girder groups
   virtual GroupIndexType GetGirderGroupCount() = 0;

   // Returns number of girders in a girder group
   virtual GirderIndexType GetGirderCount(GroupIndexType grpIdx) = 0;

   // Returns the number of girders in a span
   virtual GirderIndexType GetGirderCountBySpan(SpanIndexType spanIdx) = 0;

   // Returns the index of the pier at the start of a group
   virtual PierIndexType GetGirderGroupStartPier(GroupIndexType grpIdx) = 0;

   // Returns the index of the pier at the end of a group
   virtual PierIndexType GetGirderGroupEndPier(GroupIndexType grpIdx) = 0;

   // Returns the pier indices at the boundary of a group
   virtual void GetGirderGroupPiers(GroupIndexType grpIdx,PierIndexType* pStartPierIdx,PierIndexType* pEndPierIdx) = 0;

   // Returns the index of the span at the start of a group
   virtual SpanIndexType GetGirderGroupStartSpan(GroupIndexType grpIdx) = 0;

   // Returns the index of the span at the end of a group
   virtual SpanIndexType GetGirderGroupEndSpan(GroupIndexType grpIdx) = 0;

   // Returns the spans indices at the boundary of a group
   virtual void GetGirderGroupSpans(GroupIndexType grpIdx,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) = 0;

   // Returns the girder group index that a span is part of
   virtual GroupIndexType GetGirderGroupIndex(SpanIndexType spanIdx) = 0;

   // Returns the distance along the alignment from the start of the bridge to a particular station
   virtual Float64 GetDistanceFromStartOfBridge(Float64 station) = 0;

   // Returns the brg-brg span length for the specified girder in the specified span
   // measured along the CL girder
   virtual Float64 GetSpanLength(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   // returns the CL Pier to CL Pier girder length
   virtual Float64 GetGirderLayoutLength(const CGirderKey& girderKey) = 0;

   //
   // Segment geometry
   //

   // Returns a vector of segment index/length pairs for the length of each segment in a span for a given girder.
   // The first item in the pair is the segment index and the segment item is the length of the segment
   // in the given span. Segment lengths are measured between CL-Piers and CL-Temporary Supports except for the
   // first and last segment in a group where the start and end of the segments are measured from the CL-Bearing.
   virtual std::vector<std::pair<SegmentIndexType,Float64>> GetSegmentLengths(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   // Determines the segment, and the location on the segment, for a point measured from the start of a span along a girder line
   virtual void GetSegmentLocation(SpanIndexType spanIdx,GirderIndexType girderIdx,Float64 distFromStartOfSpan,CSegmentKey* pSegmentKey,Float64* pDistFromStartOfSegment) = 0;

   // Returns the number of segments in a girder
   virtual SegmentIndexType GetSegmentCount(const CGirderKey& girderKey) = 0;
   virtual SegmentIndexType GetSegmentCount(GroupIndexType grpIdx,GirderIndexType gdrIdx) = 0;

   // End-to-end length of segment
   virtual Float64 GetSegmentLength(const CSegmentKey& segmentKey) = 0;

   // CL Brg to CL Brg span length (measured between CL Brg of temporary supports if used)
   virtual Float64 GetSegmentSpanLength(const CSegmentKey& segmentKey) = 0;

   // CL Pier - CL Pier length of the segment centerline
   virtual Float64 GetSegmentLayoutLength(const CSegmentKey& segmentKey) = 0;

   // Distance from end of girder to C.L. bearing - along girder
   virtual Float64 GetSegmentStartEndDistance(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentEndEndDistance(const CSegmentKey& segmentKey) = 0;
   
   // Distance from C.L. pier to end of girder - along girder
   virtual Float64 GetSegmentStartBearingOffset(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentEndBearingOffset(const CSegmentKey& segmentKey) = 0;

   virtual Float64 GetSegmentStartSupportWidth(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentEndSupportWidth(const CSegmentKey& segmentKey) = 0;

   // End-to-end length of segment, measured along the grade 
   virtual Float64 GetSegmentPlanLength(const CSegmentKey& segmentKey) = 0;

   // Grade of segment
   virtual Float64 GetSegmentSlope(const CSegmentKey& segmentKey) = 0;

   // Distnace from CLPier to CL Bearing, measured along CL of segment
   virtual Float64 GetCLPierToCLBearingDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) = 0;
   virtual Float64 GetCLPierToSegmentEndDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) = 0;

   // Returns the direction of a segment
   virtual void GetSegmentBearing(const CSegmentKey& segmentKey,IDirection** ppBearing) = 0;

   // Returns the angle between a segment and the CL of its supporting element (pier or temporary support)
   virtual void GetSegmentAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) = 0;

   // Returns the key for the segment that crosses the pier
   virtual CSegmentKey GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey) = 0;

   // Gets the span indices for the spans the segment starts and ends in
   virtual void GetSpansForSegment(const CSegmentKey& segmentKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) = 0;

   // Returns normal distance from the alignment, at the given station, to the CL-Segment line (extended) 
   virtual Float64 GetSegmentOffset(const CSegmentKey& segmentKey,Float64 station) = 0;

   // Returns a point on a segment, located at distFromStartOfSegment
   virtual void GetPoint(const CSegmentKey& segmentKey,Float64 distFromStartOfSegment,IPoint2d** ppPoint) = 0;

   // Returns a point on a segment at a point of interest
   virtual void GetPoint(const pgsPointOfInterest& poi,IPoint2d** ppPoint) = 0;

   // Returns station and offset for a point on a segment
   virtual void GetStationAndOffset(const CSegmentKey& segmentKey,Float64 distFromStartOfSegment,Float64* pStation,Float64* pOffset) = 0;

   // Returns station and offset for a point on a segment
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset) = 0;

   // Computes the intersection point of a segment and a pier. Returns true if the intersection is found
   virtual bool GetSegmentPierIntersection(const CSegmentKey& segmentKey,PierIndexType pierIdx,IPoint2d** ppPoint) = 0;

   // Computes the intersection point of a segment and a temporary support. Returns true if the intersection is found
   virtual bool GetSegmentTempSupportIntersection(const CSegmentKey& segmentKey,SupportIndexType tsIdx,IPoint2d** ppPoint) = 0;


   // Gets the span index and distance from the start of the span (measured from CL Pier) for a point
   // measured from the CL Pier of the first abutment to a point on a girderline
   virtual void GetDistFromStartOfSpan(GirderIndexType gdrIdx,Float64 distFromStartOfBridge,SpanIndexType* pSpanIdx,Float64* pDistFromStartOfSpan) = 0;

   // Returns true if the girder is in an interior girder
   virtual bool IsInteriorGirder(const CGirderKey& girderKey) = 0;

   // Returns true if the girder is in an exterior girder
   virtual bool IsExteriorGirder(const CGirderKey& girderKey) = 0;

   // Returns true if the girder is in the left exterior girder
   virtual bool IsLeftExteriorGirder(const CGirderKey& girderKey) = 0;

   // Returns true if the girder is in the right exterior girder
   virtual bool IsRightExteriorGirder(const CGirderKey& girderKey) = 0;

   // Returns true if the girder segment has a roughened surface
   virtual bool AreGirderTopFlangesRoughened(const CSegmentKey& segmentKey) = 0;

   // Gets the span index for a given staiton. Returns false is the station is before or after the bridge
   virtual bool GetSpan(Float64 station,SpanIndexType* pSpanIdx) = 0;

   // clear distance between girders. If poi is on an exterior girder, the left/right parameter will
   // be zero
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight) = 0;

   // used to get girder spacing for the bridge model section view
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(SpanIndexType spanIdx,Float64 distFromStartOfSpan) = 0;

   // returns girder spacing at a pier. The vector will contain nGirders-1 spaces
   virtual std::vector<Float64> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType) = 0;

   // Returns the left and right girder spacing for a point along a girder. If the girder is an exterior girder
   // the slab overhang is returned on the exterior side of the girder.
   virtual void GetSpacingAlongGirder(const CSegmentKey& girderKey,Float64 distFromStartOfGirder,Float64* leftSpacing,Float64* rightSpacing) = 0;

   // Returns the configuration data for a segment (material properties, strands, etc)
   virtual GDRCONFIG GetSegmentConfiguration(const CSegmentKey& segmentKey) = 0;

   // Diaphragms
   virtual void GetBackSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH) = 0;
   virtual void GetAheadSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH) = 0;
   // return true if weight of diaphragm is carried by girder
   virtual bool DoesLeftSideEndDiaphragmLoadGirder(PierIndexType pier) = 0;
   virtual bool DoesRightSideEndDiaphragmLoadGirder(PierIndexType pier) = 0;
   // Get location of end diaphragm load (c.g.) measured from c.l. pier. along girder
   // Only applicable if DoesEndDiaphragmLoadGirder returns true
   virtual Float64 GetEndDiaphragmLoadLocationAtStart(const CSegmentKey& segmentKey)=0;
   virtual Float64 GetEndDiaphragmLoadLocationAtEnd(const CSegmentKey& segmentKey)=0;
   virtual std::vector<IntermedateDiaphragm> GetIntermediateDiaphragms(pgsTypes::DiaphragmType diaphragmType,const CSegmentKey& segmentKey) = 0;

   // Slab data
   virtual pgsTypes::SupportedDeckType GetDeckType() = 0;
   virtual Float64 GetSlabOffset(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) = 0;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual bool IsCompositeDeck() = 0;
   virtual bool HasOverlay() = 0;
   virtual bool IsFutureOverlay() = 0;
   virtual Float64 GetOverlayWeight() = 0;
   virtual Float64 GetOverlayDepth() = 0;
   virtual Float64 GetFillet() = 0;
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi) = 0;

   // Returns distance from the left exterior girder to the edge of slab, measured normal to the alignment
   virtual Float64 GetLeftSlabOverhang(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetLeftSlabOverhang(PierIndexType pier) = 0;

   // Returns distance from the right exterior girder to the edge of slab, measured normal to the alignment
   virtual Float64 GetRightSlabOverhang(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetRightSlabOverhang(PierIndexType pier) = 0;

   // Returns distance from the alignment to the left slab edge, measured normal to the alignment
   virtual Float64 GetLeftSlabEdgeOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pier) = 0;

   // Returns distance from the alignment to the right slab edge, measured normal to the alignment
   virtual Float64 GetRightSlabEdgeOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pier) = 0;


   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetCurbToCurbWidth(const CSegmentKey& segmentKey,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetCurbToCurbWidth(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftCurbOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightCurbOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftCurbOffset(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetRightCurbOffset(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetLeftCurbOffset(PierIndexType pier) = 0;
   virtual Float64 GetRightCurbOffset(PierIndexType pier) = 0;

   // Offset distances to curbline of interior barrier or sidewalk curb if present
   virtual Float64 GetLeftInteriorCurbOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightInteriorCurbOffset(Float64 distFromStartOfBridge) = 0;
   // this are the locations that the overlay butts up to
   virtual Float64 GetLeftOverlayToeOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightOverlayToeOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftOverlayToeOffset(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetRightOverlayToeOffset(const pgsPointOfInterest& poi) = 0;

   virtual void GetSlabPerimeter(CollectionIndexType nPoints,IPoint2dCollection** points) = 0;
   virtual void GetSlabPerimeter(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,CollectionIndexType nPoints,IPoint2dCollection** points) = 0;
   virtual void GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,IPoint2dCollection** points) = 0;

   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point) = 0;
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point) = 0;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point) = 0;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point) = 0;

   // Pier data
   virtual Float64 GetPierStation(PierIndexType pier) = 0;
   virtual Float64 GetAheadBearingStation(PierIndexType pier,const CGirderKey& girderKey) = 0;
   virtual Float64 GetBackBearingStation(PierIndexType pier,const CGirderKey& girderKey) = 0;
   virtual void GetPierDirection(PierIndexType pier,IDirection** ppDirection) = 0;
   virtual void GetPierSkew(PierIndexType pier,IAngle** ppAngle) = 0;
   virtual void GetPierPoints(PierIndexType pier,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right) = 0;
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) = 0;
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) = 0;
   virtual void GetContinuityEventIndex(PierIndexType pierIdx,EventIndexType* pLeft,EventIndexType* pRight) = 0;
   virtual pgsTypes::PierConnectionType GetPierConnectionType(PierIndexType pierIdx) = 0;

   // Computes the location of the pier in the segment coordinate system. Returns false if the pier is not located on the segment
   virtual bool GetPierLocation(PierIndexType pierIdx,const CSegmentKey& segmentKey,Float64* pDistFromStartOfSegment) = 0;

   // Computes the location of the CL Pier measured along the CL girder, measured from the CL of the first pier
   // The value that is returned is in the girder path coordinate system
   virtual Float64 GetPierLocation(PierIndexType pierIdx,GirderIndexType gdrIdx) = 0;

   // returns the skew angle of a line define defined by the orientation string at a given station
   // this is usefuly for determing the skew angle of piers that aren't in the bridge model yet
   // returns false if there is an error in the strOrientation string
   virtual bool GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew) = 0;

   // negative moment calculations and results need not be processed if a simple span analysis is
   // used or if there isn't any continuity.
   // this method returns true when negative moments should be processed for a given span.
   // Use ALL_SPANS for to evaluate all spans
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx) = 0;

   // returns the location of a temporary support within a span, measured along a girder line
   virtual void GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx,SpanIndexType* pSpanIdx,Float64* pDistFromStartOfSpan) = 0;

   // computes the distance from the start of the segment to the temporary support. returns false if the temporary support is not located on the segment
   virtual bool GetTemporarySupportLocation(SupportIndexType tsIdx,const CSegmentKey& segmentKey,Float64* pDistFromStartOfSegment) = 0;

   // returns the location of the temporary support measured along the CL girder, measured from the CL of the first pier
   virtual Float64 GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx) = 0;

   // returns the temporary support type (StrongBack or ErectionTower)
   virtual pgsTypes::TemporarySupportType GetTemporarySupportType(SupportIndexType tsIdx) = 0;

   // returns the segment connection type at the temporary support
   virtual pgsTypes::SegmentConnectionType GetSegmentConnectionTypeAtTemporarySupport(SupportIndexType tsIdx) = 0;

   // gets the segment keys for the segments framing into the left and right side of a temporary support
   virtual void GetSegmentsAtTemporarySupport(GirderIndexType gdrIdx,SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey) = 0;

   virtual void GetTemporarySupportDirection(SupportIndexType tsIdx,IDirection** ppDirection) = 0;
};

/*****************************************************************************
INTERFACE
   IMaterials

   Interface to get information about the bridge materials.

DESCRIPTION
   Interface to get information about the bridge materials.
*****************************************************************************/
// {B6904E95-0758-4fe3-A213-BFC0F3203F11}
DEFINE_GUID(IID_IMaterials, 
0xb6904e95, 0x758, 0x4fe3, 0xa2, 0x13, 0xbf, 0xc0, 0xf3, 0x20, 0x3f, 0x11);
interface IMaterials : IUnknown
{
   // Returns the concrete strength at an age of 28 days
   virtual Float64 GetSegmentFc28(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetClosurePourFc28(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetDeckFc28() = 0;
   virtual Float64 GetRailingSystemFc28(pgsTypes::TrafficBarrierOrientation orientation) = 0;

   // Returns the secant modulus at an age of 28 days
   virtual Float64 GetSegmentEc28(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetClosurePourEc28(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetDeckEc28() = 0;
   virtual Float64 GetRailingSystemEc28(pgsTypes::TrafficBarrierOrientation orientation) = 0;

   // Returns the weight density of the material at the specified interval. If the component has not
   // been added to the system yet, this method returns 0
   virtual Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourWeightDensity(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckWeightDensity(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemWeightDensity(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the age of concrete at the middle of an interval. Returns 0 if the concrete has not
   // be cast or hasn't attained sufficient strength prior to this interval.
   virtual Float64 GetSegmentConcreteAge(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourConcreteAge(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckConcreteAge(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemAge(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the concrete strength at the middle of an interval
   virtual Float64 GetSegmentFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckFc(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the secant modulus at the middle of an interval
   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) = 0;
   virtual Float64 GetClosurePourEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) = 0;
   virtual Float64 GetDeckEc(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the modulus of rupture at the middle of an interval
   virtual Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourFlexureFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourShearFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckFlexureFr(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckShearFr(IntervalIndexType intervalIdx) = 0;

   // Returns the aging coefficient during an interval
   virtual Float64 GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourAgingCoefficient(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckAgingCoefficient(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the age adjusted modulus from the middle to the end of an interval
   virtual Float64 GetSegmentAgeAdjustedEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourAgeAdjustedEc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckAgeAdjustedEc(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemAgeAdjustedEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the total free shrinkage at the relative time within the specified interval
   virtual Float64 GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) = 0;
   virtual Float64 GetClosurePourFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) = 0;
   virtual Float64 GetDeckFreeShrinkageStrain(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) = 0;
   virtual Float64 GetRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) = 0;

   // Returns the free shrinkage occuring within the specified interval
   virtual Float64 GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetClosurePourFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetDeckFreeShrinkageStrain(IntervalIndexType intervalIdx) = 0;
   virtual Float64 GetRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) = 0;

   // Returns the creep coefficient at the specified time (timeType) in interval (intervalIdx) for a loading
   // occuring at time (loadingTimeType) in interval (loadingIntervalIdx). 
   virtual Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) = 0;
   virtual Float64 GetClosurePourCreepCoefficient(const CSegmentKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) = 0;
   virtual Float64 GetDeckCreepCoefficient(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) = 0;
   virtual Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) = 0;

   // Segment Concrete
   virtual pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey) = 0;
   virtual bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentEccK1(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentEccK2(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey) = 0;

   // Closure Pour Concrete
   virtual pgsTypes::ConcreteType GetClosurePourConcreteType(const CSegmentKey& closureKey) = 0;
   virtual bool DoesClosurePourConcreteHaveAggSplittingStrength(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourConcreteAggSplittingStrength(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourStrengthDensity(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourMaxAggrSize(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourEccK1(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourEccK2(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourCreepK1(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourCreepK2(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourShrinkageK1(const CSegmentKey& closureKey) = 0;
   virtual Float64 GetClosurePourShrinkageK2(const CSegmentKey& closureKey) = 0;

   // Deck Concrete
   virtual pgsTypes::ConcreteType GetDeckConcreteType() = 0;
   virtual bool DoesDeckConcreteHaveAggSplittingStrength() = 0;
   virtual Float64 GetDeckConcreteAggSplittingStrength() = 0;
   virtual Float64 GetDeckMaxAggrSize() = 0;
   virtual Float64 GetDeckEccK1() = 0;
   virtual Float64 GetDeckEccK2() = 0;
   virtual Float64 GetDeckCreepK1() = 0;
   virtual Float64 GetDeckCreepK2() = 0;
   virtual Float64 GetDeckShrinkageK1() = 0;
   virtual Float64 GetDeckShrinkageK2() = 0;

   // Prestressing Strand
   virtual const matPsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetStrandRelaxation(const CSegmentKey& segmentKey,Float64 t1,Float64 t2,Float64 fpso,pgsTypes::StrandType strandType) = 0;

   // PT Tendon
   virtual const matPsStrand* GetTendonMaterial(const CGirderKey& girderKey) = 0;
   virtual Float64 GetTendonRelaxation(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64 t1,Float64 t2,Float64 fpso) = 0;

   // Properties of Precast Segment Longitudinal Rebar
   virtual void GetSegmentLongitudinalRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetSegmentLongitudinalRebarName(const CSegmentKey& segmentKey) = 0;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade) = 0;

   virtual void GetClosurePourLongitudinalRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetClosurePourLongitudinalRebarName(const CClosureKey& closureKey) = 0;
   virtual void GetClosurePourLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade) = 0;

   // Properties of precast Segment Transverse Rebar
   virtual void GetSegmentTransverseRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetSegmentTransverseRebarName(const CSegmentKey& segmentKey) = 0;
   virtual void GetSegmentTransverseRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade) = 0;

   virtual void GetClosurePourTransverseRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetClosurePourTransverseRebarName(const CClosureKey& closureKey) = 0;
   virtual void GetClosurePourTransverseRebarMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade) = 0;

   // Rebar properties for design
   virtual void GetSlabRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetSlabRebarName() = 0;
   virtual void GetSlabRebarMaterial(matRebar::Type& type,matRebar::Grade& grade) = 0;

   // Density limits for normal and light weight concrete
   virtual Float64 GetNWCDensityLimit() = 0; // returns the minimum density for normal weight concrete
   virtual Float64 GetLWCDensityLimit() = 0; // returns the maximum density for lightweight concrete

   // Material Properties Calcluations
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) = 0;
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) = 0;

   virtual Float64 GetFlexureFrCoefficient(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetShearFrCoefficient(const CSegmentKey& segmentKey) = 0;

   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2) = 0;
};

/*****************************************************************************
INTERFACE
   ILongRebarGeometry

   Interface for getting the geometry of longitudinal rebars in a girder

DESCRIPTION
   Interface for getting the rebar geometry.  

*****************************************************************************/
// {C2EE02C6-1785-11d3-AD6C-00105A9AF985}
DEFINE_GUID(IID_ILongRebarGeometry, 
0xc2ee02c6, 0x1785, 0x11d3, 0xad, 0x6c, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ILongRebarGeometry : IUnknown
{
   typedef enum DeckRebarType {Primary,Supplemental,All} DeckRebarType;

   virtual void GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection) = 0;
   virtual Float64 GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetDevLengthFactor(const CSegmentKey& segmentKey,IRebarSectionItem* rebarItem) = 0;
   virtual Float64 GetDevLengthFactor(IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) = 0;
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi) = 0;

   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // returns the cover to the center of the top mat of deck rebar, measured from
   // the top of the deck (including sacrificial depth)
   virtual Float64 GetCoverTopMat() = 0;

   // returns the location of the top mat of deck rebar, measured from the bottom of the deck
   virtual Float64 GetTopMatLocation(const pgsPointOfInterest& poi,DeckRebarType drt) = 0;

   // returns the area of top mat deck rebar based
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,DeckRebarType drt) = 0;

   // returns the cover to the center of the bottom mat of deck rebar, measured from
   // the bottom of the deck slab
   virtual Float64 GetCoverBottomMat() = 0;

   // returns the location of the bottom mat of deck rebar, measured from the bottom of the deck
   virtual Float64 GetBottomMatLocation(const pgsPointOfInterest& poi,DeckRebarType drt) = 0;

   // returns the area of bottom mat deck rebar based
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,DeckRebarType drt) = 0;

   virtual void GetRebarLayout(const CSegmentKey& segmentKey, IRebarLayout** rebarLayout) = 0;

   virtual REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) = 0;
   //Return index of any rebar rows that have bars outside of the girder section. An empty vector means PASS
   virtual std::vector<RowIndexType> CheckLongRebarGeometry(const CSegmentKey& segmentKey) = 0;
};

/*****************************************************************************
INTERFACE
   IStirrupGeometry

   Interface for getting the stirrup geometry.

DESCRIPTION
   Interface for getting the stirrup geometry.  The geometry is
   the stirrup spacing and area
*****************************************************************************/
// {1FFE79BE-9545-11d2-AC7B-00105A9AF985}
DEFINE_GUID(IID_IStirrupGeometry, 
0x1ffe79be, 0x9545, 0x11d2, 0xac, 0x7b, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IStirrupGeometry : IUnknown
{
   // Primary bar zones
   virtual bool AreStirrupZonesSymmetrical(const CSegmentKey& segmentKey)=0;

   // zone get is zero-based. 
   // zones are sorted left->right across entire girder
   // zones may, or may not, be symmetric about mid-girder
   virtual ZoneIndexType GetNumPrimaryZones(const CSegmentKey& segmentKey)=0;
   virtual void GetPrimaryZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end)=0;
   virtual void GetPrimaryVertStirrupBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing) = 0;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const CSegmentKey& segmentKey,ZoneIndexType zone)=0;
   virtual matRebar::Size GetPrimaryConfinementBarSize(const CSegmentKey& segmentKey,ZoneIndexType zone) = 0;

   // Horizontal Interface additional bar zones
   // zone get is zero-based. 
   // zones are sorted left->right 
   // zones may, or may not, be symmetric about mid-girder
   virtual ZoneIndexType GetNumHorizInterfaceZones(const CSegmentKey& segmentKey)=0;
   virtual void GetHorizInterfaceZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end)=0;
   virtual void GetHorizInterfaceBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing)=0;

   // Additional splitting and confinement bars
   virtual void GetAddSplittingBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing) = 0;
   virtual void GetAddConfinementBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing) = 0;

   // Calculated bar values at poi's
   // Vertical shear
   virtual Float64 GetVertStirrupAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) = 0;
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi) = 0; // stirrup angle=90 for vertical

   // Horizontal interface shear
   virtual bool DoStirrupsEngageDeck(const CSegmentKey& segmentKey)=0;
   virtual bool DoAllPrimaryStirrupsEngageDeck(const CSegmentKey& segmentKey)=0;
   virtual Float64 GetPrimaryHorizInterfaceS(const pgsPointOfInterest& poi)=0;
   virtual Float64 GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)=0;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi)=0;
   virtual Float64 GetAdditionalHorizInterfaceS(const pgsPointOfInterest& poi)=0;
   virtual Float64 GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)=0;
   virtual Float64 GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi)=0;

   // Total area of splitting shear steel between two points along the girder
   virtual Float64 GetSplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end) = 0;

   // Processed confinement bar information - returns max bar size/min spacing in required zone length at both ends of girder
   virtual void GetStartConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) = 0;
   virtual void GetEndConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) = 0;
};

/*****************************************************************************
INTERFACE
   IStrandGeometry

   Interface for getting the prestressing strand geometry.

DESCRIPTION
   Interface for getting the prestressing strand geometry.  The geometry is
   the strand slope and eccentricities.
*****************************************************************************/
//
// Options for development length computation. Approximate was added because the vertical shear design algoritm
// was very slow using the accurate method, but it gave much better results. Hence, a speed/accuracy compromise.
enum DevelopmentAdjustmentType {dlaNone, dlaApproximate, dlaAccurate};

// {99B7A322-67A8-11d2-883A-006097C68A9C}
DEFINE_GUID(IID_IStrandGeometry, 
0x99b7a322, 0x67a8, 0x11d2, 0x88, 0x3a, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IStrandGeometry : IUnknown
{
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetHsEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetSsEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetTempEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi) = 0;

   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetHsEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetSsEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetTempEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift) = 0;
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift) = 0;

   // strand eccentrity measured from the CG of the specified section type... pgsTypes::sptNetDeck is not a valid parameter
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetHsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetSsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetTempEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;

   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig,DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3

   virtual StrandIndexType GetMaxNumPermanentStrands(const CSegmentKey& segmentKey)=0;
   virtual StrandIndexType GetMaxNumPermanentStrands(LPCTSTR strGirderName) = 0;
   // get ratio of harped/straight strands if total permanent strands is used for input. returns false if total doesn't fit
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,const CSegmentKey& segmentKey, StrandIndexType* numStraight, StrandIndexType* numHarped) =0;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped)=0;
   // get next and previous number of strands - return INVALID_INDEX if at end
   virtual StrandIndexType GetNextNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)=0;
   virtual StrandIndexType GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum)=0;
   virtual StrandIndexType GetPreviousNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)=0;
   virtual StrandIndexType GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum)=0;
   // Compute strand Indices as in girder library for given filled strands
   virtual bool ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices)=0;

   // Functions to compute ordered strand filling for straight/harped/temporary fill orders
   virtual bool IsValidNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual bool IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetNextNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetPrevNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) = 0;

   // Function to compute a strand fill array using grid index (as used in in PRESTRESSCONFIG) from an ordered fill of Ns strands
   virtual ConfigStrandFillVector ComputeStrandFill(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType Ns) = 0;
   virtual ConfigStrandFillVector ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns) = 0;

   // Conversions from/to sequential fill index to grid fill index (as used in in PRESTRESSCONFIG). A single grid entry can have two strands
   virtual GridIndexType SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo) = 0;
   virtual void GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) = 0;

   virtual StrandIndexType GetNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type) = 0;
   virtual StrandIndexType GetMaxStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type) = 0;
   virtual StrandIndexType GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type) = 0;

   virtual Float64 GetStrandArea(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::StrandType type) = 0;
   virtual Float64 GetAreaPrestressStrands(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,bool bIncTemp) = 0;

   virtual Float64 GetPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type) = 0;
   virtual Float64 GetPjack(const CSegmentKey& segmentKey,bool bIncTemp) = 0;

   virtual void GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint) = 0;
   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) = 0;
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) = 0; 
   virtual void GetStrandPositionsEx(LPCTSTR strGirderName, const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, pgsTypes::MemberEndType endType, IPoint2dCollection** ppPoints) = 0;

   // Harped strands can be forced to be straight along their length
   virtual bool GetAreHarpedStrandsForcedStraight(const CSegmentKey& segmentKey)=0;
   virtual bool GetAreHarpedStrandsForcedStraightEx(LPCTSTR strGirderName) = 0;

   // harped vertical offsets are measured from original strand locations in strand grid
   virtual Float64 GetGirderTopElevation(const CSegmentKey& segmentKey) = 0;  // highest point on girder section based on strand coordinates (bottom at 0.0)
   virtual void GetHarpStrandOffsets(const CSegmentKey& segmentKey,Float64* pOffsetEnd,Float64* pOffsetHp) = 0;
   virtual void GetHarpedEndOffsetBounds(const CSegmentKey& segmentKey,Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedEndOffsetBoundsEx(const CSegmentKey& segmentKey,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) = 0;
   virtual void GetHarpedHpOffsetBounds(const CSegmentKey& segmentKey,Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedHpOffsetBoundsEx(const CSegmentKey& segmentKey,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)=0;

   virtual Float64 GetHarpedEndOffsetIncrement(const CSegmentKey& segmentKey)=0;
   virtual Float64 GetHarpedHpOffsetIncrement(const CSegmentKey& segmentKey)=0;

   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* lhp,Float64* rhp) = 0;
   virtual void GetHighestHarpedStrandLocation(const CSegmentKey& segmentKey,Float64* pElevation) = 0;
   virtual Float64 GetHarpedEndOffsetIncrement(LPCTSTR strGirderName) = 0;
   virtual Float64 GetHarpedHpOffsetIncrement(LPCTSTR strGirderName) = 0;

   virtual Uint16 GetNumHarpPoints(const CSegmentKey& segmentKey) = 0;

   virtual StrandIndexType GetNumExtendedStrands(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType standType) = 0;
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;

   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64* pStart,Float64* pEnd) = 0;
   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64* pStart,Float64* pEnd) = 0;
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual StrandIndexType GetNumDebondedStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;
   virtual RowIndexType GetNumRowsWithStrand(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType ) = 0;
   virtual StrandIndexType GetNumStrandInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const CSegmentKey& segmentKey, RowIndexType rowIdx, pgsTypes::StrandType strandType ) = 0;
   virtual StrandIndexType GetNumDebondedStrandsInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual bool IsExteriorStrandDebondedInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual bool IsDebondingSymmetric(const CSegmentKey& segmentKey) = 0;

   // these functions return the data for the number of strands given (used during design)
   virtual RowIndexType GetNumRowsWithStrand(const CSegmentKey& segmentKey,StrandIndexType nStrands,pgsTypes::StrandType strandType ) = 0;
   virtual StrandIndexType GetNumStrandInRow(const CSegmentKey& segmentKey,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const CSegmentKey& segmentKey,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType ) = 0;

   // Section locations measured from left end to right
   virtual Float64 GetDebondSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;
   virtual SectionIndexType GetNumDebondSections(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) = 0;
   virtual StrandIndexType GetNumDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;
   virtual StrandIndexType GetNumBondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;

   virtual bool CanDebondStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)=0; // can debond any of the strands?
   virtual bool CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType)=0; // can debond any of the strands?
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) = 0;
   virtual void ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) = 0; 
   virtual Float64 GetDefaultDebondLength(const CSegmentKey& segmentKey) = 0;

   // Functions to compute harped strand offsets based on available measurement types
   // Absolute offset is distance that raw strand grid locations are to be moved.
   // rHarpedFillArray is same as StrandFill in PRESTRESSCONFIG
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)=0;
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)=0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)=0;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)=0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)=0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)=0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) = 0;
   virtual Float64 ConvertHarpedOffsetEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)=0;
   virtual Float64 ConvertHarpedOffsetEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) = 0;
   virtual Float64 ConvertHarpedOffsetHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)=0;
   virtual Float64 ConvertHarpedOffsetHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) = 0;
};

/*****************************************************************************
INTERFACE
   ISectionProperties

   Interface for obtaining section properties.

DESCRIPTION
   Interface for obtaining section properties.
****************************************************************************/
// {28D53414-E8FD-4b53-A9B7-B395EB1E11E7}
DEFINE_GUID(IID_ISectionProperties, 
0x28d53414, 0xe8fd, 0x4b53, 0xa9, 0xb7, 0xb3, 0x95, 0xeb, 0x1e, 0x11, 0xe7);
interface ISectionProperties : IUnknown
{
   // returns the current section properties mode
   virtual pgsTypes::SectionPropertyMode GetSectionPropertiesMode() = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the section properties model defined in the project criteria
   virtual Float64 GetHg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYtGirder(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetStGirder(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetKt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetKb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetEIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the section properties model defined in the project criteria, except that
   // the segment concrete strength is taken to be fcgdr
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetYt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetYb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetSt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetSb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetYtGirder(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetStGirder(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the specified section property type
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYtGirder(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetStGirder(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetKt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetKb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetEIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the specified section property type, except that the segment concrete strength is
   // taken to be fcgdr
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetYt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetYb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetSt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetSb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetYtGirder(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;
   virtual Float64 GetStGirder(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) = 0;

   // Net girder properties
   virtual Float64 GetNetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetNetIg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetNetYbg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetNetYtg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;

   // Net deck properties
   virtual Float64 GetNetAd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetNetId(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetNetYbd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetNetYtd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) = 0;


   virtual Float64 GetQSlab(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAcBottomHalf(const pgsPointOfInterest& poi) = 0; // for Fig. 5.8.3.4.2-3
   virtual Float64 GetAcTopHalf(const pgsPointOfInterest& poi) = 0; // for Fig. 5.8.3.4.2-3

   virtual Float64 GetEffectiveFlangeWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetTributaryFlangeWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetTributaryFlangeWidthEx(const pgsPointOfInterest& poi, Float64* pLftFw, Float64* pRgtFw) = 0;

   virtual Float64 GetEffectiveDeckArea(const pgsPointOfInterest& poi) = 0; // deck area based on effective flange width
   virtual Float64 GetTributaryDeckArea(const pgsPointOfInterest& poi) = 0; // deck area based on tributary width
   virtual Float64 GetGrossDeckArea(const pgsPointOfInterest& poi) = 0;     // same as triburary deck area, except gross slab depth is used

   // Distance from top of slab to top of girder - Does not account for camber
   virtual Float64 GetDistTopSlabToTopGirder(const pgsPointOfInterest& poi) = 0;

   // Reporting
   virtual void ReportEffectiveFlangeWidth(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   // Volume and surface area
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSurfaceArea(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetVolume(const CSegmentKey& segmentKey) = 0;

   // Bending stiffness of entire bridge section - for deflection calculation
   // Crowns, slopes, and slab haunches are ignored.
   virtual Float64 GetBridgeEIxx(Float64 distFromStart) = 0;
   virtual Float64 GetBridgeEIyy(Float64 distFromStart) = 0;


   virtual Float64 GetSegmentWeightPerLength(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSegmentWeight(const CSegmentKey& segmentKey) = 0;

   virtual Float64 GetSegmentHeightAtPier(const CSegmentKey& segmentKey,PierIndexType pierIdx) = 0;
   virtual Float64 GetSegmentHeightAtTemporarySupport(const CSegmentKey& segmentKey,SupportIndexType tsIdx) = 0;
};

/*****************************************************************************
INTERFACE
   IShapes

DESCRIPTION
   The method on this interface return a geometric shape object for
   the gross (outline) shape of a bridge component. These shapes are ususally
   used for making images and graphical displays. Shapes are located
   in the Bridge Section Coordinate system.
*****************************************************************************/
// {B0BFEC24-7355-46d7-B552-5A177BB20EEE}
DEFINE_GUID(IID_IShapes, 
0xb0bfec24, 0x7355, 0x46d7, 0xb5, 0x52, 0x5a, 0x17, 0x7b, 0xb2, 0xe, 0xee);
interface IShapes : public IUnknown
{
   virtual void GetSegmentShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bOrient,pgsTypes::SectionCoordinateType coordinateType,IShape** ppShape) = 0;
   virtual void GetSlabShape(Float64 station,IShape** ppShape) = 0;
   virtual void GetLeftTrafficBarrierShape(Float64 station,IShape** ppShape) = 0;
   virtual void GetRightTrafficBarrierShape(Float64 station,IShape** ppShape) = 0;
};

/*****************************************************************************
INTERFACE
   IBarriers

   Interface for obtaining information barriers on the bridge

DESCRIPTION
   Interface for obtaining information barriers on the bridge
*****************************************************************************/
// {8EAC1B80-43B6-450b-B73C-4F55FC8681F6}
DEFINE_GUID(IID_IBarriers, 
0x8eac1b80, 0x43b6, 0x450b, 0xb7, 0x3c, 0x4f, 0x55, 0xfc, 0x86, 0x81, 0xf6);
interface IBarriers : public IUnknown
{
   // Traffic Barrier Properties
   virtual Float64 GetAtb(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetItb(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetYbtb(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetExteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) = 0;

   virtual bool HasInteriorBarrier(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetInteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetInteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) = 0;


   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(const CSegmentKey& segmentKey) = 0;

   // Distance from nearest edge of deck to edges of sidewalk for Dead load and Pedestrian load
   virtual void GetSidewalkDeadLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) = 0;
   virtual void GetSidewalkPedLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) = 0;

   virtual Float64 GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual bool HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation) = 0;
};

/*****************************************************************************
INTERFACE
   IGirder

   Interface for obtaining information about the girder

DESCRIPTION
   Interface for obtaining information about the girder
*****************************************************************************/
// {042428C6-03E8-4843-A9A5-09DCE1FC8CD6}
DEFINE_GUID(IID_IGirder, 
0x42428c6, 0x3e8, 0x4843, 0xa9, 0xa5, 0x9, 0xdc, 0xe1, 0xfc, 0x8c, 0xd6);
interface IGirder : IUnknown
{
   virtual bool    IsPrismatic(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey) = 0;
   virtual bool    IsSymmetric(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey) = 0; 
   virtual MatingSurfaceIndexType  GetNumberOfMatingSurfaces(const CGirderKey& girderKey) = 0;

   // Location of mating surface, measured from the CL girder. < 0 if left of CL.
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx) = 0;
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx) = 0;
   virtual FlangeIndexType GetNumberOfTopFlanges(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi) = 0; // sum of mating surface widths
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi) = 0;
   virtual FlangeIndexType GetNumberOfBottomFlanges(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi) = 0; // sum of mating surface widths
   virtual Float64 GetBottomWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMinWebWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMinTopFlangeThickness(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMinBottomFlangeThickness(const pgsPointOfInterest& poi) = 0;

   // Returns the height of the basic (non-composite) girder at the specified POI.
   // Interval isn't taken into account so you always get a value.
   virtual Float64 GetHeight(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetShearWidth(const pgsPointOfInterest& poi) = 0; // bv for vertical shear
   virtual Float64 GetShearInterfaceWidth(const pgsPointOfInterest& poi) = 0; // acv for horizontal shear

   virtual WebIndexType GetWebCount(const CGirderKey& girderKey) = 0;
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx) = 0;
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx) = 0;
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx) = 0;
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi) = 0; // horiz. distance from girder cl to cl of exterior web

   virtual void GetSegmentEndPoints(const CSegmentKey& segmentKey,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2) = 0;

   virtual Float64 GetOrientation(const CSegmentKey& segmentKey) = 0;

   virtual Float64 GetTopGirderReferenceChordElevation(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIdx) = 0;
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx) = 0;

   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi) = 0;
   virtual pgsTypes::SplittingDirection GetSplittingDirection(const CGirderKey& girderKey) = 0;

   virtual void GetProfileShape(const CSegmentKey& segmentKey,IShape** ppShape) = 0;

   // Area of shear key. uniform portion assumes no joint, section is per joint spacing.
   virtual bool HasShearKey(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType)=0;
   virtual void GetShearKeyAreas(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)=0;

   virtual void GetSegment(const CGirderKey& girderKey,Float64 distFromStartOfGirder,SegmentIndexType* pSegIdx,Float64* pDistFromStartOfSegment) = 0;
};

/*****************************************************************************
INTERFACE
   IUserDefinedLoads

   Interface to get processed User Defined Loads.

DESCRIPTION
   Interface to get processed User Defined Load information
*****************************************************************************/
// {B5BC8CBA-352D-4660-95AE-C8D43D5176A8}
DEFINE_GUID(IID_IUserDefinedLoads, 
0xb5bc8cba, 0x352d, 0x4660, 0x95, 0xae, 0xc8, 0xd4, 0x3d, 0x51, 0x76, 0xa8);
interface IUserDefinedLoads : IUnknown 
{
   enum UserDefinedLoadCase {userDC, userDW, userLL_IM};

   // structs to define loads. locations are in system units (no fractional)
   struct UserPointLoad
   {
      UserDefinedLoadCase m_LoadCase;
      Float64             m_Location; // from the left end of the firs segment in the girder
      Float64             m_Magnitude;
      std::_tstring       m_Description;
   };

   // distributed loads always in general trapezoidal form
   struct UserDistributedLoad  
   {
      UserDefinedLoadCase m_LoadCase;
      Float64             m_StartLocation; // from left support
      Float64             m_EndLocation; // from left support
      Float64             m_WStart;
      Float64             m_WEnd;
      std::_tstring       m_Description;
   };

   // moment and point loads are the same, except for the interpretation of Magnitude
   typedef UserPointLoad UserMomentLoad;

   virtual bool DoUserLoadsExist(const CSpanGirderKey& spanGirderKey) = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey) = 0;
   virtual const std::vector<UserPointLoad>* GetPointLoads(IntervalIndexType intervalIdx, const CSpanGirderKey& spanGirderKey)=0;
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(IntervalIndexType intervalIdx, const CSpanGirderKey& spanGirderKey)=0;
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(IntervalIndexType intervalIdx, const CSpanGirderKey& spanGirderKey)=0;
};


/*****************************************************************************
INTERFACE
   ITempSupport

   Interface for obtaining information about temporary supports

DESCRIPTION
   Interface for obtaining information about temporary supports
*****************************************************************************/
// {A6722C82-2796-4a4b-999E-E34C6F1A2FEE}
DEFINE_GUID(IID_ITempSupport, 
0xa6722c82, 0x2796, 0x4a4b, 0x99, 0x9e, 0xe3, 0x4c, 0x6f, 0x1a, 0x2f, 0xee);

interface ITempSupport : public IUnknown
{
   virtual void GetControlPoints(SupportIndexType tsIdx,IPoint2d** ppLeft,IPoint2d** ppAlignment_pt,IPoint2d** ppBridge_pt,IPoint2d** ppRight) = 0;
   virtual void GetDirection(SupportIndexType tsIdx,IDirection** ppDirection) = 0;
   virtual void GetSkew(SupportIndexType tsIdx,IAngle** ppAngle) = 0;
};

/*****************************************************************************
INTERFACE
   IGirderSegment

DESCRIPTION
   Interface for obtaining information about girder segments
*****************************************************************************/
// {7B03736C-E8AD-49b9-BF5C-D5F6E61B50D5}
DEFINE_GUID(IID_IGirderSegment, 
0x7b03736c, 0xe8ad, 0x49b9, 0xbf, 0x5c, 0xd5, 0xf6, 0xe6, 0x1b, 0x50, 0xd5);
interface IGirderSegment : public IUnknown
{
   // Returns the shape of the segment profile. If bIncludeClosure is true, the segment shape
   // includes its projection into the closure pour. Y=0 is at the top of the segment
   // X values are in girder path coordinates.
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IShape** ppShape) = 0;

   // Returns the shape of a the segment profile for a segment within the provided girder. If bIncludeClosure is
   // true, the segment shape includes its projection into the closure pour. Y=0 is at the top of the segment
   // X values are in girder path coordinates.
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,bool bIncludeClosure,IShape** ppShape) = 0;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,IPoint2dCollection** points) = 0;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,IPoint2dCollection** points) = 0;
   virtual void GetSegmentDirection(const CSegmentKey& segmentKey,IDirection** ppDirection) = 0;
   virtual Float64 GetTopWidth(const CSegmentKey& segmentKey) = 0;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,Float64* pStartEndDistance,Float64* pEndEndDistance) = 0;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64* pStartEndDistance,Float64* pEndEndDistance) = 0;
   virtual void GetSegmentBearingOffset(const CSegmentKey& segmentKey,Float64* pStartBearingOffset,Float64* pEndBearingOffset) = 0;
   virtual void GetSegmentStorageSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) = 0;
};

/*****************************************************************************
INTERFACE
   IClosurePour

DESCRIPTION
   Interface for obtaining information about closure pours
*****************************************************************************/
// {068E78DE-7F48-4bd6-A451-62D9A1EC634B}
DEFINE_GUID(IID_IClosurePour, 
0x68e78de, 0x7f48, 0x4bd6, 0xa4, 0x51, 0x62, 0xd9, 0xa1, 0xec, 0x63, 0x4b);
interface IClosurePour : public IUnknown
{
   // X values are in girder path coordinates.
   // Y = 0 is at the top of the closure pour
   virtual void GetClosurePourProfile(const CClosureKey& closureKey,IShape** ppShape) = 0;

   // Returns the length of the specified closure pour
   virtual Float64 GetClosurePourLength(const CClosureKey& closureKey) = 0;

   // Returns the left and right size of the closure pour.
   virtual void GetClosurePourSize(const CClosureKey& closureKey,Float64* pLeft,Float64* pRight) = 0;
};

/*****************************************************************************
INTERFACE
   ISplicedGirder

DESCRIPTION
   Interface for obtaining information about spliced girders
*****************************************************************************/
// {E8A643AD-4831-4735-9D88-21ED3E84D8F3}
DEFINE_GUID(IID_ISplicedGirder, 
0xe8a643ad, 0x4831, 0x4735, 0x9d, 0x88, 0x21, 0xed, 0x3e, 0x84, 0xd8, 0xf3);
interface ISplicedGirder : public IUnknown
{
   // returns the length of a spliced girder measured along the centerline of its segments between backs of pavement seats
   virtual Float64 GetSplicedGirderLayoutLength(const CGirderKey& girderKey) = 0;

   // returns the length of the spliced girder measured along the centerline of its segments between the CL-Bearing
   // at the end piers
   virtual Float64 GetSplicedGirderSpanLength(const CGirderKey& girderKey) = 0;

   // returns the end to end length of the spliced girder measured along the centerline of its segments
   virtual Float64 GetSplicedGirderLength(const CGirderKey& girderKey) = 0;
};

/*****************************************************************************
INTERFACE
   ITendonGeometry

DESCRIPTION
   Interface for obtaining information about the geometry of spliced girder
   ducts and tendons
*****************************************************************************/
// {209DA239-91BD-464c-895F-D1398A48125C}
DEFINE_GUID(IID_ITendonGeometry, 
0x209da239, 0x91bd, 0x464c, 0x89, 0x5f, 0xd1, 0x39, 0x8a, 0x48, 0x12, 0x5c);
interface ITendonGeometry : public IUnknown
{
   // returns the number of ducts in a girder
   virtual DuctIndexType GetDuctCount(const CGirderKey& girderKey) = 0;

   // returns the geometric centerline of a duct as a series of points. 
   // use this to plot ducts in the UI
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint2dCollection** ppPoints) = 0;

   // returns the geometric centerline of a duct for the duct configuration given in the girder object.
   // use this to plot ducts in the UI
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,const CSplicedGirderData* pSplicedGirder,IPoint2dCollection** ppPoints) = 0;

   // returns the location of the centerline of the duct in girder section coordiantes given a location in girder path coordinates
   virtual void GetDuctPoint(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IPoint2d** ppPoint) = 0;

   // returns the location of the centerline of the duct in girder section coordiantes given a POI
   virtual void GetDuctPoint(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IPoint2d** ppPoint) = 0;

   // returns number of strands in a duct
   virtual StrandIndexType GetStrandCount(const CGirderKey& girderKey,DuctIndexType ductIdx) = 0;

   // returns the area of the tendon
   virtual Float64 GetTendonArea(const CGirderKey& girderKey,IntervalIndexType intervalIdx,DuctIndexType ductIdx) = 0;

   // returns the jacking force
   virtual Float64 GetPjack(const CGirderKey& girderKey,DuctIndexType ductIdx) = 0;

   // returns the distance from the top of the non-composite girder to the CG of the tendon
   // adjustments are made for the tendon being shifted within the duct
   virtual Float64 GetDuctOffset(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) = 0;

   // returns the distance from the CG of the girder at the time the tendon in the specified duct
   // was stressed to the CG of the tendon
   // adjustments are made for the tendon being shifted within the duct
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) = 0;

   // returns the cummulative angular change of the tendon path between on end of the tendon and a poi
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) = 0;

   // returns the cummulative angular change of the tendon path between two POIs
   // if poi1 is to the left of poi2, it is assumed that jacking is from the left end otherwise it is from the right end
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,DuctIndexType ductIdx) = 0;
};

#endif // INCLUDED_IFACE_BRIDGE_H_

