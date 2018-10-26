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

#include "StdAfx.h"
#include <psgLib\ConcreteLibraryEntry.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include "ConcreteEntryDlg.h"
#include <Units\sysUnits.h>

#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
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
m_K1(1.0)
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
   pSave->BeginUnit("ConcreteMaterialEntry", 3.0);

   pSave->Property("Name",this->GetName().c_str());
   pSave->Property("Dw", m_Dw);
   pSave->Property("Fc", m_Fc);
   pSave->Property("Ds", m_Ds);
   pSave->Property("AggregateSize",m_AggSize);
   pSave->Property("K1", m_K1);
   pSave->Property("UserEc",m_bUserEc);
   pSave->Property("Ec",m_Ec);

   pSave->EndUnit();

   return false;
}

bool ConcreteLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit("ConcreteMaterialEntry"))
   {
      if (3.0 < pLoad->GetVersion() )
         THROW_LOAD(BadVersion,pLoad);

      std::string name;
      if(pLoad->Property("Name",&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("Dw", &m_Dw))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("Fc", &m_Fc))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("Ds", &m_Ds))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("AggregateSize", &m_AggSize))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 2.0 <= pLoad->GetVersion() )
      {
         if ( !pLoad->Property("K1",&m_K1) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         m_K1 = 1.0;
      }

      if ( 3.0 <= pLoad->GetVersion() )
      {
         if ( !pLoad->Property("UserEc",&m_bUserEc) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("Ec",&m_Ec) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else
      return false; // not a concrete entry
   
   return true;
}

bool ConcreteLibraryEntry::IsEqual(const ConcreteLibraryEntry& rOther, bool considerName) const
{
   bool test =   ::IsEqual(m_Fc,      rOther.m_Fc)      &&
                 ::IsEqual(m_Ds,      rOther.m_Ds)      &&
                 ::IsEqual(m_Dw,      rOther.m_Dw)      &&
                 ::IsEqual(m_AggSize, rOther.m_AggSize ) &&
                 ::IsEqual(m_K1,      rOther.m_K1)      &&
                 m_bUserEc == rOther.m_bUserEc &&
                 ::IsEqual(m_Ec,rOther.m_Ec);

   if (considerName)
      test &= this->GetName()==rOther.GetName();

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

void ConcreteLibraryEntry::SetK1(Float64 k1)
{
   m_K1 = k1;
}

Float64 ConcreteLibraryEntry::GetK1() const
{
   return m_K1;
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
bool ConcreteLibraryEntry::Edit(libUnitsMode::Mode mode, bool allowEditing)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   CConcreteEntryDlg dlg(mode, allowEditing);
   dlg.m_Fc  = this->GetFc();
   dlg.m_Ds  = this->GetStrengthDensity();
   dlg.m_Dw  = this->GetWeightDensity();
   dlg.m_AggSize = this->GetAggregateSize();
   dlg.m_EntryName    = this->GetName().c_str();
   dlg.m_K1 = this->GetK1();
   dlg.m_bUserEc = this->UserEc();
   dlg.m_Ec = this->GetEc();

   int i = dlg.DoModal();
   if (i==IDOK)
   {
      this->SetFc(dlg.m_Fc);
      this->SetStrengthDensity(dlg.m_Ds);
      this->SetWeightDensity(dlg.m_Dw );
      this->SetAggregateSize(dlg.m_AggSize);
      this->SetName(dlg.m_EntryName);
      this->SetK1(dlg.m_K1);
      this->SetEc(dlg.m_Ec);
      this->UserEc(dlg.m_bUserEc);
      return true;
   }

   return false;
}


void ConcreteLibraryEntry::MakeCopy(const ConcreteLibraryEntry& rOther)
{
   m_Fc      = rOther.m_Fc;
   m_Ds      = rOther.m_Ds;      
   m_Dw      = rOther.m_Dw;       
   m_AggSize = rOther.m_AggSize;
   m_K1      = rOther.m_K1;
   m_bUserEc = rOther.m_bUserEc;
   m_Ec      = rOther.m_Ec;
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
   os << "Dump for ConcreteLibraryEntry"<<endl;
   os << "   m_Fc ="<< m_Fc <<endl;
   os << "   m_Ec ="<< m_Ds << endl;
   os << "   m_D  ="<< m_Dw <<endl;
   os << "   m_AggSize "<< m_AggSize <<endl;
   os << "   m_K1 " << m_K1 << endl;

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
