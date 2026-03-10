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

#include <EAF\Agent.h>


#include <PsgLib\Keys.h>
#include <PsgLib\LoadFactors.h>
#include <PsgLib\ShearData.h>
#include <PsgLib\LongitudinalRebarData.h>

#include <PsgLib\BridgeDescription2.h>
#include <PsgLib\LoadManager.h>
#include <PsgLib\ClosureJointData.h>

#include "LibraryEntryObserver.h"

#include <IFace\GirderHandling.h>


class CStructuredLoad;
class ConflictList;
class CBridgeChangedHint;
class COldHaulTruck;


/////////////////////////////////////////////////////////////////////////////
// CProjectAgentImp
class CProjectAgentImp : public WBFL::EAF::Agent,
   public CProxyIProjectPropertiesEventSink<CProjectAgentImp>,
   public CProxyIEnvironmentEventSink<CProjectAgentImp>,
   public CProxyIBridgeDescriptionEventSink<CProjectAgentImp>,
   public CProxyISpecificationEventSink<CProjectAgentImp>,
   public CProxyIRatingSpecificationEventSink<CProjectAgentImp>,
   public CProxyILibraryConflictEventSink<CProjectAgentImp>,
   public CProxyILoadModifiersEventSink<CProjectAgentImp>,
   public CProxyILossParametersEventSink<CProjectAgentImp>,
   public CProxyIEventsEventSink<CProjectAgentImp>,
   public WBFL::EAF::IAgentPersist,
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

   StatusCallbackIDType m_scidBridgeDescriptionInfo;
   StatusCallbackIDType m_scidBridgeDescriptionWarning;
   StatusCallbackIDType m_scidBridgeDescriptionError;
   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidRebarStrengthWarning;
   StatusCallbackIDType m_scidLoadDescriptionWarning;
   StatusCallbackIDType m_scidConnectionGeometryWarning;

// IAgent
public:
   std::_tstring GetName() const override { return _T("ProjectAgent"); }
   bool RegisterInterfaces() override;
	bool Init() override;
	bool Reset() override;
	bool ShutDown() override;
   CLSID GetCLSID() const override;

// IAgentPersist
public:
	WBFL::EAF::Broker::LoadResult Load(IStructuredLoad* pStrLoad) override;
	bool Save(IStructuredSave* pStrSave) override;

// IProjectProperties
public:
   LPCTSTR GetBridgeName() const override;
   void SetBridgeName(LPCTSTR name) override;
   LPCTSTR GetBridgeID() const override;
   void SetBridgeID(LPCTSTR bid) override;
   LPCTSTR GetJobNumber() const override;
   void SetJobNumber(LPCTSTR jid) override;
   LPCTSTR GetEngineer() const override;
   void SetEngineer(LPCTSTR eng) override;
   LPCTSTR GetCompany() const override;
   void SetCompany(LPCTSTR company) override;
   LPCTSTR GetComments() const override;
   void SetComments(LPCTSTR comments) override;

// IEnvironment
public:
   pgsTypes::ExposureCondition GetExposureCondition() const override;
	void SetExposureCondition(pgsTypes::ExposureCondition newVal) override;

   pgsTypes::ClimateCondition GetClimateCondition() const override;
   void SetClimateCondition(pgsTypes::ClimateCondition newVal) override;

	Float64 GetRelHumidity() const override;
	void SetRelHumidity(Float64 newVal) override;

// IRoadwayData
public:
   void SetAlignmentData2(const AlignmentData2& data) override;
   const AlignmentData2& GetAlignmentData2() const override;
   void SetProfileData2(const ProfileData2& data) override;
   const ProfileData2& GetProfileData2() const override;
   void SetRoadwaySectionData(const RoadwaySectionData& data) override;
   const RoadwaySectionData& GetRoadwaySectionData() const override;

// IBridgeDescription
public:
   const CBridgeDescription2* GetBridgeDescription() const override;
   void SetBridgeDescription(const CBridgeDescription2& desc) override;
   const CDeckDescription2* GetDeckDescription() const override;
   void SetDeckDescription(const CDeckDescription2& deck) override;
   SpanIndexType GetSpanCount() const override;
   const CSpanData2* GetSpan(SpanIndexType spanIdx) const override;
   void SetSpan(SpanIndexType spanIdx,const CSpanData2& spanData) override;
   PierIndexType GetPierCount() const override;
   const CPierData2* GetPier(PierIndexType pierIdx) const override;
   const CPierData2* FindPier(PierIDType pierID) const override;
   void SetPierByIndex(PierIndexType pierIdx,const CPierData2& PierData) override;
   void SetPierByID(PierIDType pierID,const CPierData2& PierData) override;
   SupportIndexType GetTemporarySupportCount() const override;
   const CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx) const override;
   const CTemporarySupportData* FindTemporarySupport(SupportIDType tsID) const override;
   void SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData) override;
   void SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData) override;
   GroupIndexType GetGirderGroupCount() const override;
   const CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx) const override;
   const CSplicedGirderData* GetGirder(const CGirderKey& girderKey) const override;
   const CSplicedGirderData* FindGirder(GirderIDType gdrID) const override;
   void SetGirder(const CGirderKey& girderKey,const CSplicedGirderData& splicedGirder) override;
   const CPTData* GetPostTensioning(const CGirderKey& girderKey) const override;
   void SetPostTensioning(const CGirderKey& girderKey,const CPTData& ptData) override;
   const CPrecastSegmentData* GetPrecastSegmentData(const CSegmentKey& segmentKey) const override;
   void SetPrecastSegmentData(const CSegmentKey& segmentKey,const CPrecastSegmentData& segment) override;
   const CClosureJointData* GetClosureJointData(const CSegmentKey& closureKey) const override;
   void SetClosureJointData(const CSegmentKey& closureKey,const CClosureJointData& closure) override;
   void SetSpanLength(SpanIndexType spanIdx,Float64 newLength) override;
   void MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption) override;
   SupportIndexType MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation) override;
   void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt) override;
   void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml) override;
   void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing2& spacing) override;
   void SetGirderSpacingAtStartOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing) override;
   void SetGirderSpacingAtEndOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing) override;
   void SetGirderName(const CGirderKey& girderKey, LPCTSTR strGirderName) override;
   void SetGirderGroup(GroupIndexType grpIdx,const CGirderGroupData& girderGroup) override;
   void SetGirderCount(GroupIndexType grpIdx,GirderIndexType nGirders) override;
   void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::BoundaryConditionType connectionType) override;
   void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType connectionType,EventIndexType castClosureEventIdx) override;
   void SetBoundaryCondition(SupportIndexType tsIdx, pgsTypes::TemporarySupportType supportType) override;
   void SetBoundaryCondition(SupportIndexType tsIdx, pgsTypes::TempSupportSegmentConnectionType connectionType, EventIndexType castClosureEventIdx) override;
   void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan) override;
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace, Float64 spanLength, const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType eventIdx) override;
   void InsertTemporarySupport(CTemporarySupportData* pTSData,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureJointEventIdx) override;
   void DeleteTemporarySupportByIndex(SupportIndexType tsIdx) override;
   void DeleteTemporarySupportByID(SupportIDType tsID) override;
   void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method) override;
   pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod() const override;
   void UseSameNumberOfGirdersInAllGroups(bool bUseSame) override;
   bool UseSameNumberOfGirdersInAllGroups() const override;
   void SetGirderCount(GirderIndexType nGirders) override;
   void UseSameGirderForEntireBridge(bool bSame) override;
   bool UseSameGirderForEntireBridge() const override;
   void SetGirderName(LPCTSTR strGirderName) override;
   void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs) override;
   pgsTypes::SupportedBeamSpacing GetGirderSpacingType() const override;
   void SetGirderSpacing(Float64 spacing) override;
   void SetMeasurementType(pgsTypes::MeasurementType mt) override;
   pgsTypes::MeasurementType GetMeasurementType() const override;
   void SetMeasurementLocation(pgsTypes::MeasurementLocation ml) override;
   pgsTypes::MeasurementLocation GetMeasurementLocation() const override;
   void SetWearingSurfaceType(pgsTypes::WearingSurfaceType wearingSurfaceType) override;
   void SetHaunchInputDepthType(pgsTypes::HaunchInputDepthType type) override;
   pgsTypes::HaunchInputDepthType GetHaunchInputDepthType() const override;
   void SetSlabOffsetType(pgsTypes::SlabOffsetType offsetType) override;
   void SetSlabOffset(Float64 slabOffset) override;
   pgsTypes::SlabOffsetType GetSlabOffsetType() const override;
   void SetSlabOffset(SupportIndexType supportIdx, pgsTypes::PierFaceType face, Float64 offset) override;
   void SetSlabOffset(SupportIndexType supportIdx, Float64 backSlabOffset, Float64 aheadSlabOffset) override;
   Float64 GetSlabOffset(SupportIndexType supportIdx, pgsTypes::PierFaceType face) const override;
   void GetSlabOffset(SupportIndexType supportIdx, Float64* pBackSlabOffset, Float64* pAheadSlabOffset) const override;
   void SetSlabOffset(const CSegmentKey& segmentKey, pgsTypes::MemberEndType end, Float64 offset) override;
   void SetSlabOffset(const CSegmentKey& segmentKey, Float64 startSlabOffset, Float64 endSlabOffset) override;
   Float64 GetSlabOffset(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const override;
   void GetSlabOffset(const CSegmentKey& segmentKey, Float64* pStartSlabOffset, Float64* pEndSlabOffset) const override;
   void SetFillet( Float64 Fillet) override;
   Float64 GetFillet() const override;
   void SetAssumedExcessCamberType(pgsTypes::AssumedExcessCamberType cType) override;
   pgsTypes::AssumedExcessCamberType GetAssumedExcessCamberType() const override;
   void SetAssumedExcessCamber( Float64 assumedExcessCamber) override;
   void SetAssumedExcessCamber(SpanIndexType spanIdx, Float64 assumedExcessCamber) override;
   void SetAssumedExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 assumedExcessCamber) override;
   Float64 GetAssumedExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx) const override;
   void SetHaunchInputLocationType(pgsTypes::HaunchInputLocationType type) override;
   pgsTypes::HaunchInputLocationType GetHaunchInputLocationType() const override;
   void SetHaunchLayoutType(pgsTypes::HaunchLayoutType type) override;
   pgsTypes::HaunchLayoutType GetHaunchLayoutType() const override;
   void SetHaunchInputDistributionType(pgsTypes::HaunchInputDistributionType type) override;
   pgsTypes::HaunchInputDistributionType GetHaunchInputDistributionType() const override;
   void SetDirectHaunchDepths4Bridge(const std::vector<Float64>& haunchDepths) override;
   void SetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,const std::vector<Float64>& haunchDepths) override;
   void SetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx,const std::vector<Float64>& haunchDepths) override;
   void SetDirectHaunchDepthsPerSegment(GroupIndexType group,SegmentIndexType SegmentIdx,const std::vector<Float64>& haunchDepths) override;
   void SetDirectHaunchDepthsPerSegment(GroupIndexType group,GirderIndexType gdrIdx,SegmentIndexType SegmentIdx,const std::vector<Float64>& haunchDepths) override;
   std::vector<Float64> GetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx) override;
   std::vector<Float64> GetDirectHaunchDepthsPerSegment(GroupIndexType group,GirderIndexType gdrIdx,SegmentIndexType SegmentIdx) override;
   std::pair<bool,CBridgeDescription2> ConvertHaunchToSlabOffsetInput(const CBridgeDescription2& bridgeDesc, pgsTypes::SlabOffsetType) const override;
   std::pair<bool,CBridgeDescription2> ConvertHaunchToDirectHaunchInput(const CBridgeDescription2& bridgeDesc, pgsTypes::HaunchInputLocationType newHaunchInputLocationType, pgsTypes::HaunchLayoutType newHaunchLayoutType, pgsTypes::HaunchInputDistributionType newHaunchInputDistributionType, bool forceInit = false) const override;
   std::pair<bool,CBridgeDescription2> DesignHaunches(const CBridgeDescription2& bridgeDesc, const CGirderKey& rDesignGirderKey, GirderIndexType sourceGirderIdx, pgsTypes::HaunchInputDistributionType inputDistributionType, bool bApply2AllGdrs) const override;

   std::vector<pgsTypes::BoundaryConditionType> GetBoundaryConditionTypes(PierIndexType pierIdx) const override;
   std::vector<pgsTypes::PierSegmentConnectionType> GetPierSegmentConnectionTypes(PierIndexType pierIdx) const override;
   const CTimelineManager* GetTimelineManager() const override;
   void SetTimelineManager(const CTimelineManager& timelineMbr) override;
   EventIndexType AddTimelineEvent(const CTimelineEvent& timelineEvent) override;
   EventIndexType GetEventCount() const override;
   const CTimelineEvent* GetEventByIndex(EventIndexType eventIdx) const override;
   const CTimelineEvent* GetEventByID(EventIDType eventID) const override;
   void SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& stage) override;
   void SetEventByID(EventIDType eventID,const CTimelineEvent& stage) override;
   void SetSegmentConstructionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) override;
   void SetSegmentConstructionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) override;
   EventIndexType GetSegmentConstructionEventIndex(const CSegmentKey& segmentKey) const override;
   EventIDType GetSegmentConstructionEventID(const CSegmentKey& segmentKey) const override;
   EventIndexType GetPierErectionEvent(PierIndexType pierIdx) const override;
   void SetPierErectionEventByIndex(PierIndexType pierIdx,EventIndexType eventIdx) override;
   void SetPierErectionEventByID(PierIndexType pierIdx,IDType eventID) override;
   void SetTempSupportEventsByIndex(SupportIndexType tsIdx,EventIndexType erectIdx,EventIndexType removeIdx) override;
   void SetTempSupportEventsByID(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx) override;
   void SetSegmentErectionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) override;
   void SetSegmentErectionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) override;
   EventIndexType GetSegmentErectionEventIndex(const CSegmentKey& segmentKey) const override;
   EventIDType GetSegmentErectionEventID(const CSegmentKey& segmentKey) const override;
   void SetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType constructionEventIdx,EventIndexType erectionEventIdx) override;
   void SetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType constructionEventID,EventIDType erectionEventID) override;
   void GetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType* constructionEventIdx,EventIndexType* erectionEventIdx) const override;
   void GetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType* constructionEventID,EventIDType* erectionEventID) const override;
   EventIndexType GetCastClosureJointEventIndex(GroupIndexType grpIdx,IndexType closureIdx) const override;
   EventIDType GetCastClosureJointEventID(GroupIndexType grpIdx,IndexType closureIdx) const override;
   void SetCastClosureJointEventByIndex(GroupIndexType grpIdx,IndexType closureIdx,EventIndexType eventIdx) override;
   void SetCastClosureJointEventByID(GroupIndexType grpIdx,IndexType closureIdx,EventIDType eventID) override;
   EventIndexType GetStressTendonEventIndex(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   EventIDType GetStressTendonEventID(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   void SetStressTendonEventByIndex(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIndexType eventIdx) override;
   void SetStressTendonEventByID(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIDType eventID) override;
   EventIndexType GetCastLongitudinalJointEventIndex() const override;
   EventIDType GetCastLongitudinalJointEventID() const override;
   int SetCastLongitudinalJointEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline) override;
   int SetCastLongitudinalJointEventByID(EventIDType eventID, bool bAdjustTimeline) override;
   EventIndexType GetCastDeckEventIndex() const override;
   EventIDType GetCastDeckEventID() const override;
   int SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline) override;
   int SetCastDeckEventByID(EventIDType eventID,bool bAdjustTimeline) override;
   EventIndexType GetIntermediateDiaphragmsLoadEventIndex() const override;
   EventIDType GetIntermediateDiaphragmsLoadEventID() const override;
   void SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx) override;
   void SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID) override;
   EventIndexType GetRailingSystemLoadEventIndex() const override;
   EventIDType GetRailingSystemLoadEventID() const override;
   void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx) override;
   void SetRailingSystemLoadEventByID(EventIDType eventID) override;
   EventIndexType GetOverlayLoadEventIndex() const override;
   EventIDType GetOverlayLoadEventID() const override;
   void SetOverlayLoadEventByIndex(EventIndexType eventIdx) override;
   void SetOverlayLoadEventByID(EventIDType eventID) override;
   EventIndexType GetLiveLoadEventIndex() const override;
   EventIDType GetLiveLoadEventID() const override;
   void SetLiveLoadEventByIndex(EventIndexType eventIdx) override;
   void SetLiveLoadEventByID(EventIDType eventID) override;
   GroupIDType GetGroupID(GroupIndexType groupIdx) const override;
   GirderIDType GetGirderID(const CGirderKey& girderKey) const override;
   SegmentIDType GetSegmentID(const CSegmentKey& segmentKey) const override;
   void SetBearingType(pgsTypes::BearingType offsetType) override;
   pgsTypes::BearingType GetBearingType() const override;
   void SetBearingData(const CBearingData2* pBearingData) override;
   void SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, const CBearingData2* pBearingData) override;
   void SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, GirderIndexType gdrIdx, const CBearingData2* pBearingData) override;
   const CBearingData2* GetBearingData(PierIDType pierID, pgsTypes::PierFaceType face, GirderIndexType gdrIdx) const override;
   void SetConnectionGeometry(PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                              Float64 endDist, ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure,
                              Float64 bearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingOffsetMeasurementType) override;
   void GetConnectionGeometry(PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                              Float64* endDist, ConnectionLibraryEntry::EndDistanceMeasurementType* endDistMeasure,
                              Float64* bearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType* bearingOffsetMeasurementType) const override;
   void SetPierDiaphragmData(PierIndexType pierIdx, pgsTypes::PierFaceType face,
                            Float64 height, Float64 width, ConnectionLibraryEntry::DiaphragmLoadType loadType, Float64 loadLocation) override;
   void GetPierDiaphragmData(PierIndexType pierIdx, pgsTypes::PierFaceType face,
                            Float64* pHeight, Float64* pWidth, ConnectionLibraryEntry::DiaphragmLoadType* pLoadType, Float64* pLoadLocation) const override;

   bool IsCompatibleGirder(const CGirderKey& girderKey, LPCTSTR lpszGirderName) const override;
   bool AreGirdersCompatible(GroupIndexType groupIdx) const override;
   bool AreGirdersCompatible(const std::vector<std::_tstring>& vGirderNames) const override;
   bool AreGirdersCompatible(const CBridgeDescription2& bridgeDescription,const std::vector<std::_tstring>& vGirderNames) const override;

// ISegmentData 
public:
   const WBFL::Materials::PsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const override;
   void SetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const WBFL::Materials::PsStrand* pmat) override;

   const CGirderMaterial* GetSegmentMaterial(const CSegmentKey& segmentKey) const override;
   void SetSegmentMaterial(const CSegmentKey& segmentKey,const CGirderMaterial& material) override;

   const CStrandData* GetStrandData(const CSegmentKey& segmentKey) const override;
   void SetStrandData(const CSegmentKey& segmentKey,const CStrandData& strands) override;

   const CSegmentPTData* GetSegmentPTData(const CSegmentKey& segmentKey) const override;
   void SetSegmentPTData(const CSegmentKey& segmentKey,const CSegmentPTData& strands) override;

   const CHandlingData* GetHandlingData(const CSegmentKey& segmentKey) const override;
   void SetHandlingData(const CSegmentKey& segmentKey,const CHandlingData& handling) override;

// IShear
public:
   std::_tstring GetSegmentStirrupMaterial(const CSegmentKey& segmentKey) const override;
   void GetSegmentStirrupMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   void SetSegmentStirrupMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   const CShearData2* GetSegmentShearData(const CSegmentKey& segmentKey) const override;
   void SetSegmentShearData(const CSegmentKey& segmentKey,const CShearData2& data) override;
   std::_tstring GetClosureJointStirrupMaterial(const CClosureKey& closureKey) const override;
   void GetClosureJointStirrupMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   void SetClosureJointStirrupMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   const CShearData2* GetClosureJointShearData(const CClosureKey& closureKey) const override;
   void SetClosureJointShearData(const CClosureKey& closureKey,const CShearData2& data) override;
   const CShearData2* GetSegmentShearLibraryData(const CSegmentKey& segmentKey, bool& bIsEqualityWarning) const override;

// ILongitudinalRebar
public:
   std::_tstring GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey) const override;
   void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   void SetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   const CLongitudinalRebarData* GetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey) const override;
   void SetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey,const CLongitudinalRebarData& data) override;

   std::_tstring GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey) const override;
   void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const override;
   void SetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) override;
   const CLongitudinalRebarData* GetClosureJointLongitudinalRebarData(const CClosureKey& closureKey) const override;
   void SetClosureJointLongitudinalRebarData(const CClosureKey& closureKey,const CLongitudinalRebarData& data) override;

// ISpecification
public:
   std::_tstring GetSpecification() const override;
   void SetSpecification(const std::_tstring& spec) override;
   void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType) const override;
   Uint16 GetMomentCapacityMethod() const override;
   void SetAnalysisType(pgsTypes::AnalysisType analysisType) override;
   pgsTypes::AnalysisType GetAnalysisType() const override;
   std::vector<arDesignOptions> GetDesignOptions(const CGirderKey& girderKey) const override;
   const SlabOffsetCriteria& GetSlabOffsetCriteria() const override;
   bool DesignSlabHaunch() const override;
   const HaulingCriteria& GetHaulingCriteria() const override;
   pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() const override;
   pgsTypes::HaunchLoadComputationType GetHaunchLoadComputationType() const override;
   Float64 GetCamberTolerance() const override;
   Float64 GetHaunchLoadCamberFactor() const override;
   bool IsAssumedExcessCamberInputEnabled(bool considerDeckType=true) const override;
   bool IsAssumedExcessCamberForLoad() const override; 
   bool IsAssumedExcessCamberForSectProps() const override; 
   void GetTaperedSolePlateRequirements(bool* pbCheckTaperedSolePlate, Float64* pTaperedSolePlateThreshold) const override;
   void GetBearingCheckRequirements() const override;
   ISpecification::PrincipalWebStressCheckType GetPrincipalWebStressCheckType(const CSegmentKey& segmentKey) const override;
   bool ConsiderReinforcementDevelopmentLocationFactor() const override;
   WBFL::LRFD::BDSManager::Edition GetSpecificationType() const override;

// IRatingSpecification
public:
   bool IsRatingEnabled() const override;
   bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType) const override;
   void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable) override;
   std::_tstring GetRatingSpecification() const override;
   void SetADTT(Int16 adtt) override;
   Int16 GetADTT() const override;
   void SetRatingSpecification(const std::_tstring& spec) override;
   void IncludePedestrianLiveLoad(bool bInclude) override;
   bool IncludePedestrianLiveLoad() const override;
   void SetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) override;
   void GetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const override;
   Float64 GetGirderConditionFactor(const CSegmentKey& segmentKey) const override;
   void SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) override;
   void GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const override;
   Float64 GetDeckConditionFactor() const override;
   void SetSystemFactorFlexure(Float64 sysFactor) override;
   Float64 GetSystemFactorFlexure() const override;
   void SetSystemFactorShear(Float64 sysFactor) override;
   Float64 GetSystemFactorShear() const override;
   void SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC) override;
   Float64 GetDeadLoadFactor(pgsTypes::LimitState ls) const override;
   void SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW) override;
   Float64 GetWearingSurfaceFactor(pgsTypes::LimitState ls) const override;
   void SetCreepFactor(pgsTypes::LimitState ls,Float64 gCR) override;
   Float64 GetCreepFactor(pgsTypes::LimitState ls) const override;
   void SetShrinkageFactor(pgsTypes::LimitState ls,Float64 gSH) override;
   Float64 GetShrinkageFactor(pgsTypes::LimitState ls) const override;
   void SetRelaxationFactor(pgsTypes::LimitState ls,Float64 gRE) override;
   Float64 GetRelaxationFactor(pgsTypes::LimitState ls) const override;
   void SetSecondaryEffectsFactor(pgsTypes::LimitState ls,Float64 gPS) override;
   Float64 GetSecondaryEffectsFactor(pgsTypes::LimitState ls) const override;
   void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL) override;
   Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false) const override;
   Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false) const override;
   void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType, Float64 t, bool bLimitStress, Float64 fMax) override;
   Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType, bool* pbLimitStress, Float64* pfMax) const override;
   void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress) override;
   bool RateForStress(pgsTypes::LoadRatingType ratingType) const override;
   void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear) override;
   bool RateForShear(pgsTypes::LoadRatingType ratingType) const override;
   void ExcludeLegalLoadLaneLoading(bool bExclude) override;
   bool ExcludeLegalLoadLaneLoading() const override;
   void CheckYieldStress(pgsTypes::LoadRatingType ratingType,bool bCheckYieldStress) override;
   bool CheckYieldStress(pgsTypes::LoadRatingType ratingType) const override;
   void SetYieldStressLimitCoefficient(Float64 x) override;
   Float64 GetYieldStressLimitCoefficient() const override;
   void SetSpecialPermitType(pgsTypes::SpecialPermitType type) override;
   pgsTypes::SpecialPermitType GetSpecialPermitType() const override;
   Float64 GetStrengthLiveLoadFactor(pgsTypes::LoadRatingType ratingType,AxleConfiguration& axleConfig) const override;
   Float64 GetServiceLiveLoadFactor(pgsTypes::LoadRatingType ratingType) const override;
   Float64 GetReactionStrengthLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;
   Float64 GetReactionServiceLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;

// ILibraryNames
public:
   void EnumGdrConnectionNames( std::vector<std::_tstring>* pNames ) const override;
   void EnumGirderNames( std::vector<std::_tstring>* pNames ) const override;
   void EnumGirderNames( LPCTSTR strGirderFamily, std::vector<std::_tstring>* pNames ) const override;
   void EnumConcreteNames( std::vector<std::_tstring>* pNames ) const override;
   void EnumDiaphragmNames( std::vector<std::_tstring>* pNames ) const override;
   void EnumTrafficBarrierNames( std::vector<std::_tstring>* pNames ) const override;
   void EnumSpecNames( std::vector<std::_tstring>* pNames) const override;
   void EnumRatingCriteriaNames( std::vector<std::_tstring>* pNames) const override;
   void EnumLiveLoadNames( std::vector<std::_tstring>* pNames) const override;
   void EnumDuctNames( std::vector<std::_tstring>* pNames ) const override;
   void EnumHaulTruckNames( std::vector<std::_tstring>* pNames) const override;
   void EnumGirderFamilyNames( std::vector<std::_tstring>* pNames ) const override;
   std::shared_ptr<PGS::Beams::BeamFactory> GetBeamFactory(const std::_tstring& strBeamFamily,const std::_tstring& strBeamName) override;

// ILibrary
public:
   void SetLibraryManager(psgLibraryManager* pNewLibMgr) override;
   psgLibraryManager* GetLibraryManager() override; 
   const psgLibraryManager* GetLibraryManager() const override;
   const ConnectionLibraryEntry* GetConnectionEntry(LPCTSTR lpszName ) const override;
   const GirderLibraryEntry* GetGirderEntry( LPCTSTR lpszName ) const override;
   const ConcreteLibraryEntry* GetConcreteEntry( LPCTSTR lpszName ) const override;
   const DiaphragmLayoutEntry* GetDiaphragmEntry( LPCTSTR lpszName ) const override;
   const TrafficBarrierEntry* GetTrafficBarrierEntry( LPCTSTR lpszName ) const override;
   const SpecLibraryEntry* GetSpecEntry( LPCTSTR lpszName ) const override;
   const LiveLoadLibraryEntry* GetLiveLoadEntry( LPCTSTR lpszName ) const override;
   const DuctLibraryEntry* GetDuctEntry( LPCTSTR lpszName ) const override;
   const HaulTruckLibraryEntry* GetHaulTruckEntry(LPCTSTR lpszName) const override;
   ConcreteLibrary&        GetConcreteLibrary() override;
   ConnectionLibrary&      GetConnectionLibrary() override;
   GirderLibrary&          GetGirderLibrary() override;
   DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary() override;
   TrafficBarrierLibrary&  GetTrafficBarrierLibrary() override;
   SpecLibrary*            GetSpecLibrary() override;
   LiveLoadLibrary*        GetLiveLoadLibrary() override;
   DuctLibrary*            GetDuctLibrary() override;
   HaulTruckLibrary*       GetHaulTruckLibrary() override;
   std::vector<WBFL::Library::EntryUsageRecord> GetLibraryUsageRecords() const override;
   void GetMasterLibraryInfo(std::_tstring& strServer, std::_tstring& strConfiguration, std::_tstring& strMasterLib, WBFL::System::Time& time) const override;
   RatingLibrary* GetRatingLibrary() override;
   const RatingLibrary* GetRatingLibrary() const override;
   const RatingLibraryEntry* GetRatingEntry( LPCTSTR lpszName ) const override;

// ILoadModifiers
public:
   void SetDuctilityFactor(Level level,Float64 value) override;
   void SetImportanceFactor(Level level,Float64 value) override;
   void SetRedundancyFactor(Level level,Float64 value) override;
   Float64 GetDuctilityFactor() const override;
   Float64 GetImportanceFactor() const override;
   Float64 GetRedundancyFactor() const override;
   Level GetDuctilityLevel() const override;
   Level GetImportanceLevel() const override;
   Level GetRedundancyLevel() const override;

// ISegmentLifting
public:
   Float64 GetLeftLiftingLoopLocation(const CSegmentKey& segmentKey) const override;
   Float64 GetRightLiftingLoopLocation(const CSegmentKey& segmentKey) const override;
   void SetLiftingLoopLocations(const CSegmentKey& segmentKey, Float64 left,Float64 right) override;

// ISegmentHauling
public:
   Float64 GetLeadingOverhang(const CSegmentKey& segmentKey) const override;
   Float64 GetTrailingOverhang(const CSegmentKey& segmentKey) const override;
   void SetTruckSupportLocations(const CSegmentKey& segmentKey, Float64 leftLoc,Float64 rightLoc) override;
   LPCTSTR GetHaulTruck(const CSegmentKey& segmentKey) const override;
   void SetHaulTruck(const CSegmentKey& segmentKey,LPCTSTR lpszHaulTruck) override;

// IImportProjectLibrary
public:
   bool ImportProjectLibraries(IStructuredLoad* pLoad) override;

// IUserDefinedLoadData
public:
   bool HasUserDC(const CGirderKey& girderKey) const override;
   bool HasUserDW(const CGirderKey& girderKey) const override;
   bool HasUserLLIM(const CGirderKey& girderKey) const override;
   IndexType GetPointLoadCount() const override;
   IndexType AddPointLoad(EventIDType eventID,const CPointLoadData& pld) override;
   const CPointLoadData* GetPointLoad(IndexType idx) const override;
   const CPointLoadData* FindPointLoad(LoadIDType loadID) const override;
   EventIndexType GetPointLoadEventIndex(LoadIDType loadID) const override;
   EventIDType GetPointLoadEventID(LoadIDType loadID) const override;
   void UpdatePointLoad(IndexType idx, EventIDType eventID, const CPointLoadData& pld) override;
   void UpdatePointLoadByID(LoadIDType loadID, EventIDType eventID, const CPointLoadData& pld) override;
   void DeletePointLoad(IndexType idx) override;
   void DeletePointLoadByID(LoadIDType loadID) override;
   std::vector<CPointLoadData> GetPointLoads(const CSpanKey& spanKey) const override;

   IndexType GetDistributedLoadCount() const override;
   IndexType AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld) override;
   const CDistributedLoadData* GetDistributedLoad(IndexType idx) const override;
   const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const override;
   EventIndexType GetDistributedLoadEventIndex(LoadIDType loadID) const override;
   EventIDType GetDistributedLoadEventID(LoadIDType loadID) const override;
   void UpdateDistributedLoad(IndexType idx, EventIDType eventID, const CDistributedLoadData& pld) override;
   void UpdateDistributedLoadByID(LoadIDType loadID, EventIDType eventID, const CDistributedLoadData& pld) override;
   void DeleteDistributedLoad(IndexType idx) override;
   void DeleteDistributedLoadByID(LoadIDType loadID) override;
   std::vector<CDistributedLoadData> GetDistributedLoads(const CSpanKey& spanKey) const override;

   IndexType GetMomentLoadCount() const override;
   IndexType AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld) override;
   const CMomentLoadData* GetMomentLoad(IndexType idx) const override;
   const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const override;
   EventIndexType GetMomentLoadEventIndex(LoadIDType loadID) const override;
   EventIDType GetMomentLoadEventID(LoadIDType loadID) const override;
   void UpdateMomentLoad(IndexType idx, EventIDType eventID, const CMomentLoadData& pld) override;
   void UpdateMomentLoadByID(LoadIDType loadID, EventIDType eventID, const CMomentLoadData& pld) override;
   void DeleteMomentLoad(IndexType idx) override;
   void DeleteMomentLoadByID(LoadIDType loadID) override;
   std::vector<CMomentLoadData> GetMomentLoads(const CSpanKey& spanKey) const override;

   void SetConstructionLoad(Float64 load) override;
   Float64 GetConstructionLoad() const override;

// ILiveLoads
public:
   bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType) const override;
   PedestrianLoadApplicationType GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType) const override;
   void SetPedestrianLoadApplication(pgsTypes::LiveLoadType llType, PedestrianLoadApplicationType PedLoad) override;
   std::vector<std::_tstring> GetLiveLoadNames(pgsTypes::LiveLoadType llType) const override;
   void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::_tstring>& names) override;
   Float64 GetTruckImpact(pgsTypes::LiveLoadType llType) const override;
   void SetTruckImpact(pgsTypes::LiveLoadType llType,Float64 impact) override;
   Float64 GetLaneImpact(pgsTypes::LiveLoadType llType) const override;
   void SetLaneImpact(pgsTypes::LiveLoadType llType,Float64 impact) override;
   void SetRangeOfApplicabilityAction(WBFL::LRFD::RangeOfApplicabilityAction action) override;
   WBFL::LRFD::RangeOfApplicabilityAction GetRangeOfApplicabilityAction() const override;
   std::_tstring GetLLDFSpecialActionText() const override; // get common string for ignore roa case

// IEvents
public:
   void HoldEvents() override;
   void FirePendingEvents() override;
   void CancelPendingEvents() override;

// ILimits
public:
   Float64 GetMaxSlabFc(pgsTypes::ConcreteType concType) const override;
   Float64 GetMaxSegmentFci(pgsTypes::ConcreteType concType) const override;
   Float64 GetMaxSegmentFc(pgsTypes::ConcreteType concType) const override;
   Float64 GetMaxClosureFci(pgsTypes::ConcreteType concType) const override;
   Float64 GetMaxClosureFc(pgsTypes::ConcreteType concType) const override;
   Float64 GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType) const override;
   Float64 GetMaxConcreteAggSize(pgsTypes::ConcreteType concType) const override;

// ILoadFactors
public:
   const CLoadFactors* GetLoadFactors() const override;
   void SetLoadFactors(const CLoadFactors& loadFactors) override;

// IEventMap
public:
   CComBSTR GetEventName(EventIndexType eventIdx) const override;
   EventIndexType GetEventIndex(CComBSTR bstrEvent) const override;

// IEffectiveFlangeWidth
public:
   bool IgnoreEffectiveFlangeWidthLimits() const override;
   void IgnoreEffectiveFlangeWidthLimits(bool bIgnore) override;

// ILossParameters
public:
   std::_tstring GetLossMethodDescription() const override;
   PrestressLossCriteria::LossMethodType GetLossMethod() const override;
   PrestressLossCriteria::TimeDependentConcreteModelType GetTimeDependentModel() const override;
   void IgnoreCreepEffects(bool bIgnore) override;
   bool IgnoreCreepEffects() const override;
   void IgnoreShrinkageEffects(bool bIgnore) override;
   bool IgnoreShrinkageEffects()const override;
   void IgnoreRelaxationEffects(bool bIgnore) override;
   bool IgnoreRelaxationEffects() const override;
   void IgnoreTimeDependentEffects(bool bIgnoreCreep,bool bIgnoreShrinkage,bool bIgnoreRelaxation) override;
   void SetTendonPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) override;
   void GetTendonPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const override;
   void SetTemporaryStrandPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) override;
   void GetTemporaryStrandPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const override;
   void UseGeneralLumpSumLosses(bool bLumpSum) override;
   bool UseGeneralLumpSumLosses() const override;
   Float64 GetBeforeXferLosses() const override;
   void SetBeforeXferLosses(Float64 loss) override;
   Float64 GetAfterXferLosses() const override;
   void SetAfterXferLosses(Float64 loss) override;
   Float64 GetLiftingLosses() const override;
   void SetLiftingLosses(Float64 loss) override;
   Float64 GetShippingLosses() const override;
   void SetShippingLosses(Float64 loss) override;
   Float64 GetBeforeTempStrandRemovalLosses() const override;
   void SetBeforeTempStrandRemovalLosses(Float64 loss) override;
   Float64 GetAfterTempStrandRemovalLosses() const override;
   void SetAfterTempStrandRemovalLosses(Float64 loss) override;
   Float64 GetAfterDeckPlacementLosses() const override;
   void SetAfterDeckPlacementLosses(Float64 loss) override;
   Float64 GetAfterSIDLLosses() const override;
   void SetAfterSIDLLosses(Float64 loss) override;
   Float64 GetFinalLosses() const override;
   void SetFinalLosses(Float64 loss) override;

// IValidate
public:
   UINT Orientation(LPCTSTR lpszOrientation) override;

#ifdef _DEBUG
   bool AssertValid() const;
#endif//

private:
   EAF_DECLARE_AGENT_DATA;

   // status items must be managed by span girder
   void AddSegmentStatusItem(const CSegmentKey& segmentKey, const std::_tstring& message);
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
   // stored beyond the end of the array. The size of the array has been increased so that
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
   pgsTypes::ExposureCondition m_ExposureCondition;
   pgsTypes::ClimateCondition m_ClimateCondition;
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

   WBFL::LRFD::RangeOfApplicabilityAction m_RangeOfApplicabilityAction;
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
   static HRESULT SpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT RatingSpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT UnitModeProc(IStructuredSave*,IStructuredLoad*,std::shared_ptr<IEAFProgress>,CProjectAgentImp*);
   static HRESULT AlignmentProc(IStructuredSave*,IStructuredLoad*,std::shared_ptr<IEAFProgress>,CProjectAgentImp*);
   static HRESULT ProfileProc(IStructuredSave*,IStructuredLoad*,std::shared_ptr<IEAFProgress>,CProjectAgentImp*);
   static HRESULT SuperelevationProc(IStructuredSave*,IStructuredLoad*,std::shared_ptr<IEAFProgress>,CProjectAgentImp*);
   static HRESULT PierDataProc(IStructuredSave*,IStructuredLoad*,std::shared_ptr<IEAFProgress>,CProjectAgentImp*);
   static HRESULT EnvironmentProc(IStructuredSave*, IStructuredLoad*, std::shared_ptr<IEAFProgress>, CProjectAgentImp*);
   static HRESULT PierDataProc2(IStructuredSave*,IStructuredLoad*,std::shared_ptr<IEAFProgress>,CProjectAgentImp*);
   static HRESULT XSectionDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT XSectionDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT BridgeDescriptionProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT PrestressingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT PrestressingDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT ShearDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT ShearDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LongitudinalRebarDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LongitudinalRebarDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LoadFactorsProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingLoadDataProc(IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT UserLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LiveLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT SaveLiveLoad(IStructuredSave* pSave,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj,LPCTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT LoadLiveLoad(IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj,LPCTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT EffectiveFlangeWidthProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);
   static HRESULT LossesProc(IStructuredSave* pSave,IStructuredLoad* pLoad,std::shared_ptr<IEAFProgress> pProgress,CProjectAgentImp* pObj);

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

