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
// MomentLoadData.cpp: implementation of the CMomentLoadData class.
//
//////////////////////////////////////////////////////////////////////

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\MomentLoadData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMomentLoadData::CMomentLoadData():
m_LoadCase(UserLoads::DC),
m_Stage(UserLoads::BridgeSite1),
m_Span(0),
m_Girder(0),
m_Magnitude(0.0),
m_Location(0.5),
m_Fractional(true),
m_Description(_T(""))
{

}

CMomentLoadData::~CMomentLoadData()
{

}

bool CMomentLoadData::operator == (const CMomentLoadData& rOther) const
{
   if (m_Stage != rOther.m_Stage)
      return false;

   if (m_LoadCase != rOther.m_LoadCase)
      return false;

   if (m_Span != rOther.m_Span)
      return false;

   if (m_Girder != rOther.m_Girder)
      return false;

   if (m_Location != rOther.m_Location)
      return false;

   if (m_Fractional != rOther.m_Fractional)
      return false;

   if (m_Magnitude != rOther.m_Magnitude)
      return false;

   if (m_Description != rOther.m_Description)
      return false;

   return true;
}

bool CMomentLoadData::operator != (const CMomentLoadData& rOther) const
{
   return !(*this == rOther);
}


HRESULT CMomentLoadData::Save(IStructuredSave* pSave)
{
   HRESULT hr;

   pSave->BeginUnit(_T("MomentLoad"),1.0);

   hr = pSave->put_Property(_T("LoadCase"),CComVariant((long)m_LoadCase));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Stage"),CComVariant((long)m_Stage));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Span"),CComVariant(m_Span));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Girder"),CComVariant(m_Girder));
   if ( FAILED(hr) )
      return hr;
   
   hr = pSave->put_Property(_T("Location"),CComVariant(m_Location));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Magnitude"),CComVariant(m_Magnitude));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Fractional"),CComVariant((long)m_Fractional));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Description"),CComVariant(m_Description.c_str()));
   if ( FAILED(hr) )
      return hr;


   pSave->EndUnit();

   return hr;
}

HRESULT CMomentLoadData::Load(IStructuredLoad* pLoad)
{
   USES_CONVERSION;
   HRESULT hr;

   hr = pLoad->BeginUnit(_T("MomentLoad"));
   if ( FAILED(hr) )
      return hr;

   CComVariant var;
   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("LoadCase"),&var);
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

   hr = pLoad->get_Property(_T("Stage"),&var);
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


   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Span"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Span = var.iVal;

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Girder"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Girder = var.iVal;
   
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("Location"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Location = var.dblVal;

   hr = pLoad->get_Property(_T("Magnitude"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Magnitude = var.dblVal;

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Fractional"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Fractional = var.lVal != 0;

  var.vt = VT_BSTR;
  hr = pLoad->get_Property(_T("Description"),&var);
  if ( FAILED(hr) )
     return hr;

  m_Description = OLE2T(var.bstrVal);

   hr = pLoad->EndUnit();
   return hr;
}
