///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <PgsExt\ConcreteMaterial.h>
#include <Units\SysUnits.h>

#include <StdIo.h>

#include <Material\Concrete.h>
#include <Material\ACI209Concrete.h>
#include <Material\CEBFIPConcrete.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CConcreteMaterial
****************************************************************************/

// Make conversions static so they are only done once
static const Float64 gs_Fci    = ::ConvertToSysUnits( 4.0, unitMeasure::KSI );
static const Float64 gs_Fc     = ::ConvertToSysUnits(5.0,unitMeasure::KSI);
static const Float64 gs_StrengthDensity = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
static const Float64 gs_WeightDensity = gs_StrengthDensity;
static const Float64 gs_MaxAggregateSize = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
static const Float64 gs_Eci = ::ConvertToSysUnits(4200., unitMeasure::KSI);
static const Float64 gs_Ec = ::ConvertToSysUnits(4700., unitMeasure::KSI);


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConcreteMaterial::CConcreteMaterial()
{
   // Default material properties
   Type = pgsTypes::Normal;
   bHasInitial = true;
   Fci = gs_Fci;
   Fc  = gs_Fc;
   StrengthDensity = gs_StrengthDensity;
   WeightDensity = gs_WeightDensity;
   MaxAggregateSize = gs_MaxAggregateSize;
   EcK1 = 1.0;
   EcK2 = 1.0;
   CreepK1 = 1.0;
   CreepK2 = 1.0;
   ShrinkageK1 = 1.0;
   ShrinkageK2 = 1.0;
   bUserEci = false;
   Eci = gs_Eci;
   bUserEc = false;
   Ec = gs_Ec;
   bHasFct = false;
   Fct = 0;

   bBasePropertiesOnInitialValues = false;

   bACIUserParameters = false;
   CureMethod = pgsTypes::Moist;
   ACI209CementType = pgsTypes::TypeI;
   matACI209Concrete::GetModelParameters((matACI209Concrete::CureMethod)CureMethod,(matACI209Concrete::CementType)ACI209CementType,&A,&B);

   bCEBFIPUserParameters = false;
   CEBFIPCementType = pgsTypes::N;
   matCEBFIPConcrete::GetModelParameters((matCEBFIPConcrete::CementType)CEBFIPCementType,&S,&BetaSc);
}  

CConcreteMaterial::CConcreteMaterial(const CConcreteMaterial& rOther)
{
   MakeCopy(rOther);
}

CConcreteMaterial::~CConcreteMaterial()
{
}

//======================== OPERATORS  =======================================
CConcreteMaterial& CConcreteMaterial::operator= (const CConcreteMaterial& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CConcreteMaterial::operator==(const CConcreteMaterial& rOther) const
{
   if ( Type != rOther.Type )
   {
      return false;
   }

   if ( Fc != rOther.Fc )
   {
      return false;
   }

   if ( bUserEc != rOther.bUserEc )
   {
      return false;
   }

   if ( bUserEc && !IsEqual(Ec,rOther.Ec) )
   {
      return false;
   }

   if ( bHasInitial != rOther.bHasInitial )
   {
      return false;
   }

   if ( bHasInitial )
   {
      if ( Fci != rOther.Fci )
      {
         return false;
      }

      if ( bUserEci != rOther.bUserEci )
      {
         return false;
      }

      if ( bUserEci && !IsEqual(Eci,rOther.Eci) )
      {
         return false;
      }
   }

   if ( !IsEqual( WeightDensity, rOther.WeightDensity ) )
   {
      return false;
   }

   if ( StrengthDensity != rOther.StrengthDensity )
   {
      return false;
   }

   if ( MaxAggregateSize != rOther.MaxAggregateSize )
   {
      return false;
   }

   if ( EcK1 != rOther.EcK1 )
   {
      return false;
   }

   if ( EcK2 != rOther.EcK2 )
   {
      return false;
   }

   if ( CreepK1 != rOther.CreepK1 )
   {
      return false;
   }

   if ( CreepK2 != rOther.CreepK2 )
   {
      return false;
   }

   if ( ShrinkageK1 != rOther.ShrinkageK1 )
   {
      return false;
   }

   if ( ShrinkageK2 != rOther.ShrinkageK2 )
   {
      return false;
   }

   if ( bHasFct != rOther.bHasFct )
   {
      return false;
   }

   if ( bHasFct && !IsEqual(Fct,rOther.Fct) )
   {
      return false;
   }

   if ( bBasePropertiesOnInitialValues != rOther.bBasePropertiesOnInitialValues )
   {
      return false;
   }

   if ( bACIUserParameters != rOther.bACIUserParameters )
   {
      return false;
   }

   if ( bACIUserParameters )
   {
      if ( !IsEqual(A,rOther.A) )
      {
         return false;
      }

      if ( !IsEqual(B,rOther.B) )
      {
         return false;
      }
   }
   else
   {
      if ( CureMethod != rOther.CureMethod )
      {
         return false;
      }

      if ( ACI209CementType != rOther.ACI209CementType )
      {
         return false;
      }
   }

   if ( bCEBFIPUserParameters )
   {
      if ( !IsEqual(S,rOther.S) )
      {
         return false;
      }

      if ( !IsEqual(BetaSc,rOther.BetaSc) )
      {
         return false;
      }
   }
   else
   {
      if ( CEBFIPCementType != rOther.CEBFIPCementType )
      {
         return false;
      }
   }

   if ( CEBFIPCementType != rOther.CEBFIPCementType )
   {
      return false;
   }


   return true;
}

bool CConcreteMaterial::operator!=(const CConcreteMaterial& rOther) const
{
   return !operator==(rOther);
}

void CConcreteMaterial::MakeCopy(const CConcreteMaterial& rOther)
{
   Type              = rOther.Type;
   Fc                = rOther.Fc;
   bUserEc           = rOther.bUserEc;
   Ec                = rOther.Ec;
   WeightDensity     = rOther.WeightDensity;
   StrengthDensity   = rOther.StrengthDensity;
   MaxAggregateSize  = rOther.MaxAggregateSize;

   bHasInitial       = rOther.bHasInitial;
   Fci               = rOther.Fci;
   bUserEci          = rOther.bUserEci;
   Eci               = rOther.Eci;

   // AASHTO Parameters
   EcK1              = rOther.EcK1;
   EcK2              = rOther.EcK2;
   CreepK1           = rOther.CreepK1;
   CreepK2           = rOther.CreepK2;
   ShrinkageK1       = rOther.ShrinkageK1;
   ShrinkageK2       = rOther.ShrinkageK2;
   bHasFct           = rOther.bHasFct;
   Fct               = rOther.Fct;

   bBasePropertiesOnInitialValues = rOther.bBasePropertiesOnInitialValues;

   // ACI Parameters
   bACIUserParameters = rOther.bACIUserParameters;
   A                  = rOther.A;
   B                  = rOther.B;
   CureMethod         = rOther.CureMethod;
   ACI209CementType   = rOther.ACI209CementType;

   // CEB-FIP Parameters
   bCEBFIPUserParameters = rOther.bCEBFIPUserParameters;
   S                     = rOther.S;
   BetaSc                = rOther.BetaSc;
   CEBFIPCementType      = rOther.CEBFIPCementType;
}


void CConcreteMaterial::MakeAssignment(const CConcreteMaterial& rOther)
{
   MakeCopy( rOther );
}

HRESULT CConcreteMaterial::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("Concrete"),2.0);
   pStrSave->put_Property(_T("Type"),             CComVariant( lrfdConcreteUtil::GetTypeName((matConcrete::Type)Type,false).c_str() ));
   pStrSave->put_Property(_T("Fc"),               CComVariant(Fc));

   pStrSave->put_Property(_T("UserEc"),           CComVariant(bUserEc));

   if ( bUserEc )
   {
      pStrSave->put_Property(_T("Ec"),               CComVariant(Ec));
   }

   pStrSave->put_Property(_T("WeightDensity"),    CComVariant(WeightDensity));
   pStrSave->put_Property(_T("StrengthDensity"),  CComVariant(StrengthDensity));
   pStrSave->put_Property(_T("MaxAggregateSize"), CComVariant(MaxAggregateSize));

   pStrSave->put_Property(_T("InitialParameters"),CComVariant(bHasInitial));
   if ( bHasInitial )
   {
      pStrSave->put_Property(_T("Fci"),              CComVariant(Fci ));
      pStrSave->put_Property(_T("UserEci"),          CComVariant(bUserEci));
      
      if ( bUserEci )
      {
         pStrSave->put_Property(_T("Eci"),              CComVariant(Eci));
      }
   }


   // AASHTO Properties
   pStrSave->BeginUnit(_T("AASHTO"),1.0);
   pStrSave->put_Property(_T("EcK1"),             CComVariant(EcK1));
   pStrSave->put_Property(_T("EcK2"),             CComVariant(EcK2));
   pStrSave->put_Property(_T("CreepK1"),          CComVariant(CreepK1));
   pStrSave->put_Property(_T("CreepK2"),          CComVariant(CreepK2));
   pStrSave->put_Property(_T("ShrinkageK1"),      CComVariant(ShrinkageK1));
   pStrSave->put_Property(_T("ShrinkageK2"),      CComVariant(ShrinkageK2));

   if ( Type != pgsTypes::Normal )
   {
      pStrSave->put_Property(_T("HasFct"),CComVariant(bHasFct));
      if ( bHasFct )
      {
         pStrSave->put_Property(_T("Fct"),CComVariant(Fct));
      }
   }
   pStrSave->EndUnit(); // AASHTO

   pStrSave->put_Property(_T("BasePropertiesOnInitialValues"),CComVariant(bBasePropertiesOnInitialValues));

   // ACI
   pStrSave->BeginUnit(_T("ACI"),1.0);
   pStrSave->put_Property(_T("UserProperties"),CComVariant(bACIUserParameters));
   pStrSave->put_Property(_T("A"),CComVariant(A));
   pStrSave->put_Property(_T("B"),CComVariant(B));
   pStrSave->put_Property(_T("CureMethod"),CComVariant(CureMethod));
   pStrSave->put_Property(_T("CementType"),CComVariant(ACI209CementType));
   pStrSave->EndUnit(); // ACI

   // CEB-FIP (added in version 2)
   pStrSave->BeginUnit(_T("CEBFIP"),2.0);
   pStrSave->put_Property(_T("UserProperties"),CComVariant(bCEBFIPUserParameters)); // added in version 2
   pStrSave->put_Property(_T("S"),CComVariant(S)); // added in version 2
   pStrSave->put_Property(_T("BetaSc"),CComVariant(BetaSc)); // added in version 2
   pStrSave->put_Property(_T("CementType"),CComVariant(CEBFIPCementType));
   pStrSave->EndUnit(); // CEBFIP

   pStrSave->EndUnit(); // concrete

   return S_OK;
}

HRESULT CConcreteMaterial::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CComVariant var;
   USES_CONVERSION;

   pStrLoad->BeginUnit(_T("Concrete"));

   Float64 version;
   pStrLoad->get_Version(&version);

   var.vt = VT_BSTR;
   pStrLoad->get_Property(_T("Type"),&var);
   Type = (pgsTypes::ConcreteType)lrfdConcreteUtil::GetTypeFromTypeName(OLE2T(var.bstrVal));

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Fc"),&var);
   Fc = var.dblVal;

   var.vt = VT_BOOL;
   pStrLoad->get_Property(_T("UserEc"),&var);
   bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

   if ( bUserEc )
   {
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Ec"),&var);
      Ec = var.dblVal;
   }

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("WeightDensity"), &var);
   WeightDensity = var.dblVal;

   pStrLoad->get_Property(_T("StrengthDensity"),  &var);
   StrengthDensity = var.dblVal;

   pStrLoad->get_Property(_T("MaxAggregateSize"), &var);
   MaxAggregateSize = var.dblVal;

   var.vt = VT_BOOL;
   pStrLoad->get_Property(_T("InitialParameters"),&var);
   bHasInitial = (var.boolVal == VARIANT_TRUE ? true : false);

   if ( bHasInitial )
   {
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Fci"), &var);
      Fci = var.dblVal;

      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("UserEci"),&var);
      bUserEci = (var.boolVal == VARIANT_TRUE ? true : false);
      
      if ( bUserEci )
      {
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Eci"),&var);
         Eci = var.dblVal;
      }
   }


   // AASHTO
   pStrLoad->BeginUnit(_T("AASHTO"));

   var.vt = VT_R8;

   pStrLoad->get_Property(_T("EcK1"),&var);
   EcK1 = var.dblVal;

   pStrLoad->get_Property(_T("EcK2"),&var);
   EcK2 = var.dblVal;

   pStrLoad->get_Property(_T("CreepK1"),&var);
   CreepK1 = var.dblVal;

   pStrLoad->get_Property(_T("CreepK2"),&var);
   CreepK2 = var.dblVal;

   pStrLoad->get_Property(_T("ShrinkageK1"),&var);
   ShrinkageK1 = var.dblVal;

   pStrLoad->get_Property(_T("ShrinkageK2"),&var);
   ShrinkageK2 = var.dblVal;

   if ( Type != pgsTypes::Normal )
   {
      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("HasFct"),&var);
      bHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( bHasFct )
      {
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Fct"),&var);
         Fct = var.dblVal;
      }
   }

   pStrLoad->EndUnit(); // AASHTO

   var.vt = VT_BOOL;
   pStrLoad->get_Property(_T("BasePropertiesOnInitialValues"),&var);
   bBasePropertiesOnInitialValues = (var.boolVal == VARIANT_TRUE ? true : false);

   // ACI
   pStrLoad->BeginUnit(_T("ACI"));
   var.vt = VT_BOOL;
   pStrLoad->get_Property(_T("UserProperties"),&var);
   bACIUserParameters = (var.boolVal == VARIANT_TRUE ? true : false);

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("A"),&var);
   A = var.dblVal;

   pStrLoad->get_Property(_T("B"),&var);
   B = var.dblVal;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("CureMethod"),&var);
   CureMethod = (pgsTypes::CureMethod)var.iVal;

   pStrLoad->get_Property(_T("CementType"),&var);
   ACI209CementType = (pgsTypes::ACI209CementType)var.iVal;

   pStrLoad->EndUnit(); // ACI

   // CEB-FIP (added in version 2)
   if ( 1.0 < version )
   {
      pStrLoad->BeginUnit(_T("CEBFIP"));
      Float64 cebFipVersion;
      pStrLoad->get_Version(&cebFipVersion);
      if ( 1 < cebFipVersion )
      {
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserProperties"),&var);
         bCEBFIPUserParameters = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("S"),&var);
         S = var.dblVal;

         pStrLoad->get_Property(_T("BetaSc"),&var);
         BetaSc = var.dblVal;
      }

      var.vt = VT_I4;
      pStrLoad->get_Property(_T("CementType"),&var);
      CEBFIPCementType = (pgsTypes::CEBFIPCementType)var.iVal;

      if ( cebFipVersion < 2 )
      {
         matCEBFIPConcrete::GetModelParameters((matCEBFIPConcrete::CementType)CEBFIPCementType,&S,&BetaSc);
      }

      pStrLoad->EndUnit(); // CEBFIP
   }

   pStrLoad->EndUnit(); // concrete

   return S_OK;
}

#if defined _DEBUG
void CConcreteMaterial::AssertValid()
{
}
#endif
