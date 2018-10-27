///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// PGSuperGrapherImp.cpp : Implementation of CPGSuperGrapherImp
#include "stdafx.h"
#include "PGSuperGrapherImp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//CPGSuperGrapherImp::InitGraphBuilders
//
//Initialize graph builders at project load time
//
//Initialize graph builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

HRESULT CPGSuperGrapherImp::InitGraphBuilders()
{
   CGrapherBase::InitCommonGraphBuilders();
   return S_OK;
}

STDMETHODIMP CPGSuperGrapherImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   CGrapherBase::SetBroker(pBroker);
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperGrapherImp::RegInterfaces()
{
   //CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   // this agent doesn't implement any interfaces... it just provides graphs

   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperGrapherImp::Init()
{
   /* Gets done at project load time */
   EAF_AGENT_INIT;

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CPGSuperGrapherImp::Init2()
{
   return InitGraphBuilders();
}

STDMETHODIMP CPGSuperGrapherImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_PGSuperGraphingAgent;
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperGrapherImp::Reset()
{
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperGrapherImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}
