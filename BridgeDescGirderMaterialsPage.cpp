///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// BridgeDescGirderMaterialsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "PGSuperUnits.h"
#include "BridgeDescGirderMaterialsPage.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#include "SelectItemDlg.h"

#include "GirderDescDlg.h"
#include "CopyConcreteEntry.h"
#include <MfcTools\CustomDDX.h>
#include "HtmlHelp\HelpTopics.hh"

#include <PgsExt\BridgeDescription.h>

#include <Atlddx.h>
#include <system\tokenizer.h>

#include "ConcreteDetailsDlg.h"
#include "PGSuperUnits.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage property page

IMPLEMENT_DYNCREATE(CGirderDescGeneralPage, CPropertyPage)

CGirderDescGeneralPage::CGirderDescGeneralPage() : CPropertyPage(CGirderDescGeneralPage::IDD)

{
	//{{AFX_DATA_INIT(CGirderDescGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderDescGeneralPage::~CGirderDescGeneralPage()
{
}

void CGirderDescGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescGeneralPage)
	DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	DDX_Control(pDX, IDC_ECI,     m_ctrlEci);
	DDX_Control(pDX, IDC_MOD_EC,  m_ctrlEcCheck);
	DDX_Control(pDX, IDC_MOD_ECI, m_ctrlEciCheck);
   DDX_Control(pDX, IDC_GIRDER_FC, m_ctrlFc);
	DDX_Control(pDX, IDC_FCI, m_ctrlFci);
	//}}AFX_DATA_MAP

   DDX_Control(pDX,IDC_GIRDERNAME_NOTE,m_GirderTypeHyperLink);
   DDX_Control(pDX,IDC_SLABOFFSET_NOTE,m_SlabOffsetHyperLink);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,pParent->m_strGirderName);

   // concrete material
   ExchangeConcreteData(pDX);

   DDX_UnitValueAndTag( pDX, IDC_FCI, IDC_FCI_UNIT, pParent->m_GirderData.Material.Fci, pDisplayUnits->GetStressUnit() );
   // Validation: 0 < f'ci <= f'c   
   DDV_UnitValueLimitOrLess( pDX, IDC_FCI, pParent->m_GirderData.Material.Fci,  pParent->m_GirderData.Material.Fc, pDisplayUnits->GetStressUnit() );

   // Slab Offset
   DDX_Tag(pDX, IDC_ADIM_START_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_ADIM_END_UNIT,   pDisplayUnits->GetComponentDimUnit() );

   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      DDX_UnitValueAndTag( pDX, IDC_ADIM_START, IDC_ADIM_START_UNIT, m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit() );
      DDX_UnitValueAndTag( pDX, IDC_ADIM_END,   IDC_ADIM_END_UNIT,   m_SlabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit() );

      // validate slab offset... (must be greater or equal gross deck thickess)
      if ( pDX->m_bSaveAndValidate )
      {
         Float64 Lg = pBridge->GetGirderLength(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx);
         pgsPointOfInterest poiStart(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx,0.0);
         pgsPointOfInterest poiEnd(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx,Lg);
         Float64 grossDeckThicknessStart = pBridge->GetGrossSlabDepth(poiStart) + pBridge->GetFillet();
         Float64 grossDeckThicknessEnd   = pBridge->GetGrossSlabDepth(poiEnd) + pBridge->GetFillet();
         
         m_SlabOffset[pgsTypes::metStart] = (IsEqual(m_SlabOffset[pgsTypes::metStart],grossDeckThicknessStart) ? grossDeckThicknessStart : m_SlabOffset[pgsTypes::metStart]);
         m_SlabOffset[pgsTypes::metEnd]   = (IsEqual(m_SlabOffset[pgsTypes::metEnd],  grossDeckThicknessEnd  ) ? grossDeckThicknessEnd   : m_SlabOffset[pgsTypes::metEnd]);

         if ( m_SlabOffset[pgsTypes::metStart] < grossDeckThicknessStart )
         {
            pDX->PrepareEditCtrl(IDC_ADIM_START);
            CString msg;
            msg.Format(_T("The slab offset at the start of the girder must be at equal to the slab + fillet depth of %s"),FormatDimension(grossDeckThicknessStart,pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg,MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         if ( m_SlabOffset[pgsTypes::metEnd] < grossDeckThicknessEnd )
         {
            pDX->PrepareEditCtrl(IDC_ADIM_END);
            CString msg;
            msg.Format(_T("The slab offset at the end of the girder must be at equal to the slab + fillet depth of %s"),FormatDimension(grossDeckThicknessEnd,pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg,MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
   }
}

void CGirderDescGeneralPage::ExchangeConcreteData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   DDX_UnitValueAndTag( pDX, IDC_GIRDER_FC,  IDC_GIRDER_FC_UNIT,   pParent->m_GirderData.Material.Fc , pDisplayUnits->GetStressUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_GIRDER_FC,pParent->m_GirderData.Material.Fc, pDisplayUnits->GetStressUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_ECI, pParent->m_GirderData.Material.bUserEci);
   DDX_UnitValueAndTag( pDX, IDC_ECI,  IDC_ECI_UNIT,   pParent->m_GirderData.Material.Eci , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_ECI,pParent->m_GirderData.Material.Eci, pDisplayUnits->GetModEUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_EC,  pParent->m_GirderData.Material.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_EC,  IDC_EC_UNIT, pParent->m_GirderData.Material.Ec , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_EC, pParent->m_GirderData.Material.Ec, pDisplayUnits->GetModEUnit() );

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_ctrlEciCheck.GetCheck() == 1 )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }

   if ( !pDX->m_bSaveAndValidate )
   {
      GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText( matConcrete::GetTypeName((matConcrete::Type)pParent->m_GirderData.Material.Type,true).c_str() );
   }
}



void CGirderDescGeneralPage::OnCopyMaterial() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CCopyConcreteEntry dlg(true, this);
   INT_PTR result = dlg.DoModal();

   if ( result < 0 )
   {
      ::AfxMessageBox(_T("There are no Concrete Material Entries in the library"),MB_OK);
   }
   else if (result == IDOK)
   {
      const ConcreteLibraryEntry* entry = dlg.m_ConcreteEntry;

      if (entry!=NULL)
      {
         CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
         pParent->m_GirderData.Material.Type = entry->GetType();
         pParent->m_GirderData.Material.Fc = entry->GetFc();
         pParent->m_GirderData.Material.WeightDensity = entry->GetWeightDensity();
         pParent->m_GirderData.Material.StrengthDensity = entry->GetStrengthDensity();
         pParent->m_GirderData.Material.MaxAggregateSize = entry->GetAggregateSize();
         pParent->m_GirderData.Material.EcK1 = entry->GetModEK1();
         pParent->m_GirderData.Material.EcK2 = entry->GetModEK2();
         pParent->m_GirderData.Material.CreepK1 = entry->GetCreepK1();
         pParent->m_GirderData.Material.CreepK2 = entry->GetCreepK2();
         pParent->m_GirderData.Material.ShrinkageK1 = entry->GetShrinkageK1();
         pParent->m_GirderData.Material.ShrinkageK2 = entry->GetShrinkageK2();

         pParent->m_GirderData.Material.bUserEc = entry->UserEc();
         pParent->m_GirderData.Material.Ec = entry->GetEc();

         pParent->m_GirderData.Material.bHasFct = entry->HasAggSplittingStrength();
         pParent->m_GirderData.Material.Fct     = entry->GetAggSplittingStrength();

         CDataExchange dx(this,FALSE);
         ExchangeConcreteData(&dx);

         OnUserEci();
         OnUserEc();
      }
   }
}

BEGIN_MESSAGE_MAP(CGirderDescGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescGeneralPage)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
   ON_BN_CLICKED(IDC_MOD_ECI, OnUserEci)
   ON_BN_CLICKED(IDC_MOD_EC, OnUserEc)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_EN_CHANGE(IDC_FCI, OnChangeFci)
	ON_EN_CHANGE(IDC_GIRDER_FC, OnChangeGirderFc)
	ON_BN_CLICKED(IDC_MORE, OnMoreConcreteProperties)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	ON_WM_CTLCOLOR()
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderType,OnChangeSameGirderType)
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffsetType,OnChangeSlabOffsetType)
   ON_CBN_SELCHANGE(IDC_GIRDER_NAME,OnChangeGirderName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage message handlers

BOOL CGirderDescGeneralPage::OnInitDialog() 
{
   FillGirderComboBox();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   m_bUseSameGirderType = pBridgeDesc->UseSameGirderForEntireBridge();
   m_SlabOffsetType = pBridgeDesc->GetSlabOffsetType();
   if ( m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotSpan )
      m_SlabOffsetTypeCache = pgsTypes::sotGirder;
   else
      m_SlabOffsetTypeCache = pgsTypes::sotBridge;

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   const CGirderTypes* pGirderTypes = pBridgeDesc->GetSpan(pParent->m_CurrentSpanIdx)->GetGirderTypes();
   pParent->m_strGirderName = pGirderTypes->GetGirderName(pParent->m_CurrentGirderIdx);

   m_SlabOffset[pgsTypes::metStart] = pGirderTypes->GetSlabOffset(pParent->m_CurrentGirderIdx,pgsTypes::metStart);
   m_SlabOffset[pgsTypes::metEnd]   = pGirderTypes->GetSlabOffset(pParent->m_CurrentGirderIdx,pgsTypes::metEnd);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_strSlabOffsetCache[pgsTypes::metStart].Format(_T("%s"),FormatDimension(m_SlabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false));
   m_strSlabOffsetCache[pgsTypes::metEnd].Format(  _T("%s"),FormatDimension(m_SlabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit(),false));

   CPropertyPage::OnInitDialog();

   UpdateGirderTypeHyperLink();
   UpdateGirderTypeControls();

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   if ( m_strUserEc == _T("") )
      m_ctrlEc.GetWindowText(m_strUserEc);
	
   if ( m_strUserEci == _T("") )
      m_ctrlEci.GetWindowText(m_strUserEci);

   OnUserEci();
   OnUserEc();

   EnableToolTips(TRUE);

   if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   {
      // disable slab offset input if there isn't a deck
      m_SlabOffsetHyperLink.EnableWindow(FALSE);

      GetDlgItem(IDC_ADIM_START_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_START)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_START_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_ADIM_END_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_END)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_END_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_ADIM_START)->SetWindowText(_T(""));
      GetDlgItem(IDC_ADIM_END)->SetWindowText(_T(""));
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CGirderDescGeneralPage::OnUserEci()
{
   BOOL bEnable = ((CButton*)GetDlgItem(IDC_MOD_ECI))->GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);

   if (bEnable==FALSE)
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
      UpdateEci();
   }
   else
   {
      m_ctrlEci.SetWindowText(m_strUserEci);
   }
}

void CGirderDescGeneralPage::OnUserEc()
{
   BOOL bEnable = ((CButton*)GetDlgItem(IDC_MOD_EC))->GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   if (bEnable==FALSE)
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
      UpdateEc();
   }
   else
   {
      m_ctrlEc.SetWindowText(m_strUserEc); 
   }
}

void CGirderDescGeneralPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_GENERAL );
}

void CGirderDescGeneralPage::OnChangeFci() 
{
   UpdateEci();
}

void CGirderDescGeneralPage::OnChangeGirderFc() 
{
   UpdateEc();
}

void CGirderDescGeneralPage::UpdateEci()
{
   // update modulus
   if (m_ctrlEciCheck.GetCheck() == 0)
   {
      // need to manually parse strength and density values
      CString strFci, strDensity, strK1, strK2;
      m_ctrlFci.GetWindowText(strFci);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_GirderData.Material.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_GirderData.Material.EcK1);
      strK2.Format(_T("%f"),pParent->m_GirderData.Material.EcK2);

      CString strEci = CConcreteDetailsDlg::UpdateEc(strFci,strDensity,strK1,strK2);
      m_ctrlEci.SetWindowText(strEci);
   }
}

void CGirderDescGeneralPage::UpdateEc()
{
   // update modulus
   if (m_ctrlEcCheck.GetCheck() == 0)
   {
      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_GirderData.Material.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_GirderData.Material.EcK1);
      strK2.Format(_T("%f"),pParent->m_GirderData.Material.EcK2);

      CString strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}


void CGirderDescGeneralPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConcreteDetailsDlg dlg;

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   dlg.m_Type    = pParent->m_GirderData.Material.Type;
   dlg.m_Fc      = pParent->m_GirderData.Material.Fc;
   dlg.m_AggSize = pParent->m_GirderData.Material.MaxAggregateSize;
   dlg.m_bUserEc = pParent->m_GirderData.Material.bUserEc;
   dlg.m_Ds      = pParent->m_GirderData.Material.StrengthDensity;
   dlg.m_Dw      = pParent->m_GirderData.Material.WeightDensity;
   dlg.m_Ec      = pParent->m_GirderData.Material.Ec;
   dlg.m_EccK1       = pParent->m_GirderData.Material.EcK1;
   dlg.m_EccK2       = pParent->m_GirderData.Material.EcK2;
   dlg.m_CreepK1     = pParent->m_GirderData.Material.CreepK1;
   dlg.m_CreepK2     = pParent->m_GirderData.Material.CreepK2;
   dlg.m_ShrinkageK1 = pParent->m_GirderData.Material.ShrinkageK1;
   dlg.m_ShrinkageK2 = pParent->m_GirderData.Material.ShrinkageK2;
   dlg.m_bHasFct     = pParent->m_GirderData.Material.bHasFct;
   dlg.m_Fct         = pParent->m_GirderData.Material.Fct;

   dlg.m_strUserEc  = m_strUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      pParent->m_GirderData.Material.Type             = dlg.m_Type;
      pParent->m_GirderData.Material.Fc               = dlg.m_Fc;
      pParent->m_GirderData.Material.MaxAggregateSize = dlg.m_AggSize;
      pParent->m_GirderData.Material.bUserEc          = dlg.m_bUserEc;
      pParent->m_GirderData.Material.StrengthDensity  = dlg.m_Ds;
      pParent->m_GirderData.Material.WeightDensity    = dlg.m_Dw;
      pParent->m_GirderData.Material.Ec               = dlg.m_Ec;
      pParent->m_GirderData.Material.EcK1             = dlg.m_EccK1;
      pParent->m_GirderData.Material.EcK2             = dlg.m_EccK2;
      pParent->m_GirderData.Material.CreepK1          = dlg.m_CreepK1;
      pParent->m_GirderData.Material.CreepK2          = dlg.m_CreepK2;
      pParent->m_GirderData.Material.ShrinkageK1      = dlg.m_ShrinkageK1;
      pParent->m_GirderData.Material.ShrinkageK2      = dlg.m_ShrinkageK2;
      pParent->m_GirderData.Material.bHasFct          = dlg.m_bHasFct;
      pParent->m_GirderData.Material.Fct              = dlg.m_Fct;

      m_strUserEc  = dlg.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      UpdateConcreteControls();
   }
	
}

void CGirderDescGeneralPage::UpdateConcreteControls()
{
   // the concrete details were updated in the details dialog
   // update f'c and Ec in this dialog
   CDataExchange dx(this,FALSE);
   ExchangeConcreteData(&dx);
   UpdateEc();
   UpdateEci();

   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}


BOOL CGirderDescGeneralPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_MORE:
         UpdateConcreteParametersToolTip();
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}

void CGirderDescGeneralPage::UpdateConcreteParametersToolTip()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtStressData&  stress  = pDisplayUnits->GetStressUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();


   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), matConcrete::GetTypeName((matConcrete::Type)pParent->m_GirderData.Material.Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pParent->m_GirderData.Material.StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pParent->m_GirderData.Material.WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pParent->m_GirderData.Material.MaxAggregateSize,aggsize)
      );

   //if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   //{
   //   // add K1 parameter
   //   CString strK1;
   //   strK1.Format(_T("\r\n%-20s %s"),
   //      _T("K1"),FormatScalar(pParent->m_GirderData.Material.K1,scalar));

   //   strTip += strK1;
   //}

   if ( pParent->m_GirderData.Material.Type != pgsTypes::Normal && pParent->m_GirderData.Material.bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pParent->m_GirderData.Material.Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   m_strTip = strTip;
}


void CGirderDescGeneralPage::FillGirderComboBox()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   SpanIndexType spanIdx = pParent->m_CurrentSpanIdx;
   GirderIndexType gdrIdx  = pParent->m_CurrentGirderIdx;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strGirderFamilyName = pBridgeDesc->GetGirderFamilyName();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;
   
   pLibNames->EnumGirderNames(strGirderFamilyName.c_str(), &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::_tstring& name = *iter;

      pCB->AddString( name.c_str() );
   }

   UpdateGirderTypeControls();
}

HBRUSH CGirderDescGeneralPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_GIRDERNAME_NOTE:
   case IDC_SLABOFFSET_NOTE:
      pDC->SetTextColor(HYPERLINK_COLOR);
      break;
   };

   return hbr;
}

LRESULT CGirderDescGeneralPage::OnChangeSameGirderType(WPARAM wParam,LPARAM lParam)
{
   m_bUseSameGirderType = !m_bUseSameGirderType;
   UpdateGirderTypeHyperLink();
   UpdateGirderTypeControls();

   return 0;
}

void CGirderDescGeneralPage::UpdateGirderTypeControls()
{
   GetDlgItem(IDC_GIRDER_NAME)->EnableWindow( m_bUseSameGirderType ? FALSE : TRUE );
}

LRESULT CGirderDescGeneralPage::OnChangeSlabOffsetType(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTypes::SlabOffsetType temp = m_SlabOffsetType;
   m_SlabOffsetType = m_SlabOffsetTypeCache;
   m_SlabOffsetTypeCache = temp;

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   CWnd* pwndStart = GetDlgItem(IDC_ADIM_START);
   CWnd* pwndEnd   = GetDlgItem(IDC_ADIM_END);
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      // going into girder by girder slab offset mode
      CString strTempStart = m_strSlabOffsetCache[pgsTypes::metStart];
      CString strTempEnd   = m_strSlabOffsetCache[pgsTypes::metEnd];

      pwndStart->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      pwndEnd->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      pwndStart->SetWindowText(strTempStart);
      pwndEnd->SetWindowText(strTempEnd);
   }
   else if ( m_SlabOffsetType == pgsTypes::sotSpan )
   {
      //pwndStart->SetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      //pwndEnd->SetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);
   }
   else
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 slabOffset[2];
      CDataExchange dx(this,TRUE);
      DDX_UnitValueAndTag(&dx, IDC_ADIM_START,IDC_ADIM_START_UNIT, slabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_ADIM_END,  IDC_ADIM_END_UNIT,   slabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit());

      Float64 slab_offset = slabOffset[pgsTypes::metStart];

      if ( !IsEqual(slabOffset[pgsTypes::metStart],slabOffset[pgsTypes::metEnd]) )
      {
         // going to a single slab offset for the entire bridge, but the current start and end are different
         // make the user choose one
         CSelectItemDlg dlg;
         dlg.m_ItemIdx = 0;
         dlg.m_strTitle = _T("Select Slab Offset");
         dlg.m_strLabel = _T("A single slab offset will be used for the entire bridge. Select a value.");
         
         CString strItems;
         strItems.Format(_T("Start of Span (%s)\nEnd of Span (%s)"),
                         ::FormatDimension(slabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit()),
                         ::FormatDimension(slabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit()));

         dlg.m_strItems = strItems;
         if ( dlg.DoModal() == IDOK )
         {
            if ( dlg.m_ItemIdx == 0 )
               slab_offset = slabOffset[pgsTypes::metStart];
            else
               slab_offset = slabOffset[pgsTypes::metEnd];
         }
         else
         {
            return 0;
         }
      }

      GetDlgItem(IDC_ADIM_START)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      GetDlgItem(IDC_ADIM_END)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      GetDlgItem(IDC_ADIM_START)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
      GetDlgItem(IDC_ADIM_END)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
   }

   return 0;
}

void CGirderDescGeneralPage::UpdateSlabOffsetControls()
{
   // Enable/Disable Slab Offset controls
   BOOL bEnable = (m_SlabOffsetType == pgsTypes::sotGirder ? TRUE : FALSE);

   GetDlgItem(IDC_ADIM_START_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_START)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_START_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_ADIM_END_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_END)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_END_UNIT)->EnableWindow(bEnable);
}

void CGirderDescGeneralPage::UpdateGirderTypeHyperLink()
{
   if ( m_bUseSameGirderType )
   {
      // girder name is shared with the entire bridge
      m_GirderTypeHyperLink.SetWindowText(_T("This girder is used for the entire bridge"));
      m_GirderTypeHyperLink.SetURL(_T("Click to change the type of this girder"));
   }
   else
   {
      m_GirderTypeHyperLink.SetWindowText(_T("This girder type is assigned to this girder line"));
      m_GirderTypeHyperLink.SetURL(_T("Click to use this girder type for the entire bridge"));
   }
}

void CGirderDescGeneralPage::UpdateSlabOffsetHyperLink()
{
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      // slab offset is by girder
      m_SlabOffsetHyperLink.SetWindowText(_T("Slab Offsets are defined girder by girder"));
      if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for the entire bridge"));
      else
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for this span"));
   }
   else if ( m_SlabOffsetType == pgsTypes::sotBridge )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A single Slab Offset is used for the entire bridge"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define Slab Offsets by girder"));
   }
   else
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A unique Slab Offset is used in each span"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define Slab Offsets by girder"));
   }
}

void CGirderDescGeneralPage::OnChangeGirderName()
{
   CString newName;
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);
   pCB->GetWindowText(newName);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_GirderData.ResetPrestressData();

   // reset stirrups to library
   pParent->m_Shear.m_CurGrdName = newName;
   pParent->m_Shear.RestoreToLibraryDefaults();

   pParent->m_LongRebar.m_CurGrdName = newName;
   pParent->m_LongRebar.RestoreToLibraryDefaults();

}
