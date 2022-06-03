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

#include "StdAfx.h"
#include <psgLib\DuctLibraryEntry.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include "DuctEntryDlg.h"
#include <Units\sysUnits.h>

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DuctLibraryEntry::DuctLibraryEntry() :
m_OD(0),
m_ID(0),
m_ND(0),
m_Z(0)
{
}

DuctLibraryEntry::DuctLibraryEntry(const DuctLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

DuctLibraryEntry::~DuctLibraryEntry()
{
}

//======================== OPERATORS  =======================================
DuctLibraryEntry& DuctLibraryEntry::operator= (const DuctLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool DuctLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("DuctEntry"), 2.0);

   pSave->Property(_T("Name"),this->GetName().c_str());
   
   pSave->Property(_T("OD"), m_OD);
   pSave->Property(_T("ID"), m_ID);
   pSave->Property(_T("ND"), m_ND); // added in version 2
   pSave->Property(_T("Z"),  m_Z);
   pSave->EndUnit();

   return false;
}

bool DuctLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("DuctEntry")))
   {
      Float64 version = pLoad->GetVersion();

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
      {
         this->SetName(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("OD"), &m_OD))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("ID"), &m_ID))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (1 < version)
      {
         if (!pLoad->Property(_T("ND"), &m_ND))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         m_ND = m_OD;
      }

      if(!pLoad->Property(_T("Z"), &m_Z))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->EndUnit())
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      return false; // not a duct entry
   }
   
   return true;
}

bool DuctLibraryEntry::IsEqual(const DuctLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool DuctLibraryEntry::Compare(const DuctLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   bMustRename = false;

   if ( !::IsEqual(m_OD,rOther.m_OD) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("OD"),m_OD,rOther.m_OD,pDisplayUnits->ComponentDim));
   }

   if (!::IsEqual(m_ID, rOther.m_ID))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("ID"), m_ID, rOther.m_ID, pDisplayUnits->ComponentDim));
   }

   if (!::IsEqual(m_ND, rOther.m_ND))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("Nominal Diameter"), m_ND, rOther.m_ND, pDisplayUnits->ComponentDim));
   }

   if ( !::IsEqual(m_Z,rOther.m_Z) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("Z"),m_Z,rOther.m_Z,pDisplayUnits->ComponentDim));
   }

   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }

   return vDifferences.size() == 0 ? true : false;
}


void DuctLibraryEntry::SetOD(Float64 od)
{
   m_OD = od;
}

Float64 DuctLibraryEntry::GetOD() const
{
   return m_OD;
}

void DuctLibraryEntry::SetID(Float64 id)
{
   m_ID = id;
}

Float64 DuctLibraryEntry::GetID() const
{
   return m_ID;
}

void DuctLibraryEntry::SetNominalDiameter(Float64 nd)
{
   m_ND = nd;
}

Float64 DuctLibraryEntry::GetNominalDiameter() const
{
   return m_ND;
}

Float64 DuctLibraryEntry::GetInsideArea() const
{
   return M_PI*m_ID*m_ID/4;
}

HICON DuctLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DUCT_ENTRY) );
}

bool DuctLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDuctEntryDlg dlg(allowEditing);
   dlg.m_Name = CString(GetName().c_str());
   dlg.m_OD = m_OD;
   dlg.m_ID = m_ID;
   dlg.m_ND = m_ND;
   dlg.m_Z  = m_Z;

   INT_PTR i = dlg.DoModal();
   if (i==IDOK)
   {
      m_OD = dlg.m_OD;
      m_ID = dlg.m_ID;
      m_ND = dlg.m_ND;
      m_Z  = dlg.m_Z;
      return true;
   }

   return false;
}

void DuctLibraryEntry::MakeCopy(const DuctLibraryEntry& rOther)
{
   m_OD = rOther.m_OD;
   m_ID = rOther.m_ID;
   m_ND = rOther.m_ND;
   m_Z  = rOther.m_Z;
}

void DuctLibraryEntry::MakeAssignment(const DuctLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}
