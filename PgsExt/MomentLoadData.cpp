///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
m_EventIndex(INVALID_INDEX),
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
   if (m_EventIndex != rOther.m_EventIndex)
   {
      return false;
   }

   if (m_LoadCase != rOther.m_LoadCase)
   {
      return false;
   }

   if ( !m_spanKey.IsEqual(rOther.m_spanKey) )
   {
      return false;
   }

   if (m_Location != rOther.m_Location)
   {
      return false;
   }

   if (m_Fractional != rOther.m_Fractional)
   {
      return false;
   }

   if (m_Magnitude != rOther.m_Magnitude)
   {
      return false;
   }

   if (m_Description != rOther.m_Description)
   {
      return false;
   }

   return true;
}

bool CMomentLoadData::operator != (const CMomentLoadData& rOther) const
{
   return !(*this == rOther);
}


HRESULT CMomentLoadData::Save(IStructuredSave* pSave)
{
   HRESULT hr;

   pSave->BeginUnit(_T("MomentLoad"),5.0); // changed for version 4 with PGSplice

   hr = pSave->put_Property(_T("ID"),CComVariant(m_ID));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("LoadCase"),CComVariant((long)m_LoadCase));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("EventIndex"),CComVariant((long)m_EventIndex));
   if ( FAILED(hr) )
   {
      return hr;
   }

   SpanIndexType spanIdx = m_spanKey.spanIndex;
   GirderIndexType gdrIdx = m_spanKey.girderIndex;

#pragma Reminder("REVIEW: consider updating the default value and converting the old value on input")
   // In pre Jan, 2011 versions, all spans and all girders were hardcoded to 10000, then we changed to the ALL_SPANS/ALL_GIRDERS value
   // Keep backward compatibility by saving the 10k value
   spanIdx = (spanIdx == ALL_SPANS   ? 10000 : spanIdx);
   gdrIdx  = (gdrIdx  == ALL_GIRDERS ? 10000 : gdrIdx);

   hr = pSave->put_Property(_T("Span"),CComVariant(spanIdx));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("Girder"),CComVariant(gdrIdx));
   if ( FAILED(hr) )
   {
      return hr;
   }
   
   hr = pSave->put_Property(_T("Location"),CComVariant(m_Location));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("Magnitude"),CComVariant(m_Magnitude));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("Fractional"),CComVariant((long)m_Fractional));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("Description"),CComVariant(m_Description.c_str()));
   if ( FAILED(hr) )
   {
      return hr;
   }


   pSave->EndUnit();

   return hr;
}

HRESULT CMomentLoadData::Load(IStructuredLoad* pLoad)
{
   USES_CONVERSION;
   HRESULT hr;

   hr = pLoad->BeginUnit(_T("MomentLoad"));
   if ( FAILED(hr) )
   {
      return hr;
   }

   Float64 version;
   pLoad->get_Version(&version);


   CComVariant var;
   var.vt = VT_ID;
   if ( 5.0 <= version )
   {
      hr = pLoad->get_Property(_T("ID"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      m_ID = VARIANT2ID(var);
      UserLoads::ms_NextMomentLoadID = Max(UserLoads::ms_NextMomentLoadID,m_ID);
   }
   else
   {
      m_ID = UserLoads::ms_NextMomentLoadID++;
   }

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("LoadCase"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   if (var.lVal==UserLoads::DC)
   {
      m_LoadCase = UserLoads::DC;
   }
   else if(var.lVal==UserLoads::DW)
   {
      m_LoadCase = UserLoads::DW;
   }
   else if(var.lVal==UserLoads::LL_IM)
   {
      m_LoadCase = UserLoads::LL_IM;
   }
   else
   {
      ATLASSERT(false);
      return STRLOAD_E_INVALIDFORMAT;
   }

   var.vt = VT_INDEX;
   if ( version < 4 )
   {
      hr = pLoad->get_Property(_T("Stage"),&var);
   }
   else
   {
      hr = pLoad->get_Property(_T("EventIndex"),&var);
   }

   if ( FAILED(hr) )
   {
      return hr;
   }

   m_EventIndex = VARIANT2INDEX(var);
   // prior to version 3, stages were 0=BridgeSite1, 1=BridgeSite2, 2=BridgeSite3
   // Version 3 and later, stages are pgsTypes::BridgeSite1, pgsTypes::BridgeSite2, pgsTypes::BridgeSite3
   // adjust the stage value here
   if ( version < 3 )
   {
      switch(m_EventIndex)
      {
         // when the generalized stage model was created (PGSplice) the BridgeSiteX constants where removed
         // use the equivalent value
      case 0: m_EventIndex = 2;/*pgsTypes::BridgeSite1;*/ break;
      case 1: m_EventIndex = 3;/*pgsTypes::BridgeSite2;*/ break;
      case 2: m_EventIndex = 4;/*pgsTypes::BridgeSite3;*/ break;
      default:
         ATLASSERT(false);
      }
   }


   var.vt = VT_INDEX;
   hr = pLoad->get_Property(_T("Span"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;

   spanIdx = VARIANT2INDEX(var);
   if ( 10000 == spanIdx )
   {
      spanIdx = ALL_SPANS;
   }

   hr = pLoad->get_Property(_T("Girder"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   gdrIdx = VARIANT2INDEX(var);
   if ( 10000 == gdrIdx )
   {
      gdrIdx = ALL_GIRDERS;
   }

   m_spanKey.spanIndex   = spanIdx;
   m_spanKey.girderIndex  = gdrIdx;
   
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("Location"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   m_Location = var.dblVal;

   hr = pLoad->get_Property(_T("Magnitude"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   m_Magnitude = var.dblVal;

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Fractional"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   m_Fractional = var.lVal != 0;

  var.vt = VT_BSTR;
  hr = pLoad->get_Property(_T("Description"),&var);
  if ( FAILED(hr) )
  {
     return hr;
  }

  m_Description = OLE2T(var.bstrVal);

   hr = pLoad->EndUnit();
   return hr;
}
