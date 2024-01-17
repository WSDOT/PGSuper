///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
m_ID(INVALID_ID),
m_LoadCase(UserLoads::DC),
m_StageIndex(INVALID_INDEX),
m_Type(UserLoads::Trapezoidal),
m_WStart(0.0),
m_WEnd(0.0),
m_StartLocation(0.25),
m_EndLocation(0.75),
m_Fractional(true),
m_Description(_T(""))
{

}

CDistributedLoadData::CDistributedLoadData(const CDistributedLoadData& other)
{
   MakeCopy(other);
}

CDistributedLoadData::~CDistributedLoadData()
{
}

CDistributedLoadData& CDistributedLoadData::operator=(const CDistributedLoadData& other)
{
   MakeAssignment(other);
   return *this;
}

bool CDistributedLoadData::operator == (const CDistributedLoadData& rOther) const
{
   if (m_StageIndex != rOther.m_StageIndex)
      return false;

   if (m_LoadCase != rOther.m_LoadCase)
      return false;

   if ( !m_SpanKey.IsEqual(rOther.m_SpanKey) )
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

   pSave->BeginUnit(_T("DistributedLoad"),7.0); // changed for version 4 with PGSplice

   hr = pSave->put_Property(_T("ID"),CComVariant(m_ID));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("LoadCase"),CComVariant((long)m_LoadCase));
   if ( FAILED(hr) )
      return hr;

   // stopped storing this in version 7... we don't need it
   //hr = pSave->put_Property(_T("EventID"),CComVariant((long)m_EventID)); // changed to event ID in version 6
   //if ( FAILED(hr) )
   //   return hr;

   hr = pSave->put_Property(_T("Type"),CComVariant((long)m_Type));
   if ( FAILED(hr) )
      return hr;

   SpanIndexType spanIdx  = m_SpanKey.spanIndex;
   GirderIndexType gdrIdx = m_SpanKey.girderIndex;

   // In pre Jan, 2011 versions, all spans and all girders were hardcoded to 10000, then we changed to the ALL_SPANS/ALL_GIRDERS value
   // Keep backward compatibility by saving the 10k value
   spanIdx = (spanIdx == ALL_SPANS   ? 10000 : spanIdx);
   gdrIdx  = (gdrIdx  == ALL_GIRDERS ? 10000 : gdrIdx);

   hr = pSave->put_Property(_T("Span"),CComVariant(spanIdx));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("Girder"),CComVariant(gdrIdx));
   if ( FAILED(hr) )
      return hr;
   
   hr = pSave->put_Property(_T("StartLocation"),CComVariant(m_StartLocation));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("EndLocation"),CComVariant(m_EndLocation));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("WStart"),CComVariant(m_WStart));
   if ( FAILED(hr) )
      return hr;

   hr = pSave->put_Property(_T("WEnd"),CComVariant(m_WEnd));
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

HRESULT CDistributedLoadData::Load(IStructuredLoad* pLoad)
{
   USES_CONVERSION;

   HRESULT hr;

   hr = pLoad->BeginUnit(_T("DistributedLoad"));
   if ( FAILED(hr) )
      return hr;

   Float64 version;
   pLoad->get_Version(&version);

   CComVariant var;
   var.vt = VT_ID;
   if ( 5.0 <= version )
   {
      hr = pLoad->get_Property(_T("ID"),&var);
      if ( FAILED(hr) )
         return hr;

      m_ID = VARIANT2ID(var);
      UserLoads::ms_NextDistributedLoadID = Max(UserLoads::ms_NextDistributedLoadID,m_ID);
   }
   else
   {
      m_ID = UserLoads::ms_NextDistributedLoadID++;
   }

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("LoadCase"),&var);
   if ( FAILED(hr) )
      return hr;

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

   if ( version < 4 )
   {
      var.vt = VT_INDEX;
      hr = pLoad->get_Property(_T("Stage"),&var);
      m_StageIndex = VARIANT2INDEX(var);
   }
   else if ( version < 6 )
   {
      // we don't need this parameter, just forget about it
      var.vt = VT_INDEX;
      hr = pLoad->get_Property(_T("EventIndex"),&var);
      m_StageIndex = VARIANT2INDEX(var);
   }
   else if ( version < 7 )
   {
      // stopped storing this in version 7
      // we don't need this parameter, just forget about it
      var.vt = VT_ID;
      hr = pLoad->get_Property(_T("EventID"),&var);
      //m_EventID = VARIANT2INDEX(var);
   }

   if ( FAILED(hr) )
      return hr;

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

   hr = pLoad->get_Property(_T("Type"),&var);
   if ( FAILED(hr) )
      return hr;

   if (var.lVal==UserLoads::Uniform)
   {
      m_Type = UserLoads::Uniform;
   }
   else if (var.lVal==UserLoads::Trapezoidal)
   {
      m_Type = UserLoads::Trapezoidal;
   }
   else
   {
      ATLASSERT(false);
      return STRLOAD_E_INVALIDFORMAT;
   }

   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;

   var.vt = VT_INDEX;
   hr = pLoad->get_Property(_T("Span"),&var);
   if ( FAILED(hr) )
      return hr;

   spanIdx = VARIANT2INDEX(var);
   if ( 10000 == spanIdx )
      spanIdx = ALL_SPANS;

   var.vt = VT_INDEX;
   hr = pLoad->get_Property(_T("Girder"),&var);
   if ( FAILED(hr) )
      return hr;

   gdrIdx = VARIANT2INDEX(var);
   if ( 10000 == gdrIdx )
      gdrIdx = ALL_GIRDERS;

   m_SpanKey.spanIndex   = spanIdx;
   m_SpanKey.girderIndex = gdrIdx;
   
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("StartLocation"),&var);
   if ( FAILED(hr) )
      return hr;

   m_StartLocation = var.dblVal;

   hr = pLoad->get_Property(_T("EndLocation"),&var);
   if ( FAILED(hr) )
      return hr;

   m_EndLocation = var.dblVal;

   hr = pLoad->get_Property(_T("WStart"),&var);
   if ( FAILED(hr) )
      return hr;

   m_WStart = var.dblVal;

   hr = pLoad->get_Property(_T("WEnd"),&var);
   if ( FAILED(hr) )
      return hr;

   m_WEnd = var.dblVal;

   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Fractional"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Fractional = var.lVal != 0;

   if ( 1 < version )
   {
      var.vt = VT_BSTR;
      hr = pLoad->get_Property(_T("Description"),&var);
      if ( FAILED(hr) )
         return hr;

      m_Description = OLE2T(var.bstrVal);
   }

   hr = pLoad->EndUnit();
   return hr;
}

void CDistributedLoadData::MakeCopy(const CDistributedLoadData& rOther)
{
   m_ID            = rOther.m_ID;
   m_StageIndex    = rOther.m_StageIndex;
   m_LoadCase      = rOther.m_LoadCase;
   m_Type          = rOther.m_Type;
   m_SpanKey       = rOther.m_SpanKey;
   m_StartLocation = rOther.m_StartLocation;
   m_EndLocation   = rOther.m_EndLocation;
   m_WStart        = rOther.m_WStart;
   m_WEnd          = rOther.m_WEnd;
   m_Fractional    = rOther.m_Fractional;
   m_Description   = rOther.m_Description;
}

void CDistributedLoadData::MakeAssignment(const CDistributedLoadData& rOther)
{
   MakeCopy(rOther);
}
