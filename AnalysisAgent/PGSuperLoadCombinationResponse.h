///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

/**********************************************************************
The LBAM engine uses a single load factor when performing load combinations.
This works every time except for permit live loads in the Routine/Annual
permit type. For this case, the load factor is a function of ADTT and
the weight of the axles on the bridge. Since the live load is a moving
load analysis, the weight of the axles on the bridge changes as the truck
moves. This means, for each maximum/minimum value, the load factor could be
different at each location in the bridge. 

The CPGSuperLoadCombinatResponse is a specialized load combination response
object that alters the combination by dividing out the "static" live load factor
and using the live load factor computed based on truck position, ADTT,
and the weight of the axles on the bridge.

See MBE Table 6A.4.5.4.2a-1 for permit live load factors
***********************************************************************/

#include <WBFLLBAMLoadCombiner.h>
#include <WBFLLBAMLiveLoader.h>

#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

// {169DC9D7-9EE2-42ff-BEDA-B8896A489C30}
DEFINE_GUID(CLSID_PGSuperLoadCombinationResponse, 
0x169dc9d7, 0x9ee2, 0x42ff, 0xbe, 0xda, 0xb8, 0x89, 0x6a, 0x48, 0x9c, 0x30);

class CGirderModelManager;

class ATL_NO_VTABLE CPGSuperLoadCombinationResponse : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSuperLoadCombinationResponse, &CLSID_PGSuperLoadCombinationResponse>,
   public ILoadCombinationResponse
{
public:
   CPGSuperLoadCombinationResponse()
   {
   }

   virtual ~CPGSuperLoadCombinationResponse()
   {
   }

   void Initialize(ILoadCombinationResponse* pLCResponse,ILoadGroupResponse* pLGResponse,ILiveLoadModelResponse* pLLResponse,ILBAMModel* pModel,const CGirderModelManager* pModelManager);

   HRESULT FinalConstruct();
   void FinalRelease();

DECLARE_NOT_AGGREGATABLE(CPGSuperLoadCombinationResponse)

BEGIN_COM_MAP(CPGSuperLoadCombinationResponse)
   COM_INTERFACE_ENTRY(ILoadCombinationResponse)
END_COM_MAP()


// ILoadCombinationResponse
public:
   STDMETHOD(ComputeForces)(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage, /*[in]*/ResultsOrientation orientation, 
                            /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                            /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                            /*[out,retval]*/ILoadCombinationSectionResults** results);

   STDMETHOD(ComputeDeflections)(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                                 /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                                 /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                                 /*[out,retval]*/ILoadCombinationSectionResults** results);

   STDMETHOD(ComputeReactions)(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                               /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                               /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                               /*[out,retval]*/ILoadCombinationResults** results);

   STDMETHOD(ComputeSupportDeflections)(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                                        /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                                        /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                                        /*[out,retval]*/ILoadCombinationResults** results);

   STDMETHOD(ComputeStresses)(/*[in]*/BSTR LoadCombination, /*[in]*/IIDArray* POIs, /*[in]*/BSTR Stage,
                              /*[in]*/ResultsSummationType summ, /*[in]*/ForceEffectType effect, /*[in]*/OptimizationType optimization, 
                              /*[in]*/VARIANT_BOOL includeLiveLoad, /*[in]*/VARIANT_BOOL includeImpact, /*[in]*/VARIANT_BOOL computeConfig,
                              /*[out,retval]*/ILoadCombinationStressResults** results);

private:
   CComPtr<ILoadCombinationResponse> m_LCResponseDelegate; // delegate to this object except in the gLL < 0 special case
   CComPtr<ILiveLoadModelResponse> m_LiveLoadResponse;
   CComPtr<ILoadGroupResponse> m_LoadGroupResponse;
   CComPtr<ILBAMModel> m_Model;
   CComPtr<IPOIs> m_POIs;
   const CGirderModelManager* m_pGirderModelManager;
   CComPtr<ILibrary> m_pLibrary;
   CComPtr<IRatingSpecification> m_pRatingSpec;

   void GetNewLiveLoadFactors(BSTR bstrLoadCombination,ILoadCombinationResultConfiguration* pLeftConfig,ILoadCombinationResultConfiguration* pRightConfig,Float64* pgLL_Left,Float64* pgLL_Right);
};
