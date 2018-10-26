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
// EditLoadsView.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EditLoadsView.h"

#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <..\htmlhelp\HelpTopics.hh>

#include "InsertDeleteLoad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// define word values for point load and distributed loads. 
// these are put in loword of itemdata in the list control
#define W_POINT_LOAD 1
#define W_DISTRIBUTED_LOAD 2
#define W_MOMENT_LOAD 3

#define BORDER 10

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsView

IMPLEMENT_DYNCREATE(CEditLoadsView, CFormView)

CEditLoadsView::CEditLoadsView()
	: CFormView(CEditLoadsView::IDD),
   m_FirstSizeEvent(true),
   m_FormatTool(sysNumericFormatTool::Automatic, 9, 4)
{
	//{{AFX_DATA_INIT(CEditLoadsView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CEditLoadsView::~CEditLoadsView()
{
}

void CEditLoadsView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditLoadsView)
	DDX_Control(pDX, IDC_LOAD_STATIC, m_StaticCtrl);
	DDX_Control(pDX, ID_HELP, m_HelpCtrl);
	DDX_Control(pDX, IDC_ADD_POINTLOAD, m_AddPointCtrl);
	DDX_Control(pDX, IDC_ADD_MOMENTLOAD, m_AddMomentCtrl);
	DDX_Control(pDX, IDC_ADD_NEW_DISTRIBUTED, m_AddDistributedCtrl);
	DDX_Control(pDX, IDC_DELETE_LOAD, m_DeleteCtrl);
	DDX_Control(pDX, IDC_EDIT_LOAD, m_EditCtrl);
	DDX_Control(pDX, IDC_LOADS_LIST, m_LoadsListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditLoadsView, CFormView)
	//{{AFX_MSG_MAP(CEditLoadsView)
	ON_BN_CLICKED(IDC_ADD_POINTLOAD, OnAddPointload)
	ON_BN_CLICKED(IDC_ADD_MOMENTLOAD, OnAddMomentload)
	ON_BN_CLICKED(IDC_DELETE_LOAD, OnDeleteLoad)
	ON_BN_CLICKED(IDC_EDIT_LOAD, OnEditLoad)
	ON_NOTIFY(NM_DBLCLK, IDC_LOADS_LIST, OnDblclkLoadsList)
	ON_BN_CLICKED(IDC_ADD_NEW_DISTRIBUTED, OnAddNewDistributed)
	ON_NOTIFY(NM_CLICK, IDC_LOADS_LIST, OnClickLoadsList)
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsView diagnostics

#ifdef _DEBUG
void CEditLoadsView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CFormView::AssertValid();
}

void CEditLoadsView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsView message handlers

void CEditLoadsView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if (!m_IsInitialUpdate)
   {
      if (lHint==HINT_UNITSCHANGED)
      {
         UpdateUnits();
      }

      InsertData();
   }
}

void CEditLoadsView::OnInitialUpdate() 
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetDocument();
   pDoc->GetBroker(&m_pBroker);

   UpdateUnits();

   m_IsInitialUpdate = true;  // have to play a game here to get onupdate to work right

	CFormView::OnInitialUpdate();

   m_IsInitialUpdate = false;

   // make it so entire row is selected in list control
   m_LoadsListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT );

   RECT rect;
   m_LoadsListCtrl.GetClientRect(&rect);
   int wid = rect.right;
   int lft_wid = (int)(wid*0.65/5.0);  // first five columns take most of window
   int rgt_wid = (int)((wid-lft_wid*5.0)/3.0);
	
   int st;
   st = m_LoadsListCtrl.InsertColumn(0,"Type",LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(1,"Stage",LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(2,"Load Case",LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(3,"Span",LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(4,"Girder",LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(5,"Location",LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   std::string flab("Magnitude");
   st = m_LoadsListCtrl.InsertColumn(6,flab.c_str(),LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   st = m_LoadsListCtrl.InsertColumn(7,"Description",LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   InsertData();
}

void CEditLoadsView::OnAddPointload() 
{
	CEditPointLoadDlg dlg(CPointLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CEditLoadsView::OnAddMomentload() 
{
	CEditMomentLoadDlg dlg(CMomentLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CEditLoadsView::OnDeleteLoad() 
{
   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );
   if (pos!=NULL)
   {
      int nItem = m_LoadsListCtrl.GetNextSelectedItem(pos);

      DWORD data = m_LoadsListCtrl.GetItemData(nItem);
      WORD load_type = LOWORD(data);
      WORD load_idx = HIWORD(data);

      GET_IFACE(IUserDefinedLoadData, pUdl);

      if (load_type ==W_POINT_LOAD)
      {
         txnDeletePointLoad* pTxn = new txnDeletePointLoad(load_idx);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
      else if (load_type ==W_DISTRIBUTED_LOAD)
      {
         txnDeleteDistributedLoad* pTxn = new txnDeleteDistributedLoad(load_idx);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
      else if (load_type ==W_MOMENT_LOAD)
      {
         txnDeleteMomentLoad* pTxn = new txnDeleteMomentLoad(load_idx);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
      else
         CHECK(0);
   }
}

void CEditLoadsView::OnEditLoad() 
{
   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );

   if (pos!=NULL)
   {
      EditLoad(pos);
   }
}

void CEditLoadsView::UpdateUnits()
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   m_bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   if (m_bUnitsSI)
   {
      m_pLengthUnit = &unitMeasure::Meter;
      m_pForceUnit = &unitMeasure::Kilonewton;
      m_pForcePerLengthUnit = &unitMeasure::KilonewtonPerMeter;
      m_pMomentUnit = &unitMeasure::KilonewtonMeter;
   }
   else
   {
      m_pLengthUnit = &unitMeasure::Feet;
      m_pForceUnit = &unitMeasure::Kip;
      m_pForcePerLengthUnit = &unitMeasure::KipPerFoot;
      m_pMomentUnit = &unitMeasure::KipFeet;
   }
}


void CEditLoadsView::OnDblclkLoadsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );

   if (pos!=NULL)
   {
      EditLoad(pos);
   }

	*pResult = 0;
}

void CEditLoadsView::OnAddNewDistributed() 
{
	CEditDistributedLoadDlg dlg(CDistributedLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CEditLoadsView::InsertData()
{
   // for now just delete all loads and rebuild list
   m_LoadsListCtrl.DeleteAllItems();

   int irow=0;

   // Add Point Loads
   GET_IFACE(IUserDefinedLoadData, pUdl);
   Uint32 pl_cnt = pUdl->GetPointLoadCount();
   Uint32 ipl;
   for (ipl=0; ipl<pl_cnt; ipl++)
   {
      m_LoadsListCtrl.InsertItem(irow, "Point");

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_POINT_LOAD, ipl) );

      CPointLoadData rld = pUdl->GetPointLoad(ipl);

      UpdatePointLoadItem(irow, rld);

      irow++;
   }

   // Add Moment Loads
   pl_cnt = pUdl->GetMomentLoadCount();
   for (ipl=0; ipl<pl_cnt; ipl++)
   {
      m_LoadsListCtrl.InsertItem(irow, "Moment");

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_MOMENT_LOAD, ipl) );

      CMomentLoadData rld = pUdl->GetMomentLoad(ipl);

      UpdateMomentLoadItem(irow, rld);

      irow++;
   }

   // Add Distributed Loads
   Int32 dl_cnt = pUdl->GetDistributedLoadCount();
   for (Int32 idl=0; idl<dl_cnt; idl++)
   {
      m_LoadsListCtrl.InsertItem(irow, "Distributed");

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_DISTRIBUTED_LOAD, idl) );

      CDistributedLoadData rld = pUdl->GetDistributedLoad(idl);

      UpdateDistributedLoadItem(irow, rld);

      irow++;
   }

   // nothing is selected by default, so we can't edit or delete
   m_EditCtrl.EnableWindow(FALSE);
   m_DeleteCtrl.EnableWindow(FALSE);
}

void CEditLoadsView::UpdatePointLoadItem(int irow, const CPointLoadData& rld)
{
   m_LoadsListCtrl.SetItemText(irow, 1, UserLoads::GetStageName(rld.m_Stage).c_str());
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(rld.m_LoadCase).c_str());

   CString str;
   if (rld.m_Span != UserLoads::AllSpans)
      str.Format("%d", LABEL_SPAN(rld.m_Span));
   else
      str = "All Spans";

   m_LoadsListCtrl.SetItemText(irow, 3, str);

   if (rld.m_Girder != UserLoads::AllGirders)
      str.Format("%s", LABEL_GIRDER(rld.m_Girder));
   else
      str = "All Girders";

   m_LoadsListCtrl.SetItemText(irow, 4, str);

   std::string stdstr;
   if (rld.m_Fractional)
   {
      stdstr = D2S(rld.m_Location*100.0) + std::string(" %");
   }
   else
   {
      Float64 val = ::ConvertFromSysUnits(rld.m_Location, *m_pLengthUnit);
      stdstr = D2S(val) + std::string(" ") +  m_pLengthUnit->UnitTag();
   }

   m_LoadsListCtrl.SetItemText(irow, 5, stdstr.c_str());

   Float64 val = ::ConvertFromSysUnits(rld.m_Magnitude, *m_pForceUnit);
   stdstr = D2S(val) + std::string(" ") + m_pForceUnit->UnitTag();

   m_LoadsListCtrl.SetItemText(irow, 6, stdstr.c_str());
   m_LoadsListCtrl.SetItemText(irow, 7, rld.m_Description.c_str());
}

void CEditLoadsView::UpdateMomentLoadItem(int irow, const CMomentLoadData& rld)
{
   m_LoadsListCtrl.SetItemText(irow, 1, UserLoads::GetStageName(rld.m_Stage).c_str());
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(rld.m_LoadCase).c_str());

   CString str;
   if (rld.m_Span != UserLoads::AllSpans)
      str.Format("%d", LABEL_SPAN(rld.m_Span));
   else
      str = "All Spans";

   m_LoadsListCtrl.SetItemText(irow, 3, str);

   if (rld.m_Girder != UserLoads::AllGirders)
      str.Format("%s", LABEL_GIRDER(rld.m_Girder));
   else
      str = "All Girders";

   m_LoadsListCtrl.SetItemText(irow, 4, str);

   std::string stdstr;
   if (rld.m_Fractional)
   {
      stdstr = D2S(rld.m_Location*100.0) + std::string(" %");
   }
   else
   {
      Float64 val = ::ConvertFromSysUnits(rld.m_Location, *m_pLengthUnit);
      stdstr = D2S(val) + std::string(" ") +  m_pLengthUnit->UnitTag();
   }

   m_LoadsListCtrl.SetItemText(irow, 5, stdstr.c_str());

   Float64 val = ::ConvertFromSysUnits(rld.m_Magnitude, *m_pMomentUnit);
   stdstr = D2S(val) + std::string(" ") + m_pMomentUnit->UnitTag();

   m_LoadsListCtrl.SetItemText(irow, 6, stdstr.c_str());
   m_LoadsListCtrl.SetItemText(irow, 7, rld.m_Description.c_str());
}

void CEditLoadsView::UpdateDistributedLoadItem(int irow, const CDistributedLoadData& rld)
{
   if (rld.m_Type==UserLoads::Uniform)
   {
      m_LoadsListCtrl.SetItemText(irow, 0, "Uniform");
   }
   else
   {
      m_LoadsListCtrl.SetItemText(irow, 0, "Trapezoidal");
   }

   m_LoadsListCtrl.SetItemText(irow, 1, UserLoads::GetStageName(rld.m_Stage).c_str());
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(rld.m_LoadCase).c_str());

   CString str;
   if (rld.m_Span != UserLoads::AllSpans)
      str.Format("%d", LABEL_SPAN(rld.m_Span));
   else
      str = "All Spans";

   m_LoadsListCtrl.SetItemText(irow, 3, str);

   if (rld.m_Girder != UserLoads::AllGirders)
      str.Format("%s", LABEL_GIRDER(rld.m_Girder));
   else
      str = "All Girders";

   m_LoadsListCtrl.SetItemText(irow, 4, str);

   std::string stdstr;

   if (rld.m_Type==UserLoads::Uniform)
   {
      stdstr = "Entire Span";
   }
   else
   {
      if (rld.m_Fractional)
      {
         stdstr = D2S(rld.m_StartLocation*100.0) + std::string(" - ") + D2S(rld.m_EndLocation*100.0) + std::string(" %");;
      }
      else
      {
         Float64 startval = ::ConvertFromSysUnits(rld.m_StartLocation, *m_pLengthUnit);
         Float64 endval = ::ConvertFromSysUnits(rld.m_EndLocation, *m_pLengthUnit);
         stdstr = D2S(startval) + std::string(" - ") + D2S(endval) + std::string(" ") +  m_pLengthUnit->UnitTag();
      }
   }

   m_LoadsListCtrl.SetItemText(irow, 5, stdstr.c_str());

   if (rld.m_Type==UserLoads::Uniform)
   {
      Float64 val = ::ConvertFromSysUnits(rld.m_WStart, *m_pForcePerLengthUnit);
      stdstr = D2S(val) + std::string(" ") + m_pForcePerLengthUnit->UnitTag();
   }
   else
   {
      Float64 startval = ::ConvertFromSysUnits(rld.m_WStart, *m_pForcePerLengthUnit);
      Float64 endval = ::ConvertFromSysUnits(rld.m_WEnd, *m_pForcePerLengthUnit);
      stdstr = D2S(startval) + std::string(" - ") + D2S(endval) + std::string(" ") + m_pForcePerLengthUnit->UnitTag();
   }

   m_LoadsListCtrl.SetItemText(irow, 6, stdstr.c_str());
   m_LoadsListCtrl.SetItemText(irow, 7, rld.m_Description.c_str());
}

void CEditLoadsView::EditLoad(POSITION pos)
{
   int nItem = m_LoadsListCtrl.GetNextSelectedItem(pos);

   DWORD data = m_LoadsListCtrl.GetItemData(nItem);
   WORD load_type = LOWORD(data);
   WORD load_idx = HIWORD(data);

   GET_IFACE(IUserDefinedLoadData, pUdl);

   if (load_type ==W_POINT_LOAD)
   {
      // edit our point load
      CPointLoadData rld = pUdl->GetPointLoad(load_idx);

	   CEditPointLoadDlg dlg(rld,m_pBroker);
      if (dlg.DoModal() == IDOK)
      {
         if (rld!=dlg.m_Load)
         {
            txnEditPointLoad* pTxn = new txnEditPointLoad(load_idx,rld,dlg.m_Load);
            txnTxnManager::GetInstance()->Execute(pTxn);
            UpdatePointLoadItem(nItem, dlg.m_Load);
         }
      }
   }
   else if (load_type ==W_MOMENT_LOAD)
   {
      // edit our moment load
      CMomentLoadData rld = pUdl->GetMomentLoad(load_idx);

	   CEditMomentLoadDlg dlg(rld,m_pBroker);
      if (dlg.DoModal() == IDOK)
      {
         if (rld!=dlg.m_Load)
         {
            txnEditMomentLoad* pTxn = new txnEditMomentLoad(load_idx,rld,dlg.m_Load);
            txnTxnManager::GetInstance()->Execute(pTxn);
            UpdateMomentLoadItem(nItem, dlg.m_Load);
         }
      }
   }
   else if (load_type ==W_DISTRIBUTED_LOAD)
   {
      // edit our distributed load
      CDistributedLoadData rld = pUdl->GetDistributedLoad(load_idx);

	   CEditDistributedLoadDlg dlg(rld,m_pBroker);
      if (dlg.DoModal() == IDOK)
      {
         if (rld!=dlg.m_Load)
         {
            txnEditDistributedLoad* pTxn = new txnEditDistributedLoad(load_idx,rld,dlg.m_Load);
            txnTxnManager::GetInstance()->Execute(pTxn);
            UpdateDistributedLoadItem(nItem, dlg.m_Load);
         }
      }
   }
   else
      CHECK(0);

}

void CEditLoadsView::OnClickLoadsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );

   BOOL sel = (pos!=NULL) ? TRUE:FALSE;
   m_EditCtrl.EnableWindow(sel);
   m_DeleteCtrl.EnableWindow(sel);
	
	*pResult = 0;
}


void CEditLoadsView::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
	
   CWnd* pLoads  = GetDlgItem(IDC_LOADS_LIST);
   if ( pLoads == NULL)
      return;

   CWnd* pAddP   = GetDlgItem(IDC_ADD_POINTLOAD);
   CWnd* pAddM   = GetDlgItem(IDC_ADD_MOMENTLOAD);
   CWnd* pAddD   = GetDlgItem(IDC_ADD_NEW_DISTRIBUTED);
   CWnd* pEdit   = GetDlgItem(IDC_EDIT_LOAD);
   CWnd* pDelete = GetDlgItem(IDC_DELETE_LOAD);
   CWnd* pHelp   = GetDlgItem(ID_HELP);
   CWnd* pStatic = GetDlgItem(IDC_LOAD_STATIC);

   if (m_FirstSizeEvent)
   {
      // compute minimum size that we can become to avoid control collision
      m_FirstSizeEvent = false;

      CRect rBtnP, rBtnE, rBtnH, rLoads;
      pAddP->GetClientRect(&rBtnP); 
      pEdit->GetClientRect(&rBtnE); 
      pHelp->GetClientRect(&rBtnH);
      pLoads->GetClientRect(&rLoads);

      m_MinSize.cx = 2*rBtnE.Width() + 2*rBtnP.Width() + rBtnH.Width() + 8*BORDER;

      // min height
      m_MinSize.cy = rBtnP.Height() + rLoads.Height() + 5*BORDER;

      m_But1Size.cy =  rBtnE.Height();
      m_But1Size.cx =  rBtnE.Width();
      m_But2Size.cy =  rBtnP.Height();
      m_But2Size.cx =  rBtnP.Width();
      m_HelpButWidth = rBtnH.Width();
   }

   CRect rc;
   GetClientRect(&rc);

   int width  = max(m_MinSize.cx, rc.Width());
   int height = max(m_MinSize.cy, rc.Height());

   // static
   int w = width - 2*BORDER;
   int h = height - 3*BORDER - m_But1Size.cy;
   pStatic->SetWindowPos(NULL,0,0,w,h,SWP_NOMOVE | SWP_NOZORDER);

   // list
   w = w - 2*BORDER;
   h = h - m_But1Size.cy - BORDER;
   pLoads->SetWindowPos(NULL,0,0,w,h,SWP_NOMOVE | SWP_NOZORDER);

   // buttons
   int x = 2*BORDER;
   int y = height - 3*BORDER;
   pEdit->SetWindowPos(NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But1Size.cx + BORDER;
   pDelete->SetWindowPos(NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But1Size.cx + BORDER;
   pAddP->SetWindowPos(NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But2Size.cx + BORDER;
   pAddD->SetWindowPos(NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But2Size.cx + BORDER;
   pAddM->SetWindowPos(NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);

   x = width - m_HelpButWidth - BORDER;
   pHelp->SetWindowPos(NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
}


void CEditLoadsView::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_EDIT_LOADS );
	
}

std::string CEditLoadsView::D2S(Float64 val)
{
   // format number and strip off white space
   std::string str = m_FormatTool.AsString(val);

   std::string::size_type  notwhite = str.find_first_not_of(" \t\n");
   str.erase(0,notwhite);

   return str;
}

BOOL CEditLoadsView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
   // TODO: Add your specialized code here and/or call the base class
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}
