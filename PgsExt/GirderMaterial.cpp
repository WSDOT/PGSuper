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
#include <PgsExt\GirderMaterial.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <Lrfd\StrandPool.h>
#include <Material\Concrete.h>

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
CGirderMaterial::CGirderMaterial()
{
   // Default material properties
   Type = pgsTypes::Normal;
   Fci = ::ConvertToSysUnits( 4.0, unitMeasure::KSI );
   Fc  = ::ConvertToSysUnits(5.0,unitMeasure::KSI);
   StrengthDensity = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   WeightDensity = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   MaxAggregateSize = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   EcK1 = 1.0;
   EcK2 = 1.0;
   CreepK1 = 1.0;
   CreepK2 = 1.0;
   ShrinkageK1 = 1.0;
   ShrinkageK2 = 1.0;
   bUserEci = false;
   Eci = ::ConvertToSysUnits(4200., unitMeasure::KSI);
   bUserEc = false;
   Ec = ::ConvertToSysUnits(4700., unitMeasure::KSI);
   bHasFct = false;
   Fct = 0;

   pStrandMaterial[pgsTypes::Straight]  = 0;
   pStrandMaterial[pgsTypes::Harped]    = 0;
   pStrandMaterial[pgsTypes::Temporary] = 0;
}  

CGirderMaterial::CGirderMaterial(const CGirderMaterial& rOther)
{
   MakeCopy(rOther);
}

CGirderMaterial::~CGirderMaterial()
{
}

//======================== OPERATORS  =======================================
CGirderMaterial& CGirderMaterial::operator= (const CGirderMaterial& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGirderMaterial::operator==(const CGirderMaterial& rOther) const
{
   if ( Type != rOther.Type )
      return false;

   if ( Fci != rOther.Fci )
      return false;

   if ( Fc != rOther.Fc )
      return false;

   if ( !IsEqual( WeightDensity, rOther.WeightDensity ) )
      return false;

   if ( StrengthDensity != rOther.StrengthDensity )
      return false;

   if ( MaxAggregateSize != rOther.MaxAggregateSize )
      return false;

   if ( EcK1 != rOther.EcK1 )
      return false;

   if ( EcK2 != rOther.EcK2 )
      return false;

   if ( CreepK1 != rOther.CreepK1 )
      return false;

   if ( CreepK2 != rOther.CreepK2 )
      return false;

   if ( ShrinkageK1 != rOther.ShrinkageK1 )
      return false;

   if ( ShrinkageK2 != rOther.ShrinkageK2 )
      return false;

   if ( bUserEci != rOther.bUserEci )
      return false;

   if ( bUserEci && !IsEqual(Eci,rOther.Eci) )
      return false;

   if ( bUserEc != rOther.bUserEc )
      return false;

   if ( bUserEc && !IsEqual(Ec,rOther.Ec) )
      return false;

   if ( bHasFct != rOther.bHasFct )
      return false;

   if ( bHasFct && !IsEqual(Fct,rOther.Fct) )
      return false;

   if ( pStrandMaterial[pgsTypes::Straight] != rOther.pStrandMaterial[pgsTypes::Straight] )
      return false;

   if ( pStrandMaterial[pgsTypes::Harped] != rOther.pStrandMaterial[pgsTypes::Harped] )
      return false;

   if ( pStrandMaterial[pgsTypes::Temporary] != rOther.pStrandMaterial[pgsTypes::Temporary] )
      return false;

   return true;
}

bool CGirderMaterial::operator!=(const CGirderMaterial& rOther) const
{
   return !operator==(rOther);
}

//======================== OPERATIONS =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderMaterial::MakeCopy(const CGirderMaterial& rOther)
{
   pStrandMaterial[pgsTypes::Straight]  = rOther.pStrandMaterial[pgsTypes::Straight];
   pStrandMaterial[pgsTypes::Harped]    = rOther.pStrandMaterial[pgsTypes::Harped];
   pStrandMaterial[pgsTypes::Temporary] = rOther.pStrandMaterial[pgsTypes::Temporary];

   Type              = rOther.Type;
   Fci               = rOther.Fci;
   Fc                = rOther.Fc;
   WeightDensity     = rOther.WeightDensity;
   StrengthDensity   = rOther.StrengthDensity;
   MaxAggregateSize  = rOther.MaxAggregateSize;
   EcK1              = rOther.EcK1;
   EcK2              = rOther.EcK2;
   CreepK1           = rOther.CreepK1;
   CreepK2           = rOther.CreepK2;
   ShrinkageK1       = rOther.ShrinkageK1;
   ShrinkageK2       = rOther.ShrinkageK2;
   bUserEci          = rOther.bUserEci;
   Eci               = rOther.Eci;
   bUserEc           = rOther.bUserEc;
   Ec                = rOther.Ec;
   bHasFct           = rOther.bHasFct;
   Fct               = rOther.Fct;
}


void CGirderMaterial::MakeAssignment(const CGirderMaterial& rOther)
{
   MakeCopy( rOther );
}

HRESULT CGirderMaterial::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit("Concrete",1.0);
   pStrSave->put_Property("Type",             CComVariant( matConcrete::GetTypeName((matConcrete::Type)Type,false).c_str() ));
   pStrSave->put_Property("Fci",              CComVariant(Fci ));
   pStrSave->put_Property("Fc",               CComVariant(Fc));
   pStrSave->put_Property("WeightDensity",    CComVariant(WeightDensity));
   pStrSave->put_Property("StrengthDensity",  CComVariant(StrengthDensity));
   pStrSave->put_Property("MaxAggregateSize", CComVariant(MaxAggregateSize));
   pStrSave->put_Property("EcK1",             CComVariant(EcK1));
   pStrSave->put_Property("EcK2",             CComVariant(EcK2));
   pStrSave->put_Property("CreepK1",          CComVariant(CreepK1));
   pStrSave->put_Property("CreepK2",          CComVariant(CreepK2));
   pStrSave->put_Property("ShrinkageK1",      CComVariant(ShrinkageK1));
   pStrSave->put_Property("ShrinkageK2",      CComVariant(ShrinkageK2));
   pStrSave->put_Property("UserEci",          CComVariant(bUserEci));
   
   if ( bUserEci )
      pStrSave->put_Property("Eci",              CComVariant(Eci));

   pStrSave->put_Property("UserEc",           CComVariant(bUserEc));

   if ( bUserEc )
      pStrSave->put_Property("Ec",               CComVariant(Ec));

   if ( Type != pgsTypes::Normal )
   {
      pStrSave->put_Property("HasFct",CComVariant(bHasFct));
      if ( bHasFct )
         pStrSave->put_Property("Fct",CComVariant(Fct));
   }

   pStrSave->EndUnit(); // concrete

   return S_OK;
}

HRESULT CGirderMaterial::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CComVariant var;
   USES_CONVERSION;

   pStrLoad->BeginUnit("Concrete");

   var.vt = VT_BSTR;
   pStrLoad->get_Property("Type",&var);
   Type = (pgsTypes::ConcreteType)matConcrete::GetTypeFromName(OLE2A(var.bstrVal));

   var.vt = VT_R8;
   pStrLoad->get_Property("Fci", &var);
   Fci = var.dblVal;

   pStrLoad->get_Property("Fc",&var);
   Fc = var.dblVal;

   pStrLoad->get_Property("WeightDensity", &var);
   WeightDensity = var.dblVal;

   pStrLoad->get_Property("StrengthDensity",  &var);
   StrengthDensity = var.dblVal;

   pStrLoad->get_Property("MaxAggregateSize", &var);
   MaxAggregateSize = var.dblVal;

   pStrLoad->get_Property("EcK1",&var);
   EcK1 = var.dblVal;

   pStrLoad->get_Property("EcK2",&var);
   EcK2 = var.dblVal;

   pStrLoad->get_Property("CreepK1",&var);
   CreepK1 = var.dblVal;

   pStrLoad->get_Property("CreepK2",&var);
   CreepK2 = var.dblVal;

   pStrLoad->get_Property("ShrinkageK1",&var);
   ShrinkageK1 = var.dblVal;

   pStrLoad->get_Property("ShrinkageK2",&var);
   ShrinkageK2 = var.dblVal;

   var.vt = VT_BOOL;
   pStrLoad->get_Property("UserEci",&var);
   bUserEci = (var.boolVal == VARIANT_TRUE ? true : false);
   
   if ( bUserEci )
   {
      var.vt = VT_R8;
      pStrLoad->get_Property("Eci",&var);
      Eci = var.dblVal;
   }

   var.vt = VT_BOOL;
   pStrLoad->get_Property("UserEc",&var);
   bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

   if ( bUserEc )
   {
      var.vt = VT_R8;
      pStrLoad->get_Property("Ec",&var);
      Ec = var.dblVal;
   }

   if ( Type != pgsTypes::Normal )
   {
      var.vt = VT_BOOL;
      pStrLoad->get_Property("HasFct",&var);
      bHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( bHasFct )
      {
         var.vt = VT_R8;
         pStrLoad->get_Property("Fct",&var);
         Fct = var.dblVal;
      }
   }

   pStrLoad->EndUnit(); // concrete

   return S_OK;
}
