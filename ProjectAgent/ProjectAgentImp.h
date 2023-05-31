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

// ProjectAgentImp.h : Declaration of the CProjectAgentImp

#ifndef __PROJECTAGENT_H_
#define __PROJECTAGENT_H_

#include "CLSID.h"

#include "resource.h"       // main symbols

#include <StrData.h>
#include <vector>
#include "CPProjectAgent.h"
#include <MathEx.h>
#include <PsgLib\LibraryManager.h>

#include <Units\Convert.h>

#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\Keys.h>
#include <PgsExt\LoadFactors.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadManager.h>
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
   public CProxyILossParametersEventSink<CProjectAgentImp>,
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
   public ILossParameters,
   public IValidate
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
   COM_INTERFACE_ENTRY(IValidate)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CProjectAgentImp)
   CONNECTION_POINT_ENTRY( IID_IProjectPropertiesEventSink )
   CONNECTION_POINT_ENTRY( IID_IEnvironmentEventSink )
   CONNECTION_POINT_ENTRY( IID_IBridgeDescriptionEventSink )
   CONNECTION_POINT_ENTRY( IID_ISpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_IRatingSpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_ILibraryConflictEventSink )
   CONNECTION_POINT_ENTRY( IID_ILoadModifiersEventSink )
   CONNECTION_POINT_ENTRY( IID_ILossParametersEventSink )
   CONNECTION_POINT_ENTRY( IID_IEventsSink )
END_CONNECTION_POINT_MAP()

   StatusCallbackIDType m_scidBridgeDescriptionInfo;
   StatusCallbackIDType m_scidBridgeDescriptionWarning;
   StatusCallbackIDType m_scidBridgeDescriptionError;
   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidRebarStrengthWarning;
   StatusCallbackIDType m_scidLoadDescriptionWarning;
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

// IAgentPersist
public:
	STDMETHOD(Load)(/*[in]*/ IStructuredLoad* pStrLoad) override;
	STDMETHOD(Save)(/*[in]*/ IStructuredSave* pStrSave) override;

// IProjectProperties
public:
   virtual LPCTSTR GetBridgeName() const override;
   virtual void SetBridgeName(LPCTSTR name) override;
   virtual LPCTSTR GetBridgeID() const override;
   virtual void SetBridgeID(LPCTSTR bid) override;
   virtual LPCTSTR GetJobNumber() const override;
   virtual void SetJobNumber(LPCTSTR jid) override;
   virtual LPCTSTR GetEngineer() const override;
   virtual void SetEngineer(LPCTSTR eng) override;
   virtual LPCTSTR GetCompany() const override;
   virtual void SetCompany(LPCTSTR company) override;
   virtual LPCTSTR GetComments() const override;
   virtual void SetComments(LPCTSTR comments) override;

// IEnvironment
public:
   virtual enumExposureCondition GetExposureCondition() const override;
	virtual void SetExposureCondition(enumExposureCondition newVal) override;
	virtual Float64 GetRelHumidity() const override;
	virtual void SetRelHumidity(Float64 newVal) override;

// IRoadwayData
public:
   virtual void SetAlignmentData2(const AlignmentData2& data) override;
   virtual const AlignmentData2& GetAlignmentData2() const override;
   virtual void SetProfileData2(const ProfileData2& data) override;
   virtual const ProfileData2& GetProfileData2() const override;
   virtual void SetRoadwaySectionData(const RoadwaySectionData& data) override;
   virtual const RoadwaySectionData& GetRoadwaySectionData() const override;

// IBridgeDescription
public:
   virtual const CBridgeDescription2* GetBridgeDescription() const override;
   virtual void SetBridgeDescription(const CBridgeDescription2& desc) override;
   virtual const CDeckDescription2* GetDeckDescription() const override;
   virtual void SetDeckDescription(const CDeckDescription2& deck) override;
   virtual SpanIndexType GetSpanCount() const override;
   virtual const CSpanData2* GetSpan(SpanIndexType spanIdx) const override;
   virtual void SetSpan(SpanIndexType spanIdx,const CSpanData2& spanData) override;
   virtual PierIndexType GetPierCount() const override;
   virtual const CPierData2* GetPier(PierIndexType pierIdx) const override;
   virtual const CPierData2* FindPier(PierIDType pierID) const override;
   virtual void SetPierByIndex(PierIndexType pierIdx,const CPierData2& PierData) override;
   virtual void SetPierByID(PierIDType pierID,const CPierData2& PierData) override;
   virtual SupportIndexType GetTemporarySupportCount() const override;
   virtual const CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx) const override;
   virtual const CTemporarySupportData* FindTemporarySupport(SupportIDType tsID) const override;
   virtual void SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData) override;
   virtual void SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData) override;
   virtual GroupIndexType GetGirderGroupCount() const override;
   virtual const CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx) const override;
   virtual const CSplicedGirderData* GetGirder(const CGirderKey& girderKey) const override;
   virtual const CSplicedGirderData* FindGirder(GirderIDType gdrID) const override;
   virtual void SetGirder(const CGirderKey& girderKey,const CSplicedGirderData& splicedGirder) override;
   virtual const CPTData* GetPostTensioning(const CGirderKey& girderKey) const override;
   virtual void SetPostTensioning(const CGirderKey& girderKey,const CPTData& ptData) override;
   virtual const CPrecastSegmentData* GetPrecastSegmentData(const CSegmentKey& segmentKey) const override;
   virtual void SetPrecastSegmentData(const CSegmentKey& segmentKey,const CPrecastSegmentData& segment) override;
   virtual const CClosureJointData* GetClosureJointData(const CSegmentKey& closureKey) const override;
   virtual void SetClosureJointData(const CSegmentKey& closureKey,const CClosureJointData& closure) override;
   virtual void SetSpanLength(SpanIndexType spanIdx,Float64 newLength) override;
   virtual void MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption) override;
   virtual SupportIndexType MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation) override;
   virtual void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt) override;
   virtual void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml) override;
   virtual void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing2& spacing) override;
   virtual void SetGirderSpacingAtStartOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing) override;
   virtual void SetGirderSpacingAtEndOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing) override;
   virtual void SetGirderName(const CGirderKey& girderKey, LPCTSTR strGirderName) override;
   virtual void SetGirderGroup(GroupIndexType grpIdx,const CGirderGroupData& girderGroup) override;
   virtual void SetGirderCount(GroupIndexType grpIdx,GirderIndexType nGirders) override;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::BoundaryConditionType connectionType) override;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType connectionType,EventIndexType castClosureEventIdx) override;
   virtual void SetBoundaryCondition(SupportIndexType tsIdx, pgsTypes::TemporarySupportType supportType) override;
   virtual void SetBoundaryCondition(SupportIndexType tsIdx, pgsTypes::TempSupportSegmentConnectionType connectionType, EventIndexType castClosureEventIdx) override;
   virtual void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan) override;
   virtual void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace, Float64 spanLength, const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType eventIdx) override;
   virtual void InsertTemporarySupport(CTemporarySupportData* pTSData,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureJointEventIdx) override;
   virtual void DeleteTemporarySupportByIndex(SupportIndexType tsIdx) override;
   virtual void DeleteTemporarySupportByID(SupportIDType tsID) override;
   virtual void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method) override;
   virtual pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod() const override;
   virtual void UseSameNumberOfGirdersInAllGroups(bool bUseSame) override;
   virtual bool UseSameNumberOfGirdersInAllGroups() const override;
   virtual void SetGirderCount(GirderIndexType nGirders) override;
   virtual void UseSameGirderForEntireBridge(bool bSame) override;
   virtual bool UseSameGirderForEntireBridge() const override;
   virtual void SetGirderName(LPCTSTR strGirderName) override;
   virtual void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs) override;
   virtual pgsTypes::SupportedBeamSpacing GetGirderSpacingType() const override;
   virtual void SetGirderSpacing(Float64 spacing) override;
   virtual void SetMeasurementType(pgsTypes::MeasurementType mt) override;
   virtual pgsTypes::MeasurementType GetMeasurementType() const override;
   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml) override;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const override;
   virtual void SetWearingSurfaceType(pgsTypes::WearingSurfaceType wearingSurfaceType) override;
   virtual void SetHaunchInputDepthType(pgsTypes::HaunchInputDepthType type) override;
   virtual pgsTypes::HaunchInputDepthType GetHaunchInputDepthType() const override;
   virtual void SetSlabOffsetType(pgsTypes::SlabOffsetType offsetType) override;
   virtual void SetSlabOffset(Float64 slabOffset) override;
   virtual pgsTypes::SlabOffsetType GetSlabOffsetType() const override;
   virtual void SetSlabOffset(SupportIndexType supportIdx, pgsTypes::PierFaceType face, Float64 offset) override;
   virtual void SetSlabOffset(SupportIndexType supportIdx, Float64 backSlabOffset, Float64 aheadSlabOffset) override;
   virtual Float64 GetSlabOffset(SupportIndexType supportIdx, pgsTypes::PierFaceType face) const override;
   virtual void GetSlabOffset(SupportIndexType supportIdx, Float64* pBackSlabOffset, Float64* pAheadSlabOffset) const override;
   virtual void SetSlabOffset(const CSegmentKey& segmentKey, pgsTypes::MemberEndType end, Float64 offset) override;
   virtual void SetSlabOffset(const CSegmentKey& segmentKey, Float64 startSlabOffset, Float64 endSlabOffset) override;
   virtual Float64 GetSlabOffset(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const override;
   virtual void GetSlabOffset(const CSegmentKey& segmentKey, Float64* pStartSlabOffset, Float64* pEndSlabOffset) const override;
   virtual void SetFillet( Float64 Fillet) override;
   virtual Float64 GetFillet() const override;
   virtual void SetAssumedExcessCamberType(pgsTypes::AssumedExcessCamberType cType) override;
   virtual pgsTypes::AssumedExcessCamberType GetAssumedExcessCamberType() const override;
   virtual void SetAssumedExcessCamber( Float64 assumedExcessCamber) override;
   virtual void SetAssumedExcessCamber(SpanIndexType spanIdx, Float64 assumedExcessCamber) override;
   virtual void SetAssumedExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 assumedExcessCamber) override;
   virtual Float64 GetAssumedExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx) const override;
   virtual void SetHaunchInputLocationType(pgsTypes::HaunchInputLocationType type) override;
   virtual pgsTypes::HaunchInputLocationType GetHaunchInputLocationType() const override;
   virtual void SetHaunchLayoutType(pgsTypes::HaunchLayoutType type) override;
   virtual pgsTypes::HaunchLayoutType GetHaunchLayoutType() const override;
   virtual void SetHaunchInputDistributionType(pgsTypes::HaunchInputDistributionType type) override;
   virtual pgsTypes::HaunchInputDistributionType GetHaunchInputDistributionType() const override;
   virtual void SetDirectHaunchDepths4Bridge(const std::vector<Float64>& haunchDepths) override;
   virtual void SetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,const std::vector<Float64>& haunchDepths) override;
   virtual void SetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx,const std::vector<Float64>& haunchDepths) override;
   virtual void SetDirectHaunchDepthsPerSegment(GroupIndexType group,SegmentIndexType SegmentIdx,const std::vector<Float64>& haunchDepths) override;
   virtual void SetDirectHaunchDepthsPerSegment(GroupIndexType group,GirderIndexType gdrIdx,SegmentIndexType SegmentIdx,const std::vector<Float64>& haunchDepths) override;
   virtual std::vector<Float64> GetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx) override;
   virtual std::vector<Float64> GetDirectHaunchDepthsPerSegment(GroupIndexType group,GirderIndexType gdrIdx,SegmentIndexType SegmentIdx) override;
   virtual std::vector<pgsTypes::BoundaryConditionType> GetBoundaryConditionTypes(PierIndexType pierIdx) const override;
   virtual std::vector<pgsTypes::PierSegmentConnectionType> GetPierSegmentConnectionTypes(PierIndexType pierIdx) const override;
   virtual const CTimelineManager* GetTimelineManager() const override;
   virtual void SetTimelineManager(const CTimelineManager& timelineMbr) override;
   virtual EventIndexType AddTimelineEvent(const CTimelineEvent& timelineEvent) override;
   virtual EventIndexType GetEventCount() const override;
   virtual const CTimelineEvent* GetEventByIndex(EventIndexType eventIdx) const override;
   virtual const CTimelineEvent* GetEventByID(EventIDType eventID) const override;
   virtual void SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& stage) override;
   virtual void SetEventByID(EventIDType eventID,const CTimelineEvent& stage) override;
   virtual void SetSegmentConstructionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) override;
   virtual void SetSegmentConstructionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) override;
   virtual EventIndexType GetSegmentConstructionEventIndex(const CSegmentKey& segmentKey) const override;
   virtual EventIDType GetSegmentConstructionEventID(const CSegmentKey& segmentKey) const override;
   virtual EventIndexType GetPierErectionEvent(PierIndexType pierIdx) const override;
   virtual void SetPierErectionEventByIndex(PierIndexType pierIdx,EventIndexType eventIdx) override;
   virtual void SetPierErectionEventByID(PierIndexType pierIdx,IDType eventID) override;
   virtual void SetTempSupportEventsByIndex(SupportIndexType tsIdx,EventIndexType erectIdx,EventIndexType removeIdx) override;
   virtual void SetTempSupportEventsByID(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx) override;
   virtual void SetSegmentErectionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) override;
   virtual void SetSegmentErectionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) override;
   virtual EventIndexType GetSegmentErectionEventIndex(const CSegmentKey& segmentKey) const override;
   virtual EventIDType GetSegmentErectionEventID(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType constructionEventIdx,EventIndexType erectionEventIdx) override;
   virtual void SetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType constructionEventID,EventIDType erectionEventID) override;
   virtual void GetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType* constructionEventIdx,EventIndexType* erectionEventIdx) const override;
   virtual void GetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType* constructionEventID,EventIDType* erectionEventID) const override;
   virtual EventIndexType GetCastClosureJointEventIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx) const override;
   virtual EventIDType GetCastClosureJointEventID(GroupIndexType grpIdx,CollectionIndexType closureIdx) const override;
   virtual void SetCastClosureJointEventByIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIndexType eventIdx) override;
   virtual void SetCastClosureJointEventByID(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIDType eventID) override;
   virtual EventIndexType GetStressTendonEventIndex(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual EventIDType GetStressTendonEventID(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual void SetStressTendonEventByIndex(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIndexType eventIdx) override;
   virtual void SetStressTendonEventByID(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIDType eventID) override;
   virtual EventIndexType GetCastLongitudinalJointEventIndex() const override;
   virtual EventIDType GetCastLongitudinalJointEventID() const override;
   virtual int SetCastLongitudinalJointEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline) override;
   virtual int SetCastLongitudinalJointEventByID(EventIDType eventID, bool bAdjustTimeline) override;
   virtual EventIndexType GetCastDeckEventIndex() const override;
   virtual EventIDType GetCastDeckEventID() const override;
   virtual int SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline) override;
   virtual int SetCastDeckEventByID(EventIDType eventID,bool bAdjustTimeline) override;
   virtual EventIndexType GetIntermediateDiaphragmsLoadEventIndex() const override;
   virtual EventIDType GetIntermediateDiaphragmsLoadEventID() const override;
   virtual void SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID) override;
   virtual EventIndexType GetRailingSystemLoadEventIndex() const override;
   virtual EventIDType GetRailingSystemLoadEventID() const override;
   virtual void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetRailingSystemLoadEventByID(EventIDType eventID) override;
   virtual EventIndexType GetOverlayLoadEventIndex() const override;
   virtual EventIDType GetOverlayLoadEventID() const override;
   virtual void SetOverlayLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetOverlayLoadEventByID(EventIDType eventID) override;
   virtual EventIndexType GetLiveLoadEventIndex() const override;
   virtual EventIDType GetLiveLoadEventID() const override;
   virtual void SetLiveLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetLiveLoadEventByID(EventIDType eventID) override;
   virtual GroupIDType GetGroupID(GroupIndexType groupIdx) const override;
   virtual GirderIDType GetGirderID(const CGirderKey& girderKey) const override;
   virtual SegmentIDType GetSegmentID(const CSegmentKey& segmentKey) const override;
   virtual void SetBearingType(pgsTypes::BearingType offsetType) override;
   virtual pgsTypes::BearingType GetBearingType() const override;
   virtual void SetBearingData(const CBearingData2* pBearingData) override;
   virtual void SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, const CBearingData2* pBearingData) override;
   virtual void SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, GirderIndexType gdrIdx, const CBearingData2* pBearingData) override;
   virtual const CBearingData2* GetBearingData(PierIDType pierID, pgsTypes::PierFaceType face, GirderIndexType gdrIdx) const override;
   virtual void SetConnectionGeometry(PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                                      Float64 endDist, ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure,
                                      Float64 bearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingOffsetMeasurementType) override;
   virtual void GetConnectionGeometry(PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                                      Float64* endDist, ConnectionLibraryEntry::EndDistanceMeasurementType* endDistMeasure,
                                      Float64* bearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType* bearingOffsetMeasurementType) const override;
   virtual void SetPierDiaphragmData(PierIndexType pierIdx, pgsTypes::PierFaceType face,
                                    Float64 height, Float64 width, ConnectionLibraryEntry::DiaphragmLoadType loadType, Float64 loadLocation) override;
   virtual void GetPierDiaphragmData(PierIndexType pierIdx, pgsTypes::PierFaceType face,
                                    Float64* pHeight, Float64* pWidth, ConnectionLibraryEntry::DiaphragmLoadType* pLoadType, Float64* pLoadLocation) const override;

  virtual bool IsCompatibleGirder(const CGirderKey& girderKey, LPCTSTR lpszGirderName) const override;
   virtual bool AreGirdersCompatible(GroupIndexType groupIdx) const override;
   virtual bool AreGirdersCompatible(const std::vector<std::_tstring>& vGirderNames) const override;
   virtual bool AreGirdersCompatible(const CBridgeDescription2& bridgeDescription,const std::vector<std::_tstring>& vGirderNames) const override;

// ISegmentData 
public:
   virtual const WBFL::Materials::PsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const override;
   virtual void SetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const WBFL::Materials::PsStrand* pmat) override;

   virtual const CGirderMaterial* GetSegmentMaterial(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentMaterial(const CSegmentKey& segmentKey,const CGirderMaterial& material) override;

   virtual const CStrandData* GetStrandData(const CSegmentKey& segmentKey) const override;
   virtual void SetStrandData(const CSegmentKey& segmentKey,const CStrandData& strands) override;

   virtual const CSegmentPTData* GetSegmentPTData(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentPTData(const CSegmentKey& segmentKey,const CSegmentPTData& strands) override;

   virtual const CHandlingData* GetHandlingData(const CSegmentKey& segmentKey) const override;
   virtual void SetHandlingData(const CSegmentKey& segmentKey,const CHandlingData& handling) override;

// IShear
public:
   virtual std::_tstring GetSegmentStirrupMaterial(const CSegmentKey& segmentKey) const override;
   virtual void GetSegmentStirrupMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   virtual void SetSegmentStirrupMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   virtual const CShearData2* GetSegmentShearData(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentShearData(const CSegmentKey& segmentKey,const CShearData2& data) override;
   virtual std::_tstring GetClosureJointStirrupMaterial(const CClosureKey& closureKey) const override;
   virtual void GetClosureJointStirrupMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   virtual void SetClosureJointStirrupMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   virtual const CShearData2* GetClosureJointShearData(const CClosureKey& closureKey) const override;
   virtual void SetClosureJointShearData(const CClosureKey& closureKey,const CShearData2& data) override;

// ILongitudinalRebar
public:
   virtual std::_tstring GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey) const override;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   virtual void SetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   virtual const CLongitudinalRebarData* GetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey,const CLongitudinalRebarData& data) override;

   virtual std::_tstring GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey) const override;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   virtual void SetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   virtual const CLongitudinalRebarData* GetClosureJointLongitudinalRebarData(const CClosureKey& closureKey) const override;
   virtual void SetClosureJointLongitudinalRebarData(const CClosureKey& closureKey,const CLongitudinalRebarData& data) override;

// ISpecification
public:
   virtual std::_tstring GetSpecification() const override;
   virtual void SetSpecification(const std::_tstring& spec) override;
   virtual void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType) const override;
   virtual Uint16 GetMomentCapacityMethod() const override;
   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType) override;
   virtual pgsTypes::AnalysisType GetAnalysisType() const override;
   virtual std::vector<arDesignOptions> GetDesignOptions(const CGirderKey& girderKey) const override;
   virtual bool IsSlabOffsetDesignEnabled() const override;
   virtual pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() const override;
   virtual pgsTypes::HaunchLoadComputationType GetHaunchLoadComputationType() const override;
   virtual Float64 GetCamberTolerance() const override;
   virtual Float64 GetHaunchLoadCamberFactor() const override;
   virtual bool IsAssumedExcessCamberInputEnabled(bool considerDeckType=true) const override;
   virtual bool IsAssumedExcessCamberForLoad() const override; 
   virtual bool IsAssumedExcessCamberForSectProps() const override; 
   virtual void GetRequiredSlabOffsetRoundingParameters(pgsTypes::SlabOffsetRoundingMethod* pMethod, Float64* pTolerance) const override;
   virtual void GetTaperedSolePlateRequirements(bool* pbCheckTaperedSolePlate, Float64* pTaperedSolePlateThreshold) const override;
   virtual ISpecification::PrincipalWebStressCheckType GetPrincipalWebStressCheckType(const CSegmentKey& segmentKey) const override;
   virtual lrfdVersionMgr::Version GetSpecificationType() const override;

// IRatingSpecification
public:
   virtual bool IsRatingEnabled() const override;
   virtual bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType) const override;
   virtual void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable) override;
   virtual std::_tstring GetRatingSpecification() const override;
   virtual void SetADTT(Int16 adtt) override;
   virtual Int16 GetADTT() const override;
   virtual void SetRatingSpecification(const std::_tstring& spec) override;
   virtual void IncludePedestrianLiveLoad(bool bInclude) override;
   virtual bool IncludePedestrianLiveLoad() const override;
   virtual void SetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) override;
   virtual void GetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const override;
   virtual Float64 GetGirderConditionFactor(const CSegmentKey& segmentKey) const override;
   virtual void SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) override;
   virtual void GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const override;
   virtual Float64 GetDeckConditionFactor() const override;
   virtual void SetSystemFactorFlexure(Float64 sysFactor) override;
   virtual Float64 GetSystemFactorFlexure() const override;
   virtual void SetSystemFactorShear(Float64 sysFactor) override;
   virtual Float64 GetSystemFactorShear() const override;
   virtual void SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC) override;
   virtual Float64 GetDeadLoadFactor(pgsTypes::LimitState ls) const override;
   virtual void SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW) override;
   virtual Float64 GetWearingSurfaceFactor(pgsTypes::LimitState ls) const override;
   virtual void SetCreepFactor(pgsTypes::LimitState ls,Float64 gCR) override;
   virtual Float64 GetCreepFactor(pgsTypes::LimitState ls) const override;
   virtual void SetShrinkageFactor(pgsTypes::LimitState ls,Float64 gSH) override;
   virtual Float64 GetShrinkageFactor(pgsTypes::LimitState ls) const override;
   virtual void SetRelaxationFactor(pgsTypes::LimitState ls,Float64 gRE) override;
   virtual Float64 GetRelaxationFactor(pgsTypes::LimitState ls) const override;
   virtual void SetSecondaryEffectsFactor(pgsTypes::LimitState ls,Float64 gPS) override;
   virtual Float64 GetSecondaryEffectsFactor(pgsTypes::LimitState ls) const override;
   virtual void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL) override;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false) const override;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false) const override;
   virtual void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType, Float64 t, bool bLimitStress, Float64 fMax) override;
   virtual Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType, bool* pbLimitStress, Float64* pfMax) const override;
   virtual void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress) override;
   virtual bool RateForStress(pgsTypes::LoadRatingType ratingType) const override;
   virtual void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear) override;
   virtual bool RateForShear(pgsTypes::LoadRatingType ratingType) const override;
   virtual void ExcludeLegalLoadLaneLoading(bool bExclude) override;
   virtual bool ExcludeLegalLoadLaneLoading() const override;
   virtual void CheckYieldStress(pgsTypes::LoadRatingType ratingType,bool bCheckYieldStress) override;
   virtual bool CheckYieldStress(pgsTypes::LoadRatingType ratingType) const override;
   virtual void SetYieldStressLimitCoefficient(Float64 x) override;
   virtual Float64 GetYieldStressLimitCoefficient() const override;
   virtual void SetSpecialPermitType(pgsTypes::SpecialPermitType type) override;
   virtual pgsTypes::SpecialPermitType GetSpecialPermitType() const override;
   virtual Float64 GetStrengthLiveLoadFactor(pgsTypes::LoadRatingType ratingType,AxleConfiguration& axleConfig) const override;
   virtual Float64 GetServiceLiveLoadFactor(pgsTypes::LoadRatingType ratingType) const override;
   virtual Float64 GetReactionStrengthLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;
   virtual Float64 GetReactionServiceLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;

// ILibraryNames
public:
   virtual void EnumGdrConnectionNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumGirderNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumGirderNames( LPCTSTR strGirderFamily, std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumConcreteNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumDiaphragmNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumTrafficBarrierNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumSpecNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumRatingCriteriaNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumLiveLoadNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumDuctNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumHaulTruckNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumGirderFamilyNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void GetBeamFactory(const std::_tstring& strBeamFamily,const std::_tstring& strBeamName,IBeamFactory** ppFactory) override;

// ILibrary
public:
   virtual void SetLibraryManager(psgLibraryManager* pNewLibMgr) override;
   virtual psgLibraryManager* GetLibraryManager() override; 
   virtual const psgLibraryManager* GetLibraryManager() const override;
   virtual const ConnectionLibraryEntry* GetConnectionEntry(LPCTSTR lpszName ) const override;
   virtual const GirderLibraryEntry* GetGirderEntry( LPCTSTR lpszName ) const override;
   virtual const ConcreteLibraryEntry* GetConcreteEntry( LPCTSTR lpszName ) const override;
   virtual const DiaphragmLayoutEntry* GetDiaphragmEntry( LPCTSTR lpszName ) const override;
   virtual const TrafficBarrierEntry* GetTrafficBarrierEntry( LPCTSTR lpszName ) const override;
   virtual const SpecLibraryEntry* GetSpecEntry( LPCTSTR lpszName ) const override;
   virtual const LiveLoadLibraryEntry* GetLiveLoadEntry( LPCTSTR lpszName ) const override;
   virtual const DuctLibraryEntry* GetDuctEntry( LPCTSTR lpszName ) const override;
   virtual const HaulTruckLibraryEntry* GetHaulTruckEntry(LPCTSTR lpszName) const override;
   virtual ConcreteLibrary&        GetConcreteLibrary() override;
   virtual ConnectionLibrary&      GetConnectionLibrary() override;
   virtual GirderLibrary&          GetGirderLibrary() override;
   virtual DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary() override;
   virtual TrafficBarrierLibrary&  GetTrafficBarrierLibrary() override;
   virtual SpecLibrary*            GetSpecLibrary() override;
   virtual LiveLoadLibrary*        GetLiveLoadLibrary() override;
   virtual DuctLibrary*            GetDuctLibrary() override;
   virtual HaulTruckLibrary*       GetHaulTruckLibrary() override;
   virtual std::vector<libEntryUsageRecord> GetLibraryUsageRecords() const override;
   virtual void GetMasterLibraryInfo(std::_tstring& strServer, std::_tstring& strConfiguration, std::_tstring& strMasterLib, WBFL::System::Time& time) const override;
   virtual RatingLibrary* GetRatingLibrary() override;
   virtual const RatingLibrary* GetRatingLibrary() const override;
   virtual const RatingLibraryEntry* GetRatingEntry( LPCTSTR lpszName ) const override;

// ILoadModifiers
public:
   virtual void SetDuctilityFactor(Level level,Float64 value) override;
   virtual void SetImportanceFactor(Level level,Float64 value) override;
   virtual void SetRedundancyFactor(Level level,Float64 value) override;
   virtual Float64 GetDuctilityFactor() const override;
   virtual Float64 GetImportanceFactor() const override;
   virtual Float64 GetRedundancyFactor() const override;
   virtual Level GetDuctilityLevel() const override;
   virtual Level GetImportanceLevel() const override;
   virtual Level GetRedundancyLevel() const override;

// ISegmentLifting
public:
   virtual Float64 GetLeftLiftingLoopLocation(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetRightLiftingLoopLocation(const CSegmentKey& segmentKey) const override;
   virtual void SetLiftingLoopLocations(const CSegmentKey& segmentKey, Float64 left,Float64 right) override;

// ISegmentHauling
public:
   virtual Float64 GetLeadingOverhang(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetTrailingOverhang(const CSegmentKey& segmentKey) const override;
   virtual void SetTruckSupportLocations(const CSegmentKey& segmentKey, Float64 leftLoc,Float64 rightLoc) override;
   virtual LPCTSTR GetHaulTruck(const CSegmentKey& segmentKey) const override;
   virtual void SetHaulTruck(const CSegmentKey& segmentKey,LPCTSTR lpszHaulTruck) override;

// IImportProjectLibrary
public:
   virtual bool ImportProjectLibraries(IStructuredLoad* pLoad) override;

// IUserDefinedLoadData
public:
   virtual bool HasUserDC(const CGirderKey& girderKey) const override;
   virtual bool HasUserDW(const CGirderKey& girderKey) const override;
   virtual bool HasUserLLIM(const CGirderKey& girderKey) const override;
   virtual CollectionIndexType GetPointLoadCount() const override;
   virtual CollectionIndexType AddPointLoad(EventIDType eventID,const CPointLoadData& pld) override;
   virtual const CPointLoadData* GetPointLoad(CollectionIndexType idx) const override;
   virtual const CPointLoadData* FindPointLoad(LoadIDType loadID) const override;
   virtual EventIndexType GetPointLoadEventIndex(LoadIDType loadID) const override;
   virtual EventIDType GetPointLoadEventID(LoadIDType loadID) const override;
   virtual void UpdatePointLoad(CollectionIndexType idx, EventIDType eventID, const CPointLoadData& pld) override;
   virtual void UpdatePointLoadByID(LoadIDType loadID, EventIDType eventID, const CPointLoadData& pld) override;
   virtual void DeletePointLoad(CollectionIndexType idx) override;
   virtual void DeletePointLoadByID(LoadIDType loadID) override;
   virtual std::vector<CPointLoadData> GetPointLoads(const CSpanKey& spanKey) const override;

   virtual CollectionIndexType GetDistributedLoadCount() const override;
   virtual CollectionIndexType AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld) override;
   virtual const CDistributedLoadData* GetDistributedLoad(CollectionIndexType idx) const override;
   virtual const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const override;
   virtual EventIndexType GetDistributedLoadEventIndex(LoadIDType loadID) const override;
   virtual EventIDType GetDistributedLoadEventID(LoadIDType loadID) const override;
   virtual void UpdateDistributedLoad(CollectionIndexType idx, EventIDType eventID, const CDistributedLoadData& pld) override;
   virtual void UpdateDistributedLoadByID(LoadIDType loadID, EventIDType eventID, const CDistributedLoadData& pld) override;
   virtual void DeleteDistributedLoad(CollectionIndexType idx) override;
   virtual void DeleteDistributedLoadByID(LoadIDType loadID) override;
   virtual std::vector<CDistributedLoadData> GetDistributedLoads(const CSpanKey& spanKey) const override;

   virtual CollectionIndexType GetMomentLoadCount() const override;
   virtual CollectionIndexType AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld) override;
   virtual const CMomentLoadData* GetMomentLoad(CollectionIndexType idx) const override;
   virtual const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const override;
   virtual EventIndexType GetMomentLoadEventIndex(LoadIDType loadID) const override;
   virtual EventIDType GetMomentLoadEventID(LoadIDType loadID) const override;
   virtual void UpdateMomentLoad(CollectionIndexType idx, EventIDType eventID, const CMomentLoadData& pld) override;
   virtual void UpdateMomentLoadByID(LoadIDType loadID, EventIDType eventID, const CMomentLoadData& pld) override;
   virtual void DeleteMomentLoad(CollectionIndexType idx) override;
   virtual void DeleteMomentLoadByID(LoadIDType loadID) override;
   virtual std::vector<CMomentLoadData> GetMomentLoads(const CSpanKey& spanKey) const override;

   virtual void SetConstructionLoad(Float64 load) override;
   virtual Float64 GetConstructionLoad() const override;

// ILiveLoads
public:
   virtual bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType) const override;
   virtual PedestrianLoadApplicationType GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType) const override;
   virtual void SetPedestrianLoadApplication(pgsTypes::LiveLoadType llType, PedestrianLoadApplicationType PedLoad) override;
   virtual std::vector<std::_tstring> GetLiveLoadNames(pgsTypes::LiveLoadType llType) const override;
   virtual void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::_tstring>& names) override;
   virtual Float64 GetTruckImpact(pgsTypes::LiveLoadType llType) const override;
   virtual void SetTruckImpact(pgsTypes::LiveLoadType llType,Float64 impact) override;
   virtual Float64 GetLaneImpact(pgsTypes::LiveLoadType llType) const override;
   virtual void SetLaneImpact(pgsTypes::LiveLoadType llType,Float64 impact) override;
   virtual void SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action) override;
   virtual LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction() const override;
   virtual std::_tstring GetLLDFSpecialActionText() const override; // get common string for ignore roa case
   virtual bool IgnoreLLDFRangeOfApplicability() const override; // true if action is to ignore ROA

// IEvents
public:
   virtual void HoldEvents() override;
   virtual void FirePendingEvents() override;
   virtual void CancelPendingEvents() override;

// ILimits
public:
   virtual Float64 GetMaxSlabFc(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMaxSegmentFci(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMaxSegmentFc(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMaxClosureFci(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMaxClosureFc(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMaxConcreteAggSize(pgsTypes::ConcreteType concType) const override;

// ILoadFactors
public:
   const CLoadFactors* GetLoadFactors() const;
   void SetLoadFactors(const CLoadFactors& loadFactors) ;

// IEventMap
public:
   virtual CComBSTR GetEventName(EventIndexType eventIdx) const override;
   virtual EventIndexType GetEventIndex(CComBSTR bstrEvent) const override;

// IEffectiveFlangeWidth
public:
   virtual bool IgnoreEffectiveFlangeWidthLimits() const override;
   virtual void IgnoreEffectiveFlangeWidthLimits(bool bIgnore) override;

// ILossParameters
public:
   virtual std::_tstring GetLossMethodDescription() const override;
   virtual pgsTypes::LossMethod GetLossMethod() const override;
   virtual pgsTypes::TimeDependentModel GetTimeDependentModel() const override;
   virtual void IgnoreCreepEffects(bool bIgnore) override;
   virtual bool IgnoreCreepEffects() const override;
   virtual void IgnoreShrinkageEffects(bool bIgnore) override;
   virtual bool IgnoreShrinkageEffects()const override;
   virtual void IgnoreRelaxationEffects(bool bIgnore) override;
   virtual bool IgnoreRelaxationEffects() const override;
   virtual void IgnoreTimeDependentEffects(bool bIgnoreCreep,bool bIgnoreShrinkage,bool bIgnoreRelaxation) override;
   virtual void SetTendonPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) override;
   virtual void GetTendonPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const override;
   virtual void SetTemporaryStrandPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) override;
   virtual void GetTemporaryStrandPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const override;
   virtual void UseGeneralLumpSumLosses(bool bLumpSum) override;
   virtual bool UseGeneralLumpSumLosses() const override;
   virtual Float64 GetBeforeXferLosses() const override;
   virtual void SetBeforeXferLosses(Float64 loss) override;
   virtual Float64 GetAfterXferLosses() const override;
   virtual void SetAfterXferLosses(Float64 loss) override;
   virtual Float64 GetLiftingLosses() const override;
   virtual void SetLiftingLosses(Float64 loss) override;
   virtual Float64 GetShippingLosses() const override;
   virtual void SetShippingLosses(Float64 loss) override;
   virtual Float64 GetBeforeTempStrandRemovalLosses() const override;
   virtual void SetBeforeTempStrandRemovalLosses(Float64 loss) override;
   virtual Float64 GetAfterTempStrandRemovalLosses() const override;
   virtual void SetAfterTempStrandRemovalLosses(Float64 loss) override;
   virtual Float64 GetAfterDeckPlacementLosses() const override;
   virtual void SetAfterDeckPlacementLosses(Float64 loss) override;
   virtual Float64 GetAfterSIDLLosses() const override;
   virtual void SetAfterSIDLLosses(Float64 loss) override;
   virtual Float64 GetFinalLosses() const override;
   virtual void SetFinalLosses(Float64 loss) override;

// IValidate
public:
   virtual UINT Orientation(LPCTSTR lpszOrientation) override;

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

   // used for validating orientation strings
   CComPtr<IAngle> m_objAngle;
   CComPtr<IDirection> m_objDirection;

   std::_tstring m_BridgeName;
   std::_tstring m_BridgeID;
   std::_tstring m_JobNumber;
   std::_tstring m_Engineer;
   std::_tstring m_Company;
   std::_tstring m_Comments;

   pgsTypes::AnalysisType m_AnalysisType;
   bool m_bGetAnalysisTypeFromLibrary; // if true, we are reading old input... get the analysis type from the library entry

   mutable std::vector<std::_tstring> m_GirderFamilyNames;

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
   std::array<Float64, pgsTypes::LimitStateCount> m_gDC;
   std::array<Float64, pgsTypes::LimitStateCount> m_gDW;
   std::array<Float64, pgsTypes::LimitStateCount> m_gCR;
   std::array<Float64, pgsTypes::LimitStateCount> m_gSH;
   std::array<Float64, pgsTypes::LimitStateCount> m_gRE;
   std::array<Float64, pgsTypes::LimitStateCount> m_gPS;
   std::array<Float64, pgsTypes::LimitStateCount> m_gLL;

   struct TensionStressParams
   {
      Float64 coefficient;
      bool bLimitStress;
      Float64 fMax;
      TensionStressParams() {}
      TensionStressParams(Float64 a, bool b, Float64 c) : coefficient(a), bLimitStress(b), fMax(c) {}
      bool operator==(const TensionStressParams& other) const
      {
         if (!IsEqual(coefficient, other.coefficient) || bLimitStress != other.bLimitStress)
            return false;

         if (bLimitStress && !IsEqual(fMax, other.fMax))
            return false;

         return true;
      }
      bool operator!=(const TensionStressParams& other) const
      {
         return !operator==(other);
      }
   };
   std::array<TensionStressParams, pgsTypes::lrLoadRatingTypeCount> m_AllowableTensileStress; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bCheckYieldStress; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bRateForStress; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bRateForShear; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bEnableRating; // index is load rating type
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

   static HRESULT LoadOldSuperelevationData(bool bNewerFormat, IStructuredLoad* pLoad, CProjectAgentImp* pObj);

   // Bridge Description Data
   mutable CBridgeDescription2 m_BridgeDescription;

   mutable CLoadManager m_LoadManager;

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
      const LiveLoadLibraryEntry* pEntry; // nullptr if this is a system defined live load (HL-93)

      LiveLoadSelection() { pEntry = nullptr; }

      bool operator==(const LiveLoadSelection& other) const
      { return pEntry == other.pEntry; }

      bool operator<(const LiveLoadSelection& other) const
      { return EntryName < other.EntryName; }
   };

   typedef std::set<LiveLoadSelection> LiveLoadSelectionContainer;
   typedef LiveLoadSelectionContainer::iterator LiveLoadSelectionIterator;

   // index is pgsTypes::LiveLoadTypes constant
   std::array<LiveLoadSelectionContainer, pgsTypes::lltLiveLoadTypeCount> m_SelectedLiveLoads;
   std::array<Float64, pgsTypes::lltLiveLoadTypeCount> m_TruckImpact;
   std::array<Float64, pgsTypes::lltLiveLoadTypeCount> m_LaneImpact;
   std::array<PedestrianLoadApplicationType, 3> m_PedestrianLoadApplicationType; // lltDesign, lltPermit, lltFatigue only

   std::vector<std::_tstring> m_ReservedLiveLoads; // reserved live load names (names not found in library)
   bool IsReservedLiveLoad(const std::_tstring& strName) const;

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

   // if true, the time dependent effect is ignores in the time-step analysis
   bool m_bIgnoreCreepEffects;
   bool m_bIgnoreShrinkageEffects;
   bool m_bIgnoreRelaxationEffects;

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


   Float64 m_ConstructionLoad;

   HRESULT LoadPointLoads(IStructuredLoad* pLoad);
   HRESULT LoadDistributedLoads(IStructuredLoad* pLoad);
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
   Float64 GetMaxPjack(const CSegmentKey& segmentKey,StrandIndexType nStrands,const WBFL::Materials::PsStrand* pStrand) const;

   bool ResolveLibraryConflicts(const ConflictList& rList);
   void DealWithGirderLibraryChanges(bool fromLibrary);  // behavior is different if problem is caused by a library change
   
   void MoveBridge(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustNextSpan(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,Float64 newStation);

   void SpecificationRenamed();
   void SpecificationChanged(bool bFireEvent);
   void InitSpecification(const std::_tstring& spec);

   void RatingSpecificationChanged(bool bFireEvent);
   void InitRatingSpecification(const std::_tstring& spec);

   HRESULT FireContinuityRelatedSpanChange(const CSpanKey& spanKey,Uint32 lHint); 

   void UseBridgeLibraryEntries();
   void UseGirderLibraryEntries();
   void UseSegmentLibraryEntries(CPrecastSegmentData* pSegment);
   void UseDuctLibraryEntries();
   void UseDuctLibraryEntries(CPrecastSegmentData* pSegment);
   void ReleaseBridgeLibraryEntries();
   void ReleaseGirderLibraryEntries();
   void ReleaseSegmentLibraryEntries(CPrecastSegmentData* pSegment);
   void ReleaseDuctLibraryEntries();
   void ReleaseDuctLibraryEntries(CPrecastSegmentData* pSegment);

   void UpdateConcreteMaterial();
   void UpdateTimeDependentMaterials();
   void UpdateStrandMaterial();
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
   friend CProxyILossParametersEventSink<CProjectAgentImp>;

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

   // returns true of there are user loads of the specified type defined for
   // the specified girder
   bool HasUserLoad(const CGirderKey& girderKey,UserLoads::LoadCase lcType) const;

   bool m_bUpdateUserDefinedLoads; // if true, the user defined loads came from PGSuper 2.9.x or earlier and the timeline has not yet been updated

   void UpdateHaulTruck(const COldHaulTruck* pOldHaulTruck);
   const HaulTruckLibraryEntry* FindHaulTruckLibraryEntry(const COldHaulTruck* pOldHaulTruck);
   const HaulTruckLibraryEntry* FindHaulTruckLibraryEntry(Float64 kTheta,const COldHaulTruck* pOldHaulTruck);

   // Girder bearing data was added in 7/2017. Prior to that, support width was in the connection library and was used to compute face of support.
   // Need to upgrade information to new bearing data struct
   void UpgradeBearingData();
};

#endif //__PROJECTAGENT_H_

