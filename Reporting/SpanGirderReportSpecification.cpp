///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\SpanGirderReportSpecification.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PgsExt\ReportPointOfInterest.h>


/////////////////////////////////////////////////////////////////////////////////////////////
CSpanReportHint::CSpanReportHint()
{
   m_SpanIdx = INVALID_INDEX;
}

CSpanReportHint::CSpanReportHint(SpanIndexType spanIdx) :
m_SpanIdx(spanIdx)
{
}

void CSpanReportHint::SetSpan(SpanIndexType spanIdx)
{
   m_SpanIdx = spanIdx;
}

SpanIndexType CSpanReportHint::GetSpan()
{
   return m_SpanIdx;
}

int CSpanReportHint::IsMySpan(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec)
{
   auto pSpanRptSpec = std::dynamic_pointer_cast<const CSpanReportSpecification>(pRptSpec);
   if ( pSpanRptSpec == nullptr )
   {
      return -1;
   }

   auto pSpanRptHint = std::dynamic_pointer_cast<const CSpanReportHint>(pHint);
   if ( pSpanRptHint == nullptr )
   {
      return -1;
   }

   return (pSpanRptHint->m_SpanIdx == pSpanRptSpec->GetSpan() ? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
CGirderLineReportHint::CGirderLineReportHint()
{
   m_GroupIdx = INVALID_INDEX;
   m_GirderIdx = INVALID_INDEX;
}

CGirderLineReportHint::CGirderLineReportHint(GroupIndexType grpIdx,GirderIndexType gdrIdx) :
m_GroupIdx(grpIdx),
m_GirderIdx(gdrIdx)
{
}

void CGirderLineReportHint::SetGroupIndex(GroupIndexType grpIdx)
{
   m_GroupIdx = grpIdx;
}

GroupIndexType CGirderLineReportHint::GetGroupIndex() const
{
   return m_GroupIdx;
}

void CGirderLineReportHint::SetGirderIndex(GirderIndexType gdrIdx)
{
   m_GirderIdx = gdrIdx;
}

GirderIndexType CGirderLineReportHint::GetGirderIndex() const
{
   return m_GirderIdx;
}

int CGirderLineReportHint::IsMyGirder(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint, const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec)
{
   auto pGirderRptHint = std::dynamic_pointer_cast<const CGirderLineReportHint>(pHint);
   if ( pGirderRptHint == nullptr )
   {
      return -1;
   }

   if (pGirderRptHint->m_GroupIdx == ALL_SPANS && pGirderRptHint->m_GirderIdx == ALL_GIRDERS)
   {
      return 1;
   }

   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   if ( pGdrRptSpec != nullptr )
   {
      return (pGirderRptHint->m_GroupIdx == pGdrRptSpec->GetGroupIndex() && pGirderRptHint->m_GirderIdx == pGdrRptSpec->GetGirderIndex() ? 1 : 0);
   }
   else
   {
      auto pMGdrRptSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);
      if ( pMGdrRptSpec != nullptr )
      {
         return pMGdrRptSpec->IsMyGirder(CGirderKey(pGirderRptHint->m_GroupIdx,pGirderRptHint->m_GirderIdx));
      }

      auto pMVGdrRptSpec = std::dynamic_pointer_cast<const CMultiViewSpanGirderReportSpecification>(pRptSpec);
      if ( pMVGdrRptSpec != nullptr )
      {
         return pMVGdrRptSpec->IsMyGirder(CGirderKey(pGirderRptHint->m_GroupIdx ,pGirderRptHint->m_GirderIdx));
      }
   }

   return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

CGirderReportHint::CGirderReportHint()
{
   m_Hint    = 0;
}

CGirderReportHint::CGirderReportHint(const CGirderKey& girderKey,Uint32 lHint) :
m_GirderKey(girderKey),m_Hint(lHint)
{
}

void CGirderReportHint::SetHint(Uint32 lHint)
{
   m_Hint = lHint;
}

Uint32 CGirderReportHint::GetHint()
{
   return m_Hint;
}

void CGirderReportHint::SetGirderKey(const CGirderKey& girderKey)
{
   m_GirderKey = girderKey;
}

const CGirderKey& CGirderReportHint::GetGirderKey() const
{
   return m_GirderKey;
}

int CGirderReportHint::IsMyGirder(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint, const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec)
{
   auto pGirderRptHint = std::dynamic_pointer_cast<const CGirderReportHint>(pHint);
   if ( pGirderRptHint == nullptr )
   {
      return -1;
   }

   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   if ( pGirderRptSpec != nullptr )
   {
      return (pGirderRptHint->m_GirderKey == pGirderRptSpec->GetGirderKey() ? 1 : 0);
   }
   else
   {
      auto pMultiGirderRptSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);
      if ( pMultiGirderRptSpec != nullptr )
      {
         return pMultiGirderRptSpec->IsMyGirder(pGirderRptHint->m_GirderKey);
      }
   }

   return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CSpanReportSpecification::CSpanReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker>pBroker,SpanIndexType spanIdx) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetSpan(spanIdx);
}

CSpanReportSpecification::CSpanReportSpecification(const CSpanReportSpecification& other) :
CBrokerReportSpecification(other)
{
   SetSpan(other.m_Span);
}

CSpanReportSpecification::CSpanReportSpecification(const CBrokerReportSpecification& other,SpanIndexType spanIdx):
CBrokerReportSpecification(other)
{
   SetSpan(spanIdx);
}

CSpanReportSpecification::~CSpanReportSpecification(void)
{
}

std::_tstring CSpanReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

std::_tstring CSpanReportSpecification::GetReportContextString() const
{
   CString msg;
   msg.Format(_T("Span %s"),LABEL_SPAN(GetSpan()));
   return std::_tstring(msg);
}

void CSpanReportSpecification::SetSpan(SpanIndexType spanIdx)
{
   m_Span = spanIdx;
}

SpanIndexType CSpanReportSpecification::GetSpan() const
{
   return m_Span;
}

bool CSpanReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if ( nSpans <= m_Span )
   {
      return false;
   }

   return CBrokerReportSpecification::IsValid();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CGirderReportSpecification::CGirderReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker>pBroker,const CGirderKey& girderKey) :
CBrokerReportSpecification(strReportName,pBroker)
{
   m_GirderKey = girderKey;
}

CGirderReportSpecification::CGirderReportSpecification(const CGirderReportSpecification& other) :
CBrokerReportSpecification(other)
{
   m_GirderKey = other.m_GirderKey;
}

CGirderReportSpecification::~CGirderReportSpecification(void)
{
}

std::_tstring CGirderReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

std::_tstring CGirderReportSpecification::GetReportContextString() const
{
   GroupIndexType grpIdx  = m_GirderKey.groupIndex;
   GirderIndexType gdrIdx = m_GirderKey.girderIndex;

   GET_IFACE2(GetBroker(),IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   CString msg;
   if (bIsPGSuper)
   {
      if ( grpIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX )
      {
         msg.Format(_T("Span %s Girder %s"), LABEL_SPAN(m_GirderKey.groupIndex), LABEL_GIRDER(m_GirderKey.girderIndex));
      }
      else if( grpIdx != INVALID_INDEX )
      {
         msg.Format(_T("Span %s"), LABEL_SPAN(m_GirderKey.groupIndex));
      }
      else if ( gdrIdx != INVALID_INDEX )
      {
         msg.Format(_T("Girder %s"), LABEL_GIRDER(m_GirderKey.girderIndex));
      }
   }
   else
   {
      if ( grpIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX )
      {
         msg.Format(_T("Group %d Girder %s"), LABEL_GROUP(m_GirderKey.groupIndex), LABEL_GIRDER(m_GirderKey.girderIndex));
      }
      else if( grpIdx != INVALID_INDEX )
      {
         msg.Format(_T("Group %d"), LABEL_GROUP(m_GirderKey.groupIndex));
      }
      else if ( gdrIdx != INVALID_INDEX )
      {
         msg.Format(_T("Girder %s"), LABEL_GIRDER(m_GirderKey.girderIndex));
      }
   }

   return std::_tstring(msg);
}

void CGirderReportSpecification::SetGroupIndex(GroupIndexType grpIdx)
{
   m_GirderKey.groupIndex = grpIdx;
}

GroupIndexType CGirderReportSpecification::GetGroupIndex() const
{
   return m_GirderKey.groupIndex;
}

void CGirderReportSpecification::SetGirderIndex(GirderIndexType gdrIdx)
{
   m_GirderKey.girderIndex = gdrIdx;
}

GirderIndexType CGirderReportSpecification::GetGirderIndex() const
{
   return m_GirderKey.girderIndex;
}

void CGirderReportSpecification::SetGirderKey(const CGirderKey& girderKey)
{
   m_GirderKey = girderKey;
}

const CGirderKey& CGirderReportSpecification::GetGirderKey() const
{
   return m_GirderKey;
}

bool CGirderReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if (nGroups <= m_GirderKey.groupIndex && m_GirderKey.groupIndex != ALL_GROUPS)
   {
      // the group index is out of range (group probably got deleted)
      return false;
   }

   GirderIndexType nGirders = 0;
   if (m_GirderKey.groupIndex == ALL_GROUPS)
   {
      for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
      {
         nGirders = Max(nGirders, pBridge->GetGirderCount(grpIdx));
      }
   }
   else
   {
      nGirders = pBridge->GetGirderCount(m_GirderKey.groupIndex);
   }

   if ( nGirders <= m_GirderKey.girderIndex )
   {
      return false;
   }

   return CBrokerReportSpecification::IsValid();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CSegmentReportSpecification::CSegmentReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey) :
   CBrokerReportSpecification(strReportName, pBroker)
{
   m_SegmentKey = segmentKey;
}

CSegmentReportSpecification::CSegmentReportSpecification(const CSegmentReportSpecification& other) :
   CBrokerReportSpecification(other)
{
   m_SegmentKey = other.m_SegmentKey;
}

CSegmentReportSpecification::~CSegmentReportSpecification(void)
{
}

std::_tstring CSegmentReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

std::_tstring CSegmentReportSpecification::GetReportContextString() const
{
   GET_IFACE2(GetBroker(),IDocumentType, pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();
   CString strGroupLabel(bIsPGSuper ? _T("Span") : _T("Group"));
   ATLASSERT(bIsPGSuper ? m_SegmentKey.segmentIndex == 0 : true);

   CString msg;
   if (bIsPGSuper)
   {
      msg.Format(_T("%s %d Girder %s"), strGroupLabel, LABEL_GROUP(m_SegmentKey.groupIndex), LABEL_GIRDER(m_SegmentKey.girderIndex));
   }
   else
   {
      msg.Format(_T("%s %d Girder %s Segment %d"), strGroupLabel, LABEL_GROUP(m_SegmentKey.groupIndex), LABEL_GIRDER(m_SegmentKey.girderIndex), LABEL_SEGMENT(m_SegmentKey.segmentIndex));
   }

   return std::_tstring(msg);
}

void CSegmentReportSpecification::SetGroupIndex(GroupIndexType grpIdx)
{
   m_SegmentKey.groupIndex = grpIdx;
}

GroupIndexType CSegmentReportSpecification::GetGroupIndex() const
{
   return m_SegmentKey.groupIndex;
}

void CSegmentReportSpecification::SetGirderIndex(GirderIndexType gdrIdx)
{
   m_SegmentKey.girderIndex = gdrIdx;
}

GirderIndexType CSegmentReportSpecification::GetGirderIndex() const
{
   return m_SegmentKey.girderIndex;
}

void CSegmentReportSpecification::SetSegmentIndex(SegmentIndexType segIdx)
{
   m_SegmentKey.girderIndex = segIdx;
}

GirderIndexType CSegmentReportSpecification::GetSegmentIndex() const
{
   return m_SegmentKey.segmentIndex;
}

void CSegmentReportSpecification::SetSegmentKey(const CSegmentKey& segmentKey)
{
   m_SegmentKey = segmentKey;
}

const CSegmentKey& CSegmentReportSpecification::GetSegmentKey() const
{
   return m_SegmentKey;
}

bool CSegmentReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge, pBridge);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if (nGroups <= m_SegmentKey.groupIndex)
   {
      // the group index is out of range (group probably got deleted)
      return false;
   }

   GirderIndexType nGirders = pBridge->GetGirderCount(m_SegmentKey.groupIndex);
   if (nGirders <= m_SegmentKey.girderIndex)
   {
      return false;
   }

   SegmentIndexType nSegments = pBridge->GetSegmentCount(m_SegmentKey.groupIndex, m_SegmentKey.girderIndex);

   return CBrokerReportSpecification::IsValid();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CGirderLineReportSpecification::CGirderLineReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,GirderIndexType gdrIdx) :
CBrokerReportSpecification(strReportName,pBroker)
{
   m_GirderIdx = gdrIdx;
}

CGirderLineReportSpecification::CGirderLineReportSpecification(const CGirderLineReportSpecification& other) :
CBrokerReportSpecification(other)
{
   m_GirderIdx = other.m_GirderIdx;
}

CGirderLineReportSpecification::~CGirderLineReportSpecification(void)
{
}

std::_tstring CGirderLineReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

std::_tstring CGirderLineReportSpecification::GetReportContextString() const
{
   CString msg;
   msg.Format(_T("Girder Line %s"), LABEL_GIRDER(m_GirderIdx));
   return std::_tstring(msg);
}

void CGirderLineReportSpecification::SetGirderIndex(GirderIndexType gdrIdx)
{
   m_GirderIdx = gdrIdx;
}

GirderIndexType CGirderLineReportSpecification::GetGirderIndex() const
{
   return m_GirderIdx;
}

CGirderKey CGirderLineReportSpecification::GetGirderKey() const
{
   return CGirderKey(ALL_GROUPS,m_GirderIdx);
}

bool CGirderLineReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GirderIndexType nGirders = 0;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      nGirders = Max(nGirders,pBridge->GetGirderCount(grpIdx));
   }

   if ( nGirders <= m_GirderIdx )
   {
      return false;
   }

   return CBrokerReportSpecification::IsValid();
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CMultiGirderReportSpecification::CMultiGirderReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,const std::vector<CGirderKey>& girderKeys) :
CBrokerReportSpecification(strReportName,pBroker),
m_GirderKeys(girderKeys)
{
}

CMultiGirderReportSpecification::CMultiGirderReportSpecification(const CMultiGirderReportSpecification& other) :
CBrokerReportSpecification(other)
{
   m_GirderKeys = other.m_GirderKeys;
}

CMultiGirderReportSpecification::~CMultiGirderReportSpecification(void)
{
}

std::_tstring CMultiGirderReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

// comparator to sort girder keys
struct gkeyComparer
{
   bool operator() (CGirderKey i, CGirderKey j)
   {
      if (i.groupIndex == j.groupIndex)
      {
         return i.girderIndex < j.girderIndex;
      }
      else
      {
         return i.groupIndex < j.groupIndex;
      }
   }
};

std::_tstring CMultiGirderReportSpecification::GetReportContextString() const
{
   GET_IFACE2(GetBroker(),IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // sort so that keys are grouped by span
   std::vector<CGirderKey> Keys(m_GirderKeys);
   gkeyComparer comp;
   std::sort(Keys.begin(), Keys.end(), comp);

   std::_tostringstream os;
   bool bFirstGrp(true);
   GroupIndexType curGrp = Keys.front().groupIndex;
   for (auto& key : Keys)
   {
      if (bFirstGrp || key.groupIndex != curGrp)
      {
         if (!bFirstGrp)
         {
            os << _T("), ");
         }

         if (bIsPGSuper)
         {
            os << _T("Span ") << LABEL_SPAN(key.groupIndex) << _T(" (Girder ") << LABEL_GIRDER(key.girderIndex);
         }
         else
         {
            os << _T("Group ") << LABEL_GROUP(key.groupIndex) << _T(" (Girder ") << LABEL_GIRDER(key.girderIndex);
         }

         curGrp = key.groupIndex;
      }
      else
      {
         os << _T(",") << LABEL_GIRDER(key.girderIndex);
      }

      bFirstGrp = false;
   }

   os << _T(")");

   return os.str();
}

void CMultiGirderReportSpecification::SetGirderKeys(const std::vector<CGirderKey>& girderKeys)
{
   m_GirderKeys = girderKeys;
}

const std::vector<CGirderKey>& CMultiGirderReportSpecification::GetGirderKeys() const
{
   return m_GirderKeys;
}

bool CMultiGirderReportSpecification::IsMyGirder(const CGirderKey& girderKey) const
{
   std::vector<CGirderKey>::const_iterator it = std::find(m_GirderKeys.begin(), m_GirderKeys.end(), girderKey);

   return (it != m_GirderKeys.end()) ? true : false;
}

bool CMultiGirderReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   std::vector<CGirderKey>::const_iterator iter(m_GirderKeys.begin());
   std::vector<CGirderKey>::const_iterator end(m_GirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      const CGirderKey& girderKey(*iter);

      if ( nGroups <= girderKey.groupIndex )
      {
         return false;
      }

      GirderIndexType nGirders = pBridge->GetGirderCount(girderKey.groupIndex);
      if ( nGirders <= girderKey.girderIndex )
      {
         return false;
      }
   }

   return CBrokerReportSpecification::IsValid();
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

CMultiViewSpanGirderReportSpecification::CMultiViewSpanGirderReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderKeys) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetGirderKeys(girderKeys);
}

CMultiViewSpanGirderReportSpecification::CMultiViewSpanGirderReportSpecification(const CMultiViewSpanGirderReportSpecification& other) :
CBrokerReportSpecification(other)
{
   SetGirderKeys(other.m_GirderKeys);
}

CMultiViewSpanGirderReportSpecification::~CMultiViewSpanGirderReportSpecification(void)
{
}

std::_tstring CMultiViewSpanGirderReportSpecification::GetReportTitle() const
{
   return GetReportName();
}

std::_tstring CMultiViewSpanGirderReportSpecification::GetReportContextString() const
{
   return std::_tstring(); // may want to revisit
}

void CMultiViewSpanGirderReportSpecification::SetGirderKeys(const std::vector<CGirderKey>& girderKeys)
{
   m_GirderKeys = girderKeys;
}

const std::vector<CGirderKey>& CMultiViewSpanGirderReportSpecification::GetGirderKeys() const
{
   return m_GirderKeys;
}

int CMultiViewSpanGirderReportSpecification::IsMyGirder(const CGirderKey& girderKey) const
{
   std::vector<CGirderKey>::const_iterator it = std::find(m_GirderKeys.begin(), m_GirderKeys.end(), girderKey);

   return (it != m_GirderKeys.end()) ? 1 : 0;
}

bool CMultiViewSpanGirderReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   std::vector<CGirderKey>::const_iterator iter(m_GirderKeys.begin());
   std::vector<CGirderKey>::const_iterator end(m_GirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      const CGirderKey& girderKey(*iter);

      if ( nGroups <= girderKey.groupIndex )
      {
         return false;
      }

      GirderIndexType nGdrs = pBridge->GetGirderCount(girderKey.groupIndex);

      if ( nGdrs <= girderKey.groupIndex )
      {
         return false;
      }
   }

   return CBrokerReportSpecification::IsValid();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CPointOfInterestReportSpecification::CPointOfInterestReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi) :
CBrokerReportSpecification(strReportName,pBroker)
{
   m_POI = poi;
}

CPointOfInterestReportSpecification::CPointOfInterestReportSpecification(const CPointOfInterestReportSpecification& other) :
CBrokerReportSpecification(other)
{
   m_POI = other.m_POI;
}

CPointOfInterestReportSpecification::~CPointOfInterestReportSpecification(void)
{
}

std::_tstring CPointOfInterestReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

std::_tstring CPointOfInterestReportSpecification::GetReportContextString() const
{
   GET_IFACE2(GetBroker(),IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE(rptPointOfInterest, rptPOI, pDisplayUnits->GetSpanLengthUnit(), true);
   rptPOI.SetValue(POI_SPAN,m_POI);

   return rptPOI.AsString();
}

void CPointOfInterestReportSpecification::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

const pgsPointOfInterest& CPointOfInterestReportSpecification::GetPointOfInterest() const
{
   return m_POI;
}

bool CPointOfInterestReportSpecification::IsValid() const
{
   return CBrokerReportSpecification::IsValid();
}
