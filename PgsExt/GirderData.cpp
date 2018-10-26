///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\GirderData.h>
#include <Units\SysUnits.h>
#include <psgLib\StructuredSave.h>
#include <psgLib\StructuredLoad.h>
#include <StdIo.h>

#include <Lrfd\StrandPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderData
****************************************************************************/




////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderData::CGirderData()
{
   Condition = pgsTypes::cfGood;
   ConditionFactor = 1.0;
}  

CGirderData::CGirderData(const CGirderData& rOther)
{
   MakeCopy(rOther);
}

CGirderData::~CGirderData()
{
}


//======================== OPERATORS  =======================================
CGirderData& CGirderData::operator= (const CGirderData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGirderData::operator==(const CGirderData& rOther) const
{
   if ( PrestressData != rOther.PrestressData )
      return false;

   if ( ShearData != rOther.ShearData )
      return false;

   if ( LongitudinalRebarData != rOther.LongitudinalRebarData )
      return false;

   if ( HandlingData != rOther.HandlingData )
      return false;

   if ( Material != rOther.Material )
      return false;

   if ( Condition != rOther.Condition )
      return false;

   if ( !IsEqual(ConditionFactor,rOther.ConditionFactor) )
      return false;

   return true;
}

int CGirderData::GetChangeType(const CGirderData& rOther) const
{
   int ct = ctNone;

   // first check if prestressing changed
   if (PrestressData != rOther.PrestressData)
         ct |= ctPrestress;

   for ( int i = 0; i < 3; i++ )
   {
      if ( Material.pStrandMaterial[i] != rOther.Material.pStrandMaterial[i] )
      {
            ct |= ctStrand;
            break;
      }
   }

   if ( Material.Type != rOther.Material.Type ) 
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Fci,rOther.Material.Fci) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Fc,rOther.Material.Fc) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual( Material.WeightDensity, rOther.Material.WeightDensity ) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.StrengthDensity,rOther.Material.StrengthDensity) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.MaxAggregateSize,rOther.Material.MaxAggregateSize) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.EcK1,rOther.Material.EcK1) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.EcK2,rOther.Material.EcK2) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.CreepK1,rOther.Material.CreepK1) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.CreepK2,rOther.Material.CreepK2) )
   {
      ct |= ctConcrete;
   }
    else if ( !IsEqual(Material.ShrinkageK1,rOther.Material.ShrinkageK1) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.ShrinkageK2,rOther.Material.ShrinkageK2) )
   {
      ct |= ctConcrete;
   }
   else if ( Material.bUserEci != rOther.Material.bUserEci )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Eci,rOther.Material.Eci) )
   {
      ct |= ctConcrete;
   }
   else if ( Material.bUserEc != rOther.Material.bUserEc )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Ec,rOther.Material.Ec) )
   {
      ct |= ctConcrete;
   }
   else if ( Material.bHasFct != rOther.Material.bHasFct )
   {
      ct |= ctConcrete;
   }
   else if ( Material.bHasFct && !IsEqual(Material.Fct,rOther.Material.Fct) )
   {
      ct |= ctConcrete;
   }

   if ( !IsEqual(HandlingData.LeftLiftPoint,  rOther.HandlingData.LeftLiftPoint) ||
        !IsEqual(HandlingData.RightLiftPoint, rOther.HandlingData.RightLiftPoint) )
   {
      ct |= ctLifting;
   }


   if ( !IsEqual(HandlingData.LeadingSupportPoint,  rOther.HandlingData.LeadingSupportPoint) ||
        !IsEqual(HandlingData.TrailingSupportPoint, rOther.HandlingData.TrailingSupportPoint) )
   {
      ct |= ctShipping;
   }

   if ( Condition != rOther.Condition || !IsEqual(ConditionFactor,rOther.ConditionFactor) )
   {
      ct |= ctCondition;
   }

   if ( ShearData != rOther.ShearData)
   {
      ct |= ctShearData;
   }

   if ( LongitudinalRebarData != rOther.LongitudinalRebarData)
   {
      ct |= ctLongRebar;
   }

   return ct;
}



bool CGirderData::operator!=(const CGirderData& rOther) const
{
   return !operator==(rOther);
}

//======================== OPERATIONS =======================================
HRESULT CGirderData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress, 
                          Float64 fc,Float64 weightDensity,Float64 strengthDensity,Float64 maxAggSize)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   Float64 parentVersion;
   pStrLoad->get_ParentVersion(&parentVersion);

   CComBSTR bstrParentUnit;
   pStrLoad->get_ParentUnit(&bstrParentUnit);

   pStrLoad->BeginUnit(_T("PrestressData"));  // named this for historical reasons

   Float64 version;
   pStrLoad->get_Version(&version);

   // Prestressing
   hr = PrestressData.Load(pStrLoad);
   if (FAILED(hr))
      return hr;

   CComVariant var;

   if ( version < 9 )
   {
      Material.pStrandMaterial[pgsTypes::Straight]  = 0; // not used in pre-version 9 of this data block
      Material.pStrandMaterial[pgsTypes::Harped]    = 0; // not used in pre-version 9 of this data block
      Material.pStrandMaterial[pgsTypes::Temporary] = 0; // not used in pre-version 9 of this data block
      // the Project Agent will set this value later
   }
   else if ( version < 11 )
   {
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("StrandMaterialKey"),&var);
      Int32 key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);
      ATLASSERT(Material.pStrandMaterial[pgsTypes::Straight] != 0);
      Material.pStrandMaterial[pgsTypes::Harped] = Material.pStrandMaterial[pgsTypes::Straight];
      Material.pStrandMaterial[pgsTypes::Temporary] = Material.pStrandMaterial[pgsTypes::Straight];
   }
   else
   {
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("StraightStrandMaterialKey"),&var);
      Int32 key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);

      pStrLoad->get_Property(_T("HarpedStrandMaterialKey"),&var);
      key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Harped] = pPool->GetStrand(key);

      pStrLoad->get_Property(_T("TemporaryStrandMaterialKey"),&var);
      key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Temporary] = pPool->GetStrand(key);
   }

   if ( (bstrParentUnit == CComBSTR("Prestressing") && parentVersion < 4) ||
        (bstrParentUnit == CComBSTR("GirderTypes")  && parentVersion < 2) ||
        (bstrParentUnit == CComBSTR("Girders")      && parentVersion < 3) 
        )
   {
      var.Clear();
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Fci"), &var );
      Material.Fci = var.dblVal;

      if (3.0 <= version)
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Fc"), &var );
         Material.Fc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("WeightDensity"), &var );
         Material.WeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("StrengthDensity"), &var );
         Material.StrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("MaxAggregateSize"), &var );
         Material.MaxAggregateSize = var.dblVal;

      }
      else
      {
         Material.Fc               = fc;
         Material.WeightDensity    = weightDensity;
         Material.StrengthDensity  = strengthDensity;
         Material.MaxAggregateSize = maxAggSize;
      }

      if ( 4 <= version )
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("K1"), &var );
         Material.EcK1 = var.dblVal;
      }

      if ( 7 <= version )
      {
         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserEci"), &var );
         Material.bUserEci = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Eci"), &var);
         Material.Eci = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserEc"), &var );
         Material.bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Ec"), &var);
         Material.Ec = var.dblVal;
      }
   }

   pStrLoad->EndUnit(); // end PrestressData

   pStrLoad->get_Version(&version);

   if ( bstrParentUnit == CComBSTR("Girders") && 2.0 < version )
   {
      Material.Load(pStrLoad,pProgress);
   }

   if ( (bstrParentUnit == CComBSTR("Girders")     && 1.0 < version) ||
        (bstrParentUnit == CComBSTR("GirderTypes") && 1.0 < version)
      ) // if parent version greater than 1, then load shear and long rebar data
   {
      CStructuredLoad load( pStrLoad );
      ShearData.Load(&load);

      LongitudinalRebarData.Load(pStrLoad,pProgress);
      HandlingData.Load(pStrLoad,pProgress);
   }

   if ( SUCCEEDED(pStrLoad->BeginUnit(_T("Condition"))) )
   {
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("ConditionFactorType"),&var);
      Condition = (pgsTypes::ConditionFactorType)(var.lVal);

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("ConditionFactor"),&var);
      ConditionFactor = var.dblVal;
   
      pStrLoad->EndUnit();
   }

   return hr;
}

HRESULT CGirderData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PrestressData"),14.0);

   // Version 13 moved prestressed data into a separate class and added direct strand fill
   hr = PrestressData.Save(pStrSave);
   if (FAILED(hr))
      return hr;

   ///////////////// Added with data block version 11
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int32 key = pPool->GetStrandKey(Material.pStrandMaterial[pgsTypes::Straight]);
   pStrSave->put_Property(_T("StraightStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(Material.pStrandMaterial[pgsTypes::Harped]);
   pStrSave->put_Property(_T("HarpedStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(Material.pStrandMaterial[pgsTypes::Temporary]);
   pStrSave->put_Property(_T("TemporaryStrandMaterialKey"),CComVariant(key));

   // moved out of this data block in version 10 for this data block and version 3 of parent
   //pStrSave->put_Property(_T("Fci"), CComVariant(Material.Fci ));
   //pStrSave->put_Property(_T("Fc"),               CComVariant(Material.Fc));
   //pStrSave->put_Property(_T("WeightDensity"),    CComVariant(Material.WeightDensity));
   //pStrSave->put_Property(_T("StrengthDensity"),  CComVariant(Material.StrengthDensity));
   //pStrSave->put_Property(_T("MaxAggregateSize"), CComVariant(Material.MaxAggregateSize));
   //pStrSave->put_Property(_T("K1"),               CComVariant(Material.K1));
   //pStrSave->put_Property(_T("UserEci"),          CComVariant(Material.bUserEci));
   //pStrSave->put_Property(_T("Eci"),              CComVariant(Material.Eci));
   //pStrSave->put_Property(_T("UserEc"),           CComVariant(Material.bUserEc));
   //pStrSave->put_Property(_T("Ec"),               CComVariant(Material.Ec));

   pStrSave->EndUnit(); // PrestressData

   // Moved here with version 3 of parent data block
   Material.Save(pStrSave,pProgress);

   CStructuredSave save( pStrSave );
   ShearData.Save(&save);

   LongitudinalRebarData.Save(pStrSave,pProgress);
   HandlingData.Save(pStrSave,pProgress);

   pStrSave->BeginUnit(_T("Condition"),1.0);
   pStrSave->put_Property(_T("ConditionFactorType"),CComVariant(Condition));
   pStrSave->put_Property(_T("ConditionFactor"),CComVariant(ConditionFactor));
   pStrSave->EndUnit();

   return hr;
}


//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================


////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderData::MakeCopy(const CGirderData& rOther)
{
   CopyPrestressingFrom(rOther);
   CopyMaterialFrom(rOther);
   CopyShearDataFrom(rOther);
   CopyLongitudinalRebarFrom(rOther);
   CopyHandlingDataFrom(rOther);

   Condition = rOther.Condition;
   ConditionFactor = rOther.ConditionFactor;
}

void CGirderData::CopyPrestressingFrom(const CGirderData& rOther)
{
   PrestressData = rOther.PrestressData;

   Material.pStrandMaterial[pgsTypes::Straight]  = rOther.Material.pStrandMaterial[pgsTypes::Straight];
   Material.pStrandMaterial[pgsTypes::Harped]    = rOther.Material.pStrandMaterial[pgsTypes::Harped];
   Material.pStrandMaterial[pgsTypes::Temporary] = rOther.Material.pStrandMaterial[pgsTypes::Temporary];
}

void CGirderData::CopyShearDataFrom(const CGirderData& rOther)
{
   ShearData = rOther.ShearData;
}

void CGirderData::CopyLongitudinalRebarFrom(const CGirderData& rOther)
{
   LongitudinalRebarData = rOther.LongitudinalRebarData;
}

void CGirderData::CopyHandlingDataFrom(const CGirderData& rOther)
{
   HandlingData = rOther.HandlingData;
}

void CGirderData::CopyMaterialFrom(const CGirderData& rOther)
{
   Material.Type              = rOther.Material.Type;
   Material.Fci               = rOther.Material.Fci;
   Material.Fc                = rOther.Material.Fc;
   Material.WeightDensity     = rOther.Material.WeightDensity;
   Material.StrengthDensity   = rOther.Material.StrengthDensity;
   Material.MaxAggregateSize  = rOther.Material.MaxAggregateSize;
   Material.EcK1              = rOther.Material.EcK1;
   Material.EcK2              = rOther.Material.EcK2;
   Material.CreepK1           = rOther.Material.CreepK1;
   Material.CreepK2           = rOther.Material.CreepK2;
   Material.ShrinkageK1       = rOther.Material.ShrinkageK1;
   Material.ShrinkageK2       = rOther.Material.ShrinkageK2;
   Material.bUserEci          = rOther.Material.bUserEci;
   Material.Eci               = rOther.Material.Eci;
   Material.bUserEc           = rOther.Material.bUserEc;
   Material.Ec                = rOther.Material.Ec;
   Material.bHasFct           = rOther.Material.bHasFct;
   Material.Fct               = rOther.Material.Fct;
}


void CGirderData::MakeAssignment(const CGirderData& rOther)
{
   MakeCopy( rOther );
}
