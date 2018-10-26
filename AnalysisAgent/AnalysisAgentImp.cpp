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

// AnalysisAgentImp.cpp : Implementation of CAnalysisAgent
#include "stdafx.h"
#include "AnalysisAgent.h"
#include "AnalysisAgent_i.h"
#include "AnalysisAgentImp.h"
#include "..\PGSuperException.h"

#include "PGSuperLoadCombinationResponse.h"
#include "BarrierSidewalkLoadDistributionTool.h"

#include "StatusItems.h"

#include <..\HtmlHelp\HelpTopics.hh>

#include <WBFLCogo.h>

#include <IFace\DistributionFactors.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\Project.h>
#include <IFace\PointOfInterest.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\DebondUtil.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\GirderModelFactory.h>
#include <PgsExt\ClosureJointData.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderMaterial.h>
#include <PgsExt\StrandData.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\UnitServer.h>

#include <Units\SysUnitsMgr.h>
#include <Units\SysUnits.h>
#include <Lrfd\LoadModifier.h>

#include <PgsExt\StatusItem.h>
#include <IFace\StatusCenter.h>

#include <algorithm>

//////////////////////////////////////////////////////////////////////////////////
// NOTES:
//
// The pedestrian live load distribution factor is the pedestrian load.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DECLARE_LOGFILE;

#pragma Reminder("UPDATE: is this still needed?")
// VERIFY_DEFLECTIONS code was used during initial development of the deflection checks
// the idea was that deflections from the LBAM should match deflections from the
// time-step analysis (and theoretically they should). however, during initial testing
// it was discovered that the elastic and time-step deflections don't match at all and
// it is likely that adjustments are made in the time-step analysis that are not made
// in the elastic analysis. either remove this code or fix the issues with the elastic analysis.
// NOTE: LBAM elastic deflections will probably be incorrect for variable depth members unless
// the LBAM is updated to account for this. currently the LBAM uses prismatic members
// and step-wise section changes are not modeled.

// by defining this deflections are computed using elastic and time-step methods
// and compared
//#define VERIFY_DEFLECTIONS
#if defined VERIFY_DEFLECTIONS
#include <initguid.h>
#include <EAF\EAFDisplayUnits.h>
#endif

UINT CAnalysisAgentImp::DeleteBridgeSiteModels(LPVOID pParam)
{
   CAnalysisAgentImp::BridgeSiteModels* pModels = (CAnalysisAgentImp::BridgeSiteModels*)pParam;
   pModels->clear();
   delete pModels;
   return 0; // success
}

CLoadMap::CLoadMap()
{
}

void CLoadMap::AddLoadItem(ProductForceType productForceType,CComBSTR bstrName)
{
   m_LoadIDToLoadName.insert(std::make_pair(productForceType,bstrName));
   m_LoadNameToLoadID.insert(std::make_pair(bstrName,productForceType));
}

ProductForceType CLoadMap::GetProductForceType(CComBSTR bstrName)
{
   std::map<CComBSTR,ProductForceType>::iterator found( m_LoadNameToLoadID.find(bstrName) );
   if ( found == m_LoadNameToLoadID.end() )
   {
      ATLASSERT(false);
      return pftGirder;
   }
   else
      return found->second;
}

CComBSTR CLoadMap::GetGroupLoadName(ProductForceType productForceType)
{
   std::map<ProductForceType,CComBSTR>::iterator found( m_LoadIDToLoadName.find(productForceType) );
   if ( found == m_LoadIDToLoadName.end() )
   {
      ATLASSERT(false);
      return CComBSTR();
   }
   else
   {
      return found->second;
   }
}


#define TEMPORARY_SUPPORT_ID_OFFSET 10000

SupportIDType GetTempSupportID(SupportIndexType tsIdx)
{
   return (SupportIDType)(tsIdx + TEMPORARY_SUPPORT_ID_OFFSET);
}

SupportIndexType GetTempSupportIndex(SupportIDType tsID)
{
   return (SupportIndexType)(tsID - TEMPORARY_SUPPORT_ID_OFFSET);
}

void GetPierTemporarySupportIDs(PierIndexType pierIdx,SupportIDType* pBackID,SupportIDType* pAheadID)
{
   // These are the IDs for temporary supports used to maintain stability in the LBAM at
   // intermediate piers where there is a non-moment transfering boundary condition.
   *pBackID  = -((SupportIDType)pierIdx*10000);
   *pAheadID = -((SupportIDType)pierIdx*10002);
}

GirderIDType GetSuperstructureMemberID(GroupIndexType grpIdx,GirderIndexType gdrIdx)
{
   ATLASSERT(grpIdx != INVALID_INDEX);
   ATLASSERT(gdrIdx != INVALID_INDEX);

   ATLASSERT( grpIdx < Int16_Max && gdrIdx < Int16_Max );
   return ::make_Int32((Int16)grpIdx,(Int16)gdrIdx);
}

CComBSTR GetLBAMStageName(IntervalIndexType intervalIdx)
{
   std::_tostringstream os;
   os << _T("Interval_") << intervalIdx;
   CComBSTR strStageName(os.str().c_str());
   return strStageName;
}

IntervalIndexType GetIntervalFromLBAMStageName(BSTR bstrStage)
{
   USES_CONVERSION;
   CString strName(OLE2T(bstrStage));
   CString strKey(_T("Interval_"));
   int count = strName.GetLength()-strKey.GetLength();
   IntervalIndexType intervalIdx = (IntervalIndexType)_ttol(strName.Right(count));
   return intervalIdx;
}

// NOTE: If a new product load is added don't forget to change
// GetDesignStress() so that it is included in the design process

// FEM2D Analysis Model Load Case ID's
const LoadCaseIDType g_lcidGirder              =  1;
const LoadCaseIDType g_lcidStraightStrand      =  2;
const LoadCaseIDType g_lcidHarpedStrand        =  3;
const LoadCaseIDType g_lcidTemporaryStrand     =  4;
const LoadCaseIDType g_lcidUnitLoadBase        = -1000;

// This list is ordered like pgsTypes::LiveLoadType
const LiveLoadModelType g_LiveLoadModelType[] = 
{ lltDesign,             // strength and service design
  lltPermit,             // permit for Strength II design
  lltFatigue,            // fatigue limit state design
  lltPedestrian,
  lltLegalRoutineRating, // legal rating, routine traffic
  lltLegalSpecialRating, // legal rating, commercial traffic
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
         return (pgsTypes::LiveLoadType)is;
   }

   ATLASSERT(0); // should never happen since a match should always be in array
   return pgsTypes::lltDesign;
}

// useful struct's
struct SuperstructureMemberData
{
   CComBSTR stage;
   Float64 ea;
   Float64 ei;
   Float64 ea_defl;
   Float64 ei_defl;
};

// useful functions
static void CreateSuperstructureMember(Float64 length,const std::vector<SuperstructureMemberData>& vData,ISuperstructureMember** ppMbr);

static void AddLoadCase(ILoadCases* loadCases, BSTR name, BSTR description)
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

static HRESULT AddLoadGroup(ILoadGroups* loadGroups, BSTR name, BSTR description)
{
   HRESULT hr;
   CComPtr<ILoadGroup> load_group;
   hr = load_group.CoCreateInstance(CLSID_LoadGroup) ;
   ATLASSERT(SUCCEEDED(hr));
   if ( FAILED(hr) )
      return hr;

   hr = load_group->put_Name(name) ;
   ATLASSERT(SUCCEEDED(hr));
   if ( FAILED(hr) )
      return hr;

   hr = load_group->put_Description(name) ;
   ATLASSERT(SUCCEEDED(hr));
   if ( FAILED(hr) )
      return hr;

   hr = loadGroups->Add(load_group) ;
   ATLASSERT(SUCCEEDED(hr));
   if ( FAILED(hr) )
      return hr;

   return hr;
}

HRESULT CAnalysisAgentImp::FinalConstruct()
{
   HRESULT hr;
   hr = m_LBAMUtility.CoCreateInstance(CLSID_LRFDFactory);
   if ( FAILED(hr) )
      return hr;

   hr = m_UnitServer.CoCreateInstance(CLSID_UnitServer);
   if ( FAILED(hr) )
      return hr;

   m_pBridgeSiteModels = std::auto_ptr<BridgeSiteModels>(new BridgeSiteModels);

   return S_OK;
}

CAnalysisAgentImp::ModelData::ModelData(CAnalysisAgentImp *pParent)
{
   m_pParent = pParent;

   // create minimum model enveloper and initialize it (no models just yet)
   m_MinModelEnveloper.CoCreateInstance(CLSID_LBAMModelEnveloper);
   m_MinModelEnveloper->Initialize(NULL,atForce,optMinimize);

   // create maximum model enveloper and initialize it (no models just yet)
   m_MaxModelEnveloper.CoCreateInstance(CLSID_LBAMModelEnveloper);
   m_MaxModelEnveloper->Initialize(NULL,atForce,optMaximize);
}

CAnalysisAgentImp::ModelData::ModelData(const ModelData& other)
{ 
   *this = other;
}

CAnalysisAgentImp::ModelData::~ModelData()
{
}

void CAnalysisAgentImp::ModelData::CreateAnalysisEngine(ILBAMModel* theModel,pgsTypes::BridgeAnalysisType bat,ILBAMAnalysisEngine** ppEngine)
{
   ATLASSERT( bat == pgsTypes::SimpleSpan || bat == pgsTypes::ContinuousSpan );

   CComPtr<ILBAMAnalysisEngine> engine;
   engine.CoCreateInstance(CLSID_LBAMAnalysisEngine);

   // create the customized enveloped vehicular response object
   CComPtr<IEnvelopedVehicularResponse> envelopedVehicularResponse;
#if defined _USE_ORIGINAL_LIVELOADER
   envelopedVehicularResponse.CoCreateInstance(CLSID_BruteForceVehicularResponse);
#else
   envelopedVehicularResponse.CoCreateInstance(CLSID_BruteForceVehicularResponse2);
#endif

   // Special load combiner for load ratings. It allows the load combination factor
   // to vary based on the weight of the live load on the structure
   CComObject<CPGSuperLoadCombinationResponse>* pLCResponse;
   HRESULT hr = CComObject<CPGSuperLoadCombinationResponse>::CreateInstance(&pLCResponse);
   CComQIPtr<ILoadCombinationResponse> load_combo_response(pLCResponse);

   // initialize the engine with default values (NULL), except for the vehicular response enveloper
   engine->InitializeEx(theModel,atForce,
                        NULL, // load group response
                        NULL, // unit load response
                        NULL, // influence line response
                        NULL, // analysis pois
                        NULL, // basic vehicular response
                        NULL, // live load model response
                        envelopedVehicularResponse,
                        NULL, // load case response
                        load_combo_response/*NULL*/, // load combination response
                        NULL, // concurrent load combination response
                        NULL, // live load negative moment response
                        NULL); // contraflexure response

   // get the various response interfaces from the engine so we don't have to do it over and over again
   engine->get_LoadGroupResponse(&pLoadGroupResponse[bat]);
   engine->get_UnitLoadResponse(&pUnitLoadResponse[bat]);
   engine->get_LoadCaseResponse(&pLoadCaseResponse[bat]);
   engine->get_ContraflexureResponse(&pContraflexureResponse[bat]);
   engine->get_LoadCombinationResponse(&pLoadComboResponse[bat]);
   engine->get_ConcurrentLoadCombinationResponse(&pConcurrentComboResponse[bat]);
   engine->get_LiveLoadModelResponse(&pLiveLoadResponse[bat]);
   engine->get_EnvelopedVehicularResponse(&pVehicularResponse[bat]);

   CComQIPtr<ILoadCombinationResponse> load_combination_response_delegate(pLoadCaseResponse[bat]);
   pLCResponse->Initialize(load_combination_response_delegate,pLoadGroupResponse[bat],pLiveLoadResponse[bat],theModel,m_pParent);

   (*ppEngine) = engine;
   (*ppEngine)->AddRef();
}

void CAnalysisAgentImp::ModelData::AddSimpleModel(ILBAMModel* pModel)
{
   m_Model = pModel;

   CComPtr<ILBAMAnalysisEngine> engine;
   CreateAnalysisEngine(m_Model,pgsTypes::SimpleSpan,&engine);

   m_MinModelEnveloper->AddEngine(engine);
   m_MaxModelEnveloper->AddEngine(engine);


   // Setup for deflection analysis

   // Response engine for the deflection-only analysis
   pDeflLoadGroupResponse[pgsTypes::SimpleSpan].CoCreateInstance(CLSID_LoadGroupDeflectionResponse);
   pDeflLiveLoadResponse[pgsTypes::SimpleSpan].CoCreateInstance(CLSID_LiveLoadModelResponse);

   // Use a live load factory so we don't get the default brute force (slow) live loader
   CComPtr<IEnvelopedVehicularResponseFactory> pVrFactory;
   pVrFactory.CoCreateInstance(CLSID_EnvelopedVehicularResponseFactory);

   CComQIPtr<ISupportEnvelopedVehicularResponseFactory> pSupportFactory(pDeflLiveLoadResponse[pgsTypes::SimpleSpan]);
   pSupportFactory->putref_EnvelopedVehicularRepsonseFactory(pVrFactory);

   CComQIPtr<IDependOnLBAM> pDeflDepend(pDeflLoadGroupResponse[pgsTypes::SimpleSpan]);
   pDeflDepend->putref_Model(m_Model);

   CComQIPtr<IInfluenceLineResponse>         defl_influence(pDeflLoadGroupResponse[pgsTypes::SimpleSpan]);
   CComQIPtr<ILiveLoadNegativeMomentRegion>  defl_neg_moment_region(pDeflLoadGroupResponse[pgsTypes::SimpleSpan]);
   CComQIPtr<IAnalysisPOIs>                  defl_pois(pDeflLoadGroupResponse[pgsTypes::SimpleSpan]);
   CComQIPtr<IGetDistributionFactors>        defl_dfs(pDeflLoadGroupResponse[pgsTypes::SimpleSpan]);
   CComQIPtr<IGetStressPoints>               defl_gsp(pDeflLoadGroupResponse[pgsTypes::SimpleSpan]);

   pDeflLoadGroupResponse[pgsTypes::SimpleSpan].QueryInterface(&pDeflContraflexureResponse[pgsTypes::SimpleSpan]);

   CComPtr<IVehicularAnalysisContext> defl_vehicular_analysis_ctx;
   defl_vehicular_analysis_ctx.CoCreateInstance(CLSID_VehicularAnalysisContext);
   defl_vehicular_analysis_ctx->Initialize(m_Model,defl_influence,defl_neg_moment_region,defl_pois,defl_dfs,defl_gsp);

   CComQIPtr<IDependOnVehicularAnalysisContext> defl_depend_on_vehicular_analysis_ctx(pDeflLiveLoadResponse[pgsTypes::SimpleSpan]);
   defl_depend_on_vehicular_analysis_ctx->Initialize(defl_vehicular_analysis_ctx);

   CComQIPtr<IEnvelopingStrategy> strategy(pDeflLiveLoadResponse[pgsTypes::SimpleSpan]);
   strategy->get_Strategy(&pDeflEnvelopedVehicularResponse[pgsTypes::SimpleSpan]);
   ATLASSERT(pDeflEnvelopedVehicularResponse[pgsTypes::SimpleSpan]!=NULL);
}

void CAnalysisAgentImp::ModelData::AddContinuousModel(ILBAMModel* pContModel)
{
   m_ContinuousModel = pContModel;

   CComPtr<ILBAMAnalysisEngine> engine;
   CreateAnalysisEngine(m_ContinuousModel,pgsTypes::ContinuousSpan,&engine);

   m_MinModelEnveloper->AddEngine(engine);
   m_MaxModelEnveloper->AddEngine(engine);



   // Setup for deflection analysis

   // Response engine for the deflection-only analysis
   pDeflLoadGroupResponse[pgsTypes::ContinuousSpan].CoCreateInstance(CLSID_LoadGroupDeflectionResponse);
   pDeflLiveLoadResponse[pgsTypes::ContinuousSpan].CoCreateInstance(CLSID_LiveLoadModelResponse);

   // Use a live load factory so we don't get the default brute force (slow) live loader
   CComPtr<IEnvelopedVehicularResponseFactory> pVrFactory;
   pVrFactory.CoCreateInstance(CLSID_EnvelopedVehicularResponseFactory);

   CComQIPtr<ISupportEnvelopedVehicularResponseFactory> pSupportFactory(pDeflLiveLoadResponse[pgsTypes::ContinuousSpan]);
   pSupportFactory->putref_EnvelopedVehicularRepsonseFactory(pVrFactory);

   CComQIPtr<IDependOnLBAM> pDeflDepend(pDeflLoadGroupResponse[pgsTypes::ContinuousSpan]);
   pDeflDepend->putref_Model(m_ContinuousModel);

   CComQIPtr<IInfluenceLineResponse>         defl_influence(pDeflLoadGroupResponse[pgsTypes::ContinuousSpan]);
   CComQIPtr<ILiveLoadNegativeMomentRegion>  defl_neg_moment_region(pDeflLoadGroupResponse[pgsTypes::ContinuousSpan]);
   CComQIPtr<IAnalysisPOIs>                  defl_pois(pDeflLoadGroupResponse[pgsTypes::ContinuousSpan]);
   CComQIPtr<IGetDistributionFactors>        defl_dfs(pDeflLoadGroupResponse[pgsTypes::ContinuousSpan]);
   CComQIPtr<IGetStressPoints>               defl_gsp(pDeflLoadGroupResponse[pgsTypes::ContinuousSpan]);

   pDeflLoadGroupResponse[pgsTypes::ContinuousSpan].QueryInterface(&pDeflContraflexureResponse[pgsTypes::ContinuousSpan]);

   CComPtr<IVehicularAnalysisContext> defl_vehicular_analysis_ctx;
   defl_vehicular_analysis_ctx.CoCreateInstance(CLSID_VehicularAnalysisContext);
   defl_vehicular_analysis_ctx->Initialize(m_ContinuousModel,defl_influence,defl_neg_moment_region,defl_pois,defl_dfs,defl_gsp);

   CComQIPtr<IDependOnVehicularAnalysisContext> defl_depend_on_vehicular_analysis_ctx(pDeflLiveLoadResponse[pgsTypes::ContinuousSpan]);
   defl_depend_on_vehicular_analysis_ctx->Initialize(defl_vehicular_analysis_ctx);

   CComQIPtr<IEnvelopingStrategy> strategy(pDeflLiveLoadResponse[pgsTypes::ContinuousSpan]);
   strategy->get_Strategy(&pDeflEnvelopedVehicularResponse[pgsTypes::ContinuousSpan]);
   ATLASSERT(pDeflEnvelopedVehicularResponse[pgsTypes::ContinuousSpan]!=NULL);



   // get the interfaces for accessing the enveloped responses 
   for ( int i = 0; i < 3; i++ ) // force effect types
   {
      for ( int j = 0; j < 2; j++ ) // optimization types
      {
         m_MinModelEnveloper->get_LoadGroupResponse(ForceEffectType(i),OptimizationType(j),&pMinLoadGroupResponseEnvelope[i][j]);
         m_MinModelEnveloper->get_LoadCaseResponse( ForceEffectType(i),OptimizationType(j),&pMinLoadCaseResponseEnvelope[i][j]);

         m_MaxModelEnveloper->get_LoadGroupResponse(ForceEffectType(i),OptimizationType(j),&pMaxLoadGroupResponseEnvelope[i][j]);
         m_MaxModelEnveloper->get_LoadCaseResponse( ForceEffectType(i),OptimizationType(j),&pMaxLoadCaseResponseEnvelope[i][j]);
      }
   }

   m_MinModelEnveloper->get_LiveLoadModelResponse(     &pLiveLoadResponse[ pgsTypes::MinSimpleContinuousEnvelope]);
   m_MinModelEnveloper->get_LoadCombinationResponse(   &pLoadComboResponse[pgsTypes::MinSimpleContinuousEnvelope]);
   m_MinModelEnveloper->get_EnvelopedVehicularResponse(&pVehicularResponse[pgsTypes::MinSimpleContinuousEnvelope]);

   m_MaxModelEnveloper->get_LiveLoadModelResponse(     &pLiveLoadResponse[ pgsTypes::MaxSimpleContinuousEnvelope]);
   m_MaxModelEnveloper->get_LoadCombinationResponse(   &pLoadComboResponse[pgsTypes::MaxSimpleContinuousEnvelope]);
   m_MaxModelEnveloper->get_EnvelopedVehicularResponse(&pVehicularResponse[pgsTypes::MaxSimpleContinuousEnvelope]);
}

void CAnalysisAgentImp::ModelData::operator=(const CAnalysisAgentImp::ModelData& other)
{ 
   m_pParent = other.m_pParent;

   PoiMap = other.PoiMap;

   m_Model             = other.m_Model;
   m_ContinuousModel   = other.m_ContinuousModel;
   m_MinModelEnveloper = other.m_MinModelEnveloper;
   m_MaxModelEnveloper = other.m_MaxModelEnveloper;

   pLoadGroupResponse[pgsTypes::SimpleSpan]     = other.pLoadGroupResponse[pgsTypes::SimpleSpan];
   pLoadGroupResponse[pgsTypes::ContinuousSpan] = other.pLoadGroupResponse[pgsTypes::ContinuousSpan];

   pLoadCaseResponse[pgsTypes::SimpleSpan]      = other.pLoadCaseResponse[pgsTypes::SimpleSpan];
   pLoadCaseResponse[pgsTypes::ContinuousSpan]  = other.pLoadCaseResponse[pgsTypes::ContinuousSpan];

   pUnitLoadResponse[pgsTypes::SimpleSpan]      = other.pUnitLoadResponse[pgsTypes::SimpleSpan];
   pUnitLoadResponse[pgsTypes::ContinuousSpan]  = other.pUnitLoadResponse[pgsTypes::ContinuousSpan];

   pLiveLoadResponse[pgsTypes::SimpleSpan]                  = other.pLiveLoadResponse[pgsTypes::SimpleSpan];
   pLiveLoadResponse[pgsTypes::ContinuousSpan]              = other.pLiveLoadResponse[pgsTypes::ContinuousSpan];
   pLiveLoadResponse[pgsTypes::MinSimpleContinuousEnvelope] = other.pLiveLoadResponse[pgsTypes::MinSimpleContinuousEnvelope];
   pLiveLoadResponse[pgsTypes::MaxSimpleContinuousEnvelope] = other.pLiveLoadResponse[pgsTypes::MaxSimpleContinuousEnvelope];

   pLoadComboResponse[pgsTypes::SimpleSpan]                  = other.pLoadComboResponse[pgsTypes::SimpleSpan];
   pLoadComboResponse[pgsTypes::ContinuousSpan]              = other.pLoadComboResponse[pgsTypes::ContinuousSpan];
   pLoadComboResponse[pgsTypes::MinSimpleContinuousEnvelope] = other.pLoadComboResponse[pgsTypes::MinSimpleContinuousEnvelope];
   pLoadComboResponse[pgsTypes::MaxSimpleContinuousEnvelope] = other.pLoadComboResponse[pgsTypes::MaxSimpleContinuousEnvelope];

   pConcurrentComboResponse[pgsTypes::SimpleSpan]            = other.pConcurrentComboResponse[pgsTypes::SimpleSpan];
   pConcurrentComboResponse[pgsTypes::ContinuousSpan]        = other.pConcurrentComboResponse[pgsTypes::ContinuousSpan];

   pVehicularResponse[pgsTypes::SimpleSpan]                  = other.pVehicularResponse[pgsTypes::SimpleSpan];
   pVehicularResponse[pgsTypes::ContinuousSpan]              = other.pVehicularResponse[pgsTypes::ContinuousSpan];
   pVehicularResponse[pgsTypes::MinSimpleContinuousEnvelope] = other.pVehicularResponse[pgsTypes::MinSimpleContinuousEnvelope];
   pVehicularResponse[pgsTypes::MaxSimpleContinuousEnvelope] = other.pVehicularResponse[pgsTypes::MaxSimpleContinuousEnvelope];

   for ( int i = 0; i < 3; i++ ) // force effect type
   {
      for ( int j = 0; j < 2; j++ ) // optimization type
      {
         pMinLoadGroupResponseEnvelope[i][j] = other.pMinLoadGroupResponseEnvelope[i][j];
         pMinLoadCaseResponseEnvelope[i][j]  = other.pMinLoadCaseResponseEnvelope[i][j];

         pMaxLoadGroupResponseEnvelope[i][j] = other.pMaxLoadGroupResponseEnvelope[i][j];
         pMaxLoadCaseResponseEnvelope[i][j]  = other.pMaxLoadCaseResponseEnvelope[i][j];
      }
   }

   for ( int i = 0; i < 2; i++ )
   {
      pDeflLoadGroupResponse[i]          = other.pDeflLoadGroupResponse[i];
      pDeflLiveLoadResponse[i]           = other.pDeflLiveLoadResponse[i];
      pDeflEnvelopedVehicularResponse[i] = other.pDeflEnvelopedVehicularResponse[i];
   }
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisAgent
CollectionIndexType CAnalysisAgentImp::GetStressPointIndex(pgsTypes::StressLocation loc)
{
   return (CollectionIndexType)(loc);
}

#pragma Reminder("UPDATE: this can be more efficient")
// This would be more efficient if it were a static array of strings
// with direct lookup based on LoadingCombination
CComBSTR CAnalysisAgentImp::GetLoadCaseName(LoadingCombination combo)
{
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

      case lcPS:
         bstrLoadCase = _T("PS");
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLoadCase;
}

bool GetLoadCaseTypeFromName(const CComBSTR& name, LoadingCombination* pCombo)
{
   if (CComBSTR("DC") == name )
   {
      *pCombo = lcDC;
      return true;
   }
   else if (CComBSTR("DW") == name )
   {
      *pCombo = lcDW;
      return true;
   }
   else if (CComBSTR("DWp") == name )
   {
      *pCombo = lcDWp;
      return true;
   }
   else if (CComBSTR("DWf") == name )
   {
      *pCombo = lcDWf;
      return true;
   }
   else if (CComBSTR("DW_Rating") == name )
   {
      *pCombo = lcDWRating;
      return true;
   }
   else if (CComBSTR("CR") == name )
   {
      *pCombo = lcCR;
      return true;
   }
   else if (CComBSTR("SH") == name )
   {
      *pCombo = lcSH;
      return true;
   }
   else if (CComBSTR("PS") == name )
   {
      *pCombo = lcPS;
      return true;
   }
   else
   {
      // Skip the user live load case because this is added in via the
      // GetLiveLoad... functions
      ATLASSERT(CComBSTR("LL_IM") == name); // New load case added?
      return false;
   }
}

#pragma Reminder("UPDATE: this can be more efficient")
// This would be more efficient if it were a static array of strings
// with direct lookup based on LimitState
CComBSTR CAnalysisAgentImp::GetLoadCombinationName(pgsTypes::LimitState ls)
{
   // if new load combinations names are added also updated GetLimitStateFromLoadCombination
   CComBSTR bstrLimitState;
   switch(ls)
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
         bstrLimitState = _T("STRENGTH-I-Routine");
         break;

      case pgsTypes::StrengthI_LegalSpecial:
         bstrLimitState = _T("STRENGTH-I-Special");
         break;

      case pgsTypes::ServiceIII_LegalRoutine:
         bstrLimitState = _T("SERVICE-III-Routine");
         break;

      case pgsTypes::ServiceIII_LegalSpecial:
         bstrLimitState = _T("SERVICE-III-Special");
         break;

      case pgsTypes::StrengthII_PermitRoutine:
         bstrLimitState = _T("STRENGTH-II-RoutinePermit");
         break;

      case pgsTypes::ServiceI_PermitRoutine:
         bstrLimitState = _T("SERVICE-I-RoutinePermit");
         break;

      case pgsTypes::StrengthII_PermitSpecial:
         bstrLimitState = _T("STRENGTH-II-SpecialPermit");
         break;

      case pgsTypes::ServiceI_PermitSpecial:
         bstrLimitState = _T("SERVICE-I-SpecialPermit");
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLimitState;
}

pgsTypes::LimitState CAnalysisAgentImp::GetLimitStateFromLoadCombination(CComBSTR bstrLoadCombination)
{
   // if new load combinations names are added also updated GetLoadCombinationName
   pgsTypes::LimitState ls;
   if (CComBSTR("SERVICE-I") == bstrLoadCombination )
      ls = pgsTypes::ServiceI;
   else if ( CComBSTR("SERVICE-IA") == bstrLoadCombination )
      ls = pgsTypes::ServiceIA;
   else if ( CComBSTR("SERVICE-III") == bstrLoadCombination )
      ls = pgsTypes::ServiceIII;
   else if ( CComBSTR("STRENGTH-I") == bstrLoadCombination )
      ls = pgsTypes::StrengthI;
   else if ( CComBSTR("STRENGTH-II") == bstrLoadCombination )
      ls = pgsTypes::StrengthII;
   else if ( CComBSTR("FATIGUE-I") == bstrLoadCombination )
      ls = pgsTypes::FatigueI;
   else if ( CComBSTR("STRENGTH-I-Inventory") == bstrLoadCombination )
      ls = pgsTypes::StrengthI_Inventory;
   else if ( CComBSTR("STRENGTH-I-Operating") == bstrLoadCombination )
      ls = pgsTypes::StrengthI_Operating;
   else if ( CComBSTR("SERVICE-III-Inventory") == bstrLoadCombination )
      ls = pgsTypes::ServiceIII_Inventory;
   else if ( CComBSTR("SERVICE-III-Operating") == bstrLoadCombination )
      ls = pgsTypes::ServiceIII_Operating;
   else if ( CComBSTR("STRENGTH-I-Routine") == bstrLoadCombination )
      ls = pgsTypes::StrengthI_LegalRoutine;
   else if ( CComBSTR("STRENGTH-I-Special") == bstrLoadCombination )
      ls = pgsTypes::StrengthI_LegalSpecial;
   else if ( CComBSTR("SERVICE-III-Routine") == bstrLoadCombination )
      ls = pgsTypes::ServiceIII_LegalRoutine;
   else if ( CComBSTR("SERVICE-III-Special") == bstrLoadCombination )
      ls = pgsTypes::ServiceIII_LegalSpecial;
   else if ( CComBSTR("STRENGTH-II-RoutinePermit") == bstrLoadCombination )
      ls = pgsTypes::StrengthII_PermitRoutine;
   else if ( CComBSTR("SERVICE-I-RoutinePermit") == bstrLoadCombination )
      ls = pgsTypes::ServiceI_PermitRoutine;
   else if ( CComBSTR("STRENGTH-II-SpecialPermit") == bstrLoadCombination )
      ls = pgsTypes::StrengthII_PermitSpecial;
   else if ( CComBSTR("SERVICE-I-SpecialPermit") == bstrLoadCombination )
      ls = pgsTypes::ServiceI_PermitSpecial;
   else
   {
      ATLASSERT(false);
   }

   return ls;
}

#pragma Reminder("UPDATE: this can be more efficient")
// This would be more efficient if it were a static array of strings
// with direct lookup based on LiveLoadType
CComBSTR CAnalysisAgentImp::GetLoadCombinationName(pgsTypes::LiveLoadType llt)
{
   CComBSTR bstrLoadComboName;
   switch(llt)
   {
      case pgsTypes::lltDesign:
         bstrLoadComboName = "DESIGN_LL";
         break;

      case pgsTypes::lltPermit:
         bstrLoadComboName = "PERMIT_LL";
         break;

      case pgsTypes::lltFatigue:
         bstrLoadComboName = "FATIGUE_LL";
         break;

      case pgsTypes::lltPedestrian:
         bstrLoadComboName = "PEDESTRIAN_LL";
         break;

      case pgsTypes::lltLegalRating_Routine:
         bstrLoadComboName = "LEGAL_ROUTINE_LL";
         break;

      case pgsTypes::lltLegalRating_Special:
         bstrLoadComboName = "LEGAL_SPECIAL_LL";
         break;

      case pgsTypes::lltPermitRating_Routine:
         bstrLoadComboName = "PERMIT_ROUTINE_LL";
         break;

      case pgsTypes::lltPermitRating_Special:
         bstrLoadComboName = "PERMIT_SPECIAL_LL";
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLoadComboName;
}


#pragma Reminder("UPDATE: this can be more efficient")
// This would be more efficient if it were a static array of strings
// with direct lookup based on LiveLoadType
CComBSTR CAnalysisAgentImp::GetLiveLoadName(pgsTypes::LiveLoadType llt)
{
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

      case pgsTypes::lltPermitRating_Routine:
         bstrLiveLoadName = "LL+IM Permit Rating (Routine)";
         break;

      case pgsTypes::lltPermitRating_Special:
         bstrLiveLoadName = "LL+IM Permit Rating (Special)";
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLiveLoadName;
}

CComBSTR CAnalysisAgentImp::GetLoadGroupName(ProductForceType type)
{
   return m_ProductLoadMap.GetGroupLoadName(type);
}

ProductForceType CAnalysisAgentImp::GetProductForceType(const CComBSTR& bstrLoadGroup)
{
   return m_ProductLoadMap.GetProductForceType(bstrLoadGroup);
}

CComBSTR GetLoadGroupNameForUserLoad(IUserDefinedLoads::UserDefinedLoadCase lc)
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

void CAnalysisAgentImp::Invalidate(bool clearStatus)
{
   // use a worker thread to delete the models
   BridgeSiteModels* pOldModels = m_pBridgeSiteModels.release(); // release from the auto_ptr. the worker thread will take ownership and delete
   m_pBridgeSiteModels = std::auto_ptr<BridgeSiteModels>(new BridgeSiteModels);

   AfxBeginThread(CAnalysisAgentImp::DeleteBridgeSiteModels,(LPVOID)(pOldModels));

   m_ReleaseModels.clear();
   m_StorageModels.clear();

   m_SidewalkTrafficBarrierLoads.clear();

   InvalidateCamberModels();

   for ( int i = 0; i < 6; i++ )
   {
      m_CreepCoefficientDetails[CREEP_MINTIME][i].clear();
      m_CreepCoefficientDetails[CREEP_MAXTIME][i].clear();
   }

   m_NextPoi = 0;

   m_OverhangLoadSet[pgsTypes::Simple].clear();
   m_OverhangLoadSet[pgsTypes::Continuous].clear();

   if (clearStatus)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByStatusGroupID(m_StatusGroupID);
   }
}

void CAnalysisAgentImp::InvalidateCamberModels()
{
   m_PrestressDeflectionModels.clear();
   m_InitialTempPrestressDeflectionModels.clear();
   m_ReleaseTempPrestressDeflectionModels.clear();
}

void CAnalysisAgentImp::ValidateAnalysisModels(GirderIndexType gdrIdx)
{
   // Validating the analysis models consists of building the model
   // Analysis will occur when results are needed from the model.
   try
   {
      GET_IFACE(IBridge,pBridge);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            // Lets do a quick and dirty check to see if the models have been built
            // for this girder.  If release model is built, then all the models
            // should be built.
            SegmentModelData* pModelData = 0;
            pModelData = GetModelData(m_ReleaseModels,segmentKey);
            if ( pModelData == 0 )
            {
               DoReleaseAnalysis(segmentKey);
               DoStorageAnalysis(segmentKey);
            }
         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   // This will validate the bridge site stage models
   ModelData* pBSModelData = 0;
   pBSModelData = GetModelData( gdrIdx );
}

void CAnalysisAgentImp::DoReleaseAnalysis(const CSegmentKey& segmentKey)
{
   SegmentModelData* pModelData = GetModelData(m_ReleaseModels,segmentKey);
   if ( pModelData == 0 )
   {
      SegmentModelData model_data = BuildReleaseModels(segmentKey);
      std::pair<SegmentModels::iterator,bool> result = m_ReleaseModels.insert( std::make_pair(segmentKey,model_data) );
      ATLASSERT( result.second == true );
   }
}

void CAnalysisAgentImp::DoStorageAnalysis(const CSegmentKey& segmentKey)
{
   SegmentModelData* pModelData = GetModelData(m_StorageModels,segmentKey);
   if ( pModelData == 0 )
   {
      SegmentModelData model_data = BuildStorageModels(segmentKey);
      std::pair<SegmentModels::iterator,bool> result = m_StorageModels.insert( std::make_pair(segmentKey,model_data) );
      ATLASSERT( result.second == true );
   }
}

void CAnalysisAgentImp::BuildBridgeSiteModel(GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( !pBridgeDesc->IsStable() )
   {
      THROW_UNWIND(_T("Cannot perform analysis. Bridge configuration is geometrically unstable."),XREASON_UNSTABLE);
   }

   BridgeSiteModels::iterator found = m_pBridgeSiteModels->find(gdrIdx);
   if ( found == m_pBridgeSiteModels->end() )
   {
      ModelData model_data(this);
      m_pBridgeSiteModels->insert( std::make_pair(gdrIdx,model_data) );
      found = m_pBridgeSiteModels->find(gdrIdx);
   }

   ModelData* pModelData = &(found->second);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // if the models are already build, leave now
   if ( analysisType == pgsTypes::Simple && pModelData->m_Model != NULL )
   {
      return;
   }
   else if ( analysisType == pgsTypes::Continuous && pModelData->m_ContinuousModel != NULL )
   {
      return;
   }
   else if ( analysisType == pgsTypes::Envelope && pModelData->m_Model != NULL && pModelData->m_ContinuousModel != NULL )
   {
      return;
   }

   // build the simple span model
   bool bBuildSimple = false;
   if ( (analysisType == pgsTypes::Simple || analysisType == pgsTypes::Envelope) && pModelData->m_Model == NULL )
   {
      std::_tostringstream os;
      os << _T("Building Simple Span Bridge Site Analysis model for Girderline ") << LABEL_GIRDER(gdrIdx) << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      CComPtr<ILBAMModel> model;
      model.CoCreateInstance(CLSID_LBAMModel);
      model->put_Name(CComBSTR("Simple"));
      pModelData->AddSimpleModel(model);
      BuildBridgeSiteModel(gdrIdx,false,pModelData->pContraflexureResponse[pgsTypes::SimpleSpan],pModelData->pDeflContraflexureResponse[pgsTypes::SimpleSpan],model);
      bBuildSimple = true;
   }

   // build the simple made continuous model
   bool bBuildContinuous = false;
   if ( (analysisType == pgsTypes::Continuous || analysisType == pgsTypes::Envelope) && pModelData->m_ContinuousModel == NULL )
   {
      std::_tostringstream os;
      os << _T("Building Continuous Bridge Site Analysis model for Girderline ") << LABEL_GIRDER(gdrIdx) << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      CComQIPtr<ILBAMModel> continuous_model;
      continuous_model.CoCreateInstance(CLSID_LBAMModel);
      continuous_model->put_Name(CComBSTR("Continuous"));
      pModelData->AddContinuousModel(continuous_model);
      BuildBridgeSiteModel(gdrIdx,true,pModelData->pContraflexureResponse[pgsTypes::ContinuousSpan],pModelData->pDeflContraflexureResponse[pgsTypes::ContinuousSpan],continuous_model);
      bBuildContinuous = true;
   }


   // create the points of interest in the analysis models
   if ( bBuildSimple && pModelData->m_ContinuousModel != NULL )
   {
      // copy POIs from continuous model to simple model
      CComPtr<IPOIs> source_pois, destination_pois;
      pModelData->m_Model->get_POIs(&destination_pois);
      pModelData->m_ContinuousModel->get_POIs(&source_pois);
      CComPtr<IEnumPOI> enumPOI;
      source_pois->get__EnumElements(&enumPOI);
      CComPtr<IPOI> objPOI;
      while ( enumPOI->Next(1,&objPOI,NULL) != S_FALSE )
      {
         destination_pois->Add(objPOI);
         objPOI.Release();
      }
   }
   else if ( bBuildContinuous && pModelData->m_Model != NULL )
   {
      // copy POIs from simple model to continous model
      CComPtr<IPOIs> source_pois, destination_pois;
      pModelData->m_Model->get_POIs(&source_pois);
      pModelData->m_ContinuousModel->get_POIs(&destination_pois);
      CComPtr<IEnumPOI> enumPOI;
      source_pois->get__EnumElements(&enumPOI);
      CComPtr<IPOI> objPOI;
      while ( enumPOI->Next(1,&objPOI,NULL) != S_FALSE )
      {
         destination_pois->Add(objPOI);
         objPOI.Release();
      }
   }
   else
   {
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPoi( pPOI->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,gdrIdx,ALL_SEGMENTS)) );
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
         if ( poi_id == INVALID_ID )
         {
            poi_id = AddPointOfInterest( pModelData, poi );
         }
      }
   }
}

void CAnalysisAgentImp::CheckGirderEndGeometry(IBridge* pBridge,const CGirderKey& girderKey)
{
   CSegmentKey segmentKey(girderKey,0);
   Float64 s_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   segmentKey.segmentIndex = nSegments-1;
   Float64 e_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
   if (s_end_size < 0.0 || e_end_size < 0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
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
         os<<" of Girder "<<LABEL_GIRDER(girderKey.girderIndex)<<" in Span "<< LABEL_SPAN(girderKey.groupIndex) <<". \r\nThis problem can be resolved by increasing the girder End Distance in the Connection library, or by decreasing the skew angle of the girder with respect to the pier.";
      else
         os<<" of Girder "<<LABEL_GIRDER(girderKey.girderIndex)<<" in Group "<< LABEL_GROUP(girderKey.groupIndex) <<". \r\nThis problem can be resolved by increasing the girder End Distance in the Connection library, or by decreasing the skew angle of the girder with respect to the pier.";

      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,os.str().c_str());
      pStatusCenter->Add(pStatusItem);

      os<<"\r\nSee the Status Center for Details";

      THROW_UNWIND(os.str().c_str(),-1);
   }
}

void CAnalysisAgentImp::BuildBridgeSiteModel(GirderIndexType gdr,bool bContinuousModel,IContraflexureResponse* pContraflexureResponse,IContraflexureResponse* pDeflContraflexureResponse,ILBAMModel* pModel)
{
   try
   {
      // prepare load modifiers
      lrfdLoadModifier load_modifier;
      GET_IFACE(ILoadModifiers,pLoadModifiers);
      load_modifier.SetDuctilityFactor( (lrfdLoadModifier::Level)pLoadModifiers->GetDuctilityLevel() ,pLoadModifiers->GetDuctilityFactor());
      load_modifier.SetImportanceFactor((lrfdLoadModifier::Level)pLoadModifiers->GetImportanceLevel(),pLoadModifiers->GetImportanceFactor());
      load_modifier.SetRedundancyFactor((lrfdLoadModifier::Level)pLoadModifiers->GetRedundancyLevel(),pLoadModifiers->GetRedundancyFactor());

      CreateLBAMStages(gdr,pModel);
      CreateLBAMSpans(gdr,bContinuousModel,load_modifier,pModel);
      CreateLBAMSuperstructureMembers(gdr,bContinuousModel,load_modifier,pModel);

      ApplyLiveLoadDistributionFactors(gdr,bContinuousModel,pContraflexureResponse, pModel);

      // Apply Loads
      pgsTypes::AnalysisType analysisType = bContinuousModel ? pgsTypes::Continuous : pgsTypes::Simple;
      ApplySelfWeightLoad(                pModel, analysisType, gdr );
      ApplyDiaphragmLoad(                 pModel, analysisType, gdr );
      ApplyConstructionLoad(              pModel, analysisType, gdr );
      ApplyShearKeyLoad(                  pModel, analysisType, gdr );
      ApplySlabLoad(                      pModel, analysisType, gdr );
      ApplyOverlayLoad(                   pModel, analysisType, gdr, bContinuousModel );
      ApplyTrafficBarrierAndSidewalkLoad( pModel, analysisType, gdr, bContinuousModel );
      ApplyLiveLoadModel(                 pModel, gdr );
      ApplyUserDefinedLoads(              pModel, gdr );
      ApplyEquivalentPTForce(             pModel, gdr );


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

void CAnalysisAgentImp::CreateLBAMStages(GirderIndexType gdr,ILBAMModel* pModel)
{
#pragma Reminder("REVIEW: this assumes the same interval sequence for this girder in all groups")
   CGirderKey girderKey(0,gdr);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals          = pIntervals->GetIntervalCount(girderKey);
   IntervalIndexType startIntervalIdx    = pIntervals->GetFirstSegmentErectionInterval(girderKey);

   CComPtr<IStages> stages;
   pModel->get_Stages(&stages);

   // LBAM stages begin when the first segment is erected
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      CComPtr<IStage> stage;
      stage.CoCreateInstance(CLSID_Stage);
      stage->put_Name( GetLBAMStageName(intervalIdx) );
      stage->put_Description( CComBSTR(pIntervals->GetDescription(girderKey,intervalIdx)) );
      
      stages->Add(stage);
   }
}

void CAnalysisAgentImp::CreateLBAMSpans(GirderIndexType gdr,bool bContinuousModel,lrfdLoadModifier& loadModifier,ILBAMModel* pModel)
{
   // This method creates the basic layout for the LBAM
   // It creates the support, span, and temporary support objects
   
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);

   CComPtr<ISupports> supports;
   pModel->get_Supports(&supports);

   bool bHasXConstraint = false; // make sure there is at least 1 reaction in the X-direction
                                 // otherwise the LBAM will be unstable

   //
   // create the first support (abutment 0)
   //
   CComPtr<ISupport> objSupport;
   objSupport.CoCreateInstance(CLSID_Support);

   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   BoundaryConditionType boundaryCondition = GetLBAMBoundaryCondition(bContinuousModel,pPier);

   if ( boundaryCondition == bcPinned || boundaryCondition == bcFixed )
      bHasXConstraint = true;

   objSupport->put_BoundaryCondition(boundaryCondition);

   objSupport->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

   supports->Add(objSupport);

   // Layout the spans and supports along the girderline
   CComPtr<ISpans> spans;
   pModel->get_Spans(&spans);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GroupIndexType grpIdx    = pBridge->GetGirderGroupIndex(spanIdx);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx   = Min(gdr,nGirders-1);

      CGirderKey girderKey(grpIdx,gdrIdx);

      // pier indicies related to this span
      PierIndexType prevPierIdx = PierIndexType(spanIdx);
      PierIndexType nextPierIdx = prevPierIdx+1;

      // create LBAM span object
      CComPtr<ISpan> objSpan;
      objSpan.CoCreateInstance(CLSID_Span);

      Float64 span_length = pBridge->GetFullSpanLength(spanIdx,gdrIdx); // span length between CL piers, measured along the centerline of the girder
      objSpan->put_Length(span_length); 
      objSpan->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));
      spans->Add(objSpan);

      // support at right end of the span (left side of pier next pier)
      pPier = pBridgeDesc->GetPier(nextPierIdx);
      boundaryCondition = GetLBAMBoundaryCondition(bContinuousModel,pPier);

      if ( boundaryCondition == bcPinned || boundaryCondition == bcFixed )
         bHasXConstraint = true;

      objSupport.Release();
      objSupport.CoCreateInstance(CLSID_Support);
      objSupport->put_BoundaryCondition(boundaryCondition);

      objSupport->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));
      supports->Add(objSupport);
   } // next span

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
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      if ( pBridge->GetTemporarySupportType(tsIdx) == pgsTypes::StrongBack )
         continue; // no temporary support at strongback. Segment weight bears directly on the adjacent segment via the strongback

      SupportIDType tsID = pBridgeDesc->GetTemporarySupport(tsIdx)->GetID();

      SpanIndexType spanIdx;
      Float64 dist_from_start_of_span;
      pBridge->GetTemporarySupportLocation(tsIdx,gdr,&spanIdx,&dist_from_start_of_span);

      CComPtr<ISpans> spans;
      pModel->get_Spans(&spans);

      CComPtr<ISpan> objSpan;
      spans->get_Item(spanIdx,&objSpan);

      CComPtr<ITemporarySupports> objTemporarySupports;
      objSpan->get_TemporarySupports(&objTemporarySupports);

      if ( pBridge->GetSegmentConnectionTypeAtTemporarySupport(tsIdx) == pgsTypes::sctContinuousSegment )
      {
         // for temporary supports with continuous segments, just put a single temporary support
         // object at the centerline of the TS

         //   ======================================
         //                     ^
         //
         CComPtr<ITemporarySupport> objTS;
         objTS.CoCreateInstance(CLSID_TemporarySupport);

         SupportIDType ID = GetTempSupportID(tsIdx);
         objTS->put_ID(ID);
         objTS->put_Location(dist_from_start_of_span);

         objTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         objTS->put_BoundaryCondition(bcRoller);

         EventIndexType erectEventIdx, removalEventIdx;
         pTimelineMgr->GetTempSupportEvents(tsID,&erectEventIdx,&removalEventIdx);

         GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
         CGirderKey girderKey(grpIdx,gdr);
         IntervalIndexType erectIntervalIdx   = pIntervals->GetInterval(girderKey,erectEventIdx);
         IntervalIndexType removalIntervalIdx = pIntervals->GetInterval(girderKey,removalEventIdx);

         ATLASSERT(erectEventIdx == 0); // LBAM doesn't support the stage when a TS is erected
         //objTS->put_StageErected( GetLBAMStageName(erectIntervalIdx) );
         objTS->put_StageRemoved( GetLBAMStageName(removalIntervalIdx) );

         objTemporarySupports->Add(objTS);
      }
      else
      {
         ATLASSERT(pBridge->GetSegmentConnectionTypeAtTemporarySupport(tsIdx) == pgsTypes::sctClosureJoint);

         // There is a discontinuity at this temporary support prior to the closure joint being cast.
         // Model with two support objects to maintain the stability of the LBAM model.
         // The closure joint superstructure member will be modeled with hinges so it doesn't
         // attract load before it is actually installed in the physical mode.

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
         objLeftTS->put_Location(dist_from_start_of_span - left_closure_size - left_end_dist);

         objLeftTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         objLeftTS->put_BoundaryCondition(bcRoller);

         EventIndexType erectEventIdx, removalEventIdx;
         pTimelineMgr->GetTempSupportEvents(tsID,&erectEventIdx,&removalEventIdx);

         IntervalIndexType erectIntervalIdx   = pIntervals->GetInterval(leftSegmentKey,erectEventIdx);
         IntervalIndexType removalIntervalIdx = pIntervals->GetInterval(leftSegmentKey,removalEventIdx);

         ATLASSERT(erectEventIdx == 0); // LBAM doesn't support the stage when a TS is erected
         //objLeftTS->put_StageErected( GetLBAMStageName(erectIntervalIdx) );
         objLeftTS->put_StageRemoved( GetLBAMStageName(removalIntervalIdx) );

         objTemporarySupports->Add(objLeftTS);

         // Right temporary support
         CComPtr<ITemporarySupport> objRightTS;
         objRightTS.CoCreateInstance(CLSID_TemporarySupport);

         ID = -( (SupportIDType)((tsIdx+1)*100+1) );
         objRightTS->put_ID(ID);
         objRightTS->put_Location(dist_from_start_of_span + right_closure_size + right_start_dist);

         objRightTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         objRightTS->put_BoundaryCondition(bcRoller);

         pTimelineMgr->GetTempSupportEvents(tsID,&erectEventIdx,&removalEventIdx);

         erectIntervalIdx   = pIntervals->GetInterval(rightSegmentKey,erectEventIdx);
         removalIntervalIdx = pIntervals->GetInterval(rightSegmentKey,removalEventIdx);

         ATLASSERT(erectEventIdx == 0); // LBAM doesn't support the stage when a TS is erected
         //objRightTS->put_StageErected( GetLBAMStageName(erectIntervalIdx) );
         objRightTS->put_StageRemoved( GetLBAMStageName(removalIntervalIdx) );

         objTemporarySupports->Add(objRightTS);
      }
   }
}

void CAnalysisAgentImp::CreateLBAMSuperstructureMembers(GirderIndexType gdr,bool bContinuousModel,lrfdLoadModifier& loadModifier,ILBAMModel* pModel)
{
   // This method creates the superstructure members for the LBAM

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IPointOfInterest,pPOI);
   GET_IFACE(IMaterials,pMaterial);

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
   //  |-|<---- superstructure member offset = end distance

   Float64 end_distance = pBridge->GetSegmentStartEndDistance(CSegmentKey(0,gdr,0));
   ssms->put_Offset(end_distance);

   //
   // layout the girder segments... each segment is a LBAM superstructure member
   //
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      // deal with girder index when there are different number of girders in each span
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      CGirderKey girderKey(grpIdx,gdrIdx);

      IntervalIndexType nIntervals          = pIntervals->GetIntervalCount(girderKey);
      IntervalIndexType startIntervalIdx    = pIntervals->GetFirstSegmentErectionInterval(girderKey);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);

      CheckGirderEndGeometry(pBridge,girderKey); // make sure girder is on its bearing - if it isn't an UNWIND exception will throw

      // get the girder data
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         CSegmentKey segmentKey(pSegment->GetSegmentKey());

#pragma Reminder("UPDATE: Assuming prismatic members")
         // NOTE: segments can be non-prismatic. Refine the model by breaking the segment into step-wise prismatic
         // pieces and create LBAM superstructure member segments for each piece

         // get POI at mid-point of the segment (mid-segment section properties will be used for EA and EI)
         std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN, POIFIND_AND) );
         ATLASSERT( vPOI.size() == 1 );
         pgsPointOfInterest segmentPoi = vPOI.front();

         IntervalIndexType compositeClosureIntervalIdx = INVALID_INDEX;
         pgsPointOfInterest closurePoi;
         if ( segIdx < nSegments-1 )
         {
            vPOI = pPOI->GetPointsOfInterest(segmentKey,POI_CLOSURE);
            ATLASSERT(vPOI.size() == 1);
            closurePoi = vPOI.front();
            compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);
         }

         // The vector of superstructure member data contains stiffness properties by stage.
         // This should be segment data between section changes, for each stage.
         // For now, assume segments are prismatic

         std::vector<SuperstructureMemberData> vSegmentData;
         std::vector<SuperstructureMemberData> vClosureData;
         SuperstructureMemberData data;

         for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
         {
            // Segment Data

            // young's modulus in this interval
            Float64 Ec = pMaterial->GetSegmentEc(segmentKey,intervalIdx);

            // Get section properties this interval.
            // GetAg/GetIx will return gross or transformed properties based on the
            // current section properties mode
            Float64 Ag = pSectProp->GetAg(intervalIdx,segmentPoi);
            Float64 Ix = pSectProp->GetIx(intervalIdx,segmentPoi);

            data.stage = GetLBAMStageName(intervalIdx);
            data.ea = Ec*Ag;
            data.ei = Ec*Ix;

            if ( intervalIdx < liveLoadIntervalIdx )
            {
               data.ea_defl = data.ea;
               data.ei_defl = data.ei;
            }
            else
            {
               Float64 distFromStartOfBridge = pBridge->GetDistanceFromStartOfBridge(segmentPoi);
               Float64 ei_defl = pSectProp->GetBridgeEIxx( distFromStartOfBridge );
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
                  Ix = pSectProp->GetIx(intervalIdx,closurePoi);
                  data.ea = Ec*Ag;
                  data.ei = Ec*Ix;

                  if ( intervalIdx < liveLoadIntervalIdx )
                  {
                     data.ea_defl = data.ea;
                     data.ei_defl = data.ei;
                  }
                  else
                  {
                     Float64 distFromStartOfBridge = pBridge->GetDistanceFromStartOfBridge(closurePoi);
                     Float64 ei_defl = pSectProp->GetBridgeEIxx( distFromStartOfBridge );
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

         if ( !IsZero(L1) )
         {
            CComPtr<ISuperstructureMember> ssmbr;
            CreateSuperstructureMember(L1,vSegmentData,&ssmbr);
            ssms->Add(ssmbr);
            SetBoundaryConditions(bContinuousModel,pTimelineMgr,grpIdx,pSegment,pgsTypes::metStart,ssmbr);

            // Don't load the cantilever if:
            // 1) This is a simple span model and the left cantilever is not modeled
            // 2) This is a non-continuous connection and the left cantilever is not modeled
            const CPierData2* pPier;
            const CTemporarySupportData* pTS;
            pSegment->GetStartSupport(&pPier,&pTS);
            if ( (!bContinuousModel || pPier && !pPier->IsContinuousConnection()) && !bModelLeftCantilever )
            {
               ssmbr->put_LinkMember(VARIANT_TRUE);
            }
         }

         CComPtr<ISuperstructureMember> ssmbr;
         CreateSuperstructureMember(L2,vSegmentData,&ssmbr);
         ssms->Add(ssmbr);
         if ( IsZero(L1) )
            SetBoundaryConditions(bContinuousModel,pTimelineMgr,grpIdx,pSegment,pgsTypes::metStart,ssmbr);

         if ( IsZero(L3) )
            SetBoundaryConditions(bContinuousModel,pTimelineMgr,grpIdx,pSegment,pgsTypes::metEnd,  ssmbr);

         if ( !IsZero(L3) )
         {
            CComPtr<ISuperstructureMember> ssmbr;
            CreateSuperstructureMember(L3,vSegmentData,&ssmbr);
            ssms->Add(ssmbr);

            SetBoundaryConditions(bContinuousModel,pTimelineMgr,grpIdx,pSegment,pgsTypes::metEnd,  ssmbr);
            
            // Don't load the cantilever if:
            // 1) This is a simple span model and the right cantilever is not modeled
            // 2) This is a non-continuous connection and the right cantilever is not modeled
            const CPierData2* pPier;
            const CTemporarySupportData* pTS;
            pSegment->GetEndSupport(&pPier,&pTS);
            if ( (!bContinuousModel || pPier && !pPier->IsContinuousConnection()) && !bModelRightCantilever )
            {
               ssmbr->put_LinkMember(VARIANT_TRUE);
            }
         }

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSegment->GetEndSupport(&pPier,&pTS);

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

            SegmentIDType closureID = pIBridgeDesc->GetSegmentID(segmentKey);

            EventIndexType closureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(closureID);
            IntervalIndexType closureIntervalIdx = pIntervals->GetInterval(closureKey,closureEventIdx) + 1; // assume closure is at strength 1 interval after casting
            CComBSTR bstrContinuityStage( GetLBAMStageName(closureIntervalIdx) );

            CComPtr<ISuperstructureMember> closure_ssm;
            CreateSuperstructureMember(closure_length,vClosureData,&closure_ssm);
            ssms->Add(closure_ssm);
         }


         //
         // Create LBAM superstructure members at intermediate piers
         //

         // the width of the intermediate piers need to be modeled so the correct overall
         // span length is used when the structure becomes continuous
         if ( ( segIdx == nSegments-1 && grpIdx != nGroups-1 ) // last segment in all but the last group
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
            //      Temporary Supports---+-------------------+
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
            pIntervals->GetContinuityInterval(girderKey,pierIdx,&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

            // use the girder data for the pier superstructure members
            bool bContinuousConnection = bContinuousModel ? pPier->IsContinuousConnection() : false;
            
            // create superstructure member for pier, left of CL Pier
            if ( !IsZero(left_end_offset) )
            {
               CComPtr<ISuperstructureMember> left_pier_diaphragm_ssm;
               CreateSuperstructureMember(left_end_offset,vSegmentData,&left_pier_diaphragm_ssm);
               if ( bContinuousModel && bContinuousConnection )
               {
                  left_pier_diaphragm_ssm->SetEndRelease(ssRight,GetLBAMStageName(leftContinuityIntervalIdx),mrtPinned);
               }
               else
               {
                  left_pier_diaphragm_ssm->put_LinkMember(VARIANT_TRUE);
                  left_pier_diaphragm_ssm->SetEndRelease(ssRight,_T(""),mrtPinned);
               }

               ssms->Add(left_pier_diaphragm_ssm);
            }

            // create superstructure member for pier, right of CL Pier
            if ( !IsZero(right_end_offset) )
            {
               CComPtr<ISuperstructureMember> right_pier_diaphragm_ssm;
               CreateSuperstructureMember(right_end_offset,vSegmentData,&right_pier_diaphragm_ssm);
               if ( bContinuousModel && bContinuousConnection )
               {
                  // intentionally empty
               }
               else
               {
                  right_pier_diaphragm_ssm->put_LinkMember(VARIANT_TRUE);
               }
               ssms->Add(right_pier_diaphragm_ssm);
            }

            //
            // Create dummy temporary supports at the CL Bearing
            //
            CComPtr<ISpans> spans;
            pModel->get_Spans(&spans);

            CComPtr<ISupports> supports;
            pModel->get_Supports(&supports);

            CComPtr<ISupport> objPrimarySupport;
            supports->get_Item(pierIdx,&objPrimarySupport);

            SupportIDType backID, aheadID;
            GetPierTemporarySupportIDs(pierIdx,&backID,&aheadID);

            // Temporary support at CL bearing on left side of pier
            if ( !IsZero(left_end_dist+left_end_offset) )
            {
               CComPtr<ISpan> objSpan;
               spans->get_Item(spanIdx,&objSpan);

               CComPtr<ITemporarySupports> objTemporarySupports;
               objSpan->get_TemporarySupports(&objTemporarySupports);

               Float64 span_length;
               objSpan->get_Length(&span_length);

               CComPtr<ITemporarySupport> objLeftTS;
               objLeftTS.CoCreateInstance(CLSID_TemporarySupport);
               objLeftTS->put_OmitReaction(VARIANT_TRUE); // don't apply the temporary support reaction back into the structure

               objLeftTS->put_ID(backID);
               objLeftTS->put_Location(span_length - left_brg_offset );

               objLeftTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

               objLeftTS->put_BoundaryCondition(bcRoller);

               // If there is a continuous connection at the pier, remove the temporary support when the 
               // connection becomes continuous, otherwise keep it to preserve the stability of the LBAM
               if ( bContinuousConnection )
               {
                  objLeftTS->put_StageRemoved( GetLBAMStageName(leftContinuityIntervalIdx) );
               }
               else
               {
                  objLeftTS->put_StageRemoved(CComBSTR(""));
                  objPrimarySupport->AddAssociatedSupport(backID);
               }

               objTemporarySupports->Add(objLeftTS);
            }

            // Temporary support CL Bearing on right side of pier
            if ( !IsZero(right_end_dist+right_end_offset) )
            {
               CComPtr<ISpan> objSpan;
               spans->get_Item(spanIdx+1,&objSpan);

               CComPtr<ITemporarySupports> objTemporarySupports;
               objSpan->get_TemporarySupports(&objTemporarySupports);

               CComPtr<ITemporarySupport> objRightTS;
               objRightTS.CoCreateInstance(CLSID_TemporarySupport);
               objRightTS->put_OmitReaction(VARIANT_TRUE); // don't apply the temporary support reaction back into the structure


               objRightTS->put_ID(aheadID);
               objRightTS->put_Location(right_brg_offset);

               objRightTS->SetLoadModifier(lctStrength,loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),loadModifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

               objRightTS->put_BoundaryCondition(bcRoller);

               // If there is a continuous connection at the pier, remove the temporary support when the 
               // connection becomes continuous, otherwise keep it to preserve the stability of the LBAM
               if ( bContinuousConnection )
               {
                  objRightTS->put_StageRemoved( GetLBAMStageName(rightContinuityIntervalIdx) );
               }
               else
               {
                  objRightTS->put_StageRemoved(CComBSTR(""));
                  objPrimarySupport->AddAssociatedSupport(aheadID);
               }

               objTemporarySupports->Add(objRightTS);
            }
         }
      } // next segment
   } // next group
}

BoundaryConditionType CAnalysisAgentImp::GetLBAMBoundaryCondition(bool bContinuous,const CPierData2* pPier)
{
   if ( !bContinuous )
      return bcRoller;

   BoundaryConditionType bc;
   if ( pPier->IsBoundaryPier() )
   {
      switch( pPier->GetPierConnectionType() )
      {
      case pgsTypes::Hinge:
         bc = bcPinned;
         break;

      case pgsTypes::Roller:
         bc = bcRoller;
         break;

      case pgsTypes::ContinuousAfterDeck:
      case pgsTypes::ContinuousBeforeDeck:
         bc = bcPinned;
         break;

      case pgsTypes::IntegralAfterDeck:
      case pgsTypes::IntegralBeforeDeck:
         bc = bcFixed;
         break;

      case pgsTypes::IntegralAfterDeckHingeBack:
      case pgsTypes::IntegralBeforeDeckHingeBack:
      case pgsTypes::IntegralAfterDeckHingeAhead:
      case pgsTypes::IntegralBeforeDeckHingeAhead:
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

      switch( pPier->GetPierConnectionType() )
      {
      case pgsTypes::Hinge:
         bc = bcPinned;
         break;

      case pgsTypes::Roller:
         bc = bcRoller;
         break;

      default:
         ATLASSERT(FALSE); // is there a new connection type?
         bc = bcPinned;
      }

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
         bc = bcPinned;
      }
   }

   return bc;
}

CAnalysisAgentImp::SegmentModelData CAnalysisAgentImp::BuildReleaseModels(const CSegmentKey& segmentKey)
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // Build the model
   std::_tostringstream os;
   os << "Building prestress release model" << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   return BuildCastingYardModels(segmentKey,releaseIntervalIdx,0.0,0.0);
}

CAnalysisAgentImp::SegmentModelData CAnalysisAgentImp::BuildStorageModels(const CSegmentKey& segmentKey)
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

   // Build the model
   std::_tostringstream os;
   os << "Building storage model" << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   Float64 left  = pSegment->HandlingData.LeftStoragePoint;
   Float64 right = pSegment->HandlingData.RightStoragePoint;


   return BuildCastingYardModels(segmentKey,storageIntervalIdx,left,right);
}

CAnalysisAgentImp::SegmentModelData CAnalysisAgentImp::BuildCastingYardModels(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 leftSupportDistance,Float64 rightSupportDistance)
{
   // Get the interface pointers we are going to use
   GET_IFACE(IBridge,            pBridge );
   GET_IFACE(IMaterials,     pMaterial );
   GET_IFACE(ISectionProperties, pSectProp);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);



   // For casting yard models, use the entire girder length as the
   // span length.
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   // Get the material properties
   Float64 Ec = pMaterial->GetSegmentEc(segmentKey,intervalIdx);

   // Get points of interest
   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPOI( pIPoi->GetPointsOfInterest(segmentKey) );
   pIPoi->RemovePointsOfInterest(vPOI,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPOI,POI_BOUNDARY_PIER);

   // Build the Model
   SegmentModelData model_data;
   model_data.Interval = intervalIdx;
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,intervalIdx,segmentKey,leftSupportDistance,Lg-rightSupportDistance,Ec,g_lcidGirder,true,vPOI,&model_data.Model,&model_data.PoiMap);

   // Generate unit loads
   CComPtr<IFem2dPOICollection> POIs;
   model_data.Model->get_POIs(&POIs);
   LoadCaseIDType lcid = g_lcidUnitLoadBase;
   CComPtr<IFem2dLoadingCollection> loadings;
   model_data.Model->get_Loadings(&loadings);
   std::vector<pgsPointOfInterest>::iterator iter(vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPOI.end());
   for ( ; iter != end; iter++, lcid-- )
   {
      pgsPointOfInterest& poi = *iter;

      CComPtr<IFem2dLoading> loading;
      loadings->Create(lcid,&loading);

      CComPtr<IFem2dPointLoadCollection> pointLoads;
      loading->get_PointLoads(&pointLoads);

      PoiIDType modelPoiID = model_data.PoiMap.GetModelPoi(poi);

      CComPtr<IFem2dPOI> femPoi;
      POIs->Find(modelPoiID,&femPoi);

      MemberIDType mbrID;
      Float64 x;
      femPoi->get_MemberID(&mbrID);
      femPoi->get_Location(&x);

      CComPtr<IFem2dPointLoad> pointLoad;
      Float64 Py = 1.0; // upwards load of magnitude 1.0
      pointLoads->Create(0,mbrID,x,0.0,Py,0.0,lotMember,&pointLoad);

      model_data.UnitLoadIDMap.insert(std::make_pair(poi.GetID(),lcid));
   }

   return model_data;
}

Float64 CAnalysisAgentImp::GetReactions(SegmentModels& model,PierIndexType pierIdx,const CGirderKey& girderKey)
{
   GirderIndexType gdrIdx = girderKey.girderIndex;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
   
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   CSegmentKey segmentKey;
   if ( pGirder->GetPierIndex(pgsTypes::metStart) == pierIdx )
   {
      segmentKey = pGirder->GetSegment(0)->GetSegmentKey();
   }
   else if ( pGirder->GetPierIndex(pgsTypes::metEnd) == pierIdx )
   {
      segmentKey = pGirder->GetSegment(nSegments-1)->GetSegmentKey();
   }
   else
   {
      for ( SegmentIndexType segIdx = 1; segIdx < nSegments-1; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SpanIndexType spanIdx = pSegment->GetSpanIndex(pgsTypes::metStart);
         if ( spanIdx == pierIdx )
         {
            segmentKey = pSegment->GetSegmentKey();
            break;
         }
      }
   }

   SegmentModelData* pModelData = GetModelData( model, segmentKey );

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   JointIDType jointID;
   if ( pierIdx == 0 )
   {
      jointID = 0;
   }
   else
   {
      CComPtr<IFem2dJointCollection> joints;
      pModelData->Model->get_Joints(&joints);
      CollectionIndexType nJoints;
      joints->get_Count(&nJoints);
      jointID = nJoints-1;
   }

   Float64 fx,fy,mz;
   HRESULT hr = results->ComputeReactions(g_lcidGirder,jointID,&fx,&fy,&mz);
   ATLASSERT(SUCCEEDED(hr));
   return fy;
}

Float64 CAnalysisAgentImp::GetSectionStress(SegmentModels& models,pgsTypes::StressLocation stressLocation,const pgsPointOfInterest& poi)
{
   if (poi.HasAttribute(POI_CLOSURE) || poi.HasAttribute(POI_BOUNDARY_PIER) )
      return 0; // there isn't a closure joint in these segment models.... return 0.0 stress

   if ( IsDeckStressLocation(stressLocation) )
      return 0.0; // the slab isn't cast yet

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Get segment construction stage (i.e. casting yard stage)
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   SegmentModelData* pModelData = GetModelData( models, segmentKey );
   PoiIDType poi_id = pModelData->PoiMap.GetModelPoi( poi );
   if ( poi_id == INVALID_ID )
   {
      poi_id = AddPointOfInterest( models, poi );
   }

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   Float64 fx,fy,mz;
   Fem2dMbrFaceType face = IsZero( poi.GetDistFromStart() ) ? mftRight : mftLeft;

   HRESULT hr = results->ComputePOIForces(g_lcidGirder,poi_id,face,lotGlobal,&fx,&fy,&mz);
   ATLASSERT(SUCCEEDED(hr));

   GET_IFACE(ISectionProperties, pSectProp);
   Float64 A = pSectProp->GetAg(intervalIdx,poi);
   Float64 S = pSectProp->GetS(intervalIdx,poi,stressLocation);

   Float64 stress = (IsZero(A) || IsZero(S) ? 0 : fx/A + mz/S);
   stress = IsZero(stress) ? 0 : stress;

   return stress;
}

void CAnalysisAgentImp::GetSectionResults(SegmentModels& model,LoadCaseIDType lcid,const pgsPointOfInterest& poi,sysSectionValue* pFx,sysSectionValue* pFy,sysSectionValue* pMz,Float64* pDx,Float64* pDy,Float64* pRz)
{
   if ( poi.HasAttribute(POI_CLOSURE) || poi.HasAttribute(POI_BOUNDARY_PIER) )
   {
      // the POI is at a closure joint... closures have not been installed yet so
      // use 0 for the results

#if defined _DEBUG
      // verify that the closure is beyond the end of the segment (this is our assumption by checking on the attribute and not the position)
      GET_IFACE(IBridge,pBridge);
      Float64 segment_length = pBridge->GetSegmentLength(poi.GetSegmentKey());
      ATLASSERT(poi.GetDistFromStart() <= 0 || segment_length <= poi.GetDistFromStart());
#endif // _DEBUG

      pFx->Left()  = 0;
      pFx->Right() = 0;
      pFy->Left()  = 0;
      pFy->Right() = 0;
      pMz->Left()  = 0;
      pMz->Right() = 0;
      *pDx = 0;
      *pDy = 0;
      *pRz = 0;
   }
   else
   {
      SegmentModelData* pModelData = GetModelData( model, poi.GetSegmentKey() );
      ATLASSERT( pModelData != 0 );

      // Check if results have been cached for this poi.
      PoiIDType poi_id = pModelData->PoiMap.GetModelPoi( poi );

      if ( poi_id == INVALID_ID )
      {
         poi_id = AddPointOfInterest( model, poi );
      }

      ATLASSERT(poi_id != INVALID_ID);


      CComQIPtr<IFem2dModelResults> results(pModelData->Model);

      // Determine if the POI is at a joint or somewhere along the length
      // of a fem2d member
      CComPtr<IFem2dPOICollection> fem2dPOIs;
      pModelData->Model->get_POIs(&fem2dPOIs);

      CComPtr<IFem2dPOI> fem2dPOI;
      fem2dPOIs->Find(poi_id,&fem2dPOI);

      MemberIDType memberID;
      Float64 location;
      fem2dPOI->get_MemberID(&memberID);
      fem2dPOI->get_Location(&location);

      CComPtr<IFem2dMemberCollection> fem2dMembers;
      pModelData->Model->get_Members(&fem2dMembers);

      CollectionIndexType nMembers;
      fem2dMembers->get_Count(&nMembers);

      CComPtr<IFem2dMember> fem2dMember;
      fem2dMembers->Find(memberID,&fem2dMember);

      Float64 mbrLength;
      fem2dMember->get_Length(&mbrLength);

      Float64 FxRight, FyRight, MzRight;
      Float64 FxLeft,  FyLeft,  MzLeft;
      HRESULT hr;
      if ( IsZero(location) )
      {
         // POI is at the start of a fem 2d member... it is at a joint... 
         // get member end forces on each side of the joint

         // We want Left/Right face POI results. The forces on the left face of the poi
         // come from the forces on the right face of the previous member. 
         Float64 dummyFx, dummyFy, dummyMz;
         if ( memberID == 0 )
         {
            // if this is the first fem member, there isn't a previous member so
            // right face forces on the previous member are zero
            FxLeft = 0;
            FyLeft = 0;
            MzLeft = 0;
         }
         else
         {
            // forces on the left face of the POI come from the forces at the right end of the previous fem2d member
            hr = results->ComputeMemberForces(lcid,memberID-1,&dummyFx,&dummyFy,&dummyMz,&FxLeft,&FyLeft,&MzLeft);
            ATLASSERT(SUCCEEDED(hr));
         }

         // forces on the right face of the POI ceom from the forces at the left end of this fem2d member
         hr = results->ComputeMemberForces(lcid,memberID,&FxRight,&FyRight,&MzRight,&dummyFx,&dummyFy,&dummyMz);
         ATLASSERT(SUCCEEDED(hr));
      }
      else if ( IsEqual(location,mbrLength) )
      {
         // POI is at the end of a fem 2d member... it is at a joint... 
         // get member end forces on each side of the joint

         // We want Left/Right face POI results. The forces on the left face of the poi
         // come from the forces on the right face of this member. The forces on the
         // right face of the POI come from the forces on the left face of the next fem2d member.
         Float64 dummyFx, dummyFy, dummyMz;
         hr = results->ComputeMemberForces(lcid,memberID,&dummyFx,&dummyFy,&dummyMz,&FxLeft,&FyLeft,&MzLeft);
         FyLeft *= -1.0;
         ATLASSERT(SUCCEEDED(hr));

         if ( memberID == nMembers-1 )
         {
            // if this is the least fem member, there isn't a next member so
            // right face forces on the next member are zero
            FxRight = 0;
            FyRight = 0;
            MzRight = 0;
         }
         else
         {
            // forces on the right face of the POI come from the forces at the left end of the next fem2d member
            hr = results->ComputeMemberForces(lcid,memberID+1,&FxRight,&FyRight,&MzRight,&dummyFx,&dummyFy,&dummyMz);
            ATLASSERT(SUCCEEDED(hr));
         }
      }
      else
      {
         // POI is somewhere along the fem2d member... get left/right face POI results
         hr = results->ComputePOIForces(lcid,poi_id,mftLeft,lotGlobal,&FxLeft,&FyLeft,&MzLeft);
         ATLASSERT(SUCCEEDED(hr));

         FyLeft *= -1;

         hr = results->ComputePOIForces(lcid,poi_id,mftRight,lotGlobal,&FxRight,&FyRight,&MzRight);
         ATLASSERT(SUCCEEDED(hr));
      }

      pFx->Left()  = FxLeft;
      pFx->Right() = FxRight;

      pFy->Left()  = FyLeft;
      pFy->Right() = FyRight;

      pMz->Left()  = MzLeft;
      pMz->Right() = MzRight;

      hr = results->ComputePOIDeflections(lcid,poi_id,lotGlobal,pDx,pDy,pRz);
      ATLASSERT(SUCCEEDED(hr));
   }
}

void CAnalysisAgentImp::ValidateCamberModels(const CSegmentKey& segmentKey)
{
   GDRCONFIG dummy_config;

   CamberModelData camber_model_data;
   BuildCamberModel(segmentKey,false,dummy_config,&camber_model_data);

   m_PrestressDeflectionModels.insert( std::make_pair(segmentKey,camber_model_data) );

   CamberModelData initial_temp_beam;
   CamberModelData release_temp_beam;
   BuildTempCamberModel(segmentKey,false,dummy_config,&initial_temp_beam,&release_temp_beam);
   m_InitialTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,initial_temp_beam) );
   m_ReleaseTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,release_temp_beam) );
}

CAnalysisAgentImp::CamberModelData CAnalysisAgentImp::GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models)
{
   CamberModels::iterator found;
   found = models.find( segmentKey );

   if ( found == models.end() )
   {
      ValidateCamberModels(segmentKey);
      found = models.find( segmentKey );
   }

   // Model should have already been created in ValidateCamberModels
   ATLASSERT( found != models.end() );

   return (*found).second;
}

void CAnalysisAgentImp::BuildCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pModelData)
{
   Float64 Ms;  // Concentrated moments at straight strand debond location
   Float64 Msl;   // Concentrated moments at straight strand bond locations (left)
   Float64 Msr;   // Concentrated moments at straight strand bond locations (right)
   Float64 Mhl;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (left)
   Float64 Mhr;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (right)
   Float64 Nl;   // Vertical loads at left harping point
   Float64 Nr;   // Vertical loads at right harping point
   Float64 Ps;   // Force in straight strands (varies with location due to debonding)
   Float64 Ph;   // Force in harped strands
   Float64 ecc_harped_start; // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_end; // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_hp1;  // Eccentricity of harped strand at harping point (left)
   Float64 ecc_harped_hp2;  // Eccentricity of harped strand at harping point (right)
   Float64 ecc_straight_start;   // Eccentricity of straight strands (left)
   Float64 ecc_straight_end;   // Eccentricity of straight strands (right)
   Float64 ecc_straight_debond;   // Eccentricity of straight strands (location varies)
   Float64 hp1; // Location of left harping point
   Float64 hp2; // Location of right harping point
   Float64 Lg;  // Length of girder

   // These are the interfaces we will be using
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(ISectionProperties,pSectProp);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 E;
   if ( bUseConfig )
   {
      if ( config.bUserEci )
         E = config.Eci;
      else
         E = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
   }
   else
   {
      E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
   }

   //
   // Create the FEM model (includes girder dead load)
   //
   Lg = pBridge->GetSegmentLength(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( pIPoi->GetPointsOfInterest(segmentKey) );

   // Remove closure joint POI as they are off the segment
   pIPoi->RemovePointsOfInterest(vPOI,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPOI,POI_BOUNDARY_PIER);

   GET_IFACE(IGirderLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI( pLiftPOI->GetLiftingPointsOfInterest(segmentKey,0,POIFIND_OR) );

   GET_IFACE(IGirderHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI( pHaulPOI->GetHaulingPointsOfInterest(segmentKey,0,POIFIND_OR) );

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());

   std::vector<pgsPointOfInterest>::iterator newEnd( std::unique(vPOI.begin(),vPOI.end()) );
   vPOI.erase(newEnd,vPOI.end());

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,intervalIdx,segmentKey,0.0,Lg,E,g_lcidGirder,false,vPOI,&pModelData->Model,&pModelData->PoiMap);

   //
   // Apply the loads due to prestressing (use prestress force at mid-span)
   //

   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   pModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidHarpedStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);
   LoadIDType ptLoadID;
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   vPOI = pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN,POIFIND_AND);
   ATLASSERT( vPOI.size() == 1 );
   pgsPointOfInterest mid_span_poi( vPOI.front() );

#if defined _DEBUG
   ATLASSERT( mid_span_poi.IsMidSpan(POI_ERECTED_SEGMENT) == true );
#endif

   pgsPointOfInterest poiStart(segmentKey,0.0);
   pgsPointOfInterest poiEnd(segmentKey,Lg);


   // Start with harped strands because they are easiest (assume no debonding)
   hp1 = 0;
   hp2 = 0;
   Nl = 0;
   Nr = 0;
   Mhl = 0;
   Mhr = 0;


   StrandIndexType Nh = (bUseConfig ? config.PrestressConfig.GetNStrands(pgsTypes::Harped) : pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped));
   if ( 0 < Nh )
   {
      // Determine the prestress force
      if ( bUseConfig )
         Ph = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Harped,intervalIdx,pgsTypes::Start);
      else
         Ph = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Harped,intervalIdx,pgsTypes::Start);

      // get harping point locations
      vPOI.clear(); // recycle the vector
      vPOI = pIPoi->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
      ATLASSERT( 0 <= vPOI.size() && vPOI.size() < 3 );
      pgsPointOfInterest hp1_poi;
      pgsPointOfInterest hp2_poi;
      if ( vPOI.size() == 0 )
      {
         hp1_poi.SetSegmentKey(segmentKey);
         hp1_poi.SetDistFromStart(0.0);
         hp2_poi.SetSegmentKey(segmentKey);
         hp2_poi.SetDistFromStart(0.0);
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else if ( vPOI.size() == 1 )
      { 
         std::vector<pgsPointOfInterest>::const_iterator iter( vPOI.begin() );
         hp1_poi = *iter++;
         hp2_poi = hp1_poi;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else
      {
         std::vector<pgsPointOfInterest>::const_iterator iter( vPOI.begin() );
         hp1_poi = *iter++;
         hp2_poi = *iter++;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }

      // Determine eccentricity of harped strands at end and harp point
      // (assumes eccentricities are the same at each harp point - which they are because
      // of the way the input is defined)
      Float64 nHs_effective;

      if ( bUseConfig )
      {
         ecc_harped_start = pStrandGeom->GetHsEccentricity(intervalIdx, poiStart, config, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetHsEccentricity(intervalIdx, hp1_poi,  config, &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetHsEccentricity(intervalIdx, hp2_poi,  config, &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetHsEccentricity(intervalIdx, poiEnd,   config, &nHs_effective);
      }
      else
      {
         ecc_harped_start = pStrandGeom->GetHsEccentricity(intervalIdx, poiStart, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetHsEccentricity(intervalIdx, hp1_poi,  &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetHsEccentricity(intervalIdx, hp2_poi,  &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetHsEccentricity(intervalIdx, poiEnd,   &nHs_effective);
      }

      // Determine equivalent loads

      // moment
      Mhr = Ph*ecc_harped_start;
      Mhl = Ph*ecc_harped_end;

      // upward force
      Float64 e_prime_start, e_prime_end;
      e_prime_start = ecc_harped_hp1 - ecc_harped_start;
      e_prime_start = IsZero(e_prime_start) ? 0 : e_prime_start;

      e_prime_end = ecc_harped_hp2 - ecc_harped_end;
      e_prime_end = IsZero(e_prime_end) ? 0 : e_prime_end;

      if ( IsZero(hp1) )
         Nl = 0;
      else
         Nl = Ph*e_prime_start/hp1;

      if ( IsZero(Lg-hp2) )
         Nr = 0;
      else
         Nr = Ph*e_prime_end/(Lg-hp2);
   }

   // add loads to the model
   CComPtr<IFem2dPointLoad> ptLoad;
   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(pModelData->Model,0.00,&mbrID,&x);
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Mhl,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,Lg,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Mhr,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,hp1,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,Nl,0.00,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,hp2,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,Nr,0.00,lotGlobal,&ptLoad);

   //
   // Add the effects of the straight strands
   //
   loading.Release();
   pointLoads.Release();

   loadings->Create(g_lcidStraightStrand,&loading);
   loading->get_PointLoads(&pointLoads);
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   // start with the ends of the girder

   // the prestress force varies over the length of the girder.. use the mid-span value as an average
   Float64 nSsEffective;
   if ( bUseConfig )
   {
      ecc_straight_start = pStrandGeom->GetSsEccentricity(intervalIdx, poiStart, config, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetSsEccentricity(intervalIdx, poiEnd,   config, &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Straight,intervalIdx,pgsTypes::Start);

      if ( config.PrestressConfig.GetNStrands(pgsTypes::Straight) != 0 )
         Ps *= nSsEffective/config.PrestressConfig.GetNStrands(pgsTypes::Straight);
   }
   else
   {
      StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
      ecc_straight_start = pStrandGeom->GetSsEccentricity(intervalIdx, poiStart, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetSsEccentricity(intervalIdx, poiEnd,   &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Straight,intervalIdx,pgsTypes::Start);

      if ( Ns != 0 )
         Ps *= nSsEffective/Ns;
   }
   Msl = Ps*ecc_straight_start;
   Msr = Ps*ecc_straight_end;

   pgsGirderModelFactory::FindMember(pModelData->Model,0.00,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Msl,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,Lg,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Msr,lotGlobal,&ptLoad);

   // now do it at the debond sections
   if ( bUseConfig )
   {
      // use tool to extract section data
      DebondSectionComputer dbcomp(config.PrestressConfig.Debond[pgsTypes::Straight], Lg);

      // left end first
      Float64 sign = 1;
      SectionIndexType nSections = dbcomp.GetNumLeftSections();
      SectionIndexType sectionIdx = 0;
      for ( sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
      {
         StrandIndexType nDebondedAtSection;
         Float64 location;
         dbcomp.GetLeftSectionInfo(sectionIdx,&location,&nDebondedAtSection);

         // nDebonded is to be interpreted as the number of strands that become bonded at this section
         // (ok, not at this section but lt past this section)
         Float64 nSsEffective;

         Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,config,pgsTypes::Straight,intervalIdx,pgsTypes::Start);
         ecc_straight_debond = pStrandGeom->GetSsEccentricity(intervalIdx, pgsPointOfInterest(segmentKey,location),config, &nSsEffective);

         Ms = sign*Ps*ecc_straight_debond;

         pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
         ptLoad.Release();
         pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
      }

      // right end 
      sign = -1;
      nSections = dbcomp.GetNumRightSections();
      for ( sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
      {
         StrandIndexType nDebondedAtSection;
         Float64 location;
         dbcomp.GetRightSectionInfo(sectionIdx,&location,&nDebondedAtSection);

         Float64 nSsEffective;

         Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,config,pgsTypes::Straight,intervalIdx,pgsTypes::Start);
         ecc_straight_debond = pStrandGeom->GetSsEccentricity(intervalIdx, pgsPointOfInterest(segmentKey,location),config, &nSsEffective);

         Ms = sign*Ps*ecc_straight_debond;

         pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
         ptLoad.Release();
         pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
      }

   }
   else
   {
      for (int iside=0; iside<2; iside++)
      {
         // left end first, right second
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)iside;
         Float64 sign = (iside==0)?  1 : -1;
         StrandIndexType nSections = pStrandGeom->GetNumDebondSections(segmentKey,endType,pgsTypes::Straight);
         for ( Uint16 sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
         {
            Float64 location = pStrandGeom->GetDebondSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);
            if ( location < 0 || Lg < location )
               continue; // bond occurs after the end of the girder... skip this one

            StrandIndexType nDebondedAtSection = pStrandGeom->GetNumDebondedStrandsAtSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);

            // nDebonded is to be interperted as the number of strands that become bonded at this section
            // (ok, not at this section but lt past this section)
            Float64 nSsEffective;

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,intervalIdx,pgsTypes::Start);
            ecc_straight_debond = pStrandGeom->GetSsEccentricity(intervalIdx, pgsPointOfInterest(segmentKey,location), &nSsEffective);

            Ms = sign*Ps*ecc_straight_debond;

            pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
            ptLoad.Release();
            pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
         }
      }
   }
}

void CAnalysisAgentImp::BuildTempCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData)
{
   Float64 Mi;   // Concentrated moments at ends of beams for eccentric prestress forces
   Float64 Mr;
   Float64 Pti;  // Prestress force in temporary strands initially
   Float64 Ptr;  // Prestress force in temporary strands when removed
   Float64 ecc; // Eccentricity of the temporary strands
   std::vector<pgsPointOfInterest> vPOI; // Vector of points of interest

   // These are the interfaces we will be using
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   // Build models
   Float64 L;
   Float64 Ec;
   Float64 Eci;
   L = pBridge->GetSegmentLength(segmentKey);

   if ( bUseConfig )
   {
      if ( config.bUserEci )
         Eci = config.Eci;
      else
         Eci = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));

      if ( config.bUserEc )
         Ec = config.Ec;
      else
         Ec = pMaterial->GetEconc(config.Fc,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
   }
   else
   {
      Eci = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);
      Ec  = pMaterial->GetSegmentEc(segmentKey,tsRemovalIntervalIdx);
   }

   vPOI = pIPoi->GetPointsOfInterest(segmentKey);

   // Remove closure joint POI as they are off the segment
   pIPoi->RemovePointsOfInterest(vPOI,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPOI,POI_BOUNDARY_PIER);

   GET_IFACE(IGirderLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI( pLiftPOI->GetLiftingPointsOfInterest(segmentKey,0,POIFIND_OR) );

   GET_IFACE(IGirderHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI( pHaulPOI->GetHaulingPointsOfInterest(segmentKey,0,POIFIND_OR) );

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());
   std::vector<pgsPointOfInterest>::iterator newEnd( std::unique(vPOI.begin(),vPOI.end()) );
   vPOI.erase(newEnd,vPOI.end());

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,releaseIntervalIdx,   segmentKey,0.0,L,Eci,g_lcidGirder,false,vPOI,&pInitialModelData->Model,&pInitialModelData->PoiMap);
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,tsRemovalIntervalIdx, segmentKey,0.0,L,Ec, g_lcidGirder,false,vPOI,&pReleaseModelData->Model,&pReleaseModelData->PoiMap);

   // Determine the prestress forces and eccentricities
   vPOI = pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN);
   CHECK( vPOI.size() != 0 );
   const pgsPointOfInterest& mid_span_poi = vPOI.front();

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   pgsTypes::TTSUsage tempStrandUsage = (bUseConfig ? config.PrestressConfig.TempStrandUsage : pStrands->TempStrandUsage);
   Float64 nTsEffective;

   if ( bUseConfig )
   {
      Pti = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::Start);
      Ptr = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End);
      ecc = pStrandGeom->GetTempEccentricity(tsInstallIntervalIdx, pgsPointOfInterest(segmentKey,0.00),config, &nTsEffective);
   }
   else
   {
      Pti = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::Start);
      Ptr = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End);
      ecc = pStrandGeom->GetTempEccentricity(tsInstallIntervalIdx, pgsPointOfInterest(segmentKey,0.00),&nTsEffective);
   }


   // Determine equivalent loads
   Mi = Pti*ecc;
   Mr = Ptr*ecc;


   // Apply loads to initial model
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   pInitialModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidTemporaryStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);
   LoadIDType ptLoadID;
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   CComPtr<IFem2dPointLoad> ptLoad;
   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(pInitialModelData->Model,0.00,&mbrID,&x);
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Mi,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pInitialModelData->Model,L,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Mi,lotGlobal,&ptLoad);

   // apply loads to the release model
   ptLoadID = 0;
   loadings.Release();
   loading.Release();
   pReleaseModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidTemporaryStrand,&loading);

   pointLoads.Release();
   loading->get_PointLoads(&pointLoads);
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   pgsGirderModelFactory::FindMember(pReleaseModelData->Model,0.00,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Mr,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pReleaseModelData->Model,L,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00, Mr,lotGlobal,&ptLoad);
}

/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CAnalysisAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IProductLoads,             this );
   pBrokerInit->RegInterface( IID_IProductForces,            this );
   pBrokerInit->RegInterface( IID_IProductForces2,           this );
   pBrokerInit->RegInterface( IID_ICombinedForces,           this );
   pBrokerInit->RegInterface( IID_ICombinedForces2,          this );
   pBrokerInit->RegInterface( IID_ILimitStateForces,         this );
   pBrokerInit->RegInterface( IID_ILimitStateForces2,        this );
   pBrokerInit->RegInterface( IID_IExternalLoading,          this );
   pBrokerInit->RegInterface( IID_IPretensionStresses,       this );
   pBrokerInit->RegInterface( IID_IPosttensionStresses,      this );
   pBrokerInit->RegInterface( IID_ICamber,                   this );
   pBrokerInit->RegInterface( IID_IContraflexurePoints,      this );
   pBrokerInit->RegInterface( IID_IContinuity,               this );
   pBrokerInit->RegInterface( IID_IBearingDesign,            this );
   pBrokerInit->RegInterface( IID_IPrecompressedTensileZone, this );
   pBrokerInit->RegInterface( IID_IInfluenceResults,         this );

   return S_OK;
};

STDMETHODIMP CAnalysisAgentImp::Init()
{
   CREATE_LOGFILE("AnalysisAgent");

   AGENT_INIT;

   // create an array for pois going into the lbam. create it here once so we don't need to make a new one every time we need it
   HRESULT hr = m_LBAMPoi.CoCreateInstance(CLSID_IDArray);
   CHECK( SUCCEEDED(hr) );

   // Register status callbacks that we want to use
   m_scidInformationalError = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusError,IDH_GIRDER_CONNECTION_ERROR)); // informational with help for girder end offset error
   m_scidVSRatio            = pStatusCenter->RegisterCallback(new pgsVSRatioStatusCallback(m_pBroker));
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback( new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));

   // Create Product Load Map
   m_ProductLoadMap.AddLoadItem(pftGirder,_T("Girder"));
   m_ProductLoadMap.AddLoadItem(pftDiaphragm,_T("Diaphragm"));
   m_ProductLoadMap.AddLoadItem(pftConstruction,_T("Construction"));
   m_ProductLoadMap.AddLoadItem(pftSlab,_T("Slab"));
   m_ProductLoadMap.AddLoadItem(pftSlabPad,_T("Haunch"));
   m_ProductLoadMap.AddLoadItem(pftSlabPanel,_T("Slab Panel"));
   m_ProductLoadMap.AddLoadItem(pftOverlay,_T("Overlay"));
   m_ProductLoadMap.AddLoadItem(pftOverlayRating,_T("Overlay Rating"));
   m_ProductLoadMap.AddLoadItem(pftTrafficBarrier,_T("Traffic Barrier"));
   m_ProductLoadMap.AddLoadItem(pftSidewalk,_T("Sidewalk"));
   m_ProductLoadMap.AddLoadItem(pftUserDC,_T("UserDC"));
   m_ProductLoadMap.AddLoadItem(pftUserDW,_T("UserDW"));
   m_ProductLoadMap.AddLoadItem(pftUserLLIM,_T("UserLLIM"));
   m_ProductLoadMap.AddLoadItem(pftShearKey,_T("Shear Key"));
   m_ProductLoadMap.AddLoadItem(pftTotalPostTensioning,_T("Total Post Tensioning"));
   m_ProductLoadMap.AddLoadItem(pftPrimaryPostTensioning,_T("Primary Post Tensioning"));
   m_ProductLoadMap.AddLoadItem(pftSecondaryEffects,_T("Secondary Effects"));
   m_ProductLoadMap.AddLoadItem(pftCreep,_T("Creep"));
   m_ProductLoadMap.AddLoadItem(pftShrinkage,_T("Shrinkage"));
   m_ProductLoadMap.AddLoadItem(pftRelaxation,_T("Relaxation"));

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CAnalysisAgentImp::Init2()
{

   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the bridge description
   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the specification
   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the rating specification
   hr = pBrokerInit->FindConnectionPoint( IID_IRatingSpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwRatingSpecCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the load modifiers
   hr = pBrokerInit->FindConnectionPoint( IID_ILoadModifiersEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLoadModifierCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_AnalysisAgent;
   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::Reset()
{
   LOG("Reset");
   m_LBAMPoi.Release();

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::ShutDown()
{
   LOG("AnalysisAgent Log Closed");

   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IRatingSpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwRatingSpecCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILoadModifiersEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLoadModifierCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   CLOSE_LOGFILE;

   AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}

void CAnalysisAgentImp::ApplySelfWeightLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdrIdx)
{
#pragma Reminder("UPDATE: need to add dead load of closure joints in the interval when they are cast")
   // closure joint dead loads can be modeled as a point load or a distributed load over the length of the joint

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pftGirder) );
   CComBSTR bstrStage;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   MemberIDType mbrID = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType gdr = Min(gdrIdx,pGroup->GetGirderCount()-1);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdr);

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
         CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),segIdx);

         // determine which stage to apply the load
         IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         bstrStage = GetLBAMStageName(erectSegmentIntervalIdx);

         // get the dead load items
         std::vector<GirderLoad> vGirderLoads;
         std::vector<DiaphragmLoad> vDiaphragmLoads;
         GetGirderSelfWeightLoad(segmentKey,&vGirderLoads,&vDiaphragmLoads);
         // precast diaphragm loads (vDiaphragmLoads) are applied in ApplyIntermediateDiaphragmLoads

         mbrID = ApplyDistributedLoads(erectSegmentIntervalIdx,pModel,analysisType,mbrID,segmentKey,vGirderLoads,bstrStage,bstrLoadGroup);

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetEndSupport(&pPier,&pTS);
         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}

void CAnalysisAgentImp::ApplyDiaphragmLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr)
{
   ApplyDiaphragmLoadsAtPiers(pModel,analysisType,gdr);
   ApplyIntermediateDiaphragmLoads(pModel,analysisType,gdr);
}

void CAnalysisAgentImp::ApplySlabLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr)
{
   // if there is no deck, get the heck outta here!
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
      return;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);

   CComBSTR bstrSlabLoadGroup( GetLoadGroupName(pftSlab) );
   CComBSTR bstrSlabPadLoadGroup( GetLoadGroupName(pftSlabPad) );
   CComBSTR bstrPanelLoadGroup( GetLoadGroupName(pftSlabPanel) );

   MemberIDType mbrID = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirdersInGroup = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirdersInGroup-1);

      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);
      CGirderKey girderKey(pSplicedGirder->GetGirderKey());

      IntervalIndexType castSlabInterval = pIntervals->GetCastDeckInterval(girderKey);
      CComBSTR bstrStage( GetLBAMStageName(castSlabInterval) );

      if ( grpIdx != 0 )
      {
         CSegmentKey segmentKey(girderKey,0);
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
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         // Create equivalent LinearLoad vectors from the SlabLoad information
         std::vector<LinearLoad> vSlabLoads, vHaunchLoads, vPanelLoads;
         GetSlabLoad(segmentKey,vSlabLoads,vHaunchLoads,vPanelLoads);

         ApplyDistributedLoads(castSlabInterval,pModel,analysisType,mbrID,segmentKey,vSlabLoads,bstrStage,bstrSlabLoadGroup);
         ApplyDistributedLoads(castSlabInterval,pModel,analysisType,mbrID,segmentKey,vHaunchLoads,bstrStage,bstrSlabPadLoadGroup);
         mbrID = ApplyDistributedLoads(castSlabInterval,pModel,analysisType,mbrID,segmentKey,vPanelLoads,bstrStage,bstrPanelLoadGroup);

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetEndSupport(&pPier,&pTS);
         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}

void CAnalysisAgentImp::ApplyOverlayLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr,bool bContinuousModel)
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
      return;

   // Make sure we have a roadway to work with
   PierIndexType nPiers = pBridge->GetPierCount();
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      Float64 station = pBridge->GetPierStation(pierIdx);
      Float64 dfs     = pBridge->GetDistanceFromStartOfBridge(station);
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

   // setup stage and load group names
   CComBSTR bstrLoadGroup( GetLoadGroupName(pftOverlay) ); 
   CComBSTR bstrLoadGroupRating( GetLoadGroupName(pftOverlayRating) );

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   MemberIDType mbrID = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey(pSplicedGirder->GetGirderKey());
      IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval(girderKey);
      CComBSTR bstrStage( GetLBAMStageName(overlayIntervalIdx) );

      if ( grpIdx != 0 )
      {
         CSegmentKey segmentKey(girderKey,0);
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
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);

         const CPierData2* pLeftPier = NULL;
         const CTemporarySupportData* pLeftTS = NULL;
         pSegment->GetStartSupport(&pLeftPier,&pLeftTS);

         const CPierData2* pRightPier = NULL;
         const CTemporarySupportData* pRightTS = NULL;
         pSegment->GetEndSupport(&pRightPier,&pRightTS);

         // Add the overlay load in the main part of the span
         std::vector<OverlayLoad> vOverlayLoads;
         GetMainSpanOverlayLoad(segmentKey, &vOverlayLoads);

         std::vector<LinearLoad> vLinearLoads;
         std::vector<OverlayLoad>::iterator iter(vOverlayLoads.begin());
         std::vector<OverlayLoad>::iterator end(vOverlayLoads.end());
         for ( ; iter != end; iter++ )
         {
            LinearLoad load = *iter;
            vLinearLoads.push_back(load);
         }

         MemberIDType newMbrID = ApplyDistributedLoads(overlayIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroup);

         if ( !bFutureOverlay )
            ApplyDistributedLoads(overlayIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroupRating);

         // model the load at the closure joint
         if ( segIdx != nSegments-1 )
         {
            // Load at the start of the closure
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().StartLoc;
            Float64 Xend   = vLinearLoads.back().EndLoc;
            Float64 Wstart = vLinearLoads.back().wStart;
            Float64 Wend   = vLinearLoads.back().wEnd; // this is the load at the CL Pier

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
            closureLoad->put_WEnd(vNextSegmentOverlayLoads.front().wStart);
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
              grpIdx != nGroups-1 && // not the last group/span in the bridge
              pGroup->GetPier(pgsTypes::metEnd)->IsContinuousConnection() // continuous boundary condition
            )
         {
            // Determine load on left side of CL Pier
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().StartLoc;
            Float64 Xend   = vLinearLoads.back().EndLoc;
            Float64 Wstart = vLinearLoads.back().wStart;
            Float64 Wend   = vLinearLoads.back().wEnd; // this is the load at the CL Pier

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


            // Determine the load on the right side of the CL Pier
            vOverlayLoads.clear();
            CSegmentKey nextSegmentKey(segmentKey.groupIndex+1,segmentKey.girderIndex,segmentKey.segmentIndex);
            nextSegmentKey.girderIndex = Min(nextSegmentKey.girderIndex,pBridge->GetGirderCount(nextSegmentKey.groupIndex));
            GetMainSpanOverlayLoad(nextSegmentKey,&vOverlayLoads);

            Wstart = vLinearLoads.back().wEnd;
            Wend   = vOverlayLoads.front().wStart;
            Xstart = 0;
            Xend = vOverlayLoads.front().EndLoc;

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

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrLoadGroup,load2,&distLoadItem);

            if ( !bFutureOverlay )
            {
               distLoadItem.Release();
               distLoads->Add(bstrStage,bstrLoadGroupRating,load2,&distLoadItem);
            }
         }

         mbrID = newMbrID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetEndSupport(&pPier,&pTS);
         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}

void CAnalysisAgentImp::ApplyConstructionLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IBridge,pBridge);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // apply construction loads in the interval when the deck is cast
   GET_IFACE(IIntervals,pIntervals);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pftConstruction) ); 

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   MemberIDType mbrID = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey(pSplicedGirder->GetGirderKey());

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
      CComBSTR bstrStage( GetLBAMStageName(castDeckIntervalIdx) );

      if ( grpIdx != 0 )
      {
         CSegmentKey segmentKey(girderKey,0);
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
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         // Add the overlay load in the main part of the span
         std::vector<ConstructionLoad> loads;
         GetMainConstructionLoad(segmentKey, &loads);
         std::vector<ConstructionLoad>::iterator iter(loads.begin());
         std::vector<ConstructionLoad>::iterator end(loads.end());
         std::vector<LinearLoad> vLinearLoads;
         for ( ; iter != end; iter++ )
         {
            LinearLoad load = *iter;
            vLinearLoads.push_back(load);
         }

         MemberIDType newMbrID = ApplyDistributedLoads(castDeckIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroup);

         // model the load at the closure joint
         if ( segIdx != nSegments-1 )
         {
            // Load at the start of the closure
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().StartLoc;
            Float64 Xend   = vLinearLoads.back().EndLoc;
            Float64 Wstart = vLinearLoads.back().wStart;
            Float64 Wend   = vLinearLoads.back().wEnd; // this is the load at the CL Pier

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
            constructionLoad->put_WEnd(vNextSegmentLoads.front().wStart);
            constructionLoad->put_StartLocation(0.0);
            constructionLoad->put_EndLocation(-1.0);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distLoads->Add(bstrStage,bstrLoadGroup,constructionLoad,&distLoadItem);
         }

         // if we are at an intermediate pier with a continuous boundary condition
         // then we have to load the two LBAM superstructure members used model the pier connection region
         if ( analysisType == pgsTypes::Continuous && // continuous model (don't load across piers for simple span models)
              segIdx == nSegments-1 && // last segment in the girder
              grpIdx != nGroups-1 && // not the last group/span in the bridge
              pGroup->GetPier(pgsTypes::metEnd)->IsContinuousConnection() // continuous boundary condition
            )
         {
            // Determine load on left side of CL Pier
            Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
            Float64 Xstart = vLinearLoads.back().StartLoc;
            Float64 Xend   = vLinearLoads.back().EndLoc;
            Float64 Wstart = vLinearLoads.back().wStart;
            Float64 Wend   = vLinearLoads.back().wEnd; // this is the load at the CL Pier

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

            // Determine the load on the right side of the CL Pier
            loads.clear();
            CSegmentKey nextSegmentKey(segmentKey.groupIndex+1,segmentKey.girderIndex,segmentKey.segmentIndex);
            nextSegmentKey.girderIndex = Min(nextSegmentKey.girderIndex,pBridge->GetGirderCount(nextSegmentKey.groupIndex));
            GetMainConstructionLoad(nextSegmentKey,&loads);

            Wstart = vLinearLoads.back().wEnd;
            Wend   = loads.front().wStart;
            Xstart = 0;
            Xend = loads.front().EndLoc;

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

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrLoadGroup,load2,&distLoadItem);
         }

         mbrID = newMbrID;

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetEndSupport(&pPier,&pTS);
         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}

void CAnalysisAgentImp::ApplyShearKeyLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IBridge,pBridge);

   // assume shear keys are cast with deck
   GET_IFACE(IIntervals,pIntervals);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pftShearKey) ); 

   MemberIDType mbrID = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey(pSplicedGirder->GetGirderKey());

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
      CComBSTR bstrStage( GetLBAMStageName(castDeckIntervalIdx) );

      if ( grpIdx != 0 )
      {
         CSegmentKey segmentKey(girderKey,0);
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
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);

         const CPierData2* pLeftPier = NULL;
         const CTemporarySupportData* pLeftTS = NULL;
         pSegment->GetStartSupport(&pLeftPier,&pLeftTS);

         const CPierData2* pRightPier = NULL;
         const CTemporarySupportData* pRightTS = NULL;
         pSegment->GetEndSupport(&pRightPier,&pRightTS);

         // Add load in the main part of the span
         std::vector<ShearKeyLoad> skLoads;
         GetMainSpanShearKeyLoad(segmentKey, &skLoads);

         std::vector<ShearKeyLoad>::iterator iter(skLoads.begin());
         std::vector<ShearKeyLoad>::iterator end(skLoads.end());
         std::vector<LinearLoad> vLinearLoads;
         for ( ; iter != end; iter++ )
         {
            const ShearKeyLoad& skLoad = *iter;
            LinearLoad load;
            load.StartLoc = skLoad.StartLoc;
            load.EndLoc   = skLoad.EndLoc;
            load.wStart   = skLoad.UniformLoad + skLoad.StartJointLoad;
            load.wEnd     = skLoad.UniformLoad + skLoad.EndJointLoad;
            vLinearLoads.push_back(load);
         }

         mbrID = ApplyDistributedLoads(castDeckIntervalIdx,pModel,analysisType,mbrID,segmentKey,vLinearLoads,bstrStage,bstrLoadGroup);

         // Shear key and joint loads do not get applied to
         // closure joints or piers

         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSplicedGirder->GetSegment(segIdx)->GetEndSupport(&pPier,&pTS);
         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}

void CAnalysisAgentImp::GetSidewalkLoadFraction(const CSegmentKey& segmentKey,Float64* pSidewalkLoad,Float64* pFraLeft,Float64* pFraRight)
{
   ComputeSidewalksBarriersLoadFractions();

   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() );

   const SidewalkTrafficBarrierLoad& rload = found->second;
   *pSidewalkLoad = rload.m_SidewalkLoad;
   *pFraLeft      = rload.m_LeftSidewalkFraction;
   *pFraRight     = rload.m_RightSidewalkFraction;
}

void CAnalysisAgentImp::GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,Float64* pFraExtLeft, Float64* pFraIntLeft,Float64* pFraExtRight,Float64* pFraIntRight)
{
   ComputeSidewalksBarriersLoadFractions();

   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() );

   const SidewalkTrafficBarrierLoad& rload = found->second;
   *pBarrierLoad = rload.m_BarrierLoad;
   *pFraExtLeft  = rload.m_LeftExtBarrierFraction;
   *pFraIntLeft  = rload.m_LeftIntBarrierFraction;
   *pFraExtRight = rload.m_RightExtBarrierFraction;
   *pFraIntRight = rload.m_RightIntBarrierFraction;
}

void CAnalysisAgentImp::ApplyTrafficBarrierAndSidewalkLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdr,bool bContinuousModel)
{
   GET_IFACE(IGirder,            pGdr);
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);

   CComBSTR bstrBarrierLoadGroup( GetLoadGroupName(pftTrafficBarrier) ); 
   CComBSTR bstrSidewalkLoadGroup( GetLoadGroupName(pftSidewalk) ); 

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   MemberIDType mbrID = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      PierIndexType prev_pier = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType next_pier = pGroup->GetPierIndex(pgsTypes::metEnd);

      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey(pSplicedGirder->GetGirderKey());
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(girderKey);
      CComBSTR bstrStage( GetLBAMStageName(railingSystemIntervalIdx) );

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
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);

         const CPierData2* pLeftPier = NULL;
         const CTemporarySupportData* pLeftTS = NULL;
         pSegment->GetStartSupport(&pLeftPier,&pLeftTS);

         const CPierData2* pRightPier = NULL;
         const CTemporarySupportData* pRightTS = NULL;
         pSegment->GetEndSupport(&pRightPier,&pRightTS);

         Float64 left_diaphragm_width(0), right_diaphragm_width(0);
         if ( pLeftPier )
         {
            Float64 H;
            pBridge->GetAheadSideEndDiaphragmSize(pLeftPier->GetIndex(),&left_diaphragm_width,&H);
         }

         if ( pRightPier )
         {
            Float64 H;
            pBridge->GetBackSideEndDiaphragmSize(pRightPier->GetIndex(),&right_diaphragm_width,&H);
         }

         Float64 start_offset   = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 end_offset     = pBridge->GetSegmentEndEndDistance(segmentKey);
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

         Float64 Wtb_per_girder, fraExtLeft, fraExtRight, fraIntLeft, fraIntRight;
         GetTrafficBarrierLoadFraction(segmentKey,&Wtb_per_girder,&fraExtLeft,&fraIntLeft,&fraExtRight,&fraIntRight);

         Float64 Wsw_per_girder, fraLeft, fraRight;
         GetSidewalkLoadFraction(segmentKey,&Wsw_per_girder,&fraLeft,&fraRight);

         std::vector<LinearLoad> tbLoads;
         LinearLoad tbLoad;
         tbLoad.StartLoc = -left_diaphragm_width;
         tbLoad.EndLoc   = segment_length + right_diaphragm_width;
         tbLoad.wStart   = Wtb_per_girder;
         tbLoad.wEnd     = Wtb_per_girder;
         tbLoads.push_back(tbLoad);

         std::vector<LinearLoad> swLoads;
         LinearLoad swLoad;
         swLoad.StartLoc = -left_diaphragm_width;
         swLoad.EndLoc   = segment_length + right_diaphragm_width;
         swLoad.wStart   = Wsw_per_girder;
         swLoad.wEnd     = Wsw_per_girder;
         swLoads.push_back(swLoad);

         MemberIDType newMbrID = ApplyDistributedLoads(railingSystemIntervalIdx,pModel,analysisType,mbrID,segmentKey,tbLoads,bstrStage,bstrBarrierLoadGroup);
         ApplyDistributedLoads(railingSystemIntervalIdx,pModel,analysisType,mbrID,segmentKey,swLoads,bstrStage,bstrSidewalkLoadGroup);

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
              grpIdx != nGroups-1 && // not the last group/span in the bridge
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
         pSplicedGirder->GetSegment(segIdx)->GetEndSupport(&pPier,&pTS);
         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(pSplicedGirder->GetGirderKey(),nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group
}


void CAnalysisAgentImp::ComputeSidewalksBarriersLoadFractions()
{
   // return if we've already done the work
   if (!m_SidewalkTrafficBarrierLoads.empty())
      return;

   GET_IFACE( IBridge,        pBridge );
   GET_IFACE( IBarriers,      pBarriers);
   GET_IFACE( IGirder,        pGdr);
   GET_IFACE( ISpecification, pSpec );
   GET_IFACE( IBridgeDescription, pIBridgeDesc);

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
   pgsBarrierSidewalkLoadDistributionTool BSwTool(LOGGER, pIBridgeDesc, pBridge, pGdr, pBarriers);

   // Loop over all girders in bridge and compute load fractions
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount( CGirderKey(grpIdx,gdrIdx) );
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            // initialize tool to compute loads across all girders in this group
            // for the current segment.
            BSwTool.Initialize(grpIdx, segIdx, distType, nMaxDist);

            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            SidewalkTrafficBarrierLoad stbLoad;

            // compute barrier first
            Float64 FraExtLeft, FraIntLeft, FraExtRight, FraIntRight;
            BSwTool.GetTrafficBarrierLoadFraction(gdrIdx, &FraExtLeft, &FraIntLeft, &FraExtRight, &FraIntRight);

            stbLoad.m_BarrierLoad = -1.0 *( WtbExtLeft*FraExtLeft + WtbIntLeft*FraIntLeft + WtbExtRight*FraExtRight + WtbIntRight*FraIntRight);
            stbLoad.m_LeftExtBarrierFraction  = FraExtLeft;
            stbLoad.m_LeftIntBarrierFraction  = FraIntLeft;
            stbLoad.m_RightExtBarrierFraction = FraExtRight;
            stbLoad.m_RightIntBarrierFraction = FraIntRight;

            // sidewalks
            Float64 FraSwLeft, FraSwRight;
            BSwTool.GetSidewalkLoadFraction(gdrIdx, &FraSwLeft, &FraSwRight);
            stbLoad.m_SidewalkLoad = -1.0 * (WswLeft*FraSwLeft + WswRight*FraSwRight);
            stbLoad.m_LeftSidewalkFraction  = FraSwLeft;
            stbLoad.m_RightSidewalkFraction = FraSwRight;

            // store loads for this segment for later use
            m_SidewalkTrafficBarrierLoads.insert( std::make_pair(segmentKey, stbLoad) );
         }
      }
   }
}

void CAnalysisAgentImp::ApplyLiveLoadModel(ILBAMModel* pModel,GirderIndexType gdrIdx)
{
   HRESULT hr = S_OK;
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLibrary);
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(IRatingSpecification,pRatingSpec);

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

   CComPtr<IVehicularLoads> permit_routine_vehicles;
   permit_routine_liveload_model->get_VehicularLoads(&permit_routine_vehicles);

   CComPtr<IVehicularLoads> permit_special_vehicles;
   permit_special_liveload_model->get_VehicularLoads(&permit_special_vehicles);

   // get the live load names
   std::vector<std::_tstring> design_loads         = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring> permit_loads         = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);
   std::vector<std::_tstring> fatigue_loads        = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::_tstring> routine_legal_loads  = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   std::vector<std::_tstring> special_legal_loads  = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   std::vector<std::_tstring> routine_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   std::vector<std::_tstring> special_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

   // add the live loads to the models
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltDesign,              design_loads,          pLibrary, pLiveLoads, design_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltPermit,              permit_loads,          pLibrary, pLiveLoads, permit_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltFatigue,             fatigue_loads,         pLibrary, pLiveLoads, fatigue_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltLegalRating_Routine, routine_legal_loads,   pLibrary, pLiveLoads, legal_routine_vehicles);
   AddUserLiveLoads(pModel, gdrIdx, pgsTypes::lltLegalRating_Special, special_legal_loads,   pLibrary, pLiveLoads, legal_special_vehicles);
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
   permit_routine_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltPermitRating_Routine));
   permit_special_liveload_model->put_DistributionFactorType(GetLiveLoadDistributionFactorType(pgsTypes::lltPermitRating_Special));
}

void CAnalysisAgentImp::AddUserLiveLoads(ILBAMModel* pModel,GirderIndexType gdrIdx,pgsTypes::LiveLoadType llType,std::vector<std::_tstring>& strLLNames,
                                         ILibrary* pLibrary, ILiveLoads* pLiveLoads, IVehicularLoads* pVehicles)
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

   std::vector<std::_tstring>::iterator iter;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++)
   {
      const std::_tstring& strLLName = *iter;

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

void CAnalysisAgentImp::AddHL93LiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane)
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
   m_LBAMUtility->ConfigureDesignLiveLoad(pModel,llmt,IMtruck,IMlane,bUseDualTruckTrains,bUseDualTruckTrains,units,m_UnitServer);
}

void CAnalysisAgentImp::AddFatigueLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane)
{
   GET_IFACE(ISpecification,pSpec);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );
   SpecUnitType units = pSpecEntry->GetSpecificationUnits() == lrfdVersionMgr::US ? suUS : suSI;

   m_LBAMUtility->ConfigureFatigueLiveLoad(pModel,llmt,IMtruck,IMlane,units,m_UnitServer);
}

void CAnalysisAgentImp::AddDeflectionLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane)
{
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );
   SpecUnitType units = pSpecEntry->GetSpecificationUnits() == lrfdVersionMgr::US ? suUS : suSI;

   m_LBAMUtility->ConfigureDeflectionLiveLoad(pModel,lltDeflection,IMtruck,IMlane,units,m_UnitServer);
}

void CAnalysisAgentImp::AddLegalLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane)
{
   GET_IFACE(ISpecification,pSpec);

   // this is an AASHTO Legal Load for rating, use the LBAM configuration utility
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   VARIANT_BOOL bIncludeType33     = VARIANT_FALSE; // exclude 0.75(Type 3-3) + Lane unless span length is 200ft or more
   VARIANT_BOOL bIncludeDualType33 = VARIANT_FALSE; // exclude dual 0.75(Type 3-3) + Lane unless negative moments need to be processed
   VARIANT_BOOL bRemoveLaneLoad    = VARIANT_FALSE; // don't remove lane load unless directed by the engineer

   bool bOver200 = false;
   Float64 L = ::ConvertToSysUnits(200.0,unitMeasure::Feet);
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      Float64 span_length = pBridge->GetSpanLength(spanIdx);
      if ( L < span_length )
         bOver200 = true;

      if ( pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bIncludeDualType33 = VARIANT_TRUE;
         break;
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

void CAnalysisAgentImp::AddNotionalRatingLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane)
{
   GET_IFACE(ISpecification,pSpec);

   // this is an AASHTO Legal Load for rating, use the LBAM configuration utility
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   m_LBAMUtility->ConfigureNotionalRatingLoad(pModel,llmt,IMtruck,IMlane,m_UnitServer);
}

void CAnalysisAgentImp::AddSHVLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane)
{
   GET_IFACE(ISpecification,pSpec);

   // this is an AASHTO Legal Load for rating, use the LBAM configuration utility
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];
   m_LBAMUtility->ConfigureSpecializedHaulingUnits(pModel,llmt,IMtruck,IMlane,m_UnitServer);
}

void CAnalysisAgentImp::AddUserTruck(const std::_tstring& strLLName,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane,IVehicularLoads* pVehicles)
{
   // this is a user defined vehicular live load defined in the library
   const LiveLoadLibraryEntry* ll_entry = pLibrary->GetLiveLoadEntry( strLLName.c_str());
   ATLASSERT(ll_entry!=NULL);

   CComPtr<IVehicularLoad> vehicular_load; 
   vehicular_load.CoCreateInstance(CLSID_VehicularLoad);

   ATLASSERT( strLLName == ll_entry->GetName() );

   vehicular_load->put_Name(CComBSTR(strLLName.c_str()));
   vehicular_load->put_Applicability((LiveLoadApplicabilityType)ll_entry->GetLiveLoadApplicabilityType());

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
      ATLASSERT(0);
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
         bIsOver = true;
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

void CAnalysisAgentImp::AddPedestrianLoad(const std::_tstring& strLLName,Float64 wPedLL,IVehicularLoads* pVehicles)
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

void CAnalysisAgentImp::AddDummyLiveLoad(IVehicularLoads* pVehicles)
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

void CAnalysisAgentImp::ApplyUserDefinedLoads(ILBAMModel* pModel,GirderIndexType gdr)
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
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      GirderIndexType nGirdersInGroup = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirdersInGroup-1);

      CSpanGirderKey spanGirderKey(spanIdx,gdrIdx);

      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      IntervalIndexType nIntervals = pIntervals->GetIntervalCount(girderKey);

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );


         //
         // point loads
         //
         const std::vector<IUserDefinedLoads::UserPointLoad>* pPointLoads = pUdls->GetPointLoads(intervalIdx,spanGirderKey);

         if (pPointLoads != NULL)
         {
            IndexType nls = pPointLoads->size();
            for(IndexType ild=0; ild<nls; ild++)
            {
               const IUserDefinedLoads::UserPointLoad& load = pPointLoads->at(ild);

               CComPtr<IPointLoad> load2;
               load2.CoCreateInstance(CLSID_PointLoad);
               load2->put_MemberType(mtSpan);
               load2->put_MemberID(spanIdx);
               load2->put_Location(load.m_Location);
               load2->put_Fy(-1.0*load.m_Magnitude);
               load2->put_Mz(0.0);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&ptLoadItem);
            }
         }

         //
         // distributed loads
         //
         const std::vector<IUserDefinedLoads::UserDistributedLoad>* pDistLoads = pUdls->GetDistributedLoads(intervalIdx,spanGirderKey);

         if (pDistLoads!=NULL)
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
         const std::vector<IUserDefinedLoads::UserMomentLoad>* pMomentLoads = pUdls->GetMomentLoads(intervalIdx,spanGirderKey);

         if (pMomentLoads != NULL)
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

void CAnalysisAgentImp::ApplyEquivalentPTForce(ILBAMModel* pModel,GirderIndexType gdrIdx)
{
   // Applies equivalent PT forces that are used to get the total effect of PT
   // on the structure (before losses). The axial force and bending moment that
   // are computed from this load case are used to determine the concrete stress
   // due to PT. The location of the resultant pressure line is determined by
   // dividing the moment by the prestress force. The eccentricity of the pressure line
   // is used to determine stressed due to the PT.
   GET_IFACE(IEventMap,pEventMap);
   GET_IFACE(ITendonGeometry, pTendonGeometry);

   GET_IFACE(IIntervals,pIntervals);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   CComPtr<IDistributedLoads> distributedLoads;
   pModel->get_DistributedLoads(&distributedLoads);

   CComBSTR bstrLoadGroup(GetLoadGroupName(pftTotalPostTensioning));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderKey girderKey(grpIdx,gdrIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      GirderIDType gdrID = pGirder->GetID();

      const CPTData* pPTData = pGirder->GetPostTensioning();
      DuctIndexType nDucts = pPTData->GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType stressTendonInterval = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
         CComBSTR bstrStage(GetLBAMStageName(stressTendonInterval));

         std::vector<EquivPTPointLoad> ptLoads;
         std::vector<EquivPTDistributedLoad> distLoads;
         std::vector<EquivPTMomentLoad> momentLoads;
         GetEquivPTLoads(girderKey,ductIdx,ptLoads,distLoads,momentLoads);

         std::vector<EquivPTPointLoad>::iterator ptLoadIter(ptLoads.begin());
         std::vector<EquivPTPointLoad>::iterator ptLoadIterEnd(ptLoads.end());
         for ( ; ptLoadIter != ptLoadIterEnd; ptLoadIter++ )
         {
            EquivPTPointLoad& load = *ptLoadIter;

            CComPtr<IPointLoad> ptLoad;
            ptLoad.CoCreateInstance(CLSID_PointLoad);
            ptLoad->put_MemberType(mtSpan);
            ptLoad->put_MemberID(load.spanIdx);
            ptLoad->put_Location(load.X);
            ptLoad->put_Fy(load.P);
            ptLoad->put_Mz(0.0);

            CComPtr<IPointLoadItem> ptLoadItem;
            pointLoads->Add(bstrStage,bstrLoadGroup,ptLoad,&ptLoadItem);
         }


         std::vector<EquivPTDistributedLoad>::iterator distLoadIter(distLoads.begin());
         std::vector<EquivPTDistributedLoad>::iterator distLoadIterEnd(distLoads.end());
         for ( ; distLoadIter != distLoadIterEnd; distLoadIter++ )
         {
            EquivPTDistributedLoad& load = *distLoadIter;

            CComPtr<IDistributedLoad> distLoad;
            distLoad.CoCreateInstance(CLSID_DistributedLoad);
            distLoad->put_MemberType(mtSpan);
            distLoad->put_MemberID(load.spanIdx);
            distLoad->put_StartLocation(load.Xstart);
            distLoad->put_EndLocation(load.Xend);
            distLoad->put_WStart(load.Wstart);
            distLoad->put_WEnd(load.Wend);
            distLoad->put_Direction(ldFy);

            CComPtr<IDistributedLoadItem> distLoadItem;
            distributedLoads->Add(bstrStage,bstrLoadGroup,distLoad,&distLoadItem);
         }


         std::vector<EquivPTMomentLoad>::iterator momLoadIter(momentLoads.begin());
         std::vector<EquivPTMomentLoad>::iterator momLoadIterEnd(momentLoads.end());
         for ( ; momLoadIter != momLoadIterEnd; momLoadIter++ )
         {
            EquivPTMomentLoad& load = *momLoadIter;

            CComPtr<IPointLoad> ptLoad;
            ptLoad.CoCreateInstance(CLSID_PointLoad);
            ptLoad->put_MemberType(mtSpan);
            ptLoad->put_MemberID(load.spanIdx);
            ptLoad->put_Location(load.X);
            ptLoad->put_Mz(load.P);
            ptLoad->put_Fy(0.0);

            CComPtr<IPointLoadItem> ptLoadItem;
            pointLoads->Add(bstrStage,bstrLoadGroup,ptLoad,&ptLoadItem);
         }
      }
   }
}

Float64 CAnalysisAgentImp::GetPedestrianLiveLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   Float64 Wleft  = GetPedestrianLoadPerSidewalk(pgsTypes::tboLeft);
   Float64 Wright = GetPedestrianLoadPerSidewalk(pgsTypes::tboRight);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();

   // We don't know which segment we want the ped live load for... there is not enough
   // data. Assume segment 0
   CSegmentKey segmentKey(grpIdx,gdrIdx,0);

   // Pedestrian load is distributed to the same girders, and in the same fraction, as the
   // sidewalk dead load
   Float64 swLoad, fraLeft, fraRight;
   GetSidewalkLoadFraction(segmentKey,&swLoad, &fraLeft,&fraRight);

   Float64 W_per_girder = fraLeft*Wleft + fraRight*Wright;
   return W_per_girder;
}

void CAnalysisAgentImp::ApplyLiveLoadDistributionFactors(GirderIndexType gdr,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel)
{
   // get the distribution factor collection from the model
   CComPtr<IDistributionFactors> span_factors;
   pModel->get_DistributionFactors(&span_factors);

   // get the support collection before entering the loop
   CComPtr<ISupports> supports;
   pModel->get_Supports(&supports);

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

      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);

      CComBSTR bstrStageName( GetLBAMStageName(liveLoadIntervalIdx) );

      // First, we need to get the points of contraflexure as they define the limits of application of
      // the negative moment distribution factors
      CComPtr<IDblArray> cf_locs;
      pContraflexureResponse->ComputeContraflexureLocations(bstrStageName,&cf_locs);

      SpanType spanType = GetSpanType(spanIdx,gdrIdx,bContinuous);
      switch ( spanType )
      {
      case PinPin: 
         ApplyLLDF_PinPin(spanIdx,gdrIdx,cf_locs,span_factors);
         break;

      case PinFix:
         ApplyLLDF_PinFix(spanIdx,gdrIdx,cf_locs,span_factors);
         break;

      case FixPin:
         ApplyLLDF_FixPin(spanIdx,gdrIdx,cf_locs,span_factors);
         break;

      case FixFix:
         ApplyLLDF_FixFix(spanIdx,gdrIdx,cf_locs,span_factors);
         break;

      default:
            ATLASSERT(0); // should never get here
      }

      // layout distribution factors at piers

      ApplyLLDF_Support(spanIdx,gdrIdx,pgsTypes::metStart,supports);
      ApplyLLDF_Support(spanIdx,gdrIdx,pgsTypes::metEnd,  supports);
   } // span loop
}


void CAnalysisAgentImp::ConfigureLoadCombinations(ILBAMModel* pModel)
{
   HRESULT hr;

   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

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
   AddLoadCase(loadcases, CComBSTR("DC"),    CComBSTR("Component and Attachments"));
   AddLoadCase(loadcases, CComBSTR("DW"),    CComBSTR("Wearing Surfaces and Utilities"));
   AddLoadCase(loadcases, CComBSTR("DW_Rating"), CComBSTR("Wearing Surfaces and Utilities (for Load Rating)"));
   AddLoadCase(loadcases, CComBSTR("LL_IM"), CComBSTR("User defined live load"));
   AddLoadCase(loadcases, CComBSTR("CR"), CComBSTR("Creep"));
   AddLoadCase(loadcases, CComBSTR("SH"), CComBSTR("Shrinkage"));
   AddLoadCase(loadcases, CComBSTR("PS"), CComBSTR("Secondary forces due to post-tensioning"));

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
   hr = strength1->put_LiveLoadFactor( pLoadFactors->LLIMmax[pgsTypes::StrengthI] ) ;
   hr = strength1->AddLiveLoadModel(lltDesign) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = strength1->AddLiveLoadModel(lltPedestrian) ;
      hr = strength1->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = strength1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::StrengthI],   pLoadFactors->DCmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->DWmin[pgsTypes::StrengthI],   pLoadFactors->DWmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->DWmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::StrengthI], pLoadFactors->LLIMmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->CRmin[pgsTypes::StrengthI],   pLoadFactors->CRmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->SHmin[pgsTypes::StrengthI],   pLoadFactors->SHmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->PSmin[pgsTypes::StrengthI],   pLoadFactors->PSmax[pgsTypes::StrengthI]);

   hr = loadcombos->Add(strength1) ;

   // STRENGTH-II
   CComPtr<ILoadCombination> strength2;
   hr = strength2.CoCreateInstance(CLSID_LoadCombination) ;
   hr = strength2->put_Name( GetLoadCombinationName(pgsTypes::StrengthII) ) ;
   hr = strength2->put_LoadCombinationType(lctStrength) ;
   hr = strength2->put_LiveLoadFactor( pLoadFactors->LLIMmax[pgsTypes::StrengthII] ) ;
   hr = strength2->AddLiveLoadModel(lltPermit) ;

   if (permit_ped_type != ILiveLoads::PedDontApply)
   {
      hr = strength2->AddLiveLoadModel(lltPedestrian) ;
      hr = strength2->put_LiveLoadModelApplicationType(permit_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = strength2->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::StrengthII],   pLoadFactors->DCmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->DWmin[pgsTypes::StrengthII],   pLoadFactors->DWmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->DWmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::StrengthII], pLoadFactors->LLIMmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->CRmin[pgsTypes::StrengthII],   pLoadFactors->CRmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->SHmin[pgsTypes::StrengthII],   pLoadFactors->SHmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->PSmin[pgsTypes::StrengthII],   pLoadFactors->PSmax[pgsTypes::StrengthII]);

   hr = loadcombos->Add(strength2) ;

   // SERVICE-I
   CComPtr<ILoadCombination> service1;
   hr = service1.CoCreateInstance(CLSID_LoadCombination) ;
   hr = service1->put_Name( GetLoadCombinationName(pgsTypes::ServiceI) ) ;
   hr = service1->put_LoadCombinationType(lctService) ;
   hr = service1->put_LiveLoadFactor(pLoadFactors->LLIMmax[pgsTypes::ServiceI] ) ;
   hr = service1->AddLiveLoadModel(lltDesign) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service1->AddLiveLoadModel(lltPedestrian) ;
      hr = service1->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::ServiceI],   pLoadFactors->DCmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->DWmin[pgsTypes::ServiceI],   pLoadFactors->DWmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->DWmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::ServiceI], pLoadFactors->LLIMmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->CRmin[pgsTypes::ServiceI],   pLoadFactors->CRmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->SHmin[pgsTypes::ServiceI],   pLoadFactors->SHmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->PSmin[pgsTypes::ServiceI],   pLoadFactors->PSmax[pgsTypes::ServiceI]);

   hr = loadcombos->Add(service1) ;

   // SERVICE-III
   CComPtr<ILoadCombination> service3;
   hr = service3.CoCreateInstance(CLSID_LoadCombination) ;
   hr = service3->put_Name( GetLoadCombinationName(pgsTypes::ServiceIII) ) ;
   hr = service3->put_LoadCombinationType(lctService) ;
   hr = service3->put_LiveLoadFactor(pLoadFactors->LLIMmax[pgsTypes::ServiceIII] ) ;
   hr = service3->AddLiveLoadModel(lltDesign) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service3->AddLiveLoadModel(lltPedestrian) ;
      hr = service3->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service3->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::ServiceIII],   pLoadFactors->DCmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->DWmin[pgsTypes::ServiceIII],   pLoadFactors->DWmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->DWmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::ServiceIII], pLoadFactors->LLIMmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->CRmin[pgsTypes::ServiceIII],   pLoadFactors->CRmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->SHmin[pgsTypes::ServiceIII],   pLoadFactors->SHmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->PSmin[pgsTypes::ServiceIII],   pLoadFactors->PSmax[pgsTypes::ServiceIII]);
 
   hr = loadcombos->Add(service3) ;

   // SERVICE-IA... A PGSuper specific load combination not setup by the utility object
   CComPtr<ILoadCombination> service1a;
   service1a.CoCreateInstance(CLSID_LoadCombination);
   service1a->put_Name( GetLoadCombinationName(pgsTypes::ServiceIA) );
   service1a->put_LoadCombinationType(lctService);
   service1a->put_LiveLoadFactor(pLoadFactors->LLIMmax[pgsTypes::ServiceIA] );
   service1a->AddLiveLoadModel(lltDesign);

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service1a->AddLiveLoadModel(lltPedestrian) ;
      hr = service1a->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service1a->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::ServiceIA],   pLoadFactors->DCmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->DWmin[pgsTypes::ServiceIA],   pLoadFactors->DWmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->DWmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::ServiceIA], pLoadFactors->LLIMmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->CRmin[pgsTypes::ServiceIA],   pLoadFactors->CRmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->SHmin[pgsTypes::ServiceIA],   pLoadFactors->SHmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->PSmin[pgsTypes::ServiceIA],   pLoadFactors->PSmax[pgsTypes::ServiceIA]);

   loadcombos->Add(service1a);

   // FATIGUE-I
   CComPtr<ILoadCombination> fatigue1;
   fatigue1.CoCreateInstance(CLSID_LoadCombination);
   fatigue1->put_Name( GetLoadCombinationName(pgsTypes::FatigueI) );
   fatigue1->put_LoadCombinationType(lctFatigue);
   fatigue1->put_LiveLoadFactor(pLoadFactors->LLIMmax[pgsTypes::FatigueI] );
   fatigue1->AddLiveLoadModel(lltFatigue);

   if (fatigue_ped_type != ILiveLoads::PedDontApply)
   {
      hr = fatigue1->AddLiveLoadModel(lltPedestrian) ;
      hr = fatigue1->put_LiveLoadModelApplicationType(fatigue_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::FatigueI],   pLoadFactors->DCmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DWp"),    pLoadFactors->DWmin[pgsTypes::FatigueI],   pLoadFactors->DWmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DWf"),    0.0,   pLoadFactors->DWmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::FatigueI], pLoadFactors->LLIMmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("CR"),    pLoadFactors->CRmin[pgsTypes::FatigueI],   pLoadFactors->CRmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("SH"),    pLoadFactors->SHmin[pgsTypes::FatigueI],   pLoadFactors->SHmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("PS"),    pLoadFactors->PSmin[pgsTypes::FatigueI],   pLoadFactors->PSmax[pgsTypes::FatigueI]);

   loadcombos->Add(fatigue1);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 DC, DW, CR, SH, PS, LLIM;

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_Inventory);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_Inventory);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::StrengthI_Inventory);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_Inventory,true);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_inventory->put_LiveLoadFactor(LLIM);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = strengthI_inventory->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);
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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_Operating);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_Operating);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::StrengthI_Operating);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_Operating,true);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_operating->put_LiveLoadFactor(LLIM);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = strengthI_operating->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_Inventory);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_Inventory);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::ServiceIII_Inventory);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_Inventory,true);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_inventory->put_LiveLoadFactor(LLIM);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = serviceIII_inventory->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_Operating);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_Operating);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::ServiceIII_Operating);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_Operating,true);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_operating->put_LiveLoadFactor(LLIM);

   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = serviceIII_operating->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_LegalRoutine);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_LegalRoutine);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::StrengthI_LegalRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_LegalRoutine,true);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_routine->put_LiveLoadFactor(LLIM);

   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = strengthI_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_LegalRoutine);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_LegalRoutine);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::ServiceIII_LegalRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_LegalRoutine,true);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_routine->put_LiveLoadFactor(LLIM);

   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = serviceIII_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthI_LegalSpecial);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthI_LegalSpecial);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::StrengthI_LegalSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthI_LegalSpecial,true);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthI_special->put_LiveLoadFactor(LLIM);

   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = strengthI_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceIII_LegalSpecial);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_LegalSpecial);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::ServiceIII_LegalSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceIII_LegalSpecial,true);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceIII_special->put_LiveLoadFactor(LLIM);

   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = serviceIII_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

   loadcombos->Add(serviceIII_special);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitRoutine);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitRoutine);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::StrengthII_PermitRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthII_PermitRoutine,true);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthII_routine->put_LiveLoadFactor(LLIM);

   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitSpecial);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitSpecial);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::StrengthII_PermitSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthII_PermitSpecial,true);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthII_special->put_LiveLoadFactor(LLIM);

   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = strengthII_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

   loadcombos->Add(strengthII_special);

   // SERVICE-I - Permit Rating
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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitRoutine);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitRoutine);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::ServiceI_PermitRoutine);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceI_PermitRoutine,true);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceI_routine->put_LiveLoadFactor(LLIM);

   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

   loadcombos->Add(serviceI_routine);


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
   CR = pRatingSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitSpecial);
   SH = pRatingSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitSpecial);
   PS = pRatingSpec->GetPrestressFactor(     pgsTypes::ServiceI_PermitSpecial);
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceI_PermitSpecial,true);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceI_routine->put_LiveLoadFactor(LLIM);

   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("CR"),    CR, CR);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("SH"),    SH, SH);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("PS"),    PS, PS);

   loadcombos->Add(serviceI_special);

   // Design load combination... 
   // These liveload-only combinations are created so we can sum user-defined static live loads with other live loads
   CComPtr<ILoadCombination> lc_design;
   lc_design.CoCreateInstance(CLSID_LoadCombination);
   lc_design->put_Name( GetLoadCombinationName(pgsTypes::lltDesign) );
   lc_design->put_LoadCombinationType(lctUserDefined);  // this way no one can tweak the load modifiers for this case
   lc_design->put_LiveLoadFactor(1.00);
   lc_design->AddLiveLoadModel(lltDesign);

   lc_design->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00) ;

   loadcombos->Add(lc_design);

   // fatigue truck
   CComPtr<ILoadCombination> lc_fatigue;
   lc_fatigue.CoCreateInstance(CLSID_LoadCombination);
   lc_fatigue->put_Name( GetLoadCombinationName(pgsTypes::lltFatigue) );
   lc_fatigue->put_LoadCombinationType(lctFatigue);
   lc_fatigue->put_LiveLoadFactor(1.0);
   lc_fatigue->AddLiveLoadModel(lltFatigue);

   hr = lc_fatigue->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);

   loadcombos->Add(lc_fatigue);

   // Permit load combination... 
   CComPtr<ILoadCombination> lc_permit;
   lc_permit.CoCreateInstance(CLSID_LoadCombination);
   lc_permit->put_Name( GetLoadCombinationName(pgsTypes::lltPermit) );
   lc_permit->put_LoadCombinationType(lctPermit);
   lc_permit->put_LiveLoadFactor(1.00);
   lc_permit->AddLiveLoadModel(lltPermit);

   lc_permit->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00) ;

   loadcombos->Add(lc_permit);

   // pedestrian live load
   CComPtr<ILoadCombination> lc_pedestrian;
   lc_pedestrian.CoCreateInstance(CLSID_LoadCombination);
   lc_pedestrian->put_Name( GetLoadCombinationName(pgsTypes::lltPedestrian) );
   lc_pedestrian->put_LoadCombinationType(lctUserDefined);
   lc_pedestrian->put_LiveLoadFactor(1.00);
   lc_pedestrian->AddLiveLoadModel(lltPedestrian);

   lc_pedestrian->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00) ;

   loadcombos->Add(lc_pedestrian);

   // legal - routine commercial traffic
   CComPtr<ILoadCombination> lc_legal_routine;
   lc_legal_routine.CoCreateInstance(CLSID_LoadCombination);
   lc_legal_routine->put_Name( GetLoadCombinationName(pgsTypes::lltLegalRating_Routine) );
   lc_legal_routine->put_LoadCombinationType(lctStrength);
   lc_legal_routine->put_LiveLoadFactor(1.0);
   lc_legal_routine->AddLiveLoadModel(lltLegalRoutineRating);
   hr = lc_legal_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_legal_routine);

   // legal - specialized hauling vehicle
   CComPtr<ILoadCombination> lc_legal_special;
   lc_legal_special.CoCreateInstance(CLSID_LoadCombination);
   lc_legal_special->put_Name( GetLoadCombinationName(pgsTypes::lltLegalRating_Special) );
   lc_legal_special->put_LoadCombinationType(lctStrength);
   lc_legal_special->put_LiveLoadFactor(1.0);
   lc_legal_special->AddLiveLoadModel(lltLegalSpecialRating);
   hr = lc_legal_special->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_legal_special);

   // permit rating - routine
   CComPtr<ILoadCombination> lc_permit_routine;
   lc_permit_routine.CoCreateInstance(CLSID_LoadCombination);
   lc_permit_routine->put_Name( GetLoadCombinationName(pgsTypes::lltPermitRating_Routine) );
   lc_permit_routine->put_LoadCombinationType(lctFatigue);
   lc_permit_routine->put_LiveLoadFactor(1.0);
   lc_permit_routine->AddLiveLoadModel(lltPermitRoutineRating);
   hr = lc_permit_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_permit_routine);

   // permit rating - special
   CComPtr<ILoadCombination> lc_permit_special;
   lc_permit_special.CoCreateInstance(CLSID_LoadCombination);
   lc_permit_special->put_Name( GetLoadCombinationName(pgsTypes::lltPermitRating_Special) );
   lc_permit_special->put_LoadCombinationType(lctStrength);
   lc_permit_special->put_LiveLoadFactor(1.0);
   lc_permit_special->AddLiveLoadModel(lltPermitSpecialRating);
   hr = lc_permit_special->AddLoadCaseFactor(CComBSTR("LL_IM"), 1.00, 1.00);
   loadcombos->Add(lc_permit_special);

   // Now we have to map our product loads (girder, diaphragms, etc) to the LRFD load cases (DC, DW, etc)
   CComPtr<ILoadCases> load_cases;
   pModel->get_LoadCases(&load_cases);

   CComPtr<ILoadCase> load_case_dc;
   load_cases->Find(CComBSTR("DC"),&load_case_dc);
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftGirder));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftConstruction));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftSlab));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftSlabPad));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftSlabPanel));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftDiaphragm));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftSidewalk));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftTrafficBarrier));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftShearKey));
   load_case_dc->AddLoadGroup(GetLoadGroupName(pftUserDC));

   GET_IFACE(IBridge,pBridge);
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   CComPtr<ILoadCase> load_case_dw;
   load_cases->Find(CComBSTR("DW"),&load_case_dw);
   load_case_dw->AddLoadGroup(GetLoadGroupName(pftOverlay));
   load_case_dw->AddLoadGroup(GetLoadGroupName(pftUserDW));

   CComPtr<ILoadCase> load_case_dwp;
   load_cases->Find(CComBSTR("DWp"),&load_case_dwp);
   load_case_dwp->AddLoadGroup(GetLoadGroupName(pftUserDW));
   if ( !bFutureOverlay )
      load_case_dwp->AddLoadGroup(GetLoadGroupName(pftOverlay));

   CComPtr<ILoadCase> load_case_dwf;
   load_cases->Find(CComBSTR("DWf"),&load_case_dwf);
   if ( bFutureOverlay )
      load_case_dwf->AddLoadGroup(GetLoadGroupName(pftOverlay));

   CComPtr<ILoadCase> load_case_dw_rating;
   load_cases->Find(CComBSTR("DW_Rating"),&load_case_dw_rating);
   if ( !bFutureOverlay )
   {
      load_case_dw_rating->AddLoadGroup(GetLoadGroupName(pftOverlayRating));
   }
   load_case_dw_rating->AddLoadGroup(GetLoadGroupName(pftUserDW));

   CComPtr<ILoadCase> load_case_ll;
   load_cases->Find(CComBSTR("LL_IM"),&load_case_ll);
   load_case_ll->AddLoadGroup(GetLoadGroupName(pftUserLLIM));

   CComPtr<ILoadCase> load_case_ps;
   load_cases->Find(CComBSTR("PS"),&load_case_ps);
   load_case_ps->AddLoadGroup(GetLoadGroupName(pftSecondaryEffects));

   // set up load groups
   CComPtr<ILoadGroups> loadGroups;
   pModel->get_LoadGroups(&loadGroups);
   loadGroups->Clear();
   AddLoadGroup(loadGroups, GetLoadGroupName(pftGirder),         CComBSTR("Girder self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftConstruction),   CComBSTR("Construction"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftSlab),           CComBSTR("Slab self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftSlabPad),        CComBSTR("Slab Pad self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftSlabPanel),      CComBSTR("Slab Panel self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftDiaphragm),      CComBSTR("Diaphragm self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftSidewalk),       CComBSTR("Sidewalk self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftTrafficBarrier), CComBSTR("Traffic Barrier self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftShearKey),       CComBSTR("Shear Key Weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftTotalPostTensioning),CComBSTR("Total Post-Tensioning"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftSecondaryEffects),CComBSTR("PT Secondary Effects"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftOverlay),        CComBSTR("Overlay self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftOverlayRating),  CComBSTR("Overlay self weight (rating)"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftUserDC),         CComBSTR("User applied loads in DC"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftUserDW),         CComBSTR("User applied loads in DW"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftUserLLIM),       CComBSTR("User applied live load"));

#pragma Reminder("UPDATE: this is a hack to get around creating load groups on the fly")
   // Ideally these load groups would be created on the fly... there is even an interface available
   // for this. When loads are created through that interface, the program crashes... 
   // Create the loads here is a work around.
   CComPtr<ILoadCase> load_case_cr;
   load_cases->Find(CComBSTR("CR"),&load_case_cr);
   CComPtr<ILoadCase> load_case_sr;
   load_cases->Find(CComBSTR("SH"),&load_case_sr);
   //CComPtr<ILoadCase> load_case_ps;
   //load_cases->Find(CComBSTR("PS"),&load_case_ps);
   load_case_cr->AddLoadGroup(GetLoadGroupName(pftCreep));
   load_case_sr->AddLoadGroup(GetLoadGroupName(pftShrinkage));
   load_case_ps->AddLoadGroup(GetLoadGroupName(pftRelaxation));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftCreep),CComBSTR("Creep"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftShrinkage),CComBSTR("Shrinkage"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftRelaxation),CComBSTR("Relaxation"));

}

void CAnalysisAgentImp::ApplyDiaphragmLoadsAtPiers(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdr)
{
   GET_IFACE(IGirder,            pGdr);
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   GET_IFACE(IPointOfInterest,pPoi);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pftDiaphragm) ); 

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      const CGirderKey& girderKey(pGirder->GetGirderKey());

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey); // diaphragms are cast with the deck
      CComBSTR bstrStage( GetLBAMStageName(castDeckIntervalIdx) );

      // iterate over all the piers in the group
      const CPierData2* pStartPier = pGroup->GetPier(pgsTypes::metStart);
      const CPierData2* pEndPier   = pGroup->GetPier(pgsTypes::metEnd);
      for ( const CPierData2* pPier = pStartPier; // start
            pPier != NULL && pPier->GetIndex() <= pEndPier->GetIndex(); // condition
            pPier = (pPier->GetNextSpan() ? pPier->GetNextSpan()->GetNextPier() : NULL) // increment
          )
      {
         if ( pPier == pStartPier )
         {
            // only add the load on the ahead side of the pier
            std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,0),POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND));
            ATLASSERT(vPoi.size() == 1);
            pgsPointOfInterest poi(vPoi.front());

            Float64 Pback,  Mback;  // load on back side of pier
            Float64 Pahead, Mahead; // load on ahead side of pier
            GetPierDiaphragmLoads( poi, pPier->GetIndex(), &Pback, &Mback, &Pahead, &Mahead);

            const CSegmentKey& segmentKey(poi.GetSegmentKey());
            Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);

            MemberIDType mbrID = GetSuperstructureMemberID(segmentKey);
            if ( !IsZero(start_end_dist) )
               mbrID++;

            CComPtr<IPointLoad> load;
            load.CoCreateInstance(CLSID_PointLoad);
            load->put_MemberType(mtSuperstructureMember);
            load->put_MemberID(mbrID);
            load->put_Location(poi.GetDistFromStart() - start_end_dist);
            load->put_Fy(Pahead);
            load->put_Mz(Mahead);

            CComPtr<IPointLoadItem> ptLoadItem;
            pointLoads->Add(bstrStage,bstrLoadGroup,load,&ptLoadItem);

            SaveOverhangPointLoads(CSegmentKey(girderKey,0),analysisType,bstrStage,bstrLoadGroup,Pahead,Pback);
         }
         else if ( pPier == pEndPier )
         {
            // only add the load on the back side of the pier
            std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,nSegments-1),POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
            ATLASSERT(vPoi.size() == 1);
            pgsPointOfInterest poi(vPoi.front());

            const CSegmentKey& segmentKey(poi.GetSegmentKey());
            Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);

            MemberIDType mbrID = GetSuperstructureMemberID(segmentKey);
            if ( !IsZero(start_end_dist) )
               mbrID++;

            Float64 Pback,  Mback;  // load on back side of pier
            Float64 Pahead, Mahead; // load on ahead side of pier
            GetPierDiaphragmLoads( poi, pPier->GetIndex(), &Pback, &Mback, &Pahead, &Mahead);

            CComPtr<IPointLoad> load;
            load.CoCreateInstance(CLSID_PointLoad);
            load->put_MemberType(mtSuperstructureMember);
            load->put_MemberID(mbrID);
            load->put_Location(poi.GetDistFromStart() - start_end_dist);
            load->put_Fy(Pback);
            load->put_Mz(Mback);

            CComPtr<IPointLoadItem> ptLoadItem;
            pointLoads->Add(bstrStage,bstrLoadGroup,load,&ptLoadItem);

            SaveOverhangPointLoads(CSegmentKey(girderKey,nSegments-1),analysisType,bstrStage,bstrLoadGroup,Pahead,Pback);
         }
         else
         {
            // This pier is interior to the group... this only happens with spliced girder bridges
            ATLASSERT(pPier->IsInteriorPier());

            CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pPier->GetIndex(),girderKey);
            Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);

            // get superstructure member ID where the segment starts
            MemberIDType mbrID = GetSuperstructureMemberID(segmentKey);

            // if there is an overhang at the start, the mbrID for the main portion
            // of the segment is one more... increment mbrID
            if ( !IsZero(start_end_dist) )
               mbrID++;

            // get location where CL-Segment intersects CL-Pier (could be betweens ends of segment or after end of segment)
            CComPtr<IPoint2d> pntSegPierIntersection;
            bool bIntersect = pBridge->GetSegmentPierIntersection(segmentKey,pPier->GetIndex(),&pntSegPierIntersection);
            ATLASSERT(bIntersect == true);

            // get the distance from the the start face of the segment to the intersection point
            // with the CL pier.
            CComPtr<IPoint2d> pntSupport[2],pntEnd[2],pntBrg[2];
            pGdr->GetSegmentEndPoints(segmentKey,
                                      &pntSupport[pgsTypes::metStart],&pntEnd[pgsTypes::metStart],&pntBrg[pgsTypes::Start],
                                      &pntBrg[pgsTypes::metEnd],      &pntEnd[pgsTypes::metEnd],  &pntSupport[pgsTypes::metEnd]);


            Float64 dist_along_segment;
            pntEnd[pgsTypes::metStart]->DistanceEx(pntSegPierIntersection,&dist_along_segment);

            pgsPointOfInterest poi = pPoi->GetPointOfInterest(girderKey,pPier->GetIndex());

            Float64 Pback,  Mback;  // load on back side of pier
            Float64 Pahead, Mahead; // load on ahead side of pier
            GetPierDiaphragmLoads( poi, pPier->GetIndex(), &Pback, &Mback, &Pahead, &Mahead);

            if ( pPier->GetSegmentConnectionType() == pgsTypes::psctContinuousSegment ||
                 pPier->GetSegmentConnectionType() == pgsTypes::psctIntegralSegment) 
            {
               // there should not be any moment at intermediate piers with continuous segments
               ATLASSERT(IsZero(Mback));
               ATLASSERT(IsZero(Mahead));

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
               load->put_Fy(Pback + Pahead);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,bstrLoadGroup,load,&ptLoadItem);
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

               MemberIDType mbrID = GetSuperstructureMemberID(segmentKey);
               if ( !IsZero(start_end_dist) )
                  mbrID++;

               CComPtr<IPointLoad> backLoad;
               backLoad.CoCreateInstance(CLSID_PointLoad);
               backLoad->put_MemberType(mtSuperstructureMember);
               backLoad->put_MemberID(mbrID);
               backLoad->put_Location(dist_along_segment - start_end_dist - back_bearing_offset);
               backLoad->put_Fy(Pback);
               backLoad->put_Mz(Mback);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,bstrLoadGroup,backLoad,&ptLoadItem);

               mbrID = GetSuperstructureMemberID(nextSegmentKey);
               if ( !IsZero(next_start_end_dist) )
                  mbrID++;

               CComPtr<IPointLoad> aheadLoad;
               aheadLoad.CoCreateInstance(CLSID_PointLoad);
               aheadLoad->put_MemberType(mtSuperstructureMember);
               aheadLoad->put_MemberID(mbrID);
               aheadLoad->put_Location(ahead_bearing_offset-next_start_end_dist);
               aheadLoad->put_Fy(Pahead);
               aheadLoad->put_Mz(Mahead);

               ptLoadItem.Release();
               pointLoads->Add(bstrStage,bstrLoadGroup,aheadLoad,&ptLoadItem);
            }

            SaveOverhangPointLoads(CSegmentKey(girderKey,0),analysisType,bstrStage,bstrLoadGroup,Pahead,Pback);
         }
      } // next pier
   } // next group
}

void CAnalysisAgentImp::ApplyIntermediateDiaphragmLoads( ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdr)
{
   GET_IFACE(IGirder,            pGdr);
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IIntervals,pIntervals);

   CComBSTR bstrLoadGroup( GetLoadGroupName(pftDiaphragm) ); 

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      CGirderKey girderKey(pGirder->GetGirderKey());

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey); // diaphragms are cast with the deck
      CComBSTR bstrStage( GetLBAMStageName(castDeckIntervalIdx) );

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         Float64 start_offset     = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 end_offset       = pBridge->GetSegmentEndEndDistance(segmentKey);
         Float64 segment_length   = pBridge->GetSegmentLength(segmentKey);

         MemberIDType mbrID = GetSuperstructureMemberID(segmentKey);
         if ( !IsZero(start_offset) )
            mbrID++;

         bool bModelStartCantilever,bModelEndCantilever;
         pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

         for ( int diaType = 0; diaType < 2; diaType++ )
         {
            CComBSTR bstrStage;
            CComBSTR bstrLoadGroup;
            pgsTypes::DiaphragmType diaphragmType;

            if (diaType == 0)
            {
               // diaphragms constructed as part of the girder
               // the load is applied to the LBAM when the segments are erected
               diaphragmType = pgsTypes::dtPrecast;
               IntervalIndexType intervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
               bstrStage = GetLBAMStageName(intervalIdx);
               bstrLoadGroup =  GetLoadGroupName(pftGirder);
            }
            else
            {
               // diaphragms constructed at the bridge site become part of the Diaphragm loading
               diaphragmType = pgsTypes::dtCastInPlace;
               IntervalIndexType intervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
               bstrStage = GetLBAMStageName(intervalIdx);
               bstrLoadGroup = GetLoadGroupName(pftDiaphragm);
            }

            std::vector<DiaphragmLoad> loads;
            GetIntermediateDiaphragmLoads(diaphragmType, segmentKey, &loads);

            std::vector<DiaphragmLoad>::iterator iter(loads.begin());
            std::vector<DiaphragmLoad>::iterator end(loads.end());
            for ( ; iter != end; iter++ )
            {
               DiaphragmLoad& rload = *iter;

               Float64 P   = rload.Load;
               Float64 loc = rload.Loc;

               if ( loc < start_offset )
               {
                  if ( bModelStartCantilever )
                  {
                     // Load occurs before CL bearing so it goes on the previous superstructre member
                     ATLASSERT(mbrID != 0);
                     mbrID--;
                  }
                  else
                  {
                     // Load occurs before CL bearing and the cantilever is not being modeled.
                     // Put the load directly over the bearing so it produces only a reaction.
                     loc = 0.0;
                  }
               }
               else if ( segment_length-end_offset < loc && !bModelEndCantilever )
               {
                  if ( bModelEndCantilever )
                  {
                     // Load goes on the next superstructure member
                     mbrID++;
                     loc -= (segment_length - end_offset);
                  }
                  else
                  {
                     // Load is after the CL Bearing at the end of the girder and the cantilever is not being modeled.
                     // Put the load directly over the bearing so it produces only a reaction.
                     loc = segment_length - start_offset - end_offset;
                  }
               }
               else
               {
                  // Load is on the main portion of the segment. Adjust the location
                  // so it is measured from the start of the superstructure member
                  loc -= start_offset;
               }

               CComPtr<IPointLoad> load;
               load.CoCreateInstance(CLSID_PointLoad);
               load->put_MemberType(mtSuperstructureMember);
               load->put_MemberID(mbrID);
               load->put_Location(loc);
               load->put_Fy(P);

               CComPtr<IPointLoadItem> item;
               pointLoads->Add(bstrStage , bstrLoadGroup, load, &item);
            } // next load
         } // next diaphragm type
      } // next segment
   } // next group
}


void CreateSuperstructureMember(Float64 length,const std::vector<SuperstructureMemberData>& vData,ISuperstructureMember** ppMbr)
{
   CComPtr<ISuperstructureMember> ssm;
   ssm.CoCreateInstance(CLSID_SuperstructureMember);

   ssm->put_Length(length);

   std::vector<SuperstructureMemberData>::const_iterator iter;
   for ( iter = vData.begin(); iter != vData.end(); iter++ )
   {
      const SuperstructureMemberData& data = *iter;

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

void CAnalysisAgentImp::SetBoundaryConditions(bool bContinuous,const CTimelineManager* pTimelineMgr,GroupIndexType grpIdx,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,ISuperstructureMember* pSSMbr)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);

   CSegmentKey segmentKey(pSegment->GetSegmentKey());

   // Determine boundary conditions at end of segment
   const CClosureJointData* pClosure = (endType == pgsTypes::metStart ? pSegment->GetLeftClosure() : pSegment->GetRightClosure());
   const CPierData2* pPier = NULL;
   const CTemporarySupportData* pTS = NULL;
   if ( pClosure )
   {
      pPier = pClosure->GetPier();
      pTS   = pClosure->GetTemporarySupport();
   }
   else
   {
      pPier = (endType == pgsTypes::metStart ? pSegment->GetSpan(pgsTypes::metStart)->GetPrevPier() : pSegment->GetSpan(pgsTypes::metEnd)->GetNextPier());
   }

   if ( pTS )
   {
      // Boundary condition should not be continuous segment at the end of a segment
      // it can only be closure joint which is considered to be a hinge until the
      // stage after it has been cast
      ATLASSERT( pTS->GetConnectionType() == pgsTypes::sctClosureJoint );

      // if the temporary support is at an erection tower -OR-
      // if temporary support is at a strong back and the segment is a "drop in"
      // set the member end release
      //
      // "drop in" segments start and end in the same span (pier segments straddle a pier),
      // don't have any interior supports, and are supported by strong backs at each end
      bool bIsDropIn = pSegment->IsDropIn();

      if ( pTS->GetSupportType() == pgsTypes::ErectionTower || bIsDropIn )
      {
         IDType closureID = pClosure->GetID();
         CClosureKey closureKey(pClosure->GetClosureKey());

         IntervalIndexType closureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
         CComBSTR bstrContinuity( GetLBAMStageName(closureIntervalIdx) );
         pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, bstrContinuity, mrtPinned);
      }
   }
   else
   {
      if ( pPier->IsInteriorPier() )
      {
         ATLASSERT(bContinuous == true); // always a continuous model in this case

         IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
         pIntervals->GetContinuityInterval(segmentKey,pPier->GetIndex(),&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

         IntervalIndexType continuityIntervalIdx = (endType == pgsTypes::metStart ? rightContinuityIntervalIdx : leftContinuityIntervalIdx);

         CComBSTR bstrContinuity( GetLBAMStageName(continuityIntervalIdx) );
         pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, bstrContinuity, mrtPinned);
      }
      else
      {
         ATLASSERT(pPier->IsBoundaryPier());
         if ( bContinuous && pPier->IsContinuousConnection() )
         {
            // continuous at pier
            IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
            pIntervals->GetContinuityInterval(segmentKey,pPier->GetIndex(),&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);

            IntervalIndexType continuityIntervalIdx = (endType == pgsTypes::metStart ? rightContinuityIntervalIdx : leftContinuityIntervalIdx);

            CComBSTR bstrContinuity( GetLBAMStageName(continuityIntervalIdx) );
            pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, bstrContinuity, mrtPinned);
         }
         else
         {
            // not continuous at pier
            if ( 
                 (pPier->GetNextSpan() != NULL && endType == pgsTypes::metEnd)   // not the last segment and setting boundary condition at end
                 ||                                                              // -OR-
                 (pPier->GetPrevSpan() != NULL && endType == pgsTypes::metStart) // not the first segment and setting boundary condition at start
               )
            {
               pSSMbr->SetEndRelease(endType == pgsTypes::metStart ? ssLeft : ssRight, CComBSTR(""), mrtPinned);
            }
            // else
            // {
            //   don't add an end release if this is the start of the first segment or the end of the last segment
            // }
         }
      }
   }
}

CAnalysisAgentImp::ModelData* CAnalysisAgentImp::GetModelData(GirderIndexType gdrIdx)
{
   BuildBridgeSiteModel(gdrIdx); // builds or updates the model if necessary
   BridgeSiteModels::iterator found = m_pBridgeSiteModels->find(gdrIdx);
   ATLASSERT( found != m_pBridgeSiteModels->end() ); // should always find it!
   ModelData* pModelData = &(*found).second;
   return pModelData;
}

CAnalysisAgentImp::SegmentModelData* CAnalysisAgentImp::GetModelData(SegmentModels& models,const CSegmentKey& segmentKey)
{
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);
   SegmentModels::iterator found = models.find(segmentKey);
   if ( found == models.end() )
      return 0;

   SegmentModelData* pModelData = &(*found).second;

   return pModelData;
}

PoiIDType CAnalysisAgentImp::AddPointOfInterest(SegmentModels& models,const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   SegmentModelData* pModelData = GetModelData(models,segmentKey);

   Float64 loc = poi.GetDistFromStart();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISectionProperties,pSectProp);

   PoiIDType femID = pgsGirderModelFactory::AddPointOfInterest(pModelData->Model, poi);
   pModelData->PoiMap.AddMap( poi, femID );

   LOG("Adding POI " << femID << " at " << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << " ft");

   return femID;
}

PoiIDType CAnalysisAgentImp::AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi)
{
   PoiIDType femPoiID = pgsGirderModelFactory::AddPointOfInterest(models.Model,poi);
   models.PoiMap.AddMap( poi, femPoiID );
   return femPoiID;
}

PoiIDType CAnalysisAgentImp::AddPointOfInterest(ModelData* pModelData,const pgsPointOfInterest& poi)
{
   // maps a PGSuper poi into, and creates, an LBAM poi
   ATLASSERT(pModelData->m_Model != NULL || pModelData->m_ContinuousModel != NULL);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   MemberIDType mbrID = GetSuperstructureMemberID(segmentKey);

   PoiIDType poiID = (m_NextPoi)++;

   GET_IFACE(IBridge,pBridge);

   Float64 location       = poi.GetDistFromStart();
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_dist     = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_dist       = pBridge->GetSegmentEndEndDistance(segmentKey);

   // Closure POI's and POIs at CL Piers that are between groups are actually beyond the end of the segment
   // adjust the member ID and the location
   if ( poi.HasAttribute(POI_CLOSURE) || poi.HasAttribute(POI_BOUNDARY_PIER) )
   {
      if ( poi.GetDistFromStart() < 0 )
      {
         if ( !IsZero(start_dist) )
            mbrID--;

         location = 0.0;
      }
      else
      {
         IndexType nSSMbrs = GetSuperstructureMemberCount(segmentKey);
         mbrID += (MemberIDType)nSSMbrs;
         location = poi.GetDistFromStart() - segment_length;
      }
   }
   else if ( location < start_dist && !IsZero(start_dist) )
   {
      // POI is before the starting CL Bearing point and there is an overhang member
      // no adjustments needed
   }
   else if ( segment_length-end_dist < location && !IsZero(end_dist) )
   {
      // POI is after the ending CL Bearing point
      // This moves the POI onto the next superstructure member
      if ( !IsZero(start_dist) )
         mbrID++; // move to the SSMBR for the main portion of the segment

      mbrID++; // move to the next superstructure member (the overhang member for the right end of the segment)
      location -= (segment_length - end_dist);
   }
   else
   {
      // POI is between CL Bearing points... adjust the location
      // so that it is measured from the CL Bearing at the left end of the segment
      if ( !IsZero(start_dist) )
         mbrID++;

      location -= start_dist;
   }

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
      pModelData->m_Model->get_Stages(&stages);
   else
      pModelData->m_ContinuousModel->get_Stages(&stages);

   CComPtr<IEnumStage> enumStages;
   stages->get__EnumElements(&enumStages);

   CComPtr<IStage> stage;
   while ( enumStages->Next(1,&stage,NULL) != S_FALSE )
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
      pModelData->m_Model->get_SuperstructureMembers(&ssmbrs);
   else
      pModelData->m_ContinuousModel->get_SuperstructureMembers(&ssmbrs);

   CComPtr<ISuperstructureMember> ssmbr;
   ssmbrs->get_Item(mbrID,&ssmbr);

   Float64 Lssmbr; // length of superstructure member
   ssmbr->get_Length(&Lssmbr);
   ATLASSERT(IsGE(0.0,location) && IsLE(location,Lssmbr));
   }
#endif

   return poiID;
}


void CAnalysisAgentImp::AddPoiStressPoints(const pgsPointOfInterest& poi,IStage* pStage,IPOIStressPoints* pPOIStressPoints)
{
   GET_IFACE(ISectionProperties,pSectProp);

   CComBSTR bstrStage;
   pStage->get_Name(&bstrStage);
   IntervalIndexType intervalIdx = GetIntervalFromLBAMStageName(bstrStage);

   CComPtr<IStressPoints> leftStressPoints;
   leftStressPoints.CoCreateInstance(CLSID_StressPoints);

   CComPtr<IStressPoints> rightStressPoints;
   rightStressPoints.CoCreateInstance(CLSID_StressPoints);

   Float64 Ag = pSectProp->GetAg(intervalIdx,poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(poi.GetSegmentKey());

   GET_IFACE(IBridge,pBridge);
   bool bIsCompositeDeck = pBridge->IsCompositeDeck();

   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      Float64 S = pSectProp->GetS(intervalIdx,poi,stressLocation);

      Float64 Sa = IsZero(Ag) ? 0 : 1/Ag;
      Float64 Sm = IsZero(S)  ? 0 : 1/S;

      if ( stressLocation == pgsTypes::TopDeck || stressLocation == pgsTypes::BottomDeck )
      {
         if (  !bIsCompositeDeck || intervalIdx < compositeDeckIntervalIdx )
         {
            // if the deck is not composite or this is an interval before 
            // the composite deck becomes composite, the deck can't take any
            // load so use zero values to stresses come out zero.
            Sa = 0;
            Sm = 0;
         }
      }


      CComPtr<IStressPoint> stressPoint;
      stressPoint.CoCreateInstance(CLSID_StressPoint);

      stressPoint->put_Sa(Sa);
      stressPoint->put_Sm(Sm);

      leftStressPoints->Add(stressPoint);
      rightStressPoints->Add(stressPoint);
   }

   pPOIStressPoints->Insert(bstrStage,leftStressPoints,rightStressPoints);
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces
//
pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization)
{
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple     ? pgsTypes::SimpleSpan : 
                                       analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : 
                                       optimization == pgsTypes::Maximize ? pgsTypes::MaxSimpleContinuousEnvelope : pgsTypes::MinSimpleContinuousEnvelope);
   return bat;
}

pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::OptimizationType optimization)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   return GetBridgeAnalysisType(analysisType,optimization);
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results = GetShear(intervalIdx,pfType,vPoi,bat,comboType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetMoment(intervalIdx,pfType,vPoi,bat,comboType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetDeflection(intervalIdx,pfType,vPoi,bat,comboType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetRotation(intervalIdx,pfType,vPoi,bat,comboType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   USES_CONVERSION;
   
   std::_tostringstream os;

   GET_IFACE(IEventMap,pEventMap);

#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,0));
   ATLASSERT(intervalIdx != releaseIntervalIdx);
#endif

   try
   {
      // Start by checking if the model exists
      ModelData* pModelData = 0;
      pModelData = GetModelData(girderKey.girderIndex);

      bool bSecondaryEffects = pfType == pftSecondaryEffects;
      bool bPrimaryPT = pfType == pftPrimaryPostTensioning;
      if ( pfType == pftSecondaryEffects || pfType == pftPrimaryPostTensioning )
         pfType = pftTotalPostTensioning; // secondary reactions are the reactions due to post-tensioning

      CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
      CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
      
      ConfigureLBAMPoisForReactions(girderKey,pierIdx,intervalIdx,bat,false);

      CComPtr<IResult3Ds> results;

      ResultsSummationType resultsSummation = (comboType == ctCumulative ? rsCumulative : rsIncremental);

      if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
         pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
      else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
         pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);
      else
         pModelData->pLoadGroupResponse[bat]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&results);


      Float64 Fy = 0;
      CollectionIndexType nResults;
      results->get_Count(&nResults);
      for ( CollectionIndexType i = 0; i < nResults; i++ )
      {
         CComPtr<IResult3D> result;
         results->get_Item(i,&result);

         Float64 fy;
         result->get_Y(&fy);

         Fy += fy;
      }

      if ( bPrimaryPT )
      {
         // Primary PT doesn't cause any reaction (its is just axial and bending)
         Fy = 0;
      }


      return Fy;
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,pfType,vPoi,bat,comboType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

CComBSTR CAnalysisAgentImp::GetProductLoadName(ProductForceType pfType)
{
   if ( pfType <= pftShearKey )
   {
      return GetLoadGroupName(pfType);
   }
   else
   {
      switch(pfType)
      {
      case pftPrestress:
         return CComBSTR(_T("Prestress"));
      case pftTotalPostTensioning:
         return CComBSTR(_T("PostTension"));
      case pftCreep:
         return CComBSTR(_T("Creep"));
      case pftShrinkage:
         return CComBSTR(_T("Shrinkage"));
      case pftRelaxation:
         return CComBSTR(_T("Relaxation"));
      }
   }

   return CComBSTR(_T(""));
}

void CAnalysisAgentImp::GetGirderSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // get all the cross section changes
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> xsPOI( pPOI->GetPointsOfInterest(segmentKey,POI_SECTCHANGE,POIFIND_OR) );
   ATLASSERT(2 <= xsPOI.size());

   GET_IFACE(IMaterials,pMaterial);
   Float64 density = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);
   Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();

#pragma Reminder("UPDATE: this loading doesn't work for abrupt section changes")
      // for box beam and voided slabs with end blocks, this loading doesn't work quite right
      // because we don't have left/right face section properties. There is an abrupt change
      // of section at the end of solid end blocks

   // compute distributed load intensity at each section change
   GET_IFACE(ISectionProperties,pSectProp);
   std::vector<pgsPointOfInterest>::iterator iter( xsPOI.begin() );
   std::vector<pgsPointOfInterest>::iterator end( xsPOI.end() );
   pgsPointOfInterest prevPoi = *iter++;
   Float64 Ag_Prev = pSectProp->GetAg(pgsTypes::sptGross,intervalIdx,prevPoi);
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& currPoi = *iter;
      Float64 Ag_Curr = pSectProp->GetAg(pgsTypes::sptGross,intervalIdx,currPoi);

      GirderLoad load;
      load.StartLoc = prevPoi.GetDistFromStart();
      load.EndLoc   = currPoi.GetDistFromStart();
      load.wStart   = -Ag_Prev*density*g;
      load.wEnd     = -Ag_Curr*density*g;

      pDistLoad->push_back(load);

      prevPoi = currPoi;
      Ag_Prev = Ag_Curr;
   }

   // get point loads for precast diaphragms
   GetIntermediateDiaphragmLoads(pgsTypes::dtPrecast,segmentKey,pPointLoad);
}

Float64 CAnalysisAgentImp::GetTrafficBarrierLoad(const CSegmentKey& segmentKey)
{
   ValidateAnalysisModels(segmentKey.girderIndex);

   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() ); // it should be found

   return (*found).second.m_BarrierLoad;
}

Float64 CAnalysisAgentImp::GetSidewalkLoad(const CSegmentKey& segmentKey)
{
   ValidateAnalysisModels(segmentKey.girderIndex);

   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(segmentKey);
   ATLASSERT( found != m_SidewalkTrafficBarrierLoads.end() ); // it should be found

   return (*found).second.m_SidewalkLoad;
}

void CAnalysisAgentImp::GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads)
{
   ValidateAnalysisModels(segmentKey.girderIndex);

   GetMainSpanOverlayLoad(segmentKey,pOverlayLoads);
}

void CAnalysisAgentImp::GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads)
{
   ValidateAnalysisModels(segmentKey.girderIndex);
   GetMainConstructionLoad(segmentKey,pConstructionLoads);
}

bool CAnalysisAgentImp::HasShearKeyLoad(const CGirderKey& girderKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IGirder,pGirder);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   // First check if this beam has a shear key
   if ( pGirder->HasShearKey(girderKey, spacingType))
   {
      return true;
   }

   // Next check adjacent beams if we have a continous analysis
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
         GirderIndexType nGirdersInGroup = pBridge->GetGirderCount(girderKey.groupIndex);
         GirderIndexType gdrIdx = Min(nGirdersInGroup-1,girderKey.girderIndex);

         if (pGirder->HasShearKey( CGirderKey(grpIdx,gdrIdx), spacingType))
            return true;
      }
   }

   return false;
}

void CAnalysisAgentImp::GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads)
{
   ValidateAnalysisModels(segmentKey.girderIndex);
   GetMainSpanShearKeyLoad(segmentKey,pLoads);
}

bool CAnalysisAgentImp::HasPedestrianLoad()
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
      return false;

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
      return false; // sidewalks too narrow for PL

   return true;
}

bool CAnalysisAgentImp::HasSidewalkLoad(const CGirderKey& girderKey)
{
   bool bHasSidewalkLoad = false;

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      Float64 swLoad, fraLeft, fraRight;
      GetSidewalkLoadFraction(segmentKey,&swLoad,&fraLeft,&fraRight);

      if ( !IsZero(swLoad))
         return true;
   }

   return bHasSidewalkLoad;
}

bool CAnalysisAgentImp::HasPedestrianLoad(const CGirderKey& girderKey)
{
   //
   // NOTE: This code below is for distributing the pedestrian load the same way the sidewalk
   //       and traffic barrier dead loads are distributed
   //
   bool bHasPedLoad = HasPedestrianLoad();
   if ( !bHasPedLoad )
      return false; // there is no chance of having any Ped load on this bridge

   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      // any pedestrian load is good enough for this case
      bHasPedLoad = true;
   }
   else
   {
      bHasPedLoad = false;
      GET_IFACE(IBridge,pBridge);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         Float64 swLoad, fraLeft, fraRight;
         GetSidewalkLoadFraction(segmentKey, &swLoad, &fraLeft,&fraRight);

         if ( !IsZero(fraLeft) || !IsZero(fraRight) )
            return true; // there is load on one of the segments
      }
   }

   return bHasPedLoad;
}

Float64 CAnalysisAgentImp::GetPedestrianLoad(const CSegmentKey& segmentKey)
{
   return GetPedestrianLiveLoad(segmentKey.groupIndex,segmentKey.girderIndex);
}

Float64 CAnalysisAgentImp::GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation)
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
         return 0.0; // not min sidewalk, no pedestrian load

      Float64 w = pSpecEntry->GetPedestrianLiveLoad();
      Float64 W  = w*swWidth;

      return W;
   }
}

void CAnalysisAgentImp::GetSlabLoad(const CSegmentKey& segmentKey, std::vector<LinearLoad>& vSlabLoads, std::vector<LinearLoad>& vHaunchLoads, std::vector<LinearLoad>& vPanelLoads)
{
   // Create equivalent LinearLoad vectors from the SlabLoad information
   std::vector<SlabLoad> slabLoads;
   GetMainSpanSlabLoad(segmentKey,&slabLoads);
   std::vector<SlabLoad>::iterator iter1(slabLoads.begin());
   std::vector<SlabLoad>::iterator iter2(iter1+1);
   std::vector<SlabLoad>::iterator end(slabLoads.end());
   for ( ; iter2 != end; iter1++, iter2++ )
   {
      SlabLoad& prevLoad = *iter1;
      SlabLoad& nextLoad = *iter2;

      LinearLoad slabLoad;
      slabLoad.StartLoc = prevLoad.Loc;
      slabLoad.EndLoc   = nextLoad.Loc;
      slabLoad.wStart   = prevLoad.MainSlabLoad;
      slabLoad.wEnd     = nextLoad.MainSlabLoad;
      vSlabLoads.push_back(slabLoad);

      LinearLoad haunchLoad;
      haunchLoad.StartLoc = prevLoad.Loc;
      haunchLoad.EndLoc   = nextLoad.Loc;
      haunchLoad.wStart   = prevLoad.PadLoad;
      haunchLoad.wEnd     = nextLoad.PadLoad;
      vHaunchLoads.push_back(haunchLoad);

      LinearLoad panelLoad;
      panelLoad.StartLoc = prevLoad.Loc;
      panelLoad.EndLoc   = nextLoad.Loc;
      panelLoad.wStart   = prevLoad.PanelLoad;
      panelLoad.wEnd     = nextLoad.PanelLoad;
      vPanelLoads.push_back(panelLoad);
   } // next load
}

void CAnalysisAgentImp::GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads)
{
   ATLASSERT(segmentKey.groupIndex   != INVALID_INDEX);
   ATLASSERT(segmentKey.girderIndex  != INVALID_INDEX);
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);

   ATLASSERT(pSlabLoads!=0);
   pSlabLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IShapes,pShapes);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   GirderIndexType gdrIdx = Min(segmentKey.girderIndex,nGirders-1);

   // get slab fillet (there should be a better way to do this)
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   // if there is no deck, there is no load
   if ( pDeck->DeckType == pgsTypes::sdtNone )
      return;

   Float64 fillet = pDeck->Fillet;

   Float64 panel_support_width = 0;
   if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
      panel_support_width = pDeck->PanelSupport;

   // Get some important POIs that we will be using later
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );
   CHECK(vPoi.size()!=0);

   bool bIsInteriorGirder = pBridge->IsInteriorGirder( segmentKey );

   // Slab pad load assumes no camber... that is, the top of the girder is flat
   // This is the worst case loading
   // Increased/Reduced pad depth due to Sag/Crest vertical curves is accounted for
   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType poiIdx = 0; poiIdx < nPOI; poiIdx++ )
   {
      Float64 wslab;
      Float64 wslab_panel;

      const pgsPointOfInterest& poi = vPoi[poiIdx];

      Float64 top_girder_to_top_slab = pSectProp->GetDistTopSlabToTopGirder(poi);
      Float64 slab_offset            = pBridge->GetSlabOffset(poi);
      Float64 cast_depth             = pBridge->GetCastSlabDepth(poi);
      Float64 panel_depth            = pBridge->GetPanelDepth(poi);
      Float64 trib_slab_width        = pSectProp->GetTributaryFlangeWidth(poi);

      if ( bIsInteriorGirder )
      {
         // Apply the load of the main slab
         wslab = trib_slab_width * cast_depth  * pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();

         // compute the width of the deck panels
         Float64 panel_width = trib_slab_width; // start with tributary width

         // deduct width of mating surfaces
         MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(segmentKey);
         for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMatingSurfaces; msIdx++ )
         {
            panel_width -= pGdr->GetMatingSurfaceWidth(poi,msIdx);
         }

         // add panel support widths
         panel_width += 2*nMatingSurfaces*panel_support_width;
         wslab_panel = panel_width * panel_depth * pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();
      }
      else
      {
         // Exterior girder... the slab overhang can be thickened so we have figure out the weight
         // on the left and right of the girder instead of using the tributary width and slab depth

         // determine depth of the slab at the edge and flange tip
         Float64 overhang_edge_depth = pDeck->OverhangEdgeDepth;
         Float64 overhang_depth_at_flange_tip;
         if ( pDeck->OverhangTaper == pgsTypes::dotNone )
         {
            // overhang is constant depth
            overhang_depth_at_flange_tip = overhang_edge_depth;
         }
         else if ( pDeck->OverhangTaper == pgsTypes::dotTopTopFlange )
         {
            // deck overhang tapers to the top of the top flange
            overhang_depth_at_flange_tip = slab_offset;
         }
         else if ( pDeck->OverhangTaper == pgsTypes::dotBottomTopFlange )
         {
            // deck overhang tapers to the bottom of the top flange
            FlangeIndexType nFlanges = pGdr->GetNumberOfTopFlanges(segmentKey);
            Float64 flange_thickness;
            if ( gdrIdx == 0 )
               flange_thickness = pGdr->GetTopFlangeThickness(poi,0);
            else
               flange_thickness = pGdr->GetTopFlangeThickness(poi,nFlanges-1);

            overhang_depth_at_flange_tip = slab_offset + flange_thickness;
         }
         else
         {
            ATLASSERT(false); // is there a new deck overhang taper???
         }

         // Determine the slab overhang
         Float64 station,offset;
         pBridge->GetStationAndOffset(poi,&station,&offset);
         Float64 dist_from_start_of_bridge = pBridge->GetDistanceFromStartOfBridge(station);

         // slab overhang from CL of girder (normal to alignment)
         Float64 slab_overhang = (gdrIdx == 0 ? pBridge->GetLeftSlabOverhang(dist_from_start_of_bridge) : pBridge->GetRightSlabOverhang(dist_from_start_of_bridge));

         if (slab_overhang < 0.0)
         {
            // negative overhang - girder probably has no slab over it
            slab_overhang = 0.0;
         }
         else
         {
            Float64 top_width = pGdr->GetTopWidth(poi);

            // slab overhang from edge of girder (normal to alignment)
            slab_overhang -= top_width/2;
         }

         // area of slab overhang
         Float64 slab_overhang_area = slab_overhang*(overhang_edge_depth + overhang_depth_at_flange_tip)/2;

         // Determine area of slab from exterior flange tip to 1/2 distance to interior girder
         Float64 w = trib_slab_width - slab_overhang;
         Float64 slab_area = w*cast_depth;
         wslab       = (slab_area + slab_overhang_area) * pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();

         Float64 panel_width = w;

         // deduct width of mating surfaces
         MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(segmentKey);
         for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMatingSurfaces; msIdx++ )
         {
            panel_width -= pGdr->GetMatingSurfaceWidth(poi,msIdx);
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

         wslab_panel = panel_width * panel_depth * pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();
      }

      ASSERT( 0 <= wslab );
      ASSERT( 0 <= wslab_panel );

      // slab pad load
      Float64 pad_hgt = top_girder_to_top_slab - cast_depth;
      if ( pad_hgt < 0 )
         pad_hgt = 0;

      // mating surface
      Float64 mating_surface_width = 0;
      MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(segmentKey);
      ATLASSERT( nMatingSurfaces == pGdr->GetNumberOfMatingSurfaces(segmentKey) );
      for ( MatingSurfaceIndexType matingSurfaceIdx = 0; matingSurfaceIdx < nMatingSurfaces; matingSurfaceIdx++ )
      {
         mating_surface_width += pGdr->GetMatingSurfaceWidth(poi,matingSurfaceIdx);
         mating_surface_width -= 2*panel_support_width;
      }

      if ( !bIsInteriorGirder )
      {
         /// if this is an exterior girder, add back one panel support width for the exterior side of the girder
         // becuase we took off one to many panel support widths in the loop above
         mating_surface_width += panel_support_width;
      }

      // calculate load, neglecting effect of fillet
      Float64 wpad = (pad_hgt*mating_surface_width + (pad_hgt - panel_depth)*(bIsInteriorGirder ? 2 : 1)*nMatingSurfaces*panel_support_width)*  pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();
      ASSERT( 0 <= wpad );

      LOG("Poi Loc at           = " << poi.GetDistFromStart());
      LOG("Main Slab Load       = " << wslab);
      LOG("Slab Pad Load        = " << wpad);

      SlabLoad sload;
      sload.Loc          = poi.GetDistFromStart();
      sload.MainSlabLoad = -wslab;  // + is upward
      sload.PanelLoad    = -wslab_panel;
      sload.PadLoad      = -wpad;

      if ( pSlabLoads->size() < 2 )
      {
         pSlabLoads->push_back(sload);
      }
      else
      {
         if ( IsEqual(pSlabLoads->back().MainSlabLoad,-wslab) && IsEqual(pSlabLoads->back().PanelLoad,-wslab_panel) && IsEqual(pSlabLoads->back().PadLoad,-wpad) )
         {
            pSlabLoads->pop_back();
            pSlabLoads->push_back(sload);
         }
         else
         {
            pSlabLoads->push_back(sload);
         }
      }
   }
}

void CAnalysisAgentImp::GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   ATLASSERT(segmentKey.groupIndex   != INVALID_INDEX);
   ATLASSERT(segmentKey.girderIndex  != INVALID_INDEX);
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);

   Float64 P1[3], P2[3];
   GetCantileverSlabLoads(segmentKey,&P1[0],&P2[0]);
   *pP1 = P1[0];
   *pP2 = P2[0];
   *pM1 = 0;
   *pM2 = 0;
}

void CAnalysisAgentImp::GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   Float64 P1[3], P2[3];
   GetCantileverSlabLoads(segmentKey,&P1[0],&P2[0]);
   *pP1 = P1[1];
   *pP2 = P2[1];
   *pM1 = 0;
   *pM2 = 0;
}

void CAnalysisAgentImp::GetCantileverSlabLoads(const CSegmentKey& segmentKey, Float64* pP1, Float64* pP2)
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

void CAnalysisAgentImp::GetMainSpanOverlayLoad(const CSegmentKey& segmentKey, std::vector<OverlayLoad>* pOverlayLoads)
{
   CHECK(pOverlayLoads!=0);
   pOverlayLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IPointOfInterest,pIPoi);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2*   pDeck       = pBridgeDesc->GetDeckDescription();

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   GirderIndexType gdrIdx   = Min(segmentKey.girderIndex,nGirders-1);

   CSegmentKey thisSegmentKey(segmentKey);
   thisSegmentKey.girderIndex = gdrIdx;

   Float64 OverlayWeight = pBridge->GetOverlayWeight();

   GET_IFACE( ISpecification, pSpec );
   pgsTypes::OverlayLoadDistributionType overlayDistribution = pSpec->GetOverlayLoadDistributionType();

   // POIs where overlay loads are laid out
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(thisSegmentKey) );
   CHECK(vPoi.size()!=0);

   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IGirder,pGirder);

   // Width of loaded area, and load intensity
   Float64 startWidth(0), endWidth(0);
   Float64 startW(0), endW(0);
   Float64 startLoc(0), endLoc(0);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_end_dist   = pBridge->GetSegmentEndEndDistance(segmentKey);
   
   startLoc = 0.0;

   std::vector<pgsPointOfInterest>::const_iterator begin(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      // poi at end of distributed load
      const pgsPointOfInterest& endPoi = *iter;
      endLoc = endPoi.GetDistFromStart();

      Float64 station,girder_offset;
      pBridge->GetStationAndOffset(endPoi,&station,&girder_offset);
      Float64 dist_from_start_of_bridge = pBridge->GetDistanceFromStartOfBridge(station);

      // Offsets to toe where overlay starts
      Float64 left_olay_offset  = pBridge->GetLeftOverlayToeOffset(dist_from_start_of_bridge);
      Float64 right_olay_offset = pBridge->GetRightOverlayToeOffset(dist_from_start_of_bridge);

      if (overlayDistribution == pgsTypes::olDistributeEvenly)
      {
         // This one is easy. girders see overlay even if they aren't under it
         // Total overlay width at location
         endWidth = right_olay_offset - left_olay_offset;
         endW = -endWidth*OverlayWeight/nGirders;
      }
      else if (overlayDistribution == pgsTypes::olDistributeTributaryWidth)
      {
         Float64 left_slab_offset  = pBridge->GetLeftSlabEdgeOffset(dist_from_start_of_bridge);

         // Have to determine how much of overlay is actually over the girder's tributary width
         // Measure distances from left edge of deck to Left/Right edges of overlay
         Float64 DLO = left_olay_offset - left_slab_offset;
         Float64 DRO = right_olay_offset - left_slab_offset;

         // Distance from left edge of deck to CL girder
         Float64 DGDR = girder_offset - left_slab_offset;

         // Next get distances from left edge of deck to girder's left and right tributary edges
         Float64 DLT, DRT;
         if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
         {
            ATLASSERT( ::IsJointSpacing(pBridgeDesc->GetGirderSpacingType()) );

            // Joint widths
            Float64 leftJ,rightJ;
            pBridge->GetDistanceBetweenGirders(endPoi,&leftJ,&rightJ);

            Float64 width = Max(pGirder->GetTopWidth(endPoi),pGirder->GetBottomWidth(endPoi));
            Float64 width2 = width/2.0;

            // Note that tributary width ignores joint spacing
            DLT = DGDR - width2 - leftJ/2.0;;
            DRT = DGDR + width2 + rightJ/2.0;
         }
         else
         {
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
         ATLASSERT(0); //something new?
      }

      // Create load and stuff it
      if (iter != begin)
      {
         OverlayLoad load;
         load.StartLoc = startLoc;
         load.EndLoc   = endLoc;
         load.StartWcc = startWidth;
         load.EndWcc   = endWidth;
         load.wStart   = startW;
         load.wEnd     = endW;

         if (pOverlayLoads->size() == 0 )
         {
            pOverlayLoads->push_back(load);
         }
         else
         {
            // if the end of the previous load is exactly the same as the end of this load, just move the end of the previous load
            // (we know the end of the previous load and the start of this load are the same)
            if ( IsEqual(pOverlayLoads->back().wEnd,endW) && IsEqual(pOverlayLoads->back().EndWcc,endWidth) )
            {
               pOverlayLoads->back().EndLoc = endLoc;
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

void CAnalysisAgentImp::GetMainConstructionLoad(const CSegmentKey& segmentKey, std::vector<ConstructionLoad>* pConstructionLoads)
{
   GirderIndexType gdr = segmentKey.girderIndex;

   ATLASSERT(pConstructionLoads != NULL);
   pConstructionLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IPointOfInterest,pIPoi);

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   GirderIndexType gdrIdx   = Min(gdr,nGirders-1);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2*   pDeck       = pBridgeDesc->GetDeckDescription();

   GET_IFACE(IUserDefinedLoadData,pLoads);
   Float64 construction_load = pLoads->GetConstructionLoad();

   // Get some important POIs that we will be using later
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );
   CHECK(vPoi.size()!=0);

   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IGirder,pGirder);

   // loop controllers are equivalent... the version that is used doesn't have to do
   // the subtraction operation every time the loop control is evaluated
   //int num_poi = vPoi.size();
   //for ( int i = 0; i < num_poi-1; i++ )
   IndexType num_poi = vPoi.size()-1;
   for ( IndexType i = 0; i < num_poi; i++ )
   {
      const pgsPointOfInterest& prevPoi = vPoi[i];
      const pgsPointOfInterest& currPoi = vPoi[i+1];

      // Width of loaded area, and load intensity
      Float64 startWidth, endWidth;
      Float64 startW, endW;

      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      {
         Float64 left,right;
         pBridge->GetDistanceBetweenGirders(prevPoi,&left,&right);

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
         startWidth = pSectProp->GetTributaryFlangeWidth(prevPoi);
         // negative width means that slab is not over girder
         if (startWidth < 0.0)
            startWidth = 0.0;

         startW = -startWidth*construction_load;

         endWidth = pSectProp->GetTributaryFlangeWidth(currPoi);
         if (endWidth < 0.0)
            endWidth = 0.0;

         endW = -endWidth*construction_load;
      }

      // Create load and stuff it
      ConstructionLoad load;
      load.StartLoc = prevPoi.GetDistFromStart();
      load.EndLoc   = currPoi.GetDistFromStart();
      load.StartWcc = startWidth;
      load.EndWcc   = endWidth;
      load.wStart   = startW;
      load.wEnd     = endW;

      if ( pConstructionLoads->size() == 0 )
      {
         pConstructionLoads->push_back(load);
      }
      else
      {
         if ( IsEqual(pConstructionLoads->back().EndWcc,load.EndWcc) && IsEqual(pConstructionLoads->back().wEnd,load.wEnd) )
         {
            pConstructionLoads->back().EndLoc = load.EndLoc;
         }
         else
         {
            pConstructionLoads->push_back(load);
         }
      }
   }
}

void CAnalysisAgentImp::GetMainSpanShearKeyLoad(const CSegmentKey& segmentKey, std::vector<ShearKeyLoad>* pLoads)
{
   GirderIndexType gdr = segmentKey.girderIndex;

   ATLASSERT(pLoads != NULL); 
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

   GirderIndexType gdrIdx = Min(gdr,nGirders-1);

   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   // areas of shear key per interior side
   Float64 unif_area, joint_area;
   pGirder->GetShearKeyAreas(segmentKey, spacingType, &unif_area, &joint_area);

   Float64 unit_weight;
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      unit_weight = pMaterial->GetSegmentWeightDensity(segmentKey,castDeckIntervalIdx)*unitSysUnitsMgr::GetGravitationalAcceleration();
   else
      unit_weight = pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx)*unitSysUnitsMgr::GetGravitationalAcceleration();

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
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );
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


void CAnalysisAgentImp::GetIntermediateDiaphragmLoads(pgsTypes::DiaphragmType diaphragmType,const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads)
{
   ATLASSERT(pLoads != NULL);
   pLoads->clear();

   GET_IFACE( IBridge,    pBridge   );
   GET_IFACE( IMaterials, pMaterial );
   GET_IFACE( IIntervals, pIntervals );

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

#pragma Reminder("BUG: diaphragm loads are defined by span, but using segments here")
   // the location of the load is going to be incorrect for splice girder bridges


   //GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
   //gdrIdx = Min(gdrIdx,nGirders-1);
   Float64 density;
   if ( diaphragmType == pgsTypes::dtPrecast )
   {
      density = pMaterial->GetSegmentWeightDensity(segmentKey,releaseIntervalIdx); // cast with girder, using girder concrete
   }
   else
   {
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
         density = pMaterial->GetSegmentWeightDensity(segmentKey,releaseIntervalIdx); // no deck, using girder concrete
      else
         density = pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx); // cast with slab, using slab concrete
   }

   Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();

   std::vector<IntermedateDiaphragm> diaphragms = pBridge->GetIntermediateDiaphragms(diaphragmType,segmentKey);
   std::vector<IntermedateDiaphragm>::iterator iter(diaphragms.begin());
   std::vector<IntermedateDiaphragm>::iterator end(diaphragms.end());
   for ( ; iter != end; iter++ )
   {
      IntermedateDiaphragm& diaphragm = *iter;

      Float64 P;
      if ( diaphragm.m_bCompute )
         P = diaphragm.H * diaphragm.T * diaphragm.W * density * g;
      else
         P = diaphragm.P;

      Float64 Loc = diaphragm.Location;

      DiaphragmLoad load;
      load.Loc = Loc;
      load.Load = -P;

      pLoads->push_back(load);
   }
}

void CAnalysisAgentImp::GetPierDiaphragmLoads( const pgsPointOfInterest& poi, PierIndexType pierIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead)
{
   GET_IFACE(IBridge,    pBridge );
   GET_IFACE(IMaterials, pMaterial);
   GET_IFACE(ISectionProperties, pSectProp);
   GET_IFACE(IGirder,    pGirder);
   GET_IFACE(IIntervals, pIntervals);

   *pPback  = 0.0;
   *pMback  = 0.0;
   *pPahead = 0.0;
   *pMahead = 0.0;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   Float64 Density = (pBridge->GetDeckType() == pgsTypes::sdtNone ? pMaterial->GetSegmentWeightDensity(segmentKey,castDeckIntervalIdx) : pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx));
   Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();

   bool bApplyLoadToBackSide  = pBridge->DoesLeftSideEndDiaphragmLoadGirder(pierIdx);
   bool bApplyLoadToAheadSide = pBridge->DoesRightSideEndDiaphragmLoadGirder(pierIdx);

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
   Float64 trib_slab_width = 0;
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      // there isn't a deck so there isn't a tributary width.. use the
      // average spacing between the exterior girders
      Float64 left,right;
      pBridge->GetDistanceBetweenGirders(poi,&left,&right);

      Float64 width = Max(pGirder->GetTopWidth(poi),pGirder->GetBottomWidth(poi));
      trib_slab_width = width + (left+right)/2;
   }
   else
   {
      trib_slab_width = pSectProp->GetTributaryFlangeWidth( poi );
   }

   // Back side of pier
   if (bApplyLoadToBackSide)
   {
      Float64 W,H;
      pBridge->GetBackSideEndDiaphragmSize(pierIdx,&W,&H);

      *pPback = -H*W*Density*g*trib_slab_width/cos(skew);

      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
         CSegmentKey endSegmentKey(segmentKey);
         endSegmentKey.segmentIndex = nSegments-1;
         Float64 brg_offset = pBridge->GetSegmentEndBearingOffset(endSegmentKey);
         Float64 moment_arm = brg_offset - pBridge->GetEndDiaphragmLoadLocationAtEnd(endSegmentKey);
         ATLASSERT(moment_arm <= brg_offset); // diaphragm load should be on same side of pier as girder
         *pMback = *pPback * moment_arm;
      }
   }

   if (bApplyLoadToAheadSide)
   {
      Float64 W,H;
      pBridge->GetAheadSideEndDiaphragmSize(pierIdx,&W,&H);

      *pPahead = -H*W*Density*g*trib_slab_width/cos(skew);

      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         CSegmentKey startSegmentKey(segmentKey);
         startSegmentKey.segmentIndex = 0;
         Float64 brg_offset = pBridge->GetSegmentEndBearingOffset(startSegmentKey);
         Float64 moment_arm = brg_offset - pBridge->GetEndDiaphragmLoadLocationAtEnd(startSegmentKey);
         ATLASSERT(moment_arm <= brg_offset); // diaphragm load should be on same side of pier as girder
         *pMahead = *pPahead * moment_arm;
      }
   }
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   // In the casting yard, during storage, it is assumed that the girder is supported at the
   // locations of the bearings in the final bridge configuration.  For this reason, get
   // the girder deflection at the BridgeSite1 stage.  Note that this deflection is based on
   // the final modulus of elasticity and we need to base it on the modulus of elasticity
   // at release.  Since the assumptions of linear elastic models applies, simply scale
   // the deflection by Ec/Eci
   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   Float64 delta = GetDeflection(erectSegmentIntervalIdx,pftGirder,poi,bat,ctIncremental);
   Float64 rotation  = GetRotation(erectSegmentIntervalIdx,pftGirder,poi,bat,ctIncremental);
   
   GET_IFACE(IMaterials,pMaterial);
   Float64 Eci = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);
   Float64 Ec  = pMaterial->GetSegmentEc(segmentKey,erectSegmentIntervalIdx);

   delta    *= (Ec/Eci);
   rotation *= (Ec/Eci);

   *pDy = delta;
   *pRz = rotation;
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetGirderDeflectionForCamber(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetGirderDeflectionForCamber(poi,config,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
#pragma Reminder("UPDATE: use the new storage model concept for camber")

   // In the casting yard, during storage, it is assumed that the girder is supported at the
   // locations of the bearings in the final bridge configuration.  For this reason, get
   // the girder deflection at the BridgeSite1 stage.  Note that this deflection is based on
   // the final modulus of elasticity and we need to base it on the modulus of elasticity
   // at release.  Since the assumptions of linear elastic models applies, simply scale
   // the deflection by Ec/Eci
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 delta    = GetDeflection(erectSegmentIntervalIdx,pftGirder,poi,bat,ctIncremental);
   Float64 rotation = GetRotation(erectSegmentIntervalIdx,pftGirder,poi,bat,ctIncremental);

   GET_IFACE(IMaterials,pMaterial);

   Float64 Ec = pMaterial->GetSegmentEc(segmentKey,erectSegmentIntervalIdx); // this Ec used to comptue delta

   Float64 Eci;
   if ( config.bUserEci )
      Eci = config.Eci;
   else
      Eci = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),
                                           pMaterial->GetSegmentEccK1(segmentKey),
                                           pMaterial->GetSegmentEccK2(segmentKey));

   delta    *= (Ec/Eci);
   rotation *= (Ec/Eci);

   *pDy = delta;
   *pRz = rotation;
}

void CAnalysisAgentImp::GetLiveLoadMoment(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   std::vector<VehicleIndexType> MminTruck, MmaxTruck;

   GetLiveLoadMoment(llType,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Mmin,&Mmax,pMminTruck ? &MminTruck : NULL, pMmaxTruck ? &MmaxTruck : NULL);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMmin = Mmin[0];
   *pMmax = Mmax[0];

   if ( pMminTruck )
   {
      if ( 0 < MminTruck.size() )
         *pMminTruck = MminTruck[0];
      else
         *pMminTruck = -1;
   }

   if ( pMmaxTruck )
   {
      if ( 0 < MmaxTruck.size() )
         *pMmaxTruck = MmaxTruck[0];
      else
         *pMmaxTruck = -1;
   }
}

void CAnalysisAgentImp::GetLiveLoadShear(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   std::vector<VehicleIndexType> MminTruck, MmaxTruck;
   GetLiveLoadShear(llType,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Vmin,&Vmax,pMminTruck ? &MminTruck : NULL, pMmaxTruck ? &MmaxTruck : NULL);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pVmin = Vmin[0];
   *pVmax = Vmax[0];

   if ( pMminTruck )
   {
      if ( 0 < MminTruck.size() )
         *pMminTruck = MminTruck[0];
      else
         *pMminTruck = -1;
   }

   if ( pMmaxTruck )
   {
      if ( 0 < MmaxTruck.size() )
         *pMmaxTruck = MmaxTruck[0];
      else
         *pMmaxTruck = -1;
   }
}

void CAnalysisAgentImp::GetLiveLoadDeflection(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   std::vector<VehicleIndexType> DminTruck, DmaxTruck;
   GetLiveLoadDeflection(llType,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Dmin,&Dmax,&DminTruck,&DmaxTruck);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);


   *pDmin = Dmin[0];
   *pDmax = Dmax[0];

   if ( pMinConfig )
   {
      if ( 0 < DminTruck.size() )
         *pMinConfig = DminTruck[0];
      else
         *pMinConfig = -1;
   }

   if ( pMaxConfig )
   {
      if ( 0 < DmaxTruck.size() )
         *pMaxConfig = DmaxTruck[0];
      else
         *pMaxConfig = -1;
   }
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   std::vector<VehicleIndexType> RminTruck, RmaxTruck;
   GetLiveLoadRotation(llType,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&RminTruck,&RmaxTruck);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin[0];
   *pRmax = Rmax[0];

   if ( pMinConfig )
   {
      if ( 0 < RminTruck.size() )
         *pMinConfig = RminTruck[0];
      else
         *pMinConfig = -1;
   }

   if ( pMaxConfig )
   {
      if ( 0 < RmaxTruck.size() )
         *pMaxConfig = RmaxTruck[0];
      else
         *pMaxConfig = -1;
   }
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   Float64 Rmin, Rmax;
   GetLiveLoadRotation(llType,intervalIdx,pier,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,&Rmin,&Rmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   // need the POI where the girder intersects the pier
   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest poi = pPoi->GetPointOfInterest(girderKey,pier);

   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);


   Float64 TzMaxLeft, TzMaxRight;
   CComPtr<ILiveLoadConfiguration> TzMaxConfig;
   maxResults->GetResult(0,&TzMaxLeft,&TzMaxConfig,&TzMaxRight,NULL);

   Float64 TzMinLeft, TzMinRight;
   CComPtr<ILiveLoadConfiguration> TzMinConfig;
   minResults->GetResult(0,&TzMinLeft,&TzMinConfig,&TzMinRight,NULL);

   if ( pMaxConfig )
   {
      if ( TzMaxConfig )
      {
         VehicleIndexType vehIdx;
         TzMaxConfig->get_VehicleIndex(&vehIdx);
         *pMaxConfig = vehIdx;
      }
      else
      {
         *pMaxConfig = -1;
      }
   }

   if ( pMinConfig )
   {
      if ( TzMinConfig )
      {
         VehicleIndexType vehIdx;
         TzMinConfig->get_VehicleIndex(&vehIdx);
         *pMinConfig = vehIdx;
      }
      else
      {
         *pMinConfig = -1;
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
         // get reaction that corresonds to T max
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

         // get reaction that corresonds to T min
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

void CAnalysisAgentImp::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   GetLiveLoadReaction(llType,intervalIdx,pier,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,NULL,NULL,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(girderKey.girderIndex);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   ConfigureLBAMPoisForReactions(girderKey,pierIdx,intervalIdx,bat,true);


   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelResults> minResults;
   CComPtr<ILiveLoadModelResults> maxResults;
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   pModelData->pLiveLoadResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, 
          fetFy, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, 
          fetFy, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   CComPtr<ILiveLoadConfiguration> MinConfig;
   CComPtr<ILiveLoadConfiguration> MaxConfig;

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
         Rmax = rmax;
         if ( pMaxConfig )
         {
            if ( maxConfig )
            {
               VehicleIndexType vehIdx;
               maxConfig->get_VehicleIndex(&vehIdx);
               *pMaxConfig = vehIdx;
               MaxConfig.Release();
               MaxConfig = maxConfig;
            }
            else
            {
               *pMaxConfig = -1;
            }
         }
      }

      Float64 rmin;
      CComPtr<ILiveLoadConfiguration> minConfig;
      minResults->GetResult(i,&rmin,&minConfig);
      if ( rmin < Rmin )
      {
         Rmin = rmin;
         if ( pMinConfig )
         {
            if ( minConfig )
            {
               VehicleIndexType vehIdx;
               minConfig->get_VehicleIndex(&vehIdx);
               *pMinConfig = vehIdx;
               MinConfig.Release();
               MinConfig = minConfig;
            }
            else
            {
               *pMinConfig = -1;
            }
         }
      }
   }

   *pRmin = Rmin;
   *pRmax = Rmax;

   if ( pTmax )
   {
      if ( MaxConfig )
      {
         // get rotatation that corresonds to R max
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         CComPtr<IResult3Ds> results;
         MaxConfig->put_ForceEffect(fetRz);
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
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         // get rotatation that corresonds to R min
         CComPtr<IResult3Ds> results;
         MinConfig->put_ForceEffect(fetRz);
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

void CAnalysisAgentImp::GetLiveLoadStress(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig,VehicleIndexType* pTopMaxConfig,VehicleIndexType* pBotMinConfig,VehicleIndexType* pBotMaxConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   std::vector<VehicleIndexType> topMinConfig, topMaxConfig, botMinConfig, botMaxConfig;
   GetLiveLoadStress(llType,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax,&topMinConfig, &topMaxConfig, &botMinConfig, &botMaxConfig);

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
      *pTopMinConfig = topMinConfig[0];

   if ( pTopMaxConfig )
      *pTopMaxConfig = topMaxConfig[0];

   if ( pBotMinConfig )
      *pBotMinConfig = botMinConfig[0];

   if ( pBotMaxConfig )
      *pBotMaxConfig = botMaxConfig[0];
}

void CAnalysisAgentImp::GetLiveLoadModel(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,ILiveLoadModel** ppLiveLoadModel)
{
   CGirderKey key(girderKey);

   if ( key.groupIndex == ALL_GROUPS )
      key.groupIndex = 0;

   if ( key.girderIndex == ALL_GIRDERS )
      key.girderIndex = 0;

   ModelData* pModelData = GetModelData(key.girderIndex);
   ATLASSERT(pModelData->m_Model != NULL || pModelData->m_ContinuousModel != NULL);

   CComPtr<ILiveLoad> live_load;
   if ( pModelData->m_Model )
      pModelData->m_Model->get_LiveLoad(&live_load);
   else
      pModelData->m_ContinuousModel->get_LiveLoad(&live_load);

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

std::vector<std::_tstring> CAnalysisAgentImp::GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey)
{
   USES_CONVERSION;

   CComPtr<ILiveLoadModel> liveload_model;
   GetLiveLoadModel(llType,girderKey,&liveload_model);

   CComPtr<IVehicularLoads> vehicles;
   liveload_model->get_VehicularLoads(&vehicles);

   CComPtr<IEnumVehicularLoad> enum_vehicles;
   vehicles->get__EnumElements(&enum_vehicles);

   std::vector<std::_tstring> names;

   CComPtr<IVehicularLoad> vehicle;
   while ( enum_vehicles->Next(1,&vehicle,NULL) != S_FALSE )
   {
      CComBSTR bstrName;
      vehicle->get_Name(&bstrName);

      std::_tstring strName(OLE2T(bstrName));

      names.push_back(strName);

      vehicle.Release();
   }

   return names;
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   
   std::vector<Float64> Mmin, Mmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadMoment(llType,vehicleIndex,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,
                              &Mmin,&Mmax,
                              pMinAxleConfig ? &minAxleConfig : NULL,
                              pMaxAxleConfig ? &maxAxleConfig : NULL);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMmin = Mmin[0];
   *pMmax = Mmax[0];

   if ( pMinAxleConfig )
      *pMinAxleConfig = minAxleConfig[0];

   if ( pMaxAxleConfig )
      *pMaxAxleConfig = maxAxleConfig[0];
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,AxleConfiguration* pMinLeftAxleConfig,AxleConfiguration* pMinRightAxleConfig,AxleConfiguration* pMaxLeftAxleConfig,AxleConfiguration* pMaxRightAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   std::vector<AxleConfiguration> minLeftAxleConfig,minRightAxleConfig,maxLeftAxleConfig,maxRightAxleConfig;
   GetVehicularLiveLoadShear(llType,vehicleIndex,intervalIdx,vPoi,bat,
                             bIncludeImpact, bIncludeLLDF,&Vmin,&Vmax,
                             pMinLeftAxleConfig  ? &minLeftAxleConfig  : NULL,
                             pMinRightAxleConfig ? &minRightAxleConfig : NULL,
                             pMaxLeftAxleConfig  ? &maxLeftAxleConfig  : NULL,
                             pMaxRightAxleConfig ? &maxRightAxleConfig : NULL);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pVmin = Vmin[0];
   *pVmax = Vmax[0];

   if ( pMinLeftAxleConfig )
      *pMinLeftAxleConfig = minLeftAxleConfig[0];

   if ( pMinRightAxleConfig )
      *pMinRightAxleConfig = minRightAxleConfig[0];

   if ( pMaxLeftAxleConfig )
      *pMaxLeftAxleConfig = maxLeftAxleConfig[0];

   if ( pMaxRightAxleConfig )
      *pMaxRightAxleConfig = maxRightAxleConfig[0];
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadDeflection(llType,vehicleIndex,intervalIdx,vPoi,bat,
                                    bIncludeImpact, bIncludeLLDF,&Dmin,&Dmax,
                                    pMinAxleConfig ? &minAxleConfig : NULL,
                                    pMaxAxleConfig ? &maxAxleConfig : NULL);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pDmin = Dmin[0];
   *pDmax = Dmax[0];

   if ( pMinAxleConfig )
      *pMinAxleConfig = minAxleConfig[0];

   if ( pMaxAxleConfig )
      *pMaxAxleConfig = maxAxleConfig[0];
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadRotation(llType,vehicleIndex,intervalIdx,vPoi,bat,bIncludeImpact, bIncludeLLDF,
                                &Rmin,&Rmax,
                                pMinAxleConfig ? &minAxleConfig : NULL,
                                pMaxAxleConfig ? &maxAxleConfig : NULL);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin[0];
   *pRmax = Rmax[0];

   if ( pMinAxleConfig )
      *pMinAxleConfig = minAxleConfig[0];

   if ( pMaxAxleConfig )
      *pMaxAxleConfig = maxAxleConfig[0];
}

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(girderKey.girderIndex);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   ConfigureLBAMPoisForReactions(girderKey,pierIdx,intervalIdx,bat,true);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CComPtr<ILiveLoadModelResults> minResults;
   CComPtr<ILiveLoadModelResults> maxResults;
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );
   pModelData->pVehicularResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetFy, optMinimize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetFy, optMaximize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILiveLoadConfiguration> RzMaxConfig;
   CComPtr<ILiveLoadConfiguration> RzMinConfig;
   Float64 Rmax = -DBL_MAX;
   Float64 Rmin = DBL_MAX;
   CollectionIndexType nResults;
   maxResults->get_Count(&nResults);
   for ( CollectionIndexType i = 0; i < nResults; i++ )
   {
      Float64 rmax;
      CComPtr<ILiveLoadConfiguration> rzMaxConfig;
      maxResults->GetResult(i,&rmax,pMaxAxleConfig ? &rzMaxConfig : NULL);
      if ( Rmax < rmax )
      {
         Rmax = rmax;
         RzMaxConfig.Release();
         RzMaxConfig = rzMaxConfig;
      }
      
      Float64 rmin;
      CComPtr<ILiveLoadConfiguration> rzMinConfig;
      minResults->GetResult(i,&rmin,pMinAxleConfig ? &rzMinConfig : NULL);
      if ( rmin < Rmin )
      {
         Rmin = rmin;
         RzMinConfig.Release();
         RzMinConfig = rzMinConfig;
      }
   }

   *pRmin = Rmin;
   *pRmax = Rmax;

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);
   if ( pMaxAxleConfig )
   {
      AxleConfiguration maxConfig;
      CreateAxleConfig(lbam_model, RzMaxConfig, &maxConfig);
      *pMaxAxleConfig = maxConfig;
   }

   if ( pMinAxleConfig )
   {
      AxleConfiguration minConfig;
      CreateAxleConfig(lbam_model, RzMinConfig, &minConfig);
      *pMinAxleConfig = minConfig;
   }
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop,AxleConfiguration* pMaxAxleConfigTop,AxleConfiguration* pMinAxleConfigBot,AxleConfiguration* pMaxAxleConfigBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   std::vector<AxleConfiguration> minAxleConfigTop,maxAxleConfigTop,minAxleConfigBot,maxAxleConfigBot;
   GetVehicularLiveLoadStress(llType,vehicleIndex,intervalIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax,&minAxleConfigTop,&maxAxleConfigTop,&minAxleConfigBot,&maxAxleConfigBot);

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

void CAnalysisAgentImp::GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   ATLASSERT(bat == pgsTypes::SimpleSpan || bat == pgsTypes::ContinuousSpan);
   // this are the only 2 value analysis types for this function

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
   CComBSTR bstrStageName( GetLBAMStageName(liveLoadIntervalIdx) );

   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData( segmentKey.girderIndex );


   // make sure there are actually loads applied
   CComPtr<ILiveLoad> live_load;
   if ( pModelData->m_Model )
      pModelData->m_Model->get_LiveLoad(&live_load);
   else
      pModelData->m_ContinuousModel->get_LiveLoad(&live_load);

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

   if (type==IProductForces::DeflectionLiveLoadEnvelope)
   {
      pModelData->pDeflLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, 
             fetDy, optMaximize, vlcDefault, VARIANT_TRUE,VARIANT_TRUE, VARIANT_FALSE,&maxResults);

      pModelData->pDeflLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, 
             fetDy, optMinimize, vlcDefault, VARIANT_TRUE,VARIANT_TRUE, VARIANT_FALSE,&minResults);
   }
   else
   {
      ATLASSERT(type==IProductForces::DesignTruckAlone || type==IProductForces::Design25PlusLane);

      // Assumes VehicularLoads are put in in same order as enum;
      VehicleIndexType trk_idx = (VehicleIndexType)type;

      pModelData->pDeflEnvelopedVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, trk_idx,
             fetDy, optMaximize, vlcDefault, VARIANT_TRUE,dftSingleLane, VARIANT_FALSE,&maxResults);

      pModelData->pDeflEnvelopedVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStageName, lltDeflection, trk_idx,
             fetDy, optMinimize, vlcDefault, VARIANT_TRUE,dftSingleLane, VARIANT_FALSE,&minResults);
   }

   Float64 DyMaxLeft, DyMaxRight;
   maxResults->GetResult(0,&DyMaxLeft,NULL,&DyMaxRight,NULL);

   Float64 DyMinLeft, DyMinRight;
   minResults->GetResult(0,&DyMinLeft,NULL,&DyMinRight,NULL);

   *pDmin = DyMinLeft;
   *pDmax = DyMaxLeft;
}

Float64 CAnalysisAgentImp::GetDesignSlabPadMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // returns the difference in moment between the slab pad moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;
   sysSectionValue M = beam.ComputeMoment(x);

   ATLASSERT( IsEqual(M.Left(),M.Right()) );
   return M.Left();
}

Float64 CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDesignSlabPadDeflectionAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetDesignSlabPadStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot)
{
   GET_IFACE(ISectionProperties,pSectProp);
   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   Float64 M = GetDesignSlabPadMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckInterval = pIntervals->GetCastDeckInterval(poi.GetSegmentKey());

#pragma Reminder("REVIEW: should the section properties be based on fcgdr?")
   // originally the section properties are not based on fcgdr, but this looks suspicous
   Float64 Sbg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::BottomGirder);
   Float64 Stg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::TopGirder);

   *pfTop = M/Stg;
   *pfBot = M/Sbg;
}

rkPPPartUniformLoad CAnalysisAgentImp::GetDesignSlabPadModel(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectionProperties,pSectProp);

   Float64 AdStart = startSlabOffset;
   Float64 AdEnd   = endSlabOffset;

   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
   ATLASSERT(endPierIdx == startPierIdx+1);

   Float64 AoStart = pBridge->GetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex);
   Float64 AoEnd   = pBridge->GetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex);

   Float64 top_flange_width = pGdr->GetTopFlangeWidth( poi );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

#pragma Reminder("REVIEW: should the section properties be based on fcgdr?")
   // originally the section properties are not based on fcgdr, but this looks suspicous
   Float64 Ig = pSectProp->GetIx( castDeckIntervalIdx, poi );
   
   Float64 E = pMaterial->GetEconc(fcgdr,pMaterial->GetSegmentStrengthDensity(segmentKey), 
                                         pMaterial->GetSegmentEccK1(segmentKey), 
                                         pMaterial->GetSegmentEccK2(segmentKey) );

   Float64 L = pBridge->GetSegmentSpanLength(segmentKey);

   Float64 density = pMaterial->GetDeckWeightDensity(segmentKey,castDeckIntervalIdx) ;
   Float64 wStart = (AdStart - AoStart)*top_flange_width*density*unitSysUnitsMgr::GetGravitationalAcceleration();
   Float64 wEnd   = (AdEnd   - AoEnd  )*top_flange_width*density*unitSysUnitsMgr::GetGravitationalAcceleration();

#pragma Reminder("UPDATE: This is incorrect if girders are made continuous before slab casting")
#pragma Reminder("UPDATE: Don't have a roark beam for trapezoidal loads")
   // use the average load
   Float64 w = (wStart + wEnd)/2;

   rkPPPartUniformLoad beam(0,L,-w,L,E,Ig);
   return beam;
}

void CAnalysisAgentImp::DumpAnalysisModels(GirderIndexType gdrIdx)
{
   ModelData* pModelData = GetModelData(gdrIdx);

   if ( pModelData->m_Model )
   {
      CComPtr<IStructuredSave2> save;
      save.CoCreateInstance(CLSID_StructuredSave2);
      std::_tostringstream ss;
      ss << _T("LBAM_SimpleSpan_Girder_") << gdrIdx << _T(".xml");
      save->Open(CComBSTR(ss.str().c_str()));

      CComQIPtr<IStructuredStorage2> strstorage(pModelData->m_Model);
      strstorage->Save(save);

      save->Close();
   }

   if ( pModelData->m_ContinuousModel )
   {
      CComPtr<IStructuredSave2> save2;
      save2.CoCreateInstance(CLSID_StructuredSave2);
      std::_tostringstream ss;
      ss << _T("LBAM_ContinuousSpan_Girder_") << gdrIdx << _T(".xml");
      save2->Open(CComBSTR(ss.str().c_str()));

      CComQIPtr<IStructuredStorage2> strstorage(pModelData->m_ContinuousModel);
      strstorage->Save(save2);

      save2->Close();
   }
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot)
{
#pragma Reminder("REVIEW: does this make sense when time step analysis is used")
   // this is sort of a dummy function until deck shrinkage stress issues are resolved
   // if you count on deck shrinkage for elastic gain, then you have to account for the fact
   // that the deck shrinkage changes the stresses in the girder as well. deck shrinkage is
   // an external load to the girder

   // Top and bottom girder stresses are computed using the composite section method described in
   // Branson, D. E., "Time-Dependent Effects in Composite Concrete Beams", 
   // American Concrete Institute J., Vol 61 (1964) pp. 213-230

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetCompositeDeckInterval(poi.GetSegmentKey());

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);

   Float64 P, M;
   pDetails->pLosses->GetDeckShrinkageEffects(&P,&M);

   GET_IFACE(ISectionProperties,pProps);
   Float64 A  = pProps->GetAg(compositeIntervalIdx,poi);
   Float64 St = pProps->GetS(compositeIntervalIdx,poi,pgsTypes::TopGirder);
   Float64 Sb = pProps->GetS(compositeIntervalIdx,poi,pgsTypes::BottomGirder);

   *pftop = P/A + M/St;
   *pfbot = P/A + M/Sb;
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepDeflection(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   // Deflections are computing using the method of virtual work.
   // See Structural Analysis, 4th Edition, Jack C. McCormac, pg365. Section 18.5, Application of Virtual Work to Beams and Frames

   ATLASSERT(pfType != pftOverlayRating); // not used with time step analysis

   std::vector<Float64> deflections;

   if ( pfType == pftTotalPostTensioning )
   {
      // the deflections due to total post-tensioning is the sum of the primary and secondary effects
      std::vector<Float64> primary(  GetTimeStepDeflection(vPoi,intervalIdx,pftPrimaryPostTensioning,bat,comboType));
      std::vector<Float64> secondary(GetTimeStepDeflection(vPoi,intervalIdx,pftSecondaryEffects,     bat,comboType));

      deflections.reserve(vPoi.size());
      std::transform(primary.begin(),primary.end(),secondary.begin(),std::back_inserter(deflections),std::plus<Float64>());
      return deflections;
   }


#if defined VERIFY_DEFLECTIONS
   std::vector<Float64> incDeflection;
#endif

   GET_IFACE(ILosses,pILosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      const LOSSDETAILS* pLossDetails = pILosses->GetLossDetails(poi);
      Float64 delta;
      if ( comboType == ctIncremental )
      {
         delta = pLossDetails->TimeStepDetails[intervalIdx].dD[pfType]; // incremental
      }
      else
      {
         delta = pLossDetails->TimeStepDetails[intervalIdx].D[pfType]; // cummulative
      }
      deflections.push_back(delta);

#if defined VERIFY_DEFLECTIONS
      incDeflection.push_back(pLossDetails->TimeStepDetails[intervalIdx].dD[pfType]);
#endif
   }

#if defined VERIFY_DEFLECTIONS
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
   if ( (pfType != pftPrestress && 
        pfType != pftPrimaryPostTensioning &&
        pfType != pftCreep &&
        pfType != pftShrinkage &&
        pfType != pftRelaxation) 
        &&
        (intervalIdx == releaseIntervalIdx || erectionIntervalIdx <= intervalIdx)
        )
   {
      std::vector<Float64> deflections2 = GetElasticDeflection(intervalIdx,pfType,vPoi,bat);
      if ( pfType == pftGirder && intervalIdx == erectionIntervalIdx )
      {
         std::vector<Float64> d = GetElasticDeflection(releaseIntervalIdx,pfType,vPoi,bat);
         std::transform(deflections2.begin(),deflections2.end(),d.begin(),deflections2.begin(),std::minus<Float64>());
      }

      ATLASSERT(incDeflection.size() == deflections2.size());

      std::vector<Float64>::iterator d1Iter(incDeflection.begin());
      std::vector<Float64>::iterator d1IterEnd(incDeflection.end());
      std::vector<Float64>::iterator d2Iter(deflections2.begin());
      std::vector<Float64>::iterator d2IterEnd(deflections2.end());
      std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
      for ( ; d1Iter != d1IterEnd && d2Iter != d2IterEnd; d1Iter++, d2Iter++, poiIter++ )
      {
         Float64 d1 = *d1Iter;
         Float64 d2 = *d2Iter;
         if ( !IsEqual(d1,d2,0.001) ) // accuracy is 1 mm
         {
            const pgsPointOfInterest& poi(*poiIter);
            const CSegmentKey& segmentKey(poi.GetSegmentKey());
            Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
            PoiIDType poiID = poi.GetID();

            std::_tstring strAttributes(poi.GetAttributes(POI_ERECTED_SEGMENT,false));


            CString strMsg;
            strMsg.Format(_T("Time Step and Elastic Deflections don't match\nSegment %d (%s) (Xg=%s) (ID=%d)\nDts=%s, De=%s"),
               LABEL_SEGMENT(segmentKey.segmentIndex),
               strAttributes.c_str(),
               ::FormatDimension(Xg,pDisplayUnits->GetSpanLengthUnit(),true),
               poiID,
               ::FormatDimension(d1,pDisplayUnits->GetDeflectionUnit(),true),
               ::FormatDimension(d2,pDisplayUnits->GetDeflectionUnit(),true)
               );
            AfxMessageBox(strMsg);
         }
      }
   }
#endif


   return deflections;
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepDeflection(LoadingCombination combo,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,CombinationType comboType,pgsTypes::BridgeAnalysisType bat)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis
   ATLASSERT(combo != lcDWRating); // not setup to handle this yet

#pragma Reminder("UPDATE: should be able to get product loads for a load case from the LBAM")
   // that way we don't need duplicate code here

   std::vector<ProductForceType> pfTypes;
   std::vector<Float64> deflections;
   deflections.insert(deflections.begin(),vPoi.size(),0.0);
   if ( combo == lcDC )
   {
      pfTypes.push_back(pftGirder);
      pfTypes.push_back(pftConstruction);
      pfTypes.push_back(pftSlab);
      pfTypes.push_back(pftSlabPad);
      pfTypes.push_back(pftSlabPanel);
      pfTypes.push_back(pftDiaphragm); 
      pfTypes.push_back(pftSidewalk);
      pfTypes.push_back(pftTrafficBarrier);
      pfTypes.push_back(pftUserDC);
      pfTypes.push_back(pftShearKey);
   }
   else if ( combo == lcDW )
   {
      pfTypes.push_back(pftOverlay);
      pfTypes.push_back(pftUserDW);
   }
   else if ( combo == lcCR )
   {
      pfTypes.push_back(pftCreep);
      pfTypes.push_back(pftRelaxation);
   }
   else if ( combo == lcSH )
   {
      pfTypes.push_back(pftShrinkage);
   }
   else if ( combo == lcPS )
   {
      pfTypes.push_back(pftSecondaryEffects);
   }

   std::vector<ProductForceType>::iterator pfIter(pfTypes.begin());
   std::vector<ProductForceType>::iterator pfIterEnd(pfTypes.end());
   for ( ; pfIter != pfIterEnd; pfIter++ )
   {
      std::vector<Float64> delta(GetTimeStepDeflection(vPoi,intervalIdx,*pfIter,bat,comboType));

      std::transform(deflections.begin(),deflections.end(),delta.begin(),deflections.begin(),std::plus<Float64>());
   }

   return deflections;
}

void CAnalysisAgentImp::GetTimeStepDeflection(pgsTypes::LimitState limitState,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bIncludePrestress,pgsTypes::BridgeAnalysisType bat,CombinationType comboType,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   // assumes all poi are from the same girder
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pMin->clear();
   pMax->clear();

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gLLMin = pLoadFactors->LLIMmin[limitState];
   Float64 gLLMax = pLoadFactors->LLIMmax[limitState];
   Float64 gDCMin = pLoadFactors->DCmin[limitState];
   Float64 gDCMax = pLoadFactors->DCmax[limitState];
   Float64 gDWMin = pLoadFactors->DWmin[limitState];
   Float64 gDWMax = pLoadFactors->DWmax[limitState];
   Float64 gCRMax = pLoadFactors->CRmax[limitState];
   Float64 gCRMin = pLoadFactors->CRmin[limitState];
   Float64 gSHMax = pLoadFactors->SHmax[limitState];
   Float64 gSHMin = pLoadFactors->SHmin[limitState];
   Float64 gPSMax = pLoadFactors->PSmax[limitState];
   Float64 gPSMin = pLoadFactors->PSmin[limitState];

   std::vector<Float64> DC(GetTimeStepDeflection(lcDC,intervalIdx,vPoi,comboType,bat));
   std::vector<Float64> DW(GetTimeStepDeflection(lcDW,intervalIdx,vPoi,comboType,bat));
   std::vector<Float64> CR(GetTimeStepDeflection(lcCR,intervalIdx,vPoi,comboType,bat));
   std::vector<Float64> SH(GetTimeStepDeflection(lcSH,intervalIdx,vPoi,comboType,bat));
   std::vector<Float64> PS(GetTimeStepDeflection(lcPS,intervalIdx,vPoi,comboType,bat));

   std::vector<Float64> pretension;
   std::vector<Float64> posttension;
   if ( bIncludePrestress )
   {
      pretension  = GetTimeStepDeflection(vPoi,intervalIdx,pftPrestress,bat,comboType);
      posttension = GetTimeStepDeflection(vPoi,intervalIdx,pftPrimaryPostTensioning,bat,comboType);

      std::transform(pretension.begin(),pretension.end(),DC.begin(),DC.begin(),std::plus<Float64>());
      std::transform(posttension.begin(),posttension.end(),DC.begin(),DC.begin(),std::plus<Float64>());
   }
   else
   {
      pretension.resize(DC.size(),0);
      posttension.resize(DC.size(),0);
   }

   std::vector<Float64> LLMin,LLMax;
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      LLMin.insert(LLMin.begin(),vPoi.size(),0.0);
      LLMax.insert(LLMax.begin(),vPoi.size(),0.0);
   }
   else
   {
      pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
      GetCombinedLiveLoadDeflection(llType,intervalIdx,vPoi,bat,&LLMin,&LLMax);
   }

   IndexType nPoi = vPoi.size();
   for ( IndexType idx = 0; idx < nPoi; idx++ )
   {
      Float64 dMax = (0 <= DC[idx] ? gDCMax : gDCMin)*DC[idx];
      dMax += (::BinarySign(dMax) == ::BinarySign(DW[idx])    ? gDWMax : gDWMin)*DW[idx];
      dMax += (::BinarySign(dMax) == ::BinarySign(CR[idx])    ? gCRMax : gCRMin)*CR[idx];
      dMax += (::BinarySign(dMax) == ::BinarySign(SH[idx])    ? gSHMax : gSHMin)*SH[idx];
      dMax += (::BinarySign(dMax) == ::BinarySign(PS[idx])    ? gPSMax : gPSMin)*PS[idx];
      dMax += (::BinarySign(dMax) == ::BinarySign(LLMax[idx]) ? gLLMax : gLLMin)*LLMax[idx];
      pMax->push_back(dMax);

      Float64 dMin = (DC[idx] <= 0 ? gDCMax : gDCMin)*DC[idx];
      dMin += (::BinarySign(dMin) == ::BinarySign(DW[idx])    ? gDWMax : gDWMin)*DW[idx];
      dMin += (::BinarySign(dMin) == ::BinarySign(CR[idx])    ? gCRMax : gCRMin)*CR[idx];
      dMin += (::BinarySign(dMin) == ::BinarySign(SH[idx])    ? gSHMax : gSHMin)*SH[idx];
      dMin += (::BinarySign(dMin) == ::BinarySign(PS[idx])    ? gPSMax : gPSMin)*PS[idx];
      dMin += (::BinarySign(dMin) == ::BinarySign(LLMin[idx]) ? gLLMax : gLLMin)*LLMin[idx];
      pMin->push_back(dMin);
   }
}

std::_tstring CAnalysisAgentImp::GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex)
{
   USES_CONVERSION;

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   if ( vehicleIndex == INVALID_INDEX )
   {
      return OLE2T(GetLiveLoadName(llType));
   }

   ModelData* pModelData = 0;
   pModelData = GetModelData(0); // get model data for girder line zero since all have the same live loads

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,pgsTypes::SimpleSpan,&lbam_model);

   CComPtr<IVehicularLoad> vehicle;
   GetVehicularLoad(lbam_model,llmt,vehicleIndex,&vehicle);

   CComBSTR bstrName;
   vehicle->get_Name(&bstrName);

   return OLE2T(bstrName);
}

VehicleIndexType CAnalysisAgentImp::GetVehicleCount(pgsTypes::LiveLoadType llType)
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

Float64 CAnalysisAgentImp::GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex)
{
   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   ModelData* pModelData = 0;
   pModelData = GetModelData(0); // get model data for girder line zero since all have the same live loads

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,pgsTypes::SimpleSpan,&lbam_model);

   CComPtr<IVehicularLoad> vehicle;
   GetVehicularLoad(lbam_model,llmt,vehicleIndex,&vehicle);

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

void CAnalysisAgentImp::GetEquivPTLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,std::vector<EquivPTPointLoad>& ptLoads,std::vector<EquivPTDistributedLoad>& distLoads,std::vector<EquivPTMomentLoad>& momentLoads)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(ITendonGeometry,pTendonGeometry);

   ptLoads.clear();
   distLoads.clear();
   momentLoads.clear();

   GET_IFACE(IGirder,pIGirder);
   WebIndexType nWebs = pIGirder->GetWebCount(girderKey);

#pragma Reminder("UPDATE: update the equivalent PT load")
   // Get tendon geometry from ITendonGeometry interface. It has tendon geometry information that
   // is corrected for the position of the strand within the duct. This implementation is using the
   // raw input data and it only models the CL of the deck.
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(girderKey.girderIndex);
   const CPTData*             pPTData     = pGirder->GetPostTensioning();
   const CDuctData*           pDuctData   = pPTData->GetDuct(ductIdx/nWebs);

   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
   
   Float64 Pj = pDuctData->Pj;

   SpanIndexType startSpanIdx = pGroup->GetPierIndex(pgsTypes::metStart);

   if ( pDuctData->DuctGeometryType == CDuctGeometry::Linear )
   {
#pragma Reminder("IMPLEMENT")
      ATLASSERT(false); // need to implement
   }
   else if ( pDuctData->DuctGeometryType == CDuctGeometry::Parabolic )
   {
      // Equivalent load for tendon path composed of compounded second-degree parabolas.
      // See "Modern Prestress Concrete", Libby, page 472-473, Eqn (10-7).
      Float64 L = pBridge->GetSpanLength(startSpanIdx,girderKey.girderIndex);

      // Compute the end moment due to the tendon eccentricity
      SpanIndexType nSpans = pDuctData->ParabolicDuctGeometry.GetSpanCount();
      Float64 startPoint, startOffset;
      CDuctGeometry::OffsetType startOffsetType;
      pDuctData->ParabolicDuctGeometry.GetStartPoint(&startPoint,&startOffset,&startOffsetType);

      // if startPoint < 0 then it is a fractional measure... make it an absolute measure
      startPoint = (startPoint < 0 ? -startPoint*L : startPoint);

      pgsPointOfInterest startPoi = pPoi->GetPointOfInterest(girderKey,startPoint);

      // if start point offset is measured from bottom, we need to make it measured from top
      if ( startOffsetType == CDuctGeometry::BottomGirder )
      {
         Float64 Hg = pSectProp->GetHg(stressTendonIntervalIdx,startPoi);
         startOffset = Hg - startOffset;
      }

      // P*e at start of tendon
      Float64 epts = pTendonGeometry->GetEccentricity(stressTendonIntervalIdx,startPoi,ductIdx);
      Float64 Ms = Pj*epts;
      EquivPTMomentLoad start_moment;
      start_moment.P = Ms;
      start_moment.spanIdx = startSpanIdx;
      start_moment.X = startPoint;
      momentLoads.push_back(start_moment);

      // Start of duct to first low point
      Float64 lowPoint, lowOffset;
      CDuctGeometry::OffsetType lowOffsetType;
      pDuctData->ParabolicDuctGeometry.GetLowPoint(startSpanIdx,&lowPoint,&lowOffset,&lowOffsetType);
      lowPoint = (lowPoint < 0 ? -lowPoint*L  : lowPoint);

      if ( lowOffsetType == CDuctGeometry::BottomGirder )
      {
         // make lowOffset measured from top of girder
         pgsPointOfInterest lowPoi = pPoi->GetPointOfInterest(girderKey,lowPoint);
         Float64 Hg = pSectProp->GetHg(stressTendonIntervalIdx,lowPoi);

         lowOffset = Hg - lowOffset;
      }

      // we want e_prime to be positive if the parabola is a smile shape
      EquivPTDistributedLoad load;

      Float64 e_prime = lowOffset - startOffset;
      Float64 x = lowPoint - startPoint;
      Float64 w = 2*Pj*e_prime/(x*x);

      if ( !IsZero(w) )
      {
         load.spanIdx = startSpanIdx;
         load.Xstart  = startPoint;
         load.Xend    = lowPoint;
         load.Wstart  = w;
         load.Wend    = w;

         distLoads.push_back(load);
      }

      for ( PierIndexType pierIdx = startSpanIdx+1; pierIdx < nSpans; pierIdx++ )
      {
         Float64 leftIP, highOffset, rightIP;
         CDuctGeometry::OffsetType highOffsetType;
         pDuctData->ParabolicDuctGeometry.GetHighPoint(pierIdx,&leftIP,&highOffset,&highOffsetType,&rightIP);

         SpanIndexType prevSpanIdx = SpanIndexType(pierIdx-1);
         SpanIndexType nextSpanIdx = prevSpanIdx+1;

         // Low point in span - IP
         Float64 L = pBridge->GetSpanLength(prevSpanIdx,girderKey.girderIndex);

         // if leftIP is neg, then it is a fraction of the distance from the high point to the
         // previous low point, measured from the high point.... convert it to an actual measure
         leftIP = (leftIP < 0 ? -leftIP*(L-lowPoint) : leftIP);


         // highOffset is measured from boittom of girder, need to make it 
         // measured from top of girder
         if ( highOffsetType == CDuctGeometry::BottomGirder )
         {
            pgsPointOfInterest highPoi = pPoi->GetPointOfInterest(girderKey,pierIdx);
            Float64 Hg = pSectProp->GetHg(stressTendonIntervalIdx,highPoi);

            highOffset = Hg - highOffset;
         }

         Float64 ipOffset = (highOffset*(L-leftIP-lowPoint) + lowOffset*(leftIP))/(L-lowPoint);

         e_prime = lowOffset - ipOffset;
         x = (L - leftIP) - lowPoint;
         w = 2*Pj*e_prime/(x*x);

         // low point to inflection point
         if ( !IsZero(w) )
         {
            load.spanIdx = prevSpanIdx;
            load.Xstart  = lowPoint;
            load.Xend    = L - leftIP; // leftIP is measured from high point
            load.Wstart  = w;
            load.Wend    = w;
            distLoads.push_back(load);
         }

         // Inflection point to high point
         e_prime = highOffset - ipOffset;
         x = leftIP;
         w = 2*Pj*e_prime/(x*x);

         // inflection point to high point at pier
         if ( !IsZero(w) )
         {
            load.spanIdx = prevSpanIdx;
            load.Xstart  = L - leftIP; // leftIP is measured from high point
            load.Xend    = L;
            load.Wstart  = w;
            load.Wend    = w;
            distLoads.push_back(load);
         }

         // high point - IP
         pDuctData->ParabolicDuctGeometry.GetLowPoint(nextSpanIdx,&lowPoint,&lowOffset,&lowOffsetType);

         L = pBridge->GetSpanLength(nextSpanIdx,girderKey.girderIndex);
         lowPoint = (lowPoint < 0 ? -lowPoint*L : lowPoint);

         // low point is located from the bottom of the girder. we need it from the top of the girder.
         // convert it here.
         if ( lowOffsetType == CDuctGeometry::BottomGirder )
         {
            Float64 distFromStartOfSpan = lowPoint; // distance from CL Pier line, measured along the girder
            pgsPointOfInterest lowPoi = pPoi->GetPointOfInterest(girderKey,nextSpanIdx,distFromStartOfSpan);
            Float64 Hg = pSectProp->GetHg(stressTendonIntervalIdx,lowPoi);

            lowOffset = Hg - lowOffset;
         }

         // if rightIP is less than zero, rightIP is a fraction measure from the high point to the next low point
         // convert to an actual measure
         rightIP  = (rightIP  < 0 ? -rightIP*lowPoint : rightIP);

         ipOffset = (lowOffset*(rightIP) + highOffset*(lowPoint-rightIP))/(lowPoint);

         e_prime = highOffset - ipOffset;
         x = rightIP;
         w = 2*Pj*e_prime/(x*x);

         // high point to inflection point
         if ( !IsZero(w) )
         {
            load.spanIdx = nextSpanIdx;
            load.Xstart  = 0;
            load.Xend    = rightIP;
            load.Wstart  = w;
            load.Wend    = w;
            distLoads.push_back(load);
         }

         // IP to low point
         e_prime = lowOffset - ipOffset;
         x = lowPoint - rightIP;
         w = 2*Pj*e_prime/(x*x);

         // inflection point to low point
         if ( !IsZero(w) )
         {
            load.spanIdx = nextSpanIdx;
            load.Xstart  = rightIP;
            load.Xend    = lowPoint;
            load.Wstart  = w;
            load.Wend    = w;
            distLoads.push_back(load);
         }
      }

      // low point in last span to end of duct (high point)
      Float64 endPoint, endOffset;
      CDuctGeometry::OffsetType endOffsetType;
      pDuctData->ParabolicDuctGeometry.GetEndPoint(&endPoint,&endOffset,&endOffsetType);

      load.spanIdx = startSpanIdx + nSpans-1;
      L = pBridge->GetSpanLength(load.spanIdx,girderKey.girderIndex);

      endPoint = (endPoint < 0 ? -endPoint*L : endPoint);

      Float64 Lg = pBridge->GetGirderLength(girderKey);
      pgsPointOfInterest endPoi = pPoi->ConvertGirderCoordinateToPoi(girderKey,Lg - endPoint);

      // if end point offset is measured from bottom, we need to make it measured from top
      if ( startOffsetType == CDuctGeometry::BottomGirder )
      {
         Float64 Hg = pSectProp->GetHg(stressTendonIntervalIdx,endPoi);
         endOffset = Hg - endOffset;
      }

      e_prime = lowOffset - endOffset;
      x = (L-endPoint) - lowPoint;
      w = 2*Pj*e_prime/(x*x);

      if ( !IsZero(w) )
      {
         load.Xstart = lowPoint;
         load.Xend = L - endPoint;
         load.Wstart = w;
         load.Wend   = w;
         distLoads.push_back(load);
      }


      // P*e at end of tendon
      Float64 epte = pTendonGeometry->GetEccentricity(stressTendonIntervalIdx,endPoi,ductIdx);
      Float64 Me = Pj*epte;
      EquivPTMomentLoad end_moment;
      end_moment.P = -Me;
      end_moment.spanIdx = startSpanIdx + nSpans-1;
      end_moment.X = L - endPoint;
      momentLoads.push_back(end_moment);
   }
   else
   {
      ATLASSERT(pDuctData->DuctGeometryType == CDuctGeometry::Offset);
#pragma Reminder("IMPLEMENT")
      ATLASSERT(false); // need to implement
   }
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces2
//
std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   USES_CONVERSION;

   std::vector<sysSectionValue> results;
   if ( pfType == pftPrestress )
   {
#pragma Reminder("UPDATE: need better prestress shear")
      // there is a vertical component to prestress.... that's shear!

      // prestress doesn't cause shear (constant moment)
      results.insert(results.begin(),vPoi.size(),sysSectionValue(0,0));
      return results;
   }


   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
#pragma Reminder("OBSOLETE: remove if obsolete")
      ATLASSERT(false); // does this code ever get called?
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            sysSectionValue fx(0),fy(0),mz(0);
            Float64 dx(0),dy(0),rz(0);
            if ( pfType == pftGirder )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            }
            else if ( pfType == pftPrestress )
            {
#pragma Reminder("UPDATE: need shear due to prestress")
            }
            results.push_back(fy);
         }
         return results;
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         // before the segment is erected, there aren't any force results
         std::vector<sysSectionValue> results;
         results.insert(results.begin(),vPoi.size(),sysSectionValue(0,0));
         return results;
      }
      else
      {
         ResultsSummationType resultsSummation = (comboType == ctIncremental ? rsIncremental : rsCumulative);

         ModelData* pModelData = UpdateLBAMPois(vPoi);

         bool bSecondaryEffects = pfType == pftSecondaryEffects;
         bool bPrimaryPT = pfType == pftPrimaryPostTensioning;
         if ( pfType == pftSecondaryEffects || pfType == pftPrimaryPostTensioning )
            pfType = pftTotalPostTensioning; // secondary shears are the shears due to post-tensioning

         CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );


         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
      
         CollectionIndexType nResults;
         section_results->get_Count(&nResults);
         ATLASSERT(nResults == vPoi.size());
         for ( CollectionIndexType idx = 0; idx < nResults; idx++ )
         {
            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 FyLeft, FyRight;
            result->get_YLeft(&FyLeft);
            result->get_YRight(&FyRight);

            if ( bPrimaryPT )
            {
#pragma Reminder("UPDATE/REVIEW: there is a vertical component to PT... this is the shear") // see comments for prestress above
               
               // Primary PT doesn't cause any shear (its is just axial and bending)
               FyLeft  = 0;
               FyRight = 0;
            }

            results.push_back(sysSectionValue(-FyLeft,FyRight));
         }

         return results;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   USES_CONVERSION;

   std::vector<Float64> results;
   if ( pfType == pftPrestress )
   {
      // moments due to prestress has been requested... this is just P*e during the interval
      // and 0 in all other intervals. return P*e in all intervals if cumulative
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IPretensionForce,pPsForce);
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISectionProperties,pSectProp);
      pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

      std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
      for ( ; poiIter != poiIterEnd; poiIter++ )
      {
         const pgsPointOfInterest& poi(*poiIter);
         const CSegmentKey& segmentKey(poi.GetSegmentKey());

         IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
         IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval(segmentKey);

         bool bIncTempStrands = (intervalIdx < tsRemovalIntervalIdx) ? true : false;
         Float64 P;
         if ( intervalIdx < liveLoadIntervalIdx )
         {
            P = pPsForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
            if ( bIncTempStrands )
            {
              P += pPsForce->GetPrestressForce(poi,pgsTypes::Temporary,intervalIdx,pgsTypes::End);
            }
         }
         else
         {
            P = pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent);
            if ( bIncTempStrands )
            {
               P += pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Temporary);
            }
         }

         Float64 nSEffective;
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
         Float64 e = pStrandGeom->GetEccentricity( releaseIntervalIdx, poi, bIncTempStrands, &nSEffective );

#pragma Reminder("UPDATE: need better prestress moment")
         // this doesn't account for the development length factor... we should be able to call
         // a GetPrestressMoment function and not have to do all that work here
         // ...
         // and this isn't 100% correct for incremental cases. the (Loss)*(e) moment is
         // the incremental moment and the cumulative moment changes through time
         // because of losses and elastic effects
         Float64 m = -(P*e);

         if ( intervalIdx != releaseIntervalIdx && comboType == ctIncremental )
         {
            m = 0;
         }

         results.push_back(m);
      }

      return results;
   }

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
#pragma Reminder("OBSOLETE: remove if obsolete")
      ATLASSERT(false); // does this code ever get called?
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            sysSectionValue fx(0),fy(0),mz(0);
            Float64 dx(0),dy(0),rz(0);
            if ( pfType == pftGirder )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
               ATLASSERT(IsEqual(mz.Left(),-mz.Right(),0.0001));
            }
            else if ( pfType == pftPrestress )
            {
#pragma Reminder("UPDATE: need deflection due to prestress"
            }
            results.push_back( mz.Left() );
         }
      }
      else if (intervalIdx < erectionIntervalIdx )
      {
         // before the segment is erected, and this isn't release or strorage there aren't any force results
         // could be returning lifting and hauling, but we aren't for now
         results.insert(results.begin(),vPoi.size(),0.0);
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         GET_IFACE(IBridge,pBridge);

         bool bSecondaryEffects = false;
         if ( pfType == pftSecondaryEffects )
         {
            pfType = pftTotalPostTensioning;
            bSecondaryEffects = true;
         }

         bool bPrimaryPT = false;
         if ( pfType == pftPrimaryPostTensioning )
         {
            pfType = pftTotalPostTensioning;
            bPrimaryPT = true;
         }

         CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         ResultsSummationType resultsSummation = (comboType == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,resultsSummation,&section_results);


         GET_IFACE(ITendonGeometry,pTendonGeometry);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            CollectionIndexType idx = iter - vPoi.begin();

            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
            Float64 product_location = poi.GetDistFromStart() - start_offset;

            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 MzLeft, MzRight;
            result->get_ZLeft(&MzLeft);
            result->get_ZRight(&MzRight);

            Float64 Mz;
            if ( IsZero(product_location) )
               Mz = -MzRight; // use right side result at start of span
            else
               Mz = MzLeft; // use left side result at all other locations

            Float64 Mppt = 0; // moment due to primary PT (P*e)
            if ( bSecondaryEffects || bPrimaryPT )
            {
               CGirderKey girderKey(segmentKey);
               DuctIndexType nDucts = pTendonGeometry->GetDuctCount(girderKey);
               for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
               {
                  IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
                  Float64 Mpt = 0;
                  if ( intervalIdx == stressTendonIntervalIdx || comboType == ctCumulative )
                  {
#pragma Reminder("UPDATE: need to improve moments due to PT - same comments as for prestress above")
                     Float64 Ppt = pTendonGeometry->GetPjack(girderKey,ductIdx);
                     Float64 ept = pTendonGeometry->GetEccentricity(intervalIdx,poi,ductIdx);
                     Mpt = -Ppt*ept; // primary prestress moment
                  }

                  Mppt += Mpt;
               }

               if ( bPrimaryPT )
                  Mz = Mppt;
               else
                  Mz -= Mppt;
            }

            results.push_back(Mz);
         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   std::vector<Float64> vResults;
   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
      vResults = GetTimeStepDeflection(vPoi,intervalIdx,pfType,bat,comboType);
   else
      vResults = GetElasticDeflection(intervalIdx,pfType,vPoi,bat,comboType);

#pragma Reminder("UPDATE: do elevation adjustment need to be applied to product force Deflections?")

   return vResults;
}

std::vector<Float64> CAnalysisAgentImp::GetElasticDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   USES_CONVERSION;

   std::vector<Float64> results;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());

   if ( pfType == pftPrestress )
   {
      bool bRelativeToBearings = (intervalIdx == releaseIntervalIdx ? false : true);
      if ( (comboType == ctIncremental && intervalIdx == releaseIntervalIdx) || comboType == ctCumulative )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            // all versions of PGSuper that have done elastic analysis have not modified the deflections due to
            // prestress in response to prestress losses. 

#pragma Reminder("UPDATE: need to improve deflections due to prestress")
            // need to account for change in support conditions from release to storage and storage to erection
            Float64 dy = GetPrestressDeflection(poi,bRelativeToBearings);
            results.push_back(dy);
         }
      }
      else
      {
         results.insert(results.begin(),vPoi.size(),0);
      }

      return results;
   }

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
#pragma Reminder("OBSOLETE: remove if obsolete")
      ATLASSERT(false); // does this code ever get called?
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }


   try
   {
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            sysSectionValue fx(0),fy(0),mz(0);
            Float64 dx(0),dy(0),rz(0);
            if ( pfType == pftGirder )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            }
            results.push_back( dy );
         }
         return results;
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         // before the segment is erected, there aren't any force results
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         if ( pfType == pftSecondaryEffects || pfType == pftPrimaryPostTensioning )
            pfType = pftTotalPostTensioning; // secondary deflections are the deflection sdue to post-tensioning

         CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         ResultsSummationType resultsSummation = (comboType == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetDy][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetDy][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            CollectionIndexType idx = iter - vPoi.begin();

            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 dy;
            result->get_YLeft(&dy); 

            results.push_back(dy);
         }
         return results;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType)
{
   USES_CONVERSION;

   if ( type == pftCreep || type == pftShrinkage || type == pftRelaxation )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   std::vector<Float64> results;

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         ATLASSERT(type == pftGirder);
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            sysSectionValue fx(0),fy(0),mz(0);
            Float64 dx(0),dy(0),rz(0);
            if ( type == pftGirder )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            }
            results.push_back( rz );
         }
         return results;
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         // before the segment is erected, there aren't any force results except
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         if ( type == pftSecondaryEffects || type == pftPrimaryPostTensioning )
            type = pftTotalPostTensioning; // secondary rotations are the rotations due to post-tensioning

         CComBSTR bstrLoadGroup( GetLoadGroupName(type) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         ResultsSummationType resultsSummation = (comboType == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetRz][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetRz][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, resultsSummation,&section_results);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            CollectionIndexType idx = iter - vPoi.begin();

            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 rz;
            result->get_ZLeft(&rz); 

            results.push_back(rz);
         }
         return results;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}
void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
      GetStressTimeStep(intervalIdx,pfType,vPoi,bat,comboType,topLocation,botLocation,pfTop,pfBot);
   else
      GetStressElastic(intervalIdx,pfType,vPoi,bat,comboType,topLocation,botLocation,pfTop,pfBot);
}

void CAnalysisAgentImp::GetStressTimeStep(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop, fBot;
      if ( pfType == pftTotalPostTensioning )
      {
         // total post-tensioning is primary (P*e + secondary)
         fTop = pTopConcreteElement->f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][comboType]
              + pTopConcreteElement->f[pgsTypes::TopFace   ][pftSecondaryEffects     ][comboType];

         fBot = pBotConcreteElement->f[pgsTypes::BottomFace][pftPrimaryPostTensioning][comboType]
              + pBotConcreteElement->f[pgsTypes::BottomFace][pftSecondaryEffects     ][comboType];
      }
      else
      {
         fTop = pTopConcreteElement->f[pgsTypes::TopFace   ][pfType][comboType];
         fBot = pBotConcreteElement->f[pgsTypes::BottomFace][pfType][comboType];
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetStressElastic(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,CombinationType comboType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;
   GET_IFACE(IBridge,pBridge);

   // stress locations must be either both girder or both deck
   ATLASSERT( (IsGirderStressLocation(topLocation) && IsGirderStressLocation(botLocation))
              ||
              (IsDeckStressLocation(topLocation) && IsDeckStressLocation(botLocation)));

   pfTop->clear();
   pfBot->clear();

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         if ( IsGirderStressLocation(topLocation) )
         {
            std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
            std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
            for ( ; iter != end; iter++ )
            {
               const pgsPointOfInterest& poi = *iter;
               const CSegmentKey& segmentKey = poi.GetSegmentKey();

               ValidateAnalysisModels(segmentKey.girderIndex);

               Float64 top_results(0);
               Float64 bot_results(0);

   #pragma Reminder("UPDATE: storage stresses should be based on stress increment")
               // the current implementation computes storage stress based only on storage model... it should
               // be the stress at release plus the stress due the change in moment between storage and release
               if ( pfType == pftGirder )
               {
                  top_results = GetSectionStress( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), pgsTypes::TopGirder,    poi );
                  bot_results = GetSectionStress( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), pgsTypes::BottomGirder, poi );
               }
               else if ( pfType == pftPrestress )
               {
                  top_results = GetStress(intervalIdx,poi,pgsTypes::TopGirder);
                  bot_results = GetStress(intervalIdx,poi,pgsTypes::BottomGirder);
               }

               pfTop->push_back( top_results );
               pfBot->push_back( bot_results );
            }
         }
         else
         {
            pfTop->insert(pfTop->begin(),vPoi.size(),0.0);
            pfBot->insert(pfBot->begin(),vPoi.size(),0.0);
            std::vector<Float64> results;
            results.insert(results.begin(),vPoi.size(),0.0);
         }
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         // before the segment is erected, there aren't any force results
         pfTop->insert(pfTop->begin(),vPoi.size(),0.0);
         pfBot->insert(pfBot->begin(),vPoi.size(),0.0);
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
      }
      else
      {
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
         if ( IsDeckStressLocation(topLocation) && intervalIdx < compositeDeckIntervalIdx )
         {
            // deck isn't composite yet so it doesn't have any stress
            pfTop->insert(pfTop->begin(),vPoi.size(),0.0);
            pfBot->insert(pfBot->begin(),vPoi.size(),0.0);
            std::vector<Float64> results;
            results.insert(results.begin(),vPoi.size(),0.0);
         }
         else
         {
            ModelData* pModelData = UpdateLBAMPois(vPoi);

            bool bSecondaryEffects = false;
            if ( pfType == pftSecondaryEffects || pfType == pftPrimaryPostTensioning )
            {
               pfType = pftTotalPostTensioning;
               bSecondaryEffects = true;
            }


            CComBSTR bstrLoadGroup( GetLoadGroupName(pfType) );
            CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

            ResultsSummationType resultsSummation = (comboType == ctIncremental ? rsIncremental : rsCumulative);

            CComPtr<ISectionStressResults> min_results, max_results, results;
            if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            {
               pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &max_results);
               pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &min_results);
            }
            else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            {
               pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &max_results);
               pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &min_results);
            }
            else
            {
               pModelData->pLoadGroupResponse[bat]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, resultsSummation, &results);
            }
         
            CollectionIndexType stress_point_index_top = GetStressPointIndex(topLocation);
            CollectionIndexType stress_point_index_bot = GetStressPointIndex(botLocation);

            GET_IFACE(ISectionProperties,pSectionProperties);

            std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
            std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
            for ( ; iter != end; iter++ )
            {
               const pgsPointOfInterest& poi = *iter;
               CollectionIndexType idx = iter - vPoi.begin();

               const CSegmentKey& segmentKey = poi.GetSegmentKey();

               Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
               Float64 dist_from_start = poi.GetDistFromStart();

               Float64 fTop, fBot;

               if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
               {
                  CComPtr<ISectionStressResult> top_stresses;
                  max_results->get_Item(idx,&top_stresses);

                  CComPtr<ISectionStressResult> bot_stresses;
                  min_results->get_Item(idx,&bot_stresses);

                  if ( IsZero(dist_from_start - end_size) )
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

                  if ( IsZero(dist_from_start - end_size) )
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

                  if ( IsZero(dist_from_start - end_size) )
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

               if ( bSecondaryEffects )
               {
                  // need to subtract out stress caused by primary prestress moment 
   #pragma Reminder("UPDATE: add adjustment to get secondary effects")
               }

               // Adjust girder stress for release & storage
               if ( IsGirderStressLocation(topLocation) && pfType == pftGirder && !poi.HasAttribute(POI_CLOSURE) )
               {
                  ValidateAnalysisModels(vPoi.front().GetSegmentKey().girderIndex);

                  Float64 fReleaseTop = GetSectionStress( m_ReleaseModels, topLocation, poi );
                  Float64 fReleaseBot = GetSectionStress( m_ReleaseModels, botLocation, poi );

                  Float64 fStorageTop = GetSectionStress( m_StorageModels, topLocation, poi );
                  Float64 fStorageBot = GetSectionStress( m_StorageModels, botLocation, poi );

                  Float64 StopErection = pSectionProperties->GetS(erectionIntervalIdx,poi,topLocation);
                  Float64 SbotErection = pSectionProperties->GetS(erectionIntervalIdx,poi,botLocation);
                  Float64 StopRelease  = pSectionProperties->GetS(releaseIntervalIdx, poi,topLocation);
                  Float64 SbotRelease  = pSectionProperties->GetS(releaseIntervalIdx, poi,botLocation);
                  Float64 StopStorage  = pSectionProperties->GetS(storageIntervalIdx, poi,topLocation);
                  Float64 SbotStorage  = pSectionProperties->GetS(storageIntervalIdx, poi,botLocation);

                  fTop += fReleaseTop*(1-StopRelease/StopStorage) + fStorageTop*(1-StopStorage/StopErection);
                  fBot += fReleaseBot*(1-SbotRelease/SbotStorage) + fStorageBot*(1-SbotStorage/SbotErection);
               }

               pfTop->push_back(fTop);
               pfBot->push_back(fBot);
            }
         }
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }

}

void CAnalysisAgentImp::GetLiveLoadMoment(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck)
{
   USES_CONVERSION;

   pMmin->clear();
   pMmax->clear();

   if ( pMminTruck )
      pMminTruck->clear();

   if ( pMmaxTruck )
      pMmaxTruck->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));;

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetMz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetMz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);


   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CollectionIndexType idx = iter - vPoi.begin();

      Float64 MzMaxLeft, MzMaxRight;
      CComPtr<ILiveLoadConfiguration> MzMaxLeftConfig, MzMaxRightConfig;

      maxResults->GetResult(idx,
                            &MzMaxLeft, pMmaxTruck ? &MzMaxLeftConfig  : NULL,
                            &MzMaxRight,pMmaxTruck ? &MzMaxRightConfig : NULL);

      Float64 MzMinLeft, MzMinRight;
      CComPtr<ILiveLoadConfiguration> MzMinLeftConfig, MzMinRightConfig;
      minResults->GetResult(idx,
                            &MzMinLeft,  pMminTruck ? &MzMinLeftConfig  : NULL,
                            &MzMinRight, pMminTruck ? &MzMinRightConfig : NULL);

      Float64 dist_from_start = poi.GetDistFromStart();
      Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
      if ( IsZero(dist_from_start - start_offset) )
      {
         pMmin->push_back( -MzMinRight );

         if ( pMminTruck && MzMinRightConfig )
         {
            VehicleIndexType vehIdx;
            MzMinRightConfig->get_VehicleIndex(&vehIdx);
            pMminTruck->push_back(vehIdx);
         }

         pMmax->push_back( -MzMaxRight );

         if ( pMmaxTruck && MzMaxRightConfig )
         {
            VehicleIndexType vehIdx;
            MzMaxRightConfig->get_VehicleIndex(&vehIdx);
            pMmaxTruck->push_back(vehIdx);
         }
      }
      else
      {
         pMmin->push_back( MzMinLeft );

         if ( pMminTruck && MzMinLeftConfig )
         {
            VehicleIndexType vehIdx;
            MzMinLeftConfig->get_VehicleIndex(&vehIdx);
            pMminTruck->push_back(vehIdx);
         }

         pMmax->push_back( MzMaxLeft );

         if ( pMmaxTruck && MzMaxLeftConfig )
         {
            VehicleIndexType vehIdx;
            MzMaxLeftConfig->get_VehicleIndex(&vehIdx);
            pMmaxTruck->push_back(vehIdx);
         }
      }
   }
}

void CAnalysisAgentImp::GetLiveLoadShear(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMinTruck,std::vector<VehicleIndexType>* pMaxTruck)
{
   USES_CONVERSION;

   pVmin->clear();
   pVmax->clear();

   if ( pMinTruck )
      pMinTruck->clear();

   if ( pMaxTruck )
      pMaxTruck->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

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
   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFy, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFy, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 FyMaxLeft, FyMaxRight;
      CComPtr<ILiveLoadConfiguration> FyMaxLeftConfig, FyMaxRightConfig;
      maxResults->GetResult(idx,&FyMaxLeft, pMinTruck ? &FyMaxLeftConfig  : NULL,
                                &FyMaxRight,pMinTruck ? &FyMaxRightConfig : NULL);

      Float64 FyMinLeft, FyMinRight;
      CComPtr<ILiveLoadConfiguration> FyMinLeftConfig, FyMinRightConfig;
      minResults->GetResult(idx,&FyMinLeft, pMaxTruck ? &FyMinLeftConfig  : NULL,
                                &FyMinRight,pMaxTruck ? &FyMinRightConfig : NULL);

      sysSectionValue maxValues(-FyMinLeft,FyMinRight);
      sysSectionValue minValues(-FyMaxLeft,FyMaxRight);

      pVmin->push_back( minValues );
      pVmax->push_back( maxValues );

      if ( pMinTruck )
      {
         if ( -FyMaxLeft < FyMaxRight && FyMaxRightConfig )
         {
            VehicleIndexType vehIdx;
            FyMaxRightConfig->get_VehicleIndex(&vehIdx);
            pMinTruck->push_back(vehIdx);
         }
         else if ( FyMaxLeftConfig )
         {
            VehicleIndexType vehIdx;
            FyMaxLeftConfig->get_VehicleIndex(&vehIdx);
            pMinTruck->push_back(vehIdx);
         }
      }


      if ( pMaxTruck )
      {
         if ( -FyMinLeft < FyMinRight  && FyMinRightConfig )
         {
            VehicleIndexType vehIdx;
            FyMinRightConfig->get_VehicleIndex(&vehIdx);
            pMaxTruck->push_back(vehIdx);
         }
         else if ( FyMinLeftConfig )
         {
            VehicleIndexType vehIdx;
            FyMinLeftConfig->get_VehicleIndex(&vehIdx);
            pMaxTruck->push_back(vehIdx);
         }
      }
   }
}

void CAnalysisAgentImp::GetLiveLoadDeflection(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   USES_CONVERSION;

   pDmin->clear();
   pDmax->clear();

   if ( pMinConfig )
      pMinConfig->clear();

   if ( pMaxConfig )
      pMaxConfig->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetDy, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetDy, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 DyMaxLeft, DyMaxRight;
      CComPtr<ILiveLoadConfiguration> DyMaxConfig;
      maxResults->GetResult(idx,&DyMaxLeft,pMaxConfig ? &DyMaxConfig : NULL,&DyMaxRight,NULL);

      Float64 DyMinLeft, DyMinRight;
      CComPtr<ILiveLoadConfiguration> DyMinConfig;
      minResults->GetResult(idx,&DyMinLeft,pMinConfig ? &DyMinConfig : NULL,&DyMinRight,NULL);

      if ( pMaxConfig && DyMaxConfig )
      {
         VehicleIndexType vehIdx;
         DyMaxConfig->get_VehicleIndex(&vehIdx);
         pMaxConfig->push_back(vehIdx);
      }

      if ( pMinConfig && DyMinConfig )
      {
         VehicleIndexType vehIdx;
         DyMinConfig->get_VehicleIndex(&vehIdx);
         pMinConfig->push_back(vehIdx);
      }

      pDmin->push_back( DyMinLeft );
      pDmax->push_back( DyMaxLeft );
   }
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   USES_CONVERSION;

   pRmin->clear();
   pRmax->clear();

   if ( pMinConfig )
      pMinConfig->clear();

   if ( pMaxConfig )
      pMaxConfig->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 RzMaxLeft, RzMaxRight;
      CComPtr<ILiveLoadConfiguration> RzMaxConfig;
      maxResults->GetResult(idx,&RzMaxLeft,pMaxConfig ? &RzMaxConfig : NULL,&RzMaxRight,NULL);

      Float64 RzMinLeft, RzMinRight;
      CComPtr<ILiveLoadConfiguration> RzMinConfig;
      minResults->GetResult(idx,&RzMinLeft,pMinConfig ? &RzMinConfig : NULL,&RzMinRight,NULL);

      if ( pMaxConfig && RzMaxConfig )
      {
         VehicleIndexType vehIdx;
         RzMaxConfig->get_VehicleIndex(&vehIdx);
         pMaxConfig->push_back(vehIdx);
      }

      if ( pMinConfig && RzMinConfig )
      {
         VehicleIndexType vehIdx;
         RzMinConfig->get_VehicleIndex(&vehIdx);
         pMinConfig->push_back(vehIdx);
      }

      pRmin->push_back( RzMinLeft );
      pRmax->push_back( RzMaxLeft );
   }
}

void CAnalysisAgentImp::GetLiveLoadStress(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex,std::vector<VehicleIndexType>* pTopMaxIndex,std::vector<VehicleIndexType>* pBotMinIndex,std::vector<VehicleIndexType>* pBotMaxIndex)
{
   USES_CONVERSION;

   // stress locations must be either both girder or both deck
   ATLASSERT( (IsGirderStressLocation(topLocation) && IsGirderStressLocation(botLocation))
              ||
              (IsDeckStressLocation(topLocation) && IsDeckStressLocation(botLocation)));


   GET_IFACE(IBridge,pBridge);

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   if ( pTopMinIndex )
      pTopMinIndex->clear();

   if ( pTopMaxIndex )
      pTopMaxIndex->clear();

   if ( pBotMinIndex )
      pBotMinIndex->clear();

   if ( pBotMaxIndex )
      pBotMaxIndex->clear();
   
   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelStressResults> minResults;
   CComPtr<ILiveLoadModelStressResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));
   pModelData->pLiveLoadResponse[bat]->ComputeStresses( m_LBAMPoi, bstrStage, llmt, 
          fetMz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeStresses( m_LBAMPoi, bstrStage, llmt, 
          fetMz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

   CollectionIndexType top_stress_point_idx = GetStressPointIndex(topLocation);
   CollectionIndexType bot_stress_point_idx = GetStressPointIndex(botLocation);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CollectionIndexType idx = iter - vPoi.begin();

      CComPtr<IStressResult> fLeftMax, fRightMax;
      CComPtr<ILiveLoadConfiguration> fLeftMaxConfig, fRightMaxConfig;
      maxResults->GetResult(idx,&fLeftMax,pTopMaxIndex || pBotMaxIndex ? &fLeftMaxConfig : NULL,&fRightMax,pTopMaxIndex || pBotMaxIndex ? &fRightMaxConfig : NULL);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      CComPtr<ILiveLoadConfiguration> fLeftMinConfig, fRightMinConfig;
      minResults->GetResult(idx,&fLeftMin,pTopMinIndex || pBotMinIndex ? &fLeftMinConfig : NULL,&fRightMin,pTopMinIndex || pBotMinIndex ? &fRightMinConfig : NULL);

      Float64 dist_from_start = poi.GetDistFromStart();
      Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 fBotMax, fBotMin, fTopMax, fTopMin;

      if ( IsZero(dist_from_start - start_offset) )
      {
         fRightMax->GetResult(bot_stress_point_idx,&fBotMax);
         fRightMax->GetResult(top_stress_point_idx,&fTopMin);
         fRightMin->GetResult(bot_stress_point_idx,&fBotMin);
         fRightMin->GetResult(top_stress_point_idx,&fTopMax);

         if ( pTopMinIndex )
         {
            VehicleIndexType vehIdx;
            fRightMaxConfig->get_VehicleIndex(&vehIdx);
            pTopMinIndex->push_back(vehIdx);
         }

         if ( pTopMaxIndex )
         {
            VehicleIndexType vehIdx;
            fRightMinConfig->get_VehicleIndex(&vehIdx);
            pTopMaxIndex->push_back(vehIdx);
         }

         if ( pBotMinIndex )
         {
            VehicleIndexType vehIdx;
            fRightMinConfig->get_VehicleIndex(&vehIdx);
            pBotMinIndex->push_back(vehIdx);
         }

         if ( pBotMaxIndex )
         {
            VehicleIndexType vehIdx;
            fRightMaxConfig->get_VehicleIndex(&vehIdx);
            pBotMaxIndex->push_back(vehIdx);
         }
      }
      else
      {
         fLeftMax->GetResult(bot_stress_point_idx,&fBotMax);
         fLeftMax->GetResult(top_stress_point_idx,&fTopMin);
         fLeftMin->GetResult(bot_stress_point_idx,&fBotMin);
         fLeftMin->GetResult(top_stress_point_idx,&fTopMax);

         if ( pTopMinIndex )
         {
            VehicleIndexType vehIdx;
            fLeftMaxConfig->get_VehicleIndex(&vehIdx);
            pTopMinIndex->push_back(vehIdx);
         }

         if ( pTopMaxIndex )
         {
            VehicleIndexType vehIdx;
            fLeftMinConfig->get_VehicleIndex(&vehIdx);
            pTopMaxIndex->push_back(vehIdx);
         }

         if ( pBotMinIndex )
         {
            VehicleIndexType vehIdx;
            fLeftMinConfig->get_VehicleIndex(&vehIdx);
            pBotMinIndex->push_back(vehIdx);
         }

         if ( pBotMaxIndex )
         {
            VehicleIndexType vehIdx;
            fLeftMaxConfig->get_VehicleIndex(&vehIdx);
            pBotMaxIndex->push_back(vehIdx);
         }
      }

      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);
   }
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   USES_CONVERSION;

   pMmin->clear();
   pMmax->clear();

   if ( pMinAxleConfig )
      pMinAxleConfig->clear();

   if ( pMaxAxleConfig )
      pMaxAxleConfig->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetMz, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetMz, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);


   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CollectionIndexType idx = iter - vPoi.begin();

      Float64 MzMaxLeft, MzMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;

      maxResults->GetResult(idx,&MzMaxLeft, pMaxAxleConfig ? &maxLeftConfig  : NULL,
                                &MzMaxRight,pMaxAxleConfig ? &maxRightConfig : NULL);

      Float64 MzMinLeft, MzMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&MzMinLeft, pMinAxleConfig ? &minLeftConfig  : NULL,
                                &MzMinRight,pMinAxleConfig ? &minRightConfig : NULL);

      Float64 dist_from_start = poi.GetDistFromStart();
      GET_IFACE(IBridge,pBridge);
      Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
      if ( IsZero(dist_from_start - start_offset) )
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
   }
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<AxleConfiguration>* pMinLeftAxleConfig,std::vector<AxleConfiguration>* pMinRightAxleConfig,std::vector<AxleConfiguration>* pMaxLeftAxleConfig,std::vector<AxleConfiguration>* pMaxRightAxleConfig)
{
   USES_CONVERSION;

   pVmin->clear();
   pVmax->clear();

   if ( pMinLeftAxleConfig )
      pMinLeftAxleConfig->clear();

   if ( pMinRightAxleConfig )
      pMinRightAxleConfig->clear();

   if ( pMaxLeftAxleConfig )
      pMaxLeftAxleConfig->clear();

   if ( pMaxRightAxleConfig )
      pMaxRightAxleConfig->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetFy, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetFy, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 FyMaxLeft, FyMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&FyMaxLeft, pMinLeftAxleConfig  ? &maxLeftConfig  : NULL,
                                &FyMaxRight,pMinRightAxleConfig ? &maxRightConfig : NULL);

      Float64 FyMinLeft, FyMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&FyMinLeft, pMaxLeftAxleConfig  ? &minLeftConfig  : NULL,
                                &FyMinRight,pMaxRightAxleConfig ? &minRightConfig : NULL);

      sysSectionValue minValues(-FyMaxLeft,FyMaxRight);
      sysSectionValue maxValues(-FyMinLeft,FyMinRight);

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

void CAnalysisAgentImp::GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop,std::vector<AxleConfiguration>* pMaxAxleConfigTop,std::vector<AxleConfiguration>* pMinAxleConfigBot,std::vector<AxleConfiguration>* pMaxAxleConfigBot)
{
   USES_CONVERSION;

   // stress locations must be either both girder or both deck
   ATLASSERT( (IsGirderStressLocation(topLocation) && IsGirderStressLocation(botLocation))
              ||
              (IsDeckStressLocation(topLocation) && IsDeckStressLocation(botLocation)));
#pragma Reminder("UPDATE: need to deal with top/bottom deck stress due to live load")

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   if ( pMinAxleConfigTop )
      pMinAxleConfigTop->clear();

   if ( pMaxAxleConfigTop )
      pMaxAxleConfigTop->clear();

   if ( pMinAxleConfigBot )
      pMinAxleConfigBot->clear();

   if ( pMaxAxleConfigBot )
      pMaxAxleConfigBot->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelStressResults> minResults;
   CComPtr<ILiveLoadModelStressResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));


   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeStresses(m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetMz, optMinimize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeStresses(m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetMz, optMaximize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   CollectionIndexType top_stress_point_idx = GetStressPointIndex(topLocation);
   CollectionIndexType bot_stress_point_idx = GetStressPointIndex(botLocation);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CollectionIndexType idx = iter - vPoi.begin();

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CComPtr<IStressResult> fLeftMax, fRightMax;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&fLeftMax,pMaxAxleConfigTop || pMaxAxleConfigBot ? &maxLeftConfig : NULL,&fRightMax,pMaxAxleConfigTop || pMaxAxleConfigBot ? &maxRightConfig : NULL);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&fLeftMin,pMinAxleConfigTop || pMinAxleConfigBot ? &minLeftConfig : NULL,&fRightMin,pMinAxleConfigTop || pMinAxleConfigBot ? &minRightConfig : NULL);

      Float64 dist_from_start = poi.GetDistFromStart();
      GET_IFACE(IBridge,pBridge);
      Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 fBotMax, fBotMin, fTopMax, fTopMin;
      if ( IsZero(dist_from_start - start_offset) )
      {
         fRightMax->GetResult(bot_stress_point_idx,&fBotMax);
         fRightMax->GetResult(top_stress_point_idx,&fTopMin);
         fRightMin->GetResult(bot_stress_point_idx,&fBotMin);
         fRightMin->GetResult(top_stress_point_idx,&fTopMax);

         if ( pMaxAxleConfigBot )
         {
            AxleConfiguration maxAxleRightConfig;
            CreateAxleConfig(lbam_model, maxRightConfig, &maxAxleRightConfig);
            pMaxAxleConfigBot->push_back(maxAxleRightConfig);
         }

         if ( pMaxAxleConfigTop )
         {
            AxleConfiguration minAxleRightConfig;
            CreateAxleConfig(lbam_model, minRightConfig, &minAxleRightConfig);
            pMaxAxleConfigTop->push_back(minAxleRightConfig);
         }

         if ( pMinAxleConfigBot )
         {
            AxleConfiguration minAxleRightConfig;
            CreateAxleConfig(lbam_model, minRightConfig, &minAxleRightConfig);
            pMinAxleConfigBot->push_back(minAxleRightConfig);
         }

         if ( pMinAxleConfigTop )
         {
            AxleConfiguration maxAxleRightConfig;
            CreateAxleConfig(lbam_model, maxRightConfig, &maxAxleRightConfig);
            pMinAxleConfigTop->push_back(maxAxleRightConfig);
         }
      }
      else
      {
         fLeftMax->GetResult(bot_stress_point_idx,&fBotMax);
         fLeftMax->GetResult(top_stress_point_idx,&fTopMin);
         fLeftMin->GetResult(bot_stress_point_idx,&fBotMin);
         fLeftMin->GetResult(top_stress_point_idx,&fTopMax);


         if ( pMaxAxleConfigBot )
         {
            AxleConfiguration maxAxleLeftConfig;
            CreateAxleConfig(lbam_model, maxLeftConfig, &maxAxleLeftConfig);
            pMaxAxleConfigBot->push_back(maxAxleLeftConfig);
         }

         if ( pMaxAxleConfigTop )
         {
            AxleConfiguration minAxleLeftConfig;
            CreateAxleConfig(lbam_model, minLeftConfig, &minAxleLeftConfig);
            pMaxAxleConfigTop->push_back(minAxleLeftConfig);
         }

         if ( pMinAxleConfigBot )
         {
            AxleConfiguration minAxleLeftConfig;
            CreateAxleConfig(lbam_model, minLeftConfig, &minAxleLeftConfig);
            pMinAxleConfigBot->push_back(minAxleLeftConfig);
         }

         if ( pMinAxleConfigTop )
         {
            AxleConfiguration maxAxleLeftConfig;
            CreateAxleConfig(lbam_model, maxLeftConfig, &maxAxleLeftConfig);
            pMinAxleConfigTop->push_back(maxAxleLeftConfig);
         }
      }
      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);
   }
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   USES_CONVERSION;

   pDmin->clear();
   pDmax->clear();

   if ( pMinAxleConfig )
      pMinAxleConfig->clear();

   if ( pMaxAxleConfig )
      pMaxAxleConfig->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetDy, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetDy, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);
   
   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 DyMaxLeft, DyMaxRight;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&DyMaxLeft, pMaxAxleConfig ? &maxLeftConfig  : NULL,
                                &DyMaxRight,pMaxAxleConfig ? &maxRightConfig : NULL);

      Float64 DyMinLeft, DyMinRight;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&DyMinLeft, pMinAxleConfig ? &minLeftConfig  : NULL,
                                &DyMinRight,pMinAxleConfig ? &minRightConfig : NULL);

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

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   USES_CONVERSION;

   pDmin->clear();
   pDmax->clear();

   if ( pMinAxleConfig )
      pMinAxleConfig->clear();

   if ( pMaxAxleConfig )
      pMaxAxleConfig->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetRz, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetRz, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 RzMaxLeft, RzMaxRight;
      CComPtr<ILiveLoadConfiguration> RzMaxConfig;
      maxResults->GetResult(idx,&RzMaxLeft,pMaxAxleConfig ? &RzMaxConfig : NULL,&RzMaxRight,NULL);

      Float64 RzMinLeft, RzMinRight;
      CComPtr<ILiveLoadConfiguration> RzMinConfig;
      minResults->GetResult(idx,&RzMinLeft,pMinAxleConfig ? &RzMinConfig : NULL,&RzMinRight,NULL);

      pDmin->push_back( RzMinLeft );
      pDmax->push_back( RzMaxLeft );

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

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces
//
sysSectionValue CAnalysisAgentImp::GetShear(LoadingCombination combo,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> V( GetShear(combo,intervalIdx,vPoi,type,bat) );

   ATLASSERT(V.size() == 1);

   return V[0];
}

Float64 CAnalysisAgentImp::GetMoment(LoadingCombination combo,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(combo,intervalIdx,vPoi,type,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(LoadingCombination combo,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(combo,intervalIdx,vPoi,type,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}


Float64 CAnalysisAgentImp::GetReaction(LoadingCombination combo,IntervalIndexType intervalIdx,PierIndexType pierIdx,const CGirderKey& girderKey,CombinationType comboType,pgsTypes::BridgeAnalysisType bat)
{
   USES_CONVERSION;

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IBridge,pBridge);

      CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx,girderKey);

      IntervalIndexType releaseIntervalIdx      = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType liveLoadIntervalIdx     = pIntervals->GetLiveLoadInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         return 0;
      }
      else if (releaseIntervalIdx <= intervalIdx && intervalIdx < erectSegmentIntervalIdx)
      {
         ValidateAnalysisModels( girderKey.girderIndex );
         return GetReactions( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), pierIdx, girderKey );
      }
      else
      {
         // Start by checking if the model exists
         ModelData* pModelData = 0;
         pModelData = GetModelData(girderKey.girderIndex);

         CComBSTR bstrLoadCase(GetLoadCaseName(combo));
         CComBSTR bstrStage(GetLBAMStageName(intervalIdx));

         ResultsSummationType resultsSummation = (comboType == ctIncremental ? rsIncremental : rsCumulative);

         ConfigureLBAMPoisForReactions(girderKey,pierIdx,intervalIdx,bat,intervalIdx < liveLoadIntervalIdx ? false : true);

         CComPtr<IResult3Ds> results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadCase,m_LBAMPoi,bstrStage,resultsSummation,&results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadCase,m_LBAMPoi,bstrStage,resultsSummation,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeReactions(bstrLoadCase,m_LBAMPoi,bstrStage,resultsSummation,&results);

         Float64 Fy = 0;
         CollectionIndexType nResults;
         results->get_Count(&nResults);
         for ( CollectionIndexType i = 0; i < nResults; i++ )
         {
            CComPtr<IResult3D> result;
            results->get_Item(i,&result);

            Float64 fy;
            result->get_Y(&fy);

            Fy += fy;
         }

         if ( combo == lcPS )
         {
            // Secondary effects are not in the LBAM. Get them here and add them to the reaction
            if ( resultsSummation == rsCumulative )
            {
               for ( IntervalIndexType i = erectSegmentIntervalIdx; i <= intervalIdx; i++ )
               {
                  Fy += GetReaction(i,pftSecondaryEffects,pierIdx,girderKey,bat,ctIncremental);
               }
            }
            else
            {
               Fy += GetReaction(intervalIdx,pftSecondaryEffects,pierIdx,girderKey,bat,ctIncremental);
            }
         }

         return Fy;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetStress(LoadingCombination combo,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,CombinationType type,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(combo,intervalIdx,vPoi,type,bat,topLocation,botLocation,&fTop,&fBot);
   
   ATLASSERT(fTop.size() == 1 && fBot.size() == 1);

   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetCombinedLiveLoadMoment(llType,intervalIdx,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1 && Mmax.size() == 1);
   *pMin = Mmin[0];
   *pMax = Mmax[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetCombinedLiveLoadShear(llType,intervalIdx,vPoi,bat,bIncludeImpact,&Vmin,&Vmax);

   ATLASSERT( Vmin.size() == 1 && Vmax.size() == 1 );
   *pVmin = Vmin[0];
   *pVmax = Vmax[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetCombinedLiveLoadDeflection(llType,intervalIdx,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT( Dmin.size() == 1 && Dmax.size() == 1 );
   *pDmin = Dmin[0];
   *pDmax = Dmax[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   USES_CONVERSION;

#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   ATLASSERT(pIntervals->GetLiveLoadInterval(girderKey) <= intervalIdx);
#endif

   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(girderKey.girderIndex);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

   ConfigureLBAMPoisForReactions(girderKey,pierIdx,intervalIdx,bat,true);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, vbIncludeImpact, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, vbIncludeImpact, VARIANT_FALSE, &minResults);

   CollectionIndexType nResults;
   maxResults->get_Count(&nResults);
   Float64 FyMax = 0;
   Float64 FyMin = 0;
   for ( CollectionIndexType i = 0; i < nResults; i++ )
   {
      Float64 fyMax;
      maxResults->GetResult(i,&fyMax,NULL);
      FyMax += fyMax;

      Float64 fyMin;
      minResults->GetResult(i,&fyMin,NULL);
      FyMin += fyMin;
   }

   *pRmin = FyMin;
   *pRmax = FyMax;
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   GetCombinedLiveLoadStress(llType,intervalIdx,vPoi,bat,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax);

   ATLASSERT( fTopMin.size() == 1 && fTopMax.size() == 1 && fBotMin.size() == 1 && fBotMax.size() == 1 );

   *pfTopMin = fTopMin[0];
   *pfTopMax = fTopMax[0];
   *pfBotMin = fBotMin[0];
   *pfBotMax = fBotMax[0];
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces2
//
void CAnalysisAgentImp::ComputeTimeDependentEffects(const CGirderKey& girderKey)
{
   // Getting the timestep loss results, causes the creep, shrinkage, and prestress forces
   // to be added to the LBAM model...
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      GET_IFACE(ILosses,pLosses);
      GET_IFACE(IBridge,pBridge);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         pgsPointOfInterest poi(CSegmentKey(grpIdx,girderKey.girderIndex,0),0.0);
         pLosses->GetLossDetails(poi);
      }
   }
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(LoadingCombination combo,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   USES_CONVERSION;

   //if combo is  lcCR, lcSH, or lcPS, need to do the time-step analysis because it adds loads to the LBAM
   if ( combo == lcCR || combo == lcSH || combo == lcPS )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   std::vector<sysSectionValue> section_results;

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx || (intervalIdx < erectionIntervalIdx && type == ctCumulative) )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            
            sysSectionValue fx,fy,mz;
            Float64 dx(0),dy(0),rz(0);
            if ( combo == lcDC )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            }

            section_results.push_back(fy);
         }
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         // interval is before there is anything... results are zero
         section_results.insert(section_results.begin(),vPoi.size(),sysSectionValue(0,0));
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            CollectionIndexType idx = iter - vPoi.begin();

            CComPtr<ISectionResult3D> result;
            results->get_Item(idx,&result);

            Float64 FyLeft, FyRight;
            result->get_YLeft(&FyLeft);
            result->get_YRight(&FyRight);


            // if the PS load combination is requested, add in the secondary effects of prestressing since they
            // are not part of the LBAM.
            if ( combo == lcPS )
            {
               const pgsPointOfInterest& poi = *iter;

               if ( rsType == rsCumulative )
               {
                  for ( IntervalIndexType i = erectionIntervalIdx; i <= intervalIdx; i++ )
                  {
                     sysSectionValue V = GetShear(i,pftSecondaryEffects,poi,bat,ctIncremental);
                     FyLeft  -= V.Left();
                     FyRight += V.Right();
                  }
               }
               else
               {
                  sysSectionValue V = GetShear(intervalIdx,pftSecondaryEffects,poi,bat,ctIncremental);
                  FyLeft  -= V.Left();
                  FyRight += V.Right();
               }
            }

            section_results.push_back(sysSectionValue(-FyLeft,FyRight));
         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }


   return section_results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(LoadingCombination combo,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   USES_CONVERSION;

   //if combo is  lcCR, lcSH, or lcPS, need to do the time-step analysis because it adds loads to the LBAM
   if ( combo == lcCR || combo == lcSH || combo == lcPS )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   std::vector<Float64> Mz;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx || (intervalIdx < erectionIntervalIdx && type == ctCumulative) )
      {
         // prestress is release or segment is placed into storage
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            sysSectionValue fx,fy,mz;
            Float64 dx(0),dy(0),rz(0);
            if ( combo == lcDC )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            }
            ATLASSERT(IsEqual(mz.Left(),-mz.Right(),0.0001));
            Mz.push_back(mz.Left());
         }
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         // before the segment is erected
         // this is just a time step interval so there aren't any loads applied
         Mz.insert(Mz.begin(),vPoi.size(),0.0);
      }
      else
      {
         // segment is erected into full bridge
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         GET_IFACE(IBridge,pBridge);

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            CollectionIndexType idx = iter - vPoi.begin();

            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            CComPtr<ISectionResult3D> result;
            results->get_Item(idx,&result);

            Float64 MzLeft;
            result->get_ZLeft(&MzLeft);

            // if the PS load combination is requested, add in the secondary effects of prestressing since they
            // are not part of the LBAM.
            if ( combo == lcPS )
            {
               if ( rsType == rsCumulative )
               {
                  for ( IntervalIndexType i = erectionIntervalIdx; i <= intervalIdx; i++ )
                  {
                     Float64 M = GetMoment(i,pftSecondaryEffects,poi,bat,ctIncremental);
                     MzLeft += M;
                  }
               }
               else
               {
                  Float64 M = GetMoment(intervalIdx,pftSecondaryEffects,poi,bat,ctIncremental);
                  MzLeft += M;
               }
            }

            Mz.push_back(MzLeft);
         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return Mz;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(LoadingCombination combo,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
#pragma Reminder("UPDATE: do elevation adjustment need to be applied to load combo Deflections?")

   USES_CONVERSION;

   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
      return GetTimeStepDeflection(combo,intervalIdx,vPoi,type,bat);

   //if combo is  lcCR, lcSH, or lcPS, need to do the time-step analysis because it adds loads to the LBAM
   if ( combo == lcCR || combo == lcSH || combo == lcPS )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx || (intervalIdx < erectionIntervalIdx && type == ctCumulative) )
      {
         std::vector<Float64> results;
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);
            sysSectionValue fx(0),fy(0),mz(0);
            Float64 dx(0),dy(0),rz(0);
            if ( combo == lcDC )
            {
               GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            }
            results.push_back(dy);
         }
         return results;
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);

         std::vector<Float64> vResults;
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            CollectionIndexType idx = iter - vPoi.begin();
            CComPtr<ISectionResult3D> result;
            results->get_Item(idx,&result);

            Float64 Dy, Dyr;
            result->get_YLeft(&Dy);
            result->get_YRight(&Dyr);

            ATLASSERT(IsEqual(Dy,Dyr));

            vResults.push_back(Dy);
         }
         return vResults;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetStress(LoadingCombination combo,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;

   // stress locations must be either both girder or both deck
   ATLASSERT( (IsGirderStressLocation(topLocation) && IsGirderStressLocation(botLocation))
              ||
              (IsDeckStressLocation(topLocation) && IsDeckStressLocation(botLocation)));

   //if combo is  lcCR, lcSH, or lcPS, need to do the time-step analysis because it adds loads to the LBAM
   if ( combo == lcCR || combo == lcSH || combo == lcPS )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey());
   }

   pfTop->clear();
   pfBot->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx || (intervalIdx < erectionIntervalIdx && type == ctCumulative) )
      {
         if ( IsGirderStressLocation(topLocation) )
         {
            std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
            std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
            for ( ; iter != end; iter++ )
            {
               const pgsPointOfInterest& poi = *iter;
               const CSegmentKey& segmentKey = poi.GetSegmentKey();

               ValidateAnalysisModels(segmentKey.girderIndex);

               Float64 top_results(0);
               Float64 bot_results(0);
               top_results = GetSectionStress( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), pgsTypes::TopGirder,    poi );
               bot_results = GetSectionStress( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), pgsTypes::BottomGirder, poi );
               pfTop->push_back(top_results);
               pfBot->push_back(bot_results);
            }
         }
         else
         {
            pfTop->insert(pfTop->begin(),vPoi.size(),0.0);
            pfBot->insert(pfBot->begin(),vPoi.size(),0.0);
         }
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         pfTop->insert(pfTop->begin(),vPoi.size(),0.0);
         pfBot->insert(pfBot->begin(),vPoi.size(),0.0);
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase( GetLoadCaseName(combo) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionStressResults> min_results, max_results, results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
         {
            pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &max_results);
            pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &min_results);
         }
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
         {
            pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &max_results);
            pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &min_results);
         }
         else
         {
            pModelData->pLoadCaseResponse[bat]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &results);
         }
      
         CollectionIndexType stress_point_index_top = GetStressPointIndex(topLocation);
         CollectionIndexType stress_point_index_bot = GetStressPointIndex(botLocation);

         GET_IFACE(ISectionProperties,pSectionProperties);

         GET_IFACE(IBridge,pBridge);
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            CollectionIndexType idx = iter - vPoi.begin();

            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            Float64 dist_from_start = poi.GetDistFromStart();

            Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

            Float64 fTop, fBot;

            if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            {
               CComPtr<ISectionStressResult> top_stresses;
               max_results->get_Item(idx,&top_stresses);

               CComPtr<ISectionStressResult> bot_stresses;
               min_results->get_Item(idx,&bot_stresses);

               if ( IsZero(dist_from_start - end_size) )
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

               if ( IsZero(dist_from_start - end_size) )
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

               if ( IsZero(dist_from_start - end_size) )
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

            // Adjust girder stress for release & storage
            if ( IsGirderStressLocation(topLocation) && combo == lcDC && !poi.HasAttribute(POI_CLOSURE) )
            {
               ValidateAnalysisModels(vPoi.front().GetSegmentKey().girderIndex);

               Float64 fReleaseTop = GetSectionStress( m_ReleaseModels, topLocation, poi );
               Float64 fReleaseBot = GetSectionStress( m_ReleaseModels, botLocation, poi );

               Float64 fStorageTop = GetSectionStress( m_StorageModels, topLocation, poi );
               Float64 fStorageBot = GetSectionStress( m_StorageModels, botLocation, poi );

               Float64 StopErection = pSectionProperties->GetS(erectionIntervalIdx,poi,topLocation);
               Float64 SbotErection = pSectionProperties->GetS(erectionIntervalIdx,poi,botLocation);
               Float64 StopRelease  = pSectionProperties->GetS(releaseIntervalIdx, poi,topLocation);
               Float64 SbotRelease  = pSectionProperties->GetS(releaseIntervalIdx, poi,botLocation);
               Float64 StopStorage  = pSectionProperties->GetS(storageIntervalIdx, poi,topLocation);
               Float64 SbotStorage  = pSectionProperties->GetS(storageIntervalIdx, poi,botLocation);

               fTop += fReleaseTop*(1-StopRelease/StopStorage) + fStorageTop*(1-StopStorage/StopErection);
               fBot += fReleaseBot*(1-SbotRelease/SbotStorage) + fStorageBot*(1-SbotStorage/SbotErection);
            }

            pfTop->push_back(fTop);
            pfBot->push_back(fBot);
         }
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax)
{
   USES_CONVERSION;

   pMmax->clear();
   pMmin->clear();

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      pMmax->insert(pMmax->begin(),vPoi.size(),0.0);
      pMmin->insert(pMmin->begin(),vPoi.size(),0.0);
      return;
   }


   ModelData* pModelData = UpdateLBAMPois(vPoi);


   CComBSTR bstrLoadCombo( GetLoadCombinationName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CollectionIndexType idx = iter - vPoi.begin();

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      Float64 dist_from_start = poi.GetDistFromStart();
      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      Float64 MzMaxLeft, MzMaxRight;
      maxResults->GetResult(idx,&MzMaxLeft,NULL,&MzMaxRight,NULL);

      Float64 MzMinLeft, MzMinRight;
      minResults->GetResult(idx,&MzMinLeft,NULL,&MzMinRight,NULL);

      if ( IsZero(dist_from_start - end_size) )
      {
         pMmin->push_back( -MzMinRight );
         pMmax->push_back( -MzMaxRight );
      }
      else
      {
         pMmin->push_back( MzMinLeft );
         pMmax->push_back( MzMaxLeft );
      }
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax)
{
   USES_CONVERSION;

   pVmax->clear();
   pVmin->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(llType) );
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
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, incImpact, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, incImpact, VARIANT_FALSE, &minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 FyMaxLeft, FyMaxRight;
      maxResults->GetResult(idx,&FyMaxLeft,NULL,&FyMaxRight,NULL);

      Float64 FyMinLeft, FyMinRight;
      minResults->GetResult(idx,&FyMinLeft,NULL,&FyMinRight,NULL);

      pVmin->push_back( sysSectionValue(-FyMaxLeft,FyMaxRight) );
      pVmax->push_back( sysSectionValue(-FyMinLeft,FyMinRight) );
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax)
{
   USES_CONVERSION;

   pDmax->clear();
   pDmin->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      CollectionIndexType idx = iter - vPoi.begin();

      Float64 DyMaxLeft, DyMaxRight;
      maxResults->GetResult(idx,&DyMaxLeft,NULL,&DyMaxRight,NULL);

      Float64 DyMinLeft, DyMinRight;
      minResults->GetResult(idx,&DyMinLeft,NULL,&DyMinRight,NULL);

      pDmin->push_back( DyMinLeft );
      pDmax->push_back( DyMaxLeft );
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax)
{
   USES_CONVERSION;

   // stress locations must be either both girder or both deck
   ATLASSERT( (IsGirderStressLocation(topLocation) && IsGirderStressLocation(botLocation))
              ||
              (IsDeckStressLocation(topLocation) && IsDeckStressLocation(botLocation)));

#pragma Reminder("UPDATE: need to deal with top/bottom deck stress due to live load")

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo( GetLoadCombinationName(llType) );
   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   CComPtr<ILoadCombinationStressResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);


   CollectionIndexType top_stress_point_index = GetStressPointIndex(topLocation);
   CollectionIndexType bot_stress_point_index = GetStressPointIndex(botLocation);

   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CollectionIndexType idx = iter - vPoi.begin();

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      Float64 dist_from_start = poi.GetDistFromStart();
      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      CComPtr<IStressResult> fLeftMax, fRightMax;
      maxResults->GetResult(idx,&fLeftMax,NULL,&fRightMax,NULL);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      minResults->GetResult(idx,&fLeftMin,NULL,&fRightMin,NULL);

      Float64 fBotMin, fBotMax, fTopMin, fTopMax;
      if ( IsZero(dist_from_start - end_size) )
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
         std::swap(fTopMax,fTopMin);

      if ( fBotMax < fBotMin )
         std::swap(fBotMax,fBotMin);

      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);
   }
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces
//
void CAnalysisAgentImp::GetShear(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetShear(ls,intervalIdx,vPoi,bat,&Vmin,&Vmax);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pMin = Vmin[0];
   *pMax = Vmax[0];
}

void CAnalysisAgentImp::GetMoment(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetMoment(ls,intervalIdx,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMin = Mmin[0];
   *pMax = Mmax[0];
}

void CAnalysisAgentImp::GetDeflection(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncPrestress,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetDeflection(ls,intervalIdx,vPoi,bIncPrestress,bat,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin[0];
   *pMax = Dmax[0];
}

void CAnalysisAgentImp::GetStress(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludePrestress,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Fmin, Fmax;
   GetStress(ls,intervalIdx,vPoi,loc,bIncludePrestress,bat,&Fmin,&Fmax);

   ATLASSERT(Fmin.size() == 1);
   ATLASSERT(Fmax.size() == 1);

   *pMin = Fmin[0];
   *pMax = Fmax[0];
}

Float64 CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState ls,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> M = GetSlabDesignMoment(ls,vPoi,bat);

   ATLASSERT(M.size() == vPoi.size());

   return M.front();
}

bool CAnalysisAgentImp::IsStrengthIIApplicable(const CGirderKey& girderKey)
{
   // If we have permit truck, we're golden
   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
   if (bPermit)
   {
      return true;
   }
   else
   {
      // last chance is if this girder has pedestrian load and it is turned on for permit states
      ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);
      if (ILiveLoads::PedDontApply != PermitPedLoad)
      {
         return HasPedestrianLoad(girderKey);
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces2
//
void CAnalysisAgentImp::GetMoment(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         pMin->insert(pMin->begin(),vPoi.size(),0.0);
         pMax->insert(pMax->begin(),vPoi.size(),0.0);
      }
      else if (releaseIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx)
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            ATLASSERT(CGirderKey(segmentKey) == CGirderKey(poi.GetSegmentKey())); // all poi should be from the same girder
   
            ValidateAnalysisModels(segmentKey.girderIndex);

            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            ATLASSERT(IsEqual(mz.Left(),-mz.Right(),0.0001));

            pMin->push_back( mz.Left() );
            pMax->push_back( mz.Left() );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
         VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );
         CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

         // Load factors for prestress effects
         Float64 gPSMin, gPSMax;
         if ( ::IsRatingLimitState(ls) )
         {
            GET_IFACE(IRatingSpecification,pRatingSpec);
            gPSMin = pRatingSpec->GetPrestressFactor(ls);
            gPSMax = gPSMin;
         }
         else
         {
            GET_IFACE(ILoadFactors,pILoadFactors);
            const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
            gPSMin = pLoadFactors->PSmin[ls];
            gPSMax = pLoadFactors->PSmax[ls];
         }

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            CollectionIndexType idx = iter - vPoi.begin();

            Float64 MzMaxLeft, MzMaxRight;
            maxResults->GetResult(idx,&MzMaxLeft,NULL,&MzMaxRight,NULL);
      
            Float64 MzMinLeft, MzMinRight;
            minResults->GetResult(idx,&MzMinLeft,NULL,&MzMinRight,NULL);

            // add in the secondary effects of prestressing since they
            // are not part of the LBAM.
            Float64 Mps = 0;
            for ( IntervalIndexType i = erectionIntervalIdx; i <= intervalIdx; i++ )
            {
               Mps += GetMoment(i,pftSecondaryEffects,poi,bat,ctIncremental);
            }
            MzMinLeft += gPSMin*Mps;
            MzMaxLeft += gPSMax*Mps;

            pMin->push_back( MzMinLeft );
            pMax->push_back( MzMaxLeft );

         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetShear(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         pMin->insert(pMin->begin(),vPoi.size(),sysSectionValue(0,0));
         pMax->insert(pMax->begin(),vPoi.size(),sysSectionValue(0,0));
      }
      else if ( releaseIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            ATLASSERT(CGirderKey(segmentKey) == CGirderKey(poi.GetSegmentKey())); // all poi should be from the same girder

            ValidateAnalysisModels(segmentKey.girderIndex);

            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            pMin->push_back( fy );
            pMax->push_back( fy );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
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
         IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
         VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

         CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

         // Load factors for prestress effects
         Float64 gPSMin, gPSMax;
         if ( ::IsRatingLimitState(ls) )
         {
            GET_IFACE(IRatingSpecification,pRatingSpec);
            gPSMin = pRatingSpec->GetPrestressFactor(ls);
            gPSMax = gPSMin;
         }
         else
         {
            GET_IFACE(ILoadFactors,pILoadFactors);
            const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
            gPSMin = pLoadFactors->PSmin[ls];
            gPSMax = pLoadFactors->PSmax[ls];
         }

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            CollectionIndexType idx = iter - vPoi.begin();

            Float64 FyMaxLeft, FyMaxRight;
            maxResults->GetResult(idx,&FyMaxLeft,NULL,&FyMaxRight,NULL);
      
            Float64 FyMinLeft, FyMinRight;
            minResults->GetResult(idx,&FyMinLeft,NULL,&FyMinRight,NULL);

            // add in the secondary effects of prestressing since they
            // are not part of the LBAM.
            sysSectionValue Vps(0,0);
            for ( IntervalIndexType i = erectionIntervalIdx; i <= intervalIdx; i++ )
            {
               Vps += GetShear(i,pftSecondaryEffects,poi,bat,ctIncremental);
            }
            FyMaxLeft  -= gPSMax*Vps.Left();
            FyMaxRight += gPSMax*Vps.Right();
            FyMinLeft  -= gPSMin*Vps.Left();
            FyMinRight += gPSMin*Vps.Right();

            pMin->push_back( sysSectionValue(-FyMaxLeft,FyMaxRight) );
            pMax->push_back( sysSectionValue(-FyMinLeft,FyMinRight) );
         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<Float64> CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey);

   GET_IFACE(IEventMap,pEventMap);
   GET_IFACE(IBridge,pBridge);

   std::vector<Float64> vMoment;

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bExcludeNoncompositeMoments = !pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);

   try
   {
      ModelData* pModelData = UpdateLBAMPois(vPoi);

      CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
      CComBSTR bstrStage( GetLBAMStageName(liveLoadIntervalIdx) );

      VARIANT_BOOL bIncludeLiveLoad = VARIANT_TRUE;
      CComPtr<ILoadCombinationSectionResults> minResults;
      pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE,  &minResults);

      // do this outside the loop since the values don't change
      Float64 gDCmin, gDCmax;
      if ( ::IsRatingLimitState(ls) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gDCmin = pRatingSpec->GetDeadLoadFactor(ls);
         gDCmax = gDCmin;
      }
      else
      {
         GET_IFACE(ILoadFactors,pLF);
         const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

         gDCmin = pLoadFactors->DCmin[ls];
         gDCmax = pLoadFactors->DCmax[ls];
      }

      std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; iter != end; iter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         ATLASSERT(CGirderKey(segmentKey) == CGirderKey(poi.GetSegmentKey()));

         CollectionIndexType idx = iter - vPoi.begin();

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

            // remove girder moment
            IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
            Float64 Mg = GetMoment(erectSegmentIntervalIdx,pftGirder,poi,bat,ctIncremental);
            MzMin -= gDC*Mg;

            // if not continuous when deck is cast, 
            // remove slab, slab panels, diaphragms, and shear key moment
            // they don't contribute to the negative moment for deck design
            SpanIndexType startSpanIdx, endSpanIdx;
            pBridge->GetSpansForSegment(segmentKey,&startSpanIdx,&endSpanIdx);

            PierIndexType prevPierIdx = startSpanIdx;
            PierIndexType nextPierIdx = endSpanIdx + 1;

            IntervalIndexType dummy, startPierContinuityIntervalIdx, endPierContinuityIntervalIdx;
            pIntervals->GetContinuityInterval(segmentKey,prevPierIdx,&dummy,&startPierContinuityIntervalIdx);
            pIntervals->GetContinuityInterval(segmentKey,nextPierIdx,&endPierContinuityIntervalIdx,&dummy);


            if ( startPierContinuityIntervalIdx == compositeDeckIntervalIdx && 
                 endPierContinuityIntervalIdx   == compositeDeckIntervalIdx )
            {
               Float64 Mconstruction = GetMoment(castDeckIntervalIdx, pftConstruction, poi, bat, ctIncremental);
               Float64 Mslab         = GetMoment(castDeckIntervalIdx, pftSlab,         poi, bat, ctIncremental);
               Float64 Mslab_pad     = GetMoment(castDeckIntervalIdx, pftSlabPad,      poi, bat, ctIncremental);
               Float64 Mslab_panel   = GetMoment(castDeckIntervalIdx, pftSlabPanel,    poi, bat, ctIncremental);
               Float64 Mdiaphragm    = GetMoment(castDeckIntervalIdx, pftDiaphragm,    poi, bat, ctIncremental);
               Float64 Mshear_key    = GetMoment(castDeckIntervalIdx, pftShearKey,     poi, bat, ctIncremental);

               MzMin -= gDC*(Mconstruction + Mslab + Mslab_pad + Mslab_panel + Mdiaphragm + Mshear_key);
            }

            // remove user dc moments
            Float64 Muser_dc = GetMoment(castDeckIntervalIdx, pftUserDC, poi, bat, ctIncremental);
            MzMin -= gDC*Muser_dc;
         }

#pragma Reminder("REVIEW: need to account for secondary prestress effects?")
         // Do secondary prestress effects need to be taken into account for slab moment? See GetMoment

         vMoment.push_back( MzMin );

      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return vMoment;
}

void CAnalysisAgentImp::GetDeflection(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bIncludePrestress,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
#pragma Reminder("UPDATE: do elevation adjustments need to be applied to limit state Deflections?")
   USES_CONVERSION;

   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
   {
//#pragma Reminder("UPDATE: using fake code here for testing")
//      GET_IFACE(IBridge,pBridge);
//      std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
//      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
//      for ( ; iter != end; iter++ )
//      {
//         const pgsPointOfInterest& poi = *iter;
//         CollectionIndexType idx = iter - vPoi.begin();
//
//         Float64 elevAdj = pBridge->GetElevationAdjustment(poi);
//         pMin->push_back(elevAdj);
//         pMax->push_back(elevAdj);
//      }

      GetTimeStepDeflection(ls,intervalIdx,vPoi,bIncludePrestress,bat,ctCumulative,pMin,pMax);

      GET_IFACE(IBridge,pBridge);

      std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      std::vector<Float64>::iterator minIter(pMin->begin());
      std::vector<Float64>::iterator maxIter(pMax->begin());
      for ( ; iter != end; iter++, minIter++, maxIter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         CollectionIndexType idx = iter - vPoi.begin();

#pragma Reminder("UPDATE: conditionally apply deflection adjustment")
         // only doing it for time-step analysis... do we need to do this for elastic analysis too?
         Float64 elevAdj = pBridge->GetElevationAdjustment(poi);

         (*minIter) += elevAdj;
         (*maxIter) += elevAdj;
      }

      return;
   }

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

#pragma Reminder("UPDATE: need to deal with the case of bIncludePrestress is true")
   ATLASSERT(bIncludePrestress == false);

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx)
      {
         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            ValidateAnalysisModels(segmentKey.girderIndex);

            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            pMin->push_back( dy );
            pMax->push_back( dy );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
         VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

         CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            CollectionIndexType idx = iter - vPoi.begin();

            Float64 DyMaxLeft, DyMaxRight;
            maxResults->GetResult(idx,&DyMaxLeft,NULL,&DyMaxRight,NULL);
      
            Float64 DyMinLeft, DyMinRight;
            minResults->GetResult(idx,&DyMinLeft,NULL,&DyMinRight,NULL);

            pMin->push_back( DyMinLeft );
            pMax->push_back( DyMaxLeft );
         }
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetStress(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc,bool bIncludePrestress,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
      GetStressTimeStep(ls,intervalIdx,vPoi,loc,bIncludePrestress,bat,pMin,pMax);
   else
      GetStressElastic(ls,intervalIdx,vPoi,loc,bIncludePrestress,bat,pMin,pMax);
}

void CAnalysisAgentImp::GetStressTimeStep(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pMin->clear();
   pMax->clear();

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gLLMin = pLoadFactors->LLIMmin[ls];
   Float64 gLLMax = pLoadFactors->LLIMmax[ls];
   Float64 gDCMin = pLoadFactors->DCmin[ls];
   Float64 gDCMax = pLoadFactors->DCmax[ls];
   Float64 gDWMin = pLoadFactors->DWmin[ls];
   Float64 gDWMax = pLoadFactors->DWmax[ls];
   Float64 gCRMax = pLoadFactors->CRmax[ls];
   Float64 gCRMin = pLoadFactors->CRmin[ls];
   Float64 gSHMax = pLoadFactors->SHmax[ls];
   Float64 gSHMin = pLoadFactors->SHmin[ls];
   Float64 gPSMax = pLoadFactors->PSmax[ls]; // this is for secondary effects due to PT
   Float64 gPSMin = pLoadFactors->PSmin[ls]; // this is for secondary effects due to PT

   // Use half prestress if Service IA or Fatigue I (See LRFD Table 5.9.4.2.1-1)
   Float64 gPS = (ls == pgsTypes::ServiceIA || ls == pgsTypes::FatigueI) ? 0.5 : 1.0;

   GET_IFACE(ILosses,pLosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      Float64 fDC(0), fDW(0), fCR(0), fSH(0), fRX(0), fPS(0), fPT(0), fSPS(0), fLLMin(0), fLLMax(0);
      const pgsPointOfInterest& poi(*iter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pConcreteElement = (IsGirderStressLocation(stressLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      pgsTypes::FaceType face = IsTopStressLocation(stressLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace;
      for ( int i = 0; i < 13; i++ )
      {
         ProductForceType pfType = (ProductForceType)i;
         if ( pfType == pftOverlay || pfType == pftUserDW )
         {
            // Product force is a DW load
            fDW += pConcreteElement->f[face][pfType][ctCumulative];
         }
         else if ( pfType == pftUserLLIM )
         {
            // Product force is a LL
            fLLMin += pConcreteElement->f[face][pfType][ctCumulative];
            fLLMax += pConcreteElement->f[face][pfType][ctCumulative];
         }
         else if ( pfType == pftSecondaryEffects )
         {
            fSPS += pConcreteElement->f[face][pfType][ctCumulative];
         }
         else
         {
            // Product force is a DC load
            fDC += pConcreteElement->f[face][pfType][ctCumulative];
         }
      }

      if ( bIncludePrestress )
      {
         fPS = pConcreteElement->f[face][pftPrestress            ][ctCumulative];
         fPT = pConcreteElement->f[face][pftPrimaryPostTensioning][ctCumulative];
      }

      fCR    = pConcreteElement->f[face][pftCreep][ctCumulative];
      fSH    = pConcreteElement->f[face][pftShrinkage][ctCumulative];
      fRX    = pConcreteElement->f[face][pftRelaxation][ctCumulative];

      fLLMin += pConcreteElement->fLLMin[face];
      fLLMax += pConcreteElement->fLLMax[face];


      Float64 fMin = gDCMin*fDC + gDWMin*fDW + gLLMin*fLLMin + gCRMin*(fCR+fRX) + gSHMin*fSH + gPSMin*fSPS + gPS*(fPS + fPT);
      Float64 fMax = gDCMax*fDC + gDWMax*fDW + gLLMax*fLLMax + gCRMax*(fCR+fRX) + gSHMax*fSH + gPSMax*fSPS + gPS*(fPS + fPT);

      fMin = IsZero(fMin) ? 0 : fMin;
      fMax = IsZero(fMax) ? 0 : fMax;

      pMin->push_back(fMin);
      pMax->push_back(fMax);
   }
}

void CAnalysisAgentImp::GetStressElastic(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(ILoadFactors,pLoadFactors);
      Float64 gammaDCmin = pLoadFactors->GetLoadFactors()->DCmin[ls];
      Float64 gammaDCmax = pLoadFactors->GetLoadFactors()->DCmax[ls];

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         // before the segment is erected, there aren't any force results except if the interval is release or storage
         pMin->insert(pMin->begin(),vPoi.size(),0.0);
         pMax->insert(pMax->begin(),vPoi.size(),0.0);
      }
      else
      {
         ModelData* pModelData = 0;


         VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

         GET_IFACE(IBridge,pBridge);
         GET_IFACE(ISectionProperties,pSectionProperties);

         CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );


         CComPtr<ILoadCombinationStressResults> maxResults, minResults;
         if ( erectionIntervalIdx <= intervalIdx )
         {
            pModelData = UpdateLBAMPois(vPoi);

            pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
            pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);
         }


         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            Float64 fMax(0.0), fMin(0.0);
            const pgsPointOfInterest& poi = *iter;
            ATLASSERT(CGirderKey(segmentKey) == CGirderKey(poi.GetSegmentKey()));

            Float64 dist_from_start = poi.GetDistFromStart();
            Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

            if ( intervalIdx < erectionIntervalIdx )
            {
               // if poi is not at a closure joint, get the results.
               // if poi is at a closure joint, the closure joint hasn't been cast yet so there aren't any results
               ValidateAnalysisModels(segmentKey.girderIndex);

               Float64 f = GetSectionStress( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), stressLocation, poi );
               fMin = gammaDCmin*f;
               fMax = gammaDCmax*f;
            }
            else
            {
               CollectionIndexType idx = iter - vPoi.begin();

               CComPtr<IStressResult> fLeftMax, fRightMax;
               maxResults->GetResult(idx,&fLeftMax,NULL,&fRightMax,NULL);
      
               CComPtr<IStressResult> fLeftMin, fRightMin;
               minResults->GetResult(idx,&fLeftMin,NULL,&fRightMin,NULL);

               CollectionIndexType stress_point_index = GetStressPointIndex(stressLocation);

               if ( IsZero(dist_from_start - end_size) )
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
                  std::swap(fMin,fMax);
            }

            if ( bIncludePrestress )
            {
               Float64 ps = GetStress(intervalIdx,poi,stressLocation);

               Float64 k;
               if (ls == pgsTypes::ServiceIA || ls == pgsTypes::FatigueI )
                  k = 0.5; // Use half prestress stress if service IA (See Tbl 5.9.4.2.1-1 2008 or before) or Fatige I (LRFD 5.5.3.1-2009)
               else
                  k = 1.0;

               fMin += k*ps;
               fMax += k*ps;

               Float64 pt = GetStress(intervalIdx,poi,stressLocation,ALL_DUCTS);
               fMin += k*pt;
               fMax += k*pt;
            }

            if ( compositeDeckIntervalIdx < intervalIdx )
            {
               Float64 ft_ss, fb_ss;
               GetDeckShrinkageStresses(poi,&ft_ss,&fb_ss);

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
         }
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetReaction(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
{
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IBridge,pBridge);

      CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx,girderKey);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);
      if (intervalIdx < releaseIntervalIdx )
      {
         *pMin = 0;
         *pMax = 0;
      }
      else if ( releaseIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx )
      {
         ValidateAnalysisModels(girderKey.girderIndex);

         *pMin = GetReactions( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), pierIdx, girderKey );
         *pMax = *pMin;
      }
      else
      {
         // Start by checking if the model exists
         ModelData* pModelData = 0;
         pModelData = GetModelData(girderKey.girderIndex);

         ConfigureLBAMPoisForReactions(girderKey,pierIdx,intervalIdx,bat,intervalIdx < liveLoadIntervalIdx ? false : true);

         CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);
         VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

         CComPtr<ILoadCombinationResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &minResults);

         Float64 FyMax = 0;
         Float64 FyMin = 0;
         CollectionIndexType nResults;
         maxResults->get_Count(&nResults);
         for ( CollectionIndexType i = 0; i < nResults; i++ )
         {
            Float64 fyMax;
            maxResults->GetResult(i,&fyMax,NULL);
            FyMax += fyMax;
      
            Float64 fyMin;
            minResults->GetResult(i,&fyMin,NULL);
            FyMin += fyMin;
         }

         // Load factors for prestress effects
         Float64 gPSMin, gPSMax;
         if ( ::IsRatingLimitState(ls) )
         {
            GET_IFACE(IRatingSpecification,pRatingSpec);
            gPSMin = pRatingSpec->GetPrestressFactor(ls);
            gPSMax = gPSMin;
         }
         else
         {
            GET_IFACE(ILoadFactors,pILoadFactors);
            const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
            gPSMin = pLoadFactors->PSmin[ls];
            gPSMax = pLoadFactors->PSmax[ls];
         }

         Float64 Rps = 0;
         for ( IntervalIndexType i = erectionIntervalIdx; i <= intervalIdx; i++ )
         {
            Rps += GetReaction(i,pftSecondaryEffects,pierIdx,girderKey,bat,ctIncremental);
         }

         FyMin += gPSMin*Rps;
         FyMax += gPSMax*Rps;

         *pMin = FyMin;
         *pMax = FyMax;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

///////////////////////////////////////////////////////
// IExternalLoading
bool CAnalysisAgentImp::CreateLoadGroup(LPCTSTR strLoadGroupName)
{
#pragma Reminder("UPDATE: need to create load case in release and storage models")
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GirderIndexType nGirders = 0;
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      nGirders = Max(nGirders,pGroup->GetGirderCount());
   }

   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      ModelData* pModel = GetModelData(gdrIdx);
      if ( pModel->m_Model) 
      {
         CComPtr<ILoadGroups> loadGroups;
         pModel->m_Model->get_LoadGroups(&loadGroups);
         HRESULT hr = AddLoadGroup(loadGroups,CComBSTR(strLoadGroupName),CComBSTR(strLoadGroupName));
         if ( FAILED(hr) )
            return false;
      }

      if ( pModel->m_ContinuousModel) 
      {
         CComPtr<ILoadGroups> loadGroups;
         pModel->m_ContinuousModel->get_LoadGroups(&loadGroups);
         HRESULT hr = AddLoadGroup(loadGroups,CComBSTR(strLoadGroupName),CComBSTR(strLoadGroupName));
         if ( FAILED(hr) )
            return false;
      }
   }

   return true;
}

bool CAnalysisAgentImp::AddLoadGroupToLoadingCombination(LPCTSTR strLoadGroupName,LoadingCombination lcCombo)
{
   // need to do this in simple span and/or continuous models
#pragma Reminder("IMPLEMENT")
   return false;
}

bool CAnalysisAgentImp::CreateLoad(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LoadType loadType,Float64 P,LPCTSTR strLoadGroupName)
{
   GirderIndexType gdrIdx = poi.GetSegmentKey().girderIndex;
   ModelData* pModel = GetModelData(gdrIdx);

   if ( pModel->m_Model )
   {
      CreateLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,poi,loadType,P,strLoadGroupName);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,poi,loadType,P,strLoadGroupName);
   }

   return true;
}

bool CAnalysisAgentImp::CreateInitialStrainLoad(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r,LPCTSTR strLoadGroupName)
{
   GirderIndexType gdrIdx = poi1.GetSegmentKey().girderIndex;
   ModelData* pModel = GetModelData(gdrIdx);

   if ( pModel->m_Model )
   {
      CreateInitialStrainLoad(pModel->m_Model,pModel->PoiMap,intervalIdx,poi1,poi2,e,r,strLoadGroupName);
   }

   if ( pModel->m_ContinuousModel )
   {
      CreateInitialStrainLoad(pModel->m_ContinuousModel,pModel->PoiMap,intervalIdx,poi1,poi2,e,r,strLoadGroupName);
   }

   return true;
}

Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,strLoadGroupName,vPoi,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results( GetShear(intervalIdx,strLoadGroupName,vPoi,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,strLoadGroupName,vPoi,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(intervalIdx,strLoadGroupName,vPoi,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,strLoadGroupName,vPoi,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetReaction(intervalIdx,strLoadGroupName,vPoi,bat) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectSegmentIntervalIdx && (intervalIdx != releaseIntervalIdx && intervalIdx != storageIntervalIdx))
      {
         // before the segment is erected, there aren't any force results except if the interval is release or storage
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;
      }
      else if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx)
      {
         ATLASSERT(false); // does this get called?
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;

#pragma Reminder("UPDATE: what load case is this?") // need to load this model and get results for restrain cases
         //std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         //std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         //for ( ; iter != end; iter++ )
         //{
         //   const pgsPointOfInterest& poi = *iter;
         //   const CSegmentKey& segmentKey = poi.GetSegmentKey();

         //   ValidateAnalysisModels(segmentKey.girderIndex);
         //   sysSectionValue fx,fy,mz;
         //   Float64 dx,dy,rz;
         //   GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
         //   ATLASSERT(IsEqual(fx.Left(),-fx.Right()));
         //   results.push_back( fx.Left() );
         //}
         //return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         GET_IFACE(IBridge,pBridge);

         CComBSTR bstrLoadGroup( strLoadGroupName );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            CollectionIndexType idx = iter - vPoi.begin();

            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
            Float64 product_location = poi.GetDistFromStart() - start_offset;

            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 PxLeft, PxRight;
            result->get_XLeft(&PxLeft);
            result->get_XRight(&PxRight);

            Float64 Px;
            if ( IsZero(product_location) )
               Px = -PxRight; // use right side result at start of span
            else
               Px = PxLeft; // use left side result at all other locations

            results.push_back(Px);
         }

         return results;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
#pragma Reminder("IMPLEMENT")
   ATLASSERT(false); // does this ever get called?
   std::vector<sysSectionValue> vResults;
   vResults.resize(vPoi.size());
   return vResults;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(vPoi.front().GetSegmentKey());
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx && (intervalIdx != releaseIntervalIdx && intervalIdx != storageIntervalIdx))
      {
         // before the segment is erected, there aren't any force results except if the interval is release or storage
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;
      }
      else if (intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx)
      {
         ATLASSERT(false); // does this get called
         std::vector<Float64> results;
         results.insert(results.begin(),vPoi.size(),0.0);
         return results;

#pragma Reminder("UPDATE: what load case is this?") // need to load this model and get results for restrain cases
         //std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         //std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         //for ( ; iter != end; iter++ )
         //{
         //   const pgsPointOfInterest& poi = *iter;
         //   const CSegmentKey& segmentKey = poi.GetSegmentKey();

         //   ValidateAnalysisModels(segmentKey.girderIndex);
         //   sysSectionValue fx,fy,mz;
         //   Float64 dx,dy,rz;
         //   GetSectionResults( (intervalIdx == releaseIntervalIdx ? m_ReleaseModels : m_StorageModels), g_lcidGirder, poi, &fx, &fy, &mz, &dx, &dy, &rz );
         //   ATLASSERT(IsEqual(mz.Left(),-mz.Right()));
         //   results.push_back( mz.Left() );
         //}
         //return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         GET_IFACE(IBridge,pBridge);

         CComBSTR bstrLoadGroup( strLoadGroupName );
         CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == pgsTypes::MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);
         else if ( bat == pgsTypes::MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            CollectionIndexType idx = iter - vPoi.begin();

            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
            Float64 product_location = poi.GetDistFromStart() - start_offset;

            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 MzLeft, MzRight;
            result->get_ZLeft(&MzLeft);
            result->get_ZRight(&MzRight);

            Float64 Mz;
            if ( IsZero(product_location) )
               Mz = -MzRight; // use right side result at start of span
            else
               Mz = MzLeft; // use left side result at all other locations

            results.push_back(Mz);
         }

         return results;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
#pragma Reminder("IMPLEMENT")
   ATLASSERT(false); // does this get called
   std::vector<Float64> results;
   results.resize(vPoi.size());
   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
#pragma Reminder("IMPLEMENT")
   ATLASSERT(false); // does this get called
   std::vector<Float64> results;
   results.resize(vPoi.size());
   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
#pragma Reminder("IMPLEMENT")
   ATLASSERT(false); // does this get called
   std::vector<Float64> results;
   results.resize(vPoi.size());
   return results;
}

///////////////////////////
void CAnalysisAgentImp::GetDesignStress(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   // Computes design-time stresses due to external loads
   ATLASSERT(IsGirderStressLocation(loc)); // expecting stress location to be on the girder

   // figure out which stage the girder loading is applied
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx           = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalintervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx          = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx     = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType trafficBarrierIntervalIdx    = pIntervals->GetInstallRailingSystemInterval(segmentKey);
   IntervalIndexType overlayIntervalIdx           = pIntervals->GetOverlayInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx          = pIntervals->GetLiveLoadInterval(segmentKey);

   if ( intervalIdx == releaseIntervalIdx )
   {
      GetStress(ls,intervalIdx,poi,loc,false,bat,pMin,pMax);
      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // can't use this method with strength limit states
   ATLASSERT( !IsStrengthLimitState(ls) );

   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Stc = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::TopGirder);
   Float64 Sbc = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::BottomGirder);

   Float64 Stop = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::TopGirder,   fcgdr);
   Float64 Sbot = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::BottomGirder,fcgdr);

   Float64 k_top = Stc/Stop; // scale factor that converts top stress of composite section to a section with this f'c
   Float64 k_bot = Sbc/Sbot; // scale factor that converts bot stress of composite section to a section with this f'c

   Float64 ftop1, ftop2, ftop3Min, ftop3Max;
   Float64 fbot1, fbot2, fbot3Min, fbot3Max;

   ftop1 = ftop2 = ftop3Min = ftop3Max = 0;
   fbot1 = fbot2 = fbot3Min = fbot3Max = 0;

   // Load Factors
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   Float64 dc = pLoadFactors->DCmax[ls];
   Float64 dw = pLoadFactors->DWmax[ls];
   Float64 ll = pLoadFactors->LLIMmax[ls];

   Float64 ft,fb;

   // Erect Segment Interval
   GetStress(erectSegmentIntervalIdx,pftGirder,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 = dc*ft;   fbot1 = dc*fb;

   if ( erectSegmentIntervalIdx <= intervalIdx && intervalIdx < castDeckIntervalIdx )
   {
      // if the interval under consideration is between erection and deck casting
      // the only load is the girder dead load (see GetStress above)... get the stress and return
      if ( loc == pgsTypes::TopGirder )
      {
         *pMin = ftop1;
         *pMax = ftop1;
      }
      else
      {
         *pMin = fbot1;
         *pMax = fbot1;
      }

      if (*pMax < *pMin )
         std::swap(*pMin,*pMax);

      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // Bridge Site Stage 1
   GetStress(castDeckIntervalIdx,pftConstruction,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftSlab,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftSlabPad,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftSlabPanel,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetDesignSlabPadStressAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftDiaphragm,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftShearKey,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftUserDC,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dc*ft;   
   fbot1 += dc*fb;

   GetStress(castDeckIntervalIdx,pftUserDW,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop1 += dw*ft;   
   fbot1 += dw*fb;

   if ( intervalIdx == castDeckIntervalIdx )
   {
      if ( loc == pgsTypes::TopGirder )
      {
         *pMin = ftop1;
         *pMax = ftop1;
      }
      else
      {
         *pMin = fbot1;
         *pMax = fbot1;
      }

      if (*pMax < *pMin )
         std::swap(*pMin,*pMax);

      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // Bridge Site Stage 2
   GetStress(trafficBarrierIntervalIdx,pftTrafficBarrier,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop2 = dc*k_top*ft;   
   fbot2 = dc*k_bot*fb;

   GetStress(trafficBarrierIntervalIdx,pftSidewalk,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop2 += dc*k_top*ft;   
   fbot2 += dc*k_bot*fb;

   GET_IFACE(IBridge,pBridge);

   if ( overlayIntervalIdx != INVALID_INDEX && !pBridge->IsFutureOverlay() )
   {
      GetStress(overlayIntervalIdx,pftOverlay,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dw*k_top*ft;   
      fbot2 += dw*k_bot*fb;
   }

   GetStress(compositeDeckIntervalIdx,pftUserDC,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop2 += dc*k_top*ft;   
   fbot2 += dc*k_bot*fb;

   GetStress(compositeDeckIntervalIdx,pftUserDW,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop2 += dw*k_top*ft;   
   fbot2 += dw*k_bot*fb;

   if ( intervalIdx == compositeDeckIntervalIdx )
   {
      if ( loc == pgsTypes::TopGirder )
      {
         *pMin = ftop1 + ftop2;
         *pMax = ftop1 + ftop2;
      }
      else
      {
         *pMin = fbot1 + fbot2;
         *pMax = fbot1 + fbot2;
      }

      if (*pMax < *pMin )
         std::swap(*pMin,*pMax);

      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // Bridge Site Stage 3
   if (overlayIntervalIdx != INVALID_INDEX && pBridge->IsFutureOverlay() )
   {
      GetStress(overlayIntervalIdx,pftOverlay,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop3Min = ftop3Max = dw*k_top*ft;   
      fbot3Min = fbot3Max = dw*k_bot*fb;   
   }
   else
   {
      ftop3Min = ftop3Max = 0.0;   
      fbot3Min = fbot3Max = 0.0;   
   }

   GET_IFACE(ISegmentData,pSegmentData);
   const CGirderMaterial* pGirderMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

   Float64 fc_lldf = fcgdr;
   if ( pGirderMaterial->Concrete.bUserEc )
   {
      fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Concrete.Ec, pGirderMaterial->Concrete.StrengthDensity );
   }


   Float64 ftMin,ftMax,fbMin,fbMax;
   if ( ls == pgsTypes::FatigueI )
   {
      GetLiveLoadStress(pgsTypes::lltFatigue, liveLoadIntervalIdx,poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
   }
   else
   {
      GetLiveLoadStress(pgsTypes::lltDesign,  liveLoadIntervalIdx,poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
   }

   ftop3Min += ll*k_top*ftMin;   
   fbot3Min += ll*k_bot*fbMin;

   ftop3Max += ll*k_top*ftMax;   
   fbot3Max += ll*k_bot*fbMax;

   GetLiveLoadStress(pgsTypes::lltPedestrian,  liveLoadIntervalIdx,poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);

   ftop3Min += ll*k_top*ftMin;   
   fbot3Min += ll*k_bot*fbMin;

   ftop3Max += ll*k_top*ftMax;   
   fbot3Max += ll*k_bot*fbMax;

   GetStress(liveLoadIntervalIdx,pftUserLLIM,poi,bat,ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
   ftop3Min += ll*k_top*ft;   
   fbot3Min += ll*k_bot*fb;

   ftop3Max += ll*k_top*ft;   
   fbot3Max += ll*k_bot*fb;

   // slab shrinkage stresses
   Float64 ft_ss, fb_ss;
   GetDeckShrinkageStresses(poi,&ft_ss,&fb_ss);

   if ( loc == pgsTypes::TopGirder )
   {
      *pMin = ftop1 + ftop2 + ftop3Min + ft_ss;
      *pMax = ftop1 + ftop2 + ftop3Max + ft_ss;
   }
   else
   {
      *pMin = fbot1 + fbot2 + fbot3Min + fb_ss;
      *pMax = fbot1 + fbot2 + fbot3Max + fb_ss;
   }

   if (*pMax < *pMin )
   {
      std::swap(*pMin,*pMax);
   }

   *pMax = (IsZero(*pMax) ? 0 : *pMax);
   *pMin = (IsZero(*pMin) ? 0 : *pMin);
}

std::vector<Float64> CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation)
{
   std::vector<Float64> stresses;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      Float64 stress = GetStress(intervalIdx,poi,stressLocation);
      stresses.push_back(stress);
   }

   return stresses;
}

//////////////////////////////////////////////////////////////////////
// IPosttensionStresses
//
Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,DuctIndexType ductIdx)
{
   const CGirderKey& girderKey(poi.GetSegmentKey());
   
   GET_IFACE(ITendonGeometry,    pTendonGeometry);
   GET_IFACE(IPosttensionForce,  pPTForce);
   GET_IFACE(IIntervals,         pIntervals);

   DuctIndexType nDucts = pTendonGeometry->GetDuctCount(girderKey);
   if ( nDucts == 0 )
      return 0;

   DuctIndexType firstDuctIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastDuctIdx  = (ductIdx == ALL_DUCTS ? nDucts-1 : firstDuctIdx);

   Float64 stress = 0;
   for ( DuctIndexType idx = firstDuctIdx; idx <= lastDuctIdx; idx++ )
   {
      IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,idx);

      Float64 e = pTendonGeometry->GetEccentricity(ptIntervalIdx,poi,idx);
      Float64 P = pPTForce->GetTendonForce(poi,intervalIdx,pgsTypes::End,idx);

      Float64 f = 0;
      if ( intervalIdx < ptIntervalIdx )
         f = 0;
      else
         f = GetStress(ptIntervalIdx,poi,stressLocation,P,e);

      stress += f;
   }

   return stress;
}

std::vector<Float64> CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,DuctIndexType ductIdx)
{
   std::vector<Float64> stresses;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      Float64 stress = GetStress(intervalIdx,poi,stressLocation,ductIdx);
      stresses.push_back(stress);
   }

   return stresses;
}

//////////////////////////////////////////////////////////////////////
void CAnalysisAgentImp::GetConcurrentShear(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   ATLASSERT(intervalIdx != releaseIntervalIdx); // this method only works for bridge site stages
#endif

   try
   {
      // Start by checking if the model exists
      ModelData* pModelData = 0;
      pModelData = GetModelData( segmentKey.girderIndex );

      PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
      if ( poi_id == INVALID_ID )
      {
         poi_id = AddPointOfInterest( pModelData, poi );
         ATLASSERT( 0 <= poi_id ); // if this fires, the poi wasn't added... WHY???
      }

      m_LBAMPoi->Clear();
      m_LBAMPoi->Add(poi_id);

      CComBSTR bstrLoadCombo( GetLoadCombinationName(ls) );
      CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

      CComPtr<ILoadCombinations> loadCombos;
      if ( pModelData->m_Model )
         pModelData->m_Model->get_LoadCombinations(&loadCombos) ;
      else
         pModelData->m_ContinuousModel->get_LoadCombinations(&loadCombos);

      CComPtr<ILoadCombination> loadCombo;
      loadCombos->Find(bstrLoadCombo,&loadCombo);
      Float64 gLLmin, gLLmax;
      loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
      if ( gLLmin < 0 || gLLmax < 0 )
      {
         ATLASSERT( ::IsRatingLimitState(ls) ); // this can only happen for ratings
      }

      // Get the Max/Min moments
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
      VARIANT_BOOL bIncludeLiveLoad = (liveLoadIntervalIdx <= intervalIdx ? VARIANT_TRUE : VARIANT_FALSE );

      CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
      pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE, &maxResults);
      pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE, &minResults);

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
         pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigLeft, &maxShearsLeft);
         pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigRight,&maxShearsRight);
         pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigLeft, &minShearsLeft);
         pModelData->pConcurrentComboResponse[bat]->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigRight,&minShearsRight);
      }
      else
      {
         CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(pModelData->pLoadComboResponse[bat]);
         concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigLeft, &maxShearsLeft);
         concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, maxResultConfigRight,&maxShearsRight);
         concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigLeft, &minShearsLeft);
         concurrent_response->ComputeForces(m_LBAMPoi, bstrStage, roMember, minResultConfigRight,&minShearsRight);
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
      
      *pMax = sysSectionValue(-FyMaxLeft,FyMaxRight); // shear concurrent with Max moment
      *pMin = sysSectionValue(-FyMinLeft,FyMinRight); // shear concurrent with Min moment
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetViMmax(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax)
{
   GET_IFACE(ISpecification,pSpec);

   Float64 Mu_max, Mu_min;
   sysSectionValue Vi_min, Vi_max;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 Mmin,Mmax;
      sysSectionValue Vimin, Vimax;

      GetMoment( ls, intervalIdx, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax );
      GetConcurrentShear(  ls, intervalIdx, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_max = Mmax;
      Vi_max = Vimax;

      GetMoment( ls, intervalIdx, poi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax );
      GetConcurrentShear(  ls, intervalIdx, poi, pgsTypes::MinSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_min = Mmin;
      Vi_min = Vimin;
   }
   else
   {
      pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      GetMoment( ls, intervalIdx, poi, bat, &Mu_min, &Mu_max );
      GetConcurrentShear(  ls, intervalIdx, poi, bat, &Vi_min, &Vi_max );
   }

   Mu_max = IsZero(Mu_max) ? 0 : Mu_max;
   Mu_min = IsZero(Mu_min) ? 0 : Mu_min;

   // driving moment is the one with the greater magnitude
   Float64 Mu = Max(fabs(Mu_max),fabs(Mu_min));

   Float64 MuSign = 1;
   if ( IsEqual(Mu,fabs(Mu_max)) )
      MuSign = BinarySign(Mu_max);
   else
      MuSign = BinarySign(Mu_min);

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

/////////////////////////////////////////////////////////////////////////////
// ICamber
//
Uint32 CAnalysisAgentImp::GetCreepMethod()
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   return pSpecEntry->GetCreepMethod();
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,creepPeriod,constructionRate);
   return details.Ct;
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,config,creepPeriod,constructionRate);
   return details.Ct;
}

CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details;

   GET_IFACE(IEnvironment,pEnvironment);

   GET_IFACE(ISectionProperties,pSectProp);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // if fc < 0 use current fc girder
   LoadingEvent le = GetLoadingEvent(creepPeriod);
   Float64 fc = GetConcreteStrengthAtTimeOfLoading(config,le);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   details.Method = pSpecEntry->GetCreepMethod();
   ATLASSERT(details.Method == CREEP_LRFD); // only supporting LRFD method... the old WSDOT method is out

   details.Spec = (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ) ? CREEP_SPEC_PRE_2005 : CREEP_SPEC_2005;

   if ( details.Spec == CREEP_SPEC_PRE_2005 )
   {
      //  LRFD 3rd edition and earlier
      try
      {
         lrfdCreepCoefficient cc;
         cc.SetRelHumidity( pEnvironment->GetRelHumidity() );
         cc.SetSurfaceArea( pSectProp->GetSurfaceArea(segmentKey) );
         cc.SetVolume( pSectProp->GetVolume(segmentKey) );
         cc.SetFc(fc);

         switch( creepPeriod )
         {
            case cpReleaseToDiaphragm:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               break;

            case cpReleaseToDeck:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpReleaseToFinal:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDiaphragmToDeck:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity(   constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpDiaphragmToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDeckToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            default:
               ATLASSERT(false);
         }

         details.ti           = cc.GetAdjustedInitialAge();
         details.t            = cc.GetMaturity();
         details.Fc           = cc.GetFc();
         details.H            = cc.GetRelHumidity();
         details.kf           = cc.GetKf();
         details.kc           = cc.GetKc();
         details.VSratio      = cc.GetVolume() / cc.GetSurfaceArea();
         details.CuringMethod = cc.GetCuringMethod();

         details.Ct           = cc.GetCreepCoefficient();
      }
#if defined _DEBUG
      catch( lrfdXCreepCoefficient& ex )
#else
      catch( lrfdXCreepCoefficient& /*ex*/ )
#endif // _DEBUG
      {
         ATLASSERT( ex.GetReason() == lrfdXCreepCoefficient::VSRatio );

         GET_IFACE(IEAFStatusCenter,pStatusCenter);

         std::_tstring strMsg(_T("V/S Ratio exceeds maximum value per C5.4.2.3.2. Use a different method for estimating creep"));
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(segmentKey,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }
   else
   {
      // LRFD 3rd edition with 2005 interims and later
      try
      {
         lrfdCreepCoefficient2005 cc;
         cc.SetRelHumidity( pEnvironment->GetRelHumidity() );
         cc.SetSurfaceArea( pSectProp->GetSurfaceArea(segmentKey) );
         cc.SetVolume( pSectProp->GetVolume(segmentKey) );
         cc.SetFc(fc);

         cc.SetCuringMethodTimeAdjustmentFactor(pSpecEntry->GetCuringMethodTimeAdjustmentFactor());

         GET_IFACE(IMaterials,pMaterial);
         cc.SetK1( pMaterial->GetSegmentCreepK1(segmentKey) );
         cc.SetK2( pMaterial->GetSegmentCreepK2(segmentKey) );


         switch( creepPeriod )
         {
            case cpReleaseToDiaphragm:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               break;

            case cpReleaseToDeck:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpReleaseToFinal:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDiaphragmToDeck:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity(  (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max()) - cc.GetInitialAge());
               break;

            case cpDiaphragmToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDeckToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            default:
               ATLASSERT(false);
         }

         details.ti           = cc.GetAdjustedInitialAge();
         details.t            = cc.GetMaturity();
         details.Fc           = cc.GetFc();
         details.H            = cc.GetRelHumidity();
         details.kvs          = cc.GetKvs();
         details.khc          = cc.GetKhc();
         details.kf           = cc.GetKf();
         details.ktd          = cc.GetKtd();
         details.VSratio      = cc.GetVolume() / cc.GetSurfaceArea();
         details.CuringMethod = cc.GetCuringMethod();
         details.K1           = cc.GetK1();
         details.K2           = cc.GetK2();

         details.Ct           = cc.GetCreepCoefficient();
      }
#if defined _DEBUG
      catch( lrfdXCreepCoefficient& ex )
#else
      catch( lrfdXCreepCoefficient& /*ex*/ )
#endif // _DEBUG
      {
            ATLASSERT( ex.GetReason() == lrfdXCreepCoefficient::VSRatio );

         GET_IFACE(IEAFStatusCenter,pStatusCenter);

         std::_tstring strMsg(_T("V/S Ratio exceeds maximum value per C5.4.2.3.2. Use a different method for estimating creep"));
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(segmentKey,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }

   return details;
}


CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate)
{
   std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS>::iterator found = m_CreepCoefficientDetails[constructionRate][creepPeriod].find(segmentKey);
   if ( found != m_CreepCoefficientDetails[constructionRate][creepPeriod].end() )
   {
      return (*found).second;
   }

   GET_IFACE(IPointOfInterest, IPoi);
   std::vector<pgsPointOfInterest> pois( IPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN,POIFIND_AND) );
   ATLASSERT(pois.size() == 1);
   pgsPointOfInterest poi = pois[0];

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   CREEPCOEFFICIENTDETAILS ccd = GetCreepCoefficientDetails(segmentKey,config,creepPeriod,constructionRate);
   m_CreepCoefficientDetails[constructionRate][creepPeriod].insert(std::make_pair(segmentKey,ccd));
   return ccd;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetPrestressDeflection(poi,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetPrestressDeflection(poi,config,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetInitialTempPrestressDeflection(poi,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetInitialTempPrestressDeflection(poi,config,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetReleaseTempPrestressDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetReleaseTempPrestressDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate)
{
   Float64 dy,rz;
   GetCreepDeflection(poi,creepPeriod,constructionRate,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   Float64 dy,rz;
   GetCreepDeflection(poi,config,creepPeriod,constructionRate,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDeckDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDeckDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) 
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetScreedCamber(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetScreedCamber(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model         = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   Float64 Dy, Rz;
   GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&Dy,&Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);
   
   Float64 Dy, Rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&Dy, &Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model         = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   Float64 dy,rz;
   GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);
   
   Float64 dy,rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   return GetSidlDeflection(poi,config);
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MinSimpleContinuousEnvelope);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval(segmentKey);

   // NOTE: No need to validate camber models
   Float64 delta_trafficbarrier  = 0;
   Float64 delta_sidewalk        = 0;
   Float64 delta_overlay         = 0;
   Float64 delta_diaphragm       = 0;
   Float64 delta_user1           = 0;
   Float64 delta_user2           = 0;

   // adjustment factor for fcgdr that is different that current value
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,compositeDeckIntervalIdx);

   delta_diaphragm      = GetDiaphragmDeflection(poi,config);
   delta_trafficbarrier = k2*GetDeflection(compositeDeckIntervalIdx, pftTrafficBarrier, poi, bat, ctIncremental);
   delta_sidewalk       = k2*GetDeflection(compositeDeckIntervalIdx, pftSidewalk,       poi, bat, ctIncremental);

#pragma Reminder("UPDATE: need user load deflection for all intervals from deck casting to the end")
   delta_user1          = GetUserLoadDeflection(castDeckIntervalIdx, poi, config);
   delta_user2          = GetUserLoadDeflection(compositeDeckIntervalIdx, poi, config);

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
      delta_overlay = k2*GetDeflection(overlayIntervalIdx,pftOverlay,poi,bat, ctIncremental);

   Float64 dSIDL;
   bool bTempStrands = (0 < config.PrestressConfig.GetNStrands(pgsTypes::Temporary) && 
                        config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   if ( bTempStrands )
   {
      dSIDL = delta_trafficbarrier + delta_sidewalk + delta_overlay + delta_user1 + delta_user2;
   }
   else
   {
      // for SIP decks, diaphagms are applied before the cast portion of the slab so they don't apply to screed camber
      if ( deckType == pgsTypes::sdtCompositeSIP )
         delta_diaphragm = 0;

      dSIDL = delta_trafficbarrier + delta_sidewalk + delta_overlay + delta_user1 + delta_user2 + delta_diaphragm;
   }

   return dSIDL;
}

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model            = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData initTempModel    = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData releaseTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   Float64 Dy, Rz;
   GetDCamberForGirderSchedule(poi,model,initTempModel,releaseTempModel,time,&Dy,&Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1, tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);

   Float64 Dy, Rz;
   GetDCamberForGirderSchedule(poi,config,initModel,tempModel1, tempModel2, time, &Dy, &Rz);
   return Dy;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GDRCONFIG dummy_config;

   CamberModelData model  = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData model1 = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData model2 = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);
   GetCreepDeflection(poi,false,dummy_config,model,model1,model2, creepPeriod, constructionRate, pDy, pRz);
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model;
   BuildCamberModel(segmentKey,true,config,&model);

   CamberModelData model1,model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);
   
   GetCreepDeflection(poi,true,config,model,model1,model2, creepPeriod, constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MinSimpleContinuousEnvelope);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType lastIntervalIdx     = pIntervals->GetIntervalCount(segmentKey)-1;

   // final deflections
   Float64 DC = GetDeflection(lcDC,lastIntervalIdx,poi,ctCumulative,bat);
   Float64 DW = GetDeflection(lcDW,lastIntervalIdx,poi,ctCumulative,bat);
   Float64 CR = GetDeflection(lcCR,lastIntervalIdx,poi,ctCumulative,bat);
   Float64 SH = GetDeflection(lcSH,lastIntervalIdx,poi,ctCumulative,bat);
   Float64 PS = GetDeflection(lcPS,lastIntervalIdx,poi,ctCumulative,bat);
   Float64 Dfinal = DC + DW + CR + SH + PS;

   // deflections just before slab casting
   DC = GetDeflection(lcDC,castDeckIntervalIdx-1,poi,ctCumulative,bat);
   DW = GetDeflection(lcDW,castDeckIntervalIdx-1,poi,ctCumulative,bat);
   CR = GetDeflection(lcCR,castDeckIntervalIdx-1,poi,ctCumulative,bat);
   SH = GetDeflection(lcSH,castDeckIntervalIdx-1,poi,ctCumulative,bat);
   PS = GetDeflection(lcPS,castDeckIntervalIdx-1,poi,ctCumulative,bat);
   Float64 DdeckCasting = DC + DW + CR + SH + PS;

   Float64 D = Dfinal - DdeckCasting;

   *pDy = -1.0*D;

#pragma Reminder("UPDATE: need to compute rotations too")
   *pRz = 0;
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // this version is only for PGSuper design mode
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MinSimpleContinuousEnvelope);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(segmentKey);

   // NOTE: No need to validate camber models
   Float64 Dslab            = 0;
   Float64 Dslab_pad        = 0;
   Float64 Dtrafficbarrier  = 0;
   Float64 Dsidewalk        = 0;
   Float64 Doverlay         = 0;
   Float64 Ddiaphragm       = 0;
   Float64 Duser            = 0;

   Float64 Rslab            = 0;
   Float64 Rslab_pad        = 0;
   Float64 Rtrafficbarrier  = 0;
   Float64 Rsidewalk        = 0;
   Float64 Roverlay         = 0;
   Float64 Rdiaphragm       = 0;
   Float64 Ruser            = 0;

   // deflections are computed based on current parameters for the bridge.
   // E in the config object may be different than E used to compute the deflection.
   // The deflection adjustment factor accounts for the differences in E.
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,railingSystemIntervalIdx);
   GetDeckDeflection(poi,config,&Dslab,&Rslab); 
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad,&Rslab_pad);
   GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);

   
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx, pftTrafficBarrier, poi, bat, ctIncremental);
   Rtrafficbarrier = k2*GetRotation(railingSystemIntervalIdx, pftTrafficBarrier, poi, bat, ctIncremental);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx, pftSidewalk, poi, bat, ctIncremental);
   Rsidewalk = k2*GetRotation(railingSystemIntervalIdx, pftSidewalk, poi, bat, ctIncremental);

   // Only get deflections for user defined loads that occur during deck placement and later
   std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(poi.GetSegmentKey()));
   vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(),vUserLoadIntervals.end(),std::bind2nd(std::less<IntervalIndexType>(),castDeckIntervalIdx)),vUserLoadIntervals.end());
   std::vector<IntervalIndexType>::iterator userLoadIter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator userLoadIterEnd(vUserLoadIntervals.end());
   for ( ; userLoadIter != userLoadIterEnd; userLoadIter++ )
   {
      IntervalIndexType intervalIdx = *userLoadIter;
      Float64 D,R;

      k2 = GetDeflectionAdjustmentFactor(poi,config,intervalIdx);
      GetUserLoadDeflection(intervalIdx,poi,config,&D,&R);

      Duser += k2*D;
      Ruser += k2*R;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      k2 = GetDeflectionAdjustmentFactor(poi,config,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pftOverlay,poi,bat, ctIncremental);
      Roverlay = k2*GetRotation(overlayIntervalIdx,pftOverlay,poi,bat, ctIncremental);
   }

   bool bTempStrands = (0 < config.PrestressConfig.GetNStrands(pgsTypes::Temporary) &&  // there are temp strands
                   config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) // and they are not post-tensioned before shipping
                   ? true : false;

   if ( bTempStrands )
   {
      *pDy = Dslab + Dslab_pad + Dtrafficbarrier + Dsidewalk + Doverlay + Duser;
      *pRz = Rslab + Rslab_pad + Rtrafficbarrier + Rsidewalk + Roverlay + Ruser;
   }
   else
   {
      // for SIP decks, diaphagms are applied before the cast portion of the slab so they don't apply to screed camber
      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         Ddiaphragm = 0;
         Rdiaphragm = 0;
      }

      *pDy = Dslab + Dslab_pad + Dtrafficbarrier + Dsidewalk + Doverlay + Duser + Ddiaphragm;
      *pRz = Rslab + Rslab_pad + Rtrafficbarrier + Rsidewalk + Roverlay + Ruser + Rdiaphragm;
   }

   // Switch the sign. Negative deflection creates positive screed camber
   (*pDy) *= -1;
   (*pRz) *= -1;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   Float64 dy_slab = GetDeflection(castDeckIntervalIdx,pftSlab,poi,bat, ctIncremental);
   Float64 rz_slab = GetRotation(castDeckIntervalIdx,pftSlab,poi,bat, ctIncremental);

   Float64 dy_slab_pad = GetDeflection(castDeckIntervalIdx,pftSlabPad,poi,bat, ctIncremental);
   Float64 rz_slab_pad = GetRotation(castDeckIntervalIdx,pftSlabPad,poi,bat, ctIncremental);

   *pDy = dy_slab + dy_slab_pad;
   *pRz = rz_slab + rz_slab_pad;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   GetDeckDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

#pragma Reminder("UPDATE: need a new interval type for deck panel placement?")
   // assuming panels are placed at same time deck is cast
   // bridge model doesn't support the deck panel stage idea
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   *pDy = GetDeflection(castDeckIntervalIdx,pftSlabPanel,poi,bat, ctIncremental);
   *pRz = GetRotation(castDeckIntervalIdx,pftSlabPanel,poi,bat, ctIncremental);
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

#pragma Reminder("UPDATE: need a new interval type for deck panel placement?")
   // assuming panels are placed at same time deck is cast
   // bridge model doesn't support the deck panel stage idea
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   GetDeckPanelDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   *pDy = GetDeflection(castDeckIntervalIdx,pftDiaphragm,poi,bat, ctIncremental);
   *pRz = GetRotation(castDeckIntervalIdx,pftDiaphragm,poi,bat, ctIncremental);
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   GetDiaphragmDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz) 
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 Ddc = GetDeflection(intervalIdx,pftUserDC, poi,bat, ctIncremental);
   Float64 Ddw = GetDeflection(intervalIdx,pftUserDW, poi,bat, ctIncremental);

   Float64 Rdc = GetRotation(intervalIdx,pftUserDC, poi,bat, ctIncremental);
   Float64 Rdw = GetRotation(intervalIdx,pftUserDW, poi,bat, ctIncremental);

   *pDy = Ddc + Ddw;
   *pRz = Rdc + Rdw;
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetUserLoadDeflection(intervalIdx,poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,intervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);

   GetSlabBarrierOverlayDeflection(poi,config,pDy,pRz);
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // NOTE: No need to validate camber models
   Float64 Dslab = 0;
   Float64 Dslab_pad = 0;
   Float64 Dtrafficbarrier = 0;
   Float64 Dsidewalk = 0;
   Float64 Doverlay = 0;

   Float64 Rslab = 0;
   Float64 Rslab_pad = 0;
   Float64 Rtrafficbarrier = 0;
   Float64 Rsidewalk = 0;
   Float64 Roverlay = 0;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(segmentKey);
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval(segmentKey);

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GetDeckDeflection(poi,config,&Dslab,&Rslab);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad,&Rslab_pad);
   
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,railingSystemIntervalIdx);
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,poi,bat, ctIncremental);
   Rtrafficbarrier = k2*GetRotation(railingSystemIntervalIdx,pftTrafficBarrier,poi,bat, ctIncremental);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx,pftSidewalk,poi,bat, ctIncremental);
   Rsidewalk = k2*GetRotation(railingSystemIntervalIdx,pftSidewalk,poi,bat, ctIncremental);

   if ( overlayIntervalIdx != INVALID_INDEX )
   {
      Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pftOverlay,poi,bat, ctIncremental);
      Roverlay = k2*GetRotation(overlayIntervalIdx,pftOverlay,poi,bat, ctIncremental);
   }

   // Switch the sign. Negative deflection creates positive screed camber
   *pDy = Dslab + Dslab_pad + Dtrafficbarrier + Dsidewalk + Doverlay;
   *pRz = Rslab + Rslab_pad + Rtrafficbarrier + Rsidewalk + Roverlay;
}

void CAnalysisAgentImp::GetHarpedStrandEquivLoading(const CSegmentKey& segmentKey,Float64* pMl,Float64* pMr,Float64* pNl,Float64* pNr,Float64* pXl,Float64* pXr)
{
   CamberModelData modelData = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);

   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   modelData.Model->get_Loadings(&loadings);
   loadings->Find(g_lcidHarpedStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> ptLoads;
   loading->get_PointLoads(&ptLoads);

   // LoadID 0 and 1 = Moment (M)
   // LoadID 2 and 3 = Update force (N)
   CComPtr<IFem2dPointLoad> ptLoad;
   ptLoads->Find(0,&ptLoad);
   ATLASSERT(ptLoad != NULL);

   ptLoad->get_Mz(pMl);

   ptLoad.Release();
   ptLoads->Find(1,&ptLoad);
   ATLASSERT(ptLoad != NULL);
   ptLoad->get_Mz(pMr);

   ptLoad.Release();
   ptLoads->Find(2,&ptLoad);
   ATLASSERT(ptLoad != NULL);
   ptLoad->get_Fy(pNl);
   ptLoad->get_Location(pXl);

   ptLoad.Release();
   ptLoads->Find(3,&ptLoad);
   ATLASSERT(ptLoad != NULL);
   ptLoad->get_Fy(pNr);
   ptLoad->get_Location(pXr);
}

void CAnalysisAgentImp::GetTempStrandEquivLoading(const CSegmentKey& segmentKey,Float64* pMxferL,Float64* pMxferR,Float64* pMremoveL,Float64* pMremoveR)
{
   CamberModelData initTempModel    = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData releaseTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   // get moment from initial model
   CComPtr<IFem2dLoadingCollection> loadings;
   initTempModel.Model->get_Loadings(&loadings);

   CComPtr<IFem2dLoading> loading;
   loadings->Find(g_lcidTemporaryStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> ptLoads;
   loading->get_PointLoads(&ptLoads);

   CComPtr<IFem2dPointLoad> ptLoad;
   ptLoads->Find(0,&ptLoad);
   ptLoad->get_Mz(pMxferL);

   ptLoad.Release();
   ptLoads->Find(1,&ptLoad);
   ptLoad->get_Mz(pMxferR);

   loadings.Release();
   loading.Release();
   ptLoads.Release();
   ptLoad.Release();

   // get moment from release model
   releaseTempModel.Model->get_Loadings(&loadings);
   loadings->Find(g_lcidTemporaryStrand,&loading);
   loading->get_PointLoads(&ptLoads);
   ptLoads->Find(0,&ptLoad);
   ptLoad->get_Mz(pMremoveL);

   ptLoad.Release();
   ptLoads->Find(1,&ptLoad);
   ptLoad->get_Mz(pMremoveR);
}

void CAnalysisAgentImp::GetStraightStrandEquivLoading(const CSegmentKey& segmentKey,std::vector< std::pair<Float64,Float64> >* loads)
{
   loads->clear();
   // beams 1-4 (index 0-3 are for harped strands)
   CamberModelData model = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);

   CComPtr<IFem2dLoadingCollection> loadings;
   model.Model->get_Loadings(&loadings);

   CComPtr<IFem2dLoading> loading;
   loadings->Find(g_lcidStraightStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> ptLoads;
   loading->get_PointLoads(&ptLoads);

   CComPtr<IFem2dMemberCollection> members;
   model.Model->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   model.Model->get_Joints(&joints);

   CollectionIndexType nPtLoads;
   ptLoads->get_Count(&nPtLoads);
   for ( CollectionIndexType idx = 0; idx < nPtLoads; idx++ )
   {
      CComPtr<IFem2dPointLoad> ptLoad;
      ptLoads->get_Item(idx,&ptLoad);
      Float64 M,X;
      ptLoad->get_Mz(&M);
      ptLoad->get_Location(&X); // location from start of member

      // get start of member location..
      // use the x value of the start joint
      MemberIDType mbrID;
      ptLoad->get_MemberID(&mbrID);

      CComPtr<IFem2dMember> objMember;
      members->Find(mbrID,&objMember);

      JointIDType startJointID;
      objMember->get_StartJoint(&startJointID);

      CComPtr<IFem2dJoint> objStartJnt;
      joints->Find(startJointID,&objStartJnt);

      Float64 x;
      objStartJnt->get_X(&x);

      loads->push_back( std::make_pair(M,X+x) );
   }
}

/////////////////////////////////////////////////////////////////////////////
// IPretensionStresses
//
Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation)
{
   // Stress in the girder due to prestressing

   if ( stressLocation == pgsTypes::TopDeck || stressLocation == pgsTypes::BottomDeck )
      return 0.0; // pretensioning does not cause stress in the deck

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval(segmentKey);

   // This method can be optimized by caching the results.
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ISegmentData,pSegmentData);

   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   bool bIncTempStrands = (intervalIdx < tsRemovalIntervalIdx) ? true : false;
   Float64 P;
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      // If gross properties analysis, we want the prestress force at the end of the interval. It will include
      // elastic effects. If transformed properties analysis, we want the force at the start of the interval.
      pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);
      P = pPsForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,timeType);
      if ( bIncTempStrands )
      {
        P += pPsForce->GetPrestressForce(poi,pgsTypes::Temporary,intervalIdx,timeType);
      }
   }
   else
   {
      P = pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent);
      if ( bIncTempStrands )
      {
         P += pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Temporary);
      }
   }

   Float64 nSEffective;
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   Float64 e = pStrandGeom->GetEccentricity( releaseIntervalIdx, poi, bIncTempStrands, &nSEffective );

   return GetStress(poi,stressLocation,P,e);
}

Float64 CAnalysisAgentImp::GetStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 e)
{
   if ( stressLocation == pgsTypes::TopDeck || stressLocation == pgsTypes::BottomDeck )
      return 0.0; // pretensioning does not cause stress in deck

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());

   GET_IFACE(ISectionProperties,pSectProp);

   // using release interval because we are computing stress on the girder due to prestress which happens in this stage
   Float64 A = pSectProp->GetAg(releaseIntervalIdx, poi);
   Float64 S = pSectProp->GetS(releaseIntervalIdx, poi, stressLocation);

   Float64 f = (IsZero(A) || IsZero(S) ? 0 : -P/A - P*e/S);

   return f;
}

Float64 CAnalysisAgentImp::GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation stressLocation)
{
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P = pPsForce->GetPrestressForcePerStrand(poi,strandType,intervalIdx,timeType);
   Float64 nSEffective;
   Float64 e = pStrandGeom->GetEccentricity(intervalIdx,poi,strandType, &nSEffective);

   return GetStress(poi,stressLocation,P,e);
}

Float64 CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,const GDRCONFIG& config)
{
   // Computes design-time stresses due to prestressing
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval(segmentKey);

   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P;
   P = pPsForce->GetPrestressForce(poi,config,pgsTypes::Permanent,intervalIdx,timeType);
#pragma Reminder("UPDATE: need to get strand force with elastic gain due to live load")
   // NOT YET SUPPORT WHEN CONFIG IS PASSED IN
   //if ( intervalIdx < liveLoadIntervalIdx )
   //   P = pPsForce->GetPrestressForce(poi,config,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
   //else
   //   P = pPsForce->GetPrestressForceWithLiveLoad(poi,config,pgsTypes::Permanent);

#pragma Reminder("code below must be changed if designer ever uses post-tensioned tempoerary strands")
   bool bIncludeTemporaryStrands = intervalIdx < tsRemovalIntervalIdx ? true : false;
   if ( bIncludeTemporaryStrands ) 
   {
      P += pPsForce->GetPrestressForce(poi,config,pgsTypes::Temporary,intervalIdx,timeType);
   }

   Float64 nSEffective;
   Float64 e = pStrandGeom->GetEccentricity(intervalIdx,poi, config, bIncludeTemporaryStrands, &nSEffective);

   return GetStress(poi,stressLocation,P,e);
}

/////////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
//
HRESULT CAnalysisAgentImp::OnBridgeChanged(CBridgeChangedHint* pHint)
{
   LOG("OnBridgeChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnGirderFamilyChanged()
{
   LOG("OnGirderFamilyChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint)
{
   LOG("OnGirderChanged Event Received");

   Invalidate();

   return S_OK;
}

HRESULT CAnalysisAgentImp::OnLiveLoadChanged()
{
   LOG("OnLiveLoadChanged Event Received");
#pragma Reminder ("Took the easy way out here. Could Invalidate only live load and combined results")
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   LOG("OnLiveLoadNameChanged Event Received");

   GET_IFACE(IBridge,pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(0);

   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      ModelData* pModelData = 0;
      pModelData = GetModelData( gdrIdx );

      RenameLiveLoad(pModelData->m_Model,pgsTypes::lltDesign,strOldName,strNewName);
      RenameLiveLoad(pModelData->m_Model,pgsTypes::lltPermit,strOldName,strNewName);

      if ( pModelData->m_ContinuousModel )
      {
         RenameLiveLoad(pModelData->m_ContinuousModel,pgsTypes::lltDesign,strOldName,strNewName);
         RenameLiveLoad(pModelData->m_ContinuousModel,pgsTypes::lltPermit,strOldName,strNewName);
      }
   }

   return S_OK;
}

HRESULT CAnalysisAgentImp::OnConstructionLoadChanged()
{
   LOG("OnConstructionLoadChanged Event Received");
#pragma Reminder ("Took the easy way out here.")
   Invalidate();
   return S_OK;
}

void CAnalysisAgentImp::RenameLiveLoad(ILBAMModel* pModel,pgsTypes::LiveLoadType llType,LPCTSTR strOldName,LPCTSTR strNewName)
{
   USES_CONVERSION;

   CComPtr<ILiveLoad> live_load;
   pModel->get_LiveLoad(&live_load);

   // get the live load model
   CComPtr<ILiveLoadModel> liveload_model;
   if ( llType == pgsTypes::lltDesign )
      live_load->get_Design(&liveload_model);
   else
      live_load->get_Permit(&liveload_model);

   // get the vehicular loads collection
   CComPtr<IVehicularLoads> vehicles;
   liveload_model->get_VehicularLoads(&vehicles);

   CComPtr<IEnumVehicularLoad> enum_vehicles;
   vehicles->get__EnumElements(&enum_vehicles);

   CComPtr<IVehicularLoad> vehicle;
   while ( enum_vehicles->Next(1,&vehicle,NULL) != S_FALSE )
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

/////////////////////////////////////////////////////////////////////////////
// ISpecificationEventSink
//
HRESULT CAnalysisAgentImp::OnSpecificationChanged()
{
   Invalidate();
   LOG("OnSpecificationChanged Event Received");
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnAnalysisTypeChanged()
{
   //Invalidate();
   // I don't think we have to do anything when the analysis type changes
   // The casting yard models wont change
   // The simple span models, if built, wont change
   // The continuous span models, if built, wont change
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IRatingSpecificationEventSink
//
HRESULT CAnalysisAgentImp::OnRatingSpecificationChanged()
{
   Invalidate();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILoadModifiersEventSink
//
HRESULT CAnalysisAgentImp::OnLoadModifiersChanged()
{
   Invalidate();
   LOG("OnSpecificationChanged Event Received");
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData camber_model_data = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,camber_model_data,g_lcidHarpedStrand,bRelativeToBearings,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,camber_model_data,g_lcidStraightStrand,bRelativeToBearings,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model_data;
   BuildCamberModel(segmentKey,true,config,&model_data);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,model_data,g_lcidHarpedStrand,bRelativeToBearings,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,model_data,g_lcidStraightStrand,bRelativeToBearings,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model_data = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   
   GetInitialTempPrestressDeflection(poi,model_data,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model1, model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);

   GetInitialTempPrestressDeflection(poi,model1,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   GetReleaseTempPrestressDeflection(poi,model,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model1,model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);

   GetReleaseTempPrestressDeflection(poi,model2,pDy,pRz);
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,LoadCaseIDType lcid,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   if ( poi.HasAttribute(POI_CLOSURE) || poi.HasAttribute(POI_BOUNDARY_PIER) )
   {
      *pDy = 0;
      *pRz = 0;
      return;
   }

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPOI);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // We need to compute the deflection relative to the bearings even though the prestress
   // deflection occurs over the length of the entire girder.  To accomplish this, simply
   // deduct the deflection at the bearing (when the girder is supported on its ends) from
   // the deflection at the specified location.
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;

   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   if ( femPoiID == INVALID_ID )
   {
      pgsPointOfInterest thePOI;
      if ( poi.GetID() == INVALID_ID )
         thePOI = pPOI->GetPointOfInterest(segmentKey,poi.GetDistFromStart());
      else
         thePOI = poi;

      ATLASSERT( thePOI.GetID() != INVALID_ID );

      femPoiID = AddPointOfInterest(modelData,thePOI);
      ATLASSERT( 0 <= femPoiID );
   }

   HRESULT hr = results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   ATLASSERT( SUCCEEDED(hr) );

   Float64 delta = Dy;
   if ( bRelativeToBearings )
   {
      Float64 start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
      pgsPointOfInterest poiAtStart( pPOI->GetPointOfInterest(segmentKey,start_end_size) );
      ATLASSERT( 0 <= poiAtStart.GetID() );
   
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtStart);
      results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 start_delta_brg = Dy;

      Float64 end_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
      Float64 Lg = pBridge->GetSegmentLength(segmentKey);
      pgsPointOfInterest poiAtEnd( pPOI->GetPointOfInterest(segmentKey,Lg-end_end_size) );
      ATLASSERT( 0 <= poiAtEnd.GetID() );
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtEnd);
      results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 end_delta_brg = Dy;

      Float64 delta_brg = LinInterp(poi.GetDistFromStart()-start_end_size,
                                    start_delta_brg,end_delta_brg,Lg);

      delta -= delta_brg;
   }

   *pDy = delta;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   GetPrestressDeflection(poi,modelData,g_lcidTemporaryStrand,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDy,Float64* pRz)
{
   if ( poi.HasAttribute(POI_CLOSURE) || poi.HasAttribute(POI_BOUNDARY_PIER)  )
   {
      *pDy = 0;
      *pRz = 0;
      return;
   }

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPOI);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(modelData.Model,poi.GetDistFromStart(),&mbrID,&x);

   // Get deflection at poi
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;
   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   Float64 delta_poi = Dy;

   // Get deflection at start bearing
   Float64 start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   pgsPointOfInterest poi2( pPOI->GetPointOfInterest(segmentKey,start_end_size) );
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 start_delta_end_size = Dy;

   // Get deflection at end bearing
   Float64 L = pBridge->GetSegmentLength(segmentKey);
   Float64 end_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
   poi2 = pPOI->GetPointOfInterest(segmentKey,L-end_end_size);
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 end_delta_end_size = Dy;

   Float64 delta_brg = LinInterp(poi.GetDistFromStart()-start_end_size,
                                    start_delta_end_size,end_delta_end_size,L);

   Float64 delta = delta_poi - delta_brg;
   *pDy = delta;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bTempStrands = false;
   
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetNStrands(pgsTypes::Temporary) && 
                       config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

      bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) && 
                      pStrands->TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }

   Float64 Dcreep = 0;
   switch( deckType )
   {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
         (bTempStrands ? GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz) 
                       : GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz));
         break;

      case pgsTypes::sdtCompositeSIP:
         (bTempStrands ? GetCreepDeflection_SIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz) 
                       : GetCreepDeflection_SIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz));
         break;

      case pgsTypes::sdtNone:
         (bTempStrands ? GetCreepDeflection_NoDeck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz) 
                       : GetCreepDeflection_NoDeck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz));
         break;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);


   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dtpsi,Rtpsi;
   GetPrestressDeflection( poi, initTempModelData, g_lcidTemporaryStrand, true, &Dtpsi, &Rtpsi);

   Float64 Dgirder, Rgirder;
   if ( bUseConfig )
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
   else
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);

   Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate)
                             : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate));


   // creep 1 - Initial to immediately before diaphragm/temporary strands
   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      *pDy = Ct1*(Dgirder + Dps + Dtpsi);
      *pRz = Ct1*(Rgirder + Rps + Rtpsi);
      return;
   }

   // creep 2 - Immediately after diarphagm/temporary strands to deck
   Float64 Ct2 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,constructionRate) :
                               GetCreepCoefficient(segmentKey,cpReleaseToDeck,constructionRate) );

   Float64 Ct3 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,constructionRate) :
                               GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,constructionRate) );

   Float64 Ddiaphragm = GetDeflection(castDiaphragmIntervalIdx,pftDiaphragm,poi,pgsTypes::SimpleSpan, ctIncremental);
   Float64 Rdiaphragm = GetRotation(castDiaphragmIntervalIdx,pftDiaphragm,poi,pgsTypes::SimpleSpan, ctIncremental);

   Float64 Dtpsr,Rtpsr;
   GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, true, &Dtpsr, &Rtpsr);

   *pDy = (Ct2-Ct1)*(Dgirder + Dps + Dtpsi) + Ct3*(Ddiaphragm + Dtpsr);
   *pRz = (Ct2-Ct1)*(Rgirder + Rps + Rtpsi) + Ct3*(Rdiaphragm + Rtpsr);
}

void CAnalysisAgentImp::GetCreepDeflection_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Ct1 = -999;
   Float64 Dgirder = -999;
   Float64 Rgirder = -999;

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,constructionRate);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,constructionRate);
   }


   *pDy = Ct1*(Dgirder + Dps);
   *pRz = Ct1*(Rgirder + Rps);
}

void CAnalysisAgentImp::GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dtpsi, Rtpsi;
   GetPrestressDeflection( poi, initTempModelData, g_lcidTemporaryStrand, true, &Dtpsi, &Rtpsi);

   Float64 Dtpsr, Rtpsr;
   GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, true, &Dtpsr, &Rtpsr);

   Float64 Dgirder, Rgirder;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate) 
                                : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(Dgirder + Dps + Dtpsi);
      *pRz = Ct1*(Rgirder + Rps + Rtpsi);
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,   constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,   constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + Ct3*(Ddiaphragm + Duser1 + Dtpsr);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + Ct3*(Rdiaphragm + Ruser1 + Rtpsr);
   }
   else
   {
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,cpDeckToFinal,        constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + (Ct4 - Ct3)*(Ddiaphragm + Duser1 + Dtpsr) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + (Ct4 - Ct3)*(Rdiaphragm + Ruser1 + Rtpsr) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dgirder, Rgirder;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")


   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate)
                                : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(Dgirder + Dps);
      *pRz = Ct1*(Rgirder + Rps);
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,   constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,   constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + Ct3*(Ddiaphragm + Duser1);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + Ct3*(Rdiaphragm + Ruser1);
   }
   else
   {
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,cpDeckToFinal,        constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + (Ct4 - Ct3)*(Ddiaphragm + Duser1) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + (Ct4 - Ct3)*(Rdiaphragm + Ruser1) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   Float64 Dy, Dz;
   GetDCamberForGirderSchedule( poi, initModelData, initTempModelData, releaseTempModelData, time, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamber(poi,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,ICamber::cpDeckToFinal,time,&Dcreep3,&Rcreep3 );
      *pDy = Dy + Cy + Dcreep3;
      *pRz = Dz + Cz + Rcreep3;
   }
}

void CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   Float64 Dy, Dz;
   GetDCamberForGirderSchedule( poi, config, initModelData, initTempModelData, releaseTempModelData, time, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamber(poi,config,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,config,ICamber::cpDeckToFinal,time,&Dcreep3,&Rcreep3 );
      *pDy += Dcreep3;
      *pRz += Rcreep3;
   }
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GDRCONFIG dummy_config;
   GetDCamberForGirderSchedule(poi,false,dummy_config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GetDCamberForGirderSchedule(poi,true,config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bTempStrands = true;
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetNStrands(pgsTypes::Temporary) && 
                      config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrand = pSegmentData->GetStrandData(segmentKey);

      bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) && 
                      pStrand->TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   Float64 D, R;

   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      (bTempStrands ? GetD_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : GetD_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   case pgsTypes::sdtCompositeSIP:
      (bTempStrands ? GetD_SIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : GetD_SIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   case pgsTypes::sdtNone:
      (bTempStrands ? GetD_NoDeck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : GetD_NoDeck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   default:
      ATLASSERT(false);
   }

   *pDy = D;
   *pRz = R;
}

void CAnalysisAgentImp::GetD_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dcreep2;
   Float64 Rps, Rtpsi, Rtpsr, Rgirder, Rcreep1, Rdiaphragm, Rcreep2;
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi, config, true, &Dtpsi, &Rtpsi );
      GetReleaseTempPrestressDeflection( poi, config, &Dtpsr, &Rtpsr );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1);
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi, true, &Dtpsi, &Rtpsi );
      GetReleaseTempPrestressDeflection( poi, &Dtpsr, &Rtpsr );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
   }

   Float64 D1 = Dgirder + Dps + Dtpsi;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Dtpsr;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps + Rtpsi;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Rtpsr;
   Float64 R4 = R3 + Rcreep2;

   *pRz = R4;
}

void CAnalysisAgentImp::GetD_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dgirder, Dcreep;
   Float64 Rps, Rgirder, Rcreep;

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep );
   }

   Float64 D1 = Dgirder + Dps;
   Float64 D2 = D1 + Dcreep;
   *pDy = D2;

   Float64 R1 = Rgirder + Rps;
   Float64 R2 = R1 + Rcreep;
   *pRz = R2;
}

void CAnalysisAgentImp::GetD_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dpanel, Dcreep2;
   Float64 Rps, Rtpsi, Rtpsr, Rgirder, Rcreep1, Rdiaphragm, Rpanel, Rcreep2;

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,config,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,config,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,config,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi,config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetDeckPanelDeflection( poi, config, &Dpanel, &Rpanel );
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi,&Ddiaphragm,&Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate,&Dcreep2,&Rcreep2 );
      GetDeckPanelDeflection( poi,&Dpanel,&Rpanel );
   }

   Float64 D1 = Dgirder + Dps + Dtpsi;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Dtpsr + Dpanel;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps + Rtpsi;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Rtpsr + Rpanel;
   Float64 R4 = R3 + Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetD_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dgirder, Dcreep, Ddiaphragm, Dpanel, Ddeck;
   Float64 Rps, Rgirder, Rcreep, Rdiaphragm, Rpanel, Rdeck;
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetDeckDeflection( poi, config, &Ddeck, &Rdeck );
      GetDeckPanelDeflection( poi, config, &Dpanel, &Rpanel );
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetDeckDeflection( poi, &Ddeck, &Rdeck );
      GetDeckPanelDeflection( poi, &Dpanel, &Rpanel );
   }

   Float64 D1 = Dgirder + Dps;
   Float64 D2 = D1 + Dcreep;
   Float64 D3 = D2 + Ddiaphragm + Dpanel;
   *pDy = D3;

   Float64 R1 = Rgirder + Rps;
   Float64 R2 = R1 + Rcreep;
   Float64 R3 = R2 + Rdiaphragm + Rpanel;
   *pRz = R3;
}

void CAnalysisAgentImp::GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dcreep2, Duser1;
   Float64 Rps, Rtpsi, Rtpsr, Rgirder, Rcreep1, Rdiaphragm, Rcreep2, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,config,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,config,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,config,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi,config,&Ddiaphragm,&Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate,&Dcreep2,&Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, config,&Duser1,&Ruser1);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, &Duser1, &Ruser1);
   }

   Float64 D1 = Dgirder + Dps + Dtpsi;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Dtpsr + Duser1;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps + Rtpsi;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Rtpsr + Ruser1;
   Float64 R4 = R3 + Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetD_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, Dgirder, Dcreep1, Ddiaphragm, Dcreep2, Duser1;
   Float64 Rps, Rgirder, Rcreep1, Rdiaphragm, Rcreep2, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, config, &Duser1, &Ruser1);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, &Duser1, &Ruser1);
   }

   Float64 D1 = Dgirder + Dps;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Duser1;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Ruser1;
   Float64 R4 = R3 + Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;

   *pDy = beam.ComputeDeflection(x);
   *pRz = beam.ComputeRotation(x);
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey,LoadingEvent le)
{
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);

   Float64 Fc;

   switch( le )
   {
   case ICamber::leRelease:
      Fc = pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx);
      break;

   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = pMaterial->GetSegmentFc(segmentKey,castDeckIntervalIdx);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config,LoadingEvent le)
{
   Float64 Fc;

   switch( le )
   {
   case ICamber::leRelease:
   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = config.Fci;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

ICamber::LoadingEvent CAnalysisAgentImp::GetLoadingEvent(CreepPeriod creepPeriod)
{
   LoadingEvent le;
   switch( creepPeriod )
   {
   case cpReleaseToDiaphragm:
   case cpReleaseToDeck:
   case cpReleaseToFinal:
      le = leRelease;
      break;

   case cpDiaphragmToDeck:
   case cpDiaphragmToFinal:
      le = leDiaphragm;
      break;

   case cpDeckToFinal:
      le = leDeck;
      break;

   default:
      ATLASSERT(false);
   }

   return le;
}

/////////////////////////////////////////////////////////////////////////
// IContraflexurePoints
void CAnalysisAgentImp::GetContraflexurePoints(SpanIndexType span,GirderIndexType gdr,Float64* cfPoints,Uint32* nPoints)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();
   CGirderKey girderKey(grpIdx,gdr);

   ModelData* pModelData = GetModelData( gdr );

   GET_IFACE(IBridge,pBridge);

#pragma Reminder("UPDATE: this may be the wrong span length")
   // using pier to pier length when we really need the structural span length
   // also see for loop below
   Float64 span_length = pBridge->GetSpanLength(span);

   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < span; i++ )
   {
      span_start += pBridge->GetSpanLength(i);
      //const CSpanData* pSpan = pBridgeDesc->GetSpan(i);
      //const CGirderGroupData* pGroup = pBridgeDesc(pSpan)
      //GirderIndexType nGirders = pGroup->GetGirderCount();
      //GirderIndexType gdrIdx = Min(gdr,nGirders-1);

      //CSegmentKey thisSegmentKey(i,gdrIdx,0);
      //span_start += pBridge->GetSegmentSpanLength(thisSegmentKey);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];

   CComPtr<ILBAMAnalysisEngine> pEngine;
   GetEngine(pModelData,true,&pEngine);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadInterval = pIntervals->GetLiveLoadInterval(girderKey);
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

/////////////////////////////////////////////////////////////////////////
// IContinuity
bool CAnalysisAgentImp::IsContinuityFullyEffective(const CGirderKey& girderKey)
{
   // The continuity of a girder line is fully effective if the continuity stress level
   // at all continuity piers is compressive.
   //
   // Continuity is fully effective at piers where a precast segment spans
   // across the pier.
   //
   // Check for continuity at the start and end of each girder group (that is where the continuity diaphragms are located)
   // If the continuity is not effective at any continuity diaphragm, the contininuity is not effective for the full girder line

   bool bContinuous = false;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      for (int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         PierIndexType pierIdx = pGroup->GetPierIndex(endType);

         bool bContinuousLeft, bContinuousRight;
         pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

         bool bIntegralLeft, bIntegralRight;
         pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

         if ( (bContinuousLeft && bContinuousRight) || (bIntegralLeft && bIntegralRight)  )
         {
            Float64 fb = GetContinuityStressLevel(pierIdx,girderKey);
            bContinuous = (0 <= fb ? false : true);
         }
         else
         {
            bContinuous = false;
         }

         if ( bContinuous )
            return bContinuous;
      }
   }

   return bContinuous;
}

Float64 CAnalysisAgentImp::GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   ATLASSERT(girderKey.girderIndex != INVALID_INDEX);

   // for evaluation of LRFD 5.14.1.4.5 - Degree of continuity

   // If we are in simple span analysis mode, there is no continuity
   // no matter what the boundary conditions are
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   if ( analysisType == pgsTypes::Simple )
      return 0.0;


   // check the boundary conditions
   GET_IFACE(IBridge,pBridge);
   bool bIntegralLeft,bIntegralRight;
   pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);
   bool bContinuousLeft,bContinuousRight;
   pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

   if ( !bIntegralLeft && !bIntegralRight && !bContinuousLeft && !bContinuousRight )
      return 0.0;

   // how does this work for segments that span over a pier? I don't
   // think that 5.14.1.4.5 applies to spliced girder bridges unless we are looking
   // at a pier that is between groups
   ATLASSERT(pBridge->IsBoundaryPier(pierIdx));

   GroupIndexType backGroupIdx, aheadGroupIdx;
   pBridge->GetGirderGirderIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);
   ATLASSERT(backGroupIdx == aheadGroupIdx-1);

   GirderIndexType gdrIdx = girderKey.girderIndex;

   // computes the stress at the bottom of the girder on each side of the pier
   // returns the greater of the two values
   GET_IFACE(IPointOfInterest,pPOI);

   // deal with girder index when there are different number of girders in each group
   GirderIndexType prev_group_gdr_idx = gdrIdx;
   GirderIndexType next_group_gdr_idx = gdrIdx;
   prev_group_gdr_idx = Min(gdrIdx,pBridge->GetGirderCount(backGroupIdx)-1);
   next_group_gdr_idx = Min(gdrIdx,pBridge->GetGirderCount(aheadGroupIdx)-1);


   CollectionIndexType nPOI = 0;
   pgsPointOfInterest vPOI[2];

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(girderKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(girderKey);
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval(girderKey);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(girderKey);

   IntervalIndexType continuity_interval[2];

   EventIndexType leftContinuityEventIndex, rightContinuityEventIndex;
   pBridge->GetContinuityEventIndex(pierIdx,&leftContinuityEventIndex,&rightContinuityEventIndex);


   // get poi at cl bearing at end of prev group
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(backGroupIdx,prev_group_gdr_idx);
      CSegmentKey thisSegmentKey(backGroupIdx,prev_group_gdr_idx,nSegments-1);
      std::vector<pgsPointOfInterest> vPoi(pPOI->GetPointsOfInterest(thisSegmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = pIntervals->GetInterval(thisSegmentKey,leftContinuityEventIndex);
      nPOI++;
   }

   // get poi at cl bearing at start of next group
   {
      CSegmentKey thisSegmentKey(aheadGroupIdx,next_group_gdr_idx,0);
      std::vector<pgsPointOfInterest> vPoi(pPOI->GetPointsOfInterest(thisSegmentKey,POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = pIntervals->GetInterval(thisSegmentKey,rightContinuityEventIndex);
      nPOI++;
   }

   Float64 f[2] = {0,0};
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];
      ATLASSERT( 0 <= poi.GetID() );

      pgsTypes::BridgeAnalysisType bat = pgsTypes::ContinuousSpan;

      Float64 fbConstruction, fbSlab, fbSlabPad, fbTrafficBarrier, fbSidewalk, fbOverlay, fbUserDC, fbUserDW, fbUserLLIM, fbLLIM;

      Float64 fTop,fBottom;

      if ( continuity_interval[i] == castDeckIntervalIdx )
      {
         GetStress(castDeckIntervalIdx,pftSlab,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlab = fBottom;

         GetStress(castDeckIntervalIdx,pftSlabPad,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlabPad = fBottom;

         GetStress(castDeckIntervalIdx,pftConstruction,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbConstruction = fBottom;
      }
      else
      {
         fbSlab         = 0;
         fbSlabPad      = 0;
         fbConstruction = 0;
      }

      GetStress(railingSystemIntervalIdx,pftTrafficBarrier,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbTrafficBarrier = fBottom;

      GetStress(railingSystemIntervalIdx,pftSidewalk,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbSidewalk = fBottom;

      if ( overlayIntervalIdx != INVALID_INDEX )
      {
         GetStress(overlayIntervalIdx,pftOverlay,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbOverlay = fBottom;
      }
      else
      {
         fbOverlay = 0;
      }

#pragma Reminder("UPDATE: need all user defined load stages")
      // need to include all intervals when user defined loads are applied, not just compositeDeckIntervalIdx
      GetStress(compositeDeckIntervalIdx,pftUserDC,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbUserDC = fBottom;

      GetStress(compositeDeckIntervalIdx,pftUserDW,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbUserDW = fBottom;

      GetStress(compositeDeckIntervalIdx,pftUserLLIM,poi,bat, ctIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbUserLLIM = fBottom;

      Float64 fTopMin,fTopMax,fBotMin,fBotMax;
      GetCombinedLiveLoadStress(pgsTypes::lltDesign,liveLoadIntervalIdx,poi,bat,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTopMin,&fTopMax,&fBotMin,&fBotMax);
      fbLLIM = fBotMin; // greatest compression

      fBottom = fbConstruction + fbSlab + fbSlabPad + fbTrafficBarrier + fbSidewalk + fbOverlay + fbUserDC + fbUserDW + 0.5*(fbUserLLIM + fbLLIM);

      f[i] = fBottom;
   }

   return (nPOI == 1 ? f[0] : Max(f[0],f[1]));
}

/////////////////////////////////////////////////
// IPrecompressedTensileZone
bool CAnalysisAgentImp::IsInPrecompressedTensileZone(IntervalIndexType stressingIntervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,pgsTypes::BridgeAnalysisType bat)
{
   return IsInPrecompressedTensileZone(stressingIntervalIdx,poi,stressLocation,bat,NULL);
}

bool CAnalysisAgentImp::IsInPrecompressedTensileZone(IntervalIndexType stressingIntervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,pgsTypes::BridgeAnalysisType bat, const GDRCONFIG* pConfig)
{
#pragma Reminder("UPDATE: Finialize how we are going to handle the precompressed tensile zone")
   // this makes the PTZ evaluation work exactly like it says in the LRFD
   GET_IFACE(IIntervals,pIntervals);
   stressingIntervalIdx = pIntervals->GetIntervalCount(poi.GetSegmentKey())-1;

   if ( IsDeckStressLocation(stressLocation) )
      return IsDeckInPrecompressedTensileZone(stressingIntervalIdx,poi,stressLocation,bat);
   else
      return IsGirderInPrecompressedTensileZone(stressingIntervalIdx,poi,stressLocation,bat,pConfig);
}

bool CAnalysisAgentImp::IsDeckPrecompressed(const CGirderKey& girderKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(girderKey);
   if ( compositeDeckIntervalIdx == INVALID_INDEX )
      return false; // this happens when there is not a deck

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
      if ( compositeDeckIntervalIdx <= stressTendonIntervalIdx )
      {
         // this tendon is stressed after the deck is composite so the deck is considered precompressed
         return true;
      }
   }

   // didn't find a tendon that is stressed after the deck is composite... deck not precompressed
   return false;
}

/////////////////////////////////////////////////
// IInfluenceResults
std::vector<Float64> CAnalysisAgentImp::GetUnitLoadMoment(const std::vector<pgsPointOfInterest>& vPoi,const pgsPointOfInterest& unitLoadPOI,pgsTypes::BridgeAnalysisType bat,IntervalIndexType intervalIdx)
{
   std::vector<Float64> moments;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(unitLoadPOI.GetSegmentKey());
   if (intervalIdx < erectionIntervalIdx )
   {
      std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; iter != end; iter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         const CSegmentKey& segmentKey = poi.GetSegmentKey();

         IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);

         ValidateAnalysisModels(segmentKey.girderIndex);

         SegmentModels& segmentModels(intervalIdx < storageIntervalIdx ? m_ReleaseModels : m_StorageModels);
         SegmentModelData& modelData(segmentModels[unitLoadPOI.GetSegmentKey()]);

         sysSectionValue fx(0),fy(0),mz(0);
         Float64 dx(0),dy(0),rz(0);
         if ( segmentKey == unitLoadPOI.GetSegmentKey() && !unitLoadPOI.HasAttribute(POI_CLOSURE) && !unitLoadPOI.HasAttribute(POI_BOUNDARY_PIER) )
         {
            LoadCaseIDType lcid = modelData.UnitLoadIDMap[unitLoadPOI.GetID()];
            GetSectionResults( segmentModels, lcid, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            ATLASSERT(IsEqual(mz.Left(),-mz.Right(),0.0001));
         }
         moments.push_back( mz.Left() );
      }
   }
   else
   {
      CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

      ModelData* pModelData = UpdateLBAMPois(vPoi);

      PoiIDType poiID = pModelData->PoiMap.GetModelPoi(unitLoadPOI);
      ATLASSERT( 0 <= poiID );

      CComPtr<ISectionResult3Ds> section_results; // holds the unit load moment response for a unit load at poiID
      pModelData->pUnitLoadResponse[bat]->ComputeForces(m_LBAMPoi,poiID,bstrStage,roMember,&section_results);

      CollectionIndexType nResults;
      section_results->get_Count(&nResults);
      ATLASSERT(vPoi.size() == nResults);
      for ( CollectionIndexType resultIdx = 0; resultIdx < nResults; resultIdx++ )
      {
         CComPtr<ISectionResult3D> result;
         section_results->get_Item(resultIdx,&result);

         Float64 MzLeft, MzRight;
         result->get_ZLeft(&MzLeft);
         result->get_ZRight(&MzRight);
         ATLASSERT(IsEqual(MzLeft,-MzRight));

         MzLeft = IsZero(MzLeft) ? 0 : MzLeft;

         // The LBAM unit force response is derived from the influence lines for live load analysis
         // The influence lines are based on a downward (negative) unit load. We want the unit
         // load moments that will be used for deflection analysis to be based on an upward (positive)
         // unit load. For this reason, we switch the sign of the result
         moments.push_back(-MzLeft);
      }
   }

   return moments;
}

/////////////////////////////////////////////////
// IBearingDesign

bool CAnalysisAgentImp::AreBearingReactionsAvailable(IntervalIndexType intervalIdx,const CGirderKey& girderKey, bool* pBleft, bool* pBright)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();
   if (nSpans == 1 || analysisType == pgsTypes::Simple)
   {
      // Always can get for bearing reactions for single span or simple spans.
      *pBleft  = true;
      *pBright = true;
      return true;
   }
   else
   {
      if (girderKey.groupIndex == ALL_GROUPS)
      {
         *pBleft  = true;
         *pBright = true;
         return true;
      }
      else
      {
         // Get boundary conditions at both ends of span
         PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(girderKey.groupIndex);
         PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(girderKey.groupIndex);

         bool bdummy;
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(startPierIdx, &bdummy,            &bContinuousOnLeft);
         pBridge->IsContinuousAtPier(endPierIdx,   &bContinuousOnRight,&bdummy);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(startPierIdx,  &bdummy,          &bIntegralOnLeft);
         pBridge->IsIntegralAtPier(endPierIdx,    &bIntegralOnRight,&bdummy);

         // Bearing reactions are available at simple supports
         bool bSimpleOnLeft, bSimpleOnRight;
         bSimpleOnLeft  = !bContinuousOnLeft  && !bIntegralOnLeft;
         bSimpleOnRight = !bContinuousOnRight && !bIntegralOnRight;

         // Finally check if interval is before continuity
         GET_IFACE(IIntervals,pIntervals);
         if ( !bSimpleOnLeft )
         {
            EventIndexType dummyEventIdx, eventIdx;
            pBridge->GetContinuityEventIndex(startPierIdx,&dummyEventIdx,&eventIdx);
            IntervalIndexType continuityIntervalIdx = pIntervals->GetInterval(girderKey,eventIdx);

            if ( intervalIdx < continuityIntervalIdx )
            {
               bSimpleOnLeft = true;
            }
         }

         if ( !bSimpleOnRight)
         {
            EventIndexType dummyEventIdx, eventIdx;
            pBridge->GetContinuityEventIndex(endPierIdx,&eventIdx,&dummyEventIdx);
            IntervalIndexType continuityIntervalIdx = pIntervals->GetInterval(girderKey,eventIdx);

            if ( intervalIdx < continuityIntervalIdx )
            {
               bSimpleOnRight = true;
            }
         }

         *pBleft  = bSimpleOnLeft;
         *pBright = bSimpleOnRight;

          // Return true if we have simple supports at either end of girder
         if (bSimpleOnLeft || bSimpleOnRight)
         {
            return true;
         }
         else
         {
            return false;
         }
      }
   }
}

void CAnalysisAgentImp::GetBearingProductReaction(IntervalIndexType intervalIdx,ProductForceType type,const CGirderKey& girderKey,
                                                  CombinationType cmbtype, pgsTypes::BridgeAnalysisType bat,Float64* pLftEnd,Float64* pRgtEnd)
{
   // Get Pois at supports
   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi1( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),           POI_ERECTED_SEGMENT | POI_0L, POIFIND_AND) );
   std::vector<pgsPointOfInterest> vPoi2( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,nSegments-1), POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND) );
   ATLASSERT(vPoi1.size()==1);
   ATLASSERT(vPoi2.size()==1);
   ATLASSERT(vPoi1.front().IsTenthPoint(POI_ERECTED_SEGMENT));
   ATLASSERT(vPoi2.front().IsTenthPoint(POI_ERECTED_SEGMENT));

   // Loop twice to pick up left and right ends
   for (IndexType idx=0; idx<2; idx++)
   {
      pgsTypes::BridgeAnalysisType tmpbat = bat;
      std::vector<pgsPointOfInterest> vPoi;
      if (idx == 0)
      {
         vPoi.push_back(vPoi1.front());
      }
      else
      {
         vPoi.push_back(vPoi2.front());

         // Extremely TRICKY:
         // Below we are getting reactions from  end shear, we must flip sign of results to go 
         // from LBAM to beam coordinates. This means that the optimization must go the opposite when using the envelopers.
         if (bat == pgsTypes::MinSimpleContinuousEnvelope)
         {
            tmpbat = pgsTypes::MaxSimpleContinuousEnvelope;
         }
         else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
         {
            tmpbat = pgsTypes::MinSimpleContinuousEnvelope;
         }
      }

      std::vector<sysSectionValue> sec_vals = GetShear(intervalIdx, type, vPoi, tmpbat, cmbtype);

      if (idx == 0)
      {
         *pLftEnd = sec_vals.front().Right();
      }
      else
      {
         *pRgtEnd = -sec_vals.front().Left();
      }
   }

   // Last, add point loads from overhangs
   pgsTypes::AnalysisType analysisType = (bat == pgsTypes::SimpleSpan ? pgsTypes::Simple : pgsTypes::Continuous);

   Float64 lft_pnt_load, rgt_pnt_load;
   bool found_load = GetOverhangPointLoads(CSegmentKey(girderKey,0), analysisType,intervalIdx, type, cmbtype, &lft_pnt_load, &rgt_pnt_load);
   if(found_load)
   {
      *pLftEnd -= lft_pnt_load;
   }

   found_load = GetOverhangPointLoads(CSegmentKey(girderKey,nSegments-1), analysisType,intervalIdx, type, cmbtype, &lft_pnt_load, &rgt_pnt_load);
   if(found_load)
   {
      *pRgtEnd -= rgt_pnt_load;
   }
}

void CAnalysisAgentImp::GetBearingLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const CGirderKey& girderKey,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pLeftRmin,Float64* pLeftRmax,Float64* pLeftTmin,Float64* pLeftTmax,
                                Float64* pRightRmin,Float64* pRightRmax,Float64* pRightTmin,Float64* pRightTmax,
                                VehicleIndexType* pLeftMinVehIdx,VehicleIndexType* pLeftMaxVehIdx,
                                VehicleIndexType* pRightMinVehIdx,VehicleIndexType* pRightMaxVehIdx)
{
   // This is just end shears and rotations due to live load at ends of girder
   // Get Pois at supports
   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   SpanIndexType leftSpanIdx,rightSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&leftSpanIdx,&rightSpanIdx);


   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi1( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),           POI_ERECTED_SEGMENT | POI_0L, POIFIND_OR) );
   std::vector<pgsPointOfInterest> vPoi2( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,nSegments-1), POI_ERECTED_SEGMENT | POI_10L,POIFIND_OR) );
   ATLASSERT(vPoi1.size()==1);
   ATLASSERT(vPoi2.size()==1);

   pgsPointOfInterest lftPoi(vPoi1.front());
   pgsPointOfInterest rgtPoi(vPoi2.front());

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);

   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   // Loop twice to pick up left and right ends
   for (IndexType idx=0; idx<2; idx++)
   {
      pgsTypes::BridgeAnalysisType tmpbat = bat;
      std::vector<pgsPointOfInterest> vPoi;


      // Shear and reaction LLDF's can be different. Must ratio results by reaction/shear.
      Float64 lldfRatio = 1.0;
      Float64 lldfShear;
      pgsTypes::LimitState lldfLimitState;
      if (bIncludeLLDF)
      {
         bool is_fatigue = llType==pgsTypes::lltFatigue || llType==pgsTypes::lltPermitRating_Special;
         lldfLimitState = is_fatigue ? pgsTypes::FatigueI : pgsTypes::StrengthI;
         lldfShear = pLLDF->GetShearDistFactor(idx == 0 ? leftSpanIdx : rightSpanIdx,girderKey.girderIndex,lldfLimitState);

         Float64 lldfReact = pLLDF->GetReactionDistFactor(idx == 0 ? leftSpanIdx : rightSpanIdx,girderKey.girderIndex, lldfLimitState);
         lldfRatio = lldfReact/lldfShear;
      }

      if (idx==0)
      {
         vPoi.push_back(lftPoi);

         // Extremely TRICKY:
         // Below we are getting reactions from  end shear, we must flip sign of results to go 
         // from LBAM to beam coordinates. This means that the optimization must go the opposite when using the envelopers.
         if (bat == pgsTypes::MinSimpleContinuousEnvelope)
         {
            tmpbat = pgsTypes::MaxSimpleContinuousEnvelope;
         }
         else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
         {
            tmpbat = pgsTypes::MinSimpleContinuousEnvelope;
         }
      }
      else
      {
         vPoi.push_back(rgtPoi);
      }

      // Get max'd end shears from lbam
      ModelData* pModelData = UpdateLBAMPois(vPoi);
      CComPtr<ILBAMAnalysisEngine> pEngine;
      GetEngine(pModelData,tmpbat == pgsTypes::SimpleSpan ? false : true, &pEngine);
      CComPtr<IBasicVehicularResponse> response;
      pEngine->get_BasicVehicularResponse(&response); // used to get corresponding rotations

      CComPtr<ILiveLoadModelSectionResults> minResults;
      CComPtr<ILiveLoadModelSectionResults> maxResults;
      pModelData->pLiveLoadResponse[tmpbat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
             roMember, fetFy, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
      pModelData->pLiveLoadResponse[tmpbat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
             roMember, fetFy, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

      // Extract reactions and corresponding rotations
      Float64 FyMaxLeft, FyMaxRight;
      CComPtr<ILiveLoadConfiguration> FyMaxLeftConfig, FyMaxRightConfig;
      maxResults->GetResult(0,&FyMaxLeft, &FyMaxLeftConfig,
                              &FyMaxRight,&FyMaxRightConfig);

      Float64 FyMinLeft, FyMinRight;
      CComPtr<ILiveLoadConfiguration> FyMinLeftConfig, FyMinRightConfig;
      minResults->GetResult(0,&FyMinLeft,  &FyMinLeftConfig,
                              &FyMinRight, &FyMinRightConfig);

      if (idx==0)
      {
         // Left End
         // Reaction
         *pLeftRmin = FyMaxRight * lldfRatio;
         *pLeftRmax = FyMinRight * lldfRatio;

         // Vehicle indexes
         if ( pLeftMinVehIdx )
            FyMaxLeftConfig->get_VehicleIndex(pLeftMinVehIdx);

         if ( pLeftMaxVehIdx )
            FyMinLeftConfig->get_VehicleIndex(pLeftMaxVehIdx);

         // Corresponding rotations
         // get rotatation that corresonds to R min
         CComPtr<ISectionResult3Ds> results;
         FyMinLeftConfig->put_ForceEffect(fetRz);
         FyMinLeftConfig->put_Optimization(optMaximize);
         response->ComputeDeflections( m_LBAMPoi, bstrStage, FyMaxLeftConfig, &results );

         CComPtr<ISectionResult3D> result;
         results->get_Item(0,&result);

         Float64 T;
         result->get_ZLeft(&T);
         *pLeftTmin = T * lldfRatio;

         results.Release();
         result.Release();

         // get rotation that corresonds to R max
         FyMaxLeftConfig->put_ForceEffect(fetRz);
         FyMaxLeftConfig->put_Optimization(optMaximize);
         response->ComputeDeflections( m_LBAMPoi, bstrStage, FyMinLeftConfig, &results );

         results->get_Item(0,&result);
         result->get_ZLeft(&T);
         *pLeftTmax = T * lldfRatio;
      }
      else
      {
         // Right End 
         // Reaction
         *pRightRmin = FyMinLeft * lldfRatio;
         *pRightRmax = FyMaxLeft * lldfRatio;

         // Vehicle indexes
         if ( pRightMinVehIdx )
            FyMinRightConfig->get_VehicleIndex(pRightMinVehIdx);

         if ( pRightMaxVehIdx )
            FyMaxRightConfig->get_VehicleIndex(pRightMaxVehIdx);

         // Corresponding rotations
         // get rotation that corresonds to R min
         CComPtr<ISectionResult3Ds> results;
         FyMaxRightConfig->put_ForceEffect(fetRz);
         FyMaxRightConfig->put_Optimization(optMaximize);
         response->ComputeDeflections( m_LBAMPoi, bstrStage, FyMinRightConfig, &results );

         CComPtr<ISectionResult3D> result;
         results->get_Item(0,&result);

         Float64 T;
         result->get_ZRight(&T);
         *pRightTmin = T * lldfRatio;

         results.Release();
         result.Release();

         // get rotatation that corresonds to R max
         FyMinRightConfig->put_ForceEffect(fetRz);
         FyMinRightConfig->put_Optimization(optMaximize);
         response->ComputeDeflections( m_LBAMPoi, bstrStage, FyMaxRightConfig, &results );

         results->get_Item(0,&result);
         result->get_ZRight(&T);
         *pRightTmax = T * lldfRatio;
      }
   }
}

void CAnalysisAgentImp::GetBearingLiveLoadRotation(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const CGirderKey& girderKey,
                                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                                   Float64* pLeftTmin,Float64* pLeftTmax,Float64* pLeftRmin,Float64* pLeftRmax,
                                                   Float64* pRightTmin,Float64* pRightTmax,Float64* pRightRmin,Float64* pRightRmax,
                                                   VehicleIndexType* pLeftMinVehIdx,VehicleIndexType* pLeftMaxVehIdx,
                                                   VehicleIndexType* pRightMinVehIdx,VehicleIndexType* pRightMaxVehIdx)
{
   // This is just end shears and rotations due to live load at ends of girder
   // Get Pois at supports
   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi1( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),           POI_ERECTED_SEGMENT | POI_0L, POIFIND_OR) );
   std::vector<pgsPointOfInterest> vPoi2( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,nSegments-1), POI_ERECTED_SEGMENT | POI_10L,POIFIND_OR) );
   ATLASSERT(vPoi1.size()==1);
   ATLASSERT(vPoi2.size()==1);

   pgsPointOfInterest lftPoi(vPoi1.front());
   pgsPointOfInterest rgtPoi(vPoi2.front());

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComBSTR bstrStage( GetLBAMStageName(intervalIdx) );

   // Loop twice to pick up left and right ends
   for (IndexType idx=0; idx<2; idx++)
   {
      std::vector<pgsPointOfInterest> vPoi;
      if (idx==0)
      {
         vPoi.push_back(lftPoi);
      }
      else
      {
         vPoi.push_back(rgtPoi);
      }

      // Get max'd rotations from lbam
      ModelData* pModelData = UpdateLBAMPois(vPoi);
      CComPtr<ILBAMAnalysisEngine> pEngine;
      GetEngine(pModelData,bat == pgsTypes::SimpleSpan ? false : true, &pEngine);
      CComPtr<IBasicVehicularResponse> response;
      pEngine->get_BasicVehicularResponse(&response);

      CComPtr<ILiveLoadModelSectionResults> minResults;
      CComPtr<ILiveLoadModelSectionResults> maxResults;

      if ( bat == pgsTypes::SimpleSpan || bat == pgsTypes::ContinuousSpan )
      {
         pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                fetRz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
         pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                fetRz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);
      }
      else
      {
         if ( idx == 0 )
         {
            pgsTypes::BridgeAnalysisType batLeft = (bat == pgsTypes::MinSimpleContinuousEnvelope ? pgsTypes::MaxSimpleContinuousEnvelope : pgsTypes::MinSimpleContinuousEnvelope);
            pModelData->pLiveLoadResponse[batLeft]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                   fetRz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
            pModelData->pLiveLoadResponse[batLeft]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                   fetRz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);
         }
         else
         {
            pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                   fetRz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
            pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                   fetRz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);
         }
      }


      // Extract rotations and corresponding reactions
      Float64 TzMaxLeft, TzMaxRight;
      CComPtr<ILiveLoadConfiguration> TzMaxLeftConfig, TzMaxRightConfig;
      maxResults->GetResult(0,&TzMaxLeft, &TzMaxLeftConfig,
                              &TzMaxRight,&TzMaxRightConfig);

      Float64 TzMinLeft, TzMinRight;
      CComPtr<ILiveLoadConfiguration> TzMinLeftConfig, TzMinRightConfig;
      minResults->GetResult(0,&TzMinLeft,  &TzMinLeftConfig,
                              &TzMinRight, &TzMinRightConfig);

      if (idx==0)
      {
         // Left End
         // Rotation
         *pLeftTmin = TzMinLeft;
         *pLeftTmax = TzMaxLeft;

         // Vehicle indexes
         if ( pLeftMinVehIdx )
            TzMinLeftConfig->get_VehicleIndex(pLeftMinVehIdx);

         if ( pLeftMaxVehIdx )
            TzMaxLeftConfig->get_VehicleIndex(pLeftMaxVehIdx);

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
         *pLeftRmin = -T;

         results.Release();
         result.Release();

         // get rotation that corresonds to R max
         TzMaxLeftConfig->put_ForceEffect(fetRz);
         TzMaxLeftConfig->put_Optimization(optMaximize);
         response->ComputeForces( m_LBAMPoi, bstrStage, roMember, TzMaxLeftConfig, &results );

         results->get_Item(0,&result);
         result->get_YLeft(&T);
         *pLeftRmax = -T;
      }
      else
      {
         // Right End - have to play games with shear sign convention
         // Rotation
         *pRightTmin = TzMinRight;
         *pRightTmax = TzMaxRight;

         // Vehicle indexes
         if ( pRightMinVehIdx )
            TzMinRightConfig->get_VehicleIndex(pRightMinVehIdx);

         if ( pRightMaxVehIdx )
            TzMaxRightConfig->get_VehicleIndex(pRightMaxVehIdx);

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
         *pRightRmin = -T;

         results.Release();
         result.Release();

         // get rotation that corresonds to R max
         TzMaxRightConfig->put_ForceEffect(fetRz);
         TzMaxRightConfig->put_Optimization(optMaximize);
         response->ComputeForces( m_LBAMPoi, bstrStage, roMember, TzMaxRightConfig, &results );

         results->get_Item(0,&result);
         result->get_YRight(&T);
         *pRightRmax = -T;
      }
   }
}

void CAnalysisAgentImp::GetBearingCombinedReaction(LoadingCombination combo,IntervalIndexType intervalIdx,const CGirderKey& girderKey,
                                                   CombinationType cmb_type,pgsTypes::BridgeAnalysisType bat, Float64* pLftEnd,Float64* pRgtEnd)
{
   // Use lbam to get load caes for this combination
   ModelData* pModelData = GetModelData(girderKey.girderIndex);

   CComPtr<ILBAMModel> lbam;
   GetModel(pModelData, bat, &lbam);

   CComPtr<ILoadCases> load_cases;
   lbam->get_LoadCases(&load_cases);

   CComBSTR combo_name = GetLoadCaseName(combo);

   CComPtr<ILoadCase> load_case;
   load_cases->Find(combo_name, &load_case);

   CollectionIndexType nLoadGroups;
   load_case->get_LoadGroupCount(&nLoadGroups);

   // Cycle through load cases and sum reactions
   Float64 Rlft(0.0), Rrgt(0.0);
   for (CollectionIndexType ldGroupIdx = 0; ldGroupIdx < nLoadGroups; ldGroupIdx++)
   {
      CComBSTR lg_name;
      load_case->GetLoadGroup(ldGroupIdx, &lg_name);

      ProductForceType prodType = GetProductForceType(lg_name); 

      Float64 lft, rgt;
      GetBearingProductReaction(intervalIdx, prodType, girderKey, cmb_type, bat, &lft, &rgt);

      Rlft += lft;
      Rrgt += rgt;
   }

   *pLftEnd = Rlft;
   *pRgtEnd = Rrgt;
}

void CAnalysisAgentImp::GetBearingCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const CGirderKey& girderKey,
                                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                           Float64* pLeftRmin, Float64* pLeftRmax, Float64* pRightRmin,Float64* pRightRmax)
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   ATLASSERT(pIntervals->GetLiveLoadInterval(girderKey) <= intervalIdx);
#endif

   // Get bearing reactions by getting beam end shears.
   // Get Pois at supports
   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   SpanIndexType leftSpanIdx,rightSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&leftSpanIdx,&rightSpanIdx);

   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi1( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),           POI_ERECTED_SEGMENT | POI_0L, POIFIND_OR) );
   std::vector<pgsPointOfInterest> vPoi2( pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,nSegments-1), POI_ERECTED_SEGMENT | POI_10L,POIFIND_OR) );
   ATLASSERT(vPoi1.size()==1);
   ATLASSERT(vPoi2.size()==1);

   pgsPointOfInterest lftPoi(vPoi1.front());
   pgsPointOfInterest rgtPoi(vPoi2.front());

   sysSectionValue lft_min_sec_val, lft_max_sec_val, rgt_min_sec_val, rgt_max_sec_val;

  // TRICKY:
   // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
   // that the optimization must go the opposite way as well when using the envelopers
   pgsTypes::BridgeAnalysisType right_bat(bat);
   if (bat == pgsTypes::MinSimpleContinuousEnvelope)
   {
      right_bat = pgsTypes::MaxSimpleContinuousEnvelope;
   }
   else if (bat == pgsTypes::MaxSimpleContinuousEnvelope)
   {
      right_bat = pgsTypes::MinSimpleContinuousEnvelope;
   }

   GetCombinedLiveLoadShear(llType, intervalIdx, lftPoi, bat, bIncludeImpact, &lft_min_sec_val, &lft_max_sec_val);
   GetCombinedLiveLoadShear(llType, intervalIdx, rgtPoi, right_bat, bIncludeImpact, &rgt_min_sec_val, &rgt_max_sec_val);
   // Shear and reaction LLDF's can be different. Must ratio results by reaction/shear.
   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   bool is_fatigue = llType==pgsTypes::lltFatigue || llType==pgsTypes::lltPermitRating_Special;
   pgsTypes::LimitState lldfLimitState = is_fatigue ? pgsTypes::FatigueI : pgsTypes::StrengthI;
   Float64 lldfLeftShear  = pLLDF->GetShearDistFactor(leftSpanIdx, girderKey.girderIndex,lldfLimitState);
   Float64 lldfRightShear = pLLDF->GetShearDistFactor(rightSpanIdx,girderKey.girderIndex,lldfLimitState);

   Float64 lldfLeftReact  = pLLDF->GetReactionDistFactor(leftSpanIdx,  girderKey.girderIndex, lldfLimitState);
   Float64 lldfRightReact = pLLDF->GetReactionDistFactor(rightSpanIdx, girderKey.girderIndex, lldfLimitState);

   Float64 lldfLeftRatio  = lldfLeftReact/lldfLeftShear;
   Float64 lldfRightRatio = lldfRightReact/lldfRightShear;

   *pLeftRmin =  lft_min_sec_val.Right() * lldfLeftRatio;
   *pLeftRmax =  lft_max_sec_val.Right() * lldfLeftRatio;

   *pRightRmin = -rgt_max_sec_val.Left() * lldfRightRatio;
   *pRightRmax = -rgt_min_sec_val.Left() * lldfRightRatio;
}

void CAnalysisAgentImp::GetBearingLimitStateReaction(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const CGirderKey& girderKey,
                                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                     Float64* pLeftRmin, Float64* pLeftRmax, Float64* pRightRmin,Float64* pRightRmax)
{
   // We have to emulate what the LBAM does for load combinations here
   *pLeftRmin  = 0.0;
   *pLeftRmax  = 0.0;
   *pRightRmin = 0.0;
   *pRightRmax = 0.0;

   // Use lbam to get load factors for this limit state
   ModelData* pModelData = GetModelData(girderKey.girderIndex);

   CComPtr<ILBAMModel> lbam;
   GetModel(pModelData, bat, &lbam);

   CComPtr<ILoadCombinations> load_combos;
   lbam->get_LoadCombinations(&load_combos);

   CComBSTR combo_name = GetLoadCombinationName(ls);

   CComPtr<ILoadCombination> load_combo;
   load_combos->Find(combo_name, &load_combo);

   // First factor load cases
   CollectionIndexType lc_cnt;
   load_combo->get_LoadCaseFactorCount(&lc_cnt);
   for (CollectionIndexType lc_idx=0; lc_idx<lc_cnt; lc_idx++)
   {
      CComBSTR lc_name;
      Float64 min_factor, max_factor;
      load_combo->GetLoadCaseFactor(lc_idx, &lc_name, &min_factor, &max_factor);

      LoadingCombination combo;
      if(GetLoadCaseTypeFromName(lc_name, &combo))
      {
         Float64 lc_lft_res, lc_rgt_res;
         GetBearingCombinedReaction(combo, intervalIdx, girderKey, ctCumulative, bat, &lc_lft_res, &lc_rgt_res);

         *pLeftRmin  += min_factor * lc_lft_res;
         *pLeftRmax  += max_factor * lc_lft_res;
         *pRightRmin += min_factor * lc_rgt_res;
         *pRightRmax += max_factor * lc_rgt_res;
      }
   }

   // Next, factor and combine live load
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);
   if(liveLoadIntervalIdx <= intervalIdx)
   {
      Float64 LlLeftRmin(Float64_Max), LlLeftRmax(-Float64_Max), LlRightRmin(Float64_Max), LlRightRmax(-Float64_Max);

      CollectionIndexType nlls;
      load_combo->GetLiveLoadModelCount(&nlls);

      for (CollectionIndexType ills=0; ills<nlls; ills++)
      {
         LiveLoadModelType llm_type;
         load_combo->GetLiveLoadModel(0, &llm_type);

         pgsTypes::LiveLoadType lltype = GetLiveLoadTypeFromModelType(llm_type);

         // Only envelope pedestrian load if it exists
         if (lltype == pgsTypes::lltPedestrian && !HasPedestrianLoad(girderKey))
            break;

         Float64 leftRmin, leftRmax, rightRmin, rightRmax;
         GetBearingCombinedLiveLoadReaction(lltype, intervalIdx, girderKey, bat, bIncludeImpact,
                                            &leftRmin, &leftRmax, &rightRmin, &rightRmax);

         LlLeftRmin  = Min(LlLeftRmin, leftRmin);
         LlLeftRmax  = Max(LlLeftRmax, leftRmax);
         LlRightRmin = Min(LlRightRmin, rightRmin);
         LlRightRmax = Max(LlRightRmax, rightRmax);
      }

      Float64 ll_factor;
      load_combo->get_LiveLoadFactor(&ll_factor);

      *pLeftRmin  += ll_factor * LlLeftRmin;
      *pLeftRmax  += ll_factor * LlLeftRmax ;
      *pRightRmin += ll_factor * LlRightRmin;
      *pRightRmax += ll_factor * LlRightRmax;
   }

#pragma Reminder("UPDATE: assuming precast girder bridge")
   SpanIndexType span = girderKey.groupIndex;
   // Last, factor in load modifier
   CComPtr<ISpans> lbspans;
   lbam->get_Spans(&lbspans);
   CComPtr<ISpan> lbspan;
   lbspans->get_Item(span, &lbspan);

   Float64 lm_min, lm_max;
   lbspan->GetLoadModifier(lctStrength, &lm_min, &lm_max);

   *pLeftRmin  *= lm_min;
   *pLeftRmax  *= lm_max;
   *pRightRmin *= lm_min;
   *pRightRmax *= lm_max;
}


// Implementation functions and data for IBearingDesign
void CAnalysisAgentImp::ApplyOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup,
                                                MemberIDType mbrID,Float64 Pstart, Float64 Xstart, Float64 Pend, Float64 Xend, IPointLoads* pointLoads)
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

void CAnalysisAgentImp::SaveOverhangPointLoads(const CSegmentKey& segmentKey,pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage,const CComBSTR& bstrLoadGroup,Float64 Pstart,Float64 Pend)
{
   if ( !IsZero(Pstart) || !IsZero(Pend) )
   {
      OverhangLoadDataType new_val(segmentKey, bstrStage, bstrLoadGroup, Pstart, Pend);
      std::pair<OverhangLoadIterator,bool> lit = m_OverhangLoadSet[analysisType].insert( new_val );
      if ( lit.second == false )
      {
         lit.first->PStart += Pstart;
         lit.first->PEnd   += Pend;
      }
   }
}

bool CAnalysisAgentImp::GetOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType, IntervalIndexType intervalIdx,ProductForceType type,
                                              CombinationType cmbtype, Float64* pPStart, Float64* pPEnd)
{
   ATLASSERT(analysisType == pgsTypes::Simple || analysisType == pgsTypes::Continuous);
   *pPStart = 0.0;
   *pPEnd   = 0.0;

   // Need to sum results over stages, so use bridgesite ordering 
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(segmentKey);
   IntervalIndexType startIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
   std::vector<IntervalIndexType> intervals;
   for ( IntervalIndexType i = startIntervalIdx; i < nIntervals; i++ )
   {
      intervals.push_back(i);
   }

   // Start of interval loop
   std::vector<IntervalIndexType>::iterator found = std::find(intervals.begin(),intervals.end(),intervalIdx);
   IntervalIndexType end = *found;
   IntervalIndexType start = end;

   if (found == intervals.end() )
   {
      ATLASSERT(0); // shouldn't be passing in non-bridge site stages?
      return false;
   }
   else
   {
      GET_IFACE(IEventMap,pEventMap);

      CComBSTR bstrLoadGroup( GetLoadGroupName(type) );

      // Determine end of loop range
      if (cmbtype == ctCumulative)
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

         OverhangLoadIterator lit = m_OverhangLoadSet[analysisType].find( OverhangLoadDataType(segmentKey, bstrStage, bstrLoadGroup, 0.0, 0.0) );
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




/////////////////////////////////////////////////
CAnalysisAgentImp::SpanType CAnalysisAgentImp::GetSpanType(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bContinuous)
{
   if ( !bContinuous )
      return PinPin;

   GET_IFACE(IBridge,pBridge);
   PierIndexType prev_pier = spanIdx;
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
      return FixFix;

   if ( bContinuousStart && !bContinuousEnd )
      return FixPin;

   if ( !bContinuousStart && bContinuousEnd )
      return PinFix;

   if ( !bContinuousStart && !bContinuousEnd )
      return PinPin;

   ATLASSERT(0); // should never get here
   return PinPin;
}

void CAnalysisAgentImp::AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gV,Float64 gR,
                                               Float64 gFM,Float64 gFV,Float64 gD, Float64 gPedes)
{
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

Uint32 CAnalysisAgentImp::GetCfPointsInRange(IDblArray* cfLocs, Float64 spanStart, Float64 spanEnd, Float64* ptsInrg)
{
   if ( cfLocs == NULL )
      return 0;

   // we don't want to pick up cf points at the very ends
   const Float64 TOL=1.0e-07;
   spanStart+= TOL;
   spanEnd-=TOL;

   Uint32 cf_cnt=0;

   // assumption here is that the cfLocs are sorted
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
            ATLASSERT(0);
            // "More than two contraflexure points in a Span - This should be impossible"
         }
      }
      
      // no use continuing loop if we are past the span end
      if (spanEnd < cfl)
         break;
   }


   return cf_cnt;
}

void CAnalysisAgentImp::ApplyLLDF_PinPin(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors)
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);

#if defined _DEBUG
   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdr = Min(gdrIdx,nGirders-1);
      span_start += pBridge->GetSpanLength(i,gdr);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   long num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);
   ATLASSERT(num_cf_points_in_span == 0);
   // there shouldn't be any contraflexure points in a pin-pin span
#endif

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);

   Float64 gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gnM = pLLDF->GetNegMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanIdx,gdrIdx);
   Float64 gPedes = this->GetPedestrianLiveLoad(spanIdx,gdrIdx); // factor is magnitude of pedestrian live load

   AddDistributionFactors(distFactors,span_length,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
}

void CAnalysisAgentImp::ApplyLLDF_PinFix(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors)
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);

   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdr = Min(gdrIdx,nGirders-1);
      span_start += pBridge->GetSpanLength(i,gdr);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   long num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

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
   Float64 gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gnM = pLLDF->GetNegMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanIdx,gdrIdx);

   Float64 gPedes = this->GetPedestrianLiveLoad(spanIdx,gdrIdx); // factor is magnitude of pedestrian live load

   AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);

   // for the second part of the span, use the negative moment distribution factor that goes over the next pier
   PierIndexType pierIdx = spanIdx+1;
   gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);

   AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
}

void CAnalysisAgentImp::ApplyLLDF_FixPin(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors)
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);

   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdr = Min(gdrIdx,nGirders-1);
      span_start += pBridge->GetSpanLength(i,gdr);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   long num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

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

   PierIndexType pierIdx = spanIdx;

   // distribution factors from span
   Float64 gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead); // DF over pier at start of span
   Float64 gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gR  =  99999999; // this parameter not should be used so use a value that is obviously wrong to easily detect bugs
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanIdx,gdrIdx);

   Float64 gPedes = this->GetPedestrianLiveLoad(spanIdx,gdrIdx); // factor is magnitude of pedestrian live load

   AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);

   gnM = pLLDF->GetNegMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI); // DF in the span
   AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
}

void CAnalysisAgentImp::ApplyLLDF_FixFix(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors)
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);

   // sum length of previous spans
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(i);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdr = Min(gdrIdx,nGirders-1);
      span_start += pBridge->GetSpanLength(i,gdr);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];
   long num_cf_points_in_span = GetCfPointsInRange(cf_locs,span_start,span_end,cf_points_in_span);

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   Float64 gpM;
   Float64 gnM;
   Float64 gV; 
   Float64 gR;
   Float64 gFM  = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gFV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanIdx,gdrIdx);
   Float64 gPedes = this->GetPedestrianLiveLoad(spanIdx,gdrIdx); // factor is magnitude of pedestrian live load

   if ( num_cf_points_in_span == 0 )
   {
      // split span in half and use pier neg moment df for each half
      PierIndexType pierIdx = spanIdx;

      gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gR  = 99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD, gPedes);
      
#pragma Reminder("BUG? Review gdrID and segIdx passed into this method") // assumes precast girders brdige
      pierIdx = spanIdx+1;
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
      AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
   else if ( num_cf_points_in_span == 1 )
   {
      PierIndexType pierIdx = spanIdx;
      gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gR  = 99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 seg_length_1 = cf_points_in_span[0] - span_start;
      Float64 seg_length_2 = span_end - cf_points_in_span[0];

      AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      
      pierIdx = spanIdx+1;
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
      AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
   else
   {
      PierIndexType pierIdx = spanIdx;
      gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 seg_length_1 = cf_points_in_span[0] - span_start;
      Float64 seg_length_2 = cf_points_in_span[1] - cf_points_in_span[0];
      Float64 seg_length_3 = span_end - cf_points_in_span[1];

      AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      
      gnM = pLLDF->GetNegMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      
      pierIdx = spanIdx+1;
      gnM = pLLDF->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
      AddDistributionFactors(distFactors,seg_length_3,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
}

void CAnalysisAgentImp::ApplyLLDF_Support(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType endType,ISupports* supports)
{
   PierIndexType pierIdx = (endType == pgsTypes::metStart ? spanIdx : spanIdx+1);

   CComPtr<ISupport> support;
   supports->get_Item(pierIdx,&support);
   CComPtr<IDistributionFactor> df;
   support->get_DistributionFactor(&df);

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   Float64 gpM = 99999999;
   Float64 gnM = 99999999;
   Float64 gV  = 99999999;
   Float64 gR  = pLLDF->GetReactionDistFactor(pierIdx,gdrIdx,pgsTypes::StrengthI);
   Float64 gF  = pLLDF->GetReactionDistFactor(pierIdx,gdrIdx,pgsTypes::FatigueI);
   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanIdx,gdrIdx);

   // For pedestrian loads - take average of loads from adjacent spans
   Float64 leftPedes(0.0), rightPedes(0.0);
   Int32 nls(0);
   if(0 < pierIdx)
   {
      SpanIndexType prevSpanIdx = (SpanIndexType)(pierIdx-1);
      leftPedes = GetPedestrianLiveLoad(prevSpanIdx,gdrIdx);
      nls++;
   }

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if (pierIdx < nSpans)
   {
      SpanIndexType nextSpanIdx = (SpanIndexType)(pierIdx);
      rightPedes = GetPedestrianLiveLoad(nextSpanIdx,gdrIdx);
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
}

void CAnalysisAgentImp::GetEngine(ModelData* pModelData,bool bContinuous,ILBAMAnalysisEngine** pEngine)
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

Float64 CAnalysisAgentImp::GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

#pragma Reminder("REVIEW: review this code...possible bug")
   // does it still make sense for the config object to have Fci and Fc?
   // config may be something we get at an interval and it just has Ec and Fc?
   Float64 fc = (intervalIdx == releaseIntervalIdx ? config.Fci : config.Fc);

   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Ix          = pSectProp->GetIx(intervalIdx,poi);
   Float64 Ix_adjusted = pSectProp->GetIx(intervalIdx,poi,fc);

   GET_IFACE(IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(poi.GetSegmentKey(),intervalIdx);
   GET_IFACE(IMaterials,pMaterial);
   Float64 Ec_adjusted = (config.bUserEc ? config.Ec : pMaterial->GetEconc(fc,pMaterial->GetSegmentStrengthDensity(poi.GetSegmentKey()),
                                                                              pMaterial->GetSegmentEccK1(poi.GetSegmentKey()),
                                                                              pMaterial->GetSegmentEccK2(poi.GetSegmentKey())));

   Float64 EI = Ec*Ix;
   Float64 EI_adjusted = Ec_adjusted * Ix_adjusted;

   Float64 k = (IsZero(EI_adjusted) ? 0 : EI/EI_adjusted);

   return k;
}

CAnalysisAgentImp::ModelData* CAnalysisAgentImp::UpdateLBAMPois(const std::vector<pgsPointOfInterest>& vPoi)
{
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
   GirderIndexType gdrIdx = 0;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      SpanIndexType s = poi.GetSegmentKey().groupIndex;
      GirderIndexType g = poi.GetSegmentKey().girderIndex;

      gdrIdx = Max(gdrIdx,g);
   }

   // get the model
   ModelData* pModelData = 0;
   pModelData = GetModelData(gdrIdx);

   iter = vPoi.begin();
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
      if ( poi_id == INVALID_ID )
      {
         poi_id = AddPointOfInterest( pModelData, poi );
         ATLASSERT( 0 <= poi_id );
         if ( 0 <= poi_id )
            m_LBAMPoi->Add(poi_id);
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

void CAnalysisAgentImp::GetVehicularLoad(ILBAMModel* pModel,LiveLoadModelType llType,VehicleIndexType vehicleIndex,IVehicularLoad** pVehicle)
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

      case lltLegalSpecialRating :
         live_load->get_LegalSpecialRating(&liveload_model);
         break;

      case lltPermitRoutineRating:
         live_load->get_PermitRoutineRating(&liveload_model);
         break;

      case lltPermitSpecialRating:
         live_load->get_PermitSpecialRating(&liveload_model);
         break;

      case lltNone:
         *pVehicle = NULL;
         return;

     default:
        ATLASSERT(false); // is there a new load?
        *pVehicle = NULL;
        return;
   }

   CComPtr<IVehicularLoads> vehicles;
   liveload_model->get_VehicularLoads(&vehicles);

   vehicles->get_Item(vehicleIndex,pVehicle);
}

void CAnalysisAgentImp::CreateAxleConfig(ILBAMModel* pModel,ILiveLoadConfiguration* pConfig,AxleConfiguration* pAxles)
{
   pAxles->clear();

   LiveLoadModelType llType;
   pConfig->get_LiveLoadModel(&llType);

   CComPtr<IVehicularLoad> pVehicle;
   VehicleIndexType vehicleIndex;
   pConfig->get_VehicleIndex(&vehicleIndex);
   GetVehicularLoad(pModel,llType,vehicleIndex,&pVehicle);

   if ( !pVehicle )
      return;

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
      return;

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
      while ( enum_array->Next(1,&value,NULL) != S_FALSE )
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
         spacing = variable_axle_spacing;

      axleLocation += sign*spacing;
   }

   // get the location of the pivot axle, from the front of the truck
   Float64 pivot_axle_location;
   pConfig->get_TruckPosition(&pivot_axle_location);

   AxleIndexType pivot_axle_index;
   pConfig->get_PivotAxleIndex(&pivot_axle_index);

   Float64 pivot_axle_offset = ((*pAxles)[pivot_axle_index]).Location;

   // locate the axles relative to the pivot axle, then move them to the truck location
   AxleConfiguration::iterator iter;
   for ( iter = pAxles->begin(); iter != pAxles->end(); iter++ )
   {
      AxlePlacement& placement = *iter;
      placement.Location += pivot_axle_location - pivot_axle_offset;
   }
}

void CAnalysisAgentImp::GetModel(ModelData* pModelData,pgsTypes::BridgeAnalysisType bat,ILBAMModel** ppModel)
{
   switch( bat )
   {
   case pgsTypes::SimpleSpan:
      (*ppModel) = pModelData->m_Model;
      (*ppModel)->AddRef();

      break;

   case pgsTypes::ContinuousSpan:
   case pgsTypes::MinSimpleContinuousEnvelope:
   case pgsTypes::MaxSimpleContinuousEnvelope:
      (*ppModel) = pModelData->m_ContinuousModel;
      (*ppModel)->AddRef();

      break;

   default:
      ATLASSERT(false);
   }
}

DistributionFactorType CAnalysisAgentImp::GetLiveLoadDistributionFactorType(pgsTypes::LiveLoadType llType)
{
   DistributionFactorType dfType;
   GET_IFACE(IRatingSpecification,pRatingSpec);

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

Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 e)
{
   Float64 A, S; 
   GET_IFACE(ISectionProperties,pSectProp);
   A = pSectProp->GetAg(intervalIdx,poi);
   S = pSectProp->GetS(intervalIdx,poi,stressLocation);

   ATLASSERT( !IsZero(S) );

   Float64 f = -P/A - P*e/S;

   return f;
}

bool CAnalysisAgentImp::CreateLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LoadType loadType,Float64 P,LPCTSTR strLoadGroupName)
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

   if ( loadType == ltFx )
      ptLoad->put_Fx(P);
   else if ( loadType == ltFy )
      ptLoad->put_Fy(P);
   else if ( loadType == ltMz )
      ptLoad->put_Mz(P);

   pointLoads->Add(bstrStage,CComBSTR(strLoadGroupName),ptLoad,&ptLoadItem);

   return true;
}

bool CAnalysisAgentImp::CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r,LPCTSTR strLoadGroupName)
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
#pragma Reminder("UPDATE: assuming ssmbr ID is the same as its index")
      // need to be able to find a superstructure member based on its ID
      ssmbrs->get_Item(mbrID1,&ssmbr1);

      Float64 length1;
      ssmbr1->get_Length(&length1);

      for ( IDType id = mbrID1; id <= mbrID2; id++ )
      {
         if ( id == mbrID1 && !IsEqual(location1,length1) )
         {
            CComPtr<IStrainLoadItem> strainLoadItem;
            CComPtr<IStrainLoad> strainLoad;
            strainLoad.CoCreateInstance(CLSID_StrainLoad);
            strainLoad->put_MemberType(mbrType1); // mbrType1 must equal mbrType2
            strainLoad->put_MemberID(mbrID1);
            strainLoad->put_StartLocation(location1);
            strainLoad->put_EndLocation(-1.0);
            strainLoad->put_AxialStrain(e);
            strainLoad->put_CurvatureStrain(r);
            strainLoads->Add(bstrStage,CComBSTR(strLoadGroupName),strainLoad,&strainLoadItem);
         }
         else if ( id == mbrID2 && !IsZero(location2) )
         {
            CComPtr<IStrainLoadItem> strainLoadItem;
            CComPtr<IStrainLoad> strainLoad;
            strainLoad.CoCreateInstance(CLSID_StrainLoad);
            strainLoad->put_MemberType(mbrType2); // mbrType1 must equal mbrType2
            strainLoad->put_MemberID(mbrID2);
            strainLoad->put_StartLocation(0);
            strainLoad->put_EndLocation(location2);
            strainLoad->put_AxialStrain(e);
            strainLoad->put_CurvatureStrain(r);
            strainLoads->Add(bstrStage,CComBSTR(strLoadGroupName),strainLoad,&strainLoadItem);
         }
         else
         {
            CComPtr<IStrainLoadItem> strainLoadItem;
            CComPtr<IStrainLoad> strainLoad;
            strainLoad.CoCreateInstance(CLSID_StrainLoad);
            strainLoad->put_MemberType(mbrType1); // mbrType1 must equal mbrType2
            strainLoad->put_MemberID(id);
            strainLoad->put_StartLocation(0);
            strainLoad->put_EndLocation(-1);
            strainLoad->put_AxialStrain(e);
            strainLoad->put_CurvatureStrain(r);
            strainLoads->Add(bstrStage,CComBSTR(strLoadGroupName),strainLoad,&strainLoadItem);
         }
      }
   }
   else
   {
      CComPtr<IStrainLoadItem> strainLoadItem;
      CComPtr<IStrainLoad> strainLoad;
      strainLoad.CoCreateInstance(CLSID_StrainLoad);
      strainLoad->put_MemberType(mbrType1); // mbrType1 must equal mbrType2
      strainLoad->put_MemberID(mbrID1); // mbrID1 must equal mbrID2
      strainLoad->put_StartLocation(location1);
      strainLoad->put_EndLocation(location2);
      strainLoad->put_AxialStrain(e);
      strainLoad->put_CurvatureStrain(r);

      strainLoads->Add(bstrStage,CComBSTR(strLoadGroupName),strainLoad,&strainLoadItem);
   }

   return true;
}

MemberIDType CAnalysisAgentImp::ApplyDistributedLoads(IntervalIndexType intervalIdx,ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,MemberIDType ssmbrID,const CSegmentKey& segmentKey,const std::vector<LinearLoad>& vLoads,BSTR bstrStage,BSTR bstrLoadGroup)
{
#if defined _DEBUG
   // used below to error check loading geometry
   CComPtr<ISuperstructureMembers> ssmbrs;
   pModel->get_SuperstructureMembers(&ssmbrs);
#endif

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType continuityIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   bool bIsContinuous = (continuityIntervalIdx <= intervalIdx);

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
   pSegment->GetStartSupport(&pStartPier,&pStartTS);

   const CPierData2* pEndPier;
   const CTemporarySupportData* pEndTS;
   pSegment->GetEndSupport(&pEndPier,&pEndTS);


   Float64 start_offset     = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_offset       = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 segment_length   = pBridge->GetSegmentLength(segmentKey);
   Float64 L[3] = {start_offset, segment_length - start_offset - end_offset, end_offset};

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_0L);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest startPoi(vPoi.front());

   vPoi = pPOI->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_10L);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest endPoi(vPoi.front());

   GET_IFACE(IGirder,pGdr);
   Float64 HgStart = pGdr->GetHeight(startPoi);
   Float64 HgEnd   = pGdr->GetHeight(endPoi);

   bool bModelStartCantilever,bModelEndCantilever;
   pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

   // apply distributed load items
   std::vector<LinearLoad>::const_iterator iter(vLoads.begin());
   std::vector<LinearLoad>::const_iterator iterEnd(vLoads.end());
   for ( ; iter != iterEnd; iter++ )
   {
      LinearLoad load = *iter;

      // If cantilevers are not explicitly  modeled, point loads at start and end to account 
      // for the load in the overhang/cantilever. These are mostly used to make sure
      // the dead load reactions are correct. Moments are not modeled if the length of the
      // cantilever is less than the depth of the girder (it is considered a deep beam)
      Float64 Pstart(0.0), Pend(0.0); // point load
      Float64 Mstart(0.0), Mend(0.0); // moment

      Float64 start[3],  end[3]; // start and end of load on the 3 superstructure members modeling this segment
      Float64 wStart[3], wEnd[3]; // the start/end loads

      // default loads (assuming load is not on the SSMBR)
      for ( int i = 0; i < 3; i++ )
      {
         start[i]  = 0;
         end[i]    = L[i];
         wStart[i] = 0;
         wEnd[i]   = 0;
      }

      // Load on first superstructure member
      if ( load.StartLoc < start_offset )
      {
         // load starts on SSMBR at start of segment
         Float64 end_loc   = (load.EndLoc < start_offset ? load.EndLoc : start_offset);
         Float64 end_load  = (load.EndLoc < start_offset ? load.wEnd : ::LinInterp(start_offset,load.wStart,load.wEnd,end[0]-start[0]));
         Float64 start_loc = (load.StartLoc < 0 ? 0 : load.StartLoc);

         // Put a load on the cantilever if...
         if ( bModelStartCantilever // cantilever is long enough to be loaded
            || // OR
            ( !(segmentKey.segmentIndex == 0 && segmentKey.groupIndex == 0) // this is not the first segment in the first group
            && // AND
            bIsContinuous // the deck is composite
            && // AND
            analysisType == pgsTypes::Continuous // we are doing a continuous analysis
            && // AND
            ( (pStartTS) // the segment is supported by a temporary support
               || // OR
               (pStartPier && pStartPier->IsContinuousConnection())) // the segment is suppoted by a pier that has continuous boundary conditions
            )
            )
         {
            start[0]  = 0;
            wStart[0] = load.wStart;
            end[0]    = end_loc;
            wEnd[0]   = end_load;

            load.StartLoc = end_loc;
            load.wStart   = end_load;
         }
         else
         {
            Pstart = 0.5*(load.wStart + end_load)*(end_loc - start_loc);
            Mstart = (start_offset < HgStart) ? 0.0 : -Pstart*(end_loc - start_loc)/2;

            start[0]  = 0;
            end[0]    = start_offset;
            wStart[0] = 0;
            wEnd[0]   = 0;

            load.StartLoc = end_loc;
            load.wStart   = end_load;
         }
      }

      // load on last superstructure member
      if ( segment_length - end_offset < load.EndLoc )
      {
         Float64 start_loc  = (load.EndLoc < segment_length - end_offset ?  0.0 : segment_length - end_offset);
         Float64 start_load = (load.EndLoc < segment_length - end_offset ? ::LinInterp(segment_length-end_offset-load.StartLoc,load.wStart,load.wEnd,load.EndLoc-load.StartLoc) : load.wStart);
         Float64 end_loc    = Min(load.EndLoc,segment_length);

         // put load on the cantilever if...
         if ( bModelEndCantilever // cantilever is long enough to be loaded
            || // OR
            ( !(segmentKey.segmentIndex == nSegments-1 && segmentKey.groupIndex == nGroups-1) // this is not the last segment in the last group
            && // AND
            bIsContinuous // the deck is composite 
            && // AND 
            analysisType == pgsTypes::Continuous // we are doing continuous analysis
            && // AND
            ( (pEndTS) // the segment is supported by a temporary support
               || // OR
               (pEndPier && pEndPier->IsContinuousConnection()))  // the segment is supported by a pier with continuous boundary conditions
            ) 
            )
         {
            start[2]  = 0;
            wStart[2] = start_load;
            end[2]    = end_offset;
            wEnd[2]   = load.wEnd;

            load.EndLoc = start_loc;
            load.wEnd   = start_load;
         }
         else
         {
            Pend = 0.5*(start_load + load.wEnd)*(end_loc - start_loc);
            Mend = (end_offset < HgEnd) ? 0.0 : Pend*(end_loc - start_loc)/2;

            start[2]  = 0;
            end[2]    = end_offset;
            wStart[2] = 0;
            wEnd[2]   = 0;

            load.EndLoc = start_loc;
            load.wEnd   = start_load;
         }
      }

      // load on main superstructure member
      // load location is measured from start of segment... 
      //subtract the start offset so that it is measured from the start of the SSMBR
      if ( start_offset <= load.StartLoc && load.StartLoc < segment_length-start_offset)
      {
         start[1]  = load.StartLoc - start_offset;
         end[1]    = load.EndLoc   - start_offset;
         wStart[1] = load.wStart;
         wEnd[1]   = load.wEnd;
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

            mbrID++;
         }
      }

      MemberIDType mainSSMbrID = IsZero(start_offset) ? ssmbrID : ssmbrID+1;
      ApplyOverhangPointLoads(segmentKey,analysisType,bstrStage,bstrLoadGroup,mainSSMbrID,Pstart,start[1],Pend,end[1],pointLoads);

      if ( !IsZero(Mstart) )
      {
         CComPtr<IPointLoad> load;
         load.CoCreateInstance(CLSID_PointLoad);
         load->put_MemberType(mtSuperstructureMember);
         load->put_MemberID(mainSSMbrID);
         load->put_Location(0.0);
         load->put_Mz(Mstart);

         CComPtr<IPointLoadItem> loadItem;
         pointLoads->Add(bstrStage,bstrLoadGroup,load,&loadItem);
      }

      if ( !IsZero(Mend) )
      {
         CComPtr<IPointLoad> load;
         load.CoCreateInstance(CLSID_PointLoad);
         load->put_MemberType(mtSuperstructureMember);
         load->put_MemberID(mainSSMbrID);
         load->put_Location(-1.0);
         load->put_Mz(Mend);

         CComPtr<IPointLoadItem> loadItem;
         pointLoads->Add(bstrStage,bstrLoadGroup,load,&loadItem);
      }
   } // next load

   // return the ID of the next superstructure member to be loaded
   // determine how many SSMBRs were loaded/modeled
   MemberIDType mbrIDInc = 0;
   for ( int i = 0; i < 3; i++ )
   {
      if ( !IsZero(L[i]) )
         mbrIDInc++;
   }
   return ssmbrID + mbrIDInc;
}

MemberIDType CAnalysisAgentImp::GetSuperstructureMemberID(const CSegmentKey& segmentKey)
{
   GroupIndexType   grpIdx = segmentKey.groupIndex;
   GirderIndexType  gdrIdx = segmentKey.girderIndex;
   SegmentIndexType segIdx = segmentKey.segmentIndex;
   ATLASSERT(grpIdx != ALL_GROUPS && gdrIdx != ALL_GIRDERS && segIdx != ALL_SEGMENTS);

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);

   // determine the superstructure member ID
   MemberIDType mbrID = 0;

   // count number of SSMBRs in all groups and segments before the segment we are interested in
   for ( GroupIndexType g = 0; g < grpIdx; g++ )
   {
      CGirderKey thisGirderKey(g,grpIdx);
      SegmentIndexType nSegmentsThisGroup = pBridge->GetSegmentCount( thisGirderKey );
      for ( SegmentIndexType s = 0; s < nSegmentsThisGroup; s++ )
      {
         CSegmentKey thisSegmentKey(g,gdrIdx,s);

         if ( g != 0 && s == 0)
         {
            Float64 left_brg_offset = pBridge->GetSegmentStartBearingOffset( thisSegmentKey );
            Float64 left_end_dist   = pBridge->GetSegmentStartEndDistance( thisSegmentKey );
            if ( !IsZero(left_brg_offset - left_end_dist) )
            {
               mbrID++; // +1 more to get from the CL of the intermediate pier to the left end of the segment
            }
         }

         Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(thisSegmentKey);
         Float64 end_end_dist   = pBridge->GetSegmentEndEndDistance(thisSegmentKey);

         if ( !IsZero(start_end_dist) )
            mbrID++; // start overhang

         mbrID++; // main segment

         if ( !IsZero(end_end_dist) )
            mbrID++; // end overhang

         const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSegment->GetEndSupport(&pPier,&pTS);

         if ( pPier && pPier->IsInteriorPier() )
         {
            mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
         }
         else if ( segIdx < nSegments-1 )
         {
            mbrID++; // +1 to jump over the closure joint SSMBR
         }
      } // next segment

      CSegmentKey segmentKey(thisGirderKey,nSegments-1);
      Float64 right_brg_offset = pBridge->GetSegmentEndBearingOffset( segmentKey );
      Float64 right_end_dist   = pBridge->GetSegmentEndEndDistance( segmentKey );

      if ( !IsZero(right_brg_offset - right_end_dist) )
      {
         mbrID++; // +1 more to get to the CL of the intermediate pier
      }
   } // next group

   if ( segmentKey.groupIndex != 0 )
   {
      Float64 left_brg_offset = pBridge->GetSegmentStartBearingOffset( segmentKey );
      Float64 left_end_dist   = pBridge->GetSegmentStartEndDistance( segmentKey );

      if ( !IsZero(left_brg_offset - left_end_dist) )
      {
         mbrID++; // +1 more to get from the CL of the intermediate pier to the left end of the segment
      }
   }

   // count number of SSMBRs in this group, up to but not including the segment we are interested in
   for ( SegmentIndexType s = 0; s < segmentKey.segmentIndex; s++ )
   {
      CSegmentKey thisSegmentKey(grpIdx,gdrIdx,s);
      Float64 start_end_dist = pBridge->GetSegmentStartEndDistance(thisSegmentKey);
      Float64 end_end_dist   = pBridge->GetSegmentEndEndDistance(thisSegmentKey);

      if ( !IsZero(start_end_dist) )
         mbrID++; // start overhang

      mbrID++; // main segment

      if ( !IsZero(end_end_dist) )
         mbrID++; // end overhang

      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
      const CPierData2* pPier;
      const CTemporarySupportData* pTS;
      pSegment->GetEndSupport(&pPier,&pTS);

      if ( pPier && pPier->IsInteriorPier() )
      {
         mbrID += 2; // +2 to jump over left/right side intermediate diaphragm/closure SSMBR
      }
      else if ( s < nSegments-1 )
      {
         mbrID++; // +1 to jump over the closure joint SSMBR
      }
   }

   return mbrID; // superstructure member ID for the SSMBR at the start of the segment
}

IndexType CAnalysisAgentImp::GetSuperstructureMemberCount(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridge,pBridge);

   Float64 start_offset = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_offset   = pBridge->GetSegmentEndEndDistance(segmentKey);

   IndexType nSSMbrs = 0;
   if ( !IsZero(start_offset) )
      nSSMbrs++; // start overhang

   nSSMbrs++; // main segment

   if ( !IsZero(end_offset) )
      nSSMbrs++; // end overhang

   return nSSMbrs;
}

void CAnalysisAgentImp::ConfigureLBAMPoisForReactions(const CGirderKey& girderKey,PierIndexType pierIdx,IntervalIndexType intervalIdx,pgsTypes::BridgeAnalysisType bat,bool bLiveLoadReaction)
{
   // Configures the LBAM Pois for Reactions. The IDs we want for obtaining reactions depend on
   // the segment and pier boundary conditions. This method makes it easy, consistent, and seemless
   // to set up the m_LBAMPoi array

   m_LBAMPoi->Clear();
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);

   // Interior piers are easy... 
   // (this is a pier that is interior to a group)
   // (this is a non-boundary pier)
   if ( pBridge->IsInteriorPier(pierIdx) || bLiveLoadReaction )
   {
      m_LBAMPoi->Add(pierIdx);
      return;
   }

   PierIndexType nPiers = pBridge->GetPierCount();
   bool bContinuousBack, bContinuousAhead;
   pBridge->IsContinuousAtPier(pierIdx,&bContinuousBack,&bContinuousAhead);
   bool bIntegralBack, bIntegralAhead;
   pBridge->IsIntegralAtPier(pierIdx,&bIntegralBack,&bIntegralAhead);

   EventIndexType backEventIdx, aheadEventIdx;
   pBridge->GetContinuityEventIndex(pierIdx,&backEventIdx,&aheadEventIdx);

   IntervalIndexType backContinuityIntervalIdx, aheadContinuityIntervalIdx;
   if ( bat == pgsTypes::SimpleSpan )
   {
      backContinuityIntervalIdx  = INVALID_INDEX;
      aheadContinuityIntervalIdx = INVALID_INDEX;
   }
   else
   {
      backContinuityIntervalIdx  = pIntervals->GetInterval(girderKey,backEventIdx);
      aheadContinuityIntervalIdx = pIntervals->GetInterval(girderKey,aheadEventIdx);
   }

   if ( pierIdx == 0 || pierIdx == nPiers-1 // first or last pier in the bridge
      || // -OR-
       (
        ((bContinuousBack || bIntegralBack) && (backContinuityIntervalIdx <= intervalIdx)) // continuous/integral on back side and interval is after continuity is made
        && // -AND-
        ((bContinuousAhead || bIntegralAhead) && (aheadContinuityIntervalIdx <= intervalIdx)) // continuous/integral on ahead side and interval is after continuity is made
       ) 
      )
   {
      // The support element of the pier is what we want
      m_LBAMPoi->Add(pierIdx);
   }
   else
   {
      // The temporary support objects used to maintain stability in the LBAM is what we want
      SupportIDType backID, aheadID;
      GetPierTemporarySupportIDs(pierIdx,&backID,&aheadID);

      if ( (bContinuousBack || bIntegralBack) && backContinuityIntervalIdx <= intervalIdx )
      {
         // Continuous/integral on back side AND interval is after continuity is made
         m_LBAMPoi->Add(pierIdx);
      }
      else
      {
         ATLASSERT(0 < pierIdx);
         SpanIndexType spanIdx = (SpanIndexType)(pierIdx) - 1;
         GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
         GirderIndexType gdrIdx = 0; // all girders in group have same number of segments
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx,gdrIdx);
         CSegmentKey segmentKey(grpIdx,gdrIdx,nSegments-1);
         Float64 back_brg_offset = pBridge->GetSegmentEndBearingOffset(segmentKey);
         Float64 back_end_dist   = pBridge->GetSegmentEndEndDistance(segmentKey);
         Float64 back_end_offset = back_brg_offset - back_end_dist;
         if ( IsZero(back_end_dist + back_end_offset) )
         {
            // The CL Bearing is at the CL Pier so there isn't a temporary support.
            // (See how the LBAM is built)
            m_LBAMPoi->Add(pierIdx);
         }
         else
         {
            m_LBAMPoi->Add(backID);
         }
      }

      if ( (bContinuousAhead || bIntegralAhead) && aheadContinuityIntervalIdx <= intervalIdx )
      {
         // Continuous/integral on ahead side AND interval is after continuity is made
         m_LBAMPoi->Add(pierIdx);
      }
      else
      {
         SpanIndexType spanIdx = (SpanIndexType)(pierIdx);
         GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
         GirderIndexType gdrIdx = 0; // all girders in group have same number of segments
         SegmentIndexType segIdx = 0; // first segment in girder
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
         Float64 ahead_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 ahead_end_dist   = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 ahead_end_offset = ahead_brg_offset - ahead_end_dist;
         if ( IsZero(ahead_end_dist + ahead_end_offset) )
         {
            // The CL Bearing is at the CL Pier so there isn't a temporary support.
            // (See how the LBAM is built)
            m_LBAMPoi->Add(pierIdx);
         }
         else
         {
            m_LBAMPoi->Add(aheadID);
         }
      }
   }


   // make sure we don't have two of the same IDs
   CollectionIndexType nItems;
   m_LBAMPoi->get_Count(&nItems);
   if ( nItems == 2 )
   {
      SupportIDType id1, id2;
      m_LBAMPoi->get_Item(0,&id1);
      m_LBAMPoi->get_Item(1,&id2);
      if ( id1 == id2 )
      {
         m_LBAMPoi->Remove(1);
      }
   }
}

bool CAnalysisAgentImp::IsDeckInPrecompressedTensileZone(IntervalIndexType stressingIntervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,pgsTypes::BridgeAnalysisType bat)
{
   ATLASSERT(IsDeckStressLocation(stressLocation));

   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      // if there is no deck, the deck can't be in the PTZ
      return false;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   if ( stressingIntervalIdx < compositeDeckIntervalIdx )
   {
      // if the stressing occurs before the deck becomes composite it can't be precompressed
      return false;
   }

   // Get the stress when the bridge is in service (that is when live load is applied)
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
   if ( serviceLoadIntervalIdx < stressingIntervalIdx )
   {
      // stressing occurs after the bridge is in service (weird, but it can be modeled)
      // get the service load stresses from the stressing interval
      serviceLoadIntervalIdx = stressingIntervalIdx; 
   }

   Float64 fMin, fMax;
   GetStress(pgsTypes::ServiceI,serviceLoadIntervalIdx,poi,stressLocation,false/*without prestress*/,bat,&fMin,&fMax);
   if ( fMax <= 0 )
      return false; // the location is not in tension so is not in the "tension zone"

   // The section is in tension, does the prestress cause compression?
   Float64 fPreTension  = GetStress(stressingIntervalIdx,poi,stressLocation);
   Float64 fPostTension = GetStress(stressingIntervalIdx,poi,stressLocation,ALL_DUCTS);
   Float64 fPS = fPreTension + fPostTension;

   if ( 0 <= fPS )
      return false; // prestressing does not cause compression here so it is not a "precompressed" location

   return true;
}

bool CAnalysisAgentImp::IsGirderInPrecompressedTensileZone(IntervalIndexType stressingIntervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,pgsTypes::BridgeAnalysisType bat, const GDRCONFIG* pConfig)
{
   ATLASSERT(IsGirderStressLocation(stressLocation));

   // The specified location is in a precompressed tensile zone if the following requirements are true
   // 1) The location is in tension in the Service I limit state for the final interval with the
   //    contribution of prestressing
   // 2) Prestressing causes compression at the location

   // First deal with the special cases

   // Special case... At the start/end of the first/last segment the stress due to 
   // externally applied loads is zero (moment is zero) for roller/hinge
   // boundary condition and the stress due to the prestressing is also zero (strands not developed). 
   // Consider the bottom of the girder to be in a precompressed tensile zone
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   if ( segmentKey.segmentIndex == 0 ) // start of first segment (end of last segment is below)
   {
      bool bModelStartCantilever,bModelEndCantilever;
      pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

      if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || // start of segment at release
          (poi.IsTenthPoint(POI_ERECTED_SEGMENT)  == 1 && !bModelStartCantilever) ) // CL Brg at start of erected segment
      {
         PierIndexType pierIdx = pBridge->GetGirderGroupStartPier(segmentKey.groupIndex);
         pgsTypes::PierConnectionType pierConnectionType = pBridge->GetPierConnectionType(pierIdx);
         if ( pierConnectionType == pgsTypes::Hinge || pierConnectionType == pgsTypes::Roller )
         {
            if ( stressLocation == pgsTypes::BottomGirder )
               return true;
            else
               return false;
         }
      }
   }

   // end of last segment
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   if ( segmentKey.segmentIndex == nSegments-1 )
   {
      bool bModelStartCantilever,bModelEndCantilever;
      pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

      if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11 ||
          (poi.IsTenthPoint(POI_ERECTED_SEGMENT)  == 11 && !bModelEndCantilever) )
      {
         PierIndexType pierIdx = pBridge->GetGirderGroupEndPier(segmentKey.groupIndex);
         pgsTypes::PierConnectionType pierConnectionType = pBridge->GetPierConnectionType(pierIdx);
         if ( pierConnectionType == pgsTypes::Hinge || pierConnectionType == pgsTypes::Roller )
         {
            if ( stressLocation == pgsTypes::BottomGirder )
               return true;
            else
               return false;
         }
      }
   }

   // Special case... ends of any segment for stressing at release
   // Even though there is prestress, at the end faces of the girder there isn't
   // any prestress force because it hasn't been transfered to the girder yet. The
   // prestress force transfers over the transfer length.
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   if ( stressingIntervalIdx == releaseIntervalIdx && (poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11) )
   {
      if ( stressLocation == pgsTypes::BottomGirder )
         return true;
      else
         return false;
   }

   // Special case... if there aren't any strands the notion of a precompressed tensile zone
   // gets really goofy (there is no precompression). Technically the segment is a reinforced concrete
   // beam. However, this is a precast-prestressed concrete program so we need to have something reasonable
   // for the precompressed tensile zone. If there aren't any strands (or the Pjack is zero), then
   // the bottom of the girder is in the PTZ and the top is not. This is where the PTZ is usually located
   // when there are strands
   Float64 Pjack;
   if ( pConfig )
   {
      Pjack = pConfig->PrestressConfig.Pjack[pgsTypes::Straight] + pConfig->PrestressConfig.Pjack[pgsTypes::Harped] + pConfig->PrestressConfig.Pjack[pgsTypes::Temporary];
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Pjack = pStrandGeom->GetPjack(segmentKey,true/*include temp strands*/);
   }

   if ( IsZero(Pjack) && stressingIntervalIdx == releaseIntervalIdx )
   {
      if ( stressLocation == pgsTypes::BottomGirder )
         return true;
      else
         return false;
   }

   // Special case... if the POI is located "near" interior supports with continuous boundary
   // condition tension develops in the top of the girder and prestressing may cause compression
   // in this location (most likely from harped strands). From LRFD C5.14.1.4.6, this location is not
   // in a precompressed tensile zone. Assume that "near" means that the POI is somewhere between 
   // mid-span and the closest pier.
   if ( stressLocation == pgsTypes::TopGirder )
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetSpansForSegment(segmentKey,&startSpanIdx,&endSpanIdx);

      GET_IFACE(IPointOfInterest,pIPoi);
      SpanIndexType spanIdx = pIPoi->GetSpan(poi);
      PierIndexType startPierIdx = (PierIndexType)spanIdx;
      PierIndexType endPierIdx = startPierIdx+1;

      // some segments are not supported by piers at all. segments can be supported by
      // just temporary supports. an example would be the center segment in a three-segment
      // single span bridge. This special case doesn't apply to that segment.
      bool bStartAtPier = true;  // start end of segment bears on a pier
      bool bEndAtPier   = true;  // end end of segment bears on a pier

      // one of the piers must be a boundary pier or C5.14.1.4.6 doesn't apply
      // the segment must start and end in the same span (can't straddle a pier)
      if ( startSpanIdx == endSpanIdx && (pBridge->IsBoundaryPier(startPierIdx) || pBridge->IsBoundaryPier(endPierIdx)) )
      {
         Float64 Xstart, Xend; // dist from end of segment to start/end pier
         if ( pBridge->IsBoundaryPier(startPierIdx) )
         {
            // GetPierLocation returns false if the segment does not bear on the pier
            bStartAtPier = pBridge->GetPierLocation(startPierIdx,segmentKey,&Xstart);
         }
         else
         {
            // not a boundary pier so use a really big number so
            // this end doesn't control
            Xstart = DBL_MAX;
         }

         if ( pBridge->IsBoundaryPier(endPierIdx) )
         {
            // GetPierLocation returns false if the segment does not bear on the pier
            bEndAtPier = pBridge->GetPierLocation(endPierIdx,segmentKey,&Xend);
         }
         else
         {
            // not a boundary pier so use a really big number so
            // this end doesn't control
            Xend = DBL_MAX;
         }

         if ( bStartAtPier && bEndAtPier ) // both ends must bear on a pier
         {
            // distance from POI to pier at start/end of the span
            // If they are equal distances, then the POI is at mid-span
            // and we can't make a direct determination if C5.14.1.4.6 applies.
            // If they are both equal, continue with the procedure below
            Float64 offsetStart = fabs(Xstart-poi.GetDistFromStart());
            Float64 offsetEnd   = fabs(Xend-poi.GetDistFromStart());
            if ( !IsEqual(offsetStart,offsetEnd) )
            {
               // poi is closer to one pier then the other.
               // get the boundary conditions of the nearest pier
               pgsTypes::PierConnectionType pierConnectionType;
               if ( offsetStart < offsetEnd )
               {
                  // nearest pier is at the start of the span
                  pierConnectionType = pBridge->GetPierConnectionType(startPierIdx);
               }
               else
               {
                  // nearest pier is at the end of the span
                  pierConnectionType = pBridge->GetPierConnectionType(endPierIdx);
               }

               // if hinge or roller boundary condition, C5.14.1.4.6 doesn't apply.
               if ( pierConnectionType != pgsTypes::Roller && pierConnectionType != pgsTypes::Hinge )
               {
                  // connection type is some sort of continuity/integral boundary condition
                  // The top of the girder is not in the PTZ.
                  return false;
               }
            }
         }
      }
   }

   // Now deal with the regular case

   // Get the stress when the bridge is in service (that is when live load is applied)
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
   if ( serviceLoadIntervalIdx < stressingIntervalIdx )
   {
      // stressing occurs after the bridge is in service (weird, but it can be modeled)
      // get the service load stresses from the stressing interval
      serviceLoadIntervalIdx = stressingIntervalIdx; 
   }

   Float64 fMin, fMax;
   GetStress(pgsTypes::ServiceI,serviceLoadIntervalIdx,poi,stressLocation,false/*without prestress*/,bat,&fMin,&fMax);
   if ( fMax <= 0 )
   {
      return false; // the location is not in tension so is not in the "tension zone"
   }

   // The section is in tension, does the prestress cause compression?
   Float64 fPreTension, fPostTension;
   if ( pConfig )
   {
      fPreTension = GetDesignStress(stressingIntervalIdx,poi,stressLocation,*pConfig);
      fPostTension = 0; // no post-tensioning for precast girder design
   }
   else
   {
      fPreTension  = GetStress(stressingIntervalIdx,poi,stressLocation);
      fPostTension = GetStress(stressingIntervalIdx,poi,stressLocation,ALL_DUCTS);
   }
   
   Float64 fPS = fPreTension + fPostTension;
   if ( 0 <= fPS )
   {
      return false; // prestressing does not cause compression here so it is not a "precompressed" location
   }

   return true;
}
