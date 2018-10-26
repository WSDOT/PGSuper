///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   if ( pGdrRptSpec == NULL )
      return -1;

   CGirderReportHint* pGirderRptHint = dynamic_cast<CGirderReportHint*>(pHint);
   if ( pGirderRptHint == NULL )
      return -1;

   return (pGirderRptHint->m_GdrIdx == pGdrRptSpec->GetGirder() ? 1 : 0);
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
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   if ( pSGRptSpec == NULL )
      return -1;

   CSpanGirderReportHint* pSGRptHint = dynamic_cast<CSpanGirderReportHint*>(pHint);
   if ( pSGRptHint == NULL )
      return -1;

   return (pSGRptHint->m_SpanIdx == pSGRptSpec->GetSpan() && pSGRptHint->m_GdrIdx == pSGRptSpec->GetGirder() ? 1 : false);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CSpanReportSpecification::CSpanReportSpecification(LPCTSTR strReportName,IBroker* pBroker,SpanIndexType spanIdx) :
CBrokerReportSpecification(strReportName,pBroker)
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
