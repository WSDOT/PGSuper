///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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
// DistributedLoadData.cpp: implementation of the CDistributedLoadData class.
//
//////////////////////////////////////////////////////////////////////

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\DistributedLoadData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDistributedLoadData::CDistributedLoadData():
m_LoadCase(UserLoads::DC),
m_Stage(UserLoads::BridgeSite1),
m_Span(0),
m_Girder(0),
m_Type(UserLoads::Trapezoidal),
m_WStart(0.0),
m_WEnd(0.0),
m_StartLocation(0.5),
m_EndLocation(0.5),
m_Fractional(true),
m_Description("")
{

}

CDistributedLoadData::~CDistributedLoadData()
{
}

bool CDistributedLoadData::operator == (const CDistributedLoadData& rOther) const
{
   if (m_Stage != rOther.m_Stage)
      return false;

   if (m_LoadCase != rOther.m_LoadCase)
      return false;

   if (m_Span != rOther.m_Span)
      return false;

   if (m_Girder != rOther.m_Girder)
      return false;

   if (m_Type != rOther.m_Type)
      return false;

   if (m_StartLocation != rOther.m_StartLocation)
      return false;

   if (m_EndLocation != rOther.m_EndLocation)
      return false;

   if (m_Fractional != rOther.m_Fractional)
      return false;

   if (m_WStart != rOther.m_WStart)
      return false;

   if (m_WEnd != rOther.m_WEnd)
      return false;

   if (m_Description != rOther.m_Description)
      return false;

   return true;
}

bool CDistributedLoadData::operator != (const CDistributedLoadData& rOther) const
{
   return !(*this == rOther);
}


HRESULT CDistributedLoadData::Save(IStructuredSave* pSave)
{
   HRESULT hr;

   pSave->BeginUnit("DistributedLoad",2.0);

   hr = pSave->put_Property("LoadCase",CComVariant((long)m_LoadCase));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("Stage",CComVariant((long)m_Stage));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("Type",CComVariant((long)m_Type));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("Span",CComVariant(m_Span));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("Girder",CComVariant(m_Girder));
   if ( FAILED(hr) )
      return hr;
   
   hr = pSave->put_Property("StartLocation",CComVariant(m_StartLocation));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("EndLocation",CComVariant(m_EndLocation));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("WStart",CComVariant(m_WStart));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("WEnd",CComVariant(m_WEnd));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("Fractional",CComVariant((long)m_Fractional));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property("Description",CComVariant(m_Description.c_str()));
   if ( FAILED(hr) )
      return hr;

   pSave->EndUnit();

   return hr;
}

HRESULT CDistributedLoadData::Load(IStructuredLoad* pLoad)
{
   USES_CONVERSION;

   HRESULT hr;

   hr = pLoad->BeginUnit("DistributedLoad");
   if ( FAILED(hr) )
      return hr;

   double version;
   pLoad->get_Version(&version);

   CComVariant var;
   var.vt = VT_I4;
   hr = pLoad->get_Property("LoadCase",&var);
   if ( FAILED(hr) )
      return hr;

   if (var.lVal==UserLoads::DC)
      m_LoadCase = UserLoads::DC;
   else if(var.lVal==UserLoads::DW)
      m_LoadCase = UserLoads::DW;
   else if(var.lVal==UserLoads::LL_IM)
      m_LoadCase = UserLoads::LL_IM;
   else
   {
      ATLASSERT(0);
      return STRLOAD_E_INVALIDFORMAT;
   }

   hr = pLoad->get_Property("Stage",&var);
   if ( FAILED(hr) )
      return hr;

   if (var.lVal==UserLoads::BridgeSite1)
      m_Stage = UserLoads::BridgeSite1;
   else if (var.lVal==UserLoads::BridgeSite2)
      m_Stage = UserLoads::BridgeSite2;
   else if (var.lVal==UserLoads::BridgeSite3)
      m_Stage = UserLoads::BridgeSite3;
   else
   {
      ATLASSERT(0);
      return STRLOAD_E_INVALIDFORMAT;
   }

   hr = pLoad->get_Property("Type",&var);
   if ( FAILED(hr) )
      return hr;

   if (var.lVal==UserLoads::Uniform)
      m_Type = UserLoads::Uniform;
   else if (var.lVal==UserLoads::Trapezoidal)
      m_Type = UserLoads::Trapezoidal;
   else
   {
      ATLASSERT(0);
      return STRLOAD_E_INVALIDFORMAT;
   }

   var.vt = VT_I4;
   hr = pLoad->get_Property("Span",&var);
   if ( FAILED(hr) )
      return hr;

   m_Span = var.iVal;

   var.vt = VT_I4;
   hr = pLoad->get_Property("Girder",&var);
   if ( FAILED(hr) )
      return hr;

   m_Girder = var.iVal;
   
   var.vt = VT_R8;
   hr = pLoad->get_Property("StartLocation",&var);
   if ( FAILED(hr) )
      return hr;

   m_StartLocation = var.dblVal;

   hr = pLoad->get_Property("EndLocation",&var);
   if ( FAILED(hr) )
      return hr;

   m_EndLocation = var.dblVal;

   hr = pLoad->get_Property("WStart",&var);
   if ( FAILED(hr) )
      return hr;

   m_WStart = var.dblVal;

   hr = pLoad->get_Property("WEnd",&var);
   if ( FAILED(hr) )
      return hr;

   m_WEnd = var.dblVal;

   var.vt = VT_I4;
   hr = pLoad->get_Property("Fractional",&var);
   if ( FAILED(hr) )
      return hr;

   m_Fractional = var.lVal != 0;

   if ( 1 < version )
   {
      var.vt = VT_BSTR;
      hr = pLoad->get_Property("Description",&var);
      if ( FAILED(hr) )
         return hr;

      m_Description = OLE2A(var.bstrVal);
   }

   hr = pLoad->EndUnit();
   return hr;
}

