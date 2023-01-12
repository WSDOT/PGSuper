///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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


IMPLEMENT_DYNAMIC(CEditHaunchDlg, CDialog)

CEditHaunchDlg::CEditHaunchDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent /*=nullptr*/)
	: CDialog(CEditHaunchDlg::IDD, pParent),
   m_HaunchShape(pgsTypes::hsSquare)
{
   m_BridgeDesc = *pBridgeDesc;
}

CEditHaunchDlg::~CEditHaunchDlg()
{
}

void CEditHaunchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // fillet
   DDX_UnitValueAndTag( pDX, IDC_FILLET, IDC_FILLET_UNIT, m_Fillet, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_FILLET, m_Fillet, pDisplayUnits->GetComponentDimUnit() );

   DDX_Control(pDX,IDC_HAUNCH_SHAPE,m_cbHaunchShape);
   DDX_CBItemData(pDX, IDC_HAUNCH_SHAPE, m_HaunchShape);

   DDX_CBItemData(pDX, IDC_SLAB_OFFSET_TYPE, m_SlabOffsetType);

   DDX_CBItemData(pDX, IDC_ASSUMED_EXCESS_CAMBER_TYPE, m_AssumedExcessCamberType);

   // Slab offsets
   m_HaunchByBridgeDlg.UpdateData(pDX->m_bSaveAndValidate);
   m_HaunchByBearingDlg.UpdateData(pDX->m_bSaveAndValidate);
   m_HaunchBySegmentDlg.UpdateData(pDX->m_bSaveAndValidate);

   // Assumed excess camber
   if (m_bCanAssumedExcessCamberInputBeEnabled)
   {
      m_AssumedExcessCamberByBridgeDlg.UpdateData(pDX->m_bSaveAndValidate);
      m_AssumedExcessCamberBySpanDlg.UpdateData(pDX->m_bSaveAndValidate);
      m_AssumedExcessCamberByGirderDlg.UpdateData(pDX->m_bSaveAndValidate);
   }

   if (pDX->m_bSaveAndValidate)
   {
      // Haunch shape
      m_BridgeDesc.GetDeckDescription()->HaunchShape = m_HaunchShape;

      m_BridgeDesc.SetFillet(m_Fillet);

      // Slab offset
      m_BridgeDesc.SetSlabOffsetType(m_SlabOffsetType);

      // Assumed ExcessCamber
      if (m_bCanAssumedExcessCamberInputBeEnabled)
      {
         m_BridgeDesc.SetAssumedExcessCamberType(m_AssumedExcessCamberType);
      }
   }
}


BEGIN_MESSAGE_MAP(CEditHaunchDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_SLAB_OFFSET_TYPE, &CEditHaunchDlg::OnSlabOffsetTypeChanged)
   ON_CBN_SELCHANGE(IDC_ASSUMED_EXCESS_CAMBER_TYPE, &CEditHaunchDlg::OnAssumedExcessCamberTypeChanged)
   ON_BN_CLICKED(ID_HELP, &CEditHaunchDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditHaunchDlg message handlers

BOOL CEditHaunchDlg::OnInitDialog()
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

   m_cbHaunchShape.Initialize(m_HaunchShape);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_HAUNCH_SHAPE,m_HaunchShape);

   if ( IsAdjacentSpacing(m_BridgeDesc.GetGirderSpacingType()) )
   {
      m_cbHaunchShape.EnableWindow(FALSE); // cannot change haunch shape for adjacent spacing
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditHaunchDlg::InitializeData()
{
   const CDeckDescription2* pDeck = m_BridgeDesc.GetDeckDescription();
   ATLASSERT(pDeck->GetDeckType()!= pgsTypes::sdtNone); // should not be able to edit haunch if no deck

   // If girder spacing is adjacent, force haunch shape to square
   m_HaunchShape = IsAdjacentSpacing(m_BridgeDesc.GetGirderSpacingType()) ? pgsTypes::hsSquare : pDeck->HaunchShape;

   m_Fillet = m_BridgeDesc.GetFillet();

   m_SlabOffsetType = m_BridgeDesc.GetSlabOffsetType();

   m_AssumedExcessCamberType = m_bCanAssumedExcessCamberInputBeEnabled ? m_BridgeDesc.GetAssumedExcessCamberType() : pgsTypes::aecBridge;
}

pgsTypes::SlabOffsetType CEditHaunchDlg::GetSlabOffsetType()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   int curSel = pBox->GetCurSel();
   pgsTypes::SlabOffsetType slabOffsetType = (pgsTypes::SlabOffsetType)pBox->GetItemData(curSel);
   return slabOffsetType;
}

pgsTypes::AssumedExcessCamberType CEditHaunchDlg::GetAssumedExcessCamberType()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPE);
   int curSel = pBox->GetCurSel();
   pgsTypes::AssumedExcessCamberType assumedExcessCamberType = (pgsTypes::AssumedExcessCamberType)pBox->GetItemData(curSel);
   return assumedExcessCamberType;
}

void CEditHaunchDlg::OnSlabOffsetTypeChanged()
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

void CEditHaunchDlg::OnAssumedExcessCamberTypeChanged()
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

void CEditHaunchDlg::OnBnClickedHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_HAUNCH);
}
