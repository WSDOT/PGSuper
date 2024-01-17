///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include "GirderModelData.h"
#include "PGSuperLoadCombinationResponse.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CGirderModelData::CGirderModelData(const CGirderModelManager *pParent,GirderIndexType gdrLineIdx)
{
   m_pParent = pParent;
   m_GirderLineIndex = gdrLineIdx;
   m_bSimpleModelHasPretensionLoad = false;
   m_bContinuousModelHasPretensionLoad = false;

   // create minimum model enveloper and initialize it (no models just yet)
   m_MinModelEnveloper.CoCreateInstance(CLSID_LBAMModelEnveloper);
   m_MinModelEnveloper->Initialize(nullptr,atForce,optMinimize);

   // create maximum model enveloper and initialize it (no models just yet)
   m_MaxModelEnveloper.CoCreateInstance(CLSID_LBAMModelEnveloper);
   m_MaxModelEnveloper->Initialize(nullptr,atForce,optMaximize);
}

CGirderModelData::CGirderModelData(const CGirderModelData& other)
{ 
   *this = other;
}

CGirderModelData::~CGirderModelData()
{
}

void CGirderModelData::CreateAnalysisEngine(ILBAMModel* theModel,pgsTypes::BridgeAnalysisType bat,ILBAMAnalysisEngine** ppEngine)
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

   // initialize the engine with default values (nullptr), except for the vehicular response enveloper
   engine->InitializeEx(theModel,atForce,
                        nullptr, // load group response
                        nullptr, // unit load response
                        nullptr, // influence line response
                        nullptr, // analysis pois
                        nullptr, // basic vehicular response
                        nullptr, // live load model response
                        envelopedVehicularResponse,
                        nullptr, // load case response
                        load_combo_response/*nullptr*/, // load combination response
                        nullptr, // concurrent load combination response
                        nullptr, // live load negative moment response
                        nullptr); // contraflexure response

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

void CGirderModelData::AddSimpleModel(ILBAMModel* pModel)
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
   ATLASSERT(pDeflEnvelopedVehicularResponse[pgsTypes::SimpleSpan]!=nullptr);
}

void CGirderModelData::AddContinuousModel(ILBAMModel* pContModel)
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
   ATLASSERT(pDeflEnvelopedVehicularResponse[pgsTypes::ContinuousSpan]!=nullptr);



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

void CGirderModelData::operator=(const CGirderModelData& other)
{ 
   m_pParent = other.m_pParent;
   m_GirderLineIndex = other.m_GirderLineIndex;
   m_bSimpleModelHasPretensionLoad = other.m_bSimpleModelHasPretensionLoad;
   m_bContinuousModelHasPretensionLoad = other.m_bContinuousModelHasPretensionLoad;

   PoiMap = other.PoiMap;

   m_Model             = other.m_Model;
   m_ContinuousModel   = other.m_ContinuousModel;
   m_MinModelEnveloper = other.m_MinModelEnveloper;
   m_MaxModelEnveloper = other.m_MaxModelEnveloper;

   pLoadGroupResponse[pgsTypes::SimpleSpan]     = other.pLoadGroupResponse[pgsTypes::SimpleSpan];
   pLoadGroupResponse[pgsTypes::ContinuousSpan] = other.pLoadGroupResponse[pgsTypes::ContinuousSpan];

   pLoadCaseResponse[pgsTypes::SimpleSpan]      = other.pLoadCaseResponse[pgsTypes::SimpleSpan];
   pLoadCaseResponse[pgsTypes::ContinuousSpan]  = other.pLoadCaseResponse[pgsTypes::ContinuousSpan];

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
