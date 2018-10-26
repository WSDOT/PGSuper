///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

// TxDOTOptionalDesignDoc.cpp : implementation of the CTxDOTOptionalDesignDoc class
//
#include "stdafx.h"
#include "resource.h"

#include "TxDOTOptionalDesignUtilities.h"
#include "TxDOTOptionalDesignDoc.h"
#include "TOGAStatusBar.h"

#include "TxDOTOptionalDesignView.h"
#include "TogaGirderEditorSettingsSheet.h"

#include "TxDOTOptionalDesignDocProxyAgent.h"

#include "PGSuperCatCom.h"
#include <WBFLReportManagerAgent_i.c>

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFProjectLog.h>

#include <System\StructuredLoadXmlPrs.h>

#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IReportManager.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\GirderData.h>
#include <Lrfd\StrandPool.h>

#include <limits>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOGA_PLUGIN_COMMAND_COUNT 256

// misc
static bool DoesFileExist(const CString& filename)
{
   if (filename.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file!=0);
   }
}

// simple, exception-safe class for blocking events
class SimpleMutex
{
public:
   SimpleMutex(bool& flag):
   m_Flag(flag)
   {
      m_Flag = true;
   }

   ~SimpleMutex()
   {
      m_Flag = false;
   }
private:
   bool& m_Flag;
};

// Name for girder library copy
std::_tstring MakeCloneName(const std::_tstring& rOriginalName, GirderIndexType gdr)
{
   if (gdr == TOGA_ORIG_GDR)
   {
      return rOriginalName + std::_tstring(_T("(Original)"));
   }
   else
   {
      ASSERT(gdr == TOGA_FABR_GDR);
      return rOriginalName + std::_tstring(_T("(Fabricator Optional)"));
   }
}

// Default PGSuper Template File
static CString g_DefaultPGSuperFileName(_T("TogaTemplate.pgs"));

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignDoc

IMPLEMENT_DYNCREATE(CTxDOTOptionalDesignDoc, CEAFBrokerDocument)

BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignDoc, CEAFBrokerDocument)
	//{{AFX_MSG_MAP(CPGSuperDoc)
   //}}AFX_MSG_MAP
   ON_COMMAND(ID_FILE_SAVE, &CTxDOTOptionalDesignDoc::OnFileSave)
   ON_COMMAND(ID_FILE_SAVEAS, &CTxDOTOptionalDesignDoc::OnFileSaveas)
   ON_COMMAND(ID_FILE_EXPORTPGSUPERMODEL, &CTxDOTOptionalDesignDoc::OnFileExportPgsuperModel)
   ON_COMMAND(ID_VIEW_GIRDERVIEWSETTINGS, &CTxDOTOptionalDesignDoc::OnViewGirderviewsettings)
   ON_COMMAND(ID_STATUSCENTER_VIEW, &CTxDOTOptionalDesignDoc::OnStatuscenterView)
END_MESSAGE_MAP() 

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc construction/destruction

CTxDOTOptionalDesignDoc::CTxDOTOptionalDesignDoc():
m_ChangeStatus(ITxDataObserver::ctTemplateFile), // assume that all data must be rebuilt
m_VirginBroker(true),
m_InExportMode(false),
m_GirderModelEditorSettings(DEF_GV)
{
	EnableAutomation();
	AfxOleLockApp();

   m_ProjectData.Attach(this); // subscribe to events

   // Reserve command IDs for document plug ins
   GetPluginCommandManager()->ReserveCommandIDRange(TOGA_PLUGIN_COMMAND_COUNT);
}

CTxDOTOptionalDesignDoc::~CTxDOTOptionalDesignDoc()
{
   AfxOleUnlockApp();
}

// listen to events from data 
void CTxDOTOptionalDesignDoc::OnTxDotDataChanged(int change)
{
   ASSERT(change!=0);
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      this->SetModifiedFlag(TRUE);
   }

   if ( (change & ITxDataObserver::ctTemplateFile) == ITxDataObserver::ctTemplateFile)
   {
      // Need to reparse our template file
      BOOL st = ParseTemplateFile(false);
      ASSERT(st);

      // strand input data is now no good - reset number of strands to zero
      m_ProjectData.ResetStrandNoData();
   }

   m_ChangeStatus |= change; // save changes
}

HRESULT CTxDOTOptionalDesignDoc::LoadThePGSuperDocument(IStructuredLoad* pStrLoad)
{
   // get the file version
   HRESULT hr = pStrLoad->BeginUnit(_T("PGSuper"));
   if ( FAILED(hr) )
      return hr;

   double ver;
   pStrLoad->get_Version(&ver);
   if ( 1.0 < ver )
   {
      CComVariant var;
      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("Version"),&var);
   #if defined _DEBUG
      TRACE(_T("Loading data saved with PGSuper Version %s\n"),CComBSTR(var.bstrVal));
   }
   else
   {
      TRACE(_T("Loading data saved with PGSuper Version 2.1 or earlier\n"));
   #endif
   } // clses the bracket for if ( 1.0 < ver )

   return CEAFBrokerDocument::LoadTheDocument(pStrLoad);
}

void CTxDOTOptionalDesignDoc::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleOpenDocumentError(hr,lpszPathName);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while opening %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredLoad not registered") );
      msg1.LoadString( IDS_E_BADINSTALL );
      break;

   case STRLOAD_E_CANTOPEN:
      pLog->LogMessage( TEXT("Could not open file") );
      AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      break;

   case STRLOAD_E_FILENOTFOUND:
      pLog->LogMessage( TEXT("File Not Found") );
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRLOAD_E_INVALIDFORMAT:
      pLog->LogMessage( TEXT("File does not have valid PGSuper format") );
      AfxFormatString1( msg1, IDS_E_INVALIDFORMAT, lpszPathName );
      break;

   case STRLOAD_E_BADVERSION:
      pLog->LogMessage( TEXT("This file came from a newer version of PGSuper, please upgrade") );
      AfxFormatString1( msg1, IDS_E_INVALIDVERSION, lpszPathName );
      break;

   case STRLOAD_E_USERDEFINED:
      AfxFormatString1( msg1, IDS_E_USERDEFINED, lpszPathName );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while opening the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   // things here are very dire - show message box AND throw
   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );

   TxDOTBrokerRetrieverException exc;
   exc.Message = msg;
   throw exc;
}


// CEAFBrokerDocument overrides
CATID CTxDOTOptionalDesignDoc::GetAgentCategoryID()
{
   // all dynamically created agents used with this document type must
   // belong to the CATID_PGSuperAgent category
   return CATID_PGSuperAgent;
}

CATID CTxDOTOptionalDesignDoc::GetExtensionAgentCategoryID()
{
   return CATID_PGSuperExtensionAgent; // if you want extension agents
}

void CTxDOTOptionalDesignDoc::OnCreateInitialize()
{
   // called before any data is loaded/created in the document
   CEAFBrokerDocument::OnCreateInitialize();
}

void CTxDOTOptionalDesignDoc::OnCreateFinalize()
{
   // called after document is loaded and views are created
   CEAFBrokerDocument::OnCreateFinalize();
}

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignDoc diagnostics

#ifdef _DEBUG
void CTxDOTOptionalDesignDoc::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());

	CEAFBrokerDocument::AssertValid();
}

void CTxDOTOptionalDesignDoc::Dump(CDumpContext& dc) const
{
	CEAFBrokerDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignDoc commands

BOOL CTxDOTOptionalDesignDoc::LoadSpecialAgents(IBrokerInitEx2* pBrokerInit)
{
   if ( !CEAFBrokerDocument::LoadSpecialAgents(pBrokerInit) )
      return FALSE;

   // NOTE: This could be a problem... the PGSuper doc proxy agent is local to the PGSuper.exe project
   // If it is needed in this document, I recommend that we create a CPGSuperDocBase base class for
   // CPGSuperDoc and CTxDOTOptionDesignDoc. This class would go into a new DLL (on that will ultimately
   // have the PGSuper app plugin and all the associated files) and be exported. This class would implement
   // this method so all PGSuper-doc objects have the proper agents loaded.

   CComObject<CTxDOTOptionalDesignDocProxyAgent>* pDocProxyAgent;
   CComObject<CTxDOTOptionalDesignDocProxyAgent>::CreateInstance(&pDocProxyAgent);
   m_pTxDOTOptionalDesignDocProxyAgent = dynamic_cast<CTxDOTOptionalDesignDocProxyAgent*>(pDocProxyAgent);
   m_pTxDOTOptionalDesignDocProxyAgent->SetDocument( this );

   CComPtr<IAgentEx> pAgent(m_pTxDOTOptionalDesignDocProxyAgent);
   
   HRESULT hr = pBrokerInit->AddAgent( pAgent );
   if ( FAILED(hr) )
      return hr;

   // we want to use some special agents
   CLSID clsid[] = {CLSID_SysAgent,CLSID_ReportManager};
   if ( !LoadAgents(pBrokerInit, clsid, sizeof(clsid)/sizeof(CLSID) ) )
      return FALSE;

   return TRUE;
}

BOOL CTxDOTOptionalDesignDoc::Init()
{
   if ( !CEAFBrokerDocument::Init() )
      return FALSE;

   m_ProjectData.ResetData();

   try
   {
      InitializeLibraryManager();
   }
   catch(TxDOTBrokerRetrieverException exc)
   {
      // An error occurred - report the problem
      CString msg = _T("An Error Occurred While Loading the Master Library: ");
      msg += exc.Message;
      ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
      return FALSE;
   }

   return TRUE;
}

CString CTxDOTOptionalDesignDoc::GetToolbarSectionName()
{
   // Registery key for saving toolbar settings for this document type
   return _T("TxDOT Alternative Doc Toolbars");
}

void CTxDOTOptionalDesignDoc::DoIntegrateWithUI(BOOL bIntegrate)
{
   // Add the document's user interface stuff first
   CEAFMainFrame* pFrame = (CEAFMainFrame*)AfxGetMainWnd();
   if ( bIntegrate )
   {
      {
         AFX_MANAGE_STATE(AfxGetStaticModuleState());

         // set up the toolbar here
         UINT tbID = pFrame->CreateToolBar(_T("TxDOT Optional Girder Analysis"),GetPluginCommandManager());
#if defined _EAF_USING_MFC_FEATURE_PACK
         m_pMyToolBar = pFrame->GetToolBarByID(tbID);
         m_pMyToolBar->LoadToolBar(IDR_TXDOTOPTIONALDESIGNTOOLBAR,NULL);
#else
         m_pMyToolBar = pFrame->GetToolBar(tbID);
         m_pMyToolBar->LoadToolBar(IDR_TXDOTOPTIONALDESIGNTOOLBAR,NULL);
         m_pMyToolBar->CreateDropDownButton(ID_FILE_OPEN,   NULL,BTNS_DROPDOWN);
#endif
      }

      // use our status bar
      CTOGAStatusBar* pSB = new CTOGAStatusBar;
      pSB->Create(EAFGetMainFrame());
      EAFGetMainFrame()->SetStatusBar(pSB);
   }
   else
   {
      // remove toolbar here
      pFrame->DestroyToolBar(m_pMyToolBar);
      m_pMyToolBar = NULL;

      // put the status bar back the way it was
      //EAFGetMainFrame()->ResetStatusBar();
   }

   // then call base class, which handles UI integration for
   // plug-ins
   CEAFDocument::DoIntegrateWithUI(bIntegrate);
}

void CTxDOTOptionalDesignDoc::LoadDocumentSettings()
{
   // read document-based INI/Registry settings
   CEAFBrokerDocument::LoadDocumentSettings();

   CEAFApp* pApp = EAFGetApp();

   m_GirderModelEditorSettings = pApp->GetProfileInt(_T("Settings"),_T("GirderView"), DEF_GV);
}

void CTxDOTOptionalDesignDoc::SaveDocumentSettings()
{
   // Write document level INI/Registry settings
   CEAFBrokerDocument::SaveDocumentSettings();

   CEAFApp* pApp = EAFGetApp();

   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("GirderView"),m_GirderModelEditorSettings));
}

BOOL CTxDOTOptionalDesignDoc::OnNewDocumentFromTemplate(LPCTSTR lpszPathName)
{
   // Mostly Doing our own thing here. Our template is a single line file, so structured storage is overkill
   // Parent still does most of the set up
   if ( !OnNewDocument() )
      return FALSE;

   // Beam type is the name of the template file - we need to set this here
   // Get file title name
   CString name(lpszPathName);
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CString suffix;
      suffix.LoadString(IDS_TEMPLATE_SUFFIX);

      int ls = suffix.GetLength()+1; // include '.'

      int l = name.GetLength();
      name = name.Left(l-ls);  // trim off suffix

      int n = name.ReverseFind(_T('\\'));
      if (n>0)
      {
         l = name.GetLength();
         name = name.Right(l-n-1); // trim off directory information
      }
   }

   m_ProjectData.SetBeamType(name, false); // don't fire

   // Use pgsuper's engineer name and company if available
   CEAFApp* pApp = EAFGetApp();
   CString engName    = pApp->GetProfileString(_T("Options"),_T("EngineerName"));
   m_ProjectData.SetEngineer(engName);

   CString cmpyName    = pApp->GetProfileString(_T("Options"),_T("CompanyName"));
   m_ProjectData.SetCompany(cmpyName);

   // parse template and set project data
   return ParseTemplateFile(lpszPathName, true);
}

BOOL CTxDOTOptionalDesignDoc::ParseTemplateFile(bool isNewFileFromTemplate)
{
   // Get file name for template file (template file name is always beam type)
   CString beam_type = m_ProjectData.GetBeamType();
   CString suffix;

   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      suffix.LoadString(IDS_TEMPLATE_SUFFIX);
   }

   CString template_name = GetTOGAFolder() + CString(_T("\\")) + beam_type + _T(".") + suffix;

   // Read template file and update project data
   return ParseTemplateFile(template_name, isNewFileFromTemplate);
}


BOOL CTxDOTOptionalDesignDoc::ParseTemplateFile(LPCTSTR lpszPathName, bool isNewFileFromTemplate)
{
   // Read girder type, connection types, and pgsuper template file name
   CString girderEntry, leftConnEntry, rightConnEntry, projectCriteriaEntry;
   if(!::ParseTemplateFile(lpszPathName, girderEntry, leftConnEntry, rightConnEntry, projectCriteriaEntry))
   {
      ASSERT(0);
      return FALSE;
   }

   // set our data values
   m_ProjectData.SetGirderEntryName( girderEntry );
   m_ProjectData.SetLeftConnectionEntryName( leftConnEntry );
   m_ProjectData.SetRightConnectionEntryName( rightConnEntry );

   if (isNewFileFromTemplate)
   {
      // Only set project criteria and template file name on initial load from template

      m_ProjectData.SetSelectedProjectCriteriaLibrary(projectCriteriaEntry);

      // At one time, the template file also contained the name of the pgsuper seed data file
      // this is no more
      m_ProjectData.SetPGSuperFileName( g_DefaultPGSuperFileName );
   }

   return TRUE;
}

CString CTxDOTOptionalDesignDoc::GetRootNodeName()
{
   if ( m_InExportMode )
      return _T("PGSuper");
   else
      return __super::GetRootNodeName();
}

Float64 CTxDOTOptionalDesignDoc::GetRootNodeVersion()
{
   if ( m_InExportMode )
      return 2.0;
   else
      return __super::GetRootNodeVersion();
}

HRESULT CTxDOTOptionalDesignDoc::OpenDocumentRootNode(IStructuredSave* pStrSave)
{
   if ( m_InExportMode )
   {
     HRESULT hr = CEAFDocument::OpenDocumentRootNode(pStrSave);
     if ( FAILED(hr) )
        return hr;

     hr = pStrSave->put_Property(_T("Version"),CComVariant(_T("Toga")));
     if ( FAILED(hr) )
        return hr;
   }
   else
   {
      return __super::OpenDocumentRootNode(pStrSave);
   }

  return S_OK;
}

BOOL CTxDOTOptionalDesignDoc::OpenTheDocument(LPCTSTR lpszPathName)
{
   try
   {
      return CEAFBrokerDocument::OpenTheDocument(lpszPathName);
   }
   catch(...)
   {
      // error messaging should be handled below this
      return FALSE;
   }
}

BOOL CTxDOTOptionalDesignDoc::SaveTheDocument(LPCTSTR lpszPathName)
{
   return CEAFBrokerDocument::SaveTheDocument(lpszPathName);
}

HRESULT CTxDOTOptionalDesignDoc::WriteTheDocument(IStructuredSave* pStrSave)
{
   if (!m_InExportMode)
   {
      // save our own data
      HRESULT res = m_ProjectData.Save(pStrSave,NULL);
      return res;
   }
   else
   {
      // Save pgsuper document
      return CEAFBrokerDocument::WriteTheDocument(pStrSave);
   }
}

HRESULT CTxDOTOptionalDesignDoc::LoadTheDocument(IStructuredLoad* pStrLoad)
{
   HRESULT res = m_ProjectData.Load(pStrLoad,NULL);
   return res;

   //return CEAFBrokerDocument::LoadTheDocument(pStrLoad);
}


void CTxDOTOptionalDesignDoc::OnFileSave()
{
   // First, Get data from active input view if there is one
   if (!UpdateCurrentViewInputData())
      return;

   __super::OnFileSave();
}

void CTxDOTOptionalDesignDoc::OnFileSaveas()
{
   // Get data from active input view if there is one
   if (!UpdateCurrentViewInputData())
      return;

   __super::OnFileSaveAs();
}

void CTxDOTOptionalDesignDoc::OnFileExportPgsuperModel()
{
   // Get data from active input view if there is one
   if (!UpdateCurrentViewInputData())
      return;

   // Need an updated broker with all input data set in it
   IBroker* pBroker(NULL);
   try
   {
      pBroker = GetUpdatedBroker();
   }
   catch(TxDOTBrokerRetrieverException exc)
   {
      // An error occurred - report the problem
      CString msg = _T("An Error Occurred While Building the PGSuper Model: ");
      msg += exc.Message;
      ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
      return;
   }
   catch(...)
   {
      CString msg = _T("An Unknown Error Occurred While Building the PGSuper Model");
      ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
      return;
   }

   // Have broker, now prompt user and save data
   CString default_name = _T("TogaPGSuperModel.pgs");
   
   // prompt user to save current project to a template file
   CFileDialog  fildlg(FALSE,_T("pgs"),default_name,OFN_HIDEREADONLY,
                   _T("PGSuper Files (*.pgs)|*.pgs||"));

   INT_PTR stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      // try loop with a new file name
      CString file_path = fildlg.GetPathName();

      // check if file exists and prompt user if he wants to overwrite
      if (DoesFileExist(file_path))
      {
         CString msg(_T(" The file: "));
         msg += file_path + _T(" exists. Overwrite it?");
         int stm = AfxMessageBox(msg,MB_YESNOCANCEL|MB_ICONQUESTION);
         if (stm!=IDYES)
            return;
      }

      SimpleMutex mutex(m_InExportMode); // Set flag until we are done

      CEAFDocument::SaveTheDocument(file_path);
   }
}

BOOL CTxDOTOptionalDesignDoc::UpdateCurrentViewInputData()
{
   POSITION pos = GetFirstViewPosition();
   if (pos != NULL)
   {
      CView* pView = GetNextView(pos);
      CTxDOTOptionalDesignView* pTView = dynamic_cast<CTxDOTOptionalDesignView*>(pView);
      ASSERT(pTView);
      if (pTView!=NULL)
      {
         return pTView->UpdateCurrentPageData();
      }
   }
   ASSERT(0);
   return TRUE; // heaven help us
}

// ITxDOTBrokerRetriever
//
IBroker* CTxDOTOptionalDesignDoc::GetUpdatedBroker()
{
   // Get broker from parent class
   CComPtr<IBroker> pBroker;
   this->GetBroker(&pBroker);

   if (m_ChangeStatus != ITxDataObserver::ctNone)
   {
      // Data has changed, we have work to do

      if ((m_ChangeStatus & ITxDataObserver::ctTemplateFile) == ITxDataObserver::ctTemplateFile)
      {
         // Our template file has changed (could also be first run)
         CString new_pgsuper_file, old_pgsuper_file;

         {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            // Get name of pgsuper file so we can see if the whole broker must be rebuilt
            old_pgsuper_file = m_ProjectData.GetPGSuperFileName();

            // Read template file and update project data
            ParseTemplateFile(false);

            new_pgsuper_file = m_ProjectData.GetPGSuperFileName();
         }

         // We must delete broker and build a new one, then load pgsuper file
         if(!m_VirginBroker && new_pgsuper_file != old_pgsuper_file)
         {
#pragma Reminder("Test code to blast and rebuild broker")
            ASSERT(0); // not tested yet

            RecreateBroker();
         }
      }

      // Always need to load pgsuper file if new broker
      if (m_VirginBroker)
      {
         // Doesn't matter if things go well or not in this block - our broker will be tainted
         m_VirginBroker = false;

         try
         {
            // Intitialize broker with library
            GET_IFACE2( pBroker, ILibrary, pLib );
            pLib->SetLibraryManager( &m_LibMgr );

            // Need to load the pgsuper template file
            CString template_name = GetTOGAFolder() + CString(_T("\\")) + m_ProjectData.GetPGSuperFileName();

            MarryPGSuperTemplateWithBroker(template_name);
         }
         catch(...)
         {
            // Major things went wrong, and we don't care what. 
            // All we know is that a new broker must be created
            RecreateBroker();

            throw;
         }

         // Make sure our template fits the bill
         PreprocessTemplateData();
      }

      if ((m_ChangeStatus & ITxDataObserver::ctGirder) == ITxDataObserver::ctGirder)
      {
         // Use input data to modify pgsuper template data
         UpdatePgsuperModelWithData();
      }

      // our work is done here
      m_ChangeStatus = ITxDataObserver::ctNone;
   }

   return pBroker;
}

void CTxDOTOptionalDesignDoc::RecreateBroker()
{
   m_VirginBroker = true;

   this->BrokerShutDown();
   if (!this->CreateBroker())
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("An error occurred creating a new broker - This is a fatal programming error. Contact customer support");
      throw exc;
   }
}


IBroker* CTxDOTOptionalDesignDoc::GetClassicBroker()
{
   // Get broker from parent class
   // Do not update with input data
   CComPtr<IBroker> pBroker;
   this->GetBroker(&pBroker);

   return pBroker;
}

GirderLibrary* CTxDOTOptionalDesignDoc::GetGirderLibrary()
{
   ASSERT(m_LibMgr.GetGirderLibrary().GetCount() > 0);

   return &m_LibMgr.GetGirderLibrary();
}

ConnectionLibrary* CTxDOTOptionalDesignDoc::GetConnectionLibrary()
{
   ASSERT(m_LibMgr.GetConnectionLibrary().GetCount() > 0);

   return &m_LibMgr.GetConnectionLibrary();
}

SpecLibrary* CTxDOTOptionalDesignDoc::GetSpecLibrary()
{
   ASSERT(m_LibMgr.GetSpecLibrary()->GetCount() > 0);

   return m_LibMgr.GetSpecLibrary();
}

void CTxDOTOptionalDesignDoc::InitializeLibraryManager()
{

   // Use same master library as PGSuper
   CEAFApp* pApp = EAFGetApp();
   CString strMasterLibaryFile    = pApp->GetProfileString(_T("Options"),_T("MasterLibraryCache"));
   if (strMasterLibaryFile.IsEmpty())
   {
      // PGSuper's installer should take care of this, but just in case:
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("The location of the PGSuper Master Library file is not in the registry. You must run PGSuper at least once before you can run TOGA. All it takes is opening and closing a .pgs file. You can do this now.");
      WATCH(exc.Message);
      throw exc;
   }

   CString strURL   = pApp->GetProfileString(_T("Options"),_T("MasterLibraryURL"));
   if (strURL.IsEmpty())
      strURL = strMasterLibaryFile;

   CString strServer   = pApp->GetProfileString(_T("Options"),_T("CatalogServer"));

    // Hard-coded file location
//   CString strMasterLibaryFile = GetTOGAFolder() + CString(_T("\\")) + _T("MasterLibrary.lbr");

   m_LibMgr.SetName( _T("TOGA PGSuper Library") );
   m_LibMgr.SetMasterLibraryInfo(strServer,strURL);

   CComBSTR bpath(strMasterLibaryFile);

   FileStream ifile;
   if ( ifile.open(bpath) )
   {
      // try to load file
      try
      {
         // clear out library
         m_LibMgr.ClearAllEntries();

         sysStructuredLoadXmlPrs load;
         load.BeginLoad( &ifile );

         // Problem : Library Editor application specific code is in the
         // master library file. We have to read it or get an error.
         load.BeginUnit(_T("PGSuperLibrary"));
         load.BeginUnit(_T("LIBRARY_EDITOR"));
         std::_tstring str;
         load.Property(_T("EDIT_UNITS"), &str);

         if ( !m_LibMgr.LoadMe( &load ) )
         {
            TxDOTBrokerRetrieverException exc;
            exc.Message = _T("An uknown error occurred while loading the master library file.");
            WATCH(exc.Message);
            throw exc;
         }

         load.EndUnit(); //"LIBRARY_EDITOR"
         load.EndUnit(); //"PGSuperLibrary"
         load.EndLoad();

         // success!
         WATCH(_T("Master Library loaded successfully"));
      }
      catch( sysXStructuredLoad& e )
      {
         TxDOTBrokerRetrieverException exc;
         if ( e.GetExplicitReason() == sysXStructuredLoad::CantInitializeTheParser )
         {
            exc.Message = _T("Failed to initialize the xml parser. This is an installation issue.");
         }
         else
         {
            ASSERT(0);
            exc.Message = _T("An unknown error occurred while loading the master library file.");
         }

         WATCH(exc.Message);
         throw exc;

      }
      catch(...)
      {
         TxDOTBrokerRetrieverException exc;
         exc.Message = _T("An unknown error occurred while loading the master library file.");
         WATCH(exc.Message);
         throw exc;
      }
   }
   else
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message.Format(_T("Failed to open the master library file: %s"),strMasterLibaryFile);
      WATCH(exc.Message);
      throw exc;
   }

   // make all entries in master library read-only
   m_LibMgr.EnableEditingForAllEntries(false);

}


void CTxDOTOptionalDesignDoc::MarryPGSuperTemplateWithBroker(LPCTSTR lpszPathName)
{
   // Have to do some back-door dealings here to marry our broker with the pgsuper
   // template file.

   HRESULT hr = S_OK;

   {
      // NOTE: this scoping block is here for a reason. The IStructuredLoad must be
      //       destroyed before the file can be deleted.
      CComPtr<IStructuredLoad> pStrLoad;
      hr = ::CoCreateInstance( CLSID_StructuredLoad, NULL, CLSCTX_INPROC_SERVER, IID_IStructuredLoad, (void**)&pStrLoad );
      if ( FAILED(hr) )
      {
         // We are not aggregating so we should CoCreateInstance should
         // never fail with this HRESULT
         ASSERT( hr != CLASS_E_NOAGGREGATION );

         HandleOpenDocumentError( hr, lpszPathName );
      }

      hr = pStrLoad->Open( lpszPathName );
      if ( FAILED(hr) )
      {
         HandleOpenDocumentError( hr, lpszPathName );
      }

      // Our special version of load the document
      hr = LoadThePGSuperDocument(pStrLoad);
      if ( FAILED(hr) )
      {
         HandleOpenDocumentError( hr, lpszPathName );
      }

      // end unit wrapping entire file
      try
      {
         if (S_OK != pStrLoad->EndUnit())
            HandleOpenDocumentError( STRLOAD_E_INVALIDFORMAT, lpszPathName );
      }
      catch(...)
      {
         HandleOpenDocumentError( STRLOAD_E_INVALIDFORMAT, lpszPathName );
      }
      
      hr = pStrLoad->Close();
      if ( FAILED(hr) )
      {
         HandleOpenDocumentError( hr, lpszPathName );
      }
   }
}

void CTxDOTOptionalDesignDoc::PreprocessTemplateData()
{
   // At this point, we have loaded the pgsuper template file and have an active broker
   // Check that our template file meets our needs and save some information
   GET_IFACE(IBridgeDescription,pBridgeDesc);

   // Make sure our pgsuper template is what we expect
   CBridgeDescription bridgeDesc = *(pBridgeDesc->GetBridgeDescription());
   VerifyPgsuperTemplateData(bridgeDesc);
}

void CTxDOTOptionalDesignDoc::UpdatePgsuperModelWithData()
{
   // At this point, we have loaded the pgsuper template file and have an active broker
   // Now we can use our input data to modify the pgsuper project.
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   GET_IFACE(ILibrary, pLib );
   GET_IFACE(IEnvironment, pEnvironment );
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoadData);
   GET_IFACE(IProjectProperties,pProps);
   GET_IFACE(ISpecification,pSpec);

   CBridgeDescription bridgeDesc = *(pBridgeDesc->GetBridgeDescription());

   // Save comment data
   pProps->SetEngineer( std::_tstring( m_ProjectData.GetEngineer() ) );
   pProps->SetCompany( std::_tstring( m_ProjectData.GetCompany() ) );
   pProps->SetComments( std::_tstring( m_ProjectData.GetComments() ) );
   pProps->SetBridgeName( std::_tstring( m_ProjectData.GetBridge() ) );
   pProps->SetBridgeId( std::_tstring( m_ProjectData.GetBridgeID() ) );
   pProps->SetJobNumber( std::_tstring( m_ProjectData.GetJobNumber() ) );

   // Start off by setting whole bridge data
   CString gdr_name = m_ProjectData.GetGirderEntryName();
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(gdr_name);

   if (pGdrEntry==NULL)
   {
      TxDOTBrokerRetrieverException exc;
      CString stmp;
      stmp.LoadString(IDS_GDR_ERROR);
      exc.Message.Format(stmp,gdr_name);
      throw exc;
   }

   if (pGdrEntry->IsDifferentHarpedGridAtEndsUsed())
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message.Format(_T("The girder entry with name: \"%s\" has harped strands with different locations at the ends and C.L. Cannot continue"),gdr_name);
      throw exc;
   }

   bridgeDesc.SetGirderName(gdr_name);
   bridgeDesc.SetGirderLibraryEntry(pGdrEntry);

   // Set girder type to same library entry for each girder
   // We will need to change our girders later if non-standard fill is used
   CSpanData* pSpan =  bridgeDesc.GetSpan(0);

   GroupIndexType nGroups = pSpan->GetGirderTypes()->GetGirderGroupCount();
   if (nGroups != TOGA_NUM_GDRS)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("The PGSuper Template file is invalid - Separate girder types must be defined for each girder");
      throw exc;
   }

   CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();

   for(GroupIndexType iGroup = 0; iGroup< nGroups; iGroup++)
   {
      pGirderTypes->SetGirderName(iGroup, gdr_name);
      pGirderTypes->SetGirderLibraryEntry(iGroup, pGdrEntry);
   }

   // beam spacing
   Float64 gdr_width = pGdrEntry->GetBeamWidth(pgsTypes::metStart);
   Float64 spacing = m_ProjectData.GetBeamSpacing();
   if (spacing < gdr_width)
   {
      gdr_width = ::ConvertFromSysUnits(gdr_width, unitMeasure::Feet);
      TxDOTBrokerRetrieverException exc;
      exc.Message.Format(_T("The girder spacing must be greater than the girder width of %f feet"),gdr_width);
      throw exc;
   }

   bridgeDesc.SetGirderSpacingType(pgsTypes::sbsUniform);
   bridgeDesc.SetGirderSpacing(spacing);

   // connections
   // Left
   CString conL_name = m_ProjectData.GetLeftConnectionEntryName();
   const ConnectionLibraryEntry* pConLEntry = pLib->GetConnectionEntry(conL_name);
   if (pConLEntry==NULL)
   {
      TxDOTBrokerRetrieverException exc;
      CString stmp;
      stmp.LoadString(IDS_GDR_ERROR);
      exc.Message.Format(stmp,gdr_name);
      throw exc;
   }

   CPierData* pLftPier =  bridgeDesc.GetPier(0);
   pLftPier->SetGirderEndDistance(pgsTypes::Ahead,pConLEntry->GetGirderEndDistance(),pConLEntry->GetEndDistanceMeasurementType());
   pLftPier->SetBearingOffset(pgsTypes::Ahead,pConLEntry->GetGirderBearingOffset(),pConLEntry->GetBearingOffsetMeasurementType());
   pLftPier->SetSupportWidth(pgsTypes::Ahead,pConLEntry->GetSupportWidth());
   pLftPier->SetDiaphragmHeight(pgsTypes::Ahead,pConLEntry->GetDiaphragmHeight());
   pLftPier->SetDiaphragmWidth(pgsTypes::Ahead,pConLEntry->GetDiaphragmWidth());
   pLftPier->SetDiaphragmLoadType(pgsTypes::Ahead,pConLEntry->GetDiaphragmLoadType());
   pLftPier->SetDiaphragmLoadLocation(pgsTypes::Ahead,pConLEntry->GetDiaphragmLoadLocation());

   // Right
   CString conR_name = m_ProjectData.GetRightConnectionEntryName();
   const ConnectionLibraryEntry* pConREntry = pLib->GetConnectionEntry(conR_name);
   if (pConREntry==NULL)
   {
      TxDOTBrokerRetrieverException exc;
      CString stmp;
      stmp.LoadString(IDS_CONN_ERROR);
      exc.Message.Format(stmp,conR_name);
      throw exc;
   }

   CPierData* pRgtPier =  bridgeDesc.GetPier(1);
   pRgtPier->SetGirderEndDistance(pgsTypes::Back,pConREntry->GetGirderEndDistance(),pConREntry->GetEndDistanceMeasurementType());
   pRgtPier->SetBearingOffset(pgsTypes::Back,pConREntry->GetGirderBearingOffset(),pConREntry->GetBearingOffsetMeasurementType());
   pRgtPier->SetSupportWidth(pgsTypes::Back,pConREntry->GetSupportWidth());
   pRgtPier->SetDiaphragmHeight(pgsTypes::Back,pConREntry->GetDiaphragmHeight());
   pRgtPier->SetDiaphragmWidth(pgsTypes::Back,pConREntry->GetDiaphragmWidth());
   pRgtPier->SetDiaphragmLoadType(pgsTypes::Back,pConREntry->GetDiaphragmLoadType());
   pRgtPier->SetDiaphragmLoadLocation(pgsTypes::Back,pConREntry->GetDiaphragmLoadLocation());

   // Span length is bearing to bearing - must subtract connection length
   Float64 conn_len = pConLEntry->GetGirderBearingOffset();
   conn_len += pConREntry->GetGirderBearingOffset();

   Float64 span_length = m_ProjectData.GetSpanLength();
   span_length += conn_len;
   bridgeDesc.SetSpanLength(0,span_length);

   CDeckDescription* pDeck = bridgeDesc.GetDeckDescription();

   // Slab Thickness. Must also set slab offset and sac depth
   Float64 slab_thick = m_ProjectData.GetSlabThickness();
   pDeck->GrossDepth = slab_thick;
   pDeck->SacrificialDepth = 0.0;
   pDeck->OverhangEdgeDepth = slab_thick;
   pSpan->SetSlabOffset(pgsTypes::metStart, slab_thick);
   pSpan->SetSlabOffset(pgsTypes::metEnd, slab_thick);

   // Slab material properties
   pDeck->SlabEc = m_ProjectData.GetEcSlab();
   pDeck->SlabFc = m_ProjectData.GetFcSlab();

   // set deck width
   Float64 deck_width2 = TOGA_NUM_GDRS * spacing / 2.0;
   pDeck->DeckEdgePoints[0].LeftEdge = deck_width2;
   pDeck->DeckEdgePoints[0].RightEdge = deck_width2;

   // Relative Humidity
   Float64 rel_hum = m_ProjectData.GetRelativeHumidity();
   pEnvironment->SetRelHumidity( rel_hum );

   // Distribution factors
   bridgeDesc.SetDistributionFactorMethod(pgsTypes::DirectlyInput);

   Float64 lldf_mom = m_ProjectData.GetLldfMoment();
   Float64 lldf_shr = m_ProjectData.GetLldfShear();

   // TRICKY:
   // Note that there is a game played in PGSuper on these factors:
   //   They are either classified as Fatigue, or not. So Strength1 is only non-Fatigue state
   //   that needs to be set

   GirderIndexType ngdrs = pSpan->GetGirderCount();
   for (GirderIndexType ig=0; ig<ngdrs; ig++)
   {
      // set all girders even if toga doesn't use 'em all
      pSpan->SetLLDFPosMoment(ig, pgsTypes::StrengthI, lldf_mom);
      pSpan->SetLLDFNegMoment(ig, pgsTypes::StrengthI,lldf_mom);
      pSpan->SetLLDFShear(ig, pgsTypes::StrengthI, lldf_shr);

      pSpan->SetLLDFPosMoment(ig, pgsTypes::FatigueI, lldf_mom);
      pSpan->SetLLDFNegMoment(ig, pgsTypes::FatigueI,lldf_mom);
      pSpan->SetLLDFShear(ig, pgsTypes::FatigueI, lldf_shr);
   }

   // Set spec entry 
   std::_tstring curr_entry = m_ProjectData.GetSelectedProjectCriteriaLibrary();
   pSpec->SetSpecification(curr_entry);

   // Applied dead loads
   // First delete any distributed loads
   CollectionIndexType ndl = pUserDefinedLoadData->GetDistributedLoadCount();
   for (CollectionIndexType idl=0; idl<ndl; idl++)
   {
      pUserDefinedLoadData->DeleteDistributedLoad(0);
   }

   // HACK: If a distributed load is zero, place a very small value.
   // Why:
   //       1) TxDOT wants to see a load in the loads report even if it's zero
   //       2) If a zero load is added, it shows up as a warning in the Status Center
   //       3) Another solution would be to create a TOGA-specific load details report,
   //          but this would require lots of new code, and create the risk of someone
   //          adding a new load type that could slip under the radar.
   const Float64 SMALL_LOAD = ::ConvertToSysUnits(0.1,unitMeasure::NewtonPerMeter);

   // w non-comp, dc
   Float64 w = m_ProjectData.GetWNonCompDc();
   w = w==0.0 ? SMALL_LOAD : w; 
   CDistributedLoadData wncdc;
   wncdc.m_Description = _T("w non-comp, dc");
   wncdc.m_Type = UserLoads::Uniform;
   wncdc.m_WStart = w;
   wncdc.m_Stage = UserLoads::BridgeSite1;
   wncdc.m_LoadCase = UserLoads::DC;
   wncdc.m_Fractional = true;
   wncdc.m_StartLocation = 0.0;
   wncdc.m_EndLocation = -1.0;
   wncdc.m_Span = 0;

   // first load original girder, then fab'd
   wncdc.m_Girder = TOGA_ORIG_GDR;

   pUserDefinedLoadData->AddDistributedLoad(wncdc);

   wncdc.m_Girder = TOGA_FABR_GDR;

   pUserDefinedLoadData->AddDistributedLoad(wncdc);

   // w comp, dc
   w = m_ProjectData.GetWCompDc();
   w = w==0.0 ? SMALL_LOAD : w; 
   CDistributedLoadData wcdc;
   wcdc.m_Description = _T("w comp, dc");
   wcdc.m_Type = UserLoads::Uniform;
   wcdc.m_WStart = w;
   wcdc.m_Stage = UserLoads::BridgeSite2;
   wcdc.m_LoadCase = UserLoads::DC;
   wcdc.m_Fractional = true;
   wcdc.m_StartLocation = 0.0;
   wcdc.m_EndLocation = -1.0;
   wcdc.m_Span = 0;

   // first load original girder, then fab'd
   wcdc.m_Girder = TOGA_ORIG_GDR;

   pUserDefinedLoadData->AddDistributedLoad(wcdc);

   wcdc.m_Girder = TOGA_FABR_GDR;

   pUserDefinedLoadData->AddDistributedLoad(wcdc);

   // w overlay 
   // Hack: Apply this as a user-defined future overlay
   w = m_ProjectData.GetWCompDw();
   Float64 wl = w / spacing;

   pDeck->DeckType = pgsTypes::sdtCompositeCIP;
   pDeck->WearingSurface = pgsTypes::wstFutureOverlay;
   pDeck->bInputAsDepthAndDensity = false;
   pDeck->OverlayWeight = wl;


   // Tricky: Could write a lot of brittle logic here to make sure seed data is copied to girders and that 
   //         all editing information is translated properly. However, bridge data already has something like
   //         this. Just hand it a very different description (a default).
   CBridgeDescription bogus_descr;
   bridgeDesc.ReconcileEdits(m_pBroker, &bogus_descr);


   // Now we can deal with girder data for original and precaster optional designs
   // First check concrete - it's possible to not input this and go straight to anlysis
   if(m_ProjectData.GetOriginalDesignGirderData()->GetFc() == Float64_Inf)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("You must enter f'c for the Original girder");
      throw exc;
   }

   if(m_ProjectData.GetOriginalDesignGirderData()->GetFci() == Float64_Inf)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("You must enter f'ci for the Original girder");
      throw exc;
   }

   if(m_ProjectData.GetPrecasterDesignGirderData()->GetFc() == Float64_Inf)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("You must enter f'c for the Fabricator Optional girder");
      throw exc;
   }

   if(m_ProjectData.GetPrecasterDesignGirderData()->GetFci() == Float64_Inf)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("You must enter f'ci for the Fabricator Optional girder");
      throw exc;
   }

   // Now set girders' data
   SetGirderData(m_ProjectData.GetOriginalDesignGirderData(), TOGA_ORIG_GDR, gdr_name, pGdrEntry,
                 m_ProjectData.GetEcBeam(),pGirderTypes);

   SetGirderData(m_ProjectData.GetPrecasterDesignGirderData(), TOGA_FABR_GDR, gdr_name, pGdrEntry,
                 m_ProjectData.GetEcBeam(),pGirderTypes);


   // Set bridge data
   pBridgeDesc->SetBridgeDescription(bridgeDesc);

}

void CTxDOTOptionalDesignDoc::SetGirderData(CTxDOTOptionalDesignGirderData* pOdGirderData, GirderIndexType gdr, 
                                            LPCTSTR gdrName, const GirderLibraryEntry* pGdrEntry, Float64 EcBeam,
                                            CGirderTypes* pGirderTypes)
{
   CGirderData& rGirderData = pGirderTypes->GetGirderData(gdr);

   // First set non-fill related data
   // Assume that any data not defined in TOGA is set in template file
   // Girder concrete
   rGirderData.Material.bUserEci = false;
   rGirderData.Material.bUserEc = true;
   rGirderData.Material.Ec = EcBeam;
   rGirderData.Material.Fci = pOdGirderData->GetFci();
   rGirderData.Material.Fc  = pOdGirderData->GetFc();

   // Prestress material
   matPsStrand::Grade grade;
   matPsStrand::Type  type;
   matPsStrand::Size  size;
   pOdGirderData->GetStrandData(&grade, &type,&size);

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   for ( int i = 0; i < 3; i++ )
   {
      rGirderData.Material.pStrandMaterial[i] = pPool->GetStrand(grade,type,size);
      ASSERT(rGirderData.Material.pStrandMaterial[i]);
   }

   // Set seed data
   rGirderData.ShearData.CopyGirderEntryData( *pGdrEntry );
   rGirderData.LongitudinalRebarData.CopyGirderEntryData( *pGdrEntry );

   // See if standard or nonstandard fill
   if (pOdGirderData->GetStandardStrandFill())
   {
      StrandIndexType ntot = pOdGirderData->GetNumStrands();

      // We still must compute num straight/harped to appease pgsuper
      StrandIndexType ns, nh;
      bool st = pGdrEntry->GetPermStrandDistribution(ntot, &ns, &nh);
      ASSERT(st);


      rGirderData.PrestressData.SetTotalPermanentNstrands(ntot, ns, nh);

      rGirderData.PrestressData.HsoEndMeasurement = hsoTOP2BOTTOM;
      rGirderData.PrestressData.HpOffsetAtEnd = pOdGirderData->GetStrandTo();
   }
   else
   {
      // Non-standard fill 
      StrandIndexType straight_strands_to_fill = 0;
      StrandIndexType harped_strands_to_fill = 0;
      
      // Create a copy of the library entry
      // The clone is the entry we will modify to fit our non-standard strands
      GirderLibraryEntry clone_entry(*pGdrEntry); //

      // We'll be adding strands later
      clone_entry.ClearAllStrands();

      // Can have two types of non-standard fill
      if (pOdGirderData->GetUseDepressedStrands())
      {
         // With depressed strands - the hard way
         CString error_msg;
         if (!pOdGirderData->CheckAndBuildStrandRows(pGdrEntry,
            pOdGirderData->GetStrandsAtCL(),pOdGirderData->GetStrandsAtEnds(), error_msg, &clone_entry))
         {
            TxDOTBrokerRetrieverException exc;
            exc.Message = error_msg;
            throw exc;
         }

         // Our cloned entry has only the strand locations we need, now we can fill them
         straight_strands_to_fill = clone_entry.GetMaxStraightStrands();
         harped_strands_to_fill   = clone_entry.GetMaxHarpedStrands();

         StrandIndexType nt_max = straight_strands_to_fill + harped_strands_to_fill;

         // check that strand count matches
         StrandIndexType tot_chk=0;
         CTxDOTOptionalDesignGirderData::StrandRowContainer strandrows = pOdGirderData->GetStrandsAtCL();
         for(CTxDOTOptionalDesignGirderData::StrandRowIterator sit=strandrows.begin(); sit!=strandrows.end(); sit++)
         {
            tot_chk += sit->StrandsInRow;
         }

         if (tot_chk != nt_max)
         {
            TxDOTBrokerRetrieverException exc;
            exc.Message = _T("The total number of strands in the generated library entry does not match the input number of strands. This is a programming error - Cannot continue.");
            throw exc;
         }

         // We don't adjust strands for this case - they are where they need to be
         clone_entry.UseDifferentHarpedGridAtEnds(true);
         clone_entry.AllowVerticalAdjustmentEnd(false);
         clone_entry.AllowVerticalAdjustmentHP(false);
      }
      else
      {

         // All straight strands - fill harped strands as straight
         // Get available strands at each row in for our library entry
         CTxDOTOptionalDesignGirderData::AvailableStrandsInRowContainer available_rows;
         available_rows = pOdGirderData->ComputeAvailableStrandRows(pGdrEntry); 

         CTxDOTOptionalDesignGirderData::StrandRowContainer strandrows = pOdGirderData->GetStrandsAtCL();
         for(CTxDOTOptionalDesignGirderData::StrandRowIterator sit=strandrows.begin(); sit!=strandrows.end(); sit++)
         {
            Float64         row_elev       = sit->RowElev;
            StrandIndexType strands_in_row = sit->StrandsInRow;

            // Find current row in available rows
            CTxDOTOptionalDesignGirderData::AvailableStrandsInRow tester(row_elev);
            CTxDOTOptionalDesignGirderData::AvailableStrandsInRowIterator avit = available_rows.find(tester);
            if (avit != available_rows.end())
            {
               // Found strands - copy to our clone entry
               StrandIndexType num_filled = 0;

               CTxDOTOptionalDesignGirderData::AvailableStrandsInRow& ravrow = *avit;
               for(std::vector<CTxDOTOptionalDesignGirderData::StrandIncrement>::iterator srit = ravrow.AvailableStrandIncrements.begin();
                   srit != ravrow.AvailableStrandIncrements.end(); srit++)
               {
                  if (num_filled >= strands_in_row)
                  {
                     break;
                  }

                  StrandIndexType glob_idx = srit->GlobalFill;

                  if (glob_idx != INVALID_INDEX) // don't look at zero strands location
                  {
                     // Convert either to straight. Assume prismatic section
                     Float64 xStrand(0),yStrand(0);
                     CTxDOTOptionalDesignGirderData::GetGlobalStrandCoordinate(pGdrEntry, glob_idx, &xStrand, &yStrand);

                     ASSERT(xStrand>0.0); // TxDOT doesn't typically use middle strands

                     // Add straight strand to clone and update global index
                     StrandIndexType stridx = clone_entry.AddStraightStrandCoordinates(xStrand,yStrand,xStrand,yStrand,false);

                     clone_entry.AddStrandToPermStrandGrid(GirderLibraryEntry::stStraight, stridx-1);

                     num_filled += xStrand>0.0 ? 2 : 1;
                  }
               }

               straight_strands_to_fill += num_filled;

               if (num_filled != strands_in_row)
               {
                  ASSERT(0);
                  Float64 elev_in = ::ConvertFromSysUnits(row_elev, unitMeasure::Inch);
                  TxDOTBrokerRetrieverException exc;
                  exc.Message.Format(_T("Non-Standard strand input data tried and failed to fill %d strands at %.3f in. from the girder bottom. Input data does not match library."),strands_in_row,row_elev);
                  throw exc;
               }
            }
            else
            {
               Float64 elev_in = ::ConvertFromSysUnits(row_elev, unitMeasure::Inch);
               TxDOTBrokerRetrieverException exc;
               exc.Message.Format(_T("Non-Standard strand input data specified a strand at %.3f in. from the girder bottom. There are no stands at this location in the library."),elev_in);
               throw exc;
            }
         }

         // No harped strands and no need to adjust them
         clone_entry.UseDifferentHarpedGridAtEnds(false);
         clone_entry.AllowVerticalAdjustmentEnd(false);
         clone_entry.AllowVerticalAdjustmentHP(false);
      }

      // We have our modified entry - now add it to the library and set our girder to reference it
      GET_IFACE(ILibrary, pLib );
      GirderLibrary& rLibrary =  pLib->GetGirderLibrary();

      std::_tstring original_name = pGdrEntry->GetName();
      std::_tstring clone_name = MakeCloneName(original_name, gdr);
 
      while(!rLibrary.AddEntry(clone_entry, clone_name.c_str()))
      {
         // Name taken - create a new one (not pretty)
         clone_name += _T("x");
      }

      const GirderLibraryEntry* pclone = pLib->GetGirderEntry(clone_name.c_str());
      ASSERT(pclone);

      // set to girder
      pGirderTypes->SetGirderName(gdr, clone_name.c_str());
      pGirderTypes->SetGirderLibraryEntry(gdr, pclone);

      // Must set strands after library entry, otherwise data is reset
      rGirderData.PrestressData.SetTotalPermanentNstrands(straight_strands_to_fill + harped_strands_to_fill, straight_strands_to_fill, harped_strands_to_fill);
   }

   // Get Jacking 
   GET_IFACE(IPrestressForce, pPrestress );

   rGirderData.PrestressData.bPjackCalculated[pgsTypes::Permanent] = true;
   rGirderData.PrestressData.bPjackCalculated[pgsTypes::Straight] = true;
   rGirderData.PrestressData.bPjackCalculated[pgsTypes::Harped] = true;

   rGirderData.PrestressData.Pjack[pgsTypes::Permanent] = pPrestress->GetPjackMax(TOGA_SPAN, gdr, *(rGirderData.Material.pStrandMaterial[pgsTypes::Straight]), rGirderData.PrestressData.GetNstrands(pgsTypes::Permanent));
   rGirderData.PrestressData.Pjack[pgsTypes::Straight]  = pPrestress->GetPjackMax(TOGA_SPAN, gdr, *(rGirderData.Material.pStrandMaterial[pgsTypes::Straight]), rGirderData.PrestressData.GetNstrands(pgsTypes::Straight));
   rGirderData.PrestressData.Pjack[pgsTypes::Harped]    = pPrestress->GetPjackMax(TOGA_SPAN, gdr, *(rGirderData.Material.pStrandMaterial[pgsTypes::Harped]),   rGirderData.PrestressData.GetNstrands(pgsTypes::Harped));
}

void CTxDOTOptionalDesignDoc::VerifyPgsuperTemplateData(CBridgeDescription& bridgeDesc)
{
   // Do some basic checking that our pgsuper model is compatible with our assumptions
   SpanIndexType ns = bridgeDesc.GetSpanCount();
   if (ns!=1)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message.Format(_T("The PGSuper template model must have a single span. The model loaded has %d. Cannot continue"),ns);
      throw exc;
   }

   SpanIndexType ng = bridgeDesc.GetGirderCount();
   if (ng!=TOGA_NUM_GDRS)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message.Format(_T("The PGSuper template model must have %d girders. The model loaded has %d. Cannot continue"),TOGA_NUM_GDRS,ng);
      throw exc;
   }

   CDeckDescription* pDeck = bridgeDesc.GetDeckDescription();

   std::vector<CDeckPoint>::size_type ndp = pDeck->DeckEdgePoints.size();
   if (ndp!=1)
   {
      TxDOTBrokerRetrieverException exc;
      exc.Message.Format(_T("The PGSuper template model must contain a single deck edge point. The model loaded has %d. Cannot continue"),ndp);
      throw exc;
   }
}

BOOL CTxDOTOptionalDesignDoc::GetStatusBarMessageString(UINT nID,CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return __super::GetStatusBarMessageString(nID,rMessage);
}

BOOL CTxDOTOptionalDesignDoc::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return __super::GetToolTipMessageString(nID,rMessage);
}

UINT CTxDOTOptionalDesignDoc::GetGirderEditorSettings() const
{
   return m_GirderModelEditorSettings;
}

void CTxDOTOptionalDesignDoc::SetGirderEditorSettings(UINT settings)
{
   m_GirderModelEditorSettings = settings;
}

void CTxDOTOptionalDesignDoc::EditGirderViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT settings = GetGirderEditorSettings();

	CTogaGirderEditorSettingsSheet dlg(_T("Girder View Settings"));
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      SetGirderEditorSettings(settings);

      // tell the world we've changed settings
      UpdateAllViews( 0, HINT_GIRDERVIEWSETTINGSCHANGED, 0 );
   }

}

void CTxDOTOptionalDesignDoc::OnViewGirderviewsettings()
{
   EditGirderViewSettings(0);
}

void CTxDOTOptionalDesignDoc::OnStatuscenterView()
{
   CEAFBrokerDocument::OnViewStatusCenter();
}
