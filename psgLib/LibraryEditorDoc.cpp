///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
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

// LibraryEditorDoc.cpp : implementation of the CLibraryEditorDoc class
//

#include "stdafx.h"
#include <psglib\psglib.h>
#include <fstream>

#include <PsgLib\StructuredLoad.h>
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
   m_LibraryManager.SetName("Master Libraries");
   m_pMyToolBar = NULL;

   // library editor doesn't use the status center
   CEAFStatusCenter& status_center = GetStatusCenter();
   status_center.Enable(false);
}

CLibraryEditorDoc::~CLibraryEditorDoc()
{
}

CString CLibraryEditorDoc::GetToolbarSectionName()
{
   return CString("LibraryEditor Toolbars");
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
   CEAFMainFrame* pFrame = (CEAFMainFrame*)AfxGetMainWnd();
   if ( bIntegrate )
   {
      // set up the toolbar here
      UINT tbID = pFrame->CreateToolBar("Library",GetPluginCommandManager());
      m_pMyToolBar = pFrame->GetToolBar(tbID);
      m_pMyToolBar->LoadToolBar(IDR_LIBEDITORTOOLBAR,NULL);
   }
   else
   {
      // remove toolbar here
      pFrame->DestroyToolBar(m_pMyToolBar);
      m_pMyToolBar = NULL;
   }

   // then call base class, which handles UI integration for
   // plug-ins
   CEAFDocument::DoIntegrateWithUI(bIntegrate);
}

BOOL CLibraryEditorDoc::OpenTheDocument(LPCTSTR lpszPathName)
{
   // Skip this, we are doing it our own way
   //if ( !CEAFDocument::OpenTheDocument(lpszPathName) )
   //   return FALSE;
   CComPtr<IStructuredLoad> pStrLoad;
   pStrLoad.CoCreateInstance(CLSID_StructuredLoad);
   CStructuredLoad load( pStrLoad );

   HRESULT hr = pStrLoad->Open(lpszPathName);
   if ( SUCCEEDED(hr) )
   {
      try
      {
         // clear out library and load up new
	      m_LibraryManager.ClearAllEntries();

         eafTypes::UnitMode unitMode;
         if ( FAILED(pgslibReadLibraryDocHeader(pStrLoad,&unitMode)) )
            THROW_LOAD(InvalidFileFormat,(&load));

         CEAFApp* pApp = (CEAFApp*)AfxGetApp();
         pApp->SetUnitsMode(unitMode);

         // load the library 
         m_LibraryManager.LoadMe(&load);

         if ( FAILED(pStrLoad->EndUnit()) )
            THROW_LOAD(InvalidFileFormat,(&load));
      }
      catch (const sysXStructuredLoad& rLoad)
      {
         sysXStructuredLoad::Reason reason = rLoad.GetExplicitReason();
         std::string msg;
         CString cmsg;
         rLoad.GetErrorMessage(&msg);
         if (reason==sysXStructuredLoad::InvalidFileFormat)
            cmsg = "Invalid file data format. The file may have been corrupted. Extended error information is as follows: ";
         else if (reason==sysXStructuredLoad::BadVersion)
            cmsg = "Data file was written by a newer program version. Please upgrade this software. Extended error information is as follows: ";
         else if ( reason == sysXStructuredLoad::UserDefined )
            cmsg = "Error reading file. Extended error information is as follows:";
         else
            cmsg = "Undetermined error reading data file.  Extended error information is as follows: ";

         cmsg += msg.c_str();
         AfxMessageBox(cmsg,MB_OK | MB_ICONEXCLAMATION);
         return FALSE;
      }
   }
   else
      return FALSE;
	
   m_LibraryManager.EnableEditingForAllEntries(true);

   SetModifiedFlag(FALSE);

   return TRUE;
}

BOOL CLibraryEditorDoc::SaveTheDocument(LPCTSTR lpszPathName)
{
   // We are using our own method for saving library documents.
   // By-pass the base class method
	//if (!CEAFDocument::SaveTheDocument(lpszPathName))
	//	return FALSE;
	
   HANDLE hFile;
   WIN32_FIND_DATA find_file_data;
   hFile = ::FindFirstFile( lpszPathName, &find_file_data );
   if ( hFile != INVALID_HANDLE_VALUE )
   {
      ::FindClose(hFile); // don't want no stinkin resource leaks.
      // OK, The file exists.
      // check to make sure it's not read-only
	   DWORD dwAttrib = GetFileAttributes(lpszPathName);
	   if (dwAttrib & FILE_ATTRIBUTE_READONLY)
      {
         CString msg;
         msg.Format("Cannot save file. The file %s is read-only. Please try to save again to a different file.", lpszPathName);
         AfxMessageBox(msg );
         return FALSE;
      }
   }

   CComBSTR filname(lpszPathName);
   FileStream ofs;
   if (ofs.open(filname,false))
   {
      try
      {
      // save
         sysStructuredSaveXmlPrs mysave;
         mysave.BeginSave(&ofs);

         // save editor-specific information
         CEAFApp* pApp = (CEAFApp*)AfxGetApp();
         mysave.BeginUnit("LIBRARY_EDITOR", 1.0);
         if (pApp->GetUnitsMode() == eafTypes::umUS)
            mysave.Property("EDIT_UNITS", "US");
         else
            mysave.Property("EDIT_UNITS", "SI");

         // save library manager
            m_LibraryManager.SaveMe(&mysave);

         mysave.EndUnit();

         mysave.EndSave();
      }
      catch (const sysXStructuredSave& rXSave)
      {
         rXSave; // unused
         AfxMessageBox("Error saving library data. You may have a full hard disk",MB_OK||MB_ICONEXCLAMATION);
         return FALSE;
      }
   }
   else
      return FALSE;
	
   // document is now clean
   SetModifiedFlag(FALSE);

	return TRUE;// CDocument::OnSaveDocument(lpszPathName);
}

HRESULT CLibraryEditorDoc::WriteTheDocument(IStructuredSave* pStrSave)
{
   // Not using this functionality - should never get here
   ATLASSERT(false);
   return E_FAIL;
}

HRESULT CLibraryEditorDoc::LoadTheDocument(IStructuredLoad* pStrLoad)
{
   // Not using this functionality - should never get here
   ATLASSERT(false);
   return E_FAIL;
}

void CLibraryEditorDoc::OnImport()
{
	// ask user for file name
   CFileDialog  fildlg(TRUE,"pgs",NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                   "PGSuper Project File (*.pgs)|*.pgs||");
   int stf = fildlg.DoModal();
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
         hr = pStrLoad->BeginUnit("PGSuper");
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
            if ( FAILED(pStrLoad->get_Property("Version",&var)) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }

            if ( FAILED(pStrLoad->BeginUnit("Broker")) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }

            if ( FAILED(pStrLoad->BeginUnit("Agent")) )
            {
               HandleOpenDocumentError( hr, rPath );
               ASSERT(FALSE);
            }

            if ( FAILED(pStrLoad->get_Property("CLSID",&var)) )
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
         log_msg.Format("An unknown error occured while opening the file (hr = %d)",hr);
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
         log_msg.Format("An unknown error occured while closing the file (hr = %d)",hr);
         AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      }
      break;
   }

   AfxMessageBox( msg1 );
}
