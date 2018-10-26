///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Material\Concrete.h>

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
   strExteriorRailing = _T("");
   strInteriorRailing = _T("");

   pExteriorRailing = NULL;
   pInteriorRailing = NULL;

   bUseSidewalk = false;
   bUseInteriorRailing = false;

   bBarriersOnTopOfSidewalk = false;
   bSidewalkStructurallyContinuous = false;

   Width      = ::ConvertToSysUnits(6.0,unitMeasure::Feet);
   LeftDepth  = ::ConvertToSysUnits(6.0,unitMeasure::Inch);
   RightDepth = ::ConvertToSysUnits(6.0,unitMeasure::Inch);

   ConcreteType    = pgsTypes::Normal;
   fc              = ::ConvertToSysUnits(4.0,unitMeasure::KSI);
   Ec              = 0;
   bUserEc         = false;
   StrengthDensity = ::ConvertToSysUnits(155.0,unitMeasure::LbmPerFeet3);
   WeightDensity   = ::ConvertToSysUnits(155.0,unitMeasure::LbmPerFeet3);
   MaxAggSize      = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   EcK1            = 1.0;
   EcK2            = 1.0;
   CreepK1         = 1.0;
   CreepK2         = 1.0;
   ShrinkageK1     = 1.0;
   ShrinkageK2     = 1.0;
   bHasFct         = false;
   Fct             = 0.0;
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

   if ( !IsEqual(EcK1,rOther.EcK1) )
      return false;

   if ( !IsEqual(EcK2,rOther.EcK2) )
      return false;

   if ( !IsEqual(CreepK1,rOther.CreepK1) )
      return false;

   if ( !IsEqual(CreepK2,rOther.CreepK2) )
      return false;

   if ( !IsEqual(ShrinkageK1,rOther.ShrinkageK1) )
      return false;

   if ( !IsEqual(ShrinkageK2,rOther.ShrinkageK2) )
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
      hr = pStrLoad->BeginUnit(_T("RailingSystem"));
      double version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("ExteriorRailingName"), &var );
      strExteriorRailing = OLE2T(var.bstrVal);

      var.Clear();
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("Sidewalk"), &var );
      bUseSidewalk = var.boolVal == VARIANT_TRUE ? true : false;

      if ( bUseSidewalk )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Width"), &var );
         Width = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftDepth"), &var );
         LeftDepth = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightDepth"), &var );
         RightDepth = var.dblVal;

         // added in version 2
         if (2 <= version)
         {
            var.Clear();
            var.vt = VT_BOOL;

            hr = pStrLoad->get_Property(_T("BarriersOnTopOfSidewalk"),&var);
            bBarriersOnTopOfSidewalk = (var.boolVal == VARIANT_TRUE ? true : false);

            hr = pStrLoad->get_Property(_T("SidewalkStrucallyContinuous"),&var);
            bSidewalkStructurallyContinuous = (var.boolVal == VARIANT_TRUE ? true : false);
         }

         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("InteriorRailing"),&var);
         bUseInteriorRailing = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( bUseInteriorRailing )
         {
            var.Clear();
            var.vt = VT_BSTR;
            hr = pStrLoad->get_Property(_T("InteriorRailingName"),&var);
            strInteriorRailing = OLE2T(var.bstrVal);
         }
      }

      if ( 3 <= version )
      {
         // added in version 3
         if ( version == 3 )
         {
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("fc"),&var);
            fc = var.dblVal;

            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("UserEc"),&var);
            bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

            var.vt = VT_R8;
            if ( bUserEc )
            {
               hr = pStrLoad->get_Property(_T("Ec"),&var);
               Ec = var.dblVal;
            }

            hr = pStrLoad->get_Property(_T("StrengthDensity"),&var);
            StrengthDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("WeightDensity"),&var);
            WeightDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("MaxAggSize"),&var);
            MaxAggSize = var.dblVal;

            hr = pStrLoad->get_Property(_T("K1"),&var);
            EcK1 = var.dblVal;
         }
         else
         {
            pStrLoad->BeginUnit(_T("Concrete"));

            var.vt = VT_BSTR;
            hr = pStrLoad->get_Property(_T("Type"),&var);
            ConcreteType = (pgsTypes::ConcreteType)matConcrete::GetTypeFromName(OLE2T(var.bstrVal));

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("fc"),&var);
            fc = var.dblVal;

            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("UserEc"),&var);
            bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

            var.vt = VT_R8;
            if ( bUserEc )
            {
               hr = pStrLoad->get_Property(_T("Ec"),&var);
               Ec = var.dblVal;
            }

            hr = pStrLoad->get_Property(_T("StrengthDensity"),&var);
            StrengthDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("WeightDensity"),&var);
            WeightDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("MaxAggSize"),&var);
            MaxAggSize = var.dblVal;

            hr = pStrLoad->get_Property(_T("EcK1"),&var);
            EcK1 = var.dblVal;

            hr = pStrLoad->get_Property(_T("EcK2"),&var);
            EcK2 = var.dblVal;

            hr = pStrLoad->get_Property(_T("CreepK1"),&var);
            CreepK1 = var.dblVal;

            hr = pStrLoad->get_Property(_T("CreepK2"),&var);
            CreepK2 = var.dblVal;

            hr = pStrLoad->get_Property(_T("ShrinkageK1"),&var);
            ShrinkageK1 = var.dblVal;

            hr = pStrLoad->get_Property(_T("ShrinkageK2"),&var);
            ShrinkageK2 = var.dblVal;

            if ( ConcreteType != pgsTypes::Normal )
            {
               var.vt = VT_BOOL;
               hr = pStrLoad->get_Property(_T("HasFct"),&var);
               bHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

               if ( bHasFct )
               {
                  var.vt = VT_R8;
                  hr = pStrLoad->get_Property(_T("Fct"),&var);
                  Fct = var.dblVal;
               }
            }

            pStrLoad->EndUnit();
         }
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

   pStrSave->BeginUnit(_T("RailingSystem"),4.0);

   pStrSave->put_Property(_T("ExteriorRailingName"),     CComVariant(strExteriorRailing.c_str()));
   pStrSave->put_Property(_T("Sidewalk"),                CComVariant(bUseSidewalk));
   if ( bUseSidewalk )
   {
      pStrSave->put_Property(_T("Width"),     CComVariant(Width));
      pStrSave->put_Property(_T("LeftDepth"), CComVariant(LeftDepth));
      pStrSave->put_Property(_T("RightDepth"),CComVariant(RightDepth));

      // added in version 2
      pStrSave->put_Property(_T("BarriersOnTopOfSidewalk"),CComVariant(bBarriersOnTopOfSidewalk));
      pStrSave->put_Property(_T("SidewalkStrucallyContinuous"),CComVariant(bSidewalkStructurallyContinuous));


      pStrSave->put_Property(_T("InteriorRailing"), CComVariant(bUseInteriorRailing));
      if ( bUseInteriorRailing )
         pStrSave->put_Property(_T("InteriorRailingName"), CComVariant(strInteriorRailing.c_str()));
   }

   // added in version 3, updated version 4
   pStrSave->BeginUnit(_T("Concrete"),1.0);
      pStrSave->put_Property(_T("Type"),CComVariant( matConcrete::GetTypeName((matConcrete::Type)ConcreteType,false).c_str() ));
      pStrSave->put_Property(_T("fc"),CComVariant(fc));
      pStrSave->put_Property(_T("UserEc"),CComVariant(bUserEc));
      if ( bUserEc )
         pStrSave->put_Property(_T("Ec"),CComVariant(Ec));

      pStrSave->put_Property(_T("StrengthDensity"),CComVariant(StrengthDensity));
      pStrSave->put_Property(_T("WeightDensity"),CComVariant(WeightDensity));
      pStrSave->put_Property(_T("MaxAggSize"),CComVariant(MaxAggSize));
      pStrSave->put_Property(_T("EcK1"),CComVariant(EcK1));
      pStrSave->put_Property(_T("EcK2"),CComVariant(EcK2));
      pStrSave->put_Property(_T("CreepK1"),CComVariant(CreepK1));
      pStrSave->put_Property(_T("CreepK2"),CComVariant(CreepK2));
      pStrSave->put_Property(_T("ShrinkageK1"),CComVariant(ShrinkageK1));
      pStrSave->put_Property(_T("ShrinkageK2"),CComVariant(ShrinkageK2));

      if ( ConcreteType != pgsTypes::Normal )
      {
         pStrSave->put_Property(_T("HasFct"),CComVariant(bHasFct));
         if ( bHasFct )
            pStrSave->put_Property(_T("Fct"),CComVariant(Fct));
      }
   pStrSave->EndUnit();

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

   ConcreteType    = rOther.ConcreteType;
   fc              = rOther.fc;
   Ec              = rOther.Ec;
   bUserEc         = rOther.bUserEc;
   StrengthDensity = rOther.StrengthDensity;
   WeightDensity   = rOther.WeightDensity;
   MaxAggSize      = rOther.MaxAggSize;
   EcK1            = rOther.EcK1;
   EcK2            = rOther.EcK2;
   CreepK1         = rOther.CreepK1;
   CreepK2         = rOther.CreepK2;
   ShrinkageK1     = rOther.ShrinkageK1;
   ShrinkageK2     = rOther.ShrinkageK2;
   bHasFct         = rOther.bHasFct;
   Fct             = rOther.Fct;
}

void CRailingSystem::MakeAssignment(const CRailingSystem& rOther)
{
   MakeCopy( rOther );
}
