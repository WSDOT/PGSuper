///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <psgLib\ConcreteLibraryEntry.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include "ConcreteEntryDlg.h"
#include <Units\Convert.h>

#include <MathEx.h>
#include <Materials/SimpleConcrete.h>
#include <Materials/ACI209Concrete.h>
#include <Materials/CEBFIPConcrete.h>

#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   ConcreteLibraryEntry
****************************************************************************/


CString ConcreteLibraryEntry::GetConcreteType(pgsTypes::ConcreteType type)
{
   LPCTSTR lpszType;
   switch (type)
   {
   case pgsTypes::Normal:
      lpszType = _T("Normal weight");
      break;

   case pgsTypes::AllLightweight:
      lpszType = _T("All lightweight");
      break;

   case pgsTypes::SandLightweight:
      lpszType = _T("Sand lightweight");
      break;

   case pgsTypes::PCI_UHPC:
      lpszType = _T("PCI-UHPC");
      break;

   case pgsTypes::UHPC:
      lpszType = _T("UHPC");
      break;

   default:
      ATLASSERT(false);
   }
   return lpszType;
}

CString ConcreteLibraryEntry::GetConcreteCureMethod(pgsTypes::CureMethod method)
{
   LPCTSTR lpszType;
   switch(method)
   {
   case pgsTypes::Moist:
      lpszType = _T("Moist");
      break;

   case pgsTypes::Steam:
      lpszType = _T("Steam");
      break;

   default:
      ATLASSERT(false);
   }

   return lpszType;
}

CString ConcreteLibraryEntry::GetACI209CementType(pgsTypes::ACI209CementType type)
{
   LPCTSTR lpszType;
   switch(type)
   {
   case pgsTypes::TypeI:
      lpszType = _T("Type I");
      break;

   case pgsTypes::TypeIII:
      lpszType = _T("Type III");
      break;

   default:
      ATLASSERT(false);
   }

   return lpszType;
}

CString ConcreteLibraryEntry::GetCEBFIPCementType(pgsTypes::CEBFIPCementType type)
{
   LPCTSTR lpszType;
   switch(type)
   {
   case pgsTypes::RS:
      lpszType = _T("Rapid Hardening, High Strength Cement (RS)");
      break;

   case pgsTypes::N:
      lpszType = _T("Normal Hardening Cement (N)");
      break;

   case pgsTypes::R:
      lpszType = _T("Rapid Hardening Cement (R)");
      break;

   case pgsTypes::SL:
      lpszType = _T("Slowly Hardening Cement (SL)");
      break;

   default:
      ATLASSERT(false);
   }

   return lpszType;
}


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
ConcreteLibraryEntry::ConcreteLibraryEntry() :
m_Fc(WBFL::Units::ConvertToSysUnits(4.,WBFL::Units::Measure::KSI)),
m_Ec(WBFL::Units::ConvertToSysUnits(4200.,WBFL::Units::Measure::KSI)),
m_bUserEc(false),
m_Ds(WBFL::Units::ConvertToSysUnits(160.,WBFL::Units::Measure::LbfPerFeet3)),
m_Dw(WBFL::Units::ConvertToSysUnits(160.,WBFL::Units::Measure::LbfPerFeet3)),
m_AggSize(WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::Inch)),
m_EccK1(1.0),
m_EccK2(1.0),
m_CreepK1(1.0),
m_CreepK2(1.0),
m_ShrinkageK1(1.0),
m_ShrinkageK2(1.0),
m_Type(pgsTypes::Normal),
m_Fct(0), // need a good default value
m_Ffc(WBFL::Units::ConvertToSysUnits(1.5,WBFL::Units::Measure::KSI)),
m_Frr(WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::KSI)),
m_FiberLength(WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch)),
m_bPCTT(false),
m_bHasFct(false),
m_AutogenousShrinkage(0.3e-03),
m_ftcri(WBFL::Units::ConvertToSysUnits(0.75*0.75, WBFL::Units::Measure::KSI)),
m_ftcr(WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::KSI)),
m_ftloc(WBFL::Units::ConvertToSysUnits(0.75, WBFL::Units::Measure::KSI)),
m_etloc(0.0025),
m_alpha_u(0.85),
m_ecu(-0.0035),
m_gamma_u(1.0),
m_bExperimental_ecu(false),
m_bUserACIParameters(false),
m_CureMethod(pgsTypes::Moist),
m_ACI209CementType(pgsTypes::TypeI),
m_bUserCEBFIPParameters(false),
m_CEBFIPCementType(pgsTypes::N)
{
   WBFL::Materials::ACI209Concrete::GetModelParameters((WBFL::Materials::CuringType)m_CureMethod,(WBFL::Materials::CementType)m_ACI209CementType,&m_A,&m_B);
   WBFL::Materials::CEBFIPConcrete::GetModelParameters((WBFL::Materials::CEBFIPConcrete::CementType)m_CEBFIPCementType,&m_S,&m_BetaSc);
}

ConcreteLibraryEntry::~ConcreteLibraryEntry()
{
}

bool ConcreteLibraryEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("ConcreteMaterialEntry"), 8.0);

   // Version 5, re-arranged data and added AASHTO and ACI groups.
   // Version 6, added CEB-FIP Concrete
   // Version 7, added PCI UHPC
   // Version 8, added UHPC

   pSave->Property(_T("Name"),this->GetName().c_str());
   
   // added version 4
   pSave->Property(_T("Type"),WBFL::LRFD::ConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)m_Type,false).c_str());
   pSave->Property(_T("Dw"), m_Dw);
   pSave->Property(_T("Fc"), m_Fc);
   pSave->Property(_T("Ds"), m_Ds);
   pSave->Property(_T("AggregateSize"),m_AggSize);
   pSave->Property(_T("UserEc"),m_bUserEc);
   pSave->Property(_T("Ec"),m_Ec);

   pSave->BeginUnit(_T("AASHTO"),1.0);
   pSave->Property(_T("EccK1"), m_EccK1); // changed from K1 in version 4
   pSave->Property(_T("EccK2"), m_EccK2); // added in version 4
   pSave->Property(_T("CreepK1"), m_CreepK1); // added in version 4
   pSave->Property(_T("CreepK2"), m_CreepK2); // added in version 4
   pSave->Property(_T("ShrinkageK1"), m_ShrinkageK1); // added in version 4
   pSave->Property(_T("ShrinkageK2"), m_ShrinkageK2); // added in version 4

   // Version 4
   if ( m_Type != pgsTypes::Normal )
   {
      pSave->Property(_T("HasAggSplittingStrength"),m_bHasFct);
      if ( m_bHasFct )
      {
         pSave->Property(_T("AggSplittingStrength"),m_Fct);
      }
   }
   pSave->EndUnit(); // AASHTO

   // Added version 7
   pSave->BeginUnit(_T("PCIUHPC"), 2.0);
   pSave->Property(_T("Ffc"), m_Ffc);
   pSave->Property(_T("Frr"), m_Frr);
   pSave->Property(_T("FiberLength"), m_FiberLength);
   pSave->Property(_T("PCTT"), m_bPCTT);
   pSave->Property(_T("AutogenousShrinkage"), m_AutogenousShrinkage); // added in PCIUHPC 2.0
   pSave->EndUnit(); // PCIUHPC

   // Added version 8
   pSave->BeginUnit(_T("UHPC"), 2.0);
   pSave->Property(_T("ftcri"),m_ftcri);
   pSave->Property(_T("ftcr"), m_ftcr);
   pSave->Property(_T("ftloc"),m_ftloc);
   pSave->Property(_T("etloc"),m_etloc);
   pSave->Property(_T("alpha_u"), m_alpha_u);
   pSave->Property(_T("ecu"), m_ecu);
   pSave->Property(_T("Experimental_ecu"), m_bExperimental_ecu);
   pSave->Property(_T("gamma_u"), m_gamma_u); // added version 2 of this data block
   pSave->Property(_T("FiberLength"), m_FiberLength);
   pSave->EndUnit(); // UHPC

   pSave->BeginUnit(_T("ACI"),1.0);
   pSave->Property(_T("UserACI"),m_bUserACIParameters);
   pSave->Property(_T("A"),m_A);
   pSave->Property(_T("B"),m_B);
   pSave->Property(_T("CureMethod"),m_CureMethod);
   pSave->Property(_T("CementType"),m_ACI209CementType);
   pSave->EndUnit(); // ACI

   // Added version 6
   pSave->BeginUnit(_T("CEBFIP"),2.0);
   pSave->Property(_T("UserCEBFIP"),m_bUserCEBFIPParameters); // added in version 2
   pSave->Property(_T("S"),m_S); // added in version 2
   pSave->Property(_T("BetaSc"),m_BetaSc); // added in version 2
   pSave->Property(_T("CementType"),m_CEBFIPCementType);
   pSave->EndUnit(); // CEBFIP

   pSave->EndUnit();

   return false;
}

bool ConcreteLibraryEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("ConcreteMaterialEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (8.0 < version )
      {
         THROW_LOAD(BadVersion,pLoad);
      }

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
      {
         this->SetName(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }


      // Added version 4
      if ( 3.0 < version )
      {
         std::_tstring strType;
         pLoad->Property(_T("Type"),&strType);
         m_Type = (pgsTypes::ConcreteType)WBFL::LRFD::ConcreteUtil::GetTypeFromTypeName(strType.c_str());
      }

      if(!pLoad->Property(_T("Dw"), &m_Dw))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("Fc"), &m_Fc))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("Ds"), &m_Ds))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("AggregateSize"), &m_AggSize))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 2.0 <= version && version < 4)
      {
         if ( !pLoad->Property(_T("K1"),&m_EccK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else if ( 4 <= version && version < 5 )
      {
         if ( !pLoad->Property(_T("EccK1"),&m_EccK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("EccK2"),&m_EccK2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CreepK1"),&m_CreepK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CreepK2"),&m_CreepK2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ShrinkageK1"),&m_ShrinkageK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ShrinkageK2"),&m_ShrinkageK2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 3.0 <= version && version < 5 )
      {
         if ( !pLoad->Property(_T("UserEc"),&m_bUserEc) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Ec"),&m_Ec) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 4.0 <= version && version < 5 )
      {
         // Added version 4
         if ( m_Type != pgsTypes::Normal )
         {
            if ( !pLoad->Property(_T("HasAggSplittingStrength"),&m_bHasFct) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( m_bHasFct )
            {
               if ( !pLoad->Property(_T("AggSplittingStrength"),&m_Fct) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
         }
      }

      if ( 4 < version )
      {
         // Version 5

         if ( !pLoad->Property(_T("UserEc"),&m_bUserEc) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Ec"),&m_Ec) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->BeginUnit(_T("AASHTO")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("EccK1"), &m_EccK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("EccK2"), &m_EccK2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CreepK1"), &m_CreepK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CreepK2"), &m_CreepK2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ShrinkageK1"), &m_ShrinkageK1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ShrinkageK2"), &m_ShrinkageK2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if ( m_Type != pgsTypes::Normal )
         {
            if ( !pLoad->Property(_T("HasAggSplittingStrength"),&m_bHasFct) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( m_bHasFct )
            {
               if ( !pLoad->Property(_T("AggSplittingStrength"),&m_Fct) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
         }
         if ( !pLoad->EndUnit() ) // AASHTO
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (6 < version)
         {
            // Added version 7
            if(!pLoad->BeginUnit(_T("PCIUHPC"))) THROW_LOAD(InvalidFileFormat,pLoad);
            Float64 PCIUHPC_Version = pLoad->GetVersion();

            if (!pLoad->Property(_T("Ffc"), &m_Ffc)) THROW_LOAD(InvalidFileFormat, pLoad);
            if(!pLoad->Property(_T("Frr"), &m_Frr)) THROW_LOAD(InvalidFileFormat, pLoad);
            if(!pLoad->Property(_T("FiberLength"), &m_FiberLength)) THROW_LOAD(InvalidFileFormat, pLoad);
            if(!pLoad->Property(_T("PCTT"), &m_bPCTT)) THROW_LOAD(InvalidFileFormat, pLoad);

            if (1 < PCIUHPC_Version)
            {
               if (!pLoad->Property(_T("AutogenousShrinkage"), &m_AutogenousShrinkage)) THROW_LOAD(InvalidFileFormat,pLoad); // added in PCIUHPC 2.0
            }

            if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (7 < version)
         {
            // added version 8
            if (!pLoad->BeginUnit(_T("UHPC")))
            {
               // Early versions of this data block used FHWAUHPC
               // If UHPC fails, try the old name
               if (!pLoad->BeginUnit(_T("FHWAUHPC"))) 
                  THROW_LOAD(InvalidFileFormat, pLoad);
            }

            Float64 uhpc_version = pLoad->GetVersion();
            if (!pLoad->Property(_T("ftcri"), &m_ftcri)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->Property(_T("ftcr"), &m_ftcr)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->Property(_T("ftloc"), &m_ftloc)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->Property(_T("etloc"), &m_etloc)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->Property(_T("alpha_u"), &m_alpha_u)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->Property(_T("ecu"), &m_ecu)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->Property(_T("Experimental_ecu"), &m_bExperimental_ecu)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (1 < uhpc_version)
            {
               // added version 2 of this data block
               if (!pLoad->Property(_T("gamma_u"), &m_gamma_u)) THROW_LOAD(InvalidFileFormat, pLoad);
            }
            if (!pLoad->Property(_T("FiberLength"), &m_FiberLength)) THROW_LOAD(InvalidFileFormat, pLoad);
            if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // UHPC
         }

         if ( !pLoad->BeginUnit(_T("ACI")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("UserACI"),&m_bUserACIParameters))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("A"),&m_A) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("B"),&m_B) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         int value;
         if ( !pLoad->Property(_T("CureMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_CureMethod = (pgsTypes::CureMethod)value;

         if ( !pLoad->Property(_T("CementType"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_ACI209CementType = (pgsTypes::ACI209CementType)value;

         if ( !pLoad->EndUnit() ) // ACI
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 5 < version )
      {
         // version 6
         if ( !pLoad->BeginUnit(_T("CEBFIP")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 cebfipVersion = pLoad->GetVersion();

         if ( 1 < cebfipVersion )
         {
            // added in version 2
           if (!pLoad->Property(_T("UserCEBFIP"),&m_bUserCEBFIPParameters))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("S"),&m_S) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("BetaSc"),&m_BetaSc) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         int value;
         if ( !pLoad->Property(_T("CementType"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_CEBFIPCementType = (pgsTypes::CEBFIPCementType)value;

         if ( !pLoad->EndUnit() ) // CEBFIP
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->EndUnit())
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      return false; // not a concrete entry
   }
   
   return true;
}

bool ConcreteLibraryEntry::IsEqual(const ConcreteLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool ConcreteLibraryEntry::Compare(const ConcreteLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   bMustRename = false;

   if ( m_Type != rOther.m_Type )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Type"),GetConcreteType(m_Type),GetConcreteType(rOther.m_Type)));
   }

   if ( !::IsEqual(m_Fc,rOther.m_Fc) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("f'c"),m_Fc,rOther.m_Fc,pDisplayUnits->Stress));
   }

   if ( !::IsEqual(m_Ds,rOther.m_Ds) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDensityItem(_T("Unit Weight"),m_Ds,rOther.m_Ds,pDisplayUnits->Density));
   }

   if ( !::IsEqual(m_Dw,rOther.m_Dw) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDensityItem(_T("Unit Weight with Reinforcement"),m_Dw,rOther.m_Dw,pDisplayUnits->Density));
   }

   if ( m_bUserEc != rOther.m_bUserEc )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Mod. Elasticity, Ec"),m_bUserEc,rOther.m_bUserEc,_T("Checked"),_T("Unchecked")));
   }

   if ( m_bUserEc && !::IsEqual(m_Ec,rOther.m_Ec) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Ec"),m_Ec,rOther.m_Ec,pDisplayUnits->ModE));
   }

   if ( !IsUHPC(m_Type) && !::IsEqual(m_AggSize,rOther.m_AggSize) )
   {
      // Agg Size not applicable to UHPC - the UI elements in the dialog are hidden
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("Max. Aggregate Size"),m_AggSize,rOther.m_AggSize,pDisplayUnits->ComponentDim));
   }

   if ( !::IsEqual(m_EccK1,rOther.m_EccK1) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Mod. E, K1"),m_EccK1,rOther.m_EccK1));
   }

   if ( !::IsEqual(m_EccK2,rOther.m_EccK2) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Mod. E, K2"),m_EccK2,rOther.m_EccK2));
   }

   if ( !::IsEqual(m_CreepK1,rOther.m_CreepK1) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Creep, K1"),m_CreepK1,rOther.m_CreepK1));
   }

   if ( !::IsEqual(m_CreepK2,rOther.m_CreepK2) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Creep, K2"),m_CreepK2,rOther.m_CreepK2));
   }

   if ( !::IsEqual(m_ShrinkageK1,rOther.m_ShrinkageK1) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Shrinkage, K1"),m_ShrinkageK1,rOther.m_ShrinkageK1));
   }

   if ( !::IsEqual(m_ShrinkageK2,rOther.m_ShrinkageK2) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Shrinkage, K2"),m_ShrinkageK2,rOther.m_ShrinkageK2));
   }

   if ( IsLWC(m_Type) )
   {
      if ( m_bHasFct != rOther.m_bHasFct )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Agg. Splitting Strength, fct"),m_bHasFct,rOther.m_bHasFct,_T("Checked"),_T("Unchecked")));
      }

      if ( m_bHasFct && !::IsEqual(m_Fct,rOther.m_Fct) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Fct"),m_Fct,rOther.m_Fct,pDisplayUnits->Stress));
      }
   }

   if (m_Type == pgsTypes::PCI_UHPC)
   {
      if (!::IsEqual(m_Ffc, rOther.m_Ffc))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Ffc"), m_Ffc, rOther.m_Ffc, pDisplayUnits->Stress));
      }

      if (!::IsEqual(m_Frr, rOther.m_Frr))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Frr"), m_Frr, rOther.m_Frr, pDisplayUnits->Stress));
      }

      if (!::IsEqual(m_FiberLength, rOther.m_FiberLength))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Fiber Length"), m_Frr, rOther.m_Frr, pDisplayUnits->Stress));
      }

      if (m_bPCTT != rOther.m_bPCTT)
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("PCTT"), m_bPCTT, rOther.m_bPCTT, _T("Checked"), _T("Unchecked")));
      }

      if (!::IsEqual(m_AutogenousShrinkage, rOther.m_AutogenousShrinkage))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Autogenous Shrinkage"),m_AutogenousShrinkage, rOther.m_AutogenousShrinkage));
      }
   }

   if (m_Type == pgsTypes::UHPC)
   {
      if (!::IsEqual(m_ftcri, rOther.m_ftcri))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Initial effective cracking strength (ft,cri)"), m_ftcri, rOther.m_ftcri, pDisplayUnits->Stress));
      }

      if (!::IsEqual(m_ftcr, rOther.m_ftcr))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Design effective cracking strength (ft,cr)"), m_ftcr, rOther.m_ftcr, pDisplayUnits->Stress));
      }

      if (!::IsEqual(m_ftloc, rOther.m_ftloc))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Crack localization strength (ft,loc)"), m_ftloc, rOther.m_ftloc, pDisplayUnits->Stress));
      }

      if (!::IsEqual(m_etloc, rOther.m_etloc))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Crack localization strain (et,loc)"), _T(""), _T("")));// m_etloc, rOther.m_etloc));
      }

      if (!::IsEqual(m_FiberLength, rOther.m_FiberLength))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStressItem(_T("Fiber Length"), m_Frr, rOther.m_Frr, pDisplayUnits->Stress));
      }

      if (!::IsEqual(m_alpha_u, rOther.m_alpha_u))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Compression response reduction factor (alpha,u)"), _T(""), _T("")));
      }

      if (!::IsEqual(m_gamma_u, rOther.m_gamma_u))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Fiber orientation reduction factor (gamma,u)"), _T(""), _T("")));
      }

      if (!::IsEqual(m_ecu, rOther.m_ecu))
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Ultimate compression strain (e,cu)"), _T(""), _T("")));
      }
   }

   if ( m_bUserACIParameters != rOther.m_bUserACIParameters )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Use time parameters from ACI 209R-92"),m_bUserACIParameters,rOther.m_bUserACIParameters,_T("Checked"),_T("Unchecked")));
   }

   if ( m_bUserACIParameters )
   {
      if ( !::IsEqual(m_A,rOther.m_A) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("a"),m_A,rOther.m_A));
      }

      if ( !::IsEqual(m_B,rOther.m_B) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Beta"),m_B,rOther.m_B));
      }
   }
   else
   {
      if ( m_CureMethod != rOther.m_CureMethod )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Cure Method"),GetConcreteCureMethod(m_CureMethod),GetConcreteCureMethod(rOther.m_CureMethod)));
      }

      if ( m_ACI209CementType != rOther.m_ACI209CementType )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Cement Type"),GetACI209CementType(m_ACI209CementType),GetACI209CementType(rOther.m_ACI209CementType)));
      }
   }

   if ( m_bUserCEBFIPParameters != rOther.m_bUserCEBFIPParameters )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Use time parameters from CEB-FIP Model Code"),m_bUserCEBFIPParameters,rOther.m_bUserCEBFIPParameters,_T("Checked"),_T("Unchecked")));
   }

   if ( m_bUserCEBFIPParameters )
   {
      if ( !::IsEqual(m_S,rOther.m_S) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("S"),m_S,rOther.m_S));
      }

      if ( !::IsEqual(m_BetaSc,rOther.m_BetaSc) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceDoubleItem(_T("Beta SC"),m_BetaSc,rOther.m_BetaSc));
      }
   }
   else
   {
      if ( m_CEBFIPCementType != rOther.m_CEBFIPCementType )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Cement Type"),GetCEBFIPCementType(m_CEBFIPCementType),GetCEBFIPCementType(rOther.m_CEBFIPCementType)));
      }
   }

   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }

   return vDifferences.size() == 0 ? true : false;
}


//======================== ACCESS     =======================================

void ConcreteLibraryEntry::SetFc(Float64 fc)
{
   m_Fc=fc;
}

Float64 ConcreteLibraryEntry::GetFc() const
{
   return m_Fc;
}

void ConcreteLibraryEntry::SetEc(Float64 ec)
{
   m_Ec = ec;
}

Float64 ConcreteLibraryEntry::GetEc() const
{
   return m_Ec;
}

void ConcreteLibraryEntry::UserEc(bool bUserEc)
{
   m_bUserEc = bUserEc;
}

bool ConcreteLibraryEntry::UserEc() const
{
   return m_bUserEc;
}

void ConcreteLibraryEntry::SetStrengthDensity(Float64 d)
{
   m_Ds = d;
}

Float64 ConcreteLibraryEntry::GetStrengthDensity() const
{
   return m_Ds;
}

void ConcreteLibraryEntry::SetWeightDensity(Float64 d)
{
   m_Dw = d;
}

Float64 ConcreteLibraryEntry::GetWeightDensity() const
{
   return m_Dw;
}

void ConcreteLibraryEntry::SetAggregateSize(Float64 s)
{
   m_AggSize = s;
}

Float64 ConcreteLibraryEntry::GetAggregateSize()const
{
   return m_AggSize;
}

void ConcreteLibraryEntry::SetModEK1(Float64 k1)
{
   m_EccK1 = k1;
}

Float64 ConcreteLibraryEntry::GetModEK1() const
{
   return m_EccK1;
}

void ConcreteLibraryEntry::SetModEK2(Float64 k2)
{
   m_EccK2 = k2;
}

Float64 ConcreteLibraryEntry::GetModEK2() const
{
   return m_EccK2;
}

void ConcreteLibraryEntry::SetCreepK1(Float64 k1)
{
   m_CreepK1 = k1;
}

Float64 ConcreteLibraryEntry::GetCreepK1() const
{
   return m_CreepK1;
}

void ConcreteLibraryEntry::SetCreepK2(Float64 k2)
{
   m_CreepK2 = k2;
}

Float64 ConcreteLibraryEntry::GetCreepK2() const
{
   return m_CreepK2;
}

void ConcreteLibraryEntry::SetShrinkageK1(Float64 k1)
{
   m_ShrinkageK1 = k1;
}

Float64 ConcreteLibraryEntry::GetShrinkageK1() const
{
   return m_ShrinkageK1;
}

void ConcreteLibraryEntry::SetShrinkageK2(Float64 k2)
{
   m_ShrinkageK2 = k2;
}

Float64 ConcreteLibraryEntry::GetShrinkageK2() const
{
   return m_ShrinkageK2;
}

void ConcreteLibraryEntry::SetType(pgsTypes::ConcreteType type)
{
   m_Type = type;
}

pgsTypes::ConcreteType ConcreteLibraryEntry::GetType() const
{
   return m_Type;
}

void ConcreteLibraryEntry::HasAggSplittingStrength(bool bHasFct)
{
   m_bHasFct = bHasFct;
}

bool ConcreteLibraryEntry::HasAggSplittingStrength() const
{
   return m_bHasFct;
}

void ConcreteLibraryEntry::SetAggSplittingStrength(Float64 fct)
{
   m_Fct = fct;
}

Float64 ConcreteLibraryEntry::GetAggSplittingStrength() const
{
   return m_Fct;
}

void ConcreteLibraryEntry::SetPCIUHPC(Float64 ffc, Float64 frr, Float64 fiberLength,Float64 autogenousShrinkage,bool bPCTT)
{
   m_Ffc = ffc;
   m_Frr = frr;
   m_FiberLength = fiberLength;
   m_bPCTT = bPCTT;
   m_AutogenousShrinkage = autogenousShrinkage;
}

void ConcreteLibraryEntry::GetPCIUHPC(Float64* ffc, Float64* frr, Float64* fiberLength,Float64* pAutogenousShrinkage, bool* bPCTT) const
{
   *ffc = m_Ffc;
   *frr = m_Frr;
   *fiberLength = m_FiberLength;
   *bPCTT = m_bPCTT;
   *pAutogenousShrinkage = m_AutogenousShrinkage;
}

void ConcreteLibraryEntry::SetUHPC(Float64 ft_cri, Float64 ft_cr, Float64 ft_loc, Float64 et_loc,Float64 alpha_u,Float64 ecu,bool bExperimentalEcu,Float64 gammaU,Float64 fiberLength)
{
   m_ftcri = ft_cri;
   m_ftcr = ft_cr;
   m_ftloc = ft_loc;
   m_etloc = et_loc;
   m_alpha_u = alpha_u;
   m_ecu = ecu;
   m_bExperimental_ecu = bExperimentalEcu;
   m_gamma_u = gammaU;
   m_FiberLength = fiberLength;
}

void ConcreteLibraryEntry::GetUHPC(Float64* ft_cri, Float64* ft_cr, Float64* ft_loc, Float64* et_loc,Float64* alpha_u,Float64* ecu,bool* pbExperimentalEcu,Float64* pGammaU,Float64* pFiberLength) const
{
   *ft_cri = m_ftcri;
   *ft_cr = m_ftcr;
   *ft_loc = m_ftloc;
   *et_loc = m_etloc;
   *alpha_u = m_alpha_u;
   *ecu = m_ecu;
   *pbExperimentalEcu = m_bExperimental_ecu;
   *pGammaU = m_gamma_u;
   *pFiberLength = m_FiberLength;
}

bool ConcreteLibraryEntry::UserACIParameters() const
{
   return m_bUserACIParameters;
}

void ConcreteLibraryEntry::UserACIParameters(bool bUser)
{
   m_bUserACIParameters = bUser;
}

Float64 ConcreteLibraryEntry::GetAlpha() const
{
   return m_A;
}

void ConcreteLibraryEntry::SetAlpha(Float64 a)
{
   m_A = a;
}

Float64 ConcreteLibraryEntry::GetBeta() const
{
   return m_B;
}

void ConcreteLibraryEntry::SetBeta(Float64 b)
{
   m_B = b;
}

pgsTypes::CureMethod ConcreteLibraryEntry::GetCureMethod() const
{
   return m_CureMethod;
}

void ConcreteLibraryEntry::SetCureMethod(pgsTypes::CureMethod cureMethod)
{
   m_CureMethod = cureMethod;
}

pgsTypes::ACI209CementType ConcreteLibraryEntry::GetACI209CementType() const
{
   return m_ACI209CementType;
}

void ConcreteLibraryEntry::SetACI209CementType(pgsTypes::ACI209CementType cementType)
{
   m_ACI209CementType = cementType;
}

bool ConcreteLibraryEntry::UserCEBFIPParameters() const
{
   return m_bUserCEBFIPParameters;
}

void ConcreteLibraryEntry::UserCEBFIPParameters(bool bUser)
{
   m_bUserCEBFIPParameters = bUser;
}

Float64 ConcreteLibraryEntry::GetS() const
{
   return m_S;
}

void ConcreteLibraryEntry::SetS(Float64 s)
{
   m_S = s;
}

Float64 ConcreteLibraryEntry::GetBetaSc() const
{
   return m_BetaSc;
}

void ConcreteLibraryEntry::SetBetaSc(Float64 betaSc)
{
   m_BetaSc = betaSc;
}

pgsTypes::CEBFIPCementType ConcreteLibraryEntry::GetCEBFIPCementType() const
{
   return m_CEBFIPCementType;
}

void ConcreteLibraryEntry::SetCEBFIPCementType(pgsTypes::CEBFIPCementType cementType)
{
   m_CEBFIPCementType = cementType;
}

//======================== INQUIRY    =======================================
HICON  ConcreteLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CONCRETE_ENTRY) );
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool ConcreteLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   CConcreteEntryDlg dlg(allowEditing);
   dlg.m_General.m_EntryName = this->GetName().c_str();
   dlg.m_General.m_Fc        = this->GetFc();
   dlg.m_General.m_Ds        = this->GetStrengthDensity();
   dlg.m_General.m_Dw        = this->GetWeightDensity();
   dlg.m_General.m_AggSize   = this->GetAggregateSize();
   dlg.m_General.m_bUserEc   = this->UserEc();
   dlg.m_General.m_Ec        = this->GetEc();
   dlg.m_General.m_Type      = this->GetType();

   GetPCIUHPC(&dlg.m_PCIUHPC.m_ffc, &dlg.m_PCIUHPC.m_frr, &dlg.m_PCIUHPC.m_FiberLength, &dlg.m_PCIUHPC.m_AutogenousShrinkage, &dlg.m_PCIUHPC.m_bPCTT);
   GetUHPC(&dlg.m_UHPC.m_ftcri, &dlg.m_UHPC.m_ftcr, &dlg.m_UHPC.m_ftloc, &dlg.m_UHPC.m_etloc, &dlg.m_UHPC.m_alpha_u,&dlg.m_UHPC.m_ecu,&dlg.m_UHPC.m_bExperimental_ecu,&dlg.m_UHPC.m_gamma_u,&dlg.m_UHPC.m_FiberLength);

   dlg.m_AASHTO.m_EccK1       = this->GetModEK1();
   dlg.m_AASHTO.m_EccK2       = this->GetModEK2();
   dlg.m_AASHTO.m_CreepK1     = this->GetCreepK1();
   dlg.m_AASHTO.m_CreepK2     = this->GetCreepK2();
   dlg.m_AASHTO.m_ShrinkageK1 = this->GetShrinkageK1();
   dlg.m_AASHTO.m_ShrinkageK2 = this->GetShrinkageK2();
   dlg.m_AASHTO.m_bHasFct     = this->HasAggSplittingStrength();
   dlg.m_AASHTO.m_Fct         = this->GetAggSplittingStrength();

   dlg.m_ACI.m_bUserParameters = this->UserACIParameters();
   dlg.m_ACI.m_A               = this->GetAlpha();
   dlg.m_ACI.m_B               = this->GetBeta();
   dlg.m_ACI.m_CureMethod      = this->GetCureMethod();
   dlg.m_ACI.m_CementType      = this->GetACI209CementType();

   dlg.m_CEBFIP.m_bUserParameters      = this->UserCEBFIPParameters();
   dlg.m_CEBFIP.m_S                    = this->GetS();
   dlg.m_CEBFIP.m_BetaSc               = this->GetBetaSc();
   dlg.m_CEBFIP.m_CementType           = this->GetCEBFIPCementType();

   INT_PTR i = dlg.DoModal();
   dlg.SetActivePage(nPage);
   if (i==IDOK)
   {
      SetName(dlg.m_General.m_EntryName);
      SetFc(dlg.m_General.m_Fc);
      SetStrengthDensity(dlg.m_General.m_Ds);
      SetWeightDensity(dlg.m_General.m_Dw );
      SetAggregateSize(dlg.m_General.m_AggSize);
      SetEc(dlg.m_General.m_Ec);
      UserEc(dlg.m_General.m_bUserEc);
      SetType(dlg.m_General.m_Type);

      SetModEK1(dlg.m_AASHTO.m_EccK1);
      SetModEK2(dlg.m_AASHTO.m_EccK2);
      SetCreepK1(dlg.m_AASHTO.m_CreepK1);
      SetCreepK2(dlg.m_AASHTO.m_CreepK2);
      SetShrinkageK1(dlg.m_AASHTO.m_ShrinkageK1);
      SetShrinkageK2(dlg.m_AASHTO.m_ShrinkageK2);
      HasAggSplittingStrength(dlg.m_AASHTO.m_bHasFct);
      SetAggSplittingStrength(dlg.m_AASHTO.m_Fct);

      SetPCIUHPC(dlg.m_PCIUHPC.m_ffc, dlg.m_PCIUHPC.m_frr, dlg.m_PCIUHPC.m_FiberLength, dlg.m_PCIUHPC.m_AutogenousShrinkage, dlg.m_PCIUHPC.m_bPCTT);
      SetUHPC(dlg.m_UHPC.m_ftcri, dlg.m_UHPC.m_ftcr, dlg.m_UHPC.m_ftloc, dlg.m_UHPC.m_etloc, dlg.m_UHPC.m_alpha_u,dlg.m_UHPC.m_ecu,dlg.m_UHPC.m_bExperimental_ecu,dlg.m_UHPC.m_gamma_u,dlg.m_UHPC.m_FiberLength);

      UserACIParameters(dlg.m_ACI.m_bUserParameters);
      SetAlpha(dlg.m_ACI.m_A);
      SetBeta(dlg.m_ACI.m_B);
      SetCureMethod(dlg.m_ACI.m_CureMethod);
      SetACI209CementType(dlg.m_ACI.m_CementType);

      UserCEBFIPParameters(dlg.m_CEBFIP.m_bUserParameters);
      SetS(dlg.m_CEBFIP.m_S);
      SetBetaSc(dlg.m_CEBFIP.m_BetaSc);
      SetCEBFIPCementType(dlg.m_CEBFIP.m_CementType);

      return true;
   }

   return false;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
