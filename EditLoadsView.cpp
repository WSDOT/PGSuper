///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EditLoadsView.h"

#include "PGSuperAppPlugin\resource.h"

#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <..\htmlhelp\HelpTopics.hh>

#include <PgsExt\InsertDeleteLoad.h>

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

   m_SortColIdx = -1; // not sorted
   m_bSortAscending = true;
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
	ON_NOTIFY(NM_RCLICK, IDC_LOADS_LIST, OnClickLoadsList)
	ON_WM_SIZE()
   ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_NOTIFY(HDN_ITEMCLICK,0,OnHeaderClicked)
	ON_COMMAND(ID_EDIT_LOAD, OnEditLoad)
	ON_COMMAND(ID_DELETE_LOAD, OnDeleteLoad)
	ON_COMMAND(ID_ADD_POINT_LOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddNewDistributed)
	ON_COMMAND(ID_ADD_MOMENT_LOAD, OnAddMomentload)
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
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

      if ( lHint == HINT_GIRDERCHANGED )
      {
         CGirderHint* pGirderHint = (CGirderHint*)pHint;
         if ( pGirderHint->lHint == GCH_LOADING_ADDED || pGirderHint->lHint == GCH_LOADING_REMOVED )
         {
            InsertData();
            Sort(m_SortColIdx,false);
         }
      }

   }
}

void CEditLoadsView::OnInitialUpdate() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   EAFGetBroker(&m_pBroker);

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
   st = m_LoadsListCtrl.InsertColumn(0,_T("Type"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(1,_T("Stage"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(2,_T("Load Case"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(3,_T("Span"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(4,_T("Girder"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(5,_T("Location"),LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   std::_tstring flab(_T("Magnitude"));
   st = m_LoadsListCtrl.InsertColumn(6,flab.c_str(),LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   st = m_LoadsListCtrl.InsertColumn(7,_T("Description"),LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   InsertData();
   Sort(m_SortColIdx,false);
}

void CEditLoadsView::OnAddPointload() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CEditPointLoadDlg dlg(CPointLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CEditLoadsView::OnAddMomentload() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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

      DWORD_PTR data = m_LoadsListCtrl.GetItemData(nItem);
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
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
   CollectionIndexType pl_cnt = pUdl->GetPointLoadCount();
   CollectionIndexType ipl;
   for (ipl=0; ipl<pl_cnt; ipl++)
   {
      m_LoadsListCtrl.InsertItem(irow, _T("Point"));

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
      m_LoadsListCtrl.InsertItem(irow, _T("Moment"));

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_MOMENT_LOAD, ipl) );

      CMomentLoadData rld = pUdl->GetMomentLoad(ipl);

      UpdateMomentLoadItem(irow, rld);

      irow++;
   }

   // Add Distributed Loads
   CollectionIndexType dl_cnt = pUdl->GetDistributedLoadCount();
   for (CollectionIndexType idl=0; idl<dl_cnt; idl++)
   {
      m_LoadsListCtrl.InsertItem(irow, _T("Distributed"));

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
   if (rld.m_Span != ALL_SPANS)
      str.Format(_T("%d"), LABEL_SPAN(rld.m_Span));
   else
      str = _T("All Spans");

   m_LoadsListCtrl.SetItemText(irow, 3, str);

   if (rld.m_Girder != ALL_GIRDERS)
      str.Format(_T("%s"), LABEL_GIRDER(rld.m_Girder));
   else
      str = _T("All Girders");

   m_LoadsListCtrl.SetItemText(irow, 4, str);

   std::_tstring stdstr;
   if (rld.m_Fractional)
   {
      stdstr = D2S(rld.m_Location*100.0) + std::_tstring(_T(" %"));
   }
   else
   {
      Float64 val = ::ConvertFromSysUnits(rld.m_Location, *m_pLengthUnit);
      stdstr = D2S(val) + std::_tstring(_T(" ")) +  m_pLengthUnit->UnitTag();
   }

   m_LoadsListCtrl.SetItemText(irow, 5, stdstr.c_str());

   Float64 val = ::ConvertFromSysUnits(rld.m_Magnitude, *m_pForceUnit);
   stdstr = D2S(val) + std::_tstring(_T(" ")) + m_pForceUnit->UnitTag();

   m_LoadsListCtrl.SetItemText(irow, 6, stdstr.c_str());
   m_LoadsListCtrl.SetItemText(irow, 7, rld.m_Description.c_str());
}

void CEditLoadsView::UpdateMomentLoadItem(int irow, const CMomentLoadData& rld)
{
   m_LoadsListCtrl.SetItemText(irow, 1, UserLoads::GetStageName(rld.m_Stage).c_str());
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(rld.m_LoadCase).c_str());

   CString str;
   if (rld.m_Span != ALL_SPANS)
      str.Format(_T("%d"), LABEL_SPAN(rld.m_Span));
   else
      str = _T("All Spans");

   m_LoadsListCtrl.SetItemText(irow, 3, str);

   if (rld.m_Girder != ALL_GIRDERS)
      str.Format(_T("%s"), LABEL_GIRDER(rld.m_Girder));
   else
      str = _T("All Girders");

   m_LoadsListCtrl.SetItemText(irow, 4, str);

   std::_tstring stdstr;
   if (rld.m_Fractional)
   {
      stdstr = D2S(rld.m_Location*100.0) + std::_tstring(_T(" %"));
   }
   else
   {
      Float64 val = ::ConvertFromSysUnits(rld.m_Location, *m_pLengthUnit);
      stdstr = D2S(val) + std::_tstring(_T(" ")) +  m_pLengthUnit->UnitTag();
   }

   m_LoadsListCtrl.SetItemText(irow, 5, stdstr.c_str());

   Float64 val = ::ConvertFromSysUnits(rld.m_Magnitude, *m_pMomentUnit);
   stdstr = D2S(val) + std::_tstring(_T(" ")) + m_pMomentUnit->UnitTag();

   m_LoadsListCtrl.SetItemText(irow, 6, stdstr.c_str());
   m_LoadsListCtrl.SetItemText(irow, 7, rld.m_Description.c_str());
}

void CEditLoadsView::UpdateDistributedLoadItem(int irow, const CDistributedLoadData& rld)
{
   if (rld.m_Type==UserLoads::Uniform)
   {
      m_LoadsListCtrl.SetItemText(irow, 0, _T("Uniform"));
   }
   else
   {
      m_LoadsListCtrl.SetItemText(irow, 0, _T("Trapezoidal"));
   }

   m_LoadsListCtrl.SetItemText(irow, 1, UserLoads::GetStageName(rld.m_Stage).c_str());
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(rld.m_LoadCase).c_str());

   CString str;
   if (rld.m_Span != ALL_SPANS)
      str.Format(_T("%d"), LABEL_SPAN(rld.m_Span));
   else
      str = _T("All Spans");

   m_LoadsListCtrl.SetItemText(irow, 3, str);

   if (rld.m_Girder != ALL_GIRDERS)
      str.Format(_T("%s"), LABEL_GIRDER(rld.m_Girder));
   else
      str = _T("All Girders");

   m_LoadsListCtrl.SetItemText(irow, 4, str);

   std::_tstring stdstr;

   if (rld.m_Type==UserLoads::Uniform)
   {
      stdstr = _T("Entire Span");
   }
   else
   {
      if (rld.m_Fractional)
      {
         stdstr = D2S(rld.m_StartLocation*100.0) + std::_tstring(_T(" - ")) + D2S(rld.m_EndLocation*100.0) + std::_tstring(_T(" %"));;
      }
      else
      {
         Float64 startval = ::ConvertFromSysUnits(rld.m_StartLocation, *m_pLengthUnit);
         Float64 endval = ::ConvertFromSysUnits(rld.m_EndLocation, *m_pLengthUnit);
         stdstr = D2S(startval) + std::_tstring(_T(" - ")) + D2S(endval) + std::_tstring(_T(" ")) +  m_pLengthUnit->UnitTag();
      }
   }

   m_LoadsListCtrl.SetItemText(irow, 5, stdstr.c_str());

   if (rld.m_Type==UserLoads::Uniform)
   {
      Float64 val = ::ConvertFromSysUnits(rld.m_WStart, *m_pForcePerLengthUnit);
      stdstr = D2S(val) + std::_tstring(_T(" ")) + m_pForcePerLengthUnit->UnitTag();
   }
   else
   {
      Float64 startval = ::ConvertFromSysUnits(rld.m_WStart, *m_pForcePerLengthUnit);
      Float64 endval = ::ConvertFromSysUnits(rld.m_WEnd, *m_pForcePerLengthUnit);
      stdstr = D2S(startval) + std::_tstring(_T(" - ")) + D2S(endval) + std::_tstring(_T(" ")) + m_pForcePerLengthUnit->UnitTag();
   }

   m_LoadsListCtrl.SetItemText(irow, 6, stdstr.c_str());
   m_LoadsListCtrl.SetItemText(irow, 7, rld.m_Description.c_str());
}

void CEditLoadsView::EditLoad(POSITION pos)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int nItem = m_LoadsListCtrl.GetNextSelectedItem(pos);

   DWORD_PTR data = m_LoadsListCtrl.GetItemData(nItem);
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

void CEditLoadsView::OnContextMenu(CWnd* pWnd,CPoint point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( pWnd->GetSafeHwnd() != this->m_LoadsListCtrl.GetSafeHwnd() )
      return;

   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition();
   if ( pos == NULL )
   {
      // Nothing selected
      CMenu menu;
      VERIFY( menu.LoadMenu(IDR_NEWLOADS) );
      menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x,point.y, this);
   }
   else
   {
      CMenu menu;
      VERIFY( menu.LoadMenu(IDR_LOADS_CTX) );
      menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x,point.y, this);
   }
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

std::_tstring CEditLoadsView::D2S(Float64 val)
{
   // format number and strip off white space
   std::_tstring str = m_FormatTool.AsString(val);

   std::_tstring::size_type  notwhite = str.find_first_not_of(_T(" \t\n"));
   str.erase(0,notwhite);

   return str;
}

BOOL CEditLoadsView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext) )
      return FALSE;

   CEAFApp* pApp = EAFGetApp();
   m_SortColIdx = pApp->GetProfileInt(_T("Settings"),_T("LoadViewSortColumn"),-1);
   m_bSortAscending = pApp->GetProfileInt(_T("Settings"),_T("LoadViewSortAscending"),1) == 1 ? true : false;

   return TRUE;
}

void CEditLoadsView::OnDestroy()
{
   CFormView::OnDestroy();

   CEAFApp* pApp = EAFGetApp();
   pApp->WriteProfileInt(_T("Settings"),_T("LoadViewSortColumn"),m_SortColIdx);
   pApp->WriteProfileInt(_T("Settings"),_T("LoadViewSortAscending"),(int)m_bSortAscending);
}

class SortObject
{
public:
   static CComPtr<IUserDefinedLoadData> m_pUdl;
   static bool m_bSortAscending;
   static int CALLBACK SortFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
   static std::_tstring GetStage(LPARAM lParam);
   static UserLoads::LoadCase GetLoadCase(LPARAM lParam);
   static SpanIndexType GetSpan(LPARAM lParam);
   static GirderIndexType GetGirder(LPARAM lParam);
   static Float64 GetLocation(LPARAM lParam);
   static Float64 GetMagnitude(LPARAM lParam);
   static CString GetDescription(LPARAM lParam);
};

CComPtr<IUserDefinedLoadData> SortObject::m_pUdl;
bool SortObject::m_bSortAscending = true;

std::_tstring SortObject::GetStage(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return UserLoads::GetStageName(loadData.m_Stage);
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      return UserLoads::GetStageName(loadData.m_Stage);
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return UserLoads::GetStageName(loadData.m_Stage);
   }
}

UserLoads::LoadCase SortObject::GetLoadCase(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return loadData.m_LoadCase;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      return loadData.m_LoadCase;
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return loadData.m_LoadCase;
   }
}

SpanIndexType SortObject::GetSpan(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return loadData.m_Span;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      return loadData.m_Span;
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return loadData.m_Span;
   }
}

GirderIndexType SortObject::GetGirder(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return loadData.m_Girder;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      return loadData.m_Girder;
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return loadData.m_Girder;
   }
}

Float64 SortObject::GetLocation(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return (loadData.m_Fractional ? -1 : 1)*loadData.m_Location;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      if ( loadData.m_Type == UserLoads::Uniform )
         return 0;
      else
         return (loadData.m_Fractional ? -1 : 1)*loadData.m_StartLocation;
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return (loadData.m_Fractional ? -1 : 1)*loadData.m_Location;
   }
}

Float64 SortObject::GetMagnitude(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return loadData.m_Magnitude;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      return loadData.m_WStart;
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return loadData.m_Magnitude;
   }
}

CString SortObject::GetDescription(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   WORD load_idx  = HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData& loadData = m_pUdl->GetPointLoad(load_idx);
      return loadData.m_Description.c_str();
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData& loadData = m_pUdl->GetDistributedLoad(load_idx);
      return loadData.m_Description.c_str();
   }
   else
   {
      const CMomentLoadData& loadData = m_pUdl->GetMomentLoad(load_idx);
      return loadData.m_Description.c_str();
   }
}

int SortObject::SortFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
   WORD load_type_1 = LOWORD(lParam1);
   WORD load_idx_1  = HIWORD(lParam1);

   WORD load_type_2 = LOWORD(lParam2);
   WORD load_idx_2  = HIWORD(lParam2);

   int result = 0;
   switch( lParamSort )
   {
   case 0: // Load Type
      result = load_type_1 < load_type_2;
      break;

   case 1: // Stage
      result = SortObject::GetStage(lParam1) < SortObject::GetStage(lParam2);
      break;

   case 2: // Load Case
      result = SortObject::GetLoadCase(lParam1) < SortObject::GetLoadCase(lParam2);
      break;

   case 3: // Span
      result = SortObject::GetSpan(lParam1) < SortObject::GetSpan(lParam2);
      break;

   case 4: // Girder
      result = SortObject::GetGirder(lParam1) < SortObject::GetGirder(lParam2);
      break;

   case 5: // Location
      result = SortObject::GetLocation(lParam1) < SortObject::GetLocation(lParam2);
      break;

   case 6: // Magnitude
      result = SortObject::GetMagnitude(lParam1) < SortObject::GetMagnitude(lParam2);
      break;

   case 7: // Description
      result = SortObject::GetDescription(lParam1).CompareNoCase(SortObject::GetDescription(lParam2)) < 0;
      break;
}

   if ( !SortObject::m_bSortAscending )
      result = !result;

   return result;
}

void CEditLoadsView::OnHeaderClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
   NMLISTVIEW* pLV = (NMLISTVIEW*)pNMHDR;

   Sort(pLV->iItem);

   *pResult = 0;
}

void CEditLoadsView::Sort(int columnIdx,bool bReverse)
{
   if ( columnIdx < 0 )
      return; 

   if ( m_LoadsListCtrl.GetItemCount() <= 0 )
      return; // nothing to sort if there aren't any items... leave now and don't mess with icons in the header

   if ( bReverse )
      SortObject::m_bSortAscending = !m_bSortAscending;
   else
      SortObject::m_bSortAscending = m_bSortAscending;

   GET_IFACE(IUserDefinedLoadData, pUdl);
   SortObject::m_pUdl = pUdl;
 
   m_LoadsListCtrl.SortItems(SortObject::SortFunc,columnIdx);

   // remove old header image
   HDITEM old_item;
   old_item.mask  = HDI_FORMAT;
   m_LoadsListCtrl.GetHeaderCtrl()->GetItem(m_SortColIdx,&old_item);
   old_item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
   m_LoadsListCtrl.GetHeaderCtrl()->SetItem(m_SortColIdx,&old_item);

   // add header image
   HDITEM new_item;
   new_item.mask  = HDI_FORMAT;
   m_LoadsListCtrl.GetHeaderCtrl()->GetItem(columnIdx,&new_item);
   new_item.fmt  |= (SortObject::m_bSortAscending ? HDF_SORTUP : HDF_SORTDOWN); 
   m_LoadsListCtrl.GetHeaderCtrl()->SetItem(columnIdx,&new_item);


   SortObject::m_pUdl.Release();

   m_bSortAscending = SortObject::m_bSortAscending;

   m_SortColIdx = columnIdx;
}
