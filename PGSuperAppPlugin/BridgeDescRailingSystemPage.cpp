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

// BridgeDescRailingSystemPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "BridgeDescRailingSystemPage.h"
#include "BridgeDescDlg.h"
#include <PgsExt\ConcreteDetailsDlg.h>
#include "TimelineEventDlg.h"
#include <PGSuperUnits.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

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

   m_MinNWCDensity = WBFL::LRFD::ConcreteUtil::GetNWCDensityLimit();
   m_MaxLWCDensity = WBFL::LRFD::ConcreteUtil::GetLWCDensityLimit();
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
   DDV_UnitValueGreaterThanZero( pDX, IDC_LEFT_SIDEWALK_WIDTH, m_LeftRailingSystem.Width, pDisplayUnits->GetXSectionDimUnit() );
   DDX_UnitValueAndTag(pDX,IDC_LEFT_SIDEWALK_LEFT_DEPTH,  IDC_LEFT_SIDEWALK_LEFT_DEPTH_UNIT,  m_LeftRailingSystem.LeftDepth,  pDisplayUnits->GetComponentDimUnit());
   DDV_UnitValueZeroOrMore( pDX, IDC_LEFT_SIDEWALK_LEFT_DEPTH, m_LeftRailingSystem.LeftDepth, pDisplayUnits->GetXSectionDimUnit() );
   DDX_UnitValueAndTag(pDX,IDC_LEFT_SIDEWALK_RIGHT_DEPTH, IDC_LEFT_SIDEWALK_RIGHT_DEPTH_UNIT, m_LeftRailingSystem.RightDepth, pDisplayUnits->GetComponentDimUnit());
   DDV_UnitValueZeroOrMore( pDX, IDC_LEFT_SIDEWALK_RIGHT_DEPTH, m_LeftRailingSystem.RightDepth, pDisplayUnits->GetXSectionDimUnit() );

   DDX_UnitValueAndTag(pDX,IDC_RIGHT_SIDEWALK_WIDTH,       IDC_RIGHT_SIDEWALK_WIDTH_UNIT,       m_RightRailingSystem.Width,      pDisplayUnits->GetXSectionDimUnit());
   DDV_UnitValueGreaterThanZero( pDX, IDC_RIGHT_SIDEWALK_WIDTH, m_RightRailingSystem.Width, pDisplayUnits->GetXSectionDimUnit() );
   DDX_UnitValueAndTag(pDX,IDC_RIGHT_SIDEWALK_RIGHT_DEPTH, IDC_RIGHT_SIDEWALK_RIGHT_DEPTH_UNIT, m_RightRailingSystem.RightDepth, pDisplayUnits->GetComponentDimUnit());
   DDV_UnitValueZeroOrMore( pDX, IDC_RIGHT_SIDEWALK_RIGHT_DEPTH, m_RightRailingSystem.RightDepth, pDisplayUnits->GetXSectionDimUnit() );
   DDX_UnitValueAndTag(pDX,IDC_RIGHT_SIDEWALK_LEFT_DEPTH,  IDC_RIGHT_SIDEWALK_LEFT_DEPTH_UNIT,  m_RightRailingSystem.LeftDepth,  pDisplayUnits->GetComponentDimUnit());
   DDV_UnitValueZeroOrMore( pDX, IDC_RIGHT_SIDEWALK_LEFT_DEPTH, m_RightRailingSystem.LeftDepth, pDisplayUnits->GetXSectionDimUnit() );

   DDX_UnitValueAndTag(pDX,IDC_LEFT_FC, IDC_LEFT_FC_UNIT, m_LeftRailingSystem.Concrete.Fc, pDisplayUnits->GetStressUnit() );
   DDX_Check_Bool(pDX,IDC_LEFT_MOD_E, m_LeftRailingSystem.Concrete.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_LEFT_EC,  IDC_LEFT_EC_UNIT, m_LeftRailingSystem.Concrete.Ec, pDisplayUnits->GetModEUnit() );
   DDX_UnitValueAndTag( pDX, IDC_LEFT_DENSITY,  IDC_LEFT_DENSITY_UNIT, m_LeftRailingSystem.Concrete.WeightDensity, pDisplayUnits->GetDensityUnit() );

   DDX_UnitValueAndTag(pDX,IDC_RIGHT_FC, IDC_RIGHT_FC_UNIT, m_RightRailingSystem.Concrete.Fc, pDisplayUnits->GetStressUnit() );
   DDX_Check_Bool(pDX,IDC_RIGHT_MOD_E, m_RightRailingSystem.Concrete.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_RIGHT_EC,  IDC_RIGHT_EC_UNIT, m_RightRailingSystem.Concrete.Ec, pDisplayUnits->GetModEUnit() );
   DDX_UnitValueAndTag( pDX, IDC_RIGHT_DENSITY,  IDC_RIGHT_DENSITY_UNIT, m_RightRailingSystem.Concrete.WeightDensity, pDisplayUnits->GetDensityUnit() );

   if ( pDX->m_bSaveAndValidate )
   {
      DDV_UnitValueZeroOrMore( pDX, IDC_LEFT_FC, m_LeftRailingSystem.Concrete.Fc, pDisplayUnits->GetStressUnit() );
      DDV_UnitValueZeroOrMore( pDX, IDC_LEFT_EC, m_LeftRailingSystem.Concrete.Ec, pDisplayUnits->GetModEUnit() );
      DDV_UnitValueZeroOrMore( pDX, IDC_RIGHT_FC, m_RightRailingSystem.Concrete.Fc, pDisplayUnits->GetStressUnit() );
      DDV_UnitValueZeroOrMore( pDX, IDC_RIGHT_EC, m_RightRailingSystem.Concrete.Ec, pDisplayUnits->GetModEUnit() );
   }

   if ( pDX->m_bSaveAndValidate && m_LeftRailingSystem.Concrete.bUserEc )
   {
      CWnd* pWnd = GetDlgItem(IDC_LEFT_EC);
      pWnd->GetWindowText(m_strLeftUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_RightRailingSystem.Concrete.bUserEc )
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
      SetConcreteTypeLabel(IDC_LEFT_CONCRETE_TYPE_LABEL, m_LeftRailingSystem.Concrete.Type);
      SetConcreteTypeLabel(IDC_RIGHT_CONCRETE_TYPE_LABEL,m_RightRailingSystem.Concrete.Type);

      GetDlgItem(IDC_LEFT_DENSITY)->Invalidate();
      GetDlgItem(IDC_RIGHT_DENSITY)->Invalidate();
   }

   DDX_CBItemData(pDX,IDC_EVENT,m_EventIndex);

   DDX_Control(pDX, IDC_LEFT_EXTERIORBARRIER, m_LeftBrrCB);
   DDX_Control(pDX, IDC_RIGHT_EXTERIORBARRIER, m_RightBrrCB);
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
   ON_CBN_DROPDOWN(IDC_EVENT, OnEventChanging)
   ON_CBN_SELCHANGE(IDC_EVENT, OnEventChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescRailingSystemPage message handlers

BOOL CBridgeDescRailingSystemPage::OnInitDialog() 
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   m_EventIndex = pParent->m_BridgeDesc.GetTimelineManager()->GetRailingSystemLoadEventIndex();

   FillEventList();

   FillTrafficBarrierComboBoxes();

   m_LeftRailingSystem = *(pParent->m_BridgeDesc.GetLeftRailingSystem());
   m_RightRailingSystem = *(pParent->m_BridgeDesc.GetRightRailingSystem());

   EnableToolTips(TRUE);


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      GetDlgItem(IDC_EVENT)->EnableWindow(FALSE);
   }


   CPropertyPage::OnInitDialog();

   if ( m_strLeftUserEc == "" )
   {
      GetDlgItem(IDC_LEFT_EC)->GetWindowText(m_strLeftUserEc);
   }

   if ( m_strRightUserEc == "" )
   {
      GetDlgItem(IDC_RIGHT_EC)->GetWindowText(m_strRightUserEc);
   }

   CComboBox* pcbLeft = (CComboBox*)GetDlgItem(IDC_LEFT_INTERIORBARRIER);
   if ( pcbLeft->GetCurSel() == CB_ERR )
   {
      pcbLeft->SetCurSel(0);
   }
	
   CComboBox* pcbRight = (CComboBox*)GetDlgItem(IDC_RIGHT_INTERIORBARRIER);
   if ( pcbRight->GetCurSel() == CB_ERR )
   {
      pcbRight->SetCurSel(0);
   }

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
   OnMoreProperties(&m_LeftRailingSystem,&m_strLeftUserEc);
   OnLeftUserEc();
}

void CBridgeDescRailingSystemPage::OnRightMoreProperties()
{
   OnMoreProperties(&m_RightRailingSystem,&m_strRightUserEc);
   OnRightUserEc();
}

void CBridgeDescRailingSystemPage::OnMoreProperties(CRailingSystem* pRailingSystem,CString* pStrUserEc)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConcreteDetailsDlg dlg(true/*f'c*/,false/*no UHPC*/);

   UpdateData(TRUE);

   dlg.m_fc28 = pRailingSystem->Concrete.Fc;
   dlg.m_Ec28 = pRailingSystem->Concrete.Ec;
   dlg.m_bUserEc28 = pRailingSystem->Concrete.bUserEc;

   dlg.m_General.m_Type    = pRailingSystem->Concrete.Type;
   dlg.m_General.m_AggSize = pRailingSystem->Concrete.MaxAggregateSize;
   dlg.m_General.m_Ds      = pRailingSystem->Concrete.StrengthDensity;
   dlg.m_General.m_Dw      = pRailingSystem->Concrete.WeightDensity;
   dlg.m_General.m_strUserEc  = *pStrUserEc;

   dlg.m_AASHTO.m_EccK1       = pRailingSystem->Concrete.EcK1;
   dlg.m_AASHTO.m_EccK2       = pRailingSystem->Concrete.EcK2;
   dlg.m_AASHTO.m_CreepK1     = pRailingSystem->Concrete.CreepK1;
   dlg.m_AASHTO.m_CreepK2     = pRailingSystem->Concrete.CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = pRailingSystem->Concrete.ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = pRailingSystem->Concrete.ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = pRailingSystem->Concrete.bHasFct;
   dlg.m_AASHTO.m_Fct         = pRailingSystem->Concrete.Fct;

   dlg.m_ACI.m_bUserParameters = pRailingSystem->Concrete.bACIUserParameters;
   dlg.m_ACI.m_A               = pRailingSystem->Concrete.A;
   dlg.m_ACI.m_B               = pRailingSystem->Concrete.B;
   dlg.m_ACI.m_CureMethod      = pRailingSystem->Concrete.CureMethod;
   dlg.m_ACI.m_CementType      = pRailingSystem->Concrete.ACI209CementType;

   dlg.m_CEBFIP.m_bUserParameters = pRailingSystem->Concrete.bCEBFIPUserParameters;
   dlg.m_CEBFIP.m_S               = pRailingSystem->Concrete.S;
   dlg.m_CEBFIP.m_BetaSc          = pRailingSystem->Concrete.BetaSc;
   dlg.m_CEBFIP.m_CementType      = pRailingSystem->Concrete.CEBFIPCementType;

   if ( dlg.DoModal() == IDOK )
   {
      pRailingSystem->Concrete.Fc                 = dlg.m_fc28;
      pRailingSystem->Concrete.Ec                 = dlg.m_Ec28;
      pRailingSystem->Concrete.bUserEc            = dlg.m_bUserEc28;

      pRailingSystem->Concrete.Type               = dlg.m_General.m_Type;
      ATLASSERT(!IsUHPC(pRailingSystem->Concrete.Type)); // UHPC not permitted for railings
      pRailingSystem->Concrete.StrengthDensity    = dlg.m_General.m_Ds;
      pRailingSystem->Concrete.WeightDensity      = dlg.m_General.m_Dw;
      pRailingSystem->Concrete.MaxAggregateSize   = dlg.m_General.m_AggSize;

      pRailingSystem->Concrete.EcK1             = dlg.m_AASHTO.m_EccK1;
      pRailingSystem->Concrete.EcK2             = dlg.m_AASHTO.m_EccK2;
      pRailingSystem->Concrete.CreepK1          = dlg.m_AASHTO.m_CreepK1;
      pRailingSystem->Concrete.CreepK2          = dlg.m_AASHTO.m_CreepK2;
      pRailingSystem->Concrete.ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      pRailingSystem->Concrete.ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      pRailingSystem->Concrete.bHasFct          = dlg.m_AASHTO.m_bHasFct;
      pRailingSystem->Concrete.Fct              = dlg.m_AASHTO.m_Fct;

      pRailingSystem->Concrete.bACIUserParameters = dlg.m_ACI.m_bUserParameters;
      pRailingSystem->Concrete.A                  = dlg.m_ACI.m_A;
      pRailingSystem->Concrete.B                  = dlg.m_ACI.m_B;
      pRailingSystem->Concrete.CureMethod         = dlg.m_ACI.m_CureMethod;
      pRailingSystem->Concrete.ACI209CementType   = dlg.m_ACI.m_CementType;

      pRailingSystem->Concrete.bCEBFIPUserParameters = dlg.m_CEBFIP.m_bUserParameters;
      pRailingSystem->Concrete.S                     = dlg.m_CEBFIP.m_S;
      pRailingSystem->Concrete.BetaSc                = dlg.m_CEBFIP.m_BetaSc;
      pRailingSystem->Concrete.CEBFIPCementType      = dlg.m_CEBFIP.m_CementType;

      *pStrUserEc                 = dlg.m_General.m_strUserEc;

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
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_RAILING );
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
   {
      nShowCmd = SW_SHOW;
   }

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
   {
      nShowCmd = SW_SHOW;
   }

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

      strK1.Format(_T("%f"),m_RightRailingSystem.Concrete.EcK1);
      strK2.Format(_T("%f"),m_RightRailingSystem.Concrete.EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(m_RightRailingSystem.Concrete.Type,strFc,strDensity,strK1,strK2);
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

      strK1.Format(_T("%f"),m_LeftRailingSystem.Concrete.EcK1);
      strK2.Format(_T("%f"),m_LeftRailingSystem.Concrete.EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(m_LeftRailingSystem.Concrete.Type,strFc,strDensity,strK1,strK2);
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
         // DDX functions will choke if the value in the control is not a number and we'll end up in an
         // infinite loop, so first parse value to see if we have a chance of getting a number
	      const int TEXT_BUFFER_SIZE = 400;
	      TCHAR szBuffer[TEXT_BUFFER_SIZE];
         ::GetWindowText(GetDlgItem(IDC_LEFT_DENSITY)->GetSafeHwnd(), szBuffer, _countof(szBuffer));
		   Float64 d;
   		if (_sntscanf_s(szBuffer, _countof(szBuffer), _T("%lf"), &d) != 1)
         {
            pDC->SetTextColor( RED );
         }
         else
         {
            GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
            CDataExchange dx(this,TRUE);

            Float64 value;
            DDX_UnitValue(&dx, IDC_LEFT_DENSITY, value, pDisplayUnits->GetDensityUnit() );

            if ( !IsDensityInRange(value,m_LeftRailingSystem.Concrete.Type) )
            {
               pDC->SetTextColor( RED );
            }
         }
      }
      catch(...)
      {
         pDC->SetTextColor( RED );
      }
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_RIGHT_DENSITY )
   {
      try
      {
         // Again, prevent ddx from choking
	      const int TEXT_BUFFER_SIZE = 400;
	      TCHAR szBuffer[TEXT_BUFFER_SIZE];
         ::GetWindowText(GetDlgItem(IDC_RIGHT_DENSITY)->GetSafeHwnd(), szBuffer, _countof(szBuffer));
		   Float64 d;
   		if (_sntscanf_s(szBuffer, _countof(szBuffer), _T("%lf"), &d) != 1)
         {
            pDC->SetTextColor( RED );
         }
         else
         {
            GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
            CDataExchange dx(this,TRUE);

            Float64 value;
            DDX_UnitValue(&dx, IDC_RIGHT_DENSITY, value, pDisplayUnits->GetDensityUnit() );

            if ( !IsDensityInRange(value,m_RightRailingSystem.Concrete.Type) )
            {
               pDC->SetTextColor( RED );
            }
         }
      }
      catch(...)
      {
         pDC->SetTextColor( RED );
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
         pTTT->hinst = nullptr;
         break;

      case IDC_RIGHT_MORE:
         UpdateRightConcreteParametersToolTip();
         pTTT->lpszText = m_strToolTip[pgsTypes::tboRight].GetBuffer();
         pTTT->hinst = nullptr;
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
   const WBFL::Units::DensityData& density = pDisplayUnits->GetDensityUnit();
   const WBFL::Units::LengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const WBFL::Units::StressData&  stress  = pDisplayUnits->GetStressUnit();
   const WBFL::Units::ScalarData&  scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), WBFL::LRFD::ConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pRailingSystem->Concrete.Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pRailingSystem->Concrete.StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pRailingSystem->Concrete.WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pRailingSystem->Concrete.MaxAggregateSize,aggsize)
      );

   //if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   //{
   //   // add K1 parameter
   //   CString strK1;
   //   strK1.Format(_T("\r\n%-20s %s"),
   //      _T("K1"),FormatScalar(pRailingSystem->K1,scalar));

   //   strTip += strK1;
   //}

   if ( pRailingSystem->Concrete.Type != pgsTypes::Normal && pRailingSystem->Concrete.bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),_T("fct"),FormatDimension(pRailingSystem->Concrete.Fct,stress));
      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   return strTip;
}


BOOL CBridgeDescRailingSystemPage::OnKillActive()
{
   BOOL bRetValue = CPropertyPage::OnKillActive(); // calls DoDataExchange

   // Make sure data was successfully parsed before issuing a message
   if(bRetValue!=0)
   {
      // the UI should be preventing selection of PCI UHPC concrete
      ATLASSERT(!IsUHPC(m_LeftRailingSystem.Concrete.Type));
      ATLASSERT(!IsUHPC(m_RightRailingSystem.Concrete.Type));

      if ( !IsDensityInRange(m_LeftRailingSystem.Concrete.StrengthDensity,m_LeftRailingSystem.Concrete.Type) )
      {
         AfxMessageBox((m_LeftRailingSystem.Concrete.Type == pgsTypes::Normal) ? IDS_NWC_MESSAGE : IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
      }
      
      if ( !IsDensityInRange(m_RightRailingSystem.Concrete.StrengthDensity,m_RightRailingSystem.Concrete.Type) )
      {
         AfxMessageBox((m_RightRailingSystem.Concrete.Type == pgsTypes::Normal) ? IDS_NWC_MESSAGE : IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
      }
   }

   return bRetValue;
}

void CBridgeDescRailingSystemPage::SetConcreteTypeLabel(UINT nID,pgsTypes::ConcreteType type)
{
   CWnd* pWnd = GetDlgItem(nID);
   pWnd->SetWindowText( WBFL::LRFD::ConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)type,true).c_str() );
}

bool CBridgeDescRailingSystemPage::IsDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   ATLASSERT(!IsUHPC(type));
   if (type == pgsTypes::Normal )
   {
      return ( m_MinNWCDensity <= density );
   }
   else
   {
      return (density <= m_MaxLWCDensity);
   }
}

void CBridgeDescRailingSystemPage::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int eventIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( eventIdx != CB_ERR )
   {
      pcbEvent->SetCurSel(eventIdx);
   }
   else
   {
      pcbEvent->SetCurSel(0);
   }
}

void CBridgeDescRailingSystemPage::OnEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CBridgeDescRailingSystemPage::OnEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType eventIdx = (EventIndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
   }

   if (eventIdx != INVALID_INDEX)
   {
      CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

      bool bDone = false;
      bool bAdjustTimeline = false;
      while ( !bDone )
      {
         int result = pParent->m_BridgeDesc.GetTimelineManager()->SetCastDeckEventByIndex(eventIdx,bAdjustTimeline);
         if ( result == TLM_SUCCESS )
         {
            bDone = true;
         }
         else
         {
            CString strProblem = pParent->m_BridgeDesc.GetTimelineManager()->GetErrorMessage(result).c_str();
            CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));

            CString strMsg;
            strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
            if ( AfxMessageBox(strMsg,MB_YESNO | MB_ICONQUESTION) == IDYES )
            {
               bAdjustTimeline = true;
            }
            else
            {
               return;
            }

         }
      }

      FillEventList();

      pCB->SetCurSel((int)eventIdx);
   }
   else
   {
      pCB->SetCurSel((int)m_PrevEventIdx);
   }
}

EventIndexType CBridgeDescRailingSystemPage::CreateEvent()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   CTimelineEventDlg dlg(*pParent->m_BridgeDesc.GetTimelineManager(),INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      int result = pParent->m_BridgeDesc.GetTimelineManager()->AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      return eventIdx;
  }

   return INVALID_INDEX;
}