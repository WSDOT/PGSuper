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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\LongitudinalRebarData.h>
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
   CLongitudinalRebarData
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLongitudinalRebarData::CLongitudinalRebarData() :
BarType(matRebar::A615),
BarGrade(matRebar::Grade60)
{
}

CLongitudinalRebarData::CLongitudinalRebarData(const CLongitudinalRebarData& rOther)
{
   MakeCopy(rOther);
}

CLongitudinalRebarData::~CLongitudinalRebarData()
{
}

//======================== OPERATORS  =======================================
CLongitudinalRebarData& CLongitudinalRebarData::operator= (const CLongitudinalRebarData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CLongitudinalRebarData::operator == (const CLongitudinalRebarData& rOther) const
{
   if ( BarType != rOther.BarType )
   {
      return false;
   }

   if ( BarGrade != rOther.BarGrade )
   {
      return false;
   }

   return (RebarRows == rOther.RebarRows);
}

bool CLongitudinalRebarData::operator != (const CLongitudinalRebarData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CLongitudinalRebarData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   pStrLoad->BeginUnit(_T("LongitudinalRebar")); 
   Float64 version;
   pStrLoad->get_Version(&version);

   CComVariant var;

   if ( 2.0 == version )
   {
      var.Clear();
      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("RebarType"),&var);
      std::_tstring strRebarMaterial = OLE2T(var.bstrVal);
   }

   if ( 2 < version )
   {
      var.vt = VT_I4;

      pStrLoad->get_Property(_T("BarGrade"), &var );
      BarGrade = matRebar::Grade(var.lVal);

      pStrLoad->get_Property(_T("BarType"), &var );
      BarType = matRebar::Type(var.lVal);
   }


   var.Clear();
   var.vt = VT_INDEX;
   pStrLoad->get_Property(_T("RebarRowCount"), &var );
   IndexType count = VARIANT2INDEX(var);

   RebarRows.clear();
   for ( IndexType row = 0; row < count; row++ )
   {
      pStrLoad->BeginUnit(_T("RebarRow"));

      Float64 bar_version;
      pStrLoad->get_Version(&bar_version);

      RebarRow rebar_row;

      if ( 2 < bar_version )
      {
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("BarLayout"),         &var);
         rebar_row.BarLayout = (pgsTypes::RebarLayoutType)(var.lVal);

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("DistFromEnd"),        &var);
         rebar_row.DistFromEnd = var.dblVal;

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("BarLength"),        &var);
         rebar_row.BarLength = var.dblVal;
      }

      var.vt = VT_I4;
      pStrLoad->get_Property(_T("Face"),         &var);
      rebar_row.Face = (pgsTypes::FaceType)(var.lVal);

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Cover"),        &var);
      rebar_row.Cover = var.dblVal;

      var.vt = VT_I4;
      pStrLoad->get_Property(_T("NumberOfBars"), &var);
      rebar_row.NumberOfBars = var.lVal;

      if ( bar_version < 2 )
      {
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("BarSize"), &var);
         BarSizeType key = var.lVal;

         matRebar::Grade grade;
         matRebar::Type type;
         lrfdRebarPool::MapOldRebarKey(key,grade,type,rebar_row.BarSize);
      }
      else
      {
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("BarSize"), &var );
         rebar_row.BarSize = matRebar::Size(var.lVal);
      }

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("BarSpacing"), &var);
      rebar_row.BarSpacing = var.dblVal;

      if (3 < bar_version)
      {
         // added in version 4
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("ExtendedLeft"), &var);
         rebar_row.bExtendedLeft = (var.boolVal == VARIANT_TRUE);

         pStrLoad->get_Property(_T("ExtendedRight"), &var);
         rebar_row.bExtendedRight = (var.boolVal == VARIANT_TRUE);
      }

      RebarRows.push_back(rebar_row);

      pStrLoad->EndUnit();
   }

   pStrLoad->EndUnit();

   return hr;
}

HRESULT CLongitudinalRebarData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;


   pStrSave->BeginUnit(_T("LongitudinalRebar"),4.0);

   pStrSave->put_Property(_T("BarGrade"),     CComVariant(BarGrade));
   pStrSave->put_Property(_T("BarType"),      CComVariant(BarType));

   CollectionIndexType count = RebarRows.size();
   pStrSave->put_Property(_T("RebarRowCount"),CComVariant(count));
   std::vector<RebarRow>::iterator iter;
   for ( iter = RebarRows.begin(); iter != RebarRows.end(); iter++ )
   {
      pStrSave->BeginUnit(_T("RebarRow"),4.0);
      const RebarRow& rebar_row = *iter;

      pStrSave->put_Property(_T("BarLayout"),     CComVariant(rebar_row.BarLayout));
      pStrSave->put_Property(_T("DistFromEnd"),   CComVariant(rebar_row.DistFromEnd));
      pStrSave->put_Property(_T("BarLength"),     CComVariant(rebar_row.BarLength));

      pStrSave->put_Property(_T("Face"),         CComVariant(rebar_row.Face));
      pStrSave->put_Property(_T("Cover"),        CComVariant(rebar_row.Cover));
      pStrSave->put_Property(_T("NumberOfBars"), CComVariant(rebar_row.NumberOfBars));
      pStrSave->put_Property(_T("BarSize"),      CComVariant(rebar_row.BarSize));
      pStrSave->put_Property(_T("BarSpacing"),   CComVariant(rebar_row.BarSpacing));

      // added in version 4
      pStrSave->put_Property(_T("ExtendedLeft"), CComVariant(rebar_row.bExtendedLeft));
      pStrSave->put_Property(_T("ExtendedRight"), CComVariant(rebar_row.bExtendedRight));
      pStrSave->EndUnit();
   }


   pStrSave->EndUnit();

   return hr;
}

void CLongitudinalRebarData::CopyGirderEntryData(const GirderLibraryEntry* pGirderEntry)
{
   GirderLibraryEntry::LongSteelInfoVec lsiv = pGirderEntry->GetLongSteelInfo();
   GirderLibraryEntry::LongSteelInfoVec::iterator iter;

   pGirderEntry->GetLongSteelMaterial(BarType,BarGrade);

   RebarRows.clear();
   for ( iter = lsiv.begin(); iter != lsiv.end(); iter++ )
   {
      GirderLibraryEntry::LongSteelInfo& lsi = *iter;

      RebarRow rebar_row;
      rebar_row.BarLayout    = lsi.BarLayout;
      rebar_row.BarLength    = lsi.BarLength;
      rebar_row.DistFromEnd  = lsi.DistFromEnd;

      rebar_row.Face         = lsi.Face;
      rebar_row.NumberOfBars = lsi.NumberOfBars;
      rebar_row.Cover      = lsi.Cover;
      rebar_row.BarSize    = lsi.BarSize;
      rebar_row.BarSpacing = lsi.BarSpacing;

      RebarRows.push_back(rebar_row);
   }

   //strRebarMaterial = rGird.GetLongSteelMaterial();
}

bool CLongitudinalRebarData::RebarRow::operator==(const RebarRow& other) const
{
   if (BarLayout != other.BarLayout)
   {
      return false;
   }

   if (BarLayout != pgsTypes::blFullLength)
   {
      if (BarLayout != pgsTypes::blMidGirderEnds)
      {
         if (!IsEqual(BarLength, other.BarLength)) return false;
      }

      if (BarLayout != pgsTypes::blMidGirderLength)
      {
         if (!IsEqual(DistFromEnd, other.DistFromEnd)) return false;
      }
   }

   if (Face != other.Face) return false;
   if (BarSize != other.BarSize) return false;
   if (!IsEqual(Cover, other.Cover)) return false;
   if (!IsEqual(BarSpacing, other.BarSpacing)) return false;
   if (NumberOfBars != other.NumberOfBars) return false;

   if (
      BarLayout == pgsTypes::blFullLength ||
      ((BarLayout == pgsTypes::blFromLeft || BarLayout == pgsTypes::blFromRight) && IsZero(DistFromEnd))
      )
   {
      if (bExtendedLeft != other.bExtendedLeft || bExtendedRight != other.bExtendedRight)
      {
         return false;
      }
   }

   return true;
};

bool CLongitudinalRebarData::RebarRow::IsLeftEndExtended() const
{
   if (bExtendedLeft)
   {
      if (
         BarLayout == pgsTypes::blFullLength ||
         (BarLayout == pgsTypes::blFromLeft && IsZero(DistFromEnd))
         )
      {
         return true;
      }
   }

   return false;
}

bool CLongitudinalRebarData::RebarRow::IsRightEndExtended() const
{
   if (bExtendedRight)
   {
      if (
         BarLayout == pgsTypes::blFullLength ||
         (BarLayout == pgsTypes::blFromRight && IsZero(DistFromEnd))
         )
      {
         return true;
      }
   }

   return false;
}

bool CLongitudinalRebarData::RebarRow::GetRebarStartEnd(Float64 segmentLength, Float64* pBarStart, Float64* pBarEnd) const
{

   *pBarStart = 0.0;
   *pBarEnd   = 0.0;

   if ( matRebar::bsNone != BarSize && 0 < NumberOfBars )
   {
      // Determine longitudinal start and end of rebar layout
      if(BarLayout == pgsTypes::blFullLength)
      {
         *pBarStart = 0.0;
         *pBarEnd = segmentLength;
      }
      else if(BarLayout == pgsTypes::blFromLeft)
      {
         if (DistFromEnd < segmentLength)
         {
            *pBarStart = DistFromEnd;
            if (*pBarStart + BarLength < segmentLength)
            {
               *pBarEnd = *pBarStart + BarLength;
            }
            else
            {
               *pBarEnd = segmentLength;
            }
         }
         else
         {
            return false; // no bar within girder
         }
      }
      else if(BarLayout == pgsTypes::blFromRight)
      {
         if (DistFromEnd < segmentLength)
         {
            *pBarEnd = segmentLength - DistFromEnd;
            if (*pBarEnd - BarLength > 0.0)
            {
               *pBarStart = *pBarEnd - BarLength;
            }
            else
            {
               *pBarStart = 0.0;
            }
         }
         else
         {
            return false; // no bar within girder
         }
      }
      else if(BarLayout == pgsTypes::blMidGirderEnds)
      {
         Float64 gl2 = segmentLength/2.0;

         if (DistFromEnd < gl2)
         {
            *pBarStart = DistFromEnd;
            *pBarEnd   = segmentLength - DistFromEnd;
         }
         else
         {
            return false; // no bar within girder
         }
      }
      else if(BarLayout == pgsTypes::blMidGirderLength)
      {
         Float64 gl2 = segmentLength/2.0;

         if (segmentLength < BarLength)
         {
            *pBarStart = 0.0;
            *pBarEnd   = segmentLength;
         }
         else
         {
            *pBarStart = gl2 - BarLength/2.0;
            *pBarEnd   = gl2 + BarLength/2.0;
         }
      }
      else
      {
         ATLASSERT(false); // new bar layout type?
      }

      ATLASSERT(*pBarStart < *pBarEnd);

      return true;
   }
   else
   {
      return false;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLongitudinalRebarData::MakeCopy(const CLongitudinalRebarData& rOther)
{
   BarType   = rOther.BarType;
   BarGrade  = rOther.BarGrade;
   RebarRows = rOther.RebarRows;
}

void CLongitudinalRebarData::MakeAssignment(const CLongitudinalRebarData& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

#if defined _DEBUG
void CLongitudinalRebarData::AssertValid()
{
}
#endif
