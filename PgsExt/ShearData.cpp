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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ShearData.h>
#include <Units\Convert.h>
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
ShearBarType(WBFL::Materials::Rebar::Type::A615),
ShearBarGrade(WBFL::Materials::Rebar::Grade::Grade60),
bIsRoughenedSurface(true),
bAreZonesSymmetrical(true),
bUsePrimaryForSplitting(true),
SplittingBarSize(WBFL::Materials::Rebar::Size::bsNone),
SplittingBarSpacing(0),
SplittingZoneLength(0),
nSplittingBars(0),
ConfinementBarSize(WBFL::Materials::Rebar::Size::bsNone),
ConfinementBarSpacing(0),
ConfinementZoneLength(0)
{
   // make sure we have at least one primary and horiz inter zone
   CShearZoneData tmp;
   tmp.ZoneNum = 0;
   tmp.ZoneLength = Float64_Max;
   ShearZones.push_back(tmp);

   CHorizontalInterfaceZoneData htmp;
   htmp.ZoneNum = 0;
   htmp.ZoneLength = Float64_Max;
   HorizontalInterfaceZones.push_back(htmp);
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
   if ( ShearZones != rOther.ShearZones )
   {
      return false;
   }

   if ( bIsRoughenedSurface != rOther.bIsRoughenedSurface )
   {
      return false;
   }

   if ( ShearBarGrade != rOther.ShearBarGrade )
   {
      return false;
   }

   if ( ShearBarType != rOther.ShearBarType )
   {
      return false;
   }

   if ( HorizontalInterfaceZones != rOther.HorizontalInterfaceZones )
   {
      return false;
   }

   if ( bAreZonesSymmetrical != rOther.bAreZonesSymmetrical )
   {
      return false;
   }

   if ( bUsePrimaryForSplitting != rOther.bUsePrimaryForSplitting )
   {
      return false;
   }

   if ( SplittingBarSize != rOther.SplittingBarSize )
   {
      return false;
   }

   if ( SplittingBarSpacing != rOther.SplittingBarSpacing )
   {
      return false;
   }

   if ( SplittingZoneLength != rOther.SplittingZoneLength )
   {
      return false;
   }

   if ( nSplittingBars != rOther.nSplittingBars )
   {
      return false;
   }

   if ( ConfinementBarSize != rOther.ConfinementBarSize )
   {
      return false;
   }

   if ( ConfinementBarSpacing != rOther.ConfinementBarSpacing )
   {
      return false;
   }

   if ( ConfinementZoneLength != rOther.ConfinementZoneLength )
   {
      return false;
   }

   return true;
}

bool CShearData::operator != (const CShearData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CShearData::Load(WBFL::System::IStructuredLoad* pStrLoad)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   pStrLoad->BeginUnit(_T("ShearData"));  // named this for historical reasons
   Float64 version = pStrLoad->GetVersion();

   if ( 5.0 <= version && version < 7)
   {
      std::_tstring strRebarMaterial;
      pStrLoad->Property(_T("RebarType"),&strRebarMaterial); // throw out old material string
   }

   // These were data members prior to version 8. Now they are needed for conversion
   WBFL::Materials::Rebar::Size legacy_ConfinementBarSize(WBFL::Materials::Rebar::Size::bsNone);
   Uint32         NumConfinementZones(0);
   bool           bDoStirrupsEngageDeck(true);
   WBFL::Materials::Rebar::Size TopFlangeBarSize(WBFL::Materials::Rebar::Size::bsNone);
   Float64        TopFlangeBarSpacing(0.0);

   if ( version < 8 )
   {
      if ( version < 7 )
      {
         Uint32 key;
         pStrLoad->Property(_T("ConfinementBarSize"), &key );
         WBFL::Materials::Rebar::Grade grade;
         WBFL::Materials::Rebar::Type type;
         WBFL::LRFD::RebarPool::MapOldRebarKey(key,grade,type,legacy_ConfinementBarSize);
      }
      else
      {
         Uint32 key;
         pStrLoad->Property(_T("ConfinementBarSize"), &key );
         legacy_ConfinementBarSize = WBFL::Materials::Rebar::Size(key);
      }

      pStrLoad->Property(_T("ConfinementZone"), &NumConfinementZones );

      if (3.0 < version)
      {
         pStrLoad->Property(_T("DoStirrupsEngageDeck"), &bDoStirrupsEngageDeck );
      }
   }

   if (5.0 < version) // added in version 6.0
   {
      pStrLoad->Property(_T("IsRoughenedSurface"), &bIsRoughenedSurface );
   }

   if ( version < 8 )
   {
      if ( version < 7 )
      {
         Uint32 bkey;
         pStrLoad->Property(_T("TopFlangeBarSize"), &bkey );
         BarSizeType key = bkey;
         WBFL::Materials::Rebar::Grade grade;
         WBFL::Materials::Rebar::Type type;
         WBFL::LRFD::RebarPool::MapOldRebarKey(key,grade,type,TopFlangeBarSize);
      }
      else
      {
         Int32 key;
         pStrLoad->Property(_T("TopFlangeBarSize"), &key );
         TopFlangeBarSize = WBFL::Materials::Rebar::Size(key);
      }

      pStrLoad->Property(_T("TopFlangeBarSpacing"), &TopFlangeBarSpacing );
   }

   if ( 6 < version )
   {
      Int32 key;
      pStrLoad->Property(_T("ShearBarType"), &key );
      ShearBarType = WBFL::Materials::Rebar::Type(key);

      pStrLoad->Property(_T("ShearBarGrade"), &key );
      ShearBarGrade = WBFL::Materials::Rebar::Grade(key);
   }

   if ( 8 < version ) // added in version 9
   {
      pStrLoad->Property(_T("AreZonesSymmetrical"), &bAreZonesSymmetrical );
      pStrLoad->Property(_T("UsePrimaryForSplitting"), &bUsePrimaryForSplitting );
   }

   bool bConvertToVersion9 = (version < 9);

   // Shear zones
   ShearZones.clear();

   Int32 zcnt;
   hr = pStrLoad->Property(_T("ZoneCount"), &zcnt );
   if ( FAILED(hr) )
   {
      return hr;
   }

   for ( int i = 0; i < zcnt; i++ )
   {
      CShearZoneData zd;
      hr = zd.Load(pStrLoad, bConvertToVersion9, legacy_ConfinementBarSize, NumConfinementZones, bDoStirrupsEngageDeck);
      if ( FAILED(hr) )
      {
         return hr;
      }

      ShearZones.push_back( zd );
   }
   
   std::sort( ShearZones.begin(), ShearZones.end(), ShearZoneDataLess() );

   // there was a bug in the grid control that made the length of the last zone 0 when it should be Float64_Max
   // to represent "to mid-span" or "to end girder")
   if ( 0 < ShearZones.size() )
   {
      if ( IsEqual(ShearZones.back().ZoneLength,0.0) )
      {
         ShearZones.back().ZoneLength = Float64_Max;
      }

      // there was a bug that sometimes made the first zone begin at zone index 1
      // make sure the zone indexes are correct
      if ( ShearZones.front().ZoneNum != 0 )
      {
         Uint32 zoneIdx = 0;
         for(auto& sd : ShearZones)
         {
            sd.ZoneNum = zoneIdx++;
         }
      }
   }

   if (bConvertToVersion9)
   {
      // Last thing is old "top flange" bars
      if (TopFlangeBarSize != WBFL::Materials::Rebar::Size::bsNone && TopFlangeBarSpacing > 0.0)
      {
         // should be an existing zone extended to mid-girder
         CHorizontalInterfaceZoneData& rzhdat = HorizontalInterfaceZones.front();
         rzhdat.BarSize = TopFlangeBarSize;
         rzhdat.BarSpacing = TopFlangeBarSpacing;
         rzhdat.nBars = 2; // this was hard-coded previously
      }
   }
   else // Stuff added in version 9 and after...
   {
      // Horizontal interface shear zones
      HorizontalInterfaceZones.clear();

      hr = pStrLoad->Property(_T("HorizZoneCount"), &zcnt );
      if ( FAILED(hr) )
      {
         return hr;
      }

      for ( int i = 0; i < zcnt; i++ )
      {
         CHorizontalInterfaceZoneData zd;
         hr = zd.Load(pStrLoad);
         if ( FAILED(hr) )
         {
            return hr;
         }

         HorizontalInterfaceZones.push_back( zd );
      }
      
      std::sort( HorizontalInterfaceZones.begin(), HorizontalInterfaceZones.end(), HorizontalInterfaceZoneDataLess() );

      // there was a bug in the grid control that made the length of the last zone 0 when it should be Float64_Max
      // to represent "to mid-span" or "to end girder")
      if ( 0 < HorizontalInterfaceZones.size() )
      {
         if ( IsEqual(HorizontalInterfaceZones.back().ZoneLength,0.0) )
         {
            HorizontalInterfaceZones.back().ZoneLength = Float64_Max;
         }

         // there was a bug that sometimes made the first zone begin at zone index 1
         // make sure the zone indexes are correct
         if ( HorizontalInterfaceZones.front().ZoneNum != 0 )
         {
            ZoneIndexType zoneIdx = 0;
            for (auto& zd : HorizontalInterfaceZones)
            {
               zd.ZoneNum = zoneIdx++;
            }
         }
      }

      Int32 key;
      pStrLoad->Property(_T("SplittingBarSize"), &key );
      SplittingBarSize = WBFL::Materials::Rebar::Size(key);

      pStrLoad->Property(_T("SplittingBarSpacing"), &SplittingBarSpacing );
      pStrLoad->Property(_T("SplittingZoneLength"), &SplittingZoneLength );
      pStrLoad->Property(_T("nSplittingBars"), &nSplittingBars );

      pStrLoad->Property(_T("ConfinementBarSize"), &key );
      ConfinementBarSize = WBFL::Materials::Rebar::Size(key);

      pStrLoad->Property(_T("ConfinementBarSpacing"), &ConfinementBarSpacing );
      pStrLoad->Property(_T("ConfinementZoneLength"), &ConfinementZoneLength );
   }

   pStrLoad->EndUnit();

   return hr;
}

HRESULT CShearData::Save(WBFL::System::IStructuredSave* pStrSave)
{
   HRESULT hr = S_OK;


   pStrSave->BeginUnit(_T("ShearData"),9.0);

   pStrSave->Property(_T("IsRoughenedSurface"),bIsRoughenedSurface);

   pStrSave->Property(_T("ShearBarType"), (Uint32)ShearBarType);
   pStrSave->Property(_T("ShearBarGrade"), (Uint32)ShearBarGrade);

   pStrSave->Property(_T("AreZonesSymmetrical"), bAreZonesSymmetrical );
   pStrSave->Property(_T("UsePrimaryForSplitting"), bUsePrimaryForSplitting );

   pStrSave->Property(_T("ZoneCount"), (Int32)ShearZones.size() );

   ShearZoneIterator i;
   for ( i = ShearZones.begin(); i != ShearZones.end(); i++ )
   {
      CShearZoneData& pd = *i;
      hr = pd.Save(pStrSave);
      if ( FAILED(hr) )
      {
         return hr;
      }
   }

   pStrSave->Property(_T("HorizZoneCount"), (Int32)HorizontalInterfaceZones.size() );

#if defined _DEBUG
   ZoneIndexType zoneIdx = 0;
#endif
   HorizontalInterfaceZoneIterator ih;
   for ( ih = HorizontalInterfaceZones.begin(); ih != HorizontalInterfaceZones.end(); ih++ )
   {
      CHorizontalInterfaceZoneData& rd = *ih;

      ATLASSERT(rd.ZoneNum == zoneIdx++);

      hr = rd.Save( pStrSave);
      if ( FAILED(hr) )
      {
         return hr;
      }
   }

   pStrSave->Property(_T("SplittingBarSize"), (Uint32)SplittingBarSize );
   pStrSave->Property(_T("SplittingBarSpacing"), SplittingBarSpacing );
   pStrSave->Property(_T("SplittingZoneLength"), SplittingZoneLength );
   pStrSave->Property(_T("nSplittingBars"), nSplittingBars );

   pStrSave->Property(_T("ConfinementBarSize"), (Uint32)ConfinementBarSize );
   pStrSave->Property(_T("ConfinementBarSpacing"), ConfinementBarSpacing );
   pStrSave->Property(_T("ConfinementZoneLength"), ConfinementZoneLength );

   pStrSave->EndUnit();

   return hr;
}

void CShearData::CopyGirderEntryData(const GirderLibraryEntry& rGird)
{
   ATLASSERT(false); // should never get here
   //*this = rGird.GetShearData();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CShearData::MakeCopy(const CShearData& rOther)
{
   ShearBarType            = rOther.ShearBarType;
   ShearBarGrade           = rOther.ShearBarGrade;
   bIsRoughenedSurface     = rOther.bIsRoughenedSurface;
   bAreZonesSymmetrical    = rOther.bAreZonesSymmetrical;
   bUsePrimaryForSplitting = rOther.bUsePrimaryForSplitting;

   ShearZones               = rOther.ShearZones;
   HorizontalInterfaceZones = rOther.HorizontalInterfaceZones;

   SplittingBarSize = rOther.SplittingBarSize;
   SplittingBarSpacing = rOther.SplittingBarSpacing;
   SplittingZoneLength = rOther.SplittingZoneLength;
   nSplittingBars = rOther.nSplittingBars;

   ConfinementBarSize = rOther.ConfinementBarSize;
   ConfinementBarSpacing = rOther.ConfinementBarSpacing;
   ConfinementZoneLength = rOther.ConfinementZoneLength;
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
/*
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
         hr = pd.Save( pSave );
         if ( FAILED(hr) )
            return hr;
      }
   }
   else
   {
      pObj->ShearZones.clear();

      CComVariant var;
      var.vt = VT_I4;
      hr = pLoad->Property(_T("ZoneCount"), &var );
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
*/

CShearData2 CShearData::Convert() const
{
   CShearData2 shearData;
   shearData.ShearBarType  = ShearBarType;
   shearData.ShearBarGrade = ShearBarGrade;

   shearData.bIsRoughenedSurface = bIsRoughenedSurface;
   shearData.bAreZonesSymmetrical = true;
   shearData.bUsePrimaryForSplitting = true;

   ShearZoneConstIterator zoneIter(ShearZones.begin());
   ShearZoneConstIterator zoneIterEnd(ShearZones.end());
   shearData.ShearZones.clear();
   for ( ; zoneIter != zoneIterEnd; zoneIter++ )
   {
      const CShearZoneData& zd = *zoneIter;
      CShearZoneData2 zoneData = zd.Convert();
      shearData.ShearZones.push_back( zoneData );
   }

   shearData.HorizontalInterfaceZones = HorizontalInterfaceZones;
   
   shearData.SplittingBarSize    = SplittingBarSize;
   shearData.SplittingBarSpacing = SplittingBarSpacing;
   shearData.SplittingZoneLength = SplittingZoneLength;
   shearData.nSplittingBars      = nSplittingBars;

   shearData.ConfinementBarSize    = ConfinementBarSize;
   shearData.ConfinementBarSpacing = ConfinementBarSpacing;
   shearData.ConfinementZoneLength = ConfinementZoneLength;

   return shearData;
}
