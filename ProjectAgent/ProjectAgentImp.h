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

#include <PgsExt\PierData.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>

#include "LibraryEntryObserver.h"

#include <IFace\GirderHandling.h>


class CStructuredLoad;
class ConflictList;


/////////////////////////////////////////////////////////////////////////////
// CProjectAgentImp
class ATL_NO_VTABLE CProjectAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CProjectAgentImp, &CLSID_ProjectAgent>,
	public IConnectionPointContainerImpl<CProjectAgentImp>,
   public CProxyIProjectPropertiesEventSink<CProjectAgentImp>,
   public CProxyIEnvironmentEventSink<CProjectAgentImp>,
   public CProxyIBridgeDescriptionEventSink<CProjectAgentImp>,
   public CProxyISpecificationEventSink<CProjectAgentImp>,
   public CProxyIRatingSpecificationEventSink<CProjectAgentImp>,
   public CProxyILibraryConflictEventSink<CProjectAgentImp>,
   public CProxyILoadModifiersEventSink<CProjectAgentImp>,
   public IAgentEx,
   public IAgentPersist,
   public IProjectProperties,
   public IEnvironment,
   public IRoadwayData,
   public IBridgeDescription,
   public IGirderData,
   public IShear,
   public ILongitudinalRebar,
   public ISpecification,
   public IRatingSpecification,
   public ILibraryNames,
   public ILibrary,
   public ILoadModifiers,
   public IGirderHauling,
   public IGirderLifting,
   public IImportProjectLibrary,
   public IUserDefinedLoadData,
   public IEvents,
   public ILimits,
   public ILimits2,
   public ILoadFactors,
   public ILiveLoads
{  
public:
	CProjectAgentImp(); 
   virtual ~CProjectAgentImp();

DECLARE_REGISTRY_RESOURCEID(IDR_PROJECTAGENT)

BEGIN_COM_MAP(CProjectAgentImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
	COM_INTERFACE_ENTRY(IAgentPersist)
	COM_INTERFACE_ENTRY(IProjectProperties)
   COM_INTERFACE_ENTRY(IEnvironment)
   COM_INTERFACE_ENTRY(IRoadwayData)
   COM_INTERFACE_ENTRY(IBridgeDescription)
   COM_INTERFACE_ENTRY(IGirderData)
   COM_INTERFACE_ENTRY(IShear)
   COM_INTERFACE_ENTRY(ILongitudinalRebar)
   COM_INTERFACE_ENTRY(ISpecification)
   COM_INTERFACE_ENTRY(IRatingSpecification)
   COM_INTERFACE_ENTRY(ILibraryNames)
   COM_INTERFACE_ENTRY(ILibrary)
   COM_INTERFACE_ENTRY(ILoadModifiers)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
   COM_INTERFACE_ENTRY(IGirderHauling)
   COM_INTERFACE_ENTRY(IGirderLifting)
   COM_INTERFACE_ENTRY(IImportProjectLibrary)
   COM_INTERFACE_ENTRY(IUserDefinedLoadData)
   COM_INTERFACE_ENTRY(IEvents)
   COM_INTERFACE_ENTRY(ILimits)
   COM_INTERFACE_ENTRY(ILimits2)
   COM_INTERFACE_ENTRY(ILoadFactors)
   COM_INTERFACE_ENTRY(ILiveLoads)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CProjectAgentImp)
   CONNECTION_POINT_ENTRY( IID_IProjectPropertiesEventSink )
   CONNECTION_POINT_ENTRY( IID_IEnvironmentEventSink )
   CONNECTION_POINT_ENTRY( IID_IBridgeDescriptionEventSink )
   CONNECTION_POINT_ENTRY( IID_ISpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_IRatingSpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_ILibraryConflictEventSink )
   CONNECTION_POINT_ENTRY( IID_ILoadModifiersEventSink )
END_CONNECTION_POINT_MAP()

   StatusCallbackIDType m_scidGirderDescriptionWarning;


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
   virtual std::string GetBridgeName() const;
   virtual void SetBridgeName(const std::string& name);
   virtual std::string GetBridgeId() const;
   virtual void SetBridgeId(const std::string& bid);
   virtual std::string GetJobNumber() const;
   virtual void SetJobNumber(const std::string& jid);
   virtual std::string GetEngineer() const;
   virtual void SetEngineer(const std::string& eng);
   virtual std::string GetCompany() const;
   virtual void SetCompany(const std::string& company);
   virtual std::string GetComments() const;
   virtual void SetComments(const std::string& comments);
   virtual void EnableUpdate(bool bEnable);
   virtual bool IsUpdatedEnabled();
   virtual bool AreUpdatesPending();

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
   virtual const CBridgeDescription* GetBridgeDescription();
   virtual void SetBridgeDescription(const CBridgeDescription& desc);
   virtual const CDeckDescription* GetDeckDescription();
   virtual void SetDeckDescription(const CDeckDescription& deck);
   virtual const CSpanData* GetSpan(SpanIndexType spanIdx);
   virtual void SetSpan(SpanIndexType spanIdx,const CSpanData& spanData);
   virtual const CPierData* GetPier(PierIndexType pierIdx);
   virtual void SetPier(PierIndexType pierIdx,const CPierData& PierData);
   virtual void SetSpanLength(SpanIndexType spanIdx,double newLength);
   virtual void MovePier(PierIndexType pierIdx,double newStation,pgsTypes::MovePierOption moveOption);
   virtual void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt);
   virtual void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml);
   virtual void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing& spacing);
   virtual void SetGirderSpacingAtStartOfSpan(SpanIndexType spanIdx,const CGirderSpacing& spacing);
   virtual void SetGirderSpacingAtEndOfSpan(SpanIndexType spanIdx,const CGirderSpacing& spacing);
   virtual void UseSameGirderSpacingAtBothEndsOfSpan(SpanIndexType spanIdx,bool bUseSame);
   virtual void SetGirderTypes(SpanIndexType spanIdx,const CGirderTypes& girderTypes);
   virtual void SetGirderName( SpanIndexType spanIdx, GirderIndexType gdrIdx, const char* strGirderName);
   virtual void SetGirderCount(SpanIndexType spanIdx,GirderIndexType nGirders);
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierConnectionType connectionType);
   virtual void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan);
   virtual void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace, Float64 spanLength, const CSpanData* pSpanData=NULL,const CPierData* pPierData=NULL);
   virtual void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method);
   virtual pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod();
   virtual void UseSameNumberOfGirdersInAllSpans(bool bUseSame);
   virtual bool UseSameNumberOfGirdersInAllSpans();
   virtual void SetGirderCount(GirderIndexType nGirders);
   virtual void UseSameGirderForEntireBridge(bool bSame);
   virtual bool UseSameGirderForEntireBridge();
   virtual void SetGirderName(const char* strGirderName);
   virtual void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs);
   virtual pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   virtual void SetGirderSpacing(double spacing);
   virtual void SetMeasurementType(pgsTypes::MeasurementType mt);
   virtual pgsTypes::MeasurementType GetMeasurementType();
   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation();
   virtual void SetSlabOffset( Float64 slabOffset);
   virtual void SetSlabOffset( SpanIndexType spanIdx, Float64 start, Float64 end);
   virtual void SetSlabOffset( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 start, Float64 end);
   virtual pgsTypes::SlabOffsetType GetSlabOffsetType();
   virtual void GetSlabOffset( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64* pStart, Float64* pEnd);
   //virtual bool CanModelPostTensioning();
   //virtual bool IsPostTensioningModeled();
   //virtual void ConfigureBridgeForPostTensioning();
   //virtual bool CanBePostTensioned(SpanIndexType spanIdx,GirderIndexType gdrIdx);

// IGirderData
public:
   virtual const matPsStrand* GetStrandMaterial(SpanIndexType span,GirderIndexType gdr) const;
   virtual void SetStrandMaterial(SpanIndexType span,GirderIndexType gdr,const matPsStrand* pmat);
   virtual CGirderData GetGirderData(SpanIndexType span,GirderIndexType gdr) const;
   virtual bool SetGirderData(const CGirderData& data,SpanIndexType span,GirderIndexType gdr);
   virtual const CGirderMaterial* GetGirderMaterial(SpanIndexType span,GirderIndexType gdr) const;
   virtual void SetGirderMaterial(SpanIndexType span,GirderIndexType gdr,const CGirderMaterial& material);
private:
   // set girder data and return an int containing the && of what was changed
   // as per CGirderData::ChangeType
   int DoSetGirderData(const CGirderData& data,SpanIndexType span,GirderIndexType gdr);

// IShear
public:
   virtual std::string GetStirrupMaterial(SpanIndexType span,GirderIndexType gdr) const;
   virtual void SetStirrupMaterial(SpanIndexType span,GirderIndexType gdr,const char* matName);
   virtual CShearData GetShearData(SpanIndexType span,GirderIndexType gdr) const;
   virtual bool SetShearData(const CShearData& data,SpanIndexType span,GirderIndexType gdr);
private:
   bool DoSetShearData(const CShearData& data,SpanIndexType span,GirderIndexType gdr);

// ILongitudinalRebar
public:
   virtual std::string GetLongitudinalRebarMaterial(SpanIndexType span,GirderIndexType gdr) const;
   virtual void SetLongitudinalRebarMaterial(SpanIndexType span,GirderIndexType gdr,const char* matName);
   virtual CLongitudinalRebarData GetLongitudinalRebarData(SpanIndexType span,GirderIndexType gdr) const;
   virtual bool SetLongitudinalRebarData(const CLongitudinalRebarData& data,SpanIndexType span,GirderIndexType gdr);
private:
   bool DoSetLongitudinalRebarData(const CLongitudinalRebarData& data,SpanIndexType span,GirderIndexType gdr);

// ISpecification
public:
   virtual std::string GetSpecification() const;
   virtual void SetSpecification(const std::string& spec);
   virtual void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType);
   virtual Uint16 GetMomentCapacityMethod();
   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType);
   virtual pgsTypes::AnalysisType GetAnalysisType();
   virtual arDesignOptions GetDesignOptions(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual bool IsSlabOffsetDesignEnabled();
   virtual pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType();

// IRatingSpecification
public:
   virtual bool AlwaysLoadRate() const;
   virtual bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType) const;
   virtual void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable);
   virtual std::string GetRatingSpecification() const;
   virtual void SetADTT(Int16 adtt);
   virtual Int16 GetADTT() const;
   virtual void SetRatingSpecification(const std::string& spec);
   virtual void IncludePedestrianLiveLoad(bool bInclude);
   virtual bool IncludePedestrianLiveLoad() const;
   virtual void SetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor);
   virtual void GetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const;
   virtual Float64 GetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx) const;
   virtual void SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor);
   virtual void GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const;
   virtual Float64 GetDeckConditionFactor() const;
   virtual void SetSystemFactorFlexure(Float64 sysFactor);
   virtual Float64 GetSystemFactorFlexure() const;
   virtual void SetSystemFactorShear(Float64 sysFactor);
   virtual Float64 GetSystemFactorShear() const;
   virtual void SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC);
   virtual Float64 GetDeadLoadFactor(pgsTypes::LimitState ls) const;
   virtual void SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW);
   virtual Float64 GetWearingSurfaceFactor(pgsTypes::LimitState ls) const;
   virtual void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL);
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false) const;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false) const;
   virtual void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,Float64 t);
   virtual Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType) const;
   virtual Float64 GetAllowableTension(pgsTypes::LoadRatingType ratingType,SpanIndexType spanIdx,GirderIndexType gdrIdx) const;
   virtual void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress);
   virtual bool RateForStress(pgsTypes::LoadRatingType ratingType) const;
   virtual void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear);
   virtual bool RateForShear(pgsTypes::LoadRatingType ratingType) const;
   virtual void ExcludeLegalLoadLaneLoading(bool bExclude);
   virtual bool ExcludeLegalLoadLaneLoading() const;
   virtual void SetYieldStressLimitCoefficient(Float64 x);
   virtual Float64 GetYieldStressLimitCoefficient() const;
   virtual void SetSpecialPermitType(pgsTypes::SpecialPermitType type);
   virtual pgsTypes::SpecialPermitType GetSpecialPermitType() const;

// ILibraryNames
public:
   virtual void EnumGdrConnectionNames( std::vector<std::string>* pNames ) const;
   virtual void EnumGirderNames( std::vector<std::string>* pNames ) const;
   virtual void EnumGirderNames( const char* strGirderFamily, std::vector<std::string>* pNames ) const;
   virtual void EnumConcreteNames( std::vector<std::string>* pNames ) const;
   virtual void EnumDiaphragmNames( std::vector<std::string>* pNames ) const;
   virtual void EnumTrafficBarrierNames( std::vector<std::string>* pNames ) const;
   virtual void EnumSpecNames( std::vector<std::string>* pNames) const;
   virtual void EnumRatingCriteriaNames( std::vector<std::string>* pNames) const;
   virtual void EnumLiveLoadNames( std::vector<std::string>* pNames) const;
   virtual void EnumGirderFamilyNames( std::vector<std::string>* pNames );
   virtual void GetBeamFactory(const std::string& strBeamFamily,const std::string& strBeamName,IBeamFactory** ppFactory);

// ILibrary
public:
   virtual void SetLibraryManager(psgLibraryManager* pNewLibMgr);
   virtual psgLibraryManager* GetLibraryManager(); 
   virtual const ConnectionLibraryEntry* GetConnectionEntry(const char* lpszName ) const;
   virtual const GirderLibraryEntry* GetGirderEntry( const char* lpszName ) const;
   virtual const ConcreteLibraryEntry* GetConcreteEntry( const char* lpszName ) const;
   virtual const DiaphragmLayoutEntry* GetDiaphragmEntry( const char* lpszName ) const;
   virtual const TrafficBarrierEntry* GetTrafficBarrierEntry( const char* lpszName ) const;
   virtual const SpecLibraryEntry* GetSpecEntry( const char* lpszName ) const;
   virtual const LiveLoadLibraryEntry* GetLiveLoadEntry( const char* lpszName ) const;
   virtual ConcreteLibrary&        GetConcreteLibrary();
   virtual ConnectionLibrary&      GetConnectionLibrary();
   virtual GirderLibrary&          GetGirderLibrary();
   virtual DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary();
   virtual TrafficBarrierLibrary&  GetTrafficBarrierLibrary();
   virtual LiveLoadLibrary*        GetLiveLoadLibrary();
   virtual SpecLibrary*            GetSpecLibrary();
   virtual std::vector<libEntryUsageRecord> GetLibraryUsageRecords() const;
   virtual void GetMasterLibraryInfo(std::string& strPublisher,std::string& strMasterLib,sysTime& time) const;
   virtual RatingLibrary* GetRatingLibrary();
   virtual const RatingLibrary* GetRatingLibrary() const;
   virtual const RatingLibraryEntry* GetRatingEntry( const char* lpszName ) const;

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

// IGirderLifting
public:
   virtual Float64 GetLeftLiftingLoopLocation(SpanIndexType span,GirderIndexType girder);
   virtual Float64 GetRightLiftingLoopLocation(SpanIndexType span,GirderIndexType girder);
   virtual bool SetLiftingLoopLocations(SpanIndexType span,GirderIndexType girder, Float64 left,Float64 right);
private:
   bool DoSetLiftingLoopLocations(SpanIndexType span,GirderIndexType girder, Float64 left, Float64 right);

// IGirderHauling
public:
   virtual Float64 GetLeadingOverhang(SpanIndexType span,GirderIndexType girder);
   virtual Float64 GetTrailingOverhang(SpanIndexType span,GirderIndexType girder);
   virtual bool SetTruckSupportLocations(SpanIndexType span,GirderIndexType girder, Float64 leftLoc,Float64 rightLoc);
private:
   bool DoSetTruckSupportLocations(SpanIndexType span,GirderIndexType girder, Float64 leftLoc,Float64 rightLoc);

// IImportProjectLibrary
public:
   virtual bool ImportProjectLibraries(IStructuredLoad* pLoad);

// IUserDefinedLoadData
public:
   virtual CollectionIndexType GetPointLoadCount() const;
   virtual CollectionIndexType AddPointLoad(const CPointLoadData& pld);
   virtual const CPointLoadData& GetPointLoad(CollectionIndexType idx) const;
   virtual void UpdatePointLoad(CollectionIndexType idx, const CPointLoadData& pld);
   virtual void DeletePointLoad(CollectionIndexType idx);

   virtual CollectionIndexType GetDistributedLoadCount() const;
   virtual CollectionIndexType AddDistributedLoad(const CDistributedLoadData& pld);
   virtual const CDistributedLoadData& GetDistributedLoad(CollectionIndexType idx) const;
   virtual void UpdateDistributedLoad(CollectionIndexType idx, const CDistributedLoadData& pld);
   virtual void DeleteDistributedLoad(CollectionIndexType idx);

   virtual CollectionIndexType GetMomentLoadCount() const;
   virtual CollectionIndexType AddMomentLoad(const CMomentLoadData& pld);
   virtual const CMomentLoadData& GetMomentLoad(CollectionIndexType idx) const;
   virtual void UpdateMomentLoad(CollectionIndexType idx, const CMomentLoadData& pld);
   virtual void DeleteMomentLoad(CollectionIndexType idx);

   virtual void SetConstructionLoad(Float64 load);
   virtual Float64 GetConstructionLoad() const;

// ILiveLoads
public:
   virtual bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType);
   virtual bool IsPedestianLoadEnabled(pgsTypes::LiveLoadType llType);
   virtual void EnablePedestianLoad(pgsTypes::LiveLoadType llType,bool bEnable);
   virtual std::vector<std::string> GetLiveLoadNames(pgsTypes::LiveLoadType llType);
   virtual void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::string>& names);
   virtual double GetTruckImpact(pgsTypes::LiveLoadType llType);
   virtual void SetTruckImpact(pgsTypes::LiveLoadType llType,double impact);
   virtual double GetLaneImpact(pgsTypes::LiveLoadType llType);
   virtual void SetLaneImpact(pgsTypes::LiveLoadType llType,double impact);
   virtual void SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action);
   virtual LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction();
   virtual std::string GetLLDFSpecialActionText(); // get common string for ignore roa case
   virtual bool IgnoreLLDFRangeOfApplicability(); // true if action is to ignore ROA

// IEvents
public:
   virtual void HoldEvents();
   virtual void FirePendingEvents();

// ILimits
public:
   virtual double GetMaxSlabFc();
   virtual double GetMaxGirderFci();
   virtual double GetMaxGirderFc();
   virtual double GetMaxConcreteUnitWeight();
   virtual double GetMaxConcreteAggSize();

// ILimits2
public:
   virtual double GetMaxSlabFc(pgsTypes::ConcreteType concType);
   virtual double GetMaxGirderFci(pgsTypes::ConcreteType concType);
   virtual double GetMaxGirderFc(pgsTypes::ConcreteType concType);
   virtual double GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType);
   virtual double GetMaxConcreteAggSize(pgsTypes::ConcreteType concType);

// ILoadFactors
public:
   const CLoadFactors* GetLoadFactors() const;
   void SetLoadFactors(const CLoadFactors& loadFactors) ;

#ifdef _DEBUG
   bool AssertValid() const;
#endif//

private:
   DECLARE_AGENT_DATA;

   // status items must be managed by span girder
   void AddGirderStatusItem(SpanIndexType span, GirderIndexType girder, std::string& message);
   void RemoveGirderStatusItems(SpanIndexType span, GirderIndexType girder);

   // save hash value and vector of status ids
   typedef std::map<SpanGirderHashType, std::vector<long> > StatusContainer;
   typedef StatusContainer::iterator StatusIterator;

   std::map<SpanGirderHashType, std::vector<long> > m_CurrentGirderStatusItems;

   std::string m_BridgeName;
   std::string m_BridgeId;
   std::string m_JobNumber;
   std::string m_Engineer;
   std::string m_Company;
   std::string m_Comments;
   bool m_bPropertyUpdatesEnabled;
   bool m_bPropertyUpdatesPending;

   pgsTypes::AnalysisType m_AnalysisType;
   bool m_bGetAnalysisTypeFromLibrary; // if true, we are reading old input... get the analysis type from the library entry

   std::vector<std::string> m_GirderFamilyNames;

   // rating data
   std::string m_RatingSpec;
   const RatingLibraryEntry* m_pRatingEntry;
   bool m_bExcludeLegalLoadLaneLoading;
   bool m_bIncludePedestrianLiveLoad;
   pgsTypes::SpecialPermitType m_SpecialPermitType;
   Float64 m_SystemFactorFlexure;
   Float64 m_SystemFactorShear;
   Float64 m_gDC[12]; // use the IndexFromLimitState to access array
   Float64 m_gDW[12]; // use the IndexFromLimitState to access array
   Float64 m_gLL[12]; // use the IndexFromLimitState to access array
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
   double m_AlignmentOffset_Temp;
   bool m_bUseTempAlignmentOffset;

   AlignmentData2 m_AlignmentData2;
   ProfileData2   m_ProfileData2;
   RoadwaySectionData m_RoadwaySectionData;

   // Bridge Description Data
   mutable CBridgeDescription m_BridgeDescription;

   // Prestressing Data
   void UpdateJackingForce() const;
   void UpdateJackingForce(SpanIndexType span,GirderIndexType gdr);
   mutable bool m_bUpdateJackingForce;

   // Specification Data
   std::string m_Spec;
   const SpecLibraryEntry* m_pSpecEntry;


   // Live load selection
   struct LiveLoadSelection
   {
      std::string                 EntryName;
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
   double m_TruckImpact[8];
   double m_LaneImpact[8];

   std::vector<std::string> m_ReservedLiveLoads; // reserved live load names (names not found in library)
   bool IsReservedLiveLoad(const std::string& strName);

   void OnLiveLoadEntryRenamed(LiveLoadLibraryEntry* pEntry);

   LldfRangeOfApplicabilityAction m_LldfRangeOfApplicabilityAction;
   bool m_bGetIgnoreROAFromLibrary; // if true, we are reading old input... get the Ignore ROA setting from the spec library entry

   // Load Modifiers
   ILoadModifiers::Level m_DuctilityLevel;
   ILoadModifiers::Level m_ImportanceLevel;
   ILoadModifiers::Level m_RedundancyLevel;
   Float64 m_DuctilityFactor;
   Float64 m_ImportanceFactor;
   Float64 m_RedundancyFactor;

   CLoadFactors m_LoadFactors;

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
   bool m_bHoldingEvents;
   std::map<SpanGirderHashType,Uint32> m_PendingEventsHash; // hash values for span/girders that have pending events

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
   static HRESULT LiftingAndHaulingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingLoadDataProc(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT UserLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiveLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT SaveLiveLoad(IStructuredSave* pSave,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT LoadLiveLoad(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType);

   HRESULT BuildDummyBridge();
   void ValidateStrands(SpanIndexType span,GirderIndexType girder,CGirderData& girder_data,bool fromLibrary);

   Float64 GetMaxPjack(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands) const;
   Float64 GetMaxPjack(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands,const matPsStrand* pStrand) const;

   bool ResolveLibraryConflicts(const ConflictList& rList);
   void DealWithGirderLibraryChanges(bool fromLibrary);  // behavior is different if problem is caused by a library change
   void DealWithConnectionLibraryChanges(bool fromLibrary);
   
   bool CanHavePedestrianLoad() const;

   void MoveBridge(PierIndexType pierIdx,double newStation);
   void MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,double newStation);
   void MoveBridgeAdjustNextSpan(PierIndexType pierIdx,double newStation);
   void MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,double newStation);

   void SpecificationChanged(bool bFireEvent);
   void InitSpecification(const std::string& spec);

   void RatingSpecificationChanged(bool bFireEvent);
   void InitRatingSpecification(const std::string& spec);

   void UseBridgeLibraryEntries();
   void UseGirderLibraryEntries();
   void ReleaseBridgeLibraryEntries();
   void ReleaseGirderLibraryEntries();

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
   // of the GirderData, this concrete library name needed to be stored somewhere.
   // That is the purpose of this data member. It is only used for temporary storage
   // while loading old files
   std::string m_strOldGirderConcreteName;
};

#endif //__PROJECTAGENT_H_

