///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <StdIo.h>

#include <psgLib\StructuredSave.h>
#include <PsgLib\StructuredLoad.h>

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
   m_pLibraryEntry = nullptr;

   Condition = pgsTypes::cfGood;
   ConditionFactor = 1.0;

   m_bUsedShearData2 = false;
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
   if ( m_GirderName != rOther.m_GirderName )
   {
      return false;
   }

   if ( Strands != rOther.Strands )
   {
      return false;
   }

   if ( Material != rOther.Material )
   {
      return false;
   }

   if ( ShearData != rOther.ShearData )
   {
      return false;
   }

   if ( LongitudinalRebarData != rOther.LongitudinalRebarData )
   {
      return false;
   }

   if ( HandlingData != rOther.HandlingData )
   {
      return false;
   }

   if ( Condition != rOther.Condition )
   {
      return false;
   }

   if ( !IsEqual(ConditionFactor,rOther.ConditionFactor) )
   {
      return false;
   }

   return true;
}

int CGirderData::GetChangeType(const CGirderData& rOther) const
{
   int ct = ctNone;

   if (Strands != rOther.Strands)
   {
         ct |= ctPrestress;
   }

   for ( Uint16 i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);
      if ( Strands.GetStrandMaterial(strandType) != rOther.Strands.GetStrandMaterial(strandType) )
      {
            ct |= ctStrand;
      }
   }

   if ( Material.Concrete != rOther.Material.Concrete )
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

   return ct;
}



bool CGirderData::operator!=(const CGirderData& rOther) const
{
   return !operator==(rOther);
}

void CGirderData::CopyPrestressingFrom(const CGirderData& rOther)
{
   Strands = rOther.Strands;
}

void CGirderData::CopyMaterialFrom(const CGirderData& rOther)
{
   Material = rOther.Material;
}

//======================== OPERATIONS =======================================
HRESULT CGirderData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress, 
                          Float64 fc,Float64 weightDensity,Float64 strengthDensity,Float64 maxAggSize)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   CComVariant var;

   HRESULT hr_girderDataUnit = pStrLoad->BeginUnit(_T("GirderData"));

   Float64 version;
   pStrLoad->get_Version(&version);
   if ( 4 < version )
   {
      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("Girder"),&var); // added version 5
      m_GirderName = OLE2T(var.bstrVal);
   }

   Float64 parentVersion;
   pStrLoad->get_ParentVersion(&parentVersion);

   Float64 strand_data_version;
   Strands.Load(pStrLoad,pProgress,&strand_data_version);

   if ( version < 3 )
   {
      // stored here for version == 1 or 2
      var.Clear();
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Fci"), &var );
      Material.Concrete.Fci = var.dblVal;

      if (3.0 <= strand_data_version)
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Fc"), &var );
         Material.Concrete.Fc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("WeightDensity"), &var );
         Material.Concrete.WeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("StrengthDensity"), &var );
         Material.Concrete.StrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("MaxAggregateSize"), &var );
         Material.Concrete.MaxAggregateSize = var.dblVal;

      }
      else
      {
         Material.Concrete.Fc               = fc;
         Material.Concrete.WeightDensity    = weightDensity;
         Material.Concrete.StrengthDensity  = strengthDensity;
         Material.Concrete.MaxAggregateSize = maxAggSize;
      }

      if ( 4 <= strand_data_version )
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("K1"), &var );
         Material.Concrete.EcK1 = var.dblVal;
      }

      if ( 7 <= strand_data_version )
      {
         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserEci"), &var );
         Material.Concrete.bUserEci = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Eci"), &var);
         Material.Concrete.Eci = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserEc"), &var );
         Material.Concrete.bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Ec"), &var);
         Material.Concrete.Ec = var.dblVal;
      }

      pStrLoad->EndUnit(); // ends StrandData unit
   }

   if ( 2.0 < version )
   {
      // Don't load with the Concrete object. It has a different data structure.
      // Load the data from this old format and then populate the Concrete object with data.
      pStrLoad->BeginUnit(_T("Concrete"));

      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("Type"),&var);
      Material.Concrete.Type = (pgsTypes::ConcreteType)lrfdConcreteUtil::GetTypeFromName(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Fci"), &var);
      Material.Concrete.Fci = var.dblVal;

      pStrLoad->get_Property(_T("Fc"),&var);
      Material.Concrete.Fc = var.dblVal;

      pStrLoad->get_Property(_T("WeightDensity"), &var);
      Material.Concrete.WeightDensity = var.dblVal;

      pStrLoad->get_Property(_T("StrengthDensity"),  &var);
      Material.Concrete.StrengthDensity = var.dblVal;

      pStrLoad->get_Property(_T("MaxAggregateSize"), &var);
      Material.Concrete.MaxAggregateSize = var.dblVal;

      pStrLoad->get_Property(_T("EcK1"),&var);
      Material.Concrete.EcK1 = var.dblVal;

      pStrLoad->get_Property(_T("EcK2"),&var);
      Material.Concrete.EcK2 = var.dblVal;

      pStrLoad->get_Property(_T("CreepK1"),&var);
      Material.Concrete.CreepK1 = var.dblVal;

      pStrLoad->get_Property(_T("CreepK2"),&var);
      Material.Concrete.CreepK2 = var.dblVal;

      pStrLoad->get_Property(_T("ShrinkageK1"),&var);
      Material.Concrete.ShrinkageK1 = var.dblVal;

      pStrLoad->get_Property(_T("ShrinkageK2"),&var);
      Material.Concrete.ShrinkageK2 = var.dblVal;

      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("UserEci"),&var);
      Material.Concrete.bUserEci = (var.boolVal == VARIANT_TRUE ? true : false);
      
      if ( Material.Concrete.bUserEci )
      {
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Eci"),&var);
         Material.Concrete.Eci = var.dblVal;
      }

      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("UserEc"),&var);
      Material.Concrete.bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( Material.Concrete.bUserEc )
      {
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Ec"),&var);
         Material.Concrete.Ec = var.dblVal;
      }

      if ( Material.Concrete.Type != pgsTypes::Normal )
      {
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("HasFct"),&var);
         Material.Concrete.bHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( Material.Concrete.bHasFct )
         {
            var.vt = VT_R8;
            pStrLoad->get_Property(_T("Fct"),&var);
            Material.Concrete.Fct = var.dblVal;
         }
      }


      pStrLoad->EndUnit(); // Concrete
   }

   if ( 1.0 < version ) 
   {
      if ( version < 4 )
      {
         CStructuredLoad load( pStrLoad );
         ShearData.Load(&load);
         m_bUsedShearData2 = false;
      }
      else
      {
         CStructuredLoad load(pStrLoad);
         ShearData2.Load(&load);
         m_bUsedShearData2 = true;
      }
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

   if ( SUCCEEDED(hr_girderDataUnit) )
   {
      pStrLoad->EndUnit(); // GirderData
   }

   return hr;
}

HRESULT CGirderData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   ATLASSERT(false); // should never get here in PGSuper version 3 (PGSplice)

   pStrSave->BeginUnit(_T("GirderData"),5.0);

   pStrSave->put_Property(_T("Girder"),CComVariant(m_GirderName.c_str())); // added version 5

   Strands.Save(pStrSave,pProgress);

   // Moved here with version 3 of parent data block
   Material.Concrete.Save(pStrSave,pProgress);

   CStructuredSave save( pStrSave );
   ShearData.Save(&save);
   LongitudinalRebarData.Save(pStrSave,pProgress);
   HandlingData.Save(pStrSave,pProgress);

   pStrSave->BeginUnit(_T("Condition"),1.0);
   pStrSave->put_Property(_T("ConditionFactorType"),CComVariant(Condition));
   pStrSave->put_Property(_T("ConditionFactor"),CComVariant(ConditionFactor));
   pStrSave->EndUnit();

   pStrSave->EndUnit(); // GirderData

   return hr;
}

void CGirderData::MakeCopy(const CGirderData& rOther)
{
   Strands = rOther.Strands;

   m_bUsedShearData2     = rOther.m_bUsedShearData2;
   ShearData             = rOther.ShearData;
   ShearData2            = rOther.ShearData2;
   LongitudinalRebarData = rOther.LongitudinalRebarData;
   HandlingData          = rOther.HandlingData;

   Material = rOther.Material;

   Condition = rOther.Condition;
   ConditionFactor = rOther.ConditionFactor;

   m_GirderName = rOther.m_GirderName;
   m_pLibraryEntry = rOther.m_pLibraryEntry;
}

void CGirderData::MakeAssignment(const CGirderData& rOther)
{
   MakeCopy( rOther );
}

void CGirderData::SetGirderName(LPCTSTR strName)
{
   if ( m_GirderName != strName )
   {
      Strands.ResetPrestressData();
   }

   m_GirderName = strName;
}

LPCTSTR CGirderData::GetGirderName() const
{
   return m_GirderName.c_str();
}

void CGirderData::SetGirderLibraryEntry(const GirderLibraryEntry* pEntry)
{
   m_pLibraryEntry = pEntry;
}

const GirderLibraryEntry* CGirderData::GetGirderLibraryEntry() const
{
   return m_pLibraryEntry;
}
