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

// BridgeDescRailingSystemPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperDoc.h"
#include "BridgeDescRailingSystemPage.h"
#include "BridgeDescDlg.h"
#include "ConcreteDetailsDlg.h"
#include <PGSuperUnits.h>
#include "HtmlHelp\HelpTopics.hh"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
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

   GET_IFACE(IBridgeMaterialEx,pMaterial);
   m_MinNWCDensity = pMaterial->GetNWCDensityLimit();
   m_MaxLWCDensity = pMaterial->GetLWCDensityLimit();
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

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   
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
      DDV_UnitValueGreaterThanZero( pDX, IDC_LEFT_FC, m_LeftRailingSystem.fc, pDisplayUnits->GetStressUnit() );
      DDV_UnitValueGreaterThanZero( pDX, IDC_LEFT_EC, m_LeftRailingSystem.Ec, pDisplayUnits->GetModEUnit() );
      DDV_UnitValueGreaterThanZero( pDX, IDC_RIGHT_FC, m_RightRailingSystem.fc, pDisplayUnits->GetStressUnit() );
      DDV_UnitValueGreaterThanZero( pDX, IDC_RIGHT_EC, m_RightRailingSystem.Ec, pDisplayUnits->GetModEUnit() );
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

   if ( !pDX->m_bSaveAndValidate )
   {
      SetConcreteTypeLabel(IDC_LEFT_CONCRETE_TYPE_LABEL, m_LeftRailingSystem.ConcreteType);
      SetConcreteTypeLabel(IDC_RIGHT_CONCRETE_TYPE_LABEL,m_RightRailingSystem.ConcreteType);

      GetDlgItem(IDC_LEFT_DENSITY)->Invalidate();
      GetDlgItem(IDC_RIGHT_DENSITY)->Invalidate();
   }
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
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   if ( IS_SI_UNITS(pDisplayUnits) )
   {
      GetDlgItem(IDC_LEFT_DENSITY_LABEL)->SetWindowText(_T("Density"));
      GetDlgItem(IDC_RIGHT_DENSITY_LABEL)->SetWindowText(_T("Density"));
   }
   else
   {
      GetDlgItem(IDC_LEFT_DENSITY_LABEL)->SetWindowText(_T("Unit Weight"));
      GetDlgItem(IDC_RIGHT_DENSITY_LABEL)->SetWindowText(_T("Unit Weight"));
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
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;

   CComboBox* pLeftExteriorBarrier  = (CComboBox*)GetDlgItem( IDC_LEFT_EXTERIORBARRIER );
   CComboBox* pRightExteriorBarrier = (CComboBox*)GetDlgItem( IDC_RIGHT_EXTERIORBARRIER );

   CComboBox* pLeftInteriorBarrier  = (CComboBox*)GetDlgItem( IDC_LEFT_INTERIORBARRIER );
   CComboBox* pRightInteriorBarrier = (CComboBox*)GetDlgItem( IDC_RIGHT_INTERIORBARRIER );
   
   pLibNames->EnumTrafficBarrierNames( &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::_tstring& name = *iter;
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
   OnMoreProperties(&m_LeftRailingSystem);
}

void CBridgeDescRailingSystemPage::OnRightMoreProperties()
{
   OnMoreProperties(&m_RightRailingSystem);
}

void CBridgeDescRailingSystemPage::OnMoreProperties(CRailingSystem* pRailingSystem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConcreteDetailsDlg dlg;

   UpdateData(TRUE);

   dlg.m_Type    = pRailingSystem->ConcreteType;
   dlg.m_Fc      = pRailingSystem->fc;
   dlg.m_AggSize = pRailingSystem->MaxAggSize;
   dlg.m_bUserEc = pRailingSystem->bUserEc;
   dlg.m_Ds      = pRailingSystem->StrengthDensity;
   dlg.m_Dw      = pRailingSystem->WeightDensity;
   dlg.m_Ec      = pRailingSystem->Ec;
   dlg.m_EccK1       = pRailingSystem->EcK1;
   dlg.m_EccK2       = pRailingSystem->EcK2;
   dlg.m_CreepK1     = pRailingSystem->CreepK1;
   dlg.m_CreepK2     = pRailingSystem->CreepK2;
   dlg.m_ShrinkageK1 = pRailingSystem->ShrinkageK1;
   dlg.m_ShrinkageK2 = pRailingSystem->ShrinkageK2;
   dlg.m_strUserEc  = m_strRightUserEc;
   dlg.m_bHasFct    = pRailingSystem->bHasFct;
   dlg.m_Fct        = pRailingSystem->Fct;

   if ( dlg.DoModal() == IDOK )
   {
      pRailingSystem->ConcreteType     = dlg.m_Type;
      pRailingSystem->fc               = dlg.m_Fc;
      pRailingSystem->bUserEc          = dlg.m_bUserEc;
      pRailingSystem->StrengthDensity  = dlg.m_Ds;
      pRailingSystem->WeightDensity    = dlg.m_Dw;
      pRailingSystem->Ec               = dlg.m_Ec;
      pRailingSystem->MaxAggSize       = dlg.m_AggSize;
      pRailingSystem->EcK1             = dlg.m_EccK1;
      pRailingSystem->EcK2             = dlg.m_EccK2;
      pRailingSystem->CreepK1          = dlg.m_CreepK1;
      pRailingSystem->CreepK2          = dlg.m_CreepK2;
      pRailingSystem->ShrinkageK1      = dlg.m_ShrinkageK1;
      pRailingSystem->ShrinkageK2      = dlg.m_ShrinkageK2;
      m_strRightUserEc                 = dlg.m_strUserEc;
      pRailingSystem->bHasFct          = dlg.m_bHasFct;
      pRailingSystem->Fct              = dlg.m_Fct;

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
   GetDlgItem(IDC_LEFT_CONCRETE_TYPE_LABEL)->ShowWindow(nShowCmd);
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
   GetDlgItem(IDC_RIGHT_CONCRETE_TYPE_LABEL)->ShowWindow(nShowCmd);
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
      CString strFc, strDensity, strK1, strK2;
      pWndFc->GetWindowText(strFc);
      pWndDensity->GetWindowText(strDensity);

      strK1.Format(_T("%f"),m_RightRailingSystem.EcK1);
      strK2.Format(_T("%f"),m_RightRailingSystem.EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
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
      CString strFc, strDensity, strK1, strK2;
      pWndFc->GetWindowText(strFc);
      pWndDensity->GetWindowText(strDensity);

      strK1.Format(_T("%f"),m_LeftRailingSystem.EcK1);
      strK2.Format(_T("%f"),m_LeftRailingSystem.EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
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
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_LEFT_DENSITY, value, pDisplayUnits->GetDensityUnit() );

         if ( !IsDensityInRange(value,m_LeftRailingSystem.ConcreteType) )
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
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_RIGHT_DENSITY, value, pDisplayUnits->GetDensityUnit() );

         if ( !IsDensityInRange(value,m_RightRailingSystem.ConcreteType) )
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
   m_strToolTip[pgsTypes::tboLeft] = UpdateConcreteParametersToolTip(&m_LeftRailingSystem);
}

void CBridgeDescRailingSystemPage::UpdateRightConcreteParametersToolTip()
{
   m_strToolTip[pgsTypes::tboRight] = UpdateConcreteParametersToolTip(&m_RightRailingSystem);
}

CString CBridgeDescRailingSystemPage::UpdateConcreteParametersToolTip(CRailingSystem* pRailingSystem)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtStressData&  stress  = pDisplayUnits->GetStressUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), matConcrete::GetTypeName((matConcrete::Type)pRailingSystem->ConcreteType,true).c_str(),
      _T("Unit Weight"),FormatDimension(pRailingSystem->StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pRailingSystem->WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pRailingSystem->MaxAggSize,aggsize)
      );

   //if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   //{
   //   // add K1 parameter
   //   CString strK1;
   //   strK1.Format(_T("\r\n%-20s %s"),
   //      _T("K1"),FormatScalar(pRailingSystem->K1,scalar));

   //   strTip += strK1;
   //}

   if ( pRailingSystem->ConcreteType != pgsTypes::Normal && pRailingSystem->bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pRailingSystem->Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   return strTip;
}


BOOL CBridgeDescRailingSystemPage::OnKillActive()
{
   BOOL bRetValue = CPropertyPage::OnKillActive(); // calls DoDataExchange

   if ( !IsDensityInRange(m_LeftRailingSystem.StrengthDensity,m_LeftRailingSystem.ConcreteType) ||
        !IsDensityInRange(m_LeftRailingSystem.WeightDensity,  m_LeftRailingSystem.ConcreteType) )
   {
      if ( m_LeftRailingSystem.ConcreteType == pgsTypes::Normal )
         AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
      else
         AfxMessageBox(IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
   }
   else if ( !IsDensityInRange(m_RightRailingSystem.StrengthDensity,m_RightRailingSystem.ConcreteType) ||
             !IsDensityInRange(m_RightRailingSystem.WeightDensity,  m_RightRailingSystem.ConcreteType) )
   {
      if ( m_RightRailingSystem.ConcreteType == pgsTypes::Normal )
         AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
      else
         AfxMessageBox(IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
   }

   return bRetValue;
}

void CBridgeDescRailingSystemPage::SetConcreteTypeLabel(UINT nID,pgsTypes::ConcreteType type)
{
   CWnd* pWnd = GetDlgItem(nID);
   pWnd->SetWindowText( matConcrete::GetTypeName((matConcrete::Type)type,true).c_str() );
}

bool CBridgeDescRailingSystemPage::IsDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   if ( type == pgsTypes::Normal )
   {
      return ( m_MinNWCDensity <= density );
   }
   else
   {
      return (density <= m_MaxLWCDensity);
   }
}
