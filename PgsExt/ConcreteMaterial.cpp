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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ConcreteMaterial.h>
#include <Units\Convert.h>

#include <StdIo.h>

#include <Materials/SimpleConcrete.h>
#include <Materials/ACI209Concrete.h>
#include <Materials/CEBFIPConcrete.h>

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
static const Float64 gs_Fci    = WBFL::Units::ConvertToSysUnits( 4.0, WBFL::Units::Measure::KSI );
static const Float64 gs_Fc     = WBFL::Units::ConvertToSysUnits(5.0,WBFL::Units::Measure::KSI);
static const Float64 gs_StrengthDensity = WBFL::Units::ConvertToSysUnits(160.,WBFL::Units::Measure::LbfPerFeet3);
static const Float64 gs_WeightDensity = gs_StrengthDensity;
static const Float64 gs_MaxAggregateSize = WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::Inch);
static const Float64 gs_Eci = WBFL::Units::ConvertToSysUnits(4200., WBFL::Units::Measure::KSI);
static const Float64 gs_Ec = WBFL::Units::ConvertToSysUnits(4700., WBFL::Units::Measure::KSI);


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

   Ffc = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::KSI);
   Frr = WBFL::Units::ConvertToSysUnits(0.75, WBFL::Units::Measure::KSI);
   FiberLength = WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Inch);
   AutogenousShrinkage = 0.0;
   bPCTT = false;

   alpha_u = 0.85; // GS 1.4.2.4.2
   ecu = -0.0035; // GS 1.4.2.4.2
   bExperimental_ecu = false;
   ftcr = WBFL::Units::ConvertToSysUnits(0.75, WBFL::Units::Measure::KSI);
   ftcri = 0.75 * ftcr;
   ftloc = WBFL::Units::ConvertToSysUnits(0.75, WBFL::Units::Measure::KSI);
   etloc = 0.0025;
   gamma_u = 1.0; // GS 1.4.2.5.4

   bBasePropertiesOnInitialValues = false;

   bACIUserParameters = false;
   CureMethod = pgsTypes::Moist;
   ACI209CementType = pgsTypes::TypeI;
   WBFL::Materials::ACI209Concrete::GetModelParameters((WBFL::Materials::CuringType)CureMethod,(WBFL::Materials::CementType)ACI209CementType,&A,&B);

   bCEBFIPUserParameters = false;
   CEBFIPCementType = pgsTypes::N;
   WBFL::Materials::CEBFIPConcrete::GetModelParameters((WBFL::Materials::CEBFIPConcrete::CementType)CEBFIPCementType,&S,&BetaSc);
}  

CConcreteMaterial::~CConcreteMaterial()
{
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

   if (MaxAggregateSize != rOther.MaxAggregateSize)
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

   if (Type == pgsTypes::PCI_UHPC)
   {
      if (!IsEqual(Ffc, rOther.Ffc))
         return false;

      if (!IsEqual(Frr, rOther.Frr))
         return false;

      if (!IsEqual(AutogenousShrinkage,rOther.AutogenousShrinkage))
         return false;

      if (!IsEqual(FiberLength,rOther.FiberLength))
         return false;

      if (bPCTT != rOther.bPCTT)
         return false;
   }

   if (Type == pgsTypes::UHPC)
   {
      if (!IsEqual(alpha_u, rOther.alpha_u))
         return false;

      if (bExperimental_ecu != rOther.bExperimental_ecu)
         return false;

      if (bExperimental_ecu && !IsEqual(ecu, rOther.ecu, 1.0E-6))
         return false;

      if (!IsEqual(ftcri, rOther.ftcri))
         return false;

      if (!IsEqual(ftcr, rOther.ftcr))
         return false;

      if (!IsEqual(ftloc, rOther.ftloc))
         return false;

      if (!IsEqual(etloc, rOther.etloc, 1.0E-6))
         return false;

      if (!IsEqual(gamma_u, rOther.gamma_u))
         return false;

      if (!IsEqual(FiberLength, rOther.FiberLength))
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

HRESULT CConcreteMaterial::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("Concrete"),6.0);
   pStrSave->put_Property(_T("Type"),             CComVariant( WBFL::LRFD::ConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)Type,false).c_str() ));
   pStrSave->put_Property(_T("Fc"),               CComVariant(Fc));

   pStrSave->put_Property(_T("UserEc"),           CComVariant(bUserEc));

   if ( bUserEc )
   {
      pStrSave->put_Property(_T("Ec"),               CComVariant(Ec));
   }

   pStrSave->put_Property(_T("WeightDensity"),    CComVariant(WeightDensity));
   pStrSave->put_Property(_T("StrengthDensity"),  CComVariant(StrengthDensity));
   pStrSave->put_Property(_T("MaxAggregateSize"), CComVariant(MaxAggregateSize));
   
   //pStrSave->put_Property(_T("FiberLength"), CComVariant(FiberLength)); // added in version 4, removed in version 6
   // This parameter should never have been saved with the conventional concrete properties. It only applies to UHPC and is stored with the UHPC parameters.

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

   if ( Type != pgsTypes::Normal ) // this should have been if LWC since it doesn't apply to UHPC, but it is done now so leave it alone
   {
      pStrSave->put_Property(_T("HasFct"),CComVariant(bHasFct));
      if ( bHasFct )
      {
         pStrSave->put_Property(_T("Fct"),CComVariant(Fct));
      }
   }
   pStrSave->EndUnit(); // AASHTO

   // Added in Version 3
   pStrSave->BeginUnit(_T("PCI_UHPC"), 2.0);
   pStrSave->put_Property(_T("Ffc"), CComVariant(Ffc));
   pStrSave->put_Property(_T("Frr"), CComVariant(Frr));
   pStrSave->put_Property(_T("FiberLength"), CComVariant(FiberLength));
   pStrSave->put_Property(_T("AutogenousShrinkage"), CComVariant(AutogenousShrinkage)); // added in PCI_UHPC version 2
   pStrSave->put_Property(_T("PCTT"), CComVariant(bPCTT));
   pStrSave->EndUnit(); // PCI_UHPC

   // Added in Version 5
   pStrSave->BeginUnit(_T("UHPC"), 3.0);
   pStrSave->put_Property(_T("alpha_u"), CComVariant(alpha_u)); // added in UHPC version 2
   pStrSave->put_Property(_T("Experimental_ecu"), CComVariant(bExperimental_ecu));
   if(bExperimental_ecu) pStrSave->put_Property(_T("ecu"), CComVariant(ecu)); // added in UHPC version 2
   pStrSave->put_Property(_T("ftcri"), CComVariant(ftcri));
   pStrSave->put_Property(_T("ftcr"), CComVariant(ftcr));
   pStrSave->put_Property(_T("ftloc"), CComVariant(ftloc));
   pStrSave->put_Property(_T("etloc"), CComVariant(etloc));
   pStrSave->put_Property(_T("gamma_u"), CComVariant(gamma_u)); // added in UHPC version 3
   pStrSave->put_Property(_T("FiberLength"), CComVariant(FiberLength));
   pStrSave->EndUnit(); // UHPC

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
   Type = (pgsTypes::ConcreteType)WBFL::LRFD::ConcreteUtil::GetTypeFromTypeName(OLE2T(var.bstrVal));

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

   if (3 < version && version < 6)
   {
      // added in version 4 and removed in version 6
      // This parameter should never have been saved with the conventional concrete properties. It only applies to UHPC and is stored with the UHPC parameters.
      // Load the parameter then throw it away.
      pStrLoad->get_Property(_T("FiberLength"), &var);
      //FiberLength = var.dblVal;
   }

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


   if(2 < version)
   {
      // added in Version 3
      pStrLoad->BeginUnit(_T("PCI_UHPC"));

      Float64 uhpc_version;
      pStrLoad->get_Version(&uhpc_version);
      
      var.vt = VT_R8;
      
      pStrLoad->get_Property(_T("Ffc"), &var);
      Ffc = var.dblVal;
      
      pStrLoad->get_Property(_T("Frr"), &var);
      Frr = var.dblVal;

      pStrLoad->get_Property(_T("FiberLength"), &var);
      FiberLength = var.dblVal;

      if (1 < uhpc_version)
      {
         pStrLoad->get_Property(_T("AutogenousShrinkage"), &var); // added in PCI_UHPC version 2
         AutogenousShrinkage = var.dblVal;
      }

      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("PCTT"), &var);
      bPCTT = (var.boolVal == VARIANT_TRUE ? true : false);

      pStrLoad->EndUnit(); // PCI_UHPC
   }

   if (4 < version)
   {
      // added in Version 5
      if (FAILED(pStrLoad->BeginUnit(_T("UHPC"))))
      { 
         // during early development this data block was called FHWA_UHPC
         // if BeginLoad(UHPC) files, try the old name
         pStrLoad->BeginUnit(_T("FHWA_UHPC"));
      }
      var.vt = VT_R8;

      Float64 uhpc_version;
      pStrLoad->get_Version(&uhpc_version);

      if (1 < uhpc_version)
      {
         // added in version 2
         pStrLoad->get_Property(_T("alpha_u"), &var);
         alpha_u = var.dblVal;

         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("Experimental_ecu"), &var);
         bExperimental_ecu = var.boolVal == VARIANT_TRUE ? true : false;
         if (bExperimental_ecu)
         {
            var.vt = VT_R8;
            pStrLoad->get_Property(_T("ecu"), &var);
            ecu = var.dblVal;
         }
      }

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("ftcri"), &var);
      ftcri = var.dblVal;

      pStrLoad->get_Property(_T("ftcr"), &var);
      ftcr = var.dblVal;

      pStrLoad->get_Property(_T("ftloc"), &var);
      ftloc = var.dblVal;

      pStrLoad->get_Property(_T("etloc"), &var);
      etloc = var.dblVal;

      if (2 < uhpc_version)
      {
         // added in Version 3
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("gamma_u"), &var);
         gamma_u = var.dblVal;
      }

      pStrLoad->get_Property(_T("FiberLength"), &var);
      FiberLength = var.dblVal;

      pStrLoad->EndUnit(); // UHPC
   }

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
         WBFL::Materials::CEBFIPConcrete::GetModelParameters((WBFL::Materials::CEBFIPConcrete::CementType)CEBFIPCementType,&S,&BetaSc);
      }

      pStrLoad->EndUnit(); // CEBFIP
   }

   pStrLoad->EndUnit(); // concrete

   return S_OK;
}

