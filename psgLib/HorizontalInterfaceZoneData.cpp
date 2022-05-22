///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "StdAfx.h"
#include <PsgLib\HorizontalInterfaceZoneData.h>
#include <Units\Convert.h>
#include <Lrfd\RebarPool.h>
#include <StdIo.h>
#include <StrData.cpp>
#include <comdef.h> // for _variant_t

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHorizontalInterfaceZoneData
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHorizontalInterfaceZoneData::CHorizontalInterfaceZoneData():
ZoneNum(0),
BarSpacing(0),
ZoneLength(0),
BarSize(matRebar::bsNone),
nBars()
{
}

CHorizontalInterfaceZoneData::CHorizontalInterfaceZoneData(const CHorizontalInterfaceZoneData& rOther)
{
   MakeCopy(rOther);
}

CHorizontalInterfaceZoneData::~CHorizontalInterfaceZoneData()
{
}

//======================== OPERATORS  =======================================
CHorizontalInterfaceZoneData& CHorizontalInterfaceZoneData::operator= (const CHorizontalInterfaceZoneData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CHorizontalInterfaceZoneData::operator == (const CHorizontalInterfaceZoneData& rOther) const
{
   if ( ZoneNum != rOther.ZoneNum )
      return false;

   if ( !IsEqual(BarSpacing, rOther.BarSpacing) )
      return false;

   if ( !IsEqual(ZoneLength, rOther.ZoneLength ))
      return false;

   if ( BarSize != rOther.BarSize )
      return false;

   if ( nBars != rOther.nBars )
      return false;

   return true;
}

bool CHorizontalInterfaceZoneData::operator != (const CHorizontalInterfaceZoneData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CHorizontalInterfaceZoneData::Load(WBFL::System::IStructuredLoad* pStrLoad)
{
   HRESULT hr = S_OK;

   if ( SUCCEEDED(pStrLoad->BeginUnit(_T("HorizontalInterfaceZoneData"))) )
   {
      Float64 version = pStrLoad->GetVersion();
      if ( 1.0 < version )
         return STRLOAD_E_BADVERSION;

      if ( FAILED(pStrLoad->Property(_T("ZoneNum"),&ZoneNum)) )
         return STRLOAD_E_INVALIDFORMAT;

      if ( FAILED(pStrLoad->Property(_T("ZoneLength"),&ZoneLength)) )
         return STRLOAD_E_INVALIDFORMAT;

      if ( FAILED(pStrLoad->Property(_T("BarSpacing"),&BarSpacing)) )
         return STRLOAD_E_INVALIDFORMAT;

      BarSizeType key;
      if ( FAILED(pStrLoad->Property(_T("BarSize"),&key)) )
         return STRLOAD_E_INVALIDFORMAT;

      BarSize = matRebar::Size(key);

      if ( FAILED(pStrLoad->Property(_T("nBars"),&nBars)) )
         return STRLOAD_E_INVALIDFORMAT;

      if ( FAILED(pStrLoad->EndUnit()) )
         return STRLOAD_E_INVALIDFORMAT;
   }
   else
   {
      return E_FAIL;
   }

   return hr;
}

HRESULT CHorizontalInterfaceZoneData::Save(WBFL::System::IStructuredSave* pStrSave)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("HorizontalInterfaceZoneData"),1.0);
   pStrSave->Property(_T("ZoneNum"),    ZoneNum );
   pStrSave->Property(_T("ZoneLength"), ZoneLength );
   pStrSave->Property(_T("BarSpacing"), BarSpacing );
   pStrSave->Property(_T("BarSize"),(long)BarSize );
   pStrSave->Property(_T("nBars"),   nBars );
   pStrSave->EndUnit();

   return hr;
}
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CHorizontalInterfaceZoneData::MakeCopy(const CHorizontalInterfaceZoneData& rOther)
{
   ZoneNum       = rOther.ZoneNum;
   BarSpacing    = rOther.BarSpacing;
   ZoneLength    = rOther.ZoneLength;
   BarSize       = rOther.BarSize;
   nBars         = rOther.nBars;
}

void CHorizontalInterfaceZoneData::MakeAssignment(const CHorizontalInterfaceZoneData& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
