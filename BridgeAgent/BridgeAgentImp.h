///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\PoiMgr.h>
#include <PgsExt\PoiKey.h>
#include <PgsExt\DeckRebarData.h>
#include <PgsExt\RailingSystem.h>

#include "EffectiveFlangeWidthTool.h"
#include "ContinuousStandFiller.h"

#include <boost\shared_ptr.hpp>

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
   public IBridgeMaterialEx,
   public IStrandGeometry,
   public ILongRebarGeometry,
   public IStirrupGeometry,
   public IPointOfInterest,
   public ISectProp2,
   public IBarriers,
   public IStageMap,
   public IGirder,
   public IGirderLiftingPointsOfInterest,
   public IGirderHaulingPointsOfInterest,
   public IBridgeDescriptionEventSink,
   public ISpecificationEventSink,
   public IUserDefinedLoads
{
public:
   CBridgeAgentImp()
	{
      m_Level        = 0;
      m_pBroker      = 0;

      m_pRebar       = 0;

      m_AlignmentID = -1;

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
   COM_INTERFACE_ENTRY(IBridgeMaterial)
   COM_INTERFACE_ENTRY(IBridgeMaterialEx)
   COM_INTERFACE_ENTRY(IStrandGeometry)
   COM_INTERFACE_ENTRY(ILongRebarGeometry)
   COM_INTERFACE_ENTRY(IStirrupGeometry)
   COM_INTERFACE_ENTRY(IPointOfInterest)
   COM_INTERFACE_ENTRY(IStageMap)
   COM_INTERFACE_ENTRY(ISectProp2)
   COM_INTERFACE_ENTRY(IBarriers)
   COM_INTERFACE_ENTRY(IGirder)
   COM_INTERFACE_ENTRY(IGirderLiftingPointsOfInterest)
   COM_INTERFACE_ENTRY(IGirderHaulingPointsOfInterest)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
   COM_INTERFACE_ENTRY(IUserDefinedLoads)
   COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CBridgeAgentImp)
END_CONNECTION_POINT_MAP()

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidInformationalError;
   StatusCallbackIDType m_scidInformationalWarning;
   StatusCallbackIDType m_scidBridgeDescriptionError;
   StatusCallbackIDType m_scidAlignmentWarning;
   StatusCallbackIDType m_scidAlignmentError;
   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidConcreteStrengthWarning;
   StatusCallbackIDType m_scidConcreteStrengthError;
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
   virtual Float64 GetCrownPointOffset(Float64 station);
   virtual Float64 GetCrownSlope(Float64 station,Float64 offset);
   virtual void GetCrownSlope(Float64 station,Float64* pLeftSlope,Float64* pRightSlope);
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
   virtual void GetCrownPoint(Float64 station,IDirection* pDirection,IPoint2d** ppPoint);
   virtual void GetCrownPoint(Float64 station,IDirection* pDirection,IPoint3d** ppPoint);

// IBridge
public:
   virtual Float64 GetLength();
   virtual Float64 GetSpanLength(SpanIndexType span);
   virtual double GetAlignmentOffset();
   virtual double GetDistanceFromStartOfBridge(double station);
   virtual SpanIndexType GetSpanCount();
   virtual PierIndexType GetPierCount();
   virtual GirderIndexType GetGirderCount(SpanIndexType span);
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight);
   virtual std::vector<double> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType);
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(SpanIndexType spanIdx,double distFromStartOfSpan);
   virtual void GetSpacingAlongGirder(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfGirder,Float64* leftSpacing,Float64* rightSpacing);
   virtual Float64 GetGirderLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetSpanLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderPlanLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderSlope(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetCCPierLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderStartConnectionLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderEndConnectionLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderStartBearingOffset(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderEndBearingOffset(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderStartSupportWidth(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderEndSupportWidth(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetCLPierToCLBearingDistance(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType measure);
   virtual Float64 GetCLPierToGirderEndDistance(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType measure);
   virtual Float64 GetGirderOffset(SpanIndexType span,GirderIndexType gdr,Float64 station);
   virtual void GetGirderAngle(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType face,IAngle** ppAngle);
   virtual void GetGirderBearing(SpanIndexType span,GirderIndexType gdr,IDirection** ppBearing);
   virtual GDRCONFIG GetGirderConfiguration(SpanIndexType span,GirderIndexType gdr);
   virtual bool GetSpan(double station,SpanIndexType* pSpanIdx);
   virtual void GetPoint(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan,IPoint2d** ppPoint);
   virtual void GetPoint(const pgsPointOfInterest& poi,IPoint2d** ppPoint);
   virtual void GetStationAndOffset(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfBridge,Float64* pStation,Float64* pOffset);
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset);
   virtual void GetDistFromStartOfSpan(GirderIndexType gdrIdx,double distFromStartOfBridge,SpanIndexType* pSpanIdx,double* pDistFromStartOfSpan);
   virtual bool IsInteriorGirder(SpanIndexType span,GirderIndexType gdr);
   virtual bool IsExteriorGirder(SpanIndexType span,GirderIndexType gdr);
   virtual bool AreGirderTopFlangesRoughened();
   virtual void GetLeftSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH);
   virtual void GetRightSideEndDiaphragmSize(PierIndexType pier,Float64* pW,Float64* pH);
   virtual bool DoesLeftSideEndDiaphragmLoadGirder(PierIndexType pier);
   virtual bool DoesRightSideEndDiaphragmLoadGirder(PierIndexType pier);
   virtual Float64 GetEndDiaphragmLoadLocationAtStart(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetEndDiaphragmLoadLocationAtEnd(SpanIndexType span,GirderIndexType gdr);
   virtual std::vector<IntermedateDiaphragm> GetIntermediateDiaphragms(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual pgsTypes::SupportedDeckType GetDeckType();
   virtual Float64 GetSlabOffset(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end);
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi);
   virtual Float64 GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual bool IsCompositeDeck();
   virtual bool HasOverlay();
   virtual bool IsFutureOverlay();
   virtual Float64 GetOverlayWeight();
   virtual Float64 GetOverlayDepth();
   virtual Float64 GetFillet();
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi);
   virtual Float64 GetLeftSlabEdgeOffset(double distFromStartOfBridge);
   virtual Float64 GetRightSlabEdgeOffset(double distFromStartOfBridge);
   virtual Float64 GetLeftSlabOverhang(double distFromStartOfBridge);
   virtual Float64 GetRightSlabOverhang(double distFromStartOfBridge);
   virtual Float64 GetLeftSlabOverhang(SpanIndexType span,double distFromStartOfSpan);
   virtual Float64 GetRightSlabOverhang(SpanIndexType span,double distFromStartOfSpan);
   virtual Float64 GetLeftSlabOverhang(PierIndexType pier);
   virtual Float64 GetRightSlabOverhang(PierIndexType pier);
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pier);
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pier);
   virtual Float64 GetLeftSlabGirderOverhang(SpanIndexType span,double distFromStartOfSpan); // overhangs measured normal to alignment
   virtual Float64 GetRightSlabGirderOverhang(SpanIndexType span,double distFromStartOfSpan);
   virtual Float64 GetLeftCurbOffset(double distFromStartOfBridge);
   virtual Float64 GetRightCurbOffset(double distFromStartOfBridge);
   virtual Float64 GetLeftCurbOffset(SpanIndexType span,double distFromStartOfSpan);
   virtual Float64 GetRightCurbOffset(SpanIndexType span,double distFromStartOfSpan);
   virtual Float64 GetLeftCurbOffset(PierIndexType pier);
   virtual Float64 GetRightCurbOffset(PierIndexType pier);
   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetCurbToCurbWidth(SpanIndexType span,GirderIndexType gdr,double distFromStartOfSpan);
   virtual Float64 GetCurbToCurbWidth(double distFromStartOfBridge);
   virtual void GetSlabPerimeter(Uint32 nPoints,IPoint2dCollection** points);
   virtual void GetSpanPerimeter(SpanIndexType spanIdx,Uint32 nPoints,IPoint2dCollection** points);
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point);
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point);
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point);
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point);
   virtual Float64 GetPierStation(PierIndexType pier);
   virtual Float64 GetAheadBearingStation(PierIndexType pier,GirderIndexType gdr);
   virtual Float64 GetBackBearingStation(PierIndexType pier,GirderIndexType gdr);
   virtual void GetPierDirection(PierIndexType pier,IDirection** ppDirection);
   virtual void GetPierSkew(PierIndexType pier,IAngle** ppAngle);
   virtual std::string GetLeftSidePierConnection(PierIndexType pier);
   virtual std::string GetRightSidePierConnection(PierIndexType pier);
   virtual void GetPierPoints(PierIndexType pier,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right);
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight);
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight);
   virtual void GetContinuityStage(PierIndexType pierIdx,pgsTypes::Stage* pLeft,pgsTypes::Stage* pRight);
   virtual bool GetSkewAngle(Float64 station,const char* strOrientation,Float64* pSkew);
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx);

// IBridgeMaterial
public:
   virtual Float64 GetEcGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetEciGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetFcGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetFciGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetMaxAggrSizeGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetStrDensityGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetWgtDensityGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetK1Gdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetEcSlab();
   virtual Float64 GetFcSlab();
   virtual Float64 GetStrDensitySlab();
   virtual Float64 GetWgtDensitySlab();
   virtual Float64 GetMaxAggrSizeSlab();
   virtual Float64 GetDensityRailing(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetEcRailing(pgsTypes::TrafficBarrierOrientation orientation);
   virtual const matPsStrand* GetStrand(SpanIndexType span,GirderIndexType gdr);
   virtual void GetLongitudinalRebarProperties(SpanIndexType span,GirderIndexType gdr,Float64* pE,Float64 *pFy);
   virtual std::string GetLongitudinalRebarName(SpanIndexType span,GirderIndexType gdr);
   virtual void GetTransverseRebarProperties(SpanIndexType span,GirderIndexType gdr,Float64* pE,Float64 *pFy);
   virtual std::string GetTransverseRebarName(SpanIndexType span,GirderIndexType gdr);
   virtual void GetDeckRebarProperties(Float64* pE,Float64 *pFy);
   virtual std::string GetDeckRebarName();
   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1);
   virtual Float64 GetFlexureModRupture(Float64 fc);
   virtual Float64 GetShearModRupture(Float64 fc);
   virtual Float64 GetFlexureFrCoefficient();
   virtual Float64 GetShearFrCoefficient();
   virtual Float64 GetFlexureFrGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetShearFrGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetFriGdr(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetFlexureFrSlab();
   virtual Float64 GetShearFrSlab();
   virtual Float64 GetK1Slab();
   virtual Float64 GetNWCDensityLimit();

// IBridgeMaterialEx
public:
   virtual Float64 GetLWCDensityLimit();
   virtual pgsTypes::ConcreteType GetGdrConcreteType(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual bool DoesGdrConcreteHaveAggSplittingStrength(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetGdrConcreteAggSplittingStrength(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual pgsTypes::ConcreteType GetSlabConcreteType();
   virtual bool DoesSlabConcreteHaveAggSplittingStrength();
   virtual Float64 GetSlabConcreteAggSplittingStrength();
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type);
   virtual Float64 GetFlexureFrCoefficient(pgsTypes::ConcreteType type);
   virtual Float64 GetFlexureFrCoefficient(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type);
   virtual Float64 GetShearFrCoefficient(pgsTypes::ConcreteType type);
   virtual Float64 GetShearFrCoefficient(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetEccK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetEccK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetCreepK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetCreepK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetShrinkageK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetShrinkageK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetEccK1Slab();
   virtual Float64 GetEccK2Slab();
   virtual Float64 GetCreepK1Slab();
   virtual Float64 GetCreepK2Slab();
   virtual Float64 GetShrinkageK1Slab();
   virtual Float64 GetShrinkageK2Slab();
   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2);

// IStageMap
public:
   virtual CComBSTR GetStageName(pgsTypes::Stage stage);  
   virtual pgsTypes::Stage GetStageType(CComBSTR bstrStage);
   virtual CComBSTR GetLimitStateName(pgsTypes::LimitState ls);  

// ILongRebarGeometry
public:
   virtual void GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection);
   virtual Float64 GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetDevLengthFactor(SpanIndexType span,GirderIndexType gdr,IRebarSectionItem* rebarItem);
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetCoverTopMat();
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,ILongRebarGeometry::DeckRebarType drt);
   virtual Float64 GetCoverBottomMat();
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,ILongRebarGeometry::DeckRebarType drt);
   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config);

// IStirrupGeometry
public:
   virtual BarSizeType GetVertStirrupBarSize(const pgsPointOfInterest& poi);
   virtual BarSizeType GetHorzStirrupBarSize(const pgsPointOfInterest& poi);
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi);
   virtual Float64 GetHorzStirrupBarNominalDiameter(const pgsPointOfInterest& poi);
   virtual Float64 GetVertStirrupBarArea(const pgsPointOfInterest& poi);
   virtual Float64 GetHorzStirrupBarArea(const pgsPointOfInterest& poi);
   virtual Uint32 GetVertStirrupBarCount(const pgsPointOfInterest& poi);
   virtual Uint32 GetHorzStirrupBarCount(const pgsPointOfInterest& poi);
   virtual Float64 GetS(const pgsPointOfInterest& poi);
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi);

   virtual BarSizeType GetConfinementBarSize(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetLengthOfConfinementZone(SpanIndexType span,GirderIndexType gdr);

   virtual bool DoStirrupsEngageDeck(SpanIndexType span,GirderIndexType gdr);

   virtual BarSizeType GetTopFlangeBarSize(const pgsPointOfInterest& poi);
   virtual Float64 GetTopFlangeBarArea(const pgsPointOfInterest& poi);
   virtual Float64 GetTopFlangeS(const pgsPointOfInterest& poi);

   virtual ZoneIndexType GetNumZones(SpanIndexType span,GirderIndexType gdr);
   virtual Uint32 GetZoneId(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   virtual Float64 GetZoneStart(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone); // dist from start of girder
   virtual Float64 GetZoneEnd(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone) ; // dist from start of girder

   virtual BarSizeType GetVertStirrupBarSize(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   virtual BarSizeType GetHorzStirrupBarSize(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   virtual Uint32 GetVertStirrupBarCount(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   virtual Uint32 GetHorzStirrupBarCount(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   virtual Float64 GetS(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   virtual ZoneIndexType GetNumConfinementZones(SpanIndexType span,GirderIndexType gdr);
   virtual bool IsConfinementZone(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);

   virtual Float64 GetVertAv(SpanIndexType span,GirderIndexType gdr,Float64 start,Float64 end);
   virtual Float64 GetHorzAv(SpanIndexType span,GirderIndexType gdr,Float64 start,Float64 end);
   virtual void GetAv(SpanIndexType span,GirderIndexType gdr,Float64 start,Float64 end,Float64* pAvVert,Float64* pAvHorz);

private:
   const matRebar* GetVertStirrupRebar(const pgsPointOfInterest& poi);
   const matRebar* GetHorzStirrupRebar(const pgsPointOfInterest& poi);
   const matRebar* GetConfinementRebar(const pgsPointOfInterest& poi);
   const matRebar* GetTopFlangeRebar(const pgsPointOfInterest& poi);
   const matRebar* GetVertStirrupRebar(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   const matRebar* GetHorzStirrupRebar(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   const matRebar* GetConfinementRebar(SpanIndexType span,GirderIndexType gdr);

   bool GetShearZoneAtPoi(ZoneIndexType* pzone, const pgsPointOfInterest& poi);
   ZoneIndexType GetZoneIndex(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone);
   void GetZoneBounds(Float64* start, Float64* end, SpanIndexType span,GirderIndexType gdr, ZoneIndexType zone);

// IStrandGeometry
public:
   virtual Float64 GetEccentricity(const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands);
   virtual Float64 GetEccentricity(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands);
   virtual Float64 GetSsEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands);
   virtual Float64 GetHsEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands);
   virtual Float64 GetTempEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands);
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi);
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi);

   virtual Float64 GetEccentricity(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands);
   virtual Float64 GetHsEccentricity(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands);
   virtual Float64 GetSsEccentricity(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands);
   virtual Float64 GetTempEccentricity(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands);
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift);
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift);

   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig,bool bDevAdjust);
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust);
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG& rconfig,bool bDevAdjust);

   virtual StrandIndexType GetNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type);
   virtual StrandIndexType GetMaxStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type);
   virtual StrandIndexType GetMaxStrands(const char* strGirderName,pgsTypes::StrandType type);
   virtual Float64 GetStrandArea(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type);
   virtual Float64 GetAreaPrestressStrands(SpanIndexType span,GirderIndexType gdr,bool bIncTemp);

   virtual Float64 GetPjack(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type);
   virtual Float64 GetPjack(SpanIndexType span,GirderIndexType gdr,bool bIncTemp);
   virtual void GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint);
   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints);
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi,StrandIndexType Ns, pgsTypes::StrandType type, IPoint2dCollection** ppPoints);

     // highest point on girder section based on strand coordinates (bottom at 0.0)
   virtual Float64 GetGirderTopElevation(SpanIndexType span,GirderIndexType gdr);
   // harped offsets are measured from original strand locations in strand grid
   virtual void GetHarpStrandOffsets(SpanIndexType span,GirderIndexType gdr,Float64* pOffsetEnd,Float64* pOffsetHp);

   virtual void GetHarpedEndOffsetBounds(SpanIndexType span,GirderIndexType gdr,Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedEndOffsetBoundsEx(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedHpOffsetBounds(SpanIndexType span,GirderIndexType gdr,Float64* DownwardOffset, Float64* UpwardOffset);
   virtual void GetHarpedHpOffsetBoundsEx(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset);

   virtual Float64 GetHarpedEndOffsetIncrement(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetHarpedHpOffsetIncrement(SpanIndexType span,GirderIndexType gdr);

   virtual void GetHarpingPointLocations(SpanIndexType span,GirderIndexType gdr,Float64* lhp,Float64* rhp);
   virtual void GetHighestHarpedStrandLocation(SpanIndexType span,GirderIndexType gdr,Float64* pElevation);
   virtual Uint16 GetNumHarpPoints(SpanIndexType span,GirderIndexType gdr);

   virtual StrandIndexType GetMaxNumPermanentStrands(SpanIndexType span,GirderIndexType gdr);
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,SpanIndexType span,GirderIndexType gdr, StrandIndexType* numStraight, StrandIndexType* numHarped);
   virtual StrandIndexType GetNextNumPermanentStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);
   virtual StrandIndexType GetPreviousNumPermanentStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);


   virtual bool IsValidNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual StrandIndexType GetNextNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum);
   virtual StrandIndexType GetPrevNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum);

   virtual bool IsStrandDebonded(SpanIndexType span,GirderIndexType gdr,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64* pStart,Float64* pEnd);
   virtual bool IsStrandDebonded(SpanIndexType span,GirderIndexType gdr,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64* pStart,Float64* pEnd);
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual StrandIndexType GetNumDebondedStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);
   virtual RowIndexType GetNumRowsWithStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType );
   virtual StrandIndexType GetNumStrandInRow(SpanIndexType span,GirderIndexType gdr,RowIndexType rowIdx,pgsTypes::StrandType strandType );
   virtual std::vector<StrandIndexType> GetStrandsInRow(SpanIndexType span,GirderIndexType gdr, RowIndexType rowIdx, pgsTypes::StrandType strandType );
   virtual StrandIndexType GetNumDebondedStrandsInRow(SpanIndexType span,GirderIndexType gdr,RowIndexType rowIdx,pgsTypes::StrandType strandType );
   virtual bool IsExteriorStrandDebondedInRow(SpanIndexType span,GirderIndexType gdr,RowIndexType rowIdx,pgsTypes::StrandType strandType);
   virtual bool IsDebondingSymmetric(SpanIndexType span,GirderIndexType gdr);

   virtual RowIndexType GetNumRowsWithStrand(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands,pgsTypes::StrandType strandType );
   virtual StrandIndexType GetNumStrandInRow(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType );
   virtual std::vector<StrandIndexType> GetStrandsInRow(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType );

   virtual Float64 GetDebondSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);
   virtual SectionIndexType GetNumDebondSections(SpanIndexType span,GirderIndexType gdr,GirderEnd end,pgsTypes::StrandType strandType);
   virtual StrandIndexType GetNumDebondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);
   virtual StrandIndexType GetNumBondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType);

   virtual bool CanDebondStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType); // can debond any of the strands
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType, ILongArray** list); 
   virtual Float64 GetDefaultDebondLength(SpanIndexType spanIdx,GirderIndexType gdrIdx);

   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType measurementType, Float64 offset);
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType measurementType, Float64 absoluteOffset);
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType measurementType, Float64 offset);
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType measurementType, Float64 absoluteOffset);
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange);
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange);
   virtual Float64 ConvertHarpedOffsetEnd(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType);
   virtual Float64 ConvertHarpedOffsetHp(SpanIndexType span,GirderIndexType gdr,StrandIndexType Nh, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType);


// IPointOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr);
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,std::vector<pgsTypes::Stage> stages,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);
   virtual std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);
   virtual std::vector<pgsPointOfInterest> GetTenthPointPOIs(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr);
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,pgsPointOfInterest* pLeft,pgsPointOfInterest* pRight);
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,pgsPointOfInterest* pLeft,pgsPointOfInterest* pRight);
   virtual double GetDistanceFromFirstPier(const pgsPointOfInterest& poi,pgsTypes::Stage stage);
   virtual pgsPointOfInterest GetPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);
   virtual pgsPointOfInterest GetPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);
   virtual pgsPointOfInterest GetNearestPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);
   virtual pgsPointOfInterest GetNearestPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);

// ISectProp2
public:
   virtual Float64 GetHg(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetAg(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetIy(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetYt(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetYb(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetSt(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetSb(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetYtGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetStGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetKt(const pgsPointOfInterest& poi);
   virtual Float64 GetKb(const pgsPointOfInterest& poi);
   virtual Float64 GetEIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual Float64 GetAg(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetIy(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetYt(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetYb(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetSt(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetSb(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetYtGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetStGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr);
   virtual Float64 GetQSlab(const pgsPointOfInterest& poi);
   virtual Float64 GetAcBottomHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetAcTopHalf(const pgsPointOfInterest& poi);
   virtual Float64 GetEffectiveFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetTributaryFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetEffectiveDeckArea(const pgsPointOfInterest& poi);
   virtual Float64 GetTributaryDeckArea(const pgsPointOfInterest& poi);
   virtual Float64 GetGrossDeckArea(const pgsPointOfInterest& poi);
   virtual Float64 GetDistTopSlabToTopGirder(const pgsPointOfInterest& poi);
   virtual void ReportEffectiveFlangeWidth(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi);
   virtual Float64 GetSurfaceArea(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetVolume(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetBridgeEIxx(double distFromStart);
   virtual Float64 GetBridgeEIyy(double distFromStart);
   virtual void GetGirderShape(const pgsPointOfInterest& poi,bool bOrient,IShape** ppShape);
   virtual void GetSlabShape(double station,IShape** ppShape);
   virtual void GetLeftTrafficBarrierShape(double station,IShape** ppShape);
   virtual void GetRightTrafficBarrierShape(double station,IShape** ppShape);
   virtual Float64 GetGirderWeightPerLength(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetGirderWeight(SpanIndexType span,GirderIndexType gdr);

// IBarriers
public:
   virtual Float64 GetAtb(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetItb(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetYbtb(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation);
   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation);
   virtual Float64 GetSidewalkWidth(pgsTypes::TrafficBarrierOrientation orientation);
   virtual bool HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation);

// IGirder
public:
   virtual bool    IsPrismatic(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr);
   virtual bool    IsSymmetric(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr); 
   virtual MatingSurfaceIndexType  GetNumberOfMatingSurfaces(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx);
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType idx);
   virtual FlangeIndexType GetNumberOfTopFlanges(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx);
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi);
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi);
   virtual FlangeIndexType GetNumberOfBottomFlanges(SpanIndexType spanIdx,GirderIndexType gdrIdx);
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
   virtual WebIndexType  GetNumberOfWebs(SpanIndexType span,GirderIndexType gdr);
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx);
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx);
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx);
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi);
   virtual void GetGirderEndPoints(SpanIndexType span,GirderIndexType gdr,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2);
   virtual Float64 GetOrientation(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetTopGirderReferenceChordElevation(const pgsPointOfInterest& poi);
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIdx);
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx);
   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi);
   virtual pgsTypes::SplittingDirection GetSplittingDirection(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual void GetProfileShape(SpanIndexType spanIdx,GirderIndexType gdrIdx,IShape** ppShape);
   virtual bool HasShearKey(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::SupportedBeamSpacing spacingType);
   virtual void GetShearKeyAreas(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint);

// IGirderLiftingPointsOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetLiftingPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);
   virtual std::vector<pgsPointOfInterest> GetLiftingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 overhang,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);

// IGirderHaulingPointsOfInterest
public:
   virtual std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);
   virtual std::vector<pgsPointOfInterest> GetHaulingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);
   virtual Float64 GetMinimumOverhang(SpanIndexType span,GirderIndexType gdr);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged();
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(const char* strOldName,const char* strNewName);
   virtual HRESULT OnConstructionLoadChanged();

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

// IUserDefinedLoads
public:
   virtual bool DoUserLoadsExist(SpanIndexType span,GirderIndexType gdr);
   virtual const std::vector<UserPointLoad>* GetPointLoads(pgsTypes::Stage stage, SpanIndexType span,GirderIndexType gdr);
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(pgsTypes::Stage stage, SpanIndexType span,GirderIndexType gdr);
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(pgsTypes::Stage stage, SpanIndexType span,GirderIndexType gdr);

private:
   DECLARE_AGENT_DATA;

   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecificationCookie;

   StatusGroupIDType m_LoadStatusGroupID; // ID used to identify user load-related status items created by this agent

   Int32 m_AlignmentID;

   CComPtr<IGenericBridge> m_Bridge;
   CComPtr<IBridgeGeometryTool> m_BridgeGeometryTool;
   CComPtr<ICogoEngine> m_CogoEngine; // this is not the cogo model!!! just an engine to do computations with

   std::map<CollectionIndexType,CogoElementKey> m_HorzCurveKeys;
   std::map<CollectionIndexType,CogoElementKey> m_VertCurveKeys;

   std::map< SpanGirderHashType, boost::shared_ptr<matConcreteEx> > m_pGdrConc;
   std::map< SpanGirderHashType, boost::shared_ptr<matConcreteEx> > m_pGdrReleaseConc;

   Float64 m_SlabEcK1;
   Float64 m_SlabEcK2;
   Float64 m_SlabCreepK1;
   Float64 m_SlabCreepK2;
   Float64 m_SlabShrinkageK1;
   Float64 m_SlabShrinkageK2;
   std::auto_ptr<matConcreteEx> m_pSlabConc;

   std::auto_ptr<matConcreteEx> m_pRailingConc[2]; // index is pgsTypes::TrafficBarrierOrientation

   const matRebar*    m_pRebar;

   // containers to cache shapes cut at various stations
   typedef std::map<double,CComPtr<IShape> > ShapeContainer;
   ShapeContainer m_DeckShapes;
   ShapeContainer m_LeftBarrierShapes;
   ShapeContainer m_RightBarrierShapes;

   // for ISectProp2
   // Section Properties
   CComPtr<ISectionCutTool> m_SectCutTool;
   CComPtr<IEffectiveFlangeWidthTool> m_EffFlangeWidthTool;
   typedef struct SectProp
   {
      CComPtr<ISection> Section;
      CComPtr<IElasticProperties> ElasticProps;
      CComPtr<IShapeProperties> ShapeProps;
      CComPtr<IMassProperties> MassProps;

      Float64 YtopGirder;
      Float64 Perimeter;

      bool bComposite; // If false, Qslab is undefined
      Float64 Qslab;
      Float64 AcBottomHalf; // for LRFD Fig 5.8.3.4.2-3
      Float64 AcTopHalf;    // for LRFD Fig 5.8.3.4.2-3

      SectProp() { YtopGirder = 0; Perimeter = 0; bComposite = false; Qslab = 0; AcBottomHalf = 0; AcTopHalf = 0; }
   } SectProp;
   typedef std::map<PoiStageKey,SectProp> SectPropContainer;
   SectPropContainer m_SectProps; // Key = PoiStageKey object
   SectProp GetSectionProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi);

   pgsPoiMgr m_PoiMgr;
   std::set<SpanGirderHashType> m_PoiValidated; // If the span/gdr key is in the set, then the POI's have been validated

   std::set<SpanGirderHashType> m_CriticalSectionState[2];

   // adapter for working with continuous strand fills
   typedef std::map<SpanGirderHashType, CContinuousStandFiller>  StrandFillerCollection;
   StrandFillerCollection  m_StrandFillers; // a filler for every girder

   // user defined loads for each bridge site stage, hashed by span/girder
   typedef std::map<SpanGirderHashType, std::vector<UserPointLoad> >       PointLoadCollection;
   typedef std::map<SpanGirderHashType, std::vector<UserDistributedLoad> > DistributedLoadCollection;
   typedef std::map<SpanGirderHashType, std::vector<UserMomentLoad> >      MomentLoadCollection;
   PointLoadCollection         m_PointLoads[3];
   DistributedLoadCollection   m_DistributedLoads[3];
   MomentLoadCollection        m_MomentLoads[3];
   bool                        m_bUserLoadsValidated;

   std::map<Float64,Float64> m_LeftSlabEdgeOffset;
   std::map<Float64,Float64> m_RightSlabEdgeOffset;

   void Invalidate( Uint16 level );
   Uint16 Validate( Uint16 level );
   bool BuildCogoModel();
   bool BuildBridgeModel();
   bool BuildGirder();
   void ValidateGirder();

   bool LayoutPiersAndSpans(const CBridgeDescription* pBridgeDesc);
   bool LayoutGirders(const CBridgeDescription* pBridgeDesc);
   bool LayoutSuperstructureMembers(const CBridgeDescription* pBridgeDesc);
   bool LayoutDeck(const CBridgeDescription* pBridgeDesc);
   bool LayoutNoDeck(const CBridgeDescription* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutSimpleDeck(const CBridgeDescription* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutFullDeck(const CBridgeDescription* pBridgeDesc,IBridgeDeck** ppDeck);
   bool LayoutCompositeCIPDeck(const CDeckDescription* pDeck,IOverhangPathStrategy* pLeftOverhangStrategy,IOverhangPathStrategy* pRightOverhangStrategy,IBridgeDeck** ppDeck);
   bool LayoutCompositeSIPDeck(const CDeckDescription* pDeck,IOverhangPathStrategy* pLeftOverhangStrategy,IOverhangPathStrategy* pRightOverhangStrategy,IBridgeDeck** ppDeck);

   bool LayoutTrafficBarriers(const CBridgeDescription* pBridgeDesc);
   bool LayoutTrafficBarrier(const CBridgeDescription* pBridgeDesc,const CRailingSystem* pRailingSystem,pgsTypes::TrafficBarrierOrientation orientation,ISidewalkBarrier** ppBarrier);
   void LayoutGirderRebar(SpanIndexType span,GirderIndexType gdr);
   void UpdatePrestressing(SpanIndexType spanIdx,GirderIndexType gdrIdx);

   void ValidatePointsOfInterest(SpanIndexType span,GirderIndexType gdr);
   void LayoutPointsOfInterest(SpanIndexType span,GirderIndexType gdr);
   void LayoutRegularPoi(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts);
   void LayoutRegularPoiCastingYard(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts,PoiAttributeType attrib,pgsPoiMgr* pPoiMgr);
   void LayoutRegularPoiBridgeSite(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts,PoiAttributeType attrib,pgsPoiMgr* pPoiMgr);

   void LayoutSpecialPoi(SpanIndexType span,GirderIndexType gdr);
   void LayoutEndSizePoi(SpanIndexType span,GirderIndexType gdr);
   void LayoutHarpingPointPoi(SpanIndexType span,GirderIndexType gdr);
   void LayoutPrestressTransferAndDebondPoi(SpanIndexType span,GirderIndexType gdr);
   void LayoutPoiForDiaphragmLoads(SpanIndexType span,GirderIndexType gdr);
   void LayoutPoiForShear(SpanIndexType span,GirderIndexType gdr);
   void LayoutPoiForBarCutoffs(SpanIndexType span,GirderIndexType gdr);
   void LayoutPoiForHandling(SpanIndexType span,GirderIndexType gdr);
   void LayoutPoiForSectionChanges(SpanIndexType span,GirderIndexType gdr);
   void UpdateHaulingPoi(SpanIndexType span,GirderIndexType gdr);
   void UpdateLiftingPoi(SpanIndexType span,GirderIndexType gdr);

   void ValidateGirderOrientation(SpanIndexType span,GirderIndexType gdr);

   //void ValidateLiftingPointsOfInterest(SpanIndexType span,GirderIndexType gdr);
   void LayoutLiftingPoi(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts);

   //void ValidateHaulingPointsOfInterest(SpanIndexType span,GirderIndexType gdr);
   void LayoutHaulingPoi(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts);

   void LayoutHandlingPoi(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr, Uint16 nPnts, PoiAttributeType attrib,pgsPoiMgr* pPoiMgr);
   void LayoutHandlingPoi(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr, Uint16 nPnts, Float64 leftOverhang, Float64 rightOverhang, PoiAttributeType attrib, PoiAttributeType supportAttribute,pgsPoiMgr* pPoiMgr);

   Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx);

   void CheckBridge();

   void ConfigureConnection(const ConnectionLibraryEntry* pEntry, IConnection* connection);

   void InvalidateConcrete();
   bool ValidateConcrete();
   bool IsConcreteDensityInRange(Float64 density,pgsTypes::ConcreteType type);

   Float64 GetHalfElevation(const pgsPointOfInterest& poi); // returns location of half height of composite girder

   void GetAlignment(IAlignment** ppAlignment);
   void GetProfile(IProfile** ppProfile);
   void GetBarrierProperties(pgsTypes::TrafficBarrierOrientation orientation,IShapeProperties** props);

   void InvalidateUserLoads();
   void ValidateUserLoads();
   void ValidatePointLoads();
   void ValidateDistributedLoads();
   void ValidateMomentLoads();

   std::vector<UserPointLoad>* GetUserPointLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr);
   std::vector<UserDistributedLoad>* GetUserDistributedLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr);
   std::vector<UserMomentLoad>* GetUserMomentLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr);

   HRESULT GetSlabOverhangs(double distance,double* pLeft,double* pRight);
   Float64 GetDistanceFromStartOfBridge(const pgsPointOfInterest& poi);
   Float64 GetDistanceFromStartOfBridge(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan);
   HRESULT GetGirderSection(const pgsPointOfInterest& poi,IGirderSection** gdrSection);
   HRESULT GetSuperstructureMember(SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* *ssmbr);
   HRESULT GetGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,IPrecastGirder** girder);
   HRESULT GetConnections(SpanIndexType spanIdx,GirderIndexType gdrIdx,IConnection** startConnection,IConnection** endConnection);
   Float64 GetGrossSlabDepth();
   Float64 GetCastDepth();
   Float64 GetPanelDepth();
   Float64 GetSlabOverhangDepth();


   // Methods that return simple properties without data validation
   // Generally called during validation so as not to cause re-entry into the validation loop
   SpanIndexType GetSpanCount_Private();

   StrandIndexType GetNextNumStraightStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);
   StrandIndexType GetNextNumHarpedStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);
   StrandIndexType GetNextNumTempStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);
   StrandIndexType GetPrevNumStraightStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);
   StrandIndexType GetPrevNumHarpedStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);
   StrandIndexType GetPrevNumTempStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum);

   void GetGirderShapeDirect(const pgsPointOfInterest& poi,IShape** ppShape);
   BarSize GetBarSize(Int32 size);
   REBARDEVLENGTHDETAILS GetRebarDevelopmentLengthDetails(IRebar* rebar,double fc);

   Float64 GetAsTensionSideOfGirder(const pgsPointOfInterest& poi,bool bDevAdjust,bool bTensionTop);
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi, bool bDevAdjust,bool bTensionTop);
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi, const GDRCONFIG& config,bool bDevAdjust,bool bTensionTop);
   Float64 GetApsTensionSide(const pgsPointOfInterest& poi, bool bUseConfig,const GDRCONFIG& config,bool bDevAdjust,bool bTensionTop);

   Float64 GetAsDeckMats(const pgsPointOfInterest& poi,ILongRebarGeometry::DeckRebarType drt,bool bTopMat,bool bBottomMat);

   void GetShapeProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps);
   Float64 GetCutLocation(const pgsPointOfInterest& poi);

   void NoDeckEdgePoint(SpanIndexType spanIdx,PierIndexType pierIdx,DirectionType side,IPoint2d** ppPoint);

   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,IPoint2d** point);
   void GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,IPoint3d** point);
};

#endif //__BRIDGEAGENT_H_
