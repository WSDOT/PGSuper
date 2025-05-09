///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "PGSuperApp.h"
#include "EditLoadsView.h"

#include "resource.h"

#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\InsertDeleteLoad.h>
#include <PgsExt\MacroTxn.h>

#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>

#include <PgsExt\BridgeDescription2.h>

#include "PGSpliceDoc.h"

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
   m_FirstSizeEvent(true)
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

BOOL CEditLoadsView::PreTranslateMessage(MSG* pMsg)
{
   if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
   {
      if (pMsg->wParam == VK_RETURN && pMsg->hwnd == m_LoadsListCtrl.GetSafeHwnd())
      {
         return CWnd::PreTranslateMessage(pMsg);
      }
      else if (pMsg->wParam == VK_DELETE)
      {
         OnDeleteLoad();
      }
   }
   return __super::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CEditLoadsView, CFormView)
	//{{AFX_MSG_MAP(CEditLoadsView)
	ON_BN_CLICKED(IDC_ADD_POINTLOAD, OnAddPointload)
	ON_BN_CLICKED(IDC_ADD_MOMENTLOAD, OnAddMomentload)
	ON_BN_CLICKED(IDC_DELETE_LOAD, OnDeleteLoad)
	ON_BN_CLICKED(IDC_EDIT_LOAD, OnEditLoad)
	ON_NOTIFY(NM_DBLCLK, IDC_LOADS_LIST, OnDblclkLoadsList)
   ON_NOTIFY(NM_RETURN, IDC_LOADS_LIST, OnDblclkLoadsList)
	ON_BN_CLICKED(IDC_ADD_NEW_DISTRIBUTED, OnAddDistributedLoad)
	ON_NOTIFY(NM_CLICK, IDC_LOADS_LIST, OnClickLoadsList)
	ON_NOTIFY(NM_RCLICK, IDC_LOADS_LIST, OnClickLoadsList)
	ON_WM_SIZE()
   ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_NOTIFY(HDN_ITEMCLICK,0,OnHeaderClicked)
	ON_COMMAND(ID_EDIT_LOAD, OnEditLoad)
	ON_COMMAND(ID_DELETE_LOAD, OnDeleteLoad)
	ON_COMMAND(ID_ADD_POINT_LOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddDistributedLoad)
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
      if ( lHint == HINT_GIRDERCHANGED )
      {
         CGirderHint* pGirderHint = (CGirderHint*)pHint;
         if ( pGirderHint->lHint == GCH_LOADING_ADDED || pGirderHint->lHint == GCH_LOADING_REMOVED || pGirderHint->lHint == GCH_LOADING_CHANGED )
         {
            InsertData();
            Sort(m_SortColIdx,false);
         }
      }
      else
      {
         InsertData();
      }

   }
}

void CEditLoadsView::OnInitialUpdate() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   EAFGetBroker(&m_pBroker);

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
   st = m_LoadsListCtrl.InsertColumn(1,_T("Event"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(2,_T("Load Case"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(3,_T("Location"),LVCFMT_LEFT,lft_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(4,_T("Magnitude"),LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);
   st = m_LoadsListCtrl.InsertColumn(5,_T("Description"),LVCFMT_LEFT,rgt_wid);
   ATLASSERT(st!=-1);

   InsertData();
   Sort(m_SortColIdx,false);

   // Moment loads not used in PGSplice... Hide the button
   if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      GetDlgItem(IDC_ADD_MOMENTLOAD)->ShowWindow(SW_HIDE);
   }
}

void CEditLoadsView::SortBy(CEditLoadsView::Field field, CEditLoadsView::Direction direction)
{
   int columnIdx = GetColumnIndex(field);
   bool bReverse = ((m_bSortAscending && direction == Descending) || (!m_bSortAscending && direction == Ascending)) ? true : false;
   Sort(columnIdx, bReverse);
}

IndexType CEditLoadsView::GetLoadCount() const
{
   return m_LoadsListCtrl.GetItemCount();
}

std::_tstring CEditLoadsView::GetFieldValue(IndexType idx, CEditLoadsView::Field field) const
{
   CString strFieldValue = m_LoadsListCtrl.GetItemText((int)idx, GetColumnIndex(field));
   return std::_tstring(strFieldValue);
}

void CEditLoadsView::DeleteLoad(IndexType idx)
{
   m_LoadsListCtrl.SetItemState(-1, ~LVIS_SELECTED, LVIS_SELECTED);
   m_LoadsListCtrl.SetItemState((int)idx, LVIS_SELECTED, LVIS_SELECTED);
   OnDeleteLoad();
}

void CEditLoadsView::OnAddPointload() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPointLoadData load;
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->AddPointLoad(load);
}

void CEditLoadsView::OnAddDistributedLoad()
{
   CDistributedLoadData load;
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->AddDistributedLoad(load);
}

void CEditLoadsView::OnAddMomentload() 
{
   CMomentLoadData load;
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->AddMomentLoad(load);
}

void CEditLoadsView::OnDeleteLoad() 
{
   std::unique_ptr<pgsMacroTxn> macro(std::make_unique<pgsMacroTxn>());
   macro->Name(_T("Delete loads"));

   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );
   while (pos != nullptr)
   {
      int nItem = m_LoadsListCtrl.GetNextSelectedItem(pos);

      DWORD_PTR data = m_LoadsListCtrl.GetItemData(nItem);
      WORD load_type = LOWORD(data);
      LoadIDType load_id = (LoadIDType)HIWORD(data);

      if (load_type == W_POINT_LOAD)
      {
         std::unique_ptr<txnDeletePointLoad> pTxn(std::make_unique<txnDeletePointLoad>(load_id));
         macro->AddTransaction(std::move(pTxn));
      }
      else if (load_type == W_DISTRIBUTED_LOAD)
      {
         std::unique_ptr<txnDeleteDistributedLoad> pTxn(std::make_unique<txnDeleteDistributedLoad>(load_id));
         macro->AddTransaction(std::move(pTxn));
      }
      else if (load_type == W_MOMENT_LOAD)
      {
         std::unique_ptr<txnDeleteMomentLoad> pTxn(std::make_unique<txnDeleteMomentLoad>(load_id));
         macro->AddTransaction(std::move(pTxn));
      }
      else
      {
         ATLASSERT(false);
      }
   }

   CEAFTxnManager::GetInstance().Execute(std::move(macro));
}

void CEditLoadsView::OnEditLoad() 
{
   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );

   if (pos != nullptr)
   {
      EditLoad(pos);
   }
}

void CEditLoadsView::OnDblclkLoadsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition( );

   if (pos != nullptr)
   {
      EditLoad(pos);
   }

	*pResult = 0;
}

void CEditLoadsView::InsertData()
{
   // for now just delete all loads and rebuild list
   m_LoadsListCtrl.DeleteAllItems();

   int irow=0;

   // Add Point Loads
   GET_IFACE(IUserDefinedLoadData, pUdl);
   IndexType pl_cnt = pUdl->GetPointLoadCount();
   IndexType ipl;
   for (ipl = 0; ipl < pl_cnt; ipl++)
   {
      m_LoadsListCtrl.InsertItem(irow, _T("Point"));
      const CPointLoadData* pLoad = pUdl->GetPointLoad(ipl);

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_POINT_LOAD, pLoad->m_ID) );


      UpdatePointLoadItem(irow, *pLoad);

      irow++;
   }

   // Add Moment Loads
   pl_cnt = pUdl->GetMomentLoadCount();
   for (ipl = 0; ipl < pl_cnt; ipl++)
   {
      m_LoadsListCtrl.InsertItem(irow, _T("Moment"));

      const CMomentLoadData* pLoad = pUdl->GetMomentLoad(ipl);

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_MOMENT_LOAD, pLoad->m_ID) );

      UpdateMomentLoadItem(irow, *pLoad);

      irow++;
   }

   // Add Distributed Loads
   IndexType dl_cnt = pUdl->GetDistributedLoadCount();
   for (IndexType idl = 0; idl < dl_cnt; idl++)
   {
      m_LoadsListCtrl.InsertItem(irow, _T("Distributed"));

      const CDistributedLoadData* pLoad = pUdl->GetDistributedLoad(idl);

      // use item data to save/retreive type information and location in vectors
      m_LoadsListCtrl.SetItemData(irow, MAKELONG(W_DISTRIBUTED_LOAD, pLoad->m_ID) );

      UpdateDistributedLoadItem(irow, *pLoad);

      irow++;
   }

   // nothing is selected by default, so we can't edit or delete
   m_EditCtrl.EnableWindow(FALSE);
   m_DeleteCtrl.EnableWindow(FALSE);
}

CString CEditLoadsView::GetEventName(EventIDType loadID)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   EventIndexType eventIdx = pTimelineMgr->FindUserLoadEventIndex(loadID);
   CString str;
   str.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pIBridgeDesc->GetEventByIndex(eventIdx)->GetDescription());
   return str;
}

void CEditLoadsView::UpdatePointLoadItem(int irow, const CPointLoadData& ptLoad)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   m_LoadsListCtrl.SetItemText(irow, 1, GetEventName(ptLoad.m_ID));
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(ptLoad.m_LoadCase).c_str());

   CString strSpan;
   if ( ptLoad.m_SpanKey.spanIndex == ALL_SPANS )
   {
      strSpan.Format(_T("%s"),_T("All Spans"));
   }
   else
   {
      if ( ptLoad.m_bLoadOnCantilever[pgsTypes::metStart] )
      {
         strSpan.Format(_T("Span %s Start Cantilever"),LABEL_SPAN(ptLoad.m_SpanKey.spanIndex));
      }
      else if ( ptLoad.m_bLoadOnCantilever[pgsTypes::metEnd] )
      {
         strSpan.Format(_T("Span %s End Cantilever"),LABEL_SPAN(ptLoad.m_SpanKey.spanIndex));
      }
      else
      {
         strSpan.Format(_T("Span %s"),LABEL_SPAN(ptLoad.m_SpanKey.spanIndex));
      }
   }

   CString strGirder;
   if ( ptLoad.m_SpanKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(ptLoad.m_SpanKey.girderIndex));
   }

   CString strLocation;
   if (ptLoad.m_Fractional)
   {
      strLocation.Format(_T("%s"),FormatPercentage(ptLoad.m_Location));
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(ptLoad.m_Location,pDisplayUnits->GetSpanLengthUnit()));
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_LoadsListCtrl.SetItemText(irow,3,strLabel);

   CString strMagnitude;
   strMagnitude.Format(_T("%s"),FormatDimension(ptLoad.m_Magnitude,pDisplayUnits->GetGeneralForceUnit()));

   m_LoadsListCtrl.SetItemText(irow, 4, strMagnitude);
   m_LoadsListCtrl.SetItemText(irow, 5, ptLoad.m_Description.c_str());
}

void CEditLoadsView::UpdateDistributedLoadItem(int irow, const CDistributedLoadData& load)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   if (load.m_Type == UserLoads::Uniform)
   {
      m_LoadsListCtrl.SetItemText(irow, 0, _T("Uniform"));
   }
   else
   {
      m_LoadsListCtrl.SetItemText(irow, 0, _T("Trapezoidal"));
   }

   m_LoadsListCtrl.SetItemText(irow, 1, GetEventName(load.m_ID));
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(load.m_LoadCase).c_str());

   CString strSpan;
   if ( load.m_SpanKey.spanIndex == ALL_SPANS )
   {
      strSpan.Format(_T("%s"),_T("All Spans"));
   }
   else
   {
      strSpan.Format(_T("Span %s"),LABEL_SPAN(load.m_SpanKey.spanIndex));
   }

   CString strGirder;
   if ( load.m_SpanKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(load.m_SpanKey.girderIndex));
   }

   CString strLocation;
   if ( load.m_Type == UserLoads::Uniform )
   {
      strLocation.Format(_T("%s"),_T("Entire Span"));
   }
   else
   {
      if (load.m_Fractional)
      {
         strLocation.Format(_T("%s - %s"),FormatPercentage(load.m_StartLocation,false),FormatPercentage(load.m_EndLocation));
      }
      else
      {
         strLocation.Format(_T("%s - %s"),FormatDimension(load.m_StartLocation,pDisplayUnits->GetSpanLengthUnit(),false),FormatDimension(load.m_EndLocation,pDisplayUnits->GetSpanLengthUnit()));
      }
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_LoadsListCtrl.SetItemText(irow,3,strLabel);

   CString strMagnitude;
   if (load.m_Type == UserLoads::Uniform)
   {
      strMagnitude.Format(_T("%s"),FormatDimension(load.m_WStart,pDisplayUnits->GetForcePerLengthUnit()));
   }
   else
   {
      strMagnitude.Format(_T("%s - %s"),FormatDimension(load.m_WStart,pDisplayUnits->GetForcePerLengthUnit(),false),FormatDimension(load.m_WEnd,pDisplayUnits->GetForcePerLengthUnit()));
   }

   m_LoadsListCtrl.SetItemText(irow, 4, strMagnitude);
   m_LoadsListCtrl.SetItemText(irow, 5, load.m_Description.c_str());
}

void CEditLoadsView::UpdateMomentLoadItem(int irow, const CMomentLoadData& load)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   m_LoadsListCtrl.SetItemText(irow, 1, GetEventName(load.m_ID));
   m_LoadsListCtrl.SetItemText(irow, 2, UserLoads::GetLoadCaseName(load.m_LoadCase).c_str());

   CString strSpan;
   if ( load.m_SpanKey.spanIndex == ALL_SPANS )
   {
      strSpan.Format(_T("%s"),_T("All Spans"));
   }
   else
   {
      strSpan.Format(_T("Span %s"),LABEL_SPAN(load.m_SpanKey.spanIndex));
   }

   CString strGirder;
   if ( load.m_SpanKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(load.m_SpanKey.girderIndex));
   }

   CString strLocation;
   if (load.m_Fractional)
   {
      strLocation.Format(_T("%s"),FormatPercentage(load.m_Location));
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(load.m_Location,pDisplayUnits->GetSpanLengthUnit()));
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_LoadsListCtrl.SetItemText(irow,3,strLabel);

   CString strMagnitude;
   strMagnitude.Format(_T("%s"),FormatDimension(load.m_Magnitude,pDisplayUnits->GetMomentUnit()));

   m_LoadsListCtrl.SetItemText(irow, 4, strMagnitude);
   m_LoadsListCtrl.SetItemText(irow, 5, load.m_Description.c_str());
}

void CEditLoadsView::EditLoad(POSITION pos)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int nItem = m_LoadsListCtrl.GetNextSelectedItem(pos);

   DWORD_PTR data = m_LoadsListCtrl.GetItemData(nItem);
   WORD load_type = LOWORD(data);
   LoadIDType load_id = (LoadIDType)HIWORD(data);

   GET_IFACE(IEditByUI,pEditByUI);
   if (load_type == W_POINT_LOAD)
   {
      pEditByUI->EditPointLoadByID(load_id);
   }
   else if (load_type == W_MOMENT_LOAD)
   {
      pEditByUI->EditMomentLoadByID(load_id);
   }
   else if (load_type == W_DISTRIBUTED_LOAD)
   {
      pEditByUI->EditDistributedLoadByID(load_id);
   }
   else
   {
      ATLASSERT(false);
   }

}

void CEditLoadsView::OnClickLoadsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
   UINT nSelected = m_LoadsListCtrl.GetSelectedCount();
   m_EditCtrl.EnableWindow(nSelected == 1 ? TRUE : FALSE);
   m_DeleteCtrl.EnableWindow(0 < nSelected ? TRUE : FALSE);
	
	*pResult = 0;
}

void CEditLoadsView::OnContextMenu(CWnd* pWnd,CPoint point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( pWnd->GetSafeHwnd() != this->m_LoadsListCtrl.GetSafeHwnd() )
   {
      return;
   }

   POSITION pos = m_LoadsListCtrl.GetFirstSelectedItemPosition();
   if ( pos == nullptr )
   {
      // Nothing selected
      CMenu menu;
      VERIFY( menu.LoadMenu(IDR_NEWLOADS) );

      // Moment Loads not used in PGSplice
      if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         menu.RemoveMenu(ID_ADD_MOMENT_LOAD,MF_BYCOMMAND);
      }

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
   if ( pLoads == nullptr)
   {
      return;
   }

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

   int width  = Max((int)m_MinSize.cx, rc.Width());
   int height = Max((int)m_MinSize.cy, rc.Height());

   // static
   int w = width - 2*BORDER;
   int h = height - 3*BORDER - m_But1Size.cy;
   pStatic->SetWindowPos(nullptr,0,0,w,h,SWP_NOMOVE | SWP_NOZORDER);

   // list
   w = w - 2*BORDER;
   h = h - m_But1Size.cy - BORDER;
   pLoads->SetWindowPos(nullptr,0,0,w,h,SWP_NOMOVE | SWP_NOZORDER);

   // buttons
   int x = 2*BORDER;
   int y = height - 3*BORDER;
   pEdit->SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But1Size.cx + BORDER;
   pDelete->SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But1Size.cx + BORDER;
   pAddP->SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But2Size.cx + BORDER;
   pAddD->SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
   x = x + m_But2Size.cx + BORDER;
   pAddM->SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);

   x = width - m_HelpButWidth - BORDER;
   pHelp->SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
}


void CEditLoadsView::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_LOADS );
}

BOOL CEditLoadsView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext) )
   {
      return FALSE;
   }

   CWinApp* pApp = AfxGetApp();
   m_SortColIdx = pApp->GetProfileInt(_T("Settings"),_T("LoadViewSortColumn"),-1);
   m_bSortAscending = pApp->GetProfileInt(_T("Settings"),_T("LoadViewSortAscending"),1) == 1 ? true : false;

   return TRUE;
}

void CEditLoadsView::OnDestroy()
{
   CFormView::OnDestroy();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   pApp->WriteProfileInt(_T("Settings"),_T("LoadViewSortColumn"),m_SortColIdx);
   pApp->WriteProfileInt(_T("Settings"),_T("LoadViewSortAscending"),(int)m_bSortAscending);
}

class SortObject
{
public:
   static CComPtr<IUserDefinedLoadData> m_pUdl;
   static bool m_bSortAscending;
   static const CTimelineManager* m_pTimelineMgr;
   static int CALLBACK SortFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
   static EventIndexType GetEvent(LPARAM lParam);
   static UserLoads::LoadCase GetLoadCase(LPARAM lParam);
   static SpanIndexType GetSpan(LPARAM lParam);
   static GirderIndexType GetGirder(LPARAM lParam);
   static Float64 GetLocation(LPARAM lParam);
   static Float64 GetMagnitude(LPARAM lParam);
   static CString GetDescription(LPARAM lParam);
};

CComPtr<IUserDefinedLoadData> SortObject::m_pUdl;
const CTimelineManager* SortObject::m_pTimelineMgr = nullptr;
bool SortObject::m_bSortAscending = true;

EventIndexType SortObject::GetEvent(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   ATLASSERT(load_id != INVALID_ID);
   EventIndexType eventIdx = m_pTimelineMgr->FindUserLoadEventIndex(load_id);
   return eventIdx;
}

UserLoads::LoadCase SortObject::GetLoadCase(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData* pLoadData = m_pUdl->FindPointLoad(load_id);
      return pLoadData->m_LoadCase;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData* pLoadData = m_pUdl->FindDistributedLoad(load_id);
      return pLoadData->m_LoadCase;
   }
   else
   {
      const CMomentLoadData* pLoadData = m_pUdl->FindMomentLoad(load_id);
      return pLoadData->m_LoadCase;
   }
}

SpanIndexType SortObject::GetSpan(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData* pLoadData = m_pUdl->FindPointLoad(load_id);
      return pLoadData->m_SpanKey.spanIndex;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData* pLoadData = m_pUdl->FindDistributedLoad(load_id);
      return pLoadData->m_SpanKey.spanIndex;
   }
   else
   {
      const CMomentLoadData* pLoadData = m_pUdl->FindMomentLoad(load_id);
      return pLoadData->m_SpanKey.spanIndex;
   }
}

GirderIndexType SortObject::GetGirder(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData* pLoadData = m_pUdl->FindPointLoad(load_id);
      return pLoadData->m_SpanKey.girderIndex;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData* pLoadData = m_pUdl->FindDistributedLoad(load_id);
      return pLoadData->m_SpanKey.girderIndex;
   }
   else
   {
      const CMomentLoadData* pLoadData = m_pUdl->FindMomentLoad(load_id);
      return pLoadData->m_SpanKey.girderIndex;
   }
}

Float64 SortObject::GetLocation(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData* pLoadData = m_pUdl->FindPointLoad(load_id);
      return (pLoadData->m_Fractional ? -1 : 1)*pLoadData->m_Location;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData* pLoadData = m_pUdl->FindDistributedLoad(load_id);
      if ( pLoadData->m_Type == UserLoads::Uniform )
      {
         return 0;
      }
      else
      {
         return (pLoadData->m_Fractional ? -1 : 1)*pLoadData->m_StartLocation;
      }
   }
   else
   {
      const CMomentLoadData* pLoadData = m_pUdl->FindMomentLoad(load_id);
      return (pLoadData->m_Fractional ? -1 : 1)*pLoadData->m_Location;
   }
}

Float64 SortObject::GetMagnitude(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData* pLoadData = m_pUdl->FindPointLoad(load_id);
      return pLoadData->m_Magnitude;
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData* pLoadData = m_pUdl->FindDistributedLoad(load_id);
      return pLoadData->m_WStart;
   }
   else
   {
      const CMomentLoadData* pLoadData = m_pUdl->FindMomentLoad(load_id);
      return pLoadData->m_Magnitude;
   }
}

CString SortObject::GetDescription(LPARAM lParam)
{
   WORD load_type = LOWORD(lParam);
   LoadIDType load_id = (LoadIDType)HIWORD(lParam);

   if ( load_type == W_POINT_LOAD )
   {
      const CPointLoadData* pLoadData = m_pUdl->FindPointLoad(load_id);
      return pLoadData->m_Description.c_str();
   }
   else if ( load_type == W_DISTRIBUTED_LOAD )
   {
      const CDistributedLoadData* pLoadData = m_pUdl->FindDistributedLoad(load_id);
      return pLoadData->m_Description.c_str();
   }
   else
   {
      const CMomentLoadData* pLoadData = m_pUdl->FindMomentLoad(load_id);
      return pLoadData->m_Description.c_str();
   }
}

int SortObject::SortFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
   WORD load_type_1 = LOWORD(lParam1);
   WORD load_type_2 = LOWORD(lParam2);
   
   int result = 0;
   switch( lParamSort )
   {
   case 0: // Load Type
      result = load_type_1 < load_type_2;
      break;

   case 1: // Event
      result = SortObject::GetEvent(lParam1) < SortObject::GetEvent(lParam2);
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
   {
      result = !result;
   }

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
   {
      return; 
   }

   if ( m_LoadsListCtrl.GetItemCount() <= 0 )
   {
      return; // nothing to sort if there aren't any items... leave now and don't mess with icons in the header
   }

   if ( bReverse )
   {
      SortObject::m_bSortAscending = !m_bSortAscending;
   }
   else
   {
      SortObject::m_bSortAscending = m_bSortAscending;
   }

   GET_IFACE_NOCHECK(IUserDefinedLoadData, pUdl);
   SortObject::m_pUdl = pUdl;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   SortObject::m_pTimelineMgr = pIBridgeDesc->GetTimelineManager();

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


int CEditLoadsView::GetColumnIndex(CEditLoadsView::Field field) const
{
   int idx;
   switch (field)
   {
   case Type:  idx = 0;  break;
   case Event: idx = 1; break;
   case LoadCase: idx = 2; break;
   case Location: idx = 5; break;
   case Magnitude: idx = 6; break;
   case Description: idx = 7; break;
   default: ATLASSERT(false); idx = 0; break;
   }
   return idx;
}
