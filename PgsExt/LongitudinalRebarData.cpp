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
strRebarMaterial("AASHTO M31 (A615) - Grade 60")
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
   return (RebarRows == rOther.RebarRows) && (strRebarMaterial == rOther.strRebarMaterial);
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

   pStrLoad->BeginUnit("LongitudinalRebar"); 
   double version;
   pStrLoad->get_Version(&version);

   CComVariant var;

   if ( 2.0 <= version )
   {
      var.Clear();
      var.vt = VT_BSTR;
      pStrLoad->get_Property("RebarType",&var);
      strRebarMaterial = OLE2A(var.bstrVal);
   }

   var.Clear();
   var.vt = VT_I4;
   pStrLoad->get_Property("RebarRowCount", &var );
   long count = var.lVal;

   RebarRows.clear();
   for ( long row = 0; row < count; row++ )
   {
      pStrLoad->BeginUnit("RebarRow");

      RebarRow rebar_row;

      var.vt = VT_I4;
      pStrLoad->get_Property("Face",         &var);
      rebar_row.Face = (GirderFace)(var.lVal);

      var.vt = VT_R8;
      pStrLoad->get_Property("Cover",        &var);
      rebar_row.Cover = var.dblVal;

      var.vt = VT_I4;
      pStrLoad->get_Property("NumberOfBars", &var);
      rebar_row.NumberOfBars = var.lVal;

      pStrLoad->get_Property("BarSize", &var);
      rebar_row.BarSize = var.lVal;

      var.vt = VT_R8;
      pStrLoad->get_Property("BarSpacing", &var);
      rebar_row.BarSpacing = var.dblVal;

      RebarRows.push_back(rebar_row);

      pStrLoad->EndUnit();
   }

   pStrLoad->EndUnit();

   return hr;
}

HRESULT CLongitudinalRebarData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;


   pStrSave->BeginUnit("LongitudinalRebar",2.0);

   pStrSave->put_Property("RebarType",CComVariant(strRebarMaterial.c_str()));

   long count = RebarRows.size();
   pStrSave->put_Property("RebarRowCount",CComVariant(count));
   std::vector<RebarRow>::iterator iter;
   for ( iter = RebarRows.begin(); iter != RebarRows.end(); iter++ )
   {
      pStrSave->BeginUnit("RebarRow",1.0);
      const RebarRow& rebar_row = *iter;
      pStrSave->put_Property("Face",         CComVariant(rebar_row.Face));
      pStrSave->put_Property("Cover",        CComVariant(rebar_row.Cover));
      pStrSave->put_Property("NumberOfBars", CComVariant(rebar_row.NumberOfBars));
      pStrSave->put_Property("BarSize",      CComVariant(rebar_row.BarSize));
      pStrSave->put_Property("BarSpacing",   CComVariant(rebar_row.BarSpacing));
      pStrSave->EndUnit();
   }


   pStrSave->EndUnit();

   return hr;
}

void CLongitudinalRebarData::CopyGirderEntryData(const GirderLibraryEntry& rGird)
{
   GirderLibraryEntry::LongSteelInfoVec lsiv = rGird.GetLongSteelInfo();
   GirderLibraryEntry::LongSteelInfoVec::iterator iter;

   RebarRows.clear();
   for ( iter = lsiv.begin(); iter != lsiv.end(); iter++ )
   {
      GirderLibraryEntry::LongSteelInfo& lsi = *iter;

      RebarRow rebar_row;
      rebar_row.Face       = (CLongitudinalRebarData::GirderFace)lsi.Face;
      rebar_row.NumberOfBars = lsi.NumberOfBars;
      rebar_row.Cover      = lsi.Cover;
      rebar_row.BarSize    = lsi.BarSize;
      rebar_row.BarSpacing = lsi.BarSpacing;

      RebarRows.push_back(rebar_row);
   }

   strRebarMaterial = rGird.GetLongSteelMaterial();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLongitudinalRebarData::MakeCopy(const CLongitudinalRebarData& rOther)
{
   RebarRows = rOther.RebarRows;
   strRebarMaterial = rOther.strRebarMaterial;
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
