///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2009  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <atlcomcli.h> // for CAdapt
#include <WBFLCore.h>
#include <map>

using namespace ATL;

// provides a localized interface cache so the agent doesn't
// have to go to the broker over and over again to get an interface
//
// once an agent has received an interface pointer, it is stored
// in this cache object. The next time the interface is requested
// it is retreived from this local cache before going to the broker
//
// This object implements the IBroker interface in a non-COM way
// This object is designed to be a regular data member of an agent
// and serve as a wrapper class for the real IBroker interface
//
// Using the DECLARE_AGENT_DATA and IMPLEMENT_AGENT_DATA #define's given
// below, no code changes have to be made to an agent to implement
// this class. Simply use these macros
class PGSEXTCLASS pgsInterfaceCache : public IBroker
{
public:
   pgsInterfaceCache(void);
   ~pgsInterfaceCache(void);

   void SetBroker(IBroker* pBroker);

   STDMETHOD_(ULONG,AddRef)();
   STDMETHOD_(ULONG,Release)();
   STDMETHOD(QueryInterface)(REFIID riid,void** ppv);
   STDMETHOD(GetInterface)(REFIID riid,void** ppv);
   STDMETHOD(Reset)();
   STDMETHOD(ShutDown)();

private:
   IBroker* m_pBroker; // weak reference
   typedef std::map<IID, IUnknown*> Interfaces;
   Interfaces m_Interfaces;
};

#if defined NO_INTERFACE_CACHE
#define DECLARE_AGENT_DATA \
   IBroker* m_pBroker; \
   long m_AgentID;

#define AGENT_SET_BROKER(broker) m_pBroker = broker
#define AGENT_INIT     GET_IFACE(IStatusCenter,pStatusCenter); m_AgentID = pStatusCenter->GetAgentID()
#define AGENT_CLEAR_INTERFACE_CACHE  

#else

// declares the requirement data components for an agent
// In the private data area of the agent class declairation add
// DECLARE_AGENT_DATA
#define DECLARE_AGENT_DATA \
   IBroker* m_pBroker; \
   pgsInterfaceCache m_InterfaceCache; \
   long m_AgentID

#define AGENT_SET_BROKER(broker) m_InterfaceCache.SetBroker(broker); m_pBroker = &m_InterfaceCache
#define AGENT_INIT     GET_IFACE(IStatusCenter,pStatusCenter); m_AgentID = pStatusCenter->GetAgentID()
#define AGENT_CLEAR_INTERFACE_CACHE  m_InterfaceCache.Reset()


#endif // NO_INTERFACE_CACHE