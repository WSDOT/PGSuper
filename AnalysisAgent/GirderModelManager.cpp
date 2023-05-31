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

#include "stdafx.h"
#include "GirderModelManager.h"

#include <PgsExt\LoadFactors.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\StatusItem.h>

#include <PGSuperException.h>

#include "BarrierSidewalkLoadDistributionTool.h"

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\RatingSpecification.h>
#include <IFace\DistributionFactors.h>
#include <IFace\DocumentType.h>
#include <IFace\Alignment.h>
#include <IFace\Constructability.h>

#include <EAF\EAFStatusCenter.h>
#include <EAF\EAFAutoProgress.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include <Math\MathUtils.h>

#include <pgsExt\AnalysisResult.h>

#include <iterator>
#include <algorithm>
#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#if defined _DEBUG
#define VERIFY_ANALYSIS_TYPE VerifyAnalysisType()
#else
#define VERIFY_ANALYSIS_TYPE
#endif // _DEBUG

static CComBSTR gs_LoadCases[] = 
{
   _T("DC"),
   _T("DW"),
   _T("DW_Rating"),
   _T("DWp"),
   _T("DWf"),
   _T("CR"),
   _T("SH"),
   _T("RE"),
   _T("PS")
};
static int gs_nLoadCases = sizeof(gs_LoadCases)/sizeof(gs_LoadCases[0]);

static CComBSTR gs_LimitStateNames[] = 
{
   _T("SERVICE-I"),
   _T("SERVICE-IA"),
   _T("SERVICE-III"),
   _T("STRENGTH-I"),
   _T("STRENGTH-II"),
   _T("FATIGUE-I"),
   _T("STRENGTH-I-Inventory"),
   _T("STRENGTH-I-Operating"),
   _T("SERVICE-III-Inventory"),
   _T("SERVICE-III-Operating"),
   _T("STRENGTH-I-LegalRoutine"),
   _T("STRENGTH-I-LegalSpecial"),
   _T("SERVICE-III-LegalRoutine"),
   _T("SERVICE-III-LegalSpecial"),
   _T("STRENGTH-II-PermitRoutine"),
   _T("SERVICE-I-PermitRoutine"),
   _T("SERVICE-III-PermitRoutine"),
   _T("STRENGTH-II-PermitSpecial"),
   _T("SERVICE-I-PermitSpecial"),
   _T("SERVICE-III-PermitSpecial"),
   _T("STRENGTH-I-LegalEmergency"),
   _T("SERVICE-III-LegalEmergency")
};

static int gs_nLimitStates = sizeof(gs_LimitStateNames)/sizeof(gs_LimitStateNames[0]);

static CComBSTR gs_LiveLoadNames[] = 
{
   _T("LL+IM Design"),
   _T("LL+IM Permit"),
   _T("LL+IM Fatigue"),
   _T("LL+IM Pedestrian"),
   _T("LL+IM Legal Rating (Routine)"),
   _T("LL+IM Legal Rating (Special)"),
   _T("LL+IM Legal Rating (Emergency)"),
   _T("LL+IM Permit Rating (Routine)"),
   _T("LL+IM Permit Rating (Special)")
};

inline LiveLoadApplicabilityType ConvertLiveLoadApplicabilityType(pgsTypes::LiveLoadApplicabilityType type)
{
   // LBAM enums have a value of 1 more than the pgsTypes enum
   return (LiveLoadApplicabilityType)(((int)type) + 1);
}

inline pgsTypes::LiveLoadApplicabilityType ConvertLiveLoadApplicabilityType(LiveLoadApplicabilityType type)
{
   // LBAM enums have a value of 1 more than the pgsTypes enum
   return (pgsTypes::LiveLoadApplicabilityType)(((int)type) - 1);
}

COverhangLoadData::COverhangLoadData(const CSegmentKey& segmentKey, const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup, Float64 PStart, Float64 PEnd):
   segmentKey(segmentKey), bstrStage(bstrStage), bstrLoadGroup(bstrLoadGroup), PStart(PStart), PEnd(PEnd)
{
}

bool COverhangLoadData::operator==(const COverhangLoadData& rOther) const
{
   if ( segmentKey != rOther.segmentKey || (bstrStage!=rOther.bstrStage) || (bstrLoadGroup!=rOther.bstrLoadGroup) )
   {
      return false;
   }

   return true;
}

bool COverhangLoadData::operator<(const COverhangLoadData& rOther) const
{
   // we must satisfy strict weak ordering for the set to work properly
   if ( segmentKey != rOther.segmentKey)
   {
      return segmentKey < rOther.segmentKey;
   }

   if (bstrStage!=rOther.bstrStage)
   {
      return bstrStage < rOther.bstrStage;
   }
      
   if (bstrLoadGroup!=rOther.bstrLoadGroup)
   {
      return bstrLoadGroup < rOther.bstrLoadGroup;
   }

   return false;
}


#define TEMPORARY_SUPPORT_ID_OFFSET 10000

// This list is ordered like pgsTypes::LiveLoadType
const LiveLoadModelType g_LiveLoadModelType[] =
{ lltDesign,             // strength and service design
  lltPermit,             // permit for Strength II design
  lltFatigue,            // fatigue limit state design
  lltPedestrian,
  lltLegalRoutineRating, // legal rating, routine traffic
  lltLegalSpecialRating, // legal rating, commercial traffic
  lltLegalEmergencyRating, // legal rating, emergency vehicles
  lltPermitRoutineRating,        // permit rating
  lltPermitSpecialRating
}; 

// Reverse lookup in above array
pgsTypes::LiveLoadType GetLiveLoadTypeFromModelType(LiveLoadModelType llmtype)
{
   int numlls = sizeof(g_LiveLoadModelType)/sizeof(g_LiveLoadModelType[0]);
   for(int is=0; is<numlls; is++)
   {
      if (llmtype == g_LiveLoadModelType[is])
      {
         return (pgsTypes::LiveLoadType)is;
      }
   }

   ATLASSERT(false); // should never happen since a match should always be in array
   return pgsTypes::lltDesign;
}

CGirderModelManager::CGirderModelManager(SHARED_LOGFILE lf,IBroker* pBroker,StatusGroupIDType statusGroupID) :
LOGFILE(lf),
m_pBroker(pBroker),
m_StatusGroupID(statusGroupID)
{
   // create an array for pois going into the lbam. create it here once so we don't need to make a new one every time we need it
   HRESULT hr = m_LBAMPoi.CoCreateInstance(CLSID_IDArray);
   ATLASSERT( SUCCEEDED(hr) );

   hr = m_LBAMUtility.CoCreateInstance(CLSID_LRFDFactory);
   ATLASSERT( SUCCEEDED(hr) );

   hr = m_UnitServer.CoCreateInstance(CLSID_UnitServer);
   ATLASSERT( SUCCEEDED(hr) );

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidInformationalError = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusError)); // informational with help for girder end offset error
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback( new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   
   m_NextPoi = 0;
}

void CGirderModelManager::Clear()
{
   m_GirderModels.clear();
}

//////////////////////////////////////////////////
// IProductLoads
Float64 CGirderModelManager::GetTrafficBarrierLoad(const CSegmentKey& segmentKey) const
{
   ValidateGirderModels(segmentKey);

   auto found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() ); // it should be found

   return (*found).second.m_BarrierLoad;
}

Float64 CGirderModelManager::GetSidewalkLoad(const CSegmentKey& segmentKey) const
{
   ValidateGirderModels(segmentKey);

   auto found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() ); // it should be found

   return (*found).second.m_SidewalkLoad;
}

void CGirderModelManager::GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) const
{
   ValidateGirderModels(segmentKey);

   GetMainSpanOverlayLoad(segmentKey,pOverlayLoads);
}

void CGirderModelManager::GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) const
{
   ValidateGirderModels(segmentKey);
   GetMainConstructionLoad(segmentKey,pConstructionLoads);
}

bool CGirderModelManager::HasShearKeyLoad(const CGirderKey& girderKeyOrig) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IGirder,pGirder);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   GirderIndexType gdrIdx = min( girderKeyOrig.girderIndex, pBridgeDesc->GetGirderGroup(girderKeyOrig.groupIndex)->GetGirderCount()-1 );
   CGirderKey girderKey(girderKeyOrig.groupIndex, gdrIdx);

   // First check if this beam has a shear key
   if ( pGirder->HasShearKey(girderKey, spacingType))
   {
      return true;
   }

   // Next check adjacent beams if we have a continuous analysis
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   if ( analysisType == pgsTypes::Simple)
   {
      return false;
   }

   // We have a continuous analysis - walk girder line
   // If any girder in the girder line has a shear key, then there is a shear key load
   // Note: Not bothering to check boundary conditions
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      if (grpIdx != girderKey.groupIndex) // already checked this above
      {
         // if there are fewer girders in this group than in groupIdx,
         // adjust the girder index based on number of girders in group is.
         GirderIndexType nGirdersInGroup = pBridge->GetGirderCount(grpIdx);
         GirderIndexType gdrIdx = Min(nGirdersInGroup-1,girderKey.girderIndex);

         if (pGirder->HasShearKey( CGirderKey(grpIdx,gdrIdx), spacingType))
         {
            return true;
         }
      }
   }

   return false;
}

void CGirderModelManager::GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) const
{
   ValidateGirderModels(segmentKey);
   GetMainSpanShearKeyLoad(segmentKey,pLoads);
}

bool CGirderModelManager::HasLongitudinalJointLoad() const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   return pIBridgeDesc->GetBridgeDescription()->HasStructuralLongitudinalJoints();
}

void CGirderModelManager::GetLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const
{
   ValidateGirderModels(segmentKey);
   GetMainSpanLongitudinalJointLoad(segmentKey, pLoads);
}

bool CGirderModelManager::HasPedestrianLoad() const
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
   ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);
   ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   bool isRatingPed = pRatingSpec->IncludePedestrianLiveLoad();

   // if the Pedestrian on Sidewalk live load is not defined, then there can't be ped loading
   if ( DesignPedLoad==ILiveLoads::PedDontApply && PermitPedLoad==ILiveLoads::PedDontApply && 
      FatiguePedLoad==ILiveLoads::PedDontApply && !isRatingPed)
   {
      return false;
   }

   // returns true if there is a sidewalk on the bridge that is wide enough support
   // pedestrian live load
   GET_IFACE(IBarriers,pBarriers);
   GET_IFACE(ILibrary,pLibrary);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 minWidth = pSpecEntry->GetMinSidewalkWidth();

   Float64 leftWidth(0), rightWidth(0);
   if (pBarriers->HasSidewalk(pgsTypes::tboLeft))
   {
      Float64 intLoc, extLoc;
      pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboLeft, &intLoc, &extLoc);
      leftWidth  = intLoc - extLoc;
      ATLASSERT(leftWidth>0.0);
   }

   if (pBarriers->HasSidewalk(pgsTypes::tboRight))
   {
      Float64 intLoc, extLoc;
      pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboRight, &intLoc, &extLoc);
      rightWidth = intLoc - extLoc;
      ATLASSERT(rightWidth>0.0);
   }

   if ( leftWidth <= minWidth && rightWidth <= minWidth )
   {
      return false; // sidewalks too narrow for PL
   }

   return true;
}

bool CGirderModelManager::HasSidewalkLoad(const CGirderKey& girderKey) const
{
   bool bHasSidewalkLoad = false;

   GET_IFACE(IBridge,pBridge);
   GirderIndexType gdrIdx = min(girderKey.girderIndex, pBridge->GetGirderCount(girderKey.groupIndex)-1);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey.groupIndex, gdrIdx, segIdx);

      Float64 swLoad, fraLeft, fraRight;
      GetSidewalkLoadFraction(segmentKey,&swLoad,&fraLeft,&fraRight);

      if ( !IsZero(swLoad))
      {
         return true;
      }
   }

   return bHasSidewalkLoad;
}

bool CGirderModelManager::HasPedestrianLoad(const CGirderKey& girderKey) const
{
   //
   // NOTE: This code below is for distributing the pedestrian load the same way the sidewalk
   //       and traffic barrier dead loads are distributed
   //
   bool bHasPedLoad = HasPedestrianLoad();
   if ( !bHasPedLoad )
   {
      return false; // there is no chance of having any Ped load on this bridge
   }

   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      // any pedestrian load is good enough for this case
      bHasPedLoad = true;
   }
   else
   {
      bHasPedLoad = false;
      GET_IFACE(IBridge,pBridge);
      GirderIndexType gdrIdx = min(girderKey.girderIndex, pBridge->GetGirderCount(girderKey.groupIndex)-1);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey.groupIndex,gdrIdx, segIdx);

         Float64 swLoad, fraLeft, fraRight;
         GetSidewalkLoadFraction(segmentKey, &swLoad, &fraLeft,&fraRight);

         if ( !IsZero(fraLeft) || !IsZero(fraRight) )
         {
            return true; // there is load on one of the segments
         }
      }
   }

   return bHasPedLoad;
}

Float64 CGirderModelManager::GetPedestrianLoad(const CSegmentKey& segmentKey) const
{
   return GetPedestrianLiveLoad(segmentKey);
}

///////////////////////////////////////////////////
// IProductForces
Float64 CGirderModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> axial = GetAxial(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(axial.size() == 1);

   return axial.front();
}

WBFL::System::SectionValue CGirderModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<WBFL::System::SectionValue> shears = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(shears.size() == 1);

   return shears.front();
}

Float64 CGirderModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(moments.size() == 1);

   return moments.front();
}

Float64 CGirderModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(deflections.size() == 1);

   return deflections.front();
}

Float64 CGirderModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(rotations.size() == 1);

   return rotations.front();
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

Float64 CGirderModelManager::GetReaction(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, PierIndexType pierIdx, const CGirderKey& girderKey, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   REACTION r = GetReaction(intervalIdx, pfType, pierIdx, pgsTypes::stPier, girderKey, bat, resultsType);
   return r.Fy;
}

REACTION CGirderModelManager::GetReaction(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, SupportIndexType supportIdx, pgsTypes::SupportType supportType, const CGirderKey& girderKey, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   REACTION reaction;
   if ( pfType == pgsTypes::pftPostTensioning )
   {
      // Prestress and primary post-tensioning don't cause reactions
      // they only cause direction axial compression and bending
      return reaction;
   }

   ConfigureLBAMPoisForReactions(girderKey, supportIdx, supportType);

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<IResult3Ds> results;

   ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
   }


   CollectionIndexType nResults;
   results->get_Count(&nResults);
   for ( CollectionIndexType i = 0; i < nResults; i++ )
   {
      CComPtr<IResult3D> result;
      results->get_Item(i,&result);

      Float64 fx, fy, mz;
      result->get_X(&fx);
      result->get_Y(&fy);
      result->get_Z(&mz);

      reaction.Fx += fx;
      reaction.Fy += fy;
      reaction.Mz += mz;
   }


   return reaction;
}

REACTION CGirderModelManager::GetBearingReaction(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, SupportIDType supportID, const CGirderKey& girderKey, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   REACTION reaction;
   if (pfType == pgsTypes::pftPostTensioning)
   {
      // Prestress and primary post-tensioning don't cause reactions
      // they only cause direction axial compression and bending
      return reaction;
   }

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(supportID);

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey), bat);

   CComBSTR bstrLoadGroup(GetLoadGroupName(pfType));
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   CComPtr<IResult3Ds> results;

   ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

   if (bat == pgsTypes::MinSimpleContinuousEnvelope)
   {
      CAnalysisResult ar(_T(__FILE__), __LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &results);
   }
   else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
   {
      CAnalysisResult ar(_T(__FILE__), __LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__), __LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeReactions(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &results);
   }


   CollectionIndexType nResults;
   results->get_Count(&nResults);
   for (CollectionIndexType i = 0; i < nResults; i++)
   {
      CComPtr<IResult3D> result;
      results->get_Item(i, &result);

      Float64 fx, fy, mz;
      result->get_X(&fx);
      result->get_Y(&fy);
      result->get_Z(&mz);

      reaction.Fx += fx;
      reaction.Fy += fy;
      reaction.Mz += mz;
   }


   return reaction;
}

void CGirderModelManager::GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pPmin,Float64* pPmax,VehicleIndexType* pPminTruck,VehicleIndexType* pPmaxTruck) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Pmin, Pmax;
   std::vector<VehicleIndexType> PminTruck, PmaxTruck;

   GetLiveLoadAxial(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Pmin,&Pmax,pPminTruck ? &PminTruck : nullptr, pPmaxTruck ? &PmaxTruck : nullptr);

   ATLASSERT(Pmin.size() == 1);
   ATLASSERT(Pmax.size() == 1);

   *pPmin = Pmin[0];
   *pPmax = Pmax[0];

   if ( pPminTruck )
   {
      if ( 0 < PminTruck.size() )
      {
         *pPminTruck = PminTruck[0];
      }
      else
      {
         *pPminTruck = INVALID_INDEX;
      }
   }

   if ( pPmaxTruck )
   {
      if ( 0 < PmaxTruck.size() )
      {
         *pPmaxTruck = PmaxTruck[0];
      }
      else
      {
         *pPmaxTruck = INVALID_INDEX;
      }
   }
}

void CGirderModelManager::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<WBFL::System::SectionValue> Vmin, Vmax;
   std::vector<VehicleIndexType> MminTruck, MmaxTruck;
   GetLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Vmin,&Vmax,pMminTruck ? &MminTruck : nullptr, pMmaxTruck ? &MmaxTruck : nullptr);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pVmin = Vmin[0];
   *pVmax = Vmax[0];

   if ( pMminTruck )
   {
      if ( 0 < MminTruck.size() )
      {
         *pMminTruck = MminTruck[0];
      }
      else
      {
         *pMminTruck = INVALID_INDEX;
      }
   }

   if ( pMmaxTruck )
   {
      if ( 0 < MmaxTruck.size() )
      {
         *pMmaxTruck = MmaxTruck[0];
      }
      else
      {
         *pMmaxTruck = INVALID_INDEX;
      }
   }
}

void CGirderModelManager::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   std::vector<VehicleIndexType> MminTruck, MmaxTruck;

   GetLiveLoadMoment(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Mmin,&Mmax,pMminTruck ? &MminTruck : nullptr, pMmaxTruck ? &MmaxTruck : nullptr);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMmin = Mmin[0];
   *pMmax = Mmax[0];

   if ( pMminTruck )
   {
      if ( 0 < MminTruck.size() )
      {
         *pMminTruck = MminTruck[0];
      }
      else
      {
         *pMminTruck = INVALID_INDEX;
      }
   }

   if ( pMmaxTruck )
   {
      if ( 0 < MmaxTruck.size() )
      {
         *pMmaxTruck = MmaxTruck[0];
      }
      else
      {
         *pMmaxTruck = INVALID_INDEX;
      }
   }
}

void CGirderModelManager::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   std::vector<VehicleIndexType> DminTruck, DmaxTruck;
   GetLiveLoadDeflection(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Dmin,&Dmax,&DminTruck,&DmaxTruck);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);


   *pDmin = Dmin[0];
   *pDmax = Dmax[0];

   if ( pMinConfig )
   {
      if ( 0 < DminTruck.size() )
      {
         *pMinConfig = DminTruck[0];
      }
      else
      {
         *pMinConfig = INVALID_INDEX;
      }
   }

   if ( pMaxConfig )
   {
      if ( 0 < DmaxTruck.size() )
      {
         *pMaxConfig = DmaxTruck[0];
      }
      else
      {
         *pMaxConfig = INVALID_INDEX;
      }
   }
}

void CGirderModelManager::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   std::vector<VehicleIndexType> RminTruck, RmaxTruck;
   GetLiveLoadRotation(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&RminTruck,&RmaxTruck);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin[0];
   *pRmax = Rmax[0];

   if ( pMinConfig )
   {
      if ( 0 < RminTruck.size() )
      {
         *pMinConfig = RminTruck[0];
      }
      else
      {
         *pMinConfig = INVALID_INDEX;
      }
   }

   if ( pMaxConfig )
   {
      if ( 0 < RmaxTruck.size() )
      {
         *pMaxConfig = RmaxTruck[0];
      }
      else
      {
         *pMaxConfig = INVALID_INDEX;
      }
   }
}

void CGirderModelManager::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   Float64 Rmin, Rmax;
   GetLiveLoadRotation(intervalIdx,llType,pier,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,&Rmin,&Rmax,pMinConfig,pMaxConfig);
}

void CGirderModelManager::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }

   // need the POI where the girder intersects the pier
   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest poi = pPoi->GetPierPointOfInterest(girderKey,pier);

   PoiList vPoi;
   vPoi.push_back(poi);

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);


   Float64 TzMaxLeft, TzMaxRight;
   CComPtr<ILiveLoadConfiguration> TzMaxConfig;
   maxResults->GetResult(0,&TzMaxLeft,&TzMaxConfig,&TzMaxRight,nullptr);

   Float64 TzMinLeft, TzMinRight;
   CComPtr<ILiveLoadConfiguration> TzMinConfig;
   minResults->GetResult(0,&TzMinLeft,&TzMinConfig,&TzMinRight,nullptr);

   if ( pMaxConfig )
   {
      if ( TzMaxConfig )
      {
         VehicleIndexType vehicleIdx;
         TzMaxConfig->get_VehicleIndex(&vehicleIdx);
         *pMaxConfig = vehicleIdx;
      }
      else
      {
         *pMaxConfig = INVALID_INDEX;
      }
   }

   if ( pMinConfig )
   {
      if ( TzMinConfig )
      {
         VehicleIndexType vehicleIdx;
         TzMinConfig->get_VehicleIndex(&vehicleIdx);
         *pMinConfig = vehicleIdx;
      }
      else
      {
         *pMinConfig = INVALID_INDEX;
      }
   }

   *pTmin = TzMinLeft;
   *pTmax = TzMaxLeft;

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(pier);
   if ( pRmax )
   {
      if ( TzMaxConfig )
      {
         // get reaction that corresponds to T max
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         CComPtr<IResult3Ds> results;
         TzMaxConfig->put_ForceEffect(fetFy);
         TzMaxConfig->put_Optimization(optMaximize);
         response->ComputeReactions( m_LBAMPoi, bstrStage, TzMaxConfig, &results );

         CComPtr<IResult3D> result;
         results->get_Item(0,&result);

         Float64 R;
         result->get_Y(&R);
         *pRmax = R;
      }
      else
      {
         *pRmax = -1;
      }
   }

   if ( pRmin )
   {
      if ( TzMinConfig )
      {
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         // get reaction that corresponds to T min
         CComPtr<IResult3Ds> results;
         TzMinConfig->put_ForceEffect(fetFy);
         TzMinConfig->put_Optimization(optMaximize);
         response->ComputeReactions( m_LBAMPoi, bstrStage, TzMinConfig, &results );

         CComPtr<IResult3D> result;
         results->get_Item(0,&result);

         Float64 R;
         result->get_Y(&R);
         *pRmin = R;
      }
      else
      {
         *pRmin = -1;
      }
   }
}

void CGirderModelManager::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig,VehicleIndexType* pTopMaxConfig,VehicleIndexType* pBotMinConfig,VehicleIndexType* pBotMaxConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   std::vector<VehicleIndexType> topMinConfig, topMaxConfig, botMinConfig, botMaxConfig;
   GetLiveLoadStress(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax,&topMinConfig, &topMaxConfig, &botMinConfig, &botMaxConfig);

   ATLASSERT(fTopMin.size() == 1);
   ATLASSERT(fTopMax.size() == 1);
   ATLASSERT(fBotMin.size() == 1);
   ATLASSERT(fBotMax.size() == 1);
   ATLASSERT(topMinConfig.size() == 1);
   ATLASSERT(topMaxConfig.size() == 1);
   ATLASSERT(botMinConfig.size() == 1);
   ATLASSERT(botMaxConfig.size() == 1);

   *pfTopMin = fTopMin[0];
   *pfTopMax = fTopMax[0];
   *pfBotMin = fBotMin[0];
   *pfBotMax = fBotMax[0];

   if ( pTopMinConfig )
   {
      *pTopMinConfig = topMinConfig[0];
   }

   if ( pTopMaxConfig )
   {
      *pTopMaxConfig = topMaxConfig[0];
   }

   if ( pBotMinConfig )
   {
      *pBotMinConfig = botMinConfig[0];
   }

   if ( pBotMaxConfig )
   {
      *pBotMaxConfig = botMaxConfig[0];
   }
}

void CGirderModelManager::GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pPmin,Float64* pPmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   
   std::vector<Float64> Pmin, Pmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadAxial(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,
                             &Pmin,&Pmax,
                             pMinAxleConfig ? &minAxleConfig : nullptr,
                             pMaxAxleConfig ? &maxAxleConfig : nullptr);

   ATLASSERT(Pmin.size() == 1);
   ATLASSERT(Pmax.size() == 1);

   *pPmin = Pmin[0];
   *pPmax = Pmax[0];

   if ( pMinAxleConfig )
   {
      *pMinAxleConfig = minAxleConfig[0];
   }

   if ( pMaxAxleConfig )
   {
      *pMaxAxleConfig = maxAxleConfig[0];
   }
}

void CGirderModelManager::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,
                               AxleConfiguration* pMinLeftAxleConfig,
                               AxleConfiguration* pMinRightAxleConfig,
                               AxleConfiguration* pMaxLeftAxleConfig,
                               AxleConfiguration* pMaxRightAxleConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<WBFL::System::SectionValue> Vmin, Vmax;
   std::vector<AxleConfiguration> minLeftAxleConfig,minRightAxleConfig,maxLeftAxleConfig,maxRightAxleConfig;
   GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIdx,vPoi,bat,
                             bIncludeImpact, bIncludeLLDF,&Vmin,&Vmax,
                             pMinLeftAxleConfig  ? &minLeftAxleConfig  : nullptr,
                             pMinRightAxleConfig ? &minRightAxleConfig : nullptr,
                             pMaxLeftAxleConfig  ? &maxLeftAxleConfig  : nullptr,
                             pMaxRightAxleConfig ? &maxRightAxleConfig : nullptr);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pVmin = Vmin[0];
   *pVmax = Vmax[0];

   if ( pMinLeftAxleConfig )
   {
      *pMinLeftAxleConfig = minLeftAxleConfig[0];
   }

   if ( pMinRightAxleConfig )
   {
      *pMinRightAxleConfig = minRightAxleConfig[0];
   }

   if ( pMaxLeftAxleConfig )
   {
      *pMaxLeftAxleConfig = maxLeftAxleConfig[0];
   }

   if ( pMaxRightAxleConfig )
   {
      *pMaxRightAxleConfig = maxRightAxleConfig[0];
   }
}

void CGirderModelManager::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   
   std::vector<Float64> Mmin, Mmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,
                              &Mmin,&Mmax,
                              pMinAxleConfig ? &minAxleConfig : nullptr,
                              pMaxAxleConfig ? &maxAxleConfig : nullptr);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMmin = Mmin[0];
   *pMmax = Mmax[0];

   if ( pMinAxleConfig )
   {
      *pMinAxleConfig = minAxleConfig[0];
   }

   if ( pMaxAxleConfig )
   {
      *pMaxAxleConfig = maxAxleConfig[0];
   }
}

void CGirderModelManager::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIdx,vPoi,bat,
                                    bIncludeImpact, bIncludeLLDF,&Dmin,&Dmax,
                                    pMinAxleConfig ? &minAxleConfig : nullptr,
                                    pMaxAxleConfig ? &maxAxleConfig : nullptr);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pDmin = Dmin[0];
   *pDmax = Dmax[0];

   if ( pMinAxleConfig )
   {
      *pMinAxleConfig = minAxleConfig[0];
   }

   if ( pMaxAxleConfig )
   {
      *pMaxAxleConfig = maxAxleConfig[0];
   }
}

void CGirderModelManager::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact, bIncludeLLDF,
                                &Rmin,&Rmax,
                                pMinAxleConfig ? &minAxleConfig : nullptr,
                                pMaxAxleConfig ? &maxAxleConfig : nullptr);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin[0];
   *pRmax = Rmax[0];

   if ( pMinAxleConfig )
   {
      *pMinAxleConfig = minAxleConfig[0];
   }

   if ( pMaxAxleConfig )
   {
      *pMaxAxleConfig = maxAxleConfig[0];
   }
}

void CGirderModelManager::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop,AxleConfiguration* pMaxAxleConfigTop,AxleConfiguration* pMinAxleConfigBot,AxleConfiguration* pMaxAxleConfigBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   std::vector<AxleConfiguration> minAxleConfigTop,maxAxleConfigTop,minAxleConfigBot,maxAxleConfigBot;
   GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax,&minAxleConfigTop,&maxAxleConfigTop,&minAxleConfigBot,&maxAxleConfigBot);

   ATLASSERT(fTopMin.size() == 1);
   ATLASSERT(fTopMax.size() == 1);
   ATLASSERT(fBotMin.size() == 1);
   ATLASSERT(fBotMax.size() == 1);

   *pfTopMin = fTopMin[0];
   *pfTopMax = fTopMax[0];
   *pfBotMin = fBotMin[0];
   *pfBotMax = fBotMax[0];

   if ( pMinAxleConfigTop )
   {
      *pMinAxleConfigTop = minAxleConfigTop[0];
   }

   if ( pMaxAxleConfigTop )
   {
      *pMaxAxleConfigTop = maxAxleConfigTop[0];
   }

   if ( pMinAxleConfigBot )
   {
      *pMinAxleConfigBot = minAxleConfigBot[0];
   }

   if ( pMaxAxleConfigBot )
   {
      *pMaxAxleConfigBot = maxAxleConfigBot[0];
   }
}

void CGirderModelManager::GetDeflLiveLoadDeflection(IProductForces::DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const
{
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      bat = pgsTypes::SimpleSpan;
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      bat = pgsTypes::ContinuousSpan;
   }

   ATLASSERT(bat == pgsTypes::SimpleSpan || bat == pgsTypes::ContinuousSpan);
   // this are the only 2 value analysis types for this function

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   CComBSTR bstrStageName( GetLBAMStageName(liveLoadIntervalIdx) );

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(segmentKey),bat);


   // make sure there are actually loads applied
   CComPtr<ILiveLoad> live_load;
   if ( bat == pgsTypes::SimpleSpan )
   {
      pModelData->m_Model->get_LiveLoad(&live_load);
   }
   else
   {
      pModelData->m_ContinuousModel->get_LiveLoad(&live_load);
   }

   CComPtr<ILiveLoadModel> live_load_model;
   live_load->get_Deflection(&live_load_model);

   CComPtr<IVehicularLoads> vehicular_loads;
   live_load_model->get_VehicularLoads(&vehicular_loads);

   VehicleIndexType nVehicularLoads;
   vehicular_loads->get_Count(&nVehicularLoads);

   if ( nVehicularLoads == 0 )
   {
      *pDmin = 0;
      *pDmax = 0;
      return;
   }


   PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
   if ( poi_id == INVALID_ID )
   {
      poi_id = AddPointOfInterest( pModelData, poi );
      ATLASSERT( 0 <= poi_id ); // if this fires, the poi wasn't added... WHY???
   }

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(poi_id);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   if (type==IProductForces::DeflectionLiveLoadEnvelope)
   {
      ar = pModelData->pDeflLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, 
             fetDy, optMaximize, vlcDefault, VARIANT_TRUE,VARIANT_TRUE, VARIANT_FALSE,&maxResults);

      ar = pModelData->pDeflLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, 
             fetDy, optMinimize, vlcDefault, VARIANT_TRUE,VARIANT_TRUE, VARIANT_FALSE,&minResults);
   }
   else
   {
      ATLASSERT(type==IProductForces::DesignTruckAlone || type==IProductForces::Design25PlusLane);

      // Assumes VehicularLoads are put in in same order as enum;
      VehicleIndexType trk_idx = (VehicleIndexType)type;

      ar = pModelData->pDeflEnvelopedVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, trk_idx,
             fetDy, optMaximize, vlcDefault, VARIANT_TRUE,dftSingleLane, VARIANT_FALSE,&maxResults);

      ar = pModelData->pDeflEnvelopedVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, trk_idx,
             fetDy, optMinimize, vlcDefault, VARIANT_TRUE,dftSingleLane, VARIANT_FALSE,&minResults);
   }

   Float64 DyMaxLeft, DyMaxRight;
   maxResults->GetResult(0,&DyMaxLeft,nullptr,&DyMaxRight,nullptr);

   Float64 DyMinLeft, DyMinRight;
   minResults->GetResult(0,&DyMinLeft,nullptr,&DyMinRight,nullptr);

   *pDmin = DyMinLeft;
   *pDmax = DyMaxLeft;
}

void CGirderModelManager::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation,Float64* pftop,Float64* pfbot) const
{
   // This is sort of a dummy function until deck shrinkage stress issues are resolved.
   // If you count on deck shrinkage for elastic gain, then you have to account for the fact
   // that the deck shrinkage changes the stresses in the girder as well. Deck shrinkage is
   // an external load to the girder

   // Top and bottom girder stresses are computed using the composite section method described in
   // Branson, D. E., "Time-Dependent Effects in Composite Concrete Beams", 
   // American Concrete Institute J., Vol 61, Issue 2, (1964) pp. 213-230

   GET_IFACE(IBridge, pBridge);
   if (IsNonstructuralDeck(pBridge->GetDeckType()))
   {
      // no deck, no deck shrinkage stresses
      *pftop = 0;
      *pfbot = 0;
      return;
   }

   VERIFY_ANALYSIS_TYPE;

   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
   ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,INVALID_INDEX);

   Float64 P, M;
   pDetails->pLosses->GetDeckShrinkageEffects(&P,&M);

   // Tricky: Eccentricity of deck changes with fc, so we need to recompute M
   GET_IFACE(ISectionProperties,pProps);
   Float64 ed  = pProps->GetY( compositeIntervalIdx, poi, pgsTypes::TopGirder, fcGdr ) 
               + pBridge->GetGrossSlabDepth(poi) / 2; // use gross depth because shrinkage occurs at early age before sacrafical wearing surface is worn off
   ed *= -1;

   M = P * ed;

   Float64 A  = pProps->GetAg(compositeIntervalIdx,poi,fcGdr);
   Float64 St = pProps->GetS(compositeIntervalIdx,poi,topStressLocation,fcGdr);
   Float64 Sb = pProps->GetS(compositeIntervalIdx,poi,botStressLocation,fcGdr);

   *pftop = P/A + M/St;
   *pfbot = P/A + M/Sb;
}

void CGirderModelManager::GetDeckShrinkageStresses(const pgsPointOfInterest& poi, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation, Float64* pftop, Float64* pfbot) const
{
   // This is sort of a dummy function until deck shrinkage stress issues are resolved.
   // If you count on deck shrinkage for elastic gain, then you have to account for the fact
   // that the deck shrinkage changes the stresses in the girder as well. Deck shrinkage is
   // an external load to the girder

   // Top and bottom girder stresses are computed using the composite section method described in
   // Branson, D. E., "Time-Dependent Effects in Composite Concrete Beams", 
   // American Concrete Institute J., Vol 61, Issue 2, (1964) pp. 213-230
   GET_IFACE(IBridge, pBridge);
   if (IsNonstructuralDeck(pBridge->GetDeckType()) || !pBridge->IsCompositeDeck())
   {
      // no deck or deck isn't composite with girder, no deck shrinkage stresses
      *pftop = 0;
      *pfbot = 0;
      return;
   }

   VERIFY_ANALYSIS_TYPE;

   GET_IFACE(IPointOfInterest, pPoi);
   if (pPoi->IsOnSegment(poi))
   {
      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType compositeIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

      GET_IFACE(ILosses, pLosses);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, INVALID_INDEX);

      Float64 P, M;
      pDetails->pLosses->GetDeckShrinkageEffects(&P, &M);

      GET_IFACE(ISectionProperties, pProps);
      Float64 A = pProps->GetAg(compositeIntervalIdx, poi);
      Float64 St = pProps->GetS(compositeIntervalIdx, poi, topStressLocation);
      Float64 Sb = pProps->GetS(compositeIntervalIdx, poi, botStressLocation);

      Float64 n_top = 1.0; // modular ratio for top stress calculation
      Float64 n_bot = 1.0; // modular ratio for bottom stress calculation
      if (IsDeckStressLocation(topStressLocation) || IsDeckStressLocation(botStressLocation))
      {
         // if one of the stress locations is for the deck, then we need to adjust the properties with the modular ratio
         // note that St and Sb are already adjusted, but Area is not
         GET_IFACE(ILossParameters, pLossParams);
         bool bIsTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

         GET_IFACE(IMaterials, pMaterials);
         Float64 Eg = (bIsTimeStepAnalysis ? pMaterials->GetSegmentAgeAdjustedEc(poi.GetSegmentKey(), compositeIntervalIdx) : pMaterials->GetSegmentEc(poi.GetSegmentKey(), compositeIntervalIdx));
         Float64 Ed = (bIsTimeStepAnalysis ? pMaterials->GetDeckAgeAdjustedEc(deckCastingRegionIdx, compositeIntervalIdx) : pMaterials->GetDeckEc(deckCastingRegionIdx, compositeIntervalIdx));
         Float64 n = Ed / Eg;

         if (IsDeckStressLocation(topStressLocation))
            n_top = n;

         if (IsDeckStressLocation(botStressLocation))
            n_bot = n;
      }

      *pftop = n_top*P/A + M/St;
      *pfbot = n_bot*P/A + M/Sb;
   }
   else
   {
      *pftop = 0.0;
      *pfbot = 0.0;
   }
}

///////////////////////////////////////////////////////
// IProductForces2
std::vector<Float64> CGirderModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   ATLASSERT(pfType != pgsTypes::pftPostTensioning);
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   GET_IFACE(IPointOfInterest,pPoi);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );

   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetFx][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFx][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }


   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 FxLeft, FxRight;
      result->get_XLeft(&FxLeft);
      result->get_XRight(&FxRight);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 Fx;
      if ( IsZero(Xg) )
      {
         Fx = -FxRight; // use right side result at start of span
      }
      else
      {
         Fx = FxLeft; // use left side result at all other locations
      }

      results.push_back(Fx);
      idx++;
   }

   return results;
}

std::vector<WBFL::System::SectionValue> CGirderModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPostTensioning);
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<WBFL::System::SectionValue> results;
   results.reserve(vPoi.size());

   // after erection - results are in the girder models
   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadGroup(GetLoadGroupName(pfType));

   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }

   IndexType nResults;
   section_results->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 FyLeft, FyRight;
      result->get_YLeft(&FyLeft);
      result->get_YRight(&FyRight);

      WBFL::System::SectionValue V(-FyLeft,FyRight);

      results.push_back(V);
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPostTensioning);
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   GET_IFACE(IPointOfInterest,pPoi);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );

   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }


   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 MzLeft, MzRight;
      result->get_ZLeft(&MzLeft);
      result->get_ZRight(&MzRight);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 Mz;
      if ( IsZero(Xg) )
      {
         Mz = -MzRight; // use right side result at start of span
      }
      else
      {
         Mz = MzLeft; // use left side result at all other locations
      }

      results.push_back(Mz);

      idx++;
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   ATLASSERT(VerifyPoi(vPoi));

#if defined _DEBUG
/*
   if (pfType == pgsTypes::pftGirder)
   {
      // Girder deflection requests must be incremental
      // Cummulative results are incorrect due to how the LBAM is constructed
      // and changes in modulus of elasticity and boundary conditions between
      // the beginning of storage and erection
      ATLASSERT(resultsType == rtIncremental);
   }
*/
#endif

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   GET_IFACE_NOCHECK(IMaterials,pMaterials);
   GET_IFACE_NOCHECK(ISectionProperties,pSectProps);

   if ( pfType == pgsTypes::pftPostTensioning )
   {
      // Post-tensioning isn't the the LBAM. Since we are modeling secondary effects by applying direct
      // curvatures to the model as the secondary effects product load type, the post-tension deflection
      // is the deflections computed by the secondary effects load case
      pfType = pgsTypes::pftSecondaryEffects;
   }

   CComBSTR bstrLoadGroup = GetLoadGroupName(pfType);
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetDy][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
   {
      CAnalysisResult ar(_T(__FILE__), __LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetDy][optMaximize]->ComputeDeflections(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__), __LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &section_results);
   }

   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx, &result);

      Float64 Dyl, Dyr;
      result->get_YLeft(&Dyl);
      result->get_YRight(&Dyr);
      ATLASSERT(IsEqual(Dyl, Dyr));
      results.push_back(Dyl);
      idx++;
   }
   return results;
}

std::vector<Float64> CGirderModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension); // pretension results are obtained from the segment models
   ATLASSERT(VerifyPoi(vPoi));

#if defined _DEBUG
   if (pfType == pgsTypes::pftGirder)
   {
      // Girder deflection requests must be incremental
      // Cummulative results are incorrect due to how the LBAM is constructed
      // and changes in modulus of elasticity and boundary conditions between
      // the beginning of storage and erection
      ATLASSERT(resultsType == rtIncremental);
   }
#endif

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   GET_IFACE_NOCHECK(IMaterials,pMaterials);
   GET_IFACE_NOCHECK(ISectionProperties,pSectProps);

   if ( pfType == pgsTypes::pftPostTensioning )
   {
      // Post-tensioning isn't the the LBAM. Since we are modeling secondary effects by applying direct
      // curvatures to the model as the secondary effects product load type, the post-tension deflection
      // is the deflections computed by the secondary effects load case
      pfType = pgsTypes::pftSecondaryEffects;
   }

   CComBSTR bstrLoadGroup = GetLoadGroupName(pfType);
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetRz][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetRz][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }

   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 Rzl, Rzr;
      result->get_ZLeft(&Rzl);
      result->get_ZRight(&Rzr);
      ATLASSERT(IsEqual(Rzl,Rzr));
      results.push_back(Rzl);
      idx++;
   }

   return results;
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   VERIFY_ANALYSIS_TYPE;
   ATLASSERT(VerifyPoi(vPoi));

   ATLASSERT(pfType != pgsTypes::pftPretension && pfType != pgsTypes::pftPostTensioning && pfType != pgsTypes::pftSecondaryEffects);

   pfTop->reserve(vPoi.size());
   pfBot->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   CComPtr<ISectionStressResults> min_results, max_results, results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &max_results);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &min_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &max_results);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &min_results);
   }
   else
   {
      ar = pModelData->pLoadGroupResponse[bat]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &results);
   }

   CollectionIndexType stress_point_index_top = GetStressPointIndex(topLocation);
   CollectionIndexType stress_point_index_bot = GetStressPointIndex(botLocation);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 fTop, fBot;

      if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
      {
         CComPtr<ISectionStressResult> top_stresses;
         max_results->get_Item(idx,&top_stresses);

         CComPtr<ISectionStressResult> bot_stresses;
         min_results->get_Item(idx,&bot_stresses);

         if ( IsZero(Xg) )
         {
            top_stresses->GetRightResult(stress_point_index_top,&fTop);
            bot_stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            top_stresses->GetLeftResult(stress_point_index_top,&fTop);
            bot_stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }
      else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
      {
         CComPtr<ISectionStressResult> top_stresses;
         min_results->get_Item(idx,&top_stresses);

         CComPtr<ISectionStressResult> bot_stresses;
         max_results->get_Item(idx,&bot_stresses);

         if ( IsZero(Xg) )
         {
            top_stresses->GetRightResult(stress_point_index_top,&fTop);
            bot_stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            top_stresses->GetLeftResult(stress_point_index_top,&fTop);
            bot_stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }
      else
      {
         CComPtr<ISectionStressResult> stresses;
         results->get_Item(idx,&stresses);

         if ( IsZero(Xg) )
         {
            stresses->GetRightResult(stress_point_index_top,&fTop);
            stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            stresses->GetLeftResult(stress_point_index_top,&fTop);
            stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);

      idx++;
   }
}

void CGirderModelManager::GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pPmin,std::vector<Float64>* pPmax,std::vector<VehicleIndexType>* pPminTruck,std::vector<VehicleIndexType>* pPmaxTruck) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }
   
   pPmin->clear();
   pPmax->clear();

   pPmin->reserve(vPoi.size());
   pPmax->reserve(vPoi.size());

   if ( pPminTruck )
   {
      pPminTruck->clear();
      pPminTruck->reserve(vPoi.size());
   }

   if ( pPmaxTruck )
   {
      pPmaxTruck->clear();
      pPmaxTruck->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));;

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFx, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFx, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);


   GET_IFACE(IPointOfInterest,pPoi);
   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 FxMaxLeft, FxMaxRight;
      CComPtr<ILiveLoadConfiguration> FxMaxLeftConfig, FxMaxRightConfig;

      maxResults->GetResult(idx,
                            &FxMaxLeft, pPmaxTruck ? &FxMaxLeftConfig  : nullptr,
                            &FxMaxRight,pPmaxTruck ? &FxMaxRightConfig : nullptr);

      Float64 FxMinLeft, FxMinRight;
      CComPtr<ILiveLoadConfiguration> FxMinLeftConfig, FxMinRightConfig;
      minResults->GetResult(idx,
                            &FxMinLeft,  pPminTruck ? &FxMinLeftConfig  : nullptr,
                            &FxMinRight, pPminTruck ? &FxMinRightConfig : nullptr);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
      if ( IsZero(Xg) )
      {
         pPmin->push_back( -FxMinRight );

         if ( pPminTruck && FxMinRightConfig )
         {
            VehicleIndexType vehicleIdx;
            FxMinRightConfig->get_VehicleIndex(&vehicleIdx);
            pPminTruck->push_back(vehicleIdx);
         }

         pPmax->push_back( -FxMaxRight );

         if ( pPmaxTruck && FxMaxRightConfig )
         {
            VehicleIndexType vehicleIdx;
            FxMaxRightConfig->get_VehicleIndex(&vehicleIdx);
            pPmaxTruck->push_back(vehicleIdx);
         }
      }
      else
      {
         pPmin->push_back( FxMinLeft );

         if ( pPminTruck && FxMinLeftConfig )
         {
            VehicleIndexType vehicleIdx;
            FxMinLeftConfig->get_VehicleIndex(&vehicleIdx);
            pPminTruck->push_back(vehicleIdx);
         }

         pPmax->push_back( FxMaxLeft );

         if ( pPmaxTruck && FxMaxLeftConfig )
         {
            VehicleIndexType vehicleIdx;
            FxMaxLeftConfig->get_VehicleIndex(&vehicleIdx);
            pPmaxTruck->push_back(vehicleIdx);
         }
      }
      idx++;
   } // next poi
}

void CGirderModelManager::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }

   pVmin->clear();
   pVmax->clear();
   
   pVmin->reserve(vPoi.size());
   pVmax->reserve(vPoi.size());

   if ( pMminTruck )
   {
      pMminTruck->clear();
      pMminTruck->reserve(vPoi.size());
   }

   if ( pMmaxTruck )
   {
      pMmaxTruck->clear();
      pMmaxTruck->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   // TRICKY:
   // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
   // that the optimization must go the opposite way as well when using the envelopers
   if (bat == pgsTypes::MinSimpleContinuousEnvelope)
   {
      bat = pgsTypes::MaxSimpleContinuousEnvelope;
   }
   else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
   {
      bat = pgsTypes::MinSimpleContinuousEnvelope;
   }

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFy, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFy, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 FyMaxLeft, FyMaxRight;
      CComPtr<ILiveLoadConfiguration> FyMaxLeftConfig, FyMaxRightConfig;
      maxResults->GetResult(idx,&FyMaxLeft, pMminTruck ? &FyMaxLeftConfig  : nullptr,
                                &FyMaxRight,pMminTruck ? &FyMaxRightConfig : nullptr);

      Float64 FyMinLeft, FyMinRight;
      CComPtr<ILiveLoadConfiguration> FyMinLeftConfig, FyMinRightConfig;
      minResults->GetResult(idx,&FyMinLeft, pMmaxTruck ? &FyMinLeftConfig  : nullptr,
                                &FyMinRight,pMmaxTruck ? &FyMinRightConfig : nullptr);

      WBFL::System::SectionValue minValues(-FyMaxLeft,FyMaxRight);
      WBFL::System::SectionValue maxValues(-FyMinLeft,FyMinRight);

      pVmin->push_back( minValues );
      pVmax->push_back( maxValues );

      if ( pMminTruck )
      {
         if ( -FyMaxLeft < FyMaxRight && FyMaxRightConfig )
         {
            VehicleIndexType vehicleIdx;
            FyMaxRightConfig->get_VehicleIndex(&vehicleIdx);
            pMminTruck->push_back(vehicleIdx);
         }
         else if ( FyMaxLeftConfig )
         {
            VehicleIndexType vehicleIdx;
            FyMaxLeftConfig->get_VehicleIndex(&vehicleIdx);
            pMminTruck->push_back(vehicleIdx);
         }
      }


      if ( pMmaxTruck )
      {
         if ( -FyMinLeft < FyMinRight  && FyMinRightConfig )
         {
            VehicleIndexType vehicleIdx;
            FyMinRightConfig->get_VehicleIndex(&vehicleIdx);
            pMmaxTruck->push_back(vehicleIdx);
         }
         else if ( FyMinLeftConfig )
         {
            VehicleIndexType vehicleIdx;
            FyMinLeftConfig->get_VehicleIndex(&vehicleIdx);
            pMmaxTruck->push_back(vehicleIdx);
         }
      }
   }
}

void CGirderModelManager::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }

   pMmin->clear();
   pMmax->clear();

   pMmin->reserve(vPoi.size());
   pMmax->reserve(vPoi.size());

   if ( pMminTruck )
   {
      pMminTruck->clear();
      pMminTruck->reserve(vPoi.size());
   }

   if ( pMmaxTruck )
   {
      pMmaxTruck->clear();
      pMmaxTruck->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));;

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetMz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetMz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);


   GET_IFACE(IPointOfInterest,pPoi);
   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      Float64 MzMaxLeft, MzMaxRight;
      CComPtr<ILiveLoadConfiguration> MzMaxLeftConfig, MzMaxRightConfig;

      maxResults->GetResult(idx,
                            &MzMaxLeft, pMmaxTruck ? &MzMaxLeftConfig  : nullptr,
                            &MzMaxRight,pMmaxTruck ? &MzMaxRightConfig : nullptr);

      Float64 MzMinLeft, MzMinRight;
      CComPtr<ILiveLoadConfiguration> MzMinLeftConfig, MzMinRightConfig;
      minResults->GetResult(idx,
                            &MzMinLeft,  pMminTruck ? &MzMinLeftConfig  : nullptr,
                            &MzMinRight, pMminTruck ? &MzMinRightConfig : nullptr);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
      if ( IsZero(Xg) )
      {
         pMmin->push_back( -MzMinRight );

         if ( pMminTruck && MzMinRightConfig )
         {
            VehicleIndexType vehicleIdx;
            MzMinRightConfig->get_VehicleIndex(&vehicleIdx);
            pMminTruck->push_back(vehicleIdx);
         }

         pMmax->push_back( -MzMaxRight );

         if ( pMmaxTruck && MzMaxRightConfig )
         {
            VehicleIndexType vehicleIdx;
            MzMaxRightConfig->get_VehicleIndex(&vehicleIdx);
            pMmaxTruck->push_back(vehicleIdx);
         }
      }
      else
      {
         pMmin->push_back( MzMinLeft );

         if ( pMminTruck && MzMinLeftConfig )
         {
            VehicleIndexType vehicleIdx;
            MzMinLeftConfig->get_VehicleIndex(&vehicleIdx);
            pMminTruck->push_back(vehicleIdx);
         }

         pMmax->push_back( MzMaxLeft );

         if ( pMmaxTruck && MzMaxLeftConfig )
         {
            VehicleIndexType vehicleIdx;
            MzMaxLeftConfig->get_VehicleIndex(&vehicleIdx);
            pMmaxTruck->push_back(vehicleIdx);
         }
      }
      idx++; 
   } // next poi
}

void CGirderModelManager::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }

   pDmin->clear();
   pDmax->clear();

   pDmin->reserve(vPoi.size());
   pDmax->reserve(vPoi.size());

   if ( pMinConfig )
   {
      pMinConfig->clear();
      pMinConfig->reserve(vPoi.size());
   }

   if ( pMaxConfig )
   {
      pMaxConfig->clear();
      pMaxConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetDy, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetDy, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 DyMaxLeft, DyMaxRight;
      CComPtr<ILiveLoadConfiguration> DyMaxConfig;
      maxResults->GetResult(idx,&DyMaxLeft,pMaxConfig ? &DyMaxConfig : nullptr,&DyMaxRight,nullptr);

      Float64 DyMinLeft, DyMinRight;
      CComPtr<ILiveLoadConfiguration> DyMinConfig;
      minResults->GetResult(idx,&DyMinLeft,pMinConfig ? &DyMinConfig : nullptr,&DyMinRight,nullptr);

      if ( pMaxConfig && DyMaxConfig )
      {
         VehicleIndexType vehicleIdx;
         DyMaxConfig->get_VehicleIndex(&vehicleIdx);
         pMaxConfig->push_back(vehicleIdx);
      }

      if ( pMinConfig && DyMinConfig )
      {
         VehicleIndexType vehicleIdx;
         DyMinConfig->get_VehicleIndex(&vehicleIdx);
         pMinConfig->push_back(vehicleIdx);
      }

      pDmin->push_back( DyMinLeft );
      pDmax->push_back( DyMaxLeft );
   }
}

void CGirderModelManager::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }

   pRmin->clear();
   pRmax->clear();

   pRmin->reserve(vPoi.size());
   pRmax->reserve(vPoi.size());

   if ( pMinConfig )
   {
      pMinConfig->clear();
      pMinConfig->reserve(vPoi.size());
   }

   if ( pMaxConfig )
   {
      pMaxConfig->clear();
      pMaxConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 RzMaxLeft, RzMaxRight;
      CComPtr<ILiveLoadConfiguration> RzMaxConfig;
      maxResults->GetResult(idx,&RzMaxLeft,pMaxConfig ? &RzMaxConfig : nullptr,&RzMaxRight,nullptr);

      Float64 RzMinLeft, RzMinRight;
      CComPtr<ILiveLoadConfiguration> RzMinConfig;
      minResults->GetResult(idx,&RzMinLeft,pMinConfig ? &RzMinConfig : nullptr,&RzMinRight,nullptr);

      if ( pMaxConfig && RzMaxConfig )
      {
         VehicleIndexType vehicleIdx;
         RzMaxConfig->get_VehicleIndex(&vehicleIdx);
         pMaxConfig->push_back(vehicleIdx);
      }

      if ( pMinConfig && RzMinConfig )
      {
         VehicleIndexType vehicleIdx;
         RzMinConfig->get_VehicleIndex(&vehicleIdx);
         pMinConfig->push_back(vehicleIdx);
      }

      pRmin->push_back( RzMinLeft );
      pRmax->push_back( RzMaxLeft );
   }
}

void CGirderModelManager::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex,std::vector<VehicleIndexType>* pTopMaxIndex,std::vector<VehicleIndexType>* pBotMinIndex,std::vector<VehicleIndexType>* pBotMaxIndex) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // bIncludeLLDF should be true when llType is pgsTypes::lltPedestrian since we use LLDF's to set pedestrian load
   if (llType == pgsTypes::lltPedestrian)
   {
      ATLASSERT(bIncludeLLDF); // caller should know this, but force anyway
      bIncludeLLDF = true; 
   }

   GET_IFACE(IPointOfInterest,pPoi);

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   pfTopMin->reserve(vPoi.size());
   pfTopMax->reserve(vPoi.size());
   pfBotMin->reserve(vPoi.size());
   pfBotMax->reserve(vPoi.size());

   if ( pTopMinIndex )
   {
      pTopMinIndex->clear();
      pTopMinIndex->reserve(vPoi.size());
   }

   if ( pTopMaxIndex )
   {
      pTopMaxIndex->clear();
      pTopMaxIndex->reserve(vPoi.size());
   }

   if ( pBotMinIndex )
   {
      pBotMinIndex->clear();
      pBotMinIndex->reserve(vPoi.size());
   }

   if ( pBotMaxIndex )
   {
      pBotMaxIndex->clear();
      pBotMaxIndex->reserve(vPoi.size());
   }
   
   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelStressResults> minResults;
   CComPtr<ILiveLoadModelStressResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeStresses( m_LBAMPoi, bstrStage, llmt, 
          fetMz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeStresses( m_LBAMPoi, bstrStage, llmt, 
          fetMz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

   CollectionIndexType top_stress_point_index = GetStressPointIndex(topLocation);
   CollectionIndexType bot_stress_point_index = GetStressPointIndex(botLocation);

   IndexType idx = 0;
   for(const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<IStressResult> fLeftMax, fRightMax;
      CComPtr<ILiveLoadConfiguration> fLeftMaxConfig, fRightMaxConfig;
      maxResults->GetResult(idx,&fLeftMax,pTopMaxIndex || pBotMaxIndex ? &fLeftMaxConfig : nullptr,&fRightMax,pTopMaxIndex || pBotMaxIndex ? &fRightMaxConfig : nullptr);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      CComPtr<ILiveLoadConfiguration> fLeftMinConfig, fRightMinConfig;
      minResults->GetResult(idx,&fLeftMin,pTopMinIndex || pBotMinIndex ? &fLeftMinConfig : nullptr,&fRightMin,pTopMinIndex || pBotMinIndex ? &fRightMinConfig : nullptr);

      Float64 fBotMax, fBotMin, fTopMax, fTopMin;

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      if ( IsZero(Xg) )
      {
         fRightMax->GetResult(bot_stress_point_index,&fBotMax);
         fRightMax->GetResult(top_stress_point_index,&fTopMax);
         fRightMin->GetResult(bot_stress_point_index,&fBotMin);
         fRightMin->GetResult(top_stress_point_index,&fTopMin);

         if ( pTopMinIndex )
         {
            VehicleIndexType vehicleIdx;
            fRightMaxConfig->get_VehicleIndex(&vehicleIdx);
            pTopMinIndex->push_back(vehicleIdx);
         }

         if ( pTopMaxIndex )
         {
            VehicleIndexType vehicleIdx;
            fRightMinConfig->get_VehicleIndex(&vehicleIdx);
            pTopMaxIndex->push_back(vehicleIdx);
         }

         if ( pBotMinIndex )
         {
            VehicleIndexType vehicleIdx;
            fRightMinConfig->get_VehicleIndex(&vehicleIdx);
            pBotMinIndex->push_back(vehicleIdx);
         }

         if ( pBotMaxIndex )
         {
            VehicleIndexType vehicleIdx;
            fRightMaxConfig->get_VehicleIndex(&vehicleIdx);
            pBotMaxIndex->push_back(vehicleIdx);
         }
      }
      else
      {
         fLeftMax->GetResult(bot_stress_point_index,&fBotMax);
         fLeftMax->GetResult(top_stress_point_index,&fTopMax);
         fLeftMin->GetResult(bot_stress_point_index,&fBotMin);
         fLeftMin->GetResult(top_stress_point_index,&fTopMin);

         if ( pTopMinIndex )
         {
            VehicleIndexType vehicleIdx;
            fLeftMaxConfig->get_VehicleIndex(&vehicleIdx);
            pTopMinIndex->push_back(vehicleIdx);
         }

         if ( pTopMaxIndex )
         {
            VehicleIndexType vehicleIdx;
            fLeftMinConfig->get_VehicleIndex(&vehicleIdx);
            pTopMaxIndex->push_back(vehicleIdx);
         }

         if ( pBotMinIndex )
         {
            VehicleIndexType vehicleIdx;
            fLeftMinConfig->get_VehicleIndex(&vehicleIdx);
            pBotMinIndex->push_back(vehicleIdx);
         }

         if ( pBotMaxIndex )
         {
            VehicleIndexType vehicleIdx;
            fLeftMaxConfig->get_VehicleIndex(&vehicleIdx);
            pBotMaxIndex->push_back(vehicleIdx);
         }
      }

      if ( fTopMax < fTopMin )
      {
         std::swap(fTopMax,fTopMin);

         if ( pTopMinIndex && pTopMaxIndex )
         {
            std::swap(pTopMaxIndex->back(),pTopMinIndex->back());
         }
         else if ( pTopMinIndex && !pTopMaxIndex )
         {
            pTopMinIndex->push_back(pTopMaxIndex->back());
            pTopMaxIndex->pop_back();
         }
         else if ( pTopMaxIndex && !pTopMinIndex )
         {
            pTopMaxIndex->push_back(pTopMinIndex->back());
            pTopMinIndex->pop_back();
         }
      }

      if ( fBotMax < fBotMin )
      {
         std::swap(fBotMax,fBotMin);

         if ( pBotMinIndex && pBotMaxIndex )
         {
            std::swap(pBotMaxIndex->back(),pBotMinIndex->back());
         }
         else if ( pBotMinIndex && !pBotMaxIndex )
         {
            pBotMinIndex->push_back(pBotMaxIndex->back());
            pBotMaxIndex->pop_back();
         }
         else if ( pBotMaxIndex && !pBotMinIndex )
         {
            pBotMaxIndex->push_back(pBotMinIndex->back());
            pBotMinIndex->pop_back();
         }
      }

      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);

      idx++;
   } // next poi
}

void CGirderModelManager::GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pPmin,std::vector<Float64>* pPmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pPmin->clear();
   pPmax->clear();

   pPmin->reserve(vPoi.size());
   pPmax->reserve(vPoi.size());

   if ( pMinAxleConfig )
   {
      pMinAxleConfig->clear();
      pMinAxleConfig->reserve(vPoi.size());
   }

   if ( pMaxAxleConfig )
   {
      pMaxAxleConfig->clear();
      pMaxAxleConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIdx, roMember, fetFx, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   ar = pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIdx, roMember, fetFx, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for(const pgsPointOfInterest& poi : vPoi)
   {
      Float64 FxMaxLeft, FxMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;

      maxResults->GetResult(idx,&FxMaxLeft, pMaxAxleConfig ? &maxLeftConfig  : nullptr,
                                &FxMaxRight,pMaxAxleConfig ? &maxRightConfig : nullptr);

      Float64 FxMinLeft, FxMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&FxMinLeft, pMinAxleConfig ? &minLeftConfig  : nullptr,
                                &FxMinRight,pMinAxleConfig ? &minRightConfig : nullptr);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
      if ( IsZero(Xg) )
      {
         pPmin->push_back( -FxMinRight );

         if ( pMinAxleConfig )
         {
            AxleConfiguration minConfig;
            CreateAxleConfig(lbam_model, minRightConfig, &minConfig);
            pMinAxleConfig->push_back(minConfig);
         }

         pPmax->push_back( -FxMaxRight );

         if ( pMaxAxleConfig )
         {
            AxleConfiguration maxConfig;
            CreateAxleConfig(lbam_model, maxRightConfig, &maxConfig);
            pMaxAxleConfig->push_back(maxConfig);
         }
      }
      else
      {
         pPmin->push_back( FxMinLeft );

         if ( pMinAxleConfig )
         {
            AxleConfiguration minConfig;
            CreateAxleConfig(lbam_model, minLeftConfig, &minConfig);
            pMinAxleConfig->push_back(minConfig);
         }

         pPmax->push_back( FxMaxLeft );

         if ( pMaxAxleConfig )
         {
            AxleConfiguration maxConfig;
            CreateAxleConfig(lbam_model, maxLeftConfig, &maxConfig);
            pMaxAxleConfig->push_back(maxConfig);
         }
      }

      idx++;
   } // next poi
}

void CGirderModelManager::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,
                               std::vector<AxleConfiguration>* pMinLeftAxleConfig,
                               std::vector<AxleConfiguration>* pMinRightAxleConfig,
                               std::vector<AxleConfiguration>* pMaxLeftAxleConfig,
                               std::vector<AxleConfiguration>* pMaxRightAxleConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pVmin->clear();
   pVmax->clear();

   pVmin->reserve(vPoi.size());
   pVmax->reserve(vPoi.size());

   if ( pMinLeftAxleConfig )
   {
      pMinLeftAxleConfig->clear();
      pMinLeftAxleConfig->reserve(vPoi.size());
   }

   if ( pMinRightAxleConfig )
   {
      pMinRightAxleConfig->clear();
      pMinRightAxleConfig->reserve(vPoi.size());
   }

   if ( pMaxLeftAxleConfig )
   {
      pMaxLeftAxleConfig->clear();
      pMaxLeftAxleConfig->reserve(vPoi.size());
   }

   if ( pMaxRightAxleConfig )
   {
      pMaxRightAxleConfig->clear();
      pMaxRightAxleConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIdx, roMember, fetFy, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   ar = pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIdx, roMember, fetFy, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 FyMaxLeft, FyMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&FyMaxLeft, pMinLeftAxleConfig  ? &maxLeftConfig  : nullptr,
                                &FyMaxRight,pMinRightAxleConfig ? &maxRightConfig : nullptr);

      Float64 FyMinLeft, FyMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&FyMinLeft, pMaxLeftAxleConfig  ? &minLeftConfig  : nullptr,
                                &FyMinRight,pMaxRightAxleConfig ? &minRightConfig : nullptr);

      WBFL::System::SectionValue minValues(-FyMaxLeft,FyMaxRight);
      WBFL::System::SectionValue maxValues(-FyMinLeft,FyMinRight);

      pVmin->push_back( minValues );
      pVmax->push_back( maxValues );

      if ( pMinLeftAxleConfig )
      {
         AxleConfiguration minAxleLeftConfig;
         CreateAxleConfig(lbam_model, maxLeftConfig, &minAxleLeftConfig);
         pMinLeftAxleConfig->push_back(minAxleLeftConfig);
      }

      if ( pMaxLeftAxleConfig )
      {
         AxleConfiguration maxAxleLeftConfig;
         CreateAxleConfig(lbam_model, minLeftConfig, &maxAxleLeftConfig);
         pMaxLeftAxleConfig->push_back(maxAxleLeftConfig);
      }

      if ( pMinRightAxleConfig )
      {
         AxleConfiguration minAxleRightConfig;
         CreateAxleConfig(lbam_model, maxRightConfig, &minAxleRightConfig);
         pMinRightAxleConfig->push_back(minAxleRightConfig);
      }

      if ( pMaxRightAxleConfig )
      {
         AxleConfiguration maxAxleRightConfig;
         CreateAxleConfig(lbam_model, minRightConfig, &maxAxleRightConfig);
         pMaxRightAxleConfig->push_back(maxAxleRightConfig);
      }
   }
}

void CGirderModelManager::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMmin->clear();
   pMmax->clear();

   pMmin->reserve(vPoi.size());
   pMmax->reserve(vPoi.size());

   if ( pMinAxleConfig )
   {
      pMinAxleConfig->clear();
      pMinAxleConfig->reserve(vPoi.size());
   }

   if ( pMaxAxleConfig )
   {
      pMaxAxleConfig->clear();
      pMaxAxleConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIdx, roMember, fetMz, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   ar = pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIdx, roMember, fetMz, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      Float64 MzMaxLeft, MzMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;

      maxResults->GetResult(idx,&MzMaxLeft, pMaxAxleConfig ? &maxLeftConfig  : nullptr,
                                &MzMaxRight,pMaxAxleConfig ? &maxRightConfig : nullptr);

      Float64 MzMinLeft, MzMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&MzMinLeft, pMinAxleConfig ? &minLeftConfig  : nullptr,
                                &MzMinRight,pMinAxleConfig ? &minRightConfig : nullptr);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
      if ( IsZero(Xg) )
      {
         pMmin->push_back( -MzMinRight );

         if ( pMinAxleConfig )
         {
            AxleConfiguration minConfig;
            CreateAxleConfig(lbam_model, minRightConfig, &minConfig);
            pMinAxleConfig->push_back(minConfig);
         }

         pMmax->push_back( -MzMaxRight );

         if ( pMaxAxleConfig )
         {
            AxleConfiguration maxConfig;
            CreateAxleConfig(lbam_model, maxRightConfig, &maxConfig);
            pMaxAxleConfig->push_back(maxConfig);
         }
      }
      else
      {
         pMmin->push_back( MzMinLeft );

         if ( pMinAxleConfig )
         {
            AxleConfiguration minConfig;
            CreateAxleConfig(lbam_model, minLeftConfig, &minConfig);
            pMinAxleConfig->push_back(minConfig);
         }

         pMmax->push_back( MzMaxLeft );

         if ( pMaxAxleConfig )
         {
            AxleConfiguration maxConfig;
            CreateAxleConfig(lbam_model, maxLeftConfig, &maxConfig);
            pMaxAxleConfig->push_back(maxConfig);
         }
      }
      idx++;
   } // next poi
}

void CGirderModelManager::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pDmin->clear();
   pDmax->clear();

   pDmin->reserve(vPoi.size());
   pDmax->reserve(vPoi.size());

   if ( pMinAxleConfig )
   {
      pMinAxleConfig->clear();
      pMinAxleConfig->reserve(vPoi.size());
   }

   if ( pMaxAxleConfig )
   {
      pMaxAxleConfig->clear();
      pMaxAxleConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIdx, fetDy, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   ar = pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIdx, fetDy, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);
   
   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 DyMaxLeft, DyMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&DyMaxLeft, pMaxAxleConfig ? &maxLeftConfig  : nullptr,
                                &DyMaxRight,pMaxAxleConfig ? &maxRightConfig : nullptr);

      Float64 DyMinLeft, DyMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&DyMinLeft, pMinAxleConfig ? &minLeftConfig  : nullptr,
                                &DyMinRight,pMinAxleConfig ? &minRightConfig : nullptr);

      pDmin->push_back( DyMinLeft );

      if ( pMinAxleConfig )
      {
         AxleConfiguration minConfig;
         CreateAxleConfig(lbam_model, minLeftConfig, &minConfig);
         pMinAxleConfig->push_back(minConfig);
      }

      pDmax->push_back( DyMaxLeft );

      if ( pMaxAxleConfig )
      {
         AxleConfiguration maxConfig;
         CreateAxleConfig(lbam_model, maxLeftConfig, &maxConfig);
         pMaxAxleConfig->push_back(maxConfig);
      }
   }
}

void CGirderModelManager::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pRmin->clear();
   pRmax->clear();

   pRmin->reserve(vPoi.size());
   pRmax->reserve(vPoi.size());

   if ( pMinAxleConfig )
   {
      pMinAxleConfig->clear();
      pMinAxleConfig->reserve(vPoi.size());
   }

   if ( pMaxAxleConfig )
   {
      pMaxAxleConfig->clear();
      pMaxAxleConfig->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIdx, fetRz, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   ar = pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIdx, fetRz, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 RzMaxLeft, RzMaxRight;
      CComPtr<ILiveLoadConfiguration> RzMaxConfig;
      maxResults->GetResult(idx,&RzMaxLeft,pMaxAxleConfig ? &RzMaxConfig : nullptr,&RzMaxRight,nullptr);

      Float64 RzMinLeft, RzMinRight;
      CComPtr<ILiveLoadConfiguration> RzMinConfig;
      minResults->GetResult(idx,&RzMinLeft,pMinAxleConfig ? &RzMinConfig : nullptr,&RzMinRight,nullptr);

      pRmin->push_back( RzMinLeft );
      pRmax->push_back( RzMaxLeft );

      if ( pMaxAxleConfig )
      {
         AxleConfiguration maxConfig;
         CreateAxleConfig(lbam_model, RzMaxConfig, &maxConfig);
         pMaxAxleConfig->push_back(maxConfig);
      }

      if ( pMinAxleConfig )
      {
         AxleConfiguration maxConfig;
         CreateAxleConfig(lbam_model, RzMinConfig, &maxConfig);
         pMinAxleConfig->push_back(maxConfig);
      }
   }
}

void CGirderModelManager::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigurationTop,std::vector<AxleConfiguration>* pMaxAxleConfigurationTop,std::vector<AxleConfiguration>* pMinAxleConfigurationBot,std::vector<AxleConfiguration>* pMaxAxleConfigurationBot) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   pfTopMin->reserve(vPoi.size());
   pfTopMax->reserve(vPoi.size());
   pfBotMin->reserve(vPoi.size());
   pfBotMax->reserve(vPoi.size());

   if ( pMinAxleConfigurationTop )
   {
      pMinAxleConfigurationTop->clear();
      pMinAxleConfigurationTop->reserve(vPoi.size());
   }

   if ( pMaxAxleConfigurationTop )
   {
      pMaxAxleConfigurationTop->clear();
      pMaxAxleConfigurationTop->reserve(vPoi.size());
   }

   if ( pMinAxleConfigurationBot )
   {
      pMinAxleConfigurationBot->clear();
      pMinAxleConfigurationBot->reserve(vPoi.size());
   }

   if ( pMaxAxleConfigurationBot )
   {
      pMaxAxleConfigurationBot->clear();
      pMaxAxleConfigurationBot->reserve(vPoi.size());
   }

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelStressResults> minResults;
   CComPtr<ILiveLoadModelStressResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));


   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pVehicularResponse[bat]->ComputeStresses(m_LBAMPoi, bstrStage, llmt, vehicleIdx, fetMz, optMinimize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   ar = pModelData->pVehicularResponse[bat]->ComputeStresses(m_LBAMPoi, bstrStage, llmt, vehicleIdx, fetMz, optMaximize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   CollectionIndexType top_stress_point_idx = GetStressPointIndex(topLocation);
   CollectionIndexType bot_stress_point_idx = GetStressPointIndex(botLocation);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<IStressResult> fLeftMax, fRightMax;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&fLeftMax,pMaxAxleConfigurationTop || pMaxAxleConfigurationBot ? &maxLeftConfig : nullptr,&fRightMax,pMaxAxleConfigurationTop || pMaxAxleConfigurationBot ? &maxRightConfig : nullptr);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&fLeftMin,pMinAxleConfigurationTop || pMinAxleConfigurationBot ? &minLeftConfig : nullptr,&fRightMin,pMinAxleConfigurationTop || pMinAxleConfigurationBot ? &minRightConfig : nullptr);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
      Float64 fBotMax, fBotMin, fTopMax, fTopMin;
      if ( IsZero(Xg) )
      {
         fRightMax->GetResult(bot_stress_point_idx,&fBotMax);
         fRightMax->GetResult(top_stress_point_idx,&fTopMin);
         fRightMin->GetResult(bot_stress_point_idx,&fBotMin);
         fRightMin->GetResult(top_stress_point_idx,&fTopMax);

         if ( pMaxAxleConfigurationBot )
         {
            AxleConfiguration maxAxleRightConfig;
            CreateAxleConfig(lbam_model, maxRightConfig, &maxAxleRightConfig);
            pMaxAxleConfigurationBot->push_back(maxAxleRightConfig);
         }

         if ( pMaxAxleConfigurationTop )
         {
            AxleConfiguration minAxleRightConfig;
            CreateAxleConfig(lbam_model, minRightConfig, &minAxleRightConfig);
            pMaxAxleConfigurationTop->push_back(minAxleRightConfig);
         }

         if ( pMinAxleConfigurationBot )
         {
            AxleConfiguration minAxleRightConfig;
            CreateAxleConfig(lbam_model, minRightConfig, &minAxleRightConfig);
            pMinAxleConfigurationBot->push_back(minAxleRightConfig);
         }

         if ( pMinAxleConfigurationTop )
         {
            AxleConfiguration maxAxleRightConfig;
            CreateAxleConfig(lbam_model, maxRightConfig, &maxAxleRightConfig);
            pMinAxleConfigurationTop->push_back(maxAxleRightConfig);
         }
      }
      else
      {
         fLeftMax->GetResult(bot_stress_point_idx,&fBotMax);
         fLeftMax->GetResult(top_stress_point_idx,&fTopMin);
         fLeftMin->GetResult(bot_stress_point_idx,&fBotMin);
         fLeftMin->GetResult(top_stress_point_idx,&fTopMax);


         if ( pMaxAxleConfigurationBot )
         {
            AxleConfiguration maxAxleLeftConfig;
            CreateAxleConfig(lbam_model, maxLeftConfig, &maxAxleLeftConfig);
            pMaxAxleConfigurationBot->push_back(maxAxleLeftConfig);
         }

         if ( pMaxAxleConfigurationTop )
         {
            AxleConfiguration minAxleLeftConfig;
            CreateAxleConfig(lbam_model, minLeftConfig, &minAxleLeftConfig);
            pMaxAxleConfigurationTop->push_back(minAxleLeftConfig);
         }

         if ( pMinAxleConfigurationBot )
         {
            AxleConfiguration minAxleLeftConfig;
            CreateAxleConfig(lbam_model, minLeftConfig, &minAxleLeftConfig);
            pMinAxleConfigurationBot->push_back(minAxleLeftConfig);
         }

         if ( pMinAxleConfigurationTop )
         {
            AxleConfiguration maxAxleLeftConfig;
            CreateAxleConfig(lbam_model, maxLeftConfig, &maxAxleLeftConfig);
            pMinAxleConfigurationTop->push_back(maxAxleLeftConfig);
         }
      }
      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);

      idx++;
   } // next poi
}

/////////////////////////////////////////////
// ICombinedForces
Float64 CGirderModelManager::GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   CComPtr<ILBAMModel> lbam;
   GetLBAM(pModelData, bat, &lbam);

   CComPtr<ILoadCases> load_cases;
   lbam->get_LoadCases(&load_cases);

   CComBSTR combo_name = GetLoadCaseName(combo);

   CComPtr<ILoadCase> load_case;
   load_cases->Find(combo_name, &load_case);

   CollectionIndexType nLoadGroups;
   load_case->get_LoadGroupCount(&nLoadGroups);

   // Cycle through load cases and sum reactions
   Float64 R = 0;
   for (CollectionIndexType ldGroupIdx = 0; ldGroupIdx < nLoadGroups; ldGroupIdx++)
   {
      CComBSTR lg_name;
      load_case->GetLoadGroup(ldGroupIdx, &lg_name);

      pgsTypes::ProductForceType pfType = GetProductForceType(lg_name);

      Float64 r = GetReaction(intervalIdx, pfType, pierIdx, girderKey, bat, resultsType);

      R += r;
   }
   return R;
}

void CGirderModelManager::GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Pmin, Pmax;
   GetCombinedLiveLoadAxial(intervalIdx,llType,vPoi,bat,&Pmin,&Pmax);

   ATLASSERT(Pmin.size() == 1 && Pmax.size() == 1);
   *pMin = Pmin.front();
   *pMax = Pmax.front();
}

void CGirderModelManager::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<WBFL::System::SectionValue> Vmin, Vmax;
   GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,&Vmin,&Vmax);

   ATLASSERT( Vmin.size() == 1 && Vmax.size() == 1 );
   *pVmin = Vmin.front();
   *pVmax = Vmax.front();
}

void CGirderModelManager::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1 && Mmax.size() == 1);
   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CGirderModelManager::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT( Dmin.size() == 1 && Dmax.size() == 1 );
   *pDmin = Dmin.front();
   *pDmax = Dmax.front();
}

void CGirderModelManager::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,&Rmin,&Rmax);

   ATLASSERT( Rmin.size() == 1 && Rmax.size() == 1 );
   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CGirderModelManager::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   GetCombinedLiveLoadStress(intervalIdx,llType,vPoi,bat,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax);

   ATLASSERT( fTopMin.size() == 1 && fTopMax.size() == 1 && fBotMin.size() == 1 && fBotMax.size() == 1 );

   *pfTopMin = fTopMin.front();
   *pfTopMax = fTopMax.front();
   *pfBotMin = fBotMin.front();
   *pfBotMax = fBotMax.front();
}

/////////////////////////////////////////////////////
// ICombinedForces2
std::vector<Float64> CGirderModelManager::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(combo != lcPS);
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   // segment is erected into full bridge
   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType rsType = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> sectionResults;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetFx][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetFx][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }

   IndexType nResults;
   sectionResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      sectionResults->get_Item(idx,&result);

      Float64 FxLeft;
      result->get_XLeft(&FxLeft);

      results.push_back(FxLeft);
   }

   return results;
}

std::vector<WBFL::System::SectionValue> CGirderModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(combo != lcPS);
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<WBFL::System::SectionValue> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType rsType = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> sectionResults;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }

   IndexType nResults;
   sectionResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      sectionResults->get_Item(idx,&result);

      Float64 FyLeft, FyRight;
      result->get_YLeft(&FyLeft);
      result->get_YRight(&FyRight);

      results.emplace_back(-FyLeft,FyRight);
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(combo != lcPS);
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   // segment is erected into full bridge
   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType rsType = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> sectionResults;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&sectionResults);
   }

   IndexType nResults;
   sectionResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      sectionResults->get_Item(idx,&result);

      Float64 MzLeft;
      result->get_ZLeft(&MzLeft);

      results.push_back(MzLeft);
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludePreErectionUnrecov) const
{
   ATLASSERT(combo != lcCR && combo != lcSH && combo != lcRE && combo != lcPS); // this are time-step analysis load combinations
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> deflection;
   deflection.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType rsType = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadCaseResponse[bat]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
   }

   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);
   std::vector<CSegmentKey> vSegments;
   pPoi->GetSegmentKeys(vPoi, &vSegments);
   IntervalIndexType erectionIntervalIdx;
   if (vSegments.size() == 1)
   {
      erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vSegments.front());
   }
   else
   {
      CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
      erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   }

   std::vector<Float64> badGirderDeflection;
   std::vector<Float64> goodGirderDeflection;

   bool bDoApplyStorageCorrection = combo == lcDC && (resultsType == rtCumulative || (resultsType == rtIncremental && intervalIdx == erectionIntervalIdx)) && bIncludePreErectionUnrecov;
   if (bDoApplyStorageCorrection)
   {
      // the DC combination contains girder deflections based on the full weight of the girder being applied at erection.
      // this is not the correct deflection... the correct deflection is the deflection at storage plus the incremental
      // deflection due to a change in support locations.
      // get the bad girder deflection so we can remove it from the DC combination, and get the correct deflection so we can
      // add it into the DC combination.
      CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftGirder) );
      badGirderDeflection = GetDeflection(intervalIdx,OLE2T(bstrLoadGroup),vPoi,bat,rtCumulative); // get bad deflection (use the string version instead of the pfType version so the girder load name doesn't get altered)

      GET_IFACE(IProductForces2, pProdForces);
      goodGirderDeflection = pProdForces->GetDeflection(intervalIdx,pgsTypes::pftGirder,vPoi,bat,rtCumulative,false,false,bIncludePreErectionUnrecov);
   }

   IndexType nResults;
   results->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      results->get_Item(idx,&result);

      Float64 Dy, Dyr;
      result->get_YLeft(&Dy);
      result->get_YRight(&Dyr);

      ATLASSERT(IsEqual(Dy,Dyr));

      if (bDoApplyStorageCorrection)
      {
         Float64 badDy  = badGirderDeflection[idx];
         Float64 goodDy = goodGirderDeflection[idx];
         Dy += goodDy - badDy;
      }

      deflection.push_back(Dy);
   }

   return deflection;
}

std::vector<Float64> CGirderModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludePreErectionUnrecov) const
{
   ATLASSERT(combo != lcCR && combo != lcSH && combo != lcRE && combo != lcPS); // this are time-step analysis load combinations
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> rotation;
   rotation.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType rsType = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadCaseResponse[bat]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
   }

   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);
   std::vector<CSegmentKey> vSegments;
   pPoi->GetSegmentKeys(vPoi, &vSegments);
   IntervalIndexType erectionIntervalIdx;
   if (vSegments.size() == 1)
   {
      erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vSegments.front());
   }
   else
   {
      CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
      erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   }

   std::vector<Float64> badGirderRotation;
   std::vector<Float64> goodGirderRotation;
   bool bDoApplyStorageCorrection = combo == lcDC && (resultsType == rtCumulative || (resultsType == rtIncremental && intervalIdx == erectionIntervalIdx)) && bIncludePreErectionUnrecov;
   if (bDoApplyStorageCorrection)
   {
      // the DC combination contains girder rotations based on the full weight of the girder being applied at erection.
      // this is not the correct rotation... the correct rotation is the rotation at storage plus the incremental
      // rotation due to a change in support locations.
      // get the bad girder rotation so we can remove it from the DC combination, and get the correct rotation so we can
      // add it into the DC combination.
      CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftGirder) );
      badGirderRotation  = GetRotation(intervalIdx,OLE2T(bstrLoadGroup),vPoi,bat,rtCumulative); // get bad rotation (use the string version instead of the pfType version so the girder load name doesn't get altered)
      GET_IFACE(IProductForces2, pProdForces);
      goodGirderRotation = pProdForces->GetRotation(intervalIdx,pgsTypes::pftGirder,vPoi,bat,rtCumulative,false,false,bIncludePreErectionUnrecov);
   }

   IndexType nResults;
   results->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      results->get_Item(idx,&result);

      Float64 Rz, Rzr;
      result->get_ZLeft(&Rz);
      result->get_ZRight(&Rzr);

      ATLASSERT(IsEqual(Rz,Rzr));

      if (bDoApplyStorageCorrection)
      {
         Float64 badRz  = badGirderRotation[idx];
         Float64 goodRz = goodGirderRotation[idx];

         Rz += goodRz - badRz;
      }

      rotation.push_back(Rz);
   }

   return rotation;
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   VERIFY_ANALYSIS_TYPE;
   ATLASSERT(VerifyPoi(vPoi));

   pfTop->clear();
   pfBot->clear();

   pfTop->reserve(vPoi.size());
   pfBot->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType rsType = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionStressResults> min_results, max_results, results;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &max_results);
      ar = pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &min_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &max_results);
      ar = pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &min_results);
   }
   else
   {
      ar = pModelData->pLoadCaseResponse[bat]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &results);
   }

   CollectionIndexType stress_point_index_top = GetStressPointIndex(topLocation);
   CollectionIndexType stress_point_index_bot = GetStressPointIndex(botLocation);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 fTop, fBot;

      if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
      {
         CComPtr<ISectionStressResult> top_stresses;
         max_results->get_Item(idx,&top_stresses);

         CComPtr<ISectionStressResult> bot_stresses;
         min_results->get_Item(idx,&bot_stresses);

         if ( IsZero(Xg) )
         {
            top_stresses->GetRightResult(stress_point_index_top,&fTop);
            bot_stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            top_stresses->GetLeftResult(stress_point_index_top,&fTop);
            bot_stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }
      else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
      {
         CComPtr<ISectionStressResult> top_stresses;
         min_results->get_Item(idx,&top_stresses);

         CComPtr<ISectionStressResult> bot_stresses;
         max_results->get_Item(idx,&bot_stresses);

         if ( IsZero(Xg) )
         {
            top_stresses->GetRightResult(stress_point_index_top,&fTop);
            bot_stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            top_stresses->GetLeftResult(stress_point_index_top,&fTop);
            bot_stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }
      else
      {
         CComPtr<ISectionStressResult> stresses;
         results->get_Item(idx,&stresses);

         if ( IsZero(Xg) )
         {
            stresses->GetRightResult(stress_point_index_top,&fTop);
            stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            stresses->GetLeftResult(stress_point_index_top,&fTop);
            stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);

      idx++;
   } // next poi
}

void CGirderModelManager::GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pPmin,std::vector<Float64>* pPmax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pPmax->clear();
   pPmin->clear();

   pPmax->reserve(vPoi.size());
   pPmin->reserve(vPoi.size());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      pPmax->insert(pPmax->begin(),vPoi.size(),0.0);
      pPmin->insert(pPmin->begin(),vPoi.size(),0.0);
      return;
   }


   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);


   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFx, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFx, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   GET_IFACE(IPointOfInterest,pPoi);
   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 FxMaxLeft, FxMaxRight;
      maxResults->GetResult(idx,&FxMaxLeft,nullptr,&FxMaxRight,nullptr);

      Float64 FxMinLeft, FxMinRight;
      minResults->GetResult(idx,&FxMinLeft,nullptr,&FxMinRight,nullptr);

      if ( IsZero(Xg) )
      {
         pPmin->push_back( -FxMinRight );
         pPmax->push_back( -FxMaxRight );
      }
      else
      {
         pPmin->push_back( FxMinLeft );
         pPmax->push_back( FxMaxLeft );
      }

      idx++;
   }
}

void CGirderModelManager::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact, std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pVmax->clear();
   pVmin->clear();

   pVmax->reserve(vPoi.size());
   pVmin->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
   VARIANT_BOOL incImpact = bIncludeImpact ? VARIANT_TRUE: VARIANT_FALSE;

   // TRICKY:
   // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
   // that the optimization must go the opposite way as well when using the envelopers
   if (bat == pgsTypes::MinSimpleContinuousEnvelope)
   {
      bat = pgsTypes::MaxSimpleContinuousEnvelope;
   }
   else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
   {
      bat = pgsTypes::MinSimpleContinuousEnvelope;
   }

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, incImpact, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, incImpact, VARIANT_FALSE, &minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 FyMaxLeft, FyMaxRight;
      maxResults->GetResult(idx,&FyMaxLeft,nullptr,&FyMaxRight,nullptr);

      Float64 FyMinLeft, FyMinRight;
      minResults->GetResult(idx,&FyMinLeft,nullptr,&FyMinRight,nullptr);

      WBFL::System::SectionValue minValue(-FyMaxLeft,FyMaxRight);
      WBFL::System::SectionValue maxValue(-FyMinLeft,FyMinRight);

      pVmin->push_back( minValue );
      pVmax->push_back( maxValue );
   }
}

void CGirderModelManager::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMmax->clear();
   pMmin->clear();

   pMmax->reserve(vPoi.size());
   pMmin->reserve(vPoi.size());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      pMmax->insert(pMmax->begin(),vPoi.size(),0.0);
      pMmin->insert(pMmin->begin(),vPoi.size(),0.0);
      return;
   }


   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);


   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   GET_IFACE(IPointOfInterest,pPoi);
   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 MzMaxLeft, MzMaxRight;
      maxResults->GetResult(idx,&MzMaxLeft,nullptr,&MzMaxRight,nullptr);

      Float64 MzMinLeft, MzMinRight;
      minResults->GetResult(idx,&MzMinLeft,nullptr,&MzMinRight,nullptr);

      if ( IsZero(Xg) )
      {
         pMmin->push_back( -MzMinRight );
         pMmax->push_back( -MzMaxRight );
      }
      else
      {
         pMmin->push_back( MzMinLeft );
         pMmax->push_back( MzMaxLeft );
      }

      idx++;
   }
}

void CGirderModelManager::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pDmax->clear();
   pDmin->clear();

   pDmax->reserve(vPoi.size());
   pDmin->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetDy, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetDy, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 DyMaxLeft, DyMaxRight;
      maxResults->GetResult(idx,&DyMaxLeft,nullptr,&DyMaxRight,nullptr);

      Float64 DyMinLeft, DyMinRight;
      minResults->GetResult(idx,&DyMinLeft,nullptr,&DyMinRight,nullptr);

      pDmin->push_back( DyMinLeft );
      pDmax->push_back( DyMaxLeft );
   }
}

void CGirderModelManager::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pRmax->clear();
   pRmin->clear();

   pRmax->reserve(vPoi.size());
   pRmin->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetRz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetRz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 RyMaxLeft, RyMaxRight;
      maxResults->GetResult(idx,&RyMaxLeft,nullptr,&RyMaxRight,nullptr);

      Float64 RyMinLeft, RyMinRight;
      minResults->GetResult(idx,&RyMinLeft,nullptr,&RyMinRight,nullptr);

      pRmin->push_back( RyMinLeft );
      pRmax->push_back( RyMaxLeft );
   }
}

void CGirderModelManager::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   pfTopMin->reserve(vPoi.size());
   pfTopMax->reserve(vPoi.size());
   pfBotMin->reserve(vPoi.size());
   pfBotMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationStressResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);


   CollectionIndexType top_stress_point_index = GetStressPointIndex(topLocation);
   CollectionIndexType bot_stress_point_index = GetStressPointIndex(botLocation);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<IStressResult> fLeftMax, fRightMax;
      maxResults->GetResult(idx,&fLeftMax,nullptr,&fRightMax,nullptr);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      minResults->GetResult(idx,&fLeftMin,nullptr,&fRightMin,nullptr);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 fBotMin, fBotMax, fTopMin, fTopMax;
      if ( IsZero(Xg) )
      {
         fRightMax->GetResult(bot_stress_point_index,&fBotMax);
         fRightMax->GetResult(top_stress_point_index,&fTopMax);
         fRightMin->GetResult(bot_stress_point_index,&fBotMin);
         fRightMin->GetResult(top_stress_point_index,&fTopMin);
      }
      else
      {
         fLeftMax->GetResult(bot_stress_point_index,&fBotMax);
         fLeftMax->GetResult(top_stress_point_index,&fTopMax);
         fLeftMin->GetResult(bot_stress_point_index,&fBotMin);
         fLeftMin->GetResult(top_stress_point_index,&fTopMin);
      }

      if ( fTopMax < fTopMin )
      {
         std::swap(fTopMax,fTopMin);
      }

      if ( fBotMax < fBotMin )
      {
         std::swap(fBotMax,fBotMin);
      }

      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);

      idx++;
   }
}

///////////////////////////////////////////
// ILimitStateForces
void CGirderModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<WBFL::System::SectionValue> vMin, vMax;
   GetShear(intervalIdx,limitState,vPoi,bat,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CGirderModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetMoment(intervalIdx,limitState,vPoi,bat,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CGirderModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludePreErectionUnrecov,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetDeflection(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,bIncludePreErectionUnrecov,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CGirderModelManager::GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) const
{
   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(GetPierID(pierIdx));

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

   CComPtr<ILoadCombinationResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &minResults);

   Float64 FyMax = 0;
   Float64 FyMin = 0;
   CollectionIndexType nResults;
   maxResults->get_Count(&nResults);
   for ( CollectionIndexType i = 0; i < nResults; i++ )
   {
      Float64 fyMax;
      maxResults->GetResult(i,&fyMax,nullptr);
      FyMax += fyMax;

      Float64 fyMin;
      minResults->GetResult(i,&fyMin,nullptr);
      FyMin += fyMin;
   }

   *pMin = FyMin;
   *pMax = FyMax;
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetStress(intervalIdx,limitState,vPoi,bat,stressLocation,bIncludePrestress,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CGirderModelManager::GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);

#if defined _DEBUG
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   ATLASSERT(intervalIdx != releaseIntervalIdx); // this method only works for bridge site stages
#endif

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(segmentKey),bat);

   PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
   if ( poi_id == INVALID_ID )
   {
      poi_id = AddPointOfInterest( pModelData, poi );
      ATLASSERT( 0 <= poi_id ); // if this fires, the poi wasn't added... WHY???
   }

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(poi_id);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinations> loadCombos;
   if ( pModelData->m_Model )
   {
      pModelData->m_Model->get_LoadCombinations(&loadCombos) ;
   }
   else
   {
      pModelData->m_ContinuousModel->get_LoadCombinations(&loadCombos);
   }

   CComPtr<ILoadCombination> loadCombo;
   loadCombos->Find(bstrLoadCombo,&loadCombo);
   Float64 gLLmin, gLLmax;
   loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
   if ( gLLmin < 0 || gLLmax < 0 )
   {
      ATLASSERT( ::IsRatingLimitState(limitState) ); // this can only happen for ratings
   }

   // Get the Max/Min moments
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE, &minResults);

   // Get the loading configuraitons that caused max/min moments
   CComPtr<ILoadCombinationResultConfiguration> maxResultConfigLeft, maxResultConfigRight;
   Float64 MzMaxLeft, MzMaxRight;
   maxResults->GetResult(0,&MzMaxLeft,&maxResultConfigLeft,&MzMaxRight,&maxResultConfigRight);

   CComPtr<ILoadCombinationResultConfiguration> minResultConfigLeft, minResultConfigRight;
   Float64 MzMinLeft, MzMinRight;
   minResults->GetResult(0,&MzMinLeft,&minResultConfigLeft,&MzMinRight,&minResultConfigRight);

   // Get the concurrent shears
   CComPtr<ISectionResult3Ds> maxShearsLeft, maxShearsRight, minShearsLeft, minShearsRight;
   if ( bat == pgsTypes::SimpleSpan || bat == pgsTypes::ContinuousSpan )
   {
      ar = pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigLeft, &maxShearsLeft);
      ar = pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigRight,&maxShearsRight);
      ar = pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigLeft, &minShearsLeft);
      ar = pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigRight,&minShearsRight);
   }
   else
   {
      CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(pModelData->pLoadComboResponse[bat]);
      ar = concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigLeft, &maxShearsLeft);
      ar = concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigRight,&maxShearsRight);
      ar = concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigLeft, &minShearsLeft);
      ar = concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigRight,&minShearsRight);
   }

   CComPtr<ISectionResult3D> result;
   Float64 FyMaxLeft, FyMaxRight, FyMinLeft, FyMinRight;

   maxShearsLeft->get_Item(0,&result);
   result->get_YLeft(&FyMaxLeft);

   result.Release();
   maxShearsRight->get_Item(0,&result);
   result->get_YRight(&FyMaxRight);

   result.Release();
   minShearsLeft->get_Item(0,&result);
   result->get_YLeft(&FyMinLeft);

   result.Release();
   minShearsRight->get_Item(0,&result);
   result->get_YRight(&FyMinRight);
   
   *pMax = WBFL::System::SectionValue(-FyMaxLeft,FyMaxRight); // shear concurrent with Max moment
   *pMin = WBFL::System::SectionValue(-FyMinLeft,FyMinRight); // shear concurrent with Min moment
}

void CGirderModelManager::GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) const
{
   GET_IFACE(ISpecification,pSpec);

   Float64 Mu_max, Mu_min;
   WBFL::System::SectionValue Vi_min, Vi_max;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 Mmin,Mmax;
      WBFL::System::SectionValue Vimin, Vimax;

      GetMoment( intervalIdx, limitState, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax );
      GetConcurrentShear(  intervalIdx, limitState, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_max = Mmax;
      Vi_max = Vimax;

      GetMoment( intervalIdx, limitState, poi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax );
      GetConcurrentShear(  intervalIdx, limitState, poi, pgsTypes::MinSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_min = Mmin;
      Vi_min = Vimin;
   }
   else
   {
      pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      GetMoment( intervalIdx, limitState, poi, bat, &Mu_min, &Mu_max );
      GetConcurrentShear(  intervalIdx, limitState, poi, bat, &Vi_min, &Vi_max );
   }

   Mu_max = IsZero(Mu_max) ? 0 : Mu_max;
   Mu_min = IsZero(Mu_min) ? 0 : Mu_min;

   // driving moment is the one with the greater magnitude
   Float64 Mu = Max(fabs(Mu_max),fabs(Mu_min));

   Float64 MuSign = 1;
   if ( IsEqual(Mu,fabs(Mu_max)) )
   {
      MuSign = BinarySign(Mu_max);
   }
   else
   {
      MuSign = BinarySign(Mu_min);
   }

   *pMmax = MuSign*Mu;

   if ( IsEqual(Mu,fabs(Mu_max)) )
   {
      // magnutude of maximum moment is greatest
      // use least of left/right Vi_max
      *pVi = Min(fabs(Vi_max.Left()),fabs(Vi_max.Right()));
   }
   else
   {
      // magnutude of minimum moment is greatest
      // use least of left/right Vi_min
      *pVi = Min(fabs(Vi_min.Left()),fabs(Vi_min.Right()));
   }
}

///////////////////////////////////////////////////////
// ILimitStateForces2
void CGirderModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMin->clear();
   pMax->clear();

   pMin->reserve(vPoi.size());
   pMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );
   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFx, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFx, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 FxMaxLeft, FxMaxRight;
      maxResults->GetResult(idx,&FxMaxLeft,nullptr,&FxMaxRight,nullptr);

      Float64 FxMinLeft, FxMinRight;
      minResults->GetResult(idx,&FxMinLeft,nullptr,&FxMinRight,nullptr);

      pMin->push_back( FxMinLeft );
      pMax->push_back( FxMaxLeft );
   }
}

void CGirderModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<WBFL::System::SectionValue>* pMin,std::vector<WBFL::System::SectionValue>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMin->clear();
   pMax->clear();

   pMin->reserve(vPoi.size());
   pMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   // TRICKY:
   // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
   // that the optimization must go the opposite way as well when using the envelopers
   if (bat == pgsTypes::MinSimpleContinuousEnvelope)
   {
      bat = pgsTypes::MaxSimpleContinuousEnvelope;
   }
   else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
   {
      bat = pgsTypes::MinSimpleContinuousEnvelope;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 FyMaxLeft, FyMaxRight;
      maxResults->GetResult(idx,&FyMaxLeft,nullptr,&FyMaxRight,nullptr);

      Float64 FyMinLeft, FyMinRight;
      minResults->GetResult(idx,&FyMinLeft,nullptr,&FyMinRight,nullptr);

      pMin->emplace_back( -FyMaxLeft,FyMaxRight );
      pMax->emplace_back( -FyMinLeft,FyMinRight );
   }
}

void CGirderModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMin->clear();
   pMax->clear();

   pMin->reserve(vPoi.size());
   pMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );
   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 MzMaxLeft, MzMaxRight;
      maxResults->GetResult(idx,&MzMaxLeft,nullptr,&MzMaxRight,nullptr);

      Float64 MzMinLeft, MzMinRight;
      minResults->GetResult(idx,&MzMinLeft,nullptr,&MzMinRight,nullptr);

      pMin->push_back( MzMinLeft );
      pMax->push_back( MzMaxLeft );
   }
}

void CGirderModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludePreErectionUnrecov,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMin->clear();
   pMax->clear();

   pMin->reserve(vPoi.size());
   pMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL vbIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx && bIncludeLiveLoad ? VARIANT_TRUE : VARIANT_FALSE );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetDy, optMaximize, vbIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetDy, optMinimize, vbIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   // the limit state contains girder deflections based on the full weight of the girder being applied at erection.
   // (the limit state contains the load "Girder", not "Girder_Increment". We do this so the limit state definitions are consistent for all cases. The down side is that
   // we have to make an adjustment for deflections)
   // this is not the correct deflection... the correct deflection is the deflection at storage plus the incremental
   // deflection due to a change in support locations for the erected girder segment.
   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftGirder) );
   std::vector<Float64> badGirderDeflection  = GetDeflection(intervalIdx,OLE2T(bstrLoadGroup),vPoi,bat,rtCumulative);
   GET_IFACE(IProductForces2, pProdForces);
   std::vector<Float64> goodGirderDeflection = pProdForces->GetDeflection(intervalIdx,pgsTypes::pftGirder,vPoi,bat,rtCumulative,false,false,bIncludePreErectionUnrecov);
   
   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 DCmin, DCmax;
   pLoadFactors->GetDC(limitState, &DCmin, &DCmax);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 DyMaxLeft, DyMaxRight;
      maxResults->GetResult(idx,&DyMaxLeft,nullptr,&DyMaxRight,nullptr);
      ATLASSERT(IsEqual(DyMaxLeft,DyMaxRight));

      Float64 DyMinLeft, DyMinRight;
      minResults->GetResult(idx,&DyMinLeft,nullptr,&DyMinRight,nullptr);
      ATLASSERT(IsEqual(DyMinLeft,DyMinRight));

      Float64 badDy  = badGirderDeflection[idx];
      Float64 goodDy = goodGirderDeflection[idx];
      
      DyMinLeft += DCmin*(goodDy - badDy);
      DyMaxLeft += DCmax*(goodDy - badDy);

      pMin->push_back( DyMinLeft );
      pMax->push_back( DyMaxLeft );
   }

   if ( bIncludePrestress )
   {
      // prestress deflection is not included in the LBAM models... get the product results load
      // and add them in
      GET_IFACE(IProductForces2, pProductForces);
      std::vector<Float64> deltaPS = pProductForces->GetDeflection(intervalIdx,pgsTypes::pftPretension,vPoi,bat,rtCumulative,false,false,bIncludePreErectionUnrecov);
      std::transform(deltaPS.cbegin(),deltaPS.cend(),pMin->cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(deltaPS.cbegin(),deltaPS.cend(),pMax->cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});

      // Girder PT already included because it is in the pgsTypes::pftSecondaryEffects product load, however, segment PT is not because
      // it is more like prestressing...
      // we don't want to add girder PT so get the deflection in an interval that's before any girders are erected
      IntervalIndexType firstPTIntervalIdx = pIntervals->GetFirstGirderTendonStressingInterval(girderKey);
      if (firstPTIntervalIdx != INVALID_INDEX)
      {
         IntervalIndexType i = Min(firstPTIntervalIdx - 1, intervalIdx);
         std::vector<Float64> deltaPT = pProductForces->GetDeflection(i, pgsTypes::pftPostTensioning, vPoi, bat, rtCumulative, false,false,bIncludePreErectionUnrecov);
         std::transform(deltaPT.cbegin(), deltaPT.cend(), pMin->cbegin(), pMin->begin(), [](const auto& a, const auto& b) {return a + b; });
         std::transform(deltaPT.cbegin(), deltaPT.cend(), pMax->cbegin(), pMax->begin(), [](const auto& a, const auto& b) {return a + b; });
      }
   }
   else
   {
      // Results are to be without prestress so remove the PT effect
      GET_IFACE(IGirderTendonGeometry,pTendonGeom);
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
      if ( 0 < nDucts )
      {
         std::vector<Float64> deltaPT = GetDeflection(intervalIdx,pgsTypes::pftPostTensioning,vPoi,bat,rtCumulative);
         std::transform(pMin->cbegin(),pMin->cend(),deltaPT.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a - b;});
         std::transform(pMax->cbegin(),pMax->cend(),deltaPT.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a - b;});
      }
   }
}

void CGirderModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludePreErectionUnrecov,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pMin->clear();
   pMax->clear();

   pMin->reserve(vPoi.size());
   pMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL vbIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx && bIncludeLiveLoad ? VARIANT_TRUE : VARIANT_FALSE );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetRz, optMaximize, vbIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetRz, optMinimize, vbIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   // the limit state contains girder rotations based on the full weight of the girder being applied at erection.
   // (the limit state contains the load "Girder", not "Girder_Increment". We do this so the limit state definitions are consistent for all cases. The down side is that
   // we have to make an adjustment for rotation)
   // this is not the correct rotation... the correct rotation is the rotation at storage plus the incremental
   // rotation due to a change in support locations for the erected girder segment.
   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftGirder) );
   std::vector<Float64> badGirderRotation  = GetRotation(intervalIdx,OLE2T(bstrLoadGroup),vPoi,bat,rtCumulative);
   GET_IFACE(IProductForces2, pProdForces);
   std::vector<Float64> goodGirderRotation = pProdForces->GetRotation(intervalIdx,pgsTypes::pftGirder,vPoi,bat,rtCumulative,false,false,bIncludePreErectionUnrecov);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 DCmin, DCmax;
   pLoadFactors->GetDC(limitState, &DCmin, &DCmax);

   IndexType nResults;
   maxResults->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      Float64 RzMaxLeft, RzMaxRight;
      maxResults->GetResult(idx,&RzMaxLeft,nullptr,&RzMaxRight,nullptr);

      Float64 RzMinLeft, RzMinRight;
      minResults->GetResult(idx,&RzMinLeft,nullptr,&RzMinRight,nullptr);

      Float64 badRz = badGirderRotation[idx];
      Float64 goodRz = goodGirderRotation[idx];

      RzMinLeft += DCmin*(goodRz - badRz);
      RzMaxLeft += DCmax*(goodRz - badRz);

      pMin->push_back( RzMinLeft );
      pMax->push_back( RzMaxLeft );
   }

   if ( bIncludePrestress )
   {
      // prestress deflection is not included in the LBAM models... get the product results load
      // and add them in
      GET_IFACE(IProductForces2, pProductForces);
      std::vector<Float64> deltaPS = pProductForces->GetRotation(intervalIdx,pgsTypes::pftPretension,vPoi,bat,rtCumulative,false,false,bIncludePreErectionUnrecov);
      std::transform(deltaPS.cbegin(),deltaPS.cend(),pMin->cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(deltaPS.cbegin(),deltaPS.cend(),pMax->cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});


      // Girder PT already included because it is in the pgsTypes::pftSecondaryEffects product load, however, segment PT is not because
      // it is more like prestressing...
      // we don't want to add girder PT so get the deflection in an interval that's before any girders are erected
      IntervalIndexType i = pIntervals->GetFirstSegmentErectionInterval(girderKey) - 1;
      std::vector<Float64> deltaPT = pProductForces->GetRotation(i, pgsTypes::pftPostTensioning, vPoi, bat, rtCumulative, false);
      std::transform(deltaPT.cbegin(), deltaPT.cend(), pMin->cbegin(), pMin->begin(), [](const auto& a, const auto& b) {return a + b; });
      std::transform(deltaPT.cbegin(), deltaPT.cend(), pMax->cbegin(), pMax->begin(), [](const auto& a, const auto& b) {return a + b; });
   }
   else
   {
      // Results are to be without prestress so remove the PT effect
      GET_IFACE(IGirderTendonGeometry,pTendonGeom);
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
      if ( 0 < nDucts )
      {
         std::vector<Float64> deltaPT = GetRotation(intervalIdx,pgsTypes::pftPostTensioning,vPoi,bat,rtCumulative);
         std::transform(pMin->cbegin(),pMin->cend(),deltaPT.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a - b;});
         std::transform(pMax->cbegin(),pMax->cend(),deltaPT.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a - b;});
      }
   }
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   VERIFY_ANALYSIS_TYPE; // no secondary effects to deal with
   ATLASSERT(VerifyPoi(vPoi));

   pMin->clear();
   pMax->clear();

   pMin->reserve(vPoi.size());
   pMax->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationStressResults> maxResults, minResults;

   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);


   GET_IFACE(IPointOfInterest, pPoi);

   IndexType idx = 0;
   for(const pgsPointOfInterest& poi : vPoi)
   {
      Float64 fMax(0.0), fMin(0.0);

      CComPtr<IStressResult> fLeftMax, fRightMax;
      maxResults->GetResult(idx,&fLeftMax,nullptr,&fRightMax,nullptr);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      minResults->GetResult(idx,&fLeftMin,nullptr,&fRightMin,nullptr);

      CollectionIndexType stress_point_index = GetStressPointIndex(stressLocation);

      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      if ( IsZero(Xg) )
      {
         fRightMax->GetResult(stress_point_index,&fMax);
         fRightMin->GetResult(stress_point_index,&fMin);
      }
      else
      {
         fLeftMax->GetResult(stress_point_index,&fMax);
         fLeftMin->GetResult(stress_point_index,&fMin);
      }

      if (fMax < fMin )
      {
         std::swap(fMin,fMax);
      }

      Float64 k;
      if (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI )
      {
         k = 0.5; // Use half prestress stress if service IA LRFD (See Tbl 5.9.4.2.1-1 2008 or before) or Fatigue I (LRFD 5.5.3.1 2009)
      }
      else
      {
         k = 1.0;
      }

      if ( bIncludePrestress )
      {
         Float64 ps = GetStress(intervalIdx,poi,stressLocation,bIncludeLiveLoad==VARIANT_TRUE,limitState, INVALID_INDEX/*controlling live load*/);
         fMin += k*ps;
         fMax += k*ps;

         Float64 pts = GetStressFromSegmentPT(intervalIdx, poi, stressLocation, ALL_DUCTS);
         fMin += k*pts;
         fMax += k*pts;

         Float64 ptg = GetStressFromGirderPT(intervalIdx, poi, stressLocation, ALL_DUCTS);
         fMin += k*ptg;
         fMax += k*ptg;
      }

      if ( railingSystemIntervalIdx <= intervalIdx && pPoi->IsOnSegment(poi) )
      {
         Float64 ft_ss, fb_ss;
         GetDeckShrinkageStresses(poi, pgsTypes::TopGirder, pgsTypes::BottomGirder,&ft_ss,&fb_ss);

         ft_ss *= k;
         fb_ss *= k;

         if ( stressLocation == pgsTypes::TopGirder )
         {
            fMin += ft_ss;
            fMax += ft_ss;
         }
         else if ( stressLocation == pgsTypes::BottomGirder )
         {
            fMin += fb_ss;
            fMax += fb_ss;
         }
      }

      pMax->push_back(fMax);
      pMin->push_back(fMin);

      idx++;
   }
}

std::vector<Float64> CGirderModelManager::GetSlabDesignMoment(pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat) const
{
   ATLASSERT(VerifyPoi(vPoi));

   const CSegmentKey& segmentKey(vPoi.front().get().GetSegmentKey());

   std::vector<Float64> vMoment;
   vMoment.reserve(vPoi.size());

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bExcludeNoncompositeMoments = !pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType constructionIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType firstCompositeDeckIntervalIdx = pIntervals->GetFirstCompositeDeckInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE(IPointOfInterest, pPoi);

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(lastIntervalIdx) );

   VARIANT_BOOL bIncludeLiveLoad = VARIANT_TRUE;
   CComPtr<ILoadCombinationSectionResults> minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE,  &minResults);

   IndexType idx = 0;
   for(const pgsPointOfInterest& poi : vPoi)
   {
      ATLASSERT(CGirderKey(segmentKey) == CGirderKey(poi.GetSegmentKey()));

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

      Float64 MzMinLeft, MzMinRight;
      CComPtr<ILoadCombinationResultConfiguration> leftConfig,rightConfig;
      minResults->GetResult(idx,&MzMinLeft,&leftConfig,&MzMinRight,&rightConfig);

      CComPtr<ILoadCombinationResultConfiguration> min_result_config;

      Float64 MzMin = MzMinLeft;
      min_result_config = leftConfig;

      if ( bExcludeNoncompositeMoments )
      {
         // for MzMin - want Mu to be only for superimposed dead loads
         // remove girder moment

         // Get load factor that was used on DC loads
         Float64 gDC;
         CollectionIndexType nLoadCases;
         min_result_config->get_LoadCaseFactorCount(&nLoadCases);
         for ( CollectionIndexType loadCaseIdx = 0; loadCaseIdx < nLoadCases; loadCaseIdx++ )
         {
            Float64 g;
            CComBSTR bstr;
            min_result_config->GetLoadCaseFactor(loadCaseIdx,&bstr,&g);
            if ( GetLoadCaseName(lcDC) == bstr )
            {
               gDC = g;
               break;
            }
         }

         // remove moments for all dead loads before and including deck casting
         // it doesn't matter when continuity is made (before or after deck casting), none of these loads put direct tension into the deck rebar
         Float64 Mg = GetMoment(erectionIntervalIdx,pgsTypes::pftGirder,poi,bat,rtCumulative);
         Float64 Mconstruction = constructionIntervalIdx == INVALID_INDEX ? 0 : GetMoment(constructionIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtCumulative);
         Float64 Mslab         = castDeckIntervalIdx == INVALID_INDEX ? 0 : GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab,         poi, bat, rtCumulative);
         Float64 Mslab_pad     = castDeckIntervalIdx == INVALID_INDEX ? 0 : GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad,      poi, bat, rtCumulative);
         Float64 Mslab_panel   = castDeckIntervalIdx == INVALID_INDEX ? 0 : GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPanel,    poi, bat, rtCumulative);
         Float64 Mdiaphragm    = castDiaphragmIntervalIdx == INVALID_INDEX ? 0 : GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm,    poi, bat, rtCumulative);
         Float64 Mshear_key    = castShearKeyIntervalIdx == INVALID_INDEX ? 0 : GetMoment(castShearKeyIntervalIdx, pgsTypes::pftShearKey,     poi, bat, rtCumulative);
         Float64 Muser_dc      = castDeckIntervalIdx == INVALID_INDEX ? 0 : GetMoment(castDeckIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtCumulative);
         MzMin -= gDC*(Mg + Mconstruction + Mslab + Mslab_pad + Mslab_panel + Mdiaphragm + Mshear_key + Muser_dc);
      }

      vMoment.push_back( MzMin );
      idx++;
   }

   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      GET_IFACE(ICombinedForces2,pForces);

      Float64 gCRMax;
      Float64 gSHMax;
      Float64 gREMax;
      if (IsRatingLimitState(limitState))
      {
         GET_IFACE(IRatingSpecification, pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
      }
      else
      {
         GET_IFACE(ILoadFactors, pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         gCRMax = pLoadFactors->GetCRMax(limitState);
         gSHMax = pLoadFactors->GetSHMax(limitState);
         gREMax = pLoadFactors->GetREMax(limitState);
      }


      std::vector<Float64> vMcr = pForces->GetMoment(lastIntervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<Float64> vMsh = pForces->GetMoment(lastIntervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<Float64> vMre = pForces->GetMoment(lastIntervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vMcrMax;
         vMcrMax.reserve(vMcr.size());
         std::transform(vMcr.cbegin(),vMcr.cend(),std::back_inserter(vMcrMax), [&gCRMax](const Float64& value) {return value*gCRMax;});
         std::transform(vMoment.cbegin(),vMoment.cend(),vMcrMax.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(vMoment.cbegin(),vMoment.cend(),vMcr.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vMshMax;
         vMshMax.reserve(vMsh.size());
         std::transform(vMsh.cbegin(),vMsh.cend(),std::back_inserter(vMshMax), [&gSHMax](const Float64& value) {return value*gSHMax;});
         std::transform(vMoment.cbegin(),vMoment.cend(),vMshMax.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(vMoment.cbegin(),vMoment.cend(),vMsh.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vMreMax;
         vMreMax.reserve(vMre.size());
         std::transform(vMre.cbegin(),vMre.cend(),std::back_inserter(vMreMax), [&gREMax](const Float64& value) {return value*gREMax;});
         std::transform(vMoment.cbegin(),vMoment.cend(),vMreMax.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(vMoment.cbegin(),vMoment.cend(),vMre.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( bExcludeNoncompositeMoments )
      {
         // we've added in all the CR/SH/RE moments... now remove the Cumulative CR/SH/RE moments
         // that occur from the start to the time the deck is composite... what will remain is the
         // moments from composite deck to final
         std::vector<Float64> vMcr = pForces->GetMoment(firstCompositeDeckIntervalIdx,lcCR,vPoi,bat,rtCumulative);
         std::vector<Float64> vMsh = pForces->GetMoment(firstCompositeDeckIntervalIdx,lcSH,vPoi,bat,rtCumulative);
         std::vector<Float64> vMre = pForces->GetMoment(firstCompositeDeckIntervalIdx,lcRE,vPoi,bat,rtCumulative);

         if ( !IsEqual(gCRMax,1.0) )
         {
            std::vector<Float64> vMcrMax;
            vMcrMax.reserve(vMcr.size());
            std::transform(vMcr.cbegin(),vMcr.cend(),std::back_inserter(vMcrMax), [&gCRMax](const Float64& value) {return value*gCRMax;});
            std::transform(vMoment.cbegin(),vMoment.cend(),vMcrMax.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            std::transform(vMoment.cbegin(),vMoment.cend(),vMcr.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a - b;});
         }

         if ( !IsEqual(gSHMax,1.0) )
         {
            std::vector<Float64> vMshMax;
            vMshMax.reserve(vMsh.size());
            std::transform(vMsh.cbegin(),vMsh.cend(),std::back_inserter(vMshMax), [&gSHMax](const Float64& value) {return value*gSHMax;});
            std::transform(vMoment.cbegin(),vMoment.cend(),vMshMax.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            std::transform(vMoment.cbegin(),vMoment.cend(),vMsh.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a - b;});
         }

         if ( !IsEqual(gREMax,1.0) )
         {
            std::vector<Float64> vMreMax;
            vMreMax.reserve(vMre.size());
            std::transform(vMre.cbegin(),vMre.cend(),std::back_inserter(vMreMax), [&gREMax](const Float64& value) {return value*gREMax;});
            std::transform(vMoment.cbegin(),vMoment.cend(),vMreMax.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            std::transform(vMoment.cbegin(),vMoment.cend(),vMre.cbegin(),vMoment.begin(),[](const auto& a, const auto& b) {return a - b;});
         }
      }
   }


   return vMoment;
}

///////////////////////////////////
// IExternalLoading
bool CGirderModelManager::CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName)
{
   CGirderModelData* pModel = GetGirderModel(girderLineIdx);
   if ( pModel->m_Model) 
   {
      CComPtr<ILoadGroups> loadGroups;
      pModel->m_Model->get_LoadGroups(&loadGroups);
      HRESULT hr = AddLoadGroup(loadGroups,CComBSTR(strLoadingName),CComBSTR(strLoadingName));
      if ( FAILED(hr) )
      {
         ATLASSERT(false);
         return false;
      }
   }

   if ( pModel->m_ContinuousModel) 
   {
      CComPtr<ILoadGroups> loadGroups;
      pModel->m_ContinuousModel->get_LoadGroups(&loadGroups);
      HRESULT hr = AddLoadGroup(loadGroups,CComBSTR(strLoadingName),CComBSTR(strLoadingName));
      if ( FAILED(hr) )
      {
         ATLASSERT(false);
         return false;
      }
   }

   return true;
}

bool CGirderModelManager::AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo)
{
   // get the LBAM load case name for the combo type
   CComBSTR bstrLoadCaseName = GetLoadCaseName(lcCombo);

   CGirderModelData* pModel = GetGirderModel(girderLineIdx);

   if ( pModel->m_Model )
   { 
      // simple span model
      CComPtr<ILoadCases> load_cases;
      pModel->m_Model->get_LoadCases(&load_cases);

      CComPtr<ILoadCase> load_case;
      if ( FAILED(load_cases->Find(bstrLoadCaseName, &load_case)) )
      {
         ATLASSERT(false);
         return false;
      }

      if ( FAILED(load_case->AddLoadGroup(CComBSTR(strLoadingName))) )
      {
         ATLASSERT(false);
         return false;
      }
   }


   if ( pModel->m_ContinuousModel )
   { 
      // simple span model
      CComPtr<ILoadCases> load_cases;
      pModel->m_ContinuousModel->get_LoadCases(&load_cases);

      CComPtr<ILoadCase> load_case;
      if ( FAILED(load_cases->Find(bstrLoadCaseName, &load_case)) )
      {
         ATLASSERT(false);
         return false;
      }

      if ( FAILED(load_case->AddLoadGroup(CComBSTR(strLoadingName))) )
      {
         ATLASSERT(false);
         return false;
      }
   }

   return false;
}

bool CGirderModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   CGirderModelData* pModel = GetGirderModel(GetGirderLineIndex(poi.GetSegmentKey()));

   if ( pModel->m_Model )
   {
      CreateConcentratedLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,strLoadingName,poi,Fx,Fy,Mz);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateConcentratedLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,strLoadingName,poi,Fx,Fy,Mz);
   }

   return true;
}

bool CGirderModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   CGirderModelData* pModel = GetGirderModel(GetGirderLineIndex(poi.GetSegmentKey()));

   if ( pModel->m_Model )
   {
      CreateConcentratedLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,pfType,poi,Fx,Fy,Mz);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateConcentratedLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,pfType,poi,Fx,Fy,Mz);
   }

   return true;
}

bool CGirderModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   CGirderModelData* pModel = GetGirderModel(GetGirderLineIndex(poi1.GetSegmentKey()));

   if ( pModel->m_Model )
   {
      CreateUniformLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,strLoadingName,poi1,poi2,wx,wy);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateUniformLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,strLoadingName,poi1,poi2,wx,wy);
   }

   return true;
}

bool CGirderModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   CGirderModelData* pModel = GetGirderModel(GetGirderLineIndex(poi1.GetSegmentKey()));

   if ( pModel->m_Model )
   {
      CreateUniformLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,pfType,poi1,poi2,wx,wy);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateUniformLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,pfType,poi1,poi2,wx,wy);
   }

   return true;
}

bool CGirderModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   CGirderModelData* pModel = GetGirderModel(GetGirderLineIndex(poi1.GetSegmentKey()));

   if ( pModel->m_Model )
   {
      CreateInitialStrainLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,strLoadingName,poi1,poi2,e,r);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateInitialStrainLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,strLoadingName,poi1,poi2,e,r);
   }

   return true;
}

bool CGirderModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   CGirderModelData* pModel = GetGirderModel(GetGirderLineIndex(poi1.GetSegmentKey()));

   if ( pModel->m_Model )
   {
      CreateInitialStrainLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,pfType,poi1,poi2,e,r);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateInitialStrainLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,pfType,poi1,poi2,e,r);
   }

   return true;
}

std::vector<Float64> CGirderModelManager::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }

   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 PxLeft, PxRight;
      result->get_XLeft(&PxLeft);
      result->get_XRight(&PxRight);

      Float64 Px;
      if ( poi.HasAttribute(POI_START_FACE) )
      {
         Px = -PxRight;
      }
      else
      {
         Px = PxLeft;
      }

      results.push_back(Px);

      idx++;
   }

   return results;
}

std::vector<WBFL::System::SectionValue> CGirderModelManager::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi)); 
   
   std::vector<WBFL::System::SectionValue> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }

   IndexType nResults;
   section_results->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 FyLeft, FyRight;
      result->get_YLeft(&FyLeft);
      result->get_YRight(&FyRight);

      WBFL::System::SectionValue V(-FyLeft,FyRight);
      results.push_back(V);
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
   }

   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 MzLeft, MzRight;
      result->get_ZLeft(&MzLeft);
      result->get_ZRight(&MzRight);

      Float64 Mz;
      if ( poi.HasAttribute(POI_START_FACE) )
      {
         Mz = -MzRight;
      }
      else
      {
         Mz = MzLeft;
      }

      results.push_back(Mz);
      
      idx++;
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetDy][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetDy][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }

   IndexType nResults;
   section_results->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 Dy, Dyr;
      result->get_YLeft(&Dy);
      result->get_YRight(&Dyr);

      ATLASSERT(IsEqual(Dy,Dyr));
      results.push_back(Dy);
   }

   return results;
}

std::vector<Float64> CGirderModelManager::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);


   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionResult3Ds> section_results;
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetRz][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetRz][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
   }

   IndexType nResults;
   section_results->get_Count(&nResults);
   ATLASSERT(nResults == (IndexType)vPoi.size());
   for (IndexType idx = 0; idx < nResults; idx++)
   {
      CComPtr<ISectionResult3D> result;
      section_results->get_Item(idx,&result);

      Float64 Rz, Rzr;
      result->get_ZLeft(&Rz);
      result->get_ZRight(&Rzr);

      ATLASSERT(IsEqual(Rz,Rzr));
      results.push_back(Rz);
   }

   return results;
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pfTop->reserve(vPoi.size());
   pfBot->reserve(vPoi.size());

   CGirderModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   ResultsSummationType resultsSummation = (resultsType == rtIncremental ? rsIncremental : rsCumulative);

   CComPtr<ISectionStressResults> min_results, max_results, results;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
   {
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &max_results);
      ar = pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &min_results);
   }
   else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
   {
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &max_results);
      ar = pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &min_results);
   }
   else
   {
      ar = pModelData->pLoadGroupResponse[bat]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &results);
   }

   CollectionIndexType stress_point_index_top = GetStressPointIndex(topLocation);
   CollectionIndexType stress_point_index_bot = GetStressPointIndex(botLocation);

   GET_IFACE(IPointOfInterest,pPoi);

   IndexType idx = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      Float64 fTop, fBot;

      if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
      {
         CComPtr<ISectionStressResult> top_stresses;
         max_results->get_Item(idx,&top_stresses);

         CComPtr<ISectionStressResult> bot_stresses;
         min_results->get_Item(idx,&bot_stresses);

         if ( IsZero(Xg) )
         {
            top_stresses->GetRightResult(stress_point_index_top,&fTop);
            bot_stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            top_stresses->GetLeftResult(stress_point_index_top,&fTop);
            bot_stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }
      else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
      {
         CComPtr<ISectionStressResult> top_stresses;
         min_results->get_Item(idx,&top_stresses);

         CComPtr<ISectionStressResult> bot_stresses;
         max_results->get_Item(idx,&bot_stresses);

         if ( IsZero(Xg) )
         {
            top_stresses->GetRightResult(stress_point_index_top,&fTop);
            bot_stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            top_stresses->GetLeftResult(stress_point_index_top,&fTop);
            bot_stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }
      else
      {
         CComPtr<ISectionStressResult> stresses;
         results->get_Item(idx,&stresses);

         if ( IsZero(Xg) )
         {
            stresses->GetRightResult(stress_point_index_top,&fTop);
            stresses->GetRightResult(stress_point_index_bot,&fBot);
         }
         else
         {
            stresses->GetLeftResult(stress_point_index_top,&fTop);
            stresses->GetLeftResult(stress_point_index_bot,&fBot);
         }
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);

      idx++;
   }
}

std::vector<REACTION> CGirderModelManager::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // nothing is erected onto the supports yet
      return std::vector<REACTION>(vSupports.size());
   }

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   std::vector<REACTION> reactions;
   reactions.reserve(vSupports.size());

   CComBSTR bstrLoadGroup( strLoadingName );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   for(const auto& support : vSupports)
   {
      SupportIndexType supportIdx = support.first;
      pgsTypes::SupportType supportType = support.second;
      ConfigureLBAMPoisForReactions(girderKey,supportIdx,supportType);

      CComPtr<IResult3Ds> results;

      ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

      if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
      {
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
      }
      else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
      {
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
      }
      else
      {
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = pModelData->pLoadGroupResponse[bat]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
      }

      REACTION Reaction;
      CollectionIndexType nResults;
      results->get_Count(&nResults);
      for ( CollectionIndexType i = 0; i < nResults; i++ )
      {
         CComPtr<IResult3D> result;
         results->get_Item(i,&result);

         REACTION reaction;
         result->get_X(&reaction.Fx);
         result->get_Y(&reaction.Fy);
         result->get_Z(&reaction.Mz);

         Reaction += reaction;
      }
      reactions.push_back(Reaction);
   }

   return reactions;
}

////////////////////////////////////
// IReactions
std::vector<REACTION> CGirderModelManager::GM_GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   if ( intervalIdx < firstSegmentErectionIntervalIdx || pfType == pgsTypes::pftPostTensioning )
   {
      // nothing is erected onto the supports yet
      //
      // Prestress and primary post-tensioning don't cause reactions
      // they only cause direction axial compression and bending
      // (secondary PT REACTION reactions)
      return std::vector<REACTION>(vSupports.size());
   }

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   std::vector<REACTION> reactions;
   reactions.reserve(vSupports.size());

   CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   for (const auto& support : vSupports)
   {
      SupportIndexType supportIdx = support.first;
      pgsTypes::SupportType supportType = support.second;
      ConfigureLBAMPoisForReactions(girderKey,supportIdx,supportType);

      CComPtr<IResult3Ds> results;

      ResultsSummationType resultsSummation = (resultsType == rtCumulative ? rsCumulative : rsIncremental);

      IntervalIndexType tsRemovalIntervalIdx = INVALID_INDEX;
      if ( supportType == pgsTypes::stTemporary )
      {
         tsRemovalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(supportIdx);
      }

      REACTION Reaction;
      if ( supportType == pgsTypes::stTemporary && tsRemovalIntervalIdx <= intervalIdx )
      {
         Reaction.Fx = 0;
         Reaction.Fy = 0;
         Reaction.Mz = 0;
      }
      else
      {
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
         {
            CAnalysisResult ar(_T(__FILE__),__LINE__);
            ar = pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
         }
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
         {
            CAnalysisResult ar(_T(__FILE__),__LINE__);
            ar = pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
         }
         else
         {
            CAnalysisResult ar(_T(__FILE__),__LINE__);
            ar = pModelData->pLoadGroupResponse[bat]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
         }

         CollectionIndexType nResults;
         results->get_Count(&nResults);
         for ( CollectionIndexType i = 0; i < nResults; i++ )
         {
            CComPtr<IResult3D> result;
            results->get_Item(i,&result);

            REACTION reaction;
            result->get_X(&reaction.Fx);
            result->get_Y(&reaction.Fy);
            result->get_Z(&reaction.Mz);

            Reaction += reaction;
         }
      }
      reactions.push_back(Reaction);
   }

   return reactions;
}

std::vector<REACTION> CGirderModelManager::GM_GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   ATLASSERT(comboType != lcPS);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // nothing is erected onto the supports yet
      //
      // Prestress and primary post-tensioning don't cause reactions
      // they only cause direction axial compression and bending
      return std::vector<REACTION>(vSupports.size());
   }

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   std::vector<REACTION> reactions;
   reactions.reserve(vSupports.size());

   CComPtr<ILBAMModel> lbam;
   GetLBAM(pModelData, bat, &lbam);

   CComPtr<ILoadCases> load_cases;
   lbam->get_LoadCases(&load_cases);

   CComBSTR combo_name = GetLoadCaseName(comboType);

   CComPtr<ILoadCase> load_case;
   load_cases->Find(combo_name, &load_case);

   CollectionIndexType nLoadGroups;
   load_case->get_LoadGroupCount(&nLoadGroups);



   for (const auto& support : vSupports)
   {
      SupportIndexType supportIdx = support.first;
      pgsTypes::SupportType supportType = support.second;

      // Cycle through load cases and sum reactions
      REACTION R;
      for (CollectionIndexType ldGroupIdx = 0; ldGroupIdx < nLoadGroups; ldGroupIdx++)
      {
         CComBSTR lg_name;
         load_case->GetLoadGroup(ldGroupIdx, &lg_name);

         pgsTypes::ProductForceType pfType = GetProductForceType(lg_name);

         REACTION r = GetReaction(intervalIdx, pfType, supportIdx,supportType, girderKey, bat, resultsType);

         R += r;
      }
      reactions.push_back(R);
   }

   return reactions;
}

void CGirderModelManager::GM_GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax) const
{
   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   pRmin->clear();
   pRmax->clear();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   CComBSTR bstrLoadCombo( GetLoadCombinationName(limitState) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

   VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

   for (const auto& support : vSupports)
   {
      SupportIndexType supportIdx = support.first;
      pgsTypes::SupportType supportType = support.second;

      ConfigureLBAMPoisForReactions(girderKey,supportIdx,supportType);

      IntervalIndexType tsRemovalIntervalIdx = INVALID_INDEX;
      if ( supportType == pgsTypes::stTemporary )
      {
         tsRemovalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(supportIdx);
      }

      REACTION MaxReaction;
      REACTION MinReaction;
      if ( supportType == pgsTypes::stTemporary && tsRemovalIntervalIdx <= intervalIdx )
      {
         MaxReaction.Fx = 0;
         MaxReaction.Fy = 0;
         MaxReaction.Mz = 0;

         MinReaction.Fx = 0;
         MinReaction.Fy = 0;
         MinReaction.Mz = 0;
      }
      else
      {
         CComPtr<ILoadCombinationResults> FxMaxResults, FxMinResults;
         CComPtr<ILoadCombinationResults> FyMaxResults, FyMinResults;
         CComPtr<ILoadCombinationResults> MzMaxResults, MzMinResults;
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFx, optMaximize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &FxMaxResults);
         ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFx, optMinimize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &FxMinResults);
         ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &FyMaxResults);
         ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &FyMinResults);
         ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &MzMaxResults);
         ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &MzMinResults);

         CollectionIndexType nResults;
         FxMaxResults->get_Count(&nResults);
#if defined _DEBUG
         IndexType n;
         FxMinResults->get_Count(&n);
         ATLASSERT(n == nResults);

         FyMaxResults->get_Count(&n);
         ATLASSERT(n == nResults);
         FyMinResults->get_Count(&n);
         ATLASSERT(n == nResults);

         MzMaxResults->get_Count(&n);
         ATLASSERT(n == nResults);
         MzMinResults->get_Count(&n);
         ATLASSERT(n == nResults);
#endif
         for ( CollectionIndexType i = 0; i < nResults; i++ )
         {
            REACTION reaction;
            FxMaxResults->GetResult(i,&reaction.Fx,nullptr);
            FyMaxResults->GetResult(i,&reaction.Fy,nullptr);
            MzMaxResults->GetResult(i,&reaction.Mz,nullptr);
            MaxReaction += reaction;

            FxMinResults->GetResult(i,&reaction.Fx,nullptr);
            FyMinResults->GetResult(i,&reaction.Fy,nullptr);
            MzMinResults->GetResult(i,&reaction.Mz,nullptr);
            MinReaction += reaction;
         }
      }

      pRmin->push_back(MinReaction);
      pRmax->push_back(MaxReaction);
   }
}

void CGirderModelManager::GM_GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   pRmin->clear();
   pRmax->clear();

   if ( pMinAxleConfig )
   {
      pMinAxleConfig->clear();
   }

   if ( pMaxAxleConfig )
   {
      pMaxAxleConfig->clear();
   }

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   for ( const auto& pierIdx : vPiers)
   {
      ConfigureLBAMPoisForReactions(girderKey,pierIdx,pgsTypes::stPier);

      CComPtr<ILiveLoadModelResults> minResults[3], maxResults[3];
      CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      for ( int i = 0; i < 3; i++ )
      {
         ForceEffectType fet = (ForceEffectType)i;
         ar = pModelData->pVehicularResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, vehicleIdx, fet, optMinimize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults[fet]);
         ar = pModelData->pVehicularResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, vehicleIdx, fet, optMaximize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults[fet]);
      }

      CComPtr<ILiveLoadConfiguration> RzMaxConfig;
      CComPtr<ILiveLoadConfiguration> RzMinConfig;
      Float64 Rmax = -DBL_MAX;
      Float64 Rmin = DBL_MAX;
      REACTION maxReaction, minReaction;
      CollectionIndexType nResults;
      maxResults[fetFx]->get_Count(&nResults);
      for ( CollectionIndexType i = 0; i < nResults; i++ )
      {
         Float64 rmax;
         CComPtr<ILiveLoadConfiguration> rzMaxConfig;
         maxResults[fetFy]->GetResult(i,&rmax,pMaxAxleConfig ? &rzMaxConfig : nullptr);
         if ( Rmax < rmax )
         {
            Rmax = rmax;
            RzMaxConfig.Release();
            RzMaxConfig = rzMaxConfig;

            maxResults[fetFx]->GetResult(i,&maxReaction.Fx,nullptr);
            maxResults[fetFy]->GetResult(i,&maxReaction.Fy,nullptr);
            maxResults[fetMz]->GetResult(i,&maxReaction.Mz,nullptr);
         }
         
         Float64 rmin;
         CComPtr<ILiveLoadConfiguration> rzMinConfig;
         minResults[fetFy]->GetResult(i,&rmin,pMinAxleConfig ? &rzMinConfig : nullptr);
         if ( rmin < Rmin )
         {
            Rmin = rmin;
            RzMinConfig.Release();
            RzMinConfig = rzMinConfig;

            minResults[fetFx]->GetResult(i,&minReaction.Fx,nullptr);
            minResults[fetFy]->GetResult(i,&minReaction.Fy,nullptr);
            minResults[fetMz]->GetResult(i,&minReaction.Mz,nullptr);
         }
      }

      pRmin->push_back(minReaction);
      pRmax->push_back(maxReaction);

      CComPtr<ILBAMModel> lbam_model;
      GetLBAM(pModelData,bat,&lbam_model);
      if ( pMaxAxleConfig )
      {
         AxleConfiguration maxConfig;
         CreateAxleConfig(lbam_model, RzMaxConfig, &maxConfig);
         pMaxAxleConfig->push_back(maxConfig);
      }

      if ( pMinAxleConfig )
      {
         AxleConfiguration minConfig;
         CreateAxleConfig(lbam_model, RzMinConfig, &minConfig);
         pMinAxleConfig->push_back(minConfig);
      }
   } // next pier
}

void CGirderModelManager::GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx,std::vector<VehicleIndexType>* pMaxVehIdx) const
{
   GM_GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,pgsTypes::fetRz,pRmin,pRmax,nullptr,nullptr,pMinVehIdx,pMaxVehIdx);
}

void CGirderModelManager::GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<REACTION> Rmin;
   std::vector<REACTION> Rmax;
   std::vector<Float64> Tmin;
   std::vector<Float64> Tmax;
   std::vector<VehicleIndexType> vMinVehIdx;
   std::vector<VehicleIndexType> vMaxVehIdx;

   GM_GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,fetDeflection,&Rmin,&Rmax,&Tmin,&Tmax,&vMinVehIdx,&vMaxVehIdx);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();

   *pTmin = Tmin.front();
   *pTmax = Tmax.front();

   if ( pMinVehIdx )
   {
      *pMinVehIdx = vMinVehIdx.front();
   }
   
   if ( pMaxVehIdx )
   {
      *pMaxVehIdx = vMaxVehIdx.front();
   }
}


void CGirderModelManager::GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx,std::vector<VehicleIndexType>* pMaxVehIdx) const
{
   pRmin->clear();
   pRmax->clear();

   if ( pTmin )
   {
      pTmin->clear();
   }

   if ( pTmax )
   {
      pTmax->clear();
   }
   
   if ( pMinVehIdx )
   {
      pMinVehIdx->clear();
   }
   
   if ( pMaxVehIdx )
   {
      pMaxVehIdx->clear();
   }

   for( const auto& pierIdx : vPiers)
   {
      ConfigureLBAMPoisForReactions(girderKey,pierIdx,pgsTypes::stPier);

      REACTION Rmin, Rmax;
      Float64 Tmin, Tmax;
      VehicleIndexType minVehIdx, maxVehIdx;
      GM_GetLiveLoadReaction(intervalIdx,llType,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,fetDeflection,&Rmin,&Rmax,&Tmin,&Tmax,&minVehIdx,&maxVehIdx);

      pRmin->push_back(Rmin);
      pRmax->push_back(Rmax);

      if ( pTmin )
      {
         pTmin->push_back(Tmin);
      }

      if ( pTmax )
      {
         pTmax->push_back(Tmax);
      }

      if ( pMinVehIdx )
      {
         pMinVehIdx->push_back(minVehIdx);
      }

      if ( pMaxVehIdx )
      {
         pMaxVehIdx->push_back(maxVehIdx);
      }
   }
}

void CGirderModelManager::GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   // Tricky:: We play a game here where the Pedestrian uniform lane load value is equal to the live load distribution factor. In the LBAM, the lane load is a unit value.
   //          This means that we must always include the LLDF for pedestrian loads, and the response is always per girder. Force the issue:
   if (llType == pgsTypes::lltPedestrian)
   {
      bIncludeLLDF = true; 
   }

   if ( pTmin )
   {
      *pTmin = 0;
   }

   if ( pTmax )
   {
      *pTmax = 0;
   }
   
   if ( pMinVehIdx )
   {
      *pMinVehIdx = INVALID_INDEX;
   }
   
   if ( pMaxVehIdx )
   {
      *pMaxVehIdx = INVALID_INDEX;
   }

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelResults> minResults;
   CComPtr<ILiveLoadModelResults> maxResults;
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   // Get optimized live load model responses for the primary force effect tyep
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, 
          (ForceEffectType)fetPrimary, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   ar = pModelData->pLiveLoadResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, 
          (ForceEffectType)fetPrimary, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   CComPtr<ILiveLoadConfiguration> MinConfig;
   CComPtr<ILiveLoadConfiguration> MaxConfig;

   CComPtr<ILBAMAnalysisEngine> pEngine;
   GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
   CComPtr<IBasicVehicularResponse> response;
   pEngine->get_BasicVehicularResponse(&response);

   REACTION rMin, rMax;
   Float64 Rmax = -DBL_MAX;
   Float64 Rmin = DBL_MAX;
   CollectionIndexType nResults;
   maxResults->get_Count(&nResults);
   for ( CollectionIndexType i = 0; i < nResults; i++ )
   {
      Float64 rmax;
      CComPtr<ILiveLoadConfiguration> maxConfig;
      maxResults->GetResult(i,&rmax,&maxConfig);
      if ( Rmax < rmax )
      {
         // we found a new maximum value for the primary force effect type...
         // now get the corresponding reaction values
         Rmax = rmax;

         // get the vehicle index from the configuration that caused the optimized primary force effect response
         ATLASSERT(maxConfig);
         VehicleIndexType vehicleIdx;
         maxConfig->get_VehicleIndex(&vehicleIdx);
         if ( pMaxVehIdx )
         {
            *pMaxVehIdx = vehicleIdx;
         }
         MaxConfig.Release();
         MaxConfig = maxConfig;

         MaxConfig->put_Optimization(optMaximize);

         // compute the reactions for the two secondary force effect types
         Float64 Fx = 0;
         Float64 Fy = 0;
         Float64 Mz = 0;
         if ( fetPrimary == pgsTypes::fetFx )
         {
            Fx = Rmax;

            CComPtr<IResult3Ds> fyResults, mzResults;
            MaxConfig->put_ForceEffect(fetFy);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MaxConfig, &fyResults );
            MaxConfig->put_ForceEffect(fetMz);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MaxConfig, &mzResults );

            CollectionIndexType nResults;
            fyResults->get_Count(&nResults);
            for ( CollectionIndexType i = 0; i < nResults; i++ )
            {
               CComPtr<IResult3D> result;
               fyResults->get_Item(i,&result);

               Float64 fy;
               result->get_Y(&fy);
               Fy += fy;

               result.Release();
               mzResults->get_Item(i,&result);
               Float64 mz;
               result->get_Z(&mz);
               Mz += mz;
            }
         }
         else if ( fetPrimary == pgsTypes::fetFy )
         {
            Fy = Rmax;

            CComPtr<IResult3Ds> fxResults, mzResults;
            MaxConfig->put_ForceEffect(fetFx);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MaxConfig, &fxResults );
            MaxConfig->put_ForceEffect(fetMz);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MaxConfig, &mzResults );

            CollectionIndexType nResults;
            fxResults->get_Count(&nResults);
            for ( CollectionIndexType i = 0; i < nResults; i++ )
            {
               CComPtr<IResult3D> result;
               fxResults->get_Item(i,&result);

               Float64 fx;
               result->get_X(&fx);
               Fx += fx;

               result.Release();
               mzResults->get_Item(i,&result);
               Float64 mz;
               result->get_Z(&mz);
               Mz += mz;
            }
         }
         else if ( fetPrimary == pgsTypes::fetMz )
         {
            Mz = Rmax;

            CComPtr<IResult3Ds> fxResults, fyResults;
            MaxConfig->put_ForceEffect(fetFx);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MaxConfig, &fxResults );
            MaxConfig->put_ForceEffect(fetFy);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MaxConfig, &fyResults );

            CollectionIndexType nResults;
            fxResults->get_Count(&nResults);
            for ( CollectionIndexType i = 0; i < nResults; i++ )
            {
               CComPtr<IResult3D> result;
               fxResults->get_Item(i,&result);

               Float64 fx;
               result->get_X(&fx);
               Fx += fx;

               result.Release();
               fyResults->get_Item(i,&result);
               Float64 fy;
               result->get_Y(&fy);
               Fy += fy;
            }
         }
         else
         {
            ATLASSERT(false); // should never get here
         }

         rMax.Fx = Fx;
         rMax.Fy = Fy;
         rMax.Mz = Mz;
      }

      Float64 rmin;
      CComPtr<ILiveLoadConfiguration> minConfig;
      minResults->GetResult(i,&rmin,&minConfig);
      if ( rmin < Rmin )
      {
         Rmin = rmin;
         ATLASSERT(minConfig);
         VehicleIndexType vehicleIdx;
         minConfig->get_VehicleIndex(&vehicleIdx);
         if ( pMinVehIdx )
         {
            *pMinVehIdx = vehicleIdx;
         }
         MinConfig.Release();
         MinConfig = minConfig;

         MinConfig->put_Optimization(optMinimize);

         // compute the reactions for the two secondary force effect types
         Float64 Fx = 0;
         Float64 Fy = 0;
         Float64 Mz = 0;
         if ( fetPrimary == pgsTypes::fetFx )
         {
            Fx = Rmin;

            CComPtr<IResult3Ds> fyResults, mzResults;
            MinConfig->put_ForceEffect(fetFy);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MinConfig, &fyResults );
            MinConfig->put_ForceEffect(fetMz);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MinConfig, &mzResults );

            CollectionIndexType nResults;
            fyResults->get_Count(&nResults);
            for ( CollectionIndexType i = 0; i < nResults; i++ )
            {
               CComPtr<IResult3D> result;
               fyResults->get_Item(i,&result);

               Float64 fy;
               result->get_Y(&fy);
               Fy += fy;

               result.Release();
               mzResults->get_Item(i,&result);
               Float64 mz;
               result->get_Z(&mz);
               Mz += mz;
            }
         }
         else if ( fetPrimary == pgsTypes::fetFy )
         {
            Fy = Rmin;

            CComPtr<IResult3Ds> fxResults, mzResults;
            MinConfig->put_ForceEffect(fetFx);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MinConfig, &fxResults );
            MinConfig->put_ForceEffect(fetMz);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MinConfig, &mzResults );

            CollectionIndexType nResults;
            fxResults->get_Count(&nResults);
            for ( CollectionIndexType i = 0; i < nResults; i++ )
            {
               CComPtr<IResult3D> result;
               fxResults->get_Item(i,&result);

               Float64 fx;
               result->get_X(&fx);
               Fx += fx;

               result.Release();
               mzResults->get_Item(i,&result);
               Float64 mz;
               result->get_Z(&mz);
               Mz += mz;
            }
         }
         else if ( fetPrimary == pgsTypes::fetMz )
         {
            Mz = Rmin;

            CComPtr<IResult3Ds> fxResults, fyResults;
            MinConfig->put_ForceEffect(fetFx);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MinConfig, &fxResults );
            MinConfig->put_ForceEffect(fetFy);
            response->ComputeReactions( m_LBAMPoi, bstrStage, MinConfig, &fyResults );

            CollectionIndexType nResults;
            fxResults->get_Count(&nResults);
            for ( CollectionIndexType i = 0; i < nResults; i++ )
            {
               CComPtr<IResult3D> result;
               fxResults->get_Item(i,&result);

               Float64 fx;
               result->get_X(&fx);
               Fx += fx;

               result.Release();
               fyResults->get_Item(i,&result);
               Float64 fy;
               result->get_Y(&fy);
               Fy += fy;
            }
         }
         else
         {
            ATLASSERT(false); // should never get here
         }

         rMin.Fx = Fx;
         rMin.Fy = Fy;
         rMin.Mz = Mz;
      }
   }

   *pRmin = rMin;
   *pRmax = rMax;

   if ( pTmax )
   {
      if ( MaxConfig )
      {
         // get deflection that corresonds to R max
         CComPtr<IResult3Ds> results;
         MaxConfig->put_ForceEffect((ForceEffectType)fetDeflection);
         MaxConfig->put_Optimization(optMaximize);
         response->ComputeSupportDeflections( m_LBAMPoi, bstrStage, MaxConfig, &results );

         Float64 T = 0;
         CollectionIndexType nResults;
         results->get_Count(&nResults);
         for ( CollectionIndexType i = 0; i < nResults; i++ )
         {
            CComPtr<IResult3D> result;
            results->get_Item(i,&result);

            Float64 t;
            result->get_Z(&t);
            T += t;
         }
         *pTmax = T;
      }
      else
      {
         *pTmax = -1;
      }
   }

   if ( pTmin )
   {
      if ( MinConfig )
      {
         // get deflection that corresonds to R min
         CComPtr<IResult3Ds> results;
         MinConfig->put_ForceEffect((ForceEffectType)fetDeflection);
         MinConfig->put_Optimization(optMaximize);
         response->ComputeSupportDeflections( m_LBAMPoi, bstrStage, MinConfig, &results );

         Float64 T = 0;
         CollectionIndexType nResults;
         results->get_Count(&nResults);
         for ( CollectionIndexType i = 0; i < nResults; i++ )
         {
            CComPtr<IResult3D> result;
            results->get_Item(i,&result);

            Float64 t;
            result->get_Z(&t);
            T += t;
         }
         *pTmin = T;
      }
      else
      {
         *pTmin = -1;
      }
   }
}

void CGirderModelManager::GM_GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const
{
   for( const auto& pierIdx : vPiers)
   {
      ConfigureLBAMPoisForReactions(girderKey,pierIdx,pgsTypes::stPier);

      Float64 Rmin, Rmax;
      GM_GetCombinedLiveLoadReaction(intervalIdx,llType,girderKey,bat,bIncludeImpact,&Rmin,&Rmax);

      pRmin->push_back(Rmin);
      pRmax->push_back(Rmax);
   }
}

void CGirderModelManager::GM_GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) const
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   ATLASSERT(pIntervals->GetLiveLoadInterval() <= intervalIdx);
#endif

   // Start by checking if the model exists
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey),bat);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

   CComBSTR bstrLoadCombo( GetLiveLoadName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationResults> maxResults, minResults;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, vbIncludeImpact, VARIANT_FALSE, &maxResults);
   ar = pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, vbIncludeImpact, VARIANT_FALSE, &minResults);

   CollectionIndexType nResults;
   maxResults->get_Count(&nResults);
   Float64 FyMax = 0;
   Float64 FyMin = 0;
   for ( CollectionIndexType i = 0; i < nResults; i++ )
   {
      Float64 fyMax;
      maxResults->GetResult(i,&fyMax,nullptr);
      FyMax += fyMax;

      Float64 fyMin;
      minResults->GetResult(i,&fyMin,nullptr);
      FyMin += fyMin;
   }

   *pRmin = FyMin;
   *pRmax = FyMax;
}

////////////////////////////////////
// IContraflexurePoints
void CGirderModelManager::GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);

   CGirderModelData* pModelData = GetGirderModel(GetGirderLineIndex(girderKey));

   GET_IFACE(IBridge,pBridge);

   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanKey.spanIndex; i++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCountBySpan(i);
      span_start += pBridge->GetSpanLength(CSpanKey(i,Min(spanKey.girderIndex,nGirders-1)));
   }

   Float64 span_length = pBridge->GetSpanLength(spanKey);
   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];

   CComPtr<ILBAMAnalysisEngine> pEngine;
   GetEngine(pModelData,true,&pEngine);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadInterval = pIntervals->GetLiveLoadInterval();
   CComBSTR bstrStageName( GetLBAMStageName(liveLoadInterval) );

   CComPtr<ILoadGroupResponse> response;
   pEngine->get_LoadGroupResponse(&response);
   CComQIPtr<IContraflexureResponse> cfresponse(response);
   CComPtr<IDblArray> cf_locs;
   cfresponse->ComputeContraflexureLocations(bstrStageName,&cf_locs);
   
   *nPoints = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

   if ( *nPoints == 1 )
   {
      cfPoints[0] = cf_points_in_span[0] - span_start;
   }
   else if ( *nPoints == 2 )
   {
      cfPoints[0] = cf_points_in_span[0] - span_start;
      cfPoints[1] = cf_points_in_span[1] - span_start;
   }
}

//////////////////////////////////////////////////
// IBearingDesign
Float64 CGirderModelManager::GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,
                                             pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   ASSERT_GIRDER_KEY(location.GirderKey);

#if defined _DEBUG
   GET_IFACE(IBearingDesign,pBearingDesign);
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(intervalIdx,location.GirderKey);
   std::vector<PierIndexType>::iterator found = std::find(vPiers.begin(),vPiers.end(),location.PierIdx);
   ATLASSERT( found != vPiers.end() ); // if this fires, we are requesting bearing reactions at a pier that doesn't have bearing reactions
#endif
   
   if ( location.Face == rftMid )
   {
      GET_IFACE(IBridge, pBridge);
      ATLASSERT(pBridge->IsInteriorPier(location.PierIdx));
      std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
      vSupports.emplace_back(location.PierIdx,pgsTypes::stPier);
      GET_IFACE(IReactions,pReactions);
      std::vector<REACTION> vR = pReactions->GetReaction(location.GirderKey,vSupports,intervalIdx,pfType,bat,resultsType);
      Float64 R = vR.front().Fy;
      return R;
   }
   else
   {
      SupportIDType backID, aheadID;
      GetPierSupportIDs(location, &backID, &aheadID);
      SupportIDType supportID = (location.Face == rftBack ? backID : aheadID);

      REACTION reaction = GetBearingReaction(intervalIdx, pfType, supportID, location.GirderKey, bat, resultsType);
      return reaction.Fy;
   }
}

void CGirderModelManager::GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   ASSERT_GIRDER_KEY(location.GirderKey);

#if defined _DEBUG
   GET_IFACE(IBearingDesign, pBearingDesign);
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(intervalIdx, location.GirderKey);
   std::vector<PierIndexType>::iterator found = std::find(vPiers.begin(), vPiers.end(), location.PierIdx);
   ATLASSERT(found != vPiers.end()); // if this fires, we are requesting bearing reactions at a pier that doesn't have bearing reactions
#endif
   
   if ( location.Face == rftMid )
   {
      GET_IFACE(IBridge, pBridge);
      ATLASSERT(pBridge->IsInteriorPier(location.PierIdx));

      REACTION Rmin,Rmax;
      // get maximum vertical (Fy) reaction with corresponding rotation (Mz = Rz)
      GM_GetLiveLoadReaction(intervalIdx, llType, location.PierIdx, location.GirderKey, bat, bIncludeImpact, bIncludeLLDF, pgsTypes::fetFy, pgsTypes::fetRz, &Rmin, &Rmax, pTmin, pTmax, pMinVehIdx, pMaxVehIdx);
      *pRmin = Rmin.Fy;
      *pRmax = Rmax.Fy;
   }
   else
   {
      SupportIDType backID, aheadID;
      GetPierSupportIDs(location, &backID, &aheadID);
      SupportIDType supportID = (location.Face == rftBack ? backID : aheadID);

      m_LBAMPoi->Clear();
      m_LBAMPoi->Add(supportID);

      REACTION Rmin,Rmax;
      GM_GetLiveLoadReaction(intervalIdx,llType,location.GirderKey,bat,bIncludeImpact,bIncludeLLDF,pgsTypes::fetFy,pgsTypes::fetRz,&Rmin,&Rmax,pTmin,pTmax,pMinVehIdx,pMaxVehIdx);
      *pRmin = Rmin.Fy;
      *pRmax = Rmax.Fy;
   }
}

void CGirderModelManager::GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   ASSERT_GIRDER_KEY(location.GirderKey);

#if defined _DEBUG
   GET_IFACE(IBearingDesign, pBearingDesign);
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(intervalIdx, location.GirderKey);
   std::vector<PierIndexType>::iterator found = std::find(vPiers.begin(), vPiers.end(), location.PierIdx);
   ATLASSERT(found != vPiers.end()); // if this fires, we are requesting bearing reactions at a pier that doesn't have bearing reactions
#endif

   if ( location.Face == rftMid )
   {
      GET_IFACE(IProductForces,pForces);
      // rotation is the same on both sides of the pier
      pgsTypes::PierFaceType pierFace = pgsTypes::Back;
      pForces->GetLiveLoadRotation(intervalIdx, llType, location.PierIdx, location.GirderKey, pierFace, bat, bIncludeImpact, bIncludeLLDF, pTmin, pTmax, pRmin, pRmax, pMinVehIdx, pMaxVehIdx);
   }
   else
   {
      GET_IFACE(IPointOfInterest,pPoi);
      LiveLoadModelType llmt = g_LiveLoadModelType[llType];

      VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
      VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

      CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

      if ( location.Face == rftAhead )
      {
         CSegmentKey segmentKey(location.GirderKey,0);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
         ATLASSERT(vPoi.size() == 1);

         // Get max'd rotations from lbam
         CGirderModelData* pModelData = UpdateLBAMPois(vPoi);
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         CComPtr<ILiveLoadModelSectionResults> minResults;
         CComPtr<ILiveLoadModelSectionResults> maxResults;

         // may need to swap bat type for envelope mode... see product reactions
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                fetRz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
         ar = pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                fetRz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

         // Extract rotations and corresponding reactions
         Float64 TzMaxLeft, TzMaxRight;
         CComPtr<ILiveLoadConfiguration> TzMaxLeftConfig, TzMaxRightConfig;
         maxResults->GetResult(0,&TzMaxLeft, &TzMaxLeftConfig,
                                 &TzMaxRight,&TzMaxRightConfig);

         Float64 TzMinLeft, TzMinRight;
         CComPtr<ILiveLoadConfiguration> TzMinLeftConfig, TzMinRightConfig;
         minResults->GetResult(0,&TzMinLeft,  &TzMinLeftConfig,
                                 &TzMinRight, &TzMinRightConfig);

         *pTmin = TzMinLeft;
         *pTmax = TzMaxLeft;

         // Vehicle indexes
         if ( pMinVehIdx )
         {
            TzMinLeftConfig->get_VehicleIndex(pMinVehIdx);
         }

         if ( pMaxVehIdx )
         {
            TzMaxLeftConfig->get_VehicleIndex(pMaxVehIdx);
         }

         // Corresponding reactions (end shears)
         // get rotatation that corresonds to R min
         CComPtr<ISectionResult3Ds> results;
         TzMinLeftConfig->put_ForceEffect(fetFy);
         TzMinLeftConfig->put_Optimization(optMaximize);
         response->ComputeForces( m_LBAMPoi, bstrStage, roMember, TzMinLeftConfig, &results );

         CComPtr<ISectionResult3D> result;
         results->get_Item(0,&result);

         Float64 T;
         result->get_YLeft(&T);
         *pRmin = -T;

         results.Release();
         result.Release();

         // get rotation that corresonds to R max
         TzMaxLeftConfig->put_ForceEffect(fetRz);
         TzMaxLeftConfig->put_Optimization(optMaximize);
         response->ComputeForces( m_LBAMPoi, bstrStage, roMember, TzMaxLeftConfig, &results );

         results->get_Item(0,&result);
         result->get_YLeft(&T);
         *pRmax = -T;
      }
      else
      {
         ATLASSERT(location.Face == rftBack);

         GET_IFACE(IBridge,pBridge);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(location.GirderKey);
         CSegmentKey segmentKey(location.GirderKey,nSegments-1);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vPoi);
         ATLASSERT(vPoi.size() == 1);

         // Get max'd rotations from lbam
         CGirderModelData* pModelData = UpdateLBAMPois(vPoi);
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         CComPtr<ILiveLoadModelSectionResults> minResults;
         CComPtr<ILiveLoadModelSectionResults> maxResults;

         // Extremely TRICKY:
         // Below we are getting reactions from  end shear, we must flip sign of results to go 
         // from LBAM to beam coordinates. This means that the optimization must go the opposite when using the envelopers.
         pgsTypes::BridgeAnalysisType tmpbat = bat;
         if (bat == pgsTypes::MinSimpleContinuousEnvelope)
         {
            tmpbat = pgsTypes::MaxSimpleContinuousEnvelope;
         }
         else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
         {
            tmpbat = pgsTypes::MinSimpleContinuousEnvelope;
         }

         // may need to swap bat type for envelope mode... see product reactions
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = pModelData->pLiveLoadResponse[tmpbat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                fetRz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
         ar = pModelData->pLiveLoadResponse[tmpbat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                fetRz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);


         // Extract rotations and corresponding reactions
         Float64 TzMaxLeft, TzMaxRight;
         CComPtr<ILiveLoadConfiguration> TzMaxLeftConfig, TzMaxRightConfig;
         maxResults->GetResult(0,&TzMaxLeft, &TzMaxLeftConfig,
                                 &TzMaxRight,&TzMaxRightConfig);

         Float64 TzMinLeft, TzMinRight;
         CComPtr<ILiveLoadConfiguration> TzMinLeftConfig, TzMinRightConfig;
         minResults->GetResult(0,&TzMinLeft,  &TzMinLeftConfig,
                                 &TzMinRight, &TzMinRightConfig);

         *pTmin = TzMinRight;
         *pTmax = TzMaxRight;

         // Vehicle indexes
         if ( pMinVehIdx )
         {
            TzMinRightConfig->get_VehicleIndex(pMinVehIdx);
         }

         if ( pMaxVehIdx )
         {
            TzMaxRightConfig->get_VehicleIndex(pMaxVehIdx);
         }

         // Corresponding reactions (end shears)
         // get reaction that corresonds to T min
         CComPtr<ISectionResult3Ds> results;
         TzMinRightConfig->put_ForceEffect(fetFy);
         TzMinRightConfig->put_Optimization(optMaximize);
         response->ComputeForces( m_LBAMPoi, bstrStage, roMember, TzMinRightConfig, &results );

         CComPtr<ISectionResult3D> result;
         results->get_Item(0,&result);

         Float64 T;
         result->get_YRight(&T);
         *pRmin = -T;

         results.Release();
         result.Release();

         // get rotation that corresonds to R max
         TzMaxRightConfig->put_ForceEffect(fetRz);
         TzMaxRightConfig->put_Optimization(optMaximize);
         response->ComputeForces( m_LBAMPoi, bstrStage, roMember, TzMaxRightConfig, &results );

         results->get_Item(0,&result);
         result->get_YRight(&T);
         *pRmax = -T;
      }
   }
}

Float64 CGirderModelManager::GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   // Use lbam to get load case for this combination
   CGirderModelData* pModelData = GetGirderModel(GetGirderLineIndex(location.GirderKey),bat);

   CComPtr<ILBAMModel> lbam;
   GetLBAM(pModelData, bat, &lbam);

   CComPtr<ILoadCases> load_cases;
   lbam->get_LoadCases(&load_cases);

   CComBSTR combo_name = GetLoadCaseName(combo);

   CComPtr<ILoadCase> load_case;
   load_cases->Find(combo_name, &load_case);

   CollectionIndexType nLoadGroups;
   load_case->get_LoadGroupCount(&nLoadGroups);

   // Cycle through load cases and sum reactions
   Float64 R = 0;
   for (CollectionIndexType ldGroupIdx = 0; ldGroupIdx < nLoadGroups; ldGroupIdx++)
   {
      CComBSTR lg_name;
      load_case->GetLoadGroup(ldGroupIdx, &lg_name);

      pgsTypes::ProductForceType pfType = GetProductForceType(lg_name); 

      Float64 r = GetBearingProductReaction(intervalIdx, location, pfType, bat, resultsType);

      R += r;
   }

   return R;
}

void CGirderModelManager::GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                        pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                        Float64* pRmin,Float64* pRmax) const
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   ATLASSERT(pIntervals->GetLiveLoadInterval() <= intervalIdx);
#endif

   if ( location.Face == rftMid )
   {
      GET_IFACE(IReactions,pReactions);
      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,location.PierIdx,location.GirderKey,bat,bIncludeImpact,pRmin,pRmax);
   }
   else
   {
      SupportIDType backID, aheadID;
      GetPierSupportIDs(location, &backID, &aheadID);
      SupportIDType supportID = (location.Face == rftBack ? backID : aheadID);

      m_LBAMPoi->Clear();
      m_LBAMPoi->Add(supportID);

      GM_GetCombinedLiveLoadReaction(intervalIdx,llType,location.GirderKey,bat,bIncludeImpact,pRmin,pRmax);
   }
}

void CGirderModelManager::GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                  pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                  Float64* pRmin,Float64* pRmax) const
{
   if ( location.Face == rftMid )
   {
      GetReaction(intervalIdx,limitState,location.PierIdx,location.GirderKey,bat,bIncludeImpact,pRmin,pRmax);
   }
   else
   {
      // We have to emulate what the LBAM does for load combinations here
      *pRmin = 0.0;
      *pRmax = 0.0;

      // Use lbam to get load factors for this limit state
      CGirderModelData* pModelData = GetGirderModel(GetGirderLineIndex(location.GirderKey),bat);

      CComPtr<ILBAMModel> lbam;
      GetLBAM(pModelData, bat, &lbam);

      CComPtr<ILoadCombinations> load_combos;
      lbam->get_LoadCombinations(&load_combos);

      CComBSTR combo_name = GetLoadCombinationName(limitState);

      CComPtr<ILoadCombination> load_combo;
      load_combos->Find(combo_name, &load_combo);

      // First factor load cases
      CollectionIndexType lc_cnt;
      load_combo->get_LoadCaseFactorCount(&lc_cnt);
      for (CollectionIndexType lc_idx = 0; lc_idx < lc_cnt; lc_idx++)
      {
         CComBSTR lc_name;
         Float64 min_factor, max_factor;
         load_combo->GetLoadCaseFactor(lc_idx, &lc_name, &min_factor, &max_factor);

         LoadingCombinationType combo;
         if(GetLoadCaseTypeFromName(lc_name, &combo))
         {
            Float64 r = GetBearingCombinedReaction(intervalIdx, location, combo, bat, rtCumulative);
            *pRmin += min_factor * r;
            *pRmax += max_factor * r;
         }
      }

      // Next, factor and combine live load
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      if(liveLoadIntervalIdx <= intervalIdx)
      {
         Float64 LLIMmin(Float64_Max), LLIMmax(-Float64_Max);

         CollectionIndexType nlls;
         load_combo->GetLiveLoadModelCount(&nlls);

         for (CollectionIndexType ills=0; ills<nlls; ills++)
         {
            LiveLoadModelType llm_type;
            load_combo->GetLiveLoadModel(0, &llm_type);

            pgsTypes::LiveLoadType llType = GetLiveLoadTypeFromModelType(llm_type);

            // Only envelope pedestrian load if it exists
            if (llType == pgsTypes::lltPedestrian && !HasPedestrianLoad(location.GirderKey))
            {
               break;
            }

            Float64 Rmin, Rmax;
            GetBearingCombinedLiveLoadReaction(intervalIdx,location,llType,bat,bIncludeImpact,&Rmin,&Rmax);

            LLIMmin = Min(LLIMmin, Rmin);
            LLIMmax = Max(LLIMmax, Rmax);
         }

         Float64 ll_factor;
         load_combo->get_LiveLoadFactor(&ll_factor);

         *pRmin += ll_factor * LLIMmin;
         *pRmax += ll_factor * LLIMmax;
      }

      // Last, factor in load modifier
      CComPtr<ISpans> spans;
      lbam->get_Spans(&spans);
      SpanIndexType spanIdx = (location.Face == rftAhead ? location.PierIdx : location.PierIdx-1);
      CComPtr<ISpan> span;
      spans->get_Item(spanIdx,&span);

      Float64 lm_min, lm_max;
      span->GetLoadModifier(lctStrength, &lm_min, &lm_max);
      *pRmin *= lm_min;
      *pRmax *= lm_max;
   }
}

//////////////////////////////////////////////////
void CGirderModelManager::ChangeLiveLoadName(LPCTSTR strOldName,LPCTSTR strNewName)
{
   auto iter(m_GirderModels.begin());
   auto end(m_GirderModels.end());
   for ( ; iter != end; iter++ )
   {
      CGirderModelData* pModelData = &(iter->second);

      RenameLiveLoad(pModelData->m_Model,pgsTypes::lltDesign,strOldName,strNewName);
      RenameLiveLoad(pModelData->m_Model,pgsTypes::lltPermit,strOldName,strNewName);

      if ( pModelData->m_ContinuousModel )
      {
         RenameLiveLoad(pModelData->m_ContinuousModel,pgsTypes::lltDesign,strOldName,strNewName);
         RenameLiveLoad(pModelData->m_ContinuousModel,pgsTypes::lltPermit,strOldName,strNewName);
      }
   }
}

CGirderModelData* CGirderModelManager::GetGirderModel(GirderIndexType gdrLineIdx) const
{
   BuildModel(gdrLineIdx); // builds or updates the model if necessary
   auto found = m_GirderModels.find(gdrLineIdx);
   ATLASSERT( found != m_GirderModels.end() ); // should always find it!
   CGirderModelData* pModelData = &(*found).second;
   return pModelData;
}

CGirderModelData* CGirderModelManager::GetGirderModel(GirderIndexType gdrLineIdx,pgsTypes::BridgeAnalysisType bat) const
{
   BuildModel(gdrLineIdx,bat); // builds or updates the model if necessary
   auto found = m_GirderModels.find(gdrLineIdx);
   ATLASSERT( found != m_GirderModels.end() ); // should always find it!
   CGirderModelData* pModelData = &(*found).second;
   return pModelData;
}

void CGirderModelManager::BuildModel(GirderIndexType gdrLineIdx,pgsTypes::BridgeAnalysisType bat) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( !pBridgeDesc->IsStable() )
   {
      THROW_UNWIND(_T("Cannot perform analysis. Bridge configuration is geometrically unstable."),XREASON_UNSTABLE);
   }

   std::map<GirderIndexType,CGirderModelData>::iterator found = m_GirderModels.find(gdrLineIdx);
   if ( found == m_GirderModels.end() )
   {
      CGirderModelData model_data(this,gdrLineIdx);
      m_GirderModels.insert( std::make_pair(gdrLineIdx,model_data) );
      found = m_GirderModels.find(gdrLineIdx);
   }

   CGirderModelData* pModelData = &(found->second);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   // if the models are already build, leave now
   if ( bat == pgsTypes::SimpleSpan && pModelData->m_Model != nullptr )
   {
      return;
   }
   else if ( bat == pgsTypes::ContinuousSpan && pModelData->m_ContinuousModel != nullptr )
   {
      return;
   }
   else if ( (bat == pgsTypes::MaxSimpleContinuousEnvelope || bat == pgsTypes::MinSimpleContinuousEnvelope) && pModelData->m_Model != nullptr && pModelData->m_ContinuousModel != nullptr )
   {
      return;
   }

   // build the simple span model
   bool bBuildSimple = false;
   if ( (bat == pgsTypes::SimpleSpan || bat == pgsTypes::MaxSimpleContinuousEnvelope || bat == pgsTypes::MinSimpleContinuousEnvelope) && pModelData->m_Model == nullptr )
   {
      std::_tostringstream os;
      os << _T("Building Simple Span Bridge Site Analysis model for Girderline ") << LABEL_GIRDER(gdrLineIdx) << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      CComPtr<ILBAMModel> model;
      model.CoCreateInstance(CLSID_LBAMModel);
      CString strName;
      strName.Format(_T("SimpleSpan_Girder_%s"), LABEL_GIRDER(gdrLineIdx));
      model->put_Name(T2BSTR(strName));
      pModelData->AddSimpleModel(model);
      BuildLBAM(gdrLineIdx,false,pModelData->pContraflexureResponse[pgsTypes::SimpleSpan],pModelData->pDeflContraflexureResponse[pgsTypes::SimpleSpan],model);
      bBuildSimple = true;
   }

   // build the simple made continuous model
   bool bBuildContinuous = false;
   if ( (bat == pgsTypes::ContinuousSpan || bat == pgsTypes::MaxSimpleContinuousEnvelope || bat == pgsTypes::MinSimpleContinuousEnvelope) && pModelData->m_ContinuousModel == nullptr )
   {
      std::_tostringstream os;
      os << _T("Building Continuous Bridge Site Analysis model for Girderline ") << LABEL_GIRDER(gdrLineIdx) << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      CComQIPtr<ILBAMModel> continuous_model;
      continuous_model.CoCreateInstance(CLSID_LBAMModel);
      CString strName;
      strName.Format(_T("ContinuousSpan_Girder_%s"), LABEL_GIRDER(gdrLineIdx));
      continuous_model->put_Name(T2BSTR(strName));
      pModelData->AddContinuousModel(continuous_model);
      BuildLBAM(gdrLineIdx,true,pModelData->pContraflexureResponse[pgsTypes::ContinuousSpan],pModelData->pDeflContraflexureResponse[pgsTypes::ContinuousSpan],continuous_model);
      bBuildContinuous = true;
   }


   // create the points of interest in the analysis models
   if ( bBuildSimple && pModelData->m_ContinuousModel != nullptr )
   {
      // copy POIs from continuous model to simple model
      CComPtr<IPOIs> source_pois, destination_pois;
      pModelData->m_Model->get_POIs(&destination_pois);
      pModelData->m_ContinuousModel->get_POIs(&source_pois);
      CComPtr<IEnumPOI> enumPOI;
      source_pois->get__EnumElements(&enumPOI);
      CComPtr<IPOI> objPOI;
      while ( enumPOI->Next(1,&objPOI,nullptr) != S_FALSE )
      {
         destination_pois->Add(objPOI);
         objPOI.Release();
      }
   }
   else if ( bBuildContinuous && pModelData->m_Model != nullptr )
   {
      // copy POIs from simple model to continous model
      CComPtr<IPOIs> source_pois, destination_pois;
      pModelData->m_Model->get_POIs(&source_pois);
      pModelData->m_ContinuousModel->get_POIs(&destination_pois);
      CComPtr<IEnumPOI> enumPOI;
      source_pois->get__EnumElements(&enumPOI);
      CComPtr<IPOI> objPOI;
      while ( enumPOI->Next(1,&objPOI,nullptr) != S_FALSE )
      {
         destination_pois->Add(objPOI);
         objPOI.Release();
      }
   }
   else
   {
      GET_IFACE(IPointOfInterest,pPOI);
      PoiList vPoi;
      pPOI->GetPointsOfInterest(CSegmentKey(ALL_GROUPS, gdrLineIdx, ALL_SEGMENTS), &vPoi);
      for ( const pgsPointOfInterest& poi : vPoi)
      {
         PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
         if ( poi_id == INVALID_ID )
         {
            poi_id = AddPointOfInterest( pModelData, poi );
         }
      }
   }
}

void CGirderModelManager::BuildModel(GirderIndexType gdrLineIdx) const
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   BuildModel(gdrLineIdx,bat);
}

void CGirderModelManager::BuildLBAM(GirderIndexType gdrLineIdx,bool bContinuousModel,IContraflexureResponse* pContraflexureResponse,IContraflexureResponse* pDeflContraflexureResponse,ILBAMModel* pModel) const
{
   try
   {
      //pModel->put_ForceEquilibriumTolerance(WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Kip));
      //pModel->put_MomentEquilibriumTolerance(WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::KipFeet));

      // prepare load modifiers
      lrfdLoadModifier load_modifier;
      GET_IFACE(ILoadModifiers,pLoadModifiers);
      load_modifier.SetDuctilityFactor( (lrfdLoadModifier::Level)pLoadModifiers->GetDuctilityLevel() ,pLoadModifiers->GetDuctilityFactor());
      load_modifier.SetImportanceFactor((lrfdLoadModifier::Level)pLoadModifiers->GetImportanceLevel(),pLoadModifiers->GetImportanceFactor());
      load_modifier.SetRedundancyFactor((lrfdLoadModifier::Level)pLoadModifiers->GetRedundancyLevel(),pLoadModifiers->GetRedundancyFactor());

      CreateLBAMStages(gdrLineIdx,pModel);
      CreateLBAMSpans(gdrLineIdx,bContinuousModel,load_modifier,pModel);
      CreateLBAMSuperstructureMembers(gdrLineIdx,bContinuousModel,load_modifier,pModel);

      ApplyLiveLoadDistributionFactors(gdrLineIdx,bContinuousModel,pContraflexureResponse, pModel);

      // Apply Loads
      pgsTypes::AnalysisType analysisType = bContinuousModel ? pgsTypes::Continuous : pgsTypes::Simple;
      ApplySelfWeightLoad(                pModel, analysisType, gdrLineIdx );
      ApplyDiaphragmLoad(                 pModel, analysisType, gdrLineIdx );
      ApplyConstructionLoad(              pModel, analysisType, gdrLineIdx );
      ApplyShearKeyLoad(                  pModel, analysisType, gdrLineIdx);
      ApplyLongitudinalJointLoad(         pModel, analysisType, gdrLineIdx);
      ApplySlabLoad(                      pModel, analysisType, gdrLineIdx );
      ApplyOverlayLoad(                   pModel, analysisType, gdrLineIdx, bContinuousModel );
      ApplyTrafficBarrierAndSidewalkLoad( pModel, analysisType, gdrLineIdx, bContinuousModel );
      ApplyLiveLoadModel(                 pModel, gdrLineIdx );
      ApplyUserDefinedLoads(              pModel, gdrLineIdx );
      //ApplyEquivalentPretensionForce(     pModel, gdrLineIdx ); // causes recursion... do this later
      ApplyPostTensionDeformation(    pModel, gdrLineIdx );


      // Setup the product load groups and load combinations
      ConfigureLoadCombinations(pModel);
   }
   catch(...)
   {
      // usually indicates a live load distribution factor range of applicability error
      ATLASSERT(false);
      throw;
   }
}

void CGirderModelManager::CreateLBAMStages(GirderIndexType gdr,ILBAMModel* pModel) const
{
   // LBAM stages have a 1-to-1 mapping with analysis intervals
   // (interval index = stage index)
   CGirderKey girderKey(0,gdr);

   // for cases where there is a different number of girders in each group
   // find the first girder group that includes gdr
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      girderKey.groupIndex = grpIdx;
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      if ( gdr < nGirders )
      {
         break;
      }
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals          = pIntervals->GetIntervalCount();
   IntervalIndexType startIntervalIdx    = pIntervals->GetFirstSegmentErectionInterval(girderKey);

   CComPtr<IStages> stages;
   pModel->get_Stages(&stages);

   // LBAM stages begin when the first segment is erected
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      CComPtr<IStage> stage;
      stage.CoCreateInstance(CLSID_Stage);
      stage->put_Name( GetLBAMStageName(intervalIdx) );
      stage->put_Description( CComBSTR(pIntervals->GetDescription(intervalIdx).c_str()) );
      
      stages->Add(stage);
   }
}

void CGirderModelManager::CreateLBAMSpans(GirderIndexType gdr,bool bContinuousModel,const lrfdLoadModifier& loadModifier,ILBAMModel* pModel) const
{
   // This method creates the basic layout for the LBAM
   // It creates the support, span, and temporary support objects
   
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GET_IFACE(IBridge,pBridge);

   CComPtr<ISupports> supports;
   pModel->get_Supports(&supports);

   // If all columns are pinned top with pinned base fixity, and the abutments
   // both have roller supports, the model will be unstable. Change one abutment to hinge.
   //
   // _________________________________________________________
   // o            o                     o                    o
   //              |                     |
   //              |                     |
   //              |                     |
   //              |                     |
   //              o                     o
   //             ---                   ---
   bool bHasXConstraint = false; // make sure there is at least 1 reaction in the X-direction
                                 // otherwise the LBAM will be unstable

   //
   // create the first support (abutment 0)
   //
   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   CComPtr<ISupport> objSupport;
   CreateLBAMSupport(gdr,bContinuousModel,loadModifier,pPier,&objSupport);
   supports->Add(objSupport);
   BoundaryConditionType boundaryCondition;
   objSupport->get_BoundaryCondition(&boundaryCondition);
   if ( boundaryCondition == bcPinned || boundaryCondition == bcFixed )
   {
      bHasXConstraint = true;
   }

   // Layout the spans and supports along the girderline
   CComPtr<ISpans> spans;
   pModel->get_Spans(&spans);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   for (const auto& thisGirderKey : vGirderKeys)
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(thisGirderKey.groupIndex, &startSpanIdx, &endSpanIdx);
      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         CSpanKey spanKey(spanIdx, thisGirderKey.girderIndex);

         // pier indicies related to this span
         PierIndexType prevPierIdx = PierIndexType(spanIdx);
         PierIndexType nextPierIdx = prevPierIdx + 1;

         // create LBAM span object
         CComPtr<ISpan> objSpan;
         objSpan.CoCreateInstance(CLSID_Span);

         Float64 span_length = pBridge->GetFullSpanLength(spanKey); // span length between CL piers, measured along the centerline of the girder
         objSpan->put_Length(span_length);
         objSpan->SetLoadModifier(lctStrength, loadModifier.LoadModifier(lrfdTypes::StrengthI, lrfdTypes::Min), loadModifier.LoadModifier(lrfdTypes::StrengthI, lrfdTypes::Max));
         spans->Add(objSpan);

         // support at right end of the span (left side of pier next pier)
         pPier = pBridgeDesc->GetPier(nextPierIdx);

         objSupport.Release();
         CreateLBAMSupport(gdr, bContinuousModel, loadModifier, pPier, &objSupport);
         supports->Add(objSupport);
         BoundaryConditionType boundaryCondition;
         objSupport->get_BoundaryCondition(&boundaryCondition);
         if (boundaryCondition == bcPinned || boundaryCondition == bcFixed)
         {
            bHasXConstraint = true;
         }
      } // next span
   } // next girder in the girderline

   if ( !bHasXConstraint )
   {
      // there isn't any constraints in the X-direction (probably all rollers)
      // make the first support pinned
      objSupport.Release();
      supports->get_Item(0,&objSupport);
#if defined _DEBUG
      BoundaryConditionType bc;
      objSupport->get_BoundaryCondition(&bc);
      ATLASSERT(bc == bcRoller);
#endif
      objSupport->put_BoundaryCondition(bcPinned);
      bHasXConstraint = true;
   }

   //
   // Create Temporary Supports
   //

   // this models the real temporary supports in the physical bridge model

   // only used if there are temporary supports
   GET_IFACE_NOCHECK(IIntervals,pIntervals);

   IntervalIndexType startIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(CGirderKey(ALL_GROUPS,gdr)); // this is the interval that the LBAM starts with

   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      if ( pBridge->GetTemporarySupportType(tsIdx) == pgsTypes::StrongBack )
      {
         continue; // no temporary support at strongback. Segment weight bears directly on the adjacent segment via the strongback
      }

      SupportIDType tsID = pBridgeDesc->GetTemporarySupport(tsIdx)->GetID();

      SpanIndexType spanIdx;
      Float64 Xspan;
      pBridge->GetTemporarySupportLocation(tsIdx,gdr,&spanIdx,&Xspan);
      CComPtr<ISpans> spans;
      pModel->get_Spans(&spans);

      CComPtr<ISpan> objSpan;
      spans->get_Item(spanIdx,&objSpan);

      CComPtr<ITemporarySupports> objTemporarySupports;
      objSpan->get_TemporarySupports(&objTemporarySupports);

      if ( pBridge->GetSegmentConnectionTypeAtTemporarySupport(tsIdx) == pgsTypes::tsctContinuousSegment )
      {
         // for temporary supports with continuous segments, just put a single temporary support
         // object at the centerline of the TS

         //   ======================================
         //                     ^
         //
         CComPtr<ITemporarySupport> objTS;
         objTS.CoCreateInstance(CLSID_TemporarySupport);

         SupportIDType ID = GetTemporarySupportID(tsIdx);
         objTS->put_ID(ID);
         objTS->put_Location(Xspan);

         objTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         objTS->put_BoundaryCondition(bcRoller);

         IntervalIndexType erectionIntervalIdx = pIntervals->GetTemporarySupportErectionInterval(tsIdx);
         IntervalIndexType removalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);

         ATLASSERT(erectionIntervalIdx < removalIntervalIdx);

         // temporary supports are often erected in an interval that occurs before the LBAM models
         // e.g. if erected in intervalIdx = 0, and the LBAM starts at startIntervalIdx, then the TS is erected before the LBAM
         // Make sure the erection interval isn't before the start of the LBAM
         erectionIntervalIdx = Max(erectionIntervalIdx, startIntervalIdx);

         objTS->put_StageErected( GetLBAMStageName(erectionIntervalIdx) );
         objTS->put_StageRemoved( GetLBAMStageName(removalIntervalIdx) );

         objTemporarySupports->Add(objTS);
      }
      else
      {
         ATLASSERT(pBridge->GetSegmentConnectionTypeAtTemporarySupport(tsIdx) == pgsTypes::tsctClosureJoint);

         // There is a discontinuity at this temporary support prior to the closure joint being cast.
         // Model with two support objects to maintain the stability of the LBAM model.
         // The closure joint superstructure member will be modeled with hinges so it doesn't
         // attract load before it is actually installed in the physical model.

         // 
         //  =============================o===========o===============================
         //                                ^         ^ Temp Supports
         //

         CSegmentKey leftSegmentKey, rightSegmentKey;
         pBridge->GetSegmentsAtTemporarySupport(gdr,tsIdx,&leftSegmentKey,&rightSegmentKey);

         Float64 left_closure_size, right_closure_size;
         pBridge->GetClosureJointSize(leftSegmentKey,&left_closure_size,&right_closure_size);

         Float64 left_end_dist    = pBridge->GetSegmentEndEndDistance(leftSegmentKey);
         Float64 right_start_dist = pBridge->GetSegmentStartEndDistance(rightSegmentKey);

         // Left temporary support
         CComPtr<ITemporarySupport> objLeftTS;
         objLeftTS.CoCreateInstance(CLSID_TemporarySupport);

         SupportIDType ID = -((SupportIDType)(tsIdx+1)*100);
         objLeftTS->put_ID(ID);
         objLeftTS->put_Location(Xspan - left_closure_size - left_end_dist);

         objLeftTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         objLeftTS->put_BoundaryCondition(bcRoller);

         IntervalIndexType erectIntervalIdx   = pIntervals->GetTemporarySupportErectionInterval(tsIdx);
         IntervalIndexType removalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);

         //ATLASSERT(erectIntervalIdx == 0); // LBAM doesn't support the stage when a TS is erected
         //objLeftTS->put_StageErected( GetLBAMStageName(erectIntervalIdx) );
         objLeftTS->put_StageRemoved( GetLBAMStageName(removalIntervalIdx) );

         objTemporarySupports->Add(objLeftTS);

         // Right temporary support
         CComPtr<ITemporarySupport> objRightTS;
         objRightTS.CoCreateInstance(CLSID_TemporarySupport);

         ID = -( (SupportIDType)((tsIdx+1)*100+1) );
         objRightTS->put_ID(ID);
         objRightTS->put_Location(Xspan + right_closure_size + right_start_dist);

         objRightTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         objRightTS->put_BoundaryCondition(bcRoller);

         //ATLASSERT(erectIntervalIdx == 0); // LBAM doesn't support the stage when a TS is erected
         //objRightTS->put_StageErected( GetLBAMStageName(erectIntervalIdx) );
         objRightTS->put_StageRemoved( GetLBAMStageName(removalIntervalIdx) );

         objTemporarySupports->Add(objRightTS);
      }
   }
}

void CGirderModelManager::CreateLBAMSupport(GirderIndexType gdrLineIdx,bool bContinuousModel,const lrfdLoadModifier& loadModifier,const CPierData2* pPier,ISupport** ppSupport) const
{
   CComPtr<ISupport> objSupport;
   objSupport.CoCreateInstance(CLSID_Support);

   if ( pPier->GetPierModelType() == pgsTypes::pmtIdealized )
   {
      BoundaryConditionType boundaryCondition = GetLBAMBoundaryConditions(bContinuousModel,pPier);
      objSupport->put_BoundaryCondition(boundaryCondition);
   }
   else
   {
      BOOL bReleaseTop;
      if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::BoundaryConditionType boundaryCondition = pPier->GetBoundaryConditionType();
         switch(boundaryCondition)
         {
         case pgsTypes::bctContinuousAfterDeck:
         case pgsTypes::bctContinuousBeforeDeck:
            bReleaseTop = VARIANT_TRUE;
            break;

         case pgsTypes::bctIntegralAfterDeck:
         case pgsTypes::bctIntegralBeforeDeck:
         case pgsTypes::bctIntegralAfterDeckHingeBack:
         case pgsTypes::bctIntegralBeforeDeckHingeBack:
         case pgsTypes::bctIntegralAfterDeckHingeAhead:
         case pgsTypes::bctIntegralBeforeDeckHingeAhead:
            bReleaseTop = VARIANT_FALSE;
            break;

         default:
            bReleaseTop = VARIANT_TRUE;
         }
      }
      else
      {
         ATLASSERT(pPier->IsInteriorPier());
         pgsTypes::PierSegmentConnectionType connectionType = pPier->GetSegmentConnectionType();
         bReleaseTop = (connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctContinuousSegment) ? VARIANT_TRUE : VARIANT_FALSE;
      }
      objSupport->put_TopRelease(bReleaseTop);

      pgsTypes::ColumnLongitudinalBaseFixityType fixityType = pPier->GetColumnFixity();
      objSupport->put_BoundaryCondition(fixityType == pgsTypes::cftPinned ? bcPinned : bcFixed);

      GET_IFACE(IBridge,pBridge);
      PierIndexType pierIdx = pPier->GetIndex();
      GroupIndexType backGroupIdx, aheadGroupIdx;
      pBridge->GetGirderGroupIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);

      //
      // Compute a per girder axial and bending stiffness for the pier
      //

      // "per girder" is based on the average number of girders framing
      // into the pier
      Float64 nAvgGirders;
      IndexType nGirders;
      CGirderKey girderKey;
      if ( backGroupIdx == INVALID_INDEX )
      {
         nGirders = pBridge->GetGirderCount(aheadGroupIdx);
         nAvgGirders = (Float64)nGirders;

         if ( gdrLineIdx <= nGirders )
         {
            girderKey.groupIndex = aheadGroupIdx;
            girderKey.girderIndex = gdrLineIdx;
         }
         else
         {
            girderKey = CGirderKey(aheadGroupIdx,nGirders-1);
         }
      }
      else if ( aheadGroupIdx == INVALID_INDEX )
      {
         nGirders = pBridge->GetGirderCount(backGroupIdx);
         nAvgGirders = (Float64)nGirders;

         if ( gdrLineIdx <= nGirders )
         {
            girderKey.groupIndex = backGroupIdx;
            girderKey.girderIndex = gdrLineIdx;
         }
         else
         {
            girderKey = CGirderKey(backGroupIdx,nGirders-1);
         }
      }
      else
      {
         GirderIndexType nGirdersBack = pBridge->GetGirderCount(backGroupIdx);
         GirderIndexType nGirdersAhead = pBridge->GetGirderCount(aheadGroupIdx);
         nAvgGirders = (nGirdersBack + nGirdersAhead)/2.0;
         nGirders = Min(nGirdersBack,nGirdersAhead);

         if ( gdrLineIdx <= nGirders )
         {
            girderKey.groupIndex = backGroupIdx;
            girderKey.girderIndex = gdrLineIdx;
         }
         else
         {
            girderKey = CGirderKey(MinIndex(nGirdersBack,nGirdersAhead) == 0 ? backGroupIdx : aheadGroupIdx,Min(nGirdersBack,nGirdersAhead)-1);
         }
      }

      // Total EA and EI is the sum of EA and EI for each column
      // Also, use the average column height as the height of the pier
      ColumnIndexType nColumns = pBridge->GetColumnCount(pierIdx);

      Float64 sumH = 0;
      Float64 sumA = 0;
      Float64 sumI = 0;
      for ( ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++ )
      {
         Float64 H, A, I;
         pBridge->GetColumnProperties(pierIdx,colIdx,true,&H,&A,&I);
         sumH += H;
         sumA += A;
         sumI += I;
      }

      Float64 H = sumH/nColumns; // average height
      Float64 A = sumA/nAvgGirders; // area of pier columns per girder
      Float64 I = sumI/nAvgGirders; // moment of inertia of pier columns per girder

      Float64 Hdia1,Hdia2,Wdia;
      pBridge->GetPierDiaphragmSize(pierIdx,pgsTypes::Back,&Wdia,&Hdia1);
      pBridge->GetPierDiaphragmSize(pierIdx,pgsTypes::Ahead,&Wdia,&Hdia2);
      Float64 superstructure_depth = Max(Hdia1,Hdia2);
      H += superstructure_depth/2; // account for the depth of the cross beam... assume CG to be at mid-depth
      objSupport->put_Length(H);

      GET_IFACE(IMaterials,pMaterials);
      Float64 E = pMaterials->GetPierEc28(pierIdx);

      Float64 EA = E*A;
      Float64 EI = E*I;

      CComPtr<ISegmentCrossSection> objCrossSection;
      objCrossSection.CoCreateInstance(CLSID_SegmentCrossSection);
      objCrossSection->SetStiffness(EA,EI,EA,EI);

      CComPtr<ISegment> objSegment;
      objSegment.CoCreateInstance(CLSID_Segment);
      objSegment->put_Length(H);
      objSegment->putref_SegmentCrossSection(objCrossSection);

      // it seems like we should be using the interval when the pier is erected... however the LBAM model
      // starts when the first segment is erected... any stage before the first segment erection stage 
      // is invalid in the LBAM.
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectFirstSegmentIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
#if defined _DEBUG
      IntervalIndexType erectPierIntervalIdx = pIntervals->GetErectPierInterval(pierIdx);
      ATLASSERT(erectPierIntervalIdx <= erectFirstSegmentIntervalIdx);
#endif
      IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
      for ( IntervalIndexType intervalIdx = erectFirstSegmentIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
      {
         objSupport->AddSegment(GetLBAMStageName(intervalIdx),objSegment);
      }
   }

   objSupport->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

   objSupport.CopyTo(ppSupport);
}

void CGirderModelManager::CreateLBAMSuperstructureMembers(GirderIndexType gdr,bool bContinuousModel,lrfdLoadModifier& loadModifier,ILBAMModel* pModel) const
{
   // This method creates the superstructure members for the LBAM
   // Superstructure member IDs are equal to their index, going left to right along the structure

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IPointOfInterest,pPOI);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(ILossParameters, pLossParams);
   bool bTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   // Use one LBAM superstructure member for each segment in a girder
   // Use two LBAM superstructure members at intermediate piers between girder groups
   // (one from edge of prev group to CL Pier, one from CL Pier to start of next group)
   CComPtr<ISuperstructureMembers> ssms;
   pModel->get_SuperstructureMembers(&ssms);

   // Model the small overhang at the first pier since we are modeling segments with actual lengths
   // (not cl-brg to cl-brg lengths)
   //
   //  |<----------- Bridge Length --------->|
   //  |                                     |
   //  =======================================
   //    ^               ^                  ^
   //  |-|<---- superstructure member offset = end distance (or cantilever length if applicable)

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   Float64 end_distance = pBridge->GetSegmentStartEndDistance(CSegmentKey(vGirderKeys.front(),0));
   ssms->put_Offset(end_distance);

   //
   // layout the girder segments... each segment is a LBAM superstructure member
   //
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType startIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(CGirderKey(ALL_GROUPS, gdr));
   for(const auto& girderKey : vGirderKeys)
   {
      CheckGirderEndGeometry(pBridge,girderKey); // make sure girder is on its bearing - if it isn't an UNWIND exception will throw

      // get the girder data
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);

      GirderIndexType nGirders = pGroup->GetGirderCount();

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

#pragma Reminder("UPDATE: Assuming prismatic members")
         // NOTE: segments can be non-prismatic. Refine the model by breaking the segment into step-wise prismatic
         // pieces and create LBAM superstructure member segments for each piece. NOTE: If we do this, then the
         // Segment models (before erection) must do the same and the deflection adjustments where we
         // scale values by (IeEc/IrEci) must get the section properties at the location under consideration
         // and not the mid-point properties.

         // get POI at mid-point of the segment (mid-segment section properties will be used for EA and EI)
         PoiList vPOI;
         pPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPOI);
         ATLASSERT( vPOI.size() == 1 );
         const pgsPointOfInterest& segmentPoi = vPOI.front();

         IntervalIndexType compositeClosureIntervalIdx = INVALID_INDEX;
         pgsPointOfInterest closurePoi;
         if ( segIdx < nSegments-1 )
         {
            vPOI.clear();
            pPOI->GetPointsOfInterest(segmentKey,POI_CLOSURE,&vPOI);
            ATLASSERT(vPOI.size() == 1);
            closurePoi = vPOI.front();
            compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);
         }

         GET_IFACE(ILibrary, pLib);
         GET_IFACE(ISpecification, pSpec);
         const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
         bool bUse90DayStrengthFactor;
         Float64 concrete_strength_factor;
         pSpecEntry->Use90DayStrengthForSlowCuringConcrete(&bUse90DayStrengthFactor, &concrete_strength_factor);


         // The vector of superstructure member data contains stiffness properties by stage.
         // This should be segment data between section changes, for each stage.
         // For now, assume segments are prismatic

         std::vector<SuperstructureMemberData> vSegmentData;
         std::vector<SuperstructureMemberData> vClosureData;
         SuperstructureMemberData data;

         for (IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++)
         {
            // Segment Data

            // young's modulus in this interval
            Float64 Ec;
            if (bUse90DayStrengthFactor && !IsEqual(concrete_strength_factor,1.0))
            {
               // the 115% increase in concrete strength (LRFD 5.12.3.2.5) only applies to stresses
               // if the increase is enabled, use the 28 day modulus for all time after 90 days
               Float64 age = pMaterial->GetSegmentConcreteAge(segmentKey, intervalIdx, pgsTypes::Start);
               if (age < 90)
               {
                  Ec = pMaterial->GetSegmentEc(segmentKey, intervalIdx);
               }
               else
               {
                  Ec = pMaterial->GetSegmentEc28(segmentKey);
               }
            }
            else
            {
               Ec = pMaterial->GetSegmentEc(segmentKey, intervalIdx);
            }

            // Get section properties this interval.
            // GetAg/GetIx will return gross or transformed properties based on the
            // current section properties mode
            Float64 Ag, Ixx, Iyy, Ixy;
            if (intervalIdx == startIntervalIdx && !bTimeStepAnalysis)
            {
               // we want to use release properties at erection because
               // girder deflection and self-weight stress are based
               // on the properties at release.
               //
               // Also see GetIntervalFromLBAMStageName() which is used for
               // setting up the stress points
               Ag = pSectProp->GetAg(releaseIntervalIdx, segmentPoi);
               Ixx = pSectProp->GetIxx(releaseIntervalIdx, segmentPoi);
               Iyy = pSectProp->GetIyy(releaseIntervalIdx, segmentPoi);
               Ixy = pSectProp->GetIxy(releaseIntervalIdx, segmentPoi);
            }
            else
            {
               Ag = pSectProp->GetAg(intervalIdx, segmentPoi);
               Ixx = pSectProp->GetIxx(intervalIdx, segmentPoi);
               Iyy = pSectProp->GetIyy(intervalIdx, segmentPoi);
               Ixy = pSectProp->GetIxy(intervalIdx, segmentPoi);
            }

            data.stage = GetLBAMStageName(intervalIdx);
            data.ea = Ec*Ag;
            if (intervalIdx < compositeIntervalIdx)
            {
               // assume biaxial bending before the girder is composite
               // with other girders (composite happens when the deck becomes
               // composite with the girders)
               //
               // if this is a uniaxial bending case, Ixy will be zero
               // and we will get Ec(IxxIyy - 0)/Iyy = EcIxx
               data.ei = Ec*(Ixx*Iyy - Ixy*Ixy) / Iyy;
            }
            else
            {
               // after composite assume entire bridge cross section
               // behaves as a unaxial bending member
               data.ei = Ec*Ixx;
            }

            // in complex timelines some segments may not be constructed before others are erected
            // for the segments that aren't constructed yet, EA and EI are zero. This causes a problem
            // with the finite element models. Use dummy values since these elements don't do anything 
            // in the fem models
            data.ea = Max(data.ea, 1.0);
            data.ei = Max(data.ei, 1.0);

            if ( intervalIdx < liveLoadIntervalIdx )
            {
               data.ea_defl = data.ea;
               data.ei_defl = data.ei;
            }
            else
            {
               Float64 Xb = pPOI->ConvertPoiToBridgeLineCoordinate(segmentPoi);
               Float64 ei_defl = pSectProp->GetBridgeEIxx( Xb );
               ei_defl /= nGirders;

               data.ea_defl = ei_defl;
               data.ei_defl = ei_defl;
            }

            vSegmentData.push_back(data);

            // Closure Joint Data
            if ( segIdx < nSegments-1 )
            {
               // if the closure is integral, use its properties
               // otherwise use the segment properties to keep the LBAM/FEM model happy
               // (FEM crashes if EI/EA are zero)
               if ( compositeClosureIntervalIdx <= intervalIdx )
               {
                  Ec = pMaterial->GetClosureJointEc(segmentKey,intervalIdx);
                  Ag = pSectProp->GetAg(intervalIdx,closurePoi);
                  Ixx = pSectProp->GetIxx(intervalIdx, closurePoi);
                  Iyy = pSectProp->GetIyy(intervalIdx, closurePoi);
                  Ixy = pSectProp->GetIxy(intervalIdx, closurePoi);

                  data.ea = Ec*Ag;
                  data.ei = Ec*(Ixx*Iyy - Ixy*Ixy) / Iyy;

                  if ( intervalIdx < liveLoadIntervalIdx )
                  {
                     data.ea_defl = data.ea;
                     data.ei_defl = data.ei;
                  }
                  else
                  {
                     Float64 Xb = pPOI->ConvertPoiToBridgeLineCoordinate(closurePoi);
                     Float64 ei_defl = pSectProp->GetBridgeEIxx( Xb );
                     ei_defl /= nGirders;

                     data.ea_defl = ei_defl;
                     data.ei_defl = ei_defl;
                  }
               }

               vClosureData.push_back(data);
            }
         }

         bool bModelLeftCantilever, bModelRightCantilever;
         pBridge->ModelCantilevers(segmentKey,&bModelLeftCantilever,&bModelRightCantilever);

         // End to end segment length
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

         // CL Brg to end of girder
         Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 end_end_dist   = pBridge->GetSegmentEndEndDistance(segmentKey);

         // create the superstructure member
         Float64 L1 = start_end_dist;
         Float64 L2 = segment_length - start_end_dist - end_end_dist;
         Float64 L3 = end_end_dist;

         ATLASSERT( !IsZero(L2) );

         const CPierData2* pStartPier;
         const CTemporarySupportData* pStartTS;
         pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

         const CPierData2* pEndPier;
         const CTemporarySupportData* pEndTS;
         pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);

         if ( !IsZero(L1) )
         {
            CComPtr<ISuperstructureMember> ssmbr;
            CreateLBAMSuperstructureMember(L1,vSegmentData,&ssmbr);
            ssms->Add(ssmbr);
            GetLBAMBoundaryConditions(bContinuousModel,pTimelineMgr,girderKey.groupIndex,pSegment,pgsTypes::metStart,ssmbr);

            // Don't load the cantilever if:
            // 1) This is a simple span model and the left cantilever is not modeled
            // 2) This is a non-continuous connection and the left cantilever is not modeled
            if ( (bContinuousModel == false || (pStartPier && !pStartPier->IsContinuousConnection())) && bModelLeftCantilever == false )
            {
               ssmbr->put_LinkMember(VARIANT_TRUE);
            }
         }

         CComPtr<ISuperstructureMember> ssmbr;
         CreateLBAMSuperstructureMember(L2,vSegmentData,&ssmbr);
         ssms->Add(ssmbr);
         if ( IsZero(L1) || (bContinuousModel && (pStartPier && pStartPier->IsAbutment() && pStartPier->IsContinuousConnection())) )
         {
            GetLBAMBoundaryConditions(bContinuousModel,pTimelineMgr, girderKey.groupIndex,pSegment,pgsTypes::metStart,ssmbr);
         }

         if ( IsZero(L3) || (bContinuousModel && (pEndPier && pEndPier->IsAbutment() && pEndPier->IsContinuousConnection())) )
         {
            GetLBAMBoundaryConditions(bContinuousModel,pTimelineMgr, girderKey.groupIndex,pSegment,pgsTypes::metEnd,  ssmbr);
         }

         if ( !IsZero(L3) )
         {
            CComPtr<ISuperstructureMember> ssmbr;
            CreateLBAMSuperstructureMember(L3,vSegmentData,&ssmbr);
            ssms->Add(ssmbr);

            GetLBAMBoundaryConditions(bContinuousModel,pTimelineMgr, girderKey.groupIndex,pSegment,pgsTypes::metEnd,  ssmbr);
            
            // Don't load the cantilever if:
            // 1) This is a simple span model and the right cantilever is not modeled
            // 2) This is a non-continuous connection and the right cantilever is not modeled
            if ( (bContinuousModel == false || (pEndPier && !pEndPier->IsContinuousConnection())) && bModelRightCantilever == false )
            {
               ssmbr->put_LinkMember(VARIANT_TRUE);
            }
         }

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);

         //
         // Model Closure Joint
         //

         // NOTE: This doesn't work for match casting... though we aren't going to support
         // match casting as it isn't a good practice for spliced girder bridges
         if ( segIdx != nSegments-1 && pTS )
         {
            CClosureKey closureKey(segmentKey);

            // end to end length of the closure (distance between ends of the adjacent segments)
            Float64 closure_length = pBridge->GetClosureJointLength(closureKey);
            ATLASSERT( !IsZero(closure_length) );

            const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(closureKey);

            EventIndexType closureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure);
            IntervalIndexType closureIntervalIdx = pIntervals->GetInterval(closureEventIdx) + 1; // assume closure is at strength 1 interval after casting
            CComBSTR bstrContinuityStage( GetLBAMStageName(closureIntervalIdx) );

            CComPtr<ISuperstructureMember> closure_ssm;
            CreateLBAMSuperstructureMember(closure_length,vClosureData,&closure_ssm);
            ssms->Add(closure_ssm);
         }


         //
         // Create LBAM superstructure members at intermediate piers
         //

         // the width of the intermediate piers need to be modeled so the correct overall
         // span length is used when the structure becomes continuous
         if ( ( segIdx == nSegments-1 && pGroup->GetNextGirderGroup() != nullptr ) // last segment in all but the last group
               || // -OR-
              ( pPier && pPier->IsInteriorPier() ) // this is an interior pier
            )
         {
            //                                     CL Pier
            //                           | CL Brg  |         | CL Brg
            //  =============================      |     ===============================
            //                           |<------->|<------->|  bearing offset
            //                           |<->|           |<->|  end distance
            //                               |<--------->| intermediate pier member length
            //
            // Modeled like this... left and right temporary supports are removed after
            // segments are connected (i.e. LBAM member end release removed).
            //
            //
            //                           CL Brg              CL Brg
            //                           |         CL Pier   |
            //                           |         |         |
            //     SSMbr i               |i+1 i+2  | i+3  i+4|      i+5
            //  =============================o----o------o===================================
            //                           ^         ^         ^
            //                           |         |         |
            //                           |         +------------- Permanent pier
            //                           |                   |
            //                           +-------------------+--- Dummy Temporary Supports
            //
            // o indicates member end release.

            // This "game" has to be played to keep the LBAM and underlying FEM model stable. The temporary support
            // objects would not be needed if a Support could change boundary conditions at a stage. That is, the Support
            // for the real pier would be completely fixed until continuity is made, and then it would be a pin/roller.

            CSegmentKey nextSegmentKey;
            if ( pPier->IsInteriorPier() )
            {
               nextSegmentKey = segmentKey;
               nextSegmentKey.segmentIndex++;
            }
            else
            {
               nextSegmentKey.groupIndex = segmentKey.groupIndex + 1;
               nextSegmentKey.girderIndex = segmentKey.girderIndex;
               GirderIndexType nGirders = pBridge->GetGirderCount(nextSegmentKey.groupIndex);
               nextSegmentKey.girderIndex = Min(nextSegmentKey.girderIndex,nGirders-1);
               nextSegmentKey.segmentIndex = 0;
            }

            Float64 left_brg_offset  = pBridge->GetSegmentEndBearingOffset(segmentKey);
            Float64 right_brg_offset = pBridge->GetSegmentStartBearingOffset( nextSegmentKey );

            Float64 left_end_dist  = pBridge->GetSegmentEndEndDistance(segmentKey);
            Float64 right_end_dist = pBridge->GetSegmentStartEndDistance(nextSegmentKey);

            Float64 left_end_offset  = left_brg_offset  - left_end_dist;
            Float64 right_end_offset = right_brg_offset - right_end_dist;

            SpanIndexType spanIdx = pSegment->GetSpanIndex(pgsTypes::metEnd);
            PierIndexType pierIdx = PierIndexType(spanIdx+1);

            IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
            pIntervals->GetContinuityInterval(pierIdx,&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

            // use the girder data for the pier superstructure members
            bool bContinuousConnection = bContinuousModel ? pPier->IsContinuousConnection() : false;
            
            BoundaryConditionType startSpanBoundaryCondition = GetLBAMBoundaryConditions(bContinuousModel,pPier);
            BoundaryConditionType endSpanBoundaryCondition   = GetLBAMBoundaryConditions(bContinuousModel,pPier->GetNextSpan()->GetNextPier());

            // create superstructure member for pier diaphragm, left of CL Pier
            if ( !IsZero(left_end_offset) )
            {
               // left end of the segment is not at the CL Pier
               // build a superstructure member segment from the CLPier to the left end of the segment
               CComPtr<ISuperstructureMember> left_pier_diaphragm_ssm;
               CreateLBAMSuperstructureMember(left_end_offset,vSegmentData,&left_pier_diaphragm_ssm);
               if ( bContinuousModel && bContinuousConnection )
               {
                  left_pier_diaphragm_ssm->SetEndReleaseRemovalStage(ssRight,GetLBAMStageName(leftContinuityIntervalIdx));
                  left_pier_diaphragm_ssm->SetEndRelease(ssRight,mrtMz);

                  // release the axial force if the span is fully restrained axially
                  if ((startSpanBoundaryCondition == bcFixed || startSpanBoundaryCondition == bcPinned) &&
                     (endSpanBoundaryCondition == bcFixed || endSpanBoundaryCondition == bcPinned) && girderKey.groupIndex == 0 && pPier->GetPrevSpan()->GetPrevPier()->GetBoundaryConditionType() != pgsTypes::bctRoller )
                  {
                     // NOTE: it looks like we are setting the relese removal stage twice... above and here
                     // in some cases that is true and there isn't any harm in doing it twice. In other cases,
                     // the release remove stage is not set above so it needs to be set here
                     left_pier_diaphragm_ssm->SetEndReleaseRemovalStage(ssRight, GetLBAMStageName(leftContinuityIntervalIdx));
                     left_pier_diaphragm_ssm->SetEndRelease(ssRight, mrtFx);
                  }
               }
               else
               {
                  left_pier_diaphragm_ssm->put_LinkMember(VARIANT_TRUE);
                  left_pier_diaphragm_ssm->SetEndRelease(ssRight,mrtMz);
               }

               ssms->Add(left_pier_diaphragm_ssm);
            }

            // create superstructure member for pier diaphragm, right of CL Pier
            if ( !IsZero(right_end_offset) )
            {
               // right end of the segment is not at the CL pier
               // build a superstructure member segment from the CL pier to the right end of the segment
               CComPtr<ISuperstructureMember> right_pier_diaphragm_ssm;
               CreateLBAMSuperstructureMember(right_end_offset,vSegmentData,&right_pier_diaphragm_ssm);
               if ( bContinuousModel && bContinuousConnection )
               {
                  if ( startSpanBoundaryCondition == bcFixed )
                  {
                     // we are at a fixed pier, so release the moment so it doesn't go into the support
                     // before continuity is acheived
                     right_pier_diaphragm_ssm->SetEndReleaseRemovalStage(ssLeft,GetLBAMStageName(rightContinuityIntervalIdx));
                     right_pier_diaphragm_ssm->SetEndRelease(ssLeft,mrtMz);
                  }

                  // release the axial force if the span is fully restrained axially
                  if ( (startSpanBoundaryCondition == bcFixed || startSpanBoundaryCondition == bcPinned) &&
                       (endSpanBoundaryCondition   == bcFixed || endSpanBoundaryCondition   == bcPinned) )
                  {
                     // NOTE: it looks like we are setting the relese removal stage twice... above and here
                     // in some cases that is true and there isn't any harm in doing it twice. In other cases,
                     // the release remove stage is not set above so it needs to be set here
                     right_pier_diaphragm_ssm->SetEndReleaseRemovalStage(ssLeft,GetLBAMStageName(rightContinuityIntervalIdx));
                     right_pier_diaphragm_ssm->SetEndRelease(ssLeft,mrtFx);
                  }
               }
               else
               {
                  right_pier_diaphragm_ssm->put_LinkMember(VARIANT_TRUE);
                  if ( startSpanBoundaryCondition == bcFixed )
                  {
                     right_pier_diaphragm_ssm->SetEndRelease(ssLeft,mrtMz);
                  }
                  else
                  {
                     // if the pier model has in integral column, we have to add a member release
                     // against the column to keep the simple span behavior before continuity is
                     // established
                     pgsTypes::BoundaryConditionType bcType = pPier->GetBoundaryConditionType();
                     if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical &&
                         (bcType == pgsTypes::bctIntegralBeforeDeck || 
                          bcType == pgsTypes::bctIntegralAfterDeck ||
                          bcType == pgsTypes::bctIntegralAfterDeckHingeBack ||
                          bcType == pgsTypes::bctIntegralBeforeDeckHingeBack )
                        )
                     {
                        right_pier_diaphragm_ssm->SetEndRelease(ssLeft,mrtMz);
                     }
                  }
               }
               ssms->Add(right_pier_diaphragm_ssm);
            }

            // Create dummy temporary supports at the CL Bearing
            CComPtr<ISpans> spans;
            pModel->get_Spans(&spans);

            CComPtr<ISupports> supports;
            pModel->get_Supports(&supports);

            CComPtr<ISupport> objPrimarySupport;
            supports->get_Item(pierIdx,&objPrimarySupport);

            SupportIDType backID, aheadID;
            GetPierTemporarySupportIDs(pierIdx,&backID,&aheadID);

            // Temporary support at CL bearing on left side of pier
            // if the left end disance or offset is zero, the end of the beam is at the CL Pier
            // so we don't need the temporary support
            if ( !IsZero(left_end_dist+left_end_offset) && !IsZero(left_end_offset) )
            {
               // CLBearing is not at the CL Pier... create a dummy temporary support
               CComPtr<ISpan> objSpan;
               spans->get_Item(spanIdx,&objSpan);

               Float64 span_length;
               objSpan->get_Length(&span_length);

               Float64 location = span_length - left_brg_offset;

               CComPtr<ITemporarySupports> objTemporarySupports;
               objSpan->get_TemporarySupports(&objTemporarySupports);

               bool bExistingTS = false;
               CComPtr<IEnumTemporarySupport> enumTS;
               objTemporarySupports->get__EnumElements(&enumTS);
               CComPtr<ITemporarySupport> ts;
               while (enumTS->Next(1,&ts,nullptr) != S_FALSE)
               {
                  Float64 tsLocation;
                  ts->get_Location(&tsLocation);
                  if (IsEqual(location, tsLocation))
                  {
                     // there is already a temporary support at this location (this can happen if the user builds the model this way)
                     bExistingTS = true;
                     break;
                  }
                  ts.Release();
               }

               if (!bExistingTS)
               {
                  CComPtr<ITemporarySupport> objLeftTS;
                  objLeftTS.CoCreateInstance(CLSID_TemporarySupport);
                  objLeftTS->put_OmitReaction(VARIANT_TRUE); // don't apply the temporary support reaction back into the structure

                  objLeftTS->put_ID(backID);
                  objLeftTS->put_Location(location);

                  objLeftTS->SetLoadModifier(lctStrength, loadModifier.LoadModifier(lrfdTypes::StrengthI, lrfdTypes::Min), loadModifier.LoadModifier(lrfdTypes::StrengthI, lrfdTypes::Max));

                  objLeftTS->put_BoundaryCondition(bcRoller);

                  // If there is a continuous connection at the pier, remove the temporary support when the 
                  // connection becomes continuous, otherwise keep it to preserve the stability of the LBAM
                  objLeftTS->put_StageErected(CComBSTR(""));
                  if (bContinuousConnection)
                  {
                     objLeftTS->put_StageRemoved(GetLBAMStageName(leftContinuityIntervalIdx));
                  }
                  else
                  {
                     objLeftTS->put_StageRemoved(CComBSTR(""));
                  }
                  objPrimarySupport->AddAssociatedSupport(backID);

                  objTemporarySupports->Add(objLeftTS);
               }
            }

            // Temporary support CL Bearing on right side of pier
            // if the right end disance or offset is zero, the end of the beam is at the CL Pier
            // so we don't need the temporary support
            if (!IsZero(right_end_dist+right_end_offset) && !IsZero(right_end_offset))
            {
               CComPtr<ISpan> objSpan;
               spans->get_Item(spanIdx + 1, &objSpan);

               CComPtr<ITemporarySupports> objTemporarySupports;
               objSpan->get_TemporarySupports(&objTemporarySupports);

               bool bExistingTS = false;
               Float64 location = right_brg_offset;


               CComPtr<IEnumTemporarySupport> enumTS;
               objTemporarySupports->get__EnumElements(&enumTS);
               CComPtr<ITemporarySupport> ts;
               while (enumTS->Next(1, &ts, nullptr) != S_FALSE)
               {
                  Float64 tsLocation;
                  ts->get_Location(&tsLocation);
                  if (IsEqual(location, tsLocation))
                  {
                     // there is already a temporary support at this location (this can happen if the user builds the model this way)
                     bExistingTS = true;
                     break;
                  }
                  ts.Release();
               }

               if (!bExistingTS)
               {
                  CComPtr<ITemporarySupport> objRightTS;
                  objRightTS.CoCreateInstance(CLSID_TemporarySupport);
                  objRightTS->put_OmitReaction(VARIANT_TRUE); // don't apply the temporary support reaction back into the structure


                  objRightTS->put_ID(aheadID);
                  objRightTS->put_Location(location);

                  objRightTS->SetLoadModifier(lctStrength, loadModifier.LoadModifier(lrfdTypes::StrengthI, lrfdTypes::Min), loadModifier.LoadModifier(lrfdTypes::StrengthI, lrfdTypes::Max));

                  objRightTS->put_BoundaryCondition(bcRoller);

                  // If there is a continuous connection at the pier, remove the temporary support when the 
                  // connection becomes continuous, otherwise keep it to preserve the stability of the LBAM
                  objRightTS->put_StageErected(CComBSTR(""));
                  if (bContinuousConnection)
                  {
                     objRightTS->put_StageRemoved(GetLBAMStageName(rightContinuityIntervalIdx));
                  }
                  else
                  {
                     objRightTS->put_StageRemoved(CComBSTR(""));
                  }
                  objPrimarySupport->AddAssociatedSupport(aheadID);

                  objTemporarySupports->Add(objRightTS);
               }
            }
         }
      } // next segment
   } // next group
}

void CGirderModelManager::CreateLBAMSuperstructureMember(Float64 length,const std::vector<SuperstructureMemberData>& vData,ISuperstructureMember** ppMbr) const
{
   CComPtr<ISuperstructureMember> ssm;
   ssm.CoCreateInstance(CLSID_SuperstructureMember);

   ssm->put_Length(length);

   for(const auto& data : vData)
   {
      ATLASSERT(!IsZero(data.ea));
      ATLASSERT(!IsZero(data.ei));
      ATLASSERT(!IsZero(data.ea_defl));
      ATLASSERT(!IsZero(data.ei_defl));

      CComPtr<ISegment> mbrSegment;
      mbrSegment.CoCreateInstance( CLSID_Segment );
      mbrSegment->put_Length(length);

      CComPtr<ISegmentCrossSection> section;
      section.CoCreateInstance(CLSID_SegmentCrossSection);
      section->put_EAForce(data.ea);
      section->put_EIForce(data.ei);
      section->put_EADefl(data.ea_defl);
      section->put_EIDefl(data.ei_defl);
      section->put_Depth(1.0); // dummy value

      mbrSegment->putref_SegmentCrossSection(section);

      ssm->AddSegment(data.stage,mbrSegment);
   }

   (*ppMbr) = ssm;
   (*ppMbr)->AddRef();
}

void CGirderModelManager::GetLBAMBoundaryConditions(bool bContinuous,const CTimelineManager* pTimelineMgr,GroupIndexType grpIdx,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,ISuperstructureMember* pSSMbr) const
{
   CSegmentKey segmentKey(pSegment->GetSegmentKey());

   // Determine boundary conditions at end of segment
   const CClosureJointData* pClosure = (endType == pgsTypes::metStart ? pSegment->GetClosureJoint(pgsTypes::metStart) : pSegment->GetClosureJoint(pgsTypes::metEnd));
   const CPierData2* pPier = nullptr;
   const CTemporarySupportData* pTS = nullptr;
   if ( pClosure )
   {
      pPier = pClosure->GetPier();
      pTS   = pClosure->GetTemporarySupport();
   }
   else
   {
      pPier = (endType == pgsTypes::metStart ? pSegment->GetSpan(pgsTypes::metStart)->GetPrevPier() : pSegment->GetSpan(pgsTypes::metEnd)->GetNextPier());
   }

   if (pTS)
   {
      // Deal with erection towers and drop in segments
      pgsTypes::DropInType dropInType = pSegment->IsDropIn();
      // See if we have a free end (i.e. segment is freely supported by the adjacent segment at endType)
      bool isDropInAtEnd = pgsTypes::ditYesFreeBothEnds == dropInType ||
                           (endType == pgsTypes::metStart && pgsTypes::ditYesFreeStartEnd == dropInType) ||
                           (endType == pgsTypes::metEnd && pgsTypes::ditYesFreeEndEnd == dropInType);

      // Always put hinges at both sides of erection towers
      if (pTS->GetSupportType() == pgsTypes::ErectionTower || isDropInAtEnd)
      {
         // Place a hinge 
         ATLASSERT(pClosure);
         CClosureKey closureKey(pClosure->GetClosureKey());
         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType closureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
         CComBSTR bstrContinuity(GetLBAMStageName(closureIntervalIdx));
         pSSMbr->SetEndReleaseRemovalStage(endType == pgsTypes::metStart ? ssLeft : ssRight, bstrContinuity);
         pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, mrtMz);
      }
   }
   else
   {
      if ( pPier->IsInteriorPier() )
      {
         GET_IFACE(IIntervals,pIntervals);
         ATLASSERT(bContinuous == true); // always a continuous model in this case

         IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
         pIntervals->GetContinuityInterval(pPier->GetIndex(),&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

         IntervalIndexType continuityIntervalIdx = (endType == pgsTypes::metStart ? rightContinuityIntervalIdx : leftContinuityIntervalIdx);

         CComBSTR bstrContinuity( GetLBAMStageName(continuityIntervalIdx) );
         pSSMbr->SetEndReleaseRemovalStage(endType == pgsTypes::metStart ? ssLeft : ssRight, bstrContinuity);
         pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, mrtMz);
      }
      else
      {
         ATLASSERT(pPier->IsBoundaryPier());
         if ( bContinuous && pPier->IsContinuousConnection() )
         {
            // continuous at pier
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
            pIntervals->GetContinuityInterval(pPier->GetIndex(),&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

            IntervalIndexType continuityIntervalIdx = (endType == pgsTypes::metStart ? rightContinuityIntervalIdx : leftContinuityIntervalIdx);

            CComBSTR bstrContinuity( GetLBAMStageName(continuityIntervalIdx) );
            pSSMbr->SetEndReleaseRemovalStage(endType == pgsTypes::metStart ? ssLeft : ssRight, bstrContinuity);
            pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight,mrtMz);
         }
         else
         {
            // not continuous at pier
            if ( 
                 (pPier->GetNextSpan() != nullptr && endType == pgsTypes::metEnd)   // not the last segment and setting boundary condition at end
                 ||                                                              // -OR-
                 (pPier->GetPrevSpan() != nullptr && endType == pgsTypes::metStart) // not the first segment and setting boundary condition at start
               )
            {
               pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, mrtMz);
            }
            // else
            // {
            //   don't add an end release if this is the start of the first segment or the end of the last segment
            // }
         }
      }
   }
}

BoundaryConditionType CGirderModelManager::GetLBAMBoundaryConditions(bool bContinuous,const CPierData2* pPier) const
{
   if ( !bContinuous )
   {
      // we are building a simple-span only model. always return a roller boundary condition
      // the other LBAM building code will detect that all boundary conditions are rollers
      // and will change one of them to a hinge so that equilibrium in the LBAM is maintained
      return bcRoller;
   }

   BoundaryConditionType bc;
   if ( pPier->IsBoundaryPier() )
   {
      switch( pPier->GetBoundaryConditionType() )
      {
      case pgsTypes::bctHinge:
         bc = bcPinned;
         break;

      case pgsTypes::bctRoller:
         bc = bcRoller;
         break;

      case pgsTypes::bctContinuousAfterDeck:
      case pgsTypes::bctContinuousBeforeDeck:
         bc = bcPinned;
         break;

      case pgsTypes::bctIntegralAfterDeck:
      case pgsTypes::bctIntegralBeforeDeck:
         bc = bcFixed;
         break;

      case pgsTypes::bctIntegralAfterDeckHingeBack:
      case pgsTypes::bctIntegralBeforeDeckHingeBack:
      case pgsTypes::bctIntegralAfterDeckHingeAhead:
      case pgsTypes::bctIntegralBeforeDeckHingeAhead:
         bc = bcPinned;
         break;

      default:
         ATLASSERT(FALSE); // is there a new connection type?
         bc = bcRoller;
      }
   }
   else
   {
      ATLASSERT(pPier->IsInteriorPier());
      bc = bcRoller;

      switch( pPier->GetSegmentConnectionType() )
      {
      case pgsTypes::psctContinousClosureJoint:
      case pgsTypes::psctContinuousSegment:
         // do nothing for continuous case... use the pier boundary condition
         break;

      case pgsTypes::psctIntegralClosureJoint:
      case pgsTypes::psctIntegralSegment:
         // if segments are integral with pier, use a fixed boundary condition
         bc = bcFixed;
         break;

      default:
         ATLASSERT(FALSE); // is there a new connection type?
         bc = bcRoller;
      }
   }

   return bc;
}

void CGirderModelManager::ApplySelfWeightLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const
{
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IReactions,pReactions);
   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // create a load group for incremental girder loads
   // incremental loading happens when boundary conditions change (like between storage and erection)
   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftGirder) );
   CComBSTR bstrLoadGroupIncremental( bstrLoadGroup );
   bstrLoadGroupIncremental += CComBSTR(_T("_Incremental"));
   CComBSTR bstrStage;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   MemberIDType mbrID = 0;
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdrLineIdx, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      GroupIndexType grpIdx = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);

      if ( grpIdx != 0 )
      {
         CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),0);
         Float64 left_brg_offset = pBridge->GetSegmentStartBearingOffset( segmentKey );
         Float64 left_end_dist   = pBridge->GetSegmentStartEndDistance( segmentKey );

         if ( !IsZero(left_brg_offset - left_end_dist) )
         {
            mbrID++; // +1 to get to past the CL of the intermediate pier
         }
      }

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         // determine which stage to apply the load
         IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         bstrStage = GetLBAMStageName(erectSegmentIntervalIdx);

         // Because there is a change in modulus of elasticity between putting the girder segment into storage
         // and when it is erected, and because the storage and erected support locations can be different,
         // we need to compute the incremental deflection due to the change of support locations. To do this,
         // we apply a set of loads that produce the incremental moment caused by the support location change.
         // This set of loads is simply applying the storage reactions to the girder segment in the erected
         // configuration.
         // 
         // In reality, we should use this incremental load for moments, shears, and reactions, however these 
         // quantities are independent of E and thus applying the full girder self-weight to the LBAM
         // gives us what we want without additional overhead. Moment, shear, and reactions are also the most commonly requested
         // results so we don't incur the overhead of adding in storage interval results.
         //
         // When we get the incremental deformations (deflection and rotation) from the LBAM, for the girder dead load
         // or any load combination or limit state that uses the girder dead load, we have to subtract the response
         // due to pftGirder and add in the response do to pftGirder_Incremental.
         // For cumulative results, remove pftGirder, add pftGirder_Incremental, and add the cumulative response
         // from the storage interval.
         //
         // NOTE: Technically, the incremental force is that which occurs between hauling and erection. However,
         // since the elapsed time during hauling is assumed to be zero, and the principle of superposition applies
         // to linear elastic analysis, it is correct to use the increment from storage instead of the increment
         // from hauling. Hauling models are handled a little differently then storage, so using storage is easier.
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         Float64 Rleft, Rright;
         pReactions->GetSegmentReactions(segmentKey,storageIntervalIdx,pgsTypes::pftGirder,bat,rtCumulative,&Rleft,&Rright);

         Float64 left_storage_location, right_storage_location;
         pGirder->GetSegmentStorageSupportLocations(segmentKey,&left_storage_location,&right_storage_location);

         Float64 Ls = pBridge->GetSegmentLength(segmentKey);
         Float64 Xl = left_storage_location;
         Float64 Xr = Ls - right_storage_location;

         std::vector<ConcentratedLoad> vStorageReactions;
         vStorageReactions.emplace_back(Xl, -Rleft); // left reaction, apply load equal and opposite to reaction
         vStorageReactions.emplace_back(Xr, -Rright);

         for( const auto& load : vStorageReactions)
         {
            Float64 P   = load.Load;

            MemberType mbrType;
            MemberIDType mbrID2;
            Float64 loc;
            GetLoadPosition(pModel,segmentKey,load.Loc,true,&mbrType,&mbrID2,&loc);

            CComPtr<IPointLoad> ptLoad;
            ptLoad.CoCreateInstance(CLSID_PointLoad);
            ptLoad->put_MemberType(mbrType);
            ptLoad->put_MemberID(mbrID2);
            ptLoad->put_Location(loc);
            ptLoad->put_Fy(P);

            CComPtr<IPointLoadItem> item;
            pointLoads->Add(bstrStage, bstrLoadGroupIncremental, ptLoad, &item);
         } // next load

         // get the dead load items
         std::vector<SegmentLoad> vSegmentLoads;
         std::vector<DiaphragmLoad> vDiaphragmLoads;
         std::vector<ClosureJointLoad> vClosureJointLoads;
         GetSegmentSelfWeightLoad(segmentKey,&vSegmentLoads,&vDiaphragmLoads,&vClosureJointLoads);
         // precast diaphragm loads (vDiaphragmLoads) are applied in ApplyIntermediateDiaphragmLoads

         mbrID = ApplyDistributedLoadsToSegment(erectSegmentIntervalIdx,pModel,analysisType,mbrID,segmentKey,vSegmentLoads,bstrStage,bstrLoadGroup);

         if ( segIdx < nSegments-1 )
         {
            mbrID = ApplyClosureJointSelfWeightLoad(pModel,analysisType,mbrID,segmentKey,vClosureJointLoads);
         }
      } // next segment

      CSegmentKey lastSegmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( lastSegmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( lastSegmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}

MemberIDType CGirderModelManager::ApplyClosureJointSelfWeightLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,MemberIDType mbrID,const CClosureKey& closureKey,const std::vector<ClosureJointLoad>& vClosureJointLoads) const
{
   // NOTE: We must run through this method even if vClosureJointLoads.size() == 0. This method counts the closure joint superstructure members and returns the ssmbrID of the 
   // next superstructure member to be loaded in the overall bridge model

#if defined _DEBUG
   // used below to error check loading geometry
   CComPtr<ISuperstructureMembers> ssmbrs;
   pModel->get_SuperstructureMembers(&ssmbrs);
#endif

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castCJIntervalIdx = pIntervals->GetCastClosureJointInterval(closureKey);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftGirder) );
   CComBSTR bstrStage(GetLBAMStageName(castCJIntervalIdx));

   // Apply load to model
   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSplicedGirderData* pSplicedGirder = pBridgeDesc->GetGirderGroup(closureKey.groupIndex)->GetGirder(closureKey.girderIndex);

   const CPierData2* pPier;
   const CTemporarySupportData* pTS;
   pSplicedGirder->GetSegment(closureKey.segmentIndex)->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
   if ( pPier && pPier->IsInteriorPier() )
   {
      // closure joint is modeled by two superstructure members
      if ( 0 < vClosureJointLoads.size() )
      {
#if defined _DEBUG
         CComPtr<ISuperstructureMember> ssmbr;
         ssmbrs->get_Item(mbrID,&ssmbr);
         Float64 Lssmbr;
         ssmbr->get_Length(&Lssmbr);
         ATLASSERT(IsEqual(vClosureJointLoads[0].Xend-vClosureJointLoads[0].Xstart,Lssmbr));
#endif
         CComPtr<IDistributedLoad> wLeft;
         wLeft.CoCreateInstance(CLSID_DistributedLoad);
         wLeft->put_MemberType(mtSuperstructureMember);
         wLeft->put_MemberID(mbrID);
         wLeft->put_Direction(ldFy);
         wLeft->put_WStart(vClosureJointLoads[0].Wstart);
         wLeft->put_WEnd(vClosureJointLoads[0].Wend);
         wLeft->put_StartLocation(vClosureJointLoads[0].Xstart);
         wLeft->put_EndLocation(vClosureJointLoads[0].Xend);

         CComPtr<IDistributedLoadItem> loadItem;
         distLoads->Add(bstrStage,bstrLoadGroup,wLeft,&loadItem);
      }

      mbrID++;

      if ( 0 < vClosureJointLoads.size() )
      {
         ATLASSERT(vClosureJointLoads.size() == 2);
#if defined _DEBUG
         CComPtr<ISuperstructureMember> ssmbr;
         ssmbrs->get_Item(mbrID,&ssmbr);
         Float64 Lssmbr;
         ssmbr->get_Length(&Lssmbr);
         ATLASSERT(IsEqual(vClosureJointLoads[1].Xend-vClosureJointLoads[1].Xstart,Lssmbr));
#endif
         CComPtr<IDistributedLoad> wRight;
         wRight.CoCreateInstance(CLSID_DistributedLoad);
         wRight->put_MemberType(mtSuperstructureMember);
         wRight->put_MemberID(mbrID);
         wRight->put_Direction(ldFy);
         wRight->put_WStart(vClosureJointLoads[1].Wstart);
         wRight->put_WEnd(vClosureJointLoads[1].Wend);
         wRight->put_StartLocation(vClosureJointLoads[1].Xstart - vClosureJointLoads[0].Xend);
         wRight->put_EndLocation(vClosureJointLoads[1].Xend - vClosureJointLoads[1].Xstart);

         CComPtr<IDistributedLoadItem> loadItem;
         distLoads->Add(bstrStage,bstrLoadGroup,wRight,&loadItem);
      }

      mbrID++;
   }
   else //if ( segIdx < nSegments-1 )
   {
      // closure joint is modeled by one superstructure member
      if ( 0 < vClosureJointLoads.size() )
      {
         ATLASSERT(vClosureJointLoads.size() == 2);
#if defined _DEBUG
         CComPtr<ISuperstructureMember> ssmbr;
         ssmbrs->get_Item(mbrID,&ssmbr);
         Float64 Lssmbr;
         ssmbr->get_Length(&Lssmbr);
         ATLASSERT(IsEqual(vClosureJointLoads[1].Xend - vClosureJointLoads[0].Xstart,Lssmbr));
#endif

         CComPtr<IDistributedLoad> wLeft;
         wLeft.CoCreateInstance(CLSID_DistributedLoad);
         wLeft->put_MemberType(mtSuperstructureMember);
         wLeft->put_MemberID(mbrID);
         wLeft->put_Direction(ldFy);
         wLeft->put_WStart(vClosureJointLoads[0].Wstart);
         wLeft->put_WEnd(vClosureJointLoads[0].Wend);
         wLeft->put_StartLocation(vClosureJointLoads[0].Xstart);
         wLeft->put_EndLocation(vClosureJointLoads[0].Xend);

         CComPtr<IDistributedLoadItem> loadItem;
         distLoads->Add(bstrStage,bstrLoadGroup,wLeft,&loadItem);

         CComPtr<IDistributedLoad> wRight;
         wRight.CoCreateInstance(CLSID_DistributedLoad);
         wRight->put_MemberType(mtSuperstructureMember);
         wRight->put_MemberID(mbrID);
         wRight->put_Direction(ldFy);
         wRight->put_WStart(vClosureJointLoads[1].Wstart);
         wRight->put_WEnd(vClosureJointLoads[1].Wend);
         wRight->put_StartLocation(vClosureJointLoads[1].Xstart);
         wRight->put_EndLocation(vClosureJointLoads[1].Xend);

         loadItem.Release();
         distLoads->Add(bstrStage,bstrLoadGroup,wRight,&loadItem);
      }

      mbrID++;
   }

   return mbrID;
}

void CGirderModelManager::ApplyDiaphragmLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr) const
{
   ApplyDiaphragmLoadsAtPiers(pModel,analysisType,gdr);
   ApplyIntermediateDiaphragmLoads(pModel,analysisType,gdr);
}

void CGirderModelManager::ApplySlabLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr) const
{
   // if there is no deck, get the heck outta here!
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtNone )
   {
      return;
   }

   CComBSTR bstrSlabLoadGroup( GetLoadGroupName(pgsTypes::pftSlab) );
   CComBSTR bstrSlabPadLoadGroup( GetLoadGroupName(pgsTypes::pftSlabPad) );
   CComBSTR bstrPanelLoadGroup( GetLoadGroupName(pgsTypes::pftSlabPanel) );

   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IBridge, pBridge);

   IndexType nRegions = pBridge->GetDeckCastingRegionCount();
   ATLASSERT(0 < nRegions);


   MemberIDType mbrID = 0;

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(girderKey.girderIndex);

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         std::vector<SlabLoad> slabLoads;
         GetMainSpanSlabLoad(segmentKey, &slabLoads);

         MemberIDType nextMemberID;
         for (IndexType castingRegionIdx = 0; castingRegionIdx < nRegions; castingRegionIdx++)
         {
            // Create equivalent LinearLoad vectors from the SlabLoad information
            std::vector<LinearLoad> vSlabLoads, vHaunchLoads, vPanelLoads;
            GetSlabLoad(slabLoads, castingRegionIdx, vSlabLoads, vHaunchLoads, vPanelLoads);

            IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(castingRegionIdx);
            CComBSTR bstrStage(GetLBAMStageName(castDeckIntervalIdx));
            ApplyDistributedLoadsToSegment(castDeckIntervalIdx, pModel, analysisType, mbrID, segmentKey, vSlabLoads, bstrStage, bstrSlabLoadGroup);
            ApplyDistributedLoadsToSegment(castDeckIntervalIdx, pModel, analysisType, mbrID, segmentKey, vHaunchLoads, bstrStage, bstrSlabPadLoadGroup);
            nextMemberID = ApplyDistributedLoadsToSegment(castDeckIntervalIdx, pModel, analysisType, mbrID, segmentKey, vPanelLoads, bstrStage, bstrPanelLoadGroup);
         }

         mbrID = nextMemberID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier && !pPier->IsAbutment() )
         {
            IndexType nSSMbrs = GetSuperstructureMemberCount(pPier,segmentKey.girderIndex);
            mbrID += nSSMbrs;
         }
         else if ( pTS )
         {
            ATLASSERT(pTS);
            IndexType nSSMbrs = GetSuperstructureMemberCount(pTS, segmentKey.girderIndex);
            mbrID += nSSMbrs;
         }
      } // next segment
   } // next group
}

void CGirderModelManager::ApplyOverlayLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr,bool bContinuousModel) const
{
// RAB 7/21/99 - This correction is a result of a phone conversation with
// Beth Shannon of Idaho DOT.
// Per 4.6.2.2.1, the overlay load is evenly distributed over all the girders
// See pg 4-23
// "Where bridges meet the conditions specified herein, permanent loads of and 
// on the deck may be distributed uniformly among the beams and/or stringers"

   GET_IFACE(IBridge,pBridge);

   // if there isn't an overlay, get the heck outta here
   if ( !pBridge->HasOverlay() )
   {
      return;
   }

   GET_IFACE(IPointOfInterest,pPOI);

   // Make sure we have a roadway to work with
   PierIndexType nPiers = pBridge->GetPierCount();
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      Float64 station = pBridge->GetPierStation(pierIdx);
      Float64 dfs     = pPOI->ConvertRouteToBridgeLineCoordinate(station);
      Float64 loffs   = pBridge->GetLeftInteriorCurbOffset(dfs);
      Float64 roffs   = pBridge->GetRightInteriorCurbOffset(dfs);

      if (roffs <= loffs)
      {
         CString strMsg(_T("The distance between interior curb lines cannot be negative. Increase the deck width or decrease sidewalk widths."));
         pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::Railing,strMsg);

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);

         strMsg += _T(" See Status Center for Details");
         THROW_UNWIND(strMsg,XREASON_NEGATIVE_GIRDER_LENGTH);
      }
   }

   bool bFutureOverlay = pBridge->IsFutureOverlay();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Get stage when overlay is applied to the structure
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   CComBSTR bstrStage(GetLBAMStageName(overlayIntervalIdx));

   // setup stage and load group names
   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftOverlay) ); 
   CComBSTR bstrLoadGroupRating( GetLoadGroupName(pgsTypes::pftOverlayRating) );

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   MemberIDType mbrID = 0;
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      GroupIndexType grpIdx = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);

         const CPierData2* pLeftPier = nullptr;
         const CTemporarySupportData* pLeftTS = nullptr;
         pSegment->GetSupport(pgsTypes::metStart,&pLeftPier,&pLeftTS);

         const CPierData2* pRightPier = nullptr;
         const CTemporarySupportData* pRightTS = nullptr;
         pSegment->GetSupport(pgsTypes::metEnd,&pRightPier,&pRightTS);

         // Add the overlay load in the main part of the span
         std::vector<OverlayLoad> vOverlayLoads;
         GetMainSpanOverlayLoad(segmentKey, &vOverlayLoads);

         std::vector<LinearLoad> vLinearLoads;
         for ( const auto& load : vOverlayLoads)
         {
            vLinearLoads.push_back(load);
         }

         MemberIDType newMbrID = ApplyDistributedLoadsToSegment(overlayIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroup);

         if ( !bFutureOverlay )
         {
            ApplyDistributedLoadsToSegment(overlayIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroupRating);
         }

         // model the load at the closure joint
         if ( segIdx != nSegments-1 )
         {
            // Load at the start of the closure
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().Xstart;
            Float64 Xend   = vLinearLoads.back().Xend;
            Float64 Wstart = vLinearLoads.back().Wstart;
            Float64 Wend   = vLinearLoads.back().Wend; // this is the load at the CL Pier

            // Wstart is load at end of segment/start of closure
            Float64 a = segmentLength - Xstart;
            ATLASSERT( 0 <= a );
            Wstart = ::LinInterp(a,Wstart,Wend,Xend-Xstart);

            // Get load at end of closure (start of next segment)
            CSegmentKey nextSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex+1);
            std::vector<OverlayLoad> vNextSegmentOverlayLoads;
            GetMainSpanOverlayLoad(nextSegmentKey,&vNextSegmentOverlayLoads);

            CComPtr<IDistributedLoad> closureLoad;
            closureLoad.CoCreateInstance(CLSID_DistributedLoad);
            closureLoad->put_MemberType(mtSuperstructureMember);
            closureLoad->put_MemberID(newMbrID);
            closureLoad->put_Direction(ldFy);
            closureLoad->put_WStart(Wstart);
            closureLoad->put_WEnd(vNextSegmentOverlayLoads.front().Wstart);
            closureLoad->put_StartLocation(0.0);
            closureLoad->put_EndLocation(-1.0);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distLoads->Add(bstrStage,bstrLoadGroup,closureLoad,&distLoadItem);

            if ( !bFutureOverlay )
            {
               distLoadItem.Release();
               distLoads->Add(bstrStage,bstrLoadGroupRating,closureLoad,&distLoadItem);
            }
         }

         // if we are at an intermediate pier with a continuous boundary condition
         // then we have to load the two LBAM superstructure members used model the pier connection region
         if ( bContinuousModel && // continuous model (don't load across piers for simple span models)
              segIdx == nSegments-1 && // last segment in the girder
              pGroup->GetNextGirderGroup() != nullptr && // not the last group/span in the bridge
              pGroup->GetPier(pgsTypes::metEnd)->IsContinuousConnection() // continuous boundary condition
            )
         {
            Float64 leftBrgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
            if ( !IsZero(leftBrgOffset) )
            {
               // Determine load on left side of CL Pier
               Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
               Float64 Xstart = vLinearLoads.back().Xstart;
               Float64 Xend   = vLinearLoads.back().Xend;
               Float64 Wstart = vLinearLoads.back().Wstart;
               Float64 Wend   = vLinearLoads.back().Wend; // this is the load at the CL Pier

               // Wstart is load at end of segment
               Float64 a = segmentLength - Xstart;
               ATLASSERT( 0 <= a );
               Wstart = ::LinInterp(a,Wstart,Wend,Xend-Xstart);

               CComPtr<IDistributedLoad> load1;
               load1.CoCreateInstance(CLSID_DistributedLoad);
               load1->put_MemberType(mtSuperstructureMember);
               load1->put_MemberID(newMbrID);
               load1->put_Direction(ldFy);
               load1->put_WStart(Wstart);
               load1->put_WEnd(Wend);
               load1->put_StartLocation(0.0);
               load1->put_EndLocation(-1.0);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage,bstrLoadGroup,load1,&distLoadItem);

               if ( !bFutureOverlay )
               {
                  distLoadItem.Release();
                  distLoads->Add(bstrStage,bstrLoadGroupRating,load1,&distLoadItem);
               }
            }


            // Determine the load on the right side of the CL Pier
            vOverlayLoads.clear();
            CSegmentKey nextSegmentKey(segmentKey.groupIndex+1,segmentKey.girderIndex,0);
            nextSegmentKey.girderIndex = Min(nextSegmentKey.girderIndex,pBridge->GetGirderCount(nextSegmentKey.groupIndex)-1);
            Float64 rightBrgOffset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
            if ( !IsZero(rightBrgOffset) )
            {
               GetMainSpanOverlayLoad(nextSegmentKey,&vOverlayLoads);

               Float64 Wstart = vLinearLoads.back().Wend;
               Float64 Wend   = vOverlayLoads.front().Wstart;
               Float64 Xstart = 0;
               Float64 Xend = vOverlayLoads.front().Xend;

               Float64 end_offset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey) - pBridge->GetSegmentStartEndDistance(nextSegmentKey);
               Wend = ::LinInterp(end_offset,Wstart,Wend,Xend-Xstart);

               CComPtr<IDistributedLoad> load2;
               load2.CoCreateInstance(CLSID_DistributedLoad);
               load2->put_MemberType(mtSuperstructureMember);
               load2->put_MemberID(newMbrID+1);
               load2->put_Direction(ldFy);
               load2->put_WStart(Wstart);
               load2->put_WEnd(Wend);
               load2->put_StartLocation(0.0);
               load2->put_EndLocation(-1.0);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage,bstrLoadGroup,load2,&distLoadItem);

               if ( !bFutureOverlay )
               {
                  distLoadItem.Release();
                  distLoads->Add(bstrStage,bstrLoadGroupRating,load2,&distLoadItem);
               }
            }
         }

         mbrID = newMbrID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier && !pPier->IsAbutment() )
         {
            IndexType nSSMbrs = GetSuperstructureMemberCount(pPier,gdrIdx);
            mbrID += nSSMbrs;
         }
         else if ( pTS )
         {
            ATLASSERT(pTS);
            IndexType nSSMbrs = GetSuperstructureMemberCount(pTS,gdrIdx);
            mbrID += nSSMbrs;
         }
      } // next segment
   } // next group
}

void CGirderModelManager::ApplyConstructionLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE_NOCHECK(IBridge,pBridge);

   // apply construction loads in the interval when the deck is cast
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   CComBSTR bstrStage(GetLBAMStageName(constructionLoadIntervalIdx));

   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftConstruction) ); 

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   MemberIDType mbrID = 0;
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      CGirderKey girderKey(pSplicedGirder->GetGirderKey());

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         // Add the overlay load in the main part of the span
         std::vector<ConstructionLoad> loads;
         GetMainConstructionLoad(segmentKey, &loads);
         std::vector<LinearLoad> vLinearLoads;
         for( const auto& load : loads)
         {
            vLinearLoads.push_back(load);
         }

         MemberIDType newMbrID = ApplyDistributedLoadsToSegment(constructionLoadIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroup);

         // model the load at the closure joint
         if ( segIdx != nSegments-1 )
         {
            // Load at the start of the closure
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().Xstart;
            Float64 Xend   = vLinearLoads.back().Xend;
            Float64 Wstart = vLinearLoads.back().Wstart;
            Float64 Wend   = vLinearLoads.back().Wend; // this is the load at the CL Pier

            // Wstart is load at end of segment/start of closure
            Float64 a = segmentLength - Xstart;
            ATLASSERT( 0 <= a );
            Wstart = ::LinInterp(a,Wstart,Wend,Xend-Xstart);

            // Get load at end of closure (start of next segment)
            CSegmentKey nextSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex+1);
            std::vector<ConstructionLoad> vNextSegmentLoads;
            GetMainConstructionLoad(nextSegmentKey,&vNextSegmentLoads);

            CComPtr<IDistributedLoad> constructionLoad;
            constructionLoad.CoCreateInstance(CLSID_DistributedLoad);
            constructionLoad->put_MemberType(mtSuperstructureMember);
            constructionLoad->put_MemberID(newMbrID);
            constructionLoad->put_Direction(ldFy);
            constructionLoad->put_WStart(Wstart);
            constructionLoad->put_WEnd(vNextSegmentLoads.front().Wstart);
            constructionLoad->put_StartLocation(0.0);
            constructionLoad->put_EndLocation(-1.0);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distLoads->Add(bstrStage,bstrLoadGroup,constructionLoad,&distLoadItem);
         }

         // if we are at an intermediate pier with a continuous boundary condition
         // then we have to load the two LBAM superstructure members used to model the pier connection region
         if ( analysisType == pgsTypes::Continuous && // continuous model (don't load across piers for simple span models)
              segIdx == nSegments-1 && // last segment in the girder
              pGroup->GetNextGirderGroup() != nullptr && // not the last group/span in the bridge
              pGroup->GetPier(pgsTypes::metEnd)->IsContinuousConnection() // continuous boundary condition
            )
         {
            Float64 leftBrgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
            if ( !IsZero(leftBrgOffset) )
            {
               // Determine load on left side of CL Pier
               Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
               Float64 Xstart = vLinearLoads.back().Xstart;
               Float64 Xend   = vLinearLoads.back().Xend;
               Float64 Wstart = vLinearLoads.back().Wstart;
               Float64 Wend   = vLinearLoads.back().Wend; // this is the load at the CL Pier

               // Wstart is load at end of segment
               Float64 a = segmentLength - Xstart;
               ATLASSERT( 0 <= a );
               Wstart = ::LinInterp(a,Wstart,Wend,Xend-Xstart);

               CComPtr<IDistributedLoad> load1;
               load1.CoCreateInstance(CLSID_DistributedLoad);
               load1->put_MemberType(mtSuperstructureMember);
               load1->put_MemberID(newMbrID);
               load1->put_Direction(ldFy);
               load1->put_WStart(Wstart);
               load1->put_WEnd(Wend);
               load1->put_StartLocation(0.0);
               load1->put_EndLocation(-1.0);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage,bstrLoadGroup,load1,&distLoadItem);
            }

            // Determine the load on the right side of the CL Pier
            CSegmentKey nextSegmentKey(segmentKey.groupIndex+1,segmentKey.girderIndex,0);
            nextSegmentKey.girderIndex = Min(nextSegmentKey.girderIndex,pBridge->GetGirderCount(nextSegmentKey.groupIndex)-1);
            Float64 rightBrgOffset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
            if ( !IsZero(rightBrgOffset) )
            {
               loads.clear();
               GetMainConstructionLoad(nextSegmentKey,&loads);

               Float64 Wstart = vLinearLoads.back().Wend;
               Float64 Wend   = loads.front().Wstart;
               Float64 Xstart = 0;
               Float64 Xend = loads.front().Xend;

               Float64 end_offset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey) - pBridge->GetSegmentStartEndDistance(nextSegmentKey);
               Wend = ::LinInterp(end_offset,Wstart,Wend,Xend-Xstart);

               CComPtr<IDistributedLoad> load2;
               load2.CoCreateInstance(CLSID_DistributedLoad);
               load2->put_MemberType(mtSuperstructureMember);
               load2->put_MemberID(newMbrID+1);
               load2->put_Direction(ldFy);
               load2->put_WStart(Wstart);
               load2->put_WEnd(Wend);
               load2->put_StartLocation(0.0);
               load2->put_EndLocation(-1.0);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage,bstrLoadGroup,load2,&distLoadItem);
            }
         }

         mbrID = newMbrID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier && !pPier->IsAbutment() )
         {
            IndexType nSSMbrs = GetSuperstructureMemberCount(pPier,thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
         else if ( pTS )
         {
            ATLASSERT(pTS);
            IndexType nSSMbrs = GetSuperstructureMemberCount(pTS,thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
      } // next segment
   } // next group
}

void CGirderModelManager::ApplyShearKeyLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   CComBSTR bstrStage(GetLBAMStageName(castShearKeyIntervalIdx));

   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftShearKey) ); 

   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);

   MemberIDType mbrID = 0;
   for (const auto& thisGirderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      CGirderKey girderKey(pSplicedGirder->GetGirderKey());

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);

         const CPierData2* pLeftPier = nullptr;
         const CTemporarySupportData* pLeftTS = nullptr;
         pSegment->GetSupport(pgsTypes::metStart,&pLeftPier,&pLeftTS);

         const CPierData2* pRightPier = nullptr;
         const CTemporarySupportData* pRightTS = nullptr;
         pSegment->GetSupport(pgsTypes::metEnd,&pRightPier,&pRightTS);

         // Add load in the main part of the span
         std::vector<ShearKeyLoad> skLoads;
         GetMainSpanShearKeyLoad(segmentKey, &skLoads);

         std::vector<LinearLoad> vLinearLoads;
         for( const auto& skLoad : skLoads)
         {
            vLinearLoads.emplace_back(skLoad.StartLoc, skLoad.EndLoc, skLoad.UniformLoad + skLoad.StartJointLoad, skLoad.UniformLoad + skLoad.EndJointLoad);
         }

         mbrID = ApplyDistributedLoadsToSegment(castShearKeyIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroup);

         // Shear key and joint loads do not get applied to
         // closure joints or piers

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier && !pPier->IsAbutment() )
         {
            IndexType nSSMbrs = GetSuperstructureMemberCount(pPier,thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
         else if ( pTS )
         {
            ATLASSERT(pTS);
            IndexType nSSMbrs = GetSuperstructureMemberCount(pTS,thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
      } // next segment
   } // next group
}

void CGirderModelManager::ApplyLongitudinalJointLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType, GirderIndexType gdr) const
{
   if (!HasLongitudinalJointLoad())
   {
      return;
   }

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   CComBSTR bstrStage(GetLBAMStageName(castLongitudinalJointIntervalIdx));

   CComBSTR bstrLoadGroup(GetLoadGroupName(pgsTypes::pftLongitudinalJoint));

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);

   MemberIDType mbrID = 0;
   for(const auto& thisGirderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(thisGirderKey, segIdx);

         // Add the load in the main part of the span
         std::vector<LongitudinalJointLoad> vLinearLoads;
         GetMainSpanLongitudinalJointLoad(segmentKey, &vLinearLoads);

         MemberIDType newMbrID = ApplyDistributedLoadsToSegment(castLongitudinalJointIntervalIdx, pModel, analysisType, mbrID, segmentKey, vLinearLoads, bstrStage, bstrLoadGroup);

         // model the load at the closure joint
         if (segIdx != nSegments - 1)
         {
            // Load at the start of the closure
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().Xstart;
            Float64 Xend = vLinearLoads.back().Xend;
            Float64 Wstart = vLinearLoads.back().Wstart;
            Float64 Wend = vLinearLoads.back().Wend; // this is the load at the CL Pier

                                                     // Wstart is load at end of segment/start of closure
            Float64 a = segmentLength - Xstart;
            ATLASSERT(0 <= a);
            Wstart = ::LinInterp(a, Wstart, Wend, Xend - Xstart);

            // Get load at end of closure (start of next segment)
            CSegmentKey nextSegmentKey(segmentKey.groupIndex, segmentKey.girderIndex, segmentKey.segmentIndex + 1);
            std::vector<LongitudinalJointLoad> vNextSegmentLoads;
            GetMainSpanLongitudinalJointLoad(nextSegmentKey, &vNextSegmentLoads);

            CComPtr<IDistributedLoad> load;
            load.CoCreateInstance(CLSID_DistributedLoad);
            load->put_MemberType(mtSuperstructureMember);
            load->put_MemberID(newMbrID);
            load->put_Direction(ldFy);
            load->put_WStart(Wstart);
            load->put_WEnd(vNextSegmentLoads.front().Wstart);
            load->put_StartLocation(0.0);
            load->put_EndLocation(-1.0);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distLoads->Add(bstrStage, bstrLoadGroup, load, &distLoadItem);
         }

         // if we are at an intermediate pier with a continuous boundary condition
         // then we have to load the two LBAM superstructure members used to model the pier connection region
         if (analysisType == pgsTypes::Continuous && // continuous model (don't load across piers for simple span models)
            segIdx == nSegments - 1 && // last segment in the girder
            pGroup->GetNextGirderGroup() != nullptr && // not the last group/span in the bridge
            pGroup->GetPier(pgsTypes::metEnd)->IsContinuousConnection() // continuous boundary condition
            )
         {
            Float64 leftBrgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
            if (!IsZero(leftBrgOffset))
            {
               // Determine load on left side of CL Pier
               Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
               Float64 Xstart = vLinearLoads.back().Xstart;
               Float64 Xend = vLinearLoads.back().Xend;
               Float64 Wstart = vLinearLoads.back().Wstart;
               Float64 Wend = vLinearLoads.back().Wend; // this is the load at the CL Pier

                                                        // Wstart is load at end of segment
               Float64 a = segmentLength - Xstart;
               ATLASSERT(0 <= a);
               Wstart = ::LinInterp(a, Wstart, Wend, Xend - Xstart);

               CComPtr<IDistributedLoad> load1;
               load1.CoCreateInstance(CLSID_DistributedLoad);
               load1->put_MemberType(mtSuperstructureMember);
               load1->put_MemberID(newMbrID);
               load1->put_Direction(ldFy);
               load1->put_WStart(Wstart);
               load1->put_WEnd(Wend);
               load1->put_StartLocation(0.0);
               load1->put_EndLocation(-1.0);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage, bstrLoadGroup, load1, &distLoadItem);
            }

            // Determine the load on the right side of the CL Pier
            CSegmentKey nextSegmentKey(segmentKey.groupIndex + 1, segmentKey.girderIndex, 0);
            nextSegmentKey.girderIndex = Min(nextSegmentKey.girderIndex, pBridge->GetGirderCount(nextSegmentKey.groupIndex) - 1);
            Float64 rightBrgOffset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
            if (!IsZero(rightBrgOffset))
            {
               std::vector<LongitudinalJointLoad> vNextSegmentLoads;
               GetMainSpanLongitudinalJointLoad(nextSegmentKey, &vNextSegmentLoads);

               Float64 Wstart = vLinearLoads.back().Wend;
               Float64 Wend = vNextSegmentLoads.front().Wstart;
               Float64 Xstart = 0;
               Float64 Xend = vNextSegmentLoads.front().Xend;

               Float64 end_offset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey) - pBridge->GetSegmentStartEndDistance(nextSegmentKey);
               Wend = ::LinInterp(end_offset, Wstart, Wend, Xend - Xstart);

               CComPtr<IDistributedLoad> load2;
               load2.CoCreateInstance(CLSID_DistributedLoad);
               load2->put_MemberType(mtSuperstructureMember);
               load2->put_MemberID(newMbrID + 1);
               load2->put_Direction(ldFy);
               load2->put_WStart(Wstart);
               load2->put_WEnd(Wend);
               load2->put_StartLocation(0.0);
               load2->put_EndLocation(-1.0);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage, bstrLoadGroup, load2, &distLoadItem);
            }
         }

         mbrID = newMbrID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetSupport(pgsTypes::metEnd, &pPier, &pTS);
         if (pPier && !pPier->IsAbutment())
         {
            IndexType nSSMbrs = GetSuperstructureMemberCount(pPier, thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
         else if (pTS)
         {
            ATLASSERT(pTS);
            IndexType nSSMbrs = GetSuperstructureMemberCount(pTS, thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
      } // next segment
   } // next group
}

void CGirderModelManager::GetSidewalkLoadFraction(const CSegmentKey& segmentKey,Float64* pSidewalkLoad,Float64* pFraLeft,Float64* pFraRight) const
{
   ComputeSidewalksBarriersLoadFractions();

   auto found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() );

   const auto& rload = found->second;
   *pSidewalkLoad = rload.m_SidewalkLoad;
   *pFraLeft      = rload.m_LeftSidewalkFraction;
   *pFraRight     = rload.m_RightSidewalkFraction;
}

void CGirderModelManager::GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,Float64* pFraExtLeft, Float64* pFraIntLeft,Float64* pFraExtRight,Float64* pFraIntRight) const
{
   ComputeSidewalksBarriersLoadFractions();

   auto found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() );

   const auto& rload = found->second;
   *pBarrierLoad = rload.m_BarrierLoad;
   *pFraExtLeft  = rload.m_LeftExtBarrierFraction;
   *pFraIntLeft  = rload.m_LeftIntBarrierFraction;
   *pFraExtRight = rload.m_RightExtBarrierFraction;
   *pFraIntRight = rload.m_RightIntBarrierFraction;
}

Float64 CGirderModelManager::GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const
{
   GET_IFACE(IBarriers,pBarriers);

   if(!pBarriers->HasSidewalk(orientation))
   {
      return 0.0; 
   }
   else
   {
      Float64 intLoc, extLoc;
      pBarriers->GetSidewalkPedLoadEdges(orientation, &intLoc, &extLoc);
      Float64 swWidth  = intLoc - extLoc;
      ATLASSERT(swWidth>0.0); // should have checked somewhere if a sidewalk exists before calling

      GET_IFACE(ILibrary,pLibrary);
      GET_IFACE(ISpecification,pSpec);
      const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );

      Float64 Wmin = pSpecEntry->GetMinSidewalkWidth();

      if ( swWidth <= Wmin )
      {
         return 0.0; // not min sidewalk, no pedestrian load
      }

      Float64 w = pSpecEntry->GetPedestrianLiveLoad();
      Float64 W  = w*swWidth;

      return W;
   }
}

void CGirderModelManager::ApplyTrafficBarrierAndSidewalkLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr,bool bContinuousModel) const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   CComBSTR bstrStage(GetLBAMStageName(railingSystemIntervalIdx));

   CComBSTR bstrBarrierLoadGroup( GetLoadGroupName(pgsTypes::pftTrafficBarrier) ); 
   CComBSTR bstrSidewalkLoadGroup( GetLoadGroupName(pgsTypes::pftSidewalk) ); 

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);

   MemberIDType mbrID = 0;
   for(const auto& thisGirderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      PierIndexType prev_pier = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType next_pier = pGroup->GetPierIndex(pgsTypes::metEnd);

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);

         const CPierData2* pLeftPier = nullptr;
         const CTemporarySupportData* pLeftTS = nullptr;
         pSegment->GetSupport(pgsTypes::metStart,&pLeftPier,&pLeftTS);

         const CPierData2* pRightPier = nullptr;
         const CTemporarySupportData* pRightTS = nullptr;
         pSegment->GetSupport(pgsTypes::metEnd,&pRightPier,&pRightTS);

         Float64 left_diaphragm_width(0), right_diaphragm_width(0);
         if ( pLeftPier )
         {
            Float64 H;
            pBridge->GetPierDiaphragmSize(pLeftPier->GetIndex(),pgsTypes::Ahead,&left_diaphragm_width,&H);
         }

         if ( pRightPier )
         {
            Float64 H;
            pBridge->GetPierDiaphragmSize(pRightPier->GetIndex(),pgsTypes::Back,&right_diaphragm_width,&H);
         }

         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

         Float64 Wtb_per_girder, fraExtLeft, fraExtRight, fraIntLeft, fraIntRight;
         GetTrafficBarrierLoadFraction(segmentKey,&Wtb_per_girder,&fraExtLeft,&fraIntLeft,&fraExtRight,&fraIntRight);

         Float64 Wsw_per_girder, fraLeft, fraRight;
         GetSidewalkLoadFraction(segmentKey,&Wsw_per_girder,&fraLeft,&fraRight);

         std::vector<LinearLoad> tbLoads;
         tbLoads.emplace_back(-left_diaphragm_width, segment_length + right_diaphragm_width, Wtb_per_girder, Wtb_per_girder);

         std::vector<LinearLoad> swLoads;
         swLoads.emplace_back(-left_diaphragm_width, segment_length + right_diaphragm_width, Wsw_per_girder, Wsw_per_girder);

         MemberIDType newMbrID = ApplyDistributedLoadsToSegment(railingSystemIntervalIdx,pModel,analysisType,mbrID,segmentKey,tbLoads,bstrStage,bstrBarrierLoadGroup);
         ApplyDistributedLoadsToSegment(railingSystemIntervalIdx,pModel,analysisType,mbrID,segmentKey,swLoads,bstrStage,bstrSidewalkLoadGroup);

         // model the load at the closure joint
         if ( segIdx != nSegments-1 )
         {
            CComPtr<IDistributedLoad> tbClosureLoad;
            tbClosureLoad.CoCreateInstance(CLSID_DistributedLoad);
            tbClosureLoad->put_MemberType(mtSuperstructureMember);
            tbClosureLoad->put_MemberID(newMbrID);
            tbClosureLoad->put_Direction(ldFy);
            tbClosureLoad->put_WStart(Wtb_per_girder);
            tbClosureLoad->put_WEnd(Wtb_per_girder);
            tbClosureLoad->put_StartLocation(0.0);
            tbClosureLoad->put_EndLocation(-1.0);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distLoads->Add(bstrStage,bstrBarrierLoadGroup,tbClosureLoad,&distLoadItem);

            CComPtr<IDistributedLoad> swClosureLoad;
            swClosureLoad.CoCreateInstance(CLSID_DistributedLoad);
            swClosureLoad->put_MemberType(mtSuperstructureMember);
            swClosureLoad->put_MemberID(newMbrID);
            swClosureLoad->put_Direction(ldFy);
            swClosureLoad->put_WStart(Wsw_per_girder);
            swClosureLoad->put_WEnd(Wsw_per_girder);
            swClosureLoad->put_StartLocation(0.0);
            swClosureLoad->put_EndLocation(-1.0);

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrSidewalkLoadGroup,swClosureLoad,&distLoadItem);
         }

         // if we are at an intermediate pier with a continuous boundary condition
         // then we have to add the two LBAM superstructure members used model the pier connection region
         Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
         Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );
         if ( bContinuousModel && // continuous model (don't load across piers for simple span models)
              segIdx == nSegments-1 && // last segment in the girder
              pGroup->GetNextGirderGroup() != nullptr && // not the last group/span in the bridge
              pGroup->GetPier(pgsTypes::metEnd)->IsContinuousConnection() && // continuous boundary condition
              !IsZero(right_brg_offset - right_end_dist)
            )
         {
            CComPtr<IDistributedLoad> tbLoad1;
            tbLoad1.CoCreateInstance(CLSID_DistributedLoad);
            tbLoad1->put_MemberType(mtSuperstructureMember);
            tbLoad1->put_MemberID(newMbrID);
            tbLoad1->put_Direction(ldFy);
            tbLoad1->put_WStart(Wtb_per_girder);
            tbLoad1->put_WEnd(Wtb_per_girder);
            tbLoad1->put_StartLocation(0.0);
            tbLoad1->put_EndLocation(-1.0);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distLoads->Add(bstrStage,bstrBarrierLoadGroup,tbLoad1,&distLoadItem);

            CComPtr<IDistributedLoad> tbLoad2;
            tbLoad2.CoCreateInstance(CLSID_DistributedLoad);
            tbLoad2->put_MemberType(mtSuperstructureMember);
            tbLoad2->put_MemberID(newMbrID+1);
            tbLoad2->put_Direction(ldFy);
            tbLoad2->put_WStart(Wtb_per_girder);
            tbLoad2->put_WEnd(Wtb_per_girder);
            tbLoad2->put_StartLocation(0.0);
            tbLoad2->put_EndLocation(-1.0);

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrBarrierLoadGroup,tbLoad2,&distLoadItem);

            CComPtr<IDistributedLoad> swLoad1;
            swLoad1.CoCreateInstance(CLSID_DistributedLoad);
            swLoad1->put_MemberType(mtSuperstructureMember);
            swLoad1->put_MemberID(newMbrID);
            swLoad1->put_Direction(ldFy);
            swLoad1->put_WStart(Wsw_per_girder);
            swLoad1->put_WEnd(Wsw_per_girder);
            swLoad1->put_StartLocation(0.0);
            swLoad1->put_EndLocation(-1.0);

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrSidewalkLoadGroup,swLoad1,&distLoadItem);

            CComPtr<IDistributedLoad> swLoad2;
            swLoad2.CoCreateInstance(CLSID_DistributedLoad);
            swLoad2->put_MemberType(mtSuperstructureMember);
            swLoad2->put_MemberID(newMbrID+1);
            swLoad2->put_Direction(ldFy);
            swLoad2->put_WStart(Wsw_per_girder);
            swLoad2->put_WEnd(Wsw_per_girder);
            swLoad2->put_StartLocation(0.0);
            swLoad2->put_EndLocation(-1.0);

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrSidewalkLoadGroup,swLoad2,&distLoadItem);
         }

         mbrID = newMbrID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier && !pPier->IsAbutment() )
         {
            IndexType nSSMbrs = GetSuperstructureMemberCount(pPier,thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
         else if ( pTS )
         {
            ATLASSERT(pTS);
            IndexType nSSMbrs = GetSuperstructureMemberCount(pTS,thisGirderKey.girderIndex);
            mbrID += nSSMbrs;
         }
      } // next segment
   } // next group
}

void CGirderModelManager::ComputeSidewalksBarriersLoadFractions() const
{
   // return if we've already done the work
   if (!m_SidewalkTrafficBarrierLoads.empty())
   {
      return;
   }

   GET_IFACE( IBridge,        pBridge );
   GET_IFACE( IBarriers,      pBarriers);
   GET_IFACE( ISpecification, pSpec );

   // Determine weight of barriers and sidwalks
   Float64 WtbExtLeft(0.0),  WtbIntLeft(0.0),  WswLeft(0.0);
   Float64 WtbExtRight(0.0), WtbIntRight(0.0), WswRight(0.0);

   WtbExtLeft  = pBarriers->GetExteriorBarrierWeight(pgsTypes::tboLeft);
   if (pBarriers->HasInteriorBarrier(pgsTypes::tboLeft))
   {
      WtbIntLeft  = pBarriers->GetInteriorBarrierWeight(pgsTypes::tboLeft);
   }


   if (pBarriers->HasSidewalk(pgsTypes::tboLeft))
   {
      WswLeft  = pBarriers->GetSidewalkWeight(pgsTypes::tboLeft);
   }

   WtbExtRight = pBarriers->GetExteriorBarrierWeight(pgsTypes::tboRight);
   if (pBarriers->HasInteriorBarrier(pgsTypes::tboRight))
   {
      WtbIntRight  += pBarriers->GetInteriorBarrierWeight(pgsTypes::tboRight);
   }

   if (pBarriers->HasSidewalk(pgsTypes::tboRight))
   {
      WswRight  = pBarriers->GetSidewalkWeight(pgsTypes::tboRight);
   }

   GirderIndexType nMaxDist; // max number of girders/webs/mating surfaces to distribute load to
   pgsTypes::TrafficBarrierDistribution distType; // the distribution type

   pSpec->GetTrafficBarrierDistribution(&nMaxDist,&distType);


   // pgsBarrierSidewalkLoadDistributionTool does the heavy lifting to determine how 
   // sidewalks and barriers are distributed to each girder
   GET_IFACE_NOCHECK( IBridgeDescription, pIBridgeDesc);
   GET_IFACE_NOCHECK( IGirder,      pGdr);
   pgsBarrierSidewalkLoadDistributionTool BSwTool(LOGGER, pIBridgeDesc, pBridge, pGdr, pBarriers);

   // Loop over all segments in bridge and compute load fractions
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx, gdrIdx));
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            // initialize tool to compute loads across all girders in this group
            // for the current segment.
            BSwTool.Initialize(grpIdx, segIdx, distType, nMaxDist);

            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            SidewalkTrafficBarrierLoad stbLoad;

            // compute barrier first
            Float64 FraExtLeft, FraIntLeft, FraExtRight, FraIntRight;
            BSwTool.GetTrafficBarrierLoadFraction(gdrIdx, &FraExtLeft, &FraIntLeft, &FraExtRight, &FraIntRight);

            stbLoad.m_BarrierLoad = -1.0 *(WtbExtLeft*FraExtLeft + WtbIntLeft*FraIntLeft + WtbExtRight*FraExtRight + WtbIntRight*FraIntRight);
            stbLoad.m_LeftExtBarrierFraction = FraExtLeft;
            stbLoad.m_LeftIntBarrierFraction = FraIntLeft;
            stbLoad.m_RightExtBarrierFraction = FraExtRight;
            stbLoad.m_RightIntBarrierFraction = FraIntRight;

            // sidewalks
            Float64 FraSwLeft, FraSwRight;
            BSwTool.GetSidewalkLoadFraction(gdrIdx, &FraSwLeft, &FraSwRight);
            stbLoad.m_SidewalkLoad = -1.0 * (WswLeft*FraSwLeft + WswRight*FraSwRight);
            stbLoad.m_LeftSidewalkFraction = FraSwLeft;
            stbLoad.m_RightSidewalkFraction = FraSwRight;

            // store loads for this segment for later use
            m_SidewalkTrafficBarrierLoads.insert(std::make_pair(segmentKey, stbLoad));
         }
      }
   }
}

void CGirderModelManager::ApplyLiveLoadModel(ILBAMModel* pModel,GirderIndexType gdrIdx) const
{
   HRESULT hr = S_OK;
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(ILibrary,pLibrary);

   // get the live load object from the model
   CComPtr<ILiveLoad> live_load;
   pModel->get_LiveLoad(&live_load);

   // get the live load models
   CComPtr<ILiveLoadModel> design_liveload_model;
   live_load->get_Design(&design_liveload_model);

   CComPtr<ILiveLoadModel> permit_liveload_model;
   live_load->get_Permit(&permit_liveload_model);

   CComPtr<ILiveLoadModel> pedestrian_liveload_model;
   live_load->get_Pedestrian(&pedestrian_liveload_model);

   CComPtr<ILiveLoadModel> fatigue_liveload_model;
   live_load->get_Fatigue(&fatigue_liveload_model);

   CComPtr<ILiveLoadModel> legal_routine_liveload_model;
   live_load->get_LegalRoutineRating(&legal_routine_liveload_model);

   CComPtr<ILiveLoadModel> legal_special_liveload_model;
   live_load->get_LegalSpecialRating(&legal_special_liveload_model);

   CComPtr<ILiveLoadModel> legal_emergency_liveload_model;
   live_load->get_LegalEmergencyRating(&legal_emergency_liveload_model);

   CComPtr<ILiveLoadModel> permit_routine_liveload_model;
   live_load->get_PermitRoutineRating(&permit_routine_liveload_model);

   CComPtr<ILiveLoadModel> permit_special_liveload_model;
   live_load->get_PermitSpecialRating(&permit_special_liveload_model);

   // get the vehicular loads collections
   CComPtr<IVehicularLoads> design_vehicles;
   design_liveload_model->get_VehicularLoads(&design_vehicles);

   CComPtr<IVehicularLoads> permit_vehicles;
   permit_liveload_model->get_VehicularLoads(&permit_vehicles);

   CComPtr<IVehicularLoads> pedestrian_vehicles;
   pedestrian_liveload_model->get_VehicularLoads(&pedestrian_vehicles);

   CComPtr<IVehicularLoads> fatigue_vehicles;
   fatigue_liveload_model->get_VehicularLoads(&fatigue_vehicles);

   CComPtr<IVehicularLoads> legal_routine_vehicles;
   legal_routine_liveload_model->get_VehicularLoads(&legal_routine_vehicles);

   CComPtr<IVehicularLoads> legal_special_vehicles;
   legal_special_liveload_model->get_VehicularLoads(&legal_special_vehicles);

   CComPtr<IVehicularLoads> legal_emergency_vehicles;
   legal_emergency_liveload_model->get_VehicularLoads(&legal_emergency_vehicles);

   CComPtr<IVehicularLoads> permit_routine_vehicles;
   permit_routine_liveload_model->get_VehicularLoads(&permit_routine_vehicles);

   CComPtr<IVehicularLoads> permit_special_vehicles;
   permit_special_liveload_model->get_VehicularLoads(&permit_special_vehicles);

   // get the live load names
   std::vector<std::_tstring> design_loads         = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring> permit_loads         = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);
   std::vector<std::_tstring> fatigue_loads        = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::_tstring> routine_legal_loads  = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   std::vector<std::_tstring> special_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   std::vector<std::_tstring> emergency_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Emergency);
   std::vector<std::_tstring> routine_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   std::vector<std::_tstring> special_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

   // add the live loads to the models
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltDesign,              design_loads,          pLibrary, pLiveLoads, design_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltPermit,              permit_loads,          pLibrary, pLiveLoads, permit_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltFatigue,             fatigue_loads,         pLibrary, pLiveLoads, fatigue_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltLegalRating_Routine, routine_legal_loads,   pLibrary, pLiveLoads, legal_routine_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltLegalRating_Special, special_legal_loads, pLibrary, pLiveLoads, legal_special_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltLegalRating_Emergency, emergency_legal_loads, pLibrary, pLiveLoads, legal_emergency_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltPermitRating_Routine,routine_permit_loads,  pLibrary, pLiveLoads, permit_routine_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltPermitRating_Special,special_permit_loads,  pLibrary, pLiveLoads, permit_special_vehicles);

   // Add pedestrian load if applicable
   if ( pProductLoads->HasPedestrianLoad() )
   {
      // Pedestrian load can vary per span. Use unit load here and adjust magnitude of distribution factors
      // to control load.
      Float64 wPedLL = 1.0;
      AddPedestrianLoad(_T("Pedestrian on Sidewalk"),wPedLL,pedestrian_vehicles);
   }

   // The call to AddUserLiveLoads above changes the distribution factor type to default values
   // set by the LBAMUtility object that gets used to configure live load. Set the desired distribution
   // factor type here
   design_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltDesign));
   permit_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltPermit));
   pedestrian_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltPedestrian));
   fatigue_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltFatigue));
   legal_routine_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltLegalRating_Routine));
   legal_special_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltLegalRating_Special));
   legal_emergency_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltLegalRating_Emergency));
   permit_routine_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltPermitRating_Routine));
   permit_special_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltPermitRating_Special));
}

void CGirderModelManager::AddUserLiveLoads(ILBAMModel* pModel,GirderIndexType gdrIdx,pgsTypes::LiveLoadType llType,std::vector<std::_tstring>& strLLNames,
                                         ILibrary* pLibrary, ILiveLoads* pLiveLoads, IVehicularLoads* pVehicles) const
{
   HRESULT hr = S_OK;

   if ( strLLNames.size() == 0 )
   {
      // if there aren't any live loads, then added a dummy place holder load
      // so the LBAM doesn't crash when requesting live load results
      AddDummyLiveLoad(pVehicles);
      return;
   }

   Float64 truck_impact = 1.0 + pLiveLoads->GetTruckImpact(llType);
   Float64 lane_impact  = 1.0 + pLiveLoads->GetLaneImpact(llType);

   if ( llType == pgsTypes::lltDesign )
   {
      AddDeflectionLiveLoad(pModel,pLibrary,truck_impact,lane_impact);
   }

   for(const auto& strLLName : strLLNames)
   {
      if ( strLLName == std::_tstring(_T("HL-93")) )
      {
         AddHL93LiveLoad(pModel,pLibrary,llType,truck_impact,lane_impact);
      }
      else if ( strLLName == std::_tstring(_T("Fatigue")) )
      {
         AddFatigueLiveLoad(pModel,pLibrary,llType,truck_impact,lane_impact);
      }
      else if ( strLLName == std::_tstring(_T("AASHTO Legal Loads")) )
      {
         AddLegalLiveLoad(pModel,pLibrary,llType,truck_impact,lane_impact);
      }
      else if ( strLLName == std::_tstring(_T("Notional Rating Load (NRL)")) )
      {
         AddNotionalRatingLoad(pModel,pLibrary,llType,truck_impact,lane_impact);
      }
      else if (strLLName == std::_tstring(_T("Emergency Vehicles")))
      {
         AddEmergencyLiveLoad(pModel, pLibrary, llType, truck_impact, lane_impact);
      }
      else if ( strLLName == std::_tstring(_T("Single-Unit SHVs")) )
      {
         AddSHVLoad(pModel,pLibrary,llType,truck_impact,lane_impact);
      }
      else
      {
         AddUserTruck(strLLName,pLibrary,truck_impact,lane_impact,pVehicles);
      }
   }
}

void CGirderModelManager::AddHL93LiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const
{
   GET_IFACE(ISpecification,pSpec);

   // this is an HL-93 live load, use the LBAM configuration utility
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );
   SpecUnitType units = pSpecEntry->GetSpecificationUnits() == lrfdVersionMgr::US ? suUS : suSI;

   ATLASSERT( llType != pgsTypes::lltPedestrian ); // we don't want to add HL-93 to the pedestrian live load model
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   VARIANT_BOOL bUseDualTruckTrains = (1 < nSpans ? VARIANT_TRUE : VARIANT_FALSE); // always use dual truck trains if more than one span (needed for reactions at intermediate piers)
   VARIANT_BOOL bIncludeDualTandem = (nSpans == 1 ? VARIANT_FALSE : (pSpecEntry->IncludeDualTandem() ? VARIANT_TRUE : VARIANT_FALSE));
   m_LBAMUtility->ConfigureDesignLiveLoad(pModel,llmt,IMtruck,IMlane,bUseDualTruckTrains, bIncludeDualTandem,units,m_UnitServer);
}

void CGirderModelManager::AddFatigueLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const
{
   GET_IFACE(ISpecification,pSpec);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );
   SpecUnitType units = pSpecEntry->GetSpecificationUnits() == lrfdVersionMgr::US ? suUS : suSI;

   m_LBAMUtility->ConfigureFatigueLiveLoad(pModel,llmt,IMtruck,IMlane,units,m_UnitServer);
}

void CGirderModelManager::AddDeflectionLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane) const
{
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );
   SpecUnitType units = pSpecEntry->GetSpecificationUnits() == lrfdVersionMgr::US ? suUS : suSI;

   m_LBAMUtility->ConfigureDeflectionLiveLoad(pModel,lltDeflection,IMtruck,IMlane,units,m_UnitServer);
}

void CGirderModelManager::AddLegalLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const
{
   // this is an AASHTO Legal Load for rating, use the LBAM configuration utility
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   VARIANT_BOOL bIncludeType33     = VARIANT_FALSE; // exclude 0.75(Type 3-3) + Lane unless span length is 200ft or more
   VARIANT_BOOL bIncludeDualType33 = VARIANT_FALSE; // exclude dual 0.75(Type 3-3) + Lane unless negative moments need to be processed
   VARIANT_BOOL bRemoveLaneLoad    = VARIANT_FALSE; // don't remove lane load unless directed by the engineer

   bool bOver200 = false;
   Float64 L = WBFL::Units::ConvertToSysUnits(200.0,WBFL::Units::Measure::Feet);
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      Float64 span_length = pBridge->GetSpanLength(spanIdx);
      if ( L < span_length )
      {
         bOver200 = true;
      }

      if ( pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bIncludeDualType33 = VARIANT_TRUE;
      }
   }

   if ( bOver200 )
   {
      bIncludeType33 = VARIANT_TRUE;
   }

   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->ExcludeLegalLoadLaneLoading() )
   {
      bRemoveLaneLoad = VARIANT_TRUE;
   }

   m_LBAMUtility->ConfigureLegalLiveLoad(pModel,llmt,IMtruck,IMlane,bIncludeType33,bIncludeDualType33,bRemoveLaneLoad,m_UnitServer);
}

void CGirderModelManager::AddNotionalRatingLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const
{
   // this is an AASHTO Legal Load for rating, use the LBAM configuration utility
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   m_LBAMUtility->ConfigureNotionalRatingLoad(pModel,llmt,IMtruck,IMlane,m_UnitServer);
}

void CGirderModelManager::AddEmergencyLiveLoad(ILBAMModel* pModel, ILibrary* pLibrary, pgsTypes::LiveLoadType llType, Float64 IMtruck, Float64 IMlane) const
{
   // this is an AASHTO Legal Load for emergency vehicle rating, use the LBAM configuration utility

   // need to included a 200 plf lane load if spans are greater than 200 ft or if we have continuous spans
   VARIANT_BOOL bIncludeLaneLoad = VARIANT_FALSE;
   Float64 L = WBFL::Units::ConvertToSysUnits(200.0, WBFL::Units::Measure::Feet);
   GET_IFACE(IBridge, pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      Float64 span_length = pBridge->GetSpanLength(spanIdx);
      if (L < span_length || pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bIncludeLaneLoad = VARIANT_TRUE;
      }
   }


   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   m_LBAMUtility->ConfigureEmergencyRatingLoad(pModel, llmt, IMtruck, IMlane, bIncludeLaneLoad, m_UnitServer);
}

void CGirderModelManager::AddSHVLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const
{
   // this is an AASHTO Legal Load for rating, use the LBAM configuration utility
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   m_LBAMUtility->ConfigureSpecializedHaulingUnits(pModel,llmt,IMtruck,IMlane,m_UnitServer);
}

void CGirderModelManager::AddUserTruck(const std::_tstring& strLLName,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane,IVehicularLoads* pVehicles) const
{
   // this is a user defined vehicular live load defined in the library
   const LiveLoadLibraryEntry* ll_entry = pLibrary->GetLiveLoadEntry( strLLName.c_str());
   ATLASSERT(ll_entry!=nullptr);

   CComPtr<IVehicularLoad> vehicular_load; 
   vehicular_load.CoCreateInstance(CLSID_VehicularLoad);

   ATLASSERT( strLLName == ll_entry->GetName() );

   vehicular_load->put_Name(CComBSTR(strLLName.c_str()));
   vehicular_load->put_Applicability(ConvertLiveLoadApplicabilityType(ll_entry->GetLiveLoadApplicabilityType()));

   LiveLoadLibraryEntry::LiveLoadConfigurationType ll_config = ll_entry->GetLiveLoadConfigurationType();
   VehicularLoadConfigurationType lb_ll_config = vlcDefault;
   if (ll_config == LiveLoadLibraryEntry::lcTruckOnly)
   {
      lb_ll_config = vlcTruckOnly;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcLaneOnly)
   {
      lb_ll_config = vlcLaneOnly;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcTruckPlusLane)
   {
      lb_ll_config = vlcTruckPlusLane;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      lb_ll_config = vlcTruckLaneEnvelope;
   }
   else
   {
      ATLASSERT(false);
   }

   vehicular_load->put_Configuration(lb_ll_config);
   VARIANT_BOOL is_notional =  ll_entry->GetIsNotional() ? VARIANT_TRUE : VARIANT_FALSE;

   vehicular_load->put_UseNotional(is_notional);

   // loads are unfactored
   vehicular_load->put_IMLane(IMlane);
   vehicular_load->put_IMTruck(IMtruck);
   vehicular_load->put_LaneFactor(1.0);
   vehicular_load->put_TruckFactor(1.0);


   Float64 lane_load = 0;
   if ( ll_config != LiveLoadLibraryEntry::lcTruckOnly )
   {
      lane_load = ll_entry->GetLaneLoad();
   }

   // only add the lane load if a span length is long enough
   Float64 lane_load_span_length = ll_entry->GetLaneLoadSpanLength();

   bool bIsOver = false;
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      Float64 span_length = pBridge->GetSpanLength(spanIdx);
      if ( lane_load_span_length < span_length )
      {
         bIsOver = true;
      }
   }

   if ( bIsOver )
   {
      vehicular_load->put_LaneLoad(lane_load);
   }

   if ( ll_config != LiveLoadLibraryEntry::lcLaneOnly )
   {
      CComPtr<IAxles> axles;
      vehicular_load->get_Axles(&axles);

      AxleIndexType num_axles = ll_entry->GetNumAxles();
      for (AxleIndexType iaxl=0; iaxl<num_axles; iaxl++)
      {
         CComPtr<IAxle> lb_axle;
         lb_axle.CoCreateInstance(CLSID_Axle);

         LiveLoadLibraryEntry::Axle axle = ll_entry->GetAxle(iaxl);
         lb_axle->put_Weight( axle.Weight );

         // spacing for lbam axles is behind axle, for library entries it is in front of axle
         Float64 spacing(0.0);
         if (iaxl<num_axles-1)
         {
            LiveLoadLibraryEntry::Axle next_axle = ll_entry->GetAxle(iaxl+1);
            spacing = next_axle.Spacing;
         }

         lb_axle->put_Spacing(spacing);
         axles->Add(lb_axle);
      }

      AxleIndexType var_idx = ll_entry->GetVariableAxleIndex();
      if (var_idx != FIXED_AXLE_TRUCK)
      {
         vehicular_load->put_VariableAxle(var_idx-1);
         Float64 max_spac = ll_entry->GetMaxVariableAxleSpacing();
         vehicular_load->put_VariableMaxSpacing(max_spac);
      }
   }

   pVehicles->Add(vehicular_load);
}

void CGirderModelManager::AddPedestrianLoad(const std::_tstring& strLLName,Float64 wPedLL,IVehicularLoads* pVehicles) const
{
   CComPtr<IVehicularLoad> vehicular_load; 
   vehicular_load.CoCreateInstance(CLSID_VehicularLoad);

   vehicular_load->put_Name(CComBSTR(strLLName.c_str()));
   vehicular_load->put_Applicability(llaEntireStructure);
   vehicular_load->put_Configuration(vlcSidewalkOnly);
   vehicular_load->put_UseNotional(VARIANT_TRUE);
   vehicular_load->put_SidewalkLoad(wPedLL); // PL per girder

   pVehicles->Add(vehicular_load);
}

void CGirderModelManager::AddDummyLiveLoad(IVehicularLoads* pVehicles) const
{
   // this is a dummy, zero weight truck that is used when there is no live load defined
   CComPtr<IVehicularLoad> vehicular_load; 
   vehicular_load.CoCreateInstance(CLSID_VehicularLoad);

   vehicular_load->put_Name(CComBSTR("No Live Load Defined"));
   vehicular_load->put_Applicability(llaEntireStructure);
   vehicular_load->put_Configuration(vlcLaneOnly);
   vehicular_load->put_UseNotional(VARIANT_FALSE);

   // loads are unfactored
   vehicular_load->put_IMLane(1.0);
   vehicular_load->put_IMTruck(1.0);
   vehicular_load->put_LaneFactor(1.0);
   vehicular_load->put_TruckFactor(1.0);

   vehicular_load->put_LaneLoad(0.0);

   pVehicles->Add(vehicular_load);
}

DistributionFactorType CGirderModelManager::GetLiveLoadDistributionFactorType(pgsTypes::LiveLoadType llType) const
{
   DistributionFactorType dfType;
   switch (llType )
   {
   case pgsTypes::lltDesign:
      dfType = dftEnvelope;
      break;

   case pgsTypes::lltFatigue:
      dfType = dftFatigue;
      break;

   case pgsTypes::lltPedestrian:
      dfType = dftPedestrian;
      break;

   case pgsTypes::lltPermit:
      dfType = dftEnvelope;
      break;

   case pgsTypes::lltLegalRating_Routine:
      dfType = dftEnvelope;
      break;

   case pgsTypes::lltLegalRating_Special:
      dfType = dftEnvelope;
      break;

   case pgsTypes::lltLegalRating_Emergency:
      dfType = dftEnvelope; // see Q/A #21 & 22 https://www.fhwa.dot.gov/bridge/loadrating/fast1410_qa.pdf
      break;

   case pgsTypes::lltPermitRating_Routine:
     dfType = dftEnvelope;
     break;

   case pgsTypes::lltPermitRating_Special:
     dfType = dftFatigue; // single lane with muliple presense divided out
     break;

   default:
      ATLASSERT(false); // should never get here
      dfType = dftEnvelope;
   }

   return dfType;
}

void CGirderModelManager::GetLiveLoadModel(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,ILiveLoadModel** ppLiveLoadModel) const
{
   CGirderKey key(girderKey);

   if ( key.groupIndex == ALL_GROUPS )
   {
      key.groupIndex = 0;
   }

   if ( key.girderIndex == ALL_GIRDERS )
   {
      key.girderIndex = 0;
   }

   CGirderModelData* pModelData = GetGirderModel(key.girderIndex);
   ATLASSERT(pModelData->m_Model != nullptr || pModelData->m_ContinuousModel != nullptr);

   CComPtr<ILiveLoad> live_load;
   if ( pModelData->m_Model )
   {
      pModelData->m_Model->get_LiveLoad(&live_load);
   }
   else
   {
      pModelData->m_ContinuousModel->get_LiveLoad(&live_load);
   }

   // get the design and permit live load models
   switch(llType)
   {
      case pgsTypes::lltDesign:
         live_load->get_Design(ppLiveLoadModel);
         break;

      case pgsTypes::lltFatigue:
         live_load->get_Fatigue(ppLiveLoadModel);
         break;

      case pgsTypes::lltPermit:
         live_load->get_Permit(ppLiveLoadModel);
         break;

      case pgsTypes::lltPedestrian:
         live_load->get_Pedestrian(ppLiveLoadModel);
         break;

      case pgsTypes::lltLegalRating_Routine:
         live_load->get_LegalRoutineRating(ppLiveLoadModel);
         break;

      case pgsTypes::lltLegalRating_Special:
         live_load->get_LegalSpecialRating(ppLiveLoadModel);
         break;

      case pgsTypes::lltLegalRating_Emergency:
         live_load->get_LegalEmergencyRating(ppLiveLoadModel);
         break;

      case pgsTypes::lltPermitRating_Routine:
         live_load->get_PermitRoutineRating(ppLiveLoadModel);
         break;

      case pgsTypes::lltPermitRating_Special:
         live_load->get_PermitSpecialRating(ppLiveLoadModel);
         break;

      default:
         ATLASSERT(false);
   }
}

std::vector<std::_tstring> CGirderModelManager::GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey) const
{
   CComPtr<ILiveLoadModel> liveload_model;
   GetLiveLoadModel(llType,girderKey,&liveload_model);

   CComPtr<IVehicularLoads> vehicles;
   liveload_model->get_VehicularLoads(&vehicles);

   CComPtr<IEnumVehicularLoad> enum_vehicles;
   vehicles->get__EnumElements(&enum_vehicles);

   std::vector<std::_tstring> names;

   CComPtr<IVehicularLoad> vehicle;
   while ( enum_vehicles->Next(1,&vehicle,nullptr) != S_FALSE )
   {
      CComBSTR bstrName;
      vehicle->get_Name(&bstrName);

      names.emplace_back(OLE2T(bstrName));

      vehicle.Release();
   }

   return names;
}

void CGirderModelManager::ApplyUserDefinedLoads(ILBAMModel* pModel,GirderIndexType gdr) const
{
   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GET_IFACE(IUserDefinedLoads, pUdls);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      GirderIndexType nGirdersInGroup = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirdersInGroup-1);

      CSpanKey spanKey(spanIdx,gdrIdx);

      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(CGirderKey(pGroup->GetIndex(),gdrIdx));

      for ( IntervalIndexType intervalIdx = erectionIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
      {
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );


         //
         // point loads
         //
         const std::vector<IUserDefinedLoads::UserPointLoad>* pPointLoads = pUdls->GetPointLoads(intervalIdx,spanKey);

         if (pPointLoads != nullptr)
         {
            IndexType nls = pPointLoads->size();
            for(IndexType ild=0; ild<nls; ild++)
            {
               const IUserDefinedLoads::UserPointLoad& load = pPointLoads->at(ild);

               CComPtr<IPointLoad> load2;
               load2.CoCreateInstance(CLSID_PointLoad);
               load2->put_Location(load.m_Location);
               load2->put_Fy(-1.0*load.m_Magnitude);
               load2->put_Mz(0.0);

               if ( load.m_bLoadOnStartCantilever )
               {
                  CSegmentKey segmentKey(0,spanKey.girderIndex,0);
                  GirderIDType ssmbrID = GetFirstSuperstructureMemberID(segmentKey);
                  load2->put_MemberType(mtSuperstructureMember);
                  load2->put_MemberID(ssmbrID);
               }
               else if ( load.m_bLoadOnEndCantilever )
               {
                  GroupIndexType grpIdx = pGroup->GetIndex();
                  SegmentIndexType nSegments = pGroup->GetGirder(spanKey.girderIndex)->GetSegmentCount();
                  CSegmentKey segmentKey(grpIdx,spanKey.girderIndex,nSegments-1);
                  GirderIDType ssmbrID = GetFirstSuperstructureMemberID(segmentKey);
                  IndexType nSSMbrs = GetSuperstructureMemberCount(segmentKey);
                  ssmbrID += nSSMbrs-1;
                  load2->put_MemberType(mtSuperstructureMember);
                  load2->put_MemberID(ssmbrID);
               }
               else
               {
                  load2->put_MemberType(mtSpan);
                  load2->put_MemberID(spanIdx);
               }

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&ptLoadItem);
            }
         }

         //
         // distributed loads
         //
         const std::vector<IUserDefinedLoads::UserDistributedLoad>* pDistLoads = pUdls->GetDistributedLoads(intervalIdx,spanKey);

         if (pDistLoads!=nullptr)
         {
            IndexType nls = pDistLoads->size();
            for(IndexType ild=0; ild<nls; ild++)
            {
               const IUserDefinedLoads::UserDistributedLoad& load = pDistLoads->at(ild);

               CComPtr<IDistributedLoad> load2;
               load2.CoCreateInstance(CLSID_DistributedLoad);
               load2->put_MemberType(mtSpan);
               load2->put_MemberID(spanIdx);
               load2->put_StartLocation(load.m_StartLocation);
               load2->put_EndLocation(load.m_EndLocation);
               load2->put_WStart(-1.0*load.m_WStart);
               load2->put_WEnd(-1.0*load.m_WEnd);
               load2->put_Direction(ldFy);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&distLoadItem);
            }
         }

         //
         // moment loads
         //
         const std::vector<IUserDefinedLoads::UserMomentLoad>* pMomentLoads = pUdls->GetMomentLoads(intervalIdx,spanKey);

         if (pMomentLoads != nullptr)
         {
            IndexType nLoads = pMomentLoads->size();
            for(IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++)
            {
               const IUserDefinedLoads::UserMomentLoad& load = pMomentLoads->at(loadIdx);

               CComPtr<IPointLoad> load2;
               load2.CoCreateInstance(CLSID_PointLoad);
               load2->put_MemberType(mtSpan);
               load2->put_MemberID(spanIdx);
               load2->put_Location(load.m_Location);
               load2->put_Fy(0.0);
               load2->put_Mz(load.m_Magnitude);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&ptLoadItem);
            }
         }

      } // span loop
   } // interval loop
}

void CGirderModelManager::ApplyPostTensionDeformation(ILBAMModel* pModel,GirderIndexType gdrLineIdx) const
{
   // Models the post-tensioning load as curvatures. Curvature is M/EI, or in this case Pe/EI.
   // The results of the structural analysis are the secondary forces and the
   // post-tensioning deformations

   // NOTE: The deformations caused by segment tendons (plant installed) don't cause secondary effects because precast segments are
   // unrestrained at the time of tendon installation

   GET_IFACE_NOCHECK(IIntervals,pIntervals); // only used if there are tendons

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   CComPtr<IDistributedLoads> distributedLoads;
   pModel->get_DistributedLoads(&distributedLoads);

   CComPtr<IStrainLoads> strainLoads;
   pModel->get_StrainLoads(&strainLoads);

   CComPtr<ISuperstructureMembers> ssmbrs;
   pModel->get_SuperstructureMembers(&ssmbrs);

   CComBSTR bstrLoadGroup(GetLoadGroupName(pgsTypes::pftSecondaryEffects));

   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);

   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdrLineIdx, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      DuctIndexType nDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType stressTendonInterval = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);
         CComBSTR bstrStage(GetLBAMStageName(stressTendonInterval));

         std::vector<PostTensionStrainLoad> strLoads;
         GetPostTensionDeformationLoads(girderKey,ductIdx,strLoads);

         for ( const auto& load : strLoads )
         {
            MemberIDType startSSMbrID, endSSMbrID;
            Float64 startSSMbrLoc, endSSMbrLoc;
            pModel->ConvertSpanToSuperstructureLocation(load.startSpanIdx,load.Xstart,&startSSMbrID,&startSSMbrLoc);
            pModel->ConvertSpanToSuperstructureLocation(load.endSpanIdx,  load.Xend,  &endSSMbrID,  &endSSMbrLoc);

            if ( startSSMbrID == endSSMbrID )
            {
               // the load is contained within one LBAM SSMBR
               if (!IsEqual(startSSMbrLoc, endSSMbrLoc)) // don't apply the load if it doesn't cover any distance
               {
                  CComPtr<ISuperstructureMember> ssmbr;
                  ssmbrs->get_Item(startSSMbrID, &ssmbr);
                  Float64 ssmbrLength;
                  ssmbr->get_Length(&ssmbrLength);

                  CComPtr<IStrainLoad> strainLoad;
                  strainLoad.CoCreateInstance(CLSID_StrainLoad);
                  strainLoad->put_MemberType(mtSuperstructureMember);
                  strainLoad->put_MemberID(startSSMbrID);
                  strainLoad->put_StartLocation(startSSMbrLoc);
                  strainLoad->put_EndLocation(endSSMbrLoc);

                  // right now, the LBAM model only supports a constant strain... just use the average value
                  // the POI are closely spaced so this is a good approximate. The same thing is done
                  // in the Time-Step stress analysis when analyzing initial strains
                  Float64 e = 0.5*(load.eStart + load.eEnd);
                  strainLoad->put_AxialStrain(e);
                  Float64 r = 0.5*(load.rStart + load.rEnd);
                  strainLoad->put_CurvatureStrain(r);

                  CComPtr<IStrainLoadItem> strainLoadItem;
                  strainLoads->Add(bstrStage, bstrLoadGroup, strainLoad, &strainLoadItem);
               }
            }
            else
            {
               // the load spans multiple LBAM SSMBRs

               // First SSMBR
               CComPtr<ISuperstructureMember> ssmbr;
               ssmbrs->get_Item(startSSMbrID,&ssmbr);
               Float64 ssmbrLength;
               ssmbr->get_Length(&ssmbrLength);

               CComPtr<IStrainLoad> startStrainLoad;
               startStrainLoad.CoCreateInstance(CLSID_StrainLoad);
               startStrainLoad->put_MemberType(mtSuperstructureMember);
               startStrainLoad->put_MemberID(startSSMbrID);
               startStrainLoad->put_StartLocation(startSSMbrLoc);
               startStrainLoad->put_EndLocation(ssmbrLength);

               // right now, our loading only supports a constant strain... just use the average value
               Float64 e = 0.5*(load.eStart + load.eEnd);
               startStrainLoad->put_AxialStrain(e);
               Float64 r = 0.5*(load.rStart + load.rEnd);
               startStrainLoad->put_CurvatureStrain(r);
               
               CComPtr<IStrainLoadItem> strainLoadItem;
               strainLoads->Add(bstrStage,bstrLoadGroup,startStrainLoad,&strainLoadItem);

               // intermediate ssmbrs
               for ( MemberIDType ssmbrID = startSSMbrID+1; ssmbrID < endSSMbrID; ssmbrID++ )
               {
                  ssmbr.Release();
                  ssmbrs->get_Item(ssmbrID,&ssmbr);
                  ssmbr->get_Length(&ssmbrLength);

                  CComPtr<IStrainLoad> strainLoad;
                  strainLoad.CoCreateInstance(CLSID_StrainLoad);
                  strainLoad->put_MemberType(mtSuperstructureMember);
                  strainLoad->put_MemberID(ssmbrID);
                  strainLoad->put_StartLocation(0.0);
                  strainLoad->put_EndLocation(ssmbrLength);

                  // right now, our loading only supports a constant strain... just use the average value
                  e = 0.5*(load.eStart + load.eEnd);
                  strainLoad->put_AxialStrain(e);
                  r = 0.5*(load.rStart + load.rEnd);
                  strainLoad->put_CurvatureStrain(r);
               
                  strainLoadItem.Release();
                  strainLoads->Add(bstrStage,bstrLoadGroup,strainLoad,&strainLoadItem);
               } // next ssmbrID

               // Last SSMBR
               CComPtr<IStrainLoad> endStrainLoad;
               endStrainLoad.CoCreateInstance(CLSID_StrainLoad);
               endStrainLoad->put_MemberType(mtSuperstructureMember);
               endStrainLoad->put_MemberID(endSSMbrID);
               endStrainLoad->put_StartLocation(0.0);
               endStrainLoad->put_EndLocation(endSSMbrLoc);

               // right now, our loading only supports a constant strain... just use the average value
               e = 0.5*(load.eStart + load.eEnd);
               endStrainLoad->put_AxialStrain(e);
               r = 0.5*(load.rStart + load.rEnd);
               endStrainLoad->put_CurvatureStrain(r);

               strainLoadItem.Release();
               strainLoads->Add(bstrStage,bstrLoadGroup,endStrainLoad,&strainLoadItem);
            } // end of if-else if for ssmbr ID
         } // next curvature load
      } // next duct
   } // next group
}

Float64 CGirderModelManager::GetPedestrianLiveLoad(const CSpanKey& spanKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();

   // We don't know which segment we want the ped live load for... there is not enough
   // data. Assume segment 0
   CSegmentKey segmentKey(grpIdx,spanKey.girderIndex,0);

   return GetPedestrianLiveLoad(segmentKey);
}

Float64 CGirderModelManager::GetPedestrianLiveLoad(const CSegmentKey& segmentKey) const
{
   Float64 Wleft  = GetPedestrianLoadPerSidewalk(pgsTypes::tboLeft);
   Float64 Wright = GetPedestrianLoadPerSidewalk(pgsTypes::tboRight);

   // Pedestrian load is distributed to the same girders, and in the same fraction, as the
   // sidewalk dead load
   Float64 swLoad, fraLeft, fraRight;
   GetSidewalkLoadFraction(segmentKey,&swLoad, &fraLeft,&fraRight);

   Float64 W_per_girder = fraLeft*Wleft + fraRight*Wright;
   return W_per_girder;
}

void CGirderModelManager::ApplyLiveLoadDistributionFactors(GirderIndexType gdr,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel) const
{
   // get the distribution factor collection from the model
   CComPtr<IDistributionFactors> span_factors;
   pModel->get_DistributionFactors(&span_factors);

   // get the support collection before entering the loop
   CComPtr<ISupports> supports;
   pModel->get_Supports(&supports);

   CComPtr<ITemporarySupports> temp_supports;
   pModel->get_TemporarySupports(&temp_supports);

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      GirderIndexType nGirdersInGroup = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirdersInGroup-1);

      CSpanKey spanKey(spanIdx,gdrIdx);

      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      CComBSTR bstrStageName( GetLBAMStageName(liveLoadIntervalIdx) );

      // First, we need to get the points of contraflexure as they define the limits of application of
      // the negative moment distribution factors
      CComPtr<IDblArray> cf_locs;
      pContraflexureResponse->ComputeContraflexureLocations(bstrStageName,&cf_locs);

      SpanType spanType = GetSpanType(spanKey,bContinuous);
      switch ( spanType )
      {
      case PinPin: 
         ApplyLLDF_PinPin(spanKey,cf_locs,span_factors);
         break;

      case PinFix:
         ApplyLLDF_PinFix(spanKey,cf_locs,span_factors);
         break;

      case FixPin:
         ApplyLLDF_FixPin(spanKey,cf_locs,span_factors);
         break;

      case FixFix:
         ApplyLLDF_FixFix(spanKey,cf_locs,span_factors);
         break;

      default:
            ATLASSERT(false); // should never get here
      }

      // layout distribution factors at piers
      ApplyLLDF_Support(spanKey,pgsTypes::metStart,supports, temp_supports);
      ApplyLLDF_Support(spanKey,pgsTypes::metEnd,  supports, temp_supports);
   } // span loop
}

void CGirderModelManager::ConfigureLoadCombinations(ILBAMModel* pModel) const
{
   /////////////////////////////////////////////////
   // NOTE
   // The LRFD does not have a load case for relaxation. The time-step analysis computes
   // forces in the structure due to relaxation. These forces need to be included in the
   // load combinations and limit states. We will "invent" the RE load case for relxation.
   /////////////////////////////////////////////////

   HRESULT hr;

   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   GET_IFACE( ILossParameters, pLossParams);
   bool bTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   // Have multiple options for applying pedestrian loads for different limit states
   GET_IFACE(ILiveLoads,pLiveLoads);
   ILiveLoads::PedestrianLoadApplicationType design_ped_type = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
   ILiveLoads::PedestrianLoadApplicationType permit_ped_type = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);
   ILiveLoads::PedestrianLoadApplicationType fatigue_ped_type = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);

   // Setup the LRFD load combinations
   CComPtr<ILoadCases> loadcases;
   hr = pModel->get_LoadCases(&loadcases);
   loadcases->Clear();

   // Add load cases
   AddLoadCase(loadcases, CComBSTR("DC"),        CComBSTR("Component and Attachments"));
   AddLoadCase(loadcases, CComBSTR("DW"),        CComBSTR("Wearing Surfaces and Utilities"));
   AddLoadCase(loadcases, CComBSTR("DW_Rating"), CComBSTR("Wearing Surfaces and Utilities (for Load Rating)"));
   AddLoadCase(loadcases, CComBSTR("LL_IM"),     CComBSTR("User defined live load"));
   if ( bTimeStepAnalysis )
   {
      AddLoadCase(loadcases, CComBSTR("CR"),        CComBSTR("Creep"));
      AddLoadCase(loadcases, CComBSTR("SH"),        CComBSTR("Shrinkage"));
      AddLoadCase(loadcases, CComBSTR("RE"),        CComBSTR("Relaxation")); // not specifically defined in the LRFD, but we need to include relaxation effects somewhere
      AddLoadCase(loadcases, CComBSTR("PS"),        CComBSTR("Secondary Effects"));
   }

   AddLoadCase(loadcases, CComBSTR("DWp"), CComBSTR("DW for permanent loads")); // User DW + Overlay
   AddLoadCase(loadcases, CComBSTR("DWf"), CComBSTR("DW for future loads")); // Future Overlay

   // add load combinations
   CComPtr<ILoadCombinations> loadcombos;
   hr = pModel->get_LoadCombinations(&loadcombos) ;
   loadcombos->Clear();

   // STRENGTH-I
   CComPtr<ILoadCombination> strength1;
   hr = strength1.CoCreateInstance(CLSID_LoadCombination) ;
   hr = strength1->put_Name( GetLoadCombinationName(pgsTypes::StrengthI) ) ;
   hr = strength1->put_LoadCombinationType(lctStrength) ;
   hr = strength1->put_LiveLoadFactor( pLoadFactors->GetLLIMMax(pgsTypes::StrengthI) ) ;
   hr = strength1->AddLiveLoadModel(lltDesign) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = strength1->AddLiveLoadModel(lltPedestrian) ;
      hr = strength1->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = strength1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->GetDCMin(pgsTypes::StrengthI),   pLoadFactors->GetDCMax(pgsTypes::StrengthI));
   hr = strength1->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->GetDWMin(pgsTypes::StrengthI),   pLoadFactors->GetDWMax(pgsTypes::StrengthI));
   hr = strength1->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->GetDWMax(pgsTypes::StrengthI));
   hr = strength1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->GetLLIMMin(pgsTypes::StrengthI), pLoadFactors->GetLLIMMax(pgsTypes::StrengthI));
   if ( bTimeStepAnalysis )
   {
      hr = strength1->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->GetCRMin(pgsTypes::StrengthI),   pLoadFactors->GetCRMax(pgsTypes::StrengthI));
      hr = strength1->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->GetSHMin(pgsTypes::StrengthI),   pLoadFactors->GetSHMax(pgsTypes::StrengthI));
      hr = strength1->AddLoadCaseFactor(CComBSTR("RE"),    pLoadFactors->GetREMin(pgsTypes::StrengthI),   pLoadFactors->GetREMax(pgsTypes::StrengthI));
      hr = strength1->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->GetPSMin(pgsTypes::StrengthI),   pLoadFactors->GetPSMax(pgsTypes::StrengthI));
   }

   hr = loadcombos->Add(strength1) ;

   // STRENGTH-II
   CComPtr<ILoadCombination> strength2;
   hr = strength2.CoCreateInstance(CLSID_LoadCombination) ;
   hr = strength2->put_Name( GetLoadCombinationName(pgsTypes::StrengthII) ) ;
   hr = strength2->put_LoadCombinationType(lctStrength) ;
   hr = strength2->put_LiveLoadFactor( pLoadFactors->GetLLIMMax(pgsTypes::StrengthII) ) ;
   hr = strength2->AddLiveLoadModel(lltPermit) ;

   if (permit_ped_type != ILiveLoads::PedDontApply)
   {
      hr = strength2->AddLiveLoadModel(lltPedestrian) ;
      hr = strength2->put_LiveLoadModelApplicationType(permit_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = strength2->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->GetDCMin(pgsTypes::StrengthII),   pLoadFactors->GetDCMax(pgsTypes::StrengthII));
   hr = strength2->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->GetDWMin(pgsTypes::StrengthII),   pLoadFactors->GetDWMax(pgsTypes::StrengthII));
   hr = strength2->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->GetDWMax(pgsTypes::StrengthII));
   hr = strength2->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->GetLLIMMin(pgsTypes::StrengthII), pLoadFactors->GetLLIMMax(pgsTypes::StrengthII));
   if ( bTimeStepAnalysis )
   {
      hr = strength2->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->GetCRMin(pgsTypes::StrengthII),   pLoadFactors->GetCRMax(pgsTypes::StrengthII));
      hr = strength2->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->GetSHMin(pgsTypes::StrengthII),   pLoadFactors->GetSHMax(pgsTypes::StrengthII));
      hr = strength2->AddLoadCaseFactor(CComBSTR("RE"),    pLoadFactors->GetREMin(pgsTypes::StrengthII),   pLoadFactors->GetREMax(pgsTypes::StrengthII));
      hr = strength2->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->GetPSMin(pgsTypes::StrengthII),   pLoadFactors->GetPSMax(pgsTypes::StrengthII));
   }

   hr = loadcombos->Add(strength2) ;

   // SERVICE-I
   CComPtr<ILoadCombination> service1;
   hr = service1.CoCreateInstance(CLSID_LoadCombination) ;
   hr = service1->put_Name( GetLoadCombinationName(pgsTypes::ServiceI) ) ;
   hr = service1->put_LoadCombinationType(lctService) ;
   hr = service1->put_LiveLoadFactor(pLoadFactors->GetLLIMMax(pgsTypes::ServiceI) ) ;
   hr = service1->AddLiveLoadModel(lltDesign) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service1->AddLiveLoadModel(lltPedestrian) ;
      hr = service1->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->GetDCMin(pgsTypes::ServiceI),   pLoadFactors->GetDCMax(pgsTypes::ServiceI));
   hr = service1->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->GetDWMin(pgsTypes::ServiceI),   pLoadFactors->GetDWMax(pgsTypes::ServiceI));
   hr = service1->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->GetDWMax(pgsTypes::ServiceI));
   hr = service1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->GetLLIMMin(pgsTypes::ServiceI), pLoadFactors->GetLLIMMax(pgsTypes::ServiceI));
   if ( bTimeStepAnalysis )
   {
      hr = service1->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->GetCRMin(pgsTypes::ServiceI),   pLoadFactors->GetCRMax(pgsTypes::ServiceI));
      hr = service1->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->GetSHMin(pgsTypes::ServiceI),   pLoadFactors->GetSHMax(pgsTypes::ServiceI));
      hr = service1->AddLoadCaseFactor(CComBSTR("RE"),    pLoadFactors->GetREMin(pgsTypes::ServiceI),   pLoadFactors->GetREMax(pgsTypes::ServiceI));
      hr = service1->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->GetPSMin(pgsTypes::ServiceI),   pLoadFactors->GetPSMax(pgsTypes::ServiceI));
   }

   hr = loadcombos->Add(service1) ;

   // SERVICE-III
   CComPtr<ILoadCombination> service3;
   hr = service3.CoCreateInstance(CLSID_LoadCombination) ;
   hr = service3->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII) ) ;
   hr = service3->put_LoadCombinationType(lctService) ;
   hr = service3->put_LiveLoadFactor(pLoadFactors->GetLLIMMax(pgsTypes::ServiceIII) ) ;
   hr = service3->AddLiveLoadModel(lltDesign) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service3->AddLiveLoadModel(lltPedestrian) ;
      hr = service3->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service3->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->GetDCMin(pgsTypes::ServiceIII),   pLoadFactors->GetDCMax(pgsTypes::ServiceIII));
   hr = service3->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->GetDWMin(pgsTypes::ServiceIII),   pLoadFactors->GetDWMax(pgsTypes::ServiceIII));
   hr = service3->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->GetDWMax(pgsTypes::ServiceIII));
   hr = service3->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->GetLLIMMin(pgsTypes::ServiceIII), pLoadFactors->GetLLIMMax(pgsTypes::ServiceIII));
   if ( bTimeStepAnalysis )
   {
      hr = service3->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->GetCRMin(pgsTypes::ServiceIII),   pLoadFactors->GetCRMax(pgsTypes::ServiceIII));
      hr = service3->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->GetSHMin(pgsTypes::ServiceIII),   pLoadFactors->GetSHMax(pgsTypes::ServiceIII));
      hr = service3->AddLoadCaseFactor(CComBSTR("RE"),    pLoadFactors->GetREMin(pgsTypes::ServiceIII),   pLoadFactors->GetREMax(pgsTypes::ServiceIII));
      hr = service3->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->GetPSMin(pgsTypes::ServiceIII),   pLoadFactors->GetPSMax(pgsTypes::ServiceIII));
   }
 
   hr = loadcombos->Add(service3) ;

   // SERVICE-IA... A PGSuper specific load combination not setup by the utility object
   CComPtr<ILoadCombination> service1a;
   service1a.CoCreateInstance(CLSID_LoadCombination);
   service1a->put_Name( GetLoadCombinationName(pgsTypes::ServiceIA) );
   service1a->put_LoadCombinationType(lctService);
   service1a->put_LiveLoadFactor(pLoadFactors->GetLLIMMax(pgsTypes::ServiceIA) );
   service1a->AddLiveLoadModel(lltDesign);

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service1a->AddLiveLoadModel(lltPedestrian) ;
      hr = service1a->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service1a->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->GetDCMin(pgsTypes::ServiceIA),   pLoadFactors->GetDCMax(pgsTypes::ServiceIA));
   hr = service1a->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->GetDWMin(pgsTypes::ServiceIA),   pLoadFactors->GetDWMax(pgsTypes::ServiceIA));
   hr = service1a->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->GetDWMax(pgsTypes::ServiceIA));
   hr = service1a->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->GetLLIMMin(pgsTypes::ServiceIA), pLoadFactors->GetLLIMMax(pgsTypes::ServiceIA));
   if ( bTimeStepAnalysis )
   {
      hr = service1a->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->GetCRMin(pgsTypes::ServiceIA),   pLoadFactors->GetCRMax(pgsTypes::ServiceIA));
      hr = service1a->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->GetSHMin(pgsTypes::ServiceIA),   pLoadFactors->GetSHMax(pgsTypes::ServiceIA));
      hr = service1a->AddLoadCaseFactor(CComBSTR("RE"),    pLoadFactors->GetREMin(pgsTypes::ServiceIA),   pLoadFactors->GetREMax(pgsTypes::ServiceIA));
      hr = service1a->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->GetPSMin(pgsTypes::ServiceIA),   pLoadFactors->GetPSMax(pgsTypes::ServiceIA));
   }

   loadcombos->Add(service1a);

   // FATIGUE-I
   CComPtr<ILoadCombination> fatigue1;
   fatigue1.CoCreateInstance(CLSID_LoadCombination);
   fatigue1->put_Name( GetLoadCombinationName(pgsTypes::FatigueI) );
   fatigue1->put_LoadCombinationType(lctFatigue);
   fatigue1->put_LiveLoadFactor(pLoadFactors->GetLLIMMax(pgsTypes::FatigueI) );
   fatigue1->AddLiveLoadModel(lltFatigue);

   if (fatigue_ped_type != ILiveLoads::PedDontApply)
   {
      hr = fatigue1->AddLiveLoadModel(lltPedestrian) ;
      hr = fatigue1->put_LiveLoadModelApplicationType(fatigue_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->GetDCMin(pgsTypes::FatigueI),   pLoadFactors->GetDCMax(pgsTypes::FatigueI));
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->GetDWMin(pgsTypes::FatigueI),   pLoadFactors->GetDWMax(pgsTypes::FatigueI));
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->GetDWMax(pgsTypes::FatigueI));
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->GetLLIMMin(pgsTypes::FatigueI), pLoadFactors->GetLLIMMax(pgsTypes::FatigueI));
   if ( bTimeStepAnalysis )
   {
      hr = fatigue1->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->GetCRMin(pgsTypes::FatigueI),   pLoadFactors->GetCRMax(pgsTypes::FatigueI));
      hr = fatigue1->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->GetSHMin(pgsTypes::FatigueI),   pLoadFactors->GetSHMax(pgsTypes::FatigueI));
      hr = fatigue1->AddLoadCaseFactor(CComBSTR("RE"),    pLoadFactors->GetREMin(pgsTypes::FatigueI),   pLoadFactors->GetREMax(pgsTypes::FatigueI));
      hr = fatigue1->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->GetPSMin(pgsTypes::FatigueI),   pLoadFactors->GetPSMax(pgsTypes::FatigueI));
   }

   loadcombos->Add(fatigue1);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 DC, DW, CR, SH, RE, PS, LLIM;

   // Deal with pedestrian load applications for rating.
   // All rating limit states are treated the same
   bool rating_include_pedes = pRatingSpec->IncludePedestrianLiveLoad();

   // STRENGTH-I - Design Rating - Inventory Level
   CComPtr<ILoadCombination> strengthI_inventory;
   strengthI_inventory.CoCreateInstance(CLSID_LoadCombination);
   strengthI_inventory->put_Name( GetLoadCombinationName(pgsTypes::StrengthI_Inventory) );
   strengthI_inventory->put_LoadCombinationType(lctStrength);
   strengthI_inventory->AddLiveLoadModel(lltDesign);

   if (rating_include_pedes)
   {
      hr = strengthI_inventory->AddLiveLoadModel(lltPedestrian) ;
      hr = strengthI_inventory->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_Inventory);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_Inventory);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_Inventory,true);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_inventory->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_Inventory);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_Inventory);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::StrengthI_Inventory);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::StrengthI_Inventory);

      hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }
   loadcombos->Add(strengthI_inventory);


   // STRENGTH-I - Design Rating - Operating Level
   CComPtr<ILoadCombination> strengthI_operating;
   strengthI_operating.CoCreateInstance(CLSID_LoadCombination);
   strengthI_operating->put_Name( GetLoadCombinationName(pgsTypes::StrengthI_Operating) );
   strengthI_operating->put_LoadCombinationType(lctStrength);
   strengthI_operating->AddLiveLoadModel(lltDesign);

   if (rating_include_pedes)
   {
      hr = strengthI_operating->AddLiveLoadModel(lltPedestrian) ;
      hr = strengthI_operating->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_Operating);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_Operating);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_Operating,true);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_operating->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_Operating);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_Operating);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::StrengthI_Operating);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::StrengthI_Operating);

      hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(strengthI_operating);

   // SERVICE-III - Design Rating - Inventory Level
   CComPtr<ILoadCombination> serviceIII_inventory;
   serviceIII_inventory.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_inventory->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII_Inventory) );
   serviceIII_inventory->put_LoadCombinationType(lctService);
   serviceIII_inventory->AddLiveLoadModel(lltDesign);

   if (rating_include_pedes)
   {
      hr = serviceIII_inventory->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceIII_inventory->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_Inventory);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_Inventory);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_Inventory,true);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_inventory->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_Inventory);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_Inventory);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceIII_Inventory);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceIII_Inventory);

      hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceIII_inventory);

   // SERVICE-III - Design Rating - Operating Level
   CComPtr<ILoadCombination> serviceIII_operating;
   serviceIII_operating.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_operating->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII_Operating) );
   serviceIII_operating->put_LoadCombinationType(lctService);
   serviceIII_operating->AddLiveLoadModel(lltDesign);

   if (rating_include_pedes)
   {
      hr = serviceIII_operating->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceIII_operating->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_Operating);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_Operating);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_Operating,true);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_operating->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_Operating);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_Operating);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceIII_Operating);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceIII_Operating);

      hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceIII_operating);

   // STRENGTH-I - Legal Rating - Routine Commercial Traffic
   CComPtr<ILoadCombination> strengthI_routine;
   strengthI_routine.CoCreateInstance(CLSID_LoadCombination);
   strengthI_routine->put_Name( GetLoadCombinationName(pgsTypes::StrengthI_LegalRoutine) );
   strengthI_routine->put_LoadCombinationType(lctStrength);
   strengthI_routine->AddLiveLoadModel(lltLegalRoutineRating);

   if (rating_include_pedes)
   {
      hr = strengthI_routine->AddLiveLoadModel(lltPedestrian) ;
      hr = strengthI_routine->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_LegalRoutine);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_LegalRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_LegalRoutine,true);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_routine->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_LegalRoutine);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_LegalRoutine);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::StrengthI_LegalRoutine);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::StrengthI_LegalRoutine);

      hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(strengthI_routine);

   // SERVICE-III - Legal Rating - Routine Commercial Traffic
   CComPtr<ILoadCombination> serviceIII_routine;
   serviceIII_routine.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_routine->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII_LegalRoutine) );
   serviceIII_routine->put_LoadCombinationType(lctService);
   serviceIII_routine->AddLiveLoadModel(lltLegalRoutineRating);

   if (rating_include_pedes)
   {
      hr = serviceIII_routine->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceIII_routine->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_LegalRoutine,true);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_routine->put_LiveLoadFactor(LLIM);

   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_LegalRoutine);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_LegalRoutine);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceIII_LegalRoutine);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceIII_LegalRoutine);

      hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceIII_routine);

   // STRENGTH-I - Legal Rating - Special Hauling Vehicles
   CComPtr<ILoadCombination> strengthI_special;
   strengthI_special.CoCreateInstance(CLSID_LoadCombination);
   strengthI_special->put_Name( GetLoadCombinationName(pgsTypes::StrengthI_LegalSpecial) );
   strengthI_special->put_LoadCombinationType(lctStrength);
   strengthI_special->AddLiveLoadModel(lltLegalSpecialRating);

   if (rating_include_pedes)
   {
      hr = strengthI_special->AddLiveLoadModel(lltPedestrian) ;
      hr = strengthI_special->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_LegalSpecial);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_LegalSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_LegalSpecial,true);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_special->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_LegalSpecial);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_LegalSpecial);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::StrengthI_LegalSpecial);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::StrengthI_LegalSpecial);

      hr = strengthI_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = strengthI_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = strengthI_special->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = strengthI_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(strengthI_special);

   // SERVICE-III - Legal Rating - Special Hauling Vehicles
   CComPtr<ILoadCombination> serviceIII_special;
   serviceIII_special.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_special->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII_LegalSpecial) );
   serviceIII_special->put_LoadCombinationType(lctService);
   serviceIII_special->AddLiveLoadModel(lltLegalSpecialRating);

   if (rating_include_pedes)
   {
      hr = serviceIII_special->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceIII_special->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_LegalSpecial);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_LegalSpecial,true);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_special->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_LegalSpecial);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_LegalSpecial);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceIII_LegalSpecial);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceIII_LegalSpecial);

      hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceIII_special);

   // STRENGTH-I - Legal Rating - Emergency Vehicles
   CComPtr<ILoadCombination> strengthI_emergency;
   strengthI_emergency.CoCreateInstance(CLSID_LoadCombination);
   strengthI_emergency->put_Name(GetLoadCombinationName(pgsTypes::StrengthI_LegalEmergency));
   strengthI_emergency->put_LoadCombinationType(lctStrength);
   strengthI_emergency->AddLiveLoadModel(lltLegalEmergencyRating);

   if (rating_include_pedes)
   {
      hr = strengthI_emergency->AddLiveLoadModel(lltPedestrian);
      hr = strengthI_emergency->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(pgsTypes::StrengthI_LegalEmergency);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_LegalEmergency);
   LLIM = pRatingSpec->GetLiveLoadFactor(pgsTypes::StrengthI_LegalEmergency, true);
   hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_emergency->put_LiveLoadFactor(LLIM);
   if (bTimeStepAnalysis)
   {
      CR = pRatingSpec->GetCreepFactor(pgsTypes::StrengthI_LegalEmergency);
      SH = pRatingSpec->GetShrinkageFactor(pgsTypes::StrengthI_LegalEmergency);
      RE = pRatingSpec->GetRelaxationFactor(pgsTypes::StrengthI_LegalEmergency);
      PS = pRatingSpec->GetSecondaryEffectsFactor(pgsTypes::StrengthI_LegalEmergency);

      hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("CR"), CR, CR);
      hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("SH"), SH, SH);
      hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("RE"), RE, RE);
      hr = strengthI_emergency->AddLoadCaseFactor(CComBSTR("PS"), PS, PS);
   }

   loadcombos->Add(strengthI_emergency);

   // SERVICE-III - Legal Rating - Emergency Vehicles
   CComPtr<ILoadCombination> serviceIII_emergency;
   serviceIII_emergency.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_emergency->put_Name(GetLoadCombinationName(pgsTypes::ServiceIII_LegalEmergency));
   serviceIII_emergency->put_LoadCombinationType(lctService);
   serviceIII_emergency->AddLiveLoadModel(lltLegalEmergencyRating);

   if (rating_include_pedes)
   {
      hr = serviceIII_emergency->AddLiveLoadModel(lltPedestrian);
      hr = serviceIII_emergency->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(pgsTypes::ServiceIII_LegalEmergency);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalEmergency);
   LLIM = pRatingSpec->GetLiveLoadFactor(pgsTypes::ServiceIII_LegalEmergency, true);
   hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_emergency->put_LiveLoadFactor(LLIM);
   if (bTimeStepAnalysis)
   {
      CR = pRatingSpec->GetCreepFactor(pgsTypes::ServiceIII_LegalEmergency);
      SH = pRatingSpec->GetShrinkageFactor(pgsTypes::ServiceIII_LegalEmergency);
      RE = pRatingSpec->GetRelaxationFactor(pgsTypes::ServiceIII_LegalEmergency);
      PS = pRatingSpec->GetSecondaryEffectsFactor(pgsTypes::ServiceIII_LegalEmergency);

      hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("CR"), CR, CR);
      hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("SH"), SH, SH);
      hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("RE"), RE, RE);
      hr = serviceIII_emergency->AddLoadCaseFactor(CComBSTR("PS"), PS, PS);
   }

   loadcombos->Add(serviceIII_emergency);

   // STRENGTH-II - Permit Rating
   CComPtr<ILoadCombination> strengthII_routine;
   strengthII_routine.CoCreateInstance(CLSID_LoadCombination);
   strengthII_routine->put_Name( GetLoadCombinationName(pgsTypes::StrengthII_PermitRoutine) );
   strengthII_routine->put_LoadCombinationType(lctStrength);
   strengthII_routine->AddLiveLoadModel(lltPermitRoutineRating);

   if (rating_include_pedes)
   {
      hr = strengthII_routine->AddLiveLoadModel(lltPedestrian) ;
      hr = strengthII_routine->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitRoutine);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthII_PermitRoutine,true);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthII_routine->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitRoutine);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitRoutine);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::StrengthII_PermitRoutine);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::StrengthII_PermitRoutine);

      hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(strengthII_routine);

   CComPtr<ILoadCombination> strengthII_special;
   strengthII_special.CoCreateInstance(CLSID_LoadCombination);
   strengthII_special->put_Name( GetLoadCombinationName(pgsTypes::StrengthII_PermitSpecial) );
   strengthII_special->put_LoadCombinationType(lctStrength);
   strengthII_special->AddLiveLoadModel(lltPermitSpecialRating);

   if (rating_include_pedes)
   {
      hr = strengthII_special->AddLiveLoadModel(lltPedestrian) ;
      hr = strengthII_special->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitSpecial);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthII_PermitSpecial,true);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthII_special->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitSpecial);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitSpecial);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::StrengthII_PermitSpecial);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::StrengthII_PermitSpecial);

      hr = strengthII_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = strengthII_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = strengthII_special->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = strengthII_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(strengthII_special);

   // SERVICE-I - Permit Routine Rating
   CComPtr<ILoadCombination> serviceI_routine;
   serviceI_routine.CoCreateInstance(CLSID_LoadCombination);
   serviceI_routine->put_Name( GetLoadCombinationName(pgsTypes::ServiceI_PermitRoutine) );
   serviceI_routine->put_LoadCombinationType(lctService);
   serviceI_routine->AddLiveLoadModel(lltPermitRoutineRating);

   if (rating_include_pedes)
   {
      hr = serviceI_routine->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceI_routine->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitRoutine);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceI_PermitRoutine,true);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceI_routine->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitRoutine);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitRoutine);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceI_PermitRoutine);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceI_PermitRoutine);

      hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceI_routine);


   // SERVICE-III - Permit Routine Rating
   CComPtr<ILoadCombination> serviceIII_permit_routine;
   serviceIII_permit_routine.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_permit_routine->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII_PermitRoutine) );
   serviceIII_permit_routine->put_LoadCombinationType(lctService);
   serviceIII_permit_routine->AddLiveLoadModel(lltPermitRoutineRating);

   if (rating_include_pedes)
   {
      hr = serviceIII_permit_routine->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceIII_permit_routine->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_PermitRoutine);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_PermitRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_PermitRoutine,true);
   hr = serviceIII_permit_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_permit_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_permit_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_permit_routine->put_LiveLoadFactor(LLIM);
   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_PermitRoutine);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_PermitRoutine);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceIII_PermitRoutine);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceIII_PermitRoutine);

      hr = serviceIII_permit_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceIII_permit_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceIII_permit_routine->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceIII_permit_routine);

   // SERVICE-I - Permit Special Rating
   CComPtr<ILoadCombination> serviceI_special;
   serviceI_special.CoCreateInstance(CLSID_LoadCombination);
   serviceI_special->put_Name( GetLoadCombinationName(pgsTypes::ServiceI_PermitSpecial) );
   serviceI_special->put_LoadCombinationType(lctService);
   serviceI_special->AddLiveLoadModel(lltPermitSpecialRating);

   if (rating_include_pedes)
   {
      hr = serviceI_special->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceI_special->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitSpecial);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceI_PermitSpecial,true);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceI_routine->put_LiveLoadFactor(LLIM);

   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitSpecial);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitSpecial);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceI_PermitSpecial);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceI_PermitSpecial);

      hr = serviceI_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceI_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceI_special->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceI_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceI_special);


   // SERVICE-III - Permit Special Rating
   CComPtr<ILoadCombination> serviceIII_permit_special;
   serviceIII_permit_special.CoCreateInstance(CLSID_LoadCombination);
   serviceIII_permit_special->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII_PermitSpecial) );
   serviceIII_permit_special->put_LoadCombinationType(lctService);
   serviceIII_permit_special->AddLiveLoadModel(lltPermitSpecialRating);

   if (rating_include_pedes)
   {
      hr = serviceIII_permit_special->AddLiveLoadModel(lltPedestrian) ;
      hr = serviceIII_permit_special->put_LiveLoadModelApplicationType(llmaSum);
   }

   DC = pRatingSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_PermitSpecial);
   DW = pRatingSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_PermitSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_PermitSpecial,true);
   hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_permit_special->put_LiveLoadFactor(LLIM);

   if ( bTimeStepAnalysis )
   {
      CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_PermitSpecial);
      SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_PermitSpecial);
      RE = pRatingSpec->GetRelaxationFactor(    pgsTypes::ServiceIII_PermitSpecial);
      PS = pRatingSpec->GetSecondaryEffectsFactor( pgsTypes::ServiceIII_PermitSpecial);

      hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
      hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
      hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("RE"),    RE, RE);
      hr = serviceIII_permit_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
   }

   loadcombos->Add(serviceIII_permit_special);

   // Design load combination... 
   // These liveload-only combinations are created so we can sum user-defined static live loads with other live loads
   CComPtr<ILoadCombination> lc_design;
   lc_design.CoCreateInstance(CLSID_LoadCombination);
   lc_design->put_Name( GetLiveLoadName(pgsTypes::lltDesign) );
   lc_design->put_LoadCombinationType(lctUserDefined);  // this way no one can tweak the load modifiers for this case
   lc_design->put_LiveLoadFactor(1.00);
   lc_design->AddLiveLoadModel(lltDesign);

   lc_design->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00) ;

   loadcombos->Add(lc_design);

   // fatigue truck
   CComPtr<ILoadCombination> lc_fatigue;
   lc_fatigue.CoCreateInstance(CLSID_LoadCombination);
   lc_fatigue->put_Name( GetLiveLoadName(pgsTypes::lltFatigue) );
   lc_fatigue->put_LoadCombinationType(lctFatigue);
   lc_fatigue->put_LiveLoadFactor(1.0);
   lc_fatigue->AddLiveLoadModel(lltFatigue);

   hr = lc_fatigue->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);

   loadcombos->Add(lc_fatigue);

   // Permit load combination... 
   CComPtr<ILoadCombination> lc_permit;
   lc_permit.CoCreateInstance(CLSID_LoadCombination);
   lc_permit->put_Name( GetLiveLoadName(pgsTypes::lltPermit) );
   lc_permit->put_LoadCombinationType(lctPermit);
   lc_permit->put_LiveLoadFactor(1.00);
   lc_permit->AddLiveLoadModel(lltPermit);

   lc_permit->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00) ;

   loadcombos->Add(lc_permit);

   // pedestrian live load
   CComPtr<ILoadCombination> lc_pedestrian;
   lc_pedestrian.CoCreateInstance(CLSID_LoadCombination);
   lc_pedestrian->put_Name( GetLiveLoadName(pgsTypes::lltPedestrian) );
   lc_pedestrian->put_LoadCombinationType(lctUserDefined);
   lc_pedestrian->put_LiveLoadFactor(1.00);
   lc_pedestrian->AddLiveLoadModel(lltPedestrian);

   lc_pedestrian->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00) ;

   loadcombos->Add(lc_pedestrian);

   // legal - routine commercial traffic
   CComPtr<ILoadCombination> lc_legal_routine;
   lc_legal_routine.CoCreateInstance(CLSID_LoadCombination);
   lc_legal_routine->put_Name( GetLiveLoadName(pgsTypes::lltLegalRating_Routine) );
   lc_legal_routine->put_LoadCombinationType(lctStrength);
   lc_legal_routine->put_LiveLoadFactor(1.0);
   lc_legal_routine->AddLiveLoadModel(lltLegalRoutineRating);
   hr = lc_legal_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_legal_routine);

   // legal - specialized hauling vehicle
   CComPtr<ILoadCombination> lc_legal_special;
   lc_legal_special.CoCreateInstance(CLSID_LoadCombination);
   lc_legal_special->put_Name(GetLiveLoadName(pgsTypes::lltLegalRating_Special));
   lc_legal_special->put_LoadCombinationType(lctStrength);
   lc_legal_special->put_LiveLoadFactor(1.0);
   lc_legal_special->AddLiveLoadModel(lltLegalSpecialRating);
   hr = lc_legal_special->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_legal_special);

   // legal - emergency vehicle
   CComPtr<ILoadCombination> lc_legal_emergency;
   lc_legal_emergency.CoCreateInstance(CLSID_LoadCombination);
   lc_legal_emergency->put_Name(GetLiveLoadName(pgsTypes::lltLegalRating_Emergency));
   lc_legal_emergency->put_LoadCombinationType(lctStrength);
   lc_legal_emergency->put_LiveLoadFactor(1.0);
   lc_legal_emergency->AddLiveLoadModel(lltLegalEmergencyRating);
   hr = lc_legal_emergency->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_legal_emergency);

   // permit rating - routine
   CComPtr<ILoadCombination> lc_permit_routine;
   lc_permit_routine.CoCreateInstance(CLSID_LoadCombination);
   lc_permit_routine->put_Name( GetLiveLoadName(pgsTypes::lltPermitRating_Routine) );
   lc_permit_routine->put_LoadCombinationType(lctFatigue);
   lc_permit_routine->put_LiveLoadFactor(1.0);
   lc_permit_routine->AddLiveLoadModel(lltPermitRoutineRating);
   hr = lc_permit_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_permit_routine);

   // permit rating - special
   CComPtr<ILoadCombination> lc_permit_special;
   lc_permit_special.CoCreateInstance(CLSID_LoadCombination);
   lc_permit_special->put_Name( GetLiveLoadName(pgsTypes::lltPermitRating_Special) );
   lc_permit_special->put_LoadCombinationType(lctStrength);
   lc_permit_special->put_LiveLoadFactor(1.0);
   lc_permit_special->AddLiveLoadModel(lltPermitSpecialRating);
   hr = lc_permit_special->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_permit_special);

   // Now we have to map our product loads (girder, diaphragms, etc) to the LRFD load cases (DC, DW, etc)
   CComPtr<ILoadCases> load_cases;
   pModel->get_LoadCases(&load_cases);

   std::vector<LoadingCombinationType> combos;
   combos.reserve(lcLoadingCombinationTypeCount);
   combos.push_back(lcDC);
   combos.push_back(lcDW);
   combos.push_back(lcDWRating);
   combos.push_back(lcDWp);
   combos.push_back(lcDWf);
   if ( bTimeStepAnalysis )
   {
      combos.push_back(lcCR);
      combos.push_back(lcSH);
      combos.push_back(lcRE);
      combos.push_back(lcPS);
   }

   for(const auto& combo : combos)
   {
      CComPtr<ILoadCase> load_case;
      load_cases->Find(GetLoadCaseName(combo),&load_case);
      std::vector<pgsTypes::ProductForceType> pfTypes(CProductLoadMap::GetProductForces(m_pBroker,combo));

      for(const auto& pfType : pfTypes)
      {
         if ( !bTimeStepAnalysis && (pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation || pfType == pgsTypes::pftSecondaryEffects) )
         {
            continue;
         }

         load_case->AddLoadGroup(GetLoadGroupName(pfType));
      }
   }

   CComPtr<ILoadCase> load_case_ll;
   load_cases->Find(CComBSTR("LL_IM"),&load_case_ll);
   load_case_ll->AddLoadGroup(GetLoadGroupName(pgsTypes::pftUserLLIM));

   // set up load groups
   CComPtr<ILoadGroups> loadGroups;
   pModel->get_LoadGroups(&loadGroups);
   CComBSTR bstrGirder(GetLoadGroupName(pgsTypes::pftGirder));
   AddLoadGroup(loadGroups, bstrGirder,         CComBSTR("Girder self weight"));
   bstrGirder += CComBSTR(_T("_Incremental"));
   AddLoadGroup(loadGroups, bstrGirder, CComBSTR("Incremental Girder Self Weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftConstruction),   CComBSTR("Construction"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftSlab),           CComBSTR("Slab self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftSlabPad),        CComBSTR("Slab Pad self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftSlabPanel),      CComBSTR("Slab Panel self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftDiaphragm),      CComBSTR("Diaphragm self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftSidewalk),       CComBSTR("Sidewalk self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftTrafficBarrier), CComBSTR("Traffic Barrier self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftShearKey),       CComBSTR("Shear Key Weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftLongitudinalJoint), CComBSTR("Longitudinal Joint Weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftOverlay),        CComBSTR("Overlay self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftOverlayRating),  CComBSTR("Overlay self weight (rating)"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftUserDC),         CComBSTR("User applied loads in DC"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftUserDW),         CComBSTR("User applied loads in DW"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftUserLLIM),       CComBSTR("User applied live load"));
   if ( bTimeStepAnalysis )
   {
      AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftCreep),          CComBSTR("Creep"));
      AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftShrinkage),      CComBSTR("Shrinkage"));
      AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftRelaxation),     CComBSTR("Relaxation"));
      AddLoadGroup(loadGroups, GetLoadGroupName(pgsTypes::pftSecondaryEffects),CComBSTR("Secondary Effects"));
   }

   // create a couple special load groups for getting effects of the differnet strand types
   CComBSTR bstrLoadGroupX, bstrLoadGroupY;
   GetLoadGroupName(pgsTypes::Straight, bstrLoadGroupX, bstrLoadGroupY);
   AddLoadGroup(loadGroups, bstrLoadGroupX, CComBSTR("Straight Strand Equivalent Loading - Moment about Y axis"));
   AddLoadGroup(loadGroups, bstrLoadGroupY, CComBSTR("Straight Strand Equivalent Loading - Moment about X axis"));

   GetLoadGroupName(pgsTypes::Harped, bstrLoadGroupX, bstrLoadGroupY);
   AddLoadGroup(loadGroups, bstrLoadGroupX, CComBSTR("Harped Strand Equivalent Loading - Moment about Y axis"));
   AddLoadGroup(loadGroups, bstrLoadGroupY, CComBSTR("Harped Strand Equivalent Loading - Moment about X axis"));

   GetLoadGroupName(pgsTypes::Temporary, bstrLoadGroupX, bstrLoadGroupY);
   AddLoadGroup(loadGroups, bstrLoadGroupX, CComBSTR("Temporary Strand Equivalent Loading - Moment about Y axis"));
   AddLoadGroup(loadGroups, bstrLoadGroupY, CComBSTR("Temporary Strand Equivalent Loading - Moment about X axis"));
}

void CGirderModelManager::ApplyDiaphragmLoadsAtPiers(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdr) const
{
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   GET_IFACE(IPointOfInterest,pPoi);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);

   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   CComBSTR bstrDiaphragmStage(GetLBAMStageName(castDiaphragmIntervalIdx));

   IntervalIndexType firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   CComBSTR bstrDeckStage(GetLBAMStageName(firstCastDeckIntervalIdx));

   CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftDiaphragm) ); 

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      // iterate over all the piers in the group
      const CPierData2* pStartPier = pGroup->GetPier(pgsTypes::metStart);
      const CPierData2* pEndPier   = pGroup->GetPier(pgsTypes::metEnd);
      for ( const CPierData2* pPier = pStartPier; // start
            pPier != nullptr && pPier->GetIndex() <= pEndPier->GetIndex(); // condition
            pPier = (pPier->GetNextSpan() ? pPier->GetNextSpan()->GetNextPier() : nullptr) // increment
          )
      {
         PierIndexType pierIdx = pPier->GetIndex();
         if ( pPier == pStartPier )
         {
            // only add the load on the ahead side of the pier
            PoiList vPoi;
            pPoi->GetPointsOfInterest(CSegmentKey(girderKey, 0), POI_0L | POI_ERECTED_SEGMENT, &vPoi);
            ATLASSERT(vPoi.size() == 1);
            const pgsPointOfInterest& poi(vPoi.front());

            PIER_DIAPHRAGM_LOAD_DETAILS backSide, aheadSide;
            GetPierDiaphragmLoads(pierIdx, girderKey.girderIndex, &backSide, &aheadSide);

            const CSegmentKey& segmentKey(poi.GetSegmentKey());
            Float64 end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);
            Float64 Xs = end_dist; // assume P and M are applied at CL Bearing
            if (!IsZero(aheadSide.MomentArm))
            {
               // P is applied at another location (that is why moment arm != 0)
               Xs = end_dist - aheadSide.MomentArm; // moment arm is measured from CL Bearing towards the left where the load occurs... convert to the Segment Coordinate System
            }
            Xs = IsZero(Xs) ? 0.0 : Xs;
            ATLASSERT(0 <= Xs); // load must be on the segment

            bool bStartCantilever, bEndCantilever;
            pBridge->ModelCantilevers(segmentKey, &bStartCantilever, &bEndCantilever);

            MemberType mbrType;
            MemberIDType mbrID;
            Float64 Xmbr;
            GetLoadPosition(pModel, segmentKey, Xs, bStartCantilever, &mbrType, &mbrID, &Xmbr);

            CComPtr<IPointLoad> load;
            load.CoCreateInstance(CLSID_PointLoad);
            load->put_MemberType(mbrType);
            load->put_MemberID(mbrID);
            load->put_Location(Xmbr);
            load->put_Fy(aheadSide.P);
            load->put_Mz(aheadSide.M);

            CComBSTR bstrStage;
            pgsTypes::BoundaryConditionType bcType = pPier->GetBoundaryConditionType();
            if (bcType == pgsTypes::bctContinuousAfterDeck || 
                bcType == pgsTypes::bctIntegralAfterDeck ||
                bcType == pgsTypes::bctIntegralAfterDeckHingeBack )
            {
               // if the boundary conditions are continuous/integral after deck,
               // then assume the diaphragm is cast with the deck....
               bstrStage = bstrDeckStage;
            }
            else
            {
               // ... otherwise assume the diaphragm is cast with the interior diaphragms
               bstrStage = bstrDiaphragmStage;
            }

            CComPtr<IPointLoadItem> ptLoadItem;
            pointLoads->Add(bstrStage,bstrLoadGroup,load,&ptLoadItem);

            SaveOverhangPointLoads(CSegmentKey(girderKey,0),analysisType,bstrStage,bstrLoadGroup,aheadSide.P,backSide.P);
         }
         else if ( pPier == pEndPier )
         {
            // only add the load on the back side of the pier
            PoiList vPoi;
            pPoi->GetPointsOfInterest(CSegmentKey(girderKey, nSegments - 1), POI_10L | POI_ERECTED_SEGMENT, &vPoi);
            ATLASSERT(vPoi.size() == 1);
            const pgsPointOfInterest& poi(vPoi.front());

            PIER_DIAPHRAGM_LOAD_DETAILS backSide, aheadSide;
            GetPierDiaphragmLoads(pierIdx, girderKey.girderIndex, &backSide, &aheadSide);

            const CSegmentKey& segmentKey(poi.GetSegmentKey());
            Float64 Ls = pBridge->GetSegmentLength(segmentKey);
            Float64 end_dist = pBridge->GetSegmentEndEndDistance(segmentKey);
            Float64 Xs = Ls - end_dist;
            if (!IsZero(backSide.MomentArm))
            {
               // P is applied at another location (this is why moment arm != 0)
               Xs += backSide.MomentArm; // moment are is measured from CL Bearing towards the right where the load occurs... convert to Segment Coordinate System
            }
            Xs = IsZero(Xs) ? 0.0 : Xs;
            Xs = IsEqual(Xs, Ls) ? Ls : Xs;
            ATLASSERT(0 <= Xs && Xs <= Ls);

            bool bStartCantilever, bEndCantilever;
            pBridge->ModelCantilevers(segmentKey, &bStartCantilever, &bEndCantilever);

            MemberType mbrType;
            MemberIDType mbrID;
            Float64 Xmbr;
            GetLoadPosition(pModel, segmentKey, Xs, bEndCantilever, &mbrType, &mbrID, &Xmbr);

            CComPtr<IPointLoad> load;
            load.CoCreateInstance(CLSID_PointLoad);
            load->put_MemberType(mbrType);
            load->put_MemberID(mbrID);
            load->put_Location(Xmbr);
            load->put_Fy(backSide.P);
            load->put_Mz(backSide.M);

            CComBSTR bstrStage;
            pgsTypes::BoundaryConditionType bcType = pPier->GetBoundaryConditionType();
            if (bcType == pgsTypes::bctContinuousAfterDeck ||
               bcType == pgsTypes::bctIntegralAfterDeck ||
               bcType == pgsTypes::bctIntegralAfterDeckHingeAhead)
            {
               // if the boundary conditions are continuous/integral after deck,
               // then assume the diaphragm is cast with the deck....
               bstrStage = bstrDeckStage;
            }
            else
            {
               // ... otherwise assume the diaphragm is cast with the interior diaphragms
               bstrStage = bstrDiaphragmStage;
            }

            CComPtr<IPointLoadItem> ptLoadItem;
            pointLoads->Add(bstrStage,bstrLoadGroup,load,&ptLoadItem);

            SaveOverhangPointLoads(CSegmentKey(girderKey,nSegments-1),analysisType,bstrStage,bstrLoadGroup,aheadSide.P,backSide.P);
         }
         else
         {
            // This pier is interior to the group... this only happens with spliced girder bridges
            ATLASSERT(pPier->IsInteriorPier());

            CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx,girderKey);
            Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);

            // get superstructure member ID where the segment starts
            MemberIDType mbrID = GetFirstSuperstructureMemberID(segmentKey);

            // if there is an overhang at the start, the mbrID for the main portion
            // of the segment is one more... increment mbrID
            if ( !IsZero(start_end_dist) )
            {
               mbrID++;
            }

            // get location where CL-Segment intersects CL-Pier (could be betweens ends of segment or after end of segment)
            CComPtr<IPoint2d> pntSegPierIntersection;
            bool bIntersect = pBridge->GetSegmentPierIntersection(segmentKey, pierIdx,pgsTypes::pcLocal,&pntSegPierIntersection);
            ATLASSERT(bIntersect == true);

            // get the distance from the the start face of the segment to the intersection point
            // with the CL pier.
            GET_IFACE(IGirder, pGdr);
            CComPtr<IPoint2d> pntSupport[2],pntEnd[2],pntBrg[2];
            pGdr->GetSegmentEndPoints(segmentKey,pgsTypes::pcLocal,
                                      &pntSupport[pgsTypes::metStart],&pntEnd[pgsTypes::metStart],&pntBrg[pgsTypes::Start],
                                      &pntBrg[pgsTypes::metEnd],      &pntEnd[pgsTypes::metEnd],  &pntSupport[pgsTypes::metEnd]);


            Float64 dist_along_segment;
            pntEnd[pgsTypes::metStart]->DistanceEx(pntSegPierIntersection,&dist_along_segment);

            pgsPointOfInterest poi = pPoi->GetPierPointOfInterest(girderKey, pierIdx);

            PIER_DIAPHRAGM_LOAD_DETAILS backSide, aheadSide;
            GetPierDiaphragmLoads(pierIdx, girderKey.girderIndex, &backSide, &aheadSide);

            if ( pPier->GetSegmentConnectionType() == pgsTypes::psctContinuousSegment ||
                 pPier->GetSegmentConnectionType() == pgsTypes::psctIntegralSegment) 
            {
               // there should not be any moment at intermediate piers with continuous segments
               ATLASSERT(IsZero(backSide.M) && IsZero(backSide.MomentArm));
               ATLASSERT(IsZero(aheadSide.M) && IsZero(aheadSide.MomentArm));

               // Segment is continuous over the pier... apply the total load at the CL Pier
               //
               //                        +-- apply load here
               //                        |
               //                        V
               //  =================================================
               //                        ^
               //                        CL Pier

               // Apply total load at CL Pier
               CComPtr<IPointLoad> load;
               load.CoCreateInstance(CLSID_PointLoad);
               load->put_MemberType(mtSuperstructureMember);
               load->put_MemberID(mbrID);
               load->put_Location(dist_along_segment - start_end_dist);
               load->put_Fy(backSide.P + aheadSide.P);

               // apply this load along with the deck
               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrDeckStage,bstrLoadGroup,load,&ptLoadItem);
               SaveOverhangPointLoads(CSegmentKey(girderKey, 0), analysisType, bstrDeckStage, bstrLoadGroup, aheadSide.P, backSide.P);
            }
            else
            {
               // Two segments are supported at this pier
               // Apply load on each side of the pier at CL Bearings
               //
               //    Apply left load here -+   +- Apply right load here
               //                          |   |
               //        Seg i             V   V    Seg i+1
               //       ====================   =====================
               //                          o ^ o  <- temporary support
               //                            CL Pier

               CSegmentKey nextSegmentKey(segmentKey);
               nextSegmentKey.segmentIndex++;

               Float64 back_bearing_offset  = pBridge->GetSegmentEndBearingOffset(segmentKey);
               Float64 ahead_bearing_offset = pBridge->GetSegmentStartBearingOffset(nextSegmentKey);
               Float64 next_start_end_dist  = pBridge->GetSegmentStartEndDistance(nextSegmentKey);

               MemberIDType mbrID = GetFirstSuperstructureMemberID(segmentKey);
               if ( !IsZero(start_end_dist) )
               {
                  mbrID++;
               }


               // This is an interior pier and the segment connection type is one of the "closure joint" types
               // The closure joint is assumed to be cast with the diaphragm
               ATLASSERT(pPier->GetSegmentConnectionType() == pgsTypes::psctContinousClosureJoint || pPier->GetSegmentConnectionType() == pgsTypes::psctIntegralClosureJoint);
               CComBSTR bstrStageBack = bstrDiaphragmStage;


               CComPtr<IPointLoad> backLoad;
               backLoad.CoCreateInstance(CLSID_PointLoad);
               backLoad->put_MemberType(mtSuperstructureMember);
               backLoad->put_MemberID(mbrID);
               backLoad->put_Location(dist_along_segment - start_end_dist - back_bearing_offset);
               backLoad->put_Fy(backSide.P);
               backLoad->put_Mz(backSide.M);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStageBack,bstrLoadGroup,backLoad,&ptLoadItem);
               SaveOverhangPointLoads(CSegmentKey(girderKey, 0), analysisType, bstrStageBack, bstrLoadGroup, 0, backSide.P);

               mbrID = GetFirstSuperstructureMemberID(nextSegmentKey);
               Float64 location = ahead_bearing_offset - next_start_end_dist;
               if ( !IsZero(next_start_end_dist) )
               {
                  mbrID++;
                  location = 0;
               }


               // This is an interior pier and the segment connection type is one of the "closure joint" types
               // The closure joint is assumed to be cast with the diaphragm
               CComBSTR bstrStageAhead = bstrDiaphragmStage;

               CComPtr<IPointLoad> aheadLoad;
               aheadLoad.CoCreateInstance(CLSID_PointLoad);
               aheadLoad->put_MemberType(mtSuperstructureMember);
               aheadLoad->put_MemberID(mbrID);
               aheadLoad->put_Location(location);
               aheadLoad->put_Fy(aheadSide.P);
               aheadLoad->put_Mz(aheadSide.M);

               ptLoadItem.Release();
               pointLoads->Add(bstrStageAhead,bstrLoadGroup,aheadLoad,&ptLoadItem);
               SaveOverhangPointLoads(CSegmentKey(girderKey, 0), analysisType, bstrStageAhead, bstrLoadGroup, aheadSide.P, 0);
            }
         }
      } // next pier
   } // next group
}

void CGirderModelManager::ApplyIntermediateDiaphragmLoads( ILBAMModel* pLBAMModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdr) const
{
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE_NOCHECK(IPointOfInterest,pPoi);

   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();

   CComBSTR bstrLoadGroup = GetLoadGroupName(pgsTypes::pftGirder);

   CComPtr<IPointLoads> pointLoads;
   pLBAMModel->get_PointLoads(&pointLoads);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(gdr, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);

      // First apply precast diaphragm loads
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         IntervalIndexType intervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         CComBSTR bstrStage     = GetLBAMStageName(intervalIdx);

         std::vector<DiaphragmLoad> loads;
         GetPrecastDiaphragmLoads(segmentKey, &loads);

         std::vector<DiaphragmLoad>::iterator iter(loads.begin());
         std::vector<DiaphragmLoad>::iterator end(loads.end());
         for ( ; iter != end; iter++ )
         {
            DiaphragmLoad& rload = *iter;

            Float64 P   = rload.Load;

            MemberType mbrType;
            MemberIDType mbrID;
            Float64 loc;
            GetLoadPosition(pLBAMModel,segmentKey,rload.Loc,false,&mbrType,&mbrID,&loc);

            CComPtr<IPointLoad> load;
            load.CoCreateInstance(CLSID_PointLoad);
            load->put_MemberType(mbrType);
            load->put_MemberID(mbrID);
            load->put_Location(loc);
            load->put_Fy(P);

            CComPtr<IPointLoadItem> item;
            pointLoads->Add(bstrStage , bstrLoadGroup, load, &item);
         } // next load
      } // next segment

      // Next apply cast-in-place diaphragm loads
      CComBSTR bstrLoadGroup( GetLoadGroupName(pgsTypes::pftDiaphragm) ); 
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         CSpanKey spanKey(spanIdx,girderKey.girderIndex);

         std::vector<DiaphragmLoad> loads;
         GetIntermediateDiaphragmLoads(spanKey, &loads);

         for(const auto& load : loads)
         {
            Float64 P   = load.Load;
            Float64 loc = load.Loc;

            pgsPointOfInterest poi = pPoi->ConvertSpanPointToPoi(spanKey,loc);

            CComBSTR bstrStage = GetLBAMStageName(castDiaphragmIntervalIdx);

            CComPtr<IPointLoad> load;
            load.CoCreateInstance(CLSID_PointLoad);
            load->put_MemberType(mtSpan);
            load->put_MemberID(spanKey.spanIndex);
            load->put_Location(loc);
            load->put_Fy(P);

            CComPtr<IPointLoadItem> item;
            pointLoads->Add(bstrStage , bstrLoadGroup, load, &item);
         } // next load
      }
   } // next group
}

void CGirderModelManager::GetPostTensionDeformationLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,std::vector<PostTensionStrainLoad>& strainLoads) const
{
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IGirderTendonGeometry,pTendonGeometry);
   GET_IFACE(ILosses,pLosses);

   strainLoads.clear();

   GET_IFACE(IGirder,pIGirder);
   WebIndexType nWebs = pIGirder->GetWebCount(girderKey);

   // NOTE: If something other than Pj - Avg Friction - Avg Anchor Set is used for equivalent tendon forces, 
   // make the corresponding changes in the TimeStepLossEngineer
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);
   Float64 Apt = pTendonGeometry->GetGirderTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
   Float64 Pj = pTendonGeometry->GetPjack(girderKey,ductIdx);

#if defined USE_AVERAGE_TENDON_FORCE
   Float64 dfpF = pLosses->GetAverageFrictionLoss(girderKey,ductIdx);
   Float64 dfpA = pLosses->GetAverageAnchorSetLoss(girderKey,ductIdx);
   Float64 P1 = Pj - Apt*(dfpF + dfpA);
   Float64 P2 = P1;
#endif

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPoi);

   // Use the raw duct input data to get the location along the girder of the low and high points
   // since POIs aren't specifically stored at this locations. Use the IGirderTendonGeometry methods
   // to get the vertical position of the tendon with respect to the CG of the section. The
   // methods on the IGirderTendonGeometry interface account for the offset of the tendon within the duct.
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
	const CSplicedGirderData*  pGirder = pGroup->GetGirder(girderKey.girderIndex);

	SegmentIndexType nSegments = pGirder->GetSegmentCount();

	// Equivalent load for tendon path composed of compounded second-degree parabolas.
	// See "Modern Prestress Concrete", Libby, page 472-473, Eqn (10-7).
	SpanIndexType startSpanIdx = pGroup->GetPierIndex(pgsTypes::metStart);
	SpanIndexType endSpanIdx = pGroup->GetPierIndex(pgsTypes::metEnd) - 1;

	Float64 L = pBridge->GetSpanLength(startSpanIdx, girderKey.girderIndex);
	if (pGroup->GetPrevGirderGroup() == nullptr)
	{
	   // For the first group, span length is measured from the CL Bearing at the start abutment.
	   // Add the end distance at the start of the first segment so that "L" is measured from the
	   // start face of the girder
	   //
	   //  +---------------------------------------------/
	   //  |                                             \
		  //  +---------------------------------------------/
		  //  |  ^                                  ^
		  //  |  |<-------------------------------->|
		  //  |     L from GetSpanLength            |
		  //  |                                     |
		  //  |<----------------------------------->|
		  //     We want this L
	   ATLASSERT(pGroup->GetIndex() == 0);
	   ATLASSERT(startSpanIdx == 0);
	   Float64 endDist = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, 0));
	   L += endDist;
	}
	else
	{
	   //  \------+.....+---------------------------------------------/
	   //  /      |     |                                             \
		//  \------+.....+---------------------------------------------/
		//            ^                                      ^
		//            |  L measured from CL Pier             |
		//            |<------------------------------------>|
		//               |<--------------------------------->| we want this L
	   Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, 0));
	   Float64 endDist = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, 0));
	   Float64 offset = brgOffset - endDist;
	   L -= offset;
	}

	if (startSpanIdx == endSpanIdx)
	{
	   // This is a single span girder

	   if (pGroup->GetNextGirderGroup() == nullptr)
	   {
		  //  +-----------------------------------------+
		  //  |                                         |
		  //  +-----------------------------------------+
		  //  |  ^                                  ^   |
		  //  |  |<-------------------------------->|   |
		  //  |     L from GetSpanLength                |
		  //  |                                         |
		  //  |<--------------------------------------->|
		  //     We want this L
		  Float64 endDist = pBridge->GetSegmentEndEndDistance(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, nSegments - 1));
		  L += endDist;
	   }
	   else
	   {
		  //  \---------------------------------------------+.....+-------/
		  //  /                                             |     |       \
        //  \---------------------------------------------+.....+-------/
        //            ^                                      ^
        //            |  L measured from CL Pier             |
        //            |<------------------------------------>|
        //            |<--------------------------------->| we want this L
        Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, nSegments - 1));
		  Float64 endDist = pBridge->GetSegmentEndEndDistance(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, nSegments - 1));
		  Float64 offset = brgOffset - endDist;
		  L -= offset;
	   }
	}

	GET_IFACE(IMaterials, pMaterials);
	GET_IFACE(ISectionProperties, pSectProps);

	PoiList vPoi;
	pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), &vPoi);
	auto iter1(vPoi.begin());
	auto iter2(iter1 + 1);
	auto end(vPoi.end());
	for (; iter2 != end; iter1++, iter2++)
	{
	   const pgsPointOfInterest& poi1 = *iter1;
	   const pgsPointOfInterest& poi2 = *iter2;

	   if (poi1.AtSamePlace(poi2))
	   {
		  // don't continue if the POIs are at the same location... the loading will be applied over a distance
		  // of zero so the effect is zero
		  continue;
	   }

	   if (!pTendonGeometry->IsOnDuct(poi1, ductIdx) || !pTendonGeometry->IsOnDuct(poi2,ductIdx))
	   {
		  // if POIs aren't on the duct, there isn't a loading
		  continue;
	   }

      CSpanKey spanKey1;
      Float64 Xspan1;
      pPoi->ConvertPoiToSpanPoint(poi1, &spanKey1, &Xspan1);

      CSpanKey spanKey2;
      Float64 Xspan2;
      pPoi->ConvertPoiToSpanPoint(poi2, &spanKey2, &Xspan2);

      if (spanKey1.spanIndex == spanKey2.spanIndex && IsEqual(Xspan1, Xspan2))
      {
         // the POI are at the same place
         continue;
      }

#if !defined USE_AVERAGE_TENDON_FORCE
      Float64 dfpF1 = pLosses->GetGirderTendonFrictionLoss(poi1,ductIdx);
      Float64 dfpA1 = pLosses->GetGirderTendonAnchorSetLoss(poi1,ductIdx);
      Float64 P1 = Pj - Apt*(dfpF1 + dfpA1);

      Float64 dfpF2 = pLosses->GetGirderTendonFrictionLoss(poi2,ductIdx);
      Float64 dfpA2 = pLosses->GetGirderTendonAnchorSetLoss(poi2,ductIdx);
      Float64 P2 = Pj - Apt*(dfpF2 + dfpA2);
#endif // !USE_AVERAGE_TENDON_FORCE

      const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
      const CSegmentKey& segmentKey2(poi2.GetSegmentKey());

      // make sure span location 1 comes before location 2
      ATLASSERT((spanKey1.spanIndex == spanKey2.spanIndex && Xspan1 <= Xspan2) || spanKey1.spanIndex < spanKey2.spanIndex);

      Float64 eccX, eccY;
      pTendonGeometry->GetGirderTendonEccentricity(stressTendonIntervalIdx,poi1,ductIdx, &eccX, &eccY);
      Float64 M1 = -P1*eccY;

      pTendonGeometry->GetGirderTendonEccentricity(stressTendonIntervalIdx,poi2,ductIdx, &eccX, &eccY);
      Float64 M2 = -P2*eccY;

      CClosureKey closureKey1;
      bool bIsInClosure1 = pPoi->IsInClosureJoint(poi1,&closureKey1);
      
      CClosureKey closureKey2;
      bool bIsInClosure2 = pPoi->IsInClosureJoint(poi2,&closureKey2);

      Float64 E1 = (bIsInClosure1 ? pMaterials->GetClosureJointAgeAdjustedEc(closureKey1,stressTendonIntervalIdx) : pMaterials->GetSegmentAgeAdjustedEc(segmentKey1,stressTendonIntervalIdx));
      Float64 E2 = (bIsInClosure2 ? pMaterials->GetClosureJointAgeAdjustedEc(closureKey2,stressTendonIntervalIdx) : pMaterials->GetSegmentAgeAdjustedEc(segmentKey2,stressTendonIntervalIdx));

      Float64 A1 = pSectProps->GetAg(stressTendonIntervalIdx,poi1);
      Float64 A2 = pSectProps->GetAg(stressTendonIntervalIdx,poi2);

      ATLASSERT(!IsZero(E1*A1));
      ATLASSERT(!IsZero(E2*A2));

      Float64 e1 = -P1/(E1*A1);
      Float64 e2 = -P2/(E2*A2);

      Float64 I1 = pSectProps->GetIxx(stressTendonIntervalIdx,poi1);
      Float64 I2 = pSectProps->GetIxx(stressTendonIntervalIdx,poi2);

      ATLASSERT(!IsZero(E1*I1));
      ATLASSERT(!IsZero(E2*I2));

      Float64 r1 = M1/(E1*I1);
      Float64 r2 = M2/(E2*I2);

#if defined _DEBUG
      strainLoads.push_back(PostTensionStrainLoad(spanKey1.spanIndex, spanKey2.spanIndex, Xspan1, Xspan2, e1, e2, r1, r2));
#else
      strainLoads.emplace_back(spanKey1.spanIndex, spanKey2.spanIndex, Xspan1, Xspan2, e1, e2, r1, r2);
#endif
   }
}

PoiIDType CGirderModelManager::AddPointOfInterest(CGirderModelData* pModelData,const pgsPointOfInterest& poi) const
{
   // maps a PGSuper poi into, and creates, an LBAM poi
   ATLASSERT(pModelData->m_Model != nullptr || pModelData->m_ContinuousModel != nullptr);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   MemberIDType mbrID = GetFirstSuperstructureMemberID(segmentKey);

   PoiIDType poiID = (m_NextPoi)++;

   GET_IFACE(IBridge,pBridge);

   Float64 location       = poi.GetDistFromStart();
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_dist     = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_dist       = pBridge->GetSegmentEndEndDistance(segmentKey);

   CClosureKey closureKey;

   // Closure POI's and POIs in the CIP diaphragms at boundary piers are between groups are actually beyond the end of the segment
   // adjust the member ID and the location
   GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
   if ( location < 0 )
   {
      // location is before the start of the segment.
      // this would be a case when the POI is in a CIP intermediate diaphragm at a
      // boundary pier
      Float64 brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
      mbrID--;
      location = brg_offset - start_dist + poi.GetDistFromStart();
   }
   else if ( pPoi->IsInClosureJoint(poi,&closureKey) || pPoi->IsInBoundaryPierDiaphragm(poi) )
   {
      if ( poi.GetDistFromStart() < 0 )
      {
         if ( !IsZero(start_dist) )
         {
            mbrID--;
         }

         location = 0.0;
      }
      else
      {
         IndexType nSSMbrs = GetSuperstructureMemberCount(segmentKey);
         mbrID += (MemberIDType)nSSMbrs;
         location = poi.GetDistFromStart() - segment_length;
      }
   }
   else if ( location <= start_dist && !IsZero(start_dist) && poi.IsTenthPoint(POI_ERECTED_SEGMENT) != 1 )
   {
      // POI is before the starting CL Bearing point and there is an overhang member
      // no adjustments needed
   }
   else if ( segment_length-end_dist <= location+TOLERANCE && !IsZero(end_dist) && poi.IsTenthPoint(POI_ERECTED_SEGMENT) != 11 )
   {
      // POI is after the ending CL Bearing point
      // This moves the POI onto the next superstructure member
      if ( !IsZero(start_dist) )
      {
         mbrID++; // move to the SSMBR for the main portion of the segment
      }

      mbrID++; // move to the next superstructure member (the overhang member for the right end of the segment)

      if ( ::IsLT(segment_length,location) )
      {
         mbrID++;
         location -= (segment_length);
      }
      else
      {
         location -= (segment_length - end_dist);
      }
   }
   else
   {
      // POI is between CL Bearing points... adjust the location
      // so that it is measured from the CL Bearing at the left end of the segment
      if ( !IsZero(start_dist) )
      {
         mbrID++;
         location -= start_dist;
      }
   }

   location = IsZero(location) ? 0.0 : location;

   ATLASSERT(0.0 <= location);

   // Create a LBAM POI
   CComPtr<IPOI> objPOI;
   objPOI.CoCreateInstance(CLSID_POI);
   objPOI->put_ID(poiID);
   objPOI->put_MemberType(mtSuperstructureMember);
   objPOI->put_MemberID(mbrID);
   objPOI->put_Location(location); // distance from start bearing

   // Assign stress points to the POI for each stage
   // each stage will have three stress points. Bottom of Girder, Top of Girder, Top of Slab
   // for stages where the bridge doesn't have a slab on it, the stress point coefficients will be zero
   CComPtr<IPOIStressPoints> objPOIStressPoints;
   objPOI->get_POIStressPoints(&objPOIStressPoints);

   CComPtr<IStages> stages;
   if ( pModelData->m_Model )
   {
      pModelData->m_Model->get_Stages(&stages);
   }
   else
   {
      pModelData->m_ContinuousModel->get_Stages(&stages);
   }

   CComPtr<IEnumStage> enumStages;
   stages->get__EnumElements(&enumStages);

   CComPtr<IStage> stage;
   while ( enumStages->Next(1,&stage,nullptr) != S_FALSE )
   {
      AddPoiStressPoints(poi,stage,objPOIStressPoints);
      stage.Release();
   }

   // Add the LBAM POI to the LBAM
   if ( pModelData->m_Model )
   {
      CComPtr<IPOIs> pois;
      pModelData->m_Model->get_POIs(&pois);
      pois->Add(objPOI);
   }

   if ( pModelData->m_ContinuousModel )
   {
      CComPtr<IPOIs> pois;
      pModelData->m_ContinuousModel->get_POIs(&pois);
      pois->Add(objPOI);
   }

   // Record the LBAM poi in the POI map
   pModelData->PoiMap.AddMap( poi, poiID );

#if defined _DEBUG
   {
   CComPtr<ISuperstructureMembers> ssmbrs;
   if ( pModelData->m_Model )
   {
      pModelData->m_Model->get_SuperstructureMembers(&ssmbrs);
   }
   else
   {
      pModelData->m_ContinuousModel->get_SuperstructureMembers(&ssmbrs);
   }

   CComPtr<ISuperstructureMember> ssmbr;
   ssmbrs->get_Item(mbrID,&ssmbr);

   Float64 Lssmbr; // length of superstructure member
   ssmbr->get_Length(&Lssmbr);
   ATLASSERT(IsGE(0.0,location) && IsLE(location,Lssmbr));
   }
#endif

   return poiID;
}

void CGirderModelManager::AddPoiStressPoints(const pgsPointOfInterest& poi,IStage* pStage,IPOIStressPoints* pPOIStressPoints) const
{
   GET_IFACE(ISectionProperties,pSectProp);

   CComBSTR bstrStage;
   pStage->get_Name(&bstrStage);
   IntervalIndexType intervalIdx = GetIntervalFromLBAMStageName(poi,bstrStage);

   CComPtr<IStressPoints> leftStressPoints;
   leftStressPoints.CoCreateInstance(CLSID_StressPoints);

   CComPtr<IStressPoints> rightStressPoints;
   rightStressPoints.CoCreateInstance(CLSID_StressPoints);


   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

      Float64 Ca, Cbx, Cby;
      pSectProp->GetStressCoefficients(intervalIdx, poi, stressLocation, nullptr, &Ca, &Cbx, &Cby);

      CComPtr<IStressPoint> stressPoint;
      stressPoint.CoCreateInstance(CLSID_StressPoint);

      stressPoint->put_Sa(Ca);
      stressPoint->put_Sm(Cbx);

      leftStressPoints->Add(stressPoint);
      rightStressPoints->Add(stressPoint);
   }

   pPOIStressPoints->Insert(bstrStage,leftStressPoints,rightStressPoints);
}

///////////////////////////
void CGirderModelManager::GetVehicularLoad(ILBAMModel* pModel,LiveLoadModelType llType,VehicleIndexType vehicleIdx,IVehicularLoad** pVehicle) const
{
   CComPtr<ILiveLoad> live_load;
   pModel->get_LiveLoad(&live_load);

   CComPtr<ILiveLoadModel> liveload_model;
   switch(llType)
   {
      case lltDesign:
         live_load->get_Design(&liveload_model);
         break;

      case lltPermit:
         live_load->get_Permit(&liveload_model);
         break;

      case lltPedestrian:
         live_load->get_Pedestrian(&liveload_model);
         break;

      case lltDeflection:
         live_load->get_Deflection(&liveload_model);
         break;

      case lltFatigue:
         live_load->get_Fatigue(&liveload_model);
         break;

      case lltSpecial:
         live_load->get_Special(&liveload_model);
         break;

      case lltLegalRoutineRating:
         live_load->get_LegalRoutineRating(&liveload_model);
         break;

      case lltLegalSpecialRating:
         live_load->get_LegalSpecialRating(&liveload_model);
         break;

      case lltLegalEmergencyRating:
         live_load->get_LegalEmergencyRating(&liveload_model);
         break;

      case lltPermitRoutineRating:
         live_load->get_PermitRoutineRating(&liveload_model);
         break;

      case lltPermitSpecialRating:
         live_load->get_PermitSpecialRating(&liveload_model);
         break;

      case lltNone:
         *pVehicle = nullptr;
         return;

     default:
        ATLASSERT(false); // is there a new load?
        *pVehicle = nullptr;
        return;
   }

   CComPtr<IVehicularLoads> vehicles;
   liveload_model->get_VehicularLoads(&vehicles);

   vehicles->get_Item(vehicleIdx,pVehicle);
}

void CGirderModelManager::CreateAxleConfig(ILBAMModel* pModel,ILiveLoadConfiguration* pConfig,AxleConfiguration* pAxles) const
{
   pAxles->clear();

   LiveLoadModelType llType;
   pConfig->get_LiveLoadModel(&llType);

   CComPtr<IVehicularLoad> pVehicle;
   VehicleIndexType vehicleIdx;
   pConfig->get_VehicleIndex(&vehicleIdx);
   GetVehicularLoad(pModel,llType,vehicleIdx,&pVehicle);

   if ( !pVehicle )
   {
      return;
   }

   TruckDirectionType direction;
   pConfig->get_TruckDirection(&direction);
   Float64 sign = (direction == ltdForward ? -1 : 1);

   // indices of inactive axles
   CComPtr<IIndexArray> axleConfig;
   pConfig->get_AxleConfig(&axleConfig);

   Float64 variable_axle_spacing;
   pConfig->get_VariableSpacing(&variable_axle_spacing);

   AxleIndexType variable_axle_index;
   pVehicle->get_VariableAxle(&variable_axle_index);

   CComPtr<IAxles> axles;
   pVehicle->get_Axles(&axles);

   // locate the axles relative to the front of the truck
   Float64 axleLocation = 0;

   AxleIndexType nAxles;
   axles->get_Count(&nAxles);

   if (nAxles == 0 )
   {
      return;
   }

   for ( AxleIndexType axleIdx = 0; axleIdx < nAxles; axleIdx++ )
   {
      CComPtr<IAxle> axle;
      axles->get_Item(axleIdx,&axle);

      Float64 spacing;
      axle->get_Spacing(&spacing);

      Float64 wgt;
      axle->get_Weight(&wgt);

      AxlePlacement placement;
      placement.Weight = wgt;
      placement.Location = axleLocation;

      CComPtr<IEnumIndexArray> enum_array;
      axleConfig->get__EnumElements(&enum_array);
      AxleIndexType value;
      while ( enum_array->Next(1,&value,nullptr) != S_FALSE )
      {
         if ( axleIdx == value )
         {
            // this axle was picked up, so zero out it's weight
            placement.Weight = 0;
            break;
         }
      }

      pAxles->push_back(placement);

      if ( variable_axle_index == axleIdx )
      {
         spacing = variable_axle_spacing;
      }

      axleLocation += sign*spacing;
   }

   // get the location of the pivot axle, from the front of the truck
   Float64 pivot_axle_location;
   pConfig->get_TruckPosition(&pivot_axle_location);

   AxleIndexType pivot_axle_index;
   pConfig->get_PivotAxleIndex(&pivot_axle_index);

   Float64 pivot_axle_offset = ((*pAxles)[pivot_axle_index]).Location;

   // locate the axles relative to the pivot axle, then move them to the truck location
   for(auto& placement : *pAxles)
   {
      placement.Location += pivot_axle_location - pivot_axle_offset;
   }
}

void CGirderModelManager::GetLoadGroupName(pgsTypes::StrandType strandType, CComBSTR& bstrLoadGroupX, CComBSTR& bstrLoadGroupY) const
{
   switch(strandType)
   {
   case pgsTypes::Straight:
      bstrLoadGroupX = CComBSTR(_T("Straight_Strands_My")); // Y-direction moment causes x-direction deflections
      bstrLoadGroupY = CComBSTR(_T("Straight_Strands_Mx")); // x-direction moment causes y-direction deflections
      break;

   case pgsTypes::Harped:
      bstrLoadGroupX = CComBSTR(_T("Harped_Strands_My"));
      bstrLoadGroupY = CComBSTR(_T("Harped_Strands_Mx"));
      break;

   case pgsTypes::Temporary:
      bstrLoadGroupX = CComBSTR(_T("Temporary_Strands_My"));
      bstrLoadGroupY = CComBSTR(_T("Temporary_Strands_Mx"));
      break;
   }

}

CComBSTR CGirderModelManager::GetLBAMStageName(IntervalIndexType intervalIdx) const
{
   std::_tostringstream os;
   os << _T("Interval_") << LABEL_INTERVAL(intervalIdx);
   CComBSTR strStageName(os.str().c_str());
   return strStageName;
}

IntervalIndexType CGirderModelManager::GetIntervalFromLBAMStageName(const pgsPointOfInterest& poi, BSTR bstrStage) const
{
   USES_CONVERSION;
   CString strName(OLE2T(bstrStage));
   CString strKey(_T("Interval_"));
   int count = strName.GetLength()-strKey.GetLength();
   IntervalIndexType intervalIdx = (IntervalIndexType)_ttol(strName.Right(count));
   intervalIdx--; // need to undo the LABEL_INTERVAL used when creating the stage name

   // Need to match intervals used when setting up the LBAM model
   // we use section properties for release for the first interval(which is erection)
   GET_IFACE(ILossParameters, pLossParams);
   if (pLossParams->GetLossMethod() != pgsTypes::TIME_STEP)
   {
      GET_IFACE(IIntervals, pIntervals);
      const auto& segmentKey(poi.GetSegmentKey());
      IntervalIndexType startIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
      if (intervalIdx == startIntervalIdx)
      {
         intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      }
   }

   return intervalIdx;
}

SupportIDType CGirderModelManager::GetPierID(PierIndexType pierIdx) const
{
   return (SupportIDType)pierIdx;
}

SupportIDType CGirderModelManager::GetTemporarySupportID(SupportIndexType tsIdx) const
{
   return (SupportIDType)(tsIdx + TEMPORARY_SUPPORT_ID_OFFSET);
}

SupportIndexType CGirderModelManager::GetTemporarySupportIndex(SupportIDType tsID) const
{
   return (SupportIndexType)(tsID - TEMPORARY_SUPPORT_ID_OFFSET);
}

void CGirderModelManager::GetPierTemporarySupportIDs(PierIndexType pierIdx,SupportIDType* pBackID,SupportIDType* pAheadID) const
{
   // These are the IDs for temporary supports used to maintain stability in the LBAM at
   // intermediate piers where there is a non-moment transfering boundary condition.
   *pBackID  = -((SupportIDType)pierIdx*10002); // when sorted, this will come before the id for ahead
   *pAheadID = -((SupportIDType)pierIdx*10000);
}

void CGirderModelManager::GetPierSupportIDs(const ReactionLocation& location, SupportIDType* pBackID, SupportIDType* pAheadID) const
{
   GET_IFACE(IBridge, pBridge);
   ATLASSERT(pBridge->IsBoundaryPier(location.PierIdx)); // must be a boundary pier
   if (pBridge->IsAbutment(location.PierIdx))
   {
      *pBackID = GetPierID(location.PierIdx);
      *pAheadID = *pBackID;
   }
   else
   {
      SupportIDType backID, aheadID;
      GetPierTemporarySupportIDs(location.PierIdx, &backID, &aheadID);

      PierIDType pierID = GetPierID(location.PierIdx);

      CSegmentKey backSegmentKey, aheadSegmentKey;
      pBridge->GetSegmentsAtPier(location.PierIdx, location.GirderKey.girderIndex, &backSegmentKey, &aheadSegmentKey);

      Float64 left_brg_offset = pBridge->GetSegmentEndBearingOffset(backSegmentKey);
      Float64 left_end_dist = pBridge->GetSegmentEndEndDistance(backSegmentKey);

      Float64 right_brg_offset = pBridge->GetSegmentStartBearingOffset(aheadSegmentKey);
      Float64 right_end_dist = pBridge->GetSegmentStartEndDistance(aheadSegmentKey);

      *pBackID = IsEqual(left_brg_offset,left_end_dist) ? pierID : backID;
      *pAheadID = IsEqual(right_brg_offset,right_end_dist) ? pierID : aheadID;
   }
}

//////////////////////////////////////////////////
// LLDF Support Methods
CGirderModelManager::SpanType CGirderModelManager::GetSpanType(const CSpanKey& spanKey,bool bContinuousAnalysis) const
{
   // Determine the type of span we have for the purpose of applying LLDF
   // to the LBAM model

   // Determine if there are cantilevers at the ends of the span.. cantilever cause inflection points
   GET_IFACE(IBridge, pBridge);
   bool bStartCantilever(false), bEndCantilever(false);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(grpIdx, &startSpanIdx, &endSpanIdx);
      if (spanKey.spanIndex == startSpanIdx)
      {
         // this is the first span in the group... if the pier doesn't have continuity check to see if there is a cantilever at the start of the span
         PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
         bool bContinuousLeft, bContinuousRight;
         pBridge->IsContinuousAtPier(startPierIdx, &bContinuousLeft, &bContinuousRight);

         bool bIntegralLeft, bIntegralRight;
         pBridge->IsIntegralAtPier(startPierIdx, &bIntegralLeft, &bIntegralRight);

         bool bContinuousStart = bContinuousLeft || bIntegralLeft;
         if (!bContinuousStart)
         {
            CSegmentKey segmentKey(grpIdx, spanKey.girderIndex, 0);
            bool _bEndCantilever;
            pBridge->ModelCantilevers(segmentKey, &bStartCantilever, &_bEndCantilever);
         }
      }

      if (spanKey.spanIndex == endSpanIdx)
      {
         // this is the last span in the group... see if there is a cantilever at the end of the span
         PierIndexType endPierIdx = (PierIndexType)(endSpanIdx+1);
         bool bContinuousLeft, bContinuousRight;
         pBridge->IsContinuousAtPier(endPierIdx, &bContinuousLeft, &bContinuousRight);

         bool bIntegralLeft, bIntegralRight;
         pBridge->IsIntegralAtPier(endPierIdx, &bIntegralLeft, &bIntegralRight);

         bool bContinuousEnd = bContinuousRight || bIntegralRight;
         if (!bContinuousEnd)
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, spanKey.girderIndex);
            CSegmentKey segmentKey(grpIdx, spanKey.girderIndex, nSegments - 1);
            bool _bStartCantilever;
            pBridge->ModelCantilevers(segmentKey, &_bStartCantilever, &bEndCantilever);
         }
      }

      if (startSpanIdx <= spanKey.spanIndex && spanKey.spanIndex <= endSpanIdx )
      {
         // we found our group... break
         break;
      }
   }

   if ( !bContinuousAnalysis )
   {
      // we are doing simple span analysis
      if (!bStartCantilever && !bEndCantilever)
      {
         return PinPin;
      }
      else if (bStartCantilever && !bEndCantilever)
      {
         return FixPin;
      }
      else if (!bStartCantilever && bEndCantilever)
      {
         return PinFix;
      }
      else
      {
         ATLASSERT(bStartCantilever && bEndCantilever);
         return FixFix;
      }
   }

   PierIndexType prev_pier = spanKey.spanIndex;
   PierIndexType next_pier = prev_pier + 1;

   bool bContinuousLeft, bContinuousRight;
   pBridge->IsContinuousAtPier(prev_pier,&bContinuousLeft,&bContinuousRight);

   bool bIntegralLeft, bIntegralRight;
   pBridge->IsIntegralAtPier(prev_pier,&bIntegralLeft,&bIntegralRight);

   bool bContinuousStart = bContinuousRight || bIntegralRight;
   
   pBridge->IsContinuousAtPier(next_pier,&bContinuousLeft,&bContinuousRight);
   pBridge->IsIntegralAtPier(next_pier,&bIntegralLeft,&bIntegralRight);

   bool bContinuousEnd = bContinuousLeft || bIntegralLeft;

   if ( bContinuousStart && bContinuousEnd )
   {
      return FixFix;
   }

   if ( bContinuousStart && !bContinuousEnd )
   {
      if (bEndCantilever)
      {
         return FixFix;
      }
      else
      {
         return FixPin;
      }
   }

   if ( !bContinuousStart && bContinuousEnd )
   {
      if (bStartCantilever)
      {
         return FixFix;
      }
      else
      {
         return PinFix;
      }
   }

   if ( !bContinuousStart && !bContinuousEnd )
   {
      if (bStartCantilever && bEndCantilever)
      {
         return FixFix;
      }
      else
      {
         return PinPin;
      }
   }

   ATLASSERT(false); // should never get here
   return PinPin;
}

void CGirderModelManager::AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gV,Float64 gR,
                                               Float64 gFM,Float64 gFV,Float64 gD, Float64 gPedes) const
{
   if ( IsZero(length) )
   {
      return;
   }

   CComPtr<IDistributionFactorSegment> dfSegment;
   dfSegment.CoCreateInstance(CLSID_DistributionFactorSegment);
   dfSegment->put_Length(length);

   CComPtr<IDistributionFactor> df;
   df.CoCreateInstance(CLSID_DistributionFactor);
   dfSegment->putref_DistributionFactor(df);

   df->SetG(gpM, gpM, // positive moment
            gnM, gnM, // negative moment
            gV,  gV,  // shear
            gD,  gD,  // deflections
            gR,  gR,  // reaction
            gpM, gpM, // rotation
            gFM, gFV, // fatigue
            gPedes    // pedestrian loading
            );

   factors->Add(dfSegment);
}

void CGirderModelManager::AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gVStart,Float64 gVEnd,Float64 gR,
                                               Float64 gFM,Float64 gFVStart,Float64 gFVEnd,Float64 gD, Float64 gPedes) const
{
   if ( IsZero(length) )
   {
      return;
   }

   CComPtr<ILinearDistributionFactorSegment> dfSegment;
   dfSegment.CoCreateInstance(CLSID_LinearDistributionFactorSegment);
   dfSegment->put_Length(length);

   CComPtr<IDistributionFactor> dfStart;
   dfStart.CoCreateInstance(CLSID_DistributionFactor);
   dfSegment->putref_DistributionFactor(dfStart);

   CComPtr<IDistributionFactor> dfEnd;
   dfEnd.CoCreateInstance(CLSID_DistributionFactor);
   dfSegment->putref_EndDistributionFactor(dfEnd);

   dfStart->SetG(gpM, gpM, // positive moment
                 gnM, gnM, // negative moment
                 gVStart,  gVStart,  // shear
                 gD,  gD,  // deflections
                 gR,  gR,  // reaction
                 gpM, gpM, // rotation
                 gFM, gFVStart, // fatigue
                 gPedes    // pedestrian loading
                 );


   dfEnd->SetG(gpM, gpM, // positive moment
                 gnM, gnM, // negative moment
                 gVEnd,  gVEnd,  // shear
                 gD,  gD,  // deflections
                 gR,  gR,  // reaction
                 gpM, gpM, // rotation
                 gFM, gFVEnd, // fatigue
                 gPedes    // pedestrian loading
                 );

   CComQIPtr<IDistributionFactorSegment> dfs(dfSegment);
   factors->Add(dfs);
}

IndexType CGirderModelManager::GetCfPointsInRange(IDblArray* cfLocs, Float64 spanStart, Float64 spanEnd, Float64* ptsInrg) const
{
   if ( cfLocs == nullptr )
   {
      return 0;
   }

   // we don't want to pick up cf points at the very ends
   const Float64 TOL=1.0e-07;
   spanStart+= TOL;
   spanEnd-=TOL;

   IndexType cf_cnt=0;

   // assumption here that the cfLocs are sorted
   CollectionIndexType siz;
   cfLocs->get_Count(&siz);
   for (CollectionIndexType ic = 0; ic < siz; ic++)
   {
      Float64 cfl;
      cfLocs->get_Item(ic,&cfl);
      if ( (spanStart < cfl) && (cfl < spanEnd) )
      {
         // cf is within span
         cf_cnt++;
         if (cf_cnt<3)
         {
            ptsInrg[cf_cnt-1] = cfl;
         }
         else
         {
            ATLASSERT(false);
            // "More than two contraflexure points in a Span - This should be impossible"
         }
      }
      
      // no use continuing loop if we are past the span end
      if (spanEnd < cfl)
      {
         break;
      }
   }


   return cf_cnt;
}

void CGirderModelManager::ApplyLLDF_PinPin(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetFullSpanLength(spanKey);

#if defined _DEBUG
   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanKey.spanIndex; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(spanKey.girderIndex, grpIdx, grpIdx, &vGirderKeys);
      ATLASSERT(vGirderKeys.size() == 1);
      const auto& thisGirderKey = vGirderKeys.front();
      span_start += pBridge->GetSpanLength(CSpanKey(i,thisGirderKey.girderIndex));
   }

   Float64 span_end = span_start + span_length;

   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest startPoi = pPoi->ConvertSpanPointToPoi(spanKey,span_start);
   pgsPointOfInterest endPoi   = pPoi->ConvertSpanPointToPoi(spanKey,span_end);
   CSegmentKey startSegmentKey = startPoi.GetSegmentKey();
   CSegmentKey endSegmentKey   = endPoi.GetSegmentKey();

   bool bStartCantilever, bEndCantilever, bDummy;
   pBridge->ModelCantilevers(startSegmentKey,&bStartCantilever,&bDummy);
   pBridge->ModelCantilevers(endSegmentKey, &bDummy, &bEndCantilever);

   Float64 cf_points_in_span[2];
   IndexType num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);
   if ( bStartCantilever || bEndCantilever )
   {
      // there should be no more than 2 contraflexure points if cantilevers are modeled
      ATLASSERT(num_cf_points_in_span <= 2);
   }
   else
   {
      // there shouldn't be any contraflexure points in a pin-pin span
      ATLASSERT(num_cf_points_in_span == 0);
   }
#endif

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);

   Float64 gpM = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanKey);
   Float64 gPedes = this->GetPedestrianLiveLoad(spanKey); // factor is magnitude of pedestrian live load

   Float64 gVStart,  gVMid,  gVEnd;
   Float64 gFVStart, gFVMid, gFVEnd;
   bool bUseLinearLLDF = false;
   bool bTaperStart = false;
   bool bTaperEnd   = false;

   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      Float64 skewFactor = pLLDF->GetSkewCorrectionFactorForShear(spanKey,pgsTypes::StrengthI);
      if ( !IsEqual(skewFactor,1.0) )
      {
         bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
         bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
         
         if ( bObtuseStart && !bObtuseEnd )
         {
            gVStart = gV;
            gVMid   = gV/skewFactor;
            gVEnd   = gV/skewFactor;

            gFVStart = gFV;
            gFVMid   = gFV/skewFactor;
            gFVEnd   = gFV/skewFactor;

            bUseLinearLLDF = true;
            bTaperStart = true;
         }
         else if ( !bObtuseStart && bObtuseEnd )
         {
            gVStart = gV/skewFactor;
            gVMid   = gV;
            gVEnd   = gV;

            gFVStart = gFV/skewFactor;
            gFVMid   = gFV;
            gFVEnd   = gFV;

            bUseLinearLLDF = true;
            bTaperEnd = true;
         }
         else if ( bObtuseStart && bObtuseEnd )
         {
            gVStart = gV;
            gVMid   = gV/skewFactor;
            gVEnd   = gV;

            gFVStart = gFV;
            gFVMid   = gFV/skewFactor;
            gFVEnd   = gFV;

            bUseLinearLLDF = true;
            bTaperStart = true;
            bTaperEnd = true;
         }
         else
         {
            ATLASSERT(!bObtuseStart && !bObtuseEnd);
            // neither corner is obtuse, but there is a skew correction factor
            // that means one corner is acute and the other is a right angle
            // the "spanning" effect of shear still applies. The shortest span
            // is from the obtuse corner to the right angle corner
            CComPtr<IAngle> objSkewAngle;
            pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
            Float64 skewAngle;
            objSkewAngle->get_Value(&skewAngle);

            if ( IsZero(skewAngle) )
            {
               // right angle is at the start of the span
               gVStart = gV;
               gVMid   = gV/skewFactor;
               gVEnd   = gV/skewFactor;

               gFVStart = gFV;
               gFVMid   = gFV/skewFactor;
               gFVEnd   = gFV/skewFactor;

               bUseLinearLLDF = true;
               bTaperStart = true;
            }
            else
            {
               objSkewAngle.Release();
               pBridge->GetPierSkew((PierIndexType)(spanKey.spanIndex+1),&objSkewAngle);
               objSkewAngle->get_Value(&skewAngle);
               ATLASSERT( IsZero(skewAngle) ); // if both piers have a skew angle of 0, the skew correction should be 1.0 and we should be here

               gVStart = gV/skewFactor;
               gVMid   = gV;
               gVEnd   = gV;

               gFVStart = gFV/skewFactor;
               gFVMid   = gFV;
               gFVEnd   = gFV;

               bUseLinearLLDF = true;
               bTaperEnd = true;
            }
         }
      }
   }

   if ( bUseLinearLLDF )
   {
      if ( bTaperStart && !bTaperEnd)
      {
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
      }
      else if ( bTaperEnd && !bTaperStart )
      {
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
      }
      else
      {
         ATLASSERT(bTaperStart && bTaperEnd);
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVMid,gR,gFM,gFVStart,gFVMid,gD,gPedes);
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVMid,gVEnd,gR,gFM,gFVMid,gFVEnd,gD,gPedes);
      }
   }
   else
   {
      AddDistributionFactors(distFactors,span_length,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
}

void CGirderModelManager::ApplyLLDF_PinFix(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetFullSpanLength(spanKey);

   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanKey.spanIndex; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(spanKey.girderIndex, grpIdx, grpIdx, &vGirderKeys);
      ATLASSERT(vGirderKeys.size() == 1);
      const auto& thisGirderKey = vGirderKeys.front();
      span_start += pBridge->GetSpanLength(i,thisGirderKey.girderIndex);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   IndexType num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

   if ( num_cf_points_in_span == 0 )
   {
      // the entire span is in positive bending under uniform load
      // assume contraflexure point a mid-span
      num_cf_points_in_span = 1;
      cf_points_in_span[0] = span_start + span_length/2;
   }

   // there should be only 1 contraflexure point for pinned-fixed
   ATLASSERT(num_cf_points_in_span == 1);
   Float64 seg_length_1 = cf_points_in_span[0] - span_start;
   Float64 seg_length_2 = span_end - cf_points_in_span[0];

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);

   // distribution factors from span
   Float64 gpM = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanKey);

   Float64 gPedes = this->GetPedestrianLiveLoad(spanKey); // factor is magnitude of pedestrian live load

   Float64 gVStart, gVMid, gVEnd;
   Float64 gFVStart, gFVMid, gFVEnd;
   Float64 gVCF; // df at contra-flexure point. used when the CF point is in the LLDF transition zone
   Float64 gFVCF;
   bool bUseLinearLLDF = false;
   bool bTaperStart = false;
   bool bTaperEnd   = false;

   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      Float64 skewFactor = pLLDF->GetSkewCorrectionFactorForShear(spanKey,pgsTypes::StrengthI);
      if ( !IsEqual(skewFactor,1.0) )
      {
         bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
         bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
         if ( bObtuseStart && !bObtuseEnd )
         {
            gVStart = gV;
            gVEnd   = gV/skewFactor;

            gFVStart = gFV;
            gFVEnd   = gFV/skewFactor;

            if ( seg_length_1 < span_length/2 )
            {
               Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
               gVCF = gVEnd*k;
               gFVCF = gFVEnd*k;
            }

            bUseLinearLLDF = true;
            bTaperStart = true;
         }
         else if ( bObtuseEnd && !bObtuseStart )
         {
            gVStart = gV/skewFactor;
            gVEnd   = gV;

            gFVStart = gFV/skewFactor;
            gFVEnd   = gFV;

            if ( span_length/2 < seg_length_1 )
            {
               Float64 k = ::LinInterp(seg_length_1 - span_length/2,1.0,skewFactor,span_length/2);
               gVCF = gVStart*k;
               gFVCF = gFVStart*k;
            }

            bUseLinearLLDF = true;
            bTaperEnd = true;
         }
         else if ( bObtuseStart && bObtuseEnd )
         {
            gVStart = gV;
            gVMid   = gV/skewFactor;
            gVEnd   = gV;

            gFVStart = gFV;
            gFVMid   = gFV/skewFactor;
            gFVEnd   = gFV;

            if ( seg_length_1 < span_length/2 )
            {
               Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
               gVCF = gVMid*k;
               gFVCF = gFVMid*k;
            }
            else if ( span_length/2 < seg_length_1 )
            {
               Float64 k = ::LinInterp(seg_length_1 - span_length/2,1.0,skewFactor,span_length/2);
               gVCF = gVMid*k;
               gFVCF = gFVMid*k;
            }

            bUseLinearLLDF = true;
            bTaperStart = true;
            bTaperEnd = true;
         }
         else
         {
            ATLASSERT(!bObtuseStart && !bObtuseEnd);
            // neither corner is obtuse, but there is a skew correction factor
            // that means one corner is acute and the other is a right angle
            // the "spanning" effect of shear still applies. The shortest span
            // is from the obtuse corner to the right angle corner
            CComPtr<IAngle> objSkewAngle;
            pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
            Float64 skewAngle;
            objSkewAngle->get_Value(&skewAngle);

            if ( IsZero(skewAngle) )
            {
               // right angle is at the start of the span
               gVStart = gV;
               gVMid   = gV/skewFactor;
               gVEnd   = gV/skewFactor;

               gFVStart = gFV;
               gFVMid   = gFV/skewFactor;
               gFVEnd   = gFV/skewFactor;

               bUseLinearLLDF = true;
               bTaperStart = true;
            }
            else
            {
               objSkewAngle.Release();
               pBridge->GetPierSkew((PierIndexType)(spanKey.spanIndex+1),&objSkewAngle);
               objSkewAngle->get_Value(&skewAngle);
               ATLASSERT( IsZero(skewAngle) ); // if both piers have a skew angle of 0, the skew correction should be 1.0 and we should be here

               gVStart = gV/skewFactor;
               gVMid   = gV;
               gVEnd   = gV;

               gFVStart = gFV/skewFactor;
               gFVMid   = gFV;
               gFVEnd   = gFV;

               bUseLinearLLDF = true;
               bTaperEnd = true;
            }
         }
      }
   }

   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType pierIdx = (PierIndexType)(spanKey.spanIndex + 1);
   pgsTypes::PierFaceType pierFace(spanKey.spanIndex == 0 && pierIdx != nPiers-1 ? pgsTypes::Ahead : pgsTypes::Back);
   if ( bUseLinearLLDF )
   {
      if ( bTaperStart && !bTaperEnd )
      {
         if ( span_length/2 < seg_length_1 )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
         }
         else
         {
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }

         // for the second part of the span, use the negative moment distribution factor that goes over the next pier
         gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);

         AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
      }
      else if ( bTaperEnd && !bTaperStart )
      {
         if ( span_length/2 < seg_length_1 )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);

            // for the second part of the span, use the negative moment distribution factor that goes over the next pier
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);

            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }
         else
         {
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

            // for the second part of the span, use the negative moment distribution factor that goes over the next pier
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

            AddDistributionFactors(distFactors,seg_length_2 - (span_length/2 - seg_length_1),gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
         }
      }
      else
      {
         ATLASSERT(bTaperStart && bTaperEnd);
         if ( span_length/2 < seg_length_1 )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVMid,gR,gFM,gFVStart,gFVMid,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVMid,gVCF,gR,gFM,gFVMid,gFVCF,gD,gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);
            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }
         else
         {
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVMid,gR,gFM,gFVCF,gFVMid,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVMid,gVEnd,gR,gFM,gFVMid,gFVEnd,gD,gPedes);
         }
      }
   }
   else
   {
      AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);

      // for the second part of the span, use the negative moment distribution factor that goes over the next pier
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);

      AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
}

void CGirderModelManager::ApplyLLDF_FixPin(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetFullSpanLength(spanKey);

   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanKey.spanIndex; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(spanKey.girderIndex, grpIdx, grpIdx, &vGirderKeys);
      ATLASSERT(vGirderKeys.size() == 1);
      const auto& thisGirderKey = vGirderKeys.front();
      span_start += pBridge->GetSpanLength(i,thisGirderKey.girderIndex);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   IndexType num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

   if ( num_cf_points_in_span == 0 )
   {
      // the entire span is in positive bending under uniform load
      // assume contraflexure point a mid-span
      num_cf_points_in_span = 1;
      cf_points_in_span[0] = span_start + span_length/2;
   }

   // there should be only 1 contraflexure point for pinned-fixed
   ATLASSERT(num_cf_points_in_span == 1);
   Float64 seg_length_1 = cf_points_in_span[0] - span_start;
   Float64 seg_length_2 = span_end - cf_points_in_span[0];

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);

   PierIndexType pierIdx = spanKey.spanIndex;

   // distribution factors from span
   Float64 gpM = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Ahead); // DF over pier at start of span
   Float64 gV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::StrengthI);
   Float64 gR  =  99999999; // this parameter not should be used so use a value that is obviously wrong to easily detect bugs
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanKey);

   Float64 gPedes = this->GetPedestrianLiveLoad(spanKey); // factor is magnitude of pedestrian live load

   Float64 gVStart,  gVMid,  gVEnd;
   Float64 gFVStart, gFVMid, gFVEnd;
   Float64 gVCF; // df at contra-flexure point. used when the CF point is in the LLDF transition zone
   Float64 gFVCF;
   bool bUseLinearLLDF = false;
   bool bTaperStart = false;
   bool bTaperEnd = false;

   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      Float64 skewFactor = pLLDF->GetSkewCorrectionFactorForShear(spanKey,pgsTypes::StrengthI);
      if ( !IsEqual(skewFactor,1.0) )
      {
         bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
         bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
         if ( bObtuseStart && !bObtuseEnd )
         {
            gVStart = gV;
            gVEnd   = gV/skewFactor;

            gFVStart = gFV;
            gFVEnd   = gFV/skewFactor;

            if ( seg_length_1 < span_length/2 )
            {
               Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
               gVCF = gVEnd*k;
               gFVCF = gFVEnd*k;
            }

            bUseLinearLLDF = true;
            bTaperStart = true;
         }
         else if ( bObtuseEnd && !bObtuseStart )
         {
            gVStart = gV/skewFactor;
            gVEnd   = gV;

            gFVStart = gFV/skewFactor;
            gFVEnd   = gFV;

            if ( span_length/2 < seg_length_1 )
            {
               Float64 k = ::LinInterp(seg_length_1 - span_length/2,1.0,skewFactor,span_length/2);
               gVCF = gVStart*k;
               gFVCF = gFVStart*k;
            }

            bUseLinearLLDF = true;
            bTaperEnd = true;
         }
         else if ( bObtuseStart && bObtuseEnd )
         {
            gVStart = gV;
            gVMid   = gV/skewFactor;
            gVEnd   = gV;

            gFVStart = gFV;
            gFVMid   = gFV/skewFactor;
            gFVEnd   = gFV;

            if ( seg_length_1 < span_length/2 )
            {
               Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
               gVCF = gVMid*k;
               gFVCF = gFVMid*k;
            }
            else if ( span_length/2 < seg_length_1 )
            {
               Float64 k = ::LinInterp(seg_length_1 - span_length/2,1.0,skewFactor,span_length/2);
               gVCF = gVMid*k;
               gFVCF = gFVMid*k;
            }

            bUseLinearLLDF = true;
            bTaperStart = true;
            bTaperEnd = true;
         }
         else
         {
            ATLASSERT(!bObtuseStart && !bObtuseEnd);
            // neither corner is obtuse, but there is a skew correction factor
            // that means one corner is acute and the other is a right angle
            // the "spanning" effect of shear still applies. The shortest span
            // is from the obtuse corner to the right angle corner
            CComPtr<IAngle> objSkewAngle;
            pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
            Float64 skewAngle;
            objSkewAngle->get_Value(&skewAngle);

            if ( IsZero(skewAngle) )
            {
               // right angle is at the start of the span
               gVStart = gV;
               gVMid   = gV/skewFactor;
               gVEnd   = gV/skewFactor;

               gFVStart = gFV;
               gFVMid   = gFV/skewFactor;
               gFVEnd   = gFV/skewFactor;

               bUseLinearLLDF = true;
               bTaperStart = true;
            }
            else
            {
               objSkewAngle.Release();
               pBridge->GetPierSkew((PierIndexType)(spanKey.spanIndex+1),&objSkewAngle);
               objSkewAngle->get_Value(&skewAngle);
               ATLASSERT( IsZero(skewAngle) ); // if both piers have a skew angle of 0, the skew correction should be 1.0 and we should be here

               gVStart = gV/skewFactor;
               gVMid   = gV;
               gVEnd   = gV;

               gFVStart = gFV/skewFactor;
               gFVMid   = gFV;
               gFVEnd   = gFV;

               bUseLinearLLDF = true;
               bTaperEnd = true;
            }
         }
      }
   }

   pgsTypes::PierFaceType pierFace(pierIdx == 0 ? pgsTypes::Ahead : pgsTypes::Back);
   if ( bUseLinearLLDF )
   {
      if ( bTaperStart && !bTaperEnd )
      {
         if ( span_length/2 < seg_length_1 )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
         }
         else
         {
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }

         // for the second part of the span, use the negative moment distribution factor that goes over the next pier
         gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI,pierFace);

         AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
      }
      else if ( bTaperEnd && !bTaperStart )
      {
         if ( span_length/2 < seg_length_1 )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);

            // for the second part of the span, use the negative moment distribution factor that goes over the next pier
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);

            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }
         else
         {
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

            // for the second part of the span, use the negative moment distribution factor that goes over the next pier
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

            AddDistributionFactors(distFactors,seg_length_2 - (span_length/2 - seg_length_1),gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
         }
      }
      else
      {
         ATLASSERT(bTaperStart && bTaperEnd);
         if ( span_length/2 < seg_length_1 )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVMid,gR,gFM,gFVStart,gFVMid,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVMid,gVCF,gR,gFM,gFVMid,gFVCF,gD,gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);
            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }
         else
         {
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI, pierFace);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVMid,gR,gFM,gFVCF,gFVMid,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVMid,gVEnd,gR,gFM,gFVMid,gFVEnd,gD,gPedes);
         }
      }
   }
   else
   {
      AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);

      gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI); // DF in the span
      AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
}

void CGirderModelManager::ApplyLLDF_FixFix(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetFullSpanLength(spanKey);

   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanKey.spanIndex; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(spanKey.girderIndex, grpIdx, grpIdx, &vGirderKeys);
      ATLASSERT(vGirderKeys.size() == 1);
      const auto& thisGirderKey = vGirderKeys.front();
      span_start += pBridge->GetSpanLength(CSpanKey(i,thisGirderKey.girderIndex));
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   IndexType num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   Float64 gpM;
   Float64 gnM;
   Float64 gV; 
   Float64 gR;
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanKey);
   Float64 gPedes = GetPedestrianLiveLoad(spanKey); // factor is magnitude of pedestrian live load

   if ( num_cf_points_in_span == 0 )
   {
      // split span in half and use pier neg moment df for each half
      PierIndexType pierIdx = spanKey.spanIndex;

      gpM = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::StrengthI);
      gR  = 99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 gVStart, gVMid, gVEnd;
      Float64 gFVStart, gFVMid, gFVEnd;
      bool bUseLinearLLDF = false;
      bool bTaperStart = false;
      bool bTaperEnd = false;

      if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
      {
         Float64 skewFactor = pLLDF->GetSkewCorrectionFactorForShear(spanKey,pgsTypes::StrengthI);
         if ( !IsEqual(skewFactor,1.0) )
         {
            bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
            bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
            if ( bObtuseStart && !bObtuseEnd )
            {
               gVStart = gV;
               gVEnd   = gV/skewFactor;
               gFVStart = gFV;
               gFVEnd   = gFV/skewFactor;
               bUseLinearLLDF = true;
               bTaperStart = true;
            }
            else if ( bObtuseEnd && !bObtuseStart )
            {
               gVStart = gV/skewFactor;
               gVEnd   = gV;
               gFVStart = gFV/skewFactor;
               gFVEnd   = gFV;
               bUseLinearLLDF = true;
               bTaperEnd = true;
            }
            else if ( bObtuseStart && bObtuseEnd )
            {
               gVStart = gV;
               gVMid   = gV/skewFactor;
               gVEnd   = gV;

               gFVStart = gFV;
               gFVMid   = gFV/skewFactor;
               gFVEnd   = gFV;

               bUseLinearLLDF = true;
               bTaperStart = true;
               bTaperEnd = true;
            }
            else
            {
               ATLASSERT(!bObtuseStart && !bObtuseEnd);
               // neither corner is obtuse, but there is a skew correction factor
               // that means one corner is acute and the other is a right angle
               // the "spanning" effect of shear still applies. The shortest span
               // is from the obtuse corner to the right angle corner
               CComPtr<IAngle> objSkewAngle;
               pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
               Float64 skewAngle;
               objSkewAngle->get_Value(&skewAngle);

               if ( IsZero(skewAngle) )
               {
                  // right angle is at the start of the span
                  gVStart = gV;
                  gVMid   = gV/skewFactor;
                  gVEnd   = gV/skewFactor;

                  gFVStart = gFV;
                  gFVMid   = gFV/skewFactor;
                  gFVEnd   = gFV/skewFactor;

                  bUseLinearLLDF = true;
                  bTaperStart = true;
               }
               else
               {
                  objSkewAngle.Release();
                  pBridge->GetPierSkew((PierIndexType)(spanKey.spanIndex+1),&objSkewAngle);
                  objSkewAngle->get_Value(&skewAngle);
                  ATLASSERT( IsZero(skewAngle) ); // if both piers have a skew angle of 0, the skew correction should be 1.0 and we should be here

                  gVStart = gV/skewFactor;
                  gVMid   = gV;
                  gVEnd   = gV;

                  gFVStart = gFV/skewFactor;
                  gFVMid   = gFV;
                  gFVEnd   = gFV;

                  bUseLinearLLDF = true;
                  bTaperEnd = true;
               }
            }
         }
      }


      if ( bUseLinearLLDF )
      {
         if ( bTaperStart && !bTaperEnd )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD, gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
         }
         else if ( bTaperEnd && !bTaperStart )
         {
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD, gPedes);
         }
         else
         {
            ATLASSERT(bTaperStart && bTaperEnd);
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVMid,gR,gFM,gFVStart,gFVMid,gD, gPedes);
            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
            AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVMid,gVEnd,gR,gFM,gFVMid,gFVEnd,gD, gPedes);
         }
      }
      else
      {
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD, gPedes);
         
         gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
         AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      }
   }
   else if ( num_cf_points_in_span == 1 )
   {
      PierIndexType pierIdx = spanKey.spanIndex;
      gpM = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::StrengthI);
      gR  = 99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 seg_length_1 = cf_points_in_span[0] - span_start;
      Float64 seg_length_2 = span_end - cf_points_in_span[0];

      Float64 gVStart,  gVMid,  gVEnd;
      Float64 gFVStart, gFVMid, gFVEnd;
      Float64 gVCF; // df at contra-flexure point. used when the CF point is in the LLDF transition zone
      Float64 gFVCF;
      bool bUseLinearLLDF = false;
      bool bTaperStart = false;
      bool bTaperEnd = false;

      if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
      {
         Float64 skewFactor = pLLDF->GetSkewCorrectionFactorForShear(spanKey,pgsTypes::StrengthI);
         if ( !IsEqual(skewFactor,1.0) )
         {
            bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
            bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
            if ( bObtuseStart && !bObtuseEnd  )
            {
               gVStart = gV;
               gVEnd   = gV/skewFactor;

               gFVStart = gFV;
               gFVEnd   = gFV/skewFactor;

               if ( seg_length_1 < span_length/2 )
               {
                  Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
                  gVCF = gVEnd*k;
                  gFVCF = gFVEnd*k;
               }

               bUseLinearLLDF = true;
               bTaperStart = true;
            }
            else if ( bObtuseEnd && !bObtuseStart )
            {
               gVStart = gV/skewFactor;
               gVEnd   = gV;

               gFVStart = gFV/skewFactor;
               gFVEnd   = gFV;

               if ( span_length/2 < seg_length_1 )
               {
                  Float64 k = ::LinInterp(seg_length_1 - span_length/2,1.0,skewFactor,span_length/2);
                  gVCF = gVStart*k;
                  gFVCF = gFVStart*k;
               }

               bUseLinearLLDF = true;
               bTaperEnd = true;
            }
            else if ( bObtuseStart && bObtuseEnd )
            {
               gVStart = gV;
               gVMid   = gV/skewFactor;
               gVEnd   = gV;

               gFVStart = gFV;
               gFVMid   = gFV/skewFactor;
               gFVEnd   = gFV;

               if ( seg_length_1 < span_length/2 )
               {
                  Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
                  gVCF = gVMid*k;
                  gFVCF = gFVMid*k;
               }
               else if ( span_length/2 < seg_length_1 )
               {
                  Float64 k = ::LinInterp(seg_length_1 - span_length/2,1.0,skewFactor,span_length/2);
                  gVCF = gVMid*k;
                  gFVCF = gFVMid*k;
               }

               bUseLinearLLDF = true;
               bTaperStart = true;
               bTaperEnd = true;
            }
            else
            {
               ATLASSERT(!bObtuseStart && !bObtuseEnd);
               // neither corner is obtuse, but there is a skew correction factor
               // that means one corner is acute and the other is a right angle
               // the "spanning" effect of shear still applies. The shortest span
               // is from the obtuse corner to the right angle corner
               CComPtr<IAngle> objSkewAngle;
               pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
               Float64 skewAngle;
               objSkewAngle->get_Value(&skewAngle);

               if ( IsZero(skewAngle) )
               {
                  // right angle is at the start of the span
                  gVStart = gV;
                  gVMid   = gV/skewFactor;
                  gVEnd   = gV/skewFactor;

                  gFVStart = gFV;
                  gFVMid   = gFV/skewFactor;
                  gFVEnd   = gFV/skewFactor;

                  bUseLinearLLDF = true;
                  bTaperStart = true;
               }
               else
               {
                  objSkewAngle.Release();
                  pBridge->GetPierSkew((PierIndexType)(spanKey.spanIndex+1),&objSkewAngle);
                  objSkewAngle->get_Value(&skewAngle);
                  ATLASSERT( IsZero(skewAngle) ); // if both piers have a skew angle of 0, the skew correction should be 1.0 and we should be here

                  gVStart = gV/skewFactor;
                  gVMid   = gV;
                  gVEnd   = gV;

                  gFVStart = gFV/skewFactor;
                  gFVMid   = gFV;
                  gFVEnd   = gFV;

                  bUseLinearLLDF = true;
                  bTaperEnd = true;
               }
            }
         }
      }

      if ( bUseLinearLLDF )
      {
         if ( bTaperStart && !bTaperEnd )
         {
            if ( span_length/2 < seg_length_1 )
            {
               AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
               AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
            }
            else
            {
               AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
               AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
            }

            // for the second part of the span, use the negative moment distribution factor that goes over the next pier
            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);

            AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
         }
         else if ( bTaperEnd && !bTaperStart )
         {
            if ( span_length/2 < seg_length_1 )
            {
               AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);
               AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);

               // for the second part of the span, use the negative moment distribution factor that goes over the next pier
               gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);

               AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
            }
            else
            {
               AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

               // for the second part of the span, use the negative moment distribution factor that goes over the next pier
               gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
               AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

               AddDistributionFactors(distFactors,seg_length_2 - (span_length/2 - seg_length_1),gpM,gnM,gVStart,gVEnd,gR,gFM,gFVStart,gFVEnd,gD,gPedes);
            }
         }
         else 
         {
            ATLASSERT(bTaperStart && bTaperEnd);
            if ( seg_length_1 < span_length/2 )
            {
               AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);

               gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
               AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVMid,gR,gFM,gFVCF,gFVMid,gD,gPedes);
               AddDistributionFactors(distFactors,seg_length_2 - (span_length/2 - seg_length_1),gpM,gnM,gVMid,gVEnd,gR,gFM,gFVMid,gFVEnd,gD,gPedes);
            }
            else
            {
               AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gVStart,gVMid,gR,gFM,gFVStart,gFVMid,gD,gPedes);
               AddDistributionFactors(distFactors,seg_length_1 - span_length/2,gpM,gnM,gVMid,gVCF,gR,gFM,gFVMid,gFVCF,gD,gPedes);

               gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
               AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
            }
         }
      }
      else
      {
         AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);

         // for the second part of the span, use the negative moment distribution factor that goes over the next pier
         gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);

         AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      }
   }
   else
   {
      ATLASSERT(num_cf_points_in_span == 2);

      PierIndexType pierIdx = spanKey.spanIndex;
      gpM = pLLDF->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanKey,pgsTypes::StrengthI);
      gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 seg_length_1 = cf_points_in_span[0] - span_start;
      Float64 seg_length_2 = cf_points_in_span[1] - cf_points_in_span[0];
      Float64 seg_length_3 = span_end - cf_points_in_span[1];

      // below, when distributing the skew correction for shear amongst the segments
      // we are assuming that the CF points are on either side of mid-span
      ATLASSERT( cf_points_in_span[0] < span_start + span_length/2 );
      ATLASSERT( span_start + span_length/2 < cf_points_in_span[1]);

      Float64 gVStart,  gVMid, gVEnd;
      Float64 gFVStart, gFVMid, gFVEnd;
      Float64 gVCF; // df at contra-flexure point. used when the CF point is in the LLDF transition zone
      Float64 gFVCF;
      Float64 gVCF1, gVCF2;
      Float64 gFVCF1, gFVCF2;
      bool bUseLinearLLDF = false;
      bool bTaperStart = false;
      bool bTaperEnd = false;

      if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
      {
         Float64 skewFactor = pLLDF->GetSkewCorrectionFactorForShear(spanKey,pgsTypes::StrengthI);
         if ( !IsEqual(skewFactor,1.0) )
         {
            bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
            bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
            if ( bObtuseStart && !bObtuseEnd )
            {
               gVStart = gV;
               gVEnd   = gV/skewFactor;

               gFVStart = gFV;
               gFVEnd   = gFV/skewFactor;

               ATLASSERT( seg_length_1 < span_length/2 );
               Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
               gVCF = gVEnd*k;
               gFVCF = gFVEnd*k;

               bUseLinearLLDF = true;
               bTaperStart = true;
            }
            else if ( bObtuseEnd && !bObtuseStart )
            {
               gVStart = gV/skewFactor;
               gVEnd   = gV;

               gFVStart = gFV/skewFactor;
               gFVEnd   = gFV;

               ATLASSERT( span_length/2 < seg_length_1 + seg_length_2 );
               Float64 k = ::LinInterp(seg_length_1 + seg_length_2 - span_length/2,1.0,skewFactor,span_length/2);
               gVCF = gVStart*k;
               gFVCF = gFVStart*k;

               bUseLinearLLDF = true;
               bTaperEnd = true;
            }
            else if ( bObtuseStart && bObtuseEnd )
            {
               gVStart = gV;
               gVMid   = gV/skewFactor;
               gVEnd   = gV;

               gFVStart = gFV;
               gFVMid   = gFV/skewFactor;
               gFVEnd   = gFV;

               Float64 k = ::LinInterp(seg_length_1,skewFactor,1.0,span_length/2);
               gVCF1 = gVMid*k;
               gFVCF1 = gFVMid*k;

               k = ::LinInterp(seg_length_1 + seg_length_2 - span_length/2,1.0,skewFactor,span_length/2);
               gVCF2 = gVMid*k;
               gFVCF2 = gFVMid*k;

               bUseLinearLLDF = true;
               bTaperStart = true;
               bTaperEnd = true;
            }
            else
            {
               ATLASSERT(!bObtuseStart && !bObtuseEnd);
               // neither corner is obtuse, but there is a skew correction factor
               // that means one corner is acute and the other is a right angle
               // the "spanning" effect of shear still applies. The shortest span
               // is from the obtuse corner to the right angle corner
               CComPtr<IAngle> objSkewAngle;
               pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
               Float64 skewAngle;
               objSkewAngle->get_Value(&skewAngle);

               if ( IsZero(skewAngle) )
               {
                  // right angle is at the start of the span
                  gVStart = gV;
                  gVMid   = gV/skewFactor;
                  gVEnd   = gV/skewFactor;

                  gFVStart = gFV;
                  gFVMid   = gFV/skewFactor;
                  gFVEnd   = gFV/skewFactor;

                  bUseLinearLLDF = true;
                  bTaperStart = true;
               }
               else
               {
                  objSkewAngle.Release();
                  pBridge->GetPierSkew((PierIndexType)(spanKey.spanIndex+1),&objSkewAngle);
                  objSkewAngle->get_Value(&skewAngle);
                  ATLASSERT( IsZero(skewAngle) ); // if both piers have a skew angle of 0, the skew correction should be 1.0 and we should be here

                  gVStart = gV/skewFactor;
                  gVMid   = gV;
                  gVEnd   = gV;

                  gFVStart = gFV/skewFactor;
                  gFVMid   = gFV;
                  gFVEnd   = gFV;

                  bUseLinearLLDF = true;
                  bTaperEnd = true;
               }
            }
         }
      }

      if ( bUseLinearLLDF )
      {
         if ( bTaperStart && !bTaperEnd )
         {
            ATLASSERT( seg_length_1 < span_length/2 );
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
            
            gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 + seg_length_2 - span_length/2,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
            
            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
            AddDistributionFactors(distFactors,seg_length_3,gpM,gnM,gVEnd,gR,gFM,gFVEnd,gD,gPedes);
         }
         else if ( bTaperEnd && !bTaperStart )
         {
            ATLASSERT( span_length/2 < seg_length_1 + seg_length_2 );
            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);

            gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVStart,gR,gFM,gFVStart,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 + seg_length_2 - span_length/2,gpM,gnM,gVStart,gVCF,gR,gFM,gFVStart,gFVCF,gD,gPedes);
            
            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
            AddDistributionFactors(distFactors,seg_length_3,gpM,gnM,gVCF,gVEnd,gR,gFM,gFVCF,gFVEnd,gD,gPedes);
         }
         else
         {
            ATLASSERT(bTaperStart && bTaperEnd);

            AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gVStart,gVCF1,gR,gFM,gFVStart,gFVCF1,gD,gPedes);

            gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI);
            AddDistributionFactors(distFactors,span_length/2 - seg_length_1,gpM,gnM,gVCF1,gVMid,gR,gFM,gFVCF1,gFVMid,gD,gPedes);
            AddDistributionFactors(distFactors,seg_length_1 + seg_length_2 - span_length/2,gpM,gnM,gVMid,gVCF2,gR,gFM,gFVMid,gFVCF2,gD,gPedes);

            gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
            AddDistributionFactors(distFactors,seg_length_3,gpM,gnM,gVCF2,gVEnd,gR,gFM,gFVCF2,gFVEnd,gD,gPedes);
         }
      }
      else
      {
         AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
         
         gnM = pLLDF->GetNegMomentDistFactor(spanKey,pgsTypes::StrengthI);
         AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
         
         gnM = pLLDF->GetNegMomentDistFactorAtPier(spanKey.spanIndex+1,spanKey.girderIndex,pgsTypes::StrengthI,pgsTypes::Back);
         AddDistributionFactors(distFactors,seg_length_3,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      }
   }
}

void CGirderModelManager::ApplyLLDF_Support(const CSpanKey& spanKey,pgsTypes::MemberEndType endType,ISupports* supports,ITemporarySupports* tempSupports) const
{
   PierIndexType pierIdx = (endType == pgsTypes::metStart ? spanKey.spanIndex : spanKey.spanIndex+1);

   CComPtr<ISupport> support;
   supports->get_Item(pierIdx,&support);
   CComPtr<IDistributionFactor> df;
   support->get_DistributionFactor(&df);

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   GET_IFACE(IPointOfInterest, pPoi);

   // Reaction DF's are shear df's
   // shear factors can vary at each end of girder
   PoiList vPoi;
   pPoi->GetPointsOfInterest(spanKey, endType == pgsTypes::metEnd ? (POI_10L  | POI_SPAN) : (POI_0L | POI_SPAN), &vPoi);
   ATLASSERT(vPoi.size() == 1);

   const pgsPointOfInterest& poi(vPoi.front());

   Float64 gpmfat, gnmfat, gvfat;
   pLLDF->GetDistributionFactors(poi, pgsTypes::FatigueI, &gpmfat, &gnmfat, &gvfat);

   Float64 gpM = 99999999;
   Float64 gnM = 99999999;
   Float64 gV  = 99999999;
   Float64 gR  = pLLDF->GetDeflectionDistFactor(spanKey); // uniform distribution for reactions
   Float64 gF  = gvfat;
   Float64 gD  = gR; // uniform distribution for deflections (same as reactions)


   GET_IFACE(IBridge,pBridge);

   // For pedestrian loads - take average of loads from adjacent spans
   Float64 leftPedes(0.0), rightPedes(0.0);
   Int32 nls(0);
   if(0 < pierIdx)
   {
      SpanIndexType prevSpanIdx = (SpanIndexType)(pierIdx-1);
      GirderIndexType nGirders = pBridge->GetGirderCountBySpan(prevSpanIdx);
      GirderIndexType gdrIdx = Min(spanKey.girderIndex,nGirders-1);
      leftPedes = GetPedestrianLiveLoad(CSpanKey(prevSpanIdx,gdrIdx));
      nls++;
   }

   SpanIndexType nSpans = pBridge->GetSpanCount();
   if (pierIdx < nSpans)
   {
      SpanIndexType nextSpanIdx = (SpanIndexType)(pierIdx);
      GirderIndexType nGirders = pBridge->GetGirderCountBySpan(nextSpanIdx);
      GirderIndexType gdrIdx = Min(spanKey.girderIndex,nGirders-1);
      rightPedes = GetPedestrianLiveLoad(CSpanKey(nextSpanIdx,gdrIdx));
      nls++;
   }

   Float64 gPedes = (leftPedes+rightPedes)/nls;

   df->SetG(gpM, gpM, // positive moment
            gnM, gnM, // negative moment
            gV,  gV,  // shear
            gD,  gD,  // deflections
            gR,  gR,  // reaction
            gD,  gD,  // rotation
            gF,  gF,  // fatigue
            gPedes    // pedestrian
            );


   // For simple span connections at piers, temporary supports
   // are used to model the bearing reactions. Add LLDF to the
   // temporary supports so load combinations work correctly
   SupportIDType backID, aheadID;
   GetPierTemporarySupportIDs(pierIdx, &backID, &aheadID);
   CComPtr<ITemporarySupport> tempSupport;
   if (endType == pgsTypes::metEnd && SUCCEEDED(tempSupports->Find(backID, &tempSupport)))
   {
      df.Release();
      tempSupport->get_DistributionFactor(&df);
      df->SetG(gpM, gpM, // positive moment
         gnM, gnM, // negative moment
         gV, gV,  // shear
         gD, gD,  // deflections
         gR, gR,  // reaction
         gD, gD,  // rotation
         gF, gF,  // fatigue
         gPedes    // pedestrian
      );
   }

   tempSupport.Release();
   if (endType == pgsTypes::metStart && SUCCEEDED(tempSupports->Find(aheadID, &tempSupport)))
   {
      df.Release();
      tempSupport->get_DistributionFactor(&df);
      df->SetG(gpM, gpM, // positive moment
         gnM, gnM, // negative moment
         gV, gV,  // shear
         gD, gD,  // deflections
         gR, gR,  // reaction
         gD, gD,  // rotation
         gF, gF,  // fatigue
         gPedes    // pedestrian
      );
   }
}

/////////////

void CGirderModelManager::GetEngine(CGirderModelData* pModelData,bool bContinuous,ILBAMAnalysisEngine** pEngine) const
{
   CComPtr<IUnkArray> engines;
   pModelData->m_MinModelEnveloper->get_Engines(&engines);

   CollectionIndexType nEngines;
   engines->get_Count(&nEngines);

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   CComPtr<IUnknown> unk_engine;
   CollectionIndexType engineIdx = 0;
   if ( bContinuous && 1 < nSpans && 1 < nEngines )
   {
      engineIdx = 1;
   }

   engines->get_Item(engineIdx,&unk_engine);
   CComQIPtr<ILBAMAnalysisEngine> engine(unk_engine);

#if defined _DEBUG
   CComPtr<IUnknown> unk2;
   CComPtr<IUnkArray> engines2;
   pModelData->m_MaxModelEnveloper->get_Engines(&engines2);
   engines2->get_Item(engineIdx,&unk2);
   ATLASSERT(unk2.IsEqualObject(unk_engine));
#endif

   *pEngine = engine;
   (*pEngine)->AddRef();
}

void CGirderModelManager::CheckGirderEndGeometry(IBridge* pBridge,const CGirderKey& girderKey) const
{
   CSegmentKey segmentKey(girderKey,0);
   Float64 s_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   segmentKey.segmentIndex = nSegments-1;
   Float64 e_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
   if (s_end_size < 0.0 || e_end_size < 0.0)
   {
      std::_tostringstream os;
      os<<"Error - The end of the girder is located off of the bearing at the ";
      if (s_end_size < 0.0 && e_end_size < 0.0)
      {
         os<<"left and right ends";
      }
      else if (s_end_size < 0.0)
      {
         os<<"left end";
      }
      else
      {
         os<<"right end";
      }

      GET_IFACE(IDocumentType,pDocType);
      if ( pDocType->IsPGSuperDocument() )
      {
         os<<" of Girder "<<LABEL_GIRDER(girderKey.girderIndex)<<" in Span "<< LABEL_SPAN(girderKey.groupIndex) <<". \r\nThis problem can be resolved by increasing the girder End Distance in the Connection library, or by decreasing the skew angle of the girder with respect to the pier.";
      }
      else
      {
         os<<" of Girder "<<LABEL_GIRDER(girderKey.girderIndex)<<" in Group "<< LABEL_GROUP(girderKey.groupIndex) <<". \r\nThis problem can be resolved by increasing the girder End Distance in the Connection library, or by decreasing the skew angle of the girder with respect to the pier.";
      }

      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,os.str().c_str());

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->Add(pStatusItem);

      os<<"\r\nSee the Status Center for Details";

      THROW_UNWIND(os.str().c_str(),-1);
   }

   // Check that the slab offset is >= gross slab depth + fillet
   if (pBridge->GetDeckType() != pgsTypes::sdtNone && pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
         Float64 fillet = pBridge->GetFillet();

      PierIndexType startPierIdx,endPierIdx;
      pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);

         Float64 startA = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart);
         Float64 endA   = pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd);

         Float64 dSlab = pBridge->GetGrossSlabDepth(pgsPointOfInterest(segmentKey,0.0));
      if (startA - dSlab - fillet < -TOLERANCE || endA - dSlab - fillet < -TOLERANCE)
         {
            std::_tostringstream os;
         os << "Error - The slab offset must be greater than or equal to the gross slab depth plus the fillet depth for "
            << " Girder " << LABEL_GIRDER(girderKey.girderIndex) << " in Span " << LABEL_SPAN(girderKey.groupIndex) << ". \r\nThis problem can be resolved by increasing the girder's slab offset or decreasing the fillet depth.";

            pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,os.str().c_str());
    
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            pStatusCenter->Add(pStatusItem);

         os << "\r\nSee the Status Center for Details";

            THROW_UNWIND(os.str().c_str(),-1);
         }
      }
}

CComBSTR CGirderModelManager::GetLoadGroupName(pgsTypes::ProductForceType pfType) const
{
   return m_ProductLoadMap.GetGroupLoadName(pfType);
}

pgsTypes::ProductForceType CGirderModelManager::GetProductForceType(const CComBSTR& bstrLoadGroup) const
{
   return m_ProductLoadMap.GetProductForceType(bstrLoadGroup);
}

void CGirderModelManager::GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoads,std::vector<ClosureJointLoad>* pClosureJointLoads) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();

   // get all the cross section changes
   GET_IFACE(IPointOfInterest,pPoi);
   PoiList xsPOI;
   pPoi->GetPointsOfInterest(segmentKey, POI_SECTCHANGE, &xsPOI, POIFIND_OR);
   if ( variationType == pgsTypes::svtParabolic )
   {
      // single parabola
      GET_IFACE(IBridge,pBridge);
      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      Float64 Lleft  = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);
      Float64 Lright = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic);
      Float64 L = Ls - Lleft - Lright; // length of the non-prismatic portion of the segment
      IndexType nSections = 10; // break into nSections along the parabolic taper
      for ( IndexType i = 0; i < nSections; i++ )
      {
         Float64 X = Lleft + i*L/nSections;
         pgsPointOfInterest poi = pPoi->GetPointOfInterest(segmentKey,X);
         xsPOI.push_back(poi);
      }
      pPoi->SortPoiList(&xsPOI);
   }
   else if ( variationType == pgsTypes::svtDoubleParabolic )
   {
      // double parabola
      IndexType nSections = 10; // break into nSections along the parabolic taper

      // left parabola
      Float64 Lleft  = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);
      Float64 Lt     = pSegment->GetVariationLength(pgsTypes::sztLeftTapered);
      for ( IndexType i = 0; i < nSections; i++ )
      {
         Float64 X = Lleft + i*Lt/nSections;
         pgsPointOfInterest poi = pPoi->GetPointOfInterest(segmentKey,X);
         xsPOI.push_back(poi);
      }

      // right parabola
      GET_IFACE(IBridge,pBridge);
      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      Float64 Lright  = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic);
      Float64 Lr     = pSegment->GetVariationLength(pgsTypes::sztRightTapered);
      Lleft = Ls - Lright - Lr; // location of the left end of the right parabola
      for ( IndexType i = 0; i < nSections; i++ )
      {
         Float64 X = Lleft + i*Lr/nSections;
         pgsPointOfInterest poi = pPoi->GetPointOfInterest(segmentKey,X);
         xsPOI.push_back(poi);
      }
      pPoi->SortPoiList(&xsPOI);
   }
   ATLASSERT(2 <= xsPOI.size());

   GET_IFACE(IMaterials,pMaterial);
   Float64 density = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);
   Float64 g = WBFL::Units::System::GetGravitationalAcceleration();

   // compute distributed load intensity at each section change
   GET_IFACE(ISectionProperties,pSectProp);
   auto iter( xsPOI.begin() );
   auto end( xsPOI.end() );
   pgsPointOfInterest prevPoi = *iter++;
   Float64 Ag_Prev = pSectProp->GetAg(pgsTypes::sptGross,intervalIdx,prevPoi);
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& currPoi = *iter;
      Float64 Ag_Curr = pSectProp->GetAg(pgsTypes::sptGross,intervalIdx,currPoi);

      Float64 Xstart = prevPoi.GetDistFromStart();
      Float64 Xend   = currPoi.GetDistFromStart();
      Float64 Wstart   = -Ag_Prev*density*g;
      Float64 Wend     = -Ag_Curr*density*g;

      if (!IsZero(Xend, Xstart))
      {
         pSegmentLoads->emplace_back(Xstart, Xend, Wstart, Wend);
      }

      prevPoi = currPoi;
      Ag_Prev = Ag_Curr;
   }

   GetPrecastDiaphragmLoads(segmentKey,pDiaphragmLoads);

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   if ( segmentKey.segmentIndex < nSegments-1 )
   {
      GetClosureJointLoads(segmentKey,pClosureJointLoads);
   }
}

void CGirderModelManager::GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) const
{
   if ( pLoads == nullptr )
   {
      return;
   }

   pLoads->clear();

   GET_IFACE( IBridge,    pBridge   );
   GET_IFACE( IMaterials, pMaterial );
   GET_IFACE( IIntervals, pIntervals );

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 density = pMaterial->GetSegmentWeightDensity(segmentKey,releaseIntervalIdx); // cast with girder, using girder concrete

   Float64 g = WBFL::Units::System::GetGravitationalAcceleration();

   std::vector<IntermedateDiaphragm> diaphragms = pBridge->GetPrecastDiaphragms(segmentKey);
   for(const auto& diaphragm : diaphragms)
   {
      Float64 P;
      if ( diaphragm.m_bCompute )
      {
         P = diaphragm.H * diaphragm.T * diaphragm.W * density * g;
      }
      else
      {
         P = diaphragm.P;
      }

      pLoads->emplace_back(diaphragm.Location,-P);
   }

   std::sort(pLoads->begin(),pLoads->end());
}

void CGirderModelManager::GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) const
{
   if ( pLoads == nullptr )
   {
      return;
   }

   pLoads->clear();

   GET_IFACE( IBridge,    pBridge   );
   GET_IFACE( IMaterials, pMaterial );
   GET_IFACE( IIntervals, pIntervals );

   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);

   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   Float64 density = pMaterial->GetDiaphragmWeightDensity(castDiaphragmIntervalIdx);

   Float64 g = WBFL::Units::System::GetGravitationalAcceleration();

   Float64 start_brg_offset = 0;
   PierIndexType pierIdx = (PierIndexType)(spanKey.spanIndex);
   bool bIsBoundaryPier = pBridge->IsBoundaryPier(pierIdx);
   pgsTypes::BoundaryConditionType bcType = (bIsBoundaryPier ? pBridge->GetBoundaryConditionType(pierIdx) : pgsTypes::bctHinge);
   if ( 0 < spanKey.spanIndex && bIsBoundaryPier && (bcType == pgsTypes::bctHinge || bcType == pgsTypes::bctRoller) )
   {
      CSegmentKey segmentKey(girderKey,0);
      start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   }

   std::vector<IntermedateDiaphragm> diaphragms = pBridge->GetCastInPlaceDiaphragms(spanKey);
   for(const auto& diaphragm : diaphragms)
   {
      Float64 P;
      if ( diaphragm.m_bCompute )
      {
         P = diaphragm.H * diaphragm.T * diaphragm.W * density * g;
      }
      else
      {
         P = diaphragm.P;
      }

      pLoads->emplace_back(diaphragm.Location+start_brg_offset,-P);
   }
}

void CGirderModelManager::GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, PIER_DIAPHRAGM_LOAD_DETAILS* pBackSide, PIER_DIAPHRAGM_LOAD_DETAILS* pAheadSide) const
{
   GET_IFACE(IBridge,    pBridge );
   GET_IFACE(IMaterials, pMaterial);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   const auto* pPier = pIBridgeDesc->GetPier(pierIdx);

   pBackSide->TribWidth = 0;
   pBackSide->Height = 0;
   pBackSide->Width = 0;
   pBackSide->SkewAngle = 0;
   pBackSide->Density = 0;
   pBackSide->P = 0;
   pBackSide->M = 0;
   pBackSide->MomentArm = 0;

   pAheadSide->TribWidth = 0;
   pAheadSide->Height = 0;
   pAheadSide->Width = 0;
   pAheadSide->SkewAngle = 0;
   pAheadSide->Density = 0;
   pAheadSide->P = 0;
   pAheadSide->M = 0;
   pAheadSide->MomentArm = 0;

   GroupIndexType backGroupIdx, aheadGroupIdx;
   pBridge->GetGirderGroupIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);

   CGirderKey girderKey;
   if ( backGroupIdx == INVALID_INDEX )
   {
      girderKey.groupIndex = aheadGroupIdx;
      girderKey.girderIndex = Min(gdrIdx,pBridge->GetGirderCount(aheadGroupIdx)-1);
   }
   else
   {
      girderKey.groupIndex = backGroupIdx;
      girderKey.girderIndex = Min(gdrIdx,pBridge->GetGirderCount(backGroupIdx)-1);
   }

   pgsPointOfInterest poi = pPoi->GetPierPointOfInterest(girderKey,pierIdx);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   CSegmentKey backSegmentKey, aheadSegmentKey;
   pBridge->GetSegmentsAtPier(pierIdx, girderKey.girderIndex, &backSegmentKey, &aheadSegmentKey);

   IntervalIndexType castDiaphragmsIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();

   Float64 density = (pBridge->GetDeckType() == pgsTypes::sdtNone ? pMaterial->GetSegmentWeightDensity(segmentKey, castDiaphragmsIntervalIdx) : pMaterial->GetDeckWeightDensity(0/*assume region 0*/,castDiaphragmsIntervalIdx));
   Float64 g = WBFL::Units::System::GetGravitationalAcceleration();
   pBackSide->Density = density;
   pAheadSide->Density = density;

   bool bApplyLoadToBackSide  = pBridge->DoesPierDiaphragmLoadGirder(pierIdx,pgsTypes::Back);
   bool bApplyLoadToAheadSide = pBridge->DoesPierDiaphragmLoadGirder(pierIdx,pgsTypes::Ahead);

   if ( !bApplyLoadToBackSide && !bApplyLoadToAheadSide )
   {
      // none of the load is applied so leave now
      return;
   }

   // get skew angle so tributary width can be adjusted for pier skew
   // (diaphragm length is longer on skewed piers)
   CComPtr<IAngle> objSkew;
   pBridge->GetPierSkew(pierIdx,&objSkew);
   Float64 skew;
   objSkew->get_Value(&skew);

   // get the tributary width
   // Assume the end diaphragm reaction per girder based on the tributary slab width.
   // This is the width between interior girders. For exterior girders, it is the slab
   // width between the edge of deck and half way to the adjacent interior girder.
   // For a physical pier model, the cross beam can be wider then the out-to-out width
   // of the deck (e.g. an outrigger bent). For these cases it is assumed that the
   // self-weigth of the end diaphragm does not contribue to the girder reaction.
   Float64 trib_slab_width = 0;
   if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
   {
      // there isn't a deck so there isn't a tributary width.. use the
      // average spacing between the exterior girders
      Float64 left,right;
      pBridge->GetDistanceBetweenGirders(poi,&left,&right);

      GET_IFACE(IGirder, pGirder);
      Float64 width = Max(pGirder->GetTopWidth(poi),pGirder->GetBottomWidth(poi));
      trib_slab_width = width + (left+right)/2;
   }
   else
   {
      GET_IFACE(ISectionProperties, pSectProp);
      trib_slab_width = pSectProp->GetTributaryFlangeWidth( poi );
   }

   // Back side of pier
   if (bApplyLoadToBackSide)
   {
      Float64 W,H;
      pBridge->GetPierDiaphragmSize(pierIdx,pgsTypes::Back,&W,&H);

      pBackSide->Width = W;
      pBackSide->Height = H;
      pBackSide->TribWidth = trib_slab_width;
      pBackSide->SkewAngle = skew;
      pBackSide->P = -H*W*density*g*trib_slab_width/cos(skew);

      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         ATLASSERT(backSegmentKey.groupIndex != INVALID_INDEX);

         if (backSegmentKey.segmentIndex == INVALID_INDEX)
         {
            GirderIndexType nGirdersBack = pBridge->GetGirderCount(backSegmentKey.groupIndex);
            ATLASSERT(nGirdersBack <= gdrIdx);
            backSegmentKey.girderIndex = nGirdersBack - 1;
            backSegmentKey.segmentIndex = pBridge->GetSegmentCount(backSegmentKey) - 1;
         }

         ConnectionLibraryEntry::DiaphragmLoadType diaphragmLoadType = pPier->GetDiaphragmLoadType(pgsTypes::Back);

         bool bStartCantilever, bEndCantilever;
         pBridge->ModelCantilevers(backSegmentKey, &bStartCantilever, &bEndCantilever);

         Float64 moment_arm = pBridge->GetPierDiaphragmLoadLocation(backSegmentKey, pgsTypes::metEnd); // dist from CL Brg to CG Diaphragm

         if (bEndCantilever && diaphragmLoadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
         {
            // cantilever is long enough to be explicitly modeled
            Float64 end_dist = pBridge->GetSegmentEndEndDistance(backSegmentKey);
            if (end_dist < moment_arm)
            {
               // the load is beyond the end of the segment so put an equivalent force/moment at the end of the segment
               pBackSide->MomentArm = end_dist; // moment arm to end of segment
               moment_arm -= end_dist; // moment arm for P from end of segment to get moment at end of segment
               pBackSide->M = pBackSide->P * moment_arm;
            }
            else
            {
               // the load is on the end of the segment, model it with just the point load and moment arm
               pBackSide->M = 0.0;
               pBackSide->MomentArm = moment_arm;
            }
         }
         else
         {
            // cantilever is too short for explicit load modeling... use an equivalent force/moment at the CL Bearing
            pBackSide->M = pBackSide->P * moment_arm;
            pBackSide->MomentArm = 0.0;
         }

      }
   }

   if (bApplyLoadToAheadSide)
   {
      Float64 W,H;
      pBridge->GetPierDiaphragmSize(pierIdx,pgsTypes::Ahead,&W,&H);

      pAheadSide->Width = W;
      pAheadSide->Height = H;
      pAheadSide->TribWidth = trib_slab_width;
      pAheadSide->SkewAngle = skew;
      pAheadSide->P = -H*W*density*g*trib_slab_width/cos(skew);

      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         ATLASSERT(aheadSegmentKey.groupIndex != INVALID_INDEX);

         if (aheadSegmentKey.segmentIndex == INVALID_INDEX)
         {
            GirderIndexType nGirdersAhead = pBridge->GetGirderCount(aheadSegmentKey.groupIndex);
            ATLASSERT(nGirdersAhead <= gdrIdx);
            aheadSegmentKey.girderIndex = nGirdersAhead - 1;
            aheadSegmentKey.segmentIndex = 0;
         }

         ConnectionLibraryEntry::DiaphragmLoadType diaphragmLoadType = pPier->GetDiaphragmLoadType(pgsTypes::Ahead);

         bool bStartCantilever, bEndCantilever;
         pBridge->ModelCantilevers(aheadSegmentKey, &bStartCantilever, &bEndCantilever);

         Float64 moment_arm = pBridge->GetPierDiaphragmLoadLocation(aheadSegmentKey, pgsTypes::metStart); // dist from CL Brg to CG Diaphragm
         if (bStartCantilever && diaphragmLoadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
         {
            // cantilever is long enough to be explicitly modeled
            Float64 end_dist = pBridge->GetSegmentStartEndDistance(aheadSegmentKey);
            if (end_dist < moment_arm)
            {
               // the load is beyond the end of the segment so put an equivalent force/moment at the end of the segment
               pAheadSide->MomentArm = end_dist; // moment arm to end of segment
               moment_arm -= end_dist; // moment arm for P from end of segment to get moment at end of segment
               pAheadSide->M = -1*(pAheadSide->P) * moment_arm;
            }
            else
            {
               // the load is on the end of the segment, model it with just the point load and moment arm
               pAheadSide->M = 0.0;
               pAheadSide->MomentArm = moment_arm;
            }
         }
         else
         {
            // cantilever is too short for explicit load modeling... use an equivalent force/moment at the CL Bearing
            pAheadSide->M = -1*(pAheadSide->P) * moment_arm;
            pAheadSide->MomentArm = 0.0;
         }
      }
   }
}

void CGirderModelManager::GetClosureJointLoads(const CClosureKey& closureKey,std::vector<ClosureJointLoad>* pLoads) const
{
   if ( pLoads == nullptr )
   {
      return;
   }

   pLoads->clear();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castCJIntervalIdx = pIntervals->GetCastClosureJointInterval(closureKey);
   IntervalIndexType compositeCJIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

   // Get Points of interest at the end faces of the adjacent segments. 
   CSegmentKey leftSegmentKey(closureKey);
   CSegmentKey rightSegmentKey(closureKey);
   rightSegmentKey.segmentIndex++;

   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(leftSegmentKey,POI_END_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiLeftFace(vPoi.front());

   vPoi.clear();
   pPoi->GetPointsOfInterest(closureKey,POI_CLOSURE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiCenter(vPoi.front());

   vPoi.clear();
   pPoi->GetPointsOfInterest(rightSegmentKey,POI_START_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiRightFace(vPoi.front());

   IntervalIndexType leftSegmentReleaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(leftSegmentKey);
   IntervalIndexType rightSegmentReleaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(rightSegmentKey);

   GET_IFACE(ISectionProperties,pSectProps);
   Float64 Aleft  = pSectProps->GetAg(pgsTypes::sptGross,leftSegmentReleaseIntervalIdx,poiLeftFace);
   Float64 Acenter = pSectProps->GetAg(pgsTypes::sptGrossNoncomposite,compositeCJIntervalIdx,poiCenter);
   Float64 Aright = pSectProps->GetAg(pgsTypes::sptGross,rightSegmentReleaseIntervalIdx,poiRightFace);

   GET_IFACE(IMaterials,pMaterials);
   Float64 density = pMaterials->GetClosureJointWeightDensity(closureKey,castCJIntervalIdx);

   Float64 g = WBFL::Units::System::GetGravitationalAcceleration();

   Float64 Wleft   = Aleft   * density * g;
   Float64 Wcenter = Acenter * density * g;
   Float64 Wright  = Aright  * density * g;

   // Get left and right side CJ length
   Float64 Lleft, Lright;
   GET_IFACE(IBridge,pBridge);
   pBridge->GetClosureJointSize(closureKey,&Lleft,&Lright);

   pLoads->emplace_back(0.0, Lleft,-Wleft,-Wcenter);
   pLoads->emplace_back(Lleft,Lleft+Lright,-Wcenter,-Wright);
}

MemberIDType CGirderModelManager::ApplyDistributedLoadsToSegment(IntervalIndexType intervalIdx,ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,MemberIDType ssmbrID,const CSegmentKey& segmentKey,const std::vector<LinearLoad>& vLoads,BSTR bstrStage,BSTR bstrLoadGroup) const
{
#if defined _DEBUG
   // used below to error check loading geometry
   CComPtr<ISuperstructureMembers> ssmbrs;
   pModel->get_SuperstructureMembers(&ssmbrs);

   // even though we can get the first superstructure member ID for a particular segment, it is faster to pass in this value.
   // GetFirstSuperstructureMemberID has to iterate throught the structure to get the member IDs. This method is called
   // when building models so we generally know where we are in the model so we can just pass that information into this method.
   ATLASSERT(ssmbrID == GetFirstSuperstructureMemberID(segmentKey));
#endif

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   bool bIsDeckComposite = (lastCompositeDeckIntervalIdx <= intervalIdx);

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CPierData2* pStartPier;
   const CTemporarySupportData* pStartTS;
   pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

   const CPierData2* pEndPier;
   const CTemporarySupportData* pEndTS;
   pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);


   Float64 start_offset     = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_offset       = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 segment_length   = pBridge->GetSegmentLength(segmentKey);
   std::array<Float64, 3> L = {start_offset, segment_length - start_offset - end_offset, end_offset};

   GET_IFACE(IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE,&vPoi);
   ATLASSERT(vPoi.size() == 2);
   const pgsPointOfInterest& startPoi(vPoi.front());
   const pgsPointOfInterest& endPoi(vPoi.back());

   GET_IFACE(IGirder,pGdr);
   Float64 HgStart = pGdr->GetHeight(startPoi);
   Float64 HgEnd   = pGdr->GetHeight(endPoi);

   bool bModelStartCantilever,bModelEndCantilever;
   pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

   // apply distributed load items
   for(auto load : vLoads)
   {
      // force the load to be on the segment
      if ( load.Xstart < 0 )
      {
         load.Wstart = ::LinInterp(-load.Xstart,load.Wstart,load.Wend,load.Xend - load.Xstart);
         load.Xstart = 0;
      }

      if ( segment_length < load.Xend )
      {
         load.Wend = ::LinInterp(load.Xend - segment_length,load.Wstart,load.Wend,load.Xend - load.Xstart);
         load.Xend = segment_length;
      }

      // If cantilevers are not explicitly  modeled, point loads at start and end to account 
      // for the load in the overhang/cantilever. These are mostly used to make sure
      // the dead load reactions are correct. 
      Float64 Pstart(0.0), Pend(0.0); // point load

      std::array<Float64, 3> start,  end; // start and end of load on the 3 superstructure members modeling this segment
      std::array<Float64, 3> wStart, wEnd; // the start/end loads

      // default loads (assuming load is not on the SSMBR)
      for ( int i = 0; i < 3; i++ )
      {
         start[i]  = 0;
         end[i]    = L[i];
         wStart[i] = 0; // assume load not on SSMBR so load magnitude is 0
         wEnd[i]   = 0;
      }

      // Load on first superstructure member
      if ( load.Xstart < start_offset )
      {
         // load starts on SSMBR at start of segment
         Float64 start_loc = (load.Xstart < 0 ? 0 : load.Xstart);
         Float64 end_loc   = (load.Xend < start_offset ? load.Xend : start_offset);
         Float64 end_load  = (load.Xend < start_offset ? load.Wend : ::LinInterp(start_offset-load.Xstart,load.Wstart,load.Wend,load.Xend-load.Xstart));

         // Put a load on the cantilever if...
         if ( bModelStartCantilever // cantilever is long enough to be loaded
            || // OR
            ( !(segmentKey.segmentIndex == 0 && segmentKey.groupIndex == 0) // this is not the first segment in the first group
            && // AND
            bIsDeckComposite // the deck is composite
            && // AND
            analysisType == pgsTypes::Continuous // we are doing a continuous analysis
            && // AND
            ( (pStartTS) // the segment is supported by a temporary support
               || // OR
               (pStartPier && pStartPier->IsContinuousConnection())) // the segment is suppoted by a pier that has continuous boundary conditions
            )
            )
         {
            start[0]  = start_loc;
            wStart[0] = load.Wstart;
            end[0]    = end_loc;
            wEnd[0]   = end_load;

            load.Xstart = end_loc;
            load.Wstart   = end_load;
         }
         else
         {
            Pstart = 0.5*(load.Wstart + end_load)*(end_loc - start_loc);

            start[0]  = 0;
            end[0]    = start_offset;
            wStart[0] = 0;
            wEnd[0]   = 0;

            load.Xstart = end_loc;
            load.Wstart   = end_load;
         }
      }

      // load on last superstructure member
      if ( segment_length - end_offset < load.Xend )
      {
         Float64 start_loc  = (segment_length - end_offset < load.Xstart ? load.Xstart - (segment_length - end_offset) : 0);
         Float64 start_load = (segment_length - end_offset < load.Xstart ? load.Wstart : ::LinInterp(segment_length-end_offset-load.Xstart,load.Wstart,load.Wend,load.Xend-load.Xstart));
         Float64 end_loc    = Min(load.Xend - (segment_length - end_offset),end_offset);

         // put load on the cantilever if...
         if ( bModelEndCantilever // cantilever is long enough to be loaded
            || // OR
            ( !(segmentKey.segmentIndex == nSegments-1 && segmentKey.groupIndex == nGroups-1) // this is not the last segment in the last group
            && // AND
            bIsDeckComposite // the deck is composite 
            && // AND 
            analysisType == pgsTypes::Continuous // we are doing continuous analysis
            && // AND
            ( (pEndTS) // the segment is supported by a temporary support
               || // OR
               (pEndPier && pEndPier->IsContinuousConnection()))  // the segment is supported by a pier with continuous boundary conditions
            ) 
            )
         {
            start[2]  = start_loc;
            wStart[2] = start_load;
            end[2]    = end_loc;
            wEnd[2]   = load.Wend;

            load.Xend -= end_loc - start_loc;
            load.Wend   = start_load;
         }
         else
         {
            Pend = 0.5*(start_load + load.Wend)*(end_loc - start_loc);

            start[2]  = 0;
            end[2]    = end_offset;
            wStart[2] = 0;
            wEnd[2]   = 0;

            load.Xend -= end_loc - start_loc;
            load.Wend   = start_load;
         }
      }

      // load on main superstructure member
      // load location is measured from start of segment... 
      //subtract the start offset so that it is measured from the start of the SSMBR
      if ( start_offset <= load.Xstart && load.Xstart <= segment_length-end_offset)
      {
         start[1]  = load.Xstart - start_offset;
         end[1]    = load.Xend   - start_offset;
         wStart[1] = load.Wstart;
         wEnd[1]   = load.Wend;
      }

      // apply the loads to the LBAM
      MemberIDType mbrID = ssmbrID;
      for ( int i = 0; i < 3; i++ )
      {
         if ( !IsZero(L[i]) )
         {
#if defined _DEBUG
            // check the load geometry
            CComPtr<ISuperstructureMember> ssmbr;
            ssmbrs->get_Item(mbrID,&ssmbr);
            Float64 Lssmbr;
            ssmbr->get_Length(&Lssmbr);
            ATLASSERT( ::IsLE(0.0,start[i]) );
            ATLASSERT( ::IsLE(start[i],Lssmbr) );
            ATLASSERT( ::IsLE(0.0,end[i]) );
            ATLASSERT( ::IsLE(end[i],Lssmbr) );
            ATLASSERT( ::IsLE(start[i],end[i]) );
#endif
            if ( !IsZero(wStart[i]) || !IsZero(wEnd[i]) ) // don't add zero loads
            {
               CComPtr<IDistributedLoad> selfWgt;
               selfWgt.CoCreateInstance(CLSID_DistributedLoad);
               selfWgt->put_MemberType(mtSuperstructureMember);
               selfWgt->put_MemberID(mbrID);
               selfWgt->put_Direction(ldFy);
               selfWgt->put_WStart(wStart[i]);
               selfWgt->put_WEnd(wEnd[i]);
               selfWgt->put_StartLocation(start[i]);
               selfWgt->put_EndLocation(end[i]);

               CComPtr<IDistributedLoadItem> loadItem;
               distLoads->Add(bstrStage,bstrLoadGroup,selfWgt,&loadItem);
            }

            mbrID++;
         }
      }

      // Point loads at overhangs
      if ( !IsZero(Pstart) || !IsZero(Pend) ) // don't add zero loads
      {
         MemberIDType mainSSMbrID = IsZero(start_offset) ? ssmbrID : ssmbrID+1;
         ApplyOverhangPointLoads(segmentKey,analysisType,bstrStage,bstrLoadGroup,mainSSMbrID,Pstart,start[1],Pend,end[1],pointLoads);
      }
   } // next load

   // return the ID of the next superstructure member to be loaded
   // determine how many SSMBRs were loaded/modeled
   MemberIDType mbrIDInc = GetSuperstructureMemberCount(segmentKey);
   return ssmbrID + mbrIDInc;
}

void CGirderModelManager::GetSlabLoad(const std::vector<SlabLoad>& vBasicSlabLoads,IndexType castingRegionIdx, std::vector<LinearLoad>& vSlabLoads, std::vector<LinearLoad>& vHaunchLoads, std::vector<LinearLoad>& vPanelLoads) const
{
   // Create equivalent LinearLoad vectors from the SlabLoad information
   auto iter1(vBasicSlabLoads.begin());
   auto iter2(iter1+1);
   const auto end(vBasicSlabLoads.end());
   for ( ; iter2 != end; iter1++, iter2++ )
   {
      const SlabLoad& prevLoad = *iter1;
      const SlabLoad& nextLoad = *iter2;

      if (prevLoad.DeckCastingRegionIdx != castingRegionIdx || nextLoad.DeckCastingRegionIdx != castingRegionIdx)
      {
         // the load segments aren't in the target casting regino
         continue;
      }

      Float64 Xstart = prevLoad.Loc;
      Float64 Xend   = nextLoad.Loc;
      Float64 Wstart   = prevLoad.MainSlabLoad;
      Float64 Wend     = nextLoad.MainSlabLoad;
      vSlabLoads.emplace_back(Xstart,Xend,Wstart,Wend);

      Xstart = prevLoad.Loc;
      Xend   = nextLoad.Loc;
      Wstart   = prevLoad.PadLoad;
      Wend     = nextLoad.PadLoad;
      vHaunchLoads.emplace_back(Xstart, Xend, Wstart, Wend);

      Xstart = prevLoad.Loc;
      Xend   = nextLoad.Loc;
      Wstart   = prevLoad.PanelLoad;
      Wend     = nextLoad.PanelLoad;
      vPanelLoads.emplace_back(Xstart, Xend, Wstart, Wend);
   } // next load
}

void CGirderModelManager::GetLinearLoadPointsOfInterest(const CSegmentKey& segmentKey, PoiList* pvPoi) const
{
   // a consistent way to get POI's for linear loading situations
   GET_IFACE(IPointOfInterest, pPoi);
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, pvPoi);
   
   // We need to include section transition for things like U-Beams with end blocks
   // The slab pad will be wider over the end block then in the middle of the girder
   PoiList vPoi2;
   pPoi->GetPointsOfInterest(segmentKey, POI_SECTCHANGE, &vPoi2, POIFIND_OR);
   pPoi->MergePoiLists(*pvPoi, vPoi2, pvPoi);

   ATLASSERT(pvPoi->size() != 0);
}

void CGirderModelManager::GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) const
{
   // not design version - gets A's and fillet from model and condenses duplicate values
   Float64 dummyAs(0), dummyAe(0), dummyF(0);
   GetMainSpanSlabLoadEx(segmentKey, true, false, dummyAs, dummyAe, dummyF, pSlabLoads);
}

void CGirderModelManager::GetMainSpanSlabLoadEx(const CSegmentKey& segmentKey, bool doCondense, bool useDesignValues , Float64 dsnAstart, Float64 dsnAend, Float64 dsnAssumedExcessCamber,  std::vector<SlabLoad>* pSlabLoads) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   ATLASSERT(pSlabLoads != nullptr);
   pSlabLoads->clear();

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   // if there is no deck, there is no load
   pgsTypes::SupportedDeckType deckType = pDeck->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      return;
   }

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGirder);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE_NOCHECK(ISpecification, pSpec );
   GET_IFACE(IRoadway, pAlignment);
   GET_IFACE_NOCHECK(ISectionProperties,pSectProps);

   IndexType deckCastingRegionIdx = 0; // assume region zero to get properties that are common to all castings
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
   Float64 deck_density = pMaterial->GetDeckWeightDensity(deckCastingRegionIdx, castDeckIntervalIdx);
   Float64 deck_unit_weight = deck_density * WBFL::Units::System::GetGravitationalAcceleration();

   Float64 panel_support_width = 0;
   if (deckType == pgsTypes::sdtCompositeSIP )
   {
      panel_support_width = pDeck->PanelSupport;
   }

   // Get the POIs for getting the deck load.
   PoiList vPoi;
   GetLinearLoadPointsOfInterest(segmentKey, &vPoi);
   ATLASSERT(vPoi.size() != 0);

   // add the deck casting boundary POIs
   PoiList vPoi2;
   pPoi->GetPointsOfInterest(segmentKey, POI_CASTING_BOUNDARY, &vPoi2);
   pPoi->MergePoiLists(vPoi, vPoi2, &vPoi);

   // Account for girder camber
   bool bIsInteriorGirder = pBridge->IsInteriorGirder( segmentKey );
   pgsTypes::SideType side = (segmentKey.girderIndex == 0 ? pgsTypes::stLeft : pgsTypes::stRight);

   std::unique_ptr<WBFL::Math::Function> camberShape;

   // estimated girder excess camber (user input excess camber) is measured using the bearings as the datum
   PoiList vSupPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_5L | POI_10L | POI_ERECTED_SEGMENT, &vSupPoi);
   ATLASSERT(vSupPoi.size() == 3); // this haunch shape only for pgsuper-type models
   const pgsPointOfInterest& poi_left_brg(vSupPoi.front());
   const pgsPointOfInterest& poi_mid(vSupPoi[1]);
   const pgsPointOfInterest& poi_right_brg(vSupPoi.back());

   // precamber and top flange thickening is measured using the ends of the girder as the datum
   std::unique_ptr<WBFL::Math::Function> imposedShape;
   vPoi2.clear();
   pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE, &vPoi2);
   ATLASSERT(vPoi2.size() == 2);
   const pgsPointOfInterest& poi_left(vPoi2.front());
   const pgsPointOfInterest& poi_right(vPoi2.back());

   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();
   bool isHaunchDirect = haunchInputType == pgsTypes::hidHaunchDirectly || haunchInputType == pgsTypes::hidHaunchPlusSlabDirectly;

   pgsTypes::HaunchLoadComputationType HaunchLoadComputationType = pSpec->GetHaunchLoadComputationType();

   if (isHaunchDirect) 
   {
      // With direct haunch input we don't need to compute camber or flange thickening or precamber. 
      camberShape = std::make_unique<WBFL::Math::ZeroFunction>();
      imposedShape = std::make_unique<WBFL::Math::ZeroFunction>();
   }
   else if (pSpec->IsAssumedExcessCamberForLoad())
   {
#pragma Reminder("UPDATE: assuming precast girder bridge - Note that time-dependent analyses only use the zero camber approach below")
      // Shape of girder is assumed to follow the fillet dimension. Assume parabolic shape with zero at supports and
      // average end haunch - fillet at mid-girder
      
      // apply optional factor to camber
      Float64 camberFactor = pSpec->GetHaunchLoadCamberFactor();
      
      // Shape of girder is parabola assumed to follow the input assummed excess camber
      Float64 assumed_excess_camber;

      if (useDesignValues)
      {
         assumed_excess_camber = dsnAssumedExcessCamber * camberFactor;
      }
      else
      {
         // Get excess camber at mid-span. Big time assumption of PGSuper-type bridge here
         CSpanKey spanKey;
         Float64 Xspan;
         pPoi->ConvertPoiToSpanPoint(poi_mid,&spanKey,&Xspan);

         assumed_excess_camber = pBridge->GetAssumedExcessCamber(spanKey.spanIndex, spanKey.girderIndex);
         assumed_excess_camber *= camberFactor;
      }

      if (!IsZero(assumed_excess_camber))
      {
         // Create function with parabolic shape
         camberShape = std::make_unique<WBFL::Math::PolynomialFunction>(GenerateParabola(poi_left_brg.GetDistFromStart(), poi_right_brg.GetDistFromStart(), assumed_excess_camber));
      }
      else
      {
         camberShape = std::make_unique<WBFL::Math::ZeroFunction>();
      }

      Float64 top_flange_thickening = 0;
      const CPrecastSegmentData* pSegment = pBridgeDesc->GetSegment(segmentKey);
      if (pSegment->TopFlangeThickeningType != pgsTypes::tftNone && !IsZero(pSegment->TopFlangeThickening))
      {
         // top flange thickening is imposed on the girder
         Float64 sign = (pSegment->TopFlangeThickeningType == pgsTypes::tftEnds ? -1 : 1);
         top_flange_thickening += sign*pSegment->TopFlangeThickening;
      }

      if (!IsZero(top_flange_thickening))
      {
         // Create function with parabolic shape
         imposedShape = std::make_unique<WBFL::Math::PolynomialFunction>(GenerateParabola(poi_left.GetDistFromStart(), poi_right.GetDistFromStart(), top_flange_thickening));
      }
      else
      {
         imposedShape = std::make_unique<WBFL::Math::ZeroFunction>();
      }
   }
   else
   {
      // Slab pad load assumes no natural camber
      ATLASSERT(pgsTypes::hlcZeroCamber == HaunchLoadComputationType);

      camberShape = std::make_unique<WBFL::Math::ZeroFunction>();

      Float64 departure = 0; // departure from flat due to imposed curvature of the top of the girder
      const CPrecastSegmentData* pSegment = pBridgeDesc->GetSegment(segmentKey);
      if (pSegment->GetGirder()->GetGirderLibraryEntry()->CanPrecamber() && !IsZero(pSegment->Precamber))
      {
         // precamber is imposed on the girder
         departure += pSegment->Precamber;
      }

      if (pSegment->TopFlangeThickeningType != pgsTypes::tftNone && !IsZero(pSegment->TopFlangeThickening))
      {
         // top flange thickening is imposed on the girder
         Float64 sign = (pSegment->TopFlangeThickeningType == pgsTypes::tftEnds ? -1 : 1);
         departure += sign*pSegment->TopFlangeThickening;
      }

      if ( !IsZero(departure))
      {
         // there is an imposed camber and/or top flange thickening. use its shape, excluding natural camber, for the top of the girder
         imposedShape = std::make_unique<WBFL::Math::PolynomialFunction>(GenerateParabola(poi_left.GetDistFromStart(), poi_right.GetDistFromStart(), departure));
      }
      else
      {
         imposedShape = std::make_unique<WBFL::Math::ZeroFunction>();
      }
   }

   Float64 imposed_at_left_brg = imposedShape->Evaluate(poi_left_brg.GetDistFromStart());
   Float64 imposed_at_right_brg = imposedShape->Evaluate(poi_right_brg.GetDistFromStart());

   // Increased/Reduced pad depth due to Sag/Crest vertical curves is accounted for
   bool bKeepLast = false;
   Float64 Ls = pBridge->GetSegmentLength(segmentKey);
   for( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 wslab;
      Float64 wslab_panel;

      Float64 station,offset;
      pBridge->GetStationAndOffset(poi,&station,&offset);
      offset = IsZero(offset) ? 0 : offset;

      // top of alignment elevation above girder
      Float64 rdwy_elevation = pAlignment->GetElevation(station,offset);

      Float64 top_girder_to_top_slab;
      Float64 girder_chord_elevation;
      if (useDesignValues && !isHaunchDirect)
      {
         top_girder_to_top_slab = pBridge->GetTopSlabToTopGirderChordDistance(poi,dsnAstart,dsnAend);
         girder_chord_elevation = pGirder->GetTopGirderChordElevation(poi,dsnAstart,dsnAend);
      }
      else
      {
         top_girder_to_top_slab = pBridge->GetTopSlabToTopGirderChordDistance(poi);
         girder_chord_elevation = pGirder->GetTopGirderChordElevation(poi);
      }

      Float64 slab_offset = top_girder_to_top_slab;

      Float64 cast_depth             = pBridge->GetCastSlabDepth(poi);
      Float64 panel_depth            = pBridge->GetPanelDepth(poi);
      Float64 trib_slab_width;
      if (IsNonstructuralDeck(deckType))
      {
         trib_slab_width = pGirder->GetTopWidth(poi);
         if (pGirder->HasStructuralLongitudinalJoints())
         {
            Float64 joint_width = pGirder->GetStructuralLongitudinalJointWidth(poi); // this is the full width of the left and right joints
            trib_slab_width += joint_width / 2; // we want half of left and half of right joint width
         }
      }
      else
      {
         GET_IFACE(ISectionProperties, pSectProp);
         trib_slab_width = pSectProp->GetTributaryFlangeWidth(poi);
      }

      if ( bIsInteriorGirder )
      {
         // Apply the load of the main slab
         wslab = trib_slab_width * cast_depth  * deck_unit_weight;

         // compute the width of the deck panels
         Float64 panel_width = trib_slab_width; // start with tributary width

         // deduct width of mating surfaces
         MatingSurfaceIndexType nMatingSurfaces = pGirder->GetMatingSurfaceCount(segmentKey);
         for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMatingSurfaces; msIdx++ )
         {
            panel_width -= pGirder->GetMatingSurfaceWidth(poi,msIdx);
         }

         // add panel support widths
         panel_width += 2*nMatingSurfaces*panel_support_width;
         wslab_panel = panel_width * panel_depth * deck_unit_weight;
      }
      else
      {
         // Exterior girder... the slab overhang can be thickened so we have figure out the weight
         // on the left and right of the girder instead of using the tributary width and slab depth

         // determine depth of the slab at the edge and flange tip
         Float64 overhang_edge_depth = pDeck->OverhangEdgeDepth[side];
         Float64 overhang_depth_at_flange_tip;
         if ( pDeck->OverhangTaper[side] == pgsTypes::dotNone )
         {
            // overhang is constant depth
            overhang_depth_at_flange_tip = overhang_edge_depth;
         }
         else if ( pDeck->OverhangTaper[side] == pgsTypes::dotTopTopFlange )
         {
            // deck overhang tapers to the top of the top flange
            overhang_depth_at_flange_tip = slab_offset;
         }
         else if ( pDeck->OverhangTaper[side] == pgsTypes::dotBottomTopFlange )
         {
            // deck overhang tapers to the bottom of the top flange
            FlangeIndexType nFlanges = pGirder->GetTopFlangeCount(segmentKey);
            Float64 flange_thickness;
            if ( nFlanges == 0 )
            {
               flange_thickness = pGirder->GetMinTopFlangeThickness(poi);
            }
            else
            {
               if ( segmentKey.girderIndex == 0 )
               {
                  flange_thickness = pGirder->GetTopFlangeThickness(poi,0);
               }
               else
               {
                  flange_thickness = pGirder->GetTopFlangeThickness(poi,nFlanges-1);
               }
            }

            overhang_depth_at_flange_tip = slab_offset + flange_thickness;
         }
         else
         {
            ATLASSERT(false); // is there a new deck overhang taper???
         }

         // Determine the slab overhang
         Float64 station,offset;
         pBridge->GetStationAndOffset(poi,&station,&offset);
         Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(station);

         // slab overhang from CL of girder (normal to alignment)
         Float64 slab_overhang = (segmentKey.girderIndex == 0 ? pBridge->GetLeftSlabOverhang(Xb) : pBridge->GetRightSlabOverhang(Xb));

         if (slab_overhang < 0.0)
         {
            // negative overhang - girder probably has no slab over it
            slab_overhang = 0.0;
         }
         else
         {
            Float64 top_width = pGirder->GetTopWidth(poi);

            // slab overhang from edge of girder (normal to alignment)
            slab_overhang -= top_width/2;
         }

         // area of slab overhang
         Float64 slab_overhang_area = slab_overhang*(overhang_edge_depth + overhang_depth_at_flange_tip)/2;

         // Determine area of slab from exterior flange tip to 1/2 distance to interior girder
         Float64 w = trib_slab_width - slab_overhang;
         Float64 slab_area = w*cast_depth;
         wslab       = (slab_area + slab_overhang_area) * deck_unit_weight;

         Float64 panel_width = w;

         // deduct width of mating surfaces
         MatingSurfaceIndexType nMatingSurfaces = pGirder->GetMatingSurfaceCount(segmentKey);
         for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMatingSurfaces; msIdx++ )
         {
            panel_width -= pGirder->GetMatingSurfaceWidth(poi,msIdx);
         }

         // add panel support widths (2 sides per mating surface)
         panel_width += 2*nMatingSurfaces*panel_support_width;

         // the exterior mating surface doesn't have a panel on the exterior side
         // so deduct one panel support width

         panel_width -= panel_support_width;
         if (panel_width<0.0)
         {
            panel_width = 0.0; // negative overhangs can cause this condition
         }

         wslab_panel = panel_width * panel_depth * deck_unit_weight;
      }

      ASSERT( 0 <= wslab );
      ASSERT( 0 <= wslab_panel );

      // Excess camber of girder + imposed girder shape
      Float64 Xs = poi.GetDistFromStart();
      Float64 camber = camberShape->Evaluate(Xs) + imposedShape->Evaluate(Xs);

      // imposed shape is measured from end faces of the girder... 
      // we need to make an adjustment so the zero datum is at the CL Brg.
      Float64 imposed_shape_adj = -::LinInterp(Xs, imposed_at_left_brg, imposed_at_right_brg, Ls);
      camber += imposed_shape_adj;

      // height of pad for slab pad load
      Float64 real_pad_hgt;
      if (isHaunchDirect && pgsTypes::hlcDetailedAnalysis == HaunchLoadComputationType)
      {
         real_pad_hgt = pSectProps->GetStructuralHaunchDepth(poi,pgsTypes::hspDetailedDescription);
      }
      else
      {
         real_pad_hgt = top_girder_to_top_slab - cast_depth - camber;
      }

      // Don't use negative haunch depth for loading
      Float64 pad_hgt = 0.0 < real_pad_hgt ? real_pad_hgt : 0.0;
      // mating surface
      Float64 mating_surface_width = 0;
      MatingSurfaceIndexType nMatingSurfaces = pGirder->GetMatingSurfaceCount(segmentKey);
      ATLASSERT( nMatingSurfaces == pGirder->GetMatingSurfaceCount(segmentKey) );
      for ( MatingSurfaceIndexType matingSurfaceIdx = 0; matingSurfaceIdx < nMatingSurfaces; matingSurfaceIdx++ )
      {
         mating_surface_width += pGirder->GetMatingSurfaceWidth(poi,matingSurfaceIdx);
         mating_surface_width -= 2*panel_support_width;
      }

      if ( !bIsInteriorGirder )
      {
         /// if this is an exterior girder, add back one panel support width for the exterior side of the girder
         // becuase we took off one to many panel support widths in the loop above
         mating_surface_width += panel_support_width;
      }

      // calculate load, neglecting effect of fillet
      Float64 wpad = 0;
      if (IsNonstructuralDeck(deckType))
      {
         wpad = trib_slab_width * pad_hgt *  deck_unit_weight;
      }
      else
      {
         wpad = (pad_hgt*mating_surface_width + (pad_hgt - panel_depth)*(bIsInteriorGirder ? 2 : 1)*nMatingSurfaces*panel_support_width)*  deck_unit_weight;
      }
      ASSERT( 0 <= wpad );

      LOG("Poi Loc at           = " << poi.GetDistFromStart());
      LOG("Main Slab Load       = " << wslab);
      LOG("Slab Pad Load        = " << wpad);

      SlabLoad sload;
      sload.DeckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      sload.Loc          = poi.GetDistFromStart();
      sload.MainSlabLoad = -wslab;  // + is upward
      sload.PanelLoad    = -wslab_panel;
      sload.PadLoad      = -wpad;
      sload.AssumedExcessCamber = camber;
      sload.HaunchDepth  = real_pad_hgt;
      sload.SlabDepth = cast_depth;
      sload.GirderChordElevation = girder_chord_elevation;
      sload.TopSlabElevation = rdwy_elevation;
      sload.TopGirderElevation = girder_chord_elevation + camber;
      sload.Station = station;
      sload.Offset = offset;


      if ( !doCondense || pSlabLoads->size() < 2 )
      {
         pSlabLoads->push_back(sload);
      }
      else
      {
         SlabLoad lastLoad = pSlabLoads->back();
         if ( IsEqual(lastLoad.MainSlabLoad,sload.MainSlabLoad) && IsEqual(lastLoad.PanelLoad,sload.PanelLoad) && IsEqual(lastLoad.PadLoad,sload.PadLoad) && lastLoad.DeckCastingRegionIdx == sload.DeckCastingRegionIdx)
         {
            if ( !bKeepLast )
            {
               pSlabLoads->pop_back();
            }

            pSlabLoads->push_back(sload);
            bKeepLast = false;
         }
         else
         {
            pSlabLoads->push_back(sload);
            bKeepLast = true;
         }
      }
   }
}

void  CGirderModelManager::GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 AssumedExcessCamber, std::vector<SlabLoad>* pSlabLoads) const
{
   // Subtract slab pad load due to design values from that due to original model.
   ASSERT_SEGMENT_KEY(segmentKey);
   Float64 fdummy(0);
   std::vector<SlabLoad> originalSlabLoads;
   GetMainSpanSlabLoadEx(segmentKey, false, false, fdummy, fdummy, fdummy, &originalSlabLoads);
   GetMainSpanSlabLoadEx(segmentKey, false, true, Astart, Aend, AssumedExcessCamber, pSlabLoads);

   ATLASSERT(originalSlabLoads.size() == pSlabLoads->size());

   std::vector<SlabLoad>::iterator oit = originalSlabLoads.begin();
   std::vector<SlabLoad>::iterator dit = pSlabLoads->begin();
   std::vector<SlabLoad>::iterator oitend = originalSlabLoads.end();
   std::vector<SlabLoad>::iterator ditend = pSlabLoads->end();

   while(oit!=oitend && dit!=ditend)
   {
      SlabLoad& dsnSL = *dit;
      SlabLoad& orgSL = *oit;

      // adjusted load
      dsnSL.HaunchDepth  = orgSL.HaunchDepth  - dsnSL.HaunchDepth;
      dsnSL.MainSlabLoad = orgSL.MainSlabLoad - dsnSL.MainSlabLoad;
      dsnSL.PadLoad      = orgSL.PadLoad      - dsnSL.PadLoad;
      dsnSL.PanelLoad    = orgSL.PanelLoad    - dsnSL.PanelLoad;
      ATLASSERT(dsnSL.Loc == orgSL.Loc);

      oit++;
      dit++;
   }
}

void CGirderModelManager::GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   Float64 P1[3], P2[3];
   GetCantileverSlabLoads(segmentKey,&P1[0],&P2[0]);
   *pP1 = P1[0];
   *pP2 = P2[0];
   *pM1 = 0;
   *pM2 = 0;
}

void CGirderModelManager::GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const
{
   Float64 P1[3], P2[3];
   GetCantileverSlabLoads(segmentKey,&P1[0],&P2[0]);
   *pP1 = P1[1];
   *pP2 = P2[1];
   *pM1 = 0;
   *pM2 = 0;
}

void CGirderModelManager::GetCantileverSlabLoads(const CSegmentKey& segmentKey, Float64* pP1, Float64* pP2) const
{
#pragma Reminder("UPDATE: this can be better code")
   // This code is very similiar to ApplySlabLoad. In fact, the real slab cantilever loads that are
   // applied to the LBAM are generated in ApplySlabLoad. This purpose of this method and
   // the GetCantileverSlabLoad and GetCantileverSlabPadLoad methods are for reporting the loading
   // details. It would be better to just cache the cantilever loads when they are computed.
   pP1[0] = 0;
   pP1[1] = 0;
   pP1[2] = 0;

   pP2[0] = 0;
   pP2[1] = 0;
   pP2[2] = 0;

   GET_IFACE(IBridge,pBridge);

   Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_end_dist   = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   bool bModelStartCantilever,bModelEndCantilever;
   pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

   // main slab load
   std::vector<SlabLoad> sload;
   GetMainSpanSlabLoad(segmentKey,&sload);
   IndexType nLoads = sload.size();
   for (IndexType i = 0; i < nLoads-1; i++)
   {
      SlabLoad& prevLoad = sload[i];
      SlabLoad& nextLoad = sload[i+1];
      Float64 start = prevLoad.Loc;
      Float64 end = nextLoad.Loc;

      Float64 wStartMain = prevLoad.MainSlabLoad;
      Float64 wStartPad  = prevLoad.PadLoad;
      Float64 wStartPanel = prevLoad.PanelLoad;

      Float64 wEndMain = nextLoad.MainSlabLoad;
      Float64 wEndPad  = nextLoad.PadLoad;
      Float64 wEndPanel = nextLoad.PanelLoad;

      Float64 PstartMain(0.0), PendMain(0.0); // point loads at start and end to account for load in the girder overhangs (makes reactions come out right)
      Float64 PstartPad(0.0),  PendPad(0.0); // point loads at start and end to account for load in the girder overhangs (makes reactions come out right)
      Float64 PstartPanel(0.0), PendPanel(0.0); // point loads at start and end to account for load in the girder overhangs (makes reactions come out right)
      if ( start < start_end_dist && !bModelStartCantilever )
      {
         // this load item begins before the CL bearing and the cantilever is not being modeled

         // compute load intensity at CL Bearing
         Float64 wMainSlab = ::LinInterp(start_end_dist,wStartMain,wEndMain,start-end);
         Float64 wPad      = ::LinInterp(start_end_dist,wStartPad,wEndPad,start-end);
         Float64 wPanel    = ::LinInterp(start_end_dist,wStartPanel,wEndPanel,start-end);

         PstartMain  = (wStartMain  + wMainSlab)*start_end_dist/2;
         PstartPad   = (wStartPad   + wPad)*start_end_dist/2;
         PstartPanel = (wStartPanel + wPanel)*start_end_dist/2;
         
         wStartMain = wMainSlab;
         wStartPad  = wPad;
         wStartPanel = wPanel;

         start = start_end_dist;
      }

      if ( segment_length - end_end_dist < end && !bModelEndCantilever )
      {
         // this load end after the CL Bearing (right end of girder)
         // and the cantilever is not being modeled

         // compute load intensity at CL Bearing
         Float64 wMainSlab = ::LinInterp(segment_length - end_end_dist - start,wStartMain,wEndMain,start-end);
         Float64 wPad      = ::LinInterp(segment_length - end_end_dist - start,wStartPad,wEndPad,start-end);
         Float64 wPanel    = ::LinInterp(segment_length - end_end_dist - start,wStartPanel,wEndPanel,start-end);

         PendMain = (wEndMain + wMainSlab)*end_end_dist/2;
         PendPad  = (wEndPad + wPad)*end_end_dist/2;
         PendPanel = (wEndPanel + wPanel)*end_end_dist/2;

         wEndMain = wMainSlab;
         wEndPad = wPad;
         wEndPanel = wPanel;

         end = segment_length - end_end_dist;
      }

      pP1[0] += PstartMain;
      pP1[1] += PstartPad;
      pP1[2] += PstartPanel;

      pP2[0] += PendMain;
      pP2[1] += PendPad;
      pP2[2] += PendPanel;
   } // next load
}

void CGirderModelManager::GetMainSpanOverlayLoad(const CSegmentKey& segmentKey, std::vector<OverlayLoad>* pOverlayLoads) const
{
   ATLASSERT(pOverlayLoads!=0);
   pOverlayLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPoi);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2*   pDeck       = pBridgeDesc->GetDeckDescription();

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);

   Float64 OverlayWeight = pBridge->GetOverlayWeight();

   GET_IFACE( ISpecification, pSpec );
   pgsTypes::OverlayLoadDistributionType overlayDistribution = pSpec->GetOverlayLoadDistributionType();

   // POIs where overlay loads are laid out
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size()!=0);


   // Width of loaded area, and load intensity
   Float64 startWidth(0), endWidth(0);
   Float64 startW(0), endW(0);
   Float64 startLoc(0), endLoc(0);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_end_dist   = pBridge->GetSegmentEndEndDistance(segmentKey);
   
   startLoc = 0.0;

   auto begin(vPoi.begin());
   auto iter(vPoi.begin());
   auto end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      // poi at end of distributed load
      const pgsPointOfInterest& endPoi = *iter;
      endLoc = endPoi.GetDistFromStart();

      Float64 station,girder_offset;
      pBridge->GetStationAndOffset(endPoi,&station,&girder_offset);
      Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(station);

      // Offsets to toe where overlay starts
      Float64 left_olay_offset  = pBridge->GetLeftOverlayToeOffset(Xb);
      Float64 right_olay_offset = pBridge->GetRightOverlayToeOffset(Xb);

      if (overlayDistribution == pgsTypes::olDistributeEvenly)
      {
         // This one is easy. girders see overlay even if they aren't under it
         // Total overlay width at location
         endWidth = right_olay_offset - left_olay_offset;
         endW = -endWidth*OverlayWeight/nGirders;
      }
      else if (overlayDistribution == pgsTypes::olDistributeTributaryWidth)
      {
         Float64 left_slab_offset  = pBridge->GetLeftSlabEdgeOffset(Xb);

         // Have to determine how much of overlay is actually over the girder's tributary width
         // Measure distances from left edge of deck to Left/Right edges of overlay
         Float64 DLO = left_olay_offset - left_slab_offset;
         Float64 DRO = right_olay_offset - left_slab_offset;

         // Distance from left edge of deck to CL girder
         Float64 DGDR = girder_offset - left_slab_offset;

         // Next get distances from left edge of deck to girder's left and right tributary edges
         Float64 DLT, DRT;
         if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
         {
            ATLASSERT( ::IsJointSpacing(pBridgeDesc->GetGirderSpacingType()) );

            // Joint widths
            Float64 leftJ,rightJ;
            pBridge->GetDistanceBetweenGirders(endPoi,&leftJ,&rightJ);

            GET_IFACE(IGirder,pGirder);
            Float64 width = Max(pGirder->GetTopWidth(endPoi),pGirder->GetBottomWidth(endPoi));
            Float64 width2 = width/2.0;

            // Note that tributary width ignores joint spacing
            DLT = DGDR - width2 - leftJ/2.0;;
            DRT = DGDR + width2 + rightJ/2.0;
         }
         else
         {
            GET_IFACE(ISectionProperties,pSectProp);
            Float64 lftTw, rgtTw;
            Float64 tribWidth = pSectProp->GetTributaryFlangeWidthEx(endPoi, &lftTw, &rgtTw);

            DLT = DGDR - lftTw;
            DRT = DGDR + rgtTw;
         }

         // Now we have distances for all needed elements. Next figure out how much
         // of overlay lies within tributary width
         ATLASSERT(DLO < DRO); // negative overlay widths should be handled elsewhere

         if (DLO <= DLT)
         {
            if(DRT <= DRO)
            {
              endWidth = DRT-DLT;
            }
            else if (DLT < DRO)
            {
               endWidth = DRO-DLT;
            }
            else
            {
               endWidth = 0.0;
            }
         }
         else if (DLO < DRT)
         {
            if (DRO < DRT)
            {
               endWidth = DRO-DLO;
            }
            else
            {
               endWidth = DRT-DLO;
            }
         }
         else
         {
            endWidth = 0.0;
         }

         endW = -endWidth*OverlayWeight;
      }
      else
      {
         ATLASSERT(false); //something new?
      }

      // Create load and stuff it
      if (iter != begin && !IsEqual(startLoc,endLoc) )
      {
         OverlayLoad load;
         load.Xstart = startLoc;
         load.Xend   = endLoc;
         load.StartWcc = startWidth;
         load.EndWcc   = endWidth;
         load.Wstart   = startW;
         load.Wend     = endW;

         if (pOverlayLoads->size() == 0 )
         {
            pOverlayLoads->push_back(load);
         }
         else
         {
            // if the end of the previous load is exactly the same as the end of this load, just move the end of the previous load
            // (we know the end of the previous load and the start of this load are the same)
            if ( IsEqual(pOverlayLoads->back().Wend,endW) && IsEqual(pOverlayLoads->back().EndWcc,endWidth) )
            {
               pOverlayLoads->back().Xend = endLoc;
            }
            else
            {
               pOverlayLoads->push_back(load);
            }
         }
      }

      // Set variables for next go through loop
      startLoc   = endLoc;
      startWidth = endWidth;
      startW     = endW;
   }
}

bool CGirderModelManager::HasConstructionLoad(const CGirderKey& girderKey) const
{
   GET_IFACE(IUserDefinedLoadData,pLoads);
   Float64 construction_load = pLoads->GetConstructionLoad();

   return !IsZero(construction_load);
}

void CGirderModelManager::GetMainConstructionLoad(const CSegmentKey& segmentKey, std::vector<ConstructionLoad>* pConstructionLoads) const
{
   ATLASSERT(pConstructionLoads != nullptr);
   pConstructionLoads->clear();

   GET_IFACE(IBridge,pBridge);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2*   pDeck       = pBridgeDesc->GetDeckDescription();

   GET_IFACE(IUserDefinedLoadData,pLoads);
   Float64 construction_load = pLoads->GetConstructionLoad();

   // Get some important POIs that we will be using later
   GET_IFACE(IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size()!=0);

   auto prevPoiIter(vPoi.begin());
   auto currPoiIter = prevPoiIter+1;
   auto poiIterEnd(vPoi.end());

   for ( ; currPoiIter != poiIterEnd; prevPoiIter++, currPoiIter++ )
   {
      const pgsPointOfInterest& prevPoi = *prevPoiIter;
      const pgsPointOfInterest& currPoi = *currPoiIter;

      // Width of loaded area, and load intensity
      Float64 startWidth, endWidth;
      Float64 startW, endW;

      if (IsNonstructuralDeck( pBridge->GetDeckType() ))
      {
         Float64 left,right;
         pBridge->GetDistanceBetweenGirders(prevPoi,&left,&right);

         GET_IFACE(IGirder,pGirder);
         Float64 width = Max(pGirder->GetTopWidth(prevPoi),pGirder->GetBottomWidth(prevPoi));

         startWidth = width + (left+right)/2;
         startW = -startWidth*construction_load;

         pBridge->GetDistanceBetweenGirders(currPoi,&left,&right);

         width = Max(pGirder->GetTopWidth(currPoi),pGirder->GetBottomWidth(currPoi));

         endWidth = width + (left+right)/2;
         endW = -endWidth*construction_load;
      }
      else
      {
         GET_IFACE(ISectionProperties,pSectProp);
         startWidth = pSectProp->GetTributaryFlangeWidth(prevPoi);
         // negative width means that slab is not over girder
         if (startWidth < 0.0)
         {
            startWidth = 0.0;
         }

         startW = -startWidth*construction_load;

         endWidth = pSectProp->GetTributaryFlangeWidth(currPoi);
         if (endWidth < 0.0)
         {
            endWidth = 0.0;
         }

         endW = -endWidth*construction_load;
      }

      // Create load and stuff it
      ConstructionLoad load;
      load.Xstart = prevPoi.GetDistFromStart();
      load.Xend   = currPoi.GetDistFromStart();
      load.StartWcc = startWidth;
      load.EndWcc   = endWidth;
      load.Wstart   = startW;
      load.Wend     = endW;

      if ( pConstructionLoads->size() == 0 )
      {
         pConstructionLoads->push_back(load);
      }
      else
      {
         if ( IsEqual(pConstructionLoads->back().EndWcc,load.EndWcc) && IsEqual(pConstructionLoads->back().Wend,load.Wend) )
         {
            pConstructionLoads->back().Xend = load.Xend;
         }
         else
         {
            pConstructionLoads->push_back(load);
         }
      }
   }
}

void CGirderModelManager::GetMainSpanShearKeyLoad(const CSegmentKey& segmentKey, std::vector<ShearKeyLoad>* pLoads) const
{
   ATLASSERT(pLoads != nullptr); 
   pLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   // Check if there is a shear key before we get too far
   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   bool has_shear_key = pGirder->HasShearKey(segmentKey, spacingType);
   if (!has_shear_key || nGirders == 1)
   {
      // no shear key, or there is only one girder in which case there aren't shear key loads either
      return; // leave now
   }

   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();

   // areas of shear key per interior side
   Float64 unif_area, joint_area;
   pGirder->GetShearKeyAreas(segmentKey, spacingType, &unif_area, &joint_area);

   Float64 unit_weight;
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      unit_weight = pMaterial->GetSegmentWeightDensity(segmentKey, castShearKeyIntervalIdx)*WBFL::Units::System::GetGravitationalAcceleration();
   }
   else
   {
      unit_weight = pMaterial->GetDeckWeightDensity(0/*assume casting region 0*/,castShearKeyIntervalIdx)*WBFL::Units::System::GetGravitationalAcceleration();
   }

   Float64 unif_wt      = unif_area  * unit_weight;
   Float64 joint_wt_per = joint_area * unit_weight;


   // See if we need to go further
   bool is_joint_spacing = ::IsJointSpacing(spacingType);

   if ( IsZero(unif_wt) )
   {
      if ( IsZero(joint_wt_per) || !is_joint_spacing)
      {
         return; // no applied load
      }
   }

   bool is_exterior = pBridge->IsExteriorGirder(segmentKey);
   Float64 nsides = is_exterior ? 1 : 2;

   // If only uniform load, we can apply along entire length
   if ( IsZero(joint_wt_per) )
   {
      ShearKeyLoad load;
      load.StartLoc = 0.0;
      load.EndLoc   = pBridge->GetSegmentLength(segmentKey);

      load.UniformLoad = -unif_wt * nsides;

      load.StartJW = 0.0;
      load.EndJW   = 0.0;
      load.StartJointLoad = 0.0;
      load.EndJointLoad   = 0.0;

      pLoads->push_back(load);
   }
   else
   {
      // We have a joint load - apply across 
      // Get some important POIs that we will be using later
      GET_IFACE(IPointOfInterest,pIPoi);
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size()!=0);

      IndexType num_poi = vPoi.size();
      for ( IndexType i = 0; i < num_poi-1; i++ )
      {
         const pgsPointOfInterest& prevPoi = vPoi[i];
         const pgsPointOfInterest& currPoi = vPoi[i+1];

         // Width of joint, and load intensity
         Float64 startWidth, endWidth;
         Float64 startW, endW;

         Float64 left,right;
         pBridge->GetDistanceBetweenGirders(prevPoi,&left,&right);

         startWidth = (left+right)/2;
         startW = -startWidth*joint_wt_per;

         pBridge->GetDistanceBetweenGirders(currPoi,&left,&right);

         endWidth = (left+right)/2;
         endW = -endWidth*joint_wt_per;

         // Create load and stow it
         ShearKeyLoad load;
         load.StartLoc = prevPoi.GetDistFromStart();
         load.EndLoc   = currPoi.GetDistFromStart();

         load.UniformLoad = -unif_wt * nsides;;

         load.StartJW = startWidth;
         load.EndJW   = endWidth;
         load.StartJointLoad = startW;
         load.EndJointLoad   = endW;

         pLoads->push_back(load);
      }
   }
}

void CGirderModelManager::GetMainSpanLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const
{
   ATLASSERT(pLoads != nullptr);
   pLoads->clear();

   GET_IFACE(IBridge, pBridge);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   // Check if there is a longituidinal joint before we get too far
   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   if (!IsJointSpacing(spacingType) || nGirders == 1)
   {
      // no joint
      return; // leave now
   }

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType compositeLongitudinalJointIntervalIdx = pIntervals->GetCompositeLongitudinalJointInterval();

   // unit weight of joint material
   GET_IFACE(IMaterials, pMaterial);
   Float64 density = pMaterial->GetLongitudinalJointWeightDensity(castLongitudinalJointIntervalIdx);
   Float64 unit_weight = density * WBFL::Units::System::GetGravitationalAcceleration();

   PoiList vPoi;
   GetLinearLoadPointsOfInterest(segmentKey, &vPoi);

   auto iter = vPoi.begin();

   // get area of joints at first POI
   pgsPointOfInterest prevPoi(*iter);
   GET_IFACE(IShapes, pShapes);
   Float64 prevLeftJointArea(0), prevRightJointArea(0);
   CComPtr<IShape> leftShape, rightShape;
   pShapes->GetJointShapes(compositeLongitudinalJointIntervalIdx, prevPoi, false, pgsTypes::scBridge, &leftShape, &rightShape);
   if (leftShape)
   {
      CComPtr<IShapeProperties> properties;
      leftShape->get_ShapeProperties(&properties);
      properties->get_Area(&prevLeftJointArea);
   }

   if (rightShape)
   {
      CComPtr<IShapeProperties> properties;
      rightShape->get_ShapeProperties(&properties);
      properties->get_Area(&prevRightJointArea);
   }

   // work through the rest of the POI, creating linear load segments
   iter++;
   auto end = vPoi.end();
   for (; iter != end; iter++)
   {
      leftShape.Release();
      rightShape.Release();

      pgsPointOfInterest poi(*iter);
      pShapes->GetJointShapes(compositeLongitudinalJointIntervalIdx, poi, false, pgsTypes::scBridge, &leftShape, &rightShape);

      Float64 leftJointArea(0.0), rightJointArea(0.0);
      if (leftShape)
      {
         CComPtr<IShapeProperties> properties;
         leftShape->get_ShapeProperties(&properties);
         properties->get_Area(&leftJointArea);
      }

      if (rightShape)
      {
         CComPtr<IShapeProperties> properties;
         rightShape->get_ShapeProperties(&properties);
         properties->get_Area(&rightJointArea);
      }

      Float64 Xstart = prevPoi.GetDistFromStart();
      Float64 Xend = poi.GetDistFromStart();
      Float64 Wstart = -unit_weight * (prevLeftJointArea + prevRightJointArea);
      Float64 Wend = -unit_weight * (leftJointArea + rightJointArea);
      pLoads->emplace_back(Xstart, Xend, Wstart, Wend);

      prevPoi = poi;
      prevLeftJointArea = leftJointArea;
      prevRightJointArea = rightJointArea;
   }  
}

CComBSTR CGirderModelManager::GetLoadGroupNameForUserLoad(IUserDefinedLoads::UserDefinedLoadCase lc) const
{
   CComBSTR bstrLoadGroup;
   switch(lc)
   {
      case IUserDefinedLoads::userDC:
         bstrLoadGroup =  "UserDC";
      break;

      case IUserDefinedLoads::userDW:
         bstrLoadGroup =  "UserDW";
      break;

      case IUserDefinedLoads::userLL_IM:
         bstrLoadGroup =  "UserLLIM";
      break;

         default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLoadGroup;
}

void CGirderModelManager::AddLoadCase(ILoadCases* loadCases, BSTR name, BSTR description) const
{
   HRESULT hr;
   CComPtr<ILoadCase> load_case;
   hr = load_case.CoCreateInstance(CLSID_LoadCase) ;
   ATLASSERT(SUCCEEDED(hr));
   hr = load_case->put_Name(name) ;
   ATLASSERT(SUCCEEDED(hr));
   hr = load_case->put_Description(name) ;
   ATLASSERT(SUCCEEDED(hr));
   hr = loadCases->Add(load_case) ;
   ATLASSERT(SUCCEEDED(hr));
}

HRESULT CGirderModelManager::AddLoadGroup(ILoadGroups* loadGroups, BSTR name, BSTR description) const
{
   CComPtr<ILoadGroup> load_group;
   HRESULT hr = loadGroups->Find(name,&load_group);
   if ( SUCCEEDED(hr) )
   {
      // the load group has already been added... 
      return hr;
   }

   hr = load_group.CoCreateInstance(CLSID_LoadGroup) ;
   if ( FAILED(hr) )
   {
      ATLASSERT(false);
      return hr;
   }

   hr = load_group->put_Name(name) ;
   if ( FAILED(hr) )
   {
      ATLASSERT(false);
      return hr;
   }

   hr = load_group->put_Description(name) ;
   if ( FAILED(hr) )
   {
      ATLASSERT(false);
      return hr;
   }

   hr = loadGroups->Add(load_group) ;
   if ( FAILED(hr) )
   {
      ATLASSERT(false);
      return hr;
   }

   return hr;
}

CComBSTR CGirderModelManager::GetLoadCombinationName(pgsTypes::LimitState limitState) const
{
#if defined _DEBUG
   CComBSTR bstrLimitState;
   switch(limitState)
   {
      case pgsTypes::ServiceI:
         bstrLimitState = _T("SERVICE-I");
         break;

      case pgsTypes::ServiceIA:
         bstrLimitState = _T("SERVICE-IA");
         break;

      case pgsTypes::ServiceIII:
         bstrLimitState = _T("SERVICE-III");
         break;

      case pgsTypes::StrengthI:
         bstrLimitState = _T("STRENGTH-I");
         break;

      case pgsTypes::StrengthII:
         bstrLimitState = _T("STRENGTH-II");
         break;

      case pgsTypes::FatigueI:
         bstrLimitState = _T("FATIGUE-I");
         break;

      case pgsTypes::StrengthI_Inventory:
         bstrLimitState = _T("STRENGTH-I-Inventory");
         break;

      case pgsTypes::StrengthI_Operating:
         bstrLimitState = _T("STRENGTH-I-Operating");
         break;

      case pgsTypes::ServiceIII_Inventory:
         bstrLimitState = _T("SERVICE-III-Inventory");
         break;

      case pgsTypes::ServiceIII_Operating:
         bstrLimitState = _T("SERVICE-III-Operating");
         break;

      case pgsTypes::StrengthI_LegalRoutine:
         bstrLimitState = _T("STRENGTH-I-LegalRoutine");
         break;

      case pgsTypes::StrengthI_LegalSpecial:
         bstrLimitState = _T("STRENGTH-I-LegalSpecial");
         break;

      case pgsTypes::StrengthI_LegalEmergency:
         bstrLimitState = _T("STRENGTH-I-LegalEmergency");
         break;

      case pgsTypes::ServiceIII_LegalRoutine:
         bstrLimitState = _T("SERVICE-III-LegalRoutine");
         break;

      case pgsTypes::ServiceIII_LegalSpecial:
         bstrLimitState = _T("SERVICE-III-LegalSpecial");
         break;

      case pgsTypes::ServiceIII_LegalEmergency:
         bstrLimitState = _T("SERVICE-III-LegalEmergency");
         break;

      case pgsTypes::StrengthII_PermitRoutine:
         bstrLimitState = _T("STRENGTH-II-PermitRoutine");
         break;

      case pgsTypes::ServiceI_PermitRoutine:
         bstrLimitState = _T("SERVICE-I-PermitRoutine");
         break;

      case pgsTypes::ServiceIII_PermitRoutine:
         bstrLimitState = _T("SERVICE-III-PermitRoutine");
         break;

      case pgsTypes::StrengthII_PermitSpecial:
         bstrLimitState = _T("STRENGTH-II-PermitSpecial");
         break;

      case pgsTypes::ServiceI_PermitSpecial:
         bstrLimitState = _T("SERVICE-I-PermitSpecial");
         break;

      case pgsTypes::ServiceIII_PermitSpecial:
         bstrLimitState = _T("SERVICE-III-PermitSpecial");
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   ATLASSERT(gs_LimitStateNames[limitState] == bstrLimitState);
#endif

   return gs_LimitStateNames[limitState];
}

pgsTypes::LimitState CGirderModelManager::GetLimitStateFromLoadCombination(CComBSTR bstrLoadCombinationName) const
{
   for ( int i = 0; i < gs_nLimitStates; i++ )
   {
      if ( bstrLoadCombinationName == gs_LimitStateNames[i] )
      {
         pgsTypes::LimitState ls = pgsTypes::LimitState(i);
         ATLASSERT(bstrLoadCombinationName == GetLoadCombinationName(ls));
         return ls;
      }
   }
   ATLASSERT(false); // should never get here
   return pgsTypes::ServiceI;
}

CComBSTR CGirderModelManager::GetLiveLoadName(pgsTypes::LiveLoadType llt) const
{
#if defined _DEBUG
   CComBSTR bstrLiveLoadName;
   switch(llt)
   {
      case pgsTypes::lltDesign:
         bstrLiveLoadName = "LL+IM Design";
         break;

      case pgsTypes::lltPermit:
         bstrLiveLoadName = "LL+IM Permit";
         break;

      case pgsTypes::lltFatigue:
         bstrLiveLoadName = "LL+IM Fatigue";
         break;

      case pgsTypes::lltPedestrian:
         bstrLiveLoadName = "LL+IM Pedestrian";
         break;

      case pgsTypes::lltLegalRating_Routine:
         bstrLiveLoadName = "LL+IM Legal Rating (Routine)";
         break;

      case pgsTypes::lltLegalRating_Special:
         bstrLiveLoadName = "LL+IM Legal Rating (Special)";
         break;

      case pgsTypes::lltLegalRating_Emergency:
         bstrLiveLoadName = "LL+IM Legal Rating (Emergency)";
         break;

      case pgsTypes::lltPermitRating_Routine:
         bstrLiveLoadName = "LL+IM Permit Rating (Routine)";
         break;

      case pgsTypes::lltPermitRating_Special:
         bstrLiveLoadName = "LL+IM Permit Rating (Special)";
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }
   ATLASSERT(gs_LiveLoadNames[llt] == bstrLiveLoadName);
#endif

   return gs_LiveLoadNames[llt];
}

MemberIDType CGirderModelManager::GetFirstSuperstructureMemberID(const CSegmentKey& segmentKey) const
{
   // segments can be modeled with up to 3 superstructure members (even more if we ever model them as non-prismatc)
   // this method returns the ID of the first SSMbr used to model the specified segment
   GroupIndexType   grpIdx = segmentKey.groupIndex;
   GirderIndexType  gdrIdx = segmentKey.girderIndex;
   SegmentIndexType segIdx = segmentKey.segmentIndex;
   ATLASSERT(grpIdx != ALL_GROUPS && gdrIdx != ALL_GIRDERS && segIdx != ALL_SEGMENTS);

   GET_IFACE(IBridge,pBridge);
   GET_IFACE_NOCHECK(IBridgeDescription,pIBridgeDesc); // only used if there are more than one segment per girder

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);

   // determine the superstructure member ID
   MemberIDType mbrID = 0;

   // count number of SSMBRs in all groups and segments before the segment we are interested in
   for ( GroupIndexType g = 0; g < grpIdx; g++ )
   {
      GirderIndexType nGirdersThisGroup = pBridge->GetGirderCount((GroupIndexType)g);
      CGirderKey thisGirderKey(g,Min(gdrIdx,nGirdersThisGroup-1));
      SegmentIndexType nSegmentsThisGroup = pBridge->GetSegmentCount( thisGirderKey );
      for ( SegmentIndexType s = 0; s < nSegmentsThisGroup; s++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,s);

         IndexType nSSMbrs = GetSuperstructureMemberCount(thisSegmentKey);

         const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier )
         {
            nSSMbrs += GetSuperstructureMemberCount(pPier,thisSegmentKey.girderIndex);
         }
         else
         {
            nSSMbrs += GetSuperstructureMemberCount(pTS,thisSegmentKey.girderIndex);
         }
         mbrID += (MemberIDType)nSSMbrs;
      } // next segment
   } // next group

   // count number of SSMBRs in this group, up to but not including the segment we are interested in
   for ( SegmentIndexType s = 0; s < segmentKey.segmentIndex; s++ )
   {
      CSegmentKey thisSegmentKey(grpIdx,gdrIdx,s);
      IndexType nSSMbrs = GetSuperstructureMemberCount(thisSegmentKey);

      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
      const CPierData2* pPier;
      const CTemporarySupportData* pTS;
      pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);

      if ( pPier )
      {
         nSSMbrs += GetSuperstructureMemberCount(pPier,gdrIdx);
      }
      else
      {
         nSSMbrs += GetSuperstructureMemberCount(pTS,gdrIdx);
      }

      mbrID += (MemberIDType)nSSMbrs;
   }

   return mbrID; // superstructure member ID for the SSMBR at the start of the segment (usually the ID of the left overhang)
}

IndexType CGirderModelManager::GetSuperstructureMemberCount(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridge,pBridge);

   Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_offset   = pBridge->GetSegmentEndEndDistance(segmentKey);

   IndexType nSSMbrs = 0;
   if ( !IsZero(start_offset) )
   {
      nSSMbrs++; // start overhang
   }

   nSSMbrs++; // main segment

   if ( !IsZero(end_offset) )
   {
      nSSMbrs++; // end overhang
   }

   return nSSMbrs;
}

IndexType CGirderModelManager::GetSuperstructureMemberCount(const CPierData2* pPier,GirderIndexType gdrIdx) const
{
   ATLASSERT(!pPier->IsAbutment()); // SSMbr count at abutments are handled with the segments

   IndexType nSSMbrs = 0;

   pgsTypes::PierSegmentConnectionType pierSegmentConnectionType = (pPier->IsInteriorPier() ? pPier->GetSegmentConnectionType() : pgsTypes::psctContinousClosureJoint);
   if ( pPier->IsBoundaryPier() || (pierSegmentConnectionType == pgsTypes::psctContinousClosureJoint || pierSegmentConnectionType == pgsTypes::psctIntegralClosureJoint) )
   {
      // we model left and right side of cast-in-place diaphragm at permanent piers
      GET_IFACE(IBridge, pBridge);

      CSegmentKey backSegmentKey, aheadSegmentKey;
      pBridge->GetSegmentsAtPier(pPier->GetIndex(), gdrIdx, &backSegmentKey, &aheadSegmentKey);
      ATLASSERT(backSegmentKey.segmentIndex != INVALID_INDEX);
      ATLASSERT(aheadSegmentKey.segmentIndex != INVALID_INDEX);

      // if the bearing offset is zero, the point of bearing is at the CL Pier so the
      // superstructure member through the diaphragm isn't modeled
      Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(backSegmentKey);
      Float64 endDist = pBridge->GetSegmentEndEndDistance(backSegmentKey);
      if (!IsZero(brgOffset - endDist))
      {
         nSSMbrs += 1;
      }

      brgOffset = pBridge->GetSegmentStartBearingOffset(aheadSegmentKey);
      endDist = pBridge->GetSegmentStartEndDistance(aheadSegmentKey);
      if (!IsZero(brgOffset - endDist))
      {
         nSSMbrs += 1;
      }
   }
#if defined _DEBUG
   else
   {
      ATLASSERT(pPier->IsInteriorPier());
      ATLASSERT(pierSegmentConnectionType == pgsTypes::psctContinuousSegment || pierSegmentConnectionType == pgsTypes::psctIntegralSegment);
      ATLASSERT(nSSMbrs == 0);
   }
#endif

   return nSSMbrs;
}

IndexType CGirderModelManager::GetSuperstructureMemberCount(const CTemporarySupportData* pTS,GirderIndexType gdrIdx) const
{
   if ( pTS->GetConnectionType() == pgsTypes::tsctClosureJoint )
   {
      // This is what we would do if we supported match casting
      //const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(gdrIdx);
      //CClosureKey closureKey = pClosureJoint->GetClosureKey();
      //GET_IFACE(IBridge,pBridge);
      //Float64 closure_length = pBridge->GetClosureJointLength(closureKey);
      //return ( IsZero(closure_length) ? 0 : 1);

      return 1; // temporary supports with closure joints are modeled with 1 member
   }
   else
   {
      ATLASSERT( pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment );
      return 0; // no superstructure members for continuous segments
   }
}

void CGirderModelManager::GetPosition(ILBAMModel* pLBAMModel,const CSegmentKey& segmentKey,Float64 Xs,MemberType* pMbrType,MemberIDType* pMbrID,Float64* pX) const
{
   MemberIDType startMbrID = GetFirstSuperstructureMemberID(segmentKey); // ID of first superstructure member modeling the segment
   IndexType nSSMbrs = GetSuperstructureMemberCount(segmentKey); // number of superstructure members modeling the segment

   *pMbrType = mtSuperstructureMember;

   CComPtr<ISuperstructureMembers> ssmbrs;
   pLBAMModel->get_SuperstructureMembers(&ssmbrs);

   Float64 X = 0;
   for ( IndexType i = 0; i < nSSMbrs; i++ )
   {
      MemberIDType mbrID = startMbrID + (MemberIDType)i;
      CComPtr<ISuperstructureMember> ssmbr;
      ssmbrs->get_Item(mbrID,&ssmbr);
      Float64 length;
      ssmbr->get_Length(&length);
      if ( InRange(X,Xs,X+length) )
      {
         *pMbrID = mbrID;
         *pX = Xs - X;
         return;
      }

      X += length;
   }

   ATLASSERT(false); // should not get here
}

void CGirderModelManager::GetLoadPosition(ILBAMModel* pLBAMModel,const CSegmentKey& segmentKey,Float64 Xs,bool bLoadCantilevers,MemberType* pMbrType,MemberIDType* pMbrID,Float64* pXmbr) const
{
   GET_IFACE(IBridge,pBridge);
   Float64 start_offset     = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_offset       = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 segment_length   = pBridge->GetSegmentLength(segmentKey);

   bool bModelStartCantilever,bModelEndCantilever;
   pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);
   if ( bLoadCantilevers )
   {
      bModelStartCantilever = true;
      bModelEndCantilever   = true;
   }

   *pMbrType = mtSuperstructureMember;

   MemberIDType mbrID = INVALID_ID;

   if ( Xs < start_offset )
   {
      // load is on the start cantilever
      mbrID = GetFirstSuperstructureMemberID(segmentKey);

      if ( bModelStartCantilever )
      {
         *pXmbr = Xs;
      }
      else
      {
         // Load occurs before CL bearing and the cantilever is not being modeled.
         // Put the load directly over the bearing so it produces only a reaction.
         if ( !IsZero(start_offset) )
         {
            mbrID++; // current member is the little cantilever, move to next member (the main span)
         }
         *pXmbr = 0.0;
      }
   }
   else if ( segment_length-end_offset < Xs )
   {
      // load is on the end cantilever
      mbrID = GetFirstSuperstructureMemberID(segmentKey);
      IndexType nSSMbrs = GetSuperstructureMemberCount(segmentKey);
      mbrID += nSSMbrs-1;

      if ( bModelEndCantilever )
      {
         // Load goes on the next superstructure member (the little cantilever at the end)
         *pXmbr = Xs - (segment_length - end_offset);
      }
      else
      {
         // Load is after the CL Bearing at the end of the girder and the cantilever is not being modeled.
         // Put the load directly over the bearing so it produces only a reaction.
         if ( !IsZero(start_offset) )
         {
            ATLASSERT(mbrID != 0);
            mbrID--; // current member is the little cantilever, move to prev member (the main span)
         }
         *pXmbr = segment_length - start_offset - end_offset;
      }
   }
   else
   {
      // Load is on the main portion of the segment. Adjust the location
      // so it is measured from the start of the superstructure member
      mbrID = GetFirstSuperstructureMemberID(segmentKey);
      if ( !IsZero(start_offset) )
      {
         mbrID++; // current member is the little cantilever, move to next member (the main span)
      }
      *pXmbr = Xs - start_offset;
   }

   *pMbrID = mbrID;

#if defined _DEBUG
   ATLASSERT(mbrID != INVALID_ID);
   CComPtr<ISuperstructureMembers> ssmbrs;
   pLBAMModel->get_SuperstructureMembers(&ssmbrs);
   CComPtr<ISuperstructureMember> ssmbr;
   ssmbrs->get_Item(mbrID,&ssmbr);
   Float64 ssmbrLength;
   ssmbr->get_Length(&ssmbrLength);
   ATLASSERT(::InRange(0.0,*pXmbr,ssmbrLength));
#endif
}

// Implementation functions and data for IBearingDesign
void CGirderModelManager::ApplyOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup,
                                                MemberIDType mbrID,Float64 Pstart, Float64 Xstart, Float64 Pend, Float64 Xend, IPointLoads* pointLoads) const
{
   ATLASSERT(analysisType == pgsTypes::Simple || analysisType == pgsTypes::Continuous);
   // Create and apply loads to the LBAM
   if ( !IsZero(Pstart) )
   {
      CComPtr<IPointLoad> loadPstart;
      loadPstart.CoCreateInstance(CLSID_PointLoad);
      loadPstart->put_MemberType(mtSuperstructureMember);
      loadPstart->put_MemberID(mbrID);
      loadPstart->put_Location(Xstart);
      loadPstart->put_Fy(Pstart);

      CComPtr<IPointLoadItem> ptLoadItem;
      pointLoads->Add(bstrStage,bstrLoadGroup,loadPstart,&ptLoadItem);
   }

   if ( !IsZero(Pend) )
   {
      CComPtr<IPointLoad> loadPend;
      loadPend.CoCreateInstance(CLSID_PointLoad);
      loadPend->put_MemberType(mtSuperstructureMember);
      loadPend->put_MemberID(mbrID);
      loadPend->put_Location(Xend);
      loadPend->put_Fy(Pend);

      CComPtr<IPointLoadItem> ptLoadItem;
      pointLoads->Add(bstrStage,bstrLoadGroup,loadPend,&ptLoadItem);
   }

   // Store load so we can use it when computing bearing reactions
   SaveOverhangPointLoads(segmentKey,analysisType,bstrStage,bstrLoadGroup,Pstart,Pend);
}

void CGirderModelManager::SaveOverhangPointLoads(const CSegmentKey& segmentKey,pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage,const CComBSTR& bstrLoadGroup,Float64 Pstart,Float64 Pend) const
{
   if ( !IsZero(Pstart) || !IsZero(Pend) )
   {
      COverhangLoadData new_val(segmentKey, bstrStage, bstrLoadGroup, Pstart, Pend);
      auto lit = m_OverhangLoadSet[analysisType].insert( new_val );
      if ( lit.second == false )
      {
         COverhangLoadData& data(const_cast<COverhangLoadData&>(*lit.first));
         data.PStart += Pstart;
         data.PEnd   += Pend;
      }
   }
}

bool CGirderModelManager::GetOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType, IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,
                                              ResultsType resultsType, Float64* pPStart, Float64* pPEnd) const
{
   ATLASSERT(analysisType == pgsTypes::Simple || analysisType == pgsTypes::Continuous);
   *pPStart = 0.0;
   *pPEnd   = 0.0;

   // Need to sum results over stages, so use bridgesite ordering 
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType startIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
   std::vector<IntervalIndexType> intervals;
   for ( IntervalIndexType i = startIntervalIdx; i < nIntervals; i++ )
   {
      intervals.push_back(i);
   }

   // Start of interval loop
   auto found = std::find(intervals.begin(),intervals.end(),intervalIdx);
   IntervalIndexType end = *found;
   IntervalIndexType start = end;

   if (found == intervals.end() )
   {
      ATLASSERT(false); // shouldn't be passing in non-bridge site stages?
      return false;
   }
   else
   {
      CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );

      // Determine end of loop range
      if (resultsType == rtCumulative)
      {
         start = intervals.front();
      }
      else
      {
         start = end;
      }

      bool bFound = false;
      for (IntervalIndexType idx = start; idx <= end; idx++ )
      {
         CComBSTR bstrStage( GetLBAMStageName(idx) );

         auto lit = m_OverhangLoadSet[analysisType].find( COverhangLoadData(segmentKey, bstrStage, bstrLoadGroup, 0.0, 0.0) );
         if (lit != m_OverhangLoadSet[analysisType].end())
         {
            *pPStart += lit->PStart;
            *pPEnd   += lit->PEnd;
            bFound = true;
         }
      }

      return bFound;
   }
}

void CGirderModelManager::ValidateGirderModels(const CGirderKey& girderKey) const
{
   // This will validate the bridge site stage models
   GetGirderModel(GetGirderLineIndex(girderKey));
}

GirderIndexType CGirderModelManager::GetGirderLineIndex(const CGirderKey& girderKey) const
{
   return girderKey.girderIndex;
}

void CGirderModelManager::DumpAnalysisModels(GirderIndexType gdrLineIdx) const
{
   CGirderModelData* pModelData = GetGirderModel(gdrLineIdx);

   if ( pModelData->m_Model )
   {
      CComPtr<IStructuredSave2> save;
      save.CoCreateInstance(CLSID_StructuredSave2);
      std::_tostringstream ss;
      ss << _T("LBAM_SimpleSpan_Girder_") << gdrLineIdx << _T(".xml");
      save->Open(CComBSTR(ss.str().c_str()));

      CComQIPtr<IStructuredStorage2> strstorage(pModelData->m_Model);
      strstorage->Save(save);

      save->Close();

      CComQIPtr<IDiagnostics> diagnostics(pModelData->pLoadGroupResponse[pgsTypes::Simple]);
      diagnostics->DumpFEMModels();
   }

   if ( pModelData->m_ContinuousModel )
   {
      CComPtr<IStructuredSave2> save;
      save.CoCreateInstance(CLSID_StructuredSave2);
      std::_tostringstream ss;
      ss << _T("LBAM_ContinuousSpan_Girder_") << gdrLineIdx << _T(".xml");
      save->Open(CComBSTR(ss.str().c_str()));

      CComQIPtr<IStructuredStorage2> strstorage(pModelData->m_ContinuousModel);
      strstorage->Save(save);

      save->Close();

      CComQIPtr<IDiagnostics> diagnostics(pModelData->pLoadGroupResponse[pgsTypes::Continuous]);
      diagnostics->DumpFEMModels();
   }
}

void CGirderModelManager::RenameLiveLoad(ILBAMModel* pModel,pgsTypes::LiveLoadType llType,LPCTSTR strOldName,LPCTSTR strNewName)
{
   CComPtr<ILiveLoad> live_load;
   pModel->get_LiveLoad(&live_load);

   // get the live load model
   CComPtr<ILiveLoadModel> liveload_model;
   if ( llType == pgsTypes::lltDesign )
   {
      live_load->get_Design(&liveload_model);
   }
   else
   {
      live_load->get_Permit(&liveload_model);
   }

   // get the vehicular loads collection
   CComPtr<IVehicularLoads> vehicles;
   liveload_model->get_VehicularLoads(&vehicles);

   CComPtr<IEnumVehicularLoad> enum_vehicles;
   vehicles->get__EnumElements(&enum_vehicles);

   CComPtr<IVehicularLoad> vehicle;
   while ( enum_vehicles->Next(1,&vehicle,nullptr) != S_FALSE )
   {
      CComBSTR bstrName;
      vehicle->get_Name(&bstrName);
      if ( std::_tstring(strOldName) == std::_tstring(OLE2T(bstrName)) )
      {
         vehicle->put_Name(CComBSTR(strNewName));
         break;
      }

      vehicle.Release();
   }
}

CGirderModelData* CGirderModelManager::UpdateLBAMPois(const PoiList& vPoi) const
{
   ATLASSERT(VerifyPoi(vPoi));

   m_LBAMPoi->Clear();

   // Start by checking if the model exists
   // get the maximum girder index because this will govern which model we want to get
   //
   // Example: Span 1 - 4 Girders
   //          Span 2 - 5 Girders
   //          called vPoi = pPOI->GetGirderPointsOfInterest(-1,4); to get the vector of poi's for girder line 4
   //          there is not a girder line index 4 in span 1 so GetGirderPointsOfInterest returns poi's for girder line index 3
   //          the vector of POI have mixed girder lines
   //          we want the model that corresponds to the max girder index
   //
   CGirderKey girderKey(0,0);
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      GroupIndexType  grpIdx = poi.GetSegmentKey().groupIndex;
      GirderIndexType gdrIdx = poi.GetSegmentKey().girderIndex;

      if ( girderKey.girderIndex < gdrIdx )
      {
         girderKey.groupIndex  = grpIdx;
         girderKey.girderIndex = gdrIdx;
      }
   }

   // get the model
   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(GetGirderLineIndex(girderKey));

   for(const pgsPointOfInterest& poi : vPoi)
   {
      PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
      if ( poi_id == INVALID_ID )
      {
         poi_id = AddPointOfInterest( pModelData, poi );
         ATLASSERT( 0 <= poi_id );
         if ( 0 <= poi_id )
         {
            m_LBAMPoi->Add(poi_id);
         }
      }
      else
      {
         m_LBAMPoi->Add(poi_id);
      }
   }

#if defined _DEBUG
   CollectionIndexType nPOI;
   m_LBAMPoi->get_Count(&nPOI);
   ATLASSERT( nPOI == vPoi.size() );
#endif

   return pModelData;
}

void CGirderModelManager::GetLBAM(CGirderModelData* pModelData,pgsTypes::BridgeAnalysisType bat,ILBAMModel** ppModel) const
{
   switch( bat )
   {
   case pgsTypes::SimpleSpan:
      (*ppModel) = pModelData->m_Model;
      if ( *ppModel )
      {
         (*ppModel)->AddRef();
      }

      break;

   case pgsTypes::ContinuousSpan:
   case pgsTypes::MinSimpleContinuousEnvelope:
   case pgsTypes::MaxSimpleContinuousEnvelope:
      (*ppModel) = pModelData->m_ContinuousModel;
      if ( *ppModel )
      {
         (*ppModel)->AddRef();
      }

      break;

   default:
      ATLASSERT(false);
   }
}

bool CGirderModelManager::CreateConcentratedLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   USES_CONVERSION;
   return CreateConcentratedLoad(pModel,poiMap,intervalIdx,OLE2T(m_ProductLoadMap.GetGroupLoadName(pfType)),poi,Fx,Fy,Mz);
}

bool CGirderModelManager::CreateConcentratedLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
   PoiIDType lbamPoiID = poiMap.GetModelPoi(poi);
   ATLASSERT(lbamPoiID != INVALID_ID);

   CComPtr<IPOIs> pois;
   pModel->get_POIs(&pois);

   CComPtr<IPOI> lbamPOI;
   pois->Find(lbamPoiID,&lbamPOI);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   MemberType mbrType;
   MemberIDType mbrID;
   Float64 location;
   lbamPOI->get_MemberType(&mbrType);
   lbamPOI->get_MemberID(&mbrID);
   lbamPOI->get_Location(&location);

   CComPtr<IPointLoadItem> ptLoadItem;
   CComPtr<IPointLoad> ptLoad;
   ptLoad.CoCreateInstance(CLSID_PointLoad);
   ptLoad->put_MemberType(mbrType);
   ptLoad->put_MemberID(mbrID);
   ptLoad->put_Location(location);

   ptLoad->put_Fx(Fx);
   ptLoad->put_Fy(Fy);
   ptLoad->put_Mz(Mz);

   pointLoads->Add(bstrStage,CComBSTR(strLoadingName),ptLoad,&ptLoadItem);

   return true;
}

bool CGirderModelManager::CreateUniformLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   USES_CONVERSION;
   return CreateUniformLoad(pModel,poiMap,intervalIdx,OLE2T(m_ProductLoadMap.GetGroupLoadName(pfType)),poi1,poi2,wx,wy);
}

bool CGirderModelManager::CreateUniformLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
   PoiIDType lbamPoiID1 = poiMap.GetModelPoi(poi1);
   ATLASSERT(lbamPoiID1 != INVALID_ID);

   PoiIDType lbamPoiID2 = poiMap.GetModelPoi(poi2);
   ATLASSERT(lbamPoiID2 != INVALID_ID);

   CComPtr<IPOIs> pois;
   pModel->get_POIs(&pois);

   CComPtr<IPOI> lbamPOI1, lbamPOI2;
   pois->Find(lbamPoiID1,&lbamPOI1);
   pois->Find(lbamPoiID2,&lbamPOI2);

   CComPtr<IDistributedLoads> distributedLoads;
   pModel->get_DistributedLoads(&distributedLoads);

   MemberType mbrType1;
   MemberIDType mbrID1;
   Float64 location1;
   lbamPOI1->get_MemberType(&mbrType1);
   lbamPOI1->get_MemberID(&mbrID1);
   lbamPOI1->get_Location(&location1);

   MemberType mbrType2;
   MemberIDType mbrID2;
   Float64 location2;
   lbamPOI2->get_MemberType(&mbrType2);
   lbamPOI2->get_MemberID(&mbrID2);
   lbamPOI2->get_Location(&location2);

   ATLASSERT(mbrType1 == mbrType2);
   ATLASSERT(mbrType1 == mtSuperstructureMember);

   // if POI are on different members
   if ( mbrID1 != mbrID2 )
   {
      ATLASSERT(mbrID1 < mbrID2);
      CComPtr<ISuperstructureMembers> ssmbrs;
      pModel->get_SuperstructureMembers(&ssmbrs);
      CComPtr<ISuperstructureMember> ssmbr1;
      ssmbrs->get_Item(mbrID1,&ssmbr1);

      Float64 length1;
      ssmbr1->get_Length(&length1);

      for ( IDType id = mbrID1; id <= mbrID2; id++ )
      {
         if ( id == mbrID1 )
         {
            if ( !IsEqual(location1,length1) )
            {
               // model the load if it doesn't start and the end of member
               if ( !IsZero(wx) )
               {
                  CComPtr<IDistributedLoadItem> distLoadItem;
                  CComPtr<IDistributedLoad> distLoad;
                  distLoad.CoCreateInstance(CLSID_DistributedLoad);
                  distLoad->put_MemberType(mbrType1);
                  distLoad->put_MemberID(mbrID1);
                  distLoad->put_Direction(ldFx);
                  distLoad->put_Orientation(loGlobal);
                  distLoad->put_StartLocation(location1);
                  distLoad->put_EndLocation(length1);
                  distLoad->put_WStart(wx);
                  distLoad->put_WEnd(wx);
                  distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
               }

               if ( !IsZero(wy) )
               {
                  CComPtr<IDistributedLoadItem> distLoadItem;
                  CComPtr<IDistributedLoad> distLoad;
                  distLoad.CoCreateInstance(CLSID_DistributedLoad);
                  distLoad->put_MemberType(mbrType1);
                  distLoad->put_MemberID(mbrID1);
                  distLoad->put_Direction(ldFy);
                  distLoad->put_Orientation(loGlobal);
                  distLoad->put_StartLocation(location1);
                  distLoad->put_EndLocation(length1);
                  distLoad->put_WStart(wy);
                  distLoad->put_WEnd(wy);
                  distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
               }
            }
         }
         else if ( id == mbrID2 )
         {
            if ( !IsZero(location2) )
            {
               // model the load if it doesn't end at the start of this member
               if ( !IsZero(wx) )
               {
                  CComPtr<IDistributedLoadItem> distLoadItem;
                  CComPtr<IDistributedLoad> distLoad;
                  distLoad.CoCreateInstance(CLSID_DistributedLoad);
                  distLoad->put_MemberType(mbrType2);
                  distLoad->put_MemberID(mbrID2);
                  distLoad->put_Direction(ldFx);
                  distLoad->put_Orientation(loGlobal);
                  distLoad->put_StartLocation(0.0);
                  distLoad->put_EndLocation(location2);
                  distLoad->put_WStart(wx);
                  distLoad->put_WEnd(wx);
                  distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
               }

               if ( !IsZero(wy) )
               {
                  CComPtr<IDistributedLoadItem> distLoadItem;
                  CComPtr<IDistributedLoad> distLoad;
                  distLoad.CoCreateInstance(CLSID_DistributedLoad);
                  distLoad->put_MemberType(mbrType2);
                  distLoad->put_MemberID(mbrID2);
                  distLoad->put_Direction(ldFy);
                  distLoad->put_Orientation(loGlobal);
                  distLoad->put_StartLocation(0.0);
                  distLoad->put_EndLocation(location2);
                  distLoad->put_WStart(wy);
                  distLoad->put_WEnd(wy);
                  distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
               }
            }
         }
         else
         {
            // this is an intermediate member between mbrID1 && mbrID2
            // the load goes over the entire length of the member
            ATLASSERT(mbrID1 < id && id < mbrID2);
            if ( !IsZero(wx) )
            {
               CComPtr<IDistributedLoadItem> distLoadItem;
               CComPtr<IDistributedLoad> distLoad;
               distLoad.CoCreateInstance(CLSID_DistributedLoad);
               distLoad->put_MemberType(mbrType1);
               distLoad->put_MemberID(id);
               distLoad->put_Direction(ldFx);
               distLoad->put_Orientation(loGlobal);
               distLoad->put_StartLocation(0.0);
               distLoad->put_EndLocation(-1.0);
               distLoad->put_WStart(wx);
               distLoad->put_WEnd(wx);
               distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
            }

            if ( !IsZero(wy) )
            {
               CComPtr<IDistributedLoadItem> distLoadItem;
               CComPtr<IDistributedLoad> distLoad;
               distLoad.CoCreateInstance(CLSID_DistributedLoad);
               distLoad->put_MemberType(mbrType1);
               distLoad->put_MemberID(id);
               distLoad->put_Direction(ldFy);
               distLoad->put_Orientation(loGlobal);
               distLoad->put_StartLocation(0.0);
               distLoad->put_EndLocation(-1.0);
               distLoad->put_WStart(wy);
               distLoad->put_WEnd(wy);
               distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
            }
         }
      } // next id
   }
   else
   {
      // load is applied to a single member
      if ( !IsZero(wx) )
      {
         CComPtr<IDistributedLoadItem> distLoadItem;
         CComPtr<IDistributedLoad> distLoad;
         distLoad.CoCreateInstance(CLSID_DistributedLoad);
         distLoad->put_MemberType(mbrType1);
         distLoad->put_MemberID(mbrID1);
         distLoad->put_Direction(ldFx);
         distLoad->put_Orientation(loGlobal);
         distLoad->put_StartLocation(location1);
         distLoad->put_EndLocation(location2);
         distLoad->put_WStart(wx);
         distLoad->put_WEnd(wx);
         distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
      }

      if ( !IsZero(wy) )
      {
         CComPtr<IDistributedLoadItem> distLoadItem;
         CComPtr<IDistributedLoad> distLoad;
         distLoad.CoCreateInstance(CLSID_DistributedLoad);
         distLoad->put_MemberType(mbrType1);
         distLoad->put_MemberID(mbrID1);
         distLoad->put_Direction(ldFy);
         distLoad->put_Orientation(loGlobal);
         distLoad->put_StartLocation(location1);
         distLoad->put_EndLocation(location2);
         distLoad->put_WStart(wy);
         distLoad->put_WEnd(wy);
         distributedLoads->Add(bstrStage,CComBSTR(strLoadingName),distLoad,&distLoadItem);
      }
   }

   return true;
}

bool CGirderModelManager::CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   USES_CONVERSION;
   return CreateInitialStrainLoad(pModel,poiMap,intervalIdx,OLE2T(m_ProductLoadMap.GetGroupLoadName(pfType)),poi1,poi2,e,r);
}

bool CGirderModelManager::CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
   PoiIDType lbamPoiID1 = poiMap.GetModelPoi(poi1);
   ATLASSERT(lbamPoiID1 != INVALID_ID);

   PoiIDType lbamPoiID2 = poiMap.GetModelPoi(poi2);
   ATLASSERT(lbamPoiID2 != INVALID_ID);

   CComPtr<IPOIs> pois;
   pModel->get_POIs(&pois);

   CComPtr<IPOI> lbamPOI1, lbamPOI2;
   pois->Find(lbamPoiID1,&lbamPOI1);
   pois->Find(lbamPoiID2,&lbamPOI2);

   CComPtr<IStrainLoads> strainLoads;
   pModel->get_StrainLoads(&strainLoads);

   MemberType mbrType1;
   MemberIDType mbrID1;
   Float64 location1;
   lbamPOI1->get_MemberType(&mbrType1);
   lbamPOI1->get_MemberID(&mbrID1);
   lbamPOI1->get_Location(&location1);

   MemberType mbrType2;
   MemberIDType mbrID2;
   Float64 location2;
   lbamPOI2->get_MemberType(&mbrType2);
   lbamPOI2->get_MemberID(&mbrID2);
   lbamPOI2->get_Location(&location2);

   ATLASSERT(mbrType1 == mbrType2);
   ATLASSERT(mbrType1 == mtSuperstructureMember);

   // if POI are on different members
   if ( mbrID1 != mbrID2 )
   {
      ATLASSERT(mbrID1 < mbrID2);
      CComPtr<ISuperstructureMembers> ssmbrs;
      pModel->get_SuperstructureMembers(&ssmbrs);
      CComPtr<ISuperstructureMember> ssmbr1;
      ssmbrs->get_Item(mbrID1,&ssmbr1);

      Float64 length1;
      ssmbr1->get_Length(&length1);

      for ( IDType id = mbrID1; id <= mbrID2; id++ )
      {
         if ( id == mbrID1 )
         {
            if ( !IsEqual(location1,length1) )
            {
               // model the load if it doesn't start and the end of member
               CComPtr<IStrainLoadItem> strainLoadItem;
               CComPtr<IStrainLoad> strainLoad;
               strainLoad.CoCreateInstance(CLSID_StrainLoad);
               strainLoad->put_MemberType(mbrType1); // mbrType1 must equal mbrType2
               strainLoad->put_MemberID(mbrID1);
               strainLoad->put_StartLocation(location1);
               strainLoad->put_EndLocation(length1);
               strainLoad->put_AxialStrain(e);
               strainLoad->put_CurvatureStrain(r);
               strainLoads->Add(bstrStage,CComBSTR(strLoadingName),strainLoad,&strainLoadItem);
            }
         }
         else if ( id == mbrID2 )
         {
            if ( !IsZero(location2) )
            {
               // model the load if it doesn't end at the start of this member
               CComPtr<IStrainLoadItem> strainLoadItem;
               CComPtr<IStrainLoad> strainLoad;
               strainLoad.CoCreateInstance(CLSID_StrainLoad);
               strainLoad->put_MemberType(mbrType2); // mbrType1 must equal mbrType2
               strainLoad->put_MemberID(mbrID2);
               strainLoad->put_StartLocation(0.0);
               strainLoad->put_EndLocation(location2);
               strainLoad->put_AxialStrain(e);
               strainLoad->put_CurvatureStrain(r);
               strainLoads->Add(bstrStage,CComBSTR(strLoadingName),strainLoad,&strainLoadItem);
            }
         }
         else
         {
            // this is an intermediate member between mbrID1 && mbrID2
            // the load goes over the entire length of the member
            ATLASSERT(mbrID1 < id && id < mbrID2);
            CComPtr<IStrainLoadItem> strainLoadItem;
            CComPtr<IStrainLoad> strainLoad;
            strainLoad.CoCreateInstance(CLSID_StrainLoad);
            strainLoad->put_MemberType(mbrType1); // mbrType1 must equal mbrType2
            strainLoad->put_MemberID(id);
            strainLoad->put_StartLocation(0);
            strainLoad->put_EndLocation(-1);
            strainLoad->put_AxialStrain(e);
            strainLoad->put_CurvatureStrain(r);
            strainLoads->Add(bstrStage,CComBSTR(strLoadingName),strainLoad,&strainLoadItem);
         }
      }
   }
   else
   {
      // load is applied to a single member
      CComPtr<IStrainLoadItem> strainLoadItem;
      CComPtr<IStrainLoad> strainLoad;
      strainLoad.CoCreateInstance(CLSID_StrainLoad);
      strainLoad->put_MemberType(mbrType1); // mbrType1 must equal mbrType2
      strainLoad->put_MemberID(mbrID1); // mbrID1 must equal mbrID2
      strainLoad->put_StartLocation(location1);
      strainLoad->put_EndLocation(location2);
      strainLoad->put_AxialStrain(e);
      strainLoad->put_CurvatureStrain(r);

      strainLoads->Add(bstrStage,CComBSTR(strLoadingName),strainLoad,&strainLoadItem);
   }

   return true;
}

void CGirderModelManager::ConfigureLBAMPoisForReactions(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType) const
{
   // Configures the LBAM Pois for Reactions. The IDs we want for obtaining reactions depend on
   // the segment and pier boundary conditions. This method makes it easy, consistent, and seemless
   // to set up the m_LBAMPoi array

   m_LBAMPoi->Clear();
   std::vector<IDType> vIDs;

   if ( supportType == pgsTypes::stPier )
   {
      PierIndexType pierIdx = supportIdx;
      m_LBAMPoi->Add(GetPierID(pierIdx));
   }
   else
   {
      SupportIndexType tsIdx = supportIdx;
      GET_IFACE(IBridge, pBridge);
      if ( pBridge->GetSegmentConnectionTypeAtTemporarySupport(tsIdx) == pgsTypes::tsctContinuousSegment )
      {
         SupportIDType tsID = GetTemporarySupportID(tsIdx);
         vIDs.push_back(tsID);
      }
      else
      {
         // at closure joint
         // left support point
         SupportIDType ID = -((SupportIDType)(tsIdx+1)*100);
         vIDs.push_back(ID);

         // right support point
         ID = -( (SupportIDType)((tsIdx+1)*100+1) );
         vIDs.push_back(ID);
      }
   }

   // make sure we don't have two of the same IDs
   std::sort(vIDs.begin(),vIDs.end());
   vIDs.erase(std::unique(vIDs.begin(),vIDs.end()),vIDs.end());
   for( const auto& id : vIDs)
   {
      m_LBAMPoi->Add(id);
   }
}

CollectionIndexType CGirderModelManager::GetStressPointIndex(pgsTypes::StressLocation loc) const
{
   return (CollectionIndexType)(loc);
}

CComBSTR CGirderModelManager::GetLoadCaseName(LoadingCombinationType combo) const
{
#if defined _DEBUG
   // debugging code to ensure the static array of names is correct
   CComBSTR bstrLoadCase;
   switch(combo)
   {
      case lcDC:
         bstrLoadCase = _T("DC");
      break;

      case lcDW:
         bstrLoadCase = _T("DW");
      break;

      case lcDWp:
         bstrLoadCase = _T("DWp");
      break;

      case lcDWf:
         bstrLoadCase = _T("DWf");
      break;

      case lcDWRating:
         bstrLoadCase = _T("DW_Rating");
      break;

      case lcCR:
         bstrLoadCase = _T("CR");
      break;

      case lcSH:
         bstrLoadCase = _T("SH");
      break;

      case lcRE:
         bstrLoadCase = _T("RE");
      break;

      case lcPS:
         bstrLoadCase = _T("PS");
      break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }
   ATLASSERT(bstrLoadCase == gs_LoadCases[combo]);
#endif

   return gs_LoadCases[combo];
}

bool CGirderModelManager::GetLoadCaseTypeFromName(const CComBSTR& name, LoadingCombinationType* pCombo) const
{
   for ( int i = 0; i < gs_nLoadCases; i++ )
   {
      if ( name == gs_LoadCases[i] )
      {
         *pCombo = (LoadingCombinationType)i;
         return true;
      }
   }

   // Skip the user live load case because this is added in via the
   // GetLiveLoadXXX functions
   // If name isn't LL_IM, then the name wasn't found in gs_LoadCases. Is there a new load case?
   ATLASSERT(CComBSTR("LL_IM") == name); 
   return false;
}

Float64 CGirderModelManager::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const
{
   Float64 fTop, fBot;
   GetStress(intervalIdx,poi,stressLocation,stressLocation,bIncludeLiveLoad,limitState,vehicleIdx,&fTop,&fBot);
   ATLASSERT(IsEqual(fTop,fBot));
   return fTop;
}

void CGirderModelManager::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,Float64* pfTop,Float64* pfBot) const
{
   // Stress in the girder due to prestressing
   ATLASSERT(!IsStrengthLimitState(limitState));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   if ( ::IsDeckStressLocation(topLoc) && ::IsDeckStressLocation(botLoc) || intervalIdx < releaseIntervalIdx)
   {
      // pretensioning does not cause stress in the deck
      // or the interval is before release, so no stress in girder either
      *pfTop = 0;
      *pfBot = 0;
      return; 
   }

   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval();

   pgsTypes::LimitState myLimitState = (intervalIdx < liveLoadIntervalIdx ? pgsTypes::ServiceI : limitState);

   // This method can be optimized by caching the results.
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ISegmentData,pSegmentData);

   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);
   bool bIncludeElasticEffects(spMode == pgsTypes::spmGross ? true : false);

   bool bIncTempStrands = (intervalIdx < tsRemovalIntervalIdx) ? true : false;

   std::array<Float64, 3> P{ 0,0,0 };
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      P[pgsTypes::Straight] = pPsForce->GetPrestressForce(poi, pgsTypes::Straight, intervalIdx, timeType, bIncludeElasticEffects, pgsTypes::tltMinimum);
      P[pgsTypes::Harped] = pPsForce->GetPrestressForce(poi, pgsTypes::Harped, intervalIdx, timeType, bIncludeElasticEffects, pgsTypes::tltMinimum);

      if ( bIncTempStrands )
      {
         P[pgsTypes::Temporary] = pPsForce->GetPrestressForce(poi, pgsTypes::Temporary, intervalIdx, timeType, bIncludeElasticEffects, pgsTypes::tltMinimum);
      }
   }
   else
   {
      if ( bIncludeLiveLoad )
      {
         P[pgsTypes::Straight] = pPsForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Straight, myLimitState, bIncludeElasticEffects, vehicleIdx);
         P[pgsTypes::Harped]   = pPsForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Harped,   myLimitState, bIncludeElasticEffects, vehicleIdx);
      }
      else
      {
         P[pgsTypes::Straight] = pPsForce->GetPrestressForce(poi, pgsTypes::Straight, intervalIdx, timeType, bIncludeElasticEffects, pgsTypes::tltMinimum);
         P[pgsTypes::Harped]   = pPsForce->GetPrestressForce(poi, pgsTypes::Harped, intervalIdx, timeType, bIncludeElasticEffects, pgsTypes::tltMinimum);
      }

      if ( bIncTempStrands )
      {
         P[pgsTypes::Temporary] += pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Temporary, myLimitState, bIncludeElasticEffects, vehicleIdx);
      }
   }

   std::array<WBFL::Geometry::Point2d, 3> ecc
   {
      pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Straight),
      pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Harped),
      pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Temporary)
   };

   // NOTE: We can't use the eccentricity of the total strands. The eccentricity given is the geometric
   // centroid of the strands. We need the location of the resultant prestress force.

   // Compute the resultant eccentricity of the prestress force (this is different than the geometric eccentricity of the strand area)
   Float64 Pps = std::accumulate(std::cbegin(P), std::cend(P), 0.0);
   WBFL::Geometry::Point2d E = IsZero(Pps) ? WBFL::Geometry::Point2d(0,0) : std::inner_product(std::cbegin(ecc),std::cend(ecc),std::cbegin(P), WBFL::Geometry::Point2d(0,0))/Pps;

   *pfTop = GetStress(releaseIntervalIdx,poi,topLoc,Pps,E.X(), E.Y());
   *pfBot = GetStress(releaseIntervalIdx,poi,botLoc,Pps,E.X(), E.Y());
}

Float64 CGirderModelManager::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 ex,Float64 ey) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
   if ( ::IsDeckStressLocation(stressLocation) && intervalIdx < compositeDeckIntervalIdx )
   {
      return 0.0; // asking for stress in the deck but the deck is not composite yet. there can't be stress
   }

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Ca, Cbx, Cby;
   pSectProp->GetStressCoefficients(intervalIdx, poi, stressLocation, nullptr, &Ca, &Cbx, &Cby);

   Float64 f = -P*(Ca + ey*Cbx + ex*Cby);

   return f;
}

Float64 CGirderModelManager::GetStressFromSegmentPT(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation stressLocation, DuctIndexType ductIdx) const
{
   if (::IsDeckStressLocation(stressLocation))
   {
      // Segment tendons don't effect stress in the deck
      return 0.0;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(ISegmentTendonGeometry, pTendonGeometry);
   DuctIndexType nDucts = pTendonGeometry->GetDuctCount(segmentKey);
   if (nDucts == 0)
   {
      return 0;
   }

   DuctIndexType firstDuctIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastDuctIdx = (ductIdx == ALL_DUCTS ? nDucts - 1 : firstDuctIdx);

   GET_IFACE(IPosttensionForce, pPTForce);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);

   IntervalIndexType ptIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   Float64 stress = 0;
   for (DuctIndexType idx = firstDuctIdx; idx <= lastDuctIdx; idx++)
   {
      // stress in girder or deck due to PT is computed based on the post-tension force and
      // the eccentricity when the force is applied plus all the incremental change in PT force
      // times the eccentricty when the change occured.
      Float64 Pprev = 0; // total PT force at the end of the previous interval... start with 0
                         // so that the first "change" is the full PT force
      for (IntervalIndexType intIdx = ptIntervalIdx; intIdx <= intervalIdx; intIdx++)
      {
         // PT force at the end of this interval
         Float64 P = pPTForce->GetSegmentTendonForce(poi, intIdx, pgsTypes::End, idx);

         // change in PT force during this interval
         Float64 dP = P - Pprev;

         // eccentricity this interval
         Float64 eccX, eccY;
         pTendonGeometry->GetSegmentTendonEccentricity(intIdx, poi, idx, &eccX, &eccY);

         // change in stress during this interval
         Float64 f = GetStress(intIdx, poi, stressLocation, dP, eccX, eccY);

         stress += f;

         // update for next loop
         Pprev = P;
      } // next interval
   } // next duct

   return stress;
}


Float64 CGirderModelManager::GetStressFromGirderPT(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,DuctIndexType ductIdx) const
{
   const CGirderKey& girderKey(poi.GetSegmentKey());
   
   GET_IFACE(IGirderTendonGeometry,    pTendonGeometry);
   DuctIndexType nDucts = pTendonGeometry->GetDuctCount(girderKey);
   if ( nDucts == 0 )
   {
      return 0;
   }

   DuctIndexType firstDuctIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastDuctIdx  = (ductIdx == ALL_DUCTS ? nDucts-1 : firstDuctIdx);

   GET_IFACE(IPosttensionForce,  pPTForce);
   GET_IFACE(IIntervals,         pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

   Float64 stress = 0;
   for ( DuctIndexType idx = firstDuctIdx; idx <= lastDuctIdx; idx++ )
   {
      IntervalIndexType ptIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,idx);

      // stress in girder or deck due to PT is computed based on the post-tension force and
      // the eccentricity when the force is applied plus all the incremental change in PT force
      // times the eccentricty when the change occured.
      Float64 Pprev = 0; // total PT force at the end of the previous interval... start with 0
      // so that the first "change" is the full PT force
      for ( IntervalIndexType intIdx = ptIntervalIdx; intIdx <= intervalIdx; intIdx++ )
      {
         Float64 f = 0;
         if (::IsDeckStressLocation(stressLocation) && intIdx < compositeDeckIntervalIdx)
         {
            //f = 0;
         }
         else
         {
            // PT force at the end of this interval
            Float64 P = pPTForce->GetGirderTendonForce(poi,intIdx,pgsTypes::End,idx);

            // change in PT force during this interval
            Float64 dP = P - Pprev;

            // eccentricity this interval
            Float64 eccX, eccY;
            pTendonGeometry->GetGirderTendonEccentricity(intIdx,poi,idx,&eccX,&eccY);

            // change in stress during this interval
            f = GetStress(intIdx,poi,stressLocation,dP,eccX,eccY);

            // update for next interval
            Pprev = P;
         }

         stress += f;
      } // next interval
   } // next duct

   return stress;
}

VehicleIndexType CGirderModelManager::GetVehicleCount(pgsTypes::LiveLoadType llType) const
{
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModel> liveload_model;
   GetLiveLoadModel(llType,CSegmentKey(0,0,0),&liveload_model);

   CComPtr<IVehicularLoads> vehicular_loads;
   liveload_model->get_VehicularLoads(&vehicular_loads);

   VehicleIndexType nVehicles;
   vehicular_loads->get_Count(&nVehicles);
   return nVehicles;
}

Float64 CGirderModelManager::GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const
{
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(0); // get model data for girder line zero since all have the same live loads

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   CComPtr<IVehicularLoad> vehicle;
   GetVehicularLoad(lbam_model,llmt,vehicleIdx,&vehicle);

   Float64 W = 0;
   vehicle->SumAxleWeights(&W);

   // if it is one of the "Dual Trucks", divide the weight by 2
   CComBSTR bstrName;
   vehicle->get_Name(&bstrName);
   if ( bstrName == CComBSTR("LRFD Truck Train [90%(Truck + Lane)]") ||
        bstrName == CComBSTR("LRFD Low Boy (Dual Tandem + Lane)")    ||
        bstrName == CComBSTR("Two Type 3-3 separated by 30ft")    ||
        bstrName == CComBSTR("0.75(Two Type 3-3 separated by 30ft) + Lane Load") )
   {
      W /= 2; 
   }

   return W;
}

std::_tstring CGirderModelManager::GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const
{
   USES_CONVERSION;

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   if ( vehicleIdx == INVALID_INDEX )
   {
      return OLE2T(GetLiveLoadName(llType));
   }

   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(0); // get model data for girder line zero since all have the same live loads

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   CComPtr<IVehicularLoad> vehicle;
   GetVehicularLoad(lbam_model,llmt,vehicleIdx,&vehicle);

   CComBSTR bstrName;
   vehicle->get_Name(&bstrName);

   return OLE2T(bstrName);
}

pgsTypes::LiveLoadApplicabilityType CGirderModelManager::GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const
{
   ATLASSERT(vehicleIdx != INVALID_INDEX);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CGirderModelData* pModelData = nullptr;
   pModelData = GetGirderModel(0); // get model data for girder line zero since all have the same live loads

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   CComPtr<ILBAMModel> lbam_model;
   GetLBAM(pModelData,bat,&lbam_model);

   CComPtr<IVehicularLoad> vehicle;
   GetVehicularLoad(lbam_model,llmt,vehicleIdx,&vehicle);

   LiveLoadApplicabilityType applicability;
   vehicle->get_Applicability(&applicability);
   return ConvertLiveLoadApplicabilityType(applicability);
}

bool RemoveStrongbacks(const CTemporarySupportData* pTS)
{
   return (pTS->GetSupportType() == pgsTypes::StrongBack ? true : false);
}

std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> CGirderModelManager::GetSegmentErectionSupportLocations(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> vPoi;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
   IndexType nPiers = vPiers.size();
   if ( 0 < nPiers )
   {
      // segment is supported by at least one pier... piers are the highest priority "hard" supports
      if ( 2 <= nPiers )
      {
         // segment is supported by two or more piers... use the two piers closest to the ends of the segment
         if ( vPiers.front()->IsBoundaryPier() )
         {
            // if supported by a boundary pier, we want the CL Brg poi
            PoiList vSupportPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
            vPoi.emplace_back(vSupportPoi.front(),storageIntervalIdx);
         }
         else
         {
            // this is an interior pier, we want the CL Pier poi
            vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex()),storageIntervalIdx);
         }

         if ( vPiers.back()->IsBoundaryPier() )
         {
            // if supported by a boundary pier, we want the CL Brg poi
            PoiList vSupportPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
            vPoi.emplace_back(vSupportPoi.front(),storageIntervalIdx);
         }
         else
         {
            vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.back()->GetIndex()),storageIntervalIdx);
         }
      }
      else
      {
         ATLASSERT(nPiers == 1);
         std::vector<const CTemporarySupportData*> vTS = pSegment->GetTemporarySupports();
         ATLASSERT(0 < vTS.size()); // must be at least one other support besides the pier
         vTS.erase(std::remove_if(vTS.begin(),vTS.end(),RemoveStrongbacks),vTS.end());
         bool bStrongbacksOnly = false;
         if ( vTS.size() == 0 )
         {
            // the only temporary supports are strongbacks... the only way this is valid is if
            // the segment is the first or last segment
            ATLASSERT(pSegment->GetPrevSegment() == nullptr || pSegment->GetNextSegment() == nullptr);
            vTS = pSegment->GetTemporarySupports();
            bStrongbacksOnly = true;
            ATLASSERT(vTS.size() == 1); // there should only be one temporary support
         }
         ATLASSERT(0 < vTS.size()); // must be at least one non-strongback support or else the model is geometrically unstable
         pgsPointOfInterest poiPier = pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex());
         pgsPointOfInterest poiTS   = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());

         if ( vPiers.front()->GetStation() <= vTS.front()->GetStation() )
         {
            if ( vPiers.front()->IsBoundaryPier() )
            {
               // this is a boundary pier and it is to the left of the temporary support, therefore it must be at the
               // start of the segment... we want the start CL Bearing poi
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),storageIntervalIdx);
            }
            else
            {
               // this is an interior pier so we want the CL Pier poi
               vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex()),storageIntervalIdx);
            }

            pgsPointOfInterest poiTS = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());
            if ( poiTS.HasAttribute(POI_CLOSURE) )
            {
               // temp support is at a closure joint.. this means the segment does not span across this TS. Since
               // this TS is to the right of the pier, we want the CL Bearing at the end of the segment
               ATLASSERT(poiTS.GetSegmentKey() == segmentKey);
               CSegmentKey thisSegmentKey(segmentKey);
               PoiAttributeType attrib = POI_10L | POI_ERECTED_SEGMENT;
               IntervalIndexType iIdx = storageIntervalIdx;
               if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
               {
                  thisSegmentKey.segmentIndex++;
                  attrib = POI_START_FACE;
                  iIdx = intervalIdx;
               }
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),iIdx);
            }
            else
            {
               vPoi.emplace_back(poiTS,storageIntervalIdx);
            }
         }
         else
         {
            pgsPointOfInterest poiTS = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());
            if ( poiTS.HasAttribute(POI_CLOSURE) )
            {
               // temp support is at a closure joint.. this means the segment does not span across this TS. Since
               // this TS is to the left of the pier, we want the CL Bearing at the start of the segment
               CSegmentKey thisSegmentKey(segmentKey);
               PoiAttributeType attrib = POI_0L | POI_ERECTED_SEGMENT;
               IntervalIndexType iIdx = storageIntervalIdx;
               if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
               {
                  thisSegmentKey.segmentIndex--;
                  attrib = POI_END_FACE;
                  iIdx = intervalIdx;
               }
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),iIdx);
            }
            else
            {
               vPoi.emplace_back(poiTS,storageIntervalIdx);
            }

            if ( vPiers.front()->IsBoundaryPier() )
            {
               // this is a boundary pier and it is to the right of the temporary support, therefore it must be at the
               // end of the segment... we want the start CL Bearing poi
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),storageIntervalIdx);
            }
            else
            {
               // this is an interior pier so we want the CL Pier poi
               vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex()),storageIntervalIdx);
            }
         }
      }
   }
   else
   {
      // segment is supported only by temporary supports
      std::vector<const CTemporarySupportData*> vTS = pSegment->GetTemporarySupports();
      vTS.erase(std::remove_if(vTS.begin(),vTS.end(),RemoveStrongbacks),vTS.end());
      bool bStrongbacksOnly = false;
      if ( vTS.size() == 0 )
      {
         // segment is supported only by strongbacks
         vTS = pSegment->GetTemporarySupports();
         bStrongbacksOnly = true;
         ATLASSERT(vTS.size() == 2);
      }

      pgsPointOfInterest poiLeftTS  = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());
      pgsPointOfInterest poiRightTS = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.back()->GetIndex());
      ATLASSERT(poiLeftTS != poiRightTS);

      if ( poiLeftTS.HasAttribute(POI_CLOSURE) )
      {
         // we want the POI at the start CL Bearing
         CSegmentKey thisSegmentKey(segmentKey);
         PoiAttributeType attrib = POI_0L | POI_ERECTED_SEGMENT;
         IntervalIndexType iIdx = storageIntervalIdx;
         if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
         {
            thisSegmentKey.segmentIndex--;
            attrib = POI_END_FACE;
            iIdx = intervalIdx;
         }

         PoiList vSupportPoi;
         pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
         vPoi.emplace_back(vSupportPoi.front(),iIdx);
      }
      else
      {
         vPoi.emplace_back(poiLeftTS,storageIntervalIdx);
      }

      if ( poiRightTS.HasAttribute(POI_CLOSURE) )
      {
         // we want the POI at the end CL Bearing
         CSegmentKey thisSegmentKey(segmentKey);
         PoiAttributeType attrib = POI_10L | POI_ERECTED_SEGMENT;
         IntervalIndexType iIdx = storageIntervalIdx;
         if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
         {
            thisSegmentKey.segmentIndex++;
            attrib = POI_START_FACE;
            iIdx = intervalIdx;
         }
         
         PoiList vSupportPoi;
         pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
         vPoi.emplace_back(vSupportPoi.front(),iIdx);
      }
      else
      {
         vPoi.emplace_back(poiRightTS,storageIntervalIdx);
      }
   }

   ATLASSERT(vPoi.size() == 2);
   return vPoi;
}

#if defined _DEBUG
void CGirderModelManager::VerifyAnalysisType() const
{
   // Verifies that the analysis type is NOT time step
   GET_IFACE( ILossParameters, pLossParams);
   ATLASSERT( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP );
}
#endif

bool CGirderModelManager::VerifyPoi(const PoiList& vPoi) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   std::vector<CGirderKey> girderKeys;
   pPoi->GetGirderKeys(vPoi, &girderKeys);
   return (girderKeys.size() == 1) ? true : false;
}
