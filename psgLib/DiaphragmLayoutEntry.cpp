///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <psgLib\DiaphragmLayoutEntry.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   DiaphragmLayoutEntry
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================

//======================== OPERATIONS =======================================
bool DiaphragmLayoutEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("DiaphragmLayoutEntry"), 1.0);

   pSave->Property(_T("Name"),this->GetName().c_str());

   // layout
   for (DiaphragmLayoutVec::const_iterator it = m_DiaphragmLayoutVec.begin(); it!=m_DiaphragmLayoutVec.end(); it++)
   {
      pSave->BeginUnit(_T("DiaphragmLayout"), 1.0);
      pSave->Property(_T("EndOfRange"), (*it).EndOfRange);
      pSave->Property(_T("NumberOfDiaphragms"), (*it).NumberOfDiaphragms);
      pSave->EndUnit();
   }

   pSave->EndUnit();

   return false;
}

bool DiaphragmLayoutEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("DiaphragmLayoutEntry")))
   {
      if (pLoad->GetVersion()!=1.0)
         THROW_LOAD(BadVersion,pLoad);

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      DiaphragmLayout dl;
      m_DiaphragmLayoutVec.clear();
      while(pLoad->BeginUnit(_T("DiaphragmLayout")))
      {
         if(pLoad->GetVersion()!=1.0)
            THROW_LOAD(BadVersion,pLoad);

         if(!pLoad->Property(_T("EndOfRange"), &dl.EndOfRange))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("NumberOfDiaphragms"), &dl.NumberOfDiaphragms))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_DiaphragmLayoutVec.push_back(dl);
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else
   {
      return false; // not a dl entry
   }
   
   return true;
}

bool DiaphragmLayoutEntry::IsEqual(const DiaphragmLayoutEntry& rOther,bool bConsiderName) const
{
   std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool DiaphragmLayoutEntry::Compare(const DiaphragmLayoutEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{

   bMustRename = false;

   bool test =   m_DiaphragmLayoutVec == rOther.m_DiaphragmLayoutVec;

   if (considerName)
      test &= this->GetName()==rOther.GetName();

   return test;
}

HICON  DiaphragmLayoutEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DIAPHRAGM_ENTRY) );
}

//======================== ACCESS     =======================================

void DiaphragmLayoutEntry::SetDiaphragmLayout(const DiaphragmLayoutVec& vec)
{
   m_DiaphragmLayoutVec = vec;
}

DiaphragmLayoutEntry::DiaphragmLayoutVec DiaphragmLayoutEntry::GetDiaphragmLayout() const
{
   return m_DiaphragmLayoutVec;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool DiaphragmLayoutEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

//   // exchange data with dialog
//   CDiaphragmLayoutDlg dlg(mode, allowEditing);
//   dlg.m_DiaphragmLayoutVec  = this->m_DiaphragmLayoutVec;
//   dlg.m_Name = this->GetName().c_str();
//
//   int i = dlg.DoModal();
//   if (i==IDOK)
//   {
//      this->m_DiaphragmLayoutVec = dlg.m_DiaphragmLayoutVec;
//      this->SetName(dlg.m_Name);
//      return true;
//   }

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
