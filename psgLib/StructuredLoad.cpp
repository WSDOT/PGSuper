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
#include <psgLib\psglib.h>
#include <psgLib\StructuredLoad.h>

#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStructuredLoad
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStructuredLoad::CStructuredLoad(IStructuredLoad* pStrLoad)
{
   m_pStrLoad = pStrLoad;
   m_pStrLoad->AddRef();
}

CStructuredLoad::~CStructuredLoad()
{
   if ( m_pStrLoad )
   {
      m_pStrLoad->Release();
      m_pStrLoad = 0;
   }
}

//======================== OPERATORS  =======================================
bool CStructuredLoad::BeginUnit(const char* name)
{
   HRESULT hr = m_pStrLoad->BeginUnit( name );
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::EndUnit()
{
   HRESULT hr = m_pStrLoad->EndUnit();
   return SUCCEEDED(hr) ? true : false;
}

double CStructuredLoad::GetVersion()
{
   double version;
   HRESULT hr = m_pStrLoad->get_Version( &version );
   return version;
}

double CStructuredLoad::GetParentVersion()
{
   double version;
   HRESULT hr = m_pStrLoad->get_ParentVersion( &version );
   return version;
}

double CStructuredLoad::GetTopVersion()
{
   double version;
   HRESULT hr = m_pStrLoad->get_TopVersion( &version );
   return version;
}

bool CStructuredLoad::Property(const char* name, std::string* pvalue)
{
   USES_CONVERSION;
   VARIANT var;
   var.vt = VT_BSTR;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = OLE2A( var.bstrVal );
   ::SysFreeString( var.bstrVal );
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Property(const char* name, Float64* pvalue)
{
   VARIANT var;
   var.vt = VT_R8;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = var.dblVal;
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Property(const char* name, Int16* pvalue)
{
   VARIANT var;
   var.vt = VT_I2;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = var.iVal;
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Property(const char* name, Uint16* pvalue)
{
   VARIANT var;
   var.vt = VT_UI2;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = var.uiVal;
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Property(const char* name, Int32* pvalue)
{
   VARIANT var;
   var.vt = VT_I4;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = var.lVal;
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Property(const char* name, Uint32* pvalue)
{
   VARIANT var;
   var.vt = VT_UI4;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = var.ulVal;
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Property(const char* name, bool* pvalue)
{
   VARIANT var;
   var.vt = VT_BOOL;
   HRESULT hr = m_pStrLoad->get_Property( name, &var );
   *pvalue = var.boolVal == VARIANT_TRUE ? true : false;
   return SUCCEEDED(hr) ? true : false;
}

bool CStructuredLoad::Eof()const
{
   HRESULT hr = m_pStrLoad->EndOfStorage();
   return ( hr == S_OK ? true : false );
}

std::string CStructuredLoad::GetStateDump() const
{
   std::string str("State data not available");
   return str;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CStructuredLoad::AssertValid() const
{
   if ( m_pStrLoad == 0 )
      return false;

   return true;
}

void CStructuredLoad::Dump(dbgDumpContext& os) const
{
   os << "Dump for CStructuredLoad" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStructuredLoad::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStructuredLoad");

   // Nothing to test

   TESTME_EPILOG("CStructuredLoad");
}
#endif // _UNITTEST
