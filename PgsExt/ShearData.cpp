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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ShearData.h>
#include <Units\SysUnits.h>
#include <StdIo.h>
#include <StrData.cpp>
#include <atlbase.h>
#include <algorithm>
#include <psgLib\GirderLibraryEntry.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CShearData
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CShearData::CShearData():
ConfinementBarSize(0),
NumConfinementZones(0),
TopFlangeBarSize(0),
TopFlangeBarSpacing(0.0),
bDoStirrupsEngageDeck(true),
bIsRoughenedSurface(true),
strRebarMaterial(_T("AASHTO M31 (A615) - Grade 60"))
{
   // make sure we have at least one zone
   CShearZoneData tmp;
   tmp.ZoneNum = 1;
   tmp.ZoneLength = Float64_Max;
   ShearZones.push_back(tmp);
}

CShearData::CShearData(const CShearData& rOther)
{
   MakeCopy(rOther);
}

CShearData::~CShearData()
{
}

//======================== OPERATORS  =======================================
CShearData& CShearData::operator= (const CShearData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CShearData::operator == (const CShearData& rOther) const
{
   if ( ConfinementBarSize != rOther.ConfinementBarSize )
      return false;

   if ( NumConfinementZones != rOther.NumConfinementZones)
      return false;

   if ( ShearZones != rOther.ShearZones )
      return false;

   if ( bDoStirrupsEngageDeck != rOther.bDoStirrupsEngageDeck )
      return false;

   if ( bIsRoughenedSurface != rOther.bIsRoughenedSurface )
      return false;

   if ( TopFlangeBarSize != rOther.TopFlangeBarSize)
      return false;

   if ( TopFlangeBarSpacing != rOther.TopFlangeBarSpacing)
      return false;

   if ( strRebarMaterial != rOther.strRebarMaterial )
      return false;

   return true;
}

bool CShearData::operator != (const CShearData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CShearData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   pStrLoad->BeginUnit(_T("ShearData"));  // named this for historical reasons
   double version;
   pStrLoad->get_Version(&version);

   CComVariant var;

   if ( 5.0 <= version )
   {
      var.Clear();
      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("RebarType"),&var);
      strRebarMaterial = OLE2T(var.bstrVal);
   }

   var.Clear();
   var.vt = VT_UI4;
   pStrLoad->get_Property(_T("ConfinementBarSize"), &var );
   ConfinementBarSize = var.uiVal;

   var.Clear();
   var.vt = VT_UI4;
   pStrLoad->get_Property(_T("ConfinementZone"), &var );
   NumConfinementZones = var.uiVal;

   if (3.0 < version)
   {
      var.Clear();
      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("DoStirrupsEngageDeck"), &var );
      bDoStirrupsEngageDeck = (var.boolVal == VARIANT_TRUE ? true : false);
   }

   if (5.0 < version) // added in version 6.0
   {
      var.Clear();
      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("IsRoughenedSurface"), &var );
      bIsRoughenedSurface = (var.boolVal == VARIANT_TRUE ? true : false);
   }

   var.Clear();
   var.vt = VT_UI4;
   pStrLoad->get_Property(_T("TopFlangeBarSize"), &var );
   TopFlangeBarSize = var.uiVal;

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("TopFlangeBarSpacing"), &var );
   TopFlangeBarSpacing = var.dblVal;

   ShearZones.clear();

   var.vt = VT_I4;
   hr = pStrLoad->get_Property(_T("ZoneCount"), &var );
   if ( FAILED(hr) )
      return hr;

   for ( int i = 0; i < var.lVal; i++ )
   {
      CShearZoneData zd;
      hr = zd.Load( pStrLoad, pProgress );
      if ( FAILED(hr) )
         return hr;

      ShearZones.push_back( zd );
   }
   
   std::sort( ShearZones.begin(), ShearZones.end(), ShearZoneDataLess() );

   pStrLoad->EndUnit();

   return hr;
}

HRESULT CShearData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;


   pStrSave->BeginUnit(_T("ShearData"),6.0);

   pStrSave->put_Property(_T("RebarType"),CComVariant(strRebarMaterial.c_str()));
   pStrSave->put_Property(_T("ConfinementBarSize"), CComVariant(ConfinementBarSize));
   pStrSave->put_Property(_T("ConfinementZone"),    CComVariant(NumConfinementZones));
   pStrSave->put_Property(_T("DoStirrupsEngageDeck"),CComVariant(bDoStirrupsEngageDeck));
   pStrSave->put_Property(_T("IsRoughenedSurface"),CComVariant(bIsRoughenedSurface));
   pStrSave->put_Property(_T("TopFlangeBarSize"),   CComVariant(TopFlangeBarSize));
   pStrSave->put_Property(_T("TopFlangeBarSpacing"),CComVariant(TopFlangeBarSpacing));

   CComVariant var( (Int32)ShearZones.size() );
   hr = pStrSave->put_Property(_T("ZoneCount"), var );
   if ( FAILED(hr) )
      return hr;

   ShearZoneIterator i;
   for ( i = ShearZones.begin(); i != ShearZones.end(); i++ )
   {
      CShearZoneData& pd = *i;
      hr = pd.Save( pStrSave, pProgress );
      if ( FAILED(hr) )
         return hr;
   }

   pStrSave->EndUnit();

   return hr;
}

void CShearData::CopyGirderEntryData(const GirderLibraryEntry& rGird)
{
   ConfinementBarSize  = rGird.GetShearSteelBarSize();
   NumConfinementZones = rGird.GetNumConfinementZones();

   TopFlangeBarSize      = rGird.GetTopFlangeShearBarSize();
   TopFlangeBarSpacing   = rGird.GetTopFlangeShearBarSpacing();
   bDoStirrupsEngageDeck = rGird.DoStirrupsEngageDeck();
   bIsRoughenedSurface   = rGird.IsRoughenedSurface();

   ShearZones.clear();
   GirderLibraryEntry::ShearZoneInfoVec libvec = rGird.GetShearZoneInfo();

   // assume library data is sorted
   if (libvec.size()>0)
   {
      Int32 zone_num=1;
      for(GirderLibraryEntry::ShearZoneInfoVec::const_iterator it=libvec.begin(); it!=libvec.end(); it++)
      {
         CShearZoneData zd;
         zd.ZoneNum     = zone_num;
         zd.VertBarSize = it->VertBarSize;
         zd.HorzBarSize = it->HorzBarSize;
         zd.BarSpacing  = it->StirrupSpacing;
         zd.ZoneLength  = it->ZoneLength;
         zd.nVertBars   = it->nVertBars;
         zd.nHorzBars   = it->nHorzBars;

         ShearZones.push_back(zd);
         zone_num++;
      }
   }
   else
   {
      // make sure we always have at least one empty zone
      CShearZoneData zd;
      zd.ZoneNum     = 1;
      zd.VertBarSize = 0;
      zd.HorzBarSize = 0;
      zd.BarSpacing  = 0.0;
      zd.ZoneLength  = 0.0;
      zd.nVertBars   = 2;
      zd.nHorzBars   = 2;
      ShearZones.push_back(zd);
   }

   strRebarMaterial = rGird.GetShearSteelMaterial();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CShearData::MakeCopy(const CShearData& rOther)
{
   ConfinementBarSize    = rOther.ConfinementBarSize;
   NumConfinementZones   = rOther.NumConfinementZones;
   ShearZones            = rOther.ShearZones;
   TopFlangeBarSize      = rOther.TopFlangeBarSize;
   TopFlangeBarSpacing   = rOther.TopFlangeBarSpacing;
   bDoStirrupsEngageDeck = rOther.bDoStirrupsEngageDeck;
   bIsRoughenedSurface   = rOther.bIsRoughenedSurface;
   strRebarMaterial      = rOther.strRebarMaterial;
}

void CShearData::MakeAssignment(const CShearData& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
HRESULT CShearData::ShearProc(IStructuredSave* pSave,
                              IStructuredLoad* pLoad,
                              IProgress* pProgress,
                              CShearData* pObj)
{
   HRESULT hr = S_OK;

   if ( pSave )
   {
      CComVariant var( (Int32)pObj->ShearZones.size() );
      hr = pSave->put_Property(_T("ZoneCount"), var );
      if ( FAILED(hr) )
         return hr;

      ShearZoneIterator i;
      for ( i = pObj->ShearZones.begin(); i != pObj->ShearZones.end(); i++ )
      {
         CShearZoneData& pd = *i;
         hr = pd.Save( pSave, pProgress );
         if ( FAILED(hr) )
            return hr;
      }
   }
   else
   {
      pObj->ShearZones.clear();

      CComVariant var;
      var.vt = VT_I4;
      hr = pLoad->get_Property(_T("ZoneCount"), &var );
      if ( FAILED(hr) )
         return hr;

      for ( int i = 0; i < var.lVal; i++ )
      {
         CShearZoneData zd;
         hr = zd.Load( pLoad, pProgress );
         if ( FAILED(hr) )
            return hr;

         pObj->ShearZones.push_back( zd );
      }
      
      std::sort( pObj->ShearZones.begin(), pObj->ShearZones.end(), ShearZoneDataLess() );
   }
   return hr;
}
