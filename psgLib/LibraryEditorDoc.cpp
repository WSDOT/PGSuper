///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <psglib\psglib.h>
#include <fstream>

#include <PsgLib\StructuredLoad.h>
#include <PsgLib\StructuredSave.h>
#include <psglib\LibraryEditorDoc.h>

#include <System\FileStream.h>
#include <System\StructuredLoadXmlPrs.h>
#include <System\StructuredSaveXmlPrs.h>


#include <WBFLCore.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFMainFrame.h>

#include "PGSuperLibraryMgrCATID.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// cause the resource control values to be defined
#define APSTUDIO_INVOKED
#undef APSTUDIO_READONLY_SYMBOLS

#include "resource.h"       // main symbols 

#define LIBRARY_PLUGIN_COMMAND_BASE 0xC000 // 49152 (this gives us about 8100 plug commands)
#if LIBRARY_PLUGIN_COMMAND_BASE < _APS_NEXT_COMMAND_VALUE
#error "Library Manager Plugins: Command IDs interfere with plug-in commands, change the plugin command base ID"
#endif

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
   m_pMyToolBar = NULL;

   // library editor doesn't use the status center
   CEAFStatusCenter& status_center = GetStatusCenter();
   status_center.Enable(false);


   // Set the base command ID for EAFDocumentPlugin objects
   GetPluginCommandManager()->SetBaseCommandID(LIBRARY_PLUGIN_COMMAND_BASE);
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
void CLibraryEditorDoc::DoIntegrateWithUI(BOOL bIntegrate)
{
   // Add the document's user interface stuff first
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   if ( bIntegrate )
   {
#if defined _EAF_USING_MFC_FEATURE_PACK
      // We want to use tabbed views
      pFrame->EnableMDITabs(TRUE,TRUE,CMFCTabCtrl::LOCATION_TOP,TRUE,CMFCTabCtrl::STYLE_3D_ROUNDED_SCROLL,FALSE,TRUE);
      CMFCTabCtrl& tabs = pFrame->GetMDITabs();
      tabs.SetActiveTabBoldFont();

      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      UINT tbID = pFrame->CreateToolBar(_T("Library"),GetPluginCommandManager());
      m_pMyToolBar = pFrame->GetToolBarByID(tbID);
      m_pMyToolBar->LoadToolBar(IDR_LIBEDITORTOOLBAR,NULL);
#else
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      UINT tbID = pFrame->CreateToolBar(_T("Library"),GetPluginCommandManager());
      m_pMyToolBar = pFrame->GetToolBar(tbID);
      m_pMyToolBar->LoadToolBar(IDR_LIBEDITORTOOLBAR,NULL);
      m_pMyToolBar->CreateDropDownButton(ID_FILE_OPEN,NULL,BTNS_DROPDOWN);
#endif
   }
   else
   {
      // remove toolbar here
      pFrame->DestroyToolBar(m_pMyToolBar);
      m_pMyToolBar = NULL;

#if defined _EAF_USING_MFC_FEATURE_PACK
      pFrame->EnableMDITabs(FALSE);
#endif
   }

   // then call base class, which handles UI integration for
   // plug-ins
   CEAFDocument::DoIntegrateWithUI(bIntegrate);
}

HRESULT CLibraryEditorDoc::WriteTheDocument(IStructuredSave* pStrSave)
{
   CStructuredSave mysave(pStrSave);

   try
   {
      // save editor-specific information
      CEAFApp* pApp = EAFGetApp();
      mysave.BeginUnit(_T("LIBRARY_EDITOR"), 1.0);
      if (pApp->GetUnitsMode() == eafTypes::umUS)
         mysave.Property(_T("EDIT_UNITS"), _T("US"));
      else
         mysave.Property(_T("EDIT_UNITS"), _T("SI"));

      // save library manager and all the library data
      m_LibraryManager.SaveMe(&mysave);

      mysave.EndUnit();
   }
   catch (const sysXStructuredSave& rXSave)
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
   eafTypes::UnitMode unitMode;
   HRESULT hr = pgslibLoadLibrary(pStrLoad,&m_LibraryManager,&unitMode);
   if ( FAILED(hr) )
      return hr;


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
      CEAFDocument::CloseDocumentRootNode(pStrLoad);

   return S_OK;  // always return S_OK
}

HRESULT CLibraryEditorDoc::OpenDocumentRootNode(IStructuredSave* pStrSave)
{
   // base class uses app name for unit name... 
   // set the state so it gets our name and not the main applications
   AFX_MANAGE_STATE(AfxGetStaticModuleState());  

   return CEAFDocument::OpenDocumentRootNode(pStrSave);
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
   CFileDialog  fildlg(TRUE,_T("pgs"),NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                   _T("PGSuper Project File (*.pgs)|*.pgs||"));
   INT_PTR stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      CString rPath;
      rPath = fildlg.GetPathName();

      CString real_file_name = rPath;

      // NOTE: This code was taken from PGSuper... At this time, the file conversion from the VB Convert
      // project does not effect the library entries. Therefore we can (a) skip this step, and (b) don't 
      // need to generalize the PGSuper file conversion routines
//      CString real_file_name; // name of actual file to be read may be different than lpszPathName
//      long convert_status = ConvertTheDocument(rPath, &real_file_name);
//      // convert document. if file was converted, then we need to delete the converted file at the end
//      if ( -1== convert_status)
//      {
//         HandleOpenDocumentError( STRLOAD_E_INVALIDFORMAT, rPath );
//         ASSERT(FALSE);
//      }
//      else
//      {
         // NOTE: Although it looks innocent, this control block is very important!! 
         // This is because the IStructuredLoad must be destroyed before the temp 
         // file can be deleted

         CComPtr<IStructuredLoad> pStrLoad;
         HRESULT hr = ::CoCreateInstance( CLSID_StructuredLoad, NULL, CLSCTX_INPROC_SERVER, IID_IStructuredLoad, (void**)&pStrLoad );
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
         hr = pStrLoad->BeginUnit(_T("PGSuper"));
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         double ver;
         pStrLoad->get_Version(&ver);
         if(ver < FILE_VERSION)
            return;

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
//      }
//
//      if (convert_status==1)
//      {
//         // file was converted and written to a temporary file. delete the temp file
//         CFile::Remove(real_file_name);
//      }
//
//
      SetModifiedFlag();
      UpdateAllViews(0,0);
   }
}

void CLibraryEditorDoc::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
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
