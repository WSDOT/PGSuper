///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
   Fci = ::ConvertToSysUnits( 4.0, unitMeasure::KSI );
   Fc  = ::ConvertToSysUnits(5.0,unitMeasure::KSI);
   StrengthDensity = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   WeightDensity = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   MaxAggregateSize = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   K1 = 1.0;
   bUserEci = false;
   Eci = ::ConvertToSysUnits(4200., unitMeasure::KSI);
   bUserEc = false;
   Ec = ::ConvertToSysUnits(4700., unitMeasure::KSI);

   pStrandMaterial = 0;
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

   if ( K1 != rOther.K1 )
      return false;

   if ( bUserEci != rOther.bUserEci )
      return false;

   if ( !IsEqual(Eci,rOther.Eci) )
      return false;

   if ( bUserEc != rOther.bUserEc )
      return false;

   if ( !IsEqual(Ec,rOther.Ec) )
      return false;

   if ( pStrandMaterial != rOther.pStrandMaterial )
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
   pStrandMaterial = rOther.pStrandMaterial;

   Fci               = rOther.Fci;
   Fc                = rOther.Fc;
   WeightDensity     = rOther.WeightDensity;
   StrengthDensity   = rOther.StrengthDensity;
   MaxAggregateSize  = rOther.MaxAggregateSize;
   K1                = rOther.K1;
   bUserEci          = rOther.bUserEci;
   Eci               = rOther.Eci;
   bUserEc           = rOther.bUserEc;
   Ec                = rOther.Ec;
}


void CGirderMaterial::MakeAssignment(const CGirderMaterial& rOther)
{
   MakeCopy( rOther );
}
