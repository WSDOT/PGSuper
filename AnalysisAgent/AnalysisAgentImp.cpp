///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\RatingSpecification.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\BridgeDescription.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\DebondUtil.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\StageCompare.h>
#include <PgsExt\GirderModelFactory.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\UnitServer.h>

#include <Units\SysUnitsMgr.h>
#include <Units\SysUnits.h>
#include <Lrfd\LoadModifier.h>

#include <PgsExt\StatusItem.h>
#include <IFace\StatusCenter.h>

#include <algorithm>

// NOTE: RAB 7/2/09
// for some reason this dll wont link without this line here
// even though the same thing is in AnalysisAgent.cpp
// this isn't a problem with the LBAM so why it doesn't
// work for FEM2d is beyond me
#include <initguid.h>
#include <WBFLFem2d_i.c>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DECLARE_LOGFILE;

// NOTE: If a new product load is added don't forget to change
// GetDesignStress() so that it is included in the design process

// Analysis Model Load Case ID's for Product Loads
const LoadCaseIDType g_lcidGirder              =   1;
const LoadCaseIDType g_lcidConstruction        =   2;
const LoadCaseIDType g_lcidSlab                =   3;
const LoadCaseIDType g_lcidDiaphragm           =   4;
const LoadCaseIDType g_lcidSidewalk            =   5;
const LoadCaseIDType g_lcidTrafficBarrier      =   6;
const LoadCaseIDType g_lcidOverlay             =   7;
const LoadCaseIDType g_lcidOverlayRating       =   8;
const LoadCaseIDType g_lcidMinDesignTruck      =   9;
const LoadCaseIDType g_lcidMaxDesignTruck      =  10;
const LoadCaseIDType g_lcidMinDesignTandem     =  11;
const LoadCaseIDType g_lcidMaxDesignTandem     =  12;
const LoadCaseIDType g_lcidMinLiveLoad         =  13;
const LoadCaseIDType g_lcidMaxLiveLoad         =  14;

const LoadCaseIDType g_lcidMinDeflDesignTruck  =  15;
const LoadCaseIDType g_lcidMaxDeflDesignTruck  =  16;
const LoadCaseIDType g_lcidMinDefl25DesignTruck=  17;
const LoadCaseIDType g_lcidMaxDefl25DesignTruck=  18;
const LoadCaseIDType g_lcidMinDeflLiveLoad     =  19;
const LoadCaseIDType g_lcidMaxDeflLiveLoad     =  20;

const LoadCaseIDType g_lcidStraightStrand      =  21;
const LoadCaseIDType g_lcidHarpedStrand        =  22;
const LoadCaseIDType g_lcidTemporaryStrand     =  23;
const LoadCaseIDType g_lcidShearKey            =  24;

const LoadGroupIDType g_lcidDCInc           = 100; // incremental DC loading
const LoadGroupIDType g_lcidDWInc           = 101; // incremental DW loading
const LoadGroupIDType g_lcidDC              = 102; // DC loading summed over stages
const LoadGroupIDType g_lcidDW              = 103; // DW loading summed over stages
const LoadGroupIDType g_lcidLLIM_Min        = 104;
const LoadGroupIDType g_lcidLLIM_Max        = 105;

const LoadCombinationIDType g_lcidServiceI_MzMin    = 200;
const LoadCombinationIDType g_lcidServiceI_MzMax    = 201;
const LoadCombinationIDType g_lcidServiceIA_MzMin   = 202;
const LoadCombinationIDType g_lcidServiceIA_MzMax   = 203;
const LoadCombinationIDType g_lcidServiceIII_MzMin  = 204;
const LoadCombinationIDType g_lcidServiceIII_MzMax  = 205;
const LoadCombinationIDType g_lcidStrengthI_MzMin   = 206;
const LoadCombinationIDType g_lcidStrengthI_MzMax   = 207;
const LoadCombinationIDType g_lcidStrengthII_MzMin  = 208;
const LoadCombinationIDType g_lcidStrengthII_MzMax  = 209;
const LoadCombinationIDType g_lcidFatigueI_MzMin    = 210;
const LoadCombinationIDType g_lcidFatigueI_MzMax    = 211;

const LoadCombinationIDType g_lcidServiceI_FyMin    = 300;
const LoadCombinationIDType g_lcidServiceI_FyMax    = 301;
const LoadCombinationIDType g_lcidServiceIA_FyMin   = 302;
const LoadCombinationIDType g_lcidServiceIA_FyMax   = 303;
const LoadCombinationIDType g_lcidServiceIII_FyMin  = 304;
const LoadCombinationIDType g_lcidServiceIII_FyMax  = 305;
const LoadCombinationIDType g_lcidStrengthI_FyMin   = 306;
const LoadCombinationIDType g_lcidStrengthI_FyMax   = 307;
const LoadCombinationIDType g_lcidStrengthII_FyMin  = 308;
const LoadCombinationIDType g_lcidStrengthII_FyMax  = 309;
const LoadCombinationIDType g_lcidFatigueI_FyMin    = 310;
const LoadCombinationIDType g_lcidFatigueI_FyMax    = 311;

// Stress Point ID's
const Int32 g_BottomGirder       = (Int32)pgsTypes::BottomGirder;
const Int32 g_TopGirder          = (Int32)pgsTypes::TopGirder;
const Int32 g_TopSlab            = (Int32)pgsTypes::TopSlab;

const Float64 TOL=1.0e-04;

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

static void AddLoadGroup(ILoadGroups* loadGroups, BSTR name, BSTR description)
{
   HRESULT hr;
   CComPtr<ILoadGroup> load_group;
   hr = load_group.CoCreateInstance(CLSID_LoadGroup) ;
   ATLASSERT(SUCCEEDED(hr));
   hr = load_group->put_Name(name) ;
   ATLASSERT(SUCCEEDED(hr));
   hr = load_group->put_Description(name) ;
   ATLASSERT(SUCCEEDED(hr));
   hr = loadGroups->Add(load_group) ;
   ATLASSERT(SUCCEEDED(hr));
}


#define VALIDATE(x) {if ( !Validate((x)) ) THROW_SHUTDOWN("Fatal Error in Analysis Agent",XREASON_AGENTVALIDATIONFAILURE,true);}
#define INVALIDATE(x) Invalidate((x))
#define CLEAR_ALL       0
#define DEADLOADS       1
#define LIVELOADS       2

HRESULT CAnalysisAgentImp::FinalConstruct()
{
   HRESULT hr;
   hr = m_LBAMUtility.CoCreateInstance(CLSID_LRFDFactory);
   if ( FAILED(hr) )
      return hr;

   hr = m_UnitServer.CoCreateInstance(CLSID_UnitServer);
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

CAnalysisAgentImp::ModelData::ModelData(CAnalysisAgentImp *pParent)
{
   m_pParent = pParent;
   HRESULT hr;
   hr = m_Model.CoCreateInstance(CLSID_LBAMModel);
   if ( FAILED(hr) ) 
      THROW_SHUTDOWN(_T("Can't create LBAM"),XREASON_COMCREATE_ERROR,true);

   // create minimum model enveloper and initialize it (no models just yet)
   m_MinModelEnveloper.CoCreateInstance(CLSID_LBAMModelEnveloper);
   m_MinModelEnveloper->Initialize(NULL,atForce,optMinimize);

   // create maximum model enveloper and initialize it (no models just yet)
   m_MaxModelEnveloper.CoCreateInstance(CLSID_LBAMModelEnveloper);
   m_MaxModelEnveloper->Initialize(NULL,atForce,optMaximize);

   CComPtr<ILBAMAnalysisEngine> engine;
   CreateAnalysisEngine(m_Model,SimpleSpan,&engine);

   m_MinModelEnveloper->AddEngine(engine);
   m_MaxModelEnveloper->AddEngine(engine);


   // Setup for deflection analysis

   // Response engine for the deflection-only analysis
   pDeflLoadGroupResponse.CoCreateInstance(CLSID_LoadGroupDeflectionResponse);
   pDeflLiveLoadResponse.CoCreateInstance(CLSID_LiveLoadModelResponse);

   // Use a live load factory so we don't get the default brute force (slow) live loader
   CComPtr<IEnvelopedVehicularResponseFactory> pVrFactory;
   pVrFactory.CoCreateInstance(CLSID_EnvelopedVehicularResponseFactory);

   CComQIPtr<ISupportEnvelopedVehicularResponseFactory> pSupportFactory(pDeflLiveLoadResponse);
   pSupportFactory->putref_EnvelopedVehicularRepsonseFactory(pVrFactory);

   CComQIPtr<IDependOnLBAM> pDeflDepend(pDeflLoadGroupResponse);
   pDeflDepend->putref_Model(m_Model);

   CComQIPtr<IInfluenceLineResponse>         defl_influence(pDeflLoadGroupResponse);
   CComQIPtr<ILiveLoadNegativeMomentRegion>  defl_neg_moment_region(pDeflLoadGroupResponse);
   CComQIPtr<IAnalysisPOIs>                  defl_pois(pDeflLoadGroupResponse);
   CComQIPtr<IGetDistributionFactors>        defl_dfs(pDeflLoadGroupResponse);
   CComQIPtr<IGetStressPoints>               defl_gsp(pDeflLoadGroupResponse);

   CComPtr<IVehicularAnalysisContext> defl_vehicular_analysis_ctx;
   defl_vehicular_analysis_ctx.CoCreateInstance(CLSID_VehicularAnalysisContext);
   defl_vehicular_analysis_ctx->Initialize(m_Model,defl_influence,defl_neg_moment_region,defl_pois,defl_dfs,defl_gsp);

   CComQIPtr<IDependOnVehicularAnalysisContext> defl_depend_on_vehicular_analysis_ctx(pDeflLiveLoadResponse);
   defl_depend_on_vehicular_analysis_ctx->Initialize(defl_vehicular_analysis_ctx);

   CComQIPtr<IEnvelopingStrategy> strategy(pDeflLiveLoadResponse);
   strategy->get_Strategy(&pDeflEnvelopedVehicularResponse);
   ATLASSERT(pDeflEnvelopedVehicularResponse!=0);
}

CAnalysisAgentImp::ModelData::ModelData(const ModelData& other)
{ 
   *this = other;
}

CAnalysisAgentImp::ModelData::~ModelData()
{
}

void CAnalysisAgentImp::ModelData::CreateAnalysisEngine(ILBAMModel* theModel,BridgeAnalysisType bat,ILBAMAnalysisEngine** ppEngine)
{
   ATLASSERT( bat == SimpleSpan || bat == ContinuousSpan );

   CComPtr<ILBAMAnalysisEngine> engine;
   engine.CoCreateInstance(CLSID_LBAMAnalysisEngine);

   // create the customized enveloped vehicular response object
   CComPtr<IEnvelopedVehicularResponse> envelopedVehicularResponse;
#if defined _USE_ORIGINAL_LIVELOADER
   envelopedVehicularResponse.CoCreateInstance(CLSID_BruteForceVehicularResponse);
#else
   envelopedVehicularResponse.CoCreateInstance(CLSID_BruteForceVehicularResponse2);
#endif

   CComObject<CPGSuperLoadCombinationResponse>* pLCResponse;
   HRESULT hr = CComObject<CPGSuperLoadCombinationResponse>::CreateInstance(&pLCResponse);
   CComQIPtr<ILoadCombinationResponse> load_combo_response(pLCResponse);

   // initialize the engine with default values (NULL), except for the vehicular response enveloper
   engine->InitializeEx(theModel,atForce,
                        NULL, // load group response
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

void CAnalysisAgentImp::ModelData::AddContinuousModel(ILBAMModel* pContModel)
{
   m_ContinuousModel = pContModel;

   CComPtr<ILBAMAnalysisEngine> engine;
   CreateAnalysisEngine(m_ContinuousModel,ContinuousSpan,&engine);

   m_MinModelEnveloper->AddEngine(engine);
   m_MaxModelEnveloper->AddEngine(engine);

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

   m_MinModelEnveloper->get_LiveLoadModelResponse(&pLiveLoadResponse[MinSimpleContinuousEnvelope]);
   m_MinModelEnveloper->get_LoadCombinationResponse(&pLoadComboResponse[MinSimpleContinuousEnvelope]);
   m_MinModelEnveloper->get_EnvelopedVehicularResponse(&pVehicularResponse[MinSimpleContinuousEnvelope]);

   m_MaxModelEnveloper->get_LiveLoadModelResponse(&pLiveLoadResponse[MaxSimpleContinuousEnvelope]);
   m_MaxModelEnveloper->get_LoadCombinationResponse(&pLoadComboResponse[MaxSimpleContinuousEnvelope]);
   m_MaxModelEnveloper->get_EnvelopedVehicularResponse(&pVehicularResponse[MaxSimpleContinuousEnvelope]);
}

void CAnalysisAgentImp::ModelData::operator=(const CAnalysisAgentImp::ModelData& other)
{ 
   m_pParent = other.m_pParent;

   PoiMap = other.PoiMap;

   m_Model             = other.m_Model;
   m_ContinuousModel   = other.m_ContinuousModel;
   m_MinModelEnveloper = other.m_MinModelEnveloper;
   m_MaxModelEnveloper = other.m_MaxModelEnveloper;

   pLoadGroupResponse[SimpleSpan]     = other.pLoadGroupResponse[SimpleSpan];
   pLoadGroupResponse[ContinuousSpan] = other.pLoadGroupResponse[ContinuousSpan];
   pLoadCaseResponse[SimpleSpan]      = other.pLoadCaseResponse[SimpleSpan];
   pLoadCaseResponse[ContinuousSpan]  = other.pLoadCaseResponse[ContinuousSpan];

   pLiveLoadResponse[SimpleSpan]                  = other.pLiveLoadResponse[SimpleSpan];
   pLiveLoadResponse[ContinuousSpan]              = other.pLiveLoadResponse[ContinuousSpan];
   pLiveLoadResponse[MinSimpleContinuousEnvelope] = other.pLiveLoadResponse[MinSimpleContinuousEnvelope];
   pLiveLoadResponse[MaxSimpleContinuousEnvelope] = other.pLiveLoadResponse[MaxSimpleContinuousEnvelope];

   pLoadComboResponse[SimpleSpan]                  = other.pLoadComboResponse[SimpleSpan];
   pLoadComboResponse[ContinuousSpan]              = other.pLoadComboResponse[ContinuousSpan];
   pLoadComboResponse[MinSimpleContinuousEnvelope] = other.pLoadComboResponse[MinSimpleContinuousEnvelope];
   pLoadComboResponse[MaxSimpleContinuousEnvelope] = other.pLoadComboResponse[MaxSimpleContinuousEnvelope];

   pConcurrentComboResponse[SimpleSpan]            = other.pConcurrentComboResponse[SimpleSpan];
   pConcurrentComboResponse[ContinuousSpan]        = other.pConcurrentComboResponse[ContinuousSpan];

   pVehicularResponse[SimpleSpan]                  = other.pVehicularResponse[SimpleSpan];
   pVehicularResponse[ContinuousSpan]              = other.pVehicularResponse[ContinuousSpan];
   pVehicularResponse[MinSimpleContinuousEnvelope] = other.pVehicularResponse[MinSimpleContinuousEnvelope];
   pVehicularResponse[MaxSimpleContinuousEnvelope] = other.pVehicularResponse[MaxSimpleContinuousEnvelope];

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

   pDeflLoadGroupResponse = other.pDeflLoadGroupResponse;
   pDeflLiveLoadResponse  = other.pDeflLiveLoadResponse;

   pDeflEnvelopedVehicularResponse  = other.pDeflEnvelopedVehicularResponse;
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisAgent
long CAnalysisAgentImp::GetStressPointIndex(pgsTypes::StressLocation loc)
{
   long index = -999;

   switch(loc)
   {
   case pgsTypes::BottomGirder:
      index = 0;
      break;

   case pgsTypes::TopGirder:
      index = 1;
      break;

   case pgsTypes::TopSlab:
      index = 2;
      break;
   }

   return index;
}

CComBSTR CAnalysisAgentImp::GetLoadCaseName(LoadingCombination combo)
{
   CComBSTR bstrLoadCase;
   switch(combo)
   {
      case lcDC:
         bstrLoadCase = "DC";
      break;

      case lcDW:
         bstrLoadCase = "DW";
      break;

      case lcDWRating:
         bstrLoadCase = "DW_Rating";
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
   else if (CComBSTR("DW_Rating") == name )
   {
      *pCombo = lcDWRating;
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


CComBSTR CAnalysisAgentImp::GetLoadCombinationName(pgsTypes::LimitState ls)
{
   // if new load combinations names are added also updated GetLimitStateFromLoadCombination
   CComBSTR bstrLimitState;
   switch(ls)
   {
      case pgsTypes::ServiceI:
         bstrLimitState = "SERVICE-I";
         break;

      case pgsTypes::ServiceIA:
         bstrLimitState = "SERVICE-IA";
         break;

      case pgsTypes::ServiceIII:
         bstrLimitState = "SERVICE-III";
         break;

      case pgsTypes::StrengthI:
         bstrLimitState = "STRENGTH-I";
         break;

      case pgsTypes::StrengthII:
         bstrLimitState = "STRENGTH-II";
         break;

      case pgsTypes::FatigueI:
         bstrLimitState = "FATIGUE-I";
         break;

      case pgsTypes::StrengthI_Inventory:
         bstrLimitState = "STRENGTH-I-Inventory";
         break;

      case pgsTypes::StrengthI_Operating:
         bstrLimitState = "STRENGTH-I-Operating";
         break;

      case pgsTypes::ServiceIII_Inventory:
         bstrLimitState = "SERVICE-III-Inventory";
         break;

      case pgsTypes::ServiceIII_Operating:
         bstrLimitState = "SERVICE-III-Operating";
         break;

      case pgsTypes::StrengthI_LegalRoutine:
         bstrLimitState = "STRENGTH-I-Routine";
         break;

      case pgsTypes::StrengthI_LegalSpecial:
         bstrLimitState = "STRENGTH-I-Special";
         break;

      case pgsTypes::ServiceIII_LegalRoutine:
         bstrLimitState = "SERVICE-III-Routine";
         break;

      case pgsTypes::ServiceIII_LegalSpecial:
         bstrLimitState = "SERVICE-III-Special";
         break;

      case pgsTypes::StrengthII_PermitRoutine:
         bstrLimitState = "STRENGTH-II-RoutinePermit";
         break;

      case pgsTypes::ServiceI_PermitRoutine:
         bstrLimitState = "SERVICE-I-RoutinePermit";
         break;

      case pgsTypes::StrengthII_PermitSpecial:
         bstrLimitState = "STRENGTH-II-SpecialPermit";
         break;

      case pgsTypes::ServiceI_PermitSpecial:
         bstrLimitState = "SERVICE-I-SpecialPermit";
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

CComBSTR GetLoadGroupName(ProductForceType type)
{
   CComBSTR bstrLoadGroup;
   switch(type)
   {
      case pftGirder:
         bstrLoadGroup =  "Girder";
      break;

      case pftDiaphragm:
         bstrLoadGroup = "Diaphragm";
      break;

      case pftConstruction:
         bstrLoadGroup = "Construction";
      break;

      case pftSlab:
         bstrLoadGroup = "Slab";
      break;

      case pftSlabPad:
         bstrLoadGroup = "Haunch";
      break;

      case pftSlabPanel:
         bstrLoadGroup = "Slab Panel";
      break;

      case pftOverlay:
         bstrLoadGroup = "Overlay";
      break;

      case pftOverlayRating:
         bstrLoadGroup = "Overlay Rating";
      break;

      case pftTrafficBarrier:
         bstrLoadGroup = "Traffic Barrier";
      break;

      case pftSidewalk:
         bstrLoadGroup = "Sidewalk";
      break;

      case pftUserDC:
         bstrLoadGroup = "UserDC";
      break;

      case pftUserDW:
         bstrLoadGroup = "UserDW";
      break;

      case pftUserLLIM:
         bstrLoadGroup = "UserLLIM";
      break;

      case pftShearKey:
         bstrLoadGroup = "Shear Key";
      break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLoadGroup;
}


ProductForceType GetLoadGroupTypeFromName(const CComBSTR& bstrLoadGroup)
{
   if(CComBSTR("Girder") == bstrLoadGroup)
      return pftGirder;
   else if(CComBSTR("Diaphragm") == bstrLoadGroup)
      return pftDiaphragm;
   else if(CComBSTR("Construction") == bstrLoadGroup)
      return pftConstruction;
   else if(CComBSTR("Slab") == bstrLoadGroup)
      return pftSlab;
   else if(CComBSTR("Haunch") == bstrLoadGroup)
      return pftSlabPad;
   else if(CComBSTR("Slab Panel") == bstrLoadGroup)
      return pftSlabPanel;
   else if(CComBSTR("Overlay") == bstrLoadGroup)
      return pftOverlay;
   else if(CComBSTR("Overlay Rating") == bstrLoadGroup)
      return pftOverlayRating;
   else if(CComBSTR("Traffic Barrier") == bstrLoadGroup)
      return pftTrafficBarrier;
   else if(CComBSTR("Sidewalk") == bstrLoadGroup)
      return pftSidewalk;
   else if(CComBSTR("UserDC") == bstrLoadGroup)
      return pftUserDC;
   else if(CComBSTR("UserDW") == bstrLoadGroup)
      return pftUserDW;
   else if(CComBSTR("UserLLIM") == bstrLoadGroup)
      return pftUserLLIM;
   else if(CComBSTR("Shear Key") == bstrLoadGroup)
      return pftShearKey;
   else
      ATLASSERT(false); // SHOULD NEVER GET HERE
      return pftGirder; // BIG PROBLEMS
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
   m_BridgeSiteModels.clear();
   m_CastingYardModels.clear();

   m_SidewalkTrafficBarrierLoads.clear();

   InvalidateCamberModels();

   for ( int i = 0; i < 6; i++ )
   {
      m_CreepCoefficientDetails[CREEP_MINTIME][i].clear();
      m_CreepCoefficientDetails[CREEP_MAXTIME][i].clear();
   }

   m_NextPoi = 0;

   m_OverhangLoadSet.clear();

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

void CAnalysisAgentImp::ValidateAnalysisModels(SpanIndexType span,GirderIndexType gdr)
{
   // Validating the analysis models consists of building the model
   // Analysis will occur when results are needed from the model.
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   try
   {
      // Lets do a quick and dirty check to see if the models have been built
      // for this girder.  If casting yard model is built, then all the models
      // should be built.
      cyModelData* pModelData = 0;
      pModelData = GetModelData(m_CastingYardModels,span,gdr);
      if ( pModelData == 0 )
      {
         DoCastingYardAnalysis(span,gdr,pProgress);
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
   pBSModelData = GetModelData(gdr);
}

void CAnalysisAgentImp::DoCastingYardAnalysis(SpanIndexType span,GirderIndexType gdr,IProgress* pProgress)
{
   cyModelData* pModelData = GetModelData(m_CastingYardModels,span,gdr);
   if ( pModelData == 0 )
   {
      cyModelData model_data = BuildCastingYardModels(span,gdr,pProgress);
      model_data.Stage = pgsTypes::CastingYard;
      std::pair<cyGirderModels::iterator,bool> result = m_CastingYardModels.insert( std::make_pair(HashSpanGirder(span,gdr),model_data) );
      ATLASSERT( result.second == true );
   }
}

void CAnalysisAgentImp::BuildBridgeSiteModel(GirderIndexType gdr)
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   
   std::_tostringstream os;
   os << "Building Bridge Site Analysis model for Girderline " << LABEL_GIRDER(gdr) << std::ends;

   pProgress->UpdateMessage( os.str().c_str() );

   // erase the model if it exists
   m_BridgeSiteModels.erase(gdr);

   // build the model
   ModelData model_data(this);
   CComPtr<ILBAMModel> model = model_data.m_Model;
   BuildBridgeSiteModel(gdr,false,model_data.pContraflexureResponse[SimpleSpan],model);

   // build the simple made continuous model
   CComQIPtr<ILBAMModel> continuous_model;
   continuous_model.CoCreateInstance(CLSID_LBAMModel);
   model_data.AddContinuousModel(continuous_model);

   BuildBridgeSiteModel(gdr,true,model_data.pContraflexureResponse[ContinuousSpan],continuous_model);

   m_BridgeSiteModels.insert( std::make_pair(gdr,model_data) );
}


void CAnalysisAgentImp::BuildBridgeSiteModel(GirderIndexType gdr,bool bContinuousModel,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel)
{
   try
   {
      // build model
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IBridgeMaterial,pMaterial);
      GET_IFACE(ILiveLoadDistributionFactors, pDfs);
      GET_IFACE(ISectProp2,pSectProp2);
      GET_IFACE(IPointOfInterest,pPOI);

      //
      // Set up stages
      //
      CComPtr<IStages> stages;
      pModel->get_Stages(&stages);

      // girder placement
      CComPtr<IStage> stageGirderPlacement;
      stageGirderPlacement.CoCreateInstance(CLSID_Stage);
      stageGirderPlacement->put_Name(CComBSTR("Girder Placement"));
      stageGirderPlacement->put_Description(CComBSTR("Non-composite girder"));
      stages->Add(stageGirderPlacement);

      // temporary strand removal
      CComPtr<IStage> stageTempStrandRemoval;
      stageTempStrandRemoval.CoCreateInstance(CLSID_Stage);
      stageTempStrandRemoval->put_Name(CComBSTR("Temporary Strand Removal"));
      stageTempStrandRemoval->put_Description(CComBSTR("Non-composite girder"));
      stages->Add(stageTempStrandRemoval);

      // bridge site 1
      CComPtr<IStage> stageBridgeSite1;
      stageBridgeSite1.CoCreateInstance(CLSID_Stage);
      stageBridgeSite1->put_Name(CComBSTR("Bridge Site 1"));
      stageBridgeSite1->put_Description(CComBSTR("Non-composite girder, wet slab"));
      stages->Add(stageBridgeSite1);

      // bridge site 2
      CComPtr<IStage> stageBridgeSite2;
      stageBridgeSite2.CoCreateInstance(CLSID_Stage);
      stageBridgeSite2->put_Name(CComBSTR("Bridge Site 2"));
      stageBridgeSite2->put_Description(CComBSTR("Composite girder, superimposed dead loads"));
      stages->Add(stageBridgeSite2);

      // bridge site 3
      CComPtr<IStage> stageBridgeSite3;
      stageBridgeSite3.CoCreateInstance(CLSID_Stage);
      stageBridgeSite3->put_Name(CComBSTR("Bridge Site 3"));
      stageBridgeSite3->put_Description(CComBSTR("Composite girder, live load"));
      stages->Add(stageBridgeSite3);

      //
      // Prepare load modifiers
      //
      lrfdLoadModifier load_modifier;
      GET_IFACE(ILoadModifiers,pLoadModifiers);
      load_modifier.SetDuctilityFactor( (lrfdLoadModifier::Level)pLoadModifiers->GetDuctilityLevel() ,pLoadModifiers->GetDuctilityFactor());
      load_modifier.SetImportanceFactor((lrfdLoadModifier::Level)pLoadModifiers->GetImportanceLevel(),pLoadModifiers->GetImportanceFactor());
      load_modifier.SetRedundancyFactor((lrfdLoadModifier::Level)pLoadModifiers->GetRedundancyLevel(),pLoadModifiers->GetRedundancyFactor());

      //
      // Layout basic bridge geometry
      //

      // create the first support
      CComPtr<ISupports> supports;
      pModel->get_Supports(&supports);

      CComPtr<ISupport> objSupport;
      objSupport.CoCreateInstance(CLSID_Support);

      bool bIntegralOnLeft, bIntegralOnRight;
      pBridge->IsIntegralAtPier(0,&bIntegralOnLeft,&bIntegralOnRight);

      objSupport->put_BoundaryCondition(bIntegralOnRight ? bcFixed : bcPinned);

      objSupport->SetLoadModifier(lctStrength,load_modifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),load_modifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

      supports->Add(objSupport);

      // start laying out the spans and supports along the girderline
      CComPtr<ISpans> spans;
      pModel->get_Spans(&spans);

      SpanIndexType nSpans   = pBridge->GetSpanCount();
      PierIndexType lastPier = pBridge->GetPierCount();
      Float64 start_of_bridge_station = pBridge->GetPierStation(0);

      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         // deal with girder index when there are different number of girders in each span
         GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
         GirderIndexType gdrIdx = min(gdr,nGirders-1);

         PierIndexType prevPier = PierIndexType(spanIdx);
         PierIndexType nextPier = prevPier+1;

         // check if girder is on bearing - error out if not.
         Float64 s_end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);
         Float64 e_end_size = pBridge->GetGirderEndConnectionLength(spanIdx,gdrIdx);
         if (s_end_size<0.0 || e_end_size<0.0)
         {
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            std::_tostringstream os;
            os<<"Error - The end of the girder is located off of the bearing at the ";
            if (s_end_size<0.0 && e_end_size<0.0)
            {
               os<<"left and right ends";
            }
            else if (s_end_size<0.0)
            {
               os<<"left end";
            }
            else
            {
               os<<"right end";
            }

            os<<" of Girder "<<LABEL_GIRDER(gdrIdx)<<" in Span "<< LABEL_SPAN(spanIdx) <<". \r\nThis problem can be resolved by increasing the girder End Distance in the Connection library, or by decreasing the skew angle of the girder wrt the pier.";

            pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,os.str().c_str());
            pStatusCenter->Add(pStatusItem);
      
            os<<"\r\nSee the Status Center for Details";

            THROW_UNWIND(os.str().c_str(),-1);
         }

         // girder strength
         Float64 Ec = pMaterial->GetEcGdr(spanIdx,gdrIdx);

         // span
         CComPtr<ISpan> objSpan;
         objSpan.CoCreateInstance(CLSID_Span);

         Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);// distance between points of bearing
         objSpan->put_Length(span_length); 
         objSpan->SetLoadModifier(lctStrength,load_modifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),load_modifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));
         spans->Add(objSpan);

         // support at right end of the span (left side of pier)
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(nextPier,&bContinuousOnLeft,&bContinuousOnRight);

         objSupport.Release();
         objSupport.CoCreateInstance(CLSID_Support);

         // if this is not a model that accounts for continuity or if the pier
         // is continuous left and right, use a roller (no moment into the support)
         // otherwise use a fixed support
         if ( !bContinuousModel || (bContinuousOnLeft && bContinuousOnRight) )
         {
            objSupport->put_BoundaryCondition(bcRoller);
         }
         else
         {
            objSupport->put_BoundaryCondition(bcFixed);
         }

         objSupport->SetLoadModifier(lctStrength,load_modifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Min),load_modifier.LoadModifier(lrfdTypes::StrengthI,lrfdTypes::Max));

         supports->Add(objSupport);

         // stiffness for optional deflection live load - same for all segments
#pragma Reminder("UPDATE: Assuming prismatic members")
         Float64 start_of_span_station = pBridge->GetPierStation(spanIdx);
         Float64 dist_from_start_of_bridge_to_mid_span = start_of_span_station - start_of_bridge_station + span_length/2; 

         Float64 ei_defl = pSectProp2->GetBridgeEIxx( dist_from_start_of_bridge_to_mid_span );
         ei_defl /= nGirders;

         Float64 ea_defl = ei_defl;



         // layout superstructure members - 1 per span (one for each precast segment)
         // Each superstructure member will be modeled with 1 prismatic segment
         CComPtr<ISuperstructureMembers> ssms;
         pModel->get_SuperstructureMembers(&ssms);

         std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::BridgeSite3,POI_MIDSPAN);
         ATLASSERT( vPOI.size() == 1 );
         const pgsPointOfInterest& poi = vPOI.front();


         Uint16 nSegments = 1; // assume 1 segment per span (prismatic)
                               // but set up a loop for future non-prismatic members
         for ( Uint16 segment = 0; segment < nSegments; segment++ )
         {
            Float64 segment_length = span_length;

            std::vector<SuperstructureMemberData> vData;
            SuperstructureMemberData data;

            // section properties... 
            // non-composite, wet slab
            data.stage = "Girder Placement";
            data.ea = Ec*pSectProp2->GetAg(pgsTypes::GirderPlacement,poi);
            data.ei = Ec*pSectProp2->GetIx(pgsTypes::GirderPlacement,poi);
            data.ea_defl = data.ea;
            data.ei_defl = data.ei;
            vData.push_back(data);

            data.stage = "Temporary Strand Removal";
            data.ea = Ec*pSectProp2->GetAg(pgsTypes::TemporaryStrandRemoval,poi);
            data.ei = Ec*pSectProp2->GetIx(pgsTypes::TemporaryStrandRemoval,poi);
            data.ea_defl = data.ea;
            data.ei_defl = data.ei;
            vData.push_back(data);

            data.stage = "Bridge Site 1";
            data.ea = Ec*pSectProp2->GetAg(pgsTypes::BridgeSite1,poi);
            data.ei = Ec*pSectProp2->GetIx(pgsTypes::BridgeSite1,poi);
            data.ea_defl = data.ea;
            data.ei_defl = data.ei;
            vData.push_back(data);

            // fully composite
            data.stage = "Bridge Site 2";
            data.ea = Ec*pSectProp2->GetAg(pgsTypes::BridgeSite2,poi);
            data.ei = Ec*pSectProp2->GetIx(pgsTypes::BridgeSite2,poi);
            data.ea_defl = data.ea;
            data.ei_defl = data.ei;
            vData.push_back(data);

            data.stage = "Bridge Site 3";
            data.ea = Ec*pSectProp2->GetAg(pgsTypes::BridgeSite3,poi);
            data.ei = Ec*pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
            data.ea_defl = ea_defl;
            data.ei_defl = ei_defl;
            vData.push_back(data);

            CComPtr<ISuperstructureMember> ssm;
            CreateSuperstructureMember(segment_length,vData, &ssm);

            // moment release the left end of the first segment
            if ( segment == 0 )
            {
               // if we are building a continuous model
               // then create a moment release that goes away at the continuity stage.
               // otherwise it is pinned for all stages
               pBridge->IsContinuousAtPier(prevPier,&bContinuousOnLeft,&bContinuousOnRight);
               pBridge->IsIntegralAtPier(prevPier,&bIntegralOnLeft,&bIntegralOnRight);
               if ( bContinuousModel && (bContinuousOnRight || bIntegralOnRight) )
               {
                  pgsTypes::Stage left_continunity_stage, right_continuity_stage;
                  pBridge->GetContinuityStage(prevPier, &left_continunity_stage, &right_continuity_stage);

                  CComBSTR bstrContinuityStage(right_continuity_stage == pgsTypes::BridgeSite1 ? "Bridge Site 1" : "Bridge Site 2");
                  ssm->SetEndRelease(ssLeft, bstrContinuityStage, mrtPinned);
               }
               else
               {
                  ssm->SetEndRelease(ssLeft, CComBSTR(""), mrtPinned);
               }
            }

            // moment release the right end of the last segment
            if ( segment == nSegments - 1 )
            {
               // if we are building a continuous model
               // then create a moment release that goes away at the continuity stage.
               // otherwise it is pinned for all stages
               pBridge->IsContinuousAtPier(nextPier,&bContinuousOnLeft,&bContinuousOnRight);
               pBridge->IsIntegralAtPier(nextPier,&bIntegralOnLeft,&bIntegralOnRight);
               if ( bContinuousModel && (bContinuousOnLeft || bIntegralOnLeft) )
               {
                  pgsTypes::Stage left_continunity_stage, right_continuity_stage;
                  pBridge->GetContinuityStage(nextPier, &left_continunity_stage, &right_continuity_stage);

                  CComBSTR bstrContinuityStage(left_continunity_stage == pgsTypes::BridgeSite1 ? "Bridge Site 1" : "Bridge Site 2");
                  ssm->SetEndRelease(ssRight, bstrContinuityStage, mrtPinned);
               }
               else
               {
                  ssm->SetEndRelease(ssRight, CComBSTR(""), mrtPinned);
               }
            }

            ssms->Add(ssm);
         }
      }

      // Apply live load distribution factors
      ApplyLiveLoadDistributionFactors(gdr,bContinuousModel,pContraflexureResponse, pModel);

      // Loads
      ApplySelfWeightLoad(    pModel, gdr);
      ApplyDiaphragmLoad(     pModel, gdr);
      ApplyConstructionLoad(  pModel, gdr);
      ApplySlabLoad(          pModel, gdr);
      ApplyOverlayLoad(       pModel, gdr);
      ApplyTrafficBarrierAndSidewalkLoad(pModel, gdr);
      ApplyShearKeyLoad(      pModel,gdr);
      ApplyLiveLoadModel(     pModel, gdr);
      ApplyUserDefinedLoads(  pModel, gdr);


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

CAnalysisAgentImp::cyModelData CAnalysisAgentImp::BuildCastingYardModels(SpanIndexType spanIdx,GirderIndexType gdrIdx,IProgress* pProgress)
{
   // Get the interface pointers we are going to use
   GET_IFACE(IBridge,         pBridge );
   GET_IFACE(IBridgeMaterial, pMat );
   GET_IFACE(ISectProp2,      pSectProp2);

   // Build the model
   std::_tostringstream os;
   os << "Building casting yard model for Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   // For casting yard models, use the entire girder length as the
   // span length.
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   // Get the material properties
   Float64 Eci = pMat->GetEciGdr(spanIdx,gdrIdx);

   // Get points of interest
   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPOI = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_ALL,POIFIND_OR);

   // Build the Girder Model
   cyModelData model_data;
   model_data.Stage = pgsTypes::CastingYard;
   pgsGirderModelFactory::CreateGirderModel(m_pBroker,spanIdx,gdrIdx,0.0,Lg,Eci,g_lcidGirder,false,vPOI,&model_data.Model,&model_data.PoiMap);

   return model_data;
}

Float64 CAnalysisAgentImp::GetReactions(cyGirderModels& model,PierIndexType pier,GirderIndexType gdr)
{
   SpanIndexType span = SpanIndexType((pier == 0 ? 0 : pier - 1));
   cyModelData* pModelData = GetModelData( model, span, gdr );

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   JointIDType jointID;
   if ( pier == 0 )
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

Float64 CAnalysisAgentImp::GetSectionStress(cyGirderModels& model,pgsTypes::StressLocation loc,const pgsPointOfInterest& poi)
{
   cyModelData* pModelData = GetModelData( model, poi.GetSpan(), poi.GetGirder() );
   PoiIDType poi_id = pModelData->PoiMap.GetModelPoi( poi );
   if ( poi_id == INVALID_ID )
   {
      poi_id = AddCyPointOfInterest( poi );
   }

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   Float64 fx,fy,mz;

   Fem2dMbrFaceType face = IsZero( poi.GetDistFromStart() ) ? mftRight : mftLeft;

   HRESULT hr = results->ComputePOIForces(g_lcidGirder,poi_id,face,lotGlobal,&fx,&fy,&mz);
   ATLASSERT(SUCCEEDED(hr));

   GET_IFACE(ISectProp2, pSectProp2);
   Float64 A, S;

   ATLASSERT( loc != pgsTypes::TopSlab ); // this is a casting yard model... there is no top of slab
   if ( loc == pgsTypes::TopGirder )
      S = pSectProp2->GetStGirder(pgsTypes::CastingYard,poi);
   else
      S = pSectProp2->GetSb(pgsTypes::CastingYard,poi);

   A = pSectProp2->GetAg(pgsTypes::CastingYard,poi);

   Float64 stress = fx/A + mz/S;
   stress = IsZero(stress) ? 0 : stress;

   return stress;
}

void CAnalysisAgentImp::GetSectionResults(cyGirderModels& model,const pgsPointOfInterest& poi,sysSectionValue* pFx,sysSectionValue* pFy,sysSectionValue* pMz,Float64* pDx,Float64* pDy,Float64* pRz)
{
   cyModelData* pModelData = GetModelData( model, poi.GetSpan(), poi.GetGirder() );
   ATLASSERT( pModelData != 0 );

   // Check if results have been cached for this poi.
   PoiIDType poi_id = pModelData->PoiMap.GetModelPoi( poi );

   if ( poi_id == INVALID_ID )
   {
      poi_id = AddCyPointOfInterest( poi );
   }



   CComQIPtr<IFem2dModelResults> results(pModelData->Model);

   Fem2dMbrFaceType face = IsZero( poi.GetDistFromStart() ) ? mftRight : mftLeft;

   Float64 FxLeft, FyLeft, MzLeft;
   HRESULT hr = results->ComputePOIForces(g_lcidGirder,poi_id,face,lotGlobal,&FxLeft,&FyLeft,&MzLeft);
   ATLASSERT(SUCCEEDED(hr));

   FyLeft *= -1;

   Float64 FxRight, FyRight, MzRight;
   hr = results->ComputePOIForces(g_lcidGirder,poi_id,mftRight,lotGlobal,&FxRight,&FyRight,&MzRight);
   ATLASSERT(SUCCEEDED(hr));

   pFx->Left()  = FxLeft;
   pFx->Right() = FxRight;

   pFy->Left()  = FyLeft;
   pFy->Right() = FyRight;

   pMz->Left()  = MzLeft;
   pMz->Right() = MzRight;

   hr = results->ComputePOIDisplacements(g_lcidGirder,poi_id,lotGlobal,pDx,pDy,pRz);

   ATLASSERT(SUCCEEDED(hr));
}

void CAnalysisAgentImp::ValidateCamberModels(SpanIndexType span,GirderIndexType gdr)
{
   GDRCONFIG dummy_config;

   CamberModelData camber_model_data;
   BuildCamberModel(span,gdr,false,dummy_config,&camber_model_data);
   SpanGirderHashType hash = HashSpanGirder(span,gdr);
   m_PrestressDeflectionModels.insert( std::make_pair(hash,camber_model_data) );

   CamberModelData initial_temp_beam;
   CamberModelData release_temp_beam;
   BuildTempCamberModel(span,gdr,false,dummy_config,&initial_temp_beam,&release_temp_beam);
   m_InitialTempPrestressDeflectionModels.insert( std::make_pair(hash,initial_temp_beam) );
   m_ReleaseTempPrestressDeflectionModels.insert( std::make_pair(hash,release_temp_beam) );
}

CAnalysisAgentImp::CamberModelData CAnalysisAgentImp::GetPrestressDeflectionModel(SpanIndexType span,GirderIndexType gdr,CamberModels& models)
{
   SpanGirderHashType hash = HashSpanGirder(span,gdr);
   CamberModels::iterator found;
   found = models.find( hash );

   if ( found == models.end() )
   {
      ValidateCamberModels(span,gdr);
      found = models.find( hash );
   }

   // Model should have already been created in ValidateCamberModels
   ATLASSERT( found != models.end() );

   return (*found).second;
}

void CAnalysisAgentImp::BuildCamberModel(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pModelData)
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
   std::vector<pgsPointOfInterest> vPOI; // Vector of points of interest
   Float64 hp1; // Location of left harping point
   Float64 hp2; // Location of right harping point
   Float64 Lg;  // Length of girder

   // These are the interfaces we will be using
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterialEx,pMaterial);
   GET_IFACE(ISectProp2,pSectProp2);

   Float64 E;
   if ( bUseConfig )
   {
      if ( config.bUserEci )
         E = config.Eci;
      else
         E = pMaterial->GetEconc(config.Fci,pMaterial->GetStrDensityGdr(spanIdx,gdrIdx),pMaterial->GetEccK1Gdr(spanIdx,gdrIdx),pMaterial->GetEccK2Gdr(spanIdx,gdrIdx));
   }
   else
   {
      E = pMaterial->GetEciGdr(spanIdx,gdrIdx);
   }


   //
   // Create the FEM model (includes girder dead load)
   //
   Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);
   vPOI = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_ALL,POIFIND_OR);

   GET_IFACE(IGirderLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI = pLiftPOI->GetLiftingPointsOfInterest(spanIdx,gdrIdx,POI_ALL,POIFIND_OR);

   GET_IFACE(IGirderHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI = pHaulPOI->GetHaulingPointsOfInterest(spanIdx,gdrIdx,POI_ALL,POIFIND_OR);

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());
   std::vector<pgsPointOfInterest>::iterator newEnd = std::unique(vPOI.begin(),vPOI.end());
   vPOI.erase(newEnd,vPOI.end());

   pgsGirderModelFactory::CreateGirderModel(m_pBroker,spanIdx,gdrIdx,0.0,Lg,E,g_lcidGirder,false,vPOI,&pModelData->Model,&pModelData->PoiMap);

   //
   // Apply the loads due to prestressing (use prestress force at mid-span)
   //

   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   pModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidHarpedStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);
   CollectionIndexType count;
   pointLoads->get_Count(&count);
   LoadIDType ptLoadID = (LoadIDType)count;

   vPOI = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::BridgeSite3,POI_MIDSPAN);
   ATLASSERT( vPOI.size() == 1 );
   pgsPointOfInterest mid_span_poi = vPOI.front();
   ATLASSERT( mid_span_poi.IsMidSpan(pgsTypes::BridgeSite3) == true );

   pgsPointOfInterest poiStart(spanIdx,gdrIdx,0.0);
   pgsPointOfInterest poiEnd(spanIdx,gdrIdx,Lg);


   // Start with harped strands because they are easiest (assume no debonding)
   hp1 = 0;
   hp2 = 0;
   Nl = 0;
   Nr = 0;
   Mhl = 0;
   Mhr = 0;


   StrandIndexType Nh = (bUseConfig ? config.PrestressConfig.GetNStrands(pgsTypes::Harped) : pStrandGeom->GetNumStrands(spanIdx,gdrIdx,pgsTypes::Harped));
   if ( 0 < Nh )
   {
      // Determine the prestress force
      if ( bUseConfig )
         Ph = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Harped,pgsTypes::AfterXfer);
      else
         Ph = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Harped,pgsTypes::AfterXfer);

      // get harping point locations
      vPOI.clear(); // recycle the vector
      vPOI = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::BridgeSite3,POI_HARPINGPOINT);
      ATLASSERT( 0 <= vPOI.size() && vPOI.size() < 3 );
      pgsPointOfInterest hp1_poi;
      pgsPointOfInterest hp2_poi;
      if ( vPOI.size() == 0 )
      {
         hp1 = 0;
         hp2 = 0;
      }
      else if ( vPOI.size() == 1 )
      { 
         std::vector<pgsPointOfInterest>::const_iterator iter = vPOI.begin();
         hp1_poi = *iter++;
         hp2_poi = hp1_poi;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else
      {
         std::vector<pgsPointOfInterest>::const_iterator iter = vPOI.begin();
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
         ecc_harped_start = pStrandGeom->GetHsEccentricity(poiStart, config.PrestressConfig, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetHsEccentricity(hp1_poi,  config.PrestressConfig, &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetHsEccentricity(hp2_poi,  config.PrestressConfig, &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetHsEccentricity(poiEnd,   config.PrestressConfig, &nHs_effective);
      }
      else
      {
         ecc_harped_start = pStrandGeom->GetHsEccentricity(poiStart, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetHsEccentricity(hp1_poi,  &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetHsEccentricity(hp2_poi,  &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetHsEccentricity(poiEnd,   &nHs_effective);
      }

      // Determine equivalent loads

      // moment
      Mhr = Ph*ecc_harped_start;
      Mhl = Ph*ecc_harped_end;

      // upward force
      Float64 e_prime_start, e_prime_end;
      e_prime_start = ecc_harped_hp1 - ecc_harped_start;
      e_prime_start = IsZero(e_prime_start) ? 0 : e_prime_start;
      ATLASSERT( 0 <= e_prime_start );

      e_prime_end = ecc_harped_hp2 - ecc_harped_end;
      e_prime_end = IsZero(e_prime_end) ? 0 : e_prime_end;
      ATLASSERT( 0 <= e_prime_end );

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
   pointLoads->get_Count(&count);
   ptLoadID = (LoadIDType)count;


   // start with the ends of the girder

   // the prestress force varies over the length of the girder.. use the mid-span value as an average
   Float64 nSsEffective;
   if ( bUseConfig )
   {
      ecc_straight_start = pStrandGeom->GetSsEccentricity(poiStart, config.PrestressConfig, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetSsEccentricity(poiEnd,   config.PrestressConfig, &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Straight,pgsTypes::AfterXfer);

      StrandIndexType Ns = config.PrestressConfig.GetNStrands(pgsTypes::Straight);
      if ( Ns != 0 )
         Ps *= nSsEffective/Ns;
   }
   else
   {
      StrandIndexType Ns = pStrandGeom->GetNumStrands(spanIdx,gdrIdx,pgsTypes::Straight);
      ecc_straight_start = pStrandGeom->GetSsEccentricity(poiStart, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetSsEccentricity(poiEnd,   &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Straight,pgsTypes::AfterXfer);

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

         Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,config,pgsTypes::Straight,pgsTypes::AfterXfer);
         ecc_straight_debond = pStrandGeom->GetSsEccentricity(pgsPointOfInterest(spanIdx,gdrIdx,location),config.PrestressConfig, &nSsEffective);

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

         Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,config,pgsTypes::Straight,pgsTypes::AfterXfer);
         ecc_straight_debond = pStrandGeom->GetSsEccentricity(pgsPointOfInterest(spanIdx,gdrIdx,location),config.PrestressConfig, &nSsEffective);

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
         IStrandGeometry::GirderEnd girder_end = (iside==0)? IStrandGeometry::geLeftEnd : IStrandGeometry::geRightEnd;
         Float64 sign = (iside==0)?  1 : -1;
         StrandIndexType nSections = pStrandGeom->GetNumDebondSections(spanIdx,gdrIdx,girder_end,pgsTypes::Straight);
         for ( SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
         {
            Float64 location = pStrandGeom->GetDebondSection(spanIdx,gdrIdx,girder_end,sectionIdx,pgsTypes::Straight);
            if ( location < 0 || Lg < location )
               continue; // bond occurs after the end of the girder... skip this one

            StrandIndexType nDebondedAtSection = pStrandGeom->GetNumDebondedStrandsAtSection(spanIdx,gdrIdx,girder_end,sectionIdx,pgsTypes::Straight);

            // nDebonded is to be interperted as the number of strands that become bonded at this section
            // (ok, not at this section but lt past this section)
            Float64 nSsEffective;

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,pgsTypes::AfterXfer);
            ecc_straight_debond = pStrandGeom->GetSsEccentricity(pgsPointOfInterest(spanIdx,gdrIdx,location), &nSsEffective);

            Ms = sign*Ps*ecc_straight_debond;

            pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
            ptLoad.Release();
            pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
         }
      }
   }
}

void CAnalysisAgentImp::BuildTempCamberModel(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData)
{
   Float64 Mi;   // Concentrated moments at ends of beams for eccentric prestress forces
   Float64 Mr;
   Float64 Pti;  // Prestress force in temporary strands initially
   Float64 Ptr;  // Prestress force in temporary strands when removed
   Float64 ecc; // Eccentricity of the temporary strands
   std::vector<pgsPointOfInterest> vPOI; // Vector of points of interest

   // These are the interfaces we will be using
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterialEx,pMaterial);
   GET_IFACE(ISectProp2,pSectProp2);

   // Build models
   Float64 L;
   Float64 Ec;
   Float64 Eci;
   L = pBridge->GetGirderLength(spanIdx,gdrIdx);

   if ( bUseConfig )
   {
      if ( config.bUserEci )
         Eci = config.Eci;
      else
         Eci = pMaterial->GetEconc(config.Fci,pMaterial->GetStrDensityGdr(spanIdx,gdrIdx),pMaterial->GetEccK1Gdr(spanIdx,gdrIdx),pMaterial->GetEccK2Gdr(spanIdx,gdrIdx));

      if ( config.bUserEc )
         Ec = config.Ec;
      else
         Ec = pMaterial->GetEconc(config.Fc,pMaterial->GetStrDensityGdr(spanIdx,gdrIdx),pMaterial->GetEccK1Gdr(spanIdx,gdrIdx),pMaterial->GetEccK2Gdr(spanIdx,gdrIdx));
   }
   else
   {
      Eci = pMaterial->GetEciGdr(spanIdx,gdrIdx);
      Ec  = pMaterial->GetEcGdr(spanIdx,gdrIdx);
   }

   vPOI = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_ALL,POIFIND_OR);

   GET_IFACE(IGirderLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI = pLiftPOI->GetLiftingPointsOfInterest(spanIdx,gdrIdx,POI_ALL,POIFIND_OR);

   GET_IFACE(IGirderHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI = pHaulPOI->GetHaulingPointsOfInterest(spanIdx,gdrIdx,POI_ALL,POIFIND_OR);

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());
   std::vector<pgsPointOfInterest>::iterator newEnd = std::unique(vPOI.begin(),vPOI.end());
   vPOI.erase(newEnd,vPOI.end());

   pgsGirderModelFactory::CreateGirderModel(m_pBroker,spanIdx,gdrIdx,0.0,L,Eci,g_lcidGirder,false,vPOI,&pInitialModelData->Model,&pInitialModelData->PoiMap);
   pgsGirderModelFactory::CreateGirderModel(m_pBroker,spanIdx,gdrIdx,0.0,L,Ec, g_lcidGirder,false,vPOI,&pReleaseModelData->Model,&pReleaseModelData->PoiMap);

   // Determine the prestress forces and eccentricities
   vPOI = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::BridgeSite3,POI_MIDSPAN);
   CHECK( vPOI.size() != 0 );
   const pgsPointOfInterest& mid_span_poi = vPOI.front();

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(spanIdx,gdrIdx);

   pgsTypes::TTSUsage tempStrandUsage = (bUseConfig ? config.PrestressConfig.TempStrandUsage : pgirderData->PrestressData.TempStrandUsage);
   Float64 nTsEffective;

   if ( bUseConfig )
   {
      Pti = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation);
      Ptr = pPrestressForce->GetPrestressForce(mid_span_poi,config,pgsTypes::Temporary,pgsTypes::BeforeTemporaryStrandRemoval);
      ecc = pStrandGeom->GetTempEccentricity(pgsPointOfInterest(spanIdx,gdrIdx,0.00),config.PrestressConfig, &nTsEffective);
   }
   else
   {
      Pti = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation);
      Ptr = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,pgsTypes::BeforeTemporaryStrandRemoval);
      ecc = pStrandGeom->GetTempEccentricity(pgsPointOfInterest(spanIdx,gdrIdx,0.00),&nTsEffective);
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
   CollectionIndexType count;
   pointLoads->get_Count(&count);
   LoadIDType ptLoadID = (LoadIDType)count;

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
   pointLoads->get_Count(&count);
   ptLoadID = (LoadIDType)count;

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

   pBrokerInit->RegInterface( IID_IProductLoads,        this );
   pBrokerInit->RegInterface( IID_IProductForces,       this );
   pBrokerInit->RegInterface( IID_IProductForces2,      this );
   pBrokerInit->RegInterface( IID_ICombinedForces,      this );
   pBrokerInit->RegInterface( IID_ICombinedForces2,     this );
   pBrokerInit->RegInterface( IID_ILimitStateForces,    this );
   pBrokerInit->RegInterface( IID_ILimitStateForces2,   this );
   pBrokerInit->RegInterface( IID_IPrestressStresses,   this );
   pBrokerInit->RegInterface( IID_ICamber,              this );
   pBrokerInit->RegInterface( IID_IContraflexurePoints, this );
   pBrokerInit->RegInterface( IID_IContinuity,          this );
   pBrokerInit->RegInterface( IID_IBearingDesign,       this );
   return S_OK;
};

STDMETHODIMP CAnalysisAgentImp::Init()
{
   CREATE_LOGFILE("AnalysisAgent");

   AGENT_INIT;

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

   // create an array for pois going into the lbam. create it here once so we don't need to make a new one every time we need it
   m_LBAMPoi.CoCreateInstance(CLSID_IDArray);

   // Register status callbacks that we want to use
   m_scidInformationalError = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusError,IDH_GIRDER_CONNECTION_ERROR)); // informational with help for girder end offset error
   m_scidVSRatio            = pStatusCenter->RegisterCallback(new pgsVSRatioStatusCallback(m_pBroker));
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback( new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::Init2()
{
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

void CAnalysisAgentImp::ApplySelfWeightLoad(ILBAMModel* pModel,GirderIndexType gdr)
{
   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,   pGirder);
   GET_IFACE(IBridgeMaterial, pMat );
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IStageMap,pStageMap);

   CComBSTR bstrLoadGroup = GetLoadGroupName(pftGirder);
   CComBSTR bstrStage;

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      Float64 start_offset     = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);
      Float64 end_offset       = pBridge->GetGirderEndConnectionLength(spanIdx,gdrIdx);
      Float64 girder_length    = pBridge->GetGirderLength(spanIdx,gdrIdx);
      Float64 span_length      = pBridge->GetSpanLength(spanIdx,gdrIdx);// distance between points of bearing
      Float64 gdr_height       = pGirder->GetHeight(pgsPointOfInterest(spanIdx,gdrIdx,0.00));

      // determine which stage to apply the load
      pgsTypes::Stage girderLoadStage = GetGirderDeadLoadStage(spanIdx,gdrIdx);
      bstrStage = pStageMap->GetStageName(girderLoadStage);

      std::vector<GirderLoad> vGirderLoads;
      std::vector<DiaphragmLoad> vDiaphragmLoads;
      GetGirderSelfWeightLoad(spanIdx,gdrIdx,&vGirderLoads,&vDiaphragmLoads);

      // apply distributed load segments
      Float64 Pstart(0.0), Pend(0.0); // point loads at start and end to account for the selfweight of the girder overhang
      Float64 Mstart(0.0), Mend(0.0); // 
      std::vector<GirderLoad>::iterator iter;
      for ( iter = vGirderLoads.begin(); iter != vGirderLoads.end(); iter++ )
      {
         GirderLoad& load = *iter;

         Float64 start = load.StartLoc;
         Float64 end   = load.EndLoc;

         Float64 wStart = load.wStart;
         Float64 wEnd   = load.wEnd;

         if ( start < start_offset )
         {
            // this load segment begins before the cl bearing

            // compute load intensity at the cl bearing
            Float64 w = ::LinInterp(start_offset,wStart,wEnd,end-start);

            // compute the weight of the girder from the left end to the cl brg
            Pstart = (wStart + w)*start_offset/2;

            if(start_offset>gdr_height)
            {
               Mstart = -Pstart * start_offset/2;
            }

            wStart = w;
            start  = start_offset;
         }

         if ( girder_length - end_offset < end )
         {
            // this load segment ends after the cl bearing (right end of girder)

            // compute load intensity at the cl brg
            Float64 w = ::LinInterp(girder_length-end_offset-start,wStart,wEnd,end-start);

            // compute the weight of the girder from the cl brg to the right end
            Pend = (w+wEnd)*end_offset/2;

            if(end_offset>gdr_height)
            {
               Mend = Pend * end_offset/2;
            }

            wEnd = w;
            end = girder_length - end_offset;
         }

         // create the lbam load object
         // note that the lbam span member goes between cl bearings but the load locations
         // are measured from the left end of the girder... we have to shift the load locations
         // by the start offset
         if (end>start)
         {
            CComPtr<IDistributedLoad> selfWgt;
            selfWgt.CoCreateInstance(CLSID_DistributedLoad);
            selfWgt->put_MemberType(mtSpan);
            selfWgt->put_MemberID(spanIdx);
            selfWgt->put_Direction(ldFy);
            selfWgt->put_WStart(wStart);
            selfWgt->put_WEnd(wEnd);
            selfWgt->put_StartLocation(start - start_offset);
            selfWgt->put_EndLocation(end - start_offset);

            CComPtr<IDistributedLoadItem> loadItem;
            distLoads->Add(bstrStage,bstrLoadGroup,selfWgt,&loadItem);
         }
      }

      // Apply point loads at supports at either end so reactions come out right
      AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrLoadGroup, Pstart, Pend, pointLoads);

      // apply end moments to span members if cantilever is large enough
      if (!IsZero(Mstart))
      {
         CComPtr<IPointLoad> loadMstart;
         loadMstart.CoCreateInstance(CLSID_PointLoad);
         loadMstart->put_MemberType(mtSpan);
         loadMstart->put_MemberID(spanIdx);
         loadMstart->put_Location(0.0);
         loadMstart->put_Mz(Mstart);

         CComPtr<IPointLoadItem> ptLoadItem;
         pointLoads->Add(bstrStage,bstrLoadGroup,loadMstart,&ptLoadItem);
      }

      if (!IsZero(Mend))
      {
         CComPtr<IPointLoad> loadMend;
         loadMend.CoCreateInstance(CLSID_PointLoad);
         loadMend->put_MemberType(mtSpan);
         loadMend->put_MemberID(spanIdx);
         loadMend->put_Location(-1.0);
         loadMend->put_Mz(Mend);

         CComPtr<IPointLoadItem> ptLoadItem;
         pointLoads->Add(bstrStage,bstrLoadGroup,loadMend,&ptLoadItem);
      }
   }

   // precast diaphragm loads are applied in ApplyIntermediateDiaphragmLoads
}

void CAnalysisAgentImp::ApplyDiaphragmLoad(ILBAMModel* pModel,GirderIndexType gdr)
{
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage( pStageMap->GetStageName(pgsTypes::BridgeSite1) );
   CComBSTR bstrLoadGroup( GetLoadGroupName(pftDiaphragm) );

   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      ApplyEndDiaphragmLoads( pModel, bstrStage,bstrLoadGroup, spanIdx, gdrIdx );
      ApplyIntermediateDiaphragmLoads( pModel, spanIdx, gdrIdx );
   }
}

void CAnalysisAgentImp::ApplySlabLoad(ILBAMModel* pModel,GirderIndexType gdr)
{
   GET_IFACE(IBridge,pBridge);

   // if there is no deck, get the heck outta here!
   if (pBridge->GetDeckType() == pgsTypes::sdtNone )
      return;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(pgsTypes::BridgeSite1);
   CComBSTR bstrSlabLoadGroup    = GetLoadGroupName(pftSlab);
   CComBSTR bstrSlabPadLoadGroup = GetLoadGroupName(pftSlabPad);
   CComBSTR bstrPanelLoadGroup   = GetLoadGroupName(pftSlabPanel);

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      Float64 end_size = pBridge->GetGirderStartConnectionLength( spanIdx, gdrIdx );

      // Apply the portion of the slab load that is outside the cl-bearings
      // This is load that is along the length of the girder, not transverse to the
      // girder centerline.

      Float64 P1, P2, M1, M2;
      GetCantileverSlabLoad(spanIdx, gdrIdx, &P1, &M1, &P2, &M2);

      CComPtr<IPointLoadItem> ptLoadItem;

      // apply vertical loads directly to supports so they only show up as reactions
      AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrSlabLoadGroup, P1, P2, pointLoads);

      // apply moments to span members
      if (!IsZero(M1))
      {
         CComPtr<IPointLoad> loadM1;
         loadM1.CoCreateInstance(CLSID_PointLoad);
         loadM1->put_MemberType(mtSpan);
         loadM1->put_MemberID(spanIdx);
         loadM1->put_Location(0.0);
         loadM1->put_Mz(M1);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrSlabLoadGroup,loadM1,&ptLoadItem);
      }

      if (!IsZero(M2))
      {
         CComPtr<IPointLoad> loadM2;
         loadM2.CoCreateInstance(CLSID_PointLoad);
         loadM2->put_MemberType(mtSpan);
         loadM2->put_MemberID(spanIdx);
         loadM2->put_Location(-1.0);
         loadM2->put_Mz(M2);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrSlabLoadGroup,loadM2,&ptLoadItem);
      }

      GetCantileverSlabPadLoad(spanIdx, gdrIdx, &P1, &M1, &P2, &M2);
      AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrSlabPadLoadGroup, P1, P2, pointLoads);

      // apply moments to span members
      if (!IsZero(M1))
      {
         CComPtr<IPointLoad> loadM1;
         loadM1.CoCreateInstance(CLSID_PointLoad);
         loadM1->put_MemberType(mtSpan);
         loadM1->put_MemberID(spanIdx);
         loadM1->put_Location(0.0);
         loadM1->put_Mz(M1);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrSlabPadLoadGroup,loadM1,&ptLoadItem);
      }

      if (!IsZero(M2))
      {
         CComPtr<IPointLoad> loadM2;
         loadM2.CoCreateInstance(CLSID_PointLoad);
         loadM2->put_MemberType(mtSpan);
         loadM2->put_MemberID(spanIdx);
         loadM2->put_Location(-1.0);
         loadM2->put_Mz(M2);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrSlabPadLoadGroup,loadM2,&ptLoadItem);
      }

      // main slab load
      std::vector<SlabLoad> sload;
      GetMainSpanSlabLoad(spanIdx, gdrIdx, &sload);
      CollectionIndexType nLoads = sload.size();
      for (CollectionIndexType i = 0; i < nLoads-1; i++)
      {
         SlabLoad& prevLoad = sload[i];
         SlabLoad& nextLoad = sload[i+1];
         Float64 x1 = prevLoad.Loc - end_size;
         Float64 x2 = nextLoad.Loc - end_size;

         CComPtr<IDistributedLoad> slab_load;
         slab_load.CoCreateInstance(CLSID_DistributedLoad);
         slab_load->put_MemberType(mtSpan);
         slab_load->put_MemberID(spanIdx);
         slab_load->put_Direction(ldFy);
         slab_load->put_WStart(prevLoad.MainSlabLoad);
         slab_load->put_WEnd(nextLoad.MainSlabLoad);
         slab_load->put_StartLocation(x1);
         slab_load->put_EndLocation(x2);

         CComPtr<IDistributedLoadItem> distLoadItem;
         distLoads->Add(bstrStage,bstrSlabLoadGroup,slab_load,&distLoadItem);

         CComPtr<IDistributedLoad> pad_load;
         pad_load.CoCreateInstance(CLSID_DistributedLoad);
         pad_load->put_MemberType(mtSpan);
         pad_load->put_MemberID(spanIdx);
         pad_load->put_Direction(ldFy);
         pad_load->put_WStart(prevLoad.PadLoad);
         pad_load->put_WEnd(nextLoad.PadLoad);
         pad_load->put_StartLocation(x1);
         pad_load->put_EndLocation(x2);

         distLoadItem.Release();
         distLoads->Add(bstrStage,bstrSlabPadLoadGroup,pad_load,&distLoadItem);

         if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
         {
            // Panel Loads
            Float64 w1 = prevLoad.PanelLoad;
            Float64 w2 = nextLoad.PanelLoad;

            CComPtr<IDistributedLoad> panel_load;
            panel_load.CoCreateInstance(CLSID_DistributedLoad);
            panel_load->put_MemberType(mtSpan);
            panel_load->put_MemberID(spanIdx);
            panel_load->put_Direction(ldFy);
            panel_load->put_WStart(w1);
            panel_load->put_WEnd(w2);
            panel_load->put_StartLocation(x1);
            panel_load->put_EndLocation(x2);

            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrPanelLoadGroup,panel_load,&distLoadItem);
         }
      }
   }
}

void CAnalysisAgentImp::ApplyOverlayLoad(ILBAMModel* pModel,GirderIndexType gdr)
{
// RAB 7/21/99 - This correction is a result of a phone conversation with
// Beth Shannon of Idaho DOT.
// Per 4.6.2.2.1, the overlay load is evenly distributed over all the girders
// See pg 4-23
// "Where bridges meet the conditions specified herein, permanent loads of and 
// on the deck may be distributed uniformly among the beams and/or stringers"

   GET_IFACE(IBridge,pBridge);

   // Make sure we have a roadway to work with
   PierIndexType npiers = pBridge->GetPierCount();
   for (PierIndexType pier=0; pier<npiers; pier++)
   {
      Float64 station = pBridge->GetPierStation(pier);
      Float64 dfs = pBridge->GetDistanceFromStartOfBridge(station);
      Float64 loffs = pBridge->GetLeftInteriorCurbOffset(dfs);
      Float64 roffs = pBridge->GetRightInteriorCurbOffset(dfs);

      if (loffs >= roffs)
      {
         CString strMsg(_T("The distance between interior curb lines cannot be negative. Increase the deck width or decrease sidewalk widths."));
         pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,2,strMsg);

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);

         strMsg += _T(" See Status Center for Details");
         THROW_UNWIND(strMsg,XREASON_NEGATIVE_GIRDER_LENGTH);
      }
   }


   // if there isn't an overlay, get the heck outta here
   if ( !pBridge->HasOverlay() )
      return;

   GET_IFACE( IGirder,        pGdr);

   // if this is a future overlay, add it to bridge site 3 otherwise it is in bridge site 2
   GET_IFACE(IStageMap,pStageMap);
   pgsTypes::Stage stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   CComBSTR bstrLoadGroup = GetLoadGroupName(pftOverlay); 
   CComBSTR bstrLoadGroupRating = GetLoadGroupName(pftOverlayRating); 

   bool bFutureOverlay = pBridge->IsFutureOverlay();

   SpanIndexType nSpans = pBridge->GetSpanCount();

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      Float64 end_size = pBridge->GetGirderStartConnectionLength( spanIdx, gdrIdx );

      // Add the overlay load in the main part of the span
      std::vector<OverlayLoad> sload;
      GetMainSpanOverlayLoad(spanIdx, gdrIdx, &sload);
      CollectionIndexType numsl = sload.size();
      for (CollectionIndexType i = 0; i < numsl; i++)
      {
         OverlayLoad& ol = sload[i];
         Float64 x1 = ol.StartLoc - end_size;
         Float64 x2 = ol.EndLoc - end_size;
         Float64 w1 = ol.StartLoad;
         Float64 w2 = ol.EndLoad;

         CComPtr<IDistributedLoad> load;
         load.CoCreateInstance(CLSID_DistributedLoad);
         load->put_MemberType(mtSpan);
         load->put_MemberID(spanIdx);
         load->put_Direction(ldFy);
         load->put_WStart(w1);
         load->put_WEnd(w2);
         load->put_StartLocation(x1);
         load->put_EndLocation(x2);

         CComPtr<IDistributedLoadItem> distLoadItem;
         distLoads->Add(bstrStage,bstrLoadGroup,load,&distLoadItem);

         if ( !bFutureOverlay )
         {
            distLoadItem.Release();
            distLoads->Add(bstrStage,bstrLoadGroupRating,load,&distLoadItem);
         }
      }

      // Apply cantilever moment overlay loads only if overhang exceeds girder height
      // Assume that overlay load is prismatic from end of main span out to end of cantilever

      // Left end of girder
      Float64 gdr_height       = pGdr->GetHeight(pgsPointOfInterest(spanIdx,gdrIdx,0.00));

      OverlayLoad& olleft = sload.front();
      Float64 wleft =  olleft.StartLoad;  // start of first load
      Float64 pleft = wleft * end_size;

      // Right end of girder
      Float64 rend_size = pBridge->GetGirderEndConnectionLength( spanIdx, gdrIdx );

      OverlayLoad& olright = sload.back();
      Float64 wright =  olright.EndLoad;  // end of last load
      Float64 pright = wright * rend_size;

      // Apply vertical loads directly to supports so they only show up as reactions
      AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrLoadGroup, pleft, pright, pointLoads);

      if ( !bFutureOverlay )
      {
         AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrLoadGroupRating, pleft, pright, pointLoads);
      }

      // Apply cantilever moment overlay loads only if overhang exceeds girder height
      CComPtr<IPointLoadItem> ptLoadItem;
      if (end_size>gdr_height)
      {
         Float64 mleft = -pleft * end_size/2;
         CComPtr<IPointLoad> loadMleft;
         loadMleft.CoCreateInstance(CLSID_PointLoad);
         loadMleft->put_MemberType(mtSpan);
         loadMleft->put_MemberID(spanIdx);
         loadMleft->put_Location(0.0);
         loadMleft->put_Mz(mleft);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrLoadGroup,loadMleft,&ptLoadItem);

         if ( !bFutureOverlay )
         {
            ptLoadItem.Release();
            pointLoads->Add(bstrStage,bstrLoadGroupRating,loadMleft,&ptLoadItem);
         }
      }

      if (rend_size>gdr_height)
      {
         Float64 mright = pright * rend_size/2;

         CComPtr<IPointLoad> loadMright;
         loadMright.CoCreateInstance(CLSID_PointLoad);
         loadMright->put_MemberType(mtSpan);
         loadMright->put_MemberID(spanIdx);
         loadMright->put_Location(-1.0);
         loadMright->put_Mz(mright);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrLoadGroup,loadMright,&ptLoadItem);

         if ( !bFutureOverlay )
         {
            ptLoadItem.Release();
            pointLoads->Add(bstrStage,bstrLoadGroupRating,loadMright,&ptLoadItem);
         }
      }
   }
}

void CAnalysisAgentImp::ApplyConstructionLoad(ILBAMModel* pModel,GirderIndexType gdr)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IStageMap,pStageMap);
   pgsTypes::Stage stage = pgsTypes::BridgeSite1;
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   CComBSTR bstrLoadGroup = GetLoadGroupName(pftConstruction); 

   SpanIndexType nSpans = pBridge->GetSpanCount();

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      Float64 end_size = pBridge->GetGirderStartConnectionLength( spanIdx, gdrIdx );

      // Add the overlay load in the main part of the span
      std::vector<ConstructionLoad> sload;
      GetMainConstructionLoad(spanIdx, gdrIdx, &sload);
      CollectionIndexType numsl = sload.size();
      for (CollectionIndexType i = 0; i < numsl; i++)
      {
         ConstructionLoad& ol = sload[i];
         Float64 x1 = ol.StartLoc - end_size;
         Float64 x2 = ol.EndLoc - end_size;
         Float64 w1 = ol.StartLoad;
         Float64 w2 = ol.EndLoad;

         CComPtr<IDistributedLoad> load;
         load.CoCreateInstance(CLSID_DistributedLoad);
         load->put_MemberType(mtSpan);
         load->put_MemberID(spanIdx);
         load->put_Direction(ldFy);
         load->put_WStart(w1);
         load->put_WEnd(w2);
         load->put_StartLocation(x1);
         load->put_EndLocation(x2);

         CComPtr<IDistributedLoadItem> distLoadItem;
         distLoads->Add(bstrStage,bstrLoadGroup,load,&distLoadItem);
      }
   }
}

void CAnalysisAgentImp::ApplyShearKeyLoad(ILBAMModel* pModel,GirderIndexType gdr)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStageMap,pStageMap);

   CComBSTR bstrLoadGroup = GetLoadGroupName(pftShearKey); 
   CComBSTR bstrStage = pStageMap->GetStageName(pgsTypes::BridgeSite1);

   SpanIndexType nSpans = pBridge->GetSpanCount();

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      Float64 end_size = pBridge->GetGirderStartConnectionLength( spanIdx, gdrIdx );

      // Add load in the main part of the span
      std::vector<ShearKeyLoad> load;
      this->GetMainSpanShearKeyLoad(spanIdx, gdrIdx, &load);

      for (std::vector<ShearKeyLoad>::iterator it=load.begin(); it!=load.end(); it++)
      {
         const ShearKeyLoad& ol = *it;
         Float64 x1 = ol.StartLoc - end_size;
         Float64 x2 = ol.EndLoc - end_size;
         Float64 w1 = ol.UniformLoad + ol.StartJointLoad;
         Float64 w2 = ol.UniformLoad + ol.EndJointLoad;

         CComPtr<IDistributedLoad> load;
         load.CoCreateInstance(CLSID_DistributedLoad);
         load->put_MemberType(mtSpan);
         load->put_MemberID(spanIdx);
         load->put_Direction(ldFy);
         load->put_WStart(w1);
         load->put_WEnd(w2);
         load->put_StartLocation(x1);
         load->put_EndLocation(x2);

         CComPtr<IDistributedLoadItem> distLoadItem;
         distLoads->Add(bstrStage,bstrLoadGroup,load,&distLoadItem);
      }
   }
}

void CAnalysisAgentImp::GetSidewalkLoadFraction(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64* pSidewalkLoad,
                                                Float64* pFraLeft,Float64* pFraRight)
{

   ComputeSidewalksBarriersLoadFractions();

   SpanGirderHashType key = HashSpanGirder(spanIdx,gdrIdx);
   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(key);
   if ( found != m_SidewalkTrafficBarrierLoads.end() )
   {
      const SidewalkTrafficBarrierLoad& rload = found->second;
      *pSidewalkLoad = rload.m_SidewalkLoad;
      *pFraLeft = rload.m_LeftSidewalkFraction;
      *pFraRight = rload.m_RightSidewalkFraction;
   }
   else
   {
      ATLASSERT(0); // This should never happen. Results should be available for all girders in the model
   }
}

void CAnalysisAgentImp::GetTrafficBarrierLoadFraction(SpanIndexType spanIdx,GirderIndexType gdrIdx, Float64* pBarrierLoad,
                                                      Float64* pFraExtLeft, Float64* pFraIntLeft,
                                                      Float64* pFraExtRight,Float64* pFraIntRight)
{
   ComputeSidewalksBarriersLoadFractions();

   SpanGirderHashType key = HashSpanGirder(spanIdx,gdrIdx);
   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(key);
   if ( found != m_SidewalkTrafficBarrierLoads.end() )
   {
      const SidewalkTrafficBarrierLoad& rload = found->second;
      *pBarrierLoad = rload.m_BarrierLoad;
      *pFraExtLeft = rload.m_LeftExtBarrierFraction;
      *pFraIntLeft = rload.m_LeftIntBarrierFraction;
      *pFraExtRight = rload.m_RightExtBarrierFraction;
      *pFraIntRight = rload.m_RightIntBarrierFraction;
   }
   else
   {
      ATLASSERT(0); // This should never happen. Results should be available for all girders in the model
   }
}

void CAnalysisAgentImp::ApplyTrafficBarrierAndSidewalkLoad(ILBAMModel* pModel, GirderIndexType gdr)
{
   GET_IFACE( IGirder, pGdr);
   GET_IFACE(IBridge,pBridge);

   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(pgsTypes::BridgeSite2);
   CComBSTR bstrBarrierLoadGroup = GetLoadGroupName(pftTrafficBarrier); 
   CComBSTR bstrSidewalkLoadGroup = GetLoadGroupName(pftSidewalk); 

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      PierIndexType prev_pier = spanIdx;
      PierIndexType next_pier = prev_pier + 1;

      Float64 Wtb_per_girder, fraExtLeft, fraExtRight, fraIntLeft, fraIntRight;
      GetTrafficBarrierLoadFraction(spanIdx,gdrIdx,&Wtb_per_girder,&fraExtLeft,&fraIntLeft,&fraExtRight,&fraIntRight);

      Float64 Wsw_per_girder, fraLeft, fraRight;
      GetSidewalkLoadFraction(spanIdx,gdrIdx,&Wsw_per_girder,&fraLeft,&fraRight);

      CComPtr<IDistributedLoad> tbLoad;
      tbLoad.CoCreateInstance(CLSID_DistributedLoad);
      tbLoad->put_MemberType(mtSpan);
      tbLoad->put_MemberID(spanIdx);
      tbLoad->put_Direction(ldFy);
      tbLoad->put_WStart(Wtb_per_girder);
      tbLoad->put_WEnd(Wtb_per_girder);
      tbLoad->put_StartLocation(0.0);
      tbLoad->put_EndLocation(-1.0);

      CComPtr<IDistributedLoadItem> distLoadItem;
      distLoads->Add(bstrStage,bstrBarrierLoadGroup,tbLoad,&distLoadItem);

      CComPtr<IDistributedLoad> swLoad;
      swLoad.CoCreateInstance(CLSID_DistributedLoad);
      swLoad->put_MemberType(mtSpan);
      swLoad->put_MemberID(spanIdx);
      swLoad->put_Direction(ldFy);
      swLoad->put_WStart(Wsw_per_girder);
      swLoad->put_WEnd(Wsw_per_girder);
      swLoad->put_StartLocation(0.0);
      swLoad->put_EndLocation(-1.0);

      distLoadItem.Release();
      distLoads->Add(bstrStage,bstrSidewalkLoadGroup,swLoad,&distLoadItem);

      // apply cantilever forces
      // if the overhang is <= the girder height, consider the cantilever to be a deep beam
      // in the deep beam case, just apply the reaction load and not the moment
      // otherwise apply the reaction and moment
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE,POIFIND_OR);
      ATLASSERT( 2 <= vPOI.size() );
      Float64 gdrHeightStart = pGdr->GetHeight( vPOI.front() );
      Float64 gdrHeightEnd   = pGdr->GetHeight( vPOI.back() );
      Float64 Wdia,H;
      pBridge->GetRightSideEndDiaphragmSize(prev_pier,&Wdia,&H);
      Float64 OHleft = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);
      Float64 Pleft =  Wtb_per_girder*(OHleft + Wdia);
      Float64 Mleft = -Pleft*(OHleft + Wdia)/2;

      if ( OHleft <= gdrHeightStart )
         Mleft = 0;

      CComPtr<IPointLoadItem> ptLoadItem;

      // moment load applies to span
      if (!IsZero(Mleft))
      {
         CComPtr<IPointLoad> load1m;
         load1m.CoCreateInstance(CLSID_PointLoad);
         load1m->put_MemberType(mtSpan);
         load1m->put_MemberID(spanIdx);
         load1m->put_Location(0.0);
         load1m->put_Mz(Mleft);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrBarrierLoadGroup,load1m,&ptLoadItem);
      }

      pBridge->GetLeftSideEndDiaphragmSize(next_pier,&Wdia,&H);
      Float64 OHright = pBridge->GetGirderEndConnectionLength(spanIdx,gdrIdx);
      Float64 Pright =  Wtb_per_girder*(OHright + Wdia);
      Float64 Mright =  Pright*(OHright + Wdia)/2;

      if ( OHright <= gdrHeightEnd )
         Mright = 0;

      // Apply vertical loads directly to supports so they only show up as reactions
      AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrBarrierLoadGroup, Pleft, Pright, pointLoads);

      if (!IsZero(Mright))
      {
         CComPtr<IPointLoad> load2m;
         load2m.CoCreateInstance(CLSID_PointLoad);
         load2m->put_MemberType(mtSpan);
         load2m->put_MemberID(spanIdx);
         load2m->put_Location(-1.0);
         load2m->put_Mz(Mright);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrBarrierLoadGroup,load2m,&ptLoadItem);
      }

      // sidewalks
      Pleft =  Wsw_per_girder*(OHleft + Wdia);
      Mleft = -Pleft*(OHleft + Wdia)/2;
      if ( OHleft <= gdrHeightStart )
         Mleft = 0;

      if (!IsZero(Mleft))
      {
         CComPtr<IPointLoad> load1m;
         load1m.CoCreateInstance(CLSID_PointLoad);
         load1m->put_MemberType(mtSpan);
         load1m->put_MemberID(spanIdx);
         load1m->put_Location(0.0);
         load1m->put_Mz(Mleft);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrSidewalkLoadGroup,load1m,&ptLoadItem);
      }

      Pright =  Wsw_per_girder*(OHright + Wdia);
      Mright =  Pright*(OHright + Wdia)/2;

      if ( OHright <= gdrHeightEnd )
         Mright = 0;

      // Add the point loads
      AddOverhangPointLoads(spanIdx, gdrIdx, bstrStage, bstrSidewalkLoadGroup, Pleft, Pright, pointLoads);

      if (!IsZero(Mright))
      {
         CComPtr<IPointLoad> load2m;
         load2m.CoCreateInstance(CLSID_PointLoad);
         load2m->put_MemberType(mtSpan);
         load2m->put_MemberID(spanIdx);
         load2m->put_Location(-1.0);
         load2m->put_Mz(Mright);

         ptLoadItem.Release();
         pointLoads->Add(bstrStage,bstrSidewalkLoadGroup,load2m,&ptLoadItem);
      }
   }
}

void CAnalysisAgentImp::ComputeSidewalksBarriersLoadFractions()
{
   // return if we've already done the work
   if (!m_SidewalkTrafficBarrierLoads.empty())
      return;

   GET_IFACE( IBridge,        pBridge );
   GET_IFACE( IBarriers, pBarriers);
   GET_IFACE( IGirder, pGdr);
   GET_IFACE( ISpecification, pSpec );

   // Determine weight of barriers and sidwalks
   Float64 WtbExtLeft(0.0),  WtbIntLeft(0.0),  WswLeft(0.0);
   Float64 WtbExtRight(0.0), WtbIntRight(0.0), WswRight(0.0);

   WtbExtLeft  = pBarriers->GetExteriorBarrierWeight(pgsTypes::tboLeft);
   if (pBarriers->HasInteriorBarrier(pgsTypes::tboLeft))
      WtbIntLeft  = pBarriers->GetInteriorBarrierWeight(pgsTypes::tboLeft);
   if (pBarriers->HasSidewalk(pgsTypes::tboLeft))
      WswLeft  = pBarriers->GetSidewalkWeight(pgsTypes::tboLeft);

   WtbExtRight = pBarriers->GetExteriorBarrierWeight(pgsTypes::tboRight);
   if (pBarriers->HasInteriorBarrier(pgsTypes::tboRight))
      WtbIntRight  += pBarriers->GetInteriorBarrierWeight(pgsTypes::tboRight);
   if (pBarriers->HasSidewalk(pgsTypes::tboRight))
      WswRight  = pBarriers->GetSidewalkWeight(pgsTypes::tboRight);

   GirderIndexType nMaxDist; // max number of girders/webs/mating surfaces to distribute load to
   pgsTypes::TrafficBarrierDistribution distType; // the distribution type

   pSpec->GetTrafficBarrierDistribution(&nMaxDist,&distType);


   // pgsBarrierSidewalkLoadDistributionTool does the heavy lifting to determine how 
   // sidewalks and barriers are distributed to each girder
   pgsBarrierSidewalkLoadDistributionTool BSwTool(LOGGER, pBridge, pGdr, pBarriers);

   // Loop over all girders in bridge and compute load fractions
   SpanIndexType nspans = pBridge->GetSpanCount();
   for(SpanIndexType ispan=0; ispan<nspans; ispan++)
   {
      BSwTool.Initialize(ispan, distType, nMaxDist);

      GirderIndexType ngdrs = pBridge->GetGirderCount(ispan);
      for(GirderIndexType igdr=0; igdr<ngdrs; igdr++)
      {
         SidewalkTrafficBarrierLoad stbLoad;

         // compute barrier first
         Float64 FraExtLeft, FraIntLeft, FraExtRight, FraIntRight;
         BSwTool.GetTrafficBarrierLoadFraction(igdr, &FraExtLeft, &FraIntLeft, &FraExtRight, &FraIntRight);

         stbLoad.m_BarrierLoad = -1.0 *( WtbExtLeft*FraExtLeft + WtbIntLeft*FraIntLeft + WtbExtRight*FraExtRight + WtbIntRight*FraIntRight);
         stbLoad.m_LeftExtBarrierFraction  = FraExtLeft;
         stbLoad.m_LeftIntBarrierFraction  = FraIntLeft;
         stbLoad.m_RightExtBarrierFraction = FraExtRight;
         stbLoad.m_RightIntBarrierFraction = FraIntRight;

         // sidewalks
         Float64 FraSwLeft, FraSwRight;
         BSwTool.GetSidewalkLoadFraction(igdr, &FraSwLeft, &FraSwRight);
         stbLoad.m_SidewalkLoad = -1.0 * (WswLeft*FraSwLeft + WswRight*FraSwRight);
         stbLoad.m_LeftSidewalkFraction  = FraSwLeft;
         stbLoad.m_RightSidewalkFraction = FraSwRight;

         // store for later use
         m_SidewalkTrafficBarrierLoads.insert( std::make_pair(HashSpanGirder(ispan,igdr), stbLoad) );
      }
   }
}

void CAnalysisAgentImp::ApplyLiveLoadModel(ILBAMModel* pModel,GirderIndexType gdr)
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

   // get the design, permit, and pedestrian live load models
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

   // get the design, permit, and pedestrian vehicular loads collection
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

   // get the design and permit live load names
   std::vector<std::_tstring> design_loads        = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring> permit_loads        = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);
   std::vector<std::_tstring> fatigue_loads       = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::_tstring> routine_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   std::vector<std::_tstring> special_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   std::vector<std::_tstring> routine_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   std::vector<std::_tstring> special_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

   // add the design and permit live loads to the models
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltDesign,              design_loads,          pLibrary, pLiveLoads, design_vehicles);
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltPermit,              permit_loads,          pLibrary, pLiveLoads, permit_vehicles);
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltFatigue,             fatigue_loads,         pLibrary, pLiveLoads, fatigue_vehicles);
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltLegalRating_Routine, routine_legal_loads,   pLibrary, pLiveLoads, legal_routine_vehicles);
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltLegalRating_Special, special_legal_loads,   pLibrary, pLiveLoads, legal_special_vehicles);
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltPermitRating_Routine,routine_permit_loads,  pLibrary, pLiveLoads, permit_routine_vehicles);
   AddUserLiveLoads(pModel, gdr, pgsTypes::lltPermitRating_Special,special_permit_loads,  pLibrary, pLiveLoads, permit_special_vehicles);

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

void CAnalysisAgentImp::AddUserLiveLoads(ILBAMModel* pModel,GirderIndexType gdr,pgsTypes::LiveLoadType llType,std::vector<std::_tstring>& strLLNames,
                                         ILibrary* pLibrary, ILiveLoads* pLiveLoads, IVehicularLoads* pVehicles)
{
   HRESULT hr = S_OK;

   if ( strLLNames.empty())
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

   VARIANT_BOOL bUseNegativeMomentLiveLoad = VARIANT_FALSE;
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if ( 1 < nSpans )
   {
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         if ( pBridge->ProcessNegativeMoments(spanIdx) )
         {
            bUseNegativeMomentLiveLoad = VARIANT_TRUE;
            break;
         }
      }
  }
  m_LBAMUtility->ConfigureDesignLiveLoad(pModel,llmt,IMtruck,IMlane,bUseNegativeMomentLiveLoad,bUseNegativeMomentLiveLoad,units,m_UnitServer);
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
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      Float64 span_length = pBridge->GetSpanLength(spanIdx);
      if ( ::ConvertToSysUnits(200.0,unitMeasure::Feet) < span_length )
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

void CAnalysisAgentImp::ApplyUserDefinedLoads(ILBAMModel* pModel, GirderIndexType gdr)
{
   CComPtr<IDistributedLoads> distLoads;
   pModel->get_DistributedLoads(&distLoads);

   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   GET_IFACE(IUserDefinedLoads, pUdls);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStageMap,pStageMap);

   // point loads
   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType span = 0;
   for ( span = 0; span < nSpans; span++ )
   {
      for (Uint16 istg=0; istg<3; istg++)
      {
         pgsTypes::Stage stage;
         if (istg==0)
            stage = pgsTypes::BridgeSite1;
         else if (istg==1)
            stage = pgsTypes::BridgeSite2;
         else if (istg==2)
            stage = pgsTypes::BridgeSite3;

         CComBSTR bstrStage = pStageMap->GetStageName(stage);

         const std::vector<IUserDefinedLoads::UserPointLoad>* ploads = pUdls->GetPointLoads(stage, span, gdr);

         if (ploads!=NULL)
         {
            CollectionIndexType nls = ploads->size();
            for(CollectionIndexType ild=0; ild<nls; ild++)
            {
               const IUserDefinedLoads::UserPointLoad& load = ploads->at(ild);

               CComPtr<IPointLoad> load2;
               load2.CoCreateInstance(CLSID_PointLoad);
               load2->put_MemberType(mtSpan);
               load2->put_MemberID(span);
               load2->put_Location(load.m_Location);
               load2->put_Fy(-1.0*load.m_Magnitude);
               load2->put_Mz(0.0);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&ptLoadItem);
            }
         }
      }
   }

   // distributed loads
   for ( span = 0; span < nSpans; span++ )
   {
      for (Uint16 istg=0; istg<3; istg++)
      {
         pgsTypes::Stage stage;
         if (istg==0)
            stage = pgsTypes::BridgeSite1;
         else if (istg==1)
            stage = pgsTypes::BridgeSite2;
         else if (istg==2)
            stage = pgsTypes::BridgeSite3;

         CComBSTR bstrStage = pStageMap->GetStageName(stage);

         const std::vector<IUserDefinedLoads::UserDistributedLoad>* ploads = pUdls->GetDistributedLoads(stage, span, gdr);

         if (ploads!=NULL)
         {
            CollectionIndexType nls = ploads->size();
            for(CollectionIndexType ild=0; ild<nls; ild++)
            {
               const IUserDefinedLoads::UserDistributedLoad& load = ploads->at(ild);

               CComPtr<IDistributedLoad> load2;
               load2.CoCreateInstance(CLSID_DistributedLoad);
               load2->put_MemberType(mtSpan);
               load2->put_MemberID(span);
               load2->put_StartLocation(load.m_StartLocation);
               load2->put_EndLocation(load.m_EndLocation);
               load2->put_WStart(-1.0*load.m_WStart);
               load2->put_WEnd(-1.0*load.m_WEnd);
               load2->put_Direction(ldFy);

               CComPtr<IDistributedLoadItem> distLoadItem;
               distLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&distLoadItem);
            }
         }
      }
   }

   // moment loads
   for ( span = 0; span < nSpans; span++ )
   {
      for (Uint16 stageIndex = 0; stageIndex < 3; stageIndex++)
      {
         pgsTypes::Stage stage;
         if (stageIndex == 0)
            stage = pgsTypes::BridgeSite1;
         else if (stageIndex == 1)
            stage = pgsTypes::BridgeSite2;
         else if (stageIndex == 2)
            stage = pgsTypes::BridgeSite3;

         CComBSTR bstrStage = pStageMap->GetStageName(stage);

         const std::vector<IUserDefinedLoads::UserMomentLoad>* ploads = pUdls->GetMomentLoads(stage, span, gdr);

         if (ploads != NULL)
         {
            CollectionIndexType nLoads = ploads->size();
            for(CollectionIndexType loadIdx = 0; loadIdx < nLoads; loadIdx++)
            {
               const IUserDefinedLoads::UserMomentLoad& load = ploads->at(loadIdx);

               CComPtr<IPointLoad> load2;
               load2.CoCreateInstance(CLSID_PointLoad);
               load2->put_MemberType(mtSpan);
               load2->put_MemberID(span);
               load2->put_Location(load.m_Location);
               load2->put_Fy(0.0);
               load2->put_Mz(load.m_Magnitude);

               CComPtr<IPointLoadItem> ptLoadItem;
               pointLoads->Add(bstrStage,GetLoadGroupNameForUserLoad(load.m_LoadCase),load2,&ptLoadItem);
            }
         }
      }
   }
}

Float64 CAnalysisAgentImp::GetPedestrianLiveLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   Float64 Wleft = this->GetPedestrianLoadPerSidewalk(pgsTypes::tboLeft);
   Float64 Wright = this->GetPedestrianLoadPerSidewalk(pgsTypes::tboRight);

   Float64 swLoad, fraLeft, fraRight;
   GetSidewalkLoadFraction(spanIdx,gdrIdx,&swLoad, &fraLeft,&fraRight);

   Float64 W_per_girder = fraLeft*Wleft + fraRight*Wright;
   return W_per_girder;
}

void CAnalysisAgentImp::ApplyLiveLoadDistributionFactors(GirderIndexType gdr,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel)
{
   // First, we need to get the points of contraflexure as they define the limits of application of
   // the negative moment distribution factors
   CComPtr<IDblArray> cf_locs;
   pContraflexureResponse->ComputeContraflexureLocations(CComBSTR("Bridge Site 3"),&cf_locs);

   // get the distribution factor collection from the model
   CComPtr<IDistributionFactors> span_factors;
   pModel->get_DistributionFactors(&span_factors);

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   // get the support collection before entering the loop
   CComPtr<ISupports> supports;
   pModel->get_Supports(&supports);

   // go span by span and layout distribution factors
   Float64 span_end = 0;
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      SpanType spanType = GetSpanType(spanIdx,bContinuous);
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

      ApplyLLDF_Support(spanIdx,gdrIdx,nSpans,supports);
      if ( spanIdx == nSpans-1 )
         ApplyLLDF_Support(spanIdx+1,gdrIdx,nSpans,supports); // last support

   }
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
   hr = strength1->AddLoadCaseFactor(CComBSTR("DW"),    pLoadFactors->DWmin[pgsTypes::StrengthI],   pLoadFactors->DWmax[pgsTypes::StrengthI]);
   hr = strength1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::StrengthI], pLoadFactors->LLIMmax[pgsTypes::StrengthI]);

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
   hr = strength2->AddLoadCaseFactor(CComBSTR("DW"),    pLoadFactors->DWmin[pgsTypes::StrengthII],   pLoadFactors->DWmax[pgsTypes::StrengthII]);
   hr = strength2->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::StrengthII], pLoadFactors->LLIMmax[pgsTypes::StrengthII]);

   hr = loadcombos->Add(strength2) ;

   // SERVICE-I
   CComPtr<ILoadCombination> service1;
   hr = service1.CoCreateInstance(CLSID_LoadCombination) ;
   hr = service1->put_Name( GetLoadCombinationName(pgsTypes::ServiceI) ) ;
   hr = service1->put_LoadCombinationType(lctService) ;
   hr = service1->put_LiveLoadFactor(pLoadFactors->LLIMmax[pgsTypes::ServiceI] ) ;
   hr = service1->AddLiveLoadModel(lltDesign) ;
   hr = service1->AddLiveLoadModel(lltPedestrian) ;

   if (design_ped_type != ILiveLoads::PedDontApply)
   {
      hr = service1->AddLiveLoadModel(lltPedestrian) ;
      hr = service1->put_LiveLoadModelApplicationType(design_ped_type==ILiveLoads::PedConcurrentWithVehicular ? llmaSum : llmaEnvelope);
   }

   hr = service1->AddLoadCaseFactor(CComBSTR("DC"),    pLoadFactors->DCmin[pgsTypes::ServiceI],   pLoadFactors->DCmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("DW"),    pLoadFactors->DWmin[pgsTypes::ServiceI],   pLoadFactors->DWmax[pgsTypes::ServiceI]);
   hr = service1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::ServiceI], pLoadFactors->LLIMmax[pgsTypes::ServiceI]);

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
   hr = service3->AddLoadCaseFactor(CComBSTR("DW"),    pLoadFactors->DWmin[pgsTypes::ServiceIII],   pLoadFactors->DWmax[pgsTypes::ServiceIII]);
   hr = service3->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::ServiceIII], pLoadFactors->LLIMmax[pgsTypes::ServiceIII]);

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
   hr = service1a->AddLoadCaseFactor(CComBSTR("DW"),    pLoadFactors->DWmin[pgsTypes::ServiceIA],   pLoadFactors->DWmax[pgsTypes::ServiceIA]);
   hr = service1a->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::ServiceIA], pLoadFactors->LLIMmax[pgsTypes::ServiceIA]);

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
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("DW"),    pLoadFactors->DWmin[pgsTypes::FatigueI],   pLoadFactors->DWmax[pgsTypes::FatigueI]);
   hr = fatigue1->AddLoadCaseFactor(CComBSTR("LL_IM"), pLoadFactors->LLIMmin[pgsTypes::FatigueI], pLoadFactors->LLIMmax[pgsTypes::FatigueI]);

   loadcombos->Add(fatigue1);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 DC, DW, LLIM;

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
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::StrengthII_PermitRoutine,true);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = strengthII_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   strengthII_routine->put_LiveLoadFactor(LLIM);

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
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceI_PermitRoutine,true);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceI_routine->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceI_routine->put_LiveLoadFactor(LLIM);

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
   LLIM = pRatingSpec->GetLiveLoadFactor(    pgsTypes::ServiceI_PermitSpecial,true);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("DC"), DC, DC);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("DW_Rating"), DW, DW);
   hr = serviceI_special->AddLoadCaseFactor(CComBSTR("LL_IM"), LLIM, LLIM);
   serviceI_routine->put_LiveLoadFactor(LLIM);

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

   CComPtr<ILoadCase> load_case_dw;
   load_cases->Find(CComBSTR("DW"),&load_case_dw);
   load_case_dw->AddLoadGroup(GetLoadGroupName(pftOverlay));
   load_case_dw->AddLoadGroup(GetLoadGroupName(pftUserDW));

   CComPtr<ILoadCase> load_case_dw_rating;
   load_cases->Find(CComBSTR("DW_Rating"),&load_case_dw_rating);
   GET_IFACE(IBridge,pBridge);
   if ( !pBridge->IsFutureOverlay() )
   {
      load_case_dw_rating->AddLoadGroup(GetLoadGroupName(pftOverlayRating));
   }
   load_case_dw_rating->AddLoadGroup(GetLoadGroupName(pftUserDW));

   CComPtr<ILoadCase> load_case_ll;
   load_cases->Find(CComBSTR("LL_IM"),&load_case_ll);
   load_case_ll->AddLoadGroup(GetLoadGroupName(pftUserLLIM));

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
   AddLoadGroup(loadGroups, GetLoadGroupName(pftOverlay),        CComBSTR("Overlay self weight"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftOverlayRating),  CComBSTR("Overlay self weight (rating)"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftUserDC),         CComBSTR("User applied loads in DC"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftUserDW),         CComBSTR("User applied loads in DW"));
   AddLoadGroup(loadGroups, GetLoadGroupName(pftUserLLIM),       CComBSTR("User applied live load"));
}

void CAnalysisAgentImp::ApplyEndDiaphragmLoads( ILBAMModel* pModel, CComBSTR& bstrStage,CComBSTR& bstrLoadGroup, SpanIndexType span,GirderIndexType gdr )
{
   Float64 P1, P2; // 1 = left end of the girder, 2 = right end of the girder
   Float64 M1, M2;

   //
   // Determine the loads caused by the end diaphragms
   //
   this->GetEndDiaphragmLoads(span, gdr, &P1, &M1, &P2, &M2);

   // apply the loads
   CComPtr<IPointLoads> pointLoads;
   pModel->get_PointLoads(&pointLoads);

   AddOverhangPointLoads(span, gdr, bstrStage, bstrLoadGroup, P1, P2, pointLoads);

   CComPtr<IPointLoadItem> ptLoadItem;

   if ( P1 != 0.0 )
   {
      CComPtr<IPointLoad> M;
      M.CoCreateInstance(CLSID_PointLoad);
      M->put_MemberType(mtSpan);
      M->put_MemberID(span);
      M->put_Location(0.0);
      M->put_Mz(M1);

      ptLoadItem.Release();
      pointLoads->Add(bstrStage,bstrLoadGroup,M,&ptLoadItem);
   }

   if ( P2 != 0.0 )
   {
      CComPtr<IPointLoad> M;
      M.CoCreateInstance(CLSID_PointLoad);
      M->put_MemberType(mtSpan);
      M->put_MemberID(span);
      M->put_Location(-1.0);
      M->put_Mz(M2);

      ptLoadItem.Release();
      pointLoads->Add(bstrStage,bstrLoadGroup,M,&ptLoadItem);
   }
}

void CAnalysisAgentImp::ApplyIntermediateDiaphragmLoads( ILBAMModel* pModel, SpanIndexType span,GirderIndexType gdr )
{
   // Int diaphragms can either be built into the girder at casting or added in BSS1.
   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

   Float64 span_length = pBridge->GetSpanLength( span, gdr );

   GET_IFACE(IStageMap,pStageMap);

   for ( int lc = 0; lc < 2; lc++ )
   {
      pgsTypes::Stage loadStage;
      CComBSTR bstrStage;
      CComBSTR bstrLoadGroup;

      if (lc==0)
      {
         // diaphragms constructed in the casting yard become part of the Girder loading
         loadStage = pgsTypes::CastingYard;
         bstrStage = pStageMap->GetStageName(pgsTypes::GirderPlacement);
         bstrLoadGroup =  GetLoadGroupName(pftGirder);
      }
      else
      {
         // diaphragms constructed at the bridge site become part of the Diaphragm loading
         loadStage = pgsTypes::BridgeSite1;
         bstrStage = pStageMap->GetStageName(pgsTypes::BridgeSite1);
         bstrLoadGroup = GetLoadGroupName(pftDiaphragm);
      }

      std::vector<DiaphragmLoad> loads;
      GetIntermediateDiaphragmLoads(loadStage, span, gdr, &loads);

      CComPtr<IPointLoads> pointLoads;
      pModel->get_PointLoads(&pointLoads);

      for (std::vector<DiaphragmLoad>::iterator i = loads.begin(); i!=loads.end(); i++)
      {
         DiaphragmLoad& rload = *i;

         Float64 P   = rload.Load;
         Float64 loc = rload.Loc;

         loc -= end_size;

         // if diaphragm is at the end of the girder, move it to directly over the bearing so
         // that it creates a reaction only loading
         loc = ForceIntoRange(0.0,loc,span_length);

         loc = IsZero(loc) ? 0 : loc;
         loc = IsEqual(loc,span_length) ? span_length : loc;

         CComPtr<IPointLoad> load;
         load.CoCreateInstance(CLSID_PointLoad);
         load->put_MemberType(mtSpan);
         load->put_MemberID(span);
         load->put_Location(loc);
         load->put_Fy(P);

         CComPtr<IPointLoadItem> item;
         pointLoads->Add(bstrStage , bstrLoadGroup, load, &item);
      }
   }
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


CAnalysisAgentImp::ModelData* CAnalysisAgentImp::GetModelData(GirderIndexType gdr)
{
   Models::iterator found = m_BridgeSiteModels.find(gdr);
   if ( found == m_BridgeSiteModels.end() )
   {
      BuildBridgeSiteModel(gdr);
      found = m_BridgeSiteModels.find(gdr);
      ATLASSERT(found != m_BridgeSiteModels.end());

      ModelData* pModelData = &(*found).second;

      // create the points of interest in the analysis models
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest(ALL_SPANS,gdr,pgsTypes::BridgeSite3,POI_ALLOUTPUT | POI_ALLACTIONS,POIFIND_OR);
      std::vector<pgsPointOfInterest>::iterator iter;
      for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
         if ( poi_id == INVALID_ID )
         {
            poi_id = AddPointOfInterest( pModelData, poi );
         }
      }
   }

   ModelData* pModelData = &(*found).second;

   return pModelData;
}

CAnalysisAgentImp::cyModelData* CAnalysisAgentImp::GetModelData(cyGirderModels& models,SpanIndexType span,GirderIndexType gdr)
{
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   cyGirderModels::iterator found = models.find(key);
   if ( found == models.end() )
      return 0;

   cyModelData* pModelData = &(*found).second;

   return pModelData;
}

PoiIDType CAnalysisAgentImp::AddCyPointOfInterest(const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   cyModelData* pModelData = GetModelData(m_CastingYardModels,span,gdr);

   Float64 loc = poi.GetDistFromStart();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISectProp2,pSectProp2);

   PoiIDType femID = pgsGirderModelFactory::AddPointOfInterest(pModelData->Model, poi);
   pModelData->PoiMap.AddMap( poi, femID );

   LOG("Adding POI " << femID << " at " << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << " ft");

   return femID;
}

PoiIDType CAnalysisAgentImp::AddPointOfInterest(ModelData* pModelData,const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   PoiIDType poiID = (m_NextPoi)++;

   // Get the distance from the start of the girder to the CL bearing
   // The input poi parameter is measured from the start of the girder, but
   // we need to have it measured from the CL bearing from the LBAM
   GET_IFACE(IBridge,pBridge);
   Float64 start_offset = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 span_length = pBridge->GetSpanLength(span,gdr);
   if ( ::IsGT(poi.GetDistFromStart(),start_offset,0.01) || ::IsLT((start_offset+span_length),poi.GetDistFromStart(),0.01) )
      return INVALID_ID;

   CComPtr<ISpans> spans;
   pModelData->m_Model->get_Spans(&spans);

   CComPtr<ISpan> lbamSpan;
   spans->get_Item(span,&lbamSpan);

   Float64 length;
   lbamSpan->get_Length(&length);

   // Create a LBAM POI
   CComPtr<IPOI> objPOI;
   objPOI.CoCreateInstance(CLSID_POI);
   objPOI->put_ID(poiID);
   objPOI->put_MemberType(mtSpan);
   objPOI->put_MemberID(span);
   Float64 location = poi.GetDistFromStart() - start_offset;
   if ( IsEqual(location,length,0.01) )
      location = length;

   objPOI->put_Location(location); // distance from start bearing

   // Assign stress points to the POI for each stage
   // each stage will have three stress points. Bottom of Girder, Top of Girder, Top of Slab
   // for stages where the bridge doesn't have a slab on it, the stress point coefficients will be zero
   CComPtr<IPOIStressPoints> objPOIStressPoints;
   objPOI->get_POIStressPoints(&objPOIStressPoints);

   CComPtr<IStages> stages;
   pModelData->m_Model->get_Stages(&stages);

   CComPtr<IEnumStage> enumStages;
   stages->get__EnumElements(&enumStages);

   CComPtr<IStage> stage;
   while ( enumStages->Next(1,&stage,NULL) != S_FALSE )
   {
      AddPoiStressPoints(poi,stage,objPOIStressPoints);
      stage.Release();
   }

   // Add the LBAM POI to the LBAM
   CComPtr<IPOIs> pois;
   pModelData->m_Model->get_POIs(&pois);
   pois->Add(objPOI);

   if ( pModelData->m_ContinuousModel )
   {
      pois.Release();
      pModelData->m_ContinuousModel->get_POIs(&pois);
      pois->Add(objPOI);
   }

   // Record the LBAM poi in the POI map
   pModelData->PoiMap.AddMap( poi, poiID );

   return poiID;
}


void CAnalysisAgentImp::AddPoiStressPoints(const pgsPointOfInterest& poi,IStage* pStage,IPOIStressPoints* pPOIStressPoints)
{
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IStageMap,pStageMap);

   CComBSTR bstrStage;
   pStage->get_Name(&bstrStage);
   pgsTypes::Stage stage = pStageMap->GetStageType(bstrStage);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CComPtr<IStressPoints> leftStressPoints;
   leftStressPoints.CoCreateInstance(CLSID_StressPoints);

   CComPtr<IStressPoints> rightStressPoints;
   rightStressPoints.CoCreateInstance(CLSID_StressPoints);

   // Bottom of Girder
   Float64 Sa, Sm;
   Sa = 1/pSectProp2->GetAg(stage,poi);
   Sm = 1/pSectProp2->GetSb(stage,poi);

   CComPtr<IStressPoint> spBottomGirder;
   spBottomGirder.CoCreateInstance(CLSID_StressPoint);
   spBottomGirder->put_Sa(Sa);
   spBottomGirder->put_Sm(Sm);

   leftStressPoints->Add(spBottomGirder);
   rightStressPoints->Add(spBottomGirder);

   // Top of Girder
   Sa = 1/pSectProp2->GetAg(stage,poi);
   Sm = 1/pSectProp2->GetStGirder(stage,poi);

   CComPtr<IStressPoint> spTopGirder;
   spTopGirder.CoCreateInstance(CLSID_StressPoint);
   spTopGirder->put_Sa(Sa);
   spTopGirder->put_Sm(Sm);

   leftStressPoints->Add(spTopGirder);
   rightStressPoints->Add(spTopGirder);

   // Top of Slab
   if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
   {
      Sa = 1/pSectProp2->GetAg(stage,poi);
      Sm = 1/pSectProp2->GetSt(stage,poi);
   }
   else
   {
      Sa = 0;
      Sm = 0;
   }

   CComPtr<IStressPoint> spTopSlab;
   spTopSlab.CoCreateInstance(CLSID_StressPoint);
   spTopSlab->put_Sa(Sa);
   spTopSlab->put_Sm(Sm);

   leftStressPoints->Add(spTopSlab);
   rightStressPoints->Add(spTopSlab);


   pPOIStressPoints->Insert(bstrStage,leftStressPoints,rightStressPoints);
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces
//
pgsTypes::Stage CAnalysisAgentImp::GetGirderDeadLoadStage(SpanIndexType span,GirderIndexType gdr)
{
   return GetGirderDeadLoadStage(gdr);
}

pgsTypes::Stage CAnalysisAgentImp::GetGirderDeadLoadStage(GirderIndexType gdrLineIdx)
{
   return pgsTypes::GirderPlacement;
}

sysSectionValue CAnalysisAgentImp::GetShear(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results = GetShear(stage,type,vPoi,bat);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetMoment(stage,type,vPoi,bat);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDisplacement(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetDisplacement(stage,type,vPoi,bat);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetRotation(stage,type,vPoi,bat);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetReaction(pgsTypes::Stage stage,ProductForceType type,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat)
{
   USES_CONVERSION;
   
   std::_tostringstream os;

   GET_IFACE(IStageMap,pStageMap);

   ATLASSERT(stage != pgsTypes::CastingYard);

   try
   {
      // Start by checking if the model exists
      ModelData* pModelData = 0;
      pModelData = GetModelData(gdr);

      CComBSTR bstrLoadGroup = GetLoadGroupName(type);
      CComBSTR bstrStage     = pStageMap->GetStageName(stage);
      
      m_LBAMPoi->Clear();
      m_LBAMPoi->Add(pier);

      CComPtr<IResult3Ds> results;

      if ( bat == MinSimpleContinuousEnvelope )
         pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&results);
      else if ( bat == MaxSimpleContinuousEnvelope )
         pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&results);
      else
         pModelData->pLoadGroupResponse[bat]->ComputeReactions(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&results);

      CComPtr<IResult3D> result;
      results->get_Item(0,&result);

      Float64 Fy;
      result->get_Y(&Fy);

      return Fy;
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetStress(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop,fBot;
   GetStress(stage,type,vPoi,bat,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

void CAnalysisAgentImp::GetGirderSelfWeightLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad)
{
   // get all the cross section changes
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> xsPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE,POIFIND_OR);

   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 density = pMaterial->GetWgtDensityGdr(spanIdx,gdrIdx);
   Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();

   // compute distributed load intensity at each section change
   GET_IFACE(ISectProp2,pSectProp2);
   std::vector<pgsPointOfInterest>::iterator iter( xsPOI.begin() );
   std::vector<pgsPointOfInterest>::iterator end( xsPOI.end() );
   pgsPointOfInterest prevPoi = *iter++;
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest currPoi = *iter;

      pgsPointOfInterest poi(currPoi.GetSpan(),currPoi.GetGirder(),(currPoi.GetDistFromStart() + prevPoi.GetDistFromStart())/2);

      Float64 Ag = pSectProp2->GetAg(pgsTypes::CastingYard,poi);

      GirderLoad load;
      load.StartLoc = prevPoi.GetDistFromStart();
      load.EndLoc   = currPoi.GetDistFromStart();
      load.wStart   = -Ag*density*g;
      load.wEnd     = -Ag*density*g;

      pDistLoad->push_back(load);

      prevPoi = currPoi;
   }

   // get point loads for precast diaphragms
   GetIntermediateDiaphragmLoads(pgsTypes::CastingYard,spanIdx,gdrIdx,pPointLoad);
}

Float64 CAnalysisAgentImp::GetTrafficBarrierLoad(SpanIndexType span,GirderIndexType girder)
{
   ValidateAnalysisModels(span,girder);

   SpanGirderHashType hash = HashSpanGirder(span,girder);
   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(hash);
   CHECK( found != m_SidewalkTrafficBarrierLoads.end() ); // it should be found

   return (*found).second.m_BarrierLoad;
}

Float64 CAnalysisAgentImp::GetSidewalkLoad(SpanIndexType span,GirderIndexType girder)
{
   ValidateAnalysisModels(span,girder);

   SpanGirderHashType hash = HashSpanGirder(span,girder);
   SidewalkTrafficBarrierLoadIterator found = m_SidewalkTrafficBarrierLoads.find(hash);
   CHECK( found != m_SidewalkTrafficBarrierLoads.end() ); // it should be found

   return (*found).second.m_SidewalkLoad;
}

void CAnalysisAgentImp::GetOverlayLoad(SpanIndexType span,GirderIndexType girder,std::vector<OverlayLoad>* pOverlayLoads)
{
   ValidateAnalysisModels(span,girder);
   GetMainSpanOverlayLoad(span,girder,pOverlayLoads);
}

void CAnalysisAgentImp::GetConstructionLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<ConstructionLoad>* pConstructionLoads)
{
   ValidateAnalysisModels(spanIdx,gdrIdx);
   GetMainConstructionLoad(spanIdx,gdrIdx,pConstructionLoads);
}

bool CAnalysisAgentImp::HasShearKeyLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IGirder,pGirder);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   // First check if this beam has a shear key
   if ( pGirder->HasShearKey(spanIdx, gdrIdx, spacingType))
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
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for (SpanIndexType is=0; is<nSpans; is++)
   {
      if (is != spanIdx) // already checked this above
      {
         // if there are fewer girders in this span than in spanIdx,
         // adjust the girder index based on number of girders in span is.
         GirderIndexType nGirdersInSpan = pBridge->GetGirderCount(is);
         GirderIndexType ig = min(nGirdersInSpan-1,gdrIdx);

         if (pGirder->HasShearKey(is, ig, spacingType))
            return true;
      }
   }

   return false;
}

void CAnalysisAgentImp::GetShearKeyLoad(SpanIndexType span,GirderIndexType girder,std::vector<ShearKeyLoad>* pLoads)
{
   ValidateAnalysisModels(span,girder);
   GetMainSpanShearKeyLoad(span,girder,pLoads);
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

bool CAnalysisAgentImp::HasSidewalkLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   bool bHasSidewalkLoad = false;

   Float64 swLoad, fraLeft, fraRight;
   GetSidewalkLoadFraction(spanIdx,gdrIdx,&swLoad,&fraLeft,&fraRight);

   if ( !IsZero(swLoad))
      bHasSidewalkLoad = true;

   return bHasSidewalkLoad;
}

bool CAnalysisAgentImp::HasPedestrianLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   //// pedestrian load is just like live load, it is distributed using LLDF and added to the LL+IM
   //// for design purposes, consider it applied to all girders
   //return HasPedestrianLoad();
//
// NOTE: This code below is for distributing the pedestrian load the same way the sidewalk
//       and traffic barrier dead loads are distributed
//
   bool bHasPedLoad = HasPedestrianLoad();
   if ( !bHasPedLoad )
      return false; // there is no chance of having any Ped load on this bridge

   if (spanIdx==ALL_SPANS || gdrIdx==ALL_GIRDERS)
   {
      // any pedestrian load is good enough for this case
      bHasPedLoad = true;
   }
   else
   {
      Float64 swLoad, fraLeft, fraRight;
      GetSidewalkLoadFraction(spanIdx,gdrIdx,&swLoad, &fraLeft,&fraRight);

      if ( IsZero(fraLeft) && IsZero(fraRight) )
         bHasPedLoad = false;
   }

   return bHasPedLoad;
}

Float64 CAnalysisAgentImp::GetPedestrianLoad(SpanIndexType span,GirderIndexType girder)
{
   return GetPedestrianLiveLoad(span,girder);
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


void CAnalysisAgentImp::GetMainSpanSlabLoad(SpanIndexType span,GirderIndexType gdr, std::vector<SlabLoad>* pSlabLoads)
{
   ATLASSERT(pSlabLoads!=0);
   pSlabLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(ISectProp2,pSectProp2);

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   // get slab fillet (there should be a better way to do this)
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   // if there is no deck, there is no load
   if ( pDeck->DeckType == pgsTypes::sdtNone )
      return;

   Float64 fillet = pDeck->Fillet;

   Float64 panel_support_width = 0;
   if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
      panel_support_width = pDeck->PanelSupport;

   Float64 gdr_length = pBridge->GetGirderLength(span,gdrIdx);

   // Get some important POIs that we will be using later
   PoiAttributeType attrib = POI_ALLACTIONS;
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest(span,gdrIdx,pgsTypes::BridgeSite3,attrib,POIFIND_OR);
   CHECK(vPoi.size()!=0);

   bool bIsInteriorGirder = pBridge->IsInteriorGirder(span,gdrIdx);

   // Slab pad load assumes no camber... that is, the top of the girder is flat
   // This is the worst case loading
   // Increased/Reduced pad depth due to Sag/Crest vertical curves is accounted for
   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType poiIdx = 0; poiIdx < nPOI; poiIdx++ )
   {
      Float64 wslab;
      Float64 wslab_panel;

      const pgsPointOfInterest& poi = vPoi[poiIdx];

      Float64 top_girder_to_top_slab = pSectProp2->GetDistTopSlabToTopGirder(poi);
      Float64 slab_offset = pBridge->GetSlabOffset(poi);
      Float64 cast_depth  = pBridge->GetCastSlabDepth(poi);
      Float64 panel_depth = pBridge->GetPanelDepth(poi);
      Float64 trib_slab_width = pSectProp2->GetTributaryFlangeWidth(poi);

      if ( nGirders == 1 )
      {
         // single girder bridge - use the whole deck
         Float64 station,offset;
         pBridge->GetStationAndOffset(poi,&station,&offset);

         CComPtr<IShape> slab_shape;
         pSectProp2->GetSlabShape(station,&slab_shape);

         CComPtr<IShapeProperties> sectProps;
         slab_shape->get_ShapeProperties(&sectProps);

         // this deck area includes the slab panels and slab haunch
         Float64 deck_area;
         sectProps->get_Area(&deck_area);

         Float64 panel_depth = pBridge->GetPanelDepth(poi);
         Float64 panel_width = 0;
         MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(span,gdrIdx);
         for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMatingSurfaces-1; msIdx++ )
         {
            // compute width of deck panel as distance between mating surface support locations
            Float64 x1 = pGdr->GetMatingSurfaceLocation(poi,msIdx);
            Float64 x2 = pGdr->GetMatingSurfaceLocation(poi,msIdx+1);
            Float64 width = x2 - x1;

            // adjust the panel width half the mating surface width on each end
            // this puts the edges of the panel at the edges of the mating surface
            Float64 w1 = pGdr->GetMatingSurfaceWidth(poi,msIdx);
            Float64 w2 = pGdr->GetMatingSurfaceWidth(poi,msIdx+1);

            width -= w1/2 + w2/2;
            
            // added the panel support widthto the width of the panel
            width += 2*panel_support_width;

            panel_width += width;
         }

         Float64 panel_area = panel_depth * panel_width;

         deck_area -= panel_area;

         wslab       =  deck_area * pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
         wslab_panel = panel_area * pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
      }
      else
      {
         // multi-girder bridge
         if ( bIsInteriorGirder )
         {
            // Apply the load of the main slab
            wslab       = trib_slab_width * cast_depth  * pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();

            // compute the width of the deck panels
            Float64 panel_width = trib_slab_width; // start with tributary width

            // deduct width of mating surfaces
            MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(span,gdrIdx);
            for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMatingSurfaces; msIdx++ )
            {
               panel_width -= pGdr->GetMatingSurfaceWidth(poi,msIdx);
            }

            // add panel support widths
            panel_width += 2*nMatingSurfaces*panel_support_width;
            wslab_panel = panel_width * panel_depth * pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
         }
         else
         {
            // Exterior girder... the slab overhang can be thickened so we have figure out the weight
            // on the left and right of the girder instead of using the tributary width and slab depth

            // determine depth of the slab at the edge and flange tip
            Float64 overhang_edge_depth = pDeck->OverhangEdgeDepth;
            Float64 overhang_depth_at_flange_tip;
            if ( pDeck->OverhangTaper == pgsTypes::None )
            {
               // overhang is constant depth
               overhang_depth_at_flange_tip = overhang_edge_depth;
            }
            else if ( pDeck->OverhangTaper == pgsTypes::TopTopFlange )
            {
               // deck overhang tapers to the top of the top flange
               overhang_depth_at_flange_tip = slab_offset;
            }
            else if ( pDeck->OverhangTaper == pgsTypes::BottomTopFlange )
            {
               // deck overhang tapers to the bottom of the top flange
               FlangeIndexType nFlanges = pGdr->GetNumberOfTopFlanges(span,gdrIdx);
               Float64 flange_thickness;
               if ( gdr == 0 )
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
            wslab       = (slab_area + slab_overhang_area) * pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();

            Float64 panel_width = w;

            // deduct width of mating surfaces
            MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(span,gdrIdx);
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

            wslab_panel = panel_width * panel_depth * pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
         }
      }

      ASSERT( 0 <= wslab );
      ASSERT( 0 <= wslab_panel );

      // slab pad load
      Float64 pad_hgt = top_girder_to_top_slab - cast_depth;
      if ( pad_hgt < 0 )
         pad_hgt = 0;

      // mating surface
      Float64 mating_surface_width = 0;
      MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(span,gdrIdx);
      ATLASSERT( nMatingSurfaces == pGdr->GetNumberOfMatingSurfaces(span,gdrIdx) );
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
      Float64 wpad = (pad_hgt*mating_surface_width + (pad_hgt - panel_depth)*(bIsInteriorGirder ? 2 : 1)*nMatingSurfaces*panel_support_width)*  pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
      ASSERT( 0 <= wpad );

      if ( nGirders == 1 )
         wslab -= wpad; // for the nGirders == 1 case, wslab contains the pad load... remove the pad load

      LOG("Poi Loc at           = " << poi.GetDistFromStart());
      LOG("Main Slab Loa        = " << wslab);
      LOG("Slab Pad Load        = " << wpad);

      SlabLoad sload;
      sload.Loc          = poi.GetDistFromStart();
      sload.MainSlabLoad = -wslab;  // + is upward
      sload.PanelLoad    = -wslab_panel;
      sload.PadLoad      = -wpad;

      pSlabLoads->push_back(sload);
   }
}

void CAnalysisAgentImp::GetCantileverSlabLoad(SpanIndexType span,GirderIndexType gdr, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(ISectProp2,pSectProp2);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   if ( pDeck->DeckType == pgsTypes::sdtNone )
   {
      *pP1 = 0;
      *pM1 = 0;
      *pP2 = 0;
      *pM2 = 0;
      return;
   }

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   pgsPointOfInterest poi(span,gdrIdx,0.00);

   Float64 AdimStart        = pBridge->GetSlabOffset(span,gdrIdx,pgsTypes::metStart);
   Float64 AdimEnd          = pBridge->GetSlabOffset(span,gdrIdx,pgsTypes::metEnd);
   Float64 fillet           = pDeck->Fillet;
   Float64 slab_depth       = pBridge->GetGrossSlabDepth( poi );
   Float64 top_flange_width = pGdr->GetTopFlangeWidth( poi );
   Float64 gdr_height       = pGdr->GetHeight( poi );

   Float64 gdr_length = pBridge->GetGirderLength(span,gdrIdx);

   // Apply the portion of the slab load that is outside the cl-bearings
   // This is load that is along the length of the girder, not transverse to the
   // girder centerline.
   // Assume pad load extends only to end of girder

   PierIndexType prev_pier = span;
   PierIndexType next_pier = prev_pier + 1;

   // left end
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdrIdx );
   Float64 start_trib_slab_width = pSectProp2->GetTributaryFlangeWidth( poi );

   bool bIsInteriorGirder = pBridge->IsInteriorGirder(span,gdrIdx);

   Float64 wgt_dens = pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
   Float64 Pmain;
   if ( bIsInteriorGirder )
   {
      Pmain = start_trib_slab_width * slab_depth * end_size * wgt_dens;  // main slab
   }
   else
   {
      // determine depth of the slab at the edge and flange tip
      Float64 overhang_edge_depth = pDeck->OverhangEdgeDepth;
      Float64 overhang_depth_at_flange_tip = overhang_edge_depth;
      if ( pDeck->OverhangTaper == pgsTypes::None )
      {
         // overhang is constant depth
         overhang_depth_at_flange_tip = overhang_edge_depth;
      }
      else if ( pDeck->OverhangTaper == pgsTypes::TopTopFlange )
      {
         // deck overhang tapers to the top of the top flange
         overhang_depth_at_flange_tip = AdimStart;
      }
      else if ( pDeck->OverhangTaper == pgsTypes::BottomTopFlange )
      {
         // deck overhang tapers to the bottom of the top flange
         FlangeIndexType nFlanges = pGdr->GetNumberOfTopFlanges(span,gdrIdx);
         Float64 flange_thickness;
         if ( gdrIdx == 0 )
            flange_thickness = pGdr->GetTopFlangeThickness(poi,0);
         else
            flange_thickness = pGdr->GetTopFlangeThickness(poi,nFlanges-1);

         overhang_depth_at_flange_tip = AdimStart + flange_thickness;
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
      Float64 w = start_trib_slab_width - slab_overhang;
      Float64 slab_area = w*slab_depth;
      Float64 wslab = (slab_area + slab_overhang_area) * wgt_dens;

      Pmain = wslab * end_size;  // main slab
   }

   // + force is up, + moment is ccw
   *pP1 = -Pmain;
   if ( end_size < gdr_height )
      *pM1 = 0;
   else
      *pM1 = Pmain*end_size/2.0;

   // right end
   end_size = pBridge->GetGirderEndConnectionLength( span, gdrIdx );
   Float64 end_trib_slab_width = pSectProp2->GetTributaryFlangeWidth( pgsPointOfInterest( span, gdrIdx, gdr_length ) );

   if ( bIsInteriorGirder )
   {
      Pmain = end_trib_slab_width * slab_depth * end_size * wgt_dens;  // main slab
   }
   else
   {
      // determine depth of the slab at the edge and flange tip
      Float64 overhang_edge_depth = pDeck->OverhangEdgeDepth;
      Float64 overhang_depth_at_flange_tip = overhang_edge_depth;
      if ( pDeck->OverhangTaper == pgsTypes::None )
      {
         // overhang is constant depth
         overhang_depth_at_flange_tip = overhang_edge_depth;
      }
      else if ( pDeck->OverhangTaper == pgsTypes::TopTopFlange )
      {
         // deck overhang tapers to the top of the top flange
         overhang_depth_at_flange_tip = AdimEnd;
      }
      else if ( pDeck->OverhangTaper == pgsTypes::BottomTopFlange )
      {
         // deck overhang tapers to the bottom of the top flange
         FlangeIndexType nFlanges = pGdr->GetNumberOfTopFlanges(span,gdrIdx);
         Float64 flange_thickness;
         if ( gdrIdx == 0 )
            flange_thickness = pGdr->GetTopFlangeThickness(poi,0);
         else
            flange_thickness = pGdr->GetTopFlangeThickness(poi,nFlanges-1);

         overhang_depth_at_flange_tip = AdimEnd + flange_thickness;
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
      Float64 w = end_trib_slab_width - slab_overhang;
      Float64 slab_area = w*slab_depth;
      Float64 wslab = (slab_area + slab_overhang_area) * wgt_dens;

      Pmain = wslab * end_size;  // main slab
   }

   *pP2 =  -Pmain;
   if ( end_size < gdr_height )
      *pM2 = 0;
   else
      *pM2 = -(Pmain*end_size/2.0);
}

void CAnalysisAgentImp::GetCantileverSlabPadLoad(SpanIndexType span,GirderIndexType gdr, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(ISectProp2,pSectProp2);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   if ( pDeck->DeckType == pgsTypes::sdtNone )
   {
      *pP1 = 0;
      *pM1 = 0;
      *pP2 = 0;
      *pM2 = 0;
      return;
   }

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   pgsPointOfInterest poi(span,gdrIdx,0.00);

   Float64 AdimStart        = pBridge->GetSlabOffset(span,gdrIdx,pgsTypes::metStart);
   Float64 AdimEnd          = pBridge->GetSlabOffset(span,gdrIdx,pgsTypes::metEnd);
   Float64 fillet           = pDeck->Fillet;
   Float64 slab_depth       = pBridge->GetGrossSlabDepth( poi );
   Float64 top_flange_width = pGdr->GetTopFlangeWidth( poi );
   Float64 gdr_height       = pGdr->GetHeight( poi );

   Float64 gdr_length = pBridge->GetGirderLength(span,gdrIdx);

   // Apply the portion of the slab load that is outside the cl-bearings
   // This is load that is along the length of the girder, not transverse to the
   // girder centerline.
   // Assume pad load extends only to end of girder

   PierIndexType prev_pier = span;
   PierIndexType next_pier = prev_pier + 1;

   // left end
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdrIdx );
   Float64 start_trib_slab_width = pSectProp2->GetTributaryFlangeWidth( poi );

   bool bIsInteriorGirder = pBridge->IsInteriorGirder(span,gdrIdx);

   Float64 wgt_dens = pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
   Float64 Ppad = (AdimStart - slab_depth) * top_flange_width * end_size * wgt_dens; // slab pad

   // + force is up, + moment is ccw
   *pP1 = -Ppad;
   if ( end_size < gdr_height )
      *pM1 = 0;
   else
      *pM1 = Ppad*end_size/2.0;

   // right end
   end_size = pBridge->GetGirderEndConnectionLength( span, gdrIdx );
   Float64 end_trib_slab_width = pSectProp2->GetTributaryFlangeWidth( pgsPointOfInterest( span, gdrIdx, gdr_length ) );

   Ppad  = (AdimEnd - slab_depth) * top_flange_width * end_size * wgt_dens; // slab pad

   *pP2 =  -Ppad;
   if ( end_size < gdr_height )
      *pM2 = 0;
   else
      *pM2 = -(Ppad*end_size/2.0);
}

void CAnalysisAgentImp::GetMainSpanOverlayLoad(SpanIndexType span,GirderIndexType gdr, std::vector<OverlayLoad>* pOverlayLoads)
{
   CHECK(pOverlayLoads!=0);
   pOverlayLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   Float64 OverlayWeight = pBridge->GetOverlayWeight();

   GET_IFACE( ISpecification, pSpec );
   pgsTypes::OverlayLoadDistributionType olayd = pSpec->GetOverlayLoadDistributionType();

   // POIs where overlay loads are laid out
   PoiAttributeType attrib = POI_ALLACTIONS;
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest(span,gdrIdx,pgsTypes::BridgeSite3,attrib,POIFIND_OR);
   CHECK(vPoi.size()!=0);

   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGirder);

   // Width of loaded area, and load intensity
   Float64 startWidth(0), endWidth(0);
   Float64 startW(0), endW(0);
   Float64 startLoc(0), endLoc(0);

   CollectionIndexType num_poi = vPoi.size();
   ATLASSERT(num_poi>2); // otherwise our loop won't create any loads
   for ( CollectionIndexType i = 0; i < num_poi; i++ )
   {
      // poi at end of distributed load
      const pgsPointOfInterest& endPoi = vPoi[i];

      endLoc = endPoi.GetDistFromStart();

      Float64 station,girder_offset;
      pBridge->GetStationAndOffset(endPoi,&station,&girder_offset);
      Float64 dist_from_start_of_bridge = pBridge->GetDistanceFromStartOfBridge(station);

      // Offsets to toe where overlay starts
      Float64 left_olay_offset  = pBridge->GetLeftOverlayToeOffset(dist_from_start_of_bridge);
      Float64 right_olay_offset = pBridge->GetRightOverlayToeOffset(dist_from_start_of_bridge);

      if (olayd==pgsTypes::olDistributeEvenly)
      {
         // This one is easy. girders see overlay even if they aren't under it
         // Total overlay width at location
         endWidth = right_olay_offset - left_olay_offset;
         endW = -endWidth*OverlayWeight/nGirders;
      }
      else if (olayd==pgsTypes::olDistributeTributaryWidth)
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

            Float64 width = max(pGirder->GetTopWidth(endPoi),pGirder->GetBottomWidth(endPoi));
            Float64 width2 = width/2.0;

            // Note that tributary width ignores joint spacing
            DLT = DGDR - width2 - leftJ/2.0;;
            DRT = DGDR + width2 + rightJ/2.0;
         }
         else
         {
            Float64 lftTw, rgtTw;
            Float64 tribWidth = pSectProp2->GetTributaryFlangeWidthEx(endPoi, &lftTw, &rgtTw);

            DLT = DGDR - lftTw;
            DRT = DGDR + rgtTw;
         }

         // Now we have distances for all needed elements. Next figure out how much
         // of overlay lies within tributary width
         ATLASSERT(DRO>DLO); // negative overlay widths should be handled elsewhere

         if (DLO <= DLT)
         {
            if(DRO >= DRT)
            {
              endWidth = DRT-DLT;
            }
            else if (DRO > DLT)
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
      if (i!=0)
      {
         OverlayLoad load;
         load.StartLoc = startLoc;
         load.EndLoc   = endLoc;
         load.StartWcc = startWidth;
         load.EndWcc   = endWidth;
         load.StartLoad = startW;
         load.EndLoad   = endW;

         pOverlayLoads->push_back(load);
      }

      // Set variables for next go through loop
      startLoc   = endLoc;
      startWidth = endWidth;
      startW     = endW;
   }
}

void CAnalysisAgentImp::GetMainConstructionLoad(SpanIndexType span,GirderIndexType gdr, std::vector<ConstructionLoad>* pConstructionLoads)
{
   CHECK(pConstructionLoads!=0);
   pConstructionLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IPointOfInterest,pIPoi);

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   GET_IFACE(IUserDefinedLoadData,pLoads);
   Float64 construction_load = pLoads->GetConstructionLoad();

   // Get some important POIs that we will be using later
   PoiAttributeType attrib = POI_ALLACTIONS;
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest(span,gdrIdx,pgsTypes::BridgeSite1,attrib,POIFIND_OR);
   CHECK(vPoi.size()!=0);

   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGirder);

   CollectionIndexType num_poi = vPoi.size();
   for ( CollectionIndexType i = 0; i < num_poi-1; i++ )
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

         Float64 width = max(pGirder->GetTopWidth(prevPoi),pGirder->GetBottomWidth(prevPoi));

         startWidth = width + (left+right)/2;
         startW = -startWidth*construction_load;

         pBridge->GetDistanceBetweenGirders(currPoi,&left,&right);

         width = max(pGirder->GetTopWidth(currPoi),pGirder->GetBottomWidth(currPoi));

         endWidth = width + (left+right)/2;
         endW = -endWidth*construction_load;
      }
      else
      {
         startWidth = pSectProp2->GetTributaryFlangeWidth(prevPoi);
         // negative width means that slab is not over girder
         if (startWidth < 0.0)
            startWidth = 0.0;

         startW = -startWidth*construction_load;

         endWidth = pSectProp2->GetTributaryFlangeWidth(currPoi);
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
      load.StartLoad = startW;
      load.EndLoad   = endW;

      pConstructionLoads->push_back(load);
   }
}

void CAnalysisAgentImp::GetMainSpanShearKeyLoad(SpanIndexType span,GirderIndexType gdr, std::vector<ShearKeyLoad>* pLoads)
{
   CHECK(pLoads!=0); 
   pLoads->clear();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pBridgeMaterial);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   // Check if there is a shear key before we get too far
   bool is_key = pGirder->HasShearKey(span, gdrIdx, spacingType);
   if (!is_key || nGirders==1)
   {
      return;
   }

   // areas of shear key per interior side
   Float64 unif_area, joint_area;
   pGirder->GetShearKeyAreas(span, gdrIdx, spacingType, &unif_area, &joint_area);

   Float64 slab_unitw = pBridgeMaterial->GetWgtDensitySlab()* unitSysUnitsMgr::GetGravitationalAcceleration();
   Float64 unif_wt      = unif_area  * slab_unitw;
   Float64 joint_wt_per = joint_area * slab_unitw;

   // See if we need to go further
   bool is_joint_spacing = ::IsJointSpacing(spacingType);

   if (unif_wt==0.0)
   {
      if (joint_wt_per==0.0 || !is_joint_spacing)
      {
         return; // no applied load
      }
   }

   bool is_exterior = gdrIdx==0 || gdrIdx==nGirders-1;
   Float64 nsides = is_exterior ? 1 : 2;

   // If only uniform load, we can apply along entire length
   if (joint_wt_per==0.0)
   {
      ShearKeyLoad load;
      load.StartLoc = 0.0;
      load.EndLoc   = pBridge->GetGirderLength(span,gdrIdx);

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
      PoiAttributeType attrib = POI_ALLACTIONS;
      std::vector<pgsPointOfInterest> vPoi;
      vPoi = pIPoi->GetPointsOfInterest(span,gdrIdx,pgsTypes::BridgeSite3,attrib,POIFIND_OR);
      CHECK(vPoi.size()!=0);

      CollectionIndexType num_poi = vPoi.size();
      for ( CollectionIndexType i = 0; i < num_poi-1; i++ )
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


void CAnalysisAgentImp::GetIntermediateDiaphragmLoads(pgsTypes::Stage stage, SpanIndexType spanIdx,GirderIndexType gdrIdx, std::vector<DiaphragmLoad>* pLoads)
{
   CHECK(pLoads!=0);
   pLoads->clear();

   GET_IFACE( IBridge,   pBridge   );
   GET_IFACE( IBridgeMaterial, pMaterial );

   GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
   gdrIdx = min(gdrIdx,nGirders-1);

   Float64 density;
   if ( stage == pgsTypes::CastingYard )
      density = pMaterial->GetWgtDensityGdr(spanIdx,gdrIdx); // cast with girder, using girder concrete
   else
      density = pMaterial->GetWgtDensitySlab(); // cast with slab, using slab concrete

   Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();

   std::vector<IntermedateDiaphragm> diaphragms = pBridge->GetIntermediateDiaphragms(stage,spanIdx,gdrIdx);
   std::vector<IntermedateDiaphragm>::iterator iter;
   for ( iter = diaphragms.begin(); iter != diaphragms.end(); iter++ )
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

void CAnalysisAgentImp::GetEndDiaphragmLoads(SpanIndexType span,GirderIndexType gdr, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   //
   // Determine the loads caused by the end diaphragms
   //
   GET_IFACE(IBridge,   pBridge );
   GET_IFACE(IBridgeMaterial, pMat );
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGirder);

   GirderIndexType nGirders = pBridge->GetGirderCount(span);
   GirderIndexType gdrIdx = min(gdr,nGirders-1);

   *pP1 = 0.0;
   *pM1 = 0.0;
   *pP2 = 0.0;
   *pM2 = 0.0;

   Float64 H;
   Float64 W;
   Float64 Density = pMat->GetWgtDensitySlab();
   Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
   Float64 brg_offset;
   Float64 moment_arm;
   Float64 trib_slab_width;

   PierIndexType prev_pier = span;
   PierIndexType next_pier = prev_pier + 1;

   // Left end of girder
   bool apply_left = pBridge->DoesRightSideEndDiaphragmLoadGirder(prev_pier);
   if (apply_left)
   {
      // get skew angle so tributary width can be adjusted for pier skew
      // (diaphragm length is longer on skewed piers)
      CComPtr<IAngle> objSkew;
      pBridge->GetPierSkew(span,&objSkew);
      Float64 skew;
      objSkew->get_Value(&skew);

      pgsPointOfInterest poi(span,gdrIdx,0.00);
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      {
         Float64 left,right;
         pBridge->GetDistanceBetweenGirders(poi,&left,&right);

         Float64 width = max(pGirder->GetTopWidth(poi),pGirder->GetBottomWidth(poi));
         trib_slab_width = width + (left+right)/2;
      }
      else
      {
         trib_slab_width = pSectProp2->GetTributaryFlangeWidth( poi );
      }

      pBridge->GetRightSideEndDiaphragmSize(prev_pier,&W,&H);
      *pP1 = - H*W*Density*g*trib_slab_width/cos(skew);

      brg_offset = pBridge->GetGirderStartBearingOffset( span, gdrIdx );
      moment_arm = brg_offset - pBridge->GetEndDiaphragmLoadLocationAtStart(span, gdrIdx);
      CHECK(moment_arm<=brg_offset); // diaphragm load should be on same side of pier as girder
      *pM1 = - *pP1*moment_arm;
   }

   // Right end of the girder
   bool apply_right = pBridge->DoesLeftSideEndDiaphragmLoadGirder(next_pier);
   if (apply_right)
   {
      // get skew angle so tributary width can be adjusted for pier skew
      // (diaphragm length is longer on skewed piers)
      CComPtr<IAngle> objSkew;
      pBridge->GetPierSkew(span+1,&objSkew);
      Float64 skew;
      objSkew->get_Value(&skew);

      Float64 end_size = pBridge->GetGirderEndConnectionLength(span,gdrIdx);
      Float64 gdr_length = pBridge->GetGirderLength(span,gdrIdx);

      pgsPointOfInterest poi(span,gdrIdx,gdr_length-end_size);
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      {
         Float64 left,right;
         pBridge->GetDistanceBetweenGirders(poi,&left,&right);

         Float64 width = max(pGirder->GetTopWidth(poi),pGirder->GetBottomWidth(poi));
         trib_slab_width = width + (left+right)/2;
      }
      else
      {
         trib_slab_width = pSectProp2->GetTributaryFlangeWidth( poi );
      }

      pBridge->GetLeftSideEndDiaphragmSize(next_pier,&W,&H);
      *pP2 = - H*W*Density*g*trib_slab_width/cos(skew);

      brg_offset = pBridge->GetGirderEndBearingOffset( span, gdrIdx );
      moment_arm = brg_offset - pBridge->GetEndDiaphragmLoadLocationAtEnd(span, gdrIdx);
      ATLASSERT(moment_arm<=brg_offset); // diaphragm load should be on same side of pier as girder
      *pM2 = *pP2 * moment_arm;
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   pgsTypes::Stage girderLoadStage = GetGirderDeadLoadStage(span,gdr);

   Float64 delta = GetDisplacement(girderLoadStage,pftGirder,poi,SimpleSpan);
   Float64 rotation  = GetRotation(girderLoadStage,pftGirder,poi,SimpleSpan);
   GET_IFACE(IBridgeMaterial,pMat);
   Float64 Ec = pMat->GetEcGdr(span,gdr);

   Float64 Eci = pMat->GetEciGdr(span,gdr);

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
   // In the casting yard, during storage, it is assumed that the girder is supported at the
   // locations of the bearings in the final bridge configuration.  For this reason, get
   // the girder deflection at the BridgeSite1 stage.  Note that this deflection is based on
   // the final modulus of elasticity and we need to base it on the modulus of elasticity
   // at release.  Since the assumptions of linear elastic models applies, simply scale
   // the deflection by Ec/Eci
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   pgsTypes::Stage girderLoadStage = GetGirderDeadLoadStage(span,gdr);

   Float64 delta    = GetDisplacement(girderLoadStage,pftGirder,poi,SimpleSpan);
   Float64 rotation = GetRotation(girderLoadStage,pftGirder,poi,SimpleSpan);

   GET_IFACE(IBridgeMaterialEx,pMat);

   Float64 Ec = pMat->GetEcGdr(span,gdr); // this Ec used to comptue delta

   Float64 Eci;
   if ( config.bUserEci )
      Eci = config.Eci;
   else
      Eci = pMat->GetEconc(config.Fci,pMat->GetStrDensityGdr(span,gdr),pMat->GetEccK1Gdr(span,gdr),pMat->GetEccK2Gdr(span,gdr));

   delta    *= (Ec/Eci);
   rotation *= (Ec/Eci);

   *pDy = delta;
   *pRz = rotation;
}

void CAnalysisAgentImp::GetLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   std::vector<VehicleIndexType> MminTruck, MmaxTruck;

   GetLiveLoadMoment(llType,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Mmin,&Mmax,pMminTruck ? &MminTruck : NULL, pMmaxTruck ? &MmaxTruck : NULL);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMmin = Mmin[0];
   *pMmax = Mmax[0];

   if ( pMminTruck )
   {
      if ( 0 < MminTruck.size() )
         *pMminTruck = MminTruck[0];
      else
         *pMminTruck = INVALID_INDEX;
   }

   if ( pMmaxTruck )
   {
      if ( 0 < MmaxTruck.size() )
         *pMmaxTruck = MmaxTruck[0];
      else
         *pMmaxTruck = INVALID_INDEX;
   }
}

void CAnalysisAgentImp::GetLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   std::vector<VehicleIndexType> MminTruck, MmaxTruck;
   GetLiveLoadShear(llType,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Vmin,&Vmax,pMminTruck ? &MminTruck : NULL, pMmaxTruck ? &MmaxTruck : NULL);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pVmin = Vmin[0];
   *pVmax = Vmax[0];

   if ( pMminTruck )
   {
      if ( 0 < MminTruck.size() )
         *pMminTruck = MminTruck[0];
      else
         *pMminTruck = INVALID_INDEX;
   }

   if ( pMmaxTruck )
   {
      if ( 0 < MmaxTruck.size() )
         *pMmaxTruck = MmaxTruck[0];
      else
         *pMmaxTruck = INVALID_INDEX;
   }
}

void CAnalysisAgentImp::GetLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   std::vector<VehicleIndexType> DminTruck, DmaxTruck;
   GetLiveLoadDisplacement(llType,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Dmin,&Dmax,&DminTruck,&DmaxTruck);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);


   *pDmin = Dmin[0];
   *pDmax = Dmax[0];

   if ( pMinConfig )
   {
      if ( 0 < DminTruck.size() )
         *pMinConfig = DminTruck[0];
      else
         *pMinConfig = INVALID_INDEX;
   }

   if ( pMaxConfig )
   {
      if ( 0 < DmaxTruck.size() )
         *pMaxConfig = DmaxTruck[0];
      else
         *pMaxConfig = INVALID_INDEX;
   }
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   std::vector<VehicleIndexType> RminTruck, RmaxTruck;
   GetLiveLoadRotation(llType,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&RminTruck,&RmaxTruck);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin[0];
   *pRmax = Rmax[0];

   if ( pMinConfig )
   {
      if ( 0 < RminTruck.size() )
         *pMinConfig = RminTruck[0];
      else
         *pMinConfig = INVALID_INDEX;
   }

   if ( pMaxConfig )
   {
      if ( 0 < RmaxTruck.size() )
         *pMaxConfig = RmaxTruck[0];
      else
         *pMaxConfig = INVALID_INDEX;
   }
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   Float64 Rmin, Rmax;
   GetLiveLoadRotation(llType,stage,pier,gdr,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,&Rmin,&Rmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   SpanIndexType spanIdx = SpanIndexType((pierFace == pgsTypes::Back ? pier-1 : pier));

   GET_IFACE(IBridge,pBridge);
   Float64 distFromStart = pBridge->GetGirderStartConnectionLength(spanIdx,gdr);
   if ( pierFace == pgsTypes::Back )
      distFromStart += pBridge->GetSpanLength(spanIdx,gdr);

   pgsPointOfInterest poi(spanIdx,gdr,distFromStart);
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

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
         *pMaxConfig = INVALID_INDEX;
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
         // get reaction that corresonds to T max
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == SimpleSpan ? false : true, &pEngine);
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
         *pRmax = -1.0;
      }
   }

   if ( pRmin )
   {
      if ( TzMinConfig )
      {
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == SimpleSpan ? false : true, &pEngine);
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
         *pRmin = -1.0;
      }
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   GetLiveLoadReaction(llType,stage,pier,gdr,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,NULL,NULL,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(gdr);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(pier);


   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   CComPtr<ILiveLoadModelResults> minResults;
   CComPtr<ILiveLoadModelResults> maxResults;
   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   pModelData->pLiveLoadResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, 
          fetFy, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, 
          fetFy, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   CComPtr<ILiveLoadConfiguration> minConfig;
   CComPtr<ILiveLoadConfiguration> maxConfig;

   Float64 Rmax;
   maxResults->GetResult(0,&Rmax,&maxConfig);
   if ( pMaxConfig )
   {
      if ( maxConfig )
      {
         VehicleIndexType vehIdx;
         maxConfig->get_VehicleIndex(&vehIdx);
         *pMaxConfig = vehIdx;
      }
      else
      {
         *pMaxConfig = INVALID_INDEX;
      }
   }

   Float64 Rmin;
   minResults->GetResult(0,&Rmin,&minConfig);
   if ( pMinConfig )
   {
      if ( minConfig )
      {
         VehicleIndexType vehIdx;
         minConfig->get_VehicleIndex(&vehIdx);
         *pMinConfig = vehIdx;
      }
      else
      {
         *pMinConfig = INVALID_INDEX;
      }
   }

   *pRmin = Rmin;
   *pRmax = Rmax;

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(pier);
   if ( pTmax )
   {
      if ( maxConfig )
      {
         // get rotatation that corresonds to R max
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         CComPtr<IResult3Ds> results;
         maxConfig->put_ForceEffect(fetRz);
         maxConfig->put_Optimization(optMaximize);
         response->ComputeSupportDeflections( m_LBAMPoi, bstrStage, maxConfig, &results );

         CComPtr<IResult3D> result;
         results->get_Item(0,&result);

         Float64 T;
         result->get_Z(&T);
         *pTmax = T;
      }
      else
      {
         *pTmax = -1.0;
      }
   }

   if ( pTmin )
   {
      if ( minConfig )
      {
         CComPtr<ILBAMAnalysisEngine> pEngine;
         GetEngine(pModelData,bat == SimpleSpan ? false : true, &pEngine);
         CComPtr<IBasicVehicularResponse> response;
         pEngine->get_BasicVehicularResponse(&response);

         // get rotatation that corresonds to R min
         CComPtr<IResult3Ds> results;
         minConfig->put_ForceEffect(fetRz);
         minConfig->put_Optimization(optMaximize);
         response->ComputeSupportDeflections( m_LBAMPoi, bstrStage, minConfig, &results );

         CComPtr<IResult3D> result;
         results->get_Item(0,&result);

         Float64 T;
         result->get_Z(&T);
         *pTmin = T;
      }
      else
      {
         *pTmin = -1.0;
      }
   }
}

void CAnalysisAgentImp::GetLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig,VehicleIndexType* pTopMaxConfig,VehicleIndexType* pBotMinConfig,VehicleIndexType* pBotMaxConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   std::vector<VehicleIndexType> topMinConfig, topMaxConfig, botMinConfig, botMaxConfig;
   GetLiveLoadStress(llType,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,&fTopMin,&fTopMax,&fBotMin,&fBotMax,&topMinConfig, &topMaxConfig, &botMinConfig, &botMaxConfig);

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

void CAnalysisAgentImp::GetLiveLoadModel(pgsTypes::LiveLoadType llType,GirderIndexType gdrIdx,ILiveLoadModel** ppLiveLoadModel)
{
   // if the gdr index is ALL_GIRDERS then we don't really care which girder line so use 0
   // live load is the same on all girder lines
   if ( gdrIdx == ALL_GIRDERS )
      gdrIdx = 0;

   ModelData* pModelData = GetModelData(gdrIdx);

   CComPtr<ILiveLoad> live_load;
   pModelData->m_Model->get_LiveLoad(&live_load);

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

std::vector<std::_tstring> CAnalysisAgentImp::GetVehicleNames(pgsTypes::LiveLoadType llType,GirderIndexType gdr)
{
   USES_CONVERSION;

   CComPtr<ILiveLoadModel> liveload_model;
   GetLiveLoadModel(llType,gdr,&liveload_model);

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

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   
   std::vector<Float64> Mmin, Mmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadMoment(llType,vehicleIndex,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,
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

void CAnalysisAgentImp::GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,AxleConfiguration* pMinLeftAxleConfig,AxleConfiguration* pMinRightAxleConfig,AxleConfiguration* pMaxLeftAxleConfig,AxleConfiguration* pMaxRightAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   std::vector<AxleConfiguration> minLeftAxleConfig,minRightAxleConfig,maxLeftAxleConfig,maxRightAxleConfig;
   GetVehicularLiveLoadShear(llType,vehicleIndex,stage,vPoi,bat,
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

void CAnalysisAgentImp::GetVehicularLiveLoadDisplacement(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadDisplacement(llType,vehicleIndex,stage,vPoi,bat,
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

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   std::vector<AxleConfiguration> minAxleConfig,maxAxleConfig;
   GetVehicularLiveLoadRotation(llType,vehicleIndex,stage,vPoi,bat,bIncludeImpact, bIncludeLLDF,
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

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(gdr);

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(pier);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   CComPtr<ILiveLoadModelResults> minResults;
   CComPtr<ILiveLoadModelResults> maxResults;
   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   pModelData->pVehicularResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetFy, optMinimize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeReactions( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetFy, optMaximize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   Float64 Rmax;
   CComPtr<ILiveLoadConfiguration> RzMaxConfig;
   maxResults->GetResult(0,&Rmax,pMaxAxleConfig ? &RzMaxConfig : NULL);

   Float64 Rmin;
   CComPtr<ILiveLoadConfiguration> RzMinConfig;
   minResults->GetResult(0,&Rmin,pMinAxleConfig ? &RzMinConfig : NULL);

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

void CAnalysisAgentImp::GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop,AxleConfiguration* pMaxAxleConfigTop,AxleConfiguration* pMinAxleConfigBot,AxleConfiguration* pMaxAxleConfigBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   std::vector<AxleConfiguration> minAxleConfigTop,maxAxleConfigTop,minAxleConfigBot,maxAxleConfigBot;
   GetVehicularLiveLoadStress(llType,vehicleIndex,stage,vPoi,bat,bIncludeImpact,bIncludeLLDF,&fTopMin,&fTopMax,&fBotMin,&fBotMax,&minAxleConfigTop,&maxAxleConfigTop,&minAxleConfigBot,&maxAxleConfigBot);

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

void CAnalysisAgentImp::GetDeflLiveLoadDisplacement(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,Float64* pDmin,Float64* pDmax)
{
   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(poi.GetGirder());


   // make sure there are actually loads applied
   CComPtr<ILiveLoad> live_load;
   pModelData->m_Model->get_LiveLoad(&live_load);

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
      ATLASSERT( 0 <= poi_id && poi_id != INVALID_ID ); // if this fires, the poi wasn't added... WHY???
   }

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(poi_id);

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;

   if (type==IProductForces::DeflectionLiveLoadEnvelope)
   {
      pModelData->pDeflLiveLoadResponse->ComputeDeflections( m_LBAMPoi, CComBSTR("Bridge Site 3"), lltDeflection, 
             fetFy, optMaximize, vlcDefault, VARIANT_TRUE,VARIANT_TRUE, VARIANT_FALSE,&maxResults);

      pModelData->pDeflLiveLoadResponse->ComputeDeflections( m_LBAMPoi, CComBSTR("Bridge Site 3"), lltDeflection, 
             fetFy, optMinimize, vlcDefault, VARIANT_TRUE,VARIANT_TRUE, VARIANT_FALSE,&minResults);
   }
   else
   {
      ATLASSERT(type==IProductForces::DesignTruckAlone || type==IProductForces::Design25PlusLane);

      // Assumes VehicularLoads are put in in same order as enum;
      VehicleIndexType trk_idx = (VehicleIndexType)type;

      pModelData->pDeflEnvelopedVehicularResponse->ComputeDeflections( m_LBAMPoi, CComBSTR("Bridge Site 3"), lltDeflection, trk_idx,
             fetFy, optMaximize, vlcDefault, VARIANT_TRUE,dftSingleLane, VARIANT_FALSE,&maxResults);

      pModelData->pDeflEnvelopedVehicularResponse->ComputeDeflections( m_LBAMPoi, CComBSTR("Bridge Site 3"), lltDeflection, trk_idx,
             fetFy, optMinimize, vlcDefault, VARIANT_TRUE,dftSingleLane, VARIANT_FALSE,&minResults);
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetGirderStartConnectionLength(span,gdr);
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
   GET_IFACE(ISectProp2,pSectProp2);
   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   Float64 M = GetDesignSlabPadMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi);

   Float64 Sbg = pSectProp2->GetSb(pgsTypes::BridgeSite1,poi);
   Float64 Stg = pSectProp2->GetSt(pgsTypes::BridgeSite1,poi);

   *pfTop = M/Stg;
   *pfBot = M/Sbg;
}

rkPPPartUniformLoad CAnalysisAgentImp::GetDesignSlabPadModel(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterialEx,pMat);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectProp2,pSectProp2);

   Float64 AdStart = startSlabOffset;
   Float64 AdEnd   = endSlabOffset;
   Float64 AoStart = pBridge->GetSlabOffset(span,gdr,pgsTypes::metStart);
   Float64 AoEnd   = pBridge->GetSlabOffset(span,gdr,pgsTypes::metEnd);

   Float64 top_flange_width = pGdr->GetTopFlangeWidth( poi );

   Float64 Ig = pSectProp2->GetIx( pgsTypes::BridgeSite1,poi );
   
   Float64 E = pMat->GetEconc(fcgdr,pMat->GetStrDensityGdr(span,gdr), pMat->GetEccK1Gdr(span,gdr), pMat->GetEccK2Gdr(span,gdr) );

   Float64 L = pBridge->GetSpanLength( span, gdr );

   Float64 wStart = (AdStart - AoStart)*top_flange_width*pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
   Float64 wEnd   = (AdEnd   - AoEnd  )*top_flange_width*pMat->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();

#pragma Reminder("UPDATE: This is incorrect if girders are made continuous before slab casting")
#pragma Reminder("UPDATE: Don't have a roark beam for trapezoidal loads")
   // use the average load
   Float64 w = (wStart + wEnd)/2;

   rkPPPartUniformLoad beam(0,L,-w,L,E,Ig);
   return beam;
}

void CAnalysisAgentImp::DumpAnalysisModels(GirderIndexType girderLineIdx)
{
   ModelData* pModelData = GetModelData(girderLineIdx);

   CComPtr<IStructuredSave2> save;
   save.CoCreateInstance(CLSID_StructuredSave2);
   save->Open(CComBSTR("LBAM.xml"));

   CComQIPtr<IStructuredStorage2> strstorage(pModelData->m_Model);
   strstorage->Save(save);

   save->Close();

   if ( pModelData->m_ContinuousModel )
   {
      CComPtr<IStructuredSave2> save2;
      save2.CoCreateInstance(CLSID_StructuredSave2);
      save2->Open(CComBSTR("LBAM_Continuous.xml"));

      CComQIPtr<IStructuredStorage2> strstorage(pModelData->m_ContinuousModel);
      strstorage->Save(save2);

      save2->Close();
   }
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot)
{
   // this is sort of a dummy function until deck shrinkage stress issues are resolved
   // if you count on deck shrinkage for elastic gain, then you have to account for the fact
   // that the deck shrinkage changes the stresses in the girder as well. deck shrinkage is
   // an external load to the girder

   // Top and bottom girder stresses are computed using the composite section method described in
   // Branson, D. E., "Time-Dependent Effects in Composite Concrete Beams", 
   // American Concrete Institute J., Vol 61 (1964) pp. 213-230

   pgsTypes::Stage compositeStage = pgsTypes::BridgeSite3;

   GET_IFACE(ILosses,pLosses);
   LOSSDETAILS details = pLosses->GetLossDetails(poi);

   Float64 P, M;
   details.pLosses->GetDeckShrinkageEffects(&P,&M);

   GET_IFACE(ISectProp2,pProps);
   Float64 A  = pProps->GetAg(compositeStage,poi);
   Float64 St = pProps->GetStGirder(compositeStage,poi);
   Float64 Sb = pProps->GetSb(compositeStage,poi);

   *pftop = P/A + M/St;
   *pfbot = P/A + M/Sb;
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
   GetModel(pModelData,SimpleSpan,&lbam_model);

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
   GetLiveLoadModel(llType,0,&liveload_model);

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
   GetModel(pModelData,SimpleSpan,&lbam_model);

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

/////////////////////////////////////////////////////////////////////////////
// IProductForces2
//
std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat)
{
   return GetShear(stage, type, vPoi, bat, ctIncremental);
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,
                                                         BridgeAnalysisType bat, CombinationType cmb_type)
{
   USES_CONVERSION;
   GET_IFACE(IStageMap,pStageMap);

   std::vector<sysSectionValue> results;
   try
   {
      if (stage == pgsTypes::CastingYard)
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            ValidateAnalysisModels(poi.GetSpan(),poi.GetGirder());
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            results.push_back(fy);
         }
         return results;
      }
      else
      {
         ResultsSummationType rsType = (cmb_type == ctIncremental ? rsIncremental : rsCumulative);

         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadGroup = GetLoadGroupName(type);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);


         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsType,&section_results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsType,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsType,&section_results);
      
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();
            CComPtr<ISectionResult3D> result;
            section_results->get_Item(idx,&result);

            Float64 FyLeft, FyRight;
            result->get_YLeft(&FyLeft);
            result->get_YRight(&FyRight);

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

std::vector<Float64> CAnalysisAgentImp::GetMoment(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat)
{
   USES_CONVERSION;
   GET_IFACE(IStageMap,pStageMap);

   std::vector<Float64> results;
   try
   {
      if (stage == pgsTypes::CastingYard)
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels( poi.GetSpan(), poi.GetGirder() );
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            ATLASSERT(IsEqual(mz.Left(),-mz.Right()));
            results.push_back( mz.Left() );
         }
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         GET_IFACE(IBridge,pBridge);

         std::vector<pgsPointOfInterest>::const_iterator iter;
//#if defined _DEBUG
//         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
//         {
//            const pgsPointOfInterest& poi = *iter;
//
//            // Make sure the PGSuper poi and the LBAM poi are mapped to the same locations
//
//            Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
//            Float64 product_location = poi.GetDistFromStart() - start_offset;
//
//            CComPtr<IPOIs> objPOIs;
//            pModelData->m_Model->get_POIs(&objPOIs);
//
//            Int32 poi_id = pModelData->PoiMap.GetModelPoi(poi);
//
//            CComPtr<IPOI> objPOI;
//            objPOIs->Find(poi_id,&objPOI);
//
//            Float64 location;
//            objPOI->get_Location(&location);
//
//            ATLASSERT(IsEqual(location,product_location));
//         }
//#endif

         CComBSTR bstrLoadGroup = GetLoadGroupName(type);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeForces(bstrLoadGroup,m_LBAMPoi,bstrStage,roMember,rsIncremental,&section_results);

         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            IndexType idx = iter - vPoi.begin();

            Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
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

std::vector<Float64> CAnalysisAgentImp::GetDisplacement(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat)
{
   USES_CONVERSION;
   GET_IFACE(IStageMap,pStageMap);

   std::vector<Float64> results;

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         ATLASSERT( type == pftGirder );

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels(poi.GetSpan(),poi.GetGirder());
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            results.push_back( dy );
         }
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         std::vector<pgsPointOfInterest>::const_iterator iter;
//#if defined _DEBUG
//         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
//         {
//            const pgsPointOfInterest& poi = *iter;
//
//            // Make sure the PGSuper poi and the LBAM poi are mapped to the same locations
//            CComPtr<IPOIs> objPOIs;
//            pModelData->m_Model->get_POIs(&objPOIs);
//
//            Int32 poi_id = pModelData->PoiMap.GetModelPoi(poi);
//
//            CComPtr<IPOI> objPOI;
//            objPOIs->Find(poi_id,&objPOI);
//
//            Float64 location;
//            objPOI->get_Location(&location);
//
//            GET_IFACE(IBridge,pBridge);
//            Float64 start_offset = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
//            Float64 product_location = poi.GetDistFromStart() - start_offset;
//            ATLASSERT(IsEqual(location,product_location));
//         }
//#endif

         CComBSTR bstrLoadGroup = GetLoadGroupName(type);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetDy][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&section_results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetDy][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&section_results);

         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();

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

std::vector<Float64> CAnalysisAgentImp::GetRotation(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);
   std::vector<Float64> results;

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         ATLASSERT( type == pftGirder );

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels(poi.GetSpan(),poi.GetGirder());
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            results.push_back( rz );
         }
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         std::vector<pgsPointOfInterest>::const_iterator iter;
//#if defined _DEBUG
//         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
//         {
//            const pgsPointOfInterest& poi = *iter;
//
//            // Make sure the PGSuper poi and the LBAM poi are mapped to the same locations
//            CComPtr<IPOIs> objPOIs;
//            pModelData->m_Model->get_POIs(&objPOIs);
//
//            Int32 poi_id = pModelData->PoiMap.GetModelPoi(poi);
//
//            CComPtr<IPOI> objPOI;
//            objPOIs->Find(poi_id,&objPOI);
//
//            Float64 location;
//            objPOI->get_Location(&location);
//
//            GET_IFACE(IBridge,pBridge);
//            Float64 start_offset = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
//            Float64 product_location = poi.GetDistFromStart() - start_offset;
//            ATLASSERT(IsEqual(location,product_location));
//         }
//#endif

         CComBSTR bstrLoadGroup = GetLoadGroupName(type);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         CComPtr<ISectionResult3Ds> section_results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadGroupResponseEnvelope[fetRz][optMinimize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&section_results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadGroupResponseEnvelope[fetRz][optMaximize]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&section_results);
         else
            pModelData->pLoadGroupResponse[bat]->ComputeDeflections(bstrLoadGroup,m_LBAMPoi,bstrStage, rsIncremental,&section_results);

         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetStress(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;
   GET_IFACE(IStageMap,pStageMap);
   GET_IFACE(IBridge,pBridge);

   pfTop->clear();
   pfBot->clear();

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels( poi.GetSpan(), poi.GetGirder() );

            Float64 top_results;
            Float64 bot_results;
            top_results = GetSectionStress( m_CastingYardModels, pgsTypes::TopGirder,    poi );
            bot_results = GetSectionStress( m_CastingYardModels, pgsTypes::BottomGirder, poi );
            pfTop->push_back( top_results );
            pfBot->push_back( bot_results );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);


         CComBSTR bstrLoadGroup = GetLoadGroupName(type);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         CComPtr<ISectionStressResults> min_results, max_results, results;
         if ( bat == MinSimpleContinuousEnvelope )
         {
            pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, rsIncremental, &max_results);
            pModelData->pMinLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, rsIncremental, &min_results);
         }
         else if ( bat == MaxSimpleContinuousEnvelope )
         {
            pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, rsIncremental, &max_results);
            pModelData->pMaxLoadGroupResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, rsIncremental, &min_results);
         }
         else
         {
            pModelData->pLoadGroupResponse[bat]->ComputeStresses(bstrLoadGroup, m_LBAMPoi, bstrStage, rsIncremental, &results);
         }
      
         long stress_point_index_top = GetStressPointIndex(pgsTypes::TopGirder);
         long stress_point_index_bot = GetStressPointIndex(pgsTypes::BottomGirder);

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            IndexType idx = iter - vPoi.begin();

            Float64 end_size = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
            Float64 dist_from_start = poi.GetDistFromStart();


            Float64 fTop, fBot;

            if ( bat == MinSimpleContinuousEnvelope )
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
               pfTop->push_back(fTop);
               pfBot->push_back(fBot);
            }
            else if ( bat == MaxSimpleContinuousEnvelope )
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
               pfTop->push_back(fTop);
               pfBot->push_back(fBot);
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

void CAnalysisAgentImp::GetLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetMz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetMz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);


   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      IndexType idx = iter - vPoi.begin();

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
      Float64 start_offset = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
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

void CAnalysisAgentImp::GetLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMinTruck,std::vector<VehicleIndexType>* pMaxTruck)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   if (bat==MinSimpleContinuousEnvelope)
   {
      bat=MaxSimpleContinuousEnvelope;
   }
   else if (bat==MaxSimpleContinuousEnvelope)
   {
      bat=MinSimpleContinuousEnvelope;
   }

   CComPtr<ILiveLoadModelSectionResults> minResults;
   CComPtr<ILiveLoadModelSectionResults> maxResults;
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFy, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, 
          roMember, fetFy, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetDy, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetDy, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMaximize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, 
                       fetRz, optMinimize, vlcDefault, vbIncludeImpact, vbIncludeLLDF, VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex,std::vector<VehicleIndexType>* pTopMaxIndex,std::vector<VehicleIndexType>* pBotMinIndex,std::vector<VehicleIndexType>* pBotMaxIndex)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);
   pModelData->pLiveLoadResponse[bat]->ComputeStresses( m_LBAMPoi, bstrStage, llmt, 
          fetMz, optMaximize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&maxResults);
   pModelData->pLiveLoadResponse[bat]->ComputeStresses( m_LBAMPoi, bstrStage, llmt, 
          fetMz, optMinimize, vlcDefault, vbIncludeImpact,vbIncludeLLDF,VARIANT_TRUE,&minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      IndexType idx = iter - vPoi.begin();

      CComPtr<IStressResult> fLeftMax, fRightMax;
      CComPtr<ILiveLoadConfiguration> fLeftMaxConfig, fRightMaxConfig;
      maxResults->GetResult(idx,&fLeftMax,pTopMaxIndex || pBotMaxIndex ? &fLeftMaxConfig : NULL,&fRightMax,pTopMaxIndex || pBotMaxIndex ? &fRightMaxConfig : NULL);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      CComPtr<ILiveLoadConfiguration> fLeftMinConfig, fRightMinConfig;
      minResults->GetResult(idx,&fLeftMin,pTopMinIndex || pBotMinIndex ? &fLeftMinConfig : NULL,&fRightMin,pTopMinIndex || pBotMinIndex ? &fRightMinConfig : NULL);

      Float64 dist_from_start = poi.GetDistFromStart();
      GET_IFACE(IBridge,pBridge);
      Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
      Float64 fBotMax, fBotMin, fTopMax, fTopMin;

      if ( IsZero(dist_from_start - start_offset) )
      {
         fRightMax->GetResult(0,&fBotMax);
         fRightMax->GetResult(1,&fTopMin);
         fRightMin->GetResult(0,&fBotMin);
         fRightMin->GetResult(1,&fTopMax);

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
         fLeftMax->GetResult(0,&fBotMax);
         fLeftMax->GetResult(1,&fTopMin);
         fLeftMin->GetResult(0,&fBotMin);
         fLeftMin->GetResult(1,&fTopMax);

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

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetMz, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetMz, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);


   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      IndexType idx = iter - vPoi.begin();

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
      Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
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

void CAnalysisAgentImp::GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<AxleConfiguration>* pMinLeftAxleConfig,std::vector<AxleConfiguration>* pMinRightAxleConfig,std::vector<AxleConfiguration>* pMaxLeftAxleConfig,std::vector<AxleConfiguration>* pMaxRightAxleConfig)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetFy, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeForces( m_LBAMPoi, bstrStage, llmt, vehicleIndex, roMember, fetFy, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop,std::vector<AxleConfiguration>* pMaxAxleConfigTop,std::vector<AxleConfiguration>* pMinAxleConfigBot,std::vector<AxleConfiguration>* pMaxAxleConfigBot)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);


   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeStresses(m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetMz, optMinimize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeStresses(m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetMz, optMaximize, vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      IndexType idx = iter - vPoi.begin();

      CComPtr<IStressResult> fLeftMax, fRightMax;
      CComPtr<ILiveLoadConfiguration> maxLeftConfig, maxRightConfig;
      maxResults->GetResult(idx,&fLeftMax,pMaxAxleConfigTop || pMaxAxleConfigBot ? &maxLeftConfig : NULL,&fRightMax,pMaxAxleConfigTop || pMaxAxleConfigBot ? &maxRightConfig : NULL);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      CComPtr<ILiveLoadConfiguration> minLeftConfig, minRightConfig;
      minResults->GetResult(idx,&fLeftMin,pMinAxleConfigTop || pMinAxleConfigBot ? &minLeftConfig : NULL,&fRightMin,pMinAxleConfigTop || pMinAxleConfigBot ? &minRightConfig : NULL);

      Float64 dist_from_start = poi.GetDistFromStart();
      GET_IFACE(IBridge,pBridge);
      Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
      Float64 fBotMax, fBotMin, fTopMax, fTopMin;
      if ( IsZero(dist_from_start - start_offset) )
      {
         fRightMax->GetResult(0,&fBotMax);
         fRightMax->GetResult(1,&fTopMin);
         fRightMin->GetResult(0,&fBotMin);
         fRightMin->GetResult(1,&fTopMax);

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
         fLeftMax->GetResult(0,&fBotMax);
         fLeftMax->GetResult(1,&fTopMin);
         fLeftMin->GetResult(0,&fBotMin);
         fLeftMin->GetResult(1,&fTopMax);


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

void CAnalysisAgentImp::GetVehicularLiveLoadDisplacement(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetDy, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetDy, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);
   
   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

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
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   DistributionFactorType dfType = (bIncludeLLDF   ? GetLiveLoadDistributionFactorType(llType) : dftNone);

   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetRz, optMinimize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &minResults);
   pModelData->pVehicularResponse[bat]->ComputeDeflections( m_LBAMPoi, bstrStage, llmt, vehicleIndex, fetRz, optMaximize,vlcDefault, vbIncludeImpact, dfType, VARIANT_TRUE, &maxResults);

   CComPtr<ILBAMModel> lbam_model;
   GetModel(pModelData,bat,&lbam_model);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

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
sysSectionValue CAnalysisAgentImp::GetShear(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> V;
   V = GetShear(combo,stage,vPoi,type,bat);

   ATLASSERT(V.size() == 1);

   return V[0];
}

Float64 CAnalysisAgentImp::GetMoment(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetMoment(combo,stage,vPoi,type,bat);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDisplacement(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetDisplacement(combo,stage,vPoi,type,bat);
   ATLASSERT(results.size() == 1);
   return results[0];
}


Float64 CAnalysisAgentImp::GetReaction(LoadingCombination combo,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,CombinationType type,BridgeAnalysisType bat)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         SpanIndexType span = SpanIndexType(pier == 0 ? 0 : pier-1);
         ValidateAnalysisModels(span, gdr);

         return GetReactions( m_CastingYardModels, pier, gdr );
      }
      else
      {
         // Start by checking if the model exists
         ModelData* pModelData = 0;
         pModelData = GetModelData(gdr);

         CComBSTR bstrLoadCase = GetLoadCaseName(combo);
         CComBSTR bstrStage    = pStageMap->GetStageName(stage);;

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         m_LBAMPoi->Clear();
         m_LBAMPoi->Add(pier);

         CComPtr<IResult3Ds> results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeReactions(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeReactions(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeReactions(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);

         CComPtr<IResult3D> result;
         results->get_Item(0,&result);

         Float64 Fy;
         result->get_Y(&Fy);

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

void CAnalysisAgentImp::GetStress(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(combo,stage,vPoi,type,bat,&fTop,&fBot);
   
   ATLASSERT(fTop.size() == 1 && fBot.size() == 1);

   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetCombinedLiveLoadMoment(llType,stage,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1 && Mmax.size() == 1);
   *pMin = Mmin[0];
   *pMax = Mmax[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetCombinedLiveLoadShear(llType,stage,vPoi,bat,bIncludeImpact,&Vmin,&Vmax);

   ATLASSERT( Vmin.size() == 1 && Vmax.size() == 1 );
   *pVmin = Vmin[0];
   *pVmax = Vmax[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetCombinedLiveLoadDisplacement(llType,stage,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT( Dmin.size() == 1 && Dmax.size() == 1 );
   *pDmin = Dmin[0];
   *pDmax = Dmax[0];
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   ATLASSERT(stage == pgsTypes::BridgeSite3);

   // Start by checking if the model exists
   ModelData* pModelData = 0;
   pModelData = GetModelData(gdr);

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

   m_LBAMPoi->Clear();
   m_LBAMPoi->Add(pier);

   CComBSTR bstrLoadCombo = GetLoadCombinationName(llType);
   CComBSTR bstrStage     = pStageMap->GetStageName(stage);

   CComPtr<ILoadCombinationResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, vbIncludeImpact, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, vbIncludeImpact, VARIANT_FALSE, &minResults);

   Float64 FyMax;
   maxResults->GetResult(0,&FyMax,NULL);

   Float64 FyMin;
   minResults->GetResult(0,&FyMin,NULL);

   *pRmin = FyMin;
   *pRmax = FyMax;
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   GetCombinedLiveLoadStress(llType,stage,vPoi,bat,&fTopMin,&fTopMax,&fBotMin,&fBotMax);

   ATLASSERT( fTopMin.size() == 1 && fTopMax.size() == 1 && fBotMin.size() == 1 && fBotMax.size() == 1 );

   *pfTopMin = fTopMin[0];
   *pfTopMax = fTopMax[0];
   *pfBotMin = fBotMin[0];
   *pfBotMax = fBotMax[0];
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces2
//
std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<sysSectionValue> results;

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels(poi.GetSpan(),poi.GetGirder());
            
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            results.push_back(fy);
         }
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase  = GetLoadCaseName(combo);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);

         std::vector<sysSectionValue> section_results;
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();

            CComPtr<ISectionResult3D> result;
            results->get_Item(idx,&result);

            Float64 FyLeft, FyRight;
            result->get_YLeft(&FyLeft);
            result->get_YRight(&FyRight);

            section_results.push_back(sysSectionValue(-FyLeft,FyRight));
         }

         return section_results;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         std::vector<Float64> results;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            ValidateAnalysisModels( poi.GetSpan(), poi.GetGirder() );
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            ATLASSERT(IsEqual(mz.Left(),-mz.Right()));
            results.push_back(mz.Left());
         }
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase  = GetLoadCaseName(combo);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         GET_IFACE(IBridge,pBridge);

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeForces(bstrLoadCase,m_LBAMPoi,bstrStage,roMember,rsType,&results);

         std::vector<Float64> Mz;
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();

            const pgsPointOfInterest& poi = *iter;

            Float64 start_offset = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
            Float64 product_location = poi.GetDistFromStart() - start_offset;

            CComPtr<ISectionResult3D> result;
            results->get_Item(idx,&result);

            Float64 MzLeft, MzRight;
            result->get_ZLeft(&MzLeft);
            result->get_ZRight(&MzRight);

            if ( IsZero(product_location) )
               Mz.push_back(-MzRight); // use right side result at start of span
            else
               Mz.push_back(MzLeft); // use left side result at all other locations

         }
         return Mz;
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

std::vector<Float64> CAnalysisAgentImp::GetDisplacement(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         std::vector<Float64> results;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            ValidateAnalysisModels(poi.GetSpan(),poi.GetGirder());
            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );
            results.push_back(dy);
         }
         return results;
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase  = GetLoadCaseName(combo);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionResult3Ds> results;
         if ( bat == MinSimpleContinuousEnvelope )
            pModelData->pMinLoadCaseResponseEnvelope[fetFy][optMinimize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
         else if ( bat == MaxSimpleContinuousEnvelope )
            pModelData->pMaxLoadCaseResponseEnvelope[fetFy][optMaximize]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);
         else
            pModelData->pLoadCaseResponse[bat]->ComputeDeflections(bstrLoadCase,m_LBAMPoi,bstrStage,rsType,&results);

         std::vector<Float64> vResults;
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();
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

void CAnalysisAgentImp::GetStress(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pfTop->clear();
   pfBot->clear();

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels( poi.GetSpan(), poi.GetGirder() );

            Float64 top_results;
            Float64 bot_results;
            top_results = GetSectionStress( m_CastingYardModels, pgsTypes::TopGirder,    poi );
            bot_results = GetSectionStress( m_CastingYardModels, pgsTypes::BottomGirder, poi );
            pfTop->push_back(top_results);
            pfBot->push_back(bot_results);
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCase = GetLoadCaseName(combo);
         CComBSTR bstrStage    = pStageMap->GetStageName(stage);

         ResultsSummationType rsType = (type == ctIncremental ? rsIncremental : rsCumulative);

         CComPtr<ISectionStressResults> min_results, max_results, results;
         if ( bat == MinSimpleContinuousEnvelope )
         {
            pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &max_results);
            pModelData->pMinLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &min_results);
         }
         else if ( bat == MaxSimpleContinuousEnvelope )
         {
            pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMaximize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &max_results);
            pModelData->pMaxLoadCaseResponseEnvelope[fetMz][optMinimize]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &min_results);
         }
         else
         {
            pModelData->pLoadCaseResponse[bat]->ComputeStresses(bstrLoadCase, m_LBAMPoi, bstrStage, rsType, &results);
         }
      
         long stress_point_index_top = GetStressPointIndex(pgsTypes::TopGirder);
         long stress_point_index_bot = GetStressPointIndex(pgsTypes::BottomGirder);

         GET_IFACE(IBridge,pBridge);
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            IndexType idx = iter - vPoi.begin();

            SpanIndexType span  = poi.GetSpan();
            GirderIndexType gdr = poi.GetGirder();
            Float64 dist_from_start = poi.GetDistFromStart();

            Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

            Float64 fTop, fBot;

            if ( bat == MinSimpleContinuousEnvelope )
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
            else if ( bat == MaxSimpleContinuousEnvelope )
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

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax)
{
   ATLASSERT(stage == pgsTypes::BridgeSite3);

   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pMmax->clear();
   pMmin->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);


   CComBSTR bstrLoadCombo = GetLoadCombinationName(llType);
   CComBSTR bstrStage     = pStageMap->GetStageName(stage);

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      IndexType idx = iter - vPoi.begin();

      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();
      Float64 dist_from_start = poi.GetDistFromStart();
      Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

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

void CAnalysisAgentImp::GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax)
{
   ATLASSERT(stage == pgsTypes::BridgeSite3);

   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pVmax->clear();
   pVmin->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo = GetLoadCombinationName(llType);
   CComBSTR bstrStage     = pStageMap->GetStageName(stage);
   VARIANT_BOOL incImpact = bIncludeImpact ? VARIANT_TRUE: VARIANT_FALSE;

   // TRICKY:
   // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
   // that the optimization must go the opposite way as well when using the envelopers
   if (bat==MinSimpleContinuousEnvelope)
   {
      bat=MaxSimpleContinuousEnvelope;
   }
   else if (bat==MaxSimpleContinuousEnvelope)
   {
      bat=MinSimpleContinuousEnvelope;
   }

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, incImpact, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, incImpact, VARIANT_FALSE, &minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

      Float64 FyMaxLeft, FyMaxRight;
      maxResults->GetResult(idx,&FyMaxLeft,NULL,&FyMaxRight,NULL);

      Float64 FyMinLeft, FyMinRight;
      minResults->GetResult(idx,&FyMinLeft,NULL,&FyMinRight,NULL);

      pVmin->push_back( sysSectionValue(-FyMaxLeft,FyMaxRight) );
      pVmax->push_back( sysSectionValue(-FyMinLeft,FyMinRight) );
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax)
{
   ATLASSERT(stage == pgsTypes::BridgeSite3);

   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pDmax->clear();
   pDmin->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo = GetLoadCombinationName(llType);
   CComBSTR bstrStage     = pStageMap->GetStageName(stage);

   CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      IndexType idx = iter - vPoi.begin();

      Float64 DyMaxLeft, DyMaxRight;
      maxResults->GetResult(idx,&DyMaxLeft,NULL,&DyMaxRight,NULL);

      Float64 DyMinLeft, DyMinRight;
      minResults->GetResult(idx,&DyMinLeft,NULL,&DyMinRight,NULL);

      pDmin->push_back( DyMinLeft );
      pDmax->push_back( DyMaxLeft );
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax)
{
   ATLASSERT(stage == pgsTypes::BridgeSite3);

   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pfTopMin->clear();
   pfTopMax->clear();
   pfBotMin->clear();
   pfBotMax->clear();

   ModelData* pModelData = UpdateLBAMPois(vPoi);

   CComBSTR bstrLoadCombo = GetLoadCombinationName(llType);
   CComBSTR bstrStage     = pStageMap->GetStageName(stage);

   CComPtr<ILoadCombinationStressResults> maxResults, minResults;
   pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
   pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, &minResults);

   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      IndexType idx = iter - vPoi.begin();

      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();
      Float64 dist_from_start = poi.GetDistFromStart();
      Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

      CComPtr<IStressResult> fLeftMax, fRightMax;
      maxResults->GetResult(idx,&fLeftMax,NULL,&fRightMax,NULL);

      CComPtr<IStressResult> fLeftMin, fRightMin;
      minResults->GetResult(idx,&fLeftMin,NULL,&fRightMin,NULL);

      Float64 fBotMin, fBotMax, fTopMin, fTopMax;
      if ( IsZero(dist_from_start - end_size) )
      {
         fRightMax->GetResult(0,&fBotMax);
         fRightMax->GetResult(1,&fTopMin);
         fRightMin->GetResult(0,&fBotMin);
         fRightMin->GetResult(1,&fTopMax);
      }
      else
      {
         fLeftMax->GetResult(0,&fBotMax);
         fLeftMax->GetResult(1,&fTopMin);
         fLeftMin->GetResult(0,&fBotMin);
         fLeftMin->GetResult(1,&fTopMax);
      }

      pfBotMax->push_back(fBotMax);
      pfBotMin->push_back(fBotMin);
      pfTopMax->push_back(fTopMax);
      pfTopMin->push_back(fTopMin);
   }
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces
//
void CAnalysisAgentImp::GetShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetShear(ls,stage,vPoi,bat,&Vmin,&Vmax);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pMin = Vmin[0];
   *pMax = Vmax[0];
}

void CAnalysisAgentImp::GetMoment(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetMoment(ls,stage,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMin = Mmin[0];
   *pMax = Mmax[0];
}

void CAnalysisAgentImp::GetDisplacement(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetDisplacement(ls,stage,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin[0];
   *pMax = Dmax[0];
}

void CAnalysisAgentImp::GetStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludePrestress,BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Fmin, Fmax;
   GetStress(ls,stage,vPoi,loc,bIncludePrestress,bat,&Fmin,&Fmax);

   ATLASSERT(Fmin.size() == 1);
   ATLASSERT(Fmax.size() == 1);

   *pMin = Fmin[0];
   *pMax = Fmax[0];
}

Float64 CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState ls,const pgsPointOfInterest& poi,BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> M = GetSlabDesignMoment(ls,vPoi,bat);

   ATLASSERT(M.size() == vPoi.size());

   return M.front();
}

bool CAnalysisAgentImp::IsStrengthIIApplicable(SpanIndexType span, GirderIndexType girder)
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
         return HasPedestrianLoad(span, girder);
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces2
//
void CAnalysisAgentImp::GetMoment(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pMin->clear();
   pMax->clear();

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels( poi.GetSpan(), poi.GetGirder() );

            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            ATLASSERT(IsEqual(mz.Left(),-mz.Right()));

            pMin->push_back( mz.Left() );
            pMax->push_back( mz.Left() );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         VARIANT_BOOL bIncludeLiveLoad = (stage == pgsTypes::BridgeSite3 ? VARIANT_TRUE : VARIANT_FALSE );
         CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);


         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            IndexType idx = iter - vPoi.begin();

            Float64 MzMaxLeft, MzMaxRight;
            maxResults->GetResult(idx,&MzMaxLeft,NULL,&MzMaxRight,NULL);
      
            Float64 MzMinLeft, MzMinRight;
            minResults->GetResult(idx,&MzMinLeft,NULL,&MzMinRight,NULL);

            GET_IFACE(IBridge,pBridge);
            Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
            Float64 product_location = poi.GetDistFromStart() - start_offset;

            Float64 MzMin, MzMax;
            if ( IsZero(product_location) )
            {
               MzMin = -MzMinRight; // use right side result at start of span
               MzMax = -MzMaxRight; // use right side result at start of span
            }
            else
            {
               MzMin = MzMinLeft; // use left side result at all other locations
               MzMax = MzMaxLeft; // use left side result at all other locations
            }

            pMin->push_back( MzMin );
            pMax->push_back( MzMax );

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

void CAnalysisAgentImp::GetShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pMin->clear();
   pMax->clear();

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            ValidateAnalysisModels( poi.GetSpan(), poi.GetGirder() );

            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            pMin->push_back( fy );
            pMax->push_back( fy );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         // TRICKY:
         // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
         // that the optimization must go the opposite way as well when using the envelopers
         if (bat==MinSimpleContinuousEnvelope)
         {
            bat=MaxSimpleContinuousEnvelope;
         }
         else if (bat==MaxSimpleContinuousEnvelope)
         {
            bat=MinSimpleContinuousEnvelope;
         }

         VARIANT_BOOL bIncludeLiveLoad = (stage == pgsTypes::BridgeSite3 ? VARIANT_TRUE : VARIANT_FALSE );
         CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            IndexType idx = iter - vPoi.begin();

            Float64 FyMaxLeft, FyMaxRight;
            maxResults->GetResult(idx,&FyMaxLeft,NULL,&FyMaxRight,NULL);
      
            Float64 FyMinLeft, FyMinRight;
            minResults->GetResult(idx,&FyMinLeft,NULL,&FyMinRight,NULL);

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

std::vector<Float64> CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   std::vector<Float64> vMoment;

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bExcludeNoncompositeMoments = !pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   pgsTypes::Stage stage = pgsTypes::BridgeSite3;

   try
   {
      ModelData* pModelData = UpdateLBAMPois(vPoi);

      CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
      CComBSTR bstrStage     = pStageMap->GetStageName(stage);

      VARIANT_BOOL bIncludeLiveLoad = VARIANT_TRUE;
      CComPtr<ILoadCombinationSectionResults> minResults;
      pModelData->pLoadComboResponse[bat]->ComputeForces(bstrLoadCombo, m_LBAMPoi, bstrStage, roMember, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_TRUE,  &minResults);

      // do this outside the loop since the values don't change
      Float64 gDCmin, gDCmax;
      if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
      {
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
      }


      std::vector<pgsPointOfInterest>::const_iterator iter;
      for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
      {
         const pgsPointOfInterest& poi = *iter;

         IndexType idx = iter - vPoi.begin();

         Float64 MzMinLeft, MzMinRight;
         CComPtr<ILoadCombinationResultConfiguration> leftConfig,rightConfig;
         minResults->GetResult(idx,&MzMinLeft,&leftConfig,&MzMinRight,&rightConfig);

         CComPtr<ILoadCombinationResultConfiguration> min_result_config;

         GET_IFACE(IBridge,pBridge);
         Float64 start_offset = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
         Float64 product_location = poi.GetDistFromStart() - start_offset;

         Float64 MzMin;
         if ( IsZero(product_location) )
         {
            MzMin = -MzMinRight; // use right side result at start of span
            min_result_config = rightConfig;
         }
         else
         {
            MzMin = MzMinLeft; // use left side result at all other locations
            min_result_config = leftConfig;
         }

         if ( (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3) && bExcludeNoncompositeMoments )
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
            pgsTypes::Stage girderLoadStage = GetGirderDeadLoadStage(poi.GetSpan(),poi.GetGirder());
            Float64 Mg = GetMoment(girderLoadStage,pftGirder,poi,bat);
            MzMin -= gDC*Mg;

            // if not continuous in bridge site stage 1, 
            // remove slab, slab panels, diaphragms, and shear key moment
            PierIndexType prevPierIdx = (PierIndexType)poi.GetSpan();
            PierIndexType nextPierIdx = prevPierIdx + 1;
            pgsTypes::Stage start,end,dummy;
            pBridge->GetContinuityStage(prevPierIdx,&dummy,&start);
            pBridge->GetContinuityStage(nextPierIdx,&end,&dummy);

            if ( start == pgsTypes::BridgeSite2 && end == pgsTypes::BridgeSite2 )
            {
               Float64 Mconstruction = GetMoment(pgsTypes::BridgeSite1, pftConstruction,      poi, bat);
               Float64 Mslab         = GetMoment(pgsTypes::BridgeSite1, pftSlab,              poi, bat);
               Float64 Mslab_panel   = GetMoment(pgsTypes::BridgeSite1, pftSlabPanel,         poi, bat);
               Float64 Mdiaphragm    = GetMoment(pgsTypes::BridgeSite1, pftDiaphragm,         poi, bat);
               Float64 Mshear_key    = GetMoment(pgsTypes::BridgeSite1, pftShearKey,          poi, bat);

               MzMin -= gDC*(Mconstruction + Mslab + Mslab_panel + Mdiaphragm + Mshear_key);
            }

            // remove user dc moments
            Float64 Muser_dc = GetMoment(pgsTypes::BridgeSite1, pftUserDC, poi, bat);
            MzMin -= gDC*Muser_dc;
         }

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

void CAnalysisAgentImp::GetDisplacement(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   GET_IFACE(IStageMap,pStageMap);

   pMin->clear();
   pMax->clear();

   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            ValidateAnalysisModels(poi.GetSpan(),poi.GetGirder());

            sysSectionValue fx,fy,mz;
            Float64 dx,dy,rz;
            GetSectionResults( m_CastingYardModels, poi, &fx, &fy, &mz, &dx, &dy, &rz );

            pMin->push_back( dy );
            pMax->push_back( dy );
         }
      }
      else
      {
         ModelData* pModelData = UpdateLBAMPois(vPoi);

         CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         VARIANT_BOOL bIncludeLiveLoad = (stage == pgsTypes::BridgeSite3 ? VARIANT_TRUE : VARIANT_FALSE );
         CComPtr<ILoadCombinationSectionResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeDeflections(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            IndexType idx = iter - vPoi.begin();

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

void CAnalysisAgentImp::GetStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc,bool bIncludePrestress,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;
   GET_IFACE(IStageMap,pStageMap);

   pMin->clear();
   pMax->clear();

   try
   {
      ModelData* pModelData = 0;

      GET_IFACE(IBridge,pBridge);

      CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
      CComBSTR bstrStage     = pStageMap->GetStageName(stage);

      VARIANT_BOOL bIncludeLiveLoad = (stage == pgsTypes::BridgeSite3 ? VARIANT_TRUE : VARIANT_FALSE );

      CComPtr<ILoadCombinationStressResults> maxResults, minResults;
      if (stage != pgsTypes::CastingYard )
      {
         pModelData = UpdateLBAMPois(vPoi);

         pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMaximize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeStresses(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetMz, optMinimize, bIncludeLiveLoad, VARIANT_TRUE, VARIANT_FALSE, &minResults);
      }

      std::vector<pgsPointOfInterest>::const_iterator iter;
      for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
      {
         Float64 fMax, fMin;
         const pgsPointOfInterest& poi = *iter;

         SpanIndexType span  = poi.GetSpan();
         GirderIndexType gdr = poi.GetGirder();
         Float64 dist_from_start = poi.GetDistFromStart();
         Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

         if ( stage == pgsTypes::CastingYard )
         {
            ValidateAnalysisModels(span,gdr);

            fMin = GetSectionStress( m_CastingYardModels, loc, poi );
            fMax = fMin;
         }
         else
         {
            IndexType idx = iter - vPoi.begin();

            CComPtr<IStressResult> fLeftMax, fRightMax;
            maxResults->GetResult(idx,&fLeftMax,NULL,&fRightMax,NULL);
   
            CComPtr<IStressResult> fLeftMin, fRightMin;
            minResults->GetResult(idx,&fLeftMin,NULL,&fRightMin,NULL);

            IndexType stress_point_index = GetStressPointIndex(loc);

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
            Float64 ps = GetStress(stage,poi,loc);

            Float64 k;
            if (ls == pgsTypes::ServiceIA || ls == pgsTypes::FatigueI )
               k = 0.5; // Use half prestress stress if service IA (See Tbl 5.9.4.2.1-1 2008 or before) or Fatige I (LRFD 5.5.3.1-2009)
            else
               k = 1.0;

            fMin += k*ps;
            fMax += k*ps;
         }

         // if this is bridge site stage 3, add effect of deck shrinkage
         if ( stage == pgsTypes::BridgeSite3 )
         {
            Float64 ft_ss, fb_ss;
            GetDeckShrinkageStresses(poi,&ft_ss,&fb_ss);

            if ( loc == pgsTypes::TopGirder )
            {
               fMin += ft_ss;
               fMax += ft_ss;
            }
            else if ( loc == pgsTypes::BottomGirder )
            {
               fMin += fb_ss;
               fMax += fb_ss;
            }
         }

         pMax->push_back(fMax);
         pMin->push_back(fMin);
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
{
   try
   {
      if ( stage == pgsTypes::CastingYard )
      {
         SpanIndexType span = SpanIndexType(pier == 0 ? 0 : pier-1);
         ValidateAnalysisModels(span, gdr);

         *pMin = GetReactions( m_CastingYardModels, pier, gdr );
         *pMax = *pMin;
      }
      else
      {
         // Start by checking if the model exists
         ModelData* pModelData = 0;
         pModelData = GetModelData(gdr);

         m_LBAMPoi->Clear();
         m_LBAMPoi->Add(pier);

         GET_IFACE(IStageMap,pStageMap);
         CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
         CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);

         VARIANT_BOOL bIncludeLiveLoad = (stage == pgsTypes::BridgeSite3 ? VARIANT_TRUE : VARIANT_FALSE );
         CComPtr<ILoadCombinationResults> maxResults, minResults;
         pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMaximize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &maxResults);
         pModelData->pLoadComboResponse[bat]->ComputeReactions(bstrLoadCombo, m_LBAMPoi, bstrStage, rsCumulative, fetFy, optMinimize, bIncludeLiveLoad, vbIncludeImpact, VARIANT_FALSE, &minResults);

         Float64 FyMax;
         maxResults->GetResult(0,&FyMax,NULL);
      
         Float64 FyMin;
         minResults->GetResult(0,&FyMin,NULL);

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

void CAnalysisAgentImp::GetDesignStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   if ( stage == pgsTypes::CastingYard )
   {
      GetStress(ls,stage,poi,loc,false,bat,pMin,pMax);
      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // this method is only good for service limit states
   ATLASSERT( ls == pgsTypes::ServiceI || ls == pgsTypes::ServiceIA || ls == pgsTypes::ServiceIII || ls == pgsTypes::FatigueI );

   GET_IFACE(ISectProp2,pSectProp2);

   Float64 Stc = pSectProp2->GetStGirder(pgsTypes::BridgeSite2,poi);
   Float64 Sbc = pSectProp2->GetSb(pgsTypes::BridgeSite2,poi);

   Float64 Stop = pSectProp2->GetStGirder(pgsTypes::BridgeSite2,poi,fcgdr);
   Float64 Sbot = pSectProp2->GetSb(pgsTypes::BridgeSite2,poi,fcgdr);

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

   // figure out which stage the girder loading is applied
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   pgsTypes::Stage girderLoadStage = GetGirderDeadLoadStage(span,gdr);

   // Temporary Strand Removal
   GetStress(girderLoadStage,pftGirder,poi,bat,&ft,&fb);
   ftop1 = dc*ft;   fbot1 = dc*fb;

   if ( stage == pgsTypes::TemporaryStrandRemoval )
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

   // Bridge Site Stage 1
   GetStress(pgsTypes::BridgeSite1,pftConstruction,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftSlab,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftSlabPad,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetDesignSlabPadStressAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftDiaphragm,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftShearKey,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftUserDC,poi,bat,&ft,&fb);
   ftop1 += dc*ft;   fbot1 += dc*fb;

   GetStress(pgsTypes::BridgeSite1,pftUserDW,poi,bat,&ft,&fb);
   ftop1 += dw*ft;   fbot1 += dw*fb;

   if ( stage == pgsTypes::BridgeSite1 )
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
   GetStress(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat,&ft,&fb);
   ftop2 = dc*k_top*ft;   fbot2 = dc*k_bot*fb;

   GetStress(pgsTypes::BridgeSite2,pftSidewalk,poi,bat,&ft,&fb);
   ftop2 += dc*k_top*ft;   fbot2 += dc*k_bot*fb;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   if (overlay_stage==pgsTypes::BridgeSite2)
   {
      GetStress(pgsTypes::BridgeSite2,pftOverlay,poi,bat,&ft,&fb);
      ftop2 += dw*k_top*ft;   fbot2 += dw*k_bot*fb;
   }

   GetStress(pgsTypes::BridgeSite2,pftUserDC,poi,bat,&ft,&fb);
   ftop2 += dc*k_top*ft;   fbot2 += dc*k_bot*fb;

   GetStress(pgsTypes::BridgeSite2,pftUserDW,poi,bat,&ft,&fb);
   ftop2 += dw*k_top*ft;   fbot2 += dw*k_bot*fb;

   if ( stage == pgsTypes::BridgeSite2 )
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
   if (overlay_stage==pgsTypes::BridgeSite3)
   {
      GetStress(pgsTypes::BridgeSite3,pftOverlay,poi,bat,&ft,&fb);
      ftop3Min = ftop3Max = dw*k_top*ft;   
      fbot3Min = fbot3Max = dw*k_bot*fb;   
   }
   else
   {
      ftop3Min = ftop3Max = 0.0;   
      fbot3Min = fbot3Max = 0.0;   
   }

   GET_IFACE(IGirderData,pGdrData);
   const CGirderMaterial* pGirderMaterial = pGdrData->GetGirderMaterial(poi.GetSpan(),poi.GetGirder());

   Float64 fc_lldf = fcgdr;
   if ( pGirderMaterial->bUserEc )
      fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Ec, pGirderMaterial->StrengthDensity );


   Float64 ftMin,ftMax,fbMin,fbMax;
   if ( ls == pgsTypes::FatigueI )
      GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3,poi,bat,true,true,&ftMin,&ftMax,&fbMin,&fbMax);
   else
      GetLiveLoadStress(pgsTypes::lltDesign,  pgsTypes::BridgeSite3,poi,bat,true,true,&ftMin,&ftMax,&fbMin,&fbMax);

   ftop3Min += ll*k_top*ftMin;   fbot3Min += ll*k_bot*fbMin;
   ftop3Max += ll*k_top*ftMax;   fbot3Max += ll*k_bot*fbMax;

   GetLiveLoadStress(pgsTypes::lltPedestrian,  pgsTypes::BridgeSite3,poi,bat,true,true,&ftMin,&ftMax,&fbMin,&fbMax);

   ftop3Min += ll*k_top*ftMin;   fbot3Min += ll*k_bot*fbMin;
   ftop3Max += ll*k_top*ftMax;   fbot3Max += ll*k_bot*fbMax;

   GetStress(pgsTypes::BridgeSite3,pftUserLLIM,poi,bat,&ft,&fb);
   ftop3Min += ll*k_top*ft;   fbot3Min += ll*k_bot*fb;
   ftop3Max += ll*k_top*ft;   fbot3Max += ll*k_bot*fb;

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
      std::swap(*pMin,*pMax);

   *pMax = (IsZero(*pMax) ? 0 : *pMax);
   *pMin = (IsZero(*pMin) ? 0 : *pMin);
}

std::vector<Float64> CAnalysisAgentImp::GetStress(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc)
{
   std::vector<Float64> stresses;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      Float64 stress = GetStress(stage,poi,loc);
      stresses.push_back(stress);
   }

   return stresses;
}

void CAnalysisAgentImp::GetConcurrentShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   ATLASSERT(stage != pgsTypes::CastingYard); // this method only works for bridge site stages
   try
   {
      // Start by checking if the model exists
      ModelData* pModelData = 0;
      pModelData = GetModelData(poi.GetGirder());

      PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
      if ( poi_id == INVALID_ID )
      {
         poi_id = AddPointOfInterest( pModelData, poi );
         ATLASSERT( 0 <= poi_id && poi_id != INVALID_ID ); // if this fires, the poi wasn't added... WHY???
      }

      m_LBAMPoi->Clear();
      m_LBAMPoi->Add(poi_id);

      GET_IFACE(IStageMap,pStageMap);
      CComBSTR bstrLoadCombo = GetLoadCombinationName(ls);
      CComBSTR bstrStage     = pStageMap->GetStageName(stage);

         CComPtr<ILoadCombinations> loadCombos;
         pModelData->m_Model->get_LoadCombinations(&loadCombos) ;
         CComPtr<ILoadCombination> loadCombo;
         loadCombos->Find(bstrLoadCombo,&loadCombo);
         Float64 gLLmin, gLLmax;
         loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
//         ATLASSERT( 0 < gLLmin && 0 < gLLmax); // if this assert fires, we have to do the load combination "manually"
                                               // because the load factor is a function of POI and axle weights
                                               // this can only happen for permit limit states
         if ( gLLmin < 0 || gLLmax < 0 )
         {
            ATLASSERT( ::IsRatingLimitState(ls) ); // this can only happen for ratings
         }

      // Get the Max/Min moments
      VARIANT_BOOL bIncludeLiveLoad = (stage == pgsTypes::BridgeSite3 ? VARIANT_TRUE : VARIANT_FALSE );
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
      if ( bat == SimpleSpan || bat == ContinuousSpan )
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

void CAnalysisAgentImp::GetViMmax(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pVi,Float64* pMmax)
{
   GET_IFACE(ISpecification,pSpec);

   Float64 Mu_max, Mu_min;
   sysSectionValue Vi_min, Vi_max;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 Mmin,Mmax;
      sysSectionValue Vimin, Vimax;

      GetMoment( ls, stage, poi, MaxSimpleContinuousEnvelope, &Mmin, &Mmax );
      GetConcurrentShear(  ls, stage, poi, MaxSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_max = Mmax;
      Vi_max = Vimax;

      GetMoment( ls, stage, poi, MinSimpleContinuousEnvelope, &Mmin, &Mmax );
      GetConcurrentShear(  ls, stage, poi, MinSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_min = Mmin;
      Vi_min = Vimin;
   }
   else
   {
      BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
      GetMoment( ls, stage, poi, bat, &Mu_min, &Mu_max );
      GetConcurrentShear(  ls, stage, poi, bat, &Vi_min, &Vi_max );
   }

   Mu_max = IsZero(Mu_max) ? 0 : Mu_max;
   Mu_min = IsZero(Mu_min) ? 0 : Mu_min;

   // driving moment is the one with the greater magnitude
   Float64 Mu = max(fabs(Mu_max),fabs(Mu_min));

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
      *pVi = min(fabs(Vi_max.Left()),fabs(Vi_max.Right()));
   }
   else
   {
      // magnutude of minimum moment is greatest
      // use least of left/right Vi_min
      *pVi = min(fabs(Vi_min.Left()),fabs(Vi_min.Right()));
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

Float64 CAnalysisAgentImp::GetCreepCoefficient(SpanIndexType span,GirderIndexType gdr, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(span,gdr, creepPeriod, constructionRate);
   return details.Ct;
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(span,gdr,config,creepPeriod,constructionRate);
   return details.Ct;
}

CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details;

   GET_IFACE(IEnvironment,pEnvironment);

   GET_IFACE(ISectProp2,pSectProp2);

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
         cc.SetSurfaceArea( pSectProp2->GetSurfaceArea(span,gdr) );
         cc.SetVolume( pSectProp2->GetVolume(span,gdr) );
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
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(span,gdr,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());
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
         cc.SetSurfaceArea( pSectProp2->GetSurfaceArea(span,gdr) );
         cc.SetVolume( pSectProp2->GetVolume(span,gdr) );
         cc.SetFc(fc);

         cc.SetCuringMethodTimeAdjustmentFactor(pSpecEntry->GetCuringMethodTimeAdjustmentFactor());

         GET_IFACE(IBridgeMaterialEx,pMaterial);
         cc.SetK1( pMaterial->GetCreepK1Gdr(span,gdr) );
         cc.SetK2( pMaterial->GetCreepK2Gdr(span,gdr) );


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
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(span,gdr,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }

   return details;
}


CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(SpanIndexType span,GirderIndexType gdr, CreepPeriod creepPeriod, Int16 constructionRate)
{
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   std::map<SpanGirderHashType,CREEPCOEFFICIENTDETAILS>::iterator found = m_CreepCoefficientDetails[constructionRate][creepPeriod].find(key);
   if ( found != m_CreepCoefficientDetails[constructionRate][creepPeriod].end() )
   {
      return (*found).second;
   }

   GET_IFACE(IPointOfInterest, IPoi);
   std::vector<pgsPointOfInterest> pois = IPoi->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
   ATLASSERT(pois.size() == 1);
   pgsPointOfInterest poi = pois[0];

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);
   CREEPCOEFFICIENTDETAILS ccd = GetCreepCoefficientDetails(span,gdr,config,creepPeriod,constructionRate);
   m_CreepCoefficientDetails[constructionRate][creepPeriod].insert(std::make_pair(key,ccd));
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

Float64 CAnalysisAgentImp::GetUserLoadDeflection(pgsTypes::Stage stage, const pgsPointOfInterest& poi) 
{
   Float64 dy,rz;
   GetUserLoadDeflection(stage,poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(pgsTypes::Stage stage, const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetUserLoadDeflection(stage,poi,config,&dy,&rz);
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model         = GetPrestressDeflectionModel(span,gdr,m_PrestressDeflectionModels);
   CamberModelData initTempModel = GetPrestressDeflectionModel(span,gdr,m_InitialTempPrestressDeflectionModels);
   CamberModelData relsTempModel = GetPrestressDeflectionModel(span,gdr,m_ReleaseTempPrestressDeflectionModels);

   Float64 Dy, Rz;
   GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&Dy,&Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData initModel;
   BuildCamberModel(span,gdr,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(span,gdr,true,config,&tempModel1,&tempModel2);
   
   Float64 Dy, Rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&Dy, &Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model         = GetPrestressDeflectionModel(span,gdr,m_PrestressDeflectionModels);
   CamberModelData initTempModel = GetPrestressDeflectionModel(span,gdr,m_InitialTempPrestressDeflectionModels);
   CamberModelData relsTempModel = GetPrestressDeflectionModel(span,gdr,m_ReleaseTempPrestressDeflectionModels);

   Float64 dy,rz;
   GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData initModel;
   BuildCamberModel(span,gdr,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(span,gdr,true,config,&tempModel1,&tempModel2);
   
   Float64 dy,rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);
   return GetSidlDeflection(poi,config);
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // NOTE: No need to validate camber models
   Float64 delta_trafficbarrier  = 0;
   Float64 delta_sidewalk        = 0;
   Float64 delta_overlay         = 0;
   Float64 delta_diaphragm       = 0;
   Float64 delta_user1           = 0;
   Float64 delta_user2           = 0;

   // adjustment factor for fcgdr that is different that current value
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,pgsTypes::BridgeSite2);

   delta_diaphragm      = GetDiaphragmDeflection(poi,config);
   delta_trafficbarrier = k2*GetDisplacement(pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat);
   delta_sidewalk       = k2*GetDisplacement(pgsTypes::BridgeSite2, pftSidewalk,       poi, bat);
   delta_user1          = GetUserLoadDeflection(pgsTypes::BridgeSite1, poi, config);
   delta_user2          = GetUserLoadDeflection(pgsTypes::BridgeSite2, poi, config);

   if ( !pBridge->IsFutureOverlay() )
      delta_overlay = k2*GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);

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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model            = GetPrestressDeflectionModel(span,gdr,m_PrestressDeflectionModels);
   CamberModelData initTempModel    = GetPrestressDeflectionModel(span,gdr,m_InitialTempPrestressDeflectionModels);
   CamberModelData releaseTempModel = GetPrestressDeflectionModel(span,gdr,m_ReleaseTempPrestressDeflectionModels);

   Float64 Dy, Rz;
   GetDCamberForGirderSchedule(poi,model,initTempModel,releaseTempModel,time,&Dy,&Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData initModel;
   BuildCamberModel(span,gdr,true,config,&initModel);

   CamberModelData tempModel1, tempModel2;
   BuildTempCamberModel(span,gdr,true,config,&tempModel1,&tempModel2);

   Float64 Dy, Rz;
   GetDCamberForGirderSchedule(poi,config,initModel,tempModel1, tempModel2, time, &Dy, &Rz);
   return Dy;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GDRCONFIG dummy_config;

   CamberModelData model  = GetPrestressDeflectionModel(span,gdr,m_PrestressDeflectionModels);
   CamberModelData model1 = GetPrestressDeflectionModel(span,gdr,m_InitialTempPrestressDeflectionModels);
   CamberModelData model2 = GetPrestressDeflectionModel(span,gdr,m_ReleaseTempPrestressDeflectionModels);
   GetCreepDeflection(poi,false,dummy_config,model,model1,model2, creepPeriod, constructionRate, pDy, pRz);
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   
   CamberModelData model;
   BuildCamberModel(span,gdr,true,config,&model);

   CamberModelData model1,model2;
   BuildTempCamberModel(span,gdr,true,config,&model1,&model2);
   
   GetCreepDeflection(poi,true,config,model,model1,model2, creepPeriod, constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);
   GetScreedCamber(poi,config,pDy,pRz);
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // NOTE: No need to validate camber models
   Float64 Dslab            = 0;
   Float64 Dslab_pad        = 0;
   Float64 Dtrafficbarrier  = 0;
   Float64 Dsidewalk        = 0;
   Float64 Doverlay         = 0;
   Float64 Ddiaphragm       = 0;
   Float64 Duser1           = 0;
   Float64 Duser2           = 0;

   Float64 Rslab            = 0;
   Float64 Rslab_pad        = 0;
   Float64 Rtrafficbarrier  = 0;
   Float64 Rsidewalk        = 0;
   Float64 Roverlay         = 0;
   Float64 Rdiaphragm       = 0;
   Float64 Ruser1           = 0;
   Float64 Ruser2           = 0;

   // adjustment factor for fcgdr that is different that current value
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,pgsTypes::BridgeSite2);

   GetDeckDeflection(poi,config,&Dslab,&Rslab);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad,&Rslab_pad);
   GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
   
   Dtrafficbarrier = k2*GetDisplacement(pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat);
   Rtrafficbarrier = k2*GetRotation(pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat);

   Dsidewalk = k2*GetDisplacement(pgsTypes::BridgeSite2, pftSidewalk, poi, bat);
   Rsidewalk = k2*GetRotation(pgsTypes::BridgeSite2, pftSidewalk, poi, bat);

   GetUserLoadDeflection(pgsTypes::BridgeSite1, poi, config, &Duser1, &Ruser1);
   GetUserLoadDeflection(pgsTypes::BridgeSite2, poi, config, &Duser2, &Ruser2);

   if ( !pBridge->IsFutureOverlay() )
   {
      Doverlay = k2*GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
      Roverlay = k2*GetRotation(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
   }

   bool bTempStrands = (0 < config.PrestressConfig.GetNStrands(pgsTypes::Temporary) && 
      config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   if ( bTempStrands )
   {
      *pDy = Dslab + Dslab_pad + Dtrafficbarrier + Dsidewalk + Doverlay + Duser1 + Duser2;
      *pRz = Rslab + Rslab_pad + Rtrafficbarrier + Rsidewalk + Roverlay + Ruser1 + Ruser2;
   }
   else
   {
      // for SIP decks, diaphagms are applied before the cast portion of the slab so they don't apply to screed camber
      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         Ddiaphragm = 0;
         Rdiaphragm = 0;
      }

      *pDy = Dslab + Dslab_pad + Dtrafficbarrier + Dsidewalk + Doverlay + Duser1 + Duser2 + Ddiaphragm;
      *pRz = Rslab + Rslab_pad + Rtrafficbarrier + Rsidewalk + Roverlay + Ruser1 + Ruser2 + Rdiaphragm;
   }

   // Switch the sign. Negative deflection creates positive screed camber
   (*pDy) *= -1.0;
   (*pRz) *= -1.0;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   BridgeAnalysisType bat = GetBridgeAnalysisType();

   GET_IFACE(IProductForces,pProductForces);

   Float64 dy_slab = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftSlab,poi,bat);
   Float64 rz_slab = pProductForces->GetRotation(pgsTypes::BridgeSite1,pftSlab,poi,bat);

   Float64 dy_slab_pad = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftSlabPad,poi,bat);
   Float64 rz_slab_pad = pProductForces->GetRotation(pgsTypes::BridgeSite1,pftSlabPad,poi,bat);

   *pDy = dy_slab + dy_slab_pad;
   *pRz = rz_slab + rz_slab_pad;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetDeckDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,pgsTypes::BridgeSite1);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   BridgeAnalysisType bat = GetBridgeAnalysisType();

   GET_IFACE(IProductForces,pProductForces);
   *pDy = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat);
   *pRz = pProductForces->GetRotation(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat);
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetDeckPanelDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,pgsTypes::BridgeSite1);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   BridgeAnalysisType bat = GetBridgeAnalysisType();

   GET_IFACE(IProductForces,pProductForces);
   *pDy = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftDiaphragm,poi,bat);
   *pRz = pProductForces->GetRotation(pgsTypes::BridgeSite1,pftDiaphragm,poi,bat);
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetDiaphragmDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,pgsTypes::BridgeSite1);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetUserLoadDeflection(pgsTypes::Stage stage, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz) 
{
   ATLASSERT(stage==pgsTypes::BridgeSite1 || stage==pgsTypes::BridgeSite2);

   BridgeAnalysisType bat = GetBridgeAnalysisType();

   GET_IFACE(IProductForces,pProductForces);
   Float64 Ddc = pProductForces->GetDisplacement(stage,pftUserDC, poi,bat);
   Float64 Ddw = pProductForces->GetDisplacement(stage,pftUserDW, poi,bat);

   Float64 Rdc = pProductForces->GetRotation(stage,pftUserDC, poi,bat);
   Float64 Rdw = pProductForces->GetRotation(stage,pftUserDW, poi,bat);

   *pDy = Ddc + Ddw;
   *pRz = Rdc + Rdw;
}

void CAnalysisAgentImp::GetUserLoadDeflection(pgsTypes::Stage stage, const pgsPointOfInterest& poi, const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetUserLoadDeflection(stage,poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,stage);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);

   GetSlabBarrierOverlayDeflection(poi,config,pDy,pRz);
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // NOTE: No need to validate camber models
   GET_IFACE(IProductForces,pProductForces);
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

   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,pgsTypes::BridgeSite2);

   BridgeAnalysisType bat = GetBridgeAnalysisType();

   GetDeckDeflection(poi,config,&Dslab,&Rslab);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad,&Rslab_pad);
   
   Dtrafficbarrier = k2*GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
   Rtrafficbarrier = k2*GetRotation(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);

   Dsidewalk = k2*GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,poi,bat);
   Rsidewalk = k2*GetRotation(pgsTypes::BridgeSite2,pftSidewalk,poi,bat);

   GET_IFACE(IBridge,pBridge);
   if ( !pBridge->IsFutureOverlay() )
   {
      Doverlay = k2*GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
      Roverlay = k2*GetRotation(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
   }

   // Switch the sign. Negative deflection creates positive screed camber
   *pDy = Dslab + Dslab_pad + Dtrafficbarrier + Dsidewalk + Doverlay;
   *pRz = Rslab + Rslab_pad + Rtrafficbarrier + Rsidewalk + Roverlay;
}

void CAnalysisAgentImp::GetHarpedStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,Float64* pMl,Float64* pMr,Float64* pNl,Float64* pNr,Float64* pXl,Float64* pXr)
{
   CamberModelData modelData = GetPrestressDeflectionModel(span,gdr,m_PrestressDeflectionModels);

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

void CAnalysisAgentImp::GetTempStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,Float64* pMxferL,Float64* pMxferR,Float64* pMremoveL,Float64* pMremoveR)
{
   CamberModelData initTempModel    = GetPrestressDeflectionModel(span,gdr,m_InitialTempPrestressDeflectionModels);
   CamberModelData releaseTempModel = GetPrestressDeflectionModel(span,gdr,m_ReleaseTempPrestressDeflectionModels);

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

void CAnalysisAgentImp::GetStraightStrandEquivLoading(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector< std::pair<Float64,Float64> >* loads)
{
   loads->clear();
   // beams 1-4 (index 0-3 are for harped strands)
   CamberModelData model = GetPrestressDeflectionModel(spanIdx,gdrIdx,m_PrestressDeflectionModels);

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
// IPrestressStresses
//
Float64 CAnalysisAgentImp::GetStress(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc)
{
   // This method can be optimized by caching the results.
   GET_IFACE(IPrestressForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(poi.GetSpan(),poi.GetGirder());

   Float64 P;
   bool bIncTempStrands = false;
   switch( stage )
   {
   case pgsTypes::CastingYard:
      P = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterXfer);

      if ( pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         bIncTempStrands = true;
         P += pPsForce->GetStrandForce(poi,pgsTypes::Temporary,pgsTypes::AfterXfer);
      }
      break;

   case pgsTypes::Lifting:
      P  = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AtLifting);
      if ( pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
      {
         bIncTempStrands = true;
         P += pPsForce->GetStrandForce(poi,pgsTypes::Temporary,pgsTypes::AtLifting);
      }
      break;

   case pgsTypes::Hauling:
      P  = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AtShipping);
      P += pPsForce->GetStrandForce(poi,pgsTypes::Temporary,pgsTypes::AtShipping);
      bIncTempStrands = true;
      break;

   case pgsTypes::GirderPlacement:
      P  = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::BeforeTemporaryStrandRemoval);
      P += pPsForce->GetStrandForce(poi,pgsTypes::Temporary,pgsTypes::BeforeTemporaryStrandRemoval);
      bIncTempStrands = true;
      break;

   case pgsTypes::TemporaryStrandRemoval:
      P  = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandRemoval);
      bIncTempStrands = false;
      break;

   case pgsTypes::BridgeSite1:
      P = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterDeckPlacement);
      bIncTempStrands = false;
      break;

   case pgsTypes::BridgeSite2:
      P = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterSIDL);
      bIncTempStrands = false;
      break;

   case pgsTypes::BridgeSite3:
      P = pPsForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterLossesWithLiveLoad);
      bIncTempStrands = false;
      break;

   default:
      ATLASSERT(false);
      break;
   }

   Float64 nSEffective;
   Float64 e = pStrandGeom->GetEccentricity( poi, bIncTempStrands, &nSEffective );

   return GetStress(poi,loc,P,e);
}

Float64 CAnalysisAgentImp::GetStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 e)
{
   if ( loc == pgsTypes::TopSlab )
      return 0.0;

   Float64 A;
   Float64 S;

   GET_IFACE(ISectProp2,pSectProp2);

   // using casting yard stage because we are computing stress on the girder due to prestress which happens in this stage
   A = pSectProp2->GetAg(pgsTypes::CastingYard, poi);
   S = (loc == pgsTypes::TopGirder ? pSectProp2->GetStGirder(pgsTypes::CastingYard, poi) : pSectProp2->GetSb(pgsTypes::CastingYard, poi) );

   Float64 f = -P/A - P*e/S;

   return f;
}

Float64 CAnalysisAgentImp::GetStressPerStrand(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc)
{
   GET_IFACE(IPrestressForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   pgsTypes::LossStage lossStage;
   switch( stage )
   {
   case pgsTypes::CastingYard:
      lossStage = pgsTypes::AfterXfer;
      break;

   case pgsTypes::Lifting:
      lossStage = pgsTypes::AtLifting;
      break;

   case pgsTypes::Hauling:
      lossStage = pgsTypes::AtShipping;
      break;

   case pgsTypes::GirderPlacement:
      lossStage = pgsTypes::BeforeTemporaryStrandRemoval;
      break;

   case pgsTypes::TemporaryStrandRemoval:
      lossStage = pgsTypes::AfterTemporaryStrandRemoval;
      break;

   case pgsTypes::BridgeSite1:
      lossStage = pgsTypes::AfterDeckPlacement;
      break;

   case pgsTypes::BridgeSite2:
      lossStage = pgsTypes::AfterSIDL;
      break;

   case pgsTypes::BridgeSite3:
      lossStage = pgsTypes::AfterLosses;
      break;
   }

   Float64 P = pPsForce->GetPrestressForcePerStrand(poi,strandType,lossStage);
   Float64 nSEffective;
   Float64 e = pStrandGeom->GetEccentricity(poi,strandType, &nSEffective);

   return GetStress(poi,loc,P,e);
}

Float64 CAnalysisAgentImp::GetDesignStress(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config)
{
   GET_IFACE(IPrestressForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   pgsTypes::LossStage lossStage;
   switch( stage )
   {
   case pgsTypes::CastingYard:
      lossStage = pgsTypes::AfterXfer;
      break;

   case pgsTypes::Lifting:
      lossStage = pgsTypes::AtLifting;
      break;

   case pgsTypes::Hauling:
      lossStage = pgsTypes::AtShipping;
      break;

   case pgsTypes::GirderPlacement:
      lossStage = pgsTypes::BeforeTemporaryStrandRemoval;
      break;

   case pgsTypes::TemporaryStrandRemoval:
      lossStage = pgsTypes::AfterTemporaryStrandRemoval;
      break;

   case pgsTypes::BridgeSite1:
      lossStage = pgsTypes::AfterDeckPlacement;
      break;

   case pgsTypes::BridgeSite2:
      lossStage = pgsTypes::AfterSIDL;
      break;

   case pgsTypes::BridgeSite3:
      lossStage = pgsTypes::AfterLossesWithLiveLoad;
      break;
   }

   Float64 P = pPsForce->GetPrestressForce(poi,config,pgsTypes::Permanent,lossStage);

#pragma Reminder("code below must be changed if designer ever uses post-tensioned tempoerary strands")
   if ( stage==pgsTypes::CastingYard || stage==pgsTypes::Lifting || stage==pgsTypes::Hauling || stage==pgsTypes::GirderPlacement) 
      P += pPsForce->GetPrestressForce(poi,config,pgsTypes::Temporary,lossStage);

   Float64 nSEffective;
   bool bIncludeTemporaryStrands = StageCompare(stage,pgsTypes::TemporaryStrandRemoval) < 0 ? true : false;
   Float64 e = pStrandGeom->GetEccentricity(poi, config.PrestressConfig, bIncludeTemporaryStrands, &nSEffective);

   return GetStress(poi,loc,P,e);
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

HRESULT CAnalysisAgentImp::OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
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
      pModelData = GetModelData(gdrIdx);

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
   CamberModelData camber_model_data = GetPrestressDeflectionModel(poi.GetSpan(),poi.GetGirder(),m_PrestressDeflectionModels);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,camber_model_data,g_lcidHarpedStrand,bRelativeToBearings,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,camber_model_data,g_lcidStraightStrand,bRelativeToBearings,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model_data;
   BuildCamberModel(span,gdr,true,config,&model_data);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,model_data,g_lcidHarpedStrand,bRelativeToBearings,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,model_data,g_lcidStraightStrand,bRelativeToBearings,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model_data = GetPrestressDeflectionModel(span,gdr,m_InitialTempPrestressDeflectionModels);
   
   GetInitialTempPrestressDeflection(poi,model_data,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model1, model2;
   BuildTempCamberModel(span,gdr,true,config,&model1,&model2);

   GetInitialTempPrestressDeflection(poi,model1,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   CamberModelData model = GetPrestressDeflectionModel(poi.GetSpan(),poi.GetGirder(),m_ReleaseTempPrestressDeflectionModels);

   GetReleaseTempPrestressDeflection(poi,model,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CamberModelData model1,model2;
   BuildTempCamberModel(span,gdr,true,config,&model1,&model2);

   GetReleaseTempPrestressDeflection(poi,model2,pDy,pRz);
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,LoadCaseIDType lcid,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPOI);

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
      if ( poi.GetID() == INVALID_ID || !poi.HasStage(pgsTypes::CastingYard) )
         thePOI = pPOI->GetPointOfInterest(pgsTypes::CastingYard,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart());
      else
         thePOI = poi;

      ATLASSERT( 0 <= thePOI.GetID() );
      ATLASSERT( thePOI.HasStage(pgsTypes::CastingYard) );

      femPoiID = modelData.PoiMap.GetModelPoi(thePOI);
      ATLASSERT( 0 <= femPoiID );
   }

   HRESULT hr = results->ComputePOIDisplacements(lcid,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   ATLASSERT( SUCCEEDED(hr) );

   Float64 delta = Dy;
   if ( bRelativeToBearings )
   {
      Float64 start_end_size = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
      pgsPointOfInterest poiAtStart = pPOI->GetPointOfInterest(pgsTypes::CastingYard,poi.GetSpan(),poi.GetGirder(),start_end_size);
      ATLASSERT( 0 <= poiAtStart.GetID() );
   
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtStart);
      results->ComputePOIDisplacements(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 start_delta_brg = Dy;

      Float64 end_end_size = pBridge->GetGirderEndConnectionLength( poi.GetSpan(), poi.GetGirder() );
      Float64 Lg = pBridge->GetGirderLength(poi.GetSpan(),poi.GetGirder());
      pgsPointOfInterest poiAtEnd = pPOI->GetPointOfInterest(pgsTypes::CastingYard,poi.GetSpan(),poi.GetGirder(),Lg-end_end_size);
      ATLASSERT( 0 <= poiAtEnd.GetID() );
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtEnd);
      results->ComputePOIDisplacements(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
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
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPOI);

   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(modelData.Model,poi.GetDistFromStart(),&mbrID,&x);

   // Get displacement at poi
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;
   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   results->ComputePOIDisplacements(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   Float64 delta_poi = Dy;

   // Get displacement at start bearing
   Float64 start_end_size = pBridge->GetGirderStartConnectionLength( poi.GetSpan(), poi.GetGirder() );
   pgsPointOfInterest poi2 = pPOI->GetPointOfInterest(pgsTypes::BridgeSite3,poi.GetSpan(),poi.GetGirder(),start_end_size);
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDisplacements(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 start_delta_end_size = Dy;

   // Get displacement at end bearing
   Float64 L = pBridge->GetGirderLength(poi.GetSpan(),poi.GetGirder());
   Float64 end_end_size = pBridge->GetGirderEndConnectionLength( poi.GetSpan(), poi.GetGirder() );
   poi2 = pPOI->GetPointOfInterest(pgsTypes::BridgeSite3,poi.GetSpan(),poi.GetGirder(),L-end_end_size);
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDisplacements(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
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
      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();

      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(IGirderData,pGirderData);
      const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

      bTempStrands = (0 < pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary) && 
                      pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
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

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

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

   Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(span,gdr,config,cpReleaseToDiaphragm,constructionRate)
                            : GetCreepCoefficient(span,gdr,cpReleaseToDiaphragm,constructionRate));


   // creep 1 - Initial to immediately before diaphragm/temporary strands
   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      *pDy = Ct1*(Dgirder + Dps + Dtpsi);
      *pRz = Ct1*(Rgirder + Rps + Rtpsi);
      return;
   }

   // creep 2 - Immediately after diarphagm/temporary strands to deck
   Float64 Ct2 = (bUseConfig ? GetCreepCoefficient(span,gdr,config,cpReleaseToDeck,constructionRate) :
                              GetCreepCoefficient(span,gdr,cpReleaseToDeck,constructionRate) );

   Float64 Ct3 = (bUseConfig ? GetCreepCoefficient(span,gdr,config,cpDiaphragmToDeck,constructionRate) :
                              GetCreepCoefficient(span,gdr,cpDiaphragmToDeck,constructionRate) );

   Float64 Ddiaphragm = GetDisplacement(pgsTypes::BridgeSite1,pftDiaphragm,poi,SimpleSpan);
   Float64 Rdiaphragm = GetRotation(pgsTypes::BridgeSite1,pftDiaphragm,poi,SimpleSpan);

   Float64 Dtpsr,Rtpsr;
   GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, true, &Dtpsr, &Rtpsr);

   *pDy = (Ct2-Ct1)*(Dgirder + Dps + Dtpsi) + Ct3*(Ddiaphragm + Dtpsr);
   *pRz = (Ct2-Ct1)*(Rgirder + Rps + Rtpsi) + Ct3*(Rdiaphragm + Rtpsr);
}

void CAnalysisAgentImp::GetCreepDeflection_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDeck );

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

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
      Ct1 = GetCreepCoefficient(span,gdr,config,cpReleaseToDeck,constructionRate);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      Ct1 = GetCreepCoefficient(span,gdr,cpReleaseToDeck,constructionRate);
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();


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

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(pgsTypes::BridgeSite1,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(pgsTypes::BridgeSite2,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(pgsTypes::BridgeSite1,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(pgsTypes::BridgeSite2,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(span,gdr,config,cpReleaseToDiaphragm,constructionRate) 
                                : GetCreepCoefficient(span,gdr,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(Dgirder + Dps + Dtpsi);
      *pRz = Ct1*(Rgirder + Rps + Rtpsi);
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(span,gdr,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,config,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,config,cpDiaphragmToDeck,   constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(span,gdr,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,cpDiaphragmToDeck,   constructionRate);
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
         Ct1 = GetCreepCoefficient(span,gdr,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(span,gdr,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(span,gdr,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(span,gdr,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(span,gdr,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(span,gdr,cpDeckToFinal,        constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + (Ct4 - Ct3)*(Ddiaphragm + Duser1 + Dtpsr) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + (Ct4 - Ct3)*(Rdiaphragm + Ruser1 + Rtpsr) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();


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

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(pgsTypes::BridgeSite1,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(pgsTypes::BridgeSite2,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(pgsTypes::BridgeSite1,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(pgsTypes::BridgeSite2,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(span,gdr,config,cpReleaseToDiaphragm,constructionRate)
                                : GetCreepCoefficient(span,gdr,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(Dgirder + Dps);
      *pRz = Ct1*(Rgirder + Rps);
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(span,gdr,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,config,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,config,cpDiaphragmToDeck,   constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(span,gdr,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,cpDiaphragmToDeck,   constructionRate);
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
         Ct1 = GetCreepCoefficient(span,gdr,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(span,gdr,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(span,gdr,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(span,gdr,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(span,gdr,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(span,gdr,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(span,gdr,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(span,gdr,cpDeckToFinal,        constructionRate);
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GDRCONFIG dummy_config;

   GetDCamberForGirderSchedule(poi,false,dummy_config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GetDCamberForGirderSchedule(poi,true,config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool bTempStrands = true;
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetNStrands(pgsTypes::Temporary) && 
                      config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(IGirderData,pGirderData);
      const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

      bTempStrands = (0 < pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary) && 
                      pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
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
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,config,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,config,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,config,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi,config,&Ddiaphragm,&Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate,&Dcreep2,&Rcreep2 );
      GetUserLoadDeflection(pgsTypes::BridgeSite1, poi, config,&Duser1,&Ruser1);
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
      GetUserLoadDeflection(pgsTypes::BridgeSite1, poi, &Duser1, &Ruser1);
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
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(pgsTypes::BridgeSite1, poi, config, &Duser1, &Ruser1);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(pgsTypes::BridgeSite1, poi, &Duser1, &Ruser1);
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 x = poi.GetDistFromStart() - start_size;

   *pDy = beam.ComputeDeflection(x);
   *pRz = beam.ComputeRotation(x);
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(SpanIndexType span,GirderIndexType gdr,LoadingEvent le)
{
   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 Fc;

   switch( le )
   {
   case ICamber::leRelease:
      Fc = pMaterial->GetFciGdr(span,gdr);
      break;

   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = pMaterial->GetFcGdr(span,gdr);
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
   ModelData* pModelData = GetModelData(gdr);

   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetSpanLength(span,gdr);

   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < span; i++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(i);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);
      span_start += pBridge->GetSpanLength(i,gdrIdx);
   }

   Float64 span_end = span_start + span_length;

   Float64 cf_points_in_span[2];

   CComPtr<ILBAMAnalysisEngine> pEngine;
   GetEngine(pModelData,true,&pEngine);

   CComPtr<ILoadGroupResponse> response;
   pEngine->get_LoadGroupResponse(&response);
   CComQIPtr<IContraflexureResponse> cfresponse(response);
   CComPtr<IDblArray> cf_locs;
   cfresponse->ComputeContraflexureLocations(CComBSTR("Bridge Site 3"),&cf_locs);
   
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
bool CAnalysisAgentImp::IsContinuityFullyEffective(GirderIndexType girderline)
{
   GET_IFACE(IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   bool bContinuous = false;

   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      bool bContinuousLeft, bContinuousRight;
      pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

      bool bIntegralLeft, bIntegralRight;
      pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

      if ( (bContinuousLeft && bContinuousRight) || (bIntegralLeft && bIntegralRight)  )
      {
         Float64 fb = GetContinuityStressLevel(pierIdx,girderline);
         bContinuous = (0 <= fb ? false : true);
      }
      else
      {
         bContinuous = false;
      }

      if ( bContinuous )
         break;
   }

   return bContinuous;
}

Float64 CAnalysisAgentImp::GetContinuityStressLevel(PierIndexType pier,GirderIndexType gdr)
{
   // computes the stress at the bottom of the girder on each side of the pier
   // returns the greater of the two values
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPOI);

   PierIndexType prev_span = pier - 1;
   SpanIndexType next_span = SpanIndexType(pier);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if ( nSpans <= next_span )
      next_span = nSpans-1;

   // deal with girder index when there are different number of girders in each span
   GirderIndexType prev_span_gdr_idx = gdr;
   GirderIndexType next_span_gdr_idx = gdr;
   if ( prev_span != ALL_SPANS )
      prev_span_gdr_idx = min(gdr,pBridge->GetGirderCount(prev_span)-1);

   if ( next_span != ALL_SPANS )
      next_span_gdr_idx = min(gdr,pBridge->GetGirderCount(next_span)-1);


   CollectionIndexType nPOI = 0;
   pgsPointOfInterest vPOI[2];

   pgsTypes::Stage continuity_stage[2];
   pgsTypes::Stage left_continuity_stage, right_continuity_stage;
   pBridge->GetContinuityStage(pier,&left_continuity_stage,&right_continuity_stage);


   Float64 end_length = 0;
   Float64 prev_girder_length = 0;
   if ( prev_span != ALL_SPANS )
   {
      // get poi at cl bearing at end of prev span
      end_length = pBridge->GetGirderEndConnectionLength(prev_span,prev_span_gdr_idx);
      end_length = max(0.0,end_length);// don't allow geometric error to stop us
      prev_girder_length = pBridge->GetGirderLength(prev_span,prev_span_gdr_idx);
      Float64 dist_from_start_of_girder = prev_girder_length - end_length;
      vPOI[nPOI] = pPOI->GetPointOfInterest(left_continuity_stage,prev_span,prev_span_gdr_idx,dist_from_start_of_girder);
      continuity_stage[nPOI] = left_continuity_stage;
      nPOI++;
   }

   Float64 start_length = 0;
   if ( next_span < nSpans && next_span != prev_span )
   {
      // get poi at cl bearing at start of next span
      start_length = pBridge->GetGirderStartConnectionLength(next_span,next_span_gdr_idx);
      start_length = max(0.0,start_length);
      vPOI[nPOI] = pPOI->GetPointOfInterest(right_continuity_stage,next_span,next_span_gdr_idx,start_length);
      continuity_stage[nPOI] = right_continuity_stage;
      nPOI++;
   }

   Float64 f[2] = {0,0};

   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];
      ATLASSERT( 0 <= poi.GetID() );

      BridgeAnalysisType bat = ContinuousSpan;

      Float64 fbConstruction, fbSlab, fbSlabPad, fbTrafficBarrier, fbSidewalk, fbOverlay, fbUserDC, fbUserDW, fbUserLLIM, fbLLIM;

      Float64 fTop,fBottom;

      if ( continuity_stage[i] == pgsTypes::BridgeSite1 )
      {
         GetStress(pgsTypes::BridgeSite1,pftSlab,poi,bat,&fTop,&fBottom);
         fbSlab = fBottom;

         GetStress(pgsTypes::BridgeSite1,pftSlabPad,poi,bat,&fTop,&fBottom);
         fbSlabPad = fBottom;

         GetStress(pgsTypes::BridgeSite1,pftConstruction,poi,bat,&fTop,&fBottom);
         fbConstruction = fBottom;
      }
      else
      {
         fbSlab = 0;
         fbSlabPad = 0;
         fbConstruction = 0;
      }

      GetStress(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat,&fTop,&fBottom);
      fbTrafficBarrier = fBottom;

      GetStress(pgsTypes::BridgeSite2,pftSidewalk,poi,bat,&fTop,&fBottom);
      fbSidewalk = fBottom;

      GET_IFACE(IBridge,pBridge);
      if ( !pBridge->IsFutureOverlay() )
         GetStress(pgsTypes::BridgeSite2,pftOverlay,poi,bat,&fTop,&fBottom);
      else
         GetStress(pgsTypes::BridgeSite3,pftOverlay,poi,bat,&fTop,&fBottom);

      fbOverlay = fBottom;

      GetStress(pgsTypes::BridgeSite2,pftUserDC,poi,bat,&fTop,&fBottom);
      fbUserDC = fBottom;

      GetStress(pgsTypes::BridgeSite2,pftUserDW,poi,bat,&fTop,&fBottom);
      fbUserDW = fBottom;

      GetStress(pgsTypes::BridgeSite3,pftUserLLIM,poi,bat,&fTop,&fBottom);
      fbUserLLIM = fBottom;

      Float64 fTopMin,fTopMax,fBotMin,fBotMax;
      GetCombinedLiveLoadStress(pgsTypes::lltDesign,pgsTypes::BridgeSite3,poi,bat,&fTopMin,&fTopMax,&fBotMin,&fBotMax);
      fbLLIM = fBotMin; // greatest compression

      fBottom = fbConstruction + fbSlab + fbSlabPad + fbTrafficBarrier + fbSidewalk + fbOverlay + fbUserDC + fbUserDW + 0.5*(fbUserLLIM + fbLLIM);

      f[i] = fBottom;
   }

   return (nPOI == 1 ? f[0] : _cpp_max(f[0],f[1]));
}

/////////////////////////////////////////////////
// IBearingDesign

bool CAnalysisAgentImp::AreBearingReactionsAvailable(SpanIndexType span,GirderIndexType gdr, bool* pBleft, bool* pBright)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISpecification,pSpec);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   SpanIndexType nspans = pBridge->GetSpanCount();

   if (nspans==1 || analysisType==pgsTypes::Simple)
   {
      // Always can get for bearing reactions for single span or simple spans.
      *pBleft  = true;
      *pBright = true;
      return true;
   }
   else
   {
      if (span==ALL_SPANS)
      {
         *pBleft  = false;
         *pBright = false;

         return false;
      }
      else
      {
         // Get boundary conditions at both ends of span
         bool bdummy;
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(span  ,&bdummy,            &bContinuousOnLeft);
         pBridge->IsContinuousAtPier(span+1,&bContinuousOnRight,&bdummy);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(span,  &bdummy,          &bIntegralOnLeft);
         pBridge->IsIntegralAtPier(span+1,&bIntegralOnRight,&bdummy);

         // Bearing reactions are available at simple supports
         bool bSimpleOnLeft, bSimpleOnRight;
         bSimpleOnLeft  = !bContinuousOnLeft  && !bIntegralOnLeft;
         bSimpleOnRight = !bContinuousOnRight && !bIntegralOnRight;

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

void CAnalysisAgentImp::GetBearingProductReaction(pgsTypes::Stage stage,ProductForceType type,SpanIndexType span,GirderIndexType gdr,
                                                  CombinationType cmbtype, BridgeAnalysisType bat,Float64* pLftEnd,Float64* pRgtEnd)
{
   // Get Pois at supports
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPois( pIPOI->GetPointsOfInterest(span,gdr,stage, POI_0L | POI_10L,POIFIND_OR) );
   ATLASSERT(vPois.size()==2);

   pgsPointOfInterest lftPoi(vPois.front());
   pgsPointOfInterest rgtPoi(vPois.back());

   // Loop twice to pick up left and right ends
   for (IndexType idx=0; idx<2; idx++)
   {
      BridgeAnalysisType tmpbat = bat;
      std::vector<pgsPointOfInterest> vPoi;
      if (idx==0)
      {
         vPoi.push_back(lftPoi);
      }
      else
      {
         vPoi.push_back(rgtPoi);

         // Extremely TRICKY:
         // Below we are getting reactions from  end shear, we must flip sign of results to go 
         // from LBAM to beam coordinates. This means that the optimization must go the opposite when using the envelopers.
         if (bat==MinSimpleContinuousEnvelope)
         {
            tmpbat=MaxSimpleContinuousEnvelope;
         }
         else if (bat==MaxSimpleContinuousEnvelope)
         {
            tmpbat=MinSimpleContinuousEnvelope;
         }
      }

      std::vector<sysSectionValue> sec_vals = GetShear(stage, type, vPoi, tmpbat, cmbtype);

      if (idx==0)
      {
         *pLftEnd =  sec_vals.front().Left();
      }
      else
      {
         *pRgtEnd = -sec_vals.front().Right();
      }
   }

   // Last, add point loads from overhangs
   Float64 lft_pnt_load, rgt_pnt_load;
   bool found_load = GetOverhangPointLoads(span, gdr, stage, type, cmbtype, &lft_pnt_load, &rgt_pnt_load);
   if(found_load)
   {
      *pLftEnd -= lft_pnt_load;
      *pRgtEnd -= rgt_pnt_load;
   }
}

void CAnalysisAgentImp::GetBearingLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,
                                BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pLeftRmin,Float64* pLeftRmax,Float64* pLeftTmin,Float64* pLeftTmax,
                                Float64* pRightRmin,Float64* pRightRmax,Float64* pRightTmin,Float64* pRightTmax,
                                VehicleIndexType* pLeftMinVehIdx,VehicleIndexType* pLeftMaxVehIdx,
                                VehicleIndexType* pRightMinVehIdx,VehicleIndexType* pRightMaxVehIdx)
{
   // This is just end shears and rotations due to live load at ends of girder
   // Get Pois at supports
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> SuppPois( pIPOI->GetPointsOfInterest(span,gdr,stage, POI_0L | POI_10L,POIFIND_OR) );
   ATLASSERT(SuppPois.size()==2);

   pgsPointOfInterest lftPoi(SuppPois.front());
   pgsPointOfInterest rgtPoi(SuppPois.back());

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   // Shear and reaction LLDF's can be different. Must ratio results by reaction/shear.
   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   Float64 lldfRatio = 1.0;
   Float64 lldfShear;
   pgsTypes::LimitState lldfLimitState;
   if (bIncludeLLDF)
   {
      bool is_fatigue = llType==pgsTypes::lltFatigue || llType==pgsTypes::lltPermitRating_Special;
      lldfLimitState = is_fatigue ? pgsTypes::FatigueI : pgsTypes::StrengthI;
      lldfShear = pLLDF->GetShearDistFactor(span,gdr,lldfLimitState);
   }

   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

   // Loop twice to pick up left and right ends
   for (IndexType idx=0; idx<2; idx++)
   {
      BridgeAnalysisType tmpbat=bat;
      std::vector<pgsPointOfInterest> vPoi;
      if (idx==0)
      {
         vPoi.push_back(lftPoi);

         // Extremely TRICKY:
         // Below we are getting reactions from  end shear, we must flip sign of results to go 
         // from LBAM to beam coordinates. This means that the optimization must go the opposite when using the envelopers.
         if (bat==MinSimpleContinuousEnvelope)
         {
            tmpbat=MaxSimpleContinuousEnvelope;
         }
         else if (bat==MaxSimpleContinuousEnvelope)
         {
            tmpbat=MinSimpleContinuousEnvelope;
         }
      }
      else
      {
         vPoi.push_back(rgtPoi);
      }

      // Ratio for reaction/shear LLDF
      if (bIncludeLLDF)
      {
         Float64 lldfReact = pLLDF->GetReactionDistFactor(span+idx, gdr, lldfLimitState);
         lldfRatio = lldfReact/lldfShear;
      }

      // Get max'd end shears from lbam
      ModelData* pModelData = UpdateLBAMPois(vPoi);
      CComPtr<ILBAMAnalysisEngine> pEngine;
      GetEngine(pModelData,tmpbat == SimpleSpan ? false : true, &pEngine);
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
         *pLeftRmin = -FyMaxLeft * lldfRatio;
         *pLeftRmax = -FyMinLeft * lldfRatio;

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
         *pRightRmin = -FyMinRight * lldfRatio;
         *pRightRmax = -FyMaxRight * lldfRatio;

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

void CAnalysisAgentImp::GetBearingLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,
                                                   BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                                   Float64* pLeftTmin,Float64* pLeftTmax,Float64* pLeftRmin,Float64* pLeftRmax,
                                                   Float64* pRightTmin,Float64* pRightTmax,Float64* pRightRmin,Float64* pRightRmax,
                                                   VehicleIndexType* pLeftMinVehIdx,VehicleIndexType* pLeftMaxVehIdx,
                                                   VehicleIndexType* pRightMinVehIdx,VehicleIndexType* pRightMaxVehIdx)
{
   // This is just end shears and rotations due to live load at ends of girder
   // Get Pois at supports
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> SuppPois( pIPOI->GetPointsOfInterest(span,gdr,stage, POI_0L | POI_10L,POIFIND_OR) );
   ATLASSERT(SuppPois.size()==2);

   pgsPointOfInterest lftPoi(SuppPois.front());
   pgsPointOfInterest rgtPoi(SuppPois.back());

   LiveLoadModelType llmt = g_LiveLoadModelType[llType];

   VARIANT_BOOL vbIncludeImpact = (bIncludeImpact ? VARIANT_TRUE : VARIANT_FALSE);
   VARIANT_BOOL vbIncludeLLDF   = (bIncludeLLDF   ? VARIANT_TRUE : VARIANT_FALSE);

   GET_IFACE(IStageMap,pStageMap);
   CComBSTR bstrStage = pStageMap->GetStageName(stage);

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
      GetEngine(pModelData,bat == SimpleSpan ? false : true, &pEngine);
      CComPtr<IBasicVehicularResponse> response;
      pEngine->get_BasicVehicularResponse(&response);

      CComPtr<ILiveLoadModelSectionResults> minResults;
      CComPtr<ILiveLoadModelSectionResults> maxResults;
      CComBSTR bstrStage = pStageMap->GetStageName(stage);

      if ( bat == SimpleSpan || bat == ContinuousSpan )
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
            BridgeAnalysisType batLeft = (bat == MinSimpleContinuousEnvelope ? MaxSimpleContinuousEnvelope : MinSimpleContinuousEnvelope);
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

void CAnalysisAgentImp::GetBearingCombinedReaction(LoadingCombination combo,pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,
                                                   CombinationType cmb_type,BridgeAnalysisType bat, Float64* pLftEnd,Float64* pRgtEnd)
{
   // Use lbam to get load caes for this combination
   ModelData* pModelData = GetModelData(gdr);

   CComPtr<ILBAMModel> lbam;
   GetModel(pModelData, bat, &lbam);

   CComPtr<ILoadCases> load_cases;
   lbam->get_LoadCases(&load_cases);

   CComBSTR combo_name = GetLoadCaseName(combo);

   CComPtr<ILoadCase> load_case;
   load_cases->Find(combo_name, &load_case);

   CollectionIndexType lc_cnt;
   load_case->get_LoadGroupCount(&lc_cnt);

   // Cycle through load cases and sum reactions
   Float64 Rlft(0.0), Rrgt(0.0);
   for (CollectionIndexType idx=0; idx<lc_cnt; idx++)
   {
      CComBSTR lc_name;
      load_case->GetLoadGroup(idx, &lc_name);

      ProductForceType case_type = GetLoadGroupTypeFromName(lc_name); 

      Float64 lft, rgt;
      GetBearingProductReaction(stage, case_type, span, gdr, cmb_type, bat, &lft, &rgt);

      Rlft += lft;
      Rrgt += rgt;
   }

   *pLftEnd = Rlft;
   *pRgtEnd = Rrgt;
}

void CAnalysisAgentImp::GetBearingCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,
                                                           BridgeAnalysisType bat,bool bIncludeImpact,
                                                           Float64* pLeftRmin, Float64* pLeftRmax, Float64* pRightRmin,Float64* pRightRmax)
{
   ATLASSERT(stage == pgsTypes::BridgeSite3);

   // Get bearing reactions by getting beam end shears.
   // Get Pois at supports
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> SuppPois( pIPOI->GetPointsOfInterest(span,gdr,stage, POI_0L | POI_10L,POIFIND_OR) );
   ATLASSERT(SuppPois.size()==2);

   pgsPointOfInterest lftPoi(SuppPois.front());
   pgsPointOfInterest rgtPoi(SuppPois.back());

   sysSectionValue lft_min_sec_val, lft_max_sec_val, rgt_min_sec_val, rgt_max_sec_val;

  // TRICKY:
   // For shear, we must flip sign of results to go from LBAM to beam coordinates. This means
   // that the optimization must go the opposite way as well when using the envelopers
   BridgeAnalysisType right_bat(bat);
   if (bat==MinSimpleContinuousEnvelope)
   {
      right_bat = MaxSimpleContinuousEnvelope;
   }
   else if (bat==MaxSimpleContinuousEnvelope)
   {
      right_bat = MinSimpleContinuousEnvelope;
   }

   GetCombinedLiveLoadShear(llType, stage, lftPoi,       bat, bIncludeImpact, &lft_min_sec_val, &lft_max_sec_val);
   GetCombinedLiveLoadShear(llType, stage, rgtPoi, right_bat, bIncludeImpact, &rgt_min_sec_val, &rgt_max_sec_val);

   // Shear and reaction LLDF's can be different. Must ratio results by reaction/shear.
   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   bool is_fatigue = llType==pgsTypes::lltFatigue || llType==pgsTypes::lltPermitRating_Special;
   pgsTypes::LimitState lldfLimitState = is_fatigue ? pgsTypes::FatigueI : pgsTypes::StrengthI;
   Float64 lldfShear = pLLDF->GetShearDistFactor(span,gdr,lldfLimitState);

   Float64 lldfLeftReact  = pLLDF->GetReactionDistFactor(span, gdr, lldfLimitState);
   Float64 lldfRightReact = pLLDF->GetReactionDistFactor(span+1, gdr, lldfLimitState);

   Float64 lldfLeftRatio = lldfLeftReact/lldfShear;
   Float64 lldfRightRatio = lldfRightReact/lldfShear;

   *pLeftRmin =  lft_min_sec_val.Left() * lldfLeftRatio;
   *pLeftRmax =  lft_max_sec_val.Left() * lldfLeftRatio;

   *pRightRmin = -rgt_max_sec_val.Right() * lldfRightRatio;
   *pRightRmax = -rgt_min_sec_val.Right() * lldfRightRatio;
}

void CAnalysisAgentImp::GetBearingLimitStateReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,
                                                     BridgeAnalysisType bat,bool bIncludeImpact,
                                                     Float64* pLeftRmin, Float64* pLeftRmax, Float64* pRightRmin,Float64* pRightRmax)
{
   // We have to emulate what the LBAM does for load combinations here
   *pLeftRmin  = 0.0;
   *pLeftRmax  = 0.0;
   *pRightRmin = 0.0;
   *pRightRmax = 0.0;

   // Use lbam to get load factors for this limit state
   ModelData* pModelData = GetModelData(gdr);

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
         GetBearingCombinedReaction(combo, stage, span, gdr, ctCummulative, bat, &lc_lft_res, &lc_rgt_res);

         *pLeftRmin  += min_factor * lc_lft_res;
         *pLeftRmax  += max_factor * lc_lft_res;
         *pRightRmin += min_factor * lc_rgt_res;
         *pRightRmax += max_factor * lc_rgt_res;
      }
   }

   // Next, factor and combine live load
   if(stage == pgsTypes::BridgeSite3)
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
         if (lltype==pgsTypes::lltPedestrian && !HasPedestrianLoad(span, gdr))
            break;

         Float64 leftRmin, leftRmax, rightRmin, rightRmax;
         GetBearingCombinedLiveLoadReaction(lltype, stage, span, gdr, bat, bIncludeImpact,
                                            &leftRmin, &leftRmax, &rightRmin, &rightRmax);

         LlLeftRmin  = min(LlLeftRmin, leftRmin);
         LlLeftRmax  = max(LlLeftRmax, leftRmax);
         LlRightRmin = min(LlRightRmin, rightRmin);
         LlRightRmax = max(LlRightRmax, rightRmax);
      }

      Float64 ll_factor;
      load_combo->get_LiveLoadFactor(&ll_factor);

      *pLeftRmin  += ll_factor * LlLeftRmin;
      *pLeftRmax  += ll_factor * LlLeftRmax ;
      *pRightRmin += ll_factor * LlRightRmin;
      *pRightRmax += ll_factor * LlRightRmax;
   }

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
void CAnalysisAgentImp::AddOverhangPointLoads(SpanIndexType spanIdx, GirderIndexType gdrIdx, const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup,
                                              Float64 PStart, Float64 PEnd, IPointLoads* pointLoads)
{
   // Create and apply loads to the LBAM
   if (PStart != 0.0)
   {
      CComPtr<IPointLoad> loadPstart;
      loadPstart.CoCreateInstance(CLSID_PointLoad);
      loadPstart->put_MemberType(mtSupport);
      loadPstart->put_MemberID(spanIdx);
      loadPstart->put_Location(0.0);
      loadPstart->put_Fy(PStart);

      CComPtr<IPointLoadItem> ptLoadItem;
      pointLoads->Add(bstrStage,bstrLoadGroup,loadPstart,&ptLoadItem);
   }

   if (PEnd != 0.0)
   {
      CComPtr<IPointLoad> loadPend;
      loadPend.CoCreateInstance(CLSID_PointLoad);
      loadPend->put_MemberType(mtSupport);
      loadPend->put_MemberID(spanIdx+1);
      loadPend->put_Location(0.0);
      loadPend->put_Fy(PEnd);

      CComPtr<IPointLoadItem> ptLoadItem;
      pointLoads->Add(bstrStage,bstrLoadGroup,loadPend,&ptLoadItem);
   }

   // Store load so we can use it when computing bearing reactions
   if (PStart!=0.0 || PEnd!=0.0)
   {
      // Note that load will only be inserted once. You might be tempted to sum loads here on multiple
      // insertions. BE VERY CAREFUL - This function gets called multiple times (once for each
      // analysis model type). 
      OverhangLoadDataType new_val(spanIdx, gdrIdx, bstrStage, bstrLoadGroup, PStart, PEnd);
      std::pair<OverhangLoadIterator,bool> lit = m_OverhangLoadSet.insert( new_val );
   }
}

bool CAnalysisAgentImp::GetOverhangPointLoads(SpanIndexType spanIdx, GirderIndexType gdrIdx, pgsTypes::Stage stage,ProductForceType type,
                                              CombinationType cmbtype, Float64* pPStart, Float64* pPEnd)
{
   *pPStart = 0.0;
   *pPEnd   = 0.0;

   // Need to sum results over stages, so use bridgesite ordering 
   const int nstages=5;
   pgsTypes::Stage stage_order[nstages]={pgsTypes::GirderPlacement, pgsTypes::TemporaryStrandRemoval, pgsTypes::BridgeSite1, pgsTypes::BridgeSite2, pgsTypes::BridgeSite3};

   // Get iterator to current stage in stage loop
   pgsTypes::Stage* pcurrent = std::find(stage_order, stage_order+nstages, stage);

   if (pcurrent == stage_order+nstages)
   {
      ATLASSERT(0); // not found. shouldn't be passing in non-bridge site stages?
      return false;
   }
   else
   {
      GET_IFACE(IStageMap,pStageMap);

      CComBSTR bstrLoadGroup( GetLoadGroupName(type) );

      // Determine start and end of loop range
      pgsTypes::Stage* pend = pcurrent+1;

      pgsTypes::Stage* pstart;
      if (cmbtype==ctCummulative)
      {
         pstart = &stage_order[0];
      }
      else
      {
         pstart = pcurrent;
      }

      bool found = false;
      while(pstart!=pend)
      {
         CComBSTR bstrStage( pStageMap->GetStageName(*pstart) );

         OverhangLoadIterator lit = m_OverhangLoadSet.find( OverhangLoadDataType(spanIdx, gdrIdx, bstrStage, bstrLoadGroup, 0.0, 0.0) );
         if (lit != m_OverhangLoadSet.end())
         {
            *pPStart += lit->PStart;
            *pPEnd   += lit->PEnd;
            found = true;
         }

         pstart++;
      }

      return found;
   }
}


/////////////////////////////////////////////////
CAnalysisAgentImp::SpanType CAnalysisAgentImp::GetSpanType(SpanIndexType span,bool bContinuous)
{
   if ( !bContinuous )
      return PinPin;

   GET_IFACE(IBridge,pBridge);
   PierIndexType prev_pier = span;
   PierIndexType next_pier = prev_pier + 1;

   bool bContinuousLeft, bContinuousRight;
   pBridge->IsContinuousAtPier(prev_pier,&bContinuousLeft,&bContinuousRight);

   bool bIntegralLeft, bIntegralRight;
   pBridge->IsIntegralAtPier(prev_pier,&bIntegralLeft,&bIntegralRight);

   bool bFixStart = bContinuousRight || bIntegralRight;
   
   pBridge->IsContinuousAtPier(next_pier,&bContinuousLeft,&bContinuousRight);
   pBridge->IsIntegralAtPier(next_pier,&bIntegralLeft,&bIntegralRight);

   bool bFixEnd = bContinuousLeft || bIntegralLeft;

   if ( bFixStart && bFixEnd )
      return FixFix;

   if ( bFixStart && !bFixEnd )
      return FixPin;

   if ( !bFixStart && bFixEnd )
      return PinFix;

   if ( !bFixStart && !bFixEnd )
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
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(i);
      GirderIndexType gdr = min(gdrIdx,nGirders-1);
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
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(i);
      GirderIndexType gdr = min(gdrIdx,nGirders-1);
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
   gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx+1,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);

   AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
}

void CAnalysisAgentImp::ApplyLLDF_FixPin(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors)
{
   GET_IFACE(IBridge,pBridge);

   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(i);
      GirderIndexType gdr = min(gdrIdx,nGirders-1);
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
   Float64 gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead); // DF over pier at start of span
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
   Float64 span_start = 0;
   for ( SpanIndexType i = 0; i < spanIdx; i++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(i);
      GirderIndexType gdr = min(gdrIdx,nGirders-1);
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

      gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gR  = 99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD, gPedes);
      
      gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx+1,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
      AddDistributionFactors(distFactors,span_length/2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
   else if ( num_cf_points_in_span == 1 )
   {
      gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gR  = 99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 seg_length_1 = cf_points_in_span[0] - span_start;
      Float64 seg_length_2 = span_end - cf_points_in_span[0];

      AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      
      gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx+1,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
      AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
   else
   {
      gpM = pLLDF->GetMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
      gV  = pLLDF->GetShearDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      gR  =  99999999; // this parameter should not be used so use a value that is obviously wrong to easily detect bugs

      Float64 seg_length_1 = cf_points_in_span[0] - span_start;
      Float64 seg_length_2 = cf_points_in_span[1] - cf_points_in_span[0];
      Float64 seg_length_3 = span_end - cf_points_in_span[1];

      AddDistributionFactors(distFactors,seg_length_1,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      
      gnM = pLLDF->GetNegMomentDistFactor(spanIdx,gdrIdx,pgsTypes::StrengthI);
      AddDistributionFactors(distFactors,seg_length_2,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
      
      gnM = pLLDF->GetNegMomentDistFactorAtPier(spanIdx+1,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
      AddDistributionFactors(distFactors,seg_length_3,gpM,gnM,gV,gR,gFM,gFV,gD,gPedes);
   }
}

void CAnalysisAgentImp::ApplyLLDF_Support(PierIndexType pierIdx,GirderIndexType gdrIdx,SpanIndexType nSpans,ISupports* supports)
{
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

   // For pedestrian loads - take average of loads from adjacent spans
   Float64 leftPedes(0.0), rightPedes(0.0);
   Int32 nls(0);
   if(pierIdx>0)
   {
      leftPedes = this->GetPedestrianLiveLoad(pierIdx-1,gdrIdx);
      nls++;
   }

   if (pierIdx < nSpans)
   {
      rightPedes = this->GetPedestrianLiveLoad(pierIdx,gdrIdx);
      nls++;
   }

   Float64 gPedes = (leftPedes+rightPedes)/nls;

   GET_IFACE(IBridge,pBridge);
   SpanIndexType spanIdx = pierIdx;
   if ( spanIdx == pBridge->GetSpanCount() )
      spanIdx--;

   Float64 gD  = pLLDF->GetDeflectionDistFactor(spanIdx,gdrIdx);

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

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   CComPtr<IUnknown> unk_engine;
   IndexType engineIdx = 0;
   if ( bContinuous && 1 < nSpans )
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

Float64 CAnalysisAgentImp::GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::Stage stage)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   Float64 fc = (stage == pgsTypes::CastingYard ? config.Fci : config.Fc);

   GET_IFACE(ISectProp2,pSectProp2);

   Float64 Ix          = pSectProp2->GetIx(stage,poi);
   Float64 Ix_adjusted = pSectProp2->GetIx(stage,poi,fc);

   GET_IFACE(IBridgeMaterialEx,pMaterial);
   Float64 Ec = pMaterial->GetEcGdr(span,gdr);
   Float64 Ec_adjusted = (config.bUserEc ? config.Ec : pMaterial->GetEconc(fc,pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetEccK1Gdr(span,gdr),pMaterial->GetEccK2Gdr(span,gdr)));

   Float64 EI = Ec*Ix;
   Float64 EI_adjusted = Ec_adjusted * Ix_adjusted;

   Float64 k = EI/EI_adjusted;

   return k;
}

BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType()
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);
   return bat;
}

CAnalysisAgentImp::ModelData* CAnalysisAgentImp::UpdateLBAMPois(const std::vector<pgsPointOfInterest>& vPoi)
{
   m_LBAMPoi->Clear();

   // Start by checking if the model exists
   // get the maximum girder index because this will govern which model we want to get
   //
   // Example: Span 1 - 4 Girders
   //          Span 2 - 5 Girders
   //          called vPoi = pPOI->GetPointsOfInterest(-1,4,POI_GRAPHICAL); to get the vector of poi's for girder line 4
   //          there is not a girder line index 4 in span 1 so GetPointsOfInterest returns poi's for girder line index 3
   //          the vector of POI have mixed girder lines
   //          we want the model that corresponds to the max girder index
   //
   GirderIndexType gdrIdx = 0;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      gdrIdx = max(gdrIdx,poi.GetGirder());
   }

   // get the model
   ModelData* pModelData = 0;
   pModelData = GetModelData(gdrIdx);

   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      PoiIDType poi_id = pModelData->PoiMap.GetModelPoi(poi);
      if ( poi_id == INVALID_ID )
      {
         poi_id = AddPointOfInterest( pModelData, poi );
         ATLASSERT( 0 <= poi_id && poi_id != INVALID_ID );
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
   CComPtr<IIndexArray> lngAxleConfig;
   pConfig->get_AxleConfig(&lngAxleConfig);

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
      lngAxleConfig->get__EnumElements(&enum_array);
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

void CAnalysisAgentImp::GetModel(ModelData* pModelData,BridgeAnalysisType bat,ILBAMModel** ppModel)
{
   switch( bat )
   {
   case SimpleSpan:
      (*ppModel) = pModelData->m_Model;
      (*ppModel)->AddRef();

      break;

   case ContinuousSpan:
   case MinSimpleContinuousEnvelope:
   case MaxSimpleContinuousEnvelope:
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