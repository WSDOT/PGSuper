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
#include <PgsExt\DeckRebarData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDeckRebarData
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDeckRebarData::CDeckRebarData() :
strRebarMaterial("AASHTO M31 (615) - Grade 60")
{
   TopCover    = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   BottomCover = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   TopRebarKey    = INVALID_BAR_SIZE;
   BottomRebarKey = INVALID_BAR_SIZE;

   TopSpacing    = ::ConvertToSysUnits(18.0,unitMeasure::Inch);
   BottomSpacing = ::ConvertToSysUnits(18.0,unitMeasure::Inch);

   // default values are for LRFD imperical deck design
   TopLumpSum    = ::ConvertToSysUnits(0.18,unitMeasure::Inch2PerFoot);
   BottomLumpSum = ::ConvertToSysUnits(0.27,unitMeasure::Inch2PerFoot);
}

CDeckRebarData::CDeckRebarData(const CDeckRebarData& rOther)
{
   MakeCopy(rOther);
}

CDeckRebarData::~CDeckRebarData()
{
}

//======================== OPERATORS  =======================================
CDeckRebarData& CDeckRebarData::operator= (const CDeckRebarData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
HRESULT CDeckRebarData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit("DeckRebar",2.0);

   pStrSave->put_Property("RebarType",CComVariant(strRebarMaterial.c_str()));

   pStrSave->put_Property("TopCover",CComVariant(TopCover));
   pStrSave->put_Property("TopLumpSumArea",CComVariant(TopLumpSum));
   pStrSave->put_Property("TopRebarKey",CComVariant(TopRebarKey));
   pStrSave->put_Property("TopSpacing",CComVariant(TopSpacing));

   pStrSave->put_Property("BottomCover",CComVariant(BottomCover));
   pStrSave->put_Property("BottomLumpSumArea",CComVariant(BottomLumpSum));
   pStrSave->put_Property("BottomRebarKey",CComVariant(BottomRebarKey));
   pStrSave->put_Property("BottomSpacing",CComVariant(BottomSpacing));

   pStrSave->put_Property("NegMomentCount",CComVariant((long)NegMomentRebar.size()));
   std::vector<NegMomentRebarData>::iterator iter;
   for ( iter = NegMomentRebar.begin(); iter != NegMomentRebar.end(); iter++ )
   {
      NegMomentRebarData& rebar = *iter;
      pStrSave->BeginUnit("NegMomentRebar",1.0);

      pStrSave->put_Property("Pier",CComVariant(rebar.PierIdx));
      pStrSave->put_Property("Mat",CComVariant(rebar.Mat));
      pStrSave->put_Property("LumpSumArea",CComVariant(rebar.LumpSum));
      pStrSave->put_Property("RebarKey",CComVariant(rebar.RebarKey));
      pStrSave->put_Property("Spacing",CComVariant(rebar.Spacing));
      pStrSave->put_Property("LeftCutoff",CComVariant(rebar.LeftCutoff));
      pStrSave->put_Property("RightCutoff",CComVariant(rebar.RightCutoff));

      pStrSave->EndUnit();
   }

   pStrSave->EndUnit();

   return hr;
}

HRESULT CDeckRebarData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   HRESULT hr = S_OK;

   pStrLoad->BeginUnit("DeckRebar");

   CComVariant var;

   double version;
   pStrLoad->get_Version(&version);

   if ( 2.0 <= version )
   {
      var.vt = VT_BSTR;
      pStrLoad->get_Property("RebarType",&var);
      strRebarMaterial = OLE2A(var.bstrVal);
   }

   var.vt = VT_R8;
   pStrLoad->get_Property("TopCover",&var);
   TopCover = var.dblVal;

   pStrLoad->get_Property("TopLumpSumArea",&var);
   TopLumpSum = var.dblVal;

   var.vt = VT_I4;
   pStrLoad->get_Property("TopRebarKey",&var);
   TopRebarKey = BarSizeType(var.lVal);

   var.vt = VT_R8;
   pStrLoad->get_Property("TopSpacing",&var);
   TopSpacing = var.dblVal;

   pStrLoad->get_Property("BottomCover",&var);
   BottomCover = var.dblVal;

   pStrLoad->get_Property("BottomLumpSumArea",&var);
   BottomLumpSum = var.dblVal;

   var.vt = VT_I4;
   pStrLoad->get_Property("BottomRebarKey",&var);
   BottomRebarKey = BarSizeType(var.lVal);

   var.vt = VT_R8;
   pStrLoad->get_Property("BottomSpacing",&var);
   BottomSpacing = var.dblVal;

   var.vt = VT_I4;
   pStrLoad->get_Property("NegMomentCount",&var);
   long count = var.lVal;
   NegMomentRebar.clear();

   for ( long i = 0; i < count; i++ )
   {
      NegMomentRebarData rebar;
      pStrLoad->BeginUnit("NegMomentRebar");

      var.vt = VT_I2;
      pStrLoad->get_Property("Pier",&var);
      rebar.PierIdx = var.iVal;

      pStrLoad->get_Property("Mat",&var);
      rebar.Mat = (RebarMat)var.iVal;

      var.vt = VT_R8;
      pStrLoad->get_Property("LumpSumArea",&var);
      rebar.LumpSum = var.dblVal;

      var.vt = VT_I4;
      pStrLoad->get_Property("RebarKey",&var);
      rebar.RebarKey = BarSizeType(var.lVal);

      var.vt = VT_R8;
      pStrLoad->get_Property("Spacing",&var);
      rebar.Spacing = var.dblVal;

      pStrLoad->get_Property("LeftCutoff",&var);
      rebar.LeftCutoff = var.dblVal;

      pStrLoad->get_Property("RightCutoff",&var);
      rebar.RightCutoff = var.dblVal;

      pStrLoad->EndUnit();

      NegMomentRebar.push_back(rebar);
   }

   pStrLoad->EndUnit();

   return hr;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CDeckRebarData::MakeCopy(const CDeckRebarData& rOther)
{
   TopCover    = rOther.TopCover;
   BottomCover = rOther.BottomCover;

   TopRebarKey    = rOther.TopRebarKey;
   BottomRebarKey = rOther.BottomRebarKey;

   TopSpacing    = rOther.TopSpacing;
   BottomSpacing = rOther.BottomSpacing;

   TopLumpSum    = rOther.TopLumpSum;
   BottomLumpSum = rOther.BottomLumpSum;

   NegMomentRebar = rOther.NegMomentRebar;

   strRebarMaterial = rOther.strRebarMaterial;
}

void CDeckRebarData::MakeAssignment(const CDeckRebarData& rOther)
{
   MakeCopy( rOther );
}

bool CDeckRebarData::operator!=(const CDeckRebarData& rOther) const
{
   return !operator==(rOther);
}

bool CDeckRebarData::operator==(const CDeckRebarData& rOther) const
{
   if ( TopCover != rOther.TopCover )
      return false;

   if ( BottomCover != rOther.BottomCover )
      return false;

   if ( TopRebarKey != rOther.TopRebarKey )
      return false;

   if ( BottomRebarKey != rOther.BottomRebarKey )
      return false;

   if ( TopSpacing != rOther.TopSpacing )
      return false;

   if ( BottomSpacing != rOther.BottomSpacing )
      return false;

   if ( TopLumpSum != rOther.TopLumpSum )
      return false;

   if ( BottomLumpSum != rOther.BottomLumpSum )
      return false;

   if ( NegMomentRebar != rOther.NegMomentRebar )
      return false;

   if ( strRebarMaterial != rOther.strRebarMaterial )
      return false;

   return true;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
bool CDeckRebarData::NegMomentRebarData::operator!=(const CDeckRebarData::NegMomentRebarData& rOther) const
{
   return !operator==(rOther);
}

bool CDeckRebarData::NegMomentRebarData::operator==(const CDeckRebarData::NegMomentRebarData& rOther) const
{
   if ( PierIdx != rOther.PierIdx )
      return false;

   if ( Mat != rOther.Mat )
      return false;

   if ( LumpSum != rOther.LumpSum )
      return false;

   if ( RebarKey != rOther.RebarKey )
      return false;

   if ( Spacing != rOther.Spacing )
      return false;

   if ( LeftCutoff != rOther.LeftCutoff )
      return false;

   if ( RightCutoff != rOther.RightCutoff )
      return false;

   return true;
}
