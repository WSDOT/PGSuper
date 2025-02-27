///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
// PointLoadData.cpp: implementation of the CPointLoadData class.
//
//////////////////////////////////////////////////////////////////////

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PointLoadData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IDType UserLoads::ms_NextDistributedLoadID = 0;
IDType UserLoads::ms_NextPointLoadID = 10000;
IDType UserLoads::ms_NextMomentLoadID = 20000;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPointLoadData::CPointLoadData():
m_ID(INVALID_ID),
m_LoadCase(UserLoads::DC),
m_StageIndex(INVALID_INDEX),
m_SpanKey(0,0),
m_Magnitude(0.0),
m_Location(0.5),
m_Fractional(true),
m_Description(_T(""))
{
   m_bLoadOnCantilever[pgsTypes::metStart] = false;
   m_bLoadOnCantilever[pgsTypes::metEnd]   = false;
}

CPointLoadData::CPointLoadData(const CPointLoadData& other)
{
   MakeCopy(other);
}

CPointLoadData::~CPointLoadData()
{

}

CPointLoadData& CPointLoadData::operator=(const CPointLoadData& other)
{
   MakeAssignment(other);
   return *this;
}

bool CPointLoadData::operator == (const CPointLoadData& rOther) const
{
   if (m_StageIndex != rOther.m_StageIndex)
   {
      return false;
   }

   if (m_LoadCase != rOther.m_LoadCase)
   {
      return false;
   }

   if (!m_SpanKey.IsEqual(rOther.m_SpanKey))
   {
      return false;
   }

   if ( m_bLoadOnCantilever[pgsTypes::metStart] != rOther.m_bLoadOnCantilever[pgsTypes::metStart] )
   {
      return false;
   }

   if ( m_bLoadOnCantilever[pgsTypes::metEnd] != rOther.m_bLoadOnCantilever[pgsTypes::metEnd] )
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

bool CPointLoadData::operator != (const CPointLoadData& rOther) const
{
   return !(*this == rOther);
}


HRESULT CPointLoadData::Save(IStructuredSave* pSave)
{
   HRESULT hr;

   pSave->BeginUnit(_T("PointLoad"),8.0);

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

   // stopped storing this with version 8
   // we don't need to store the event ID... the timeline manager stores our load ID with an event
   //hr = pSave->put_Property(_T("EventID"),CComVariant((long)m_EventID)); // storing ID starting with version 7
   //if ( FAILED(hr) )
   //{
   //   return hr;
   //}

   // In pre Jan, 2011 versions, "all spans" and "all girders" were hardcoded to 10000, then we changed to the ALL_SPANS/ALL_GIRDERS value
   // Keep backward compatibility by saving the 10k value
   SpanIndexType spanIdx  = (m_SpanKey.spanIndex   == ALL_SPANS   ? 10000 : m_SpanKey.spanIndex);
   GirderIndexType gdrIdx = (m_SpanKey.girderIndex == ALL_GIRDERS ? 10000 : m_SpanKey.girderIndex);

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

   // added in version 6
   hr = pSave->put_Property(_T("LoadOnStartCantilever"),CComVariant(m_bLoadOnCantilever[pgsTypes::metStart]));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pSave->put_Property(_T("LoadOnEndCantilever"),CComVariant(m_bLoadOnCantilever[pgsTypes::metEnd]));
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

HRESULT CPointLoadData::Load(IStructuredLoad* pLoad)
{
   USES_CONVERSION;
   HRESULT hr;

   hr = pLoad->BeginUnit(_T("PointLoad"));
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
      UserLoads::ms_NextPointLoadID = Max(UserLoads::ms_NextPointLoadID,m_ID);
   }
   else
   {
      m_ID = UserLoads::ms_NextPointLoadID++;
   }

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("LoadCase"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   if (var.lVal == UserLoads::DC)
   {
      m_LoadCase = UserLoads::DC;
   }
   else if(var.lVal == UserLoads::DW)
   {
      m_LoadCase = UserLoads::DW;
   }
   else if(var.lVal == UserLoads::LL_IM)
   {
      m_LoadCase = UserLoads::LL_IM;
   }
   else
   {
      ATLASSERT(false);
      return STRLOAD_E_INVALIDFORMAT;
   }

   if ( version < 4 )
   {
      var.vt = VT_INDEX;
      hr = pLoad->get_Property(_T("Stage"),&var);
      m_StageIndex = VARIANT2INDEX(var);
   }
   else if ( version < 7 )
   {
      // we didn't need to store the event index... just load it and forget it
      var.vt = VT_INDEX;
      hr = pLoad->get_Property(_T("EventIndex"),&var);
      m_StageIndex = VARIANT2INDEX(var);
   }
   else if ( version < 8 )
   {
      // starting with version 8, we don't store the event ID
      // prior to version 8, we stored it, but don't need it
      // just load the event id and forget about it
      var.vt = VT_ID;
      hr = pLoad->get_Property(_T("EventID"),&var);
      //m_EventID = VARIANT2ID(var);
   }

   if ( FAILED(hr) )
   {
      return hr;
   }

   // prior to version 3, stages were 0=BridgeSite1, 1=BridgeSite2, 2=BridgeSite3
   // Version 3 and later, stages are pgsTypes::BridgeSite1, pgsTypes::BridgeSite2, pgsTypes::BridgeSite3
   // adjust the stage value here
   if ( version < 3 )
   {
      switch(m_StageIndex)
      {
         // when the generalized stage model was created (PGSplice) the BridgeSiteX constants where removed
         // use the equivalent value
      case 0: m_StageIndex = 2;/*pgsTypes::BridgeSite1;*/ break;
      case 1: m_StageIndex = 3;/*pgsTypes::BridgeSite2;*/ break;
      case 2: m_StageIndex = 4;/*pgsTypes::BridgeSite3;*/ break;
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

   // see note in Save method about span/girder == 10000
   m_SpanKey.spanIndex = VARIANT2INDEX(var);
   if ( 10000 == m_SpanKey.spanIndex)
   {
      m_SpanKey.spanIndex = ALL_SPANS;
   }

   var.vt = VT_INDEX;
   hr = pLoad->get_Property(_T("Girder"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   m_SpanKey.girderIndex = VARIANT2INDEX(var);
   if ( 10000 == m_SpanKey.girderIndex )
   {
      m_SpanKey.girderIndex = ALL_GIRDERS;
   }

   if ( 6 <= version )
   {
      var.vt = VT_I4;
      hr = pLoad->get_Property(_T("LoadOnStartCantilever"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      m_bLoadOnCantilever[pgsTypes::metStart] = var.lVal != 0;

      var.vt = VT_I4;
      hr = pLoad->get_Property(_T("LoadOnEndCantilever"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      m_bLoadOnCantilever[pgsTypes::metEnd] = var.lVal != 0;
   }
   
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

   if ( 1 < version )
   {
      var.vt = VT_BSTR;
      hr = pLoad->get_Property(_T("Description"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      m_Description = OLE2T(var.bstrVal);
   }

   hr = pLoad->EndUnit();
   return hr;
}

void CPointLoadData::MakeCopy(const CPointLoadData& rOther)
{
   m_ID                                    = rOther.m_ID;
   m_LoadCase                              = rOther.m_LoadCase;
   m_SpanKey                               = rOther.m_SpanKey;
   m_bLoadOnCantilever[pgsTypes::metStart] = rOther.m_bLoadOnCantilever[pgsTypes::metStart];
   m_bLoadOnCantilever[pgsTypes::metEnd]   = rOther.m_bLoadOnCantilever[pgsTypes::metEnd];
   m_Location                              = rOther.m_Location;
   m_Fractional                            = rOther.m_Fractional;
   m_Magnitude                             = rOther.m_Magnitude;
   m_Description                           = rOther.m_Description;
   m_StageIndex                            = rOther.m_StageIndex;
}

void CPointLoadData::MakeAssignment(const CPointLoadData& rOther)
{
   MakeCopy(rOther);
}
