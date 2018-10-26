///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// BridgeAgentImp.h : Declaration of the CBridgeAgentImp

#ifndef __BRIDGEAGENT_H_
#define __BRIDGEAGENT_H_

#include "resource.h"       // main symbols
#include <IFace\Project.h> // For EventSink
#include <IFace\Alignment.h>
#include <IFace\Intervals.h>

#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\PoiMgr.h>
#include <PgsExt\PoiKey.h>
#include <PgsExt\DeckRebarData.h>
#include <PgsExt\RailingSystem.h>
#include <PgsExt\ConcreteMaterial.h>
#include <PgsExt\PTData.h>

#include "EffectiveFlangeWidthTool.h"
#include "ContinuousStandFiller.h"

#include <memory>

#include <Math\Polynomial2d.h>
#include <Math\CompositeFunction2d.h>
#include <Math\LinFunc2d.h>

#include "StatusItems.h"
#include "ConcreteManager.h"
#include "IntervalManager.h"

#if defined _USE_MULTITHREADING
#include <PgsExt\ThreadManager.h>
#endif


class gmTrafficBarrier;
class matPsStrand;

/////////////////////////////////////////////////////////////////////////////
// CBridgeAgentImp
class ATL_NO_VTABLE CBridgeAgentImp : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CBridgeAgentImp, &CLSID_BridgeAgent>,
   public IConnectionPointContainerImpl<CBridgeAgentImp>,
   public IAgentEx,
   public IRoadway,
   public IGeometry,
   public IBridge,
   public IMaterials,
   public IStrandGeometry,
   public ILongRebarGeometry,
   public IStirrupGeometry,
   public IPointOfInterest,
   public ISectionProperties,
   public IShapes,
   public IBarriers,
   public ISegmentLiftingPointsOfInterest,
   public ISegmentHaulingPointsOfInterest,
   public IBridgeDescriptionEventSink,
   public ISpecificationEventSink,
   public ILossParametersEventSink,
   public IUserDefinedLoads,
   public ITempSupport,
   public IGirder,
   public ITendonGeometry,
   public IIntervals
{
   friend class CStrandMoverSwapper;
public:
   CBridgeAgentImp() :
      m_LOTFSectionPropertiesKey(CSegmentKey(INVALID_INDEX,INVALID_INDEX,INVALID_INDEX),0.0,INVALID_INDEX)
	{
      m_Level        = 0;
      m_pBroker      = 0;

      m_bUserLoadsValidated  = false;
      m_bDeckParametersValidated = false;

      m_DeltaX = 0;
      m_DeltaY = 0;
	}

   HRESULT FinalConstruct();
   void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_BRIDGEAGENT)

BEGIN_COM_MAP(CBridgeAgentImp)
   COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(IRoadway)
   COM_INTERFACE_ENTRY(IGeometry)
   COM_INTERFACE_ENTRY(IBridge)
   COM_INTERFACE_ENTRY(IMaterials)
   COM_INTERFACE_ENTRY(IStrandGeometry)
   COM_INTERFACE_ENTRY(ILongRebarGeometry)
   COM_INTERFACE_ENTRY(IStirrupGeometry)
   COM_INTERFACE_ENTRY(IPointOfInterest)
   COM_INTERFACE_ENTRY(ISectionProperties)
   COM_INTERFACE_ENTRY(IShapes)
   COM_INTERFACE_ENTRY(IBarriers)
   COM_INTERFACE_ENTRY(ISegmentLiftingPointsOfInterest)
   COM_INTERFACE_ENTRY(ISegmentHaulingPointsOfInterest)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
   COM_INTERFACE_ENTRY(ILossParametersEventSink)
   COM_INTERFACE_ENTRY(IUserDefinedLoads)
   COM_INTERFACE_ENTRY(ITempSupport)
   COM_INTERFACE_ENTRY(IGirder)
   COM_INTERFACE_ENTRY(ITendonGeometry)
   COM_INTERFACE_ENTRY(IIntervals)
   COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CBridgeAgentImp)
END_CONNECTION_POINT_MAP()


#if defined _USE_MULTITHREADING
   CThreadManager m_ThreadManager;
#endif

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidInformationalError;
   StatusCallbackIDType m_scidInformationalWarning;
   StatusCallbackIDType m_scidBridgeDescriptionError;
   StatusCallbackIDType m_scidBridgeDescriptionWarning;
   StatusCallbackIDType m_scidAlignmentWarning;
   StatusCallbackIDType m_scidAlignmentError;
   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidGirderDescriptionInform;
   StatusCallbackIDType m_scidGirderDescriptionError;
   StatusCallbackIDType m_scidPointLoadWarning;
   StatusCallbackIDType m_scidDistributedLoadWarning;
   StatusCallbackIDType m_scidMomentLoadWarning;
   StatusCallbackIDType m_scidZeroOverlayWarning;

// IAgent
public:
   STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker) override;
	STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// IRoadway
public:
   virtual void GetStartPoint(Float64 n,Float64* pStartStation,Float64* pStartElevation,Float64* pGrade,IPoint2d** ppPoint) override;
   virtual void GetEndPoint(Float64 n,Float64* pEndStation,Float64* pEndlevation,Float64* pGrade,IPoint2d** ppPoint) override;
   virtual Float64 GetSlope(Float64 station,Float64 offset) override;
   virtual Float64 GetProfileGrade(Float64 station) override;
   virtual Float64 GetElevation(Float64 station,Float64 offset) override;
   virtual void GetBearing(Float64 station,IDirection** ppBearing) override;
   virtual void GetBearingNormal(Float64 station,IDirection** ppNormal) override;
   virtual void GetPoint(Float64 station,Float64 offset,IDirection* pBearing,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) override;
   virtual void GetStationAndOffset(pgsTypes::PlanCoordinateType pcType,IPoint2d* point,Float64* pStation,Float64* pOffset) override;
   virtual CollectionIndexType GetCurveCount() override;
   virtual void GetCurve(CollectionIndexType idx,IHorzCurve** ppCurve) override;
   virtual void GetCurvePoint(IndexType hcIdx,CurvePointTypes cpType,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) override;
   virtual HCURVESTATIONS GetCurveStations(IndexType hcIdx) override;
   virtual CollectionIndexType GetVertCurveCount() override;
   virtual void GetVertCurve(CollectionIndexType idx,IVertCurve** ppCurve) override;
   virtual void GetRoadwaySurface(Float64 station,IDirection* pDirection,IPoint2dCollection** ppPoints) override;
   virtual Float64 GetCrownPointOffset(Float64 station) override;

// IGeometry
public:
   virtual HRESULT Angle(IPoint2d* from,IPoint2d* vertex,IPoint2d* to,IAngle** angle) override;
   virtual HRESULT Area(IPoint2dCollection* points,Float64* area) override;
   virtual HRESULT Distance(IPoint2d* from,IPoint2d* to,Float64* dist) override;
   virtual HRESULT Direction(IPoint2d* from,IPoint2d* to,IDirection** dir) override;
   virtual HRESULT Inverse(IPoint2d* from,IPoint2d* to,Float64* dist,IDirection** dir) override;
   virtual HRESULT ByDistAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varAngle,Float64 offset,IPoint2d** point) override;
   virtual HRESULT ByDistDefAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varDefAngle,Float64 offset,IPoint2d** point) override;
   virtual HRESULT ByDistDir(IPoint2d* from,Float64 dist,VARIANT varDir,Float64 offset,IPoint2d** point) override;
   virtual HRESULT PointOnLine(IPoint2d* from,IPoint2d* to,Float64 dist,Float64 offset,IPoint2d** point) override;
   virtual HRESULT ParallelLineByPoints(IPoint2d* from,IPoint2d* to,Float64 offset,IPoint2d** p1,IPoint2d** p2) override;
   virtual HRESULT ParallelLineSegment(ILineSegment2d* ls,Float64 offset,ILineSegment2d** linesegment) override;
   virtual HRESULT Bearings(IPoint2d* p1,VARIANT varDir1,Float64 offset1,IPoint2d* p2,VARIANT varDir2,Float64 offset2,IPoint2d** point) override;
   virtual HRESULT BearingCircle(IPoint2d* p1,VARIANT varDir,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point) override;
   virtual HRESULT Circles(IPoint2d* p1,Float64 r1,IPoint2d* p2,Float64 r2,IPoint2d* nearest,IPoint2d** point) override;
   virtual HRESULT LineByPointsCircle(IPoint2d* p1,IPoint2d* p2,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point) override;
   virtual HRESULT LinesByPoints(IPoint2d* p11,IPoint2d* p12,Float64 offset1,IPoint2d* p21,IPoint2d* p22,Float64 offset2,IPoint2d** point) override;
   virtual HRESULT Lines(ILineSegment2d* l1,Float64 offset1,ILineSegment2d* l2,Float64 offset2,IPoint2d** point) override;
   virtual HRESULT LineSegmentCircle(ILineSegment2d* pSeg,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest, IPoint2d** point) override;
   virtual HRESULT PointOnLineByPoints(IPoint2d* pnt,IPoint2d* start,IPoint2d* end,Float64 offset,IPoint2d** point) override;
   virtual HRESULT PointOnLineSegment(IPoint2d* from,ILineSegment2d* seg,Float64 offset,IPoint2d** point) override;
   virtual HRESULT PointOnCurve(IPoint2d* pnt,IHorzCurve* curve,IPoint2d** point) override;
   virtual HRESULT Arc(IPoint2d* from, IPoint2d* vertex, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points) override;
   virtual HRESULT BetweenPoints(IPoint2d* from, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points) override;
   virtual HRESULT LineSegment(ILineSegment2d* seg,CollectionIndexType nParts,IPoint2dCollection** points) override;
	virtual HRESULT HorzCurve(IHorzCurve* curve, CollectionIndexType nParts, IPoint2dCollection** points) override;
   virtual HRESULT Path(IPath* pPath,CollectionIndexType nParts,Float64 start,Float64 end,IPoint2dCollection** points) override;
   virtual HRESULT External(IPoint2d* center1, Float64 radius1,IPoint2d* center2,Float64 radius2,TangentSignType sign, IPoint2d** t1,IPoint2d** t2) override;
   virtual HRESULT Cross(IPoint2d* center1, Float64 radius1,IPoint2d* center2, Float64 radius2, TangentSignType sign, IPoint2d** t1,IPoint2d** t2) override;
   virtual HRESULT Point(IPoint2d* center, Float64 radius,IPoint2d* point, TangentSignType sign, IPoint2d** tangent) override;

// IBridge
public:
   virtual Float64 GetLength() override;
   virtual Float64 GetSpanLength(SpanIndexType span) override;
   virtual Float64 GetGirderLayoutLength(const CGirderKey& girderKey) override;
   virtual Float64 GetGirderSpanLength(const CGirderKey& girderKey) override;
   virtual Float64 GetGirderLength(const CGirderKey& girderKey) override;
   virtual Float64 GetAlignmentOffset() override;
   virtual SpanIndexType GetSpanCount() override;
   virtual PierIndexType GetPierCount() override;
   virtual SupportIndexType GetTemporarySupportCount() override;
   virtual GroupIndexType GetGirderGroupCount() override;
   virtual GirderIndexType GetGirderCount(GroupIndexType grpIdx) override;
   virtual GirderIndexType GetGirderlineCount() override;
   virtual GirderIndexType GetGirderCountBySpan(SpanIndexType spanIdx) override;
   virtual SegmentIndexType GetSegmentCount(const CGirderKey& girderKey) override;
   virtual SegmentIndexType GetSegmentCount(GroupIndexType grpIdx,GirderIndexType gdrIdx) override;
   virtual PierIndexType GetGirderGroupStartPier(GroupIndexType grpIdx) override;
   virtual PierIndexType GetGirderGroupEndPier(GroupIndexType grpIdx) override;
   virtual void GetGirderGroupPiers(GroupIndexType grpIdx,PierIndexType* pStartPierIdx,PierIndexType* pEndPierIdx) override;
   virtual SpanIndexType GetGirderGroupStartSpan(GroupIndexType grpIdx) override;
   virtual SpanIndexType GetGirderGroupEndSpan(GroupIndexType grpIdx) override;
   virtual void GetGirderGroupSpans(GroupIndexType grpIdx,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) override;
   virtual GroupIndexType GetGirderGroupIndex(SpanIndexType spanIdx) override;
   virtual void GetGirderGroupIndex(PierIndexType pierIdx,GroupIndexType* pBackGroupIdx,GroupIndexType* pAheadGroupIdx) override;
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight) override;
   virtual void GetBottomFlangeClearance(const pgsPointOfInterest& poi,Float64* pLeft,Float64* pRight) override;
   virtual std::vector<Float64> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType) override;
   virtual Float64 GetGirderOffset(GirderIndexType gdrIdx,PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::OffsetMeasurementType offsetMeasureDatum) override;
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(Float64 station) override;
   virtual std::vector<Float64> CBridgeAgentImp::GetGirderSpacing(SpanIndexType spanIdx,Float64 Xspan) override;
   virtual void GetSpacingAlongGirder(const CGirderKey& girderKey,Float64 Xg,Float64* leftSpacing,Float64* rightSpacing) override;
   virtual void GetSpacingAlongGirder(const pgsPointOfInterest& poi,Float64* leftSpacing,Float64* rightSpacing) override;
   virtual std::vector<std::pair<SegmentIndexType,Float64>> GetSegmentLengths(const CSpanKey& spanKey) override;
   virtual Float64 GetSegmentLength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentSpanLength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentLayoutLength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentFramingLength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentPlanLength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentSlope(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSlabOffset(GroupIndexType grpIdx,PierIndexType pierIdx,GirderIndexType gdrIdx) override;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi) override;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config) override;
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) override;

   virtual void GetSlabOffset(const CSegmentKey& segmentKey,Float64* pStart,Float64* pEnd) override;
   virtual Float64 GetElevationAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetRotationAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetSpanLength(SpanIndexType spanIdx,GirderIndexType gdrIdx) override;
   virtual Float64 GetSpanLength(const CSpanKey& spanKey) override;
   virtual Float64 GetFullSpanLength(const CSpanKey& spanKey) override;
   virtual Float64 GetGirderlineLength(GirderIndexType gdrLineIdx) override;
   virtual Float64 GetCantileverLength(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType endType) override;
   virtual Float64 GetCantileverLength(const CSpanKey& spanKey,pgsTypes::MemberEndType endType) override;
   virtual Float64 GetSegmentStartEndDistance(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentEndEndDistance(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentStartBearingOffset(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentEndBearingOffset(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentStartSupportWidth(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentEndSupportWidth(const CSegmentKey& segmentKey) override;
   virtual Float64 GetCLPierToCLBearingDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) override;
   virtual Float64 GetCLPierToSegmentEndDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) override;
   virtual Float64 GetSegmentOffset(const CSegmentKey& segmentKey,Float64 station) override;
   virtual void GetSegmentAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) override;
   virtual void GetSegmentSkewAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) override;
   virtual void GetSegmentBearing(const CSegmentKey& segmentKey,IDirection** ppBearing) override;
   virtual CSegmentKey GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey) override;
   virtual void GetSpansForSegment(const CSegmentKey& segmentKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) override;
   virtual GDRCONFIG GetSegmentConfiguration(const CSegmentKey& segmentKey) override;
   virtual void ModelCantilevers(const CSegmentKey& segmentKey,bool* pbStartCantilever,bool* pbEndCantilever) override;
   virtual bool GetSpan(Float64 station,SpanIndexType* pSpanIdx) override;
   virtual void GetPoint(const CSegmentKey& segmentKey,Float64 Xpoi,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) override;
   virtual void GetPoint(const pgsPointOfInterest& poi,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) override;
   virtual bool GetSegmentPierIntersection(const CSegmentKey& segmentKey,PierIndexType pierIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) override;
   virtual bool GetSegmentTempSupportIntersection(const CSegmentKey& segmentKey,SupportIndexType tsIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) override;
   virtual void GetStationAndOffset(const CSegmentKey& segmentKey,Float64 Xpoi,Float64* pStation,Float64* pOffset) override;
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset) override;
   virtual bool IsInteriorGirder(const CGirderKey& girderKey) override;
   virtual bool IsExteriorGirder(const CGirderKey& girderKey) override;
   virtual bool IsLeftExteriorGirder(const CGirderKey& girderKey) override;
   virtual bool IsRightExteriorGirder(const CGirderKey& girderKey) override;
   virtual bool IsObtuseCorner(const CSpanKey& spanKey,pgsTypes::MemberEndType endType) override;
   virtual bool AreGirderTopFlangesRoughened(const CSegmentKey& segmentKey) override;
   virtual void GetClosureJointProfile(const CClosureKey& closureKey,IShape** ppShape) override;
   virtual Float64 GetClosureJointLength(const CClosureKey& closureKey) override;
   virtual void GetClosureJointSize(const CClosureKey& closureKey,Float64* pLeft,Float64* pRight) override;
   virtual void GetAngleBetweenSegments(const CClosureKey& closureKey,IAngle** ppAngle) override;
   virtual void GetPierDiaphragmSize(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,Float64* pW,Float64* pH) override;
   virtual bool DoesPierDiaphragmLoadGirder(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) override;
   virtual Float64 GetPierDiaphragmLoadLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endTYpe) override;
   virtual std::vector<IntermedateDiaphragm> GetPrecastDiaphragms(const CSegmentKey& segmentKey) override;
   virtual std::vector<IntermedateDiaphragm> GetCastInPlaceDiaphragms(const CSpanKey& spanKey) override;
   virtual pgsTypes::SupportedDeckType GetDeckType() override;
   virtual pgsTypes::WearingSurfaceType GetWearingSurfaceType() override;
   virtual bool IsCompositeDeck() override;
   virtual bool HasOverlay() override;
   virtual bool IsFutureOverlay() override;
   virtual Float64 GetOverlayWeight() override;
   virtual Float64 GetOverlayDepth() override;
   virtual Float64 GetSacrificalDepth() override;
   virtual Float64 GetFillet(SpanIndexType spanIdx, GirderIndexType gdrIdx) override;
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetLeftSlabEdgeOffset(Float64 Xb) override;
   virtual Float64 GetRightSlabEdgeOffset(Float64 Xb) override;
   virtual Float64 GetLeftSlabOverhang(Float64 Xb) override;
   virtual Float64 GetRightSlabOverhang(Float64 Xb) override;
   virtual Float64 GetLeftSlabOverhang(SpanIndexType spanIdx,Float64 Xspan) override;
   virtual Float64 GetRightSlabOverhang(SpanIndexType spanIdx,Float64 Xspan) override;
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pierIdx) override;
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pierIdx) override;
   virtual Float64 GetLeftCurbOffset(Float64 Xb) override;
   virtual Float64 GetRightCurbOffset(Float64 Xb) override;
   virtual Float64 GetLeftCurbOffset(SpanIndexType spanIdx,Float64 Xspan) override;
   virtual Float64 GetRightCurbOffset(SpanIndexType spanIdx,Float64 Xspan) override;
   virtual Float64 GetLeftCurbOffset(PierIndexType pierIdx) override;
   virtual Float64 GetRightCurbOffset(PierIndexType pierIdx) override;
   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetCurbToCurbWidth(const CSegmentKey& segmentKey,Float64 Xs) override;
   virtual Float64 GetCurbToCurbWidth(Float64 Xb) override;
   virtual Float64 GetLeftInteriorCurbOffset(Float64 Xb) override;
   virtual Float64 GetRightInteriorCurbOffset(Float64 Xb) override;
   virtual Float64 GetLeftInteriorCurbOffset(PierIndexType pierIdx) override;
   virtual Float64 GetRightInteriorCurbOffset(PierIndexType pierIdx) override;
   virtual Float64 GetInteriorCurbToCurbWidth(Float64 Xb) override;
   virtual Float64 GetLeftOverlayToeOffset(Float64 Xb) override;
   virtual Float64 GetRightOverlayToeOffset(Float64 Xb) override;
   virtual Float64 GetLeftOverlayToeOffset(const pgsPointOfInterest& poi) override;
   virtual Float64 GetRightOverlayToeOffset(const pgsPointOfInterest& poi) override;
   virtual void GetSlabPerimeter(CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) override;
   virtual void GetSlabPerimeter(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) override;
   virtual void GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) override;
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) override;
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) override;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) override;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) override;
   virtual void GetLeftCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) override;
   virtual void GetLeftCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) override;
   virtual void GetRightCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) override;
   virtual void GetRightCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) override;
   virtual Float64 GetPierStation(PierIndexType pierIdx) override;
   virtual Float64 GetBearingStation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) override;
   virtual void GetBearingPoint(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,const CGirderKey& girderKey,Float64* pStation,Float64* pOffset) override;
   virtual void GetPierDirection(PierIndexType pierIdx,IDirection** ppDirection) override;
   virtual void GetPierSkew(PierIndexType pierIdx,IAngle** ppAngle) override;
   virtual void GetPierPoints(PierIndexType pierIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right) override;
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) override;
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) override;
   virtual bool GetPierLocation(PierIndexType pierIdx,const CSegmentKey& segmentKey,Float64* pXs) override;
   virtual bool GetPierLocation(const CGirderKey& girderKey,PierIndexType pierIdx,Float64* pXgp) override;
   virtual bool GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew) override;
   virtual pgsTypes::PierModelType GetPierModelType(PierIndexType pierIdx) override;
   virtual ColumnIndexType GetColumnCount(PierIndexType pierIdx) override;
   virtual void GetColumnProperties(PierIndexType pierIdx,ColumnIndexType colIdx,bool bSkewAdjust,Float64* pHeight,Float64* pA,Float64* pI) override;
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx) override;
   virtual pgsTypes::BoundaryConditionType GetBoundaryConditionType(PierIndexType pierIdx) override;
   virtual pgsTypes::PierSegmentConnectionType GetPierSegmentConnectionType(PierIndexType pierIdx) override;
   virtual bool IsAbutment(PierIndexType pierIdx) override;
   virtual bool IsPier(PierIndexType pierIdx) override;
   virtual bool HasCantilever(PierIndexType pierIdx) override;
   virtual bool IsInteriorPier(PierIndexType pierIdx) override;
   virtual bool IsBoundaryPier(PierIndexType pierIdx) override;
   virtual void GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx,SpanIndexType* pSpanIdx,Float64* pXspan) override;
   virtual bool GetTemporarySupportLocation(SupportIndexType tsIdx,const CSegmentKey& segmentKey,Float64* pXs) override;
   virtual Float64 GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx) override;
   virtual pgsTypes::TemporarySupportType GetTemporarySupportType(SupportIndexType tsIdx) override;
   virtual pgsTypes::TempSupportSegmentConnectionType GetSegmentConnectionTypeAtTemporarySupport(SupportIndexType tsIdx) override;
   virtual void GetSegmentsAtTemporarySupport(GirderIndexType gdrIdx,SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey) override;
   virtual void GetTemporarySupportDirection(SupportIndexType tsIdx,IDirection** ppDirection) override;

// IMaterials
public:
   virtual Float64 GetSegmentFc28(const CSegmentKey& segmentKey) override;
   virtual Float64 GetClosureJointFc28(const CSegmentKey& closureKey) override;
   virtual Float64 GetDeckFc28() override;
   virtual Float64 GetRailingSystemFc28(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetPierFc28(IndexType pierIdx) override;

   virtual Float64 GetSegmentEc28(const CSegmentKey& segmentKey) override;
   virtual Float64 GetClosureJointEc28(const CSegmentKey& closureKey) override;
   virtual Float64 GetDeckEc28() override;
   virtual Float64 GetRailingSystemEc28(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetPierEc28(IndexType pierIdx) override;

   virtual Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetClosureJointWeightDensity(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetDeckWeightDensity(IntervalIndexType intervalIdx) override;
   virtual Float64 GetRailingSystemWeightDensity(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) override;

   virtual Float64 GetSegmentConcreteAge(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetClosureJointConcreteAge(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetDeckConcreteAge(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetRailingSystemAge(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;

   virtual Float64 GetSegmentFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetClosureJointFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetDeckFc(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;

   virtual Float64 GetSegmentDesignFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetClosureJointDesignFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetDeckDesignFc(IntervalIndexType intervalIdx) override;

   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) override;
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) override;
   virtual Float64 GetDeckEc(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   
   virtual Float64 GetSegmentLambda(const CSegmentKey& segmentKey) override;
   virtual Float64 GetClosureJointLambda(const CClosureKey& closureKey) override;
   virtual Float64 GetDeckLambda() override;
   virtual Float64 GetRailingSystemLambda(pgsTypes::TrafficBarrierOrientation orientation) override;

   virtual Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetClosureJointFlexureFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetClosureJointShearFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetDeckFlexureFr(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;
   virtual Float64 GetDeckShearFr(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) override;

   virtual Float64 GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetClosureJointAgingCoefficient(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetDeckAgingCoefficient(IntervalIndexType intervalIdx) override;
   virtual Float64 GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) override;

   virtual Float64 GetSegmentAgeAdjustedEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetClosureJointAgeAdjustedEc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetDeckAgeAdjustedEc(IntervalIndexType intervalIdx) override;
   virtual Float64 GetRailingSystemAgeAdjustedEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) override;

   virtual Float64 GetTotalSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) override;
   virtual Float64 GetTotalClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) override;
   virtual Float64 GetTotalDeckFreeShrinkageStrain(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) override;
   virtual Float64 GetTotalRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;

   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) override;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalClosureJointFreeShrinkageStrainDetails(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) override;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalDeckFreeShrinkageStrainDetails(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) override;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalRailingSystemFreeShrinakgeStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;

   virtual Float64 GetIncrementalSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetIncrementalClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) override;
   virtual Float64 GetIncrementalDeckFreeShrinkageStrain(IntervalIndexType intervalIdx) override;
   virtual Float64 GetIncrementalRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) override;

   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalClosureJointFreeShrinkageStrainDetails(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalDeckFreeShrinkageStrainDetails(IntervalIndexType intervalIdx) override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalRailingSystemFreeShrinakgeStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) override;

   virtual Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetClosureJointCreepCoefficient(const CSegmentKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetDeckCreepCoefficient(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType) override;

   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetSegmentCreepCoefficientDetails(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetClosureJointCreepCoefficientDetails(const CSegmentKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetDeckCreepCoefficientDetails(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetRailingSystemCreepCoefficientDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) override;

   virtual pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey) override;
   virtual bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentEccK1(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentEccK2(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey) override;
   virtual const matConcreteBase* GetSegmentConcrete(const CSegmentKey& segmentKey) override;
   virtual pgsTypes::ConcreteType GetClosureJointConcreteType(const CClosureKey& closureKey) override;
   virtual bool DoesClosureJointConcreteHaveAggSplittingStrength(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointConcreteAggSplittingStrength(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointStrengthDensity(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointMaxAggrSize(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointEccK1(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointEccK2(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointCreepK1(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointCreepK2(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointShrinkageK1(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointShrinkageK2(const CClosureKey& closureKey) override;
   virtual const matConcreteBase* GetClosureJointConcrete(const CClosureKey& closureKey) override;
   virtual pgsTypes::ConcreteType GetDeckConcreteType() override;
   virtual bool DoesDeckConcreteHaveAggSplittingStrength() override;
   virtual Float64 GetDeckConcreteAggSplittingStrength() override;
   virtual Float64 GetDeckMaxAggrSize() override;
   virtual Float64 GetDeckStrengthDensity() override;
   virtual Float64 GetDeckEccK1() override;
   virtual Float64 GetDeckEccK2() override;
   virtual Float64 GetDeckCreepK1() override;
   virtual Float64 GetDeckCreepK2() override;
   virtual Float64 GetDeckShrinkageK1() override;
   virtual Float64 GetDeckShrinkageK2() override;
   virtual const matConcreteBase* GetDeckConcrete() override;
   virtual const matPsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) override;
   virtual Float64 GetIncrementalStrandRelaxation(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType) override;
   virtual INCREMENTALRELAXATIONDETAILS GetIncrementalStrandRelaxationDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType) override;
   virtual const matPsStrand* GetTendonMaterial(const CGirderKey& girderKey) override;
   virtual Float64 GetIncrementalTendonRelaxation(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso) override;
   virtual INCREMENTALRELAXATIONDETAILS GetIncrementalTendonRelaxationDetails(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso) override;
   virtual void GetSegmentLongitudinalRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) override;
   virtual std::_tstring GetSegmentLongitudinalRebarName(const CSegmentKey& segmentKey) override;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade) override;
   virtual void GetClosureJointLongitudinalRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) override;
   virtual std::_tstring GetClosureJointLongitudinalRebarName(const CClosureKey& closureKey) override;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade) override;
   virtual void GetSegmentTransverseRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) override;
   virtual void GetSegmentTransverseRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade) override;
   virtual std::_tstring GetSegmentTransverseRebarName(const CSegmentKey& segmentKey) override;
   virtual void GetClosureJointTransverseRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) override;
   virtual void GetClosureJointTransverseRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade) override;
   virtual std::_tstring GetClosureJointTransverseRebarName(const CClosureKey& closureKey) override;
   virtual void GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu) override;
   virtual std::_tstring GetDeckRebarName() override;
   virtual void GetDeckRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade) override;
   virtual Float64 GetNWCDensityLimit() override;
   virtual Float64 GetLWCDensityLimit() override;
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) override;
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) override;
   virtual Float64 GetFlexureFrCoefficient(const CSegmentKey& segmentKey) override;
   virtual Float64 GetShearFrCoefficient(const CSegmentKey& segmentKey) override;
   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2) override;

// ILongRebarGeometry
public:
   virtual void GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection) override;
   virtual Float64 GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust) override;
   virtual Float64 GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) override;
   virtual Float64 GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) override;
   virtual Float64 GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) override;
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem) override;
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) override;
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetCoverTopMat() override;
   virtual Float64 GetTopMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) override;
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) override;
   virtual Float64 GetCoverBottomMat() override;
   virtual Float64 GetBottomMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) override;
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) override;
   virtual void GetDeckReinforcing(const pgsPointOfInterest& poi,pgsTypes::DeckRebarMatType matType,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bAdjForDevLength,Float64* pAs,Float64* pYb) override;
   virtual void GetRebarLayout(const CSegmentKey& segmentKey, IRebarLayout** rebarLayout) override;
   virtual void GetClosureJointRebarLayout(const CClosureKey& closureKey, IRebarLayout** rebarLayout) override;
   virtual REBARDEVLENGTHDETAILS GetSegmentRebarDevelopmentLengthDetails(const CSegmentKey& segmetnKey,IRebar* rebar, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) override;
   virtual REBARDEVLENGTHDETAILS GetDeckRebarDevelopmentLengthDetails(IRebar* rebar, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) override;

// IStirrupGeometry
public:
   virtual bool AreStirrupZonesSymmetrical(const CSegmentKey& segmentKey) override;

   virtual ZoneIndexType GetPrimaryZoneCount(const CSegmentKey& segmentKey) override;
   virtual void GetPrimaryZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end) override;
   virtual void GetPrimaryVertStirrupBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing) override;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const CSegmentKey& segmentKey,ZoneIndexType zone) override;
   virtual matRebar::Size GetPrimaryConfinementBarSize(const CSegmentKey& segmentKey,ZoneIndexType zone) override;

   virtual ZoneIndexType GetHorizInterfaceZoneCount(const CSegmentKey& segmentKey) override;
   virtual void GetHorizInterfaceZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end) override;
   virtual void GetHorizInterfaceBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing) override;

   virtual void GetAddSplittingBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing) override;
   virtual void GetAddConfinementBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing) override;

   virtual Float64 GetVertStirrupAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) override;
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi) override;
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi) override; // stirrup angle=90 for vertical

   virtual bool DoStirrupsEngageDeck(const CSegmentKey& segmentKey) override;
   virtual bool DoAllPrimaryStirrupsEngageDeck(const CSegmentKey& segmentKey) override;
   virtual Float64 GetPrimaryHorizInterfaceBarSpacing(const pgsPointOfInterest& poi) override;
   virtual Float64 GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) override;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi) override;
   virtual Float64 GetAdditionalHorizInterfaceBarSpacing(const pgsPointOfInterest& poi) override;
   virtual Float64 GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) override;
   virtual Float64 GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi) override;

   virtual Float64 GetSplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end) override;

   virtual void GetStartConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) override;
   virtual void GetEndConfinementBarInfo(  const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) override;

   virtual bool AreStirrupZoneLengthsCombatible(const CGirderKey& girderKey) override;

// IStrandGeometry
public:
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands) override;
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands) override;

   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands) override;
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands) override;

   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) override;
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) override;

   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) override;
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) override;

   virtual Float64 GetStrandLocation(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, IntervalIndexType intervalIdx) override;

   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetMaxStrandSlope(const CSegmentKey& segmentKey) override;

   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) override;

   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi, DevelopmentAdjustmentType devAdjust, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust, const GDRCONFIG* pConfig = nullptr) override;

   virtual StrandIndexType GetStrandCount(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const GDRCONFIG* pConfig=nullptr) override;
   virtual StrandIndexType GetMaxStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type) override;
   virtual StrandIndexType GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type) override;
   virtual Float64 GetStrandArea(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType) override;
   virtual Float64 GetStrandArea(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType) override;
   virtual Float64 GetAreaPrestressStrands(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,bool bIncTemp) override;

   virtual Float64 GetPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetPjack(const CSegmentKey& segmentKey,bool bIncTemp) override;
   virtual void GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint) override;
   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) override;
   virtual void GetStrandPositionEx(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, const PRESTRESSCONFIG& rconfig,IPoint2d** ppPoint) override;
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) override;
   virtual void GetStrandPositionsEx(LPCTSTR strGirderName, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type,pgsTypes::MemberEndType endType, IPoint2dCollection** ppPoints) override;

   // Harped strands can be forced to be straight along their length
   virtual bool GetAreHarpedStrandsForcedStraight(const CSegmentKey& segmentKey) override;

   virtual void GetHarpedStrandControlHeights(const CSegmentKey& segmentKey,Float64* pHgStart,Float64* pHgHp1,Float64* pHgHp2,Float64* pHgEnd) override;

   // harped offsets are measured from original strand locations in strand grid
   virtual void GetHarpStrandOffsets(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* pOffsetEnd,Float64* pOffsetHp) override;

   virtual void GetHarpedEndOffsetBounds(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* DownwardOffset, Float64* UpwardOffset) override;
   virtual void GetHarpedEndOffsetBoundsEx(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset) override;
   virtual void GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) override;
   virtual void GetHarpedHpOffsetBounds(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* DownwardOffset, Float64* UpwardOffset) override;
   virtual void GetHarpedHpOffsetBoundsEx(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset) override;
   virtual void GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) override;

   virtual Float64 GetHarpedEndOffsetIncrement(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHarpedHpOffsetIncrement(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHarpedEndOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType) override;
   virtual Float64 GetHarpedHpOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType) override;

   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* lhp,Float64* rhp) override;
   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* pX1,Float64* pX2,Float64* pX3,Float64* pX4) override;
   virtual void GetHighestHarpedStrandLocationEnds(const CSegmentKey& segmentKey,Float64* pElevation) override;
   virtual void GetHighestHarpedStrandLocationHPs(const CSegmentKey& segmentKey,Float64* pElevation) override;
   virtual IndexType GetNumHarpPoints(const CSegmentKey& segmentKey) override;

   virtual StrandIndexType GetMaxNumPermanentStrands(const CSegmentKey& segmentKey) override;
   virtual StrandIndexType GetMaxNumPermanentStrands(LPCTSTR strGirderName) override;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,const CSegmentKey& segmentKey, StrandIndexType* numStraight, StrandIndexType* numHarped) override;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped) override;
   virtual StrandIndexType GetNextNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) override;
   virtual StrandIndexType GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum) override;
   virtual StrandIndexType GetPreviousNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) override;
   virtual StrandIndexType GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum) override;
   virtual bool ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices) override;


   virtual bool IsValidNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) override;
   virtual bool IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) override;
   virtual StrandIndexType GetNextNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) override;
   virtual StrandIndexType GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) override;
   virtual StrandIndexType GetPrevNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) override;
   virtual StrandIndexType GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) override;

   virtual StrandIndexType GetNumExtendedStrands(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType standType) override;
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) override;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) override;

   virtual ConfigStrandFillVector ComputeStrandFill(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType Ns) override;
   virtual ConfigStrandFillVector ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns) override;

   virtual GridIndexType SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo) override;
   virtual void GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) override;

   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig,Float64* pStart,Float64* pEnd) override;
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) override;
   virtual StrandIndexType GetNumDebondedStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, pgsTypes::DebondMemberEndType end) override;
   virtual RowIndexType GetNumRowsWithStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType ) override;
   virtual StrandIndexType GetNumStrandInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) override;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const pgsPointOfInterest& poi, RowIndexType rowIdx, pgsTypes::StrandType strandType ) override;
   virtual StrandIndexType GetNumDebondedStrandsInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType ) override;
   virtual bool IsExteriorStrandDebondedInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType) override;
   virtual bool HasDebonding(const CSegmentKey& segmentKey) override;
   virtual bool IsDebondingSymmetric(const CSegmentKey& segmentKey) override;

   virtual RowIndexType GetNumRowsWithStrand(const pgsPointOfInterest& poi,StrandIndexType nStrands,pgsTypes::StrandType strandType ) override;
   virtual StrandIndexType GetNumStrandInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType ) override;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType ) override;

   virtual Float64 GetDebondSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) override;
   virtual SectionIndexType GetNumDebondSections(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) override;
   virtual StrandIndexType GetNumDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) override;
   virtual StrandIndexType GetNumBondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) override;
   virtual std::vector<StrandIndexType> GetDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) override;

   virtual bool CanDebondStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) override; // can debond any of the strands
   virtual bool CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType) override;
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) override;
   virtual void ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) override; 
   virtual Float64 GetDefaultDebondLength(const CSegmentKey& segmentKey) override;

   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) override;
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) override;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) override;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) override;
   virtual Float64 ConvertHarpedOffsetEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) override;
   virtual Float64 ConvertHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) override;
   virtual Float64 ConvertHarpedOffsetHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) override;
   virtual Float64 ConvertHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) override;

   virtual pgsTypes::TTSUsage GetTemporaryStrandUsage(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;

// IPointOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey) override;
   virtual std::vector<pgsPointOfInterest> GetPointOfInterests(Float64 station,IDirection* pDirection) override;
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR) override;
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSpanKey& spanKey) override;
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSpanKey& spanKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR) override;
   virtual pgsPointOfInterest GetPointOfInterest(PoiIDType poiID) override;
   virtual pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs,Float64 tolerance=0.001) override;
   virtual bool GetPointOfInterest(const CSegmentKey& segmentKey,Float64 station,IDirection* pDirection,pgsPointOfInterest* pPoi) override;
   virtual std::vector<pgsPointOfInterest> GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey) override;
   virtual std::vector<pgsPointOfInterest> GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey,const GDRCONFIG& config) override;
   virtual bool GetPointOfInterest(const CGirderKey& girderKey,Float64 station,IDirection* pDirection,bool bProjectSegmentEnds,pgsPointOfInterest* pPoi) override;
   virtual pgsPointOfInterest GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs) override;
   virtual pgsPointOfInterest GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR) override;
   virtual pgsPointOfInterest GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR) override;
   virtual pgsPointOfInterest GetPierPointOfInterest(const CGirderKey& girderKey,PierIndexType pierIdx) override;
   virtual pgsPointOfInterest GetTemporarySupportPointOfInterest(const CGirderKey& girderKey,SupportIndexType tsIdx) override;
   virtual void RemovePointsOfInterest(std::vector<pgsPointOfInterest>& vPoi,PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute) override;
   virtual bool IsInClosureJoint(const pgsPointOfInterest& poi,CClosureKey* pClosureKey) override;
   virtual bool IsOnSegment(const pgsPointOfInterest& poi) override;
   virtual bool IsOffSegment(const pgsPointOfInterest& poi) override;
   virtual bool IsOnGirder(const pgsPointOfInterest& poi) override;
   virtual bool IsInIntermediateDiaphragm(const pgsPointOfInterest& poi) override;
   virtual bool IsInCriticalSectionZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState) override;
   virtual pgsPointOfInterest ConvertSpanPointToPoi(const CSpanKey& spanKey,Float64 Xspan,Float64 tolerance=0.001) override;
   virtual void ConvertPoiToSpanPoint(const pgsPointOfInterest& poi,CSpanKey* pSpanKey,Float64* pXspan) override;
   virtual void ConvertSpanPointToSegmentCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXs) override;
   virtual void ConvertSegmentCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xs,CSpanKey* pSpanKey,Float64* pXspan) override;
   virtual void ConvertSpanPointToSegmentPathCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXsp) override;
   virtual void ConvertSegmentPathCoordinateToSpanPoint(const CSegmentKey& sSegmentKey,Float64 Xsp,CSpanKey* pSpanKey,Float64* pXspan) override;
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterestInRange(Float64 xLeft,const pgsPointOfInterest& poi,Float64 xRight) override;
   virtual PierIndexType GetPier(const pgsPointOfInterest& poi) override;
   virtual std::list<std::vector<pgsPointOfInterest>> GroupBySegment(const std::vector<pgsPointOfInterest>& vPoi) override;
   virtual std::list<std::vector<pgsPointOfInterest>> GroupByGirder(const std::vector<pgsPointOfInterest>& vPoi) override;
   virtual std::vector<CSegmentKey> GetSegmentKeys(const std::vector<pgsPointOfInterest>& vPoi) override;
   virtual std::vector<CSegmentKey> GetSegmentKeys(const std::vector<pgsPointOfInterest>& vPoi,const CGirderKey& girderKey) override;
   virtual std::vector<CGirderKey> GetGirderKeys(const std::vector<pgsPointOfInterest>& vPoi) override;
   virtual Float64 ConvertPoiToSegmentPathCoordinate(const pgsPointOfInterest& poi) override;
   virtual pgsPointOfInterest ConvertSegmentPathCoordinateToPoi(const CSegmentKey& segmentKey,Float64 Xsp,Float64 tolerance=0.001) override;
   virtual Float64 ConvertSegmentPathCoordinateToSegmentCoordinate(const CSegmentKey& segmentKey,Float64 Xsp) override;
   virtual Float64 ConvertSegmentCoordinateToGirderCoordinate(const CSegmentKey& segmentKey,Float64 Xs) override;
   //virtual void ConvertGirderCoordinateToSegmentCoordinate(const CGirderKey& girderKey,Float64 Xg,CSegmentKey* pSegmentKey,Float64* pXs) override;
   virtual Float64 ConvertSegmentCoordinateToGirderlineCoordinate(const CSegmentKey& segmentKey,Float64 Xs) override;
   virtual Float64 ConvertSegmentPathCoordinateToGirderPathCoordinate(const CSegmentKey& segmentKey,Float64 Xsp) override;
   virtual Float64 ConvertSegmentCoordinateToSegmentPathCoordinate(const CSegmentKey& segmentKey,Float64 Xs) override;
   virtual Float64 ConvertPoiToGirderCoordinate(const pgsPointOfInterest& poi) override;
   virtual pgsPointOfInterest ConvertGirderCoordinateToPoi(const CGirderKey& girderKey,Float64 Xg,Float64 tolerance=0.001) override;
   virtual Float64 ConvertPoiToGirderPathCoordinate(const pgsPointOfInterest& poi) override;
   virtual pgsPointOfInterest ConvertGirderPathCoordinateToPoi(const CGirderKey& girderKey,Float64 Xgp,Float64 tolerance=0.001) override;
   virtual Float64 ConvertGirderCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float64 Xg) override;
   virtual Float64 ConvertGirderPathCoordinateToGirderCoordinate(const CGirderKey& girderKey,Float64 Xgp) override;
   virtual Float64 ConvertGirderPathCoordinateToGirderlineCoordinate(const CGirderKey& girderKey,Float64 Xgp) override;
   virtual Float64 ConvertPoiToGirderlineCoordinate(const pgsPointOfInterest& poi) override;
   virtual pgsPointOfInterest ConvertGirderlineCoordinateToPoi(GirderIndexType gdrIdx,Float64 Xgl,Float64 tolerance=0.001) override;
   virtual Float64 ConvertRouteToBridgeLineCoordinate(Float64 station) override;
   virtual Float64 ConvertBridgeLineToRouteCoordinate(Float64 Xb) override;
   virtual Float64 ConvertPoiToBridgeLineCoordinate(const pgsPointOfInterest& poi) override;

// ISectionProperties
public:
   virtual pgsTypes::SectionPropertyMode GetSectionPropertiesMode() override;
   virtual Float64 GetHg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) override;
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) override;
   virtual Float64 GetKt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetKb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetEIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;

   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) override;
   virtual Float64 GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) override;
   virtual Float64 GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) override;
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr) override;
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr) override;

   virtual Float64 GetHg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) override;
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) override;
   virtual Float64 GetKt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetKb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetEIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;

   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) override;
   virtual Float64 GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) override;
   virtual Float64 GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) override;
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc) override;
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc) override;

   virtual Float64 GetNetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetIg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetYbg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetYtg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetAd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetId(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetYbd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;
   virtual Float64 GetNetYtd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) override;

   virtual Float64 GetQSlab(const pgsPointOfInterest& poi) override;
   virtual Float64 GetAcBottomHalf(const pgsPointOfInterest& poi) override;
   virtual Float64 GetAcTopHalf(const pgsPointOfInterest& poi) override;
   virtual Float64 GetEffectiveFlangeWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetTributaryFlangeWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetTributaryFlangeWidthEx(const pgsPointOfInterest& poi, Float64* pLftFw, Float64* pRgtFw) override;
   virtual Float64 GetEffectiveDeckArea(const pgsPointOfInterest& poi) override;
   virtual Float64 GetTributaryDeckArea(const pgsPointOfInterest& poi) override;
   virtual Float64 GetGrossDeckArea(const pgsPointOfInterest& poi) override;
   virtual Float64 GetDistTopSlabToTopGirder(const pgsPointOfInterest& poi) override;
   virtual Float64 GetDistTopSlabToTopGirder(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) override;
   virtual void ReportEffectiveFlangeWidth(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi) override;
   virtual Float64 GetSegmentSurfaceArea(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentVolume(const CSegmentKey& segmentKey) override;
   virtual Float64 GetClosureJointSurfaceArea(const CClosureKey& closureKey) override;
   virtual Float64 GetClosureJointVolume(const CClosureKey& closureKey) override;
   virtual Float64 GetDeckSurfaceArea() override;
   virtual Float64 GetDeckVolume() override;
   virtual Float64 GetBridgeEIxx(Float64 Xb) override;
   virtual Float64 GetBridgeEIyy(Float64 Xb) override;
   virtual Float64 GetSegmentWeightPerLength(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentWeight(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSegmentHeightAtPier(const CSegmentKey& segmentKey,PierIndexType pierIdx) override;
   virtual Float64 GetSegmentHeightAtTemporarySupport(const CSegmentKey& segmentKey,SupportIndexType tsIdx) override;

// IShapes
public:
   virtual void GetSegmentShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bOrient,pgsTypes::SectionCoordinateType coordinateType,IShape** ppShape) override;
   virtual void GetSlabShape(Float64 station,IDirection* pDirection,bool bIncludeHaunch,IShape** ppShape) override;
   virtual void GetLeftTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape) override;
   virtual void GetRightTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape) override;


// IBarriers
public:
   virtual Float64 GetAtb(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetItb(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetYbtb(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetExteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual bool HasInteriorBarrier(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetInteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetInteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual void GetSidewalkDeadLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) override;
   virtual void GetSidewalkPedLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) override;
   virtual bool HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation) override;

// ISegmentLiftingPointsOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetLiftingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR) override;
   virtual std::vector<pgsPointOfInterest> GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType attrib,Uint32 mode = POIFIND_OR) override;

// ISegmentHaulingPointsOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR) override;
   virtual std::vector<pgsPointOfInterest> GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode = POIFIND_OR) override;
   virtual Float64 GetMinimumOverhang(const CSegmentKey& segmentKey) override;

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   virtual HRESULT OnGirderFamilyChanged() override;
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   virtual HRESULT OnLiveLoadChanged() override;
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   virtual HRESULT OnConstructionLoadChanged() override;

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged() override;
   virtual HRESULT OnAnalysisTypeChanged() override;

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged() override;

// IUserDefinedLoads
public:
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey) override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey) override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IUserDefinedLoads::UserDefinedLoadCase loadCase) override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IUserDefinedLoads::UserDefinedLoadCase loadCase) override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx) override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx) override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx) override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx) override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) override;
   virtual const std::vector<UserPointLoad>* GetPointLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey) override;
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey) override;
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey) override;

// ITempSupport
public:
   virtual void GetControlPoints(SupportIndexType tsIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppLeft,IPoint2d** ppAlignment_pt,IPoint2d** ppBridge_pt,IPoint2d** ppRight) override;
   virtual void GetDirection(SupportIndexType tsIdx,IDirection** ppDirection) override;
   virtual void GetSkew(SupportIndexType tsIdx,IAngle** ppAngle) override;

// IGirder
public:
   virtual bool    IsPrismatic(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey) override;
   virtual bool    IsSymmetricSegment(const CSegmentKey& segmentKey) override;
   virtual bool    IsSymmetric(IntervalIndexType intervalIdx,const CGirderKey& girderKey) override; 
   virtual MatingSurfaceIndexType  GetNumberOfMatingSurfaces(const CGirderKey& girderKey) override;
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx) override;
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx) override;
   virtual FlangeIndexType GetNumberOfTopFlanges(const CGirderKey& girderKey) override;
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi) override;
   virtual FlangeIndexType GetNumberOfBottomFlanges(const CSegmentKey& segmentKey) override;
   virtual Float64 GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) override;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetBottomWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetMinWebWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetWebThicknessAtDuct(const pgsPointOfInterest& poi,DuctIndexType ductIdx) override;
   virtual Float64 GetMinTopFlangeThickness(const pgsPointOfInterest& poi) override;
   virtual Float64 GetMinBottomFlangeThickness(const pgsPointOfInterest& poi) override;
   virtual Float64 GetHeight(const pgsPointOfInterest& poi) override;
   virtual Float64 GetShearWidth(const pgsPointOfInterest& poi) override;
   virtual Float64 GetShearInterfaceWidth(const pgsPointOfInterest& poi) override;
   virtual WebIndexType GetWebCount(const CGirderKey& girderKey) override;
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx) override;
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx) override;
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx) override;
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi) override;
   virtual Float64 GetWebWidth(const pgsPointOfInterest& poi) override;
   virtual void GetSegmentEndPoints(const CSegmentKey& segmentKey,pgsTypes::PlanCoordinateType pcType,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2) override;
   virtual Float64 GetOrientation(const CSegmentKey& segmentKey) override;
   virtual Float64 GetProfileChordElevation(const pgsPointOfInterest& poi) override;
   virtual Float64 GetTopGirderChordElevation(const pgsPointOfInterest& poi) override;
   virtual Float64 GetTopGirderChordElevation(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) override;
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIdx,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi) override;
   virtual pgsTypes::SplittingDirection GetSplittingDirection(const CGirderKey& girderKey) override;
   virtual void GetProfileShape(const CSegmentKey& segmentKey,IShape** ppShape) override;
   virtual bool HasShearKey(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType) override;
   virtual void GetShearKeyAreas(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) override;
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IShape** ppShape) override;
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IShape** ppShape) override;
   virtual Float64 GetSegmentHeight(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64 Xsp) override;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IPoint2dCollection** points) override;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IPoint2dCollection** points) override;
   virtual void GetSegmentDirection(const CSegmentKey& segmentKey,IDirection** ppDirection) override;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,Float64* pStartEndDistance,Float64* pEndEndDistance) override;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,Float64* pStartEndDistance,Float64* pEndEndDistance) override;
   virtual void GetSegmentBearingOffset(const CSegmentKey& segmentKey,Float64* pStartBearingOffset,Float64* pEndBearingOffset) override;
   virtual void GetSegmentStorageSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) override;
   virtual void GetSegmentReleaseSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) override;
   virtual const stbGirder* GetSegmentStabilityModel(const CSegmentKey& segmentKey) override;
   virtual const stbGirder* GetSegmentStabilityModel(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig) override;
   virtual const stbLiftingStabilityProblem* GetSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey) override;
   virtual const stbLiftingStabilityProblem* GetSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD) override;
   virtual const stbHaulingStabilityProblem* GetSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey) override;
   virtual const stbHaulingStabilityProblem* GetSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig,ISegmentHaulingDesignPointsOfInterest* pPOId) override;

// ITendonGeometry
public:
   DuctIndexType GetDuctCount(const CGirderKey& girderKey);
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint2dCollection** ppPoints) override;
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint3dCollection** ppPoints) override;
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,const CSplicedGirderData* pGirder,IPoint2dCollection** ppPoints) override;
   virtual void GetDuctPoint(const pgsPointOfInterest& poit,DuctIndexType ductIdx,IPoint2d** ppPoint) override;
   virtual void GetDuctPoint(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IPoint2d** ppPoint) override;
   virtual Float64 GetOutsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetInsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetInsideDuctArea(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual StrandIndexType GetTendonStrandCount(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetTendonArea(const CGirderKey& girderKey,IntervalIndexType intervalIdx,DuctIndexType ductIdx) override;
   virtual void GetTendonSlope(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IVector3d** ppSlope) override;
   virtual void GetTendonSlope(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IVector3d** ppSlope) override;
   virtual Float64 GetMinimumRadiusOfCurvature(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetPjack(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetFpj(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetDuctOffset(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) override;
   virtual Float64 GetDuctLength(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) override;
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) override;
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) override;
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,DuctIndexType ductIdx) override;
   virtual pgsTypes::JackingEndType GetJackingEnd(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetAptTopHalf(const pgsPointOfInterest& poi) override;
   virtual Float64 GetAptBottomHalf(const pgsPointOfInterest& poi) override;

// IIntervals
public:
   virtual IntervalIndexType GetIntervalCount() override;
   virtual EventIndexType GetStartEvent(IntervalIndexType idx) override; 
   virtual EventIndexType GetEndEvent(IntervalIndexType idx) override; 
   virtual Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) override;
   virtual Float64 GetDuration(IntervalIndexType idx) override;
   virtual LPCTSTR GetDescription(IntervalIndexType idx) override;
   virtual IntervalIndexType GetInterval(EventIndexType eventIdx) override;
   virtual IntervalIndexType GetErectPierInterval(PierIndexType pierIdx) override;
   virtual IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetLiftSegmentInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetFirstLiftSegmentInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastLiftSegmentInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetFirstStorageInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastStorageInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetHaulSegmentInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey) override;
   virtual bool IsSegmentErectionInterval(IntervalIndexType intervalIdx) override;
   virtual bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) override;
   virtual IntervalIndexType GetTemporaryStrandStressingInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetTemporaryStrandInstallationInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) override;
   virtual IntervalIndexType GetCastClosureJointInterval(const CClosureKey& closureKey) override;
   virtual IntervalIndexType GetCompositeClosureJointInterval(const CClosureKey& closureKey) override;
   virtual IntervalIndexType GetFirstCompositeClosureJointInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastCompositeClosureJointInterval(const CGirderKey& girderKey) override;
   virtual void GetContinuityInterval(PierIndexType pierIdx,IntervalIndexType* pBack,IntervalIndexType* pAhead) override;
   virtual IntervalIndexType GetCastIntermediateDiaphragmsInterval() override;
   virtual IntervalIndexType GetCompositeIntermediateDiaphragmsInterval() override;
   virtual IntervalIndexType GetCastDeckInterval() override;
   virtual IntervalIndexType GetCompositeDeckInterval() override;
   virtual IntervalIndexType GetLiveLoadInterval() override;
   virtual IntervalIndexType GetLoadRatingInterval() override;
   virtual IntervalIndexType GetOverlayInterval() override;
   virtual IntervalIndexType GetInstallRailingSystemInterval() override;
   virtual IntervalIndexType GetFirstTendonStressingInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetLastTendonStressingInterval(const CGirderKey& girderKey) override;
   virtual IntervalIndexType GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual bool IsTendonStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) override;
   virtual bool IsStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) override;
   virtual IntervalIndexType GetTemporarySupportErectionInterval(SupportIndexType tsIdx) override;
   virtual IntervalIndexType GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) override;
   virtual std::vector<IntervalIndexType> GetTemporarySupportRemovalIntervals(GroupIndexType groupIdx) override;
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey) override;
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey,pgsTypes::ProductForceType pfType) override;
   virtual std::vector<IntervalIndexType> GetSpecCheckIntervals(const CGirderKey& girderKey) override;

private:
   DECLARE_EAF_AGENT_DATA;

   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwLossParametersCookie;

   StatusGroupIDType m_LoadStatusGroupID; // ID used to identify user load-related status items created by this agent

   CComPtr<ICogoEngine> m_CogoEngine; // this is not the cogo model!!! just an engine to do computations with
   CComPtr<ICogoModel> m_CogoModel;
   CComPtr<IGenericBridge> m_Bridge;

   // Add these values to local coordinates to get global coordinates
   Float64 m_DeltaX;
   Float64 m_DeltaY;

   CComPtr<IBridgeGeometryTool> m_BridgeGeometryTool;

   std::map<CollectionIndexType,CogoObjectID> m_HorzCurveKeys;
   std::map<CollectionIndexType,CogoObjectID> m_VertCurveKeys;

   CConcreteManager m_ConcreteManager;
   CIntervalManager m_IntervalManager;

   // equations used to compute elevation adjustments in a segment due
   // to temporary support elevation adjustments and pier "A" dimensions
   std::map<CSegmentKey,mathLinFunc2d> m_ElevationAdjustmentEquations;

   // containers to cache shapes cut at various stations
   struct SectionCutKey
   {
      Float64 station;
      Float64 direction;
      bool operator<(const SectionCutKey& other) const
      {
         if ( station < other.station )
         {
            return true;
         }

         if ( IsEqual(station,other.station) )
         {
            if ( direction < other.direction )
            {
               return true;
            }
         }

         return false;
      }
   };
   typedef std::map<SectionCutKey,CComPtr<IShape> > ShapeContainer;
   ShapeContainer m_DeckShapes;
   ShapeContainer m_LeftBarrierShapes;
   ShapeContainer m_RightBarrierShapes;

   // for ISectionProperties
   // Section Properties
   CComPtr<ISectionCutTool> m_SectCutTool;
   CComPtr<IEffectiveFlangeWidthTool> m_EffFlangeWidthTool;
   typedef struct SectProp
   {
      CComPtr<IElasticProperties> ElasticProps;
      CComPtr<IShapeProperties> ShapeProps;

      Float64 YtopGirder;
      Float64 Perimeter;

      bool bComposite; // If false, Qslab is undefined
      Float64 Qslab;
      Float64 AcBottomHalf; // for LRFD Fig 5.8.3.4.2-3
      Float64 AcTopHalf;    // for LRFD Fig 5.8.3.4.2-3

      SectProp() { YtopGirder = 0; Perimeter = 0; bComposite = false; Qslab = 0; AcBottomHalf = 0; AcTopHalf = 0; }
   } SectProp;
   typedef std::map<PoiIntervalKey,SectProp> SectPropContainer; // Key = PoiIntervalKey object
   std::unique_ptr<SectPropContainer> m_pSectProps[pgsTypes::sptSectionPropertyTypeCount]; // index = one of the pgsTypes::SectionPropertyType constants

   // These are the last on the fly section property results
   // (LOTF = last on the fly)
   PoiIntervalKey m_LOTFSectionPropertiesKey;
   pgsTypes::SectionPropertyType m_LOTFSectionPropertiesType;
   SectProp m_LOTFSectionProperties;

   // Cache to hold the girder section objects.
   typedef std::map<pgsPointOfInterest,CComPtr<IGirderSection>> GirderSectionCache;
   std::unique_ptr<GirderSectionCache> m_pGirderSectionCache[2]; // array index is pgsTypes::SectionCoordinateType
   void InvalidateGirderSections(pgsTypes::SectionCoordinateType scType);
   static UINT DeleteGirderSectionCache(LPVOID pParam);

   // Caches to hold the segment values
   typedef std::map<CSegmentKey, Float64> SegmentVolumeCache;
   std::unique_ptr<SegmentVolumeCache> m_pSegmentVolumeCache;
   void InvalidateSegmentVolumes();
   static UINT DeleteSegmentVolumeCache(LPVOID pParam);

   typedef std::map<CSegmentKey, Float64> SegmentSurfaceAreaCache;
   std::unique_ptr<SegmentSurfaceAreaCache> m_pSegmentSurfaceAreaCache;
   void InvalidateSegmentSurfaceAreas();
   static UINT DeleteSegmentSurfaceAreaCache(LPVOID pParam);

   void InvalidateSectionProperties(pgsTypes::SectionPropertyType sectPropType);
   static UINT DeleteSectionProperties(LPVOID pParam);
   pgsTypes::SectionPropertyType GetSectionPropertiesType(); // returns the section properties types for the current section properties mode
   PoiIntervalKey GetSectionPropertiesKey(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType);
   SectProp GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType);
   HRESULT GetSection(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType,ISection** ppSection);
   Float64 ComputeY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,IShapeProperties* sprops);
   Float64 ComputeYtopGirder(IShapeProperties* compositeProps,IShapeProperties* beamProps);

   std::map<PoiIntervalKey,CComPtr<IShape>> m_Shapes;

   // Points of interest for precast segments (precast girders/spliced girder segments)
   std::unique_ptr<pgsPoiMgr> m_pPoiMgr;
   pgsPoiMgr* CreatePoiManager();
   void InvalidatePointsOfInterest();
   static UINT DeletePoiManager(LPVOID pParam);
   std::set<CGirderKey> m_ValidatedPoi;

   // keeps track of which girders have had critical sections locations determined
   std::set<CGirderKey> m_CriticalSectionState[9]; // use the LimitStateToShearIndex method to map limit state to array index

   // Adapter for working with strand fills
   // DON'T ACCESS THIS COLLECTION DIRECTLY - USE ACCESS FUNCTIONS BELOW
   typedef std::map<CSegmentKey, CStrandFiller>  StrandFillerCollection;
   StrandFillerCollection  m_StrandFillers; // a filler for every girder

   CContinuousStrandFiller* GetContinuousStrandFiller(const CSegmentKey& segmentKey);
   CDirectStrandFiller* GetDirectStrandFiller(const CSegmentKey& segmentKey);
   void InitializeStrandFiller(const GirderLibraryEntry* pGirderEntry, const CSegmentKey& segmentKey);

   class CUserLoadKey : public CSpanKey
   {
   public:
      CUserLoadKey(const CSpanKey& key,IntervalIndexType intervalIdx) :
         CSpanKey(key.spanIndex,key.girderIndex),m_IntervalIdx(intervalIdx) {}
      
         CUserLoadKey(const CUserLoadKey& rOther) :
         CSpanKey(rOther),m_IntervalIdx(rOther.m_IntervalIdx){}

      bool operator<(const CUserLoadKey& rOther) const
      {
         if ( CSpanKey::operator<(rOther) || 
            ( CSpanKey::IsEqual(rOther) && m_IntervalIdx < rOther.m_IntervalIdx) )
         {
            return true;
         }

         return false;
      }

   private:
      IntervalIndexType m_IntervalIdx;
   };

   // user defined loads, keyed by span/girder 
   std::map<CUserLoadKey,std::vector<UserPointLoad>> m_PointLoads;
   std::map<CUserLoadKey,std::vector<UserDistributedLoad>> m_DistributedLoads;
   std::map<CUserLoadKey,std::vector<UserMomentLoad>> m_MomentLoads;

   bool                        m_bUserLoadsValidated;

   std::map<Float64,Float64> m_LeftSlabEdgeOffset;
   std::map<Float64,Float64> m_RightSlabEdgeOffset;
   Float64 GetSlabEdgeOffset(std::map<Float64, Float64>& slabEdgeOffset, DirectionType side, Float64 Xb);
   Float64 GetCurbOffset(DirectionType side, Float64 Xb);

   // Stability Modeling
   std::map<CSegmentKey,stbGirder> m_StabilityModels;
   std::map<CSegmentKey,stbLiftingStabilityProblem> m_LiftingStabilityProblems;
   std::map<CSegmentKey,stbHaulingStabilityProblem> m_HaulingStabilityProblems;
   stbGirder m_DesignStabilityModel;
   stbLiftingStabilityProblem m_LiftingDesignStabilityProblem;
   stbHaulingStabilityProblem m_HaulingDesignStabilityProblem;
   void ConfigureSegmentStabilityModel(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,stbGirder* pGirder);
   void ConfigureSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD,stbLiftingStabilityProblem* problem);
   void ConfigureSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,ISegmentHaulingDesignPointsOfInterest* pPoiD,stbHaulingStabilityProblem* problem);

   void Invalidate( Uint16 level );
   Uint16 Validate( Uint16 level );
   bool BuildCogoModel();
   bool BuildBridgeModel();
   bool BuildGirder();
   void ValidateGirder();
   void ValidateElevationAdjustments(const CSegmentKey& segmentKey);

   // helper functions for building the bridge model
   bool LayoutPiers(const CBridgeDescription2* pBridgeDesc);
   bool LayoutGirders(const CBridgeDescription2* pBridgeDesc);
   void GetHaunchDepth(const CPrecastSegmentData* pSegment,Float64* pStartHaunch,Float64* pMidHaunch,Float64* pEndHaunch,Float64* pFillet);
   bool LayoutDeck(const CBridgeDescription2* pBridgeDesc);
   bool LayoutNoDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutSimpleDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutFullDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutCompositeCIPDeck(const CBridgeDescription2* pBridgeDesc,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck);
   bool LayoutCompositeSIPDeck(const CBridgeDescription2* pBridgeDesc,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck);

   bool LayoutTrafficBarriers(const CBridgeDescription2* pBridgeDesc);
   bool LayoutTrafficBarrier(const CBridgeDescription2* pBridgeDesc,const CRailingSystem* pRailingSystem,pgsTypes::TrafficBarrierOrientation orientation,ISidewalkBarrier** ppBarrier);
   void CreateBarrierObject(IBarrier** pBarrier, const TrafficBarrierEntry*  pBarrierEntry, pgsTypes::TrafficBarrierOrientation orientation);
   void LayoutDeckRebar(const CDeckDescription2* pDeck,IBridgeDeck* deck);
   void LayoutSegmentRebar(const CSegmentKey& segmentKey);
   void LayoutClosureJointRebar(const CClosureKey& closureKey);
   void UpdatePrestressing(GroupIndexType groupIdx,GirderIndexType girderIdx,SegmentIndexType segmentIdx);

   // functions for building the new CBridgeGeometry model
   bool BuildBridgeGeometryModel();

   // Functions to validate points of interest for precast segments
   void ValidatePointsOfInterest(const CGirderKey& girderKey);
   void LayoutPointsOfInterest(const CGirderKey& girderKey);
   void LayoutSpanPoi(const CSpanKey& spanKey,Uint16 nPnts);
   void LayoutRegularPoi(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 segmentOffset);
   void LayoutSpecialPoi(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutEndSizePoi(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutHarpingPointPoi(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutPrestressTransferAndDebondPoi(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutPoiForPrecastDiaphragmLoads(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutPoiForIntermediateDiaphragmLoads(const CSpanKey& spanKey);
   void LayoutPoiForShear(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutPoiForSlabBarCutoffs(const CGirderKey& girderKey);
   void LayoutPoiForSegmentBarCutoffs(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutPoiForHandling(const CSegmentKey& segmentKey);
   void LayoutPoiForSectionChanges(const CSegmentKey& segmentKey);
   void LayoutPoiForTemporarySupports(const CSegmentKey& segmentKey);
   void LayoutPoiForPiers(const CSegmentKey& segmentKey);
   void LayoutPoiForTendons(const CGirderKey& girderKey);
   void LayoutPoiForTendon(const CGirderKey& girderKey,const CLinearDuctGeometry& ductGeometry);
   void LayoutPoiForTendon(const CGirderKey& girderKey,const CParabolicDuctGeometry& ductGeometry);
   void LayoutPoiForTendon(const CGirderKey& girderKey,const COffsetDuctGeometry& ductGeometry);

   void ValidateSegmentOrientation(const CSegmentKey& segmentKey);

   void LayoutLiftingPoi(const CSegmentKey& segmentKey,Uint16 nPnts);
   void LayoutHaulingPoi(const CSegmentKey& segmentKey,Uint16 nPnts);
   void LayoutHandlingPoi(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey, Uint16 nPnts, PoiAttributeType attrib,pgsPoiMgr* pPoiMgr);
   void LayoutHandlingPoi(const CSegmentKey& segmentKey, Uint16 nPnts, Float64 leftOverhang, Float64 rightOverhang, PoiAttributeType attrib, PoiAttributeType supportAttribute,PoiAttributeType poiReference,pgsPoiMgr* pPoiMgr);

   void CheckBridge();

   Float64 GetHalfElevation(const pgsPointOfInterest& poi); // returns location of half height of composite girder

   void GetAlignment(IAlignment** ppAlignment);
   void GetProfile(IProfile** ppProfile);
   void GetBarrierProperties(pgsTypes::TrafficBarrierOrientation orientation,IShapeProperties** props);

   void InvalidateUserLoads();
   void ValidateUserLoads();
   void ValidatePointLoads();
   void ValidateDistributedLoads();
   void ValidateMomentLoads();

   ZoneIndexType GetPrimaryShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   const CShearZoneData2* GetPrimaryShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   ZoneIndexType GetPrimaryZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone);

   ZoneIndexType GetHorizInterfaceShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   const CHorizontalInterfaceZoneData* GetHorizInterfaceShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   ZoneIndexType GetHorizInterfaceZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone);

   Float64 GetPrimarySplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end, const CShearData2* pShearData);


   // Cache shear data - copies are expensive
   void InvalidateStirrupData();
   const CShearData2* GetShearData(const CSegmentKey& segmentKey);

   typedef std::map<CSegmentKey, CShearData2> ShearDataMap;
   typedef ShearDataMap::const_iterator ShearDataIterator;
   ShearDataMap  m_ShearData;

   bool m_bDeckParametersValidated;
   Float64 m_DeckSurfaceArea;
   Float64 m_DeckVolume;
   void ValidateDeckParameters();
   void InvalidateDeckParameters();

   HRESULT GetSlabOverhangs(Float64 distance,Float64* pLeft,Float64* pRight);
   Float64 ConvertSegmentToBridgeLineCoordinate(const CSegmentKey& segmentKey,Float64 Xs);
   HRESULT GetGirderSection(const pgsPointOfInterest& poi,pgsTypes::SectionCoordinateType csType,IGirderSection** gdrSection);
   HRESULT GetSuperstructureMember(const CGirderKey& girderKey,ISuperstructureMember* *ssmbr);
   HRESULT GetSegment(const CSegmentKey& segmentKey,ISuperstructureMemberSegment** segment);
   HRESULT GetGirder(const CSegmentKey& segmentKey,IPrecastGirder** girder);
   HRESULT GetGirder(const pgsPointOfInterest& poi,IPrecastGirder** girder);
   Float64 GetGrossSlabDepth();
   Float64 GetCastDepth();
   Float64 GetPanelDepth();
   Float64 GetSlabOverhangDepth();


   // Methods that return simple properties without data validation
   // Generally called during validation so as not to cause re-entry into the validation loop
   SpanIndexType GetSpanCount_Private();
   PierIndexType GetPierCount_Private();

   StrandIndexType GetNextNumStraightStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   StrandIndexType GetNextNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   StrandIndexType GetNextNumHarpedStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   StrandIndexType GetNextNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   StrandIndexType GetNextNumTempStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   StrandIndexType GetNextNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   StrandIndexType GetPrevNumStraightStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   StrandIndexType GetPrevNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   StrandIndexType GetPrevNumHarpedStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   StrandIndexType GetPrevNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   StrandIndexType GetPrevNumTempStrands(const CSegmentKey& segmentKeyr,StrandIndexType curNum);
   StrandIndexType GetPrevNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum);

   void GetSegmentShapeDirect(const pgsPointOfInterest& poi,IShape** ppShape);
   BarSize GetBarSize(matRebar::Size size);
   RebarGrade GetRebarGrade(matRebar::Grade grade);
   MaterialSpec GetRebarSpecification(matRebar::Type type);


   Float64 GetAsTensionSideOfGirder(const pgsPointOfInterest& poi,bool bDevAdjust,bool bTensionTop);
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust,bool bTensionTop, const GDRCONFIG* pConfig=nullptr);

   Float64 GetAptTensionSide(const pgsPointOfInterest& poi,bool bTensionTop);

   Float64 GetAsDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat);
   Float64 GetLocationDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat);
   void GetDeckMatData(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat,bool bAdjForDevLength,Float64* pAs,Float64* pYb);

   void GetShapeProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps);
   void GetShapeProperties(pgsTypes::SectionPropertyType sectPropType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps);
   Float64 GetCutLocation(const pgsPointOfInterest& poi);

   void NoDeckEdgePoint(GroupIndexType grpIdx,SegmentIndexType segIdx,pgsTypes::MemberEndType end,DirectionType side,IPoint2d** ppPoint);

   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint2d** point);
   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint3d** point);
   void GetCurbLinePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint2d** point);
   void GetCurbLinePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint3d** point);
   void CreateCompositeOverlayEdgePaths(const CBridgeDescription2* pBridgeDesc,IPath** ppLeftPath,IPath** ppRightPath);

   std::shared_ptr<mathFunction2d> CreateGirderProfile(const CSplicedGirderData* pGirder);
   std::shared_ptr<mathFunction2d> CreateGirderBottomFlangeProfile(const CSplicedGirderData* pGirder);
   std::shared_ptr<mathFunction2d> CreateGirderProfile(const CSplicedGirderData* pGirder,bool bGirderProfile);
   void GetSegmentRange(const CSegmentKey& segmentKey,Float64* pXStart,Float64* pXEnd);

   Float64 ConvertDuctOffsetToDuctElevation(const CGirderKey& girderKey,Float64 Xg,Float64 offset,CDuctGeometry::OffsetType offsetType);
   void CreateDuctCenterline(const CGirderKey& girderKey,const CLinearDuctGeometry& geometry,IPoint2dCollection** ppPoints);
   void CreateDuctCenterline(const CGirderKey& girderKey,const CParabolicDuctGeometry& geometry,IPoint2dCollection** ppPoints);
   void CreateDuctCenterline(const CGirderKey& girderKey,const COffsetDuctGeometry& geometry,IPoint2dCollection** ppPoints);

   mathCompositeFunction2d CreateDuctCenterline(const CGirderKey& girderKey,const CLinearDuctGeometry& geometry);
   mathCompositeFunction2d CreateDuctCenterline(const CGirderKey& girderKey,const CParabolicDuctGeometry& geometry);

   SegmentIndexType GetSegmentIndex(const CSplicedGirderData* pGirder,Float64 Xb);
   SegmentIndexType GetSegmentIndex(const CGirderKey& girderKey,ILine2d* pLine,IPoint2d** ppIntersection);

   SpanIndexType GetSpanIndex(Float64 Xb);
   PierIndexType GetGenericBridgePierIndex(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType);
   void GetGenericBridgePier(PierIndexType pierIdx,IBridgePier** ppPier);
   void GetGirderLine(const CSegmentKey& segmentKey,IGirderLine** ppGirderLine);
   void GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey,ISuperstructureMemberSegment** ppSegment);
   void GetPierLine(PierIndexType pierIdx,IPierLine** ppPierLine);
   void GetTemporarySupportLine(SupportIndexType tsIdx,IPierLine** ppPierLine);

   const GirderLibraryEntry* GetGirderLibraryEntry(const CGirderKey& girderKey);
   GroupIndexType GetGirderGroupAtPier(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace);

   void CreateTendons(const CBridgeDescription2* pBridgeDesc,const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,ITendonCollection** ppTendons);
   void CreateParabolicTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const CParabolicDuctGeometry& ductGeometry,ITendonCollection** ppTendons);
   void CreateLinearTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const CLinearDuctGeometry& ductGeometry,ITendonCollection** ppTendons);
   void CreateOffsetTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const COffsetDuctGeometry& ductGeometry,ITendonCollection* tendons,ITendonCollection** ppTendons);

   void CreateStrandMover(LPCTSTR strGirderName,Float64 Hg,pgsTypes::AdjustableStrandType adjType,IStrandMover** ppStrandMover);

   INCREMENTALRELAXATIONDETAILS GetIncrementalRelaxationDetails(Float64 fpi,const matPsStrand* pStrand,Float64 tStart,Float64 tEnd,Float64 tStress);

   // Returns the geometric CG location of the strands measured in Girder Coordinates
   Float64 GetHsLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   Float64 GetSsLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   Float64 GetTempLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);

   Float64 GetHsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG*pConfig, Float64* nEffectiveStrands);
   Float64 GetSsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, Float64* nEffectiveStrands);
   Float64 GetTempEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, Float64* nEffectiveStrands);

   Float64 GetSuperstructureDepth(PierIndexType pierIdx);

   bool GirderLineIntersect(const CGirderKey& girderKey,ILine2d* pLine,SegmentIndexType segIdxHint,SegmentIndexType* pSegIdx,Float64* pXs);
   bool SegmentLineIntersect(const CSegmentKey& segmentKey,ILine2d* pLine,Float64* pXs);

   void ComputeHpFill(const GirderLibraryEntry* pGdrEntry,IStrandGridFiller* pStrandGridFiller, IIndexArray* pFill, IIndexArray** ppHPfill);

   Float64 ComputePierDiaphragmHeight(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace);
   Float64 ComputePierDiaphragmWidth(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace);

   REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(const CSegmentKey& segmentKey, IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct);
};

#endif //__BRIDGEAGENT_H_
