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

// BridgeDescRailingSystemPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "BridgeDescRailingSystemPage.h"
#include "BridgeDescDlg.h"
#include "ConcreteDetailsDlg.h"
#include <PGSuperUnits.h>
#include "HtmlHelp\HelpTopics.hh"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <MfcTools\CustomDDX.h>
#include <PGSuperColors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescRailingSystemPage property page

IMPLEMENT_DYNCREATE(CBridgeDescRailingSystemPage, CPropertyPage)

CBridgeDescRailingSystemPage::CBridgeDescRailingSystemPage() : CPropertyPage(CBridgeDescRailingSystemPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescRailingSystemPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   EAFGetBroker(&m_pBroker);

   GET_IFACE(IBridgeMaterial,pMaterial);
   m_MinNWCDensity = pMaterial->GetNWCDensityLimit();
   m_bIsNWC = true;
}

CBridgeDescRailingSystemPage::~CBridgeDescRailingSystemPage()
{
}

void CBridgeDescRailingSystemPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeDescRailingSystemPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   GET_IFACE(IDisplayUnits,pDisplayUnits);
   
   DDX_CBStringExactCase(pDX, IDC_LEFT_EXTERIORBARRIER,  m_LeftRailingSystem.strExteriorRailing );
	DDX_CBStringExactCase(pDX, IDC_RIGHT_EXTERIORBARRIER, m_RightRailingSystem.strExteriorRailing );

   DDX_CBStringExactCase(pDX, IDC_LEFT_INTERIORBARRIER,  m_LeftRailingSystem.strInteriorRailing );
	DDX_CBStringExactCase(pDX, IDC_RIGHT_INTERIORBARRIER, m_RightRailingSystem.strInteriorRailing );

   DDX_Check_Bool(pDX,IDC_LEFT_SIDEWALK,  m_LeftRailingSystem.bUseSidewalk );
   DDX_Check_Bool(pDX,IDC_RIGHT_SIDEWALK, m_RightRailingSystem.bUseSidewalk );

   DDX_Check_Bool(pDX,IDC_LEFT_SIDEWALK_PLACEMENT,  m_LeftRailingSystem.bBarriersOnTopOfSidewalk );
   DDX_Check_Bool(pDX,IDC_RIGHT_SIDEWALK_PLACEMENT, m_RightRailingSystem.bBarriersOnTopOfSidewalk );

   DDX_Check_Bool(pDX,IDC_LEFT_STRUCTURALLY_CONTINUOUS,  m_LeftRailingSystem.bSidewalkStructurallyContinuous );
   DDX_Check_Bool(pDX,IDC_RIGHT_STRUCTURALLY_CONTINUOUS, m_RightRailingSystem.bSidewalkStructurallyContinuous );

   DDX_Check_Bool(pDX,IDC_LEFT_INTERIOR_BARRIER,  m_LeftRailingSystem.bUseInteriorRailing);
   DDX_Check_Bool(pDX,IDC_RIGHT_INTERIOR_BARRIER, m_RightRailingSystem.bUseInteriorRailing);

   if ( pDX->m_bSaveAndValidate && !m_LeftRailingSystem.bUseSidewalk )
   {
      GetDlgItem(IDC_LEFT_SIDEWALK_WIDTH)->SetWindowText(m_CacheWidth[0]);
      GetDlgItem(IDC_LEFT_SIDEWALK_LEFT_DEPTH)->SetWindowText(m_CacheLeftDepth[0]);
      GetDlgItem(IDC_LEFT_SIDEWALK_RIGHT_DEPTH)->SetWindowText(m_CacheRightDepth[0]);
   }

   if ( pDX->m_bSaveAndValidate && !m_RightRailingSystem.bUseSidewalk )
   {
      GetDlgItem(IDC_RIGHT_SIDEWALK_WIDTH)->SetWindowText(m_CacheWidth[1]);
      GetDlgItem(IDC_RIGHT_SIDEWALK_LEFT_DEPTH)->SetWindowText(m_CacheLeftDepth[1]);
      GetDlgItem(IDC_RIGHT_SIDEWALK_RIGHT_DEPTH)->SetWindowText(m_CacheRightDepth[1]);
   }

   DDX_UnitValueAndTag(pDX,IDC_LEFT_SIDEWALK_WIDTH,       IDC_LEFT_SIDEWALK_WIDTH_UNIT,       m_LeftRailingSystem.Width,      pDisplayUnits->GetXSectionDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_LEFT_SIDEWALK_LEFT_DEPTH,  IDC_LEFT_SIDEWALK_LEFT_DEPTH_UNIT,  m_LeftRailingSystem.LeftDepth,  pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_LEFT_SIDEWALK_RIGHT_DEPTH, IDC_LEFT_SIDEWALK_RIGHT_DEPTH_UNIT, m_LeftRailingSystem.RightDepth, pDisplayUnits->GetComponentDimUnit());

   DDX_UnitValueAndTag(pDX,IDC_RIGHT_SIDEWALK_WIDTH,       IDC_RIGHT_SIDEWALK_WIDTH_UNIT,       m_RightRailingSystem.Width,      pDisplayUnits->GetXSectionDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_RIGHT_SIDEWALK_RIGHT_DEPTH, IDC_RIGHT_SIDEWALK_RIGHT_DEPTH_UNIT, m_RightRailingSystem.RightDepth, pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_RIGHT_SIDEWALK_LEFT_DEPTH,  IDC_RIGHT_SIDEWALK_LEFT_DEPTH_UNIT,  m_RightRailingSystem.LeftDepth,  pDisplayUnits->GetComponentDimUnit());

   DDX_UnitValueAndTag(pDX,IDC_LEFT_FC, IDC_LEFT_FC_UNIT, m_LeftRailingSystem.fc, pDisplayUnits->GetStressUnit() );
   DDX_Check_Bool(pDX,IDC_LEFT_MOD_E, m_LeftRailingSystem.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_LEFT_EC,  IDC_LEFT_EC_UNIT, m_LeftRailingSystem.Ec, pDisplayUnits->GetModEUnit() );
   DDX_UnitValueAndTag( pDX, IDC_LEFT_DENSITY,  IDC_LEFT_DENSITY_UNIT, m_LeftRailingSystem.WeightDensity, pDisplayUnits->GetDensityUnit() );

   DDX_UnitValueAndTag(pDX,IDC_RIGHT_FC, IDC_RIGHT_FC_UNIT, m_RightRailingSystem.fc, pDisplayUnits->GetStressUnit() );
   DDX_Check_Bool(pDX,IDC_RIGHT_MOD_E, m_RightRailingSystem.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_RIGHT_EC,  IDC_RIGHT_EC_UNIT, m_RightRailingSystem.Ec, pDisplayUnits->GetModEUnit() );
   DDX_UnitValueAndTag( pDX, IDC_RIGHT_DENSITY,  IDC_RIGHT_DENSITY_UNIT, m_RightRailingSystem.WeightDensity, pDisplayUnits->GetDensityUnit() );

   if ( pDX->m_bSaveAndValidate )
   {
      pDX->PrepareCtrl(IDC_LEFT_FC);
      DDV_UnitValueGreaterThanZero( pDX, m_LeftRailingSystem.fc, pDisplayUnits->GetStressUnit() );

      pDX->PrepareCtrl(IDC_LEFT_EC);
      DDV_UnitValueGreaterThanZero( pDX, m_LeftRailingSystem.Ec, pDisplayUnits->GetModEUnit() );

      pDX->PrepareCtrl(IDC_RIGHT_FC);
      DDV_UnitValueGreaterThanZero( pDX, m_RightRailingSystem.fc, pDisplayUnits->GetStressUnit() );

      pDX->PrepareCtrl(IDC_RIGHT_EC);
      DDV_UnitValueGreaterThanZero( pDX, m_RightRailingSystem.Ec, pDisplayUnits->GetModEUnit() );
   }

   if ( pDX->m_bSaveAndValidate && m_LeftRailingSystem.bUserEc )
   {
      CWnd* pWnd = GetDlgItem(IDC_LEFT_EC);
      pWnd->GetWindowText(m_strLeftUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_RightRailingSystem.bUserEc )
   {
      CWnd* pWnd = GetDlgItem(IDC_RIGHT_EC);
      pWnd->GetWindowText(m_strRightUserEc);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

      *(pParent->m_BridgeDesc.GetLeftRailingSystem())  = m_LeftRailingSystem;
      *(pParent->m_BridgeDesc.GetRightRailingSystem()) = m_RightRailingSystem;
   }

   bool bLeftNWC = true;
   bool bRightNWC = true;
   if ( m_LeftRailingSystem.bUseSidewalk && (m_LeftRailingSystem.StrengthDensity < m_MinNWCDensity || m_LeftRailingSystem.WeightDensity < m_MinNWCDensity) )
   {
      bLeftNWC = false;
   }

   if ( m_RightRailingSystem.bUseSidewalk && (m_RightRailingSystem.StrengthDensity < m_MinNWCDensity || m_RightRailingSystem.WeightDensity < m_MinNWCDensity) )
   {
      bRightNWC = false;
   }

   m_bIsNWC = bLeftNWC && bRightNWC;
}


BEGIN_MESSAGE_MAP(CBridgeDescRailingSystemPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescRailingSystemPage)
   ON_CBN_SELCHANGE(IDC_LEFT_EXTERIORBARRIER, OnLeftExteriorBarrierChanged)
   ON_CBN_SELCHANGE(IDC_RIGHT_EXTERIORBARRIER, OnRightExteriorBarrierChanged)
	ON_BN_CLICKED(IDC_LEFT_SIDEWALK, OnLeftSidewalk)
	ON_BN_CLICKED(IDC_RIGHT_SIDEWALK, OnRightSidewalk)
	ON_BN_CLICKED(IDC_LEFT_INTERIOR_BARRIER, OnLeftInteriorBarrier)
	ON_BN_CLICKED(IDC_RIGHT_INTERIOR_BARRIER, OnRightInteriorBarrier)
	ON_BN_CLICKED(IDC_LEFT_MORE, OnLeftMoreProperties)
	ON_BN_CLICKED(IDC_RIGHT_MORE, OnRightMoreProperties)
	ON_BN_CLICKED(IDC_LEFT_MOD_E, OnLeftUserEc)
	ON_EN_CHANGE(IDC_LEFT_FC, OnChangeLeftFc)
	ON_EN_CHANGE(IDC_LEFT_DENSITY, OnChangeLeftDensity)
	ON_BN_CLICKED(IDC_RIGHT_MOD_E, OnRightUserEc)
	ON_EN_CHANGE(IDC_RIGHT_FC, OnChangeRightFc)
	ON_EN_CHANGE(IDC_RIGHT_DENSITY, OnChangeRightDensity)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_WM_CTLCOLOR()
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescRailingSystemPage message handlers

BOOL CBridgeDescRailingSystemPage::OnInitDialog() 
{
   FillTrafficBarrierComboBoxes();

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   m_LeftRailingSystem = *(pParent->m_BridgeDesc.GetLeftRailingSystem());
   m_RightRailingSystem = *(pParent->m_BridgeDesc.GetRightRailingSystem());


   EnableToolTips(TRUE);

   CPropertyPage::OnInitDialog();

   if ( m_strLeftUserEc == "" )
      GetDlgItem(IDC_LEFT_EC)->GetWindowText(m_strLeftUserEc);

   if ( m_strRightUserEc == "" )
      GetDlgItem(IDC_RIGHT_EC)->GetWindowText(m_strRightUserEc);

   CComboBox* pcbLeft = (CComboBox*)GetDlgItem(IDC_LEFT_INTERIORBARRIER);
   if ( pcbLeft->GetCurSel() == CB_ERR )
      pcbLeft->SetCurSel(0);
	
   CComboBox* pcbRight = (CComboBox*)GetDlgItem(IDC_RIGHT_INTERIORBARRIER);
   if ( pcbRight->GetCurSel() == CB_ERR )
      pcbRight->SetCurSel(0);

   // Initialize the cached values
   GetDlgItem(IDC_LEFT_SIDEWALK_WIDTH)->GetWindowText(m_CacheWidth[0]);
   GetDlgItem(IDC_LEFT_SIDEWALK_LEFT_DEPTH)->GetWindowText(m_CacheLeftDepth[0]);
   GetDlgItem(IDC_LEFT_SIDEWALK_RIGHT_DEPTH)->GetWindowText(m_CacheRightDepth[0]);
   m_CacheInteriorBarrierCheck[0] = IsDlgButtonChecked(IDC_LEFT_INTERIOR_BARRIER);
   m_CacheInteriorBarrierIdx[0] = pcbLeft->GetCurSel();
	
   GetDlgItem(IDC_RIGHT_SIDEWALK_WIDTH)->GetWindowText(m_CacheWidth[1]);
   GetDlgItem(IDC_RIGHT_SIDEWALK_LEFT_DEPTH)->GetWindowText(m_CacheLeftDepth[1]);
   GetDlgItem(IDC_RIGHT_SIDEWALK_RIGHT_DEPTH)->GetWindowText(m_CacheRightDepth[1]);
   m_CacheInteriorBarrierCheck[1] = IsDlgButtonChecked(IDC_RIGHT_INTERIOR_BARRIER);
   m_CacheInteriorBarrierIdx[1] = pcbRight->GetCurSel();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   if ( IS_SI_UNITS(pDisplayUnits) )
   {
      GetDlgItem(IDC_LEFT_DENSITY_LABEL)->SetWindowText("Density");
      GetDlgItem(IDC_RIGHT_DENSITY_LABEL)->SetWindowText("Density");
   }
   else
   {
      GetDlgItem(IDC_LEFT_DENSITY_LABEL)->SetWindowText("Unit Weight");
      GetDlgItem(IDC_RIGHT_DENSITY_LABEL)->SetWindowText("Unit Weight");
   }

   // Update the UI elements
   OnLeftSidewalk();
   OnRightSidewalk();
   OnLeftInteriorBarrier();
   OnRightInteriorBarrier();

   OnLeftExteriorBarrierChanged();
   OnRightExteriorBarrierChanged();

   OnLeftUserEc();
   OnRightUserEc();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescRailingSystemPage::FillTrafficBarrierComboBoxes()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::string> names;
   std::vector<std::string>::iterator iter;

   CComboBox* pLeftExteriorBarrier  = (CComboBox*)GetDlgItem( IDC_LEFT_EXTERIORBARRIER );
   CComboBox* pRightExteriorBarrier = (CComboBox*)GetDlgItem( IDC_RIGHT_EXTERIORBARRIER );

   CComboBox* pLeftInteriorBarrier  = (CComboBox*)GetDlgItem( IDC_LEFT_INTERIORBARRIER );
   CComboBox* pRightInteriorBarrier = (CComboBox*)GetDlgItem( IDC_RIGHT_INTERIORBARRIER );
   
   pLibNames->EnumTrafficBarrierNames( &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::string& name = *iter;
      pLeftExteriorBarrier->AddString( name.c_str() );
      pRightExteriorBarrier->AddString( name.c_str() );

      pLeftInteriorBarrier->AddString( name.c_str() );
      pRightInteriorBarrier->AddString( name.c_str() );
   }
}

void CBridgeDescRailingSystemPage::OnLeftSidewalk() 
{
   BOOL bEnable = IsDlgButtonChecked(IDC_LEFT_SIDEWALK);

	EnableSidewalkDimensions(bEnable,TRUE);

   CEdit* pWidth      = (CEdit*)GetDlgItem(IDC_LEFT_SIDEWALK_WIDTH);
   CEdit* pLeftDepth  = (CEdit*)GetDlgItem(IDC_LEFT_SIDEWALK_LEFT_DEPTH);
   CEdit* pRightDepth = (CEdit*)GetDlgItem(IDC_LEFT_SIDEWALK_RIGHT_DEPTH);

   GetDlgItem(IDC_LEFT_INTERIOR_BARRIER)->EnableWindow(bEnable);
   GetDlgItem(IDC_LEFT_SIDEWALK_PLACEMENT)->EnableWindow(bEnable);
   GetDlgItem(IDC_LEFT_STRUCTURALLY_CONTINUOUS)->EnableWindow(bEnable);

   if ( bEnable )
   {
      pWidth->SetWindowText(m_CacheWidth[0]);
      pLeftDepth->SetWindowText(m_CacheLeftDepth[0]);
      pRightDepth->SetWindowText(m_CacheRightDepth[0]);

      CheckDlgButton(IDC_LEFT_INTERIOR_BARRIER,m_CacheInteriorBarrierCheck[0]);
   }
   else
   {
      pWidth->GetWindowText(m_CacheWidth[0]);
      pWidth->SetSel(0,-1);
      pWidth->Clear();

      pLeftDepth->GetWindowText(m_CacheLeftDepth[0]);
      pLeftDepth->SetSel(0,-1);
      pLeftDepth->Clear();

      pRightDepth->GetWindowText(m_CacheRightDepth[0]);
      pRightDepth->SetSel(0,-1);
      pRightDepth->Clear();

      m_CacheInteriorBarrierCheck[0] = IsDlgButtonChecked(IDC_LEFT_INTERIOR_BARRIER);
      CheckDlgButton(IDC_LEFT_INTERIOR_BARRIER,0);
   }
   OnLeftInteriorBarrier();
   OnLeftExteriorBarrierChanged();
}

void CBridgeDescRailingSystemPage::OnRightSidewalk() 
{
   BOOL bEnable = IsDlgButtonChecked(IDC_RIGHT_SIDEWALK);

	EnableSidewalkDimensions(bEnable,FALSE);

   GetDlgItem(IDC_RIGHT_INTERIOR_BARRIER)->EnableWindow(bEnable);
   GetDlgItem(IDC_RIGHT_SIDEWALK_PLACEMENT)->EnableWindow(bEnable);
   GetDlgItem(IDC_RIGHT_STRUCTURALLY_CONTINUOUS)->EnableWindow(bEnable);

   CEdit* pWidth      = (CEdit*)GetDlgItem(IDC_RIGHT_SIDEWALK_WIDTH);
   CEdit* pLeftDepth  = (CEdit*)GetDlgItem(IDC_RIGHT_SIDEWALK_LEFT_DEPTH);
   CEdit* pRightDepth = (CEdit*)GetDlgItem(IDC_RIGHT_SIDEWALK_RIGHT_DEPTH);

   if ( bEnable )
   {
      pWidth->SetWindowText(m_CacheWidth[1]);
      pLeftDepth->SetWindowText(m_CacheLeftDepth[1]);
      pRightDepth->SetWindowText(m_CacheRightDepth[1]);

      CheckDlgButton(IDC_RIGHT_INTERIOR_BARRIER,m_CacheInteriorBarrierCheck[1]);
   }
   else
   {
      pWidth->GetWindowText(m_CacheWidth[1]);
      pWidth->SetSel(0,-1);
      pWidth->Clear();

      pLeftDepth->GetWindowText(m_CacheLeftDepth[1]);
      pLeftDepth->SetSel(0,-1);
      pLeftDepth->Clear();

      pRightDepth->GetWindowText(m_CacheRightDepth[1]);
      pRightDepth->SetSel(0,-1);
      pRightDepth->Clear();

      m_CacheInteriorBarrierCheck[1] = IsDlgButtonChecked(IDC_RIGHT_INTERIOR_BARRIER);
      CheckDlgButton(IDC_RIGHT_INTERIOR_BARRIER,0);
   }
   OnRightInteriorBarrier();
   OnRightExteriorBarrierChanged();
}

void CBridgeDescRailingSystemPage::OnLeftInteriorBarrier() 
{
   BOOL bEnable = IsDlgButtonChecked(IDC_LEFT_INTERIOR_BARRIER);

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LEFT_INTERIORBARRIER);

   if ( bEnable )
   {
      pCB->SetCurSel(m_CacheInteriorBarrierIdx[0]);
   }
   else
   {
      if ( pCB->IsWindowEnabled() )
      {
         m_CacheInteriorBarrierIdx[0] = pCB->GetCurSel();
         pCB->SetCurSel(-1);
      }
   }

   EnableInteriorBarrier(bEnable,IDC_LEFT_INTERIORBARRIER);
}

void CBridgeDescRailingSystemPage::OnRightInteriorBarrier() 
{
   BOOL bEnable = IsDlgButtonChecked(IDC_RIGHT_INTERIOR_BARRIER);

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_RIGHT_INTERIORBARRIER);

   if ( bEnable )
   {
      pCB->SetCurSel(m_CacheInteriorBarrierIdx[1]);
   }
   else
   {
      if ( pCB->IsWindowEnabled() )
      {
         m_CacheInteriorBarrierIdx[1] = pCB->GetCurSel();
         pCB->SetCurSel(-1);
      }
   }

   EnableInteriorBarrier(bEnable,IDC_RIGHT_INTERIORBARRIER);
}

void CBridgeDescRailingSystemPage::OnLeftMoreProperties()
{
   CConcreteDetailsDlg dlg;

   UpdateData(TRUE);

   dlg.m_Fc      = m_LeftRailingSystem.fc;
   dlg.m_AggSize = m_LeftRailingSystem.MaxAggSize;
   dlg.m_bUserEc = m_LeftRailingSystem.bUserEc;
   dlg.m_Ds      = m_LeftRailingSystem.StrengthDensity;
   dlg.m_Dw      = m_LeftRailingSystem.WeightDensity;
   dlg.m_Ec      = m_LeftRailingSystem.Ec;
   dlg.m_K1      = m_LeftRailingSystem.K1;
   dlg.m_strUserEc  = m_strLeftUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      m_LeftRailingSystem.fc               = dlg.m_Fc;
      m_LeftRailingSystem.bUserEc          = dlg.m_bUserEc;
      m_LeftRailingSystem.StrengthDensity  = dlg.m_Ds;
      m_LeftRailingSystem.WeightDensity    = dlg.m_Dw;
      m_LeftRailingSystem.Ec               = dlg.m_Ec;
      m_LeftRailingSystem.MaxAggSize       = dlg.m_AggSize;
      m_LeftRailingSystem.K1               = dlg.m_K1;
      m_strLeftUserEc                      = dlg.m_strUserEc;

      UpdateData(FALSE);
   }
}

void CBridgeDescRailingSystemPage::OnRightMoreProperties()
{
   CConcreteDetailsDlg dlg;

   UpdateData(TRUE);

   dlg.m_Fc      = m_RightRailingSystem.fc;
   dlg.m_AggSize = m_RightRailingSystem.MaxAggSize;
   dlg.m_bUserEc = m_RightRailingSystem.bUserEc;
   dlg.m_Ds      = m_RightRailingSystem.StrengthDensity;
   dlg.m_Dw      = m_RightRailingSystem.WeightDensity;
   dlg.m_Ec      = m_RightRailingSystem.Ec;
   dlg.m_K1      = m_RightRailingSystem.K1;
   dlg.m_strUserEc  = m_strRightUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      m_RightRailingSystem.fc               = dlg.m_Fc;
      m_RightRailingSystem.bUserEc          = dlg.m_bUserEc;
      m_RightRailingSystem.StrengthDensity  = dlg.m_Ds;
      m_RightRailingSystem.WeightDensity    = dlg.m_Dw;
      m_RightRailingSystem.Ec               = dlg.m_Ec;
      m_RightRailingSystem.MaxAggSize       = dlg.m_AggSize;
      m_RightRailingSystem.K1               = dlg.m_K1;
      m_strRightUserEc                      = dlg.m_strUserEc;

      UpdateData(FALSE);
   }
}

void CBridgeDescRailingSystemPage::EnableSidewalkDimensions(BOOL bEnable,BOOL bLeft)
{
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_WIDTH_LABEL : IDC_RIGHT_SIDEWALK_WIDTH_LABEL)->EnableWindow(bEnable);
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_WIDTH       : IDC_RIGHT_SIDEWALK_WIDTH      )->EnableWindow(bEnable);
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_WIDTH_UNIT  : IDC_RIGHT_SIDEWALK_WIDTH_UNIT )->EnableWindow(bEnable);

   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_LEFT_DEPTH_LABEL : IDC_RIGHT_SIDEWALK_LEFT_DEPTH_LABEL)->EnableWindow(bEnable);
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_LEFT_DEPTH       : IDC_RIGHT_SIDEWALK_LEFT_DEPTH      )->EnableWindow(bEnable);
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_LEFT_DEPTH_UNIT  : IDC_RIGHT_SIDEWALK_LEFT_DEPTH_UNIT )->EnableWindow(bEnable);

   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_RIGHT_DEPTH_LABEL : IDC_RIGHT_SIDEWALK_RIGHT_DEPTH_LABEL)->EnableWindow(bEnable);
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_RIGHT_DEPTH       : IDC_RIGHT_SIDEWALK_RIGHT_DEPTH      )->EnableWindow(bEnable);
   GetDlgItem(bLeft ? IDC_LEFT_SIDEWALK_RIGHT_DEPTH_UNIT  : IDC_RIGHT_SIDEWALK_RIGHT_DEPTH_UNIT )->EnableWindow(bEnable);
}

void CBridgeDescRailingSystemPage::EnableInteriorBarrier(BOOL bEnable,int nID)
{
   GetDlgItem(nID)->EnableWindow(bEnable);
}


void CBridgeDescRailingSystemPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGEDESC_RAILING );
}

void CBridgeDescRailingSystemPage::OnLeftExteriorBarrierChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LEFT_EXTERIORBARRIER);
   int curSel = pCB->GetCurSel();
   CString strRailing;
   pCB->GetLBText(curSel,strRailing);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const TrafficBarrierEntry* pEntry = pLibrary->GetTrafficBarrierEntry( strRailing );
   int nShowCmd = SW_SHOW;
   if ( pEntry->GetWeightMethod() == TrafficBarrierEntry::Input )
   {
      nShowCmd = SW_HIDE;
   }

   if ( IsDlgButtonChecked(IDC_LEFT_SIDEWALK) )
      nShowCmd = SW_SHOW;

   GetDlgItem(IDC_LEFT_CONCRETE_LABEL)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_MOD_E)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_EC)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_EC_UNIT)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_FC_LABEL)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_FC)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_FC_UNIT)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_DENSITY_LABEL)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_DENSITY)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_DENSITY_UNIT)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_LEFT_MORE)->ShowWindow(nShowCmd);
}

void CBridgeDescRailingSystemPage::OnRightExteriorBarrierChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_RIGHT_EXTERIORBARRIER);
   int curSel = pCB->GetCurSel();
   CString strRailing;
   pCB->GetLBText(curSel,strRailing);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const TrafficBarrierEntry* pEntry = pLibrary->GetTrafficBarrierEntry( strRailing );
   int nShowCmd = SW_SHOW;
   if ( pEntry->GetWeightMethod() == TrafficBarrierEntry::Input )
   {
      nShowCmd = SW_HIDE;
   }

   if ( IsDlgButtonChecked(IDC_RIGHT_SIDEWALK) )
      nShowCmd = SW_SHOW;

   GetDlgItem(IDC_RIGHT_CONCRETE_LABEL)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_MOD_E)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_EC)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_EC_UNIT)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_FC_LABEL)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_FC)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_FC_UNIT)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_DENSITY_LABEL)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_DENSITY)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_DENSITY_UNIT)->ShowWindow(nShowCmd);
   GetDlgItem(IDC_RIGHT_MORE)->ShowWindow(nShowCmd);
}

void CBridgeDescRailingSystemPage::OnLeftUserEc()
{
   CButton* pChkBox = (CButton*)GetDlgItem(IDC_LEFT_MOD_E);
   BOOL bEnable = pChkBox->GetCheck();

   CWnd* pWnd = GetDlgItem(IDC_LEFT_EC);
   if (bEnable==FALSE)
   {
      pWnd->GetWindowText(m_strLeftUserEc);
      UpdateLeftEc();
   }
   else
   {
      pWnd->SetWindowText(m_strLeftUserEc);
   }

   pWnd->EnableWindow(bEnable);
   GetDlgItem(IDC_LEFT_EC_UNIT)->EnableWindow(bEnable);
}

void CBridgeDescRailingSystemPage::OnChangeLeftFc()
{
   UpdateLeftEc();
}

void CBridgeDescRailingSystemPage::OnChangeLeftDensity()
{
   UpdateLeftEc();
}

void CBridgeDescRailingSystemPage::OnRightUserEc()
{
   CButton* pChkBox = (CButton*)GetDlgItem(IDC_RIGHT_MOD_E);
   BOOL bEnable = pChkBox->GetCheck();

   CWnd* pWnd = GetDlgItem(IDC_RIGHT_EC);
   if (bEnable==FALSE)
   {
      pWnd->GetWindowText(m_strRightUserEc);
      UpdateRightEc();
   }
   else
   {
      pWnd->SetWindowText(m_strRightUserEc);
   }

   pWnd->EnableWindow(bEnable);
   GetDlgItem(IDC_RIGHT_EC_UNIT)->EnableWindow(bEnable);
}

void CBridgeDescRailingSystemPage::OnChangeRightFc()
{
   UpdateRightEc();
}

void CBridgeDescRailingSystemPage::OnChangeRightDensity()
{
   UpdateRightEc();
}

void CBridgeDescRailingSystemPage::UpdateRightEc()
{
   CWnd* pWndEc = GetDlgItem(IDC_RIGHT_EC);
   CWnd* pWndFc = GetDlgItem(IDC_RIGHT_FC);
   CWnd* pWndDensity = GetDlgItem(IDC_RIGHT_DENSITY);

   // update modulus
   if ( !IsDlgButtonChecked(IDC_RIGHT_MOD_E) )
   {
      // blank out ec
      CString strEc;
      pWndEc->SetWindowText(strEc);

      // need to manually parse strength and density values
      CString strFc, strDensity, strK1;
      pWndFc->GetWindowText(strFc);
      pWndDensity->GetWindowText(strDensity);

      strK1.Format("%f",m_RightRailingSystem.K1);

      strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1);
      pWndEc->SetWindowText(strEc);
   }
}

void CBridgeDescRailingSystemPage::UpdateLeftEc()
{
   CWnd* pWndEc = GetDlgItem(IDC_LEFT_EC);
   CWnd* pWndFc = GetDlgItem(IDC_LEFT_FC);
   CWnd* pWndDensity = GetDlgItem(IDC_LEFT_DENSITY);

   // update modulus
   if ( !IsDlgButtonChecked(IDC_LEFT_MOD_E) )
   {
      // blank out ec
      CString strEc;
      pWndEc->SetWindowText(strEc);

      // need to manually parse strength and density values
      CString strFc, strDensity, strK1;
      pWndFc->GetWindowText(strFc);
      pWndDensity->GetWindowText(strDensity);

      strK1.Format("%f",m_LeftRailingSystem.K1);

      strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1);
      pWndEc->SetWindowText(strEc);
   }
}

HBRUSH CBridgeDescRailingSystemPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hbr = CPropertyPage::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_LEFT_CONCRETE_LABEL || pWnd->GetDlgCtrlID() == IDC_RIGHT_CONCRETE_LABEL )
   {
      pDC->SetTextColor( GetSysColor(COLOR_ACTIVECAPTION) );
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_LEFT_DENSITY && 0 < pWnd->GetWindowTextLength())
   {
      try
      {
         GET_IFACE(IDisplayUnits,pDisplayUnits);
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_LEFT_DENSITY, value, pDisplayUnits->GetDensityUnit() );

         if (value < m_MinNWCDensity )
         {
            pDC->SetTextColor( RED );
         }
      }
      catch(...)
      {
      }
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_RIGHT_DENSITY )
   {
      try
      {
         GET_IFACE(IDisplayUnits,pDisplayUnits);
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_RIGHT_DENSITY, value, pDisplayUnits->GetDensityUnit() );

         if (value < m_MinNWCDensity )
         {
            pDC->SetTextColor( RED );
         }
      }
      catch(...)
      {
      }
   }

   return hbr;
}


BOOL CBridgeDescRailingSystemPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_LEFT_MORE:
         UpdateLeftConcreteParametersToolTip();
         pTTT->lpszText = m_strToolTip[pgsTypes::tboLeft].GetBuffer();
         pTTT->hinst = NULL;
         break;

      case IDC_RIGHT_MORE:
         UpdateRightConcreteParametersToolTip();
         pTTT->lpszText = m_strToolTip[pgsTypes::tboRight].GetBuffer();
         pTTT->hinst = NULL;
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip

      return TRUE;
   }
   return FALSE;
}

void CBridgeDescRailingSystemPage::UpdateLeftConcreteParametersToolTip()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format("%-20s %s\r\n%-20s %s\r\n%-20s %s",
      "Density for Strength",FormatDimension(m_LeftRailingSystem.StrengthDensity,density),
      "Density for Weight",  FormatDimension(m_LeftRailingSystem.WeightDensity,density),
      "Max Aggregate Size",  FormatDimension(m_LeftRailingSystem.MaxAggSize,aggsize)
      );

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      // add K1 parameter
      CString strK1;
      strK1.Format("\r\n%-20s %s",
         "K1",FormatScalar(m_LeftRailingSystem.K1,scalar));

      strTip += strK1;
   }

   CString strPress("\r\n\r\nPress button to edit");
   strTip += strPress;

   m_strToolTip[pgsTypes::tboLeft] = strTip;
}

void CBridgeDescRailingSystemPage::UpdateRightConcreteParametersToolTip()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format("%-20s %s\r\n%-20s %s\r\n%-20s %s",
      "Density for Strength",FormatDimension(m_RightRailingSystem.StrengthDensity,density),
      "Density for Weight",  FormatDimension(m_RightRailingSystem.WeightDensity,density),
      "Max Aggregate Size",  FormatDimension(m_RightRailingSystem.MaxAggSize,aggsize)
      );

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      // add K1 parameter
      CString strK1;
      strK1.Format("\r\n%-20s %s",
         "K1",FormatScalar(m_RightRailingSystem.K1,scalar));

      strTip += strK1;
   }

   CString strPress("\r\n\r\nPress button to edit");
   strTip += strPress;

   m_strToolTip[pgsTypes::tboRight] = strTip;
}

BOOL CBridgeDescRailingSystemPage::OnKillActive()
{
   BOOL bRetValue = CPropertyPage::OnKillActive(); // calls DoDataExchange

   if ( !m_bIsNWC )
      AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);

   return bRetValue;
}
