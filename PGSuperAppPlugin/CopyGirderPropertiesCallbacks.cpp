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
#include "CopyGirderPropertiesCallbacks.h"

#include <EAF\EAFUtilities.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <IFace\DocumentType.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\GirderHandling.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\GirderLabel.h>

#include <Reporter\ReportingUtils.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////// reporting utilities
inline void ColorFromRow(rptRcTable* p_table, RowIndexType row, ColumnIndexType nCols)
{
   // Color background of From girder row
   for (ColumnIndexType col = 0; col < nCols; col++)
   {
      rptRiStyle::FontColor color = rptRiStyle::Yellow;
      (*p_table)(row, col) << new rptRcBgColor(color);
      (*p_table)(row, col).SetFillBackGroundColor(color);
   }
}

// Common function to write comparison cell
inline void WriteCompareCell(rptRcTable* p_table, RowIndexType row, ColumnIndexType col, bool isFrom, bool isEqual)
{
   if (!isFrom)
   {
      if (isEqual)
      {
         (*p_table)(row, col) << color(Green) << _T("Yes") << color(Black);
      }
      else
      {
         (*p_table)(row, col) << color(Red) << _T("No") << color(Black);
      }
   }
   else
   {
      (*p_table)(row, col) << symbol(NBSP);
   }
}

inline const CString& GetStrandDefinitionType(pgsTypes::StrandDefinitionType strandDefinitionType,pgsTypes::AdjustableStrandType adjustableStrandType)
{
   static std::array<CString, 5> strStrandDefTypes{
      _T("# of Permanent Strands"),
      _T("Figure it out"),
      _T("Strand Locations"),
      _T("Strand Rows"),
      _T("Individual Strands")
   };

   static CString strAdjHarped(_T("# of Straight and Harped"));
   static CString strAdjStraight(_T("# of Straight and Adjustable Straight"));
   static CString strAdjStraightHarped(_T("# of Straight and # of Adjustable"));

   if (strandDefinitionType == pgsTypes::sdtStraightHarped)
   {
      return adjustableStrandType == pgsTypes::asHarped ? strAdjHarped : (adjustableStrandType == pgsTypes::asStraight ? strAdjStraight : strAdjStraightHarped);
   }
   else
   {
      return strStrandDefTypes[strandDefinitionType];
   }
}

inline LPTSTR GetDuctMaterialStr(pgsTypes::DuctType dtype)
{
   switch (dtype)
   {
   case pgsTypes::dtMetal:
      return _T("Galvanized ferrous metal");
   case pgsTypes::dtPlastic:
      return _T("Polyethylene");
   case pgsTypes::dtFormed:
      return _T("Formed in concrete with removable cores");
   }

   return _T("Invalid");
}

inline LPTSTR GetInstallEventStr(pgsTypes::SegmentPTEventType etype)
{
   switch (etype)
   {
   case pgsTypes::sptetRelease:
      return _T("Immediately after release");
   case pgsTypes::sptetStorage:
      return _T("At beginning of storage");
   case pgsTypes::sptetHauling:
      return _T("Immediately before hauling");
   }

   return _T("Invalid");
}

inline LPTSTR GetJackEndStr(pgsTypes::JackingEndType etype)
{
   switch (etype)
   {
   case pgsTypes::jeStart:
      return _T("Start");
   case pgsTypes::jeEnd:
      return _T("End");
   case pgsTypes::jeBoth:
      return _T("Both");
   }

   return _T("Invalid");
}
   
inline LPTSTR GetFaceTypeStr(pgsTypes::FaceType ftype)
{
   switch (ftype)
   {
   case pgsTypes::TopFace:
      return _T("Top");
   case pgsTypes::BottomFace:
      return _T("Bottom");
   }

   return _T("Invalid");
}

inline LPTSTR GetRebarLayoutTypeStr(pgsTypes::RebarLayoutType layoutType)
{
   switch(layoutType)
   {
   case pgsTypes::blFromLeft:
      return _T("Left End");

   case pgsTypes::blFromRight:
      return _T("Right End");

   case pgsTypes::blFullLength:
      return _T("Full Length");

   case pgsTypes::blMidGirderLength:
      return _T("Mid-Girder-Length");

   case pgsTypes::blMidGirderEnds:
      return _T("Mid-Girder-Ends");
   }
   ATLASSERT(false);
   return _T("Invalid");
}


// Declaration of girder comparison reports
void GirderAllPropertiesComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, const CGirderKey& fromGirderKey);
void GirderMaterialsComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, const CGirderKey& fromGirderKey);
bool GirderPrestressingComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void debonding(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void GirderSegmentPostensioningComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void GirderGroupPostensioningComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void GirderLongRebarComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void GirderPrimaryStirrupComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void GirderSecondaryStirrupComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);
void GirderHandlingComparison(rptParagraph* pPara, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey);

////////////////////////////////////////////////////
//////////////////// Transaction Classes ////////////
////////////////////////////////////////////////////

txnCopyGirderAllProperties::txnCopyGirderAllProperties(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   m_FromGirderKey = fromGirderKey;
   m_ToGirderKeys  = toGirderKeys;
}

txnCopyGirderAllProperties::~txnCopyGirderAllProperties()
{
}

bool txnCopyGirderAllProperties::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   m_strOldNames.clear();

   // May need to change library entry type as well as data
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strFromName = pBridgeDesc->GetGirderGroup(m_FromGirderKey.groupIndex)->GetGirderName(m_FromGirderKey.girderIndex);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      std::_tstring strOldName = pBridgeDesc->GetGirderGroup(toGirderKey.groupIndex)->GetGirderName(toGirderKey.girderIndex);
      m_strOldNames.push_back(strOldName);

      // Only actually set data if from!=to and name actually changes
      if (toGirderKey != m_FromGirderKey && strOldName != strFromName)
      {
         // Set to new library entry
         pIBridgeDesc->SetGirderName(toGirderKey,strFromName.c_str() );
      }
   }

   return true;
}

void txnCopyGirderAllProperties::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // May need to change library entry type as well as data
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strFromName = pBridgeDesc->GetGirderGroup(m_FromGirderKey.groupIndex)->GetGirderName(m_FromGirderKey.girderIndex);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<std::_tstring>::iterator nameIter(m_strOldNames.begin());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      const std::_tstring& strOldName = *nameIter;

      std::_tstring strName = pBridgeDesc->GetGirderGroup(toGirderKey.groupIndex)->GetGirderName(toGirderKey.girderIndex);

      // Only actually set data if from!=to and name actually changes
      if (toGirderKey != m_FromGirderKey && strOldName != strName)
      {
         // Set to new library entry
         pIBridgeDesc->SetGirderName(toGirderKey,strName.c_str() );
      }

      nameIter++;
   }
}

txnTransaction* txnCopyGirderAllProperties::CreateClone() const
{
   return new txnCopyGirderAllProperties(m_FromGirderKey,m_ToGirderKeys);
}

std::_tstring txnCopyGirderAllProperties::Name() const
{
   return _T("");
}

//----------------------

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
   GET_IFACE2(pBroker,IBridge,pBridge);

   CSegmentKey fromSegmentKey(m_FromGirderKey,0);
   const CShearData2* pNewShearData = pShear->GetSegmentShearData(fromSegmentKey);

   m_OldShearData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         const CShearData2* pOldShearData = pShear->GetSegmentShearData(toSegmentKey);
         m_OldShearData.push_back(*pOldShearData);
         pShear->SetSegmentShearData(toSegmentKey,*pNewShearData);
      }
   }

   return true;
}

void txnCopyGirderStirrups::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IShear,pShear);
   GET_IFACE2(pBroker,IBridge,pBridge);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CShearData2>::iterator shearIter(m_OldShearData.begin());
   for ( ; iter != end; iter++, shearIter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         CShearData2& shearData = *shearIter;
         pShear->SetSegmentShearData(toSegmentKey,shearData);
      }
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
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   m_OldPrestressData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      if (toGirderKey != m_FromGirderKey)
      {
         // pretensioning
         SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
         ATLASSERT(nSegments == pBridge->GetSegmentCount(m_FromGirderKey));
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey fromSegmentKey(m_FromGirderKey, segIdx);
            const CStrandData* pNewStrandData = pSegmentData->GetStrandData(fromSegmentKey);
            const CSegmentPTData* pNewSegmentPTData = pSegmentData->GetSegmentPTData(fromSegmentKey);

            CSegmentKey toSegmentKey(toGirderKey, segIdx);

            const CStrandData* pOldStrandData = pSegmentData->GetStrandData(toSegmentKey);
            m_OldPrestressData.push_back(*pOldStrandData);
            pSegmentData->SetStrandData(toSegmentKey, *pNewStrandData);

            const CSegmentPTData* pOldSegmentPTData = pSegmentData->GetSegmentPTData(toSegmentKey);
            m_OldSegmentPTData.push_back(*pOldSegmentPTData);
            pSegmentData->SetSegmentPTData(toSegmentKey, *pNewSegmentPTData);

         }

         // post tensioning
         const CPTData* pNewPTData = pIBridgeDesc->GetPostTensioning(m_FromGirderKey);
         m_OldPTData.push_back(*pNewPTData);
         pIBridgeDesc->SetPostTensioning(toGirderKey, *pNewPTData);
      }
   }

   return true;
}

void txnCopyGirderPrestressing::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CStrandData>::iterator prestressIter(m_OldPrestressData.begin());
   std::vector<CSegmentPTData>::iterator segmentPTIter(m_OldSegmentPTData.begin());
   std::vector<CPTData>::iterator ptIter(m_OldPTData.begin());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      if (toGirderKey != m_FromGirderKey)
      {
         // pre tensioning
         SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey toSegmentKey(toGirderKey, segIdx);

            CStrandData& strandData = *prestressIter;
            pSegmentData->SetStrandData(toSegmentKey, strandData);

            CSegmentPTData& ptData = *segmentPTIter;
            pSegmentData->SetSegmentPTData(toSegmentKey, ptData);

            prestressIter++;
            segmentPTIter++;
         }

         // post tensioning
         CPTData& ptData = *ptIter;
         pIBridgeDesc->SetPostTensioning(toGirderKey, ptData);
      }
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
   GET_IFACE2(pBroker,IBridge,pBridge);

   m_OldHandlingData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      ATLASSERT(nSegments==pBridge->GetSegmentCount(m_FromGirderKey));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey fromSegmentKey(m_FromGirderKey,segIdx);
         const CHandlingData* pNewHandlingData = pSegmentData->GetHandlingData(fromSegmentKey);

         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         const CHandlingData* pOldHandlingData = pSegmentData->GetHandlingData(toSegmentKey);
         m_OldHandlingData.push_back(*pOldHandlingData);
         pSegmentData->SetHandlingData(toSegmentKey,*pNewHandlingData);
      }
   }

   return true;
}

void txnCopyGirderHandling::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CHandlingData>::iterator handlingIter(m_OldHandlingData.begin());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         CHandlingData& handlingData = *handlingIter;
         pSegmentData->SetHandlingData(toSegmentKey,handlingData);
         handlingIter++;
      }
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
   GET_IFACE2(pBroker,IBridge,pBridge);

   m_OldMaterialData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      ATLASSERT(nSegments==pBridge->GetSegmentCount(m_FromGirderKey));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey fromSegmentKey(m_FromGirderKey,segIdx);
         const CGirderMaterial* pNewMaterialData = pSegmentData->GetSegmentMaterial(fromSegmentKey);

         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         const CGirderMaterial* pOldMaterialData = pSegmentData->GetSegmentMaterial(toSegmentKey);
         m_OldMaterialData.push_back(*pOldMaterialData);
         pSegmentData->SetSegmentMaterial(toSegmentKey,*pNewMaterialData);
      }
   }

   return true;
}

void txnCopyGirderMaterial::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CGirderMaterial>::iterator materialIter(m_OldMaterialData.begin());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         CGirderMaterial& materialData = *materialIter;
         pSegmentData->SetSegmentMaterial(toSegmentKey,materialData);
         materialIter++;
      }
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
   GET_IFACE2(pBroker,IBridge,pBridge);

   m_OldRebarData.clear();
   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      ATLASSERT(nSegments==pBridge->GetSegmentCount(m_FromGirderKey));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey fromSegmentKey(m_FromGirderKey,segIdx);
         const CLongitudinalRebarData* pNewRebarData = pRebar->GetSegmentLongitudinalRebarData(fromSegmentKey);

         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         const CLongitudinalRebarData* pOldRebarlData = pRebar->GetSegmentLongitudinalRebarData(toSegmentKey);
         m_OldRebarData.push_back(*pOldRebarlData);
         pRebar->SetSegmentLongitudinalRebarData(toSegmentKey,*pNewRebarData);
      }
   }

   return true;
}

void txnCopyGirderRebar::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILongitudinalRebar,pRebar);
   GET_IFACE2(pBroker,IBridge,pBridge);

   std::vector<CGirderKey>::iterator iter(m_ToGirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_ToGirderKeys.end());
   std::vector<CLongitudinalRebarData>::iterator rebarIter(m_OldRebarData.begin());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& toGirderKey = *iter;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(toGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey toSegmentKey(toGirderKey,segIdx);

         CLongitudinalRebarData& rebarData = *rebarIter;
         pRebar->SetSegmentLongitudinalRebarData(toSegmentKey,rebarData);
         rebarIter++;
      }
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

// ----------------------------


CCopyGirderAllProperties::CCopyGirderAllProperties()
{
}

LPCTSTR CCopyGirderAllProperties::GetName()
{
   return _T("All Girder Properties");
}

BOOL CCopyGirderAllProperties::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   // this one we can always do
   return TRUE;
}

txnTransaction* CCopyGirderAllProperties::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderAllProperties(fromGirderKey,toGirderKeys);
}

UINT CCopyGirderAllProperties::GetGirderEditorTabIndex()
{
   return EGD_GENERAL;
}

rptParagraph* CCopyGirderAllProperties::BuildComparisonReportParagraph(const CGirderKey& fromGirderKey)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GirderAllPropertiesComparison(pPara, pBroker, fromGirderKey);

   return pPara;
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderMaterial::CCopyGirderMaterial()
{
}

LPCTSTR CCopyGirderMaterial::GetName()
{
   return _T("Concrete Material Properties");
}

BOOL CCopyGirderMaterial::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return TRUE;
}

txnTransaction* CCopyGirderMaterial::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderMaterial(fromGirderKey,toGirderKeys);
}

UINT CCopyGirderMaterial::GetGirderEditorTabIndex()
{
   return EGD_CONCRETE;
}

rptParagraph* CCopyGirderMaterial::BuildComparisonReportParagraph(const CGirderKey& fromGirderKey)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GirderMaterialsComparison(pPara, pBroker, fromGirderKey);

   return pPara;
}

/////////////////////////////////////////////////////////////////////////////

CCopyGirderRebar::CCopyGirderRebar()
{
}

LPCTSTR CCopyGirderRebar::GetName()
{
   return _T("Longitudinal Reinforcement");
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

UINT CCopyGirderRebar::GetGirderEditorTabIndex()
{
   return EGD_LONG_REINF;
}

rptParagraph* CCopyGirderRebar::BuildComparisonReportParagraph(const CGirderKey& fromGirderKey)
{
   rptParagraph* pPara = new rptParagraph;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   GirderLongRebarComparison(pPara, pBroker, pDisplayUnits, fromGirderKey);

   return pPara;
}

///////////////////////////////////////////////////////////////////////////

CCopyGirderStirrups::CCopyGirderStirrups()
{
}

LPCTSTR CCopyGirderStirrups::GetName()
{
   return _T("Transverse Reinforcement");
}

BOOL CCopyGirderStirrups::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return TRUE;
}

txnTransaction* CCopyGirderStirrups::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderStirrups(fromGirderKey,toGirderKeys);
}

UINT CCopyGirderStirrups::GetGirderEditorTabIndex()
{
   return EGD_STIRRUPS;
}

rptParagraph* CCopyGirderStirrups::BuildComparisonReportParagraph(const CGirderKey& fromGirderKey)
{
   rptParagraph* pPara = new rptParagraph;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   GirderPrimaryStirrupComparison(pPara, pBroker, pDisplayUnits, fromGirderKey);
   GirderSecondaryStirrupComparison(pPara, pBroker, pDisplayUnits, fromGirderKey);
   
   return pPara;
}


///////////////////////////////////////////////////////////////////////////

CCopyGirderPrestressing::CCopyGirderPrestressing()
{
}

LPCTSTR CCopyGirderPrestressing::GetName()
{
   return _T("Prestressing");
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

UINT CCopyGirderPrestressing::GetGirderEditorTabIndex()
{
   return EGD_PRESTRESSING;
}

rptParagraph* CCopyGirderPrestressing::BuildComparisonReportParagraph(const CGirderKey& fromGirderKey)
{
   rptParagraph* pPara = new rptParagraph;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);


   if (GirderPrestressingComparison(pPara, pBroker, pDisplayUnits, fromGirderKey))
   {
      debonding(pPara, pBroker, pDisplayUnits, fromGirderKey);
   }

   if (bIsSplicedGirder)
   {
      GirderGroupPostensioningComparison(pPara, pBroker, pDisplayUnits, fromGirderKey);
      GirderSegmentPostensioningComparison(pPara, pBroker, pDisplayUnits, fromGirderKey);
   }

   return pPara;
}


////////////////////////////////////////////////////////////////////////////

CCopyGirderHandling::CCopyGirderHandling()
{
}

LPCTSTR CCopyGirderHandling::GetName()
{
   return _T("Temporary Conditions");
}

BOOL CCopyGirderHandling::CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return TRUE;
}

txnTransaction* CCopyGirderHandling::CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys)
{
   return new txnCopyGirderHandling(fromGirderKey,toGirderKeys);
}

UINT CCopyGirderHandling::GetGirderEditorTabIndex()
{
   return EGD_TRANSPORTATION;
}

rptParagraph* CCopyGirderHandling::BuildComparisonReportParagraph(const CGirderKey& fromGirderKey)
{
   rptParagraph* pPara = new rptParagraph;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   GirderHandlingComparison(pPara, pBroker, pDisplayUnits, fromGirderKey);

   return pPara;
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Reporting functions /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void GirderAllPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, const CGirderKey & fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ColumnIndexType nCols = 4;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Girder All Properties Comparison"));
   *pPara << p_table<<rptNewLine;

   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("Girder");
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   (*p_table)(0,col++) << _T("Type");
   (*p_table)(0,col++) << _T("Rating") << rptNewLine << _T("Condition") << rptNewLine << _T("Factor");

   RowIndexType row = 1;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CGirderGroupData* pFromGroup = pBridgeDesc->GetGirderGroup(fromGirderKey.groupIndex);

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         CGirderKey girderKey(grpIdx, gdrIdx);

         bool isFrom = fromGirderKey == girderKey;
         if (isFrom)
         {
            ColorFromRow(p_table, row, nCols);
         }

         col = 0;
         (*p_table)(row, col++) << pgsGirderLabel::GetGirderLabel(girderKey);

         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         const CSplicedGirderData* pFromGirder = pFromGroup->GetGirder(fromGirderKey.girderIndex);

         WriteCompareCell(p_table, row, col++, isFrom, *pGirder == *pFromGirder);

         (*p_table)(row,col++) << pGirder->GetGirderName();
         (*p_table)(row,col++) << pGirder->GetConditionFactor();

         row++;
      }
   }
}

void GirderPrimaryStirrupComparison(rptParagraph* pPara, IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IShear,pShear);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, span,            pDisplayUnits->GetSpanLengthUnit(),  false );

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   ColumnIndexType nCols = 13;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Primary Transverse Reinforcement"));
   *pPara << p_table << rptNewLine;

   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("Girder");
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   (*p_table)(0, col++) << _T("Material");
   (*p_table)(0, col++) << _T("Zone")<< rptNewLine <<_T("#");
   (*p_table)(0, col++) << COLHDR(_T("Zone")<<rptNewLine<<_T("Length"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0, col++) << _T("Bar")<< rptNewLine <<_T("Size");
   (*p_table)(0, col++)<<  COLHDR(_T("Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0, col++) << _T("# of")<< rptNewLine <<_T("Vertical") << rptNewLine <<_T("Legs");
   (*p_table)(0, col++) << _T("# Legs")<< rptNewLine <<_T("Extended") << rptNewLine <<_T("into Deck");
   (*p_table)(0, col++) << _T("Confinement")<< rptNewLine << _T("Bar")<< rptNewLine <<_T("Size");
   (*p_table)(0, col++) << _T("Symmetric")<< rptNewLine << _T("about")<< rptNewLine <<_T("Mid-Span?");
   (*p_table)(0, col++) << _T("Use for")<< _T("Splitting")<< rptNewLine <<_T("Resistance?");
   (*p_table)(0, col++) << _T("Top")<< rptNewLine << _T("Flange")<< rptNewLine <<_T("Roughened?");

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            col = 0;

            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            (*p_table)(row, col++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            const CShearData2* pShr = pShear->GetSegmentShearData(segmentKey);

            bool bEqual = false;
            if (segIdx < pBridge->GetSegmentCount(fromGirderKey))
            {
               const CShearData2* pFromShr = pShear->GetSegmentShearData(CSegmentKey(fromGirderKey,segIdx));
               bEqual = *pFromShr == *pShr;
            }

            WriteCompareCell(p_table, row, col++, isFrom, bEqual);

            IndexType nZones = pShr->ShearZones.size();
            if (nZones == 0)
            {
               (*p_table)(row, col++) << RPT_NA;
            }
            else
            {
               (*p_table)(row, col++) << pShear->GetSegmentStirrupMaterial(segmentKey);

               bool bFirst(true);
               for (IndexType iZone = 0; iZone < nZones; iZone++)
               {
                  ColumnIndexType iCol = col;
                  const CShearZoneData2& stirrupZone = pShr->ShearZones[iZone];

                  (*p_table)(row, iCol++) << iZone+1 << rptNewLine;
                  if (iZone < nZones - 1)
                  {
                     (*p_table)(row, iCol++) << span.SetValue(stirrupZone.ZoneLength) << rptNewLine;
                  }
                  else
                  {
                     (*p_table)(row, iCol++) << _T("to mid-span");
                  }
                  (*p_table)(row, iCol++) << lrfdRebarPool::GetBarSize(stirrupZone.VertBarSize) << rptNewLine;
                  (*p_table)(row, iCol++) << dim.SetValue(stirrupZone.BarSpacing) << rptNewLine;
                  (*p_table)(row, iCol++) << stirrupZone.nVertBars << rptNewLine;
                  (*p_table)(row, iCol++) << stirrupZone.nHorzInterfaceBars << rptNewLine;
                  (*p_table)(row, iCol++) << lrfdRebarPool::GetBarSize(stirrupZone.ConfinementBarSize) << rptNewLine;

                  if (bFirst)
                  {
                     bFirst = false;
                     (*p_table)(row, iCol++) << (pShr->bAreZonesSymmetrical ? _T("Yes") : _T("No")) << rptNewLine;
                     (*p_table)(row, iCol++) << (pShr->bUsePrimaryForSplitting ? _T("Yes") : _T("No")) << rptNewLine;
                     (*p_table)(row, iCol++) << (pShr->bIsRoughenedSurface ? _T("Yes") : _T("No")) << rptNewLine;
                  }
               }
            }
            row++;
         }
      }
   }
}

void GirderSecondaryStirrupComparison(rptParagraph* pPara, IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IShear,pShear);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, span,            pDisplayUnits->GetSpanLengthUnit(),  false );

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   ColumnIndexType nCols = 8;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Additional Transverse Reinforcement"));
   p_table->SetNumberOfHeaderRows(2);
   *pPara << p_table << rptNewLine;

   ColumnIndexType col = 0;
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Girder");
   p_table->SetColumnSpan(0, col, 4);
   (*p_table)(0, col) << _T("Splitting Reinforcement");
   (*p_table)(1, col++) << COLHDR(_T("Zone")<<rptNewLine<<_T("Length"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(1, col++) << _T("Bar")<< rptNewLine <<_T("Size");
   (*p_table)(1, col++)<<  COLHDR(_T("Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1, col++) << _T("# of")<< rptNewLine <<_T("Legs");
   p_table->SetColumnSpan(0, col, 3);
   (*p_table)(0, col) << _T("Bottom Flange Confinement");
   (*p_table)(1, col++) << COLHDR(_T("Zone")<<rptNewLine<<_T("Length"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(1, col++) << _T("Bar")<< rptNewLine <<_T("Size");
   (*p_table)(1, col++)<<  COLHDR(_T("Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            col = 0;

            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            const CShearData2* pShr = pShear->GetSegmentShearData(segmentKey);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            (*p_table)(row, col++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            if (pShr->SplittingBarSize == matRebar::bsNone)
            {
               (*p_table)(row, col++) << RPT_NA;
               col += 3;
            }
            else
            {
               (*p_table)(row, col++) << span.SetValue(pShr->SplittingZoneLength) << rptNewLine;
               (*p_table)(row, col++) << lrfdRebarPool::GetBarSize(pShr->SplittingBarSize) << rptNewLine;
               (*p_table)(row, col++) << dim.SetValue(pShr->SplittingBarSpacing) << rptNewLine;
               (*p_table)(row, col++) << pShr->nSplittingBars << rptNewLine;
            }

            if (pShr->ConfinementBarSize == matRebar::bsNone)
            {
               (*p_table)(row, col++) << RPT_NA;
               col += 2;
            }
            else
            {
               (*p_table)(row, col++) << span.SetValue(pShr->ConfinementZoneLength) << rptNewLine;
               (*p_table)(row, col++) << lrfdRebarPool::GetBarSize(pShr->ConfinementBarSize) << rptNewLine;
               (*p_table)(row, col++) << dim.SetValue(pShr->ConfinementBarSpacing) << rptNewLine;
            }

            row++;
         }
      }
   }
}

void GirderHandlingComparison(rptParagraph* pPara, IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria); 
   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria); 

   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(),  false );

   bool isHauling = pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled();
   bool isLifting = pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled();

   ColumnIndexType nCols = 10;
   if (!isHauling)
   {
      nCols -= 2;
   }
   else if ( pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::hmWSDOT )
   {
      nCols++;
   }

   if (!isLifting)
   {
      nCols -= 2;
   }

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Girder Temporary Conditions Comparison"));
   *pPara << p_table << rptNewLine;

   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("Girder");
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   (*p_table)(0,col++) << COLHDR(_T("Left") << rptNewLine << _T("Release") << rptNewLine << _T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR(_T("Right") << rptNewLine << _T("Release") << rptNewLine << _T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR(_T("Left") << rptNewLine << _T("Storage") << rptNewLine << _T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR(_T("Right") << rptNewLine << _T("Storage") << rptNewLine << _T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   if (isLifting)
   {
      (*p_table)(0, col++) << COLHDR(_T("Left") << rptNewLine << _T("Lifting") << rptNewLine << _T("Loop") << rptNewLine << _T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_table)(0, col++) << COLHDR(_T("Right") << rptNewLine << _T("Lifting") << rptNewLine << _T("Loop") << rptNewLine << _T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   if (isHauling)
   {
      (*p_table)(0, col++) << COLHDR(_T("Trailing") << rptNewLine << _T("Truck") << rptNewLine << _T("Support") << rptNewLine << _T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_table)(0, col++) << COLHDR(_T("Leading") << rptNewLine << _T("Truck") << rptNewLine << _T("Support") << rptNewLine << _T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      if (pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::hmWSDOT)
      {
         (*p_table)(0, col++) << _T("Haul Truck");
      }
   }

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            col = 0;

            CSegmentKey fromSegmentKey(fromGirderKey,segIdx);
            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromSegmentKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            (*p_table)(row, col++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            const CHandlingData* pHandlingData = pSegmentData->GetHandlingData(segmentKey);

            bool bEqual = false;
            if (segIdx < pBridge->GetSegmentCount(fromGirderKey))
            {
               const CHandlingData* pFromHandlingData = pSegmentData->GetHandlingData(fromSegmentKey);

               // This is not exactly correct here because the == function will compare all values regardless of hauling/lifting settings
               bEqual = *pFromHandlingData == *pHandlingData;
            }

            WriteCompareCell(p_table, row, col++, isFrom, bEqual);

            // Deal with magic numbers for release and storage locations
            if (-1 == pHandlingData->LeftReleasePoint)
            {
               (*p_table)(row, col++) << _T("END");
            }
            else if (-2 == pHandlingData->LeftReleasePoint)
            {
               (*p_table)(row, col++) << _T("BRG");
            }
            else
            {
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->LeftReleasePoint);
            }

            if (-1 == pHandlingData->RightReleasePoint)
            {
               (*p_table)(row, col++) << _T("END");
            }
            else if (-2 == pHandlingData->RightReleasePoint)
            {
               (*p_table)(row, col++) << _T("BRG");
            }
            else
            {
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->RightReleasePoint);
            }

            if (-1 == pHandlingData->LeftStoragePoint)
            {
               (*p_table)(row, col++) << _T("BRG");
            }
            else
            {
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->LeftStoragePoint);
            }

            if (-1 == pHandlingData->RightStoragePoint)
            {
               (*p_table)(row, col++) << _T("BRG");
            }
            else
            {
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->RightStoragePoint);
            }

            if (isLifting)
            {
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->LeftLiftPoint);
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->RightLiftPoint);
            }

            if (isHauling)
            {
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->TrailingSupportPoint);
               (*p_table)(row, col++) << loc.SetValue(pHandlingData->LeadingSupportPoint);
               if (pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::hmWSDOT)
               {
                  (*p_table)(row, col++) << pHandlingData->HaulTruckName;
               }
            }

            row++;
         }
      }
   }
}

void GirderMaterialsComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, const CGirderKey& fromGirderKey)
{
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );

   ColumnIndexType nCols = 8;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Girder Material Comparison"));
   *pPara << p_table<<rptNewLine;

   ColumnIndexType iCol = 0;
   (*p_table)(0,iCol++) << _T("Girder");
   (*p_table)(0,iCol++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   (*p_table)(0,iCol++) << _T("Concrete") << rptNewLine << _T("Type");
   (*p_table)(0,iCol++) << COLHDR(RPT_FC,rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0, iCol++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*p_table)(0,iCol++) << COLHDR(_T("Density") << rptNewLine << _T("for") << rptNewLine << _T("Strength"),rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,iCol++) << COLHDR(_T("Density") << rptNewLine << _T("for") << rptNewLine << _T("Weight"),rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,iCol++) << COLHDR(_T("Max") << rptNewLine << _T("Aggregate") << rptNewLine << _T("Size"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   GET_IFACE2(pBroker,ILibrary, pLib );
   GET_IFACE2(pBroker,ISpecification, pSpec );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // special considerations for f'ci if time step
   int loss_method = pSpecEntry->GetLossMethod();
   bool isTimeStep = loss_method == LOSSES_TIME_STEP;

   RowIndexType row = 1;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            iCol = 0;
            (*p_table)(row, iCol++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

            bool bEqual = false;
            if (segIdx < pBridge->GetSegmentCount(fromGirderKey))
            {
               const CGirderMaterial* pFromMaterial = pSegmentData->GetSegmentMaterial(CSegmentKey(fromGirderKey,segIdx));
               bEqual = *pFromMaterial == *pMaterial;
            }

            WriteCompareCell(p_table, row, iCol++, isFrom, bEqual);

            pgsTypes::ConcreteType type = pMaterial->Concrete.Type;
            std::_tstring  name = lrfdConcreteUtil::GetTypeName((matConcrete::Type)type, false);
            (*p_table)(row, iCol++) << name;

            if (isTimeStep && pMaterial->Concrete.bBasePropertiesOnInitialValues)
            {
               (*p_table)(row, iCol++) << RPT_NA;
            }
            else
            {
               (*p_table)(row, iCol++) << stress.SetValue(pMaterial->Concrete.Fc);
            }

            if (isTimeStep && !pMaterial->Concrete.bBasePropertiesOnInitialValues)
            {
               (*p_table)(row, iCol++) << RPT_NA;
            }
            else
            {
               (*p_table)(row, iCol++) << stress.SetValue(pMaterial->Concrete.Fci);
            }

            (*p_table)(row, iCol++) << density.SetValue(pMaterial->Concrete.StrengthDensity);
            (*p_table)(row, iCol++) << density.SetValue(pMaterial->Concrete.WeightDensity);
            (*p_table)(row, iCol++) << dim.SetValue(pMaterial->Concrete.MaxAggregateSize);

            row++;
         }
      }
   }
}


// return true if debonding exists
bool GirderPrestressingComparison(rptParagraph* pPara, IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey)
{
   bool was_debonding = false;

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   // Only report temporary strands if at least one girder has some
   bool bTempStrands = false;
   for (GroupIndexType grpIdx = 0; grpIdx<nGroups && !bTempStrands; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx<nGirders && !bTempStrands; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            if (0 < pStrandGeometry->GetStrandCount(CSegmentKey(grpIdx, gdrIdx, segIdx), pgsTypes::Temporary))
            {
               bTempStrands = true;
               break;
            }
         }
      }
   }

   ColumnIndexType nCols = bTempStrands ? 15 : 12;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Pretensioned Prestressing Strands Comparison"));
   *pPara << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col = 0;
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Girder");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");

   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << _T("Strand")<<rptNewLine<<_T("Definition");

   p_table->SetColumnSpan(0, col, 3);
   (*p_table)(0, col) << _T("Straight Strands");
   (*p_table)(1, col++) << _T("Material");
   (*p_table)(1, col++) << _T("# of")<<rptNewLine<<_T("Strands");
   (*p_table)(1, col++) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

   p_table->SetColumnSpan(0, col, 6);
   (*p_table)(0, col) << _T("Adjustable Strands");
   (*p_table)(1, col++) << _T("Material");
   (*p_table)(1, col++) << _T("Type");
   (*p_table)(1, col++) << _T("# of")<<rptNewLine<<_T("Strands");
   (*p_table)(1, col++) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   (*p_table)(1, col++) << COLHDR(_T("Girder End")<<rptNewLine<<_T("Offset"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1, col++)<< COLHDR(_T("Harping Pt")<<rptNewLine<<_T("Offset"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(0, col, 3);
      (*p_table)(0, col) << _T("Temporary Strands");
      (*p_table)(1, col++) << _T("Material");
      (*p_table)(1, col++) << _T("# of")<<rptNewLine<<_T("Strands");
      (*p_table)(1, col++) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   }

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            col = 0;

            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            (*p_table)(row, col++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

            bool bEqual = false;
            if (segIdx < pBridge->GetSegmentCount(fromGirderKey))
            {
               const CStrandData* pFromStrands = pSegmentData->GetStrandData(CSegmentKey(fromGirderKey,segIdx));
               bEqual = *pFromStrands == *pStrands;
            }

            WriteCompareCell(p_table, row, col++, isFrom, bEqual);

            auto adjustableStrandType = pStrands->GetAdjustableStrandType();
            auto strandDefinitionType = pStrands->GetStrandDefinitionType();
            (*p_table)(row, col++) << GetStrandDefinitionType(strandDefinitionType, adjustableStrandType);

            (*p_table)(row, col++) << pStrands->GetStrandMaterial(pgsTypes::Straight)->GetName();

            (*p_table)(row, col) << pStrands->GetStrandCount(pgsTypes::Straight);
            StrandIndexType nd = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);
            if (0 < nd)
            {
               (*p_table)(row, col) << rptNewLine << nd << _T(" Debonded");
               was_debonding = true;
            }
            StrandIndexType nExtLeft = pStrandGeometry->GetNumExtendedStrands(segmentKey, pgsTypes::metStart, pgsTypes::Straight);
            StrandIndexType nExtRight = pStrandGeometry->GetNumExtendedStrands(segmentKey, pgsTypes::metEnd, pgsTypes::Straight);
            if (0 < nExtLeft || 0 < nExtRight)
            {
               (*p_table)(row, col) << rptNewLine << nExtLeft << _T(" Ext. Left");
               (*p_table)(row, col) << rptNewLine << nExtRight << _T(" Ext. Right");
            }

            col++;

            (*p_table)(row, col++) << force.SetValue(pStrands->GetPjack(pgsTypes::Straight));

            (*p_table)(row, col++) << pStrands->GetStrandMaterial(pgsTypes::Harped)->GetName();
            (*p_table)(row, col++) << LABEL_HARP_TYPE(pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey));
            (*p_table)(row, col++) << pStrands->GetStrandCount(pgsTypes::Harped);
            (*p_table)(row, col++) << force.SetValue(pStrands->GetPjack(pgsTypes::Harped));

            if (strandDefinitionType == pgsTypes::sdtTotal || strandDefinitionType == pgsTypes::sdtStraightHarped || strandDefinitionType == pgsTypes::sdtDirectSelection || strandDefinitionType == pgsTypes::sdtDirectRowInput)
            {
               ConfigStrandFillVector confvec = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, pStrands->GetStrandCount(pgsTypes::Harped));

               // convert to absolute adjustment
               Float64 adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(segmentKey, pgsTypes::metStart, confvec,
                  pStrands->GetHarpStrandOffsetMeasurementAtEnd(), pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
               (*p_table)(row, col++) << dim.SetValue(adjustment);

               adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(segmentKey, pgsTypes::metStart, confvec,
                  pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint(), pStrands->GetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd));
               (*p_table)(row, col++) << dim.SetValue(adjustment);
            }
            else
            {
               ATLASSERT(strandDefinitionType == pgsTypes::sdtDirectStrandInput);
               (*p_table)(row, col++) << _T("");
               (*p_table)(row, col++) << _T("");
            }

            if (bTempStrands)
            {
               (*p_table)(row, col++) << pStrands->GetStrandMaterial(pgsTypes::Temporary)->GetName();
               (*p_table)(row, col++) << pStrands->GetStrandCount(pgsTypes::Temporary);
               (*p_table)(row, col++) << force.SetValue(pStrands->GetPjack(pgsTypes::Temporary));
            }

            row++;
         }
      }
   }

   *pPara<<_T("Girder End Offset - Distance the harped strands at the girder ends are adjusted vertically from their default (library) locations.")<<rptNewLine;
   *pPara<<_T("Harping Point Offset - Distance the harped strands at the harping point are adjusted vertically from their default (library) locations.")<<rptNewLine;
   return was_debonding;
}

// Utility classes for storing and retreiving debond section information
struct DebondSectionData
{
   Float64 m_EndDistance;
   StrandIndexType m_DebondCount;

   DebondSectionData(Float64 endDist):
   m_EndDistance(endDist), m_DebondCount(1) 
   {;}

   bool operator==(const DebondSectionData& rOther) const
   { 
      return ::IsEqual(m_EndDistance,rOther.m_EndDistance);
   }

   bool operator<(const DebondSectionData& rOther) const 
   { 
      return m_EndDistance < rOther.m_EndDistance; 
   }

private:
   DebondSectionData()
   {;}
};

typedef std::set<DebondSectionData> DebondSectionSet;
typedef DebondSectionSet::iterator DebondSectionSetIter;


struct DebondRowData
{
   Float64 m_Elevation;
   DebondSectionSet m_DebondSections;

   DebondRowData(Float64 elevation):
   m_Elevation(elevation)
   {;}

   StrandIndexType GetNumDebonds() const
   {
      StrandIndexType num=0;
      for (DebondSectionSetIter it=m_DebondSections.begin(); it!=m_DebondSections.end(); it++)
      {
         num += it->m_DebondCount;
      }

      return num;
   }

   bool operator==(const DebondRowData& rOther) const
   { 
      return ::IsEqual(m_Elevation,rOther.m_Elevation);
   }

   bool operator<(const DebondRowData& rOther) const 
   { 
      return m_Elevation < rOther.m_Elevation; 
   }
};

typedef std::set<DebondRowData> DebondRowDataSet;
typedef DebondRowDataSet::iterator DebondRowDataSetIter;

class DebondComparison
{
public:
   // status of debonding
   enum DebondStatus {IsDebonding, NonSymmetricDebonding, NoDebonding};

   DebondStatus Init(IBridge* pBridge, IStrandGeometry* pStrandGeometry);

   // get debond row data for a given index. order is assumed as same order in Init function loop
   std::pair<CSegmentKey, DebondRowDataSet>* GetDebondRowDataSet(IndexType idx)
   {
      assert(idx<(GirderIndexType)m_DebondRowData.size());
      return &m_DebondRowData[idx];
   }

   std::set<Float64> m_SectionLocations;

private:
   std::vector<std::pair<CSegmentKey,DebondRowDataSet>> m_DebondRowData;
};

DebondComparison::DebondStatus DebondComparison::Init(IBridge* pBridge, IStrandGeometry* pStrandGeometry)
{
   assert(m_DebondRowData.empty());

   StrandIndexType total_num_debonded=0;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            if (!pStrandGeometry->IsDebondingSymmetric(segmentKey))
            {
               // can't do for non-symetrical strands
               return NonSymmetricDebonding;
            }

            m_DebondRowData.push_back(std::make_pair(segmentKey,DebondRowDataSet()));
            DebondRowDataSet& row_data_set = m_DebondRowData.back().second;

            CComPtr<IPoint2dCollection> strand_coords;
            pStrandGeometry->GetStrandPositions(pgsPointOfInterest(segmentKey, 0.0), pgsTypes::Straight, &strand_coords);

            CollectionIndexType num_strands;
            strand_coords->get_Count(&num_strands);

            for (StrandIndexType istrand = 0; istrand < num_strands; istrand++)
            {
               Float64 dist_start, dist_end;
               if (pStrandGeometry->IsStrandDebonded(segmentKey, istrand, pgsTypes::Straight, nullptr, &dist_start, &dist_end))
               {
                  total_num_debonded++;

                  // blindly insert section location
                  m_SectionLocations.insert(dist_start);

                  // Get to building row data
                  Float64 elev;
                  CComPtr<IPoint2d> point;
                  strand_coords->get_Item(istrand, &point);
                  point->get_Y(&elev);

                  DebondRowDataSetIter row_iter = row_data_set.find(DebondRowData(elev));
                  if (row_iter != row_data_set.end())
                  {
                     // found row, now look for end distance
                     DebondSectionData section_data(dist_start);
                     DebondSectionSetIter section_iter = row_iter->m_DebondSections.find(section_data);
                     if (section_iter != row_iter->m_DebondSections.end())
                     {
                        // existing section/row with debonds - increment count
                        DebondSectionData& data(const_cast<DebondSectionData&>(*section_iter));
                        data.m_DebondCount++;
                     }
                     else
                     {
                        // new section location in row
                        DebondRowData& rowData(const_cast<DebondRowData&>(*row_iter));
                        rowData.m_DebondSections.insert(section_data);
                     }
                  }
                  else
                  {
                     // create a new row
                     DebondRowData row_data(elev);
                     row_data.m_DebondSections.insert(DebondSectionData(dist_start));
                     row_data_set.insert(row_data);
                  }
               }
            }
         }
      }
   }

   if (total_num_debonded==0)
      return NoDebonding;
   else
      return IsDebonding;
}

void debonding(rptParagraph* pPara, IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   // First need to build data structure with all debond elevations/locations
   DebondComparison debond_comparison;
   DebondComparison::DebondStatus status = debond_comparison.Init(pBridge, pStrandGeometry);

   // First deal with odd cases
   if (status==DebondComparison::NoDebonding)
   {
      return; // nothing to do
   }


   if (status==DebondComparison::NonSymmetricDebonding)
   {

      *pPara<<color(Red) << bold(ON) << _T("Debonding in one or more girders is Unsymmetrical - Cannot perform comparison") << bold(OFF) << color(Black)<<rptNewLine;
      return;
   }

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(), false );

   ColumnIndexType num_section_locations = debond_comparison.m_SectionLocations.size();
   ColumnIndexType num_cols = 3 + num_section_locations;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(num_cols,_T("Debonding of Prestressing Strands"));
   *pPara << p_table<<rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col = 0;
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Girder");
   
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(_T("Strand")<<rptNewLine<<_T("Elev"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Total #")<<rptNewLine<<_T("Strands")<<rptNewLine<<_T("Debond");

   p_table->SetColumnSpan(0, col, num_section_locations);
   (*p_table)(0,col) << COLHDR(_T("# of Strands Debonded at")<<rptNewLine<<_T("Distance from Ends of Girder"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());

   // section location header columns
   for (std::set<Float64>::iterator siter=debond_comparison.m_SectionLocations.begin(); siter!=debond_comparison.m_SectionLocations.end(); siter++)
   {
      (*p_table)(1,col++) << loc.SetValue( *siter );
   }

   // Data rows in table
   RowIndexType row=p_table->GetNumberOfHeaderRows();

   // Crucial that the nested loop below matches that in DebondComparison::Init()
   IndexType index = 0;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, num_cols);
            }

            (*p_table)(row, 0) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            DebondRowDataSet& row_data = debond_comparison.GetDebondRowDataSet(index)->second;
            if (row_data.empty())
            {
               (*p_table)(row, 1) << RPT_NA; // no debonding in this girder
            }
            else
            {
               bool first = true;
               for (DebondRowDataSetIter riter = row_data.begin(); riter != row_data.end(); riter++)
               {
                  ColumnIndexType col = 1;

                  if (!first)(*p_table)(row, col) << rptNewLine;
                  (*p_table)(row, col++) << dim.SetValue(riter->m_Elevation);

                  if (!first)
                  {
                     (*p_table)(row, col) << rptNewLine;
                  }
                  (*p_table)(row, col++) << riter->GetNumDebonds();

                  // cycle through each section and see if we have debonds there
                  for (std::set<Float64>::iterator siter = debond_comparison.m_SectionLocations.begin(); siter != debond_comparison.m_SectionLocations.end(); siter++)
                  {
                     Float64 section_location = *siter;
                     StrandIndexType num_debonds = 0;

                     DebondSectionData section_data(section_location);
                     DebondSectionSetIter section_iter = riter->m_DebondSections.find(section_data);
                     if (section_iter != riter->m_DebondSections.end())
                     {
                        num_debonds = section_iter->m_DebondCount;
                     }

                     if (!first)(*p_table)(row, col) << rptNewLine;
                     (*p_table)(row, col++) << num_debonds;
                  }

                  if (first)
                     first = false;
               }
            }

            row++;
            index++;
         }
      }
   }
}

void GirderGroupPostensioningComparison(rptParagraph * pPara, IBroker * pBroker, IEAFDisplayUnits * pDisplayUnits, const CGirderKey & fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IEventMap,pEventMap);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );

   ColumnIndexType col = 0;
   RowIndexType row = 0;

   ColumnIndexType nCols = 12;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Full-Length Group Post-Tensioning Comparison"));
   *pPara << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Girder");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Duct") << rptNewLine <<_T("Material");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Strand") << rptNewLine <<_T("Size");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Installation") << rptNewLine <<_T("Method");
   p_table->SetColumnSpan(0, col, 7);
   (*p_table)(0, col) << _T("Ducts");
   (*p_table)(1, col++) << _T("Duct") << rptNewLine <<_T("#");
   (*p_table)(1, col++) << _T("Duct") << rptNewLine <<_T("Type");
   (*p_table)(1, col++) << _T("Duct") << rptNewLine <<_T("Shape");
   (*p_table)(1, col++) << _T("# of") << rptNewLine <<_T("Strands");
   (*p_table)(1, col++) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   (*p_table)(1, col++) << _T("Jacking") << rptNewLine <<_T("End");
   (*p_table)(1, col++) << _T("Event");

   row = p_table->GetNumberOfHeaderRows();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         CGirderKey girderKey(grpIdx, gdrIdx);

         GirderIndexType nWebs = pGirder->GetWebCount(girderKey);

         bool isFrom = fromGirderKey == girderKey;
         if (isFrom)
         {
            ColorFromRow(p_table, row, nCols);
         }

         col = 0;
         (*p_table)(row, col++) << pgsGirderLabel::GetGirderLabel(girderKey);

         const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
         const CPTData* pPTFromData = pIBridgeDesc->GetPostTensioning(fromGirderKey);

         WriteCompareCell(p_table, row, col++, isFrom, *pPTFromData == *pPTData);

         DuctIndexType nDucts = pPTData->GetDuctCount();
         if (nDucts == 0)
         {
            (*p_table)(row, col++) << RPT_NA;
         }
         else
         {
            (*p_table)(row, col++) << GetDuctMaterialStr(pPTData->DuctType);
            (*p_table)(row, col++) << pPTData->pStrand->GetName();
            (*p_table)(row, col++) << (pPTData->InstallationType == pgsTypes::sitPush ? _T("Push") : _T("Pull"));

            for (DuctIndexType iDuct = 0; iDuct < nDucts; iDuct++)
            {
               ColumnIndexType lcol = col;

               if (nWebs == 1)
               {
                  (*p_table)(row, lcol++) << LABEL_DUCT(iDuct) << rptNewLine;
               }
               else
               {
                  DuctIndexType first_duct_in_row = nWebs*(iDuct);
                  DuctIndexType last_duct_in_row  = first_duct_in_row + nWebs - 1;
                  CString strLabel;
                  strLabel.Format(_T("%d - %d"), LABEL_DUCT(first_duct_in_row), LABEL_DUCT(last_duct_in_row));
                  (*p_table)(row, lcol++) << strLabel << rptNewLine;
               }

               const CDuctData* pDuct = pPTData->GetDuct(iDuct);
               (*p_table)(row, lcol++) << pDuct->Name << rptNewLine;
               (*p_table)(row, lcol++) << (pDuct->DuctGeometryType == CSegmentDuctData::Parabolic ? _T("Parabolic") : _T("Linear")) << rptNewLine;
               (*p_table)(row, lcol++) << pDuct->nStrands << rptNewLine;
               (*p_table)(row, lcol++) << force.SetValue(pDuct->Pj) << rptNewLine;
               (*p_table)(row, lcol++) << GetJackEndStr(pDuct->JackingEnd) << rptNewLine;

               EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(gdrIdx,iDuct);
               (*p_table)(row, lcol++) << pEventMap->GetEventName(eventIdx) << rptNewLine;
            }
         }

         row++;
      }
   }
}

void GirderSegmentPostensioningComparison(rptParagraph * pPara, IBroker * pBroker, IEAFDisplayUnits * pDisplayUnits, const CGirderKey & fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pGirder);

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );

   ColumnIndexType col = 0;
   RowIndexType row = 0;

   ColumnIndexType nCols = 18;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Segment Post-Tensioning Comparison"));
   *pPara << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(3);
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Girder");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Duct Material");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Strand") << rptNewLine <<_T("Size");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Installation") << rptNewLine <<_T("Method");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Time of") << rptNewLine <<_T("Installation");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Duct #");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Duct Type");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Duct Shape");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("# of") << rptNewLine <<_T("Strands");
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0, col++) << _T("Jacking End");

   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("Left End");
   (*p_table)(1, col++) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1, col++) << _T("Face");

   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("Middle");
   (*p_table)(1, col++) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1, col++) << _T("Face");

   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("Right End");
   (*p_table)(1, col++) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1, col++) << _T("Face");

   row = p_table->GetNumberOfHeaderRows();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            col = 0;
            (*p_table)(row, col++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            const CSegmentPTData* pPTData = &(pIBridgeDesc->GetPrecastSegmentData(segmentKey)->Tendons);

            bool bEqual = false;
            if (segIdx < pBridge->GetSegmentCount(fromGirderKey))
            {
               const CSegmentPTData* pPTFromData = &(pIBridgeDesc->GetPrecastSegmentData(CSegmentKey(fromGirderKey,segIdx))->Tendons);
               bEqual = *pPTFromData == *pPTData;
            }

            WriteCompareCell(p_table, row, col++, isFrom, bEqual);

            GirderIndexType nWebs = pGirder->GetWebCount(segmentKey);

            DuctIndexType nDucts = pPTData->GetDuctCount();
            if (nDucts == 0)
            {
               (*p_table)(row, col++) << RPT_NA;
            }
            else
            {
               (*p_table)(row, col++) << GetDuctMaterialStr(pPTData->DuctType);
               (*p_table)(row, col++) << pPTData->m_pStrand->GetName();
               (*p_table)(row, col++) << (pPTData->InstallationType == pgsTypes::sitPush ? _T("Push") : _T("Pull"));
               (*p_table)(row, col++) << GetInstallEventStr(pPTData->InstallationEvent);

               for (DuctIndexType iDuct = 0; iDuct < nDucts; iDuct++)
               {
                  ColumnIndexType lcol = col;
                  if (nWebs == 1)
                  {
                     (*p_table)(row, lcol++) << LABEL_DUCT(iDuct) << rptNewLine;
                  }
                  else
                  {
                     DuctIndexType first_duct_in_row = nWebs*(iDuct);
                     DuctIndexType last_duct_in_row  = first_duct_in_row + nWebs - 1;
                     CString strLabel;
                     strLabel.Format(_T("%d - %d"), LABEL_DUCT(first_duct_in_row), LABEL_DUCT(last_duct_in_row));
                     (*p_table)(row, lcol++) << strLabel << rptNewLine;
                  }

                  const CSegmentDuctData* pDuct = pPTData->GetDuct(iDuct);
                  (*p_table)(row, lcol++) << pDuct->Name << rptNewLine;
                  (*p_table)(row, lcol++) << (pDuct->DuctGeometryType == CSegmentDuctData::Parabolic ? _T("Parabolic") : _T("Linear")) << rptNewLine;
                  (*p_table)(row, lcol++) << pDuct->nStrands << rptNewLine;
                  (*p_table)(row, lcol++) << force.SetValue(pDuct->Pj) << rptNewLine;
                  (*p_table)(row, lcol++) << GetJackEndStr(pDuct->JackingEnd) << rptNewLine;
                  (*p_table)(row, lcol++) << dim.SetValue(pDuct->DuctPoint[CSegmentDuctData::Left].first) << rptNewLine;
                  (*p_table)(row, lcol++) << GetFaceTypeStr(pDuct->DuctPoint[CSegmentDuctData::Left].second) << rptNewLine;
                  if (pDuct->DuctGeometryType == CSegmentDuctData::Parabolic)
                  {
                     (*p_table)(row, lcol++) << dim.SetValue(pDuct->DuctPoint[CSegmentDuctData::Middle].first) << rptNewLine;
                     (*p_table)(row, lcol++) << GetFaceTypeStr(pDuct->DuctPoint[CSegmentDuctData::Middle].second) << rptNewLine;
                  }
                  else
                  {
                     (*p_table)(row, lcol++) << RPT_NA << rptNewLine;
                     (*p_table)(row, lcol++) << RPT_NA << rptNewLine;
                  }
                  (*p_table)(row, lcol++) << dim.SetValue(pDuct->DuctPoint[CSegmentDuctData::Right].first) << rptNewLine;
                  (*p_table)(row, lcol++) << GetFaceTypeStr(pDuct->DuctPoint[CSegmentDuctData::Right].second) << rptNewLine;
               }
            }

            row++;
         }
      }
   }
}

void GirderLongRebarComparison(rptParagraph* pPara, IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const CGirderKey& fromGirderKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongitudinalRebar,pLongitudinalRebar);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, span,            pDisplayUnits->GetSpanLengthUnit(),  false );

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   ColumnIndexType nCols = 14;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Longitudinal Rebar Comparison"));
   *pPara << p_table << rptNewLine;

   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("Girder");
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Girder?");
   (*p_table)(0, col++) << _T("Material");
   (*p_table)(0, col++) << _T("Row")<< rptNewLine <<_T("#");
   (*p_table)(0, col++) << _T("Measured")<< rptNewLine <<_T("From");
   (*p_table)(0, col++) << COLHDR(_T("Distance")<<rptNewLine<<_T("from End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0, col++) << COLHDR(_T("Bar")<<rptNewLine<<_T("Length"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0, col++) << _T("Girder")<< rptNewLine <<_T("Face");
   (*p_table)(0, col++)<<  COLHDR(_T("Cover"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0, col++) << _T("Bar")<< rptNewLine <<_T("Size");
   (*p_table)(0, col++) << _T("# of")<< rptNewLine <<_T("Bars");
   (*p_table)(0, col++)<<  COLHDR(_T("Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0, col++) << _T("Anchored")<< rptNewLine <<_T("Left");
   (*p_table)(0, col++) << _T("Anchored")<< rptNewLine <<_T("Right");

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            col = 0;

            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

            bool isFrom = fromGirderKey == segmentKey;
            if (isFrom)
            {
               ColorFromRow(p_table, row, nCols);
            }

            (*p_table)(row, col++) << pgsGirderLabel::GetSegmentLabel(segmentKey);

            const CLongitudinalRebarData* pRebar = pLongitudinalRebar->GetSegmentLongitudinalRebarData(segmentKey);

            bool bEqual = false;
            if (segIdx < pBridge->GetSegmentCount(fromGirderKey))
            {
               const CLongitudinalRebarData* pFromRebar = pLongitudinalRebar->GetSegmentLongitudinalRebarData(CSegmentKey(fromGirderKey,segIdx));
               bEqual = *pFromRebar == *pRebar;
            }

            WriteCompareCell(p_table, row, col++, isFrom, bEqual);

            IndexType nRows = pRebar->RebarRows.size();
            if (nRows == 0)
            {
               (*p_table)(row, col++) << RPT_NA;
            }
            else
            {
               (*p_table)(row, col++) << pLongitudinalRebar->GetSegmentLongitudinalRebarMaterial(segmentKey);

               for (IndexType iRow = 0; iRow < nRows; iRow++)
               {
                  ColumnIndexType iCol = col;
                  const CLongitudinalRebarData::RebarRow& rebarRow = pRebar->RebarRows[iRow];

                  (*p_table)(row, iCol++) << iRow+1 << rptNewLine;
                  (*p_table)(row, iCol++) << GetRebarLayoutTypeStr(rebarRow.BarLayout) << rptNewLine;
                  if (rebarRow.BarLayout == pgsTypes::blFromLeft || rebarRow.BarLayout == pgsTypes::blFromRight)
                  {
                     (*p_table)(row, iCol++) << span.SetValue(rebarRow.DistFromEnd) << rptNewLine;
                     (*p_table)(row, iCol++) << span.SetValue(rebarRow.BarLength) << rptNewLine;
                  }
                  else
                  {
                     (*p_table)(row, iCol++) << _T("-") << rptNewLine;
                     (*p_table)(row, iCol++) << _T("-") << rptNewLine;
                  }

                  (*p_table)(row, iCol++) << (rebarRow.Face == pgsTypes::TopFace ? _T("Top") : _T("Bottom")) << rptNewLine;
                  (*p_table)(row, iCol++) << dim.SetValue(rebarRow.Cover) << rptNewLine;
                  (*p_table)(row, iCol++) << lrfdRebarPool::GetBarSize(rebarRow.BarSize) << rptNewLine;
                  (*p_table)(row, iCol++) << rebarRow.NumberOfBars << rptNewLine;
                  (*p_table)(row, iCol++) << dim.SetValue(rebarRow.BarSpacing) << rptNewLine;

                  if (rebarRow.BarLayout == pgsTypes::blFullLength || (rebarRow.BarLayout == pgsTypes::blFromLeft && IsZero(rebarRow.DistFromEnd)))
                  {
                     (*p_table)(row, iCol++) << (!rebarRow.bExtendedLeft ? _T("No") : _T("Yes")) << rptNewLine;
                  }
                  else
                  {
                     (*p_table)(row, iCol++) << _T("-") << rptNewLine;
                  }

                  if (rebarRow.BarLayout == pgsTypes::blFullLength || (rebarRow.BarLayout == pgsTypes::blFromRight && IsZero(rebarRow.DistFromEnd)))
                  {
                     (*p_table)(row, iCol++) << (!rebarRow.bExtendedRight ? _T("No") : _T("Yes")) << rptNewLine;
                  }
                  else
                  {
                     (*p_table)(row, iCol++) << _T("-") << rptNewLine;
                  }

               }
            }
            row++;
         }
      }
   }
}
