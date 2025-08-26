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

#include "StdAfx.h"
#include <PsgLib\BearingData2.h>

CBearingData2::CBearingData2()
{ 
   Init();
}

void CBearingData2::Init()
{
    bNeedsDefaults = true;

    DefinitionType = btBasic;
    Shape = bsRectangular;
    Length = WBFL::Units::ConvertToSysUnits(11.0, WBFL::Units::Measure::Inch);
    Width = WBFL::Units::ConvertToSysUnits(27.0, WBFL::Units::Measure::Inch);
    BearingCount = 1;
    Spacing = 0.0;
    Height = 0.0;
    RecessHeight = 0.0;
    RecessLength = 0.0;
    SolePlateHeight = 0.0;

    ElastomerThickness = WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Inch);
    CoverThickness = WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch);
    ShimThickness = WBFL::Units::ConvertToSysUnits(0.0747, WBFL::Units::Measure::Inch);
    NumIntLayers = 2;
    UseExtPlates = false;
    FixedX = false;
    FixedY = false;

    //ShearDeformationOverride = 0.0;
}

Float64 CBearingData2::GetNetBearingHeight() const
{
   return Height - RecessHeight + SolePlateHeight;
}

HRESULT CBearingData2::Load(IStructuredLoad* pStrLoad, std::shared_ptr<IEAFProgress> pProgress)
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

      if (version > 1)
      {
          var.vt = VT_I4;
          hr = pStrLoad->get_Property(_T("DefinitionType"), &var);
          DefinitionType = (BearingDefinitionType)var.lVal;
          var.vt = VT_R8;
          hr = pStrLoad->get_Property(_T("ElastomerThickness"), &var);
          ElastomerThickness = var.dblVal;
          var.vt = VT_I4;
          hr = pStrLoad->get_Property(_T("NumIntLayers"), &var);
          NumIntLayers = var.lVal;
          var.vt = VT_R8;
          hr = pStrLoad->get_Property(_T("CoverThickness"), &var);
          CoverThickness = var.dblVal;
          var.vt = VT_R8;
          hr = pStrLoad->get_Property(_T("ShimThickness"), &var);
          ShimThickness = var.dblVal;
          var.vt = VT_BOOL;
          hr = pStrLoad->get_Property(_T("FixedX"), &var);
          FixedX = (var.boolVal == VARIANT_TRUE ? true : false);
          hr = pStrLoad->get_Property(_T("FixedY"), &var);
          FixedY = (var.boolVal == VARIANT_TRUE ? true : false);
          hr = pStrLoad->get_Property(_T("UsesExtPlates"), &var);
          UseExtPlates = (var.boolVal == VARIANT_TRUE ? true : false);
          hr = pStrLoad->get_Property(_T("ShearDeformationOverride"), &var);
          ShearDeformationOverride = var.dblVal;
      }

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

HRESULT CBearingData2::Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress) const
{
   pStrSave->BeginUnit(_T("BearingData2"),2.0);
   pStrSave->put_Property(_T("DefinitionType"), CComVariant(DefinitionType));
   pStrSave->put_Property(_T("ElastomerThickness"), CComVariant(ElastomerThickness));
   pStrSave->put_Property(_T("NumIntLayers"), CComVariant(NumIntLayers));
   pStrSave->put_Property(_T("CoverThickness"), CComVariant(CoverThickness));
   pStrSave->put_Property(_T("ShimThickness"), CComVariant(ShimThickness));
   pStrSave->put_Property(_T("FixedX"), CComVariant(FixedX));
   pStrSave->put_Property(_T("FixedY"), CComVariant(FixedY));
   pStrSave->put_Property(_T("UsesExtPlates"), CComVariant(UseExtPlates));
   pStrSave->put_Property(_T("ShearDeformationOverride"), CComVariant(ShearDeformationOverride));
   pStrSave->put_Property(_T("Shape"), CComVariant(Shape));
   pStrSave->put_Property(_T("Length"), CComVariant(Length));
   pStrSave->put_Property(_T("Width"), CComVariant(Width));
   pStrSave->put_Property(_T("BearingCount"),    CComVariant(BearingCount));
   pStrSave->put_Property(_T("Spacing"),    CComVariant(Spacing));
   pStrSave->put_Property(_T("Height"),    CComVariant(Height));
   pStrSave->put_Property(_T("RecessHeight"),    CComVariant(RecessHeight));
   pStrSave->put_Property(_T("RecessLength"),    CComVariant(RecessLength));
   pStrSave->put_Property(_T("SolePlateHeight"),    CComVariant(SolePlateHeight));
   pStrSave->EndUnit();

   return S_OK;
}

CBearingData2& CBearingData2::operator= (const CBearingData2& rOther)
{

    Length = rOther.Length;
    Width = rOther.Width;
    Height = rOther.Height;
    Shape = rOther.Shape;
    BearingCount = rOther.BearingCount;
    Spacing = rOther.Spacing;
    RecessHeight = rOther.RecessHeight;
    RecessLength = rOther.RecessLength;
    SolePlateHeight = rOther.SolePlateHeight;
    ElastomerThickness = rOther.ElastomerThickness;
    CoverThickness = rOther.CoverThickness;
    ShimThickness = rOther.ShimThickness;
    NumIntLayers = rOther.NumIntLayers;
    UseExtPlates = rOther.UseExtPlates;
    FixedX = rOther.FixedX;
    FixedY = rOther.FixedY;
    ShearDeformationOverride = rOther.ShearDeformationOverride;
    DefinitionType = rOther.DefinitionType;

    ATLASSERT(*this == rOther); // should be equal after assignment
    return *this;
}

bool CBearingData2::operator==(const CBearingData2& rOther) const
{
   if (Shape != rOther.Shape)
   {
       return false;
   }

   if (Shape == bsRectangular)
   {
      if (!IsEqual(Length, rOther.Length))
      {
          return false;
      }

      if (!IsEqual(Width, rOther.Width))
      {
          return false;
      }
   }
   else
   {
      if (!IsEqual(Length, rOther.Length))
      {
          return false;
      }
   }

   if (BearingCount != rOther.BearingCount)
   {
       return false;
   }

   if (BearingCount > 1)
   {
      if (!IsEqual(Spacing, rOther.Spacing))
      {
          return false;
      }
   }

   if (!IsEqual(Height, rOther.Height))
   {
       return false;
   }

   if (!IsEqual(RecessHeight, rOther.RecessHeight))
   {
       return false;
   }

   if (!IsEqual(RecessLength, rOther.RecessLength))
   {
       return false;
   }

   if (!IsEqual(SolePlateHeight, rOther.SolePlateHeight))
   {
       return false;
   }

   if (!IsEqual(ElastomerThickness, rOther.ElastomerThickness))
   {
       return false;
   }

   if (!IsEqual(CoverThickness, rOther.CoverThickness))
   {
       return false;
   }

   if (!IsEqual(ShimThickness, rOther.ShimThickness))
   {
       return false;
   }

   if (!IsEqual(NumIntLayers, rOther.NumIntLayers))
   {
       return false;
   }

   if (UseExtPlates != rOther.UseExtPlates)
   {
       return false;
   }

   if (FixedX != rOther.FixedX)
   {
       return false;
   }

   if (FixedY != rOther.FixedY)
   {
       return false;
   }

   if (!IsEqual(ShearDeformationOverride, rOther.ShearDeformationOverride))
   {
       return false;
   }

   if (DefinitionType != rOther.DefinitionType)
   {
       return false;
   }

   return true;
}

bool CBearingData2::operator!=(const CBearingData2& rOther) const
{
   return !CBearingData2::operator==(rOther);
}
