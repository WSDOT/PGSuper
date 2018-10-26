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

// ProjectAgentImp.h : Declaration of the CProjectAgentImp

#ifndef __PROJECTAGENT_H_
#define __PROJECTAGENT_H_

#include "resource.h"       // main symbols

#include <StrData.h>
#include <vector>
#include "CPProjectAgent.h"
#include <MathEx.h>
#include <PsgLib\LibraryManager.h>

#include <Units\SysUnits.h>

#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\Keys.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\LoadFactors.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include "LibraryEntryObserver.h"

#include <IFace\GirderHandling.h>


class CStructuredLoad;
class ConflictList;
class CBridgeChangedHint;


/////////////////////////////////////////////////////////////////////////////
// CProjectAgentImp
class ATL_NO_VTABLE CProjectAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
   //public CComRefCountTracer<CProjectAgentImp,CComObjectRootEx<CComSingleThreadModel> >,
	public CComCoClass<CProjectAgentImp, &CLSID_ProjectAgent>,
	public IConnectionPointContainerImpl<CProjectAgentImp>,
   public CProxyIProjectPropertiesEventSink<CProjectAgentImp>,
   public CProxyIEnvironmentEventSink<CProjectAgentImp>,
   public CProxyIBridgeDescriptionEventSink<CProjectAgentImp>,
   public CProxyISpecificationEventSink<CProjectAgentImp>,
   public CProxyIRatingSpecificationEventSink<CProjectAgentImp>,
   public CProxyILibraryConflictEventSink<CProjectAgentImp>,
   public CProxyILoadModifiersEventSink<CProjectAgentImp>,
   public CProxyIEventsEventSink<CProjectAgentImp>,
   public IAgentEx,
   public IAgentPersist,
   public IProjectProperties,
   public IEnvironment,
   public IRoadwayData,
   public IBridgeDescription,
   public ISegmentData,
   public IShear,
   public ILongitudinalRebar,
   public ISpecification,
   public IRatingSpecification,
   public ILibraryNames,
   public ILibrary,
   public ILoadModifiers,
   public ISegmentHauling,
   public ISegmentLifting,
   public IImportProjectLibrary,
   public IUserDefinedLoadData,
   public IEvents,
   public ILimits,
   public ILoadFactors,
   public ILiveLoads,
   public IEventMap,
   public IEffectiveFlangeWidth,
   public ILossParameters
{  
public:
	CProjectAgentImp(); 
   virtual ~CProjectAgentImp();

   DECLARE_PROTECT_FINAL_CONSTRUCT();
   HRESULT FinalConstruct();
   void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_PROJECTAGENT)

BEGIN_COM_MAP(CProjectAgentImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
	COM_INTERFACE_ENTRY(IAgentPersist)
	COM_INTERFACE_ENTRY(IProjectProperties)
   COM_INTERFACE_ENTRY(IEnvironment)
   COM_INTERFACE_ENTRY(IRoadwayData)
   COM_INTERFACE_ENTRY(IBridgeDescription)
   COM_INTERFACE_ENTRY(ISegmentData)
   COM_INTERFACE_ENTRY(IShear)
   COM_INTERFACE_ENTRY(ILongitudinalRebar)
   COM_INTERFACE_ENTRY(ISpecification)
   COM_INTERFACE_ENTRY(IRatingSpecification)
   COM_INTERFACE_ENTRY(ILibraryNames)
   COM_INTERFACE_ENTRY(ILibrary)
   COM_INTERFACE_ENTRY(ILoadModifiers)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
   COM_INTERFACE_ENTRY(ISegmentHauling)
   COM_INTERFACE_ENTRY(ISegmentLifting)
   COM_INTERFACE_ENTRY(IImportProjectLibrary)
   COM_INTERFACE_ENTRY(IUserDefinedLoadData)
   COM_INTERFACE_ENTRY(IEvents)
   COM_INTERFACE_ENTRY(ILimits)
   COM_INTERFACE_ENTRY(ILoadFactors)
   COM_INTERFACE_ENTRY(ILiveLoads)
   COM_INTERFACE_ENTRY(IEventMap)
   COM_INTERFACE_ENTRY(IEffectiveFlangeWidth)
   COM_INTERFACE_ENTRY(ILossParameters)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CProjectAgentImp)
   CONNECTION_POINT_ENTRY( IID_IProjectPropertiesEventSink )
   CONNECTION_POINT_ENTRY( IID_IEnvironmentEventSink )
   CONNECTION_POINT_ENTRY( IID_IBridgeDescriptionEventSink )
   CONNECTION_POINT_ENTRY( IID_ISpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_IRatingSpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_ILibraryConflictEventSink )
   CONNECTION_POINT_ENTRY( IID_ILoadModifiersEventSink )
   CONNECTION_POINT_ENTRY( IID_IEventsSink )
END_CONNECTION_POINT_MAP()

   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidBridgeDescriptionWarning;
   StatusCallbackIDType m_scidRebarStrengthWarning;


// IAgent
public:
	STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker);
   STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// IAgentPersist
public:
	STDMETHOD(Load)(/*[in]*/ IStructuredLoad* pStrLoad);
	STDMETHOD(Save)(/*[in]*/ IStructuredSave* pStrSave);

// IProjectProperties
public:
   virtual std::_tstring GetBridgeName() const;
   virtual void SetBridgeName(const std::_tstring& name);
   virtual std::_tstring GetBridgeID() const;
   virtual void SetBridgeID(const std::_tstring& bid);
   virtual std::_tstring GetJobNumber() const;
   virtual void SetJobNumber(const std::_tstring& jid);
   virtual std::_tstring GetEngineer() const;
   virtual void SetEngineer(const std::_tstring& eng);
   virtual std::_tstring GetCompany() const;
   virtual void SetCompany(const std::_tstring& company);
   virtual std::_tstring GetComments() const;
   virtual void SetComments(const std::_tstring& comments);

// IEnvironment
public:
   virtual enumExposureCondition GetExposureCondition() const;
	virtual void SetExposureCondition(enumExposureCondition newVal);
	virtual Float64 GetRelHumidity() const;
	virtual void SetRelHumidity(Float64 newVal);

// IRoadwayData
public:
   virtual void SetAlignmentData2(const AlignmentData2& data);
   virtual AlignmentData2 GetAlignmentData2() const;
   virtual void SetProfileData2(const ProfileData2& data);
   virtual ProfileData2 GetProfileData2() const;
   virtual void SetRoadwaySectionData(const RoadwaySectionData& data);
   virtual RoadwaySectionData GetRoadwaySectionData() const;

// IBridgeDescription
public:
   virtual const CBridgeDescription2* GetBridgeDescription();
   virtual void SetBridgeDescription(const CBridgeDescription2& desc);
   virtual const CDeckDescription2* GetDeckDescription();
   virtual void SetDeckDescription(const CDeckDescription2& deck);
   virtual SpanIndexType GetSpanCount();
   virtual const CSpanData2* GetSpan(SpanIndexType spanIdx);
   virtual void SetSpan(SpanIndexType spanIdx,const CSpanData2& spanData);
   virtual PierIndexType GetPierCount();
   virtual const CPierData2* GetPier(PierIndexType pierIdx);
   virtual const CPierData2* FindPier(PierIDType pierID);
   virtual void SetPierByIndex(PierIndexType pierIdx,const CPierData2& PierData);
   virtual void SetPierByID(PierIDType pierID,const CPierData2& PierData);
   virtual SupportIndexType GetTemporarySupportCount();
   virtual const CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx);
   virtual const CTemporarySupportData* FindTemporarySupport(SupportIDType tsID);
   virtual void SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData);
   virtual void SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData);
   virtual GroupIndexType GetGirderGroupCount();
   virtual const CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx);
   virtual const CSplicedGirderData* GetGirder(const CGirderKey& girderKey);
   virtual const CSplicedGirderData* FindGirder(GirderIDType gdrID);
   virtual void SetGirder(const CGirderKey& girderKey,const CSplicedGirderData& splicedGirder);
   virtual const CPTData* GetPostTensioning(const CGirderKey& girderKey);
   virtual void SetPostTensioning(const CGirderKey& girderKey,const CPTData& ptData);
   virtual const CPrecastSegmentData* GetPrecastSegmentData(const CSegmentKey& segmentKey);
   virtual void SetPrecastSegmentData(const CSegmentKey& segmentKey,const CPrecastSegmentData& segment);
   virtual const CClosureJointData* GetClosureJointData(const CSegmentKey& closureKey);
   virtual void SetClosureJointData(const CSegmentKey& closureKey,const CClosureJointData& closure);
   virtual void SetSpanLength(SpanIndexType spanIdx,Float64 newLength);
   virtual void MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption);
   virtual SupportIndexType MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation);
   virtual void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt);
   virtual void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml);
   virtual void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing2& spacing);
   virtual void SetGirderSpacingAtStartOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing);
   virtual void SetGirderSpacingAtEndOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing);
   virtual void SetGirderName(const CGirderKey& girderKey, LPCTSTR strGirderName);
   virtual void SetGirderGroup(GroupIndexType grpIdx,const CGirderGroupData& girderGroup);
   virtual void SetGirderCount(GroupIndexType grpIdx,GirderIndexType nGirders);
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierConnectionType connectionType);
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType connectionType,EventIndexType castClosureEventIdx);
   virtual void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan);
   virtual void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace, Float64 spanLength, const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType eventIdx);
   virtual void InsertTemporarySupport(CTemporarySupportData* pTSData,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureJointEventIdx);
   virtual void DeleteTemporarySupportByIndex(SupportIndexType tsIdx);
   virtual void DeleteTemporarySupportByID(SupportIDType tsID);
   virtual void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method);
   virtual pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod();
   virtual void UseSameNumberOfGirdersInAllGroups(bool bUseSame);
   virtual bool UseSameNumberOfGirdersInAllGroups();
   virtual void SetGirderCount(GirderIndexType nGirders);
   virtual void UseSameGirderForEntireBridge(bool bSame);
   virtual bool UseSameGirderForEntireBridge();
   virtual void SetGirderName(LPCTSTR strGirderName);
   virtual void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs);
   virtual pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   virtual void SetGirderSpacing(Float64 spacing);
   virtual void SetMeasurementType(pgsTypes::MeasurementType mt);
   virtual pgsTypes::MeasurementType GetMeasurementType();
   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation();
   virtual void SetSlabOffsetType(pgsTypes::SlabOffsetType offsetType);
   virtual void SetSlabOffset(Float64 slabOffset);
   virtual void SetSlabOffset(GroupIndexType grpIdx, PierIndexType pierIdx, Float64 offset);
   virtual void SetSlabOffset(GroupIndexType grpIdx, PierIndexType pierIdx, GirderIndexType gdrIdx, Float64 offset);
   virtual Float64 GetSlabOffset(GroupIndexType grpidx, PierIndexType pierIdx, GirderIndexType gdrIdx);
   virtual pgsTypes::SlabOffsetType GetSlabOffsetType();
   virtual std::vector<pgsTypes::PierConnectionType> GetPierConnectionTypes(PierIndexType pierIdx);
   virtual std::vector<pgsTypes::PierSegmentConnectionType> GetPierSegmentConnectionTypes(PierIndexType pierIdx);
   virtual const CTimelineManager* GetTimelineManager();
   virtual void SetTimelineManager(CTimelineManager& timelineMbr);
   virtual EventIndexType AddTimelineEvent(const CTimelineEvent& timelineEvent);
   virtual EventIndexType GetEventCount();
   virtual const CTimelineEvent* GetEventByIndex(EventIndexType eventIdx);
   virtual const CTimelineEvent* GetEventByID(EventIDType eventID);
   virtual void SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& stage);
   virtual void SetEventByID(EventIDType eventID,const CTimelineEvent& stage);
   virtual void SetSegmentConstructionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx);
   virtual void SetSegmentConstructionEventByID(const CSegmentKey& segmentKey,EventIDType eventID);
   virtual EventIndexType GetSegmentConstructionEventIndex(const CSegmentKey& segmentKey);
   virtual EventIDType GetSegmentConstructionEventID(const CSegmentKey& segmentKey);
   virtual EventIndexType GetPierErectionEvent(PierIndexType pierIdx);
   virtual void SetPierErectionEventByIndex(PierIndexType pierIdx,EventIndexType eventIdx);
   virtual void SetPierErectionEventByID(PierIndexType pierIdx,IDType eventID);
   virtual void SetTempSupportEventsByIndex(SupportIndexType tsIdx,EventIndexType erectIdx,EventIndexType removeIdx);
   virtual void SetTempSupportEventsByID(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx);
   virtual void SetSegmentErectionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx);
   virtual void SetSegmentErectionEventByID(const CSegmentKey& segmentKey,EventIDType eventID);
   virtual EventIndexType GetSegmentErectionEventIndex(const CSegmentKey& segmentKey);
   virtual EventIDType GetSegmentErectionEventID(const CSegmentKey& segmentKey);
   virtual EventIndexType GetCastClosureJointEventIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx);
   virtual EventIDType GetCastClosureJointEventID(GroupIndexType grpIdx,CollectionIndexType closureIdx);
   virtual void SetCastClosureJointEventByIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIndexType eventIdx);
   virtual void SetCastClosureJointEventByID(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIDType eventID);
   virtual EventIndexType GetStressTendonEventIndex(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual EventIDType GetStressTendonEventID(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual void SetStressTendonEventByIndex(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIndexType eventIdx);
   virtual void SetStressTendonEventByID(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIDType eventID);
   virtual EventIndexType GetCastDeckEventIndex();
   virtual EventIDType GetCastDeckEventID();
   virtual int SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline);
   virtual int SetCastDeckEventByID(EventIDType eventID,bool bAdjustTimeline);
   virtual EventIndexType GetRailingSystemLoadEventIndex();
   virtual EventIDType GetRailingSystemLoadEventID();
   virtual void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx);
   virtual void SetRailingSystemLoadEventByID(EventIDType eventID);
   virtual EventIndexType GetOverlayLoadEventIndex();
   virtual EventIDType GetOverlayLoadEventID();
   virtual void SetOverlayLoadEventByIndex(EventIndexType eventIdx);
   virtual void SetOverlayLoadEventByID(EventIDType eventID);
   virtual EventIndexType GetLiveLoadEventIndex();
   virtual EventIDType GetLiveLoadEventID();
   virtual void SetLiveLoadEventByIndex(EventIndexType eventIdx);
   virtual void SetLiveLoadEventByID(EventIDType eventID);
   virtual GroupIDType GetGroupID(GroupIndexType groupIdx);
   virtual GirderIDType GetGirderID(const CGirderKey& girderKey);
   virtual SegmentIDType GetSegmentID(const CSegmentKey& segmentKey);

// ISegmentData 
public:
   virtual const matPsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const;
   virtual void SetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const matPsStrand* pmat);

   virtual const CGirderMaterial* GetSegmentMaterial(const CSegmentKey& segmentKey) const;
   virtual void SetSegmentMaterial(const CSegmentKey& segmentKey,const CGirderMaterial& material);

   virtual const CStrandData* GetStrandData(const CSegmentKey& segmentKey);
   virtual void SetStrandData(const CSegmentKey& segmentKey,const CStrandData& strands);

   virtual const CHandlingData* GetHandlingData(const CSegmentKey& segmentKey);
   virtual void SetHandlingData(const CSegmentKey& segmentKey,const CHandlingData& handling);

// IShear
public:
   virtual std::_tstring GetSegmentStirrupMaterial(const CSegmentKey& segmentKey) const;
   virtual void GetSegmentStirrupMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade);
   virtual void SetSegmentStirrupMaterial(const CSegmentKey& segmentKey,matRebar::Type type,matRebar::Grade grade);
   virtual const CShearData2* GetSegmentShearData(const CSegmentKey& segmentKey) const;
   virtual void SetSegmentShearData(const CSegmentKey& segmentKey,const CShearData2& data);
   virtual std::_tstring GetClosureJointStirrupMaterial(const CClosureKey& closureKey) const;
   virtual void GetClosureJointStirrupMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade);
   virtual void SetClosureJointStirrupMaterial(const CClosureKey& closureKey,matRebar::Type type,matRebar::Grade grade);
   virtual const CShearData2* GetClosureJointShearData(const CClosureKey& closureKey) const;
   virtual void SetClosureJointShearData(const CClosureKey& closureKey,const CShearData2& data);

// ILongitudinalRebar
public:
   virtual std::_tstring GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey) const;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade);
   virtual void SetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type type,matRebar::Grade grade);
   virtual const CLongitudinalRebarData* GetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey) const;
   virtual void SetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey,const CLongitudinalRebarData& data);

   virtual std::_tstring GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey) const;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade);
   virtual void SetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type type,matRebar::Grade grade);
   virtual const CLongitudinalRebarData* GetClosureJointLongitudinalRebarData(const CClosureKey& closureKey) const;
   virtual void SetClosureJointLongitudinalRebarData(const CClosureKey& closureKey,const CLongitudinalRebarData& data);

// ISpecification
public:
   virtual std::_tstring GetSpecification();
   virtual void SetSpecification(const std::_tstring& spec);
   virtual void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType);
   virtual Uint16 GetMomentCapacityMethod();
   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType);
   virtual pgsTypes::AnalysisType GetAnalysisType();
   virtual arDesignOptions GetDesignOptions(const CGirderKey& girderKey);
   virtual bool IsSlabOffsetDesignEnabled();
   virtual pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType();

// IRatingSpecification
public:
   virtual bool AlwaysLoadRate();
   virtual bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType);
   virtual void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable);
   virtual std::_tstring GetRatingSpecification();
   virtual void SetADTT(Int16 adtt);
   virtual Int16 GetADTT();
   virtual void SetRatingSpecification(const std::_tstring& spec);
   virtual void IncludePedestrianLiveLoad(bool bInclude);
   virtual bool IncludePedestrianLiveLoad();
   virtual void SetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor);
   virtual void GetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor);
   virtual Float64 GetGirderConditionFactor(const CSegmentKey& segmentKey);
   virtual void SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor);
   virtual void GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor);
   virtual Float64 GetDeckConditionFactor();
   virtual void SetSystemFactorFlexure(Float64 sysFactor);
   virtual Float64 GetSystemFactorFlexure();
   virtual void SetSystemFactorShear(Float64 sysFactor);
   virtual Float64 GetSystemFactorShear();
   virtual void SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC);
   virtual Float64 GetDeadLoadFactor(pgsTypes::LimitState ls);
   virtual void SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW);
   virtual Float64 GetWearingSurfaceFactor(pgsTypes::LimitState ls);
   virtual void SetCreepFactor(pgsTypes::LimitState ls,Float64 gCR);
   virtual Float64 GetCreepFactor(pgsTypes::LimitState ls);
   virtual void SetShrinkageFactor(pgsTypes::LimitState ls,Float64 gSH);
   virtual Float64 GetShrinkageFactor(pgsTypes::LimitState ls);
   virtual void SetPrestressFactor(pgsTypes::LimitState ls,Float64 gPS);
   virtual Float64 GetPrestressFactor(pgsTypes::LimitState ls);
   virtual void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL);
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false);
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false);
   virtual void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,Float64 t);
   virtual Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType);
   virtual Float64 GetAllowableTension(pgsTypes::LoadRatingType ratingType,const CSegmentKey& segmentKey);
   virtual void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress);
   virtual bool RateForStress(pgsTypes::LoadRatingType ratingType);
   virtual void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear);
   virtual bool RateForShear(pgsTypes::LoadRatingType ratingType);
   virtual void ExcludeLegalLoadLaneLoading(bool bExclude);
   virtual bool ExcludeLegalLoadLaneLoading();
   virtual void SetYieldStressLimitCoefficient(Float64 x);
   virtual Float64 GetYieldStressLimitCoefficient();
   virtual void SetSpecialPermitType(pgsTypes::SpecialPermitType type);
   virtual pgsTypes::SpecialPermitType GetSpecialPermitType();

// ILibraryNames
public:
   virtual void EnumGdrConnectionNames( std::vector<std::_tstring>* pNames ) const;
   virtual void EnumGirderNames( std::vector<std::_tstring>* pNames ) const;
   virtual void EnumGirderNames( LPCTSTR strGirderFamily, std::vector<std::_tstring>* pNames ) const;
   virtual void EnumConcreteNames( std::vector<std::_tstring>* pNames ) const;
   virtual void EnumDiaphragmNames( std::vector<std::_tstring>* pNames ) const;
   virtual void EnumTrafficBarrierNames( std::vector<std::_tstring>* pNames ) const;
   virtual void EnumSpecNames( std::vector<std::_tstring>* pNames) const;
   virtual void EnumRatingCriteriaNames( std::vector<std::_tstring>* pNames) const;
   virtual void EnumLiveLoadNames( std::vector<std::_tstring>* pNames) const;
   virtual void EnumGirderFamilyNames( std::vector<std::_tstring>* pNames );
   virtual void GetBeamFactory(const std::_tstring& strBeamFamily,const std::_tstring& strBeamName,IBeamFactory** ppFactory);

// ILibrary
public:
   virtual void SetLibraryManager(psgLibraryManager* pNewLibMgr);
   virtual psgLibraryManager* GetLibraryManager(); 
   virtual const ConnectionLibraryEntry* GetConnectionEntry(LPCTSTR lpszName ) const;
   virtual const GirderLibraryEntry* GetGirderEntry( LPCTSTR lpszName ) const;
   virtual const ConcreteLibraryEntry* GetConcreteEntry( LPCTSTR lpszName ) const;
   virtual const DiaphragmLayoutEntry* GetDiaphragmEntry( LPCTSTR lpszName ) const;
   virtual const TrafficBarrierEntry* GetTrafficBarrierEntry( LPCTSTR lpszName ) const;
   virtual const SpecLibraryEntry* GetSpecEntry( LPCTSTR lpszName ) const;
   virtual const LiveLoadLibraryEntry* GetLiveLoadEntry( LPCTSTR lpszName ) const;
   virtual ConcreteLibrary&        GetConcreteLibrary();
   virtual ConnectionLibrary&      GetConnectionLibrary();
   virtual GirderLibrary&          GetGirderLibrary();
   virtual DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary();
   virtual TrafficBarrierLibrary&  GetTrafficBarrierLibrary();
   virtual LiveLoadLibrary*        GetLiveLoadLibrary();
   virtual SpecLibrary*            GetSpecLibrary();
   virtual std::vector<libEntryUsageRecord> GetLibraryUsageRecords() const;
   virtual void GetMasterLibraryInfo(std::_tstring& strPublisher,std::_tstring& strMasterLib,sysTime& time) const;
   virtual RatingLibrary* GetRatingLibrary();
   virtual const RatingLibrary* GetRatingLibrary() const;
   virtual const RatingLibraryEntry* GetRatingEntry( LPCTSTR lpszName ) const;

// ILoadModifiers
public:
   virtual void SetDuctilityFactor(Level level,Float64 value);
   virtual void SetImportanceFactor(Level level,Float64 value);
   virtual void SetRedundancyFactor(Level level,Float64 value);
   virtual Float64 GetDuctilityFactor();
   virtual Float64 GetImportanceFactor();
   virtual Float64 GetRedundancyFactor();
   virtual Level GetDuctilityLevel();
   virtual Level GetImportanceLevel();
   virtual Level GetRedundancyLevel();

// ISegmentLifting
public:
   virtual Float64 GetLeftLiftingLoopLocation(const CSegmentKey& segmentKey);
   virtual Float64 GetRightLiftingLoopLocation(const CSegmentKey& segmentKey);
   virtual void SetLiftingLoopLocations(const CSegmentKey& segmentKey, Float64 left,Float64 right);

// ISegmentHauling
public:
   virtual Float64 GetLeadingOverhang(const CSegmentKey& segmentKey);
   virtual Float64 GetTrailingOverhang(const CSegmentKey& segmentKey);
   virtual void SetTruckSupportLocations(const CSegmentKey& segmentKey, Float64 leftLoc,Float64 rightLoc);

// IImportProjectLibrary
public:
   virtual bool ImportProjectLibraries(IStructuredLoad* pLoad);

// IUserDefinedLoadData
public:
   virtual CollectionIndexType GetPointLoadCount() const;
   virtual CollectionIndexType AddPointLoad(const CPointLoadData& pld);
   virtual const CPointLoadData* GetPointLoad(CollectionIndexType idx) const;
   virtual const CPointLoadData* FindPointLoad(LoadIDType loadID) const;
   virtual void UpdatePointLoad(CollectionIndexType idx, const CPointLoadData& pld);
   virtual void DeletePointLoad(CollectionIndexType idx);

   virtual CollectionIndexType GetDistributedLoadCount() const;
   virtual CollectionIndexType AddDistributedLoad(const CDistributedLoadData& pld);
   virtual const CDistributedLoadData* GetDistributedLoad(CollectionIndexType idx) const;
   virtual const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const;
   virtual void UpdateDistributedLoad(CollectionIndexType idx, const CDistributedLoadData& pld);
   virtual void DeleteDistributedLoad(CollectionIndexType idx);

   virtual CollectionIndexType GetMomentLoadCount() const;
   virtual CollectionIndexType AddMomentLoad(const CMomentLoadData& pld);
   virtual const CMomentLoadData* GetMomentLoad(CollectionIndexType idx) const;
   virtual const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const;
   virtual void UpdateMomentLoad(CollectionIndexType idx, const CMomentLoadData& pld);
   virtual void DeleteMomentLoad(CollectionIndexType idx);

   virtual void SetConstructionLoad(Float64 load);
   virtual Float64 GetConstructionLoad() const;

// ILiveLoads
public:
   virtual bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType);
   virtual PedestrianLoadApplicationType GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType);
   virtual void SetPedestrianLoadApplication(pgsTypes::LiveLoadType llType, PedestrianLoadApplicationType PedLoad);
   virtual std::vector<std::_tstring> GetLiveLoadNames(pgsTypes::LiveLoadType llType);
   virtual void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::_tstring>& names);
   virtual Float64 GetTruckImpact(pgsTypes::LiveLoadType llType);
   virtual void SetTruckImpact(pgsTypes::LiveLoadType llType,Float64 impact);
   virtual Float64 GetLaneImpact(pgsTypes::LiveLoadType llType);
   virtual void SetLaneImpact(pgsTypes::LiveLoadType llType,Float64 impact);
   virtual void SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action);
   virtual LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction();
   virtual std::_tstring GetLLDFSpecialActionText(); // get common string for ignore roa case
   virtual bool IgnoreLLDFRangeOfApplicability(); // true if action is to ignore ROA

// IEvents
public:
   virtual void HoldEvents();
   virtual void FirePendingEvents();
   virtual void CancelPendingEvents();

// ILimits
public:
   virtual Float64 GetMaxSlabFc(pgsTypes::ConcreteType concType);
   virtual Float64 GetMaxSegmentFci(pgsTypes::ConcreteType concType);
   virtual Float64 GetMaxSegmentFc(pgsTypes::ConcreteType concType);
   virtual Float64 GetMaxClosureFci(pgsTypes::ConcreteType concType);
   virtual Float64 GetMaxClosureFc(pgsTypes::ConcreteType concType);
   virtual Float64 GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType);
   virtual Float64 GetMaxConcreteAggSize(pgsTypes::ConcreteType concType);

// ILoadFactors
public:
   const CLoadFactors* GetLoadFactors() const;
   void SetLoadFactors(const CLoadFactors& loadFactors) ;

// IEventMap
public:
   virtual CComBSTR GetEventName(EventIndexType eventIdx);  
   virtual EventIndexType GetEventIndex(CComBSTR bstrEvent);
   virtual CComBSTR GetLimitStateName(pgsTypes::LimitState ls);  

// IEffectiveFlangeWidth
public:
   virtual bool IgnoreEffectiveFlangeWidthLimits();
   virtual void IgnoreEffectiveFlangeWidthLimits(bool bIgnore);

// ILossParameters
public:
   virtual pgsTypes::LossMethod GetLossMethod();
   virtual void IgnoreTimeDependentEffects(bool bIgnore);
   virtual bool IgnoreTimeDependentEffects();
   virtual void SetTendonPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction);
   virtual void GetTendonPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction);
   virtual void SetTemporaryStrandPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction);
   virtual void GetTemporaryStrandPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction);
   virtual void UseGeneralLumpSumLosses(bool bLumpSum);
   virtual bool UseGeneralLumpSumLosses();
   virtual Float64 GetBeforeXferLosses();
   virtual void SetBeforeXferLosses(Float64 loss);
   virtual Float64 GetAfterXferLosses();
   virtual void SetAfterXferLosses(Float64 loss);
   virtual Float64 GetLiftingLosses();
   virtual void SetLiftingLosses(Float64 loss);
   virtual Float64 GetShippingLosses();
   virtual void SetShippingLosses(Float64 loss);
   virtual Float64 GetBeforeTempStrandRemovalLosses();
   virtual void SetBeforeTempStrandRemovalLosses(Float64 loss);
   virtual Float64 GetAfterTempStrandRemovalLosses();
   virtual void SetAfterTempStrandRemovalLosses(Float64 loss);
   virtual Float64 GetAfterDeckPlacementLosses();
   virtual void SetAfterDeckPlacementLosses(Float64 loss);
   virtual Float64 GetAfterSIDLLosses();
   virtual void SetAfterSIDLLosses(Float64 loss);
   virtual Float64 GetFinalLosses();
   virtual void SetFinalLosses(Float64 loss);

#ifdef _DEBUG
   bool AssertValid() const;
#endif//

private:
   DECLARE_EAF_AGENT_DATA;

   // status items must be managed by span girder
   void AddSegmentStatusItem(const CSegmentKey& segmentKey, std::_tstring& message);
   void RemoveSegmentStatusItems(const CSegmentKey& segmentKey);

   // save hash value and vector of status ids
   typedef std::map<CSegmentKey, std::vector<StatusItemIDType> > StatusContainer;
   typedef StatusContainer::iterator StatusIterator;

   StatusContainer m_CurrentGirderStatusItems;

   std::_tstring m_BridgeName;
   std::_tstring m_BridgeID;
   std::_tstring m_JobNumber;
   std::_tstring m_Engineer;
   std::_tstring m_Company;
   std::_tstring m_Comments;

   pgsTypes::AnalysisType m_AnalysisType;
   bool m_bGetAnalysisTypeFromLibrary; // if true, we are reading old input... get the analysis type from the library entry

   std::vector<std::_tstring> m_GirderFamilyNames;

   bool m_bIgnoreEffectiveFlangeWidthLimits;

   // rating data
   std::_tstring m_RatingSpec;
   const RatingLibraryEntry* m_pRatingEntry;
   bool m_bExcludeLegalLoadLaneLoading;
   bool m_bIncludePedestrianLiveLoad;
   pgsTypes::SpecialPermitType m_SpecialPermitType;
   Float64 m_SystemFactorFlexure;
   Float64 m_SystemFactorShear;
   // NOTE: the next group of arrays (m_gDC, m_gDW, etc) are larger than they need to be.
   // This is because of a bug in earlier versions of PGSuper. In those version, data was
   // stored beyond the end of the array. The size of the array has been incresed so that
   // this error wont occur.
   Float64 m_gDC[pgsTypes::LimitStateCount]; // use the IndexFromLimitState to access array
   Float64 m_gDW[pgsTypes::LimitStateCount]; // use the IndexFromLimitState to access array
   Float64 m_gCR[pgsTypes::LimitStateCount]; // use the IndexFromLimitState to access array
   Float64 m_gSH[pgsTypes::LimitStateCount]; // use the IndexFromLimitState to access array
   Float64 m_gPS[pgsTypes::LimitStateCount]; // use the IndexFromLimitState to access array
   Float64 m_gLL[pgsTypes::LimitStateCount]; // use the IndexFromLimitState to access array

   Float64 m_AllowableTensionCoefficient[6]; // index is load rating type
   bool    m_bRateForStress[6]; // index is load rating type (for permit rating, it means to do the service I checks, otherwise service III)
   bool    m_bRateForShear[6]; // index is load rating type
   bool    m_bEnableRating[6]; // index is load rating type
   Int16 m_ADTT; // < 0 = Unknown
   Float64 m_AllowableYieldStressCoefficient; // fr <= xfy for Service I permit rating

   // Environment Data
   enumExposureCondition m_ExposureCondition;
   Float64 m_RelHumidity;

   // Alignment Data
   Float64 m_AlignmentOffset_Temp;
   bool m_bUseTempAlignmentOffset;

   AlignmentData2 m_AlignmentData2;
   ProfileData2   m_ProfileData2;
   RoadwaySectionData m_RoadwaySectionData;

   // Bridge Description Data
   mutable CBridgeDescription2 m_BridgeDescription;

   // Prestressing Data
   void UpdateJackingForce();
   void UpdateJackingForce(const CSegmentKey& segmentKey);
   mutable bool m_bUpdateJackingForce;

   // Specification Data
   std::_tstring m_Spec;
   const SpecLibraryEntry* m_pSpecEntry;


   // Live load selection
   struct LiveLoadSelection
   {
      std::_tstring                 EntryName;
      const LiveLoadLibraryEntry* pEntry; // NULL if this is a system defined live load (HL-93)

      LiveLoadSelection() { pEntry = NULL; }

      bool operator==(const LiveLoadSelection& other) const
      { return pEntry == other.pEntry; }

      bool operator<(const LiveLoadSelection& other) const
      { return EntryName < other.EntryName; }
   };

   typedef std::vector<LiveLoadSelection> LiveLoadSelectionContainer;
   typedef LiveLoadSelectionContainer::iterator LiveLoadSelectionIterator;

   // index is pgsTypes::LiveLoadTypes constant
   LiveLoadSelectionContainer m_SelectedLiveLoads[8];
   Float64 m_TruckImpact[8];
   Float64 m_LaneImpact[8];
   PedestrianLoadApplicationType m_PedestrianLoadApplicationType[3]; // lltDesign, lltPermit, lltFatigue only

   std::vector<std::_tstring> m_ReservedLiveLoads; // reserved live load names (names not found in library)
   bool IsReservedLiveLoad(const std::_tstring& strName);

   LldfRangeOfApplicabilityAction m_LldfRangeOfApplicabilityAction;
   bool m_bGetIgnoreROAFromLibrary; // if true, we are reading old input... get the Ignore ROA setting from the spec library entry

   // Load Modifiers
   ILoadModifiers::Level m_DuctilityLevel;
   ILoadModifiers::Level m_ImportanceLevel;
   ILoadModifiers::Level m_RedundancyLevel;
   Float64 m_DuctilityFactor;
   Float64 m_ImportanceFactor;
   Float64 m_RedundancyFactor;

   // Load Factors
   CLoadFactors m_LoadFactors;

   //
   // Prestress Losses
   //

   bool m_bIgnoreTimeDependentEffects; // if true, time dependent effects (creep, shrinkage, relaxation)
   // are ignored in the time-step analysis

   // General Lump Sum Losses
   bool m_bGeneralLumpSum; // if true, the loss method specified in the project criteria is ignored
                           // and general lump sum losses are used
   Float64 m_BeforeXferLosses;
   Float64 m_AfterXferLosses;
   Float64 m_LiftingLosses;
   Float64 m_ShippingLosses;
   Float64 m_BeforeTempStrandRemovalLosses;
   Float64 m_AfterTempStrandRemovalLosses;
   Float64 m_AfterDeckPlacementLosses;
   Float64 m_AfterSIDLLosses;
   Float64 m_FinalLosses;

   // Post-tension Tendon
   Float64 m_Dset_PT;
   Float64 m_WobbleFriction_PT;
   Float64 m_FrictionCoefficient_PT;

   // Post-tension Temporary Top Strand
   Float64 m_Dset_TTS;
   Float64 m_WobbleFriction_TTS;
   Float64 m_FrictionCoefficient_TTS;

   // user defined loads
   typedef std::vector<CPointLoadData> PointLoadList;
   typedef PointLoadList::iterator PointLoadListIterator;
   PointLoadList m_PointLoads;

   typedef std::vector<CDistributedLoadData> DistributedLoadList;
   typedef DistributedLoadList::iterator DistributedLoadListIterator;
   DistributedLoadList m_DistributedLoads;

   typedef std::vector<CMomentLoadData> MomentLoadList;
   typedef MomentLoadList::iterator MomentLoadListIterator;
   MomentLoadList m_MomentLoads;

   Float64 m_ConstructionLoad;

   HRESULT SavePointLoads(IStructuredSave* pSave);
   HRESULT LoadPointLoads(IStructuredLoad* pLoad);
   HRESULT SaveDistributedLoads(IStructuredSave* pSave);
   HRESULT LoadDistributedLoads(IStructuredLoad* pLoad);
   HRESULT SaveMomentLoads(IStructuredSave* pSave);
   HRESULT LoadMomentLoads(IStructuredLoad* pLoad);

   Uint32 m_PendingEvents;
   int m_EventHoldCount;
   bool m_bFiringEvents;
   std::map<CGirderKey,Uint32> m_PendingEventsHash; // girders that have pending events
   std::vector<CBridgeChangedHint*> m_PendingBridgeChangedHints;

   // Callback methods for structured storage map
   static HRESULT SpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT RatingSpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT UnitModeProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT AlignmentProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT ProfileProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT SuperelevationProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT PierDataProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT PierDataProc2(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT XSectionDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT XSectionDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT BridgeDescriptionProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT PrestressingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT PrestressingDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT ShearDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT ShearDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LongitudinalRebarDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LongitudinalRebarDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LoadFactorsProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingLoadDataProc(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT UserLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiveLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT SaveLiveLoad(IStructuredSave* pSave,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT LoadLiveLoad(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT EffectiveFlangeWidthProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LossesProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);

   void ValidateStrands(const CSegmentKey& segmentKey,CPrecastSegmentData* pSegment,bool fromLibrary);
   void ConvertLegacyDebondData(CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGdrEntry);
   void ConvertLegacyExtendedStrandData(CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGdrEntry);

   Float64 GetMaxPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType nStrands) const;
   Float64 GetMaxPjack(const CSegmentKey& segmentKey,StrandIndexType nStrands,const matPsStrand* pStrand) const;

   bool ResolveLibraryConflicts(const ConflictList& rList);
   void DealWithGirderLibraryChanges(bool fromLibrary);  // behavior is different if problem is caused by a library change
   
   void MoveBridge(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustNextSpan(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,Float64 newStation);

   void SpecificationChanged(bool bFireEvent);
   void InitSpecification(const std::_tstring& spec);

   void RatingSpecificationChanged(bool bFireEvent);
   void InitRatingSpecification(const std::_tstring& spec);

   HRESULT FireContinuityRelatedSpanChange(const CSpanKey& spanKey,Uint32 lHint); 

   void UseBridgeLibraryEntries();
   void UseGirderLibraryEntries();
   void ReleaseBridgeLibraryEntries();
   void ReleaseGirderLibraryEntries();

   void VerifyRebarGrade();

   void ValidateBridgeModel();
   IDType m_BridgeStabilityStatusItemID;

   DECLARE_STRSTORAGEMAP(CProjectAgentImp)

   psgLibraryManager* m_pLibMgr;
   pgsLibraryEntryObserver m_LibObserver;
   friend pgsLibraryEntryObserver;

   friend CProxyIProjectPropertiesEventSink<CProjectAgentImp>;
   friend CProxyIEnvironmentEventSink<CProjectAgentImp>;
   friend CProxyIBridgeDescriptionEventSink<CProjectAgentImp>;
   friend CProxyISpecificationEventSink<CProjectAgentImp>;
   friend CProxyIRatingSpecificationEventSink<CProjectAgentImp>;
   friend CProxyILibraryConflictEventSink<CProjectAgentImp>;
   friend CProxyILoadModifiersEventSink<CProjectAgentImp>;

   // In early versions of PGSuper, the girder concrete was stored by named reference
   // to a concrete girder library entry. Then, concrete parameters where assigned to
   // each girder individually. In order to read old files and populate the data fields
   // of the GirderData object, this concrete library name needed to be stored somewhere.
   // That is the purpose of this data member. It is only used for temporary storage
   // while loading old files
   std::_tstring m_strOldGirderConcreteName;

   // Older versions of PGSuper (before version 3.0), only conventional precast girder
   // bridges where modeled. Version 3.0 added spliced girder capabilities. The
   // stages for precast girder bridges were a fixed model. This method is called
   // when loading old files and maps the old fixed stage model into the current
   // user defined staging model
   void CreatePrecastGirderBridgeTimelineEvents();

   CPrecastSegmentData* GetSegment(const CSegmentKey& segmentKey);
   const CPrecastSegmentData* GetSegment(const CSegmentKey& segmentKey) const;
};

#endif //__PROJECTAGENT_H_

