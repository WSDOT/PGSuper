///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Units\sysUnits.h>

#include <MathEx.h>
#include <Material\Concrete.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   ConcreteLibraryEntry
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
ConcreteLibraryEntry::ConcreteLibraryEntry() :
m_Fc(::ConvertToSysUnits(4.,unitMeasure::KSI)),
m_Ec(::ConvertToSysUnits(4200.,unitMeasure::KSI)),
m_bUserEc(false),
m_Ds(::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3)),
m_Dw(::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3)),
m_AggSize(::ConvertToSysUnits(0.75,unitMeasure::Inch)),
m_EccK1(1.0),
m_EccK2(1.0),
m_CreepK1(1.0),
m_CreepK2(1.0),
m_ShrinkageK1(1.0),
m_ShrinkageK2(1.0),
m_Type(pgsTypes::Normal),
m_Fct(0), // need a good default value
m_bHasFct(false),
m_bUserACIParameters(false),
m_A(::ConvertToSysUnits(4.0,unitMeasure::Day)),
m_B(0.85),
m_CureMethod(pgsTypes::Moist),
m_ACI209CementType(pgsTypes::TypeI),
m_CEBFIPCementType(pgsTypes::N)
{
}

ConcreteLibraryEntry::ConcreteLibraryEntry(const ConcreteLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

ConcreteLibraryEntry::~ConcreteLibraryEntry()
{
}

//======================== OPERATORS  =======================================
ConcreteLibraryEntry& ConcreteLibraryEntry::operator= (const ConcreteLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool ConcreteLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("ConcreteMaterialEntry"), 6.0);

   // Version 5, re-arranged data and added AASHTO and ACI groups.
   // Version 6, added CEB-FIP Concrete

   pSave->Property(_T("Name"),this->GetName().c_str());
   
   // added version 4
   pSave->Property(_T("Type"),matConcrete::GetTypeName((matConcrete::Type)m_Type,false).c_str());
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

   pSave->BeginUnit(_T("ACI"),1.0);
   pSave->Property(_T("UserACI"),m_bUserACIParameters);
   pSave->Property(_T("A"),m_A);
   pSave->Property(_T("B"),m_B);
   pSave->Property(_T("CureMethod"),m_CureMethod);
   pSave->Property(_T("CementType"),m_ACI209CementType);
   pSave->EndUnit(); // ACI

   // Added version 6
   pSave->BeginUnit(_T("CEBFIP"),1.0);
   pSave->Property(_T("CementType"),m_CEBFIPCementType);
   pSave->EndUnit(); // CEBFIP

   pSave->EndUnit();

   return false;
}

bool ConcreteLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("ConcreteMaterialEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (6.0 < version )
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
         m_Type = (pgsTypes::ConcreteType)matConcrete::GetTypeFromName(strType.c_str());
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

bool ConcreteLibraryEntry::IsEqual(const ConcreteLibraryEntry& rOther, bool considerName) const
{
   bool test =   m_Type == rOther.m_Type &&
                 ::IsEqual(m_Fc,      rOther.m_Fc)      &&
                 ::IsEqual(m_Ds,      rOther.m_Ds)      &&
                 ::IsEqual(m_Dw,      rOther.m_Dw)      &&
                 ::IsEqual(m_AggSize, rOther.m_AggSize ) &&
                 ::IsEqual(m_EccK1,   rOther.m_EccK1)      &&
                 ::IsEqual(m_EccK2,   rOther.m_EccK2)      &&
                 ::IsEqual(m_CreepK1,   rOther.m_CreepK1)      &&
                 ::IsEqual(m_CreepK2,   rOther.m_CreepK2)      &&
                 ::IsEqual(m_ShrinkageK1,   rOther.m_ShrinkageK1)      &&
                 ::IsEqual(m_ShrinkageK2,   rOther.m_ShrinkageK2)      &&
                 m_bUserEc == rOther.m_bUserEc &&
                 ::IsEqual(m_Ec,rOther.m_Ec);

   if ( m_Type != pgsTypes::Normal )
   {
      test &= (m_bHasFct == rOther.m_bHasFct);

      if ( m_bHasFct )
      {
         test &= ::IsEqual(m_Fct,rOther.m_Fct);
      }
   }

   test &= (m_bUserACIParameters == rOther.m_bUserACIParameters);
   if ( m_bUserACIParameters )
   {
      test &= ::IsEqual(m_A,rOther.m_A);
      test &= ::IsEqual(m_B,rOther.m_B);
   }
   else
   {
      test &= m_CureMethod   == rOther.m_CureMethod;
      test &= m_ACI209CementType   == rOther.m_ACI209CementType;
   }

   test &= m_CEBFIPCementType == rOther.m_CEBFIPCementType;


   if (considerName)
   {
      test &= this->GetName()==rOther.GetName();
   }

   return test;
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

   dlg.m_CEBFIP.m_CementType   = this->GetCEBFIPCementType();

   INT_PTR i = dlg.DoModal();
   dlg.SetActivePage(nPage);
   if (i==IDOK)
   {
      this->SetName(dlg.m_General.m_EntryName);
      this->SetFc(dlg.m_General.m_Fc);
      this->SetStrengthDensity(dlg.m_General.m_Ds);
      this->SetWeightDensity(dlg.m_General.m_Dw );
      this->SetAggregateSize(dlg.m_General.m_AggSize);
      this->SetEc(dlg.m_General.m_Ec);
      this->UserEc(dlg.m_General.m_bUserEc);
      this->SetType(dlg.m_General.m_Type);

      this->SetModEK1(dlg.m_AASHTO.m_EccK1);
      this->SetModEK2(dlg.m_AASHTO.m_EccK2);
      this->SetCreepK1(dlg.m_AASHTO.m_CreepK1);
      this->SetCreepK2(dlg.m_AASHTO.m_CreepK2);
      this->SetShrinkageK1(dlg.m_AASHTO.m_ShrinkageK1);
      this->SetShrinkageK2(dlg.m_AASHTO.m_ShrinkageK2);
      this->HasAggSplittingStrength(dlg.m_AASHTO.m_bHasFct);
      this->SetAggSplittingStrength(dlg.m_AASHTO.m_Fct);

      this->UserACIParameters(dlg.m_ACI.m_bUserParameters);
      this->SetAlpha(dlg.m_ACI.m_A);
      this->SetBeta(dlg.m_ACI.m_B);
      this->SetCureMethod(dlg.m_ACI.m_CureMethod);
      this->SetACI209CementType(dlg.m_ACI.m_CementType);

      this->SetCEBFIPCementType(dlg.m_CEBFIP.m_CementType);

      return true;
   }

   return false;
}


void ConcreteLibraryEntry::MakeCopy(const ConcreteLibraryEntry& rOther)
{
   m_Type    = rOther.m_Type;
   m_Fc      = rOther.m_Fc;
   m_Ds      = rOther.m_Ds;      
   m_Dw      = rOther.m_Dw;       
   m_AggSize = rOther.m_AggSize;
   m_bUserEc = rOther.m_bUserEc;
   m_Ec      = rOther.m_Ec;

   // AASHTO
   m_EccK1    = rOther.m_EccK1;
   m_EccK2    = rOther.m_EccK2;
   m_CreepK1  = rOther.m_CreepK1;
   m_CreepK2  = rOther.m_CreepK2;
   m_ShrinkageK1  = rOther.m_ShrinkageK1;
   m_ShrinkageK2  = rOther.m_ShrinkageK2;
   m_bHasFct  = rOther.m_bHasFct;
   m_Fct      = rOther.m_Fct;

   // ACI
   m_bUserACIParameters = rOther.m_bUserACIParameters;
   m_A                  = rOther.m_A;
   m_B                  = rOther.m_B;
   m_CureMethod         = rOther.m_CureMethod;
   m_ACI209CementType   = rOther.m_ACI209CementType;

   // CEB-FIP
   m_CEBFIPCementType = rOther.m_CEBFIPCementType;
}

void ConcreteLibraryEntry::MakeAssignment(const ConcreteLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
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
#if defined _DEBUG
bool ConcreteLibraryEntry::AssertValid() const
{
   return libLibraryEntry::AssertValid();
}

void ConcreteLibraryEntry::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for ConcreteLibraryEntry")<<endl;
   os << _T("   m_Fc =")<< m_Fc <<endl;
   os << _T("   m_Ec =")<< m_Ds << endl;
   os << _T("   m_D  =")<< m_Dw <<endl;
   os << _T("   m_AggSize ")<< m_AggSize <<endl;
   os << _T("   m_EccK1 ") << m_EccK1 << endl;
   os << _T("   m_EccK2 ") << m_EccK2 << endl;

   libLibraryEntry::Dump( os );
}
#endif // _DEBUG

#if defined _UNITTEST
bool ConcreteLibraryEntry::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("ConcreteLibraryEntry");

   // tests are performed on entire library.

   TESTME_EPILOG("ConcreteMaterial");
}
#endif // _UNITTEST
