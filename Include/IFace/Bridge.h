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

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\LongRebarInstance.h>

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
#define POIFIND_OR    1  // Find POIs that have at least one of the specified attributes
#define POIFIND_AND   2  // Find POIs that have all of the specified attributes

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
   virtual Float64 GetLength() = 0; // overall length of bridge
   virtual Float64 GetSpanLength(SpanIndexType span) = 0; // returns the pier to pier span length
   virtual Float64 GetAlignmentOffset() = 0;

   virtual SpanIndexType GetSpanCount() = 0;
   virtual GirderIndexType GetGirderCount(SpanIndexType span) = 0;
   virtual PierIndexType GetPierCount() = 0;

   virtual Float64 GetDistanceFromStartOfBridge(Float64 station) = 0;

   // Girder geometry
   virtual Float64 GetGirderLength(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetSpanLength(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetGirderPlanLength(SpanIndexType span,GirderIndexType gdr) = 0; // along grade
   virtual Float64 GetGirderSlope(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetCCPierLength(SpanIndexType span,GirderIndexType gdr) = 0; // C-C Pier length along girder line
   // Distance from end of girder to bearing - along girder
   virtual Float64 GetGirderStartConnectionLength(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetGirderEndConnectionLength(SpanIndexType span,GirderIndexType gdr) = 0;
   // Distance from C.L. pier to end of girder - along girder
   virtual Float64 GetGirderStartBearingOffset(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetGirderEndBearingOffset(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetGirderStartSupportWidth(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetGirderEndSupportWidth(SpanIndexType span,GirderIndexType gdr) = 0;

   virtual Float64 GetCLPierToCLBearingDistance(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType measure) = 0;
   virtual Float64 GetCLPierToGirderEndDistance(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType measure) = 0;

   virtual Float64 GetGirderOffset(SpanIndexType span,GirderIndexType gdr,Float64 station) = 0;

   virtual void GetPoint(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan,IPoint2d** ppPoint) = 0;
   virtual void GetPoint(const pgsPointOfInterest& poi,IPoint2d** ppPoint) = 0;
   virtual void GetStationAndOffset(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfBridge,Float64* pStation,Float64* pOffset) = 0;
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset) = 0;
   virtual void GetGirderBearing(SpanIndexType span,GirderIndexType gdr,IDirection** ppBearing) = 0;
   virtual void GetGirderAngle(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType face,IAngle** ppAngle) = 0;
   virtual void GetDistFromStartOfSpan(GirderIndexType gdrIdx,Float64 distFromStartOfBridge,SpanIndexType* pSpanIdx,Float64* pDistFromStartOfSpan) = 0;
   virtual bool IsInteriorGirder(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual bool IsExteriorGirder(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual bool AreGirderTopFlangesRoughened(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual bool GetSpan(Float64 station,SpanIndexType* pSpanIdx) = 0;

   // clear distance between girders. If poi is on an exterior girder, the left/right parameter will
   // be zero
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight) = 0;

   // used to get girder spacing for the bridge model section view
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(SpanIndexType spanIdx,Float64 distFromStartOfSpan) = 0;

   // returns girder spacing at a pier. The vector will contain nGirders-1 spaces
   virtual std::vector<Float64> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType) = 0;

   virtual void GetSpacingAlongGirder(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfGirder,Float64* leftSpacing,Float64* rightSpacing) = 0;

   virtual GDRCONFIG GetGirderConfiguration(SpanIndexType span,GirderIndexType gdr) = 0;

   // Diaphragms
   virtual void GetLeftSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH) = 0;
   virtual void GetRightSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH) = 0;
   // return true if weight of diaphragm is carried by girder
   virtual bool DoesLeftSideEndDiaphragmLoadGirder(PierIndexType pier) = 0;
   virtual bool DoesRightSideEndDiaphragmLoadGirder(PierIndexType pier) = 0;
   // Get location of end diaphragm load (c.g.) measured from c.l. pier. along girder
   // Only applicable if DoesEndDiaphragmLoadGirder returns true
   virtual Float64 GetEndDiaphragmLoadLocationAtStart(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetEndDiaphragmLoadLocationAtEnd(SpanIndexType span,GirderIndexType gdr)=0;
   virtual std::vector<IntermedateDiaphragm> GetIntermediateDiaphragms(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   // Slab data
   virtual pgsTypes::SupportedDeckType GetDeckType() = 0;
   virtual Float64 GetSlabOffset(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) = 0;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual bool IsCompositeDeck() = 0;
   virtual bool HasOverlay() = 0;
   virtual bool IsFutureOverlay() = 0;
   virtual Float64 GetOverlayWeight() = 0;
   virtual Float64 GetOverlayDepth() = 0;
   virtual Float64 GetSacrificalDepth() = 0;
   virtual Float64 GetFillet() = 0;
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetLeftSlabOverhang(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightSlabOverhang(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftSlabEdgeOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetRightSlabEdgeOffset(Float64 distFromStartOfBridge) = 0;
   virtual Float64 GetLeftSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetRightSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetLeftSlabGirderOverhang(SpanIndexType span,Float64 distFromStartOfSpan) = 0; // overhangs normal to alignment
   virtual Float64 GetRightSlabGirderOverhang(SpanIndexType span,Float64 distFromStartOfSpan) = 0;
   virtual Float64 GetLeftSlabOverhang(PierIndexType pier) = 0;
   virtual Float64 GetRightSlabOverhang(PierIndexType pier) = 0;
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pier) = 0;
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pier) = 0;
   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetCurbToCurbWidth(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan) = 0;
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
   virtual Float64 GetAheadBearingStation(PierIndexType pier,GirderIndexType gdr) = 0;
   virtual Float64 GetBackBearingStation(PierIndexType pier,GirderIndexType gdr) = 0;
   virtual void GetPierDirection(PierIndexType pier,IDirection** ppDirection) = 0;
   virtual void GetPierSkew(PierIndexType pier,IAngle** ppAngle) = 0;
   virtual void GetPierPoints(PierIndexType pier,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right) = 0;
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) = 0;
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) = 0;
   virtual void GetContinuityStage(PierIndexType pierIdx,pgsTypes::Stage* pLeft,pgsTypes::Stage* pRight) = 0;

   // returns the skew angle of a line define defined by the orientation string at a given station
   // this is usefuly for determing the skew angle of piers that aren't in the bridge model yet
   // returns false if there is an error in the strOrientation string
   virtual bool GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew) = 0;

   // negative moment calculations and results need not be processed if a simple span analysis is
   // used or if there isn't any continuity.
   // this method returns true when negative moments should be processed for a given span.
   // Use ALL_SPANS for to evaluate all spans
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx) = 0;
};

/*****************************************************************************
INTERFACE
   IBridgeMaterial

   Interface to get information about the bridge materials.

DESCRIPTION
   Interface to get information about the bridge materials.
*****************************************************************************/
// {AD20C774-677E-11d2-883A-006097C68A9C}
DEFINE_GUID(IID_IBridgeMaterial, 
0xad20c774, 0x677e, 0x11d2, 0x88, 0x3a, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IBridgeMaterial : IUnknown
{
   // Girder Concrete
   virtual Float64 GetEcGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetFcGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetStrDensityGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetWgtDensityGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetMaxAggrSizeGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetFlexureFrGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetShearFrGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetK1Gdr(SpanIndexType span,GirderIndexType gdr) = 0; // obsolete

   // Girder Concrete at release
   virtual Float64 GetEciGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetFciGdr(SpanIndexType span, GirderIndexType gdr) = 0;
   virtual Float64 GetFriGdr(SpanIndexType span, GirderIndexType gdr) = 0;

   // Slab Concrete
   virtual Float64 GetEcSlab() = 0;
   virtual Float64 GetFcSlab() = 0;
   virtual Float64 GetStrDensitySlab() = 0;
   virtual Float64 GetWgtDensitySlab() = 0;
   virtual Float64 GetMaxAggrSizeSlab() = 0;
   virtual Float64 GetFlexureFrSlab() = 0;
   virtual Float64 GetShearFrSlab() = 0;
   virtual Float64 GetK1Slab() = 0; // obsolete

   // Traffic Barrier and Sidewalk Density
   virtual Float64 GetDensityRailing(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual Float64 GetEcRailing(pgsTypes::TrafficBarrierOrientation orientation) = 0;

   // Prestressing Strand
   virtual const matPsStrand* GetStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType) = 0;

   // Properties of Girder Longitudinal Rebar
   virtual void GetLongitudinalRebarProperties(SpanIndexType span,GirderIndexType gdr,Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetLongitudinalRebarName(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual void GetLongitudinalRebarMaterial(SpanIndexType spanIdx,GirderIndexType gdrIdx,matRebar::Type& type,matRebar::Grade& grade) = 0;

   // Properties of Girder Transverse Rebar
   virtual void GetTransverseRebarProperties(SpanIndexType span,GirderIndexType gdr,Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetTransverseRebarName(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual void GetTransverseRebarMaterial(SpanIndexType spanIdx,GirderIndexType gdrIdx,matRebar::Type& type,matRebar::Grade& grade) = 0;

   // Rebar properties for design
   virtual void GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu) = 0;
   virtual std::_tstring GetDeckRebarName() = 0;
   virtual void GetDeckRebarMaterial(matRebar::Type& type,matRebar::Grade& grade) = 0;

   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1) = 0; /// obsolete

   virtual Float64 GetFlexureModRupture(Float64 fc) = 0; // obsolete
   virtual Float64 GetShearModRupture(Float64 fc) = 0; // obsolete
   virtual Float64 GetFlexureFrCoefficient() = 0; // obsolete
   virtual Float64 GetShearFrCoefficient() = 0; // obsolete

   virtual Float64 GetNWCDensityLimit() = 0; // returns the minimum density for normal weight concrete
};

// {E7DBBC86-9DD7-41e1-8D78-D28FD8316C5D}
DEFINE_GUID(IID_IBridgeMaterialEx, 
0xe7dbbc86, 0x9dd7, 0x41e1, 0x8d, 0x78, 0xd2, 0x8f, 0xd8, 0x31, 0x6c, 0x5d);
interface IBridgeMaterialEx : IBridgeMaterial
{
   virtual Float64 GetLWCDensityLimit() = 0; // returns the maximum density for lightweight concrete
   virtual pgsTypes::ConcreteType GetGdrConcreteType(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual bool DoesGdrConcreteHaveAggSplittingStrength(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetGdrConcreteAggSplittingStrength(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual pgsTypes::ConcreteType GetSlabConcreteType() = 0;
   virtual bool DoesSlabConcreteHaveAggSplittingStrength() = 0;
   virtual Float64 GetSlabConcreteAggSplittingStrength() = 0;
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) = 0;
   virtual Float64 GetFlexureFrCoefficient(pgsTypes::ConcreteType type) = 0;
   virtual Float64 GetFlexureFrCoefficient(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) = 0;
   virtual Float64 GetShearFrCoefficient(pgsTypes::ConcreteType type) = 0;
   virtual Float64 GetShearFrCoefficient(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   virtual Float64 GetEccK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetEccK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetCreepK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetCreepK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetShrinkageK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetShrinkageK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   virtual Float64 GetEccK1Slab() = 0;
   virtual Float64 GetEccK2Slab() = 0;
   virtual Float64 GetCreepK1Slab() = 0;
   virtual Float64 GetCreepK2Slab() = 0;
   virtual Float64 GetShrinkageK1Slab() = 0;
   virtual Float64 GetShrinkageK2Slab() = 0;

   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2) = 0;
};

/*****************************************************************************
INTERFACE
   IStageMap

   Interface for managing stages.

DESCRIPTION
   Interface for managing stages. Provides information about stage names
   and sequences

*****************************************************************************/
// {B1728315-80C4-4174-A587-CE5EF80C5E15}
DEFINE_GUID(IID_IStageMap, 
0xb1728315, 0x80c4, 0x4174, 0xa5, 0x87, 0xce, 0x5e, 0xf8, 0xc, 0x5e, 0x15);
interface IStageMap : IUnknown
{
   virtual CComBSTR GetStageName(pgsTypes::Stage stage) = 0;  
   virtual pgsTypes::Stage GetStageType(CComBSTR bstrStage) = 0;
   virtual CComBSTR GetLimitStateName(pgsTypes::LimitState ls) = 0;  
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
   virtual Float64 GetDevLengthFactor(SpanIndexType span,GirderIndexType gdr,IRebarSectionItem* rebarItem) = 0;
   virtual Float64 GetDevLengthFactor(IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) = 0;
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi) = 0;

   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   virtual Float64 GetCoverTopMat() = 0;
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,DeckRebarType drt) = 0;

   virtual Float64 GetCoverBottomMat() = 0;
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,DeckRebarType drt) = 0;

   virtual void GetRebarLayout(SpanIndexType span,GirderIndexType gdr, IRebarLayout** rebarLayout) = 0;

   virtual REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) = 0;
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
   virtual bool AreStirrupZonesSymmetrical(SpanIndexType span,GirderIndexType gdr)=0;

   // zone get is zero-based. 
   // zones are sorted left->right across entire girder
   // zones may, or may not, be symmetric about mid-girder
   virtual ZoneIndexType GetNumPrimaryZones(SpanIndexType span,GirderIndexType gdr)=0;
   virtual void GetPrimaryZoneBounds(SpanIndexType span,GirderIndexType gdr, ZoneIndexType zone, Float64* start, Float64* end)=0;
   virtual void GetPrimaryVertStirrupBarInfo(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing) = 0;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone)=0;
   virtual matRebar::Size GetPrimaryConfinementBarSize(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone) = 0;

   // Horizontal Interface additional bar zones
   // zone get is zero-based. 
   // zones are sorted left->right 
   // zones may, or may not, be symmetric about mid-girder
   virtual ZoneIndexType GetNumHorizInterfaceZones(SpanIndexType span,GirderIndexType gdr)=0;
   virtual void GetHorizInterfaceZoneBounds(SpanIndexType span,GirderIndexType gdr, ZoneIndexType zone, Float64* start, Float64* end)=0;
   virtual void GetHorizInterfaceBarInfo(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing)=0;

   // Additional splitting and confinement bars
   virtual void GetAddSplittingBarInfo(SpanIndexType span,GirderIndexType gdr, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing) = 0;
   virtual void GetAddConfinementBarInfo(SpanIndexType span,GirderIndexType gdr, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing) = 0;

   // Calculated bar values at poi's
   // Vertical shear
   virtual Float64 GetVertStirrupAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) = 0;
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi) = 0; // stirrup angle=90 for vertical

   // Horizontal interface shear
   virtual bool DoStirrupsEngageDeck(SpanIndexType span,GirderIndexType gdr)=0;
   virtual bool DoAllPrimaryStirrupsEngageDeck(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetPrimaryHorizInterfaceS(const pgsPointOfInterest& poi)=0;
   virtual Float64 GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)=0;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi)=0;
   virtual Float64 GetAdditionalHorizInterfaceS(const pgsPointOfInterest& poi)=0;
   virtual Float64 GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)=0;
   virtual Float64 GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi)=0;

   // Total area of splitting shear steel between two points along the girder
   virtual Float64 GetSplittingAv(SpanIndexType span,GirderIndexType gdr,Float64 start,Float64 end) = 0;

   // Processed confinement bar information - returns max bar size/min spacing in required zone length at both ends of girder
   virtual void GetStartConfinementBarInfo(SpanIndexType span,GirderIndexType gdr, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) = 0;
   virtual void GetEndConfinementBarInfo(  SpanIndexType span,GirderIndexType gdr, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) = 0;

   // Returns true if the stirrup layout geometry is ok
   virtual bool AreStirrupZoneLengthsCombatible(SpanIndexType span,GirderIndexType gdr) = 0;
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
   enum GirderEnd { geLeftEnd, geRightEnd};

   virtual Float64 GetEccentricity(const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetEccentricity(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetHsEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetSsEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetTempEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi) = 0;

   virtual Float64 GetEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetHsEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetSsEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetTempEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, Float64* nEffectiveStrands) = 0;
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig) = 0;
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig) = 0;

   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig,DevelopmentAdjustmentType devAdjust) = 0; // Fig. 5.8.3.4.2-3

   // Functions to compute ordered strand filling for total permanent strands fill order
   virtual StrandIndexType GetMaxNumPermanentStrands(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual StrandIndexType GetMaxNumPermanentStrands(LPCTSTR strGirderName) = 0;
   // get ratio of harped/straight strands if total permanent strands is used for input. returns false if total doesn't fit
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,SpanIndexType span,GirderIndexType gdr, StrandIndexType* numStraight, StrandIndexType* numHarped) =0;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped)=0;
   // get next and previous number of strands - return INVALID_INDEX if at end
   virtual StrandIndexType GetNextNumPermanentStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)=0;
   virtual StrandIndexType GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum)=0;
   virtual StrandIndexType GetPreviousNumPermanentStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)=0;
   virtual StrandIndexType GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum)=0;
   // Compute strand Indices as in girder library for given filled strands
   virtual bool ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices)=0;

   // Functions to compute ordered strand filling for straight/harped/temporary fill orders
   virtual bool IsValidNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual bool IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetNextNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetPrevNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum) = 0;
   virtual StrandIndexType GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) = 0;

   // Function to compute a strand fill array using grid index (as used in in PRESTRESSCONFIG) from an ordered fill of Ns strands
   virtual ConfigStrandFillVector ComputeStrandFill(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType Ns) = 0;
   virtual ConfigStrandFillVector ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns) = 0;

   // Conversions from/to sequential fill index to grid fill index (as used in in PRESTRESSCONFIG). A single grid entry can have two strands
   virtual GridIndexType SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo) = 0;
   virtual void GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) = 0;

   virtual StrandIndexType GetNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type) = 0;
   virtual StrandIndexType GetMaxStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type) = 0;
   virtual StrandIndexType GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type) = 0;

   virtual Float64 GetStrandArea(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type) = 0;
   virtual Float64 GetAreaPrestressStrands(SpanIndexType span,GirderIndexType gdr,bool bIncTemp) = 0;

   virtual Float64 GetPjack(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type) = 0;
   virtual Float64 GetPjack(SpanIndexType span,GirderIndexType gdr,bool bIncTemp) = 0;

   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) = 0;
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) = 0; 
   virtual void GetStrandPositionsEx(LPCTSTR strGirderName, const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, pgsTypes::MemberEndType endType, IPoint2dCollection** ppPoints) = 0;

   // Harped strands can be forced to be straight along their length
   virtual bool GetAreHarpedStrandsForcedStraight(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual bool GetAreHarpedStrandsForcedStraightEx(LPCTSTR strGirderName) = 0;

   // Harped vertical offsets are measured from original strand locations in strand grid defined in the library
   virtual Float64 GetGirderTopElevation(SpanIndexType span,GirderIndexType gdr) = 0;  // highest point on girder section based on strand coordinates (bottom at 0.0)
   virtual void GetHarpStrandOffsets(SpanIndexType span,GirderIndexType gdr,Float64* pOffsetEnd,Float64* pOffsetHp) = 0;
   virtual void GetHarpedEndOffsetBounds(SpanIndexType span,GirderIndexType gdr,Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedEndOffsetBoundsEx(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) = 0;
   virtual void GetHarpedHpOffsetBounds(SpanIndexType span,GirderIndexType gdr,Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedHpOffsetBoundsEx(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)=0;
   virtual void GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)=0;

   virtual Float64 GetHarpedEndOffsetIncrement(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetHarpedHpOffsetIncrement(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetHarpedEndOffsetIncrement(LPCTSTR strGirderName) = 0;
   virtual Float64 GetHarpedHpOffsetIncrement(LPCTSTR strGirderName) = 0;

   virtual void GetHarpingPointLocations(SpanIndexType span,GirderIndexType gdr,Float64* lhp,Float64* rhp) = 0;
   virtual void GetHighestHarpedStrandLocation(SpanIndexType span,GirderIndexType gdr,Float64* pElevation) = 0;

   virtual Uint16 GetNumHarpPoints(SpanIndexType span,GirderIndexType gdr) = 0;

   virtual StrandIndexType GetNumExtendedStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::MemberEndType endType,pgsTypes::StrandType standType) = 0;
   virtual bool IsExtendedStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual bool IsExtendedStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;

   // Debonding of individual strands. StrandIndex used in these functions is 
   // same order as those returned by GetStrandPositions
   virtual bool IsStrandDebonded(SpanIndexType span,GirderIndexType gdr,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64* pStart,Float64* pEnd) = 0;
   virtual bool IsStrandDebonded(SpanIndexType span,GirderIndexType gdr,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const PRESTRESSCONFIG& config,Float64* pStart,Float64* pEnd) = 0;
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual StrandIndexType GetNumDebondedStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType) = 0;
   virtual RowIndexType GetNumRowsWithStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType ) = 0;
   virtual StrandIndexType GetNumStrandInRow(SpanIndexType span,GirderIndexType gdr,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual std::vector<StrandIndexType> GetStrandsInRow(SpanIndexType span,GirderIndexType gdr, RowIndexType rowIdx, pgsTypes::StrandType strandType ) = 0;
   virtual StrandIndexType GetNumDebondedStrandsInRow(SpanIndexType span,GirderIndexType gdr,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual bool IsExteriorStrandDebondedInRow(SpanIndexType span,GirderIndexType gdr,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual bool IsDebondingSymmetric(SpanIndexType span,GirderIndexType gdr) = 0;

   // These functions return the data for the number of strands given (used during design)
   // StrandIndex used in these functions is same order as those retured by GetStrandPositions
   virtual RowIndexType GetNumRowsWithStrand(SpanIndexType span,GirderIndexType gdr, const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strandType ) = 0;
   virtual StrandIndexType GetNumStrandInRow(SpanIndexType span,GirderIndexType gdr,const PRESTRESSCONFIG& rconfig,RowIndexType rowIdx,pgsTypes::StrandType strandType ) = 0;
   virtual std::vector<StrandIndexType> GetStrandsInRow(SpanIndexType span,GirderIndexType gdr,const PRESTRESSCONFIG& rconfig,RowIndexType rowIdx, pgsTypes::StrandType strandType ) = 0;

   // Section locations measured from left end to right
   virtual Float64 GetDebondSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;
   virtual SectionIndexType GetNumDebondSections(SpanIndexType span,GirderIndexType gdr,GirderEnd end,pgsTypes::StrandType strandType) = 0;
   virtual StrandIndexType GetNumDebondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;
   virtual StrandIndexType GetNumBondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;
   virtual std::vector<StrandIndexType> GetDebondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) = 0;

   virtual bool CanDebondStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)=0; // can debond any of the strands?
   virtual bool CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType)=0; // can debond any of the strands?
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list)=0; 
   virtual void ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) = 0; 
   virtual Float64 GetDefaultDebondLength(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   // Functions to compute harped strand offsets based on available measurement types
   // Absolute offset is distance that raw strand grid locations are to be moved.
   // rHarpedFillArray is same as StrandFill in PRESTRESSCONFIG
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) = 0;
   virtual Float64 ConvertHarpedOffsetEnd(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) = 0;
   virtual Float64 ConvertHarpedOffsetEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) = 0;
   virtual Float64 ConvertHarpedOffsetHp(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) = 0;
   virtual Float64 ConvertHarpedOffsetHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) = 0;
};

/*****************************************************************************
INTERFACE
   IPointOfInterest

   Interface for getting the points of interest.

DESCRIPTION
   Interface for getting the points of interest.
*****************************************************************************/
// {1CF877CC-6856-11d2-883B-006097C68A9C}
DEFINE_GUID(IID_IPointOfInterest, 
0x1cf877cc, 0x6856, 0x11d2, 0x88, 0x3b, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IPointOfInterest : IUnknown
{
   // Returns all points of interest for a span/girder
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr) = 0;

   // Returns all points of interest for a span/girder that include the attrib attributes in the specified stage
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;

   // Returns all points of interest for a span/girder that include the attrib attributes in all of the the specified stages
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;

   // Returns all of the 10th point points of interest for a span/girder
   virtual std::vector<pgsPointOfInterest> GetTenthPointPOIs(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr) = 0;

   // Returns critical section for shear points of interest for the span/girder
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,pgsPointOfInterest* pLeft,pgsPointOfInterest* pRight) = 0;

   // Returns critical section for shear points of interest for the span/girder based on the supplied girder configuration
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,pgsPointOfInterest* pLeft,pgsPointOfInterest* pRight) = 0;

   // Returns the dist from the first pier for the given point of interest
   virtual Float64 GetDistanceFromFirstPier(const pgsPointOfInterest& poi,pgsTypes::Stage stage) = 0;

   virtual pgsPointOfInterest GetPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) = 0;
   virtual pgsPointOfInterest GetPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) = 0;
   virtual pgsPointOfInterest GetNearestPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) = 0;
   virtual pgsPointOfInterest GetNearestPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) = 0;
};

/*****************************************************************************
INTERFACE
   ISectProp2

   Interface for obtaining section properties.

DESCRIPTION
   Interface for obtaining section properties.
****************************************************************************/
// {28D53414-E8FD-4b53-A9B7-B395EB1E11E7}
DEFINE_GUID(IID_ISectProp2, 
0x28d53414, 0xe8fd, 0x4b53, 0xa9, 0xb7, 0xb3, 0x95, 0xeb, 0x1e, 0x11, 0xe7);
interface ISectProp2 : IUnknown
{
   virtual Float64 GetHg(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetAg(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetIy(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYt(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYb(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSt(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSb(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetYtGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetStGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetKt(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetKb(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetEIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;

   virtual Float64 GetAg(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetIy(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetYt(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetYb(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetSt(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetSb(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetYtGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;
   virtual Float64 GetStGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr) = 0;

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
   virtual void ReportEffectiveFlangeWidth(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   // Volume and surface area
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSurfaceArea(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetVolume(SpanIndexType span,GirderIndexType gdr) = 0;

   // Bending stiffness of entire bridge section - for deflection calculation
   // Crowns, slopes, and slab haunches are ignored.
   virtual Float64 GetBridgeEIxx(Float64 distFromStart) = 0;
   virtual Float64 GetBridgeEIyy(Float64 distFromStart) = 0;

   virtual void GetGirderShape(const pgsPointOfInterest& poi,pgsTypes::Stage stage,bool bOrient,IShape** ppShape) = 0;
   virtual void GetSlabShape(Float64 station,IShape** ppShape) = 0;
   virtual void GetLeftTrafficBarrierShape(Float64 station,IShape** ppShape) = 0;
   virtual void GetRightTrafficBarrierShape(Float64 station,IShape** ppShape) = 0;

   virtual Float64 GetGirderWeightPerLength(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetGirderWeight(SpanIndexType span,GirderIndexType gdr) = 0;
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

   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(SpanIndexType span,GirderIndexType gdr) = 0;

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
   virtual bool    IsPrismatic(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr) = 0;
   virtual bool    IsSymmetric(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr) = 0; 
   virtual MatingSurfaceIndexType  GetNumberOfMatingSurfaces(SpanIndexType span,GirderIndexType gdr) = 0;

   // Location of mating surface, measured from the CL girder. < 0 if left of CL.
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx) = 0;
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx) = 0;
   virtual FlangeIndexType GetNumberOfTopFlanges(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi) = 0; // sum of mating surface widths
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi) = 0;
   virtual FlangeIndexType GetNumberOfBottomFlanges(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) = 0;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi) = 0; // sum of mating surface widths
   virtual Float64 GetBottomWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMinWebWidth(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMinTopFlangeThickness(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMinBottomFlangeThickness(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetHeight(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetShearWidth(const pgsPointOfInterest& poi) = 0; // bv for vertical shear
   virtual Float64 GetShearInterfaceWidth(const pgsPointOfInterest& poi) = 0; // acv for horizontal shear

   virtual WebIndexType GetNumberOfWebs(SpanIndexType span,GirderIndexType gdr) = 0;
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx) = 0;
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx) = 0;
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx) = 0;
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi) = 0; // horiz. distance from girder cl to cl of exterior web

   virtual void GetGirderEndPoints(SpanIndexType span,GirderIndexType gdr,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2) = 0;

   virtual Float64 GetOrientation(SpanIndexType span,GirderIndexType gdr) = 0;

   virtual Float64 GetTopGirderReferenceChordElevation(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIdx) = 0;
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx) = 0;

   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi) = 0;
   virtual pgsTypes::SplittingDirection GetSplittingDirection(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   virtual void GetProfileShape(SpanIndexType spanIdx,GirderIndexType gdrIdx,IShape** ppShape) = 0;

   // Area of shear key. uniform portion assumes no joint, section is per joint spacing.
   virtual bool HasShearKey(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::SupportedBeamSpacing spacingType)=0;
   virtual void GetShearKeyAreas(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)=0;
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
      Float64             m_Location; // from left support
      Float64             m_Magnitude;
      std::_tstring         m_Description;
   };

   // distributed loads always in general trapezoidal form
   struct UserDistributedLoad  
   {
      UserDefinedLoadCase m_LoadCase;
      Float64             m_StartLocation; // from left support
      Float64             m_EndLocation; // from left support
      Float64             m_WStart;
      Float64             m_WEnd;
      std::_tstring         m_Description;
   };

   // moment and point loads are the same, except for the interpretation of Magnitude
   typedef UserPointLoad UserMomentLoad;

   virtual bool DoUserLoadsExist(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual const std::vector<UserPointLoad>* GetPointLoads(pgsTypes::Stage stage, SpanIndexType span, GirderIndexType gdr)=0;
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(pgsTypes::Stage stage, SpanIndexType span, GirderIndexType gdr)=0;
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(pgsTypes::Stage stage, SpanIndexType span, GirderIndexType gdr)=0;
};


#endif // INCLUDED_IFACE_BRIDGE_H_

