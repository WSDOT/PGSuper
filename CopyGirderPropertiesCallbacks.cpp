///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "CopyGirderPropertiesCallbacks.h"

#include <EAF\EAFUtilities.h>

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>

txnCopyGirderType::txnCopyGirderType(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderType::~txnCopyGirderType()
{
}

bool txnCopyGirderType::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strNewName = pBridgeDesc->GetSpan(fromSpanIdx)->GetGirderTypes()->GetGirderName(fromGirderIdx);

   m_strOldNames.clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      std::_tstring strOldName = pBridgeDesc->GetSpan(toSpanIdx)->GetGirderTypes()->GetGirderName(toGirderIdx);
      m_strOldNames.push_back(strOldName);
      pIBridgeDesc->SetGirderName(toSpanIdx,toGirderIdx,strNewName.c_str() );
   }

   return true;
}

void txnCopyGirderType::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<std::_tstring>::iterator nameIter(m_strOldNames.begin());
   for ( ; iter != end; iter++, nameIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      std::_tstring strOldName = *nameIter;
      pIBridgeDesc->SetGirderName(toSpanIdx,toGirderIdx,strOldName.c_str() );
   }
}

txnTransaction* txnCopyGirderType::CreateClone() const
{
   return new txnCopyGirderType(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderType::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderStirrups::txnCopyGirderStirrups(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderStirrups::~txnCopyGirderStirrups()
{
}

bool txnCopyGirderStirrups::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IShear,pShear);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CShearData& newShearData = pBridgeDesc->GetSpan(fromSpanIdx)->GetGirderTypes()->GetGirderData(fromGirderIdx).ShearData;

   m_OldShearData.clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      const CShearData& oldShearData = pBridgeDesc->GetSpan(toSpanIdx)->GetGirderTypes()->GetGirderData(toGirderIdx).ShearData;
      m_OldShearData.push_back(oldShearData);
      pShear->SetShearData(newShearData,toSpanIdx,toGirderIdx);
   }

   return true;
}

void txnCopyGirderStirrups::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IShear,pShear);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<CShearData>::iterator shearIter(m_OldShearData.begin());
   for ( ; iter != end; iter++, shearIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      CShearData& shearData = *shearIter;
      pShear->SetShearData(shearData,toSpanIdx,toGirderIdx);
   }
}

txnTransaction* txnCopyGirderStirrups::CreateClone() const
{
   return new txnCopyGirderStirrups(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderStirrups::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderPrestressing::txnCopyGirderPrestressing(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderPrestressing::~txnCopyGirderPrestressing()
{
}

bool txnCopyGirderPrestressing::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPrestressData& newPrestressData = pBridgeDesc->GetSpan(fromSpanIdx)->GetGirderTypes()->GetGirderData(fromGirderIdx).PrestressData;

   m_OldPrestressData.clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      const CPrestressData& oldPrestressData = pBridgeDesc->GetSpan(toSpanIdx)->GetGirderTypes()->GetGirderData(toGirderIdx).PrestressData;
      m_OldPrestressData.push_back(oldPrestressData);
      pGirderData->SetPrestressData(newPrestressData,toSpanIdx,toGirderIdx);
   }

   return true;
}

void txnCopyGirderPrestressing::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<CPrestressData>::iterator prestressIter(m_OldPrestressData.begin());
   for ( ; iter != end; iter++, prestressIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      CPrestressData& prestressData = *prestressIter;
      pGirderData->SetPrestressData(prestressData,toSpanIdx,toGirderIdx);
   }
}

txnTransaction* txnCopyGirderPrestressing::CreateClone() const
{
   return new txnCopyGirderPrestressing(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderPrestressing::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderHandling::txnCopyGirderHandling(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderHandling::~txnCopyGirderHandling()
{
}

bool txnCopyGirderHandling::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CHandlingData& newHandlingData = pBridgeDesc->GetSpan(fromSpanIdx)->GetGirderTypes()->GetGirderData(fromGirderIdx).HandlingData;

   m_OldHandlingData.clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      const CHandlingData& oldHandlingData = pBridgeDesc->GetSpan(toSpanIdx)->GetGirderTypes()->GetGirderData(toGirderIdx).HandlingData;
      m_OldHandlingData.push_back(oldHandlingData);
      pGirderData->SetHandlingData(newHandlingData,toSpanIdx,toGirderIdx);
   }

   return true;
}

void txnCopyGirderHandling::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<CHandlingData>::iterator handlingIter(m_OldHandlingData.begin());
   for ( ; iter != end; iter++, handlingIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      CHandlingData& handlingData = *handlingIter;
      pGirderData->SetHandlingData(handlingData,toSpanIdx,toGirderIdx);
   }
}

txnTransaction* txnCopyGirderHandling::CreateClone() const
{
   return new txnCopyGirderHandling(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderHandling::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderMaterial::txnCopyGirderMaterial(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderMaterial::~txnCopyGirderMaterial()
{
}

bool txnCopyGirderMaterial::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderMaterial& newMaterialData = pBridgeDesc->GetSpan(fromSpanIdx)->GetGirderTypes()->GetGirderData(fromGirderIdx).Material;

   m_OldMaterialData.clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      const CGirderMaterial& oldMaterialData = pBridgeDesc->GetSpan(toSpanIdx)->GetGirderTypes()->GetGirderData(toGirderIdx).Material;
      m_OldMaterialData.push_back(oldMaterialData);
      pGirderData->SetGirderMaterial(toSpanIdx,toGirderIdx,newMaterialData);
   }

   return true;
}

void txnCopyGirderMaterial::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<CGirderMaterial>::iterator materialIter(m_OldMaterialData.begin());
   for ( ; iter != end; iter++, materialIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      CGirderMaterial& materialData = *materialIter;
      pGirderData->SetGirderMaterial(toSpanIdx,toGirderIdx,materialData);
   }
}

txnTransaction* txnCopyGirderMaterial::CreateClone() const
{
   return new txnCopyGirderMaterial(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderMaterial::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderRebar::txnCopyGirderRebar(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderRebar::~txnCopyGirderRebar()
{
}

bool txnCopyGirderRebar::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,ILongitudinalRebar,pRebar);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CLongitudinalRebarData& newRebarData = pBridgeDesc->GetSpan(fromSpanIdx)->GetGirderTypes()->GetGirderData(fromGirderIdx).LongitudinalRebarData;

   m_OldRebarData.clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      const CLongitudinalRebarData& oldRebarlData = pBridgeDesc->GetSpan(toSpanIdx)->GetGirderTypes()->GetGirderData(toGirderIdx).LongitudinalRebarData;
      m_OldRebarData.push_back(oldRebarlData);
      pRebar->SetLongitudinalRebarData(newRebarData,toSpanIdx,toGirderIdx);
   }

   return true;
}

void txnCopyGirderRebar::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILongitudinalRebar,pRebar);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<CLongitudinalRebarData>::iterator rebarIter(m_OldRebarData.begin());
   for ( ; iter != end; iter++, rebarIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      CLongitudinalRebarData& rebarData = *rebarIter;
      pRebar->SetLongitudinalRebarData(rebarData,toSpanIdx,toGirderIdx);
   }
}

txnTransaction* txnCopyGirderRebar::CreateClone() const
{
   return new txnCopyGirderRebar(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderRebar::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderSlabOffset::txnCopyGirderSlabOffset(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   m_FromSpanGirderHashValue = fromSpanGirderHashValue;
   m_ToSpanGirderHashValues  = toSpanGirderHashValues;
}

txnCopyGirderSlabOffset::~txnCopyGirderSlabOffset()
{
}

bool txnCopyGirderSlabOffset::Execute()
{
   SpanIndexType fromSpanIdx;
   GirderIndexType fromGirderIdx;
   UnhashSpanGirder(m_FromSpanGirderHashValue,&fromSpanIdx,&fromGirderIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   Float64 newSlabOffset[2];
   pIBridgeDesc->GetSlabOffset(fromSpanIdx,fromGirderIdx,&newSlabOffset[pgsTypes::metStart],&newSlabOffset[pgsTypes::metEnd]);

   m_OldSlabOffsetData[pgsTypes::metStart].clear();
   m_OldSlabOffsetData[pgsTypes::metEnd].clear();
   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   for ( ; iter != end; iter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      Float64 oldSlabOffset[2];
      pIBridgeDesc->GetSlabOffset(toSpanIdx,toGirderIdx,&oldSlabOffset[pgsTypes::metStart],&oldSlabOffset[pgsTypes::metEnd]);
      m_OldSlabOffsetData[pgsTypes::metStart].push_back(oldSlabOffset[pgsTypes::metStart]);
      m_OldSlabOffsetData[pgsTypes::metEnd  ].push_back(oldSlabOffset[pgsTypes::metEnd]);

      pIBridgeDesc->SetSlabOffset(toSpanIdx,toGirderIdx,newSlabOffset[pgsTypes::metStart],newSlabOffset[pgsTypes::metEnd]);
   }

   return true;
}

void txnCopyGirderSlabOffset::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::vector<SpanGirderHashType>::iterator iter(m_ToSpanGirderHashValues.begin());
   std::vector<SpanGirderHashType>::iterator end(m_ToSpanGirderHashValues.end());
   std::vector<Float64>::iterator startSlabOffsetIter(m_OldSlabOffsetData[pgsTypes::metStart].begin());
   std::vector<Float64>::iterator endSlabOffsetIter(m_OldSlabOffsetData[pgsTypes::metEnd].begin());
   for ( ; iter != end; iter++, startSlabOffsetIter++, endSlabOffsetIter++ )
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType toSpanIdx;
      GirderIndexType toGirderIdx;
      UnhashSpanGirder(hashval,&toSpanIdx,&toGirderIdx);

      Float64 slabOffset[2];
      slabOffset[pgsTypes::metStart] = *startSlabOffsetIter;
      slabOffset[pgsTypes::metEnd  ] = *endSlabOffsetIter;

      pIBridgeDesc->SetSlabOffset(toSpanIdx,toGirderIdx,slabOffset[pgsTypes::metStart],slabOffset[pgsTypes::metEnd]);
   }
}

txnTransaction* txnCopyGirderSlabOffset::CreateClone() const
{
   return new txnCopyGirderSlabOffset(m_FromSpanGirderHashValue,m_ToSpanGirderHashValues);
}

std::_tstring txnCopyGirderSlabOffset::Name() const
{
   return _T("");
}

//----------------------

CCopyGirderType::CCopyGirderType()
{
}

LPCTSTR CCopyGirderType::GetName()
{
   return _T("Copy Girder Type");
}

BOOL CCopyGirderType::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->UseSameGirderForEntireBridge() ? FALSE : TRUE;
}

txnTransaction* CCopyGirderType::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderType(fromSpanGirderHashValue,toSpanGirderHashValues);
}

///////////////////////////////////////////////////////////////////////////

CCopyGirderStirrups::CCopyGirderStirrups()
{
}

LPCTSTR CCopyGirderStirrups::GetName()
{
   return _T("Copy Transverse Reinforcement");
}

BOOL CCopyGirderStirrups::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return TRUE;
}

txnTransaction* CCopyGirderStirrups::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderStirrups(fromSpanGirderHashValue,toSpanGirderHashValues);
}

///////////////////////////////////////////////////////////////////////////

CCopyGirderPrestressing::CCopyGirderPrestressing()
{
}

LPCTSTR CCopyGirderPrestressing::GetName()
{
   return _T("Copy Prestressing");
}

BOOL CCopyGirderPrestressing::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   // if the source and any of the destination girders are not the same type
   // the prestressing and longitudinal reinforcement data must be copied

   SpanIndexType fromSpan;
   GirderIndexType fromGirder;
   UnhashSpanGirder(fromSpanGirderHashValue,&fromSpan,&fromGirder);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::_tstring strFromGirder = pBridgeDesc->GetSpan(fromSpan)->GetGirderTypes()->GetGirderName(fromGirder);

   BOOL bCanCopy = TRUE;

   std::vector<SpanGirderHashType>::const_iterator iter;
   for ( iter = toSpanGirderHashValues.begin(); iter != toSpanGirderHashValues.end(); iter++ )
   {
      SpanGirderHashType dwTo = *iter;
      SpanIndexType toSpan;
      GirderIndexType toGirder;
      UnhashSpanGirder(dwTo,&toSpan,&toGirder);
      std::_tstring strToGirder = pBridgeDesc->GetSpan(toSpan)->GetGirderTypes()->GetGirderName(toGirder);

      if ( strFromGirder != strToGirder )
      {
         return FALSE;
      }
   }

   return TRUE;
}

txnTransaction* CCopyGirderPrestressing::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderPrestressing(fromSpanGirderHashValue,toSpanGirderHashValues);
}

////////////////////////////////////////////////////////////////////////////

CCopyGirderHandling::CCopyGirderHandling()
{
}

LPCTSTR CCopyGirderHandling::GetName()
{
   return _T("Copy Handling Locations");
}

BOOL CCopyGirderHandling::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return TRUE;
}

txnTransaction* CCopyGirderHandling::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderHandling(fromSpanGirderHashValue,toSpanGirderHashValues);
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderMaterial::CCopyGirderMaterial()
{
}

LPCTSTR CCopyGirderMaterial::GetName()
{
   return _T("Copy Concrete Material Properties");
}

BOOL CCopyGirderMaterial::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return TRUE;
}

txnTransaction* CCopyGirderMaterial::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderMaterial(fromSpanGirderHashValue,toSpanGirderHashValues);
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderRebar::CCopyGirderRebar()
{
}

LPCTSTR CCopyGirderRebar::GetName()
{
   return _T("Copy Longitudinal Reinforcement");
}

BOOL CCopyGirderRebar::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   // if the source and any of the destination girders are not the same type
   // the prestressing and longitudinal reinforcement data must be copied

   SpanIndexType fromSpan;
   GirderIndexType fromGirder;
   UnhashSpanGirder(fromSpanGirderHashValue,&fromSpan,&fromGirder);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::_tstring strFromGirder = pBridgeDesc->GetSpan(fromSpan)->GetGirderTypes()->GetGirderName(fromGirder);

   BOOL bCanCopy = TRUE;

   std::vector<SpanGirderHashType>::const_iterator iter;
   for ( iter = toSpanGirderHashValues.begin(); iter != toSpanGirderHashValues.end(); iter++ )
   {
      SpanGirderHashType dwTo = *iter;
      SpanIndexType toSpan;
      GirderIndexType toGirder;
      UnhashSpanGirder(dwTo,&toSpan,&toGirder);
      std::_tstring strToGirder = pBridgeDesc->GetSpan(toSpan)->GetGirderTypes()->GetGirderName(toGirder);

      if ( strFromGirder != strToGirder )
      {
         return FALSE;
      }
   }

   return TRUE;
}

txnTransaction* CCopyGirderRebar::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderRebar(fromSpanGirderHashValue,toSpanGirderHashValues);
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderSlabOffset::CCopyGirderSlabOffset()
{
}

LPCTSTR CCopyGirderSlabOffset::GetName()
{
   return _T("Copy Slab Offset (\"A\" Dimensions)");
}

BOOL CCopyGirderSlabOffset::CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotGirder ? TRUE : FALSE;
}

txnTransaction* CCopyGirderSlabOffset::CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues)
{
   return new txnCopyGirderSlabOffset(fromSpanGirderHashValue,toSpanGirderHashValues);
}

