///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
CDeckRebarData::CDeckRebarData()
{
   TopCover    = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   BottomCover = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   TopRebarType = matRebar::A615;
   TopRebarGrade = matRebar::Grade60;
   TopRebarSize = matRebar::bsNone;

   BottomRebarType = matRebar::A615;
   BottomRebarGrade = matRebar::Grade60;
   BottomRebarSize = matRebar::bsNone;

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

   pStrSave->BeginUnit(_T("DeckRebar"),3.0);

   pStrSave->put_Property(_T("TopCover"),CComVariant(TopCover));
   pStrSave->put_Property(_T("TopLumpSumArea"),CComVariant(TopLumpSum));
   pStrSave->put_Property(_T("TopRebarType"),CComVariant(TopRebarType));
   pStrSave->put_Property(_T("TopRebarGrade"),CComVariant(TopRebarGrade));
   pStrSave->put_Property(_T("TopRebarSize"),CComVariant(TopRebarSize));
   pStrSave->put_Property(_T("TopSpacing"),CComVariant(TopSpacing));

   pStrSave->put_Property(_T("BottomCover"),CComVariant(BottomCover));
   pStrSave->put_Property(_T("BottomLumpSumArea"),CComVariant(BottomLumpSum));
   pStrSave->put_Property(_T("BottomRebarType"),CComVariant(BottomRebarType));
   pStrSave->put_Property(_T("BottomRebarGrade"),CComVariant(BottomRebarGrade));
   pStrSave->put_Property(_T("BottomRebarSize"),CComVariant(BottomRebarSize));
   pStrSave->put_Property(_T("BottomSpacing"),CComVariant(BottomSpacing));

   pStrSave->put_Property(_T("NegMomentCount"),CComVariant((long)NegMomentRebar.size()));
   std::vector<NegMomentRebarData>::iterator iter;
   for ( iter = NegMomentRebar.begin(); iter != NegMomentRebar.end(); iter++ )
   {
      NegMomentRebarData& rebar = *iter;
      pStrSave->BeginUnit(_T("NegMomentRebar"),1.0);

      pStrSave->put_Property(_T("Pier"),CComVariant(rebar.PierIdx));
      pStrSave->put_Property(_T("Mat"),CComVariant(rebar.Mat));
      pStrSave->put_Property(_T("LumpSumArea"),CComVariant(rebar.LumpSum));
      pStrSave->put_Property(_T("RebarType"),CComVariant(rebar.RebarType));
      pStrSave->put_Property(_T("RebarGrade"),CComVariant(rebar.RebarGrade));
      pStrSave->put_Property(_T("RebarSize"),CComVariant(rebar.RebarSize));
      pStrSave->put_Property(_T("Spacing"),CComVariant(rebar.Spacing));
      pStrSave->put_Property(_T("LeftCutoff"),CComVariant(rebar.LeftCutoff));
      pStrSave->put_Property(_T("RightCutoff"),CComVariant(rebar.RightCutoff));

      pStrSave->EndUnit();
   }

   pStrSave->EndUnit();

   return hr;
}

HRESULT CDeckRebarData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   HRESULT hr = S_OK;

   pStrLoad->BeginUnit(_T("DeckRebar"));

   CComVariant var;

   double version;
   pStrLoad->get_Version(&version);

   if ( 2.0 == version )
   {
      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("RebarType"),&var);
      std::_tstring strRebarMaterial = OLE2T(var.bstrVal);
   }

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("TopCover"),&var);
   TopCover = var.dblVal;

   pStrLoad->get_Property(_T("TopLumpSumArea"),&var);
   TopLumpSum = var.dblVal;

   if ( version < 3 )
   {
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("TopRebarKey"),&var);
      BarSizeType TopRebarKey = BarSizeType(var.lVal);
      lrfdRebarPool::MapOldRebarKey(TopRebarKey,TopRebarGrade,TopRebarType,TopRebarSize);
   }
   else
   {
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("TopRebarType"),&var);
      TopRebarType = matRebar::Type(var.lVal);

      pStrLoad->get_Property(_T("TopRebarGrade"),&var);
      TopRebarGrade = matRebar::Grade(var.lVal);

      pStrLoad->get_Property(_T("TopRebarSize"),&var);
      TopRebarSize = matRebar::Size(var.lVal);
   }

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("TopSpacing"),&var);
   TopSpacing = var.dblVal;

   pStrLoad->get_Property(_T("BottomCover"),&var);
   BottomCover = var.dblVal;

   pStrLoad->get_Property(_T("BottomLumpSumArea"),&var);
   BottomLumpSum = var.dblVal;


   if ( version < 3 )
   {
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("BottomRebarKey"),&var);
      BarSizeType BottomRebarKey = BarSizeType(var.lVal);
      lrfdRebarPool::MapOldRebarKey(BottomRebarKey,BottomRebarGrade,BottomRebarType,BottomRebarSize);
   }
   else
   {
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("BottomRebarType"),&var);
      BottomRebarType = matRebar::Type(var.lVal);

      pStrLoad->get_Property(_T("BottomRebarGrade"),&var);
      BottomRebarGrade = matRebar::Grade(var.lVal);

      pStrLoad->get_Property(_T("BottomRebarSize"),&var);
      BottomRebarSize = matRebar::Size(var.lVal);
   }

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("BottomSpacing"),&var);
   BottomSpacing = var.dblVal;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("NegMomentCount"),&var);
   long count = var.lVal;
   NegMomentRebar.clear();

   for ( long i = 0; i < count; i++ )
   {
      NegMomentRebarData rebar;
      pStrLoad->BeginUnit(_T("NegMomentRebar"));

      var.vt = VT_I2;
      pStrLoad->get_Property(_T("Pier"),&var);
      rebar.PierIdx = var.iVal;

      pStrLoad->get_Property(_T("Mat"),&var);
      rebar.Mat = (RebarMat)var.iVal;

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("LumpSumArea"),&var);
      rebar.LumpSum = var.dblVal;

      if ( version < 3 )
      {
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("RebarKey"),&var);
         BarSizeType RebarKey = BarSizeType(var.lVal);
         lrfdRebarPool::MapOldRebarKey(RebarKey,rebar.RebarGrade,rebar.RebarType,rebar.RebarSize);
      }
      else
      {
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("RebarType"),&var);
         rebar.RebarType = matRebar::Type(var.lVal);

         pStrLoad->get_Property(_T("RebarGrade"),&var);
         rebar.RebarGrade = matRebar::Grade(var.lVal);

         pStrLoad->get_Property(_T("RebarSize"),&var);
         rebar.RebarSize = matRebar::Size(var.lVal);
      }

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Spacing"),&var);
      rebar.Spacing = var.dblVal;

      pStrLoad->get_Property(_T("LeftCutoff"),&var);
      rebar.LeftCutoff = var.dblVal;

      pStrLoad->get_Property(_T("RightCutoff"),&var);
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

   TopRebarType = rOther.TopRebarType;
   TopRebarGrade = rOther.TopRebarGrade;
   TopRebarSize = rOther.TopRebarSize;

   BottomRebarType = rOther.BottomRebarType;
   BottomRebarGrade = rOther.BottomRebarGrade;
   BottomRebarSize = rOther.BottomRebarSize;

   TopSpacing    = rOther.TopSpacing;
   BottomSpacing = rOther.BottomSpacing;

   TopLumpSum    = rOther.TopLumpSum;
   BottomLumpSum = rOther.BottomLumpSum;

   NegMomentRebar = rOther.NegMomentRebar;
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

   if ( TopRebarType != rOther.TopRebarType )
      return false;

   if ( TopRebarGrade != rOther.TopRebarGrade )
      return false;

   if ( TopRebarSize != rOther.TopRebarSize )
      return false;


   if ( BottomRebarType != rOther.BottomRebarType )
      return false;

   if ( BottomRebarGrade != rOther.BottomRebarGrade )
      return false;

   if ( BottomRebarSize != rOther.BottomRebarSize )
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

   if ( RebarType != rOther.RebarType )
      return false;

   if ( RebarGrade != rOther.RebarGrade )
      return false;

   if ( RebarSize != rOther.RebarSize )
      return false;

   if ( Spacing != rOther.Spacing )
      return false;

   if ( LeftCutoff != rOther.LeftCutoff )
      return false;

   if ( RightCutoff != rOther.RightCutoff )
      return false;

   return true;
}
