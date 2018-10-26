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

#include "stdafx.h"
#include <Reporting\SpanGirderReportSpecification.h>
#include <IFace\Bridge.h>

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
   if ( pSpanRptSpec == NULL )
      return -1;

   CSpanReportHint* pSpanRptHint = dynamic_cast<CSpanReportHint*>(pHint);
   if ( pSpanRptHint == NULL )
      return -1;

   return (pSpanRptHint->m_SpanIdx == pSpanRptSpec->GetSpan() ? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
CGirderReportHint::CGirderReportHint()
{
   m_GdrIdx = INVALID_INDEX;
}

CGirderReportHint::CGirderReportHint(GirderIndexType gdrIdx) :
m_GdrIdx(gdrIdx)
{
}

void CGirderReportHint::SetGirder(GirderIndexType gdrIdx)
{
   m_GdrIdx = gdrIdx;
}

GirderIndexType CGirderReportHint::GetGirder()
{
   return m_GdrIdx;
}

int CGirderReportHint::IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec)
{
   CGirderReportHint* pGirderRptHint = dynamic_cast<CGirderReportHint*>(pHint);
   if ( pGirderRptHint == NULL )
      return -1;

   if (pGirderRptHint->m_GdrIdx == ALL_GIRDERS)
      return 1;

   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   if ( pGdrRptSpec != NULL )
   {
      return (pGirderRptHint->m_GdrIdx == pGdrRptSpec->GetGirder() ? 1 : 0);
   }
   else
   {
      CMultiGirderReportSpecification* pMGdrRptSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
      if ( pMGdrRptSpec != NULL )
      {
         return pMGdrRptSpec->IsMyGirder(0 ,pGirderRptHint->m_GdrIdx); // only look at span 0
      }

      CMultiViewSpanGirderReportSpecification* pMVGdrRptSpec = dynamic_cast<CMultiViewSpanGirderReportSpecification*>(pRptSpec);
      if ( pMVGdrRptSpec != NULL )
      {
         return pMVGdrRptSpec->IsMyGirder(0 ,pGirderRptHint->m_GdrIdx); // only look at span 0
      }
   }

   return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

CSpanGirderReportHint::CSpanGirderReportHint()
{
   m_SpanIdx = INVALID_INDEX;
   m_GdrIdx  = INVALID_INDEX;
   m_Hint    = 0;
}

CSpanGirderReportHint::CSpanGirderReportHint(SpanIndexType spanIdx,GirderIndexType gdrIdx,Uint32 lHint) :
m_SpanIdx(spanIdx), m_GdrIdx(gdrIdx),m_Hint(lHint)
{
}

void CSpanGirderReportHint::SetHint(Uint32 lHint)
{
   m_Hint = lHint;
}

Uint32 CSpanGirderReportHint::GetHint()
{
   return m_Hint;
}

void CSpanGirderReportHint::SetGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   m_SpanIdx = spanIdx;
   m_GdrIdx  = gdrIdx;
}

void CSpanGirderReportHint::GetGirder(SpanIndexType& spanIdx,GirderIndexType& gdrIdx)
{
   spanIdx = m_SpanIdx;
   gdrIdx  = m_GdrIdx;
}

int CSpanGirderReportHint::IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec)
{
   CSpanGirderReportHint* pSGRptHint = dynamic_cast<CSpanGirderReportHint*>(pHint);
   if ( pSGRptHint == NULL )
      return -1;

   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   if ( pSGRptSpec != NULL )
   { 
      if( pSGRptHint->m_SpanIdx == pSGRptSpec->GetSpan() && pSGRptHint->m_GdrIdx == pSGRptSpec->GetGirder() )
      {
         return 1;
      }
      else if ( pSGRptHint->m_SpanIdx == ALL_SPANS && pSGRptHint->m_GdrIdx == ALL_GIRDERS )
      {
         return 1;
      }
      else if ( pSGRptHint->m_SpanIdx == ALL_SPANS && pSGRptHint->m_GdrIdx == pSGRptSpec->GetGirder() )
      {
         return 1;
      }
      else if( pSGRptHint->m_SpanIdx == pSGRptSpec->GetSpan() && pSGRptHint->m_GdrIdx == ALL_GIRDERS )
      {
         return 1;
      }
      else
      {
         return 0;
      }
   }
   else
   {
      CMultiGirderReportSpecification* pMGdrRptSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
      if ( pMGdrRptSpec != NULL )
      {
         return pMGdrRptSpec->IsMyGirder(pSGRptHint->m_SpanIdx ,pSGRptHint->m_GdrIdx);
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
   GET_IFACE2(m_Broker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if ( nSpans <= m_Span )
      return RPT_E_INVALIDSPAN;

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CGirderReportSpecification::CGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,GirderIndexType gdrIdx) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetGirder(gdrIdx);
}

CGirderReportSpecification::CGirderReportSpecification(const CGirderReportSpecification& other) :
CBrokerReportSpecification(other)
{
   SetGirder(other.m_Girder);
}

CGirderReportSpecification::~CGirderReportSpecification(void)
{
}

std::_tstring CGirderReportSpecification::GetReportTitle() const
{
   CString msg;
   msg.Format(_T("%s - Girder Line %s"),GetReportName().c_str(),LABEL_GIRDER(GetGirder()));
   return std::_tstring(msg);
}

void CGirderReportSpecification::SetGirder(GirderIndexType gdrIdx)
{
   m_Girder = gdrIdx;
}

GirderIndexType CGirderReportSpecification::GetGirder() const
{
   return m_Girder;
}

HRESULT CGirderReportSpecification::Validate() const
{
   GET_IFACE2(m_Broker,IBridge,pBridge);
   GirderIndexType nGirders = 0;

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      nGirders = max(nGirders,pBridge->GetGirderCount(spanIdx));
   }

   if ( nGirders <= m_Girder )
      return RPT_E_INVALIDGIRDER;

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
CSpanGirderReportSpecification::CSpanGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx) :
CSpanReportSpecification(strReportName,pBroker,spanIdx)
{
   SetGirder(gdrIdx);
}

CSpanGirderReportSpecification::CSpanGirderReportSpecification(const CSpanGirderReportSpecification& other) :
CSpanReportSpecification(other)
{
   SetGirder(other.m_Girder);
}

CSpanGirderReportSpecification::CSpanGirderReportSpecification(const CBrokerReportSpecification& other,SpanIndexType spanIdx,GirderIndexType gdrIdx):
CSpanReportSpecification(other,spanIdx)
{
   SetGirder(gdrIdx);
}

CSpanGirderReportSpecification::~CSpanGirderReportSpecification(void)
{
}

std::_tstring CSpanGirderReportSpecification::GetReportTitle() const
{
   CString msg;
   msg.Format(_T("%s - Span %d, Girder %s"),GetReportName().c_str(),LABEL_SPAN(GetSpan()),LABEL_GIRDER(GetGirder()));
   return std::_tstring(msg);
}

void CSpanGirderReportSpecification::SetGirder(GirderIndexType gdrIdx)
{
   m_Girder = gdrIdx;
}

GirderIndexType CSpanGirderReportSpecification::GetGirder() const
{
   return m_Girder;
}

HRESULT CSpanGirderReportSpecification::Validate() const
{
   HRESULT hr = CSpanReportSpecification::Validate();
   if ( FAILED(hr) )
      return hr;

   GET_IFACE2(m_Broker,IBridge,pBridge);
   GirderIndexType nGirders = 0;

   if ( m_Span == ALL_SPANS )
   {
      SpanIndexType nSpans = pBridge->GetSpanCount();
      for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         nGirders = max(nGirders,pBridge->GetGirderCount(spanIdx));
      }
   }
   else
   {
      nGirders = pBridge->GetGirderCount(m_Span);
   }

   if ( nGirders <= m_Girder )
      return RPT_E_INVALIDGIRDER;

   return S_OK;

}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CMultiGirderReportSpecification::CMultiGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker, const std::vector<SpanGirderHashType>& girderlist) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetGirderList(girderlist);
}

CMultiGirderReportSpecification::CMultiGirderReportSpecification(const CMultiGirderReportSpecification& other) :
CBrokerReportSpecification(other)
{
   SetGirderList(other.m_GirderList);
}

CMultiGirderReportSpecification::~CMultiGirderReportSpecification(void)
{
}

std::_tstring CMultiGirderReportSpecification::GetReportTitle() const
{
   return GetReportName();
}

void CMultiGirderReportSpecification::SetGirderList(const std::vector<SpanGirderHashType>& girderlist)
{
   m_GirderList = girderlist;
}

std::vector<SpanGirderHashType> CMultiGirderReportSpecification::GetGirderList() const
{
   return m_GirderList;
}

int CMultiGirderReportSpecification::IsMyGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx) const
{
   SpanGirderHashType hash = HashSpanGirder(spanIdx, gdrIdx);

   std::vector<SpanGirderHashType>::const_iterator it = std::find(m_GirderList.begin(), m_GirderList.end(), hash);

   return (it != m_GirderList.end()) ? 1 : 0;
}

HRESULT CMultiGirderReportSpecification::Validate() const
{
   GET_IFACE2(m_Broker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   for (std::vector<SpanGirderHashType>::const_iterator it=m_GirderList.begin(); it!=m_GirderList.end(); it++)
   {
      SpanIndexType spanIdx;
      GirderIndexType gdrIdx;
      UnhashSpanGirder(*it,&spanIdx,&gdrIdx);

      if ( nSpans <= spanIdx )
         return RPT_E_INVALIDSPAN;

      GirderIndexType nGdrs = pBridge->GetGirderCount(spanIdx);

      if ( nGdrs <= gdrIdx )
         return RPT_E_INVALIDGIRDER;
   }

   return CBrokerReportSpecification::Validate();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CMultiViewSpanGirderReportSpecification::CMultiViewSpanGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker, const std::vector<SpanGirderHashType>& girderlist) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetGirderList(girderlist);
}

CMultiViewSpanGirderReportSpecification::CMultiViewSpanGirderReportSpecification(const CMultiViewSpanGirderReportSpecification& other) :
CBrokerReportSpecification(other)
{
   SetGirderList(other.m_GirderList);
}

CMultiViewSpanGirderReportSpecification::~CMultiViewSpanGirderReportSpecification(void)
{
}

std::_tstring CMultiViewSpanGirderReportSpecification::GetReportTitle() const
{
   return GetReportName();
}

void CMultiViewSpanGirderReportSpecification::SetGirderList(const std::vector<SpanGirderHashType>& girderlist)
{
   m_GirderList = girderlist;
}

std::vector<SpanGirderHashType> CMultiViewSpanGirderReportSpecification::GetGirderList() const
{
   return m_GirderList;
}

int CMultiViewSpanGirderReportSpecification::IsMyGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx) const
{
   SpanGirderHashType hash = HashSpanGirder(spanIdx, gdrIdx);

   std::vector<SpanGirderHashType>::const_iterator it = std::find(m_GirderList.begin(), m_GirderList.end(), hash);

   return (it != m_GirderList.end()) ? 1 : 0;
}

HRESULT CMultiViewSpanGirderReportSpecification::Validate() const
{
   GET_IFACE2(m_Broker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   for (std::vector<SpanGirderHashType>::const_iterator it=m_GirderList.begin(); it!=m_GirderList.end(); it++)
   {
      SpanIndexType spanIdx;
      GirderIndexType gdrIdx;
      UnhashSpanGirder(*it,&spanIdx,&gdrIdx);

      if ( nSpans <= spanIdx )
         return RPT_E_INVALIDSPAN;

      GirderIndexType nGdrs = pBridge->GetGirderCount(spanIdx);

      if ( nGdrs <= gdrIdx )
         return RPT_E_INVALIDGIRDER;
   }

   return CBrokerReportSpecification::Validate();
}
