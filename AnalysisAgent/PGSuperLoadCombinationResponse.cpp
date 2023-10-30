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

// EngAgentImp.cpp : Implementation of CEngAgentImp
#include "stdafx.h"
#include "PGSuperLoadCombinationResponse.h"

#include "GirderModelManager.h"

#include <EAF\EAFUtilities.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPGSuperLoadCombinationResponse
void CPGSuperLoadCombinationResponse::Initialize(ILoadCombinationResponse* pLCResponse,ILoadGroupResponse* pLGResponse,ILiveLoadModelResponse* pLLResponse,ILBAMModel* pModel,const CGirderModelManager* pModelManager)
{
   m_LCResponseDelegate = pLCResponse;
   m_LiveLoadResponse = pLLResponse;
   m_LoadGroupResponse = pLGResponse;

   m_Model = pModel;
   m_pGirderModelManager = pModelManager;

   m_Model->get_POIs(&m_POIs);

   // Get these interfaces once so we don't have to do it over and over
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   pBroker->GetInterface(IID_ILibrary,(IUnknown**)&m_pLibrary);
   pBroker->GetInterface(IID_IRatingSpecification,(IUnknown**)&m_pRatingSpec);
}

HRESULT CPGSuperLoadCombinationResponse::FinalConstruct()
{
   return S_OK;
}

void CPGSuperLoadCombinationResponse::FinalRelease()
{
   m_LCResponseDelegate.Release();
}

//////////////////////////////////////////////////////////////////////////////////////////////
// ILoadCombinationResponse
STDMETHODIMP CPGSuperLoadCombinationResponse::ComputeForces(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage, /*[in]*/ResultsOrientation orientation, 
                            /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                            /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                            /*[out,retval]*/ILoadCombinationSectionResults** results)
{
   VARIANT_BOOL bComputeConfig = (computeConfig == VARIANT_TRUE || includeLiveLoad == VARIANT_TRUE) ? VARIANT_TRUE : computeConfig;

   HRESULT hr = m_LCResponseDelegate->ComputeForces(LoadCombination, POIs, Stage, orientation, 
                            summ, effect, optimization, 
                            includeLiveLoad, includeImpact, bComputeConfig,
                            results);

   if ( includeLiveLoad == VARIANT_TRUE )
   {
      CComPtr<IIDArray> singlePOI;
      singlePOI.CoCreateInstance(CLSID_IDArray);

      CComPtr<ILoadCombinations> loadCombos;
      m_Model->get_LoadCombinations(&loadCombos) ;
      CComPtr<ILoadCombination> loadCombo;
      loadCombos->Find(LoadCombination,&loadCombo);
      Float64 gLLmin, gLLmax;
      loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
   
      if ( gLLmin < 0 || gLLmax < 0 )
      {
         // we have to re-sum the limit state combination using the correct
         // load factor for the live load

         // for each result...
         IndexType nResults;
         (*results)->get_Count(&nResults);
         for ( IndexType resultIdx = 0; resultIdx < nResults; resultIdx++ )
         {
            singlePOI->Clear();

            Float64 left_result, right_result;
            CComPtr<ILoadCombinationResultConfiguration> left_config, right_config;

            // get the results
            (*results)->GetResult(resultIdx, &left_result, &left_config, &right_result, &right_config);
            
            Float64 gLL_left_new, gLL_right_new;
            GetNewLiveLoadFactors(LoadCombination,left_config,right_config,&gLL_left_new,&gLL_right_new);
            
            // re-do the load combination
            left_config->put_LiveLoadFactor(gLL_left_new);
            right_config->put_LiveLoadFactor(gLL_right_new);

            CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(m_LCResponseDelegate);
            ATLASSERT(concurrent_response != nullptr);

            CComPtr<ISectionResult3Ds> left_results_new, right_results_new;
            PoiIDType poiID;
            POIs->get_Item(resultIdx,&poiID);
            singlePOI->Add(poiID);

            concurrent_response->ComputeForces(singlePOI,Stage,orientation,left_config,&left_results_new);
            concurrent_response->ComputeForces(singlePOI,Stage,orientation,right_config,&right_results_new);

            CComPtr<ISectionResult3D> left_result_new, right_result_new;
            left_results_new->get_Item(0, &left_result_new);
            right_results_new->get_Item(0,&right_result_new);

            Float64 left_value_new, right_value_new, dummy;
            left_result_new->GetSingleResult(effect, &left_value_new, &dummy);
            right_result_new->GetSingleResult(effect,&dummy, &right_value_new);

            (*results)->SetResult(resultIdx,left_value_new,left_config,right_value_new,right_config);
         }
      }
   }

   return hr;
}

STDMETHODIMP CPGSuperLoadCombinationResponse::ComputeDeflections(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                                 /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                                 /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                                 /*[out,retval]*/ILoadCombinationSectionResults** results)
{
   VARIANT_BOOL bComputeConfig = (computeConfig == VARIANT_TRUE || includeLiveLoad == VARIANT_TRUE) ? VARIANT_TRUE : computeConfig;
   
   HRESULT hr = m_LCResponseDelegate->ComputeDeflections(LoadCombination, POIs, Stage,
                                 summ, effect, optimization, 
                                 includeLiveLoad, includeImpact, bComputeConfig,
                                 results);

   if ( includeLiveLoad == VARIANT_TRUE )
   {
      CComPtr<IIDArray> singlePOI;
      singlePOI.CoCreateInstance(CLSID_IDArray);

      CComPtr<ILoadCombinations> loadCombos;
      m_Model->get_LoadCombinations(&loadCombos) ;
      CComPtr<ILoadCombination> loadCombo;
      loadCombos->Find(LoadCombination,&loadCombo);
      Float64 gLLmin, gLLmax;
      loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
   
      if ( gLLmin < 0 || gLLmax < 0 )
      {
         // we have to re-sum the limit state combination using the correct
         // load factor for the live load

         // for each result...
         IndexType nResults;
         (*results)->get_Count(&nResults);
         for ( IndexType resultIdx = 0; resultIdx < nResults; resultIdx++ )
         {
            singlePOI->Clear();

            Float64 left_result, right_result;
            CComPtr<ILoadCombinationResultConfiguration> left_config, right_config;

            // get the results
            (*results)->GetResult(resultIdx, &left_result, &left_config, &right_result, &right_config);
            
            Float64 gLL_left_new, gLL_right_new;
            GetNewLiveLoadFactors(LoadCombination,left_config,right_config,&gLL_left_new,&gLL_right_new);
            
            // re-do the load combination
            left_config->put_LiveLoadFactor(gLL_left_new);
            right_config->put_LiveLoadFactor(gLL_right_new);

            CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(m_LCResponseDelegate);
            ATLASSERT(concurrent_response != nullptr);

            CComPtr<ISectionResult3Ds> left_results_new, right_results_new;
            PoiIDType poiID;
            POIs->get_Item(resultIdx,&poiID);
            singlePOI->Add(poiID);

            concurrent_response->ComputeDeflections(singlePOI,Stage,left_config,&left_results_new);
            concurrent_response->ComputeDeflections(singlePOI,Stage,right_config,&right_results_new);

            CComPtr<ISectionResult3D> left_result_new, right_result_new;
            left_results_new->get_Item(0, &left_result_new);
            right_results_new->get_Item(0,&right_result_new);

            Float64 left_value_new, right_value_new, dummy;
            left_result_new->GetSingleResult(effect, &left_value_new, &dummy);
            right_result_new->GetSingleResult(effect,&dummy, &right_value_new);

            (*results)->SetResult(resultIdx,left_value_new,left_config,right_value_new,right_config);
         }
      }
   }

   return hr;
}

STDMETHODIMP CPGSuperLoadCombinationResponse::ComputeReactions(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                               /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                               /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                               /*[out,retval]*/ILoadCombinationResults** results)
{
   VARIANT_BOOL bComputeConfig = (computeConfig == VARIANT_TRUE || includeLiveLoad == VARIANT_TRUE) ? VARIANT_TRUE : computeConfig;
   
   HRESULT hr = m_LCResponseDelegate->ComputeReactions(LoadCombination, POIs, Stage,
                               summ, effect, optimization, 
                               includeLiveLoad, includeImpact, bComputeConfig,
                               results);

   if ( includeLiveLoad == VARIANT_TRUE )
   {
      CComPtr<IIDArray> singlePOI;
      singlePOI.CoCreateInstance(CLSID_IDArray);

      CComPtr<ILoadCombinations> loadCombos;
      m_Model->get_LoadCombinations(&loadCombos) ;
      CComPtr<ILoadCombination> loadCombo;
      loadCombos->Find(LoadCombination,&loadCombo);
      Float64 gLLmin, gLLmax;
      loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
   
      if ( gLLmin < 0 || gLLmax < 0 )
      {
         // we have to re-sum the limit state combination using the correct
         // load factor for the live load

         // for each result...
         IndexType nResults;
         (*results)->get_Count(&nResults);
         for ( IndexType resultIdx = 0; resultIdx < nResults; resultIdx++ )
         {
            singlePOI->Clear();

            Float64 result;
            CComPtr<ILoadCombinationResultConfiguration> config;

            // get the results
            (*results)->GetResult(resultIdx, &result, &config);
            
            Float64 gLL_left_new, gLL_right_new;
            GetNewLiveLoadFactors(LoadCombination,config,config,&gLL_left_new,&gLL_right_new);
            
            // re-do the load combination
            config->put_LiveLoadFactor(gLL_left_new);

            CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(m_LCResponseDelegate);
            ATLASSERT(concurrent_response != nullptr);

            CComPtr<IResult3Ds> results_new;
            PoiIDType poiID;
            POIs->get_Item(resultIdx,&poiID);
            singlePOI->Add(poiID);

            concurrent_response->ComputeReactions(singlePOI,Stage,config,&results_new);

            CComPtr<IResult3D> result_new;
            results_new->get_Item(0,&result_new);

            Float64 value_new;
            result_new->GetSingleResult(effect,&value_new);

            (*results)->SetResult(resultIdx,value_new,config);
         }
      }
   }

   return hr;
}

STDMETHODIMP CPGSuperLoadCombinationResponse::ComputeSupportDeflections(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                                        /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                                        /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                                        /*[out,retval]*/ILoadCombinationResults** results)
{
   VARIANT_BOOL bComputeConfig = (computeConfig == VARIANT_TRUE || includeLiveLoad == VARIANT_TRUE) ? VARIANT_TRUE : computeConfig;
   
   HRESULT hr = m_LCResponseDelegate->ComputeSupportDeflections(LoadCombination, POIs, Stage,
                                        summ, effect, optimization, 
                                        includeLiveLoad, includeImpact, computeConfig,
                                        results);

   if ( includeLiveLoad == VARIANT_TRUE )
   {
      CComPtr<IIDArray> singlePOI;
      singlePOI.CoCreateInstance(CLSID_IDArray);

      CComPtr<ILoadCombinations> loadCombos;
      m_Model->get_LoadCombinations(&loadCombos) ;
      CComPtr<ILoadCombination> loadCombo;
      loadCombos->Find(LoadCombination,&loadCombo);
      Float64 gLLmin, gLLmax;
      loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
   
      if ( gLLmin < 0 || gLLmax < 0 )
      {
         // we have to re-sum the limit state combination using the correct
         // load factor for the live load

         // for each result...
         IndexType nResults;
         (*results)->get_Count(&nResults);
         for ( IndexType resultIdx = 0; resultIdx < nResults; resultIdx++ )
         {
            singlePOI->Clear();

            Float64 result;
            CComPtr<ILoadCombinationResultConfiguration> config;

            // get the results
            (*results)->GetResult(resultIdx, &result, &config);
            
            Float64 gLL_left_new, gLL_right_new;
            GetNewLiveLoadFactors(LoadCombination,config,config,&gLL_left_new,&gLL_right_new);
            
            // re-do the load combination
            config->put_LiveLoadFactor(gLL_left_new);

            CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(m_LCResponseDelegate);
            ATLASSERT(concurrent_response != nullptr);

            CComPtr<IResult3Ds> results_new;
            PoiIDType poiID;
            POIs->get_Item(resultIdx,&poiID);
            singlePOI->Add(poiID);

            concurrent_response->ComputeSupportDeflections(singlePOI,Stage,config,&results_new);

            CComPtr<IResult3D> result_new;
            results_new->get_Item(0,&result_new);

            Float64 value_new;
            result_new->GetSingleResult(effect,&value_new);

            (*results)->SetResult(resultIdx,value_new,config);
         }
      }
   }

   return hr;
}

STDMETHODIMP CPGSuperLoadCombinationResponse::ComputeStresses(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                              /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                              /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                              /*[out,retval]*/ILoadCombinationStressResults** results)
{
   VARIANT_BOOL bComputeConfig = (computeConfig == VARIANT_TRUE || includeLiveLoad == VARIANT_TRUE) ? VARIANT_TRUE : computeConfig;

   HRESULT hr = m_LCResponseDelegate->ComputeStresses(LoadCombination, POIs, Stage,
                              summ, effect, optimization, 
                              includeLiveLoad, includeImpact, computeConfig,
                              results);

   if ( includeLiveLoad == VARIANT_TRUE )
   {
      CComPtr<IIDArray> singlePOI;
      singlePOI.CoCreateInstance(CLSID_IDArray);

      CComPtr<ILoadCombinations> loadCombos;
      m_Model->get_LoadCombinations(&loadCombos) ;
      CComPtr<ILoadCombination> loadCombo;
      loadCombos->Find(LoadCombination,&loadCombo);
      Float64 gLLmin, gLLmax;
      loadCombo->FindLoadCaseFactor(CComBSTR("LL_IM"),&gLLmin,&gLLmax);
   
      if ( gLLmin < 0 || gLLmax < 0 )
      {
         // we have to re-sum the limit state combination using the correct
         // load factor for the live load

         // for each result...
         IndexType nResults;
         (*results)->get_Count(&nResults);
         for ( IndexType resultIdx = 0; resultIdx < nResults; resultIdx++ )
         {
            singlePOI->Clear();

            CComPtr<IStressResult> left_result, right_result;
            CComPtr<ILoadCombinationResultConfiguration> left_config, right_config;

            // get the results
            (*results)->GetResult(resultIdx, &left_result, &left_config, &right_result, &right_config);
            
            Float64 gLL_left_new, gLL_right_new;
            GetNewLiveLoadFactors(LoadCombination,left_config,right_config,&gLL_left_new,&gLL_right_new);
            
            // re-do the load combination
            left_config->put_LiveLoadFactor(gLL_left_new);
            right_config->put_LiveLoadFactor(gLL_right_new);

            CComQIPtr<IConcurrentLoadCombinationResponse> concurrent_response(m_LCResponseDelegate);
            ATLASSERT(concurrent_response != nullptr);

            CComPtr<ISectionStressResults> left_results_new, right_results_new;
            PoiIDType poiID;
            POIs->get_Item(resultIdx,&poiID);
            singlePOI->Add(poiID);

            concurrent_response->ComputeStresses(singlePOI,Stage,left_config,&left_results_new);
            concurrent_response->ComputeStresses(singlePOI,Stage,right_config,&right_results_new);

            CComPtr<ISectionStressResult> left_result_new, right_result_new;
            left_results_new->get_Item(0, &left_result_new);
            right_results_new->get_Item(0,&right_result_new);

            CComPtr<IStressResult> left_stress_result, right_stress_result;
            left_result_new->CreateLeftStressResult(&left_stress_result);
            right_result_new->CreateLeftStressResult(&right_stress_result);

            (*results)->SetResult(resultIdx,left_stress_result,left_config,right_stress_result,right_config);
         }
      }
   }

   return hr;
}

void CPGSuperLoadCombinationResponse::GetNewLiveLoadFactors(BSTR bstrLoadCombination,ILoadCombinationResultConfiguration* pLeftConfig,ILoadCombinationResultConfiguration* pRightConfig,Float64* pgLL_Left,Float64* pgLL_Right)
{
#if defined _DEBUG
   Float64 gLL_left_old, gLL_right_old;
   pLeftConfig->get_LiveLoadFactor(&gLL_left_old);
   pRightConfig->get_LiveLoadFactor(&gLL_right_old);
   ATLASSERT( gLL_left_old < 0 );
   ATLASSERT( gLL_right_old < 0 );

   IndexType nLiveLoadConfigs;
   pLeftConfig->GetLiveLoadConfigurationCount(&nLiveLoadConfigs);
   ATLASSERT(nLiveLoadConfigs == 1);
   pRightConfig->GetLiveLoadConfigurationCount(&nLiveLoadConfigs);
   ATLASSERT(nLiveLoadConfigs == 1);
#endif // _DEBUG

   // get the live load configuration (index = 0)
   CComPtr<ILiveLoadConfiguration> left_live_load_config, right_live_load_config;
   pLeftConfig->GetLiveLoadConfiguration( 0,&left_live_load_config);
   pRightConfig->GetLiveLoadConfiguration(0,&right_live_load_config);

   // convert the array of used axles to axle weights
   AxleConfiguration left_axle_config, right_axle_config;
   m_pGirderModelManager->CreateAxleConfig(m_Model, left_live_load_config,  &left_axle_config);
   m_pGirderModelManager->CreateAxleConfig(m_Model, right_live_load_config, &right_axle_config);

   // sum the weights
   AxleConfiguration::iterator iter;
   Float64 Wleft = 0;
   for ( iter = left_axle_config.begin(); iter != left_axle_config.end(); iter++ )
   {
      AxlePlacement placement = *iter;
      Wleft += placement.Weight;
   }

   Float64 Wright = 0;
   for ( iter = right_axle_config.begin(); iter != right_axle_config.end(); iter++ )
   {
      AxlePlacement placement = *iter;
      Wright += placement.Weight;
   }

   // compute the new load factor
   const RatingLibraryEntry* pRatingEntry = m_pLibrary->GetRatingEntry( m_pRatingSpec->GetRatingSpecification().c_str() );
   const CLiveLoadFactorModel* pLFModel;

   // need to back out rating type from load combination name (need another back door into the agent)
   pgsTypes::LimitState ls = m_pGirderModelManager->GetLimitStateFromLoadCombination(bstrLoadCombination);
   pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);

   if ( ratingType == pgsTypes::lrPermit_Routine )
   {
      pLFModel = &pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
   }
   else if ( ratingType == pgsTypes::lrPermit_Special )
   {
      pLFModel = &pRatingEntry->GetLiveLoadFactorModel(m_pRatingSpec->GetSpecialPermitType());
   }
   else
   {
      pLFModel = &pRatingEntry->GetLiveLoadFactorModel(ratingType);
   }

   Int16 adtt = m_pRatingSpec->GetADTT();

   Float64 gLL_left  = pLFModel->GetStrengthLiveLoadFactor(adtt,Wleft);
   Float64 gLL_right = pLFModel->GetStrengthLiveLoadFactor(adtt,Wright);

   *pgLL_Left  = gLL_left;
   *pgLL_Right = gLL_right;
}