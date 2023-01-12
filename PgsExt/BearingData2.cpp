///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <PgsExt\BearingData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBearingData2::CBearingData2()
{ 
   Init();
}

void CBearingData2::Init()
{
   bNeedsDefaults = true;

   Shape = bsRectangular;
   Length =  WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);
   Width  = Length;
   BearingCount = 1;
   Spacing = 0.0;
   Height  = 0.0;
   RecessHeight = 0.0;
   RecessLength = 0.0;
   SolePlateHeight = 0.0;
}

Float64 CBearingData2::GetNetBearingHeight() const
{
   return Height - RecessHeight + SolePlateHeight;
}

HRESULT CBearingData2::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   // If we are loading, we have been intialized at some time in the past
   bNeedsDefaults = false;

   try
   {
      hr = pStrLoad->BeginUnit(_T("BearingData2"));

      Float64 version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("Shape"), &var);
      Shape = (BearingShape)var.lVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Length"), &var);
      Length = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Width"), &var);
      Width = var.dblVal;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("BearingCount"), &var);
      BearingCount = var.lVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Spacing"), &var);
      Spacing = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Height"), &var);
      Height = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("RecessHeight"), &var);
      RecessHeight = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("RecessLength"), &var);
      RecessLength = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SolePlateHeight"), &var);
      SolePlateHeight = var.dblVal;

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      return STRLOAD_E_INVALIDFORMAT;
   }

   return S_OK;
}

HRESULT CBearingData2::Save(IStructuredSave* pStrSave,IProgress* pProgress) const
{
   pStrSave->BeginUnit(_T("BearingData2"),1.0);
   pStrSave->put_Property(_T("Shape"),    CComVariant(Shape));
   pStrSave->put_Property(_T("Length"),    CComVariant(Length));
   pStrSave->put_Property(_T("Width"),    CComVariant(Width));
   pStrSave->put_Property(_T("BearingCount"),    CComVariant(BearingCount));
   pStrSave->put_Property(_T("Spacing"),    CComVariant(Spacing));
   pStrSave->put_Property(_T("Height"),    CComVariant(Height));
   pStrSave->put_Property(_T("RecessHeight"),    CComVariant(RecessHeight));
   pStrSave->put_Property(_T("RecessLength"),    CComVariant(RecessLength));
   pStrSave->put_Property(_T("SolePlateHeight"),    CComVariant(SolePlateHeight));
   pStrSave->EndUnit();

   return S_OK;
}

bool CBearingData2::operator==(const CBearingData2& rOther) const
{
   if (Shape != rOther.Shape)
      return false;

   if (Shape == bsRectangular)
   {
      if (!IsEqual(Length, rOther.Length))
         return false;

      if (!IsEqual(Width, rOther.Width))
         return false;
   }
   else
   {
      if (!IsEqual(Length, rOther.Length))
         return false;
   }

   if (BearingCount != rOther.BearingCount)
      return false;

   if (BearingCount > 1)
   {
      if (!IsEqual(Spacing, rOther.Spacing))
         return false;
   }

   if (!IsEqual(Height, rOther.Height))
      return false;

   if (!IsEqual(RecessHeight, rOther.RecessHeight))
      return false;

   if (!IsEqual(RecessLength, rOther.RecessLength))
      return false;

   if (!IsEqual(SolePlateHeight, rOther.SolePlateHeight))
      return false;

   return true;
}

bool CBearingData2::operator!=(const CBearingData2& rOther) const
{
   return !CBearingData2::operator==(rOther);
}
