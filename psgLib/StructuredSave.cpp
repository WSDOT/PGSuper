///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <psgLib\StructuredSave.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStructuredSave
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStructuredSave::CStructuredSave(::IStructuredSave* pStrSave)
{
   m_pStrSave = pStrSave;
   m_pStrSave->AddRef();
}

CStructuredSave::~CStructuredSave()
{
   if ( m_pStrSave )
   {
      m_pStrSave->Release();
      m_pStrSave = 0;
   }
}

//======================== OPERATORS  =======================================
void CStructuredSave::BeginUnit(LPCTSTR name, Float64 version)
{
   m_pStrSave->BeginUnit( name, version );
}

void CStructuredSave::EndUnit()
{
   m_pStrSave->EndUnit();
}

Float64 CStructuredSave::GetVersion()
{
   Float64 version;
   m_pStrSave->get_Version( &version );
   return version;
}

Float64 CStructuredSave::GetParentVersion()
{
   Float64 version;
   m_pStrSave->get_ParentVersion( &version );
   return version;
}

std::_tstring CStructuredSave::GetParentUnit()
{
   USES_CONVERSION;
   CComBSTR bstr;
   HRESULT hr = m_pStrSave->get_ParentUnit(&bstr);
   return OLE2T(bstr);
}

Float64 CStructuredSave::GetTopVersion()
{
   Float64 version;
   m_pStrSave->get_TopVersion( &version );
   return version;
}

void CStructuredSave::Property(LPCTSTR name, LPCTSTR value)
{
   USES_CONVERSION;
   VARIANT var;
   var.vt = VT_BSTR;
   var.bstrVal = T2BSTR( value );
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Float64 value)
{
   VARIANT var;
   var.vt = VT_R8;
   var.dblVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Int16 value)
{
   VARIANT var;
   var.vt = VT_I2;
   var.lVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Uint16 value)
{
   VARIANT var;
   var.vt = VT_UI2;
   var.ulVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Int32 value)
{
   VARIANT var;
   var.vt = VT_I4;
   var.lVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Uint32 value)
{
   VARIANT var;
   var.vt = VT_UI4;
   var.ulVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Int64 value)
{
   VARIANT var;
   var.vt = VT_I8;
   var.llVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, Uint64 value)
{
   VARIANT var;
   var.vt = VT_UI8;
   var.ullVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, LONG value)
{
   VARIANT var;
   var.vt = VT_I4;
   var.lVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, ULONG value)
{
   VARIANT var;
   var.vt = VT_UI4;
   var.ulVal = value;
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::Property(LPCTSTR name, bool value)
{
   VARIANT var;
   var.vt = VT_BOOL;
   var.boolVal = (value == true ? VARIANT_TRUE : VARIANT_FALSE );
   m_pStrSave->put_Property( name, var );
}

void CStructuredSave::PutUnit(LPCTSTR xml)
{
   m_pStrSave->SaveRawUnit(xml);
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
