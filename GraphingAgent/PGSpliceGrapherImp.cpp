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

// PGSpliceGraphReporterImp.cpp : Implementation of CPGSpliceGrapherImp
#include "stdafx.h"
#include "GraphingAgent_i.h"
#include "PGSpliceGrapherImp.h"
#include <Graphing\DeflectionHistoryGraphBuilder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//CPGSpliceGrapherImp::InitGraphBuilders
//
//Initialize graph builders at project load time
//
//Initialize graph builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

HRESULT CPGSpliceGrapherImp::InitGraphBuilders()
{
   CGrapherBase::InitCommonGraphBuilders();

   GET_IFACE(IGraphManager,pGraphMgr);
   pGraphMgr->AddGraphBuilder(new CDeflectionHistoryGraphBuilder);

   return S_OK;
}

STDMETHODIMP CPGSpliceGrapherImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   CGrapherBase::SetBroker(pBroker);
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceGrapherImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   // this agent doesn't implement any interfaces... it just provides graphs

   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceGrapherImp::Init()
{
   /* Gets done at project load time */
   EAF_AGENT_INIT;

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CPGSpliceGrapherImp::Init2()
{
   return InitGraphBuilders();
}

STDMETHODIMP CPGSpliceGrapherImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_PGSpliceGraphingAgent;
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceGrapherImp::Reset()
{
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceGrapherImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}
