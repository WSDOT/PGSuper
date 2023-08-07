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

// psgLib.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "resource.h"

#include <initguid.h>

#include "CLSID.h"

#include <psgLib\psgLib.h>
#include <psgLib\StructuredLoad.h>
#include <psgLib\LibraryEntryDifferenceItem.h>
#include "LibraryEntryConflict.h"

#include <WBFLGeometry_i.c>
#include <WBFLTools_i.c>
#include <Plugins\Beams.h>

#include <IFace\BeamFamily.h>
#include <IFace\BeamFactory.h>
#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "LibraryAppPlugin.h"

#include <BridgeLinkCATID.h>

#include "PGSuperLibraryMgrCATID.h"
#include <Plugins\BeamFactoryCATID.h>
#include <PGSuperCatCom.h>
#include <PGSpliceCatCom.h>


#include "dllmain.h"

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CPsgLibApp
// See psgLib.cpp for the implementation of this class
//

class CPsgLibApp : public CWinApp
{
public:
	CPsgLibApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPsgLibApp)
	public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPsgLibApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
   afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPsgLibApp

BEGIN_MESSAGE_MAP(CPsgLibApp, CWinApp)
	//{{AFX_MSG_MAP(CPsgLibApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPsgLibApp construction

CPsgLibApp::CPsgLibApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPsgLibApp object

CPsgLibApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPsgLibApp initialization

BOOL CPsgLibApp::InitInstance()
{
   // enable use of activeX controls
   AfxEnableControlContainer();

   // This call will initialize the grid library
	GXInit();

   WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSLibrary Editor Components"),CATID_PGSuperLibraryManagerPlugin);

   return CWinApp::InitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Special entry points required for inproc servers

int CPsgLibApp::ExitInstance() 
{
   GXForceTerminate();
	return CWinApp::ExitInstance();
}

void CPsgLibApp::OnHelp()
{
   // must have a handler for ID_HELP otherwise CDialog::InitDialog() will hide the help button
}

std::_tstring psglibGetFirstEntryName(const WBFL::Library::ILibrary& rlib)
{
   WBFL::Library::KeyListType key_list;
   rlib.KeyList(key_list);
   ATLASSERT(key_list.size()>0);
   return key_list[0];
}

template <class EntryType, class LibType>
bool do_deal_with_library_conflicts(ConflictList* pList, LibType* pMasterLib, const LibType& projectLib, const std::_tstring& publisher, const std::_tstring& configuration, const std::_tstring& libName, const EntryType& dummy, bool isImported,bool bForceUpdate)
{
   // loop over entries in project library and check to see if names are the same
   WBFL::Library::KeyListType project_keys;
   projectLib.KeyList(project_keys);
   // create a key list with all names in it for the sole purpose of dealing with 
   // name conflicts with newly added entries
   WBFL::Library::KeyListType master_keys;
   pMasterLib->KeyList(master_keys);
   master_keys.insert(master_keys.end(),project_keys.begin(),project_keys.end());

   for (WBFL::Library::KeyListIterator ik=project_keys.begin(); ik!=project_keys.end(); ik++)
   {
      const std::_tstring& name= *ik;
      const EntryType* pproject = 0;
      const EntryType* pmaster = pMasterLib->LookupEntry(name.c_str());
      if (pmaster!=0)
      {
         // name is the same, now compare contents
         pproject = projectLib.LookupEntry(name.c_str());
         ATLASSERT(pproject!=0);

         std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
         bool bMustRename = false;
         bool bSame = (bForceUpdate ? pmaster->IsEqual(*pproject) : pmaster->Compare(*pproject,vDifferences,bMustRename));
         if (!bSame)
         {
            // we have a conflict - ask user what he wants to do about it.
            if ( !bForceUpdate && vDifferences.size() == 0 )
            {
               // the library entry was lazy and didn't specify the exact nature of the conflict.
               // provide something to show in the conflict resolution dialog
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Unspecified conflicts"),_T(""),_T("")));
            }

            LibConflictOutcome res;
            std::_tstring new_name;

            if ( bForceUpdate )
            {
               res = OverWrite;
            }
            else
            {
               res = psglibResolveLibraryEntryConflict(publisher,configuration,name,libName,master_keys,isImported,vDifferences,bMustRename,&new_name);
            }

            std::for_each(vDifferences.begin(), vDifferences.end(), [](auto* pItem) {delete pItem; });
            vDifferences.clear();

            if (res==Rename)
            {
               // user wants to rename entry - add renamed entry to list
               if (!pMasterLib->AddEntry(*pproject, new_name.c_str(),false))
               {
                  return false;
               }

               master_keys.push_back(new_name);

               // now we have a conflict - need to save names so that we can update references
               pList->AddConflict(*pMasterLib, name, new_name);
            }
            else if (res==OverWrite)
            {
               // user wants to replace project entry with master - don't need to do anything
               pList->AddConflict(*pMasterLib, name, name);
            }
            else
            {
               ATLASSERT(false);
            }
         }
         pmaster->Release();
         pproject->Release();
      }
      else
      {
         // Project entry is totally unique. copy it into master library
         const EntryType* pproject = projectLib.LookupEntry(name.c_str());
         ATLASSERT(pproject!=0);
         if (!pMasterLib->AddEntry(*pproject, pproject->GetName().c_str(),false))
         {
            return false;
         }

         pproject->Release();
      }
   }

   return true;
}

bool psglibDealWithLibraryConflicts(ConflictList* pList, psgLibraryManager* pMasterMgr, const psgLibraryManager& projectMgr, bool isImported, bool bForceUpdate)
{
   // cycle through project library and see if entry names match master library. If the names
   // match and the entries are the same, no problem. If the entry names match and the entries are
   // different, we have a conflict

   std::_tstring strServer, strConfiguration, strLibFile;
   pMasterMgr->GetMasterLibraryInfo(strServer, strConfiguration, strLibFile);


     if (!do_deal_with_library_conflicts( pList, &(pMasterMgr->GetConcreteLibrary()), projectMgr.GetConcreteLibrary(), strServer, strConfiguration, _T("Concrete Library"), ConcreteLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, &(pMasterMgr->GetConnectionLibrary()), projectMgr.GetConnectionLibrary(), strServer, strConfiguration, _T("Connection Library"), ConnectionLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, &(pMasterMgr->GetGirderLibrary()),  projectMgr.GetGirderLibrary(), strServer, strConfiguration, _T("Girder Library"), GirderLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, &(pMasterMgr->GetDiaphragmLayoutLibrary()), projectMgr.GetDiaphragmLayoutLibrary(), strServer, strConfiguration, _T("Diaphragm Library"), DiaphragmLayoutEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, &(pMasterMgr->GetTrafficBarrierLibrary()), projectMgr.GetTrafficBarrierLibrary(), strServer, strConfiguration, _T("Traffic Barrier Library"), TrafficBarrierEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, pMasterMgr->GetSpecLibrary(), *(projectMgr.GetSpecLibrary()), strServer, strConfiguration, _T("Project Criteria Library"), SpecLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, pMasterMgr->GetLiveLoadLibrary(), *(projectMgr.GetLiveLoadLibrary()), strServer, strConfiguration, _T("User Defined Live Load Library"), LiveLoadLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, pMasterMgr->GetRatingLibrary(), *(projectMgr.GetRatingLibrary()), strServer, strConfiguration, _T("Rating Criteria Library"), RatingLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, pMasterMgr->GetDuctLibrary(), *(projectMgr.GetDuctLibrary()), strServer, strConfiguration, _T("Duct Library"), DuctLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

     if (!do_deal_with_library_conflicts( pList, pMasterMgr->GetHaulTruckLibrary(), *(projectMgr.GetHaulTruckLibrary()), strServer, strConfiguration, _T("Haul Truck Library"), HaulTruckLibraryEntry(),isImported,bForceUpdate))
     {
        return false;
     }

   return true;
}

bool do_make_saveable_copy(const WBFL::Library::ILibrary& lib, WBFL::Library::ILibrary* ptempLib)
{
   WBFL::Library::KeyListType key_list;
   lib.KeyList(key_list);
   for (WBFL::Library::KeyListIterator i = key_list.begin(); i!=key_list.end(); i++)
   {
      LPCTSTR key = i->c_str();
      // only copy entries to temp library if they are not read only, or if 
      // they are referenced
      if (lib.IsEditingEnabled(key) || lib.GetEntryRefCount(key)>0)
      {
         std::unique_ptr<WBFL::Library::LibraryEntry> pent(lib.CreateEntryClone(key));
         if (!ptempLib->AddEntry(*pent, key))
         {
            return false;
         }
      }
   }
   return true;
}


bool psglibMakeSaveableCopy(const psgLibraryManager& libMgr, psgLibraryManager* ptempManager)
{
   // concrete entries
   if (!do_make_saveable_copy(libMgr.GetConcreteLibrary(), &(ptempManager->GetConcreteLibrary())))
   {
      return false;
   }

   // connections
   if (!do_make_saveable_copy(libMgr.GetConnectionLibrary(), &(ptempManager->GetConnectionLibrary())))
   {
      return false;
   }

   // girders
   if (!do_make_saveable_copy(libMgr.GetGirderLibrary(), &(ptempManager->GetGirderLibrary())))
   {
      return false;
   }

   // diaphragms
   if (!do_make_saveable_copy(libMgr.GetDiaphragmLayoutLibrary(), &(ptempManager->GetDiaphragmLayoutLibrary())))
   {
      return false;
   }

   // traffic barriers
   if (!do_make_saveable_copy(libMgr.GetTrafficBarrierLibrary(), &(ptempManager->GetTrafficBarrierLibrary())))
   {
      return false;
   }

   // project criteria
   if (!do_make_saveable_copy( *(libMgr.GetSpecLibrary()), ptempManager->GetSpecLibrary()))
   {
      return false;
   }

   // user defined live load
   if (!do_make_saveable_copy( *(libMgr.GetLiveLoadLibrary()), ptempManager->GetLiveLoadLibrary()))
   {
      return false;
   }

   // rating specification
   if (!do_make_saveable_copy( *(libMgr.GetRatingLibrary()), ptempManager->GetRatingLibrary()))
   {
      return false;
   }

   // ducts
   if (!do_make_saveable_copy( *(libMgr.GetDuctLibrary()), ptempManager->GetDuctLibrary()))
   {
      return false;
   }

   // hauling trucks
   if (!do_make_saveable_copy( *(libMgr.GetHaulTruckLibrary()), ptempManager->GetHaulTruckLibrary()))
   {
      return false;
   }

   return true;
}

void psglibCreateLibNameEnum( std::vector<std::_tstring>* pNames, const WBFL::Library::ILibrary& prjLib)
{
   pNames->clear();

   // Get all the entires from the project library
   prjLib.KeyList( *pNames );
}

LibConflictOutcome psglibResolveLibraryEntryConflict(const std::_tstring& strPublisher, const std::_tstring& strConfiguration, const std::_tstring& entryName, const std::_tstring& libName, const std::vector<std::_tstring>& keylists, bool isImported,const std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences,bool bMustRename,std::_tstring* pNewName)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CLibraryEntryConflict dlg(strPublisher, strConfiguration, entryName,libName, keylists, isImported, vDifferences, bMustRename);
   dlg.DoModal();

   CLibraryEntryConflict::OutCome outcom = dlg.m_OutCome;
   LibConflictOutcome result;

   if (outcom==CLibraryEntryConflict::Rename)
   {
      *pNewName = std::_tstring(dlg.m_NewName);
      result = Rename;
   }
   else if (outcom==CLibraryEntryConflict::OverWrite)
   {
      result = OverWrite;
   }
   else
   {
      ATLASSERT(false);
   }

   CEAFDocument* pDoc = EAFGetDocument();
   pDoc->SetModifiedFlag();

   return result;
}

bool psglibImportEntries(IStructuredLoad* pStrLoad,psgLibraryManager* pLibMgr)
{
   // Load the library data into a temporary library. Then deal with entry
   // conflict resolution.
   psgLibraryManager temp_manager;
   try
   {
      CStructuredLoad load( pStrLoad );
      temp_manager.LoadMe( &load );
   }
   catch(...)
   {
      return false;
   }

   // merge project library into master library and deal with conflicts
   ConflictList the_conflict_list;
   if (!psglibDealWithLibraryConflicts(&the_conflict_list, pLibMgr, temp_manager,true,false))
   {
      return false;
   }

   return true;
}

HRESULT pgslibReadProjectDocHeader(LPCTSTR lpszRootNodeName,IStructuredLoad* pStrLoad)
{
   HRESULT hr = pStrLoad->BeginUnit(lpszRootNodeName);
   if ( FAILED(hr) )
   {
      return hr;
   }

   Float64 ver;
   pStrLoad->get_Version(&ver);

   if ( 1.0 < ver )
   {
      CComVariant var;
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Version"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pStrLoad->BeginUnit(_T("Broker"));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pStrLoad->BeginUnit(_T("Agent"));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pStrLoad->get_Property(_T("CLSID"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }
   }

   return S_OK;
}

HRESULT pgslibReadLibraryDocHeader(IStructuredLoad* pStrLoad,eafTypes::UnitMode* pUnitsMode)
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString strAppUnit = AfxGetApp()->m_pszAppName;
   strAppUnit.Trim();
   strAppUnit.Replace(_T(" "),_T(""));
   pStrLoad->BeginUnit(strAppUnit); // it is ok if this fails as not all documents have this unit

   HRESULT hr = pStrLoad->BeginUnit(_T("LIBRARY_EDITOR"));
   if ( FAILED(hr) )
   {
      return hr;
   }

   Float64 ver;
   pStrLoad->get_Version(&ver);
   if (ver!=1.0)
   {
      return E_FAIL; // bad version
   }

   // editor units
   CComVariant var;
   var.vt = VT_BSTR;
   hr = pStrLoad->get_Property(_T("EDIT_UNITS"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   std::_tstring str(OLE2T(var.bstrVal));

   if (str==_T("US"))
   {
      *pUnitsMode = eafTypes::umUS;
   }
   else
   {
      *pUnitsMode = eafTypes::umSI;
   }

   return S_OK;
}

HRESULT pgslibLoadLibrary(LPCTSTR strFileName,psgLibraryManager* pLibMgr,eafTypes::UnitMode* pUnitMode, bool bIsMasterLibrary)
{
   CComPtr<IStructuredLoad> pStrLoad;
   pStrLoad.CoCreateInstance(CLSID_StructuredLoad);
   HRESULT hr = pStrLoad->Open(strFileName);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr =  pgslibLoadLibrary(pStrLoad,pLibMgr,pUnitMode, bIsMasterLibrary);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pStrLoad->Close();
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}

HRESULT pgslibLoadLibrary(IStructuredLoad* pStrLoad,psgLibraryManager* pLibMgr,eafTypes::UnitMode* pUnitMode, bool bIsMasterLibrary)
{
   try
   {
      CStructuredLoad load(pStrLoad);

      // clear out library and load up new
      pLibMgr->ClearAllEntries();

      if (bIsMasterLibrary)
      {
         if (FAILED(pgslibReadLibraryDocHeader(pStrLoad, pUnitMode)))
         {
            THROW_LOAD(InvalidFileFormat, (&load));
         }
      }

      // load the library 
      pLibMgr->LoadMe(&load);

      if (bIsMasterLibrary)
      {
         pStrLoad->EndUnit(); // _T("LIBRARY_EDITOR")
      }
   }
   catch (const WBFL::System::XStructuredLoad& rLoad)
   {
      WBFL::System::XStructuredLoad::Reason reason = rLoad.GetReasonCode();
      CString cmsg;
      if (bIsMasterLibrary)
      {
         cmsg = _T("Error loading Master Library\n\n");
      }
      if (reason == WBFL::System::XStructuredLoad::InvalidFileFormat)
      {
         cmsg += _T("Invalid file format. The file may have been corrupted. Extended error information is as follows: ");
      }
      else if (reason == WBFL::System::XStructuredLoad::BadVersion)
      {
         cmsg += _T("Data file was written by a newer program version. Please upgrade this software. Extended error information is as follows: ");
      }
      else if (reason == WBFL::System::XStructuredLoad::UserDefined)
      {
         //cmsg = _T("Error reading file. Extended error information is as follows:");
      }
      else
      {
         cmsg += _T("Undetermined error reading data file.  Extended error information is as follows: ");
      }

      std::_tstring msg = rLoad.GetErrorMessage();
      cmsg += msg.c_str();

      AfxMessageBox(cmsg, MB_OK | MB_ICONEXCLAMATION);
      return E_FAIL;
   }
   catch (...)
   {
      return E_FAIL;
   }

   return S_OK;
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _AtlModule.GetLockCount()==0) ? S_OK : S_FALSE;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

HRESULT RegisterComponents(bool bRegister)
{
   HRESULT hr = S_OK;

   // Need to register the library application plugin with the PGSuperAppPlugin category
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_LibraryAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();

    hr = RegisterComponents(true);


   return S_OK;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
   HRESULT hr = _AtlModule.DllUnregisterServer();
   hr = RegisterComponents(false);

   return hr;
}
