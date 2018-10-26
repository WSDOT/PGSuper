///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PgsExt\ReportPointOfInterest.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

int CSpanReportHint::IsMySpan(CReportHint* pHint,CReportSpecification* pRptSpec)
{
   CSpanReportSpecification* pSpanRptSpec = dynamic_cast<CSpanReportSpecification*>(pRptSpec);
   if ( pSpanRptSpec == nullptr )
   {
      return -1;
   }

   CSpanReportHint* pSpanRptHint = dynamic_cast<CSpanReportHint*>(pHint);
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

int CGirderLineReportHint::IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec)
{
   CGirderLineReportHint* pGirderRptHint = dynamic_cast<CGirderLineReportHint*>(pHint);
   if ( pGirderRptHint == nullptr )
   {
      return -1;
   }

   if (pGirderRptHint->m_GroupIdx == ALL_SPANS && pGirderRptHint->m_GirderIdx == ALL_GIRDERS)
   {
      return 1;
   }

   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   if ( pGdrRptSpec != nullptr )
   {
      return (pGirderRptHint->m_GroupIdx == pGdrRptSpec->GetGroupIndex() && pGirderRptHint->m_GirderIdx == pGdrRptSpec->GetGirderIndex() ? 1 : 0);
   }
   else
   {
      CMultiGirderReportSpecification* pMGdrRptSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
      if ( pMGdrRptSpec != nullptr )
      {
         return pMGdrRptSpec->IsMyGirder(CGirderKey(pGirderRptHint->m_GroupIdx,pGirderRptHint->m_GirderIdx));
      }

      CMultiViewSpanGirderReportSpecification* pMVGdrRptSpec = dynamic_cast<CMultiViewSpanGirderReportSpecification*>(pRptSpec);
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

int CGirderReportHint::IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec)
{
   CGirderReportHint* pGirderRptHint = dynamic_cast<CGirderReportHint*>(pHint);
   if ( pGirderRptHint == nullptr )
   {
      return -1;
   }

   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   if ( pGirderRptSpec != nullptr )
   {
      return (pGirderRptHint->m_GirderKey == pGirderRptSpec->GetGirderKey() ? 1 : 0);
   }
   else
   {
      CMultiGirderReportSpecification* pMultiGirderRptSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
      if ( pMultiGirderRptSpec != nullptr )
      {
         return pMultiGirderRptSpec->IsMyGirder(pGirderRptHint->m_GirderKey);
      }
   }

   return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CSpanReportSpecification::CSpanReportSpecification(LPCTSTR strReportName,IBroker* pBroker,SpanIndexType spanIdx) :
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
   CString msg;
   msg.Format(_T("%s - Span %d"),GetReportName().c_str(),LABEL_SPAN(GetSpan()));
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

HRESULT CSpanReportSpecification::Validate() const
{
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if ( nSpans <= m_Span )
   {
      return RPT_E_INVALID_SPAN;
   }

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CGirderReportSpecification::CGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const CGirderKey& girderKey) :
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
   GET_IFACE(IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();
   CString strGroupLabel(bIsPGSuper ? _T("Span") : _T("Group"));

   CString msg;
   msg.Format(_T("%s - %s %d Girder %s"),GetReportName().c_str(),strGroupLabel,LABEL_GROUP(m_GirderKey.groupIndex),LABEL_GIRDER(m_GirderKey.girderIndex));
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

HRESULT CGirderReportSpecification::Validate() const
{
   GET_IFACE(IBridge,pBridge);
   GirderIndexType nGirders = 0;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      nGirders = Max(nGirders,pBridge->GetGirderCount(grpIdx));
   }

   if ( nGirders <= m_GirderKey.girderIndex )
   {
      return RPT_E_INVALID_GIRDER;
   }

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CGirderLineReportSpecification::CGirderLineReportSpecification(LPCTSTR strReportName,IBroker* pBroker,GirderIndexType gdrIdx) :
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
   CString msg;
   msg.Format(_T("%s - Girder Line %s"),GetReportName().c_str(),LABEL_GIRDER(m_GirderIdx));
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

HRESULT CGirderLineReportSpecification::Validate() const
{
   GET_IFACE(IBridge,pBridge);
   GirderIndexType nGirders = 0;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      nGirders = Max(nGirders,pBridge->GetGirderCount(grpIdx));
   }

   if ( nGirders <= m_GirderIdx )
   {
      return RPT_E_INVALID_GIRDER;
   }

   return CBrokerReportSpecification::Validate();
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CMultiGirderReportSpecification::CMultiGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const std::vector<CGirderKey>& girderKeys) :
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
   return GetReportName();
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

HRESULT CMultiGirderReportSpecification::Validate() const
{
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   std::vector<CGirderKey>::const_iterator iter(m_GirderKeys.begin());
   std::vector<CGirderKey>::const_iterator end(m_GirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      const CGirderKey& girderKey(*iter);

      if ( nGroups <= girderKey.groupIndex )
      {
         return RPT_E_INVALID_GROUP;
      }

      GirderIndexType nGirders = pBridge->GetGirderCount(girderKey.groupIndex);
      if ( nGirders <= girderKey.girderIndex )
      {
         return RPT_E_INVALID_GIRDER;
      }
   }

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

CMultiViewSpanGirderReportSpecification::CMultiViewSpanGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker, const std::vector<CGirderKey>& girderKeys) :
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

HRESULT CMultiViewSpanGirderReportSpecification::Validate() const
{
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   std::vector<CGirderKey>::const_iterator iter(m_GirderKeys.begin());
   std::vector<CGirderKey>::const_iterator end(m_GirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      const CGirderKey& girderKey(*iter);

      if ( nGroups <= girderKey.groupIndex )
      {
         return RPT_E_INVALID_SPAN;
      }

      GirderIndexType nGdrs = pBridge->GetGirderCount(girderKey.groupIndex);

      if ( nGdrs <= girderKey.groupIndex )
      {
         return RPT_E_INVALID_GIRDER;
      }
   }

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CPointOfInterestReportSpecification::CPointOfInterestReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const pgsPointOfInterest& poi) :
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
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE(rptPointOfInterest, rptPOI, pDisplayUnits->GetSpanLengthUnit(), true);
   rptPOI.SetValue(POI_SPAN,m_POI);

   CString msg;
   msg.Format(_T("%s - %s"),GetReportName().c_str(),rptPOI.AsString().c_str());
   return std::_tstring(msg);
}

void CPointOfInterestReportSpecification::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

const pgsPointOfInterest& CPointOfInterestReportSpecification::GetPointOfInterest() const
{
   return m_POI;
}

HRESULT CPointOfInterestReportSpecification::Validate() const
{
   return CBrokerReportSpecification::Validate();
}
