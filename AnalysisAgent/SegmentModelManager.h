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

#pragma once

#include "SegmentModelData.h"
#include "ProductLoadMap.h"
#include <IFace\AnalysisResults.h>

// CSegmentModelManager
//
// Manages the creation, access, and storage of analysis models of precast segments.
// This are the models of the individual segments that are used before they are erected
// into the bridge and can be modeled with the LBAM.

class CSegmentModelManager
{
public:
   CSegmentModelManager(SHARED_LOGFILE lf,IBroker* pBroker);

   void Clear();

   void DumpAnalysisModels(GirderIndexType gdrIdx) const;

   Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   Float64 GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const pgsPointOfInterest& poi, ResultsType resultsType) const;
   Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const;
   Float64 GetReaction(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType) const;

   // IReactions
   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,ResultsType resultsType,Float64* pRleft,Float64* pRright) const;
   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,ResultsType resultsType,Float64* pRleft,Float64* pRright) const;

   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const PoiList& vPoi, ResultsType resultsType) const;
   std::vector<Float64> GetPretensionXDeflection(IntervalIndexType intervalIdx, const PoiList& vPoi, ResultsType resultsType) const;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;

   std::vector<Float64> GetUnitLoadMoment(IntervalIndexType intervalIdx,const PoiList& vPoi,const pgsPointOfInterest& unitLoadPOI) const;
   std::vector<WBFL::System::SectionValue> GetUnitCoupleMoment(IntervalIndexType intervalIdx,const PoiList& vPoi,const pgsPointOfInterest& unitMomentPOI) const;

   Float64 GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   Float64 GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const;
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType comboType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const;

   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;

   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const;
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,Float64* pMin,Float64* pMax) const;
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,bool bIncludePrestress,Float64* pMin,Float64* pMax) const;
   void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,PierIndexType pierIdx,const CGirderKey& girderKey,bool bIncludeImpact,Float64* pMin,Float64* pMax) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax) const;

   void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,std::vector<WBFL::System::SectionValue>* pMin,std::vector<WBFL::System::SectionValue>* pMax) const;
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;

   // IExternalLoad
   bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName);
   bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType comboType);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;
   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const;

   // Returns POIs at segment supports where deflection is zero, or dependent on adjacent segment if segment is a drop in,
   // at time of erection. 
   std::vector<pgsPointOfInterest> GetDeflectionDatumLocationsForSegment(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx, pgsTypes::DropInType dropInType) const;


private:
	DECLARE_SHARED_LOGFILE;
   IBroker* m_pBroker; // must be a weak reference (this is the agent's pointer and it is a weak refernece)

   CProductLoadMap m_ProductLoadMap;

   typedef std::map<CSegmentKey,CSegmentModelData> SegmentModels;
   mutable SegmentModels m_ReleaseModels;
   mutable SegmentModels m_LiftingModels;
   mutable SegmentModels m_StorageModels;
   mutable SegmentModels m_HaulingModels;

   typedef std::multimap<LoadingCombinationType,std::_tstring> LoadCombinationMap;
   mutable std::map<CGirderKey,LoadCombinationMap> m_LoadCombinationMaps;
   const LoadCombinationMap& GetLoadCombinationMap(const CGirderKey& girderKey) const;
   LoadCombinationMap& GetLoadCombinationMap(const CGirderKey& girderKey);

   void DumpAnalysisModels(GirderIndexType gdrIdx,const SegmentModels* pModels,LPCTSTR name) const;

   void GetPrestressSectionResults(IntervalIndexType intervalIdx,const PoiList& vPoi,ResultsType resultsType,std::vector<WBFL::System::SectionValue>* pvFx,std::vector<WBFL::System::SectionValue>* pvFy,std::vector<WBFL::System::SectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const;
   void GetPrestressSectionResults(IntervalIndexType intervalIdx,const PoiList& vPoi,std::vector<WBFL::System::SectionValue>* pvFx,std::vector<WBFL::System::SectionValue>* pvFy,std::vector<WBFL::System::SectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const;


   void GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const PoiList& vPoi,ResultsType resultsType,std::vector<WBFL::System::SectionValue>* pvFx,std::vector<WBFL::System::SectionValue>* pvFy,std::vector<WBFL::System::SectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const;
   void GetSectionResults(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,std::vector<WBFL::System::SectionValue>* pvFx,std::vector<WBFL::System::SectionValue>* pvFy,std::vector<WBFL::System::SectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const;
   void GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const PoiList& vPoi,std::vector<WBFL::System::SectionValue>* pvFx,std::vector<WBFL::System::SectionValue>* pvFy,std::vector<WBFL::System::SectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const;

   void GetSectionStresses(IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,const PoiList& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;
   void GetSectionStress(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const;

   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,Float64* pRleft,Float64* pRright) const;
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType) const;
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey) const;


   PoiIDPairType AddPointOfInterest(CSegmentModelData* pModelData,const pgsPointOfInterest& poi) const;
   void ZeroResults(const PoiList& vPoi,std::vector<WBFL::System::SectionValue>* pvFx,std::vector<WBFL::System::SectionValue>* pvFy,std::vector<WBFL::System::SectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const;


   CSegmentModelData* GetSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const;


   void BuildReleaseModel(const CSegmentKey& segmentKey) const;
   void BuildLiftingModel(const CSegmentKey& segmentKey) const;
   void BuildStorageModel(const CSegmentKey& segmentKey) const;
   void BuildHaulingModel(const CSegmentKey& segmentKey) const;

   CSegmentModelData* GetReleaseModel(const CSegmentKey& segmentKey) const;
   CSegmentModelData* GetLiftingModel(const CSegmentKey& segmentKey) const;
   CSegmentModelData* GetStorageModel(const CSegmentKey& segmentKey) const;
   CSegmentModelData* GetHaulingModel(const CSegmentKey& segmentKey) const;

   CSegmentModelData BuildSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 leftSupportDistance,Float64 rightSupportDistance,PoiAttributeType refAttribute) const;
   PoiIDType AddPointOfInterest(SegmentModels& models,const pgsPointOfInterest& poi) const;

   void ApplyPretensionLoad(CSegmentModelData* pModelData,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const;
   void ApplyPostTensionLoad(CSegmentModelData* pModelData, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const;

   CSegmentModelData* GetModelData(SegmentModels& models,const CSegmentKey& segmentKey) const;

   LoadCaseIDType GetLoadCaseID(pgsTypes::ProductForceType pfType) const;
   void GetLoadCaseID(pgsTypes::StrandType strandType,LoadCaseIDType* plcidMx,LoadCaseIDType* plcidMy) const;
   LoadCaseIDType GetLoadCaseID(CSegmentModelData* pModelData,LPCTSTR strLoadingName) const;
   LoadCaseIDType GetGirderIncrementalLoadCaseID() const;
   LoadCaseIDType GetFirstExternalLoadCaseID() const;

   void GetMemberLocation(const pgsPointOfInterest& poi,CSegmentModelData* pModelData,MemberIDType* pMbrID,Float64* pLocation);

   CSegmentKey GetSegmentKey(const CGirderKey& girderKey,PierIndexType pierIdx) const;

   void AddLoading(CSegmentModelData& model_data,pgsTypes::ProductForceType pfType) const;

   bool CreateConcentratedLoad(IntervalIndexType intervalIdx, LoadCaseIDType lcid,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx, LoadCaseIDType lcid,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);

   bool VerifyPoi(const PoiList& vPoi) const; // all POI must be for the same segment
};
