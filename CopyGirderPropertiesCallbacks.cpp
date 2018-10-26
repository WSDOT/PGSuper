///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderData.h>

txnCopyGirderType::txnCopyGirderType(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderType::~txnCopyGirderType()
{
}

bool txnCopyGirderType::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strNewName = pBridgeDesc->GetGirderGroup(m_FromGirderKey.groupIndex)->GetGirderName(m_FromGirderKey.girderIndex);

   m_strOldNames.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      std::_tstring strOldName = pBridgeDesc->GetGirderGroup(toGirderKey.groupIndex)->GetGirderName(toGirderKey.girderIndex);
      m_strOldNames.push_back(strOldName);
      pIBridgeDesc->SetGirderName(toGirderKey,strNewName.c_str() );
   }

   return true;
}

void txnCopyGirderType::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<std::_tstring>::iterator nameIter(m_strOldNames.begin());
   for ( ; iter != end; iter++, nameIter++ )
   {
      CGirderKey& toGirderKey = *iter;

      std::_tstring strOldName = *nameIter;
      pIBridgeDesc->SetGirderName(toGirderKey,strOldName.c_str() );
   }
}

txnTransaction* txnCopyGirderType::CreateClone() const
{
   return new txnCopyGirderType(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderType::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderStirrups::txnCopyGirderStirrups(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderStirrups::~txnCopyGirderStirrups()
{
}

bool txnCopyGirderStirrups::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IShear,pShear);

#pragma Reminder("UPDATE: assuming precast girder")
   CSegmentKey fromSegmentKey(m_FromGirderKey,0);
   const CShearData2* pNewShearData = pShear->GetSegmentShearData(fromSegmentKey);

   m_OldShearData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      const CShearData2* pOldShearData = pShear->GetSegmentShearData(toSegmentKey);
      m_OldShearData.push_back(*pOldShearData);
      pShear->SetSegmentShearData(toSegmentKey,*pNewShearData);
   }

   return true;
}

void txnCopyGirderStirrups::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IShear,pShear);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CShearData2>::iterator shearIter(m_OldShearData.begin());
   for ( ; iter != end; iter++, shearIter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      CShearData2& shearData = *shearIter;
      pShear->SetSegmentShearData(toSegmentKey,shearData);
   }
}

txnTransaction* txnCopyGirderStirrups::CreateClone() const
{
   return new txnCopyGirderStirrups(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderStirrups::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderPrestressing::txnCopyGirderPrestressing(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderPrestressing::~txnCopyGirderPrestressing()
{
}

bool txnCopyGirderPrestressing::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   CSegmentKey fromSegmentKey(m_FromGirderKey,0);
   const CStrandData* pNewStrandData = pSegmentData->GetStrandData(fromSegmentKey);

   m_OldPrestressData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      const CStrandData* pOldStrandData = pSegmentData->GetStrandData(toSegmentKey);
      m_OldPrestressData.push_back(*pOldStrandData);
      pSegmentData->SetStrandData(toSegmentKey,*pNewStrandData);
   }

   return true;
}

void txnCopyGirderPrestressing::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CStrandData>::iterator prestressIter(m_OldPrestressData.begin());
   for ( ; iter != end; iter++, prestressIter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      CStrandData& strandData = *prestressIter;
      pSegmentData->SetStrandData(toSegmentKey,strandData);
   }
}

txnTransaction* txnCopyGirderPrestressing::CreateClone() const
{
   return new txnCopyGirderPrestressing(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderPrestressing::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderHandling::txnCopyGirderHandling(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderHandling::~txnCopyGirderHandling()
{
}

bool txnCopyGirderHandling::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   CSegmentKey fromSegmentKey(m_FromGirderKey,0);

   const CHandlingData* pNewHandlingData = pSegmentData->GetHandlingData(fromSegmentKey);

   m_OldHandlingData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      const CHandlingData* pOldHandlingData = pSegmentData->GetHandlingData(toSegmentKey);
      m_OldHandlingData.push_back(*pOldHandlingData);
      pSegmentData->SetHandlingData(toSegmentKey,*pNewHandlingData);
   }

   return true;
}

void txnCopyGirderHandling::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CHandlingData>::iterator handlingIter(m_OldHandlingData.begin());
   for ( ; iter != end; iter++, handlingIter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);
      CHandlingData& handlingData = *handlingIter;
      pSegmentData->SetHandlingData(toSegmentKey,handlingData);
   }
}

txnTransaction* txnCopyGirderHandling::CreateClone() const
{
   return new txnCopyGirderHandling(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderHandling::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderMaterial::txnCopyGirderMaterial(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderMaterial::~txnCopyGirderMaterial()
{
}

bool txnCopyGirderMaterial::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   CSegmentKey fromSegmentKey(m_FromGirderKey,0);

   const CGirderMaterial* pNewMaterialData = pSegmentData->GetSegmentMaterial(fromSegmentKey);

   m_OldMaterialData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      const CGirderMaterial* pOldMaterialData = pSegmentData->GetSegmentMaterial(toSegmentKey);
      m_OldMaterialData.push_back(*pOldMaterialData);
      pSegmentData->SetSegmentMaterial(toSegmentKey,*pNewMaterialData);
   }

   return true;
}

void txnCopyGirderMaterial::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CGirderMaterial>::iterator materialIter(m_OldMaterialData.begin());
   for ( ; iter != end; iter++, materialIter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      CGirderMaterial& materialData = *materialIter;
      pSegmentData->SetSegmentMaterial(toSegmentKey,materialData);
   }
}

txnTransaction* txnCopyGirderMaterial::CreateClone() const
{
   return new txnCopyGirderMaterial(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderMaterial::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderRebar::txnCopyGirderRebar(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderRebar::~txnCopyGirderRebar()
{
}

bool txnCopyGirderRebar::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILongitudinalRebar,pRebar);

   CSegmentKey fromSegmentKey(m_FromGirderKey,0);
   const CLongitudinalRebarData* pNewRebarData = pRebar->GetSegmentLongitudinalRebarData(fromSegmentKey);

   m_OldRebarData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      const CLongitudinalRebarData* pOldRebarlData = pRebar->GetSegmentLongitudinalRebarData(toSegmentKey);
      m_OldRebarData.push_back(*pOldRebarlData);
      pRebar->SetSegmentLongitudinalRebarData(toSegmentKey,*pNewRebarData);
   }

   return true;
}

void txnCopyGirderRebar::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILongitudinalRebar,pRebar);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CLongitudinalRebarData>::iterator rebarIter(m_OldRebarData.begin());
   for ( ; iter != end; iter++, rebarIter++ )
   {
      CSegmentKey toSegmentKey(*iter,0);

      CLongitudinalRebarData& rebarData = *rebarIter;
      pRebar->SetSegmentLongitudinalRebarData(toSegmentKey,rebarData);
   }
}

txnTransaction* txnCopyGirderRebar::CreateClone() const
{
   return new txnCopyGirderRebar(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderRebar::Name() const
{
   return _T("");
}

//----------------------

txnCopyGirderSlabOffset::txnCopyGirderSlabOffset(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderSlabOffset::~txnCopyGirderSlabOffset()
{
}

bool txnCopyGirderSlabOffset::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_FromGirderKey.groupIndex);
   PierIndexType startPierIdx = pGroup->GetPier(pgsTypes::metStart)->GetIndex();
   PierIndexType endPierIdx   = pGroup->GetPier(pgsTypes::metEnd)->GetIndex();

   std::vector<Float64> newSlabOffsets;
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      Float64 slabOffset = pGroup->GetSlabOffset(pierIdx,m_FromGirderKey.girderIndex);
      newSlabOffsets.push_back(slabOffset);
   }

   m_OldSlabOffsetData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      std::vector<Float64> oldSlabOffsets;
      CGirderKey toGirderKey(*iter);
      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
      {
         Float64 slabOffset = pIBridgeDesc->GetSlabOffset(toGirderKey.groupIndex,pierIdx,toGirderKey.girderIndex);
         oldSlabOffsets.push_back(slabOffset);

         pIBridgeDesc->SetSlabOffset(toGirderKey.groupIndex,pierIdx,toGirderKey.girderIndex,newSlabOffsets[pierIdx-startPierIdx]);
      }
      m_OldSlabOffsetData.push_back(oldSlabOffsets);
   }

   return true;
}

void txnCopyGirderSlabOffset::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<std::vector<Float64>>::iterator slabOffsetIter(m_OldSlabOffsetData.begin());
   for ( ; iter != end; iter++, slabOffsetIter++)
   {
      CGirderKey toGirderKey(*iter);
      std::vector<Float64> slabOffsets = *slabOffsetIter;

      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(toGirderKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPier(pgsTypes::metStart)->GetIndex();
      PierIndexType endPierIdx   = pGroup->GetPier(pgsTypes::metEnd)->GetIndex();
      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
      {
         Float64 slabOffset = slabOffsets[pierIdx-startPierIdx];
         pIBridgeDesc->SetSlabOffset(toGirderKey.groupIndex,pierIdx,toGirderKey.girderIndex,slabOffset);
      }
   }
}

txnTransaction* txnCopyGirderSlabOffset::CreateClone() const
{
   return new txnCopyGirderSlabOffset(m_FromGirderKey,m_ToGirderKeys);
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

BOOL CCopyGirderType::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->UseSameGirderForEntireBridge() ? FALSE : TRUE;
}

txnTransaction* CCopyGirderType::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderType(fromGirderKey,toGirderKeys);
}

///////////////////////////////////////////////////////////////////////////

CCopyGirderStirrups::CCopyGirderStirrups()
{
}

LPCTSTR CCopyGirderStirrups::GetName()
{
   return _T("Copy Transverse Reinforcement");
}

BOOL CCopyGirderStirrups::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return TRUE;
}

txnTransaction* CCopyGirderStirrups::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderStirrups(fromGirderKey,toGirderKeys);
}

///////////////////////////////////////////////////////////////////////////

CCopyGirderPrestressing::CCopyGirderPrestressing()
{
}

LPCTSTR CCopyGirderPrestressing::GetName()
{
   return _T("Copy Prestressing");
}

BOOL CCopyGirderPrestressing::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   // if the source and any of the destination girders are not the same type
   // the prestressing and longitudinal reinforcement data must be copied
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::_tstring strFromGirder = pBridgeDesc->GetGirderGroup(fromGirderKey.groupIndex)->GetGirderName(fromGirderKey.girderIndex);

   BOOL bCanCopy = TRUE;

   std::vector<CGirderKey>::const_iterator iter;
   for ( iter = toGirderKeys.begin(); iter != toGirderKeys.end(); iter++ )
   {
      CGirderKey toGirderKey(*iter);

      std::_tstring strToGirder = pBridgeDesc->GetGirderGroup(toGirderKey.groupIndex)->GetGirderName(toGirderKey.girderIndex);

      if ( strFromGirder != strToGirder )
      {
         return FALSE;
      }
   }

   return TRUE;
}

txnTransaction* CCopyGirderPrestressing::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderPrestressing(fromGirderKey,toGirderKeys);
}

////////////////////////////////////////////////////////////////////////////

CCopyGirderHandling::CCopyGirderHandling()
{
}

LPCTSTR CCopyGirderHandling::GetName()
{
   return _T("Copy Handling Locations");
}

BOOL CCopyGirderHandling::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return TRUE;
}

txnTransaction* CCopyGirderHandling::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderHandling(fromGirderKey,toGirderKeys);
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderMaterial::CCopyGirderMaterial()
{
}

LPCTSTR CCopyGirderMaterial::GetName()
{
   return _T("Copy Concrete Material Properties");
}

BOOL CCopyGirderMaterial::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return TRUE;
}

txnTransaction* CCopyGirderMaterial::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderMaterial(fromGirderKey,toGirderKeys);
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderRebar::CCopyGirderRebar()
{
}

LPCTSTR CCopyGirderRebar::GetName()
{
   return _T("Copy Longitudinal Reinforcement");
}

BOOL CCopyGirderRebar::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   // if the source and any of the destination girders are not the same type
   // the prestressing and longitudinal reinforcement data must be copied
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::_tstring strFromGirder = pBridgeDesc->GetGirderGroup(fromGirderKey.groupIndex)->GetGirderName(fromGirderKey.girderIndex);

   BOOL bCanCopy = TRUE;

   std::vector<CGirderKey>::const_iterator iter;
   for ( iter = toGirderKeys.begin(); iter != toGirderKeys.end(); iter++ )
   {
      CGirderKey toGirderKey(*iter);

      std::_tstring strToGirder = pBridgeDesc->GetGirderGroup(toGirderKey.groupIndex)->GetGirderName(toGirderKey.girderIndex);

      if ( strFromGirder != strToGirder )
      {
         return FALSE;
      }
   }

   return TRUE;
}

txnTransaction* CCopyGirderRebar::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderRebar(fromGirderKey,toGirderKeys);
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderSlabOffset::CCopyGirderSlabOffset()
{
}

LPCTSTR CCopyGirderSlabOffset::GetName()
{
   return _T("Copy Slab Offset (\"A\" Dimensions)");
}

BOOL CCopyGirderSlabOffset::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotGirder ? TRUE : FALSE;
}

txnTransaction* CCopyGirderSlabOffset::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderSlabOffset(fromGirderKey,toGirderKeys);
}

