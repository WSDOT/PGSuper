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
#include "StdAfx.h"
#include <PsgLib\ShearZoneData.h>
#include <Units\SysUnits.h>
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
   CShearZoneData
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CShearZoneData::CShearZoneData():
ZoneNum(0),
BarSpacing(0),
ZoneLength(0),
VertBarSize(matRebar::bsNone),
nVertBars(2.0),
nHorzInterfaceBars(2.0),
ConfinementBarSize(matRebar::bsNone),
legacy_HorzBarSize(matRebar::bsNone),
legacy_nHorzBars(2)
{

}

CShearZoneData::CShearZoneData(const CShearZoneData& rOther)
{
   MakeCopy(rOther);
}

CShearZoneData::~CShearZoneData()
{
}

//======================== OPERATORS  =======================================
CShearZoneData& CShearZoneData::operator= (const CShearZoneData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CShearZoneData::operator == (const CShearZoneData& rOther) const
{
   if ( ZoneNum != rOther.ZoneNum )
      return false;

   if ( !IsEqual(BarSpacing, rOther.BarSpacing) )
      return false;

   if ( !IsEqual(ZoneLength, rOther.ZoneLength ))
      return false;

   if ( VertBarSize != rOther.VertBarSize )
      return false;

   if ( nVertBars != rOther.nVertBars )
      return false;

   if ( nHorzInterfaceBars != rOther.nHorzInterfaceBars )
      return false;

   if ( ConfinementBarSize != rOther.ConfinementBarSize )
      return false;

   if ( legacy_HorzBarSize != rOther.legacy_HorzBarSize )
      return false;

   if ( legacy_nHorzBars != rOther.legacy_nHorzBars )
      return false;

   return true;
}

bool CShearZoneData::operator != (const CShearZoneData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CShearZoneData::Load(sysIStructuredLoad* pStrLoad, bool bConvertToShearDataVersion9, 
                matRebar::Size confinementBarSize,Uint32 NumConfinementZones, 
                bool bDoStirrupsEngageDeck)
{
   HRESULT hr = S_OK;

   if ( SUCCEEDED(pStrLoad->BeginUnit(_T("ShearZoneData"))) )
   {
      Float64 version = pStrLoad->GetVersion();
      if ( 4.0 < version )
         return STRLOAD_E_BADVERSION;

      if ( version < 2 )
      {
         if ( FAILED(pStrLoad->Property(_T("ZoneNum"),&ZoneNum)) )
            return STRLOAD_E_INVALIDFORMAT;

         BarSizeType key;
         if ( FAILED(pStrLoad->Property(_T("BarSize"),&key)) )
            return STRLOAD_E_INVALIDFORMAT;

         matRebar::Grade grade;
         matRebar::Type type;
         lrfdRebarPool::MapOldRebarKey(key,grade,type,VertBarSize);

         if ( FAILED(pStrLoad->Property(_T("BarSpacing"),&BarSpacing)) )
            return STRLOAD_E_INVALIDFORMAT;

         if ( FAILED(pStrLoad->Property(_T("ZoneLength"),&ZoneLength)) )
            return STRLOAD_E_INVALIDFORMAT;

         if ( 1.1 <= version )
         {
            Uint32 val;
            if ( FAILED(pStrLoad->Property(_T("NbrLegs"),&val)) )
               return STRLOAD_E_INVALIDFORMAT;

            nVertBars = (Float64)val;
         }
         else
         {
            nVertBars = 2.0;
         }

         legacy_HorzBarSize = matRebar::bsNone;
         nHorzInterfaceBars   = 2.0;
      }
      else if (version < 5)
      {
         if ( FAILED(pStrLoad->Property(_T("ZoneNum"),&ZoneNum)) )
            return STRLOAD_E_INVALIDFORMAT;

         if ( FAILED(pStrLoad->Property(_T("ZoneLength"),&ZoneLength)) )
            return STRLOAD_E_INVALIDFORMAT;

         if ( FAILED(pStrLoad->Property(_T("BarSpacing"),&BarSpacing)) )
            return STRLOAD_E_INVALIDFORMAT;

         if ( version < 3 )
         {
            BarSizeType key;
            if ( FAILED(pStrLoad->Property(_T("VertBarSize"),&key)) )
               return STRLOAD_E_INVALIDFORMAT;
         
            matRebar::Grade grade;
            matRebar::Type type;
            lrfdRebarPool::MapOldRebarKey(key,grade,type,VertBarSize);
         }
         else
         {
            Int32 key;
            if ( FAILED(pStrLoad->Property(_T("VertBarSize"),&key)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               VertBarSize = matRebar::Size(key);
         }

         Uint32 val;
         if ( FAILED(pStrLoad->Property(_T("VertBars"),&val)) )
            return STRLOAD_E_INVALIDFORMAT;

         nVertBars = (Float64)val;

         if ( version < 3 )
         {
            Int16 key;
            if ( FAILED(pStrLoad->Property(_T("HorzBarSize"),&key)) )
               return STRLOAD_E_INVALIDFORMAT;
         
            matRebar::Grade grade;
            matRebar::Type type;
            lrfdRebarPool::MapOldRebarKey(key,grade,type,legacy_HorzBarSize);
         }
         else if (version < 4)
         {
            Int32 key;
            if ( FAILED(pStrLoad->Property(_T("HorzBarSize"),&key)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               legacy_HorzBarSize = matRebar::Size(key);

            Uint32 val;
            if ( FAILED(pStrLoad->Property(_T("HorzBars"),&val)) )
               return STRLOAD_E_INVALIDFORMAT;

            nHorzInterfaceBars = (Float64)val;
         }

         if (version > 3)
         {
            if (version < 9)
            {
               Uint32 val;
               if ( FAILED(pStrLoad->Property(_T("HorzInterfaceBars"),&val)) )
                  return STRLOAD_E_INVALIDFORMAT;

               nHorzInterfaceBars = (Float64)val;
            }
            else
            {
               if ( FAILED(pStrLoad->Property(_T("HorzInterfaceBars"),&nHorzInterfaceBars)) )
                  return STRLOAD_E_INVALIDFORMAT;
            }


            Int32 key;
            if ( FAILED(pStrLoad->Property(_T("ConfinementBarSize"),&key)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               ConfinementBarSize = matRebar::Size(key);
         }
      }

      if ( FAILED(pStrLoad->EndUnit()) )
         return STRLOAD_E_INVALIDFORMAT;

      // Convert old data if need be
      if (bConvertToShearDataVersion9)
      {
         assert(version < 4); // should only happen if

         // Confinement as part of zone was added in version 4
         if (ZoneNum <= NumConfinementZones)
         {
            ConfinementBarSize = confinementBarSize;
         }
         else
         {
            ConfinementBarSize = matRebar::bsNone;
         }

         nHorzInterfaceBars = bDoStirrupsEngageDeck ? nVertBars : 0.0;

      }
   }
   else
   {
      return E_FAIL;
   }


   return hr;
}

HRESULT CShearZoneData::Save(sysIStructuredSave* pStrSave)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("ShearZoneData"),4.0);
   pStrSave->Property(_T("ZoneNum"),    ZoneNum );
   pStrSave->Property(_T("ZoneLength"), ZoneLength );
   pStrSave->Property(_T("BarSpacing"), BarSpacing );
   pStrSave->Property(_T("VertBarSize"),(long)VertBarSize );
   pStrSave->Property(_T("VertBars"),   nVertBars );
   pStrSave->Property(_T("HorzInterfaceBars"),   nHorzInterfaceBars );
   pStrSave->Property(_T("ConfinementBarSize"),(long)ConfinementBarSize );
   pStrSave->EndUnit();

   return hr;
}
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CShearZoneData::MakeCopy(const CShearZoneData& rOther)
{
   ZoneNum       = rOther.ZoneNum;
   BarSpacing    = rOther.BarSpacing;
   ZoneLength    = rOther.ZoneLength;
   VertBarSize   = rOther.VertBarSize;
   nVertBars     = rOther.nVertBars;
   nHorzInterfaceBars     = rOther.nHorzInterfaceBars;
   ConfinementBarSize     = rOther.ConfinementBarSize;

   legacy_HorzBarSize = rOther.legacy_HorzBarSize;
   legacy_nHorzBars   = rOther.legacy_nHorzBars;
}

void CShearZoneData::MakeAssignment(const CShearZoneData& rOther)
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
