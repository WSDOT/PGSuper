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

// BridgeAgentImp.h : Declaration of the CBridgeAgentImp

#ifndef __BRIDGEAGENT_H_
#define __BRIDGEAGENT_H_

#include "CLSID.h"

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
#include "BridgeGeometryModelBuilder.h"

#include <memory>

#include <Math\PolynomialFunction.h>
#include <Math\CompositeFunction.h>
#include <Math\LinearFunction.h>

#include "StatusItems.h"
#include "ConcreteManager.h"
#include "IntervalManager.h"

#if defined _USE_MULTITHREADING
#include <PgsExt\ThreadManager.h>
#endif

class gmTrafficBarrier;

namespace WBFL
{
   namespace Materials
   {
      class PsStrand;
   };
};

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
   public IGirderTendonGeometry,
   public ISegmentTendonGeometry,
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
   COM_INTERFACE_ENTRY(IGirderTendonGeometry)
   COM_INTERFACE_ENTRY(ISegmentTendonGeometry)
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
   StatusCallbackIDType m_scidConnectionGeometryWarning;

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
   virtual void GetStartPoint(Float64 n,Float64* pStartStation,Float64* pStartElevation,Float64* pGrade,IPoint2d** ppPoint) const override;
   virtual void GetEndPoint(Float64 n,Float64* pEndStation,Float64* pEndlevation,Float64* pGrade,IPoint2d** ppPoint) const override;
   virtual Float64 GetSlope(Float64 station,Float64 offset) const override;
   virtual Float64 GetProfileGrade(Float64 station) const override;
   virtual Float64 GetElevation(Float64 station,Float64 offset) const override;
   virtual void GetBearing(Float64 station,IDirection** ppBearing) const override;
   virtual void GetBearingNormal(Float64 station,IDirection** ppNormal) const override;
   virtual void GetPoint(Float64 station,Float64 offset,IDirection* pBearing,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const override;
   virtual void GetStationAndOffset(pgsTypes::PlanCoordinateType pcType,IPoint2d* point,Float64* pStation,Float64* pOffset) const override;
   virtual CollectionIndexType GetCurveCount() const override;
   virtual void GetCurve(CollectionIndexType idx, pgsTypes::PlanCoordinateType pcType,ICompoundCurve** ppCurve) const override;
   virtual HCURVESTATIONS GetCurveStations(IndexType hcIdx) const override;
   virtual CollectionIndexType GetVertCurveCount() const override;
   virtual void GetVertCurve(CollectionIndexType idx,IVerticalCurve** ppCurve) const override;
   virtual void GetRoadwaySurface(Float64 station,IAngle* pSkewAngle, IPoint2dCollection** ppPoints) const override;
   virtual IndexType GetCrownPointIndexCount(Float64 station) const override;
   virtual IndexType GetAlignmentPointIndex(Float64 station) const override;
   virtual Float64 GetAlignmentOffset(IndexType crownPointIdx, Float64 station) const override;
   virtual IndexType GetProfileGradeLineIndex(Float64 station) const override;
   virtual Float64 GetProfileGradeLineOffset(IndexType crownPointIdx, Float64 station) const override;

// IGeometry
public:
   virtual HRESULT Angle(IPoint2d* from,IPoint2d* vertex,IPoint2d* to,IAngle** angle) const override;
   virtual HRESULT Area(IPoint2dCollection* points,Float64* area) const override;
   virtual HRESULT Distance(IPoint2d* from,IPoint2d* to,Float64* dist) const override;
   virtual HRESULT Direction(IPoint2d* from,IPoint2d* to,IDirection** dir) const override;
   virtual HRESULT Inverse(IPoint2d* from,IPoint2d* to,Float64* dist,IDirection** dir) const override;
   virtual HRESULT ByDistAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varAngle,Float64 offset,IPoint2d** point) const override;
   virtual HRESULT ByDistDefAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varDefAngle,Float64 offset,IPoint2d** point) const override;
   virtual HRESULT ByDistDir(IPoint2d* from,Float64 dist,VARIANT varDir,Float64 offset,IPoint2d** point) const override;
   virtual HRESULT PointOnLine(IPoint2d* from,IPoint2d* to,Float64 dist,Float64 offset,IPoint2d** point) const override;
   virtual HRESULT ParallelLineByPoints(IPoint2d* from,IPoint2d* to,Float64 offset,IPoint2d** p1,IPoint2d** p2) const override;
   virtual HRESULT ParallelLineSegment(ILineSegment2d* ls,Float64 offset,ILineSegment2d** linesegment) const override;
   virtual HRESULT Bearings(IPoint2d* p1,VARIANT varDir1,Float64 offset1,IPoint2d* p2,VARIANT varDir2,Float64 offset2,IPoint2d** point) const override;
   virtual HRESULT BearingCircle(IPoint2d* p1,VARIANT varDir,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point) const override;
   virtual HRESULT Circles(IPoint2d* p1,Float64 r1,IPoint2d* p2,Float64 r2,IPoint2d* nearest,IPoint2d** point) const override;
   virtual HRESULT LineByPointsCircle(IPoint2d* p1,IPoint2d* p2,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point) const override;
   virtual HRESULT LinesByPoints(IPoint2d* p11,IPoint2d* p12,Float64 offset1,IPoint2d* p21,IPoint2d* p22,Float64 offset2,IPoint2d** point) const override;
   virtual HRESULT Lines(ILineSegment2d* l1,Float64 offset1,ILineSegment2d* l2,Float64 offset2,IPoint2d** point) const override;
   virtual HRESULT LineSegmentCircle(ILineSegment2d* pSeg,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest, IPoint2d** point) const override;
   virtual HRESULT PointOnLineByPoints(IPoint2d* pnt,IPoint2d* start,IPoint2d* end,Float64 offset,IPoint2d** point) const override;
   virtual HRESULT PointOnLineSegment(IPoint2d* from,ILineSegment2d* seg,Float64 offset,IPoint2d** point) const override;
   virtual HRESULT Arc(IPoint2d* from, IPoint2d* vertex, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points) const override;
   virtual HRESULT BetweenPoints(IPoint2d* from, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points) const override;
   virtual HRESULT LineSegment(ILineSegment2d* seg,CollectionIndexType nParts,IPoint2dCollection** points) const override;
	virtual HRESULT CompoundCurve(ICompoundCurve* curve, CollectionIndexType nParts, IPoint2dCollection** points) const override;
   virtual HRESULT Path(IPath* pPath,CollectionIndexType nParts,Float64 start,Float64 end,IPoint2dCollection** points) const override;
   virtual HRESULT External(IPoint2d* center1, Float64 radius1,IPoint2d* center2,Float64 radius2,TangentSignType sign, IPoint2d** t1,IPoint2d** t2) const override;
   virtual HRESULT Cross(IPoint2d* center1, Float64 radius1,IPoint2d* center2, Float64 radius2, TangentSignType sign, IPoint2d** t1,IPoint2d** t2) const override;
   virtual HRESULT Point(IPoint2d* center, Float64 radius,IPoint2d* point, TangentSignType sign, IPoint2d** tangent) const override;

// IBridge
public:
   virtual bool IsAsymmetricGirder(const CGirderKey& girderKey) const override;
   virtual bool HasAsymmetricGirders() const override;
   virtual bool HasAsymmetricPrestressing() const override;
   virtual bool HasTiltedGirders() const override;
   virtual Float64 GetLength() const override;
   virtual Float64 GetSpanLength(SpanIndexType span) const override;
   virtual Float64 GetGirderLayoutLength(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderSpanLength(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderLength(const CGirderKey& girderKey) const override;
   virtual Float64 GetAlignmentOffset() const override;
   virtual SpanIndexType GetSpanCount() const override;
   virtual PierIndexType GetPierCount() const override;
   virtual SupportIndexType GetTemporarySupportCount() const override;
   virtual GroupIndexType GetGirderGroupCount() const override;
   virtual GirderIndexType GetGirderCount(GroupIndexType grpIdx) const override;
   virtual GirderIndexType GetGirderlineCount() const override;
   virtual void GetGirderline(GirderIndexType gdrLineIdx, std::vector<CGirderKey>* pvGirderKeys) const override;
   virtual void GetGirderline(GirderIndexType gdrLineIdx, GroupIndexType startGroupIdx, GroupIndexType endGroupIdx, std::vector<CGirderKey>* pvGirderKeys) const override;
   virtual void GetGirderline(const CGirderKey& girderKey, std::vector<CGirderKey>* pvGirderKeys) const override;
   virtual GirderIndexType GetGirderCountBySpan(SpanIndexType spanIdx) const override;
   virtual SegmentIndexType GetSegmentCount(const CGirderKey& girderKey) const override;
   virtual SegmentIndexType GetSegmentCount(GroupIndexType grpIdx,GirderIndexType gdrIdx) const override;
   virtual PierIndexType GetGirderGroupStartPier(GroupIndexType grpIdx) const override;
   virtual PierIndexType GetGirderGroupEndPier(GroupIndexType grpIdx) const override;
   virtual void GetGirderGroupPiers(GroupIndexType grpIdx,PierIndexType* pStartPierIdx,PierIndexType* pEndPierIdx) const override;
   virtual SpanIndexType GetGirderGroupStartSpan(GroupIndexType grpIdx) const override;
   virtual SpanIndexType GetGirderGroupEndSpan(GroupIndexType grpIdx) const override;
   virtual void GetGirderGroupSpans(GroupIndexType grpIdx,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) const override;
   virtual GroupIndexType GetGirderGroupIndex(SpanIndexType spanIdx) const override;
   virtual void GetGirderGroupIndex(PierIndexType pierIdx,GroupIndexType* pBackGroupIdx,GroupIndexType* pAheadGroupIdx) const override;
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight) const override;
   virtual void GetBottomFlangeClearance(const pgsPointOfInterest& poi,Float64* pLeft,Float64* pRight) const override;
   virtual std::vector<Float64> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType) const override;
   virtual std::vector<Float64> CBridgeAgentImp::GetGirderSpacingAtTemporarySupport(SupportIndexType tsIdx, pgsTypes::PierFaceType pierFace, pgsTypes::MeasurementLocation measureLocation, pgsTypes::MeasurementType measureType) const override;
   virtual Float64 GetGirderOffset(GirderIndexType gdrIdx,PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::OffsetMeasurementType offsetMeasureDatum) const override;
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(Float64 station) const override;
   virtual std::vector<Float64> GetGirderSpacing(SpanIndexType spanIdx,Float64 Xspan) const override;
   virtual void GetSpacingAlongGirder(const CGirderKey& girderKey,Float64 Xg,Float64* leftSpacing,Float64* rightSpacing) const override;
   virtual void GetSpacingAlongGirder(const pgsPointOfInterest& poi,Float64* leftSpacing,Float64* rightSpacing) const override;
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacingAtBottomClGirder(Float64 station) const override;
   virtual std::vector<std::pair<SegmentIndexType,Float64>> GetSegmentLengths(const CSpanKey& spanKey) const override;
   virtual Float64 GetSegmentLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentSpanLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentLayoutLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentFramingLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentPlanLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentSlope(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSlabOffset(const CSegmentKey& segmentKey, pgsTypes::MemberEndType end) const override;
   virtual void GetSlabOffset(const CSegmentKey& segmentKey, Float64* pStart, Float64* pEnd) const override;
   virtual Float64 GetElevationAdjustment(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetRotationAdjustment(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetSpanLength(SpanIndexType spanIdx, GirderIndexType gdrIdx) const override;
   virtual Float64 GetSpanLength(const CSpanKey& spanKey) const override;
   virtual Float64 GetFullSpanLength(const CSpanKey& spanKey) const override;
   virtual Float64 GetGirderlineLength(GirderIndexType gdrLineIdx) const override;
   virtual Float64 GetCantileverLength(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetCantileverLength(const CSpanKey& spanKey,pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetSegmentStartEndDistance(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentEndEndDistance(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentStartBearingOffset(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentEndBearingOffset(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentStartSupportWidth(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentEndSupportWidth(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetCLPierToCLBearingDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) const override;
   virtual Float64 GetCLPierToSegmentEndDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) const override;
   virtual Float64 GetSegmentOffset(const CSegmentKey& segmentKey,Float64 station) const override;
   virtual void GetSegmentAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) const override;
   virtual void GetSegmentSkewAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) const override;
   virtual void GetSegmentBearing(const CSegmentKey& segmentKey,IDirection** ppBearing) const override;
   virtual void GetSegmentNormal(const CSegmentKey& segmentKey, IDirection** ppNormal) const override;
   virtual CSegmentKey GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey) const override;
   virtual void GetSegmentsAtPier(PierIndexType pierIdx, GirderIndexType gdrIdx, CSegmentKey* pBackSegmentKey, CSegmentKey* pAheadSegmentKey) const override;
   virtual void GetSpansForSegment(const CSegmentKey& segmentKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) const override;
   virtual void ResolveSegmentVariation(const CPrecastSegmentData* pSegment, std::array<Float64, 4>& Xhp) const override;
   virtual GDRCONFIG GetSegmentConfiguration(const CSegmentKey& segmentKey) const override;
   virtual void ModelCantilevers(const CSegmentKey& segmentKey,bool* pbLeftCantilever,bool* pbRightCantilever) const override;
   virtual void ModelCantilevers(const CSegmentKey& segmentKey, Float64 leftSupportDistance, Float64 rightSupportDistance, bool* pbLeftCantilever, bool* pbRightCantilever) const override;
   virtual bool GetSpan(Float64 station,SpanIndexType* pSpanIdx) const override;
   virtual void GetPoint(const CSegmentKey& segmentKey,Float64 Xpoi,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const override;
   virtual void GetPoint(const pgsPointOfInterest& poi,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const override;
   virtual bool GetSegmentPierIntersection(const CSegmentKey& segmentKey,PierIndexType pierIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const override;
   virtual bool GetSegmentTempSupportIntersection(const CSegmentKey& segmentKey,SupportIndexType tsIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const override;
   virtual void GetStationAndOffset(const CSegmentKey& segmentKey,Float64 Xpoi,Float64* pStation,Float64* pOffset) const override;
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset) const override;
   virtual bool IsInteriorGirder(const CGirderKey& girderKey) const override;
   virtual bool IsExteriorGirder(const CGirderKey& girderKey) const override;
   virtual bool IsLeftExteriorGirder(const CGirderKey& girderKey) const override;
   virtual bool IsRightExteriorGirder(const CGirderKey& girderKey) const override;
   virtual bool IsObtuseCorner(const CSpanKey& spanKey,pgsTypes::MemberEndType endType) const override;
   virtual bool AreGirderTopFlangesRoughened(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetClosureJointLength(const CClosureKey& closureKey) const override;
   virtual void GetClosureJointSize(const CClosureKey& closureKey,Float64* pLeft,Float64* pRight) const override;
   virtual void GetAngleBetweenSegments(const CClosureKey& closureKey,IAngle** ppAngle) const override;
   virtual void GetPierDiaphragmSize(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,Float64* pW,Float64* pH) const override;
   virtual bool DoesPierDiaphragmLoadGirder(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const override;
   virtual Float64 GetPierDiaphragmLoadLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endTYpe) const override;
   virtual std::vector<IntermedateDiaphragm> GetPrecastDiaphragms(const CSegmentKey& segmentKey) const override;
   virtual std::vector<IntermedateDiaphragm> GetCastInPlaceDiaphragms(const CSpanKey& spanKey) const override;
   virtual pgsTypes::SupportedDeckType GetDeckType() const override;
   virtual pgsTypes::WearingSurfaceType GetWearingSurfaceType() const override;
   virtual bool IsCompositeDeck() const override;
   virtual bool HasOverlay() const override;
   virtual bool IsFutureOverlay() const override;
   virtual Float64 GetOverlayWeight() const override;
   virtual Float64 GetOverlayDepth(IntervalIndexType interval) const override;
   virtual Float64 GetSacrificalDepth() const override;
   virtual Float64 GetFillet() const override;
   virtual Float64 GetAssumedExcessCamber(SpanIndexType spanIdx,GirderIndexType gdr) const override;
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi) const override;
   virtual pgsTypes::HaunchInputDepthType GetHaunchInputDepthType() const override;
   virtual Float64 GetLeftSlabEdgeOffset(Float64 Xb) const override;
   virtual Float64 GetRightSlabEdgeOffset(Float64 Xb) const override;
   virtual Float64 GetLeftSlabOverhang(Float64 Xb) const override;
   virtual Float64 GetRightSlabOverhang(Float64 Xb) const override;
   virtual Float64 GetLeftSlabOverhang(SpanIndexType spanIdx,Float64 Xspan) const override;
   virtual Float64 GetRightSlabOverhang(SpanIndexType spanIdx,Float64 Xspan) const override;
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pierIdx) const override;
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pierIdx) const override;
   virtual Float64 GetLeftCurbOffset(Float64 Xb) const override;
   virtual Float64 GetRightCurbOffset(Float64 Xb) const override;
   virtual Float64 GetLeftCurbOffset(SpanIndexType spanIdx,Float64 Xspan) const override;
   virtual Float64 GetRightCurbOffset(SpanIndexType spanIdx,Float64 Xspan) const override;
   virtual Float64 GetLeftCurbOffset(PierIndexType pierIdx) const override;
   virtual Float64 GetRightCurbOffset(PierIndexType pierIdx) const override;
   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetCurbToCurbWidth(const CSegmentKey& segmentKey,Float64 Xs) const override;
   virtual Float64 GetCurbToCurbWidth(Float64 Xb) const override;
   virtual Float64 GetLeftInteriorCurbOffset(Float64 Xb) const override;
   virtual Float64 GetRightInteriorCurbOffset(Float64 Xb) const override;
   virtual Float64 GetLeftInteriorCurbOffset(PierIndexType pierIdx) const override;
   virtual Float64 GetRightInteriorCurbOffset(PierIndexType pierIdx) const override;
   virtual Float64 GetInteriorCurbToCurbWidth(Float64 Xb) const override;
   virtual Float64 GetLeftOverlayToeOffset(Float64 Xb) const override;
   virtual Float64 GetRightOverlayToeOffset(Float64 Xb) const override;
   virtual Float64 GetLeftOverlayToeOffset(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetRightOverlayToeOffset(const pgsPointOfInterest& poi) const override;
   virtual void GetSlabPerimeter(CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) const override;
   virtual void GetSlabPerimeter(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) const override;
   virtual void GetSlabPerimeter(PierIndexType startPierIdx, Float64 Xstart, PierIndexType endPierIdx, Float64 Xend, CollectionIndexType nPoints, pgsTypes::PlanCoordinateType pcType, IPoint2dCollection** points) const override;
   virtual void GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) const override;
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const override;
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const override;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const override;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const override;
   virtual void GetLeftCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const override;
   virtual void GetLeftCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const override;
   virtual void GetRightCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const override;
   virtual void GetRightCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const override;
   virtual Float64 GetRoadwayToTopGirderChordDistance(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTopSlabToTopGirderChordDistance(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTopSlabToTopGirderChordDistance(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) const override;
   virtual IndexType GetDeckCastingRegionCount() const override;
   virtual void GetDeckCastingRegionLimits(IndexType regionIdx, PierIndexType* pStartPierIdx, Float64* pXstart, PierIndexType* pEndPierIdx, Float64* pXend, CCastingRegion::RegionType* pRegionType, IndexType* pSequenceIdx, const CCastDeckActivity* pActivity = nullptr) const override;
   virtual void GetDeckCastingRegionPerimeter(IndexType regionIdx, IndexType nPoints, pgsTypes::PlanCoordinateType pcType, CCastingRegion::RegionType* pRegionType, IndexType* pSequenceIdx, const CCastDeckActivity* pActivity, IPoint2dCollection** ppPoints) const override;
   virtual void GetDeckCastingRegionPerimeter(IndexType regionIdx, SpanIndexType startSpanIdx, SpanIndexType endSpanIdx, IndexType nPoints, pgsTypes::PlanCoordinateType pcType, CCastingRegion::RegionType* pRegionType, IndexType* pSequenceIdx, const CCastDeckActivity* pActivity, IPoint2dCollection** ppPoints) const override;
   virtual Float64 GetPierStation(PierIndexType pierIdx) const override;
   virtual Float64 GetBearingStation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const override;
   virtual void GetWorkingPointLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,const CGirderKey& girderKey,Float64* pStation,Float64* pOffset) const override;
   virtual void GetPierDirection(PierIndexType pierIdx,IDirection** ppDirection) const override;
   virtual void GetPierSkew(PierIndexType pierIdx,IAngle** ppAngle) const override;
   virtual void GetPierPoints(PierIndexType pierIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right) const override;
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) const override;
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) const override;
   virtual bool GetPierLocation(PierIndexType pierIdx,const CSegmentKey& segmentKey,Float64* pXs) const override;
   virtual bool GetPierLocation(const CGirderKey& girderKey,PierIndexType pierIdx,Float64* pXgp) const override;
   virtual bool GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew) const override;
   virtual pgsTypes::PierModelType GetPierModelType(PierIndexType pierIdx) const override;
   virtual ColumnIndexType GetColumnCount(PierIndexType pierIdx) const override;
   virtual void GetColumnProperties(PierIndexType pierIdx,ColumnIndexType colIdx,bool bSkewAdjust,Float64* pHeight,Float64* pA,Float64* pI) const override;
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx) const override;
   virtual pgsTypes::BoundaryConditionType GetBoundaryConditionType(PierIndexType pierIdx) const override;
   virtual pgsTypes::PierSegmentConnectionType GetPierSegmentConnectionType(PierIndexType pierIdx) const override;
   virtual bool IsAbutment(PierIndexType pierIdx) const override;
   virtual bool IsPier(PierIndexType pierIdx) const override;
   virtual bool IsInteriorPier(PierIndexType pierIdx) const override;
   virtual bool IsBoundaryPier(PierIndexType pierIdx) const override;
   virtual void GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx,SpanIndexType* pSpanIdx,Float64* pXspan) const override;
   virtual bool GetTemporarySupportLocation(SupportIndexType tsIdx,const CSegmentKey& segmentKey,Float64* pXs) const override;
   virtual Float64 GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx) const override;
   virtual pgsTypes::TemporarySupportType GetTemporarySupportType(SupportIndexType tsIdx) const override;
   virtual Float64 GetTemporarySupportStation(SupportIndexType tsIdx) const override;
   virtual pgsTypes::TempSupportSegmentConnectionType GetSegmentConnectionTypeAtTemporarySupport(SupportIndexType tsIdx) const override;
   virtual void GetSegmentsAtTemporarySupport(GirderIndexType gdrIdx,SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey) const override;
   virtual void GetTemporarySupportDirection(SupportIndexType tsIdx,IDirection** ppDirection) const override;
   virtual bool HasTemporarySupportElevationAdjustments() const override;
   virtual std::vector<BearingElevationDetails> GetBearingElevationDetails(PierIndexType pierIdx,pgsTypes::PierFaceType face,GirderIndexType gdrIdx, bool bIgnoreUnrecoverableDeformations) const override;
   virtual std::vector<BearingElevationDetails> GetBearingElevationDetailsAtGirderEdges(PierIndexType pierIdx,pgsTypes::PierFaceType face,GirderIndexType gdrIdx) const override;
   virtual void GetPierDisplaySettings(pgsTypes::DisplayEndSupportType* pStartPierType, pgsTypes::DisplayEndSupportType* pEndPierType, PierIndexType* pStartPierNumber) const override;

// IMaterials
public:
   virtual Float64 GetSegmentFc28(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetClosureJointFc28(const CSegmentKey& closureKey) const override;
   virtual Float64 GetDeckFc28() const override;
   virtual Float64 GetRailingSystemFc28(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetPierFc28(IndexType pierIdx) const override;
   virtual Float64 GetLongitudinalJointFc28() const override;

   virtual Float64 GetSegmentEc28(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetClosureJointEc28(const CSegmentKey& closureKey) const override;
   virtual Float64 GetDeckEc28() const override;
   virtual Float64 GetRailingSystemEc28(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetPierEc28(IndexType pierIdx) const override;
   virtual Float64 GetLongitudinalJointEc28() const override;

   virtual Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetClosureJointWeightDensity(const CClosureKey& closureKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetDeckWeightDensity(IndexType castingRegionIdx,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetDiaphragmWeightDensity(IntervalIndexType intervalIdx) const override;
   virtual Float64 GetRailingSystemWeightDensity(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetLongitudinalJointWeightDensity(IntervalIndexType intervalIdx) const override;

   virtual Float64 GetSegmentConcreteAge(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetClosureJointConcreteAge(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetDeckConcreteAge(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetRailingSystemAge(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetLongitudinalJointConcreteAge(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType) const override;

   virtual Float64 GetSegmentFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetClosureJointFc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetDeckFc(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetLongitudinalJointFc(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const override;

   virtual Float64 GetSegmentDesignFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetClosureJointDesignFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetDeckDesignFc(IntervalIndexType intervalIdx) const override;

   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) const override;
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) const override;
   virtual Float64 GetDeckEc(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetLongitudinalJointEc(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const override;

   virtual Float64 GetSegmentLambda(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetClosureJointLambda(const CClosureKey& closureKey) const override;
   virtual Float64 GetDeckLambda() const override;
   virtual Float64 GetRailingSystemLambda(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetLongitudinalJointLambda() const override;

   virtual Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetClosureJointFlexureFr(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetClosureJointShearFr(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetDeckFlexureFr(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetDeckShearFr(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const override;
   virtual Float64 GetLongitudinalJointFlexureFr(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const override;
   virtual Float64 GetLongitudinalJointShearFr(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const override;

   virtual Float64 GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetClosureJointAgingCoefficient(const CClosureKey& closureKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetDeckAgingCoefficient(IndexType castingRegionIdx, IntervalIndexType intervalIdx) const override;
   virtual Float64 GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetLongitudinalJointAgingCoefficient(IntervalIndexType intervalIdx) const override;

   virtual Float64 GetSegmentAgeAdjustedEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetClosureJointAgeAdjustedEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetDeckAgeAdjustedEc(IndexType castingRegionIdx, IntervalIndexType intervalIdx) const override;
   virtual Float64 GetRailingSystemAgeAdjustedEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetLongitudinalJointAgeAdjustedEc(IntervalIndexType intervalIdx) const override;

   virtual Float64 GetTotalSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const override;
   virtual Float64 GetTotalClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const override;
   virtual Float64 GetTotalDeckFreeShrinkageStrain(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const override;
   virtual Float64 GetTotalRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetTotalLongitudinalJointFreeShrinkageStrain(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time) const override;

   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetTotalSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetTotalClosureJointFreeShrinkageStrainDetails(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetTotalDeckFreeShrinkageStrainDetails(IndexType castingRegionIdx, IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetTotalRailingSystemFreeShrinakgeStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetTotalLongitudinalJointFreeShrinkageStrainDetails(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time) const override;

   virtual Float64 GetIncrementalSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetIncrementalClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetIncrementalDeckFreeShrinkageStrain(IndexType castingRegionIdx, IntervalIndexType intervalIdx) const override;
   virtual Float64 GetIncrementalRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const override;
   virtual Float64 GetIncrementalLongitudinalJointFreeShrinkageStrain(IntervalIndexType intervalIdx) const override;

   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalClosureJointFreeShrinkageStrainDetails(const CClosureKey& closureKey,IntervalIndexType intervalIdx) const override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalDeckFreeShrinkageStrainDetails(IndexType castingRegionIdx, IntervalIndexType intervalIdx) const override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalRailingSystemFreeShrinakgeStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const override;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalLongitudinalJointFreeShrinkageStrainDetails(IntervalIndexType intervalIdx) const override;

   virtual Float64 GetSegmentAutogenousShrinkage(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetClosureJointAutogenousShrinkage(const CClosureKey& closureKey) const override;
   virtual Float64 GetDeckAutogenousShrinkage() const override;

   virtual Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetClosureJointCreepCoefficient(const CClosureKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetDeckCreepCoefficient(IndexType castingRegionIdx, IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType) const override;
   virtual Float64 GetLongitudinalJointCreepCoefficient(IntervalIndexType loadingIntervalIdx, pgsTypes::IntervalTimeType loadingTimeType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType) const override;

   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetSegmentCreepCoefficientDetails(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetClosureJointCreepCoefficientDetails(const CClosureKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetDeckCreepCoefficientDetails(IndexType castingRegionIdx, IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetRailingSystemCreepCoefficientDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const override;
   virtual std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetLongitudinalJointCreepCoefficientDetails(IntervalIndexType loadingIntervalIdx, pgsTypes::IntervalTimeType loadingTimeType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType) const override;

   virtual pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey) const override;
   virtual bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteFiberLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentEccK1(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentEccK2(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey) const override;
   virtual const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetSegmentConcrete(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteFirstCrackingStrength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteInitialEffectiveCrackingStrength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteDesignEffectiveCrackingStrength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteCrackLocalizationStrength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteCrackLocalizationStrain(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentConcreteFiberOrientationReductionFactor(const CSegmentKey& segmentKey) const override;

   virtual pgsTypes::ConcreteType GetClosureJointConcreteType(const CClosureKey& closureKey) const override;
   virtual bool DoesClosureJointConcreteHaveAggSplittingStrength(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointConcreteAggSplittingStrength(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointStrengthDensity(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointMaxAggrSize(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointConcreteFiberLength(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointEccK1(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointEccK2(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointCreepK1(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointCreepK2(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointShrinkageK1(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointShrinkageK2(const CClosureKey& closureKey) const override;
   virtual const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetClosureJointConcrete(const CClosureKey& closureKey) const override;

   virtual pgsTypes::ConcreteType GetDeckConcreteType() const override;
   virtual bool DoesDeckConcreteHaveAggSplittingStrength() const override;
   virtual Float64 GetDeckConcreteAggSplittingStrength() const override;
   virtual Float64 GetDeckMaxAggrSize() const override;
   virtual Float64 GetDeckStrengthDensity() const override;
   virtual Float64 GetDeckEccK1() const override;
   virtual Float64 GetDeckEccK2() const override;
   virtual Float64 GetDeckCreepK1() const override;
   virtual Float64 GetDeckCreepK2() const override;
   virtual Float64 GetDeckShrinkageK1() const override;
   virtual Float64 GetDeckShrinkageK2() const override;
   virtual const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetDeckConcrete(IndexType castingRegionIdx) const override;


   virtual pgsTypes::ConcreteType GetLongitudinalJointConcreteType() const override;
   virtual bool DoesLongitudinalJointConcreteHaveAggSplittingStrength() const override;
   virtual Float64 GetLongitudinalJointConcreteAggSplittingStrength() const override;
   virtual Float64 GetLongitudinalJointStrengthDensity() const override;
   virtual Float64 GetLongitudinalJointMaxAggrSize() const override;
   virtual Float64 GetLongitudinalJointEccK1() const override;
   virtual Float64 GetLongitudinalJointEccK2() const override;
   virtual Float64 GetLongitudinalJointCreepK1() const override;
   virtual Float64 GetLongitudinalJointCreepK2() const override;
   virtual Float64 GetLongitudinalJointShrinkageK1() const override;
   virtual Float64 GetLongitudinalJointShrinkageK2() const override;
   virtual const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetLongitudinalJointConcrete() const override;

   virtual const WBFL::Materials::PsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;
   virtual Float64 GetIncrementalStrandRelaxation(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType) const override;
   virtual INCREMENTALRELAXATIONDETAILS GetIncrementalStrandRelaxationDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType) const override;
   virtual const WBFL::Materials::PsStrand* GetGirderTendonMaterial(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonIncrementalRelaxation(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso) const override;
   virtual INCREMENTALRELAXATIONDETAILS GetGirderTendonIncrementalRelaxationDetails(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso) const override;
   virtual const WBFL::Materials::PsStrand* GetSegmentTendonMaterial(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonIncrementalRelaxation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, IntervalIndexType intervalIdx, Float64 fpso) const override;
   virtual INCREMENTALRELAXATIONDETAILS GetSegmentTendonIncrementalRelaxationDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx, IntervalIndexType intervalIdx, Float64 fpso) const override;
   virtual void GetSegmentLongitudinalRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) const override;
   virtual std::_tstring GetSegmentLongitudinalRebarName(const CSegmentKey& segmentKey) const override;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade) const override;
   virtual void GetClosureJointLongitudinalRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) const override;
   virtual std::_tstring GetClosureJointLongitudinalRebarName(const CClosureKey& closureKey) const override;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade) const override;
   virtual void GetSegmentTransverseRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) const override;
   virtual void GetSegmentTransverseRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade) const override;
   virtual std::_tstring GetSegmentTransverseRebarName(const CSegmentKey& segmentKey) const override;
   virtual void GetClosureJointTransverseRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) const override;
   virtual void GetClosureJointTransverseRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade) const override;
   virtual std::_tstring GetClosureJointTransverseRebarName(const CClosureKey& closureKey) const override;
   virtual void GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu) const override;
   virtual std::_tstring GetDeckRebarName() const override;
   virtual void GetDeckRebarMaterial(WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade) const override;
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) const override;
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) const override;
   virtual Float64 GetSegmentFlexureFrCoefficient(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentShearFrCoefficient(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetClosureJointFlexureFrCoefficient(const CClosureKey& closureKey) const override;
   virtual Float64 GetClosureJointShearFrCoefficient(const CClosureKey& closureKey) const override;
   virtual Float64 GetEconc(pgsTypes::ConcreteType type, Float64 fc,Float64 density,Float64 K1,Float64 K2) const override;
   virtual bool HasUHPC() const override;

// ILongRebarGeometry
public:
   virtual void GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection) const override;
   virtual Float64 GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const override;
   virtual Float64 GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const override;
   virtual Float64 GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const override;
   virtual Float64 GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const override;
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem) const override;
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) const override;
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetCoverTopMat() const override;
   virtual Float64 GetTopMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const override;
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const override;
   virtual Float64 GetCoverBottomMat() const override;
   virtual Float64 GetBottomMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const override;
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const override;
   virtual void GetDeckReinforcing(const pgsPointOfInterest& poi,pgsTypes::DeckRebarMatType matType,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bAdjForDevLength,Float64* pAs,Float64* pYb) const override;
   virtual void GetRebarLayout(const CSegmentKey& segmentKey, IRebarLayout** rebarLayout) const override;
   virtual void GetClosureJointRebarLayout(const CClosureKey& closureKey, IRebarLayout** rebarLayout) const override;
   virtual REBARDEVLENGTHDETAILS GetSegmentRebarDevelopmentLengthDetails(const CSegmentKey& segmetnKey,IRebar* rebar, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct, bool bIsTopBar, bool bEpoxyCoated, bool bMeetsCoverRequirements) const override;
   virtual REBARDEVLENGTHDETAILS GetDeckRebarDevelopmentLengthDetails(IRebar* rebar, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct, bool bIsTopBar, bool bEpoxyCoated, bool bMeetsCoverRequirements) const override;
   virtual bool IsAnchored(const pgsPointOfInterest& poi) const;

// IStirrupGeometry
public:
   virtual bool AreStirrupZonesSymmetrical(const CSegmentKey& segmentKey) const override;

   virtual ZoneIndexType GetPrimaryZoneCount(const CSegmentKey& segmentKey) const override;
   virtual void GetPrimaryZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end) const override;
   virtual void GetPrimaryVertStirrupBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, WBFL::Materials::Rebar::Size* pSize, Float64* pCount, Float64* pSpacing) const override;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const CSegmentKey& segmentKey,ZoneIndexType zone) const override;
   virtual WBFL::Materials::Rebar::Size GetPrimaryConfinementBarSize(const CSegmentKey& segmentKey,ZoneIndexType zone) const override;

   virtual ZoneIndexType GetHorizInterfaceZoneCount(const CSegmentKey& segmentKey) const override;
   virtual void GetHorizInterfaceZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end) const override;
   virtual void GetHorizInterfaceBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, WBFL::Materials::Rebar::Size* pSize, Float64* pCount, Float64* pSpacing) const override;

   virtual void GetAddSplittingBarInfo(const CSegmentKey& segmentKey, WBFL::Materials::Rebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing) const override;
   virtual void GetAddConfinementBarInfo(const CSegmentKey& segmentKey, WBFL::Materials::Rebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing) const override;

   virtual Float64 GetVertStirrupAvs(const pgsPointOfInterest& poi, WBFL::Materials::Rebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) const override;
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi) const override; // stirrup angle=90 for vertical

   virtual bool DoStirrupsEngageDeck(const CSegmentKey& segmentKey) const override;
   virtual bool DoAllPrimaryStirrupsEngageDeck(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetPrimaryHorizInterfaceBarSpacing(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, WBFL::Materials::Rebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) const override;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAdditionalHorizInterfaceBarSpacing(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, WBFL::Materials::Rebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) const override;
   virtual Float64 GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi) const override;

   virtual Float64 GetSplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end) const override;

   virtual void GetStartConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, WBFL::Materials::Rebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) const override;
   virtual void GetEndConfinementBarInfo(  const CSegmentKey& segmentKey, Float64 requiredZoneLength, WBFL::Materials::Rebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) const override;

   virtual bool AreStirrupZoneLengthsCombatible(const CGirderKey& girderKey) const override;

// IStrandGeometry
public:
   virtual WBFL::Geometry::Point2d GetStrandCG(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig = nullptr) const override;
   virtual WBFL::Geometry::Point2d GetStrandCG(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const override;

   virtual WBFL::Geometry::Point2d GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig = nullptr) const override;
   virtual WBFL::Geometry::Point2d GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const override;
   virtual WBFL::Geometry::Point2d GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig = nullptr) const override;
   virtual WBFL::Geometry::Point2d GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const override;

   virtual void GetStrandProfile(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, StrandIndexType strandIdx, IPoint2dCollection** ppProfilePoints) const override;
   virtual void GetStrandProfile(const CPrecastSegmentData* pSegment, const CStrandData* pStrands,pgsTypes::StrandType strandType, StrandIndexType strandIdx, IPoint2dCollection** ppProfilePoints) const override;
   virtual void GetStrandCGProfile(const CSegmentKey& segmentKey, bool bIncTemp, IPoint2dCollection** ppProfilePoints) const override;

   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetMaxStrandSlope(const CSegmentKey& segmentKey) const override;

   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi, DevelopmentAdjustmentType devAdjust, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust, const GDRCONFIG* pConfig = nullptr) const override;

   virtual StrandIndexType GetNumStrandsBottomHalf(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const override;

   virtual StrandIndexType GetStrandCount(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const GDRCONFIG* pConfig=nullptr) const override;
   virtual std::pair<StrandIndexType, StrandIndexType> GetStrandCount(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig) const override;
   virtual StrandIndexType GetMaxStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const override;
   virtual StrandIndexType GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type) const override;

   virtual Float64 GetStrandArea(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetPjack(const CSegmentKey& segmentKey,bool bIncTemp) const override;
   virtual Float64 GetJackingStress(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint) const override;
   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) const override;
   virtual void GetStrandPositionEx(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, const PRESTRESSCONFIG& rconfig,IPoint2d** ppPoint) const override;
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) const override;
   virtual void GetStrandPositionsEx(LPCTSTR strGirderName, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type,pgsTypes::MemberEndType endType, IPoint2dCollection** ppPoints) const override;

   // Harped strands can be forced to be straight along their length
   virtual bool GetAreHarpedStrandsForcedStraight(const CSegmentKey& segmentKey) const override;

   virtual void GetHarpedStrandControlHeights(const CSegmentKey& segmentKey,Float64* pHgStart,Float64* pHgHp1,Float64* pHgHp2,Float64* pHgEnd) const override;

   // harped offsets are measured from original strand locations in strand grid
   virtual void GetHarpStrandOffsets(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* pOffsetEnd,Float64* pOffsetHp) const override;

   virtual void GetHarpedEndOffsetBounds(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* DownwardOffset, Float64* UpwardOffset) const override;
   virtual void GetHarpedEndOffsetBoundsEx(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset) const override;
   virtual void GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) const override;
   virtual void GetHarpedHpOffsetBounds(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* DownwardOffset, Float64* UpwardOffset) const override;
   virtual void GetHarpedHpOffsetBoundsEx(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset) const override;
   virtual void GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) const override;

   virtual Float64 GetHarpedEndOffsetIncrement(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHarpedHpOffsetIncrement(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHarpedEndOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType) const override;
   virtual Float64 GetHarpedHpOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType) const override;

   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* lhp,Float64* rhp) const override;
   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* pX1,Float64* pX2,Float64* pX3,Float64* pX4) const override;
   virtual void GetHighestHarpedStrandLocationEnds(const CSegmentKey& segmentKey,Float64* pElevation) const override;
   virtual void GetHighestHarpedStrandLocationHPs(const CSegmentKey& segmentKey,Float64* pElevation) const override;
   virtual IndexType GetNumHarpPoints(const CSegmentKey& segmentKey) const override;

   virtual StrandIndexType GetMaxNumPermanentStrands(const CSegmentKey& segmentKey) const override;
   virtual StrandIndexType GetMaxNumPermanentStrands(LPCTSTR strGirderName) const override;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,const CSegmentKey& segmentKey, StrandIndexType* numStraight, StrandIndexType* numHarped) const override;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped) const override;
   virtual StrandIndexType GetNextNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const override;
   virtual StrandIndexType GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum) const override;
   virtual StrandIndexType GetPreviousNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const override;
   virtual StrandIndexType GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum) const override;
   virtual void ComputePermanentStrandIndices(const CSegmentKey& segmentKey, pgsTypes::StrandType strType, IIndexArray** permIndices) const override;
   virtual void ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices) const override;


   virtual bool IsValidNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) const override;
   virtual bool IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) const override;
   virtual StrandIndexType GetNextNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) const override;
   virtual StrandIndexType GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) const override;
   virtual StrandIndexType GetPrevNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) const override;
   virtual StrandIndexType GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) const override;

   virtual StrandIndexType GetNumExtendedStrands(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType standType) const override;
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) const override;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) const override;

   virtual ConfigStrandFillVector ComputeStrandFill(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType Ns) const override;
   virtual ConfigStrandFillVector ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns) const override;

   virtual GridIndexType SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo) const override;
   virtual void GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) const override;
   virtual void GridPositionToStrandPosition(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) const override;

   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig,Float64* pStart,Float64* pEnd) const override;
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType, const GDRCONFIG* pConfig=nullptr) const override;
   virtual StrandIndexType GetNumDebondedStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, pgsTypes::DebondMemberEndType end, const GDRCONFIG* pConfig=nullptr) const override;
   virtual RowIndexType GetNumRowsWithStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType ) const override;
   virtual StrandIndexType GetNumStrandInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const override;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const pgsPointOfInterest& poi, RowIndexType rowIdx, pgsTypes::StrandType strandType ) const override;
   virtual StrandIndexType GetNumDebondedStrandsInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const override;
   virtual bool IsExteriorStrandDebondedInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType) const override;
   virtual bool IsExteriorWebStrandDebondedInRow(const pgsPointOfInterest& poi, WebIndexType webIdx, RowIndexType rowIdx, pgsTypes::StrandType strandType) const override;
   virtual Float64 GetUnadjustedStrandRowElevation(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const override;
   virtual bool HasDebonding(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig=nullptr) const override;
   virtual bool IsDebondingSymmetric(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig=nullptr) const override;

   virtual RowIndexType GetNumRowsWithStrand(const pgsPointOfInterest& poi,StrandIndexType nStrands,pgsTypes::StrandType strandType ) const override;
   virtual StrandIndexType GetNumStrandInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const override;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType ) const override;

   virtual Float64 GetDebondSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const override;
   virtual SectionIndexType GetNumDebondSections(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const override;
   virtual StrandIndexType GetNumDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const override;
   virtual StrandIndexType GetNumBondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const override;
   virtual std::vector<StrandIndexType> GetDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const override;

   virtual bool CanDebondStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override; // can debond any of the strands
   virtual bool CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType) const override;
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) const override;
   virtual void ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) const override; 

   virtual std::vector<RowIndexType> GetRowsWithDebonding(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig) const override;
   virtual IndexType GetDebondConfigurationCountByRow(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, RowIndexType rowIdx, const GDRCONFIG* pConfig) const override;
   virtual void GetDebondConfigurationByRow(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, RowIndexType rowIdx, IndexType configIdx, const GDRCONFIG* pConfig, Float64* pXstart, Float64* pLstrand, Float64* pCgX, Float64* pCgY, StrandIndexType* pnStrands) const override;

   virtual std::vector<CComPtr<IRect2d>> GetWebWidthProjectionsForDebonding(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType, Float64* pfraDebonded, Float64* pBottomFlangeToWebWidthRatio) const override;

   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const override;
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const override;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const override;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const override;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const override;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const override;
   virtual Float64 ConvertHarpedOffsetEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const override;
   virtual Float64 ConvertHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const override;
   virtual Float64 ConvertHarpedOffsetHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const override;
   virtual Float64 ConvertHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const override;

   virtual pgsTypes::TTSUsage GetTemporaryStrandUsage(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;

   virtual void ResolveHarpPointLocations(const CPrecastSegmentData* pSegment, const CStrandData* pStrands, std::array<Float64, 4>& Xhp) const override;
   virtual void ResolveStrandRowElevations(const CPrecastSegmentData* pSegment, const CStrandData* pStrands, const CStrandRow& strandRow, std::array<Float64, 4>& Xhp, std::array<Float64, 4>& Y) const override;

// IPointOfInterest
public:
   virtual void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiList* pPoiList) const override;
   virtual void GetPointsOfInterest(Float64 station,IDirection* pDirection, std::vector<pgsPointOfInterest>* pvPoi) const override;
   virtual void GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib, PoiList* pPoiList,Uint32 mode = POIFIND_OR) const override;
   virtual void GetPointsOfInterest(const CSpanKey& spanKey, PoiList* pPoiList) const override;
   virtual void GetPointsOfInterest(const CSpanKey& spanKey,PoiAttributeType attrib, PoiList* pPoiList,Uint32 mode = POIFIND_OR) const override;
   virtual void MergePoiLists(const PoiList& list1, const PoiList& list2, PoiList* pPoiList) const override;
   virtual void SortPoiList(PoiList* pPoiList) const override;
   virtual const pgsPointOfInterest& GetPointOfInterest(PoiIDType poiID) const override;
   virtual pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs) const override;
   virtual bool GetPointOfInterest(const CSegmentKey& segmentKey,Float64 station,IDirection* pDirection,pgsPointOfInterest* pPoi) const override;
   virtual void GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey, PoiList* pPoiList) const override;
   virtual void GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey,const GDRCONFIG& config, std::vector<pgsPointOfInterest>* pvPoi) const override;
   virtual bool GetPointOfInterest(const CGirderKey& girderKey,Float64 station,IDirection* pDirection,bool bProjectSegmentEnds,pgsPointOfInterest* pPoi) const override;
   virtual const pgsPointOfInterest& GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs) const override;
   virtual const pgsPointOfInterest& GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR) const override;
   virtual const pgsPointOfInterest& GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR) const override;
   virtual pgsPointOfInterest GetPierPointOfInterest(const CGirderKey& girderKey,PierIndexType pierIdx) const override;
   virtual pgsPointOfInterest GetTemporarySupportPointOfInterest(const CGirderKey& girderKey,SupportIndexType tsIdx) const override;
   virtual void RemovePointsOfInterest(PoiList& vPoi,PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute) const override;
   virtual bool IsInClosureJoint(const pgsPointOfInterest& poi,CClosureKey* pClosureKey) const override;
   virtual bool IsOnSegment(const pgsPointOfInterest& poi) const override;
   virtual bool IsOffSegment(const pgsPointOfInterest& poi) const override;
   virtual bool IsOnGirder(const pgsPointOfInterest& poi) const override;
   virtual bool IsInBoundaryPierDiaphragm(const pgsPointOfInterest& poi) const override;
   virtual bool IsInCriticalSectionZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState) const override;
   virtual IndexType GetDeckCastingRegion(const pgsPointOfInterest& poi) const override;
   virtual pgsPointOfInterest ConvertSpanPointToPoi(const CSpanKey& spanKey,Float64 Xspan) const override;
   virtual void ConvertPoiToSpanPoint(const pgsPointOfInterest& poi,CSpanKey* pSpanKey,Float64* pXspan) const override;
   virtual void ConvertSpanPointToSegmentCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXs) const override;
   virtual void ConvertSegmentCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xs,CSpanKey* pSpanKey,Float64* pXspan) const override;
   virtual void ConvertSpanPointToSegmentPathCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXsp) const override;
   virtual void ConvertSegmentPathCoordinateToSpanPoint(const CSegmentKey& sSegmentKey,Float64 Xsp,CSpanKey* pSpanKey,Float64* pXspan) const override;
   virtual void GetPointsOfInterestInRange(Float64 xLeft,const pgsPointOfInterest& poi,Float64 xRight,PoiList* pPoiList) const override;
   virtual PierIndexType GetPier(const pgsPointOfInterest& poi) const override;
   virtual void GetDuctRange(const CGirderKey& girderKey, DuctIndexType ductIdx, const pgsPointOfInterest** ppEtartPoi, const pgsPointOfInterest** ppEndPoi) const override;
   virtual void GroupBySegment(const PoiList& vPoi, std::list<PoiList>* pList) const override;
   virtual void GroupByGirder(const PoiList& vPoi, std::list<PoiList>* pList) const override;
   virtual void GetSegmentKeys(const PoiList& vPoi, std::vector<CSegmentKey>* pvSegments) const override;
   virtual void GetSegmentKeys(const PoiList& vPoi,const CGirderKey& girderKey, std::vector<CSegmentKey>*pvSegments) const override;
   virtual void GetGirderKeys(const PoiList& vPoi, std::vector<CGirderKey>* pvGirders) const override;
   virtual Float64 ConvertPoiToSegmentPathCoordinate(const pgsPointOfInterest& poi) const override;
   virtual pgsPointOfInterest ConvertSegmentPathCoordinateToPoi(const CSegmentKey& segmentKey,Float64 Xsp) const override;
   virtual Float64 ConvertSegmentPathCoordinateToSegmentCoordinate(const CSegmentKey& segmentKey,Float64 Xsp) const override;
   virtual Float64 ConvertSegmentCoordinateToGirderCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const override;
   //virtual void ConvertGirderCoordinateToSegmentCoordinate(const CGirderKey& girderKey,Float64 Xg,CSegmentKey* pSegmentKey,Float64* pXs) const override;
   virtual Float64 ConvertSegmentCoordinateToGirderlineCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const override;
   virtual Float64 ConvertSegmentPathCoordinateToGirderPathCoordinate(const CSegmentKey& segmentKey,Float64 Xsp) const override;
   virtual Float64 ConvertSegmentCoordinateToSegmentPathCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const override;
   virtual Float64 ConvertPoiToGirderCoordinate(const pgsPointOfInterest& poi) const override;
   virtual pgsPointOfInterest ConvertGirderCoordinateToPoi(const CGirderKey& girderKey,Float64 Xg) const override;
   virtual Float64 ConvertPoiToGirderPathCoordinate(const pgsPointOfInterest& poi) const override;
   virtual pgsPointOfInterest ConvertGirderPathCoordinateToPoi(const CGirderKey& girderKey,Float64 Xgp) const override;
   virtual Float64 ConvertGirderCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float64 Xg) const override;
   virtual Float64 ConvertGirderPathCoordinateToGirderCoordinate(const CGirderKey& girderKey,Float64 Xgp) const override;
   virtual Float64 ConvertGirderPathCoordinateToGirderlineCoordinate(const CGirderKey& girderKey,Float64 Xgp) const override;
   virtual Float64 ConvertPoiToGirderlineCoordinate(const pgsPointOfInterest& poi) const override;
   virtual pgsPointOfInterest ConvertGirderlineCoordinateToPoi(GirderIndexType gdrIdx,Float64 Xgl) const override;
   virtual Float64 ConvertRouteToBridgeLineCoordinate(Float64 station) const override;
   virtual Float64 ConvertBridgeLineToRouteCoordinate(Float64 Xb) const override;
   virtual Float64 ConvertPoiToBridgeLineCoordinate(const pgsPointOfInterest& poi) const override;

// ISectionProperties
public:
   virtual pgsTypes::SectionPropertyMode GetSectionPropertiesMode() const override;

   virtual pgsTypes::HaunchAnalysisSectionPropertiesType GetHaunchAnalysisSectionPropertiesType() const override;

   virtual std::vector<WBFL::Geometry::Point2d> GetStressPoints(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetStressCoefficients(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location, const GDRCONFIG* pConfig, Float64* pCa, Float64 *pCbx, Float64* pCby, IndexType* pControllingStressPointIndex = nullptr) const override;

   virtual Float64 GetHg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual void GetCentroid(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, IPoint2d** ppCG) const override;
   virtual Float64 GetIxx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetIyy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetIxy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetXleft(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetXright(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetY(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location) const override;
   virtual Float64 GetS(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location) const override;
   virtual Float64 GetKt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetKb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetEIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;

   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) const override;
   virtual Float64 GetIxx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) const override;
   virtual Float64 GetIyy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fcgdr) const override;
   virtual Float64 GetIxy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fcgdr) const override;
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr) const override;
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr) const override;

   virtual Float64 GetHg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual void GetCentroid(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, IPoint2d** ppCG) const override;
   virtual Float64 GetIxx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetIyy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetIxy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetXleft(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetXright(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location) const override;
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) const override;
   virtual Float64 GetKt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetKb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetEIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;

   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) const override;
   virtual Float64 GetIxx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) const override;
   virtual Float64 GetIyy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fc) const override;
   virtual Float64 GetIxy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fc) const override;
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc) const override;
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc) const override;

   virtual Float64 GetNetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetIxx(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetIyy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetIxy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetYbg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetYtg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetAd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetId(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetYbd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetNetYtd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;

   virtual Float64 GetQSlab(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetQSlab(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64 fc) const override;
   virtual Float64 GetQ(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 Yclip) const override;
   virtual Float64 GetQ(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 Yclip) const override;
   virtual Float64 GetAcBottomHalf(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAcTopHalf(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetEffectiveFlangeWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTributaryFlangeWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTributaryFlangeWidthEx(const pgsPointOfInterest& poi, Float64* pLftFw, Float64* pRgtFw) const override;
   virtual Float64 GetEffectiveDeckArea(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTributaryDeckArea(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetGrossDeckArea(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetStructuralHaunchDepth(const pgsPointOfInterest& poi,pgsTypes::HaunchAnalysisSectionPropertiesType haunchAType) const override;
   virtual void ReportEffectiveFlangeWidth(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const override;
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi) const override;
   virtual void GetSegmentVolumeAndSurfaceArea(const CSegmentKey& segmentKey, Float64* pVolume, Float64* pSurfaceArea, int surfaceAreaType = INCLUDE_HALF_INTERIOR_SURFACE_AREA) const override;
   virtual void GetClosureJointVolumeAndSurfaceArea(const CClosureKey& closureKey, Float64* pVolume, Float64* pSurfaceArea) const override;
   virtual void GetDeckVolumeAndSurfaceArea(Float64* pVolume, Float64* pSurfaceArea) const override;
   virtual void GetBridgeStiffness(Float64 Xb, Float64* pEIxx, Float64* pEIyy, Float64* pEIxy) const override;
   virtual Float64 GetBridgeEIxx(Float64 Xb) const override;
   virtual Float64 GetBridgeEIyy(Float64 Xb) const override;
   virtual Float64 GetBridgeEIxy(Float64 Xb) const override;
   virtual Float64 GetSegmentWeightPerLength(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentWeight(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentHeightAtPier(const CSegmentKey& segmentKey,PierIndexType pierIdx) const override;
   virtual Float64 GetSegmentHeightAtTemporarySupport(const CSegmentKey& segmentKey,SupportIndexType tsIdx) const override;
   virtual Float64 GetSegmentHeight(const CPrecastSegmentData* pSegment, Float64 Xs) const override;
   virtual bool IsStructuralSection(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx) const override;

// IShapes
public:
   virtual void GetSegmentShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bOrient,pgsTypes::SectionCoordinateType coordinateType,IShape** ppShape, IndexType* pGirderIndex=nullptr, IndexType* pSlabIndex=nullptr) const override;
   virtual void GetSegmentShape(const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias,IShape** ppShape) const override;
   virtual void GetSegmentSectionShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, bool bOrient,pgsTypes::SectionCoordinateType csType, IShape** ppShape, IndexType* pGirderIndex=nullptr, IndexType* pSlabIndex=nullptr) const override;
   virtual void GetSlabShape(Float64 station,IDirection* pDirection,bool bIncludeHaunchbool,IShape** ppShape) const override;
   virtual void GetLeftTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape) const override;
   virtual void GetRightTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape) const override;
   virtual void GetJointShapes(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bOrient, pgsTypes::SectionCoordinateType coordinateType, IShape** ppLeftJointShape, IShape** ppRightJointShape) const override;
   virtual void GetSlabAnalysisShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 haunchDepth, bool bFollowMatingSurfaceProfile,IShape** ppShape) const override;

// IBarriers
public:
   virtual Float64 GetAtb(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetItb(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetYbtb(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetExteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual bool HasInteriorBarrier(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetInteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual Float64 GetInteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual void GetSidewalkDeadLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) const override;
   virtual void GetSidewalkPedLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) const override;
   virtual bool HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const override;

// ISegmentLiftingPointsOfInterest
public:
   virtual void GetLiftingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib, PoiList* pPoiList,Uint32 mode = POIFIND_OR) const override;
   virtual void GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType attrib, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode = POIFIND_OR) const override;

// ISegmentHaulingPointsOfInterest
public:
   virtual void GetHaulingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,PoiList* pPoiList,Uint32 mode = POIFIND_OR) const override;
   virtual void GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib, std::vector<pgsPointOfInterest> * pvPoi,Uint32 mode = POIFIND_OR) const override;
   virtual Float64 GetMinimumOverhang(const CSegmentKey& segmentKey) const override;

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
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey) const override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey) const override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IUserDefinedLoads::UserDefinedLoadCase loadCase) const override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IUserDefinedLoads::UserDefinedLoadCase loadCase) const override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx) const override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) const override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) const override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx) const override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx) const override;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) const override;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase) const override;
   virtual const std::vector<UserPointLoad>* GetPointLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey) const override;
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey) const override;
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey) const override;
   virtual const std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CGirderKey& girderKey) const override;

// ITempSupport
public:
   virtual void GetControlPoints(SupportIndexType tsIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppLeft,IPoint2d** ppAlignment_pt,IPoint2d** ppBridge_pt,IPoint2d** ppRight) const override;
   virtual void GetDirection(SupportIndexType tsIdx,IDirection** ppDirection) const override;
   virtual void GetSkew(SupportIndexType tsIdx,IAngle** ppAngle) const override;
   virtual std::vector<SupportIndexType> GetTemporarySupports(GroupIndexType grpIdx) const override;
   virtual std::vector<TEMPORARYSUPPORTELEVATIONDETAILS> GetElevationDetails(SupportIndexType tsIdx,GirderIndexType gdrIndex) const override;

// IGirder
public:
   virtual bool    IsPrismatic(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey) const override;
   virtual bool    IsSymmetricSegment(const CSegmentKey& segmentKey) const override;
   virtual bool    IsSymmetric(IntervalIndexType intervalIdx,const CGirderKey& girderKey) const override; 
   virtual MatingSurfaceIndexType  GetMatingSurfaceCount(const CGirderKey& girderKey) const override;
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType msIdx, bool bGirderOnly = false) const override;
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType msIdx, bool bGirderOnly = false) const override;
   virtual bool GetMatingSurfaceProfile(const pgsPointOfInterest& poi, MatingSurfaceIndexType msIdx, bool bGirderOnly, IPoint2dCollection** ppPoints) const override;
   virtual FlangeIndexType GetTopFlangeCount(const CGirderKey& girderKey) const override;
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual bool CanTopFlangeBeLongitudinallyThickened(const CSegmentKey& segmentKey) const override;
   virtual pgsTypes::TopFlangeThickeningType GetTopFlangeThickeningType(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetTopFlangeThickening(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetTopFlangeThickening(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTopFlangeThickening(const CSegmentKey& segmentKey, Float64 Xpoi) const override;
   virtual Float64 GetTopFlangeThickening(const CPrecastSegmentData* pSegment, Float64 Xs) const override;
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi, Float64* pLeft=nullptr, Float64* pRight=nullptr) const override;
   virtual FlangeIndexType GetBottomFlangeCount(const CGirderKey& girderKey) const override;
   virtual Float64 GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const override;
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetBottomWidth(const pgsPointOfInterest& poi, Float64* pLeft=nullptr, Float64* pRight=nullptr) const override;
   virtual Float64 GetMinWebWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetWebThicknessAtDuct(const pgsPointOfInterest& poi,DuctIndexType ductIdx) const override;
   virtual Float64 GetMinTopFlangeThickness(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetMinBottomFlangeThickness(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetHeight(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetShearWidth(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetShearInterfaceWidth(const pgsPointOfInterest& poi) const override;
   virtual InterfaceShearWidthDetails GetInterfaceShearWidthDetails(const pgsPointOfInterest& poi) const override;
   virtual WebIndexType GetWebCount(const CGirderKey& girderKey) const override;
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx) const override;
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx) const override;
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx) const override;
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetWebWidth(const pgsPointOfInterest& poi) const override;
   virtual void GetSegmentEndPoints(const CSegmentKey& segmentKey,pgsTypes::PlanCoordinateType pcType,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2) const override;
   virtual void GetSegmentPlanPoints(const CSegmentKey& segmentKey, pgsTypes::PlanCoordinateType pcType, IPoint2d** ppEnd1Left, IPoint2d** ppEnd1, IPoint2d** ppEnd1Right, IPoint2d** ppEnd2Right, IPoint2d** ppEnd2, IPoint2d** ppEnd2Left) const override;
   virtual Float64 GetOrientation(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetWorkPointShiftOffset(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetTransverseTopFlangeSlope(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetProfileChordElevation(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTopGirderChordElevation(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetTopGirderChordElevation(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) const override;
   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi) const override;
   virtual pgsTypes::SplittingDirection GetSplittingDirection(const CGirderKey& girderKey) const override;
   virtual bool CanPrecamber(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetPrecamber(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetPrecamber(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetPrecamber(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const override;
   virtual Float64 GetPrecamber(const CPrecastSegmentData* pSegment, Float64 Xs) const override;
   virtual Float64 GetPrecamberSlope(const pgsPointOfInterest& poi) const override;
   virtual bool HasShearKey(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType) const override;
   virtual void GetShearKeyAreas(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const override;
   virtual bool HasStructuralLongitudinalJoints() const override;
   virtual Float64 GetStructuralLongitudinalJointWidth(const pgsPointOfInterest& poi) const override;
   virtual void GetStructuralLongitudinalJointWidth(const pgsPointOfInterest& poi, Float64* pLeft, Float64* pRight) const override;
   virtual void GetSegmentPlan(const CSegmentKey& segmentKey, IShape** ppShape) const override;
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IShape** ppShape) const override;
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IShape** ppShape) const override;
   virtual void GetClosureJointProfile(const CClosureKey& closureKey, IShape** ppShape) const override;
   virtual Float64 GetSegmentHeight(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64 Xsp) const override;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IPoint2dCollection** points) const override;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IPoint2dCollection** points) const override;
   virtual void GetSegmentDirection(const CSegmentKey& segmentKey,IDirection** ppDirection) const override;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,Float64* pStartEndDistance,Float64* pEndEndDistance) const override;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,Float64* pStartEndDistance,Float64* pEndEndDistance) const override;
   virtual void GetSegmentBearingOffset(const CSegmentKey& segmentKey,Float64* pStartBearingOffset,Float64* pEndBearingOffset) const override;
   virtual void GetSegmentStorageSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) const override;
   virtual void GetSegmentReleaseSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) const override;
   virtual const WBFL::Stability::Girder* GetSegmentLiftingStabilityModel(const CSegmentKey& segmentKey) const override;
   virtual const WBFL::Stability::LiftingStabilityProblem* GetSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey) const override;
   virtual const WBFL::Stability::LiftingStabilityProblem* GetSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey,const HANDLINGCONFIG& handlingConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD) const override;
   virtual const WBFL::Stability::Girder* GetSegmentHaulingStabilityModel(const CSegmentKey& segmentKey) const override;
   virtual const WBFL::Stability::HaulingStabilityProblem* GetSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey) const override;
   virtual const WBFL::Stability::HaulingStabilityProblem* GetSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey,const HANDLINGCONFIG& handlingConfig,ISegmentHaulingDesignPointsOfInterest* pPOId) const override;
   virtual std::vector<std::tuple<Float64, Float64, std::_tstring>> GetWebSections(const pgsPointOfInterest& poi) const override;

// IGirderTendonGeometry
public:
   virtual DuctIndexType GetDuctCount(const CGirderKey& girderKey) const override;
   virtual void GetDuctRange(const CGirderKey& girderKey, DuctIndexType ductIdx, Float64* pXgs, Float64* pXge) const override;
   virtual bool IsOnDuct(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint2dCollection** ppPoints) const override;
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint3dCollection** ppPoints) const override;
   virtual void GetDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, DuctIndexType ductIdx, const CPTData* pPTData, IPoint2dCollection** ppPoints) const override;
   virtual void GetDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, const CDuctData* pDuctData, IPoint2dCollection** ppPoints) const override;
   virtual void GetGirderDuctPoint(const pgsPointOfInterest& poit,DuctIndexType ductIdx,IPoint2d** ppPoint) const override;
   virtual void GetGirderDuctPoint(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IPoint2d** ppPoint) const override;
   virtual Float64 GetOutsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetInsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetNominalDiameter(const CGirderKey& girderKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetInsideDuctArea(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual StrandIndexType GetTendonStrandCount(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderTendonArea(const CGirderKey& girderKey,IntervalIndexType intervalIdx,DuctIndexType ductIdx) const override;
   virtual void GetGirderTendonSlope(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IVector3d** ppSlope) const override;
   virtual void GetGirderTendonSlope(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IVector3d** ppSlope) const override;
   virtual Float64 GetMinimumRadiusOfCurvature(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetPjack(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetFpj(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderDuctOffset(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) const override;
   virtual Float64 GetDuctLength(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual void GetGirderTendonEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx,Float64* pEccX,Float64* pEccY) const override;
   virtual void GetGirderTendonEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx, Float64* pEccX, Float64* pEccY) const override;
   virtual Float64 GetGirderTendonAngularChange(const pgsPointOfInterest& poi,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetGirderTendonAngularChange(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,DuctIndexType ductIdx) const override;
   virtual pgsTypes::JackingEndType GetJackingEnd(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderAptTopHalf(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetGirderAptBottomHalf(const pgsPointOfInterest& poi) const override;


// ISegmentTendonGeometry
public:
   virtual DuctIndexType GetDuctCount(const CSegmentKey& segmentKey) const override;
   virtual DuctIndexType GetMaxDuctCount(const CGirderKey& girderKey) const override;
   virtual bool IsOnDuct(const pgsPointOfInterest& poi) const override;
   virtual void GetDuctCenterline(const CSegmentKey& segmentKey, DuctIndexType ductIdx, IPoint2dCollection** ppPoints) const override;
   virtual void GetDuctCenterline(const CSegmentKey& segmentKey, DuctIndexType ductIdx, IPoint3dCollection** ppPoints) const override;
   virtual void GetDuctCenterline(const CSegmentKey& segmentKey, const CPrecastSegmentData* pSegment, DuctIndexType ductIdx, const CSegmentPTData* pPTData, IPoint2dCollection** ppPoints) const override;
   virtual void GetDuctCenterline(const CSegmentKey& segmentKey, const CPrecastSegmentData* pSegment, const CSegmentDuctData* pDuctData, IPoint2dCollection** ppPoints) const override;
   virtual void GetSegmentDuctPoint(const CSegmentKey& segmentKey, Float64 Xs, DuctIndexType ductIdx, IPoint2d** ppPoint) const override;
   virtual void GetSegmentDuctPoint(const pgsPointOfInterest& poi, DuctIndexType ductIdx, IPoint2d** ppPoint) const override;
   virtual void GetSegmentDuctPoint(const pgsPointOfInterest& poi, const CPrecastSegmentData* pSegment, DuctIndexType ductIdx, IPoint3d** ppPoint) const override;
   virtual void GetSegmentDuctPoint(const pgsPointOfInterest& poi, const CPrecastSegmentData* pSegment, const CSegmentDuctData* pDuctData, IPoint3d** ppPoint) const override;
   virtual Float64 GetOutsideDiameter(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetInsideDiameter(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetNominalDiameter(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetInsideDuctArea(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual StrandIndexType GetTendonStrandCount(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentTendonArea(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx, DuctIndexType ductIdx) const override;
   virtual void GetSegmentTendonSlope(const pgsPointOfInterest& poi, DuctIndexType ductIdx, IVector3d** ppSlope) const override;
   virtual void GetSegmentTendonSlope(const CSegmentKey& segmentKey, Float64 Xs, DuctIndexType ductIdx, IVector3d** ppSlope) const override;
   virtual Float64 GetMinimumRadiusOfCurvature(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetPjack(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetFpj(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentDuctOffset(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   virtual Float64 GetDuctLength(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual void GetSegmentTendonEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, DuctIndexType ductIdx, Float64* pEccX, Float64* pEccY) const override;
   virtual void GetSegmentTendonEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, DuctIndexType ductIdx, Float64* pEccX, Float64* pEccY) const override;
   virtual Float64 GetSegmentTendonAngularChange(const pgsPointOfInterest& poi, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetSegmentTendonAngularChange(const pgsPointOfInterest& poi1, const pgsPointOfInterest& poi2, DuctIndexType ductIdx) const override;
   virtual pgsTypes::JackingEndType GetJackingEnd(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentAptTopHalf(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetSegmentAptBottomHalf(const pgsPointOfInterest& poi) const override;

// IIntervals
public:
   virtual IntervalIndexType GetIntervalCount() const override;
   virtual EventIndexType GetStartEvent(IntervalIndexType idx) const override; 
   virtual EventIndexType GetEndEvent(IntervalIndexType idx) const override; 
   virtual Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const override;
   virtual Float64 GetDuration(IntervalIndexType idx) const override;
   virtual std::_tstring GetDescription(IntervalIndexType idx) const override;
   virtual IntervalIndexType GetInterval(EventIndexType eventIdx) const override;
   virtual IntervalIndexType GetErectPierInterval(PierIndexType pierIdx) const override;
   virtual IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetLiftSegmentInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetFirstLiftSegmentInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastLiftSegmentInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetFirstStorageInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastStorageInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetHaulSegmentInterval(const CSegmentKey& segmentKey) const override;
   virtual bool IsHaulSegmentInterval(IntervalIndexType intervalIdx) const override;
   virtual IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey) const override;
   virtual bool IsSegmentErectionInterval(IntervalIndexType intervalIdx) const override;
   virtual bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const override;
   virtual IntervalIndexType GetTemporaryStrandStressingInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetTemporaryStrandInstallationInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetCastClosureJointInterval(const CClosureKey& closureKey) const override;
   virtual IntervalIndexType GetCompositeClosureJointInterval(const CClosureKey& closureKey) const override;
   virtual IntervalIndexType GetFirstCompositeClosureJointInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastCompositeClosureJointInterval(const CGirderKey& girderKey) const override;
   virtual void GetContinuityInterval(PierIndexType pierIdx,IntervalIndexType* pBack,IntervalIndexType* pAhead) const override;
   virtual IntervalIndexType GetCastIntermediateDiaphragmsInterval() const override;
   virtual IntervalIndexType GetCompositeIntermediateDiaphragmsInterval() const override;
   virtual IntervalIndexType GetCastLongitudinalJointInterval() const override;
   virtual IntervalIndexType GetCompositeLongitudinalJointInterval() const override;
   virtual IntervalIndexType GetCastDeckInterval(IndexType castingRegionIdx) const override;
   virtual IntervalIndexType GetFirstCastDeckInterval() const override;
   virtual IntervalIndexType GetLastCastDeckInterval() const override;
   virtual IntervalIndexType GetCompositeDeckInterval(IndexType castingRegionIdx) const override;
   virtual IntervalIndexType GetFirstCompositeDeckInterval() const override;
   virtual IntervalIndexType GetLastCompositeDeckInterval() const override;
   virtual IntervalIndexType GetCastShearKeyInterval() const override;
   virtual IntervalIndexType GetConstructionLoadInterval() const override;
   virtual IntervalIndexType GetLiveLoadInterval() const override;
   virtual IntervalIndexType GetLoadRatingInterval() const override;
   virtual IntervalIndexType GetOverlayInterval() const override;
   virtual IntervalIndexType GetInstallRailingSystemInterval() const override;
   virtual IntervalIndexType GetStressSegmentTendonInterval(const CSegmentKey& segmentKey) const override;
   virtual IntervalIndexType GetFirstSegmentTendonStressingInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastSegmentTendonStressingInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetFirstGirderTendonStressingInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetLastGirderTendonStressingInterval(const CGirderKey& girderKey) const override;
   virtual IntervalIndexType GetStressGirderTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual bool IsGirderTendonStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const override;
   virtual bool IsSegmentTendonStressingInterval(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const override;
   virtual bool IsStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const override;
   virtual IntervalIndexType GetTemporarySupportErectionInterval(SupportIndexType tsIdx) const override;
   virtual IntervalIndexType GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) const override;
   virtual std::vector<IntervalIndexType> GetTemporarySupportRemovalIntervals(GroupIndexType groupIdx) const override;
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey) const override;
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey,pgsTypes::ProductForceType pfType) const override;
   virtual bool IsUserDefinedLoadingInterval(IntervalIndexType intervalIdx) const override;
   virtual IntervalIndexType GetNoncompositeUserLoadInterval() const override;
   virtual IntervalIndexType GetCompositeUserLoadInterval() const override;
   virtual IntervalIndexType GetLastNoncompositeInterval() const override;
   virtual IntervalIndexType GetLastCompositeInterval() const override;
   virtual IntervalIndexType GetGeometryControlInterval() const override;
   virtual std::vector<IntervalIndexType> GetReportingGeometryControlIntervals() const override;
   virtual std::vector<IntervalIndexType> GetSpecCheckGeometryControlIntervals() const override;


private:
   DECLARE_EAF_AGENT_DATA;
   DECLARE_LOGFILE;

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

   std::map<IndexType,std::pair<IndexType,CogoObjectID>> m_CompoundCurveKeys; // key is hc index in cogo model. value is (input hc index,cogomodel curve id)... if an input curve has zero radius it is not created in the curve collection in the cogo model
   std::map<IndexType,std::pair<IndexType,CogoObjectID>> m_VertCurveKeys;

   // gets a horizontal curve in local coordinates without making a copy
   void GetCurve(CollectionIndexType idx, ICompoundCurve** ppCurve) const;

   // Cache state of asymmetric prestressing information
   enum AsymmetricPrestressing  { Unknown, Yes,  No  };
   mutable AsymmetricPrestressing m_AsymmetricPrestressing{ Unknown };

   CConcreteManager m_ConcreteManager;
   CIntervalManager m_IntervalManager;

   struct PoiLocation
   {
      PoiIDType ID;
      Float64 Station;
      Float64 Offset;
      CComPtr<IPoint2d> pntLocal;
      CComPtr<IPoint2d> pntGlobal;

      bool operator<(const PoiLocation& other) const { return ID < other.ID; }
   };
   typedef std::set<PoiLocation> PoiLocationCache;
   mutable std::unique_ptr<PoiLocationCache> m_pPoiLocationCache;
   void GetPoiLocation(const pgsPointOfInterest& poi, Float64* pStation, Float64* pOffset, IPoint2d** ppLocal, IPoint2d** ppGlobal) const;
   void InvalidatePoiLocationCache();
   static UINT DeletePoiLocationCache(LPVOID pParam);

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
   mutable ShapeContainer m_DeckShapes; // cache
   mutable ShapeContainer m_LeftBarrierShapes;
   mutable ShapeContainer m_RightBarrierShapes;

   // for ISectionProperties
   // Section Properties
   CComPtr<ISectionCutTool> m_SectCutTool;
   CComPtr<IEffectiveFlangeWidthTool> m_EffFlangeWidthTool;
   typedef struct SectProp
   {
      CComPtr<ISection> Section; // this is going to be a composite section, in girder section coordinates
      IndexType GirderShapeIndex; // index into the composite for the girder shape
      IndexType SlabShapeIndex; // index into the composite for the slab shape

      Float64 dx, dy; // these are the offset values to get the section into bridge section coordinates

      CComPtr<IElasticProperties> ElasticProps; // Elastic Properties (EA, EI, etc)
      CComPtr<IShapeProperties> ShapeProps; // Shape Properties (Area, I, CG, etc)

      Float64 YtopGirder; // distance from centroid to the top of the basic girder
      Float64 Perimeter; // perimeter of the basic girder

      bool bComposite; // If false, Qslab is undefined
      Float64 Qslab; // first moment of aread
      Float64 AcBottomHalf; // for LRFD Fig 5.7.3.4.2-3 (pre2017: 5.8.3.4.2-3)
      Float64 AcTopHalf;    // for LRFD Fig 5.7.3.4.2-3 (pre2017: 5.8.3.4.2-3)

      mutable std::map<Float64, Float64> Q; // key is Yclip and value is Q

      SectProp() { GirderShapeIndex = INVALID_INDEX; SlabShapeIndex = INVALID_INDEX; dx = -99999;dy = -99999;YtopGirder = 0; Perimeter = 0; bComposite = false; Qslab = 0; AcBottomHalf = 0; AcTopHalf = 0; }
   } SectProp;
   typedef std::map<PoiIntervalKey,SectProp> SectPropContainer; // Key = PoiIntervalKey object
   std::array<std::unique_ptr<SectPropContainer>, pgsTypes::sptSectionPropertyTypeCount> m_pSectProps; // index = one of the pgsTypes::SectionPropertyType constants

   // These are the last on the fly section property results
   // (LOTF = last on the fly)
   mutable PoiIntervalKey m_LOTFSectionPropertiesKey;
   mutable pgsTypes::SectionPropertyType m_LOTFSectionPropertiesType;
   mutable SectProp m_LOTFSectionProperties;

   void InvalidateSectionProperties(pgsTypes::SectionPropertyType sectPropType);
   static UINT DeleteSectionProperties(LPVOID pParam);
   pgsTypes::SectionPropertyType GetSectionPropertiesType() const; // returns the section properties types for the current section properties mode
   PoiIntervalKey GetSectionPropertiesKey(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType) const;
   const SectProp& GetSectionProperties(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::SectionPropertyType sectPropType) const;
   HRESULT CreateSection(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType,pgsTypes::SectionCoordinateType coordinateType,IndexType* pGdrIdx,IndexType* pSlabIdx,ISection** ppSection) const;
   Float64 ComputeY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,IShapeProperties* sprops) const;
   Float64 ComputeYtopGirder(IShapeProperties* compositeProps,IShape* pShape) const;

   // Points of interest for precast segments (precast girders/spliced girder segments)
   std::unique_ptr<pgsPoiMgr> m_pPoiMgr;
   pgsPoiMgr* CreatePoiManager();
   void InvalidatePointsOfInterest();
   static UINT DeletePoiManager(LPVOID pParam);
   std::set<CGirderKey> m_ValidatedPoi;

   // keeps track of which girders have had critical sections locations determined
   mutable std::array<std::set<CGirderKey>,9> m_CriticalSectionState; // use the LimitStateToShearIndex method to map limit state to array index

   // Adapter for working with strand fills
   // DON'T ACCESS THIS COLLECTION DIRECTLY - USE ACCESS FUNCTIONS BELOW
   typedef std::map<CSegmentKey, CStrandFiller>  StrandFillerCollection;
   mutable StrandFillerCollection  m_StrandFillers; // a filler for every girder

   CContinuousStrandFiller* GetContinuousStrandFiller(const CSegmentKey& segmentKey) const;
   CDirectStrandFiller* GetDirectStrandFiller(const CSegmentKey& segmentKey) const;
   void InitializeStrandFiller(const GirderLibraryEntry* pGirderEntry, const CSegmentKey& segmentKey) const;

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

   mutable std::map<Float64,Float64> m_LeftSlabEdgeOffset;
   mutable std::map<Float64,Float64> m_RightSlabEdgeOffset;
   Float64 GetSlabEdgeOffset(std::map<Float64, Float64>& slabEdgeOffset, DirectionType side, Float64 Xb) const;
   Float64 GetCurbOffset(DirectionType side, Float64 Xb) const;

   // Stability Modeling
   mutable std::map<CSegmentKey, WBFL::Stability::Girder> m_LiftingStabilityModels;
   mutable std::map<CSegmentKey, WBFL::Stability::Girder> m_HaulingStabilityModels;
   mutable std::map<CSegmentKey,WBFL::Stability::LiftingStabilityProblem> m_LiftingStabilityProblems;
   mutable std::map<CSegmentKey,WBFL::Stability::HaulingStabilityProblem> m_HaulingStabilityProblems;
   mutable WBFL::Stability::LiftingStabilityProblem m_LiftingDesignStabilityProblem;
   mutable WBFL::Stability::HaulingStabilityProblem m_HaulingDesignStabilityProblem;
   void ConfigureSegmentStabilityModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,WBFL::Stability::Girder* pGirder) const;
   void ConfigureSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& handlingConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD,WBFL::Stability::LiftingStabilityProblem* problem) const;
   void ConfigureSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& handlingConfig,ISegmentHaulingDesignPointsOfInterest* pPoiD,WBFL::Stability::HaulingStabilityProblem* problem) const;

   void Invalidate( Uint16 level );
   Uint16 Validate( Uint16 level );
   bool BuildCogoModel();
   bool BuildBridgeModel();
   bool BuildGirders();
   void ValidateGirders();

   // helper function for computing station range for roadway surface objects
   std::pair<Float64,Float64> ComputeReasonableSurfaceStationRange(const CBridgeDescription2* pBridgeDesc, const AlignmentData2& alignmentData, IAlignment* pAlignment);

   // helper functions for building the bridge model
   bool LayoutPiers(const CBridgeDescription2* pBridgeDesc);
   bool LayoutGirders(const CBridgeDescription2* pBridgeDesc);
   bool LayoutSsmHaunches();
   void GetHaunchDepth4ADimInput(const CPrecastSegmentData* pSegment,CComPtr<IDblArray>& pHaunchDepths);
   void GetHaunchDepth4BySpanInput(const CPrecastSegmentData* pSegment,const CBridgeDescription2* pBridgeDesc, CComPtr<IDblArray>& pHaunchDepths);
   bool LayoutDeck(const CBridgeDescription2* pBridgeDesc);
   bool LayoutOverlayDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutSimpleDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutFullDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutCompositeCIPDeck(const CBridgeDescription2* pBridgeDesc,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck);
   bool LayoutCompositeSIPDeck(const CBridgeDescription2* pBridgeDesc,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck);
   bool LayoutDeckCastingRegions(const CBridgeDescription2* pBridgeDesc, ICastingRegions* pCastingRegions);
   bool AssignDeckMaterial(const CBridgeDescription2* pBridgeDesc, IBridgeDeck* pDeck);

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
   void LayoutPoiForSlabCastingRegions(const CGirderKey& girderKey);
   void LayoutPoiForSlabBarCutoffs(const CGirderKey& girderKey);
   void LayoutPoiForSegmentBarCutoffs(const CSegmentKey& segmentKey,Float64 segmentOffset);
   void LayoutPoiForHandling(const CSegmentKey& segmentKey);
   void LayoutPoiForSectionChanges(const CSegmentKey& segmentKey);
   void LayoutPoiForTemporarySupports(const CSegmentKey& segmentKey);
   void LayoutPoiForPiers(const CSegmentKey& segmentKey);
   void LayoutPoiForTendons(const CGirderKey& girderKey);

   void ValidateSegmentOrientation(const CSegmentKey& segmentKey) const;

   void LayoutLiftingPoi(const CSegmentKey& segmentKey,Uint16 nPnts);
   void LayoutHaulingPoi(const CSegmentKey& segmentKey,Uint16 nPnts);
   void LayoutHandlingPoi(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey, Uint16 nPnts, PoiAttributeType attrib,pgsPoiMgr* pPoiMgr) const;
   void LayoutHandlingPoi(const CSegmentKey& segmentKey, Uint16 nPnts, Float64 leftOverhang, Float64 rightOverhang, PoiAttributeType attrib, PoiAttributeType supportAttribute,PoiAttributeType poiReference,pgsPoiMgr* pPoiMgr) const;

   void CheckBridge();

   Float64 GetHalfElevation(const pgsPointOfInterest& poi) const; // returns location of half height of composite girder

   void GetAlignment(IAlignment** ppAlignment) const;
   void GetProfile(IProfile** ppProfile) const;
   void GetBarrierProperties(pgsTypes::TrafficBarrierOrientation orientation,IShapeProperties** props) const;

   void InvalidateUserLoads();
   void ValidateUserLoads();
   void ValidatePointLoads();
   void ValidateDistributedLoads();
   void ValidateMomentLoads();

   ZoneIndexType GetPrimaryShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData) const;
   const CShearZoneData2* GetPrimaryShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData) const;
   ZoneIndexType GetPrimaryZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone) const;

   ZoneIndexType GetHorizInterfaceShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData) const;
   const CHorizontalInterfaceZoneData* GetHorizInterfaceShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData) const;
   ZoneIndexType GetHorizInterfaceZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone) const;

   Float64 GetPrimarySplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end, const CShearData2* pShearData) const;


   // Cache shear data - copies are expensive
   void InvalidateStirrupData();
   const CShearData2* GetShearData(const CSegmentKey& segmentKey) const;
   ZoneIndexType GetHorizInterfaceZoneCount(const CSegmentKey& segmentKey,const CShearData2* pShearData) const;


   typedef std::map<CSegmentKey, CShearData2> ShearDataMap;
   typedef ShearDataMap::const_iterator ShearDataIterator;
   mutable ShearDataMap  m_ShearData;

   mutable bool m_bDeckParametersValidated;
   mutable Float64 m_DeckSurfaceArea;
   mutable Float64 m_DeckVolume;
   void ValidateDeckParameters() const;
   void InvalidateDeckParameters();

   HRESULT GetSlabOverhangs(Float64 distance,Float64* pLeft,Float64* pRight) const;
   Float64 ConvertSegmentToBridgeLineCoordinate(const CSegmentKey& segmentKey,Float64 Xs) const;
   HRESULT GetGirderSection(const pgsPointOfInterest& poi,IGirderSection** gdrSection) const; // section is in girder section coordiantes
   HRESULT GetSuperstructureMember(const CGirderKey& girderKey,ISuperstructureMember* *ssmbr) const;
   HRESULT GetSegment(const CSegmentKey& segmentKey,ISuperstructureMemberSegment** segment) const;
   HRESULT GetGirder(const CSegmentKey& segmentKey,IPrecastGirder** girder) const;
   HRESULT GetGirder(const pgsPointOfInterest& poi,IPrecastGirder** girder) const;
   Float64 GetGrossSlabDepth() const;
   Float64 GetCastDepth() const;
   Float64 GetPanelDepth() const;
   Float64 GetSlabOverhangDepth(pgsTypes::SideType side) const;


   // Methods that return simple properties without data validation
   // Generally called during validation so as not to cause re-entry into the validation loop
   SpanIndexType GetSpanCount_Private() const;
   PierIndexType GetPierCount_Private() const;
   Float64 GetIxy_Private(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const;

   StrandIndexType GetNextNumStraightStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const;
   StrandIndexType GetNextNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum) const;
   StrandIndexType GetNextNumHarpedStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const;
   StrandIndexType GetNextNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum) const;
   StrandIndexType GetNextNumTempStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const;
   StrandIndexType GetNextNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum) const;
   StrandIndexType GetPrevNumStraightStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const;
   StrandIndexType GetPrevNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum) const;
   StrandIndexType GetPrevNumHarpedStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const;
   StrandIndexType GetPrevNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum) const;
   StrandIndexType GetPrevNumTempStrands(const CSegmentKey& segmentKeyr,StrandIndexType curNum) const;
   StrandIndexType GetPrevNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum) const;

   void GetSegmentShapeDirect(const pgsPointOfInterest& poi,IShape** ppShape) const;
   BarSize GetBarSize(WBFL::Materials::Rebar::Size size) const;
   RebarGrade GetRebarGrade(WBFL::Materials::Rebar::Grade grade) const;
   MaterialSpec GetRebarSpecification(WBFL::Materials::Rebar::Type type) const;


   Float64 GetAsTensionSideOfGirder(const pgsPointOfInterest& poi,bool bDevAdjust,bool bTensionTop) const;
   Float64 GetApsInHalfDepth(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust,bool bBottomHalf, const GDRCONFIG* pConfig=nullptr) const;

   Float64 GetGirderAptTensionSide(const pgsPointOfInterest& poi,bool bTensionTop) const;
   Float64 GetSegmentAptTensionSide(const pgsPointOfInterest& poi, bool bTensionTop) const;

   Float64 GetAsDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat) const;
   Float64 GetLocationDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat) const;
   void GetDeckMatData(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat,bool bAdjForDevLength,Float64* pAs,Float64* pYb) const;

   void GetShapeProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps) const;
   void GetShapeProperties(pgsTypes::SectionPropertyType sectPropType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps) const;
   Float64 GetCutLocation(const pgsPointOfInterest& poi) const;

   void NoDeckEdgePoint(GroupIndexType grpIdx,SegmentIndexType segIdx,pgsTypes::MemberEndType end,DirectionType side,IPoint2d** ppPoint) const;

   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const;
   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const;
   void GetCurbLinePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const;
   void GetCurbLinePoint(Float64 station, IDirection* direction,DirectionType side,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const;
   void CreateOverlayDeckEdgePaths(const CBridgeDescription2* pBridgeDesc,IPath** ppLeftPath,IPath** ppRightPath);

   std::shared_ptr<WBFL::Math::Function> CreateGirderProfile(const CSplicedGirderData* pGirder) const;
   std::shared_ptr<WBFL::Math::Function> CreateGirderBottomFlangeProfile(const CSplicedGirderData* pGirder) const;
   std::shared_ptr<WBFL::Math::Function> CreateGirderProfile(const CSplicedGirderData* pGirder,bool bGirderProfile) const;
   void GetSegmentRange(const CSegmentKey& segmentKey,Float64* pXStart,Float64* pXEnd) const;

   Float64 ConvertSegmentDuctOffsetToDuctElevation(const CSegmentKey& segmentKey, const CPrecastSegmentData* pSegment, Float64 Xs, Float64 offset, pgsTypes::FaceType offsetType) const;

   Float64 ConvertDuctOffsetToDuctElevation(const CGirderKey& girderKey,const CSplicedGirderData* pGirder,Float64 Xg,Float64 offset,CDuctGeometry::OffsetType offsetType) const;
   void CreateDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, const CLinearDuctGeometry& geometry,IPoint2dCollection** ppPoints) const;
   void CreateDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, const CParabolicDuctGeometry& geometry,IPoint2dCollection** ppPoints) const;
   void CreateDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, const COffsetDuctGeometry& geometry,IPoint2dCollection** ppPoints) const;

   std::unique_ptr<WBFL::Math::CompositeFunction> CreateDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, const CLinearDuctGeometry& geometry) const;
   std::unique_ptr<WBFL::Math::CompositeFunction> CreateDuctCenterline(const CGirderKey& girderKey, const CSplicedGirderData* pGirder, const CParabolicDuctGeometry& geometry) const;

   SegmentIndexType GetSegmentIndex(const CSplicedGirderData* pGirder,Float64 Xb) const;
   SegmentIndexType GetSegmentIndex(const CGirderKey& girderKey,ILine2d* pLine,IPoint2d** ppIntersection) const;

   SpanIndexType GetSpanIndex(Float64 Xb) const;
   PierIndexType GetGenericBridgePierIndex(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType) const;
   void GetGenericBridgePier(PierIndexType pierIdx,IBridgePier** ppPier) const;
   void GetGirderLine(const CSegmentKey& segmentKey,IGirderLine** ppGirderLine) const;
   void GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey,ISuperstructureMemberSegment** ppSegment) const;
   void GetPierLine(PierIndexType pierIdx,IPierLine** ppPierLine) const;
   void GetTemporarySupportLine(SupportIndexType tsIdx,IPierLine** ppPierLine) const;
   void GetSupports(const CSegmentKey& segmentKey, IPierLine** ppStartLine, IPierLine** ppEndLine) const;

   const GirderLibraryEntry* GetGirderLibraryEntry(const CGirderKey& girderKey) const;
   GroupIndexType GetGirderGroupAtPier(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const;

   // methods for plant installed tendons
   bool CreateTendons(const CPrecastSegmentData* pSegment, ISuperstructureMemberSegment* pSSMbrSegment, ITendonCollection** ppTendons) const;
   void CreateParabolicTendon(const CSegmentKey& segmentKey, DuctIndexType ductIdx, ISuperstructureMemberSegment* pSSMbrSegment, const CSegmentDuctData* pDuctGeometry, ITendonCollection** ppTendons) const;
   void CreateLinearTendon(const CSegmentKey& segmentKey, DuctIndexType ductIdx, ISuperstructureMemberSegment* pSSMbrSegment, const CSegmentDuctData* pDuctGeometry, ITendonCollection** ppTendons) const;

   // methods for field installed tendons
   bool CreateTendons(const CBridgeDescription2* pBridgeDesc,const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,ITendonCollection** ppTendons) const;
   void CreateParabolicTendon(const CGirderKey& girderKey,DuctIndexType ductIdx,ISuperstructureMember* pSSMbr,const CParabolicDuctGeometry& ductGeometry,ITendonCollection** ppTendons) const;
   void CreateLinearTendon(const CGirderKey& girderKey, DuctIndexType ductIdx, ISuperstructureMember* pSSMbr,const CLinearDuctGeometry& ductGeometry,ITendonCollection** ppTendons) const;
   void CreateOffsetTendon(const CGirderKey& girderKey, DuctIndexType ductIdx, ISuperstructureMember* pSSMbr,const COffsetDuctGeometry& ductGeometry,ITendonCollection* tendons,ITendonCollection** ppTendons) const;

   void CreateStrandMover(LPCTSTR strGirderName,Float64 Hg,pgsTypes::AdjustableStrandType adjType,IStrandMover** ppStrandMover) const;

   INCREMENTALRELAXATIONDETAILS GetIncrementalRelaxationDetails(Float64 fpi,const WBFL::Materials::PsStrand* pStrand,Float64 tStart,Float64 tEnd,Float64 tStress) const;

   /// Returns true if strands are engaged with the precast segment in the specified interval and location
   ///
   /// Strands are not considered to be engaged if the are off the segment, not yet installed, or have been removed as in the case of temporary strands
   bool AreStrandsEngaged(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType,const GDRCONFIG* pConfig) const;

   Float64 GetSuperstructureDepth(PierIndexType pierIdx) const;

   bool GirderLineIntersect(const CGirderKey& girderKey,ILine2d* pLine,SegmentIndexType segIdxHint,SegmentIndexType* pSegIdx,Float64* pXs) const;
   bool SegmentLineIntersect(const CSegmentKey& segmentKey,ILine2d* pLine,Float64* pXs) const;

   void ComputeHpFill(const GirderLibraryEntry* pGdrEntry,IStrandGridFiller* pStrandGridFiller, IIndexArray* pFill, IIndexArray** ppHPfill) const;

   Float64 ComputePierDiaphragmHeight(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const;
   Float64 ComputePierDiaphragmWidth(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const;

   REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(const CSegmentKey& segmentKey, IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct, bool bIsTopBar, bool bEpoxyCoated, bool bMeetsCoverRequirements) const;

   void ApplyDebonding(const CPrecastSegmentData* pSegment, IStrandGridModel* pStrandGridModel) const;

   void ResolveStrandRowElevations(const CSegmentKey& segmentKey, const CStrandRow& strandRow, const std::array<Float64, 4>& Xhp, std::array<Float64, 4>& Y) const;
   void CreateStrandModel(IPrecastGirder* girder,ISuperstructureMemberSegment* segment, const CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGirderEntry) const;

   Float64 GetOverallHeight(const pgsPointOfInterest& poi) const;

   const WBFL::Math::LinearFunction& GetGirderTopChordElevationFunction(const CSegmentKey& segmentKey) const;
   void ValidateGirderTopChordElevation(const CGirderKey& girderKey) const;
   void ValidateGirderTopChordElevationADimInput(const CGirderKey& girderKey,const CBridgeDescription2* pBridgeDesc,std::map<CSegmentKey,WBFL::Math::LinearFunction>* pFunctions) const;
   void ValidateGirderTopChordElevationDirectHaunchInput(const CGirderKey& girderKey,const CBridgeDescription2* pBridgeDesc,std::map<CSegmentKey,WBFL::Math::LinearFunction>* pFunctions) const;
   mutable std::map<CSegmentKey,WBFL::Math::LinearFunction> m_GirderTopChordElevationFunctions; // linear functions that represent the top girder chord elevations

   // Common function to return bearing elevation details at bearings or at girder edges
   enum BearingElevLocType { batBearings, batGirderEdges };
   std::vector<BearingElevationDetails> GetBearingElevationDetails_Generic(PierIndexType pierIdx,pgsTypes::PierFaceType face, BearingElevLocType locType,GirderIndexType gdrIdx,bool bIgnoreUnrecoverableDeformations) const;

   std::vector<IntermedateDiaphragm> CBridgeAgentImp::GetCastInPlaceDiaphragms(const CSpanKey& spanKey, bool bLocationOnly) const;
   Float64 GetHalfElevation(Float64 gdrHeight, Float64 deckThickness) const;

   void GetSlabPerimeter(PierIndexType startPierIdx, Float64 Xstart, PierIndexType endPierIdx, Float64 Xend, CollectionIndexType nPoints, pgsTypes::PlanCoordinateType pcType, const CCastDeckActivity* pActivity, IPoint2dCollection** points) const;

// Orientation of girder segments. cached from bridge geometry model builder
   GirderOrientationCollection  m_GirderOrientationCollection;
};

#endif //__BRIDGEAGENT_H_
