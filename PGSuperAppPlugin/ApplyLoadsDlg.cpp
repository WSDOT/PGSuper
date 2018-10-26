// ApplyLoadsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ApplyLoadsDlg.h"

#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\DeckDescription2.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>

// CApplyLoadsDlg dialog

IMPLEMENT_DYNAMIC(CApplyLoadsDlg, CDialog)

CApplyLoadsDlg::CApplyLoadsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CApplyLoadsDlg::IDD, pParent)
{

}

CApplyLoadsDlg::~CApplyLoadsDlg()
{
}

void CApplyLoadsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_USER_LOADS, m_ctrlUserLoads);

   if ( pDX->m_bSaveAndValidate )
   {
      m_ApplyLoads.Enable(true);

      bool bValue;
      DDX_Check_Bool(pDX,IDC_RAILING_SYSTEM,bValue);
      m_ApplyLoads.ApplyRailingSystemLoad(bValue);

      DDX_Check_Bool(pDX,IDC_OVERLAY,bValue);
      m_ApplyLoads.ApplyOverlayLoad(bValue);

      DDX_Check_Bool(pDX,IDC_LIVELOAD,bValue);
      m_ApplyLoads.ApplyLiveLoad(bValue);
   }
   else
   {
      bool bValue = m_ApplyLoads.IsRailingSystemLoadApplied();
      DDX_Check_Bool(pDX,IDC_RAILING_SYSTEM,bValue);

      bValue = m_ApplyLoads.IsOverlayLoadApplied();
      DDX_Check_Bool(pDX,IDC_OVERLAY,bValue);

      bValue = m_ApplyLoads.IsLiveLoadApplied();
      DDX_Check_Bool(pDX,IDC_LIVELOAD,bValue);
   }
}


BEGIN_MESSAGE_MAP(CApplyLoadsDlg, CDialog)
   ON_BN_CLICKED(IDC_RAILING_SYSTEM, &CApplyLoadsDlg::OnRailingSystemClicked)
   ON_BN_CLICKED(IDC_OVERLAY, &CApplyLoadsDlg::OnOverlayClicked)
   ON_BN_CLICKED(IDC_LIVELOAD, &CApplyLoadsDlg::OnLiveloadClicked)
END_MESSAGE_MAP()


// CApplyLoadsDlg message handlers

BOOL CApplyLoadsDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   m_RailingSystemEventIdx = pTimelineMgr->GetRailingSystemLoadEventIndex();
   m_OverlayEventIdx       = pTimelineMgr->GetOverlayLoadEventIndex();
   m_LiveLoadEventIdx      = pTimelineMgr->GetLiveLoadEventIndex();

   CDialog::OnInitDialog();

   InitUserLoads();

   CString strNote;
   if ( m_ThisEventIdx == m_RailingSystemEventIdx )
   {
      // the railing system is installed in this event so disable
      // the check box so we don't end up with the railing system
      // not defined in any event
      GetDlgItem(IDC_RAILING_SYSTEM)->EnableWindow(FALSE);
      strNote = _T("");
   }
   else
   {
      strNote.Format(_T("(Installed during Event %d)"),LABEL_EVENT(m_RailingSystemEventIdx));
   }
   CWnd* pWnd = GetDlgItem(IDC_RAILING_SYSTEM_NOTE);
   pWnd->SetWindowText(strNote);


   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth )
   {
      GetDlgItem(IDC_OVERLAY)->EnableWindow(FALSE);
      GetDlgItem(IDC_OVERLAY_NOTE)->EnableWindow(FALSE);
      strNote = _T("(Bridge does not have an overlay)");
   }
   else if ( m_ThisEventIdx == m_OverlayEventIdx )
   {
      GetDlgItem(IDC_OVERLAY)->EnableWindow(FALSE);
      GetDlgItem(IDC_OVERLAY_NOTE)->EnableWindow(FALSE);
      strNote = _T("");
   }
   else
   {
      strNote.Format(_T("(Installed during Event %d)"),LABEL_EVENT(m_OverlayEventIdx));
   }
   pWnd = GetDlgItem(IDC_OVERLAY_NOTE);
   pWnd->SetWindowText(strNote);

   if ( m_ThisEventIdx == m_LiveLoadEventIdx )
   {
      GetDlgItem(IDC_LIVELOAD)->EnableWindow(FALSE);
      strNote = _T("");
   }
   else
   {
      strNote.Format(_T("(Opened to traffic during Event %d)"),LABEL_EVENT(m_LiveLoadEventIdx));
   }
   pWnd = GetDlgItem(IDC_LIVELOAD_NOTE);
   pWnd->SetWindowText(strNote);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CApplyLoadsDlg::OnRailingSystemClicked()
{
   CString strNote;

   if ( IsDlgButtonChecked(IDC_RAILING_SYSTEM) )
   {
      if ( m_ThisEventIdx == m_RailingSystemEventIdx )
      {
         strNote.Format(_T(""));
      }
      else
      {
         strNote.Format(_T("(Moving installation from Event %d to this event)"),LABEL_EVENT(m_RailingSystemEventIdx));
      }
   }
   else
   {
      if ( m_ThisEventIdx == m_RailingSystemEventIdx )
      {
         strNote.Format(_T(""));
      }
      else
      {
         strNote.Format(_T("(Installed during Event %d)"),LABEL_EVENT(m_RailingSystemEventIdx));
      }
   }

   CWnd* pWnd = GetDlgItem(IDC_RAILING_SYSTEM_NOTE);
   pWnd->SetWindowText(strNote);
}

void CApplyLoadsDlg::OnOverlayClicked()
{
   CString strNote;

   if ( IsDlgButtonChecked(IDC_OVERLAY) )
   {
      if ( m_ThisEventIdx == m_OverlayEventIdx )
      {
         strNote.Format(_T(""));
      }
      else
      {
         strNote.Format(_T("(Moving installation from Event %d to this event)"),LABEL_EVENT(m_OverlayEventIdx));
      }
   }
   else
   {
      if ( m_ThisEventIdx == m_OverlayEventIdx )
      {
         strNote.Format(_T(""));
      }
      else
      {
         strNote.Format(_T("(Installed during Event %d)"),LABEL_EVENT(m_OverlayEventIdx));
      }
   }

   CWnd* pWnd = GetDlgItem(IDC_OVERLAY_NOTE);
   pWnd->SetWindowText(strNote);
}

void CApplyLoadsDlg::OnLiveloadClicked()
{
   CString strNote;

   if ( IsDlgButtonChecked(IDC_LIVELOAD) )
   {
      if ( m_ThisEventIdx == m_LiveLoadEventIdx )
      {
         strNote.Format(_T(""));
      }
      else
      {
         strNote.Format(_T("(Moving opening to traffic from Event %d to this event)"),LABEL_EVENT(m_LiveLoadEventIdx));
      }
   }
   else
   {
      if ( m_ThisEventIdx == m_LiveLoadEventIdx )
      {
         strNote.Format(_T(""));
      }
      else
      {
         strNote.Format(_T("(Opened to traffic during Event %d)"),LABEL_EVENT(m_LiveLoadEventIdx));
      }
   }

   CWnd* pWnd = GetDlgItem(IDC_LIVELOAD_NOTE);
   pWnd->SetWindowText(strNote);
}

void CApplyLoadsDlg::InitUserLoads()
{
   m_ctrlUserLoads.SetExtendedStyle ( LVS_EX_FULLROWSELECT );

   int st;
   st = m_ctrlUserLoads.InsertColumn(0,_T("Type")/*,LVCFMT_LEFT,lft_wid*/);
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(1,_T("Load Case")/*,LVCFMT_LEFT,lft_wid*/);
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(2,_T("Location")/*,LVCFMT_LEFT,lft_wid*/);
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(3,_T("Magnitude")/*,LVCFMT_LEFT,rgt_wid*/);
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(4,_T("Description")/*,LVCFMT_LEFT,rgt_wid*/);
   ATLASSERT(st!=-1);

   int rowIdx = 0;
   IndexType nLoads = m_ApplyLoads.GetUserLoadCount();
   for ( IndexType idx = 0; idx < nLoads; idx++, rowIdx++ )
   {
      LoadIDType loadID = m_ApplyLoads.GetUserLoadID(idx);

      UserLoads::UserLoadType loadType = UserLoads::GetUserLoadTypeFromID(loadID);
      switch(loadType)
      {
      case UserLoads::Distributed:
         AddDistributedLoad(rowIdx,loadID);
         break;

      case UserLoads::Point:
         AddPointLoad(rowIdx,loadID);
         break;

      case UserLoads::Moment:
         AddMomentLoad(rowIdx,loadID);
         break;

      default:
         ATLASSERT(false); // should never get here
      }
   }

   int nItems = m_ctrlUserLoads.GetItemCount();
   int nCol = m_ctrlUserLoads.GetHeaderCtrl()->GetItemCount();
   if ( nItems == 0 )
   {
      for ( int i = 0; i < nCol; i++ )
         m_ctrlUserLoads.SetColumnWidth(i,LVSCW_AUTOSIZE_USEHEADER);
   }
   else
   {
      for ( int i = 0; i < nCol; i++ )
      {
         m_ctrlUserLoads.SetColumnWidth(i,LVSCW_AUTOSIZE_USEHEADER);
         int cx1 = m_ctrlUserLoads.GetColumnWidth(i);
         m_ctrlUserLoads.SetColumnWidth(i,LVSCW_AUTOSIZE);
         int cx2 = m_ctrlUserLoads.GetColumnWidth(i);
         m_ctrlUserLoads.SetColumnWidth(i,max(cx1,cx2));
      }
   }
}

void CApplyLoadsDlg::AddDistributedLoad(int rowIdx,LoadIDType loadID)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   const CDistributedLoadData& loadData = pUserDefinedLoads->FindDistributedLoad(loadID);

   m_ctrlUserLoads.InsertItem(rowIdx, _T("Distributed"));

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if (loadData.m_Type == UserLoads::Uniform)
   {
      m_ctrlUserLoads.SetItemText(rowIdx, 0, _T("Uniform"));
   }
   else
   {
      m_ctrlUserLoads.SetItemText(rowIdx, 0, _T("Trapezoidal"));
   }

   m_ctrlUserLoads.SetItemText(rowIdx, 1, UserLoads::GetLoadCaseName(loadData.m_LoadCase).c_str());

   CString strSpan;
   if ( loadData.m_SpanGirderKey.spanIndex == ALL_SPANS )
      strSpan.Format(_T("%s"),_T("All Spans"));
   else
      strSpan.Format(_T("Span %d"),LABEL_SPAN(loadData.m_SpanGirderKey.spanIndex));

   CString strGirder;
   if ( loadData.m_SpanGirderKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(loadData.m_SpanGirderKey.girderIndex));
   }

   CString strLocation;
   if ( loadData.m_Type == UserLoads::Uniform )
   {
      strLocation.Format(_T("%s"),_T("Entire Span"));
   }
   else
   {
      if (loadData.m_Fractional)
      {
         strLocation.Format(_T("%s - %s"),FormatPercentage(loadData.m_StartLocation,false),FormatPercentage(loadData.m_EndLocation));
      }
      else
      {
         strLocation.Format(_T("%s - %s"),FormatDimension(loadData.m_StartLocation,pDisplayUnits->GetSpanLengthUnit(),false),FormatDimension(loadData.m_EndLocation,pDisplayUnits->GetSpanLengthUnit()));
      }
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_ctrlUserLoads.SetItemText(rowIdx,2,strLabel);

   CString strMagnitude;
   if (loadData.m_Type == UserLoads::Uniform)
   {
      strMagnitude.Format(_T("%s"),FormatDimension(loadData.m_WStart,pDisplayUnits->GetForcePerLengthUnit()));
   }
   else
   {
      strMagnitude.Format(_T("%s - %s"),FormatDimension(loadData.m_WStart,pDisplayUnits->GetForcePerLengthUnit(),false),FormatDimension(loadData.m_WEnd,pDisplayUnits->GetForcePerLengthUnit()));
   }

   m_ctrlUserLoads.SetItemText(rowIdx, 3, strMagnitude);
   m_ctrlUserLoads.SetItemText(rowIdx, 4, loadData.m_Description.c_str());
}

void CApplyLoadsDlg::AddPointLoad(int rowIdx,LoadIDType loadID)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   const CPointLoadData& loadData = pUserDefinedLoads->FindPointLoad(loadID);

   m_ctrlUserLoads.InsertItem(rowIdx, _T("Point"));

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   m_ctrlUserLoads.SetItemText(rowIdx, 1, UserLoads::GetLoadCaseName(loadData.m_LoadCase).c_str());

   CString strSpan;
   if ( loadData.m_SpanGirderKey.spanIndex == ALL_SPANS )
      strSpan.Format(_T("%s"),_T("All Spans"));
   else
      strSpan.Format(_T("Span %d"),LABEL_SPAN(loadData.m_SpanGirderKey.spanIndex));

   CString strGirder;
   if ( loadData.m_SpanGirderKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(loadData.m_SpanGirderKey.girderIndex));
   }

   CString strLocation;
   if (loadData.m_Fractional)
   {
      strLocation.Format(_T("%s"),FormatPercentage(loadData.m_Location));
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(loadData.m_Location,pDisplayUnits->GetSpanLengthUnit()));
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_ctrlUserLoads.SetItemText(rowIdx,2,strLabel);

   CString strMagnitude;
   strMagnitude.Format(_T("%s"),FormatDimension(loadData.m_Magnitude,pDisplayUnits->GetGeneralForceUnit()));

   m_ctrlUserLoads.SetItemText(rowIdx, 3, strMagnitude);
   m_ctrlUserLoads.SetItemText(rowIdx, 4, loadData.m_Description.c_str());
}

void CApplyLoadsDlg::AddMomentLoad(int rowIdx,LoadIDType loadID)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   const CMomentLoadData& loadData = pUserDefinedLoads->FindMomentLoad(loadID);

   m_ctrlUserLoads.InsertItem(rowIdx, _T("Moment"));

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   m_ctrlUserLoads.SetItemText(rowIdx, 1, UserLoads::GetLoadCaseName(loadData.m_LoadCase).c_str());

   CString strSpan;
   if ( loadData.m_SpanGirderKey.spanIndex == ALL_SPANS )
      strSpan.Format(_T("%s"),_T("All Spans"));
   else
      strSpan.Format(_T("Span %d"),LABEL_SPAN(loadData.m_SpanGirderKey.spanIndex));

   CString strGirder;
   if ( loadData.m_SpanGirderKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(loadData.m_SpanGirderKey.girderIndex));
   }

   CString strLocation;
   if (loadData.m_Fractional)
   {
      strLocation.Format(_T("%s"),FormatPercentage(loadData.m_Location));
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(loadData.m_Location,pDisplayUnits->GetSpanLengthUnit()));
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_ctrlUserLoads.SetItemText(rowIdx,2,strLabel);

   CString strMagnitude;
   strMagnitude.Format(_T("%s"),FormatDimension(loadData.m_Magnitude,pDisplayUnits->GetMomentUnit()));

   m_ctrlUserLoads.SetItemText(rowIdx, 3, strMagnitude);
   m_ctrlUserLoads.SetItemText(rowIdx, 4, loadData.m_Description.c_str());
}
