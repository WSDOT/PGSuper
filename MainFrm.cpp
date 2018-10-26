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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PGSuper.h"

#include "MainFrm.h"

#include "Resource.h"

#include "Splash.h"

#include "ToolBarDlg.h"
#include "AnalysisResultsView.h"
#include "FactorOfSafetyView.h"
#include "EditLoadsView.h"
#include "LibraryEditor\LibraryEditorView.h"

#include "GirderModelChildFrame.h"

#include "StatusCenterImp.h"

#include <boost\shared_ptr.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


   // The following control bar id's are reserved by MFC. (see AFXRES.h)
   // We must start numbering our control bars after 0xE803
   //#define AFX_IDW_TOOLBAR                 0xE800  // main Toolbar for window
   //#define AFX_IDW_STATUS_BAR              0xE801  // Status bar window
   //#define AFX_IDW_PREVIEW_BAR             0xE802  // PrintPreview Dialog Bar
   //#define AFX_IDW_RESIZE_BAR              0xE803  // OLE in-place resize bar
#define MY_FIRST_TOOLBAR  0xE804

// menu stuff
const int CMENU_BASE = 100; // command id's will be numbered 100, 101, 102, etc for each menu item
const int MAX_CMENUS = 100; // this is limiting, but it is MFC's fault.
const int CMENU_MAX  = CMENU_BASE + MAX_CMENUS;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
   //{{AFX_MSG_MAP(CMainFrame)
   ON_WM_CREATE()
   ON_COMMAND(ID_VIEW_BRIDGEMODELEDITOR, OnViewBridgeModelEditor)
   ON_COMMAND(ID_VIEW_GIRDEREDITOR, OnViewGirderEditor)
   ON_COMMAND(ID_VIEW_LIBRARYEDITOR, OnViewLibraryEditor)
   ON_COMMAND(ID_VIEW_ANALYSISRESULTS, OnViewAnalysisResults)
   //ON_COMMAND(ID_VIEW_REPORTS, OnViewReports)
   ON_NOTIFY(TBN_DROPDOWN,MY_FIRST_TOOLBAR,OnViewReports)
   ON_COMMAND(ID_VIEW_STABILITY,OnViewStability)
   ON_WM_CLOSE()
   ON_COMMAND(ID_VIEW_TOOLBAR, OnViewToolbar)
   ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateViewToolbar)
   ON_UPDATE_COMMAND_UI(ID_VIEW_BRIDGEMODELEDITOR, OnUpdateEditors)
	ON_WM_DROPFILES()
   ON_UPDATE_COMMAND_UI(ID_VIEW_GIRDEREDITOR, OnUpdateEditors)
   ON_UPDATE_COMMAND_UI(ID_VIEW_LIBRARYEDITOR, OnUpdateEditors)
   ON_UPDATE_COMMAND_UI(ID_VIEW_ANALYSISRESULTS, OnUpdateEditors)
   ON_UPDATE_COMMAND_UI(ID_VIEW_REPORTS, OnUpdateEditors)
   ON_UPDATE_COMMAND_UI(ID_VIEW_STABILITY, OnUpdateEditors)
   ON_UPDATE_COMMAND_UI(ID_EDIT_USERLOADS, OnUpdateEditors)
	ON_COMMAND(ID_EDIT_USERLOADS, OnEditUserloads)
	//}}AFX_MSG_MAP
   // Global help commands
   //
   ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
   ON_COMMAND(ID_HELP, OnHelp)
   ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
   ON_COMMAND(ID_DEFAULT_HELP, OnHelp)

   ON_COMMAND_RANGE(CMENU_BASE, CMENU_MAX, OnToolbarMenuSelected)
END_MESSAGE_MAP()

static UINT indicators[] =
{
   ID_SEPARATOR,           // status line indicator
   ID_INDICATOR_ANALYSIS,
   ID_INDICATOR_STATUS,
   ID_INDICATOR_MODIFIED,
   ID_INDICATOR_AUTOCALC_ON,
   ID_INDICATOR_CAPS,
   ID_INDICATOR_NUM,
   ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
CMDIFrameWnd()
{
   m_bDisableFailCreateMsg = false;
   m_bCreateCanceled = false;
   m_bShowToolTips = TRUE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   // Restore the layout of the application window
   WINDOWPLACEMENT wp;
   if ( ((CPGSuperApp*)AfxGetApp())->ReadWindowPlacement(CString((LPCSTR)IDS_REG_WNDPOS),&wp))
   {
      SetWindowPlacement(&wp);
   }

   // Restore tool tips mode
   m_bShowToolTips = (AfxGetApp()->GetProfileInt(CString((LPCSTR)IDS_REG_SETTINGS),
                                                 CString((LPCSTR)IDS_TOOLTIP_STATE),
                                                 1) !=0 );

   // Create the toolbars
   // When adding a new toolbar, simply add its resource ID to
   // GetToolBarResourceIDs().
   EnableDocking(CBRS_ALIGN_ANY);

   DWORD dwToolbarStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_SIZE_DYNAMIC;
   if ( m_bShowToolTips )
      dwToolbarStyle |= CBRS_TOOLTIPS | CBRS_FLYBY;

   std::vector<UINT> vNames = GetToolBarResourceIDs();
   std::vector<UINT>::iterator iter;
   UINT nCount = 0;
   for ( iter = vNames.begin(); iter != vNames.end(); iter++ )
   {
	   m_ToolBars.push_back( boost::shared_ptr<CToolBar>(new CToolBar) );

      if ( !m_ToolBars[nCount]->Create(this,dwToolbarStyle, MY_FIRST_TOOLBAR + nCount) ||
           !m_ToolBars[nCount]->LoadToolBar( *iter ) )
      {
         WARN(true, "Failed to create toolbar " << nCount );
         return -1;
      }

      m_ToolBars[nCount]->EnableDocking(CBRS_ALIGN_ANY);
      m_ToolBars[nCount]->SetWindowText( CString((LPCSTR)(*iter)) );

      if ( nCount == 0 )
         DockControlBar( m_ToolBars[nCount].get() );
      else
         DockControlBarLeftOf( m_ToolBars[nCount].get(), m_ToolBars[nCount-1].get());

      nCount++;
   }

   // Add a drop-down arrow to the Report button
   // REF: http://www.codejock.com/support/articles/mfc/general/g_5.asp
   CToolBar* pToolBar = m_ToolBars[GetToolBarIndex(IDR_STDTOOLBAR)].get();
   DWORD dwExStyle = TBSTYLE_EX_DRAWDDARROWS;
   pToolBar->GetToolBarCtrl().SendMessage(TB_SETEXTENDEDSTYLE,0,(LPARAM)dwExStyle);
   int idx = pToolBar->CommandToIndex(ID_VIEW_REPORTS);
   DWORD dwStyle = pToolBar->GetButtonStyle(idx);
   dwStyle |= BTNS_WHOLEDROPDOWN; //TBSTYLE_DROPDOWN;
   pToolBar->SetButtonStyle(idx,dwStyle);


   UpdateToolbarStatusItems(pgsTypes::statusOK);

   if (!m_wndStatusBar.Create(this) ||
      !m_wndStatusBar.SetIndicators(indicators,
        sizeof(indicators)/sizeof(UINT)))
   {
      TRACE0("Failed to create status bar\n");
      return -1;      // fail to create
   }
   m_wndStatusBar.SetPaneText( 1, "" );
   m_wndStatusBar.SetPaneStyle( 2, SBPS_NORMAL | SBT_OWNERDRAW );
   m_wndStatusBar.SetPaneStyle( 3, SBPS_DISABLED );

   LoadBarState( CString((LPCSTR)IDS_TOOLBAR_STATE) );

   //CMDITabInfo tabInfo;
   //tabInfo.m_bAutoColor      = true;
   //tabInfo.m_bDocumentMenu   = true;
   //tabInfo.m_bFlatFrame      = true;
   //tabInfo.m_bTabCloseButton = true;
   //tabInfo.m_bEnableTabSwap  = true;
   //tabInfo.m_bTabIcons       = true;
   //tabInfo.m_bActiveTabCloseButton = false;
   //tabInfo.m_tabLocation = CMFCTabCtrl::LOCATION_TOP;
   //tabInfo.m_style       = CMFCTabCtrl::STYLE_3D_ONENOTE;
   //EnableMDITabbedGroups(true,tabInfo);

   return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
   return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
   CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
   CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
CPGSuperDoc* CMainFrame::GetPGSuperDocument()
{
   CMDIChildWnd* pActiveChild = MDIGetActive();
   CDocument* pDocument = NULL;
   pDocument = GetActiveDocument();

   if (pDocument == NULL && pActiveChild != NULL)
      pDocument = pActiveChild->GetActiveDocument();

   return (CPGSuperDoc*)(pDocument);
}

CView* CMainFrame::CreateOrActivateFrame(CCountedMultiDocTemplate* pTemplate,CRuntimeClass* pViewClass)
{
   // If a view (specified by pViewClass) already exists, 
   // then activate the MDI child window containing
   // the view.  Otherwise, create a new view for the document.
   m_bCreateCanceled = false;
   m_bDisableFailCreateMsg = false;

   CMDIChildWnd* pMDIActive = MDIGetActive();
   ASSERT(pMDIActive != NULL);

   CDocument* pDoc = pMDIActive->GetActiveDocument();
   ASSERT(pDoc != NULL);

   // How many of this type of view can we create?
   int max_view_count = pTemplate->GetMaxViewCount();

   if (max_view_count >= 1)
   {
      // Scan all views attached to the document, counting the number
      // of views that are the same type as pViewClass.  Retain a pointer
      // to the last view of this type for activation if necessary
      int view_count = 0;
      CView* pView;
      CView* pLastView = NULL;
      POSITION pos = pDoc->GetFirstViewPosition();
      while (pos != NULL && view_count <= max_view_count)
      {
         pView = pDoc->GetNextView(pos);
         if (pView->IsKindOf(pViewClass))
         {
            pLastView = pView;
            view_count++;
         }
      }

      if (view_count >= max_view_count)
      {
         // :TODO: rab 12.02.96 : Finish implementation
         // :METHOD: CMainFrame::CreateOrActivateFrame()
         //
         // The frame must be maximized if the current frame was maximized
         // The frame must be restored to its previous position if the
         // current frame is in its normal position.
         //
         // Want IDE behavior!!!


         // This attempt would cause the maximim view count to be
         // exceeded. Activate the last view of this type.
         pLastView->GetParentFrame()->ActivateFrame();
         return pLastView;
      }
   }
   

   // If we get this far, the view count is less than the maximum or the
   // maximum is unlimited (-1). Create a new view
   CView* pNewView = 0;
   CMDIChildWnd* pNewFrame = (CMDIChildWnd*)(pTemplate->CreateNewFrame(pDoc, pMDIActive));
   BOOL bCreated = (pNewFrame == NULL ? FALSE : TRUE);
   if ( !bCreated )
   {
      // Child frame was not created
      if ( !m_bDisableFailCreateMsg )
      {
         // We aren't disabling the message.
         CString msg;
         CString msg1;
         msg1.LoadString( IDS_E_CREATEWND );
         CString msg2;
         msg2.LoadString( IDS_E_LOWRESOURCES );
         AfxFormatString2( msg, IDS_E_FORMAT, msg1, msg2 );
         AfxMessageBox( msg );
      }

      pNewView = 0;
   }
   else
   {
      ASSERT_KINDOF(CMDIChildWnd, pNewFrame);

      pTemplate->InitialUpdateFrame(pNewFrame, pDoc);
      pNewView = pNewFrame->GetActiveView();
   }

   if ( m_bCreateCanceled )
   {
      // During the initial update process, the user canceled the creation of this
      // view (most likely the OnInitialUpdate() method (or one it calls) displayed
      // a progress window and ther user pressed the cancel button).

      // We need to destroy the view, but lets do it by destroying its frame window.
      pNewFrame->MDIDestroy();

      m_bCreateCanceled = false;
   }

   m_bCreateCanceled = false;
   m_bDisableFailCreateMsg = false;

   return pNewView;
}

void CMainFrame::UpdateFrameTitle(LPCTSTR lpszDocName)
{
   UpdateFrameTitleForDocument(lpszDocName);
}


void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
   // Copied from CMDIFrameWnd
   // Modified to not check if the window is maximized.
   // The filename is always displayed.

   if ((GetStyle() & FWS_ADDTOTITLE) == 0)
      return;     // leave it alone!

   CMDIChildWnd* pActiveChild = MDIGetActive();
   CDocument* pDocument = GetActiveDocument();

   if (pDocument == NULL && pActiveChild != NULL)
      pDocument = pActiveChild->GetActiveDocument();

   if (bAddToTitle &&
       (pActiveChild != NULL) &&
       (pDocument != NULL) )
   {
      if (pDocument->GetPathName().GetLength() == 0)
         UpdateFrameTitleForDocument(pDocument->GetTitle());
      else
      {
         char title[_MAX_PATH];
         WORD cbBuf = _MAX_PATH;

         ::GetFileTitle(pDocument->GetPathName(),title,cbBuf);

         UpdateFrameTitleForDocument(title);
      }
   }
   else
      UpdateFrameTitleForDocument(NULL);
}

void CMainFrame::UpdateFrameTitleForDocument(LPCTSTR lpszDocName)
{
   // Copied from CFrameWnd.
   // Modified to remove the :n

   // copy first part of title loaded at time of frame creation
   TCHAR szText[256+_MAX_PATH];

   if (GetStyle() & FWS_PREFIXTITLE)
   {
      szText[0] = '\0';   // start with nothing

      // get name of currently active view
      if (lpszDocName != NULL)
      {
         lstrcpy(szText, lpszDocName);
         lstrcat(szText, _T(" - "));
      }
      lstrcat(szText, m_strTitle);
   }
   else
   {
      // get name of currently active view
      lstrcpy(szText, m_strTitle);
      if (lpszDocName != NULL)
      {
         lstrcat(szText, _T(" - "));
         lstrcat(szText, lpszDocName);
      }
   }

   // set title if changed, but don't remove completely
   // Note: will be excessive for MDI Frame with maximized child
   AfxSetWindowText(m_hWnd, szText);
}

void CMainFrame::OnViewBridgeModelEditor() 
{
   CreateOrActivateFrame(theApp.GetBridgeModelEditorTemplate(),RUNTIME_CLASS(CBridgePlanView));
}

void CMainFrame::CreateAnalysisResultsView()
{
   OnViewAnalysisResults();
}

void CMainFrame::CreateGirderEditorView(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   CView* pView = CreateOrActivateFrame(theApp.GetGirderModelEditorTemplate(),RUNTIME_CLASS(CGirderModelElevationView));
   CGirderModelChildFrame* pFrame = (CGirderModelChildFrame*)(pView->GetParent()->GetParent());
   pFrame->SelectSpanAndGirder(spanIdx,gdrIdx);
}

void CMainFrame::OnViewGirderEditor() 
{
   CPGSuperDoc* pDoc = GetPGSuperDocument();
   if ( pDoc->GetStatusCenter().GetSeverity() == pgsTypes::statusError )
   {
      AfxMessageBox("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details.",MB_OK);
   }
   else
   {
      CreateOrActivateFrame(theApp.GetGirderModelEditorTemplate(),RUNTIME_CLASS(CGirderModelElevationView));
   }
}

void CMainFrame::OnViewLibraryEditor() 
{
   CreateOrActivateFrame(theApp.GetLibraryEditorTemplate(),RUNTIME_CLASS(CLibraryEditorView));
}

void CMainFrame::OnViewAnalysisResults() 
{
   CPGSuperDoc* pDoc = GetPGSuperDocument();
   if ( pDoc->GetStatusCenter().GetSeverity() == pgsTypes::statusError )
   {
      AfxMessageBox("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details.",MB_OK);
   }
   else
   {
      CreateOrActivateFrame(theApp.GetAnalysisResultsViewTemplate(),RUNTIME_CLASS(CAnalysisResultsView));
   }
}

void CMainFrame::OnViewReports(NMHDR* pnmhdr,LRESULT* plr) 
{
   // INVALID_INDEX causes the user to be prompted to select a report
   //CreateReport(INVALID_INDEX);

   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_VIEW_REPORTS )
      return; // not our button

   CPGSuperDoc* pDoc = GetPGSuperDocument();

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_REPORTS) );

   CMenu* pMenu = menu.GetSubMenu(0);

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   GET_IFACE2( pBroker, IReportManager, pRptMgr );
   std::vector<std::string> rptNames = pRptMgr->GetReportNames();

   int i = 0;
   std::vector<std::string>::iterator iter;
   for ( iter = rptNames.begin(); iter != rptNames.end(); iter++ )
   {
      std::string rptName = *iter;
      pMenu->AppendMenuA(MF_STRING,pDoc->GetReportCommand(i,false),rptName.c_str());
      i++;
   }

   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CToolBar* pToolBar = m_ToolBars[GetToolBarIndex(IDR_STDTOOLBAR)].get();
   int idx = pToolBar->CommandToIndex(ID_VIEW_REPORTS);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, this );
}

void CMainFrame::CreateReport(CollectionIndexType rptIdx,bool bPromptForSpec)
{
   CPGSuperDoc* pDoc = GetPGSuperDocument();
   if ( pDoc->GetStatusCenter().GetSeverity() == pgsTypes::statusError )
   {
      AfxMessageBox("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details.",MB_OK);
   }
   else
   {
      CreateOrActivateFrame(theApp.GetReportViewTemplate(rptIdx,bPromptForSpec),RUNTIME_CLASS(CReportView));
   }
}

void CMainFrame::OnViewStability() 
{
   CPGSuperDoc* pDoc = GetPGSuperDocument();
   if ( pDoc->GetStatusCenter().GetSeverity() == pgsTypes::statusError )
   {
      AfxMessageBox("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details.",MB_OK);
   }
   else
   {
      CreateOrActivateFrame(theApp.GetFactorOfSafetyViewTemplate(),RUNTIME_CLASS(CFactorOfSafetyView));
   }
}

void CMainFrame::OnEditUserloads() 
{
   CreateOrActivateFrame(theApp.GetEditLoadsViewTemplate(),RUNTIME_CLASS(CEditLoadsView));
}

void CMainFrame::AutoCalcEnabled( bool bEnable )
{
   CString status_text;
   if ( bEnable )
      status_text.LoadString(ID_INDICATOR_AUTOCALC_ON);
   else
      status_text.LoadString(ID_INDICATOR_AUTOCALC_OFF);

   m_wndStatusBar.SetPaneText(4, status_text, TRUE);
}

void CMainFrame::EnableModifiedFlag(BOOL bEnable)
{
   UINT nStyle;
    
   if ( bEnable )
      nStyle = SBPS_NORMAL;
   else
      nStyle = SBPS_DISABLED;

   if ( m_wndStatusBar.GetSafeHwnd() )
      m_wndStatusBar.SetPaneStyle( 3, nStyle );
}

void CMainFrame::SetAnalysisTypeStatusIndicator(pgsTypes::AnalysisType analysisType)
{
   CString strAnalysisType;
   switch( analysisType )
   {
   case pgsTypes::Simple:
      strAnalysisType = "Simple Span";
      break;
   case pgsTypes::Continuous:
      strAnalysisType = "Continuous";
      break;
   case pgsTypes::Envelope:
      strAnalysisType = "Envelope";
      break;
   }

   m_wndStatusBar.SetPaneText(1,strAnalysisType,TRUE);
}

void CMainFrame::DisableFailCreateMessage()
{
   m_bDisableFailCreateMsg = true;
}

void CMainFrame::CreateCanceled()
{
   m_bCreateCanceled = true;
}

void CMainFrame::OnClose() 
{
   SaveBarState( CString((LPCSTR)IDS_TOOLBAR_STATE) );

   // Save the ToolTips state
   AfxGetApp()->WriteProfileInt(CString((LPCSTR)IDS_REG_SETTINGS),
                                CString((LPCSTR)IDS_TOOLTIP_STATE),
                                (m_bShowToolTips != 0) );

   // Save the layout of the application window
   WINDOWPLACEMENT wp;
   wp.length = sizeof wp;
   if (GetWindowPlacement(&wp))
   {
      wp.flags = 0;
      wp.showCmd = SW_SHOWNORMAL;
      ((CPGSuperApp*)AfxGetApp())->WriteWindowPlacement(CString((LPCSTR)IDS_REG_WNDPOS),&wp);
   }

   CMDIFrameWnd::OnClose();
}


void CMainFrame::DockControlBarLeftOf(CToolBar* Bar,CToolBar* LeftOf)
{
   CRect rect;
   DWORD dw;
   UINT n;

   // get MFC to adjust the dimensions of all docked ToolBars
   // so that GetWindowRect will be accurate
   RecalcLayout();
   LeftOf->GetWindowRect(&rect);
   rect.OffsetRect(1,0);
   dw=LeftOf->GetBarStyle();
   n = 0;
   n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
   n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
   n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
   n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;

   // When we take the default parameters on rect, DockControlBar will dock
   // each Toolbar on a seperate line.  By calculating a rectangle, we in effect
   // are simulating a Toolbar being dragged to that location and docked.
   DockControlBar(Bar,n,&rect);
}

std::vector<UINT> CMainFrame::GetToolBarResourceIDs()
{
   std::vector<UINT> vNames;

   // Resource ID's of the toolbars.
   vNames.push_back( IDR_STDTOOLBAR  );
   vNames.push_back( IDR_LIBTOOLBAR );
   vNames.push_back( IDR_HELPTOOLBAR );

   return vNames;
}

int CMainFrame::GetToolBarIndex(UINT nID)
{
   std::vector<UINT>::iterator found;
   std::vector<UINT> vNames = GetToolBarResourceIDs();
   found = std::find(vNames.begin(),vNames.end(),nID);
   if ( found == vNames.end() )
      return -1;

   return (found - vNames.begin());
}

std::vector<BOOL> CMainFrame::GetToolBarStates()
{
   std::vector<BOOL> vStates;

   std::vector<boost::shared_ptr<CToolBar> >::iterator iter;
   for ( iter = m_ToolBars.begin(); iter < m_ToolBars.end(); iter++ )
   {
      CToolBar* pToolBar = (*iter).get();
      vStates.push_back( pToolBar->IsWindowVisible() ? TRUE : FALSE );
   }

   return vStates;
}

void CMainFrame::SetToolBarStates(const std::vector<BOOL>& vStates)
{
   CHECK( vStates.size() == m_ToolBars.size() );

   std::vector<boost::shared_ptr<CToolBar> >::iterator tb_iter = m_ToolBars.begin();
   std::vector<BOOL>::const_iterator state_iter = vStates.begin();
   for ( ; tb_iter < m_ToolBars.end() && state_iter < vStates.end(); tb_iter++, state_iter++ )
   {
      CToolBar* pToolBar = (*tb_iter).get();
      BOOL bShow = *state_iter;
      BOOL bIsVisible = pToolBar->IsWindowVisible();
  
      if ( bIsVisible && !bShow || !bIsVisible && bShow )
        ShowControlBar( pToolBar, bShow, FALSE );

      DWORD dwStyle = pToolBar->GetBarStyle();
      BOOL bToolTipsEnabled = sysFlags<DWORD>::IsSet( dwStyle, CBRS_TOOLTIPS );
      if ( bToolTipsEnabled && !m_bShowToolTips || !bToolTipsEnabled && m_bShowToolTips )
      {
         if ( m_bShowToolTips )
            sysFlags<DWORD>::Set( &dwStyle, CBRS_TOOLTIPS | CBRS_FLYBY );
         else
            sysFlags<DWORD>::Clear( &dwStyle, CBRS_TOOLTIPS | CBRS_FLYBY );

         pToolBar->SetBarStyle( dwStyle );
      }
   }
}

void CMainFrame::ToggleToolBarState(Uint16 idx)
{
   CToolBar* pToolBar = m_ToolBars[idx].get();
   BOOL bIsVisible = pToolBar->IsWindowVisible();
   ShowControlBar( pToolBar, !bIsVisible, FALSE );
}

void CMainFrame::OnViewToolbar() 
{
   CToolBarDlg dlg;
   dlg.m_ToolBarResId  = GetToolBarResourceIDs();
   dlg.m_ToolBarStates = GetToolBarStates();
   dlg.m_bShowToolTips = m_bShowToolTips;
   
   if ( dlg.DoModal() == IDOK )
   {
      m_bShowToolTips = dlg.m_bShowToolTips;
      SetToolBarStates( dlg.m_ToolBarStates );
   }
}

void CMainFrame::OnUpdateViewToolbar(CCmdUI* pCmdUI) 
{
   // Does nothing... Gobble up this message so the MFC
   // framework wont put a check mark next to the menu item.
}

void CMainFrame::OnUpdateEditors(CCmdUI* pCmdUI)
{
   CPGSuperDoc* pDoc = GetPGSuperDocument();
   if ( pDoc == 0 )
   {
      pCmdUI->Enable( FALSE );
      return;
   }

   pCmdUI->Enable( TRUE );
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
   // Add a toolbar popup menu if a right click happens in the docking space
   if (pMsg->message == WM_RBUTTONDOWN)
   {
      CWnd* pWnd = CWnd::FromHandlePermanent(pMsg->hwnd);
      CControlBar* pBar = DYNAMIC_DOWNCAST(CControlBar, pWnd);

      if (pBar != NULL)
      {
         CMenu menu;

         std::vector<BOOL> vStates = GetToolBarStates();
         std::vector<UINT> vID     = GetToolBarResourceIDs();
         CHECK( vStates.size() == vID.size() );

         std::vector<BOOL>::iterator state_iter = vStates.begin();
         std::vector<UINT>::iterator id_iter    = vID.begin();

         menu.CreatePopupMenu();

         UINT offset = 0;
         for ( ; state_iter < vStates.end() && id_iter < vID.end(); state_iter++, id_iter++ )
         {
            BOOL bShown = *state_iter;
            UINT nID = *id_iter;

            UINT iFlags = MF_STRING | MF_ENABLED;
            if ( bShown )
               iFlags |= MF_CHECKED;

            menu.AppendMenu( iFlags, CMENU_BASE + offset, CString((LPCSTR)nID) );
            offset++;
         }

         CPoint pt;
         pt.x = LOWORD(pMsg->lParam);
         pt.y = HIWORD(pMsg->lParam);
         pBar->ClientToScreen(&pt);

         menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this );
      }
   }
   
   return CMDIFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnToolbarMenuSelected(UINT id)
{
   Uint16 idx = id - CMENU_BASE;
   ToggleToolBarState( idx );
}


void CMainFrame::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_DISPLAY_TOPIC, 0 );
}

void CMainFrame::OnHelpFinder()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_FINDER, 0 );
}

void CMainFrame::OnDropFiles(HDROP hDropInfo) 
{
   // Don't allow multiple files to be dropped
	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
   if (nFiles>1)
      ::AfxMessageBox("Error - Multiple file drop not allowed. Please drop only one file at a time onto PGSuper",MB_ICONEXCLAMATION|MB_OK);
   else
      CMDIFrameWnd::OnDropFiles(hDropInfo);
}

void CMainFrame::UpdateToolbarStatusItems(pgsTypes::StatusSeverityType severity)
{
   CToolBar* pToolbar = m_ToolBars[GetToolBarIndex(IDR_STDTOOLBAR)].get();

   switch(severity)
   {
   case pgsTypes::statusOK:
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER, FALSE);
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER2,TRUE);
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER3,TRUE);
      break;

   case pgsTypes::statusWarning:
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER, TRUE);
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER2,FALSE);
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER3,TRUE);
      break;

   case pgsTypes::statusError:
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER, TRUE);
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER2,TRUE);
      pToolbar->GetToolBarCtrl().HideButton(ID_VIEW_STATUSCENTER3,FALSE);
      break;
   }
}

void CMainFrame::UpdateStatusBar()
{
   CRect rect;
   if ( m_wndStatusBar.GetSafeHwnd() )
   {
      m_wndStatusBar.GetStatusBarCtrl().GetRect(2,&rect);
      m_wndStatusBar.InvalidateRect(rect);
      m_wndStatusBar.UpdateWindow();
   }
}

