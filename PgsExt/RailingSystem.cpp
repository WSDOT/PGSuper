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
#include <PgsExt\RailingSystem.h>
#include <WbflAtlExt.h>
#include <PGSuperException.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CRailingSystem
****************************************************************************/


CRailingSystem::CRailingSystem()
{
   strExteriorRailing = "";
   strInteriorRailing = "";

   pExteriorRailing = NULL;
   pInteriorRailing = NULL;

   bUseSidewalk = false;
   bUseInteriorRailing = false;

   bBarriersOnTopOfSidewalk = false;
   bSidewalkStructurallyContinuous = false;

   Width      = ::ConvertToSysUnits(6.0,unitMeasure::Feet);
   LeftDepth  = ::ConvertToSysUnits(6.0,unitMeasure::Inch);
   RightDepth = ::ConvertToSysUnits(6.0,unitMeasure::Inch);

   fc              = ::ConvertToSysUnits(4.0,unitMeasure::KSI);
   Ec              = 0;
   bUserEc         = false;
   StrengthDensity = ::ConvertToSysUnits(155.0,unitMeasure::LbmPerFeet3);
   WeightDensity   = ::ConvertToSysUnits(155.0,unitMeasure::LbmPerFeet3);
   MaxAggSize      = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   K1              = 1.0;
}

CRailingSystem::CRailingSystem(const CRailingSystem& rOther)
{
   MakeCopy(rOther);
}

CRailingSystem::~CRailingSystem()
{
}

//======================== OPERATORS  =======================================
CRailingSystem& CRailingSystem::operator= (const CRailingSystem& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CRailingSystem::operator == (const CRailingSystem& rOther) const
{
   if ( strExteriorRailing != rOther.strExteriorRailing )
      return false;

   if ( bUseSidewalk != rOther.bUseSidewalk )
      return false;

   if ( bUseSidewalk )
   {
      if ( !IsEqual(Width,rOther.Width) )
         return false;

      if ( !IsEqual(LeftDepth,rOther.LeftDepth) )
         return false;

      if ( !IsEqual(RightDepth,rOther.RightDepth) )
         return false;

      if ( bBarriersOnTopOfSidewalk != rOther.bBarriersOnTopOfSidewalk )
         return false;
   
      if ( bSidewalkStructurallyContinuous != rOther.bSidewalkStructurallyContinuous )
         return false;

      if ( bUseInteriorRailing != rOther.bUseInteriorRailing )
         return false;

      if ( bUseInteriorRailing )
      {
         if ( strInteriorRailing != rOther.strInteriorRailing )
            return false;
      }
   }

   if ( !IsEqual(fc,rOther.fc) )
      return false;

   if ( bUserEc != rOther.bUserEc )
      return false;

   if ( bUserEc && !IsEqual(Ec,rOther.Ec) )
      return false;

   if ( !IsEqual(StrengthDensity,rOther.StrengthDensity) )
      return false;

   if ( !IsEqual(WeightDensity,rOther.WeightDensity) )
      return false;

   if ( !IsEqual(MaxAggSize,rOther.MaxAggSize) )
      return false;

   if ( !IsEqual(K1,rOther.K1) )
      return false;

   return true;
}

bool CRailingSystem::operator != (const CRailingSystem& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CRailingSystem::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit("RailingSystem");
      double version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property("ExteriorRailingName", &var );
      strExteriorRailing = OLE2A(var.bstrVal);

      var.Clear();
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property("Sidewalk", &var );
      bUseSidewalk = var.boolVal == VARIANT_TRUE ? true : false;

      if ( bUseSidewalk )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("Width", &var );
         Width = var.dblVal;

         hr = pStrLoad->get_Property("LeftDepth", &var );
         LeftDepth = var.dblVal;

         hr = pStrLoad->get_Property("RightDepth", &var );
         RightDepth = var.dblVal;

         // added in version 2
         if (2 <= version)
         {
            var.Clear();
            var.vt = VT_BOOL;

            hr = pStrLoad->get_Property("BarriersOnTopOfSidewalk",&var);
            bBarriersOnTopOfSidewalk = (var.boolVal == VARIANT_TRUE ? true : false);

            hr = pStrLoad->get_Property("SidewalkStrucallyContinuous",&var);
            bSidewalkStructurallyContinuous = (var.boolVal == VARIANT_TRUE ? true : false);
         }

         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property("InteriorRailing",&var);
         bUseInteriorRailing = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( bUseInteriorRailing )
         {
            var.Clear();
            var.vt = VT_BSTR;
            hr = pStrLoad->get_Property("InteriorRailingName",&var);
            strInteriorRailing = OLE2A(var.bstrVal);
         }
      }

      if ( 3 <= version )
      {
         // added in version 3
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("fc",&var);
         fc = var.dblVal;

         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property("UserEc",&var);
         bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         if ( bUserEc )
         {
            hr = pStrLoad->get_Property("Ec",&var);
            Ec = var.dblVal;
         }

         hr = pStrLoad->get_Property("StrengthDensity",&var);
         StrengthDensity = var.dblVal;

         hr = pStrLoad->get_Property("WeightDensity",&var);
         WeightDensity = var.dblVal;

         hr = pStrLoad->get_Property("MaxAggSize",&var);
         MaxAggSize = var.dblVal;

         hr = pStrLoad->get_Property("K1",&var);
         K1 = var.dblVal;
      }

      hr = pStrLoad->EndUnit();
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   return hr;
}

HRESULT CRailingSystem::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit("RailingSystem",3.0);

   pStrSave->put_Property("ExteriorRailingName",     CComVariant(strExteriorRailing.c_str()));
   pStrSave->put_Property("Sidewalk",                CComVariant(bUseSidewalk));
   if ( bUseSidewalk )
   {
      pStrSave->put_Property("Width",     CComVariant(Width));
      pStrSave->put_Property("LeftDepth", CComVariant(LeftDepth));
      pStrSave->put_Property("RightDepth",CComVariant(RightDepth));

      // added in version 2
      pStrSave->put_Property("BarriersOnTopOfSidewalk",CComVariant(bBarriersOnTopOfSidewalk));
      pStrSave->put_Property("SidewalkStrucallyContinuous",CComVariant(bSidewalkStructurallyContinuous));


      pStrSave->put_Property("InteriorRailing", CComVariant(bUseInteriorRailing));
      if ( bUseInteriorRailing )
         pStrSave->put_Property("InteriorRailingName", CComVariant(strInteriorRailing.c_str()));
   }

   // added in version 3
   pStrSave->put_Property("fc",CComVariant(fc));
   pStrSave->put_Property("UserEc",CComVariant(bUserEc));
   if ( bUserEc )
      pStrSave->put_Property("Ec",CComVariant(Ec));

   pStrSave->put_Property("StrengthDensity",CComVariant(StrengthDensity));
   pStrSave->put_Property("WeightDensity",CComVariant(WeightDensity));
   pStrSave->put_Property("MaxAggSize",CComVariant(MaxAggSize));
   pStrSave->put_Property("K1",CComVariant(K1));

   pStrSave->EndUnit();

   return hr;
}
//======================== ACCESS     =======================================
const TrafficBarrierEntry* CRailingSystem::GetExteriorRailing() const
{
   return pExteriorRailing;
}

void CRailingSystem::SetExteriorRailing(const TrafficBarrierEntry* pRailing)
{
   pExteriorRailing = pRailing;
}

const TrafficBarrierEntry* CRailingSystem::GetInteriorRailing() const
{
   return pInteriorRailing;
}

void CRailingSystem::SetInteriorRailing(const TrafficBarrierEntry* pRailing)
{
   pInteriorRailing = pRailing;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CRailingSystem::MakeCopy(const CRailingSystem& rOther)
{
   strExteriorRailing = rOther.strExteriorRailing;
   strInteriorRailing = rOther.strInteriorRailing;

   pExteriorRailing = rOther.pExteriorRailing;
   pInteriorRailing = rOther.pInteriorRailing;

   bUseSidewalk        = rOther.bUseSidewalk;
   bUseInteriorRailing = rOther.bUseInteriorRailing;
   bBarriersOnTopOfSidewalk = rOther.bBarriersOnTopOfSidewalk;
   bSidewalkStructurallyContinuous = rOther.bSidewalkStructurallyContinuous;

   Width      = rOther.Width;
   LeftDepth  = rOther.LeftDepth;
   RightDepth = rOther.RightDepth;

   fc              = rOther.fc;
   Ec              = rOther.Ec;
   bUserEc         = rOther.bUserEc;
   StrengthDensity = rOther.StrengthDensity;
   WeightDensity   = rOther.WeightDensity;
   MaxAggSize      = rOther.MaxAggSize;
   K1              = rOther.K1;
}

void CRailingSystem::MakeAssignment(const CRailingSystem& rOther)
{
   MakeCopy( rOther );
}
