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

#include <PgsExt\PoiMap.h>

class CGirderModelManager;

class CGirderModelData
{
public:
   CGirderModelData(const CGirderModelManager *pParent,GirderIndexType gdrLineIdx);
   CGirderModelData(const CGirderModelData& other);
   ~CGirderModelData();
   void CreateAnalysisEngine(ILBAMModel* theModel,pgsTypes::BridgeAnalysisType bat,ILBAMAnalysisEngine** ppEngine);
   void AddSimpleModel(ILBAMModel* pModel);
   void AddContinuousModel(ILBAMModel* pContModel);
   void operator=(const CGirderModelData& other);

   const CGirderModelManager* m_pParent;
   GirderIndexType m_GirderLineIndex;
   bool m_bSimpleModelHasPretensionLoad;
   bool m_bContinuousModelHasPretensionLoad;

   // Model and response engines for the bridge site force analysis
   CComPtr<ILBAMModel>               m_Model;
   CComPtr<ILBAMModel>               m_ContinuousModel;
   CComPtr<ILBAMModelEnveloper>      m_MinModelEnveloper;
   CComPtr<ILBAMModelEnveloper>      m_MaxModelEnveloper;

   // Index is pgsTypes::SimpleSpan or pgsTypes::ContinuousSpan
   CComPtr<ILoadGroupResponse>       pLoadGroupResponse[2]; // Girder, Traffic Barrier, etc
   CComPtr<ILoadCaseResponse>        pLoadCaseResponse[2];  // DC, DW, etc
   CComPtr<IContraflexureResponse>   pContraflexureResponse[2];

   // first index is force effect: fetFx, fetFy, fetMz
   // second index is optimization: optMinimize, optMaximize
   CComPtr<ILoadGroupResponse>       pMinLoadGroupResponseEnvelope[3][2]; // Girder, Traffic Barrier, etc
   CComPtr<ILoadGroupResponse>       pMaxLoadGroupResponseEnvelope[3][2]; // Girder, Traffic Barrier, etc
   CComPtr<ILoadCaseResponse>        pMinLoadCaseResponseEnvelope[3][2];  // DC, DW, etc
   CComPtr<ILoadCaseResponse>        pMaxLoadCaseResponseEnvelope[3][2];  // DC, DW, etc

   // Index is pgsTypes::SimpleSpan, pgsTypes::ContinuousSpan, pgsTypes::MinSimpleContinousEnvelope, pgsTypes::MaxSimpleContinuousEnvelope
   CComPtr<ILiveLoadModelResponse>   pLiveLoadResponse[4];  // LL+IM
   CComPtr<ILoadCombinationResponse> pLoadComboResponse[4]; // Service I, Strength I, etc

   // Index is pgsTypes::SimpleSpan or pgsTypes::ContinuousSpan
   CComPtr<IConcurrentLoadCombinationResponse> pConcurrentComboResponse[2];
   
   // Index is pgsTypes::SimpleSpan, pgsTypes::ContinuousSpan, pgsTypes::MinSimpleContinousEnvelope, pgsTypes::MaxSimpleContinuousEnvelope
   CComPtr<IEnvelopedVehicularResponse> pVehicularResponse[4];

   // response engines for the deflection analysis
   // Index is pgsTypes::SimpleSpan or pgsTypes::ContinuousSpan
   CComPtr<ILoadGroupResponse>          pDeflLoadGroupResponse[2];
   CComPtr<ILiveLoadModelResponse>      pDeflLiveLoadResponse[2];
   CComPtr<IEnvelopedVehicularResponse> pDeflEnvelopedVehicularResponse[2];
   CComPtr<IContraflexureResponse>      pDeflContraflexureResponse[2]; 

   pgsPoiMap PoiMap;
};
