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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\InterfaceCache.h>

bool operator<(REFIID a,REFIID b);
bool operator<(REFIID a,REFIID b)
{
   /*
   typedef struct _GUID {
      unsigned long  Data1;
      unsigned short Data2;
      unsigned short Data3;
      unsigned char  Data4[8];} GUID;
    */

    if ( a.Data1 > b.Data1 )
       return false;
    if ( a.Data1 < b.Data1 )
       return true;

    if ( a.Data2 > b.Data2 )
       return false;
    if ( a.Data2 < b.Data2 )
       return true;

    if ( a.Data3 > b.Data3 )
       return false;
    if ( a.Data3 < b.Data3 )
       return true;

    for ( int i = 0; i < 8; i++ )
    {
       if ( a.Data4[i] > b.Data4[i] )
          return false;
       if ( a.Data4[i] < b.Data4[i] )
          return true;
    }

    return false;
}

pgsInterfaceCache::pgsInterfaceCache(void)
{
   m_pBroker = NULL;
}

pgsInterfaceCache::~pgsInterfaceCache(void)
{
}

void pgsInterfaceCache::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

STDMETHODIMP_(ULONG) pgsInterfaceCache::AddRef()
{
   // do nothing
   return 1;
}

STDMETHODIMP_(ULONG) pgsInterfaceCache::Release()
{
   // do nothing
   return 1;
}

STDMETHODIMP pgsInterfaceCache::QueryInterface(REFIID riid,void** ppv)
{
   return m_pBroker->QueryInterface(riid,ppv);
}

STDMETHODIMP pgsInterfaceCache::GetInterface(REFIID riid,void** ppv)
{
   Interfaces::iterator found = m_Interfaces.find(riid);
   if ( found != m_Interfaces.end() )
   {
      IUnknown* punk = (*found).second;
      (*ppv) = (void*)punk;

      return S_OK;
   }

   // interface not found... go to the broker
   HRESULT hr = m_pBroker->GetInterface(riid,ppv); // calls AddRef(), released in Reset
   if ( FAILED(hr) )
      return hr; // interface not found

   // interface found, cache it
   m_Interfaces.insert( std::make_pair(riid,(IUnknown*)(*ppv)) );

   return S_OK;
}

STDMETHODIMP pgsInterfaceCache::Reset()
{
   Interfaces::iterator iter;
   for ( iter = m_Interfaces.begin(); iter != m_Interfaces.end(); iter++ )
   {
      (*iter).second->Release();
   }
   m_Interfaces.clear();

   if ( m_pBroker )
      m_pBroker->Reset();

   return S_OK;
}

STDMETHODIMP pgsInterfaceCache::ShutDown()
{
   Reset();
   if ( m_pBroker )
      m_pBroker->ShutDown();

   return S_OK;
}
