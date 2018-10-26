///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// LiveLoadSelectDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "LiveLoadSelectDlg.h"
#include <..\htmlhelp\helptopics.hh>


#include <PgsExt\BridgeDescription2.h>
#include "PGSuperAppPlugin\TimelineEventDlg.h"

#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CLiveLoadSelectDlg dialog


CLiveLoadSelectDlg::CLiveLoadSelectDlg(std::vector< std::_tstring>& allNames, std::vector< std::_tstring>& dsgnNames,
                                       std::vector< std::_tstring>& fatigueNames,std::vector< std::_tstring>& str2Names,CWnd* pParent /*=NULL*/)
	: CDialog(CLiveLoadSelectDlg::IDD, pParent), 
     m_AllNames(allNames),
     m_DesignNames(dsgnNames),
     m_FatigueNames(fatigueNames),
     m_PermitNames(str2Names),
     m_bHasPedestrianLoad(false)
{
	//{{AFX_DATA_INIT(CLiveLoadSelectDlg)
	//}}AFX_DATA_INIT
   m_DesignTruckImpact = 0;
   m_DesignLaneImpact = 0;
   m_FatigueTruckImpact = 0;
   m_FatigueLaneImpact = 0;
   m_PermitTruckImpact = 0;
   m_PermitLaneImpact = 0;

   m_DesignPedesType = ILiveLoads::PedDontApply;
   m_FatiguePedesType = ILiveLoads::PedDontApply;
   m_PermitPedesType = ILiveLoads::PedDontApply;
}


void CLiveLoadSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLiveLoadSelectDlg)
	DDX_Control(pDX, IDC_DESIGN_LL_LIST, m_ctlDesignLL);
	DDX_Control(pDX, IDC_FATIGUE_LL_LIST, m_ctlFatigueLL);
	DDX_Control(pDX, IDC_STRENGTH_LL_LIST, m_ctlPermitLL);
	//}}AFX_DATA_MAP

   DDX_Percentage(pDX,IDC_DESIGN_TRUCK_IMPACT,  m_DesignTruckImpact);
   DDX_Percentage(pDX,IDC_DESIGN_LANE_IMPACT,   m_DesignLaneImpact);
   DDX_Percentage(pDX,IDC_FATIGUE_TRUCK_IMPACT, m_FatigueTruckImpact);
   DDX_Percentage(pDX,IDC_FATIGUE_LANE_IMPACT,  m_FatigueLaneImpact);
   DDX_Percentage(pDX,IDC_PERMIT_TRUCK_IMPACT,  m_PermitTruckImpact);
   DDX_Percentage(pDX,IDC_PERMIT_LANE_IMPACT,   m_PermitLaneImpact);

   DDX_CBItemData(pDX, IDC_FATIGUE_PEDES_COMBO, m_FatiguePedesType);
   DDX_CBItemData(pDX, IDC_PERMIT_PEDES_COMBO,  m_PermitPedesType);
   DDX_CBItemData(pDX, IDC_DESIGN_PEDES_COMBO,  m_DesignPedesType);

   DDX_CBItemData(pDX,IDC_EVENT, m_LiveLoadEvent);

   if (pDX->m_bSaveAndValidate)
   {
      if ( m_LiveLoadEvent == INVALID_INDEX )
      {
         pDX->PrepareCtrl(IDC_EVENT);
         AfxMessageBox(_T("Select a live load event"));
         pDX->Fail();
      }

      m_DesignNames.clear(); // reuse vector 
      int cnt = m_ctlDesignLL.GetCount();
      int icnt;
      for (icnt=0; icnt<cnt; icnt++)
      {
         int chk = m_ctlDesignLL.GetCheck(icnt);
         if (chk==1)
         {
            CString str;
            m_ctlDesignLL.GetText(icnt, str);
            m_DesignNames.push_back( std::_tstring(str));
         }
      }

      // selected fatigue names
      m_FatigueNames.clear(); // reuse vector 
      cnt = m_ctlFatigueLL.GetCount();
      for (icnt=0; icnt<cnt; icnt++)
      {
         int chk = m_ctlFatigueLL.GetCheck(icnt);
         if (chk==1)
         {
            CString str;
            m_ctlFatigueLL.GetText(icnt, str);
            m_FatigueNames.push_back( std::_tstring(str));
         }
      }

      // selected permit names
      m_PermitNames.clear(); // reuse vector 
      cnt = m_ctlPermitLL.GetCount();
      for (icnt=0; icnt<cnt; icnt++)
      {
         int chk = m_ctlPermitLL.GetCheck(icnt);
         if (chk==1)
         {
            CString str;
            m_ctlPermitLL.GetText(icnt, str);
            m_PermitNames.push_back( std::_tstring(str));
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CLiveLoadSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CLiveLoadSelectDlg)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
   ON_CBN_SELCHANGE(IDC_EVENT, OnEventChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnEventChanging)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadSelectDlg message handlers

void CLiveLoadSelectDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SELECT_LIVELOAD);
}

BOOL CLiveLoadSelectDlg::OnInitDialog() 
{
   // Strings for pedestrian load application
   SetPedestrianComboText(IDC_DESIGN_PEDES_COMBO, IDC_DESIGN_PEDES_STATIC);
   SetPedestrianComboText(IDC_FATIGUE_PEDES_COMBO,IDC_FATIGUE_PEDES_STATIC);
   SetPedestrianComboText(IDC_PERMIT_PEDES_COMBO, IDC_PERMIT_PEDES_STATIC);

   FillEventList();

	CDialog::OnInitDialog();
	
   m_ctlDesignLL.SetCheckStyle( BS_AUTOCHECKBOX );
   m_ctlFatigueLL.SetCheckStyle( BS_AUTOCHECKBOX );
   m_ctlPermitLL.SetCheckStyle( BS_AUTOCHECKBOX );

   for (std::vector< std::_tstring>::iterator it=m_AllNames.begin(); it!=m_AllNames.end(); it++)
   {
      LPCTSTR str = it->c_str();
      m_ctlDesignLL.AddString(str);
      m_ctlFatigueLL.AddString(str);
      m_ctlPermitLL.AddString(str);
   }


   // Set the check marks for the various loads
   // design
   std::vector< std::_tstring>::reverse_iterator iter;
   for ( iter = m_DesignNames.rbegin(); iter != m_DesignNames.rend(); iter++)
   {
      LPCTSTR str = iter->c_str();
      int idx = m_ctlDesignLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlDesignLL.SetCheck( idx, 1 );
         m_ctlDesignLL.SetTopIndex(idx);
      }
   }

   // fatigue
   for (iter = m_FatigueNames.rbegin(); iter != m_FatigueNames.rend(); iter++)
   {
      LPCTSTR str = iter->c_str();
      int idx = m_ctlFatigueLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlFatigueLL.SetCheck( idx, 1 );
         m_ctlFatigueLL.SetTopIndex(idx);
      }
   }

   // permit
   for (iter = m_PermitNames.rbegin(); iter != m_PermitNames.rend(); iter++)
   {
      LPCTSTR str = iter->c_str();
      int idx = m_ctlPermitLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlPermitLL.SetCheck( idx, 1 );
         m_ctlPermitLL.SetTopIndex(idx);
      }
   }

   // Set text for pedestrian load application


   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_LIVELOAD),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLiveLoadSelectDlg::SetPedestrianComboText(int iCombo, int iStatic)
{
   CComboBox* pCombo = (CComboBox*)GetDlgItem(iCombo);
   CWnd* pStatic = GetDlgItem(iStatic);

   int idx = pCombo->AddString(_T("No Ped. Load, vehicular live load only"));
   pCombo->SetItemData(idx,(DWORD_PTR)ILiveLoads::PedDontApply);

   idx = pCombo->AddString(_T("Concurrently with vehicular live load"));
   pCombo->SetItemData(idx,(DWORD_PTR)ILiveLoads::PedConcurrentWithVehicular);

   idx = pCombo->AddString(_T("Enveloped with vehicular live load"));
   pCombo->SetItemData(idx,(DWORD_PTR)ILiveLoads::PedEnvelopeWithVehicular);

   pCombo->EnableWindow(m_bHasPedestrianLoad? TRUE:FALSE);
   pStatic->EnableWindow(m_bHasPedestrianLoad? TRUE:FALSE);
}

void CLiveLoadSelectDlg::FillEventList()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      GetDlgItem(IDC_EVENT_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EVENT)->ShowWindow(SW_HIDE);
   }

   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int eventIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

#pragma Reminder("UPDATE: this dialog needs to work on a local bridge model... and use the local timeline manager")
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
   }

   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
      pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);
   }

   if ( eventIdx != CB_ERR )
   {
      pcbEvent->SetCurSel(eventIdx);
   }
   else
   {
      pcbEvent->SetCurSel(0);
   }
}

void CLiveLoadSelectDlg::OnEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CLiveLoadSelectDlg::OnEventChanged()
{
#pragma Reminder("UPDATE: this dialog needs to work on a local bridge model... and use the local timeline manager")
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType idx = (IndexType)pCB->GetItemData(curSel);
   if ( idx == CREATE_TIMELINE_EVENT )
   {
      EventIndexType eventIdx = CreateEvent();
      if (eventIdx != INVALID_INDEX)
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         pIBridgeDesc->SetLiveLoadEventByIndex(eventIdx);

         FillEventList();

         pCB->SetCurSel((int)idx);
         m_LiveLoadEvent = eventIdx;
      }
      else
      {
         pCB->SetCurSel((int)m_PrevEventIdx);
      }
   }
}

EventIndexType CLiveLoadSelectDlg::CreateEvent()
{
#pragma Reminder("UPDATE: this dialog needs to work on a local bridge model... and use the local timeline manager")
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CTimelineEventDlg dlg(*pTimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      return pIBridgeDesc->AddTimelineEvent(*dlg.m_pTimelineEvent);
  }

   return INVALID_INDEX;
}