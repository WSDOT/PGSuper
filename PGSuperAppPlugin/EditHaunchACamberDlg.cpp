///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// EditHaunchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "EditHaunchACamberDlg.h"
#include "EditHaunchDlg.h"

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include "PGSuperUnits.h"
#include "PGSuperDoc.h"
#include "Utilities.h"

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\ClosureJointData.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CEditHaunchACamberDlg, CDialog)

CEditHaunchACamberDlg::CEditHaunchACamberDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CEditHaunchACamberDlg::IDD, pParent)
{
}

CEditHaunchACamberDlg::~CEditHaunchACamberDlg()
{
}

void CEditHaunchACamberDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   DDX_CBItemData(pDX,IDC_SLAB_OFFSET_TYPE,m_SlabOffsetType);
   DDX_CBItemData(pDX,IDC_ASSUMED_EXCESS_CAMBER_TYPE,m_AssumedExcessCamberType);

   CBridgeDescription2* pBridge = GetBridgeDesc();
   // Slab offset
   if (pDX->m_bSaveAndValidate)
   {
      // Slab offset
      pBridge->SetSlabOffsetType(m_SlabOffsetType);

      // Assumed ExcessCamber
      if (m_bCanAssumedExcessCamberInputBeEnabled)
      {
         pBridge->SetAssumedExcessCamberType(m_AssumedExcessCamberType);
      }
   }

   // Slab offsets
   BOOL st;
   st = m_HaunchByBridgeDlg.UpdateData(pDX->m_bSaveAndValidate);
   if (pDX->m_bSaveAndValidate && FALSE == st)
   {
      pDX->Fail();
   }

   st = m_HaunchByBearingDlg.UpdateData(pDX->m_bSaveAndValidate);
   if (pDX->m_bSaveAndValidate && FALSE == st)
   {
      pDX->Fail();
   }

   st = m_HaunchBySegmentDlg.UpdateData(pDX->m_bSaveAndValidate);
   if (pDX->m_bSaveAndValidate && FALSE == st)
   {
      pDX->Fail();
   }

   // Assumed excess camber
   if (m_bCanAssumedExcessCamberInputBeEnabled)
   {
      st = m_AssumedExcessCamberByBridgeDlg.UpdateData(pDX->m_bSaveAndValidate);
      if (pDX->m_bSaveAndValidate && FALSE == st)
      {
         pDX->Fail();
      }

      st = m_AssumedExcessCamberBySpanDlg.UpdateData(pDX->m_bSaveAndValidate);
      if (pDX->m_bSaveAndValidate && FALSE == st)
      {
         pDX->Fail();
      }

      st = m_AssumedExcessCamberByGirderDlg.UpdateData(pDX->m_bSaveAndValidate);
      if (pDX->m_bSaveAndValidate && FALSE == st)
      {
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CEditHaunchACamberDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_SLAB_OFFSET_TYPE, &CEditHaunchACamberDlg::OnSlabOffsetTypeChanged)
   ON_CBN_SELCHANGE(IDC_ASSUMED_EXCESS_CAMBER_TYPE, &CEditHaunchACamberDlg::OnAssumedExcessCamberTypeChanged)
   ON_BN_CLICKED(ID_HELP, &CEditHaunchACamberDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditHaunchACamberDlg message handlers

BOOL CEditHaunchACamberDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISpecification, pSpec );
   m_bCanAssumedExcessCamberInputBeEnabled = pSpec->IsAssumedExcessCamberInputEnabled();

   // Initialize our data structure for current data
   InitializeData();

   // Embed dialogs for into current. A discription may be found at
   // http://www.codeproject.com/KB/dialog/embedded_dialog.aspx

   // Set up embedded dialogs
   {
      CWnd* pBox = GetDlgItem(IDC_SLAB_OFFSET_BOX);
      pBox->ShowWindow(SW_HIDE);

      CRect boxRect;
      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      VERIFY(m_HaunchByBridgeDlg.Create(CHaunchByBridgeDlg::IDD, this));
      VERIFY(m_HaunchByBridgeDlg.SetWindowPos(GetDlgItem(IDC_SLAB_OFFSET_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_HaunchByBearingDlg.Create(CHaunchByBearingDlg::IDD, this));
      VERIFY(m_HaunchByBearingDlg.SetWindowPos(GetDlgItem(IDC_SLAB_OFFSET_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_HaunchBySegmentDlg.Create(CHaunchBySegmentDlg::IDD, this));
      VERIFY(m_HaunchBySegmentDlg.SetWindowPos(GetDlgItem(IDC_SLAB_OFFSET_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      pBox = GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_BOX);
      pBox->ShowWindow(SW_HIDE);

      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      VERIFY(m_AssumedExcessCamberByBridgeDlg.Create(CAssumedExcessCamberByBridgeDlg::IDD, this));
      VERIFY(m_AssumedExcessCamberByBridgeDlg.SetWindowPos(GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_AssumedExcessCamberBySpanDlg.Create(CAssumedExcessCamberBySpanDlg::IDD, this));
      VERIFY(m_AssumedExcessCamberBySpanDlg.SetWindowPos(GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_AssumedExcessCamberByGirderDlg.Create(CAssumedExcessCamberByGirderDlg::IDD, this));
      VERIFY(m_AssumedExcessCamberByGirderDlg.SetWindowPos(GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));
   }

   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   // Slab offset type combo
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   int sqidx = pBox->AddString( GetSlabOffsetTypeAsString(pgsTypes::sotBridge, bIsPGSuper));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotBridge);
   sqidx = pBox->AddString( GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine, bIsPGSuper));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotBearingLine);
   sqidx = pBox->AddString( GetSlabOffsetTypeAsString(pgsTypes::sotSegment, bIsPGSuper));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotSegment);

   // Assumed excess camber type combo
   pBox =(CComboBox*)GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPE);
   sqidx = pBox->AddString( GetAssumedExcessCamberTypeAsString(pgsTypes::aecBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecBridge);
   sqidx = pBox->AddString( GetAssumedExcessCamberTypeAsString(pgsTypes::aecSpan));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecSpan);
   sqidx = pBox->AddString( GetAssumedExcessCamberTypeAsString(pgsTypes::aecGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecGirder);

   if (!m_bCanAssumedExcessCamberInputBeEnabled)
   {
      pBox->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_GROUP)->EnableWindow(FALSE);
   }

   CDialog::OnInitDialog();

   OnSlabOffsetTypeChanged();
   OnAssumedExcessCamberTypeChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditHaunchACamberDlg::InitializeData()
{
   const CBridgeDescription2* pBridge = GetBridgeDesc();
   const CDeckDescription2* pDeck = pBridge->GetDeckDescription();

   ATLASSERT(pDeck->GetDeckType()!= pgsTypes::sdtNone); // should not be able to edit haunch if no deck

   m_SlabOffsetType = pBridge->GetSlabOffsetType();

   m_AssumedExcessCamberType = m_bCanAssumedExcessCamberInputBeEnabled ? pBridge->GetAssumedExcessCamberType() : pgsTypes::aecBridge;
}

pgsTypes::SlabOffsetType CEditHaunchACamberDlg::GetSlabOffsetType()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   int curSel = pBox->GetCurSel();
   pgsTypes::SlabOffsetType slabOffsetType = (pgsTypes::SlabOffsetType)pBox->GetItemData(curSel);
   return slabOffsetType;
}

pgsTypes::AssumedExcessCamberType CEditHaunchACamberDlg::GetAssumedExcessCamberType()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPE);
   int curSel = pBox->GetCurSel();
   pgsTypes::AssumedExcessCamberType assumedExcessCamberType = (pgsTypes::AssumedExcessCamberType)pBox->GetItemData(curSel);
   return assumedExcessCamberType;
}

void CEditHaunchACamberDlg::OnSlabOffsetTypeChanged()
{
   switch(GetSlabOffsetType())
   {
   case pgsTypes::sotBridge:
      m_HaunchByBridgeDlg.ShowWindow(SW_SHOW);
      m_HaunchByBearingDlg.ShowWindow(SW_HIDE);
      m_HaunchBySegmentDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::sotBearingLine:
      m_HaunchByBridgeDlg.ShowWindow(SW_HIDE);
      m_HaunchByBearingDlg.ShowWindow(SW_SHOW);
      m_HaunchBySegmentDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::sotSegment:
      m_HaunchByBridgeDlg.ShowWindow(SW_HIDE);
      m_HaunchByBearingDlg.ShowWindow(SW_HIDE);
      m_HaunchBySegmentDlg.ShowWindow(SW_SHOW);
      break;
   default:
      ATLASSERT(0);
      break;
   };
}

void CEditHaunchACamberDlg::OnAssumedExcessCamberTypeChanged()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPE);
   
   int curSel = pBox->GetCurSel();
   m_AssumedExcessCamberType = (pgsTypes::AssumedExcessCamberType)pBox->GetItemData(curSel);

   switch(m_AssumedExcessCamberType)
   {
   case pgsTypes::aecBridge:
      m_AssumedExcessCamberByBridgeDlg.ShowWindow(SW_SHOW);
      m_AssumedExcessCamberBySpanDlg.ShowWindow(SW_HIDE);
      m_AssumedExcessCamberByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::aecSpan:
      m_AssumedExcessCamberByBridgeDlg.ShowWindow(SW_HIDE);
      m_AssumedExcessCamberBySpanDlg.ShowWindow(SW_SHOW);
      m_AssumedExcessCamberByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::aecGirder:
      m_AssumedExcessCamberByBridgeDlg.ShowWindow(SW_HIDE);
      m_AssumedExcessCamberBySpanDlg.ShowWindow(SW_HIDE);
      m_AssumedExcessCamberByGirderDlg.ShowWindow(SW_SHOW);
      break;
   default:
      ATLASSERT(0);
      break;
   };
}

void CEditHaunchACamberDlg::OnBnClickedHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_HAUNCH);
}

CBridgeDescription2* CEditHaunchACamberDlg::GetBridgeDesc()
{
   CEditHaunchDlg* pParent = (CEditHaunchDlg*)GetParent();
   return &(pParent->m_BridgeDesc);
}
