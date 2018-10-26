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

CSpanReportSpecification::CSpanReportSpecification(const char* strReportName,IBroker* pBroker,SpanIndexType spanIdx) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetSpan(spanIdx);
}

CSpanReportSpecification::~CSpanReportSpecification(void)
{
}

std::string CSpanReportSpecification::GetReportTitle() const
{
   CString msg;
   msg.Format("%s - Span %d",GetReportName().c_str(),LABEL_SPAN(GetSpan()));
   return std::string(msg);
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
CSpanGirderReportSpecification::CSpanGirderReportSpecification(const char* strReportName,IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx) :
CSpanReportSpecification(strReportName,pBroker,spanIdx)
{
   SetGirder(gdrIdx);
}

CSpanGirderReportSpecification::~CSpanGirderReportSpecification(void)
{
}

std::string CSpanGirderReportSpecification::GetReportTitle() const
{
   CString msg;
   msg.Format("%s - Span %d, Girder %s",GetReportName().c_str(),LABEL_SPAN(GetSpan()),LABEL_GIRDER(GetGirder()));
   return std::string(msg);
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
   GirderIndexType nGirders = pBridge->GetGirderCount(m_Span);
   if ( nGirders <= m_Girder )
      return RPT_E_INVALIDGIRDER;

   return S_OK;
}
