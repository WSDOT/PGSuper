///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include <boost\shared_ptr.hpp>

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
   CBridgeAgentImp()
	{
      m_Level        = 0;
      m_pBroker      = 0;

      m_AlignmentID = INVALID_ID;

      m_bUserLoadsValidated  = false;
	}

   HRESULT FinalConstruct();
   void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_BRIDGEAGENT)

BEGIN_COM_MAP(CBridgeAgentImp)
   COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(IRoadway)
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
   StatusCallbackIDType m_scidPointLoadWarning;
   StatusCallbackIDType m_scidDistributedLoadWarning;
   StatusCallbackIDType m_scidMomentLoadWarning;

// IAgent
public:
   STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker);
	STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// IRoadway
public:
   virtual Float64 GetSlope(Float64 station,Float64 offset);
   virtual Float64 GetProfileGrade(Float64 station);
   virtual Float64 GetElevation(Float64 station,Float64 offset);
   virtual void GetBearing(Float64 station,IDirection** ppBearing);
   virtual void GetBearingNormal(Float64 station,IDirection** ppNormal);
   virtual void GetPoint(Float64 station,Float64 offset,IDirection* pBearing,IPoint2d** ppPoint);
   virtual void GetStationAndOffset(IPoint2d* point,Float64* pStation,Float64* pOffset);
   virtual CollectionIndexType GetCurveCount();
   virtual void GetCurve(CollectionIndexType idx,IHorzCurve** ppCurve);
   virtual CollectionIndexType GetVertCurveCount();
   virtual void GetVertCurve(CollectionIndexType idx,IVertCurve** ppCurve);
   virtual void GetRoadwaySurface(Float64 station,IPoint2dCollection** ppPoints);

// IBridge
public:
   virtual Float64 GetLength();
   virtual Float64 GetSpanLength(SpanIndexType span);
   virtual Float64 GetGirderLayoutLength(const CGirderKey& girderKey);
   virtual Float64 GetGirderSpanLength(const CGirderKey& girderKey);
   virtual Float64 GetGirderLength(const CGirderKey& girderKey);
   virtual Float64 GetAlignmentOffset();
   virtual Float64 GetDistanceFromStartOfBridge(Float64 station);
   virtual Float64 GetDistanceFromStartOfBridge(const pgsPointOfInterest& poi);
   virtual SpanIndexType GetSpanCount();
   virtual PierIndexType GetPierCount();
   virtual SupportIndexType GetTemporarySupportCount();
   virtual GroupIndexType GetGirderGroupCount();
   virtual GirderIndexType GetGirderCount(GroupIndexType grpIdx);
   virtual GirderIndexType GetGirderCountBySpan(SpanIndexType spanIdx);
   virtual SegmentIndexType GetSegmentCount(const CGirderKey& girderKey);
   virtual SegmentIndexType GetSegmentCount(GroupIndexType grpIdx,GirderIndexType gdrIdx);
   virtual PierIndexType GetGirderGroupStartPier(GroupIndexType grpIdx);
   virtual PierIndexType GetGirderGroupEndPier(GroupIndexType grpIdx);
   virtual void GetGirderGroupPiers(GroupIndexType grpIdx,PierIndexType* pStartPierIdx,PierIndexType* pEndPierIdx);
   virtual SpanIndexType GetGirderGroupStartSpan(GroupIndexType grpIdx);
   virtual SpanIndexType GetGirderGroupEndSpan(GroupIndexType grpIdx);
   virtual void GetGirderGroupSpans(GroupIndexType grpIdx,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx);
   virtual GroupIndexType GetGirderGroupIndex(SpanIndexType spanIdx);
   virtual void GetGirderGroupIndex(PierIndexType pierIdx,GroupIndexType* pBackGroupIdx,GroupIndexType* pAheadGroupIdx);
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight);
   virtual void GetBottomFlangeClearance(const pgsPointOfInterest& poi,Float64* pLeft,Float64* pRight);
   virtual std::vector<Float64> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType);
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(Float64 station);
   virtual void GetSpacingAlongGirder(const CGirderKey& girderKey,Float64 Xg,Float64* leftSpacing,Float64* rightSpacing);
   virtual void GetSpacingAlongGirder(const pgsPointOfInterest& poi,Float64* leftSpacing,Float64* rightSpacing);
   virtual std::vector<std::pair<SegmentIndexType,Float64>> GetSegmentLengths(const CSpanKey& spanKey);
   virtual Float64 GetSegmentLength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentSpanLength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentLayoutLength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentPlanLength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentSlope(const CSegmentKey& segmentKey);
   virtual Float64 GetSlabOffset(GroupIndexType grpIdx,PierIndexType pierIdx,GirderIndexType gdrIdx);
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi);
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetSlabOffset(const CSegmentKey& segmentKey,Float64* pStart,Float64* pEnd);
   virtual Float64 GetElevationAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetRotationAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetSpanLength(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetSpanLength(const CSpanKey& spanKey);
   virtual Float64 GetFullSpanLength(const CSpanKey& spanKey);
   virtual Float64 GetCantileverLength(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType endType);
   virtual Float64 GetCantileverLength(const CSpanKey& spanKey,pgsTypes::MemberEndType endType);
   virtual Float64 GetSegmentStartEndDistance(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentEndEndDistance(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentStartBearingOffset(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentEndBearingOffset(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentStartSupportWidth(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentEndSupportWidth(const CSegmentKey& segmentKey);
   virtual Float64 GetCLPierToCLBearingDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure);
   virtual Float64 GetCLPierToSegmentEndDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure);
   virtual Float64 GetSegmentOffset(const CSegmentKey& segmentKey,Float64 station);
   virtual void GetSegmentAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle);
   virtual void GetSegmentBearing(const CSegmentKey& segmentKey,IDirection** ppBearing);
   virtual CSegmentKey GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey);
   virtual void GetSpansForSegment(const CSegmentKey& segmentKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx);
   virtual GDRCONFIG GetSegmentConfiguration(const CSegmentKey& segmentKey);
   virtual void ModelCantilevers(const CSegmentKey& segmentKey,bool* pbStartCantilever,bool* pbEndCantilever);
   virtual bool GetSpan(Float64 station,SpanIndexType* pSpanIdx);
   virtual void GetPoint(const CSegmentKey& segmentKey,Float64 Xpoi,IPoint2d** ppPoint);
   virtual void GetPoint(const pgsPointOfInterest& poi,IPoint2d** ppPoint);
   virtual bool GetSegmentPierIntersection(const CSegmentKey& segmentKey,PierIndexType pierIdx,IPoint2d** ppPoint);
   virtual bool GetSegmentTempSupportIntersection(const CSegmentKey& segmentKey,SupportIndexType tsIdx,IPoint2d** ppPoint);
   virtual void GetStationAndOffset(const CSegmentKey& segmentKey,Float64 Xpoi,Float64* pStation,Float64* pOffset);
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset);
   virtual bool IsInteriorGirder(const CGirderKey& girderKey);
   virtual bool IsExteriorGirder(const CGirderKey& girderKey);
   virtual bool IsLeftExteriorGirder(const CGirderKey& girderKey);
   virtual bool IsRightExteriorGirder(const CGirderKey& girderKey);
   virtual bool IsObtuseCorner(const CSpanKey& spanKey,pgsTypes::MemberEndType endType);
   virtual bool AreGirderTopFlangesRoughened(const CSegmentKey& segmentKey);
   virtual void GetClosureJointProfile(const CClosureKey& closureKey,IShape** ppShape);
   virtual Float64 GetClosureJointLength(const CClosureKey& closureKey);
   virtual void GetClosureJointSize(const CClosureKey& closureKey,Float64* pLeft,Float64* pRight);
   virtual void GetAngleBetweenSegments(const CClosureKey& closureKey,IAngle** ppAngle);
   virtual void GetBackSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH);
   virtual void GetAheadSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH);
   virtual bool DoesLeftSideEndDiaphragmLoadGirder(PierIndexType pier);
   virtual bool DoesRightSideEndDiaphragmLoadGirder(PierIndexType pier);
   virtual Float64 GetEndDiaphragmLoadLocationAtStart(const CSegmentKey& segmentKey);
   virtual Float64 GetEndDiaphragmLoadLocationAtEnd(const CSegmentKey& segmentKey);
   virtual std::vector<IntermedateDiaphragm> GetPrecastDiaphragms(const CSegmentKey& segmentKey);
   virtual std::vector<IntermedateDiaphragm> GetCastInPlaceDiaphragms(const CSpanKey& spanKey);
   virtual pgsTypes::SupportedDeckType GetDeckType();
   virtual bool IsCompositeDeck();
   virtual bool HasOverlay();
   virtual bool IsFutureOverlay();
   virtual Float64 GetOverlayWeight();
   virtual Float64 GetOverlayDepth();
   virtual Float64 GetSacrificalDepth();
   virtual Float64 GetFillet();
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetLeftSlabEdgeOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetRightSlabEdgeOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetLeftSlabOverhang(Float64 distFromStartOfBridge);
   virtual Float64 GetRightSlabOverhang(Float64 distFromStartOfBridge);
   virtual Float64 GetLeftSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan);
   virtual Float64 GetRightSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan);
   virtual Float64 GetLeftSlabOverhang(PierIndexType pier);
   virtual Float64 GetRightSlabOverhang(PierIndexType pier);
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pier);
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pier);
   virtual Float64 GetLeftCurbOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetRightCurbOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetLeftCurbOffset(SpanIndexType span,Float64 distFromStartOfSpan);
   virtual Float64 GetRightCurbOffset(SpanIndexType span,Float64 distFromStartOfSpan);
   virtual Float64 GetLeftCurbOffset(PierIndexType pier);
   virtual Float64 GetRightCurbOffset(PierIndexType pier);
   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetCurbToCurbWidth(const CSegmentKey& segmentKey,Float64 distFromStartOfSpan);
   virtual Float64 GetCurbToCurbWidth(Float64 distFromStartOfBridge);
   virtual Float64 GetLeftInteriorCurbOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetRightInteriorCurbOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetLeftOverlayToeOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetRightOverlayToeOffset(Float64 distFromStartOfBridge);
   virtual Float64 GetLeftOverlayToeOffset(const pgsPointOfInterest& poi);
   virtual Float64 GetRightOverlayToeOffset(const pgsPointOfInterest& poi);
   virtual void GetSlabPerimeter(CollectionIndexType nPoints,IPoint2dCollection** points);
   virtual void GetSlabPerimeter(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,CollectionIndexType nPoints,IPoint2dCollection** points);
   virtual void GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,IPoint2dCollection** points);
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point);
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point);
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point);
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point);
   virtual Float64 GetPierStation(PierIndexType pier);
   virtual Float64 GetAheadBearingStation(PierIndexType pier,const CGirderKey& girderKey);
   virtual Float64 GetBackBearingStation(PierIndexType pier,const CGirderKey& girderKey);
   virtual void GetPierDirection(PierIndexType pier,IDirection** ppDirection);
   virtual void GetPierSkew(PierIndexType pier,IAngle** ppAngle);
   virtual void GetPierPoints(PierIndexType pier,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right);
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight);
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight);
   virtual void GetContinuityEventIndex(PierIndexType pierIdx,EventIndexType* pLeft,EventIndexType* pRight);
   virtual bool GetPierLocation(PierIndexType pierIdx,const CSegmentKey& segmentKey,Float64* pXs);
   virtual bool GetPierLocation(const CGirderKey& girderKey,PierIndexType pierIdx,Float64* pXgp);
   virtual bool GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew);
   virtual pgsTypes::PierModelType GetPierModelType(PierIndexType pierIdx);
   virtual ColumnIndexType GetColumnCount(PierIndexType pierIdx);
   virtual void GetColumnProperties(PierIndexType pierIdx,Float64* pHeight,Float64* pA,Float64* pI,Float64* pE);
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx);
   virtual pgsTypes::BoundaryConditionType GetBoundaryConditionType(PierIndexType pierIdx);
   virtual pgsTypes::PierSegmentConnectionType GetSegmentConnectionType(PierIndexType pierIdx);
   virtual bool IsAbutment(PierIndexType pierIdx);
   virtual bool IsPier(PierIndexType pierIdx);
   virtual bool HasCantilever(PierIndexType pierIdx);
   virtual bool IsInteriorPier(PierIndexType pierIdx);
   virtual bool IsBoundaryPier(PierIndexType pierIdx);
   virtual void GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx,SpanIndexType* pSpanIdx,Float64* pXspan);
   virtual bool GetTemporarySupportLocation(SupportIndexType tsIdx,const CSegmentKey& segmentKey,Float64* pXs);
   virtual Float64 GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx);
   virtual pgsTypes::TemporarySupportType GetTemporarySupportType(SupportIndexType tsIdx);
   virtual pgsTypes::TempSupportSegmentConnectionType GetSegmentConnectionTypeAtTemporarySupport(SupportIndexType tsIdx);
   virtual void GetSegmentsAtTemporarySupport(GirderIndexType gdrIdx,SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey);
   virtual void GetTemporarySupportDirection(SupportIndexType tsIdx,IDirection** ppDirection);

// IMaterials
public:
   virtual Float64 GetSegmentFc28(const CSegmentKey& segmentKey);
   virtual Float64 GetClosureJointFc28(const CSegmentKey& closureKey);
   virtual Float64 GetDeckFc28();
   virtual Float64 GetRailingSystemFc28(pgsTypes::TrafficBarrierOrientation orientation);

   virtual Float64 GetSegmentEc28(const CSegmentKey& segmentKey);
   virtual Float64 GetClosureJointEc28(const CSegmentKey& closureKey);
   virtual Float64 GetDeckEc28();
   virtual Float64 GetRailingSystemEc28(pgsTypes::TrafficBarrierOrientation orientation);

   virtual Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointWeightDensity(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetDeckWeightDensity(IntervalIndexType intervalIdx);
   virtual Float64 GetRailingSystemWeightDensity(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentConcreteAge(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetClosureJointConcreteAge(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetDeckConcreteAge(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetRailingSystemAge(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);

   virtual Float64 GetSegmentFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetDeckFc(IntervalIndexType intervalIdx);
   virtual Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged);
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged);
   virtual Float64 GetDeckEc(IntervalIndexType intervalIdx);
   virtual Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointFlexureFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointShearFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetDeckFlexureFr(IntervalIndexType intervalIdx);
   virtual Float64 GetDeckShearFr(IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointAgingCoefficient(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetDeckAgingCoefficient(IntervalIndexType intervalIdx);
   virtual Float64 GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentAgeAdjustedEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointAgeAdjustedEc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetDeckAgeAdjustedEc(IntervalIndexType intervalIdx);
   virtual Float64 GetRailingSystemAgeAdjustedEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time);
   virtual Float64 GetClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time);
   virtual Float64 GetDeckFreeShrinkageStrain(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time);
   virtual Float64 GetRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);

   virtual Float64 GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
   virtual Float64 GetClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx);
   virtual Float64 GetDeckFreeShrinkageStrain(IntervalIndexType intervalIdx);
   virtual Float64 GetRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx);

   virtual Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetClosureJointCreepCoefficient(const CSegmentKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetDeckCreepCoefficient(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType);

   virtual pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey);
   virtual bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentEccK1(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentEccK2(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey);
   virtual const matConcreteBase* GetSegmentConcrete(const CSegmentKey& segmentKey);
   virtual pgsTypes::ConcreteType GetClosureJointConcreteType(const CClosureKey& closureKey);
   virtual bool DoesClosureJointConcreteHaveAggSplittingStrength(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointConcreteAggSplittingStrength(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointStrengthDensity(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointMaxAggrSize(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointEccK1(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointEccK2(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointCreepK1(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointCreepK2(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointShrinkageK1(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointShrinkageK2(const CClosureKey& closureKey);
   virtual const matConcreteBase* GetClosureJointConcrete(const CClosureKey& closureKey);
   virtual pgsTypes::ConcreteType GetDeckConcreteType();
   virtual bool DoesDeckConcreteHaveAggSplittingStrength();
   virtual Float64 GetDeckConcreteAggSplittingStrength();
   virtual Float64 GetDeckMaxAggrSize();
   virtual Float64 GetDeckEccK1();
   virtual Float64 GetDeckEccK2();
   virtual Float64 GetDeckCreepK1();
   virtual Float64 GetDeckCreepK2();
   virtual Float64 GetDeckShrinkageK1();
   virtual Float64 GetDeckShrinkageK2();
   virtual const matConcreteBase* GetDeckConcrete();
   virtual const matPsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   virtual Float64 GetStrandRelaxation(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType);
   virtual const matPsStrand* GetTendonMaterial(const CGirderKey& girderKey);
   virtual Float64 GetTendonRelaxation(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso);
   virtual void GetSegmentLongitudinalRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu);
   virtual std::_tstring GetSegmentLongitudinalRebarName(const CSegmentKey& segmentKey);
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade);
   virtual void GetClosureJointLongitudinalRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu);
   virtual std::_tstring GetClosureJointLongitudinalRebarName(const CClosureKey& closureKey);
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade);
   virtual void GetSegmentTransverseRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu);
   virtual void GetSegmentTransverseRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade);
   virtual std::_tstring GetSegmentTransverseRebarName(const CSegmentKey& segmentKey);
   virtual void GetClosureJointTransverseRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu);
   virtual void GetClosureJointTransverseRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade);
   virtual std::_tstring GetClosureJointTransverseRebarName(const CClosureKey& closureKey);
   virtual void GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu);
   virtual std::_tstring GetDeckRebarName();
   virtual void GetDeckRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade);
   virtual Float64 GetNWCDensityLimit();
   virtual Float64 GetLWCDensityLimit();
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type);
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type);
   virtual Float64 GetFlexureFrCoefficient(const CSegmentKey& segmentKey);
   virtual Float64 GetShearFrCoefficient(const CSegmentKey& segmentKey);
   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2);

// ILongRebarGeometry
public:
   virtual void GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection);
   virtual Float64 GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem);
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct);
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetCoverTopMat();
   virtual Float64 GetTopMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory);
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory);
   virtual Float64 GetCoverBottomMat();
   virtual Float64 GetBottomMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory);
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory);
   virtual void GetDeckReinforcing(const pgsPointOfInterest& poi,pgsTypes::DeckRebarMatType matType,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bAdjForDevLength,Float64* pAs,Float64* pYb);
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetRebarLayout(const CSegmentKey& segmentKey, IRebarLayout** rebarLayout);
   virtual void GetClosureJointRebarLayout(const CClosureKey& closureKey, IRebarLayout** rebarLayout);
   virtual REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(IRebar* rebar, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct);

// IStirrupGeometry
public:
   virtual bool AreStirrupZonesSymmetrical(const CSegmentKey& segmentKey);

   virtual ZoneIndexType GetPrimaryZoneCount(const CSegmentKey& segmentKey);
   virtual void GetPrimaryZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end);
   virtual void GetPrimaryVertStirrupBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing);
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const CSegmentKey& segmentKey,ZoneIndexType zone);
   virtual matRebar::Size GetPrimaryConfinementBarSize(const CSegmentKey& segmentKey,ZoneIndexType zone);

   virtual ZoneIndexType GetHorizInterfaceZoneCount(const CSegmentKey& segmentKey);
   virtual void GetHorizInterfaceZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end);
   virtual void GetHorizInterfaceBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing);

   virtual void GetAddSplittingBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing);
   virtual void GetAddConfinementBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing);

   virtual Float64 GetVertStirrupAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing);
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi);
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi); // stirrup angle=90 for vertical

   virtual bool DoStirrupsEngageDeck(const CSegmentKey& segmentKey);
   virtual bool DoAllPrimaryStirrupsEngageDeck(const CSegmentKey& segmentKey);
   virtual Float64 GetPrimaryHorizInterfaceBarSpacing(const pgsPointOfInterest& poi);
   virtual Float64 GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing);
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi);
   virtual Float64 GetAdditionalHorizInterfaceBarSpacing(const pgsPointOfInterest& poi);
   virtual Float64 GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing);
   virtual Float64 GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi);

   virtual Float64 GetSplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end);

   virtual void GetStartConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing);
   virtual void GetEndConfinementBarInfo(  const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing);

   virtual bool AreStirrupZoneLengthsCombatible(const CGirderKey& girderKey);

private:
   ZoneIndexType GetPrimaryShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   const CShearZoneData2* GetPrimaryShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   ZoneIndexType GetPrimaryZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone);

   ZoneIndexType GetHorizInterfaceShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   const CHorizontalInterfaceZoneData* GetHorizInterfaceShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData);
   ZoneIndexType GetHorizInterfaceZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone);

   bool IsPoiInEndRegion(const pgsPointOfInterest& poi, Float64 distFromEnds);

   virtual Float64 GetPrimarySplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end, const CShearData2* pShearData);


   // Cache shear data - copies are expensive
   void InvalidateStirrupData();
   const CShearData2* GetShearData(const CSegmentKey& segmentKey);

   typedef std::map<CSegmentKey, CShearData2> ShearDataMap;
   typedef ShearDataMap::const_iterator ShearDataIterator;
   ShearDataMap  m_ShearData;


// IStrandGeometry
public:
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands);
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands);

   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands);
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands);

   virtual Float64 GetStrandLocation(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, IntervalIndexType intervalIdx);
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi);
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi);

   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands);
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, pgsTypes::StrandType strandType, Float64* nEffectiveStrands);
   
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands);
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, pgsTypes::StrandType strandType,Float64* nEffectiveStrands);

   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift);
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift);

   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust);
   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, DevelopmentAdjustmentType devAdjust);
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust);
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig,DevelopmentAdjustmentType devAdjust);

   virtual StrandIndexType GetStrandCount(const CSegmentKey& segmentKey,pgsTypes::StrandType type);
   virtual StrandIndexType GetMaxStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type);
   virtual StrandIndexType GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type);
   virtual Float64 GetStrandArea(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType);
   virtual Float64 GetStrandArea(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType);
   virtual Float64 GetAreaPrestressStrands(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,bool bIncTemp);

   virtual Float64 GetPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type);
   virtual Float64 GetPjack(const CSegmentKey& segmentKey,bool bIncTemp);
   virtual void GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint);
   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints);
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints);
   virtual void GetStrandPositionsEx(LPCTSTR strGirderName, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type,pgsTypes::MemberEndType endType, IPoint2dCollection** ppPoints);

   // Harped strands can be forced to be straight along their length
   virtual bool GetAreHarpedStrandsForcedStraight(const CSegmentKey& segmentKey);

   virtual void GetHarpedStrandControlHeights(const CSegmentKey& segmentKey,Float64* pHgStart,Float64* pHgHp1,Float64* pHgHp2,Float64* pHgEnd);

     // highest point on girder section based on strand coordinates (bottom at 0.0)
   virtual Float64 GetGirderTopElevation(const CSegmentKey& segmentKey);
   // harped offsets are measured from original strand locations in strand grid
   virtual void GetHarpStrandOffsets(const CSegmentKey& segmentKey,Float64* pOffsetEnd,Float64* pOffsetHp);

   virtual void GetHarpedEndOffsetBounds(const CSegmentKey& segmentKey,Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedEndOffsetBoundsEx(const CSegmentKey& segmentKey,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType,Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedHpOffsetBounds(const CSegmentKey& segmentKey,Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedHpOffsetBoundsEx(const CSegmentKey& segmentKey,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType, Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset);

   virtual Float64 GetHarpedEndOffsetIncrement(const CSegmentKey& segmentKey);
   virtual Float64 GetHarpedHpOffsetIncrement(const CSegmentKey& segmentKey);
   virtual Float64 GetHarpedEndOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType);
   virtual Float64 GetHarpedHpOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType);

   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* lhp,Float64* rhp);
   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* pX1,Float64* pX2,Float64* pX3,Float64* pX4);
   virtual void GetHighestHarpedStrandLocation(const CSegmentKey& segmentKey,Float64* pElevation);
   virtual IndexType GetNumHarpPoints(const CSegmentKey& segmentKey);

   virtual StrandIndexType GetMaxNumPermanentStrands(const CSegmentKey& segmentKey);
   virtual StrandIndexType GetMaxNumPermanentStrands(LPCTSTR strGirderName);
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,const CSegmentKey& segmentKey, StrandIndexType* numStraight, StrandIndexType* numHarped);
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped);
   virtual StrandIndexType GetNextNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   virtual StrandIndexType GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   virtual StrandIndexType GetPreviousNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum);
   virtual StrandIndexType GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum);
   virtual bool ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices);


   virtual bool IsValidNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual bool IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual StrandIndexType GetNextNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual StrandIndexType GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual StrandIndexType GetPrevNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual StrandIndexType GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum);

   virtual StrandIndexType GetNumExtendedStrands(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType standType);
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config);

   virtual ConfigStrandFillVector ComputeStrandFill(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType Ns);
   virtual ConfigStrandFillVector ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns);

   virtual GridIndexType SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo);
   virtual void GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2);

   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64* pStart,Float64* pEnd);
   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64* pStart,Float64* pEnd);
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual StrandIndexType GetNumDebondedStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   virtual RowIndexType GetNumRowsWithStrand(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType );
   virtual StrandIndexType GetNumStrandInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType );
   virtual std::vector<StrandIndexType> GetStrandsInRow(const CSegmentKey& segmentKey, RowIndexType rowIdx, pgsTypes::StrandType strandType );
   virtual StrandIndexType GetNumDebondedStrandsInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType );
   virtual bool IsExteriorStrandDebondedInRow(const CSegmentKey& segmentKey,RowIndexType rowIdx,pgsTypes::StrandType strandType);
   virtual bool IsDebondingSymmetric(const CSegmentKey& segmentKey);

   virtual RowIndexType GetNumRowsWithStrand(const CSegmentKey& segmentKey,StrandIndexType nStrands,pgsTypes::StrandType strandType );
   virtual StrandIndexType GetNumStrandInRow(const CSegmentKey& segmentKey,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType );
   virtual std::vector<StrandIndexType> GetStrandsInRow(const CSegmentKey& segmentKey,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType );

   virtual Float64 GetDebondSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);
   virtual SectionIndexType GetNumDebondSections(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType);
   virtual StrandIndexType GetNumDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);
   virtual StrandIndexType GetNumBondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);
   virtual std::vector<StrandIndexType> GetDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);

   virtual bool CanDebondStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType); // can debond any of the strands
   virtual bool CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType);
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list);
   virtual void ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list); 
   virtual Float64 GetDefaultDebondLength(const CSegmentKey& segmentKey);

   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset);
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset);
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset);
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset);
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset);
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset);
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset);
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset);
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange);
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange);
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange);
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange);
   virtual Float64 ConvertHarpedOffsetEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType);
   virtual Float64 ConvertHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType);
   virtual Float64 ConvertHarpedOffsetHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType);
   virtual Float64 ConvertHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType);

// IPointOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey);
   virtual pgsPointOfInterest GetPointOfInterest(PoiIDType poiID);
   virtual pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs);
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR);
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSpanKey& spanKey);
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(const CSpanKey& spanKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR);
   virtual std::vector<pgsPointOfInterest> GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey);
   virtual std::vector<pgsPointOfInterest> GetCriticalSections(pgsTypes::LimitState ls,const CGirderKey& girderKey,const GDRCONFIG& config);
   virtual pgsPointOfInterest GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xs);
   virtual pgsPointOfInterest GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR);
   virtual pgsPointOfInterest GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib = 0,Uint32 mode = POIFIND_OR);
   virtual pgsPointOfInterest GetPierPointOfInterest(const CGirderKey& girderKey,PierIndexType pierIdx);
   virtual pgsPointOfInterest GetTemporarySupportPointOfInterest(const CGirderKey& girderKey,SupportIndexType tsIdx);
   virtual void RemovePointsOfInterest(std::vector<pgsPointOfInterest>& vPoi,PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute);
   virtual bool IsInClosureJoint(const pgsPointOfInterest& poi,CClosureKey* pClosureKey);
   virtual bool IsOnSegment(const pgsPointOfInterest& poi);
   virtual bool IsOffSegment(const pgsPointOfInterest& poi);
   virtual bool IsOnGirder(const pgsPointOfInterest& poi);
   virtual bool IsInIntermediateDiaphragm(const pgsPointOfInterest& poi);
   virtual pgsPointOfInterest ConvertSpanPointToPoi(const CSpanKey& spanKey,Float64 Xspan);
   virtual void ConvertPoiToSpanPoint(const pgsPointOfInterest& poi,CSpanKey* pSpanKey,Float64* pXspan);
   virtual void ConvertSpanPointToSegmentCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXs);
   virtual void ConvertSegmentCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xs,CSpanKey* pSpanKey,Float64* pXspan);
   virtual void ConvertSpanPointToSegmentPathCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXsp);
   virtual void ConvertSegmentPathCoordinateToSpanPoint(const CSegmentKey& sSegmentKey,Float64 Xsp,CSpanKey* pSpanKey,Float64* pXspan);
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterestInRange(Float64 xLeft,const pgsPointOfInterest& poi,Float64 xRight);
   virtual std::list<std::vector<pgsPointOfInterest>> GroupBySegment(const std::vector<pgsPointOfInterest>& vPoi);
   virtual std::list<std::vector<pgsPointOfInterest>> GroupByGirder(const std::vector<pgsPointOfInterest>& vPoi);
   virtual std::vector<CSegmentKey> GetSegmentKeys(const std::vector<pgsPointOfInterest>& vPoi);
   virtual std::vector<CGirderKey> GetGirderKeys(const std::vector<pgsPointOfInterest>& vPoi);
   virtual Float64 ConvertPoiToSegmentPathCoordinate(const pgsPointOfInterest& poi);
   virtual pgsPointOfInterest ConvertSegmentPathCoordinateToPoi(const CSegmentKey& segmentKey,Float64 Xsp);
   virtual Float64 ConvertSegmentPathCoordinateToSegmentCoordinate(const CSegmentKey& segmentKey,Float64 Xsp);
   virtual Float64 ConvertSegmentCoordinateToGirderCoordinate(const CSegmentKey& segmentKey,Float64 Xs);
   //virtual void ConvertGirderCoordinateToSegmentCoordinate(const CGirderKey& girderKey,Float64 Xg,CSegmentKey* pSegmentKey,Float64* pXs);
   virtual Float64 ConvertSegmentCoordinateToGirderlineCoordinate(const CSegmentKey& segmentKey,Float64 Xs);
   virtual Float64 ConvertSegmentPathCoordinateToGirderPathCoordinate(const CSegmentKey& segmentKey,Float64 Xsp);
   virtual Float64 ConvertSegmentCoordinateToSegmentPathCoordinate(const CSegmentKey& segmentKey,Float64 Xs);
   virtual Float64 ConvertPoiToGirderCoordinate(const pgsPointOfInterest& poi);
   virtual pgsPointOfInterest ConvertGirderCoordinateToPoi(const CGirderKey& girderKey,Float64 Xg);
   virtual Float64 ConvertPoiToGirderPathCoordinate(const pgsPointOfInterest& poi);
   virtual pgsPointOfInterest ConvertGirderPathCoordinateToPoi(const CGirderKey& girderKey,Float64 Xgp);
   virtual Float64 ConvertGirderCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float64 Xg);
   virtual Float64 ConvertGirderPathCoordinateToGirderCoordinate(const CGirderKey& girderKey,Float64 Xgp);
   virtual Float64 ConvertGirderPathCoordinateToGirderlineCoordinate(const CGirderKey& girderKey,Float64 Xgp);
   virtual Float64 ConvertPoiToGirderlineCoordinate(const pgsPointOfInterest& poi);
   virtual pgsPointOfInterest ConvertGirderlineCoordinateToPoi(GirderIndexType gdrIdx,Float64 Xgl);

// ISectionProperties
public:
   virtual pgsTypes::SectionPropertyMode GetSectionPropertiesMode();
   virtual Float64 GetHg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location);
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location);
   virtual Float64 GetKt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetKb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetEIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);

   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr);
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr);

   virtual Float64 GetHg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location);
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location);
   virtual Float64 GetKt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetKb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetEIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);

   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc);
   virtual Float64 GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc);
   virtual Float64 GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc);
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc);
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc);

   virtual Float64 GetNetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetIg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetYbg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetYtg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetAd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetId(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetYbd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetNetYtd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);

   virtual Float64 GetQSlab(const pgsPointOfInterest& poi);
   virtual Float64 GetAcBottomHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetAcTopHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetEffectiveFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetTributaryFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetTributaryFlangeWidthEx(const pgsPointOfInterest& poi, Float64* pLftFw, Float64* pRgtFw);
   virtual Float64 GetEffectiveDeckArea(const pgsPointOfInterest& poi);
   virtual Float64 GetTributaryDeckArea(const pgsPointOfInterest& poi);
   virtual Float64 GetGrossDeckArea(const pgsPointOfInterest& poi);
   virtual Float64 GetDistTopSlabToTopGirder(const pgsPointOfInterest& poi);
   virtual void ReportEffectiveFlangeWidth(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi);
   virtual Float64 GetSegmentSurfaceArea(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentVolume(const CSegmentKey& segmentKey);
   virtual Float64 GetClosureJointSurfaceArea(const CClosureKey& closureKey);
   virtual Float64 GetClosureJointVolume(const CClosureKey& closureKey);
   virtual Float64 GetDeckSurfaceArea();
   virtual Float64 GetDeckVolume();
   virtual Float64 GetBridgeEIxx(Float64 Xb);
   virtual Float64 GetBridgeEIyy(Float64 Xb);
   virtual Float64 GetSegmentWeightPerLength(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentWeight(const CSegmentKey& segmentKey);
   virtual Float64 GetSegmentHeightAtPier(const CSegmentKey& segmentKey,PierIndexType pierIdx);
   virtual Float64 GetSegmentHeightAtTemporarySupport(const CSegmentKey& segmentKey,SupportIndexType tsIdx);

// IShapes
public:
   virtual void GetSegmentShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bOrient,pgsTypes::SectionCoordinateType coordinateType,IShape** ppShape);
   virtual void GetSlabShape(Float64 station,IShape** ppShape);
   virtual void GetLeftTrafficBarrierShape(Float64 station,IShape** ppShape);
   virtual void GetRightTrafficBarrierShape(Float64 station,IShape** ppShape);


// IBarriers
public:
   virtual Float64 GetAtb(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetItb(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetYbtb(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetExteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation);
   virtual bool HasInteriorBarrier(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetInteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetInteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation);
   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(const CSegmentKey& segmentKey);
   virtual Float64 GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation);
   virtual void GetSidewalkDeadLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge);
   virtual void GetSidewalkPedLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge);
   virtual bool HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation);

// ISegmentLiftingPointsOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetLiftingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR);
   virtual std::vector<pgsPointOfInterest> GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType attrib,Uint32 mode = POIFIND_OR);

// ISegmentHaulingPointsOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode = POIFIND_OR);
   virtual std::vector<pgsPointOfInterest> GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode = POIFIND_OR);
   virtual Float64 GetMinimumOverhang(const CSegmentKey& segmentKey);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint);
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName);
   virtual HRESULT OnConstructionLoadChanged();

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged();

// IUserDefinedLoads
public:
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey);
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey);
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IUserDefinedLoads::UserDefinedLoadCase loadCase);
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IUserDefinedLoads::UserDefinedLoadCase loadCase);
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx);
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx);
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase);
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase);
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx);
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx);
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase);
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase);
   virtual const std::vector<UserPointLoad>* GetPointLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey);
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey);
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey);

// ITempSupport
public:
   virtual void GetControlPoints(SupportIndexType tsIdx,IPoint2d** ppLeft,IPoint2d** ppAlignment_pt,IPoint2d** ppBridge_pt,IPoint2d** ppRight);
   virtual void GetDirection(SupportIndexType tsIdx,IDirection** ppDirection);
   virtual void GetSkew(SupportIndexType tsIdx,IAngle** ppAngle);

// IGirder
public:
   virtual bool    IsPrismatic(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey);
   virtual bool    IsSymmetric(IntervalIndexType intervalIdx,const CGirderKey& girderKey); 
   virtual MatingSurfaceIndexType  GetNumberOfMatingSurfaces(const CGirderKey& girderKey);
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx);
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx);
   virtual FlangeIndexType GetNumberOfTopFlanges(const CGirderKey& girderKey);
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi);
   virtual FlangeIndexType GetNumberOfBottomFlanges(const CSegmentKey& segmentKey);
   virtual Float64 GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetBottomWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetMinWebWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetMinTopFlangeThickness(const pgsPointOfInterest& poi);
   virtual Float64 GetMinBottomFlangeThickness(const pgsPointOfInterest& poi);
   virtual Float64 GetHeight(const pgsPointOfInterest& poi);
   virtual Float64 GetShearWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetShearInterfaceWidth(const pgsPointOfInterest& poi);
   virtual WebIndexType GetWebCount(const CGirderKey& girderKey);
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx);
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx);
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx);
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi);
   virtual Float64 GetWebWidth(const pgsPointOfInterest& poi);
   virtual void GetSegmentEndPoints(const CSegmentKey& segmentKey,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2);
   virtual Float64 GetOrientation(const CSegmentKey& segmentKey);
   virtual Float64 GetTopGirderReferenceChordElevation(const pgsPointOfInterest& poi);
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIdx);
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx);
   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi);
   virtual pgsTypes::SplittingDirection GetSplittingDirection(const CGirderKey& girderKey);
   virtual void GetProfileShape(const CSegmentKey& segmentKey,IShape** ppShape);
   virtual bool HasShearKey(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType);
   virtual void GetShearKeyAreas(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint);
   virtual void GetSegment(const CGirderKey& girderKey,Float64 distFromStartOfGirder,SegmentIndexType* pSegIdx,Float64* pDistFromStartOfSegment);
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IShape** ppShape);
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IShape** ppShape);
   virtual Float64 GetSegmentHeight(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64 Xsp);
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IPoint2dCollection** points);
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IPoint2dCollection** points);
   virtual void GetSegmentDirection(const CSegmentKey& segmentKey,IDirection** ppDirection);
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,Float64* pStartEndDistance,Float64* pEndEndDistance);
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,Float64* pStartEndDistance,Float64* pEndEndDistance);
   virtual void GetSegmentBearingOffset(const CSegmentKey& segmentKey,Float64* pStartBearingOffset,Float64* pEndBearingOffset);
   virtual void GetSegmentStorageSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd);

// ITendonGeometry
public:
   DuctIndexType GetDuctCount(const CGirderKey& girderKey);
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint2dCollection** ppPoints);
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,const CSplicedGirderData* pGirder,IPoint2dCollection** ppPoints);
   virtual void GetDuctPoint(const pgsPointOfInterest& poit,DuctIndexType ductIdx,IPoint2d** ppPoint);
   virtual void GetDuctPoint(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IPoint2d** ppPoint);
   virtual Float64 GetOutsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetInsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetInsideDuctArea(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual StrandIndexType GetTendonStrandCount(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetTendonArea(const CGirderKey& girderKey,IntervalIndexType intervalIdx,DuctIndexType ductIdx);
   virtual void GetTendonSlope(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IVector3d** ppSlope);
   virtual void GetTendonSlope(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IVector3d** ppSlope);
   virtual Float64 GetMinimumRadiusOfCurvature(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetPjack(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetFpj(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetDuctOffset(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx);
   virtual Float64 GetDuctLength(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx);
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx);
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,DuctIndexType ductIdx);
   virtual pgsTypes::JackingEndType GetJackingEnd(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetAptTopHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetAptBottomHalf(const pgsPointOfInterest& poi);

// IIntervals
public:
   virtual IntervalIndexType GetIntervalCount();
   virtual EventIndexType GetStartEvent(IntervalIndexType idx); 
   virtual EventIndexType GetEndEvent(IntervalIndexType idx); 
   virtual Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType);
   virtual Float64 GetDuration(IntervalIndexType idx);
   virtual LPCTSTR GetDescription(IntervalIndexType idx);
   virtual IntervalIndexType GetInterval(EventIndexType eventIdx);
   virtual IntervalIndexType GetErectPierInterval(PierIndexType pierIdx);
   virtual IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetLiftSegmentInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetHaulSegmentInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey);
   virtual bool IsSegmentErectionInterval(IntervalIndexType intervalIdx);
   virtual bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx);
   virtual IntervalIndexType GetTemporaryStrandInstallationInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey);
   virtual IntervalIndexType GetCastClosureJointInterval(const CClosureKey& closureKey);
   virtual IntervalIndexType GetCompositeClosureJointInterval(const CClosureKey& closureKey);
   virtual void GetContinuityInterval(const CGirderKey& girderKey,PierIndexType pierIdx,IntervalIndexType* pBack,IntervalIndexType* pAhead);
   virtual IntervalIndexType GetCastDeckInterval();
   virtual IntervalIndexType GetCompositeDeckInterval();
   virtual IntervalIndexType GetLiveLoadInterval();
   virtual IntervalIndexType GetLoadRatingInterval();
   virtual IntervalIndexType GetOverlayInterval();
   virtual IntervalIndexType GetInstallRailingSystemInterval();
   virtual IntervalIndexType GetFirstTendonStressingInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetLastTendonStressingInterval(const CGirderKey& girderKey);
   virtual IntervalIndexType GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual bool IsTendonStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx);
   virtual IntervalIndexType GetTemporarySupportErectionInterval(SupportIndexType tsIdx);
   virtual IntervalIndexType GetTemporarySupportRemovalInterval(SupportIndexType tsIdx);
   virtual std::vector<IntervalIndexType> GetTemporarySupportRemovalIntervals(GroupIndexType groupIdx);
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey);
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey,ProductForceType pfType);
   virtual std::vector<IntervalIndexType> GetSpecCheckIntervals(const CGirderKey& girderKey);

private:
   DECLARE_EAF_AGENT_DATA;

   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwLossParametersCookie;

   StatusGroupIDType m_LoadStatusGroupID; // ID used to identify user load-related status items created by this agent

   IDType m_AlignmentID;

   CComPtr<ICogoEngine> m_CogoEngine; // this is not the cogo model!!! just an engine to do computations with
   CComPtr<ICogoModel> m_CogoModel;
   CComPtr<IGenericBridge> m_Bridge;

   CComPtr<IBridgeGeometryTool> m_BridgeGeometryTool;

   std::map<CollectionIndexType,CogoObjectID> m_HorzCurveKeys;
   std::map<CollectionIndexType,CogoObjectID> m_VertCurveKeys;

   CConcreteManager m_ConcreteManager;
   CIntervalManager m_IntervalManager;

   // equations used to compute elevation adjustments in a segment due
   // to temporary support elevation adjustments and pier "A" dimensions
   std::map<CSegmentKey,mathLinFunc2d> m_ElevationAdjustmentEquations;

   // containers to cache shapes cut at various stations
   typedef std::map<Float64,CComPtr<IShape> > ShapeContainer;
   ShapeContainer m_DeckShapes;
   ShapeContainer m_LeftBarrierShapes;
   ShapeContainer m_RightBarrierShapes;

   // for ISectionProperties
   // Section Properties
   CComPtr<ISectionCutTool> m_SectCutTool;
   CComPtr<IEffectiveFlangeWidthTool> m_EffFlangeWidthTool;
   typedef struct SectProp
   {
      CComPtr<ISection> Section;
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
   std::auto_ptr<SectPropContainer> m_pSectProps[pgsTypes::sptSectionPropertyTypeCount]; // index = one of the pgsTypes::SectionPropertyType constants
   void InvalidateSectionProperties(pgsTypes::SectionPropertyType sectPropType);
   static UINT DeleteSectionProperties(LPVOID pParam);
   pgsTypes::SectionPropertyType GetSectionPropertiesType(); // returns the section properties types for the current section properties mode
   SectProp GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType);
   Float64 ComputeY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,IShapeProperties* sprops);
   Float64 ComputeYtopGirder(IShapeProperties* compositeProps,IShapeProperties* beamProps);

   // Points of interest for precast segments (precast girders/spliced girder segments)
   pgsPoiMgr m_PoiMgr;
   std::set<CGirderKey> m_ValidatedPoi;

   // keeps track of which girders have had critical sections locations determined
   std::set<CGirderKey> m_CriticalSectionState[2]; // index 0 = Strength I, index 1 = Strength II

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

   void Invalidate( Uint16 level );
   Uint16 Validate( Uint16 level );
   bool BuildCogoModel();
   bool BuildBridgeModel();
   bool BuildGirder();
   void ValidateGirder();
   void ValidateElevationAdjustments(const CSegmentKey& segmentKey);

   // helper functions for building the bridge model
   bool LayoutGirders(const CBridgeDescription2* pBridgeDesc);
   void GetHaunchDepth(const CPrecastSegmentData* pSegment,Float64* pStartHaunch,Float64* pEndHaunch);
   bool LayoutDeck(const CBridgeDescription2* pBridgeDesc);
   bool LayoutNoDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutSimpleDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutFullDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutCompositeCIPDeck(const CDeckDescription2* pDeck,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck);
   bool LayoutCompositeSIPDeck(const CDeckDescription2* pDeck,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck);

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

   Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx);

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

   HRESULT GetSlabOverhangs(Float64 distance,Float64* pLeft,Float64* pRight);
   Float64 GetDistanceFromStartOfBridge(const CSegmentKey& segmentKey,Float64 distFromStartOfSegment);
   HRESULT GetGirderSection(const pgsPointOfInterest& poi,pgsTypes::SectionCoordinateType csType,IGirderSection** gdrSection);
   HRESULT GetSuperstructureMember(const CGirderKey& girderKey,ISuperstructureMember* *ssmbr);
   HRESULT GetSegment(const CSegmentKey& segmentKey,ISegment** segment);
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
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi, DevelopmentAdjustmentType devAdjust,bool bTensionTop);
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi, const GDRCONFIG& config,DevelopmentAdjustmentType devAdjust,bool bTensionTop);
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi, bool bUseConfig,const GDRCONFIG& config,DevelopmentAdjustmentType devAdjust,bool bTensionTop);

   Float64 GetAptTensionSide(const pgsPointOfInterest& poi,bool bTensionTop);

   Float64 GetAsDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat);
   Float64 GetLocationDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat);
   void GetDeckMatData(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat,bool bAdjForDevLength,Float64* pAs,Float64* pYb);

   void GetShapeProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps);
   void GetShapeProperties(pgsTypes::SectionPropertyType sectPropType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps);
   Float64 GetCutLocation(const pgsPointOfInterest& poi);

   void NoDeckEdgePoint(GroupIndexType grpIdx,SegmentIndexType segIdx,pgsTypes::MemberEndType end,DirectionType side,IPoint2d** ppPoint);

   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,IPoint2d** point);
   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,IPoint3d** point);
   void CreateCompositeOverlayEdgePaths(const CBridgeDescription2* pBridgeDesc,IPath** ppLeftPath,IPath** ppRightPath);

   REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(const CComBSTR& name, Float64 Ab, Float64 db, Float64 fy, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct);

   boost::shared_ptr<mathFunction2d> CreateGirderProfile(const CSplicedGirderData* pGirder);
   boost::shared_ptr<mathFunction2d> CreateGirderBottomFlangeProfile(const CSplicedGirderData* pGirder);
   boost::shared_ptr<mathFunction2d> CreateGirderProfile(const CSplicedGirderData* pGirder,bool bGirderProfile);
   void GetSegmentRange(const CSegmentKey& segmentKey,Float64* pXStart,Float64* pXEnd);

   Float64 ConvertDuctOffsetToDuctElevation(const CGirderKey& girderKey,Float64 Xg,Float64 offset,CDuctGeometry::OffsetType offsetType);
   mathPolynomial2d GenerateParabola1(Float64 x1,Float64 y1,Float64 x2,Float64 y2,Float64 slope);
   mathPolynomial2d GenerateParabola2(Float64 x1,Float64 y1,Float64 x2,Float64 y2,Float64 slope);
   void GenerateReverseParabolas(Float64 x1,Float64 y1,Float64 x2,Float64 x3,Float64 y3,mathPolynomial2d* pLeftParabola,mathPolynomial2d* pRightParabola);
   void CreateDuctCenterline(const CGirderKey& girderKey,const CLinearDuctGeometry& geometry,IPoint2dCollection** ppPoints);
   void CreateDuctCenterline(const CGirderKey& girderKey,const CParabolicDuctGeometry& geometry,IPoint2dCollection** ppPoints);
   void CreateDuctCenterline(const CGirderKey& girderKey,const COffsetDuctGeometry& geometry,IPoint2dCollection** ppPoints);

   mathCompositeFunction2d CreateDuctCenterline(const CGirderKey& girderKey,const CLinearDuctGeometry& geometry);
   mathCompositeFunction2d CreateDuctCenterline(const CGirderKey& girderKey,const CParabolicDuctGeometry& geometry);

   SegmentIndexType GetSegmentIndex(const CSplicedGirderData* pGirder,Float64 distFromStartOfBridge);
   SegmentIndexType GetSegmentIndex(const CGirderKey& girderKey,ILine2d* pLine,IPoint2d** ppIntersection);

   SpanIndexType GetSpanIndex(Float64 distFromStartOfBridge);
   PierIndexType GetGenericBridgePierIndex(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType);
   void GetGirderLine(const CSegmentKey& segmentKey,IGirderLine** ppGirderLine);
   void GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey,ISegment** ppSegment);
   void GetPierLine(PierIndexType pierIdx,IPierLine** ppPierLine);
   void GetTemporarySupportLine(SupportIndexType tsIdx,IPierLine** ppPierLine);

   const GirderLibraryEntry* GetGirderLibraryEntry(const CGirderKey& girderKey);
   GroupIndexType GetGirderGroupAtPier(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace);

   void CreateTendons(const CBridgeDescription2* pBridgeDesc,const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,ITendonCollection** ppTendons);
   void CreateParabolicTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const CParabolicDuctGeometry& ductGeometry,ITendonCollection** ppTendons);
   void CreateLinearTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const CLinearDuctGeometry& ductGeometry,ITendonCollection** ppTendons);
   void CreateOffsetTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const COffsetDuctGeometry& ductGeometry,ITendonCollection* tendons,ITendonCollection** ppTendons);

   void CreateStrandMover(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,IStrandMover** ppStrandMover);

   Float64 GetRelaxation(Float64 fpi,const matPsStrand* pStrand,Float64 tStart,Float64 tEnd,Float64 tStress);

   // Returns the geometric CG location of the strands measured in Girder Coordinates
   Float64 GetHsLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   Float64 GetSsLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   Float64 GetTempLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);

   Float64 GetHsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands);
   Float64 GetSsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands);
   Float64 GetTempEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands);
   Float64 GetHsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands);
   Float64 GetSsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands);
   Float64 GetTempEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands);

   Float64 GetSuperstructureDepth(PierIndexType pierIdx);
};

#endif //__BRIDGEAGENT_H_
