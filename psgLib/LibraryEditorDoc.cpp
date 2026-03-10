///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2025  Washington State Department of Transportation
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

// LibraryEditorDoc.cpp : implementation of the CLibraryEditorDoc class
//

#include "stdafx.h"
#include <PsgLib\PsgLib.h>
#include <fstream>

#include <PsgLib\StructuredLoad.h>
#include <PsgLib\StructuredSave.h>
#include <PsgLib\LibraryEditorDoc.h>
#include <PsgLib\BeamFamilyManager.h>

#include <System\FileStream.h>
#include <System\StructuredLoadXml.h>
#include <System\StructuredSaveXml.h>




#include <EAF\EAFApp.h>
#include <EAF\EAFMainFrame.h>

#include "PGSuperLibraryMgrCATID.h"
#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"

#include "LibraryEditorStatusBar.h"





// cause the resource control values to be defined
#define APSTUDIO_INVOKED
#undef APSTUDIO_READONLY_SYMBOLS

#include "resource.h"       // main symbols 

#define LIBRARY_PLUGIN_COMMAND_COUNT 256

static const Float64 FILE_VERSION=1.00;

#define ID_LIBEDITORTOOLBAR ID_MAINFRAME_TOOLBAR+1

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorDoc

IMPLEMENT_DYNCREATE(CLibraryEditorDoc, CEAFDocument)

BEGIN_MESSAGE_MAP(CLibraryEditorDoc, CEAFDocument)
	//{{AFX_MSG_MAP(CLibraryEditorDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_IMPORT, OnImport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorDoc construction/destruction

CLibraryEditorDoc::CLibraryEditorDoc()
{
   m_LibraryManager.SetName(_T("Master Libraries"));

   // library editor doesn't use the status center
   auto& status_center = GetStatusCenter();
   status_center.Enable(false);

   // Reserve command IDs for document plug ins
   UINT nCommands = GetPluginCommandManager()->ReserveCommandIDRange(LIBRARY_PLUGIN_COMMAND_COUNT);
   ATLASSERT(nCommands == LIBRARY_PLUGIN_COMMAND_COUNT);

   EnableUIHints(FALSE); // not using UIHints feature
}

CLibraryEditorDoc::~CLibraryEditorDoc()
{
}

CString CLibraryEditorDoc::GetToolbarSectionName()
{
   return CString(_T("LibraryEditor Toolbars"));
}

CATID CLibraryEditorDoc::GetDocumentPluginCATID()
{
   // all dynamically created plugins used with this document type must
   // belong to the CATID_PGSuperLibraryManagerPlugin category
   return CATID_PGSuperLibraryManagerPlugin;
}

HINSTANCE CLibraryEditorDoc::GetResourceInstance()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetInstanceHandle();
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorDoc diagnostics

#ifdef _DEBUG
void CLibraryEditorDoc::AssertValid() const
{
	CEAFDocument::AssertValid();
}

void CLibraryEditorDoc::Dump(CDumpContext& dc) const
{
	CEAFDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorDoc commands
BOOL CLibraryEditorDoc::Init()
{
   if ( !__super::Init() )
   {
      return FALSE;
   }

   if ( FAILED(PGS::Library::BeamFamilyManager::Init(CATID_PGSuperBeamFamily)) )
   {
      return FALSE;
   }

   if ( FAILED(PGS::Library::BeamFamilyManager::Init(CATID_PGSpliceBeamFamily)) )
   {
      return FALSE;
   }

   return TRUE;
}

void CLibraryEditorDoc::DoIntegrateWithUI(BOOL bIntegrate)
{
   __super::DoIntegrateWithUI(bIntegrate);

   CEAFMainFrame* pFrame = EAFGetMainFrame();

   if ( bIntegrate )
   {
      // set up the toolbar here
      {
         AFX_MANAGE_STATE(AfxGetStaticModuleState());
         UINT tbID = pFrame->CreateToolBar(_T("Library"), GetPluginCommandManager());
         m_MyToolBar = pFrame->GetToolBar(tbID);
         m_MyToolBar->LoadToolBar(IDR_LIBEDITORTOOLBAR, nullptr);
         m_MyToolBar->CreateDropDownButton(ID_FILE_OPEN, nullptr, BTNS_DROPDOWN);
      }

      // use our status bar
      CLibraryEditorStatusBar* pSB = new CLibraryEditorStatusBar;
      pSB->Create(pFrame);
      pFrame->SetStatusBar(pSB);
   }
   else
   {
      // remove toolbar here
      pFrame->DestroyToolBar(m_MyToolBar->GetToolBarID());
      m_MyToolBar = nullptr;

      // reset the status bar
      pFrame->SetStatusBar(nullptr);
   }
}

HRESULT CLibraryEditorDoc::WriteTheDocument(IStructuredSave* pStrSave)
{
   CStructuredSave mysave(pStrSave);

   try
   {
      // save editor-specific information
      CEAFApp* pApp = EAFGetApp();
      mysave.BeginUnit(_T("LIBRARY_EDITOR"), 1.0);
      if (pApp->GetUnitsMode() == WBFL::EAF::UnitMode::US)
      {
         mysave.Property(_T("EDIT_UNITS"), _T("US"));
      }
      else
      {
         mysave.Property(_T("EDIT_UNITS"), _T("SI"));
      }

      // save library manager and all the library data
      m_LibraryManager.SaveMe(&mysave);

      mysave.EndUnit();
   }
   catch (const WBFL::System::XStructuredSave& rXSave)
   {
      rXSave; // unused
      AfxMessageBox(_T("Error saving library data. You may have a full hard disk"),MB_OK|MB_ICONEXCLAMATION);
      return E_FAIL;
   }
	
   // document is now clean
   SetModifiedFlag(FALSE);

   return S_OK;
}

HRESULT CLibraryEditorDoc::LoadTheDocument(IStructuredLoad* pStrLoad)
{
   WBFL::EAF::UnitMode unitMode;
   HRESULT hr = pgslibLoadLibrary(pStrLoad,&m_LibraryManager,&unitMode);
   if ( FAILED(hr) )
   {
      return hr;
   }


   CEAFApp* pApp = EAFGetApp();
   pApp->SetUnitsMode(unitMode);
	
   m_LibraryManager.EnableEditingForAllEntries(true);

   SetModifiedFlag(FALSE);

   return S_OK;
}

HRESULT CLibraryEditorDoc::OpenDocumentRootNode(IStructuredLoad* pStrLoad)
{
   // base class uses app name for unit name... 
   // set the state so it gets our name and not the main applications
   AFX_MANAGE_STATE(AfxGetStaticModuleState());  

   // it is ok if this fails... before moving to the EAF implementation and persisting
   // plug-in data, there was not a structured storage unit that wrapped the entire file
   m_hrOpenRootNode = CEAFDocument::OpenDocumentRootNode(pStrLoad);
   return S_OK;  // always return S_OK
}

HRESULT CLibraryEditorDoc::CloseDocumentRootNode(IStructuredLoad* pStrLoad)
{
   // it is ok if this fails... before moving to the EAF implementation and persisting
   // plug-in data, there was not a structured storage unit that wrapped the entire file
   
   // only call the base-class to EndUnit of the unit was opened
   if ( m_hrOpenRootNode == S_OK )
   {
      CEAFDocument::CloseDocumentRootNode(pStrLoad);
   }

   return S_OK;  // always return S_OK
}

HRESULT CLibraryEditorDoc::OpenDocumentRootNode(IStructuredSave* pStrSave)
{
   // base class uses app name for unit name... 
   // set the state so it gets our name and not the main applications
   AFX_MANAGE_STATE(AfxGetStaticModuleState());  

   return CEAFDocument::OpenDocumentRootNode(pStrSave);
}

void CLibraryEditorDoc::LoadDocumentSettings()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::LoadDocumentSettings();
}

void CLibraryEditorDoc::SaveDocumentSettings()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::SaveDocumentSettings();
}

CString CLibraryEditorDoc::GetDocumentationRootLocation()
{
   CEAFApp* pApp = EAFGetApp();
   return pApp->GetDocumentationRootLocation();
}

CString CLibraryEditorDoc::GetRootNodeName()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString strAppUnit = AfxGetApp()->m_pszAppName;
   strAppUnit.Trim();
   strAppUnit.Replace(_T(" "),_T(""));

   return strAppUnit;
}

void CLibraryEditorDoc::OnImport()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// ask user for file name
   CFileDialog  fildlg(TRUE,_T("pgs"),nullptr,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                   _T("PGSuper Project File (*.pgs)|*.pgs|PGSplice Project File (*.spl)|*.spl||"));
   INT_PTR stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      CString rPath;
      rPath = fildlg.GetPathName();

      CString real_file_name = rPath;

      {
         // NOTE: Although it looks innocent, this control block is very important!! 
         // This is because the IStructuredLoad must be destroyed before the temp 
         // file can be deleted

         CComPtr<IStructuredLoad> pStrLoad;
         HRESULT hr = ::CoCreateInstance( CLSID_StructuredLoad, nullptr, CLSCTX_INPROC_SERVER, IID_IStructuredLoad, (void**)&pStrLoad );
         if ( FAILED(hr) )
         {
            // We are not aggregating so we should CoCreateInstance should
            // never fail with this HRESULT
            ASSERT( hr != CLASS_E_NOAGGREGATION );

            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         hr = pStrLoad->Open( real_file_name );
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         // advance the structured load pointer to the correct point for agent
         HRESULT hrPGSuper  = pStrLoad->BeginUnit(_T("PGSuper"));
         HRESULT hrPGSplice = pStrLoad->BeginUnit(_T("PGSplice"));
         if ( FAILED(hrPGSuper) && FAILED(hrPGSplice) )
         {
            HandleOpenDocumentError( hrPGSuper, rPath );
            ASSERT(FALSE);
         }

         Float64 ver;
         pStrLoad->get_Version(&ver);
         if(ver < FILE_VERSION)
         {
            return;
         }

         if ( 1.0 < ver )
         {
            CComVariant var;
            var.vt = VT_BSTR;
            if ( FAILED(pStrLoad->get_Property(_T("Version"),&var)) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }

            if ( FAILED(pStrLoad->BeginUnit(_T("Broker"))) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }

            if ( FAILED(pStrLoad->BeginUnit(_T("Agent"))) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }

            if ( FAILED(pStrLoad->get_Property(_T("CLSID"),&var)) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }
         }

         bool bOK = psglibImportEntries(pStrLoad,&m_LibraryManager);
         if ( !bOK )
         {
            HandleOpenDocumentError( E_FAIL, rPath );
            ASSERT(FALSE);
         }

         hr = pStrLoad->Close();
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }
      }

      SetModifiedFlag();
      UpdateAllViews(0,0);
   }
}

void CLibraryEditorDoc::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      msg1.LoadString( IDS_E_BADINSTALL );
      break;

   case STRLOAD_E_CANTOPEN:
      AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      break;

   case STRLOAD_E_FILENOTFOUND:
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRLOAD_E_INVALIDFORMAT:
      AfxFormatString1( msg1, IDS_E_INVALIDFORMAT, lpszPathName );
      break;

   case STRLOAD_E_BADVERSION:
      AfxFormatString1( msg1, IDS_E_INVALIDVERSION, lpszPathName );
      break;

   case STRLOAD_E_USERDEFINED:
      AfxFormatString1( msg1, IDS_E_USERDEFINED, lpszPathName );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while opening the file (hr = %d)"),hr);
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   AfxMessageBox( msg1 );
}

void CLibraryEditorDoc::HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      msg1.LoadString( IDS_E_BADINSTALL );
      break;

   case STRSAVE_E_CANTOPEN:
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRSAVE_E_BADWRITE:
      AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while closing the file (hr = %d)"),hr);
         AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      }
      break;
   }

   AfxMessageBox( msg1 );
}

BOOL CLibraryEditorDoc::GetStatusBarMessageString(UINT nID,CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return __super::GetStatusBarMessageString(nID,rMessage);
}

BOOL CLibraryEditorDoc::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return __super::GetToolTipMessageString(nID,rMessage);
}
