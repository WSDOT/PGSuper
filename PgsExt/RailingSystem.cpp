///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Materials/Concrete.h>

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

   pExteriorRailing = nullptr;
   pInteriorRailing = nullptr;

   bUseSidewalk = false;
   bUseInteriorRailing = false;

   bBarriersOnTopOfSidewalk = false;
   bSidewalkStructurallyContinuous = false;

   Width      = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Feet);
   LeftDepth  = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch);
   RightDepth = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch);

   Concrete.Type    = pgsTypes::Normal;
   Concrete.bHasInitial     = false;
   Concrete.Fci             = -99999; // not used
   Concrete.Eci             = -99999; // not used
   Concrete.bUserEci        = false;  // not used
   Concrete.Fc              = WBFL::Units::ConvertToSysUnits(4.0,WBFL::Units::Measure::KSI);
   Concrete.Ec              = 0;
   Concrete.bUserEc         = false;
   Concrete.StrengthDensity = WBFL::Units::ConvertToSysUnits(155.0,WBFL::Units::Measure::LbmPerFeet3);
   Concrete.WeightDensity   = WBFL::Units::ConvertToSysUnits(155.0,WBFL::Units::Measure::LbmPerFeet3);
   Concrete.MaxAggregateSize= WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::Inch);
   Concrete.EcK1            = 1.0;
   Concrete.EcK2            = 1.0;
   Concrete.CreepK1         = 1.0;
   Concrete.CreepK2         = 1.0;
   Concrete.ShrinkageK1     = 1.0;
   Concrete.ShrinkageK2     = 1.0;
   Concrete.bHasFct         = false;
   Concrete.Fct             = 0.0;

   Concrete.bACIUserParameters = false;
   Concrete.A                  = WBFL::Units::ConvertToSysUnits(4.0,WBFL::Units::Measure::Day);
   Concrete.B                  = 0.85;
   Concrete.CureMethod         = pgsTypes::Moist;
   Concrete.ACI209CementType   = pgsTypes::TypeI;

   Concrete.CEBFIPCementType   = pgsTypes::N;
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
   {
      return false;
   }

   if ( bUseSidewalk != rOther.bUseSidewalk )
   {
      return false;
   }

   if ( bUseSidewalk )
   {
      if ( !IsEqual(Width,rOther.Width) )
      {
         return false;
      }

      if ( !IsEqual(LeftDepth,rOther.LeftDepth) )
      {
         return false;
      }

      if ( !IsEqual(RightDepth,rOther.RightDepth) )
      {
         return false;
      }

      if ( bBarriersOnTopOfSidewalk != rOther.bBarriersOnTopOfSidewalk )
      {
         return false;
      }
   
      if ( bSidewalkStructurallyContinuous != rOther.bSidewalkStructurallyContinuous )
      {
         return false;
      }

      if ( bUseInteriorRailing != rOther.bUseInteriorRailing )
      {
         return false;
      }

      if ( bUseInteriorRailing )
      {
         if ( strInteriorRailing != rOther.strInteriorRailing )
         {
            return false;
         }
      }
   }

   if ( Concrete != rOther.Concrete )
   {
      return false;
   }


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
      Float64 version;
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

      if ( 3 <= version && version < 5 )
      {
         // added in version 3
         if ( version == 3 )
         {
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("fc"),&var);
            Concrete.Fc = var.dblVal;

            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("UserEc"),&var);
            Concrete.bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

            var.vt = VT_R8;
            if ( Concrete.bUserEc )
            {
               hr = pStrLoad->get_Property(_T("Ec"),&var);
               Concrete.Ec = var.dblVal;
            }

            hr = pStrLoad->get_Property(_T("StrengthDensity"),&var);
            Concrete.StrengthDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("WeightDensity"),&var);
            Concrete.WeightDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("MaxAggSize"),&var);
            Concrete.MaxAggregateSize = var.dblVal;

            hr = pStrLoad->get_Property(_T("K1"),&var);
            Concrete.EcK1 = var.dblVal;
         }
         else
         {
            pStrLoad->BeginUnit(_T("Concrete"));

            var.vt = VT_BSTR;
            hr = pStrLoad->get_Property(_T("Type"),&var);
            Concrete.Type = (pgsTypes::ConcreteType)lrfdConcreteUtil::GetTypeFromTypeName(OLE2T(var.bstrVal));

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("fc"),&var);
            Concrete.Fc = var.dblVal;

            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("UserEc"),&var);
            Concrete.bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

            var.vt = VT_R8;
            if ( Concrete.bUserEc )
            {
               hr = pStrLoad->get_Property(_T("Ec"),&var);
               Concrete.Ec = var.dblVal;
            }

            hr = pStrLoad->get_Property(_T("StrengthDensity"),&var);
            Concrete.StrengthDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("WeightDensity"),&var);
            Concrete.WeightDensity = var.dblVal;

            hr = pStrLoad->get_Property(_T("MaxAggSize"),&var);
            Concrete.MaxAggregateSize = var.dblVal;

            hr = pStrLoad->get_Property(_T("EcK1"),&var);
            Concrete.EcK1 = var.dblVal;

            hr = pStrLoad->get_Property(_T("EcK2"),&var);
            Concrete.EcK2 = var.dblVal;

            hr = pStrLoad->get_Property(_T("CreepK1"),&var);
            Concrete.CreepK1 = var.dblVal;

            hr = pStrLoad->get_Property(_T("CreepK2"),&var);
            Concrete.CreepK2 = var.dblVal;

            hr = pStrLoad->get_Property(_T("ShrinkageK1"),&var);
            Concrete.ShrinkageK1 = var.dblVal;

            hr = pStrLoad->get_Property(_T("ShrinkageK2"),&var);
            Concrete.ShrinkageK2 = var.dblVal;

            if ( Concrete.Type != pgsTypes::Normal )
            {
               var.vt = VT_BOOL;
               hr = pStrLoad->get_Property(_T("HasFct"),&var);
               Concrete.bHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

               if ( Concrete.bHasFct )
               {
                  var.vt = VT_R8;
                  hr = pStrLoad->get_Property(_T("Fct"),&var);
                  Concrete.Fct = var.dblVal;
               }
            }

            pStrLoad->EndUnit();
         }
      }
      else if ( 5 <= version )
      {
         // version 5 and later
         Concrete.Load(pStrLoad,pProgress);
      }

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return hr;
}

HRESULT CRailingSystem::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("RailingSystem"),5.0);

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
      {
         pStrSave->put_Property(_T("InteriorRailingName"), CComVariant(strInteriorRailing.c_str()));
      }
   }

   // added in version 3, updated version 4, removed version 5, replaced with concrete object
   //pStrSave->BeginUnit(_T("Concrete"),1.0);
   //   pStrSave->put_Property(_T("Type"),CComVariant( lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)ConcreteType,false).c_str() ));
   //   pStrSave->put_Property(_T("fc"),CComVariant(fc));
   //   pStrSave->put_Property(_T("UserEc"),CComVariant(bUserEc));
   //   if ( bUserEc )
   //      pStrSave->put_Property(_T("Ec"),CComVariant(Ec));

   //   pStrSave->put_Property(_T("StrengthDensity"),CComVariant(StrengthDensity));
   //   pStrSave->put_Property(_T("WeightDensity"),CComVariant(WeightDensity));
   //   pStrSave->put_Property(_T("MaxAggSize"),CComVariant(MaxAggSize));
   //   pStrSave->put_Property(_T("EcK1"),CComVariant(EcK1));
   //   pStrSave->put_Property(_T("EcK2"),CComVariant(EcK2));
   //   pStrSave->put_Property(_T("CreepK1"),CComVariant(CreepK1));
   //   pStrSave->put_Property(_T("CreepK2"),CComVariant(CreepK2));
   //   pStrSave->put_Property(_T("ShrinkageK1"),CComVariant(ShrinkageK1));
   //   pStrSave->put_Property(_T("ShrinkageK2"),CComVariant(ShrinkageK2));

   //   if ( ConcreteType != pgsTypes::Normal )
   //   {
   //      pStrSave->put_Property(_T("HasFct"),CComVariant(bHasFct));
   //      if ( bHasFct )
   //         pStrSave->put_Property(_T("Fct"),CComVariant(Fct));
   //   }
   //pStrSave->EndUnit();
   Concrete.Save(pStrSave,pProgress);

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

   Concrete   = rOther.Concrete;
}

void CRailingSystem::MakeAssignment(const CRailingSystem& rOther)
{
   MakeCopy( rOther );
}
