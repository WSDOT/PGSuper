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

// psgLib.h : main header file for the PSGLIB DLL
//

#if !defined(AFX_PSGLIB_H__B76E130B_26EE_11D2_9D39_00609710E6CE__INCLUDED_)
#define AFX_PSGLIB_H__B76E130B_26EE_11D2_9D39_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#ifndef __AFXWIN_H__
//	#error include 'stdafx.h' before including this file for PCH
//#endif

#include "resource.h"		// main symbols


#include <psgLib\LibraryManager.h>
#include <WBFLCore.h>


// set help file location for dll.
bool PSGLIBFUNC WINAPI psglibSetHelpFileLocation(const std::string& rpath);

// simple function to get the first entry out of 
std::string PSGLIBFUNC WINAPI psglibGetFirstEntryName(const libILibrary& rlib);


// Fills pNames with the library entry names from the supplied libraries.
// Duplicate names are removed.
void PSGLIBFUNC WINAPI psglibCreateLibNameEnum( std::vector<std::string>* pNames, const libILibrary& prjLib);

// simple class for storing conflict resolutions
class PSGLIBCLASS ConflictList
{
public:
   // add a conflict for the given library 
   void AddConflict(const libILibrary& rLib,
                    const std::string& originalName, const std::string& newName)
   {
      ConflictData dat;
      dat.OriginalName = originalName;
      dat.NewName = newName;
      std::string type_name(rLib.GetDisplayName());
      m_Conflicts.insert(std::make_pair(type_name, dat));
   }

   // return true if a conflict exists for the given library and entry. 
   // Also return the new name and the library name if true
   bool IsConflict(const libILibrary& rLib, const std::string& entryName, std::string* pNewName) const
   {
      std::string type_name(rLib.GetDisplayName());
      std::pair <ConflictIterator,ConflictIterator> ip = m_Conflicts.equal_range(type_name);
      bool found = false;
      for (ConflictIterator it=ip.first; it!=ip.second; it++)
      {
         if (entryName == it->second.OriginalName)
         {
            *pNewName = it->second.NewName;
            return true;
         }
      }
      return false;
   }

   bool AreThereAnyConflicts() const
   {
      return !m_Conflicts.empty();
   }

private:
   struct ConflictData
   {
      std::string OriginalName;
      std::string NewName;
   };
   typedef std::multimap<std::string, ConflictData> ConflictContainer;
   typedef ConflictContainer::const_iterator ConflictIterator;
   ConflictContainer m_Conflicts;
};

enum LibConflictOutcome {Rename, OverWrite};

// resolve libary conflicts
bool PSGLIBFUNC WINAPI psglibDealWithLibraryConflicts(ConflictList* pList, psgLibraryManager* pMasterMgr, const psgLibraryManager& projectMgr,bool isImported,bool bForceUpdate);
bool PSGLIBFUNC WINAPI psglibMakeSaveableCopy(const psgLibraryManager& libMgr, psgLibraryManager* ptempManager);

LibConflictOutcome PSGLIBFUNC WINAPI psglibResolveLibraryEntryConflict(const std::string& entryName, const std::string& libName, const std::vector<std::string>& keylists, bool isImported,std::string* pNewName);

bool PSGLIBFUNC WINAPI psglibImportEntries(IStructuredLoad* pStrLoad,double version,psgLibraryManager* pLibMgr);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PSGLIB_H__B76E130B_26EE_11D2_9D39_00609710E6CE__INCLUDED_)
