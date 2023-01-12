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

// GirderDescGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "PGSuperUnits.h"
#include "GirderDescGeneralPage.h"
#include "Utilities.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include <EAF\EAFDisplayUnits.h>

#include "SelectItemDlg.h"
#include "GirderDescDlg.h"
#include "TimelineEventDlg.h"

#include <PgsExt\ConcreteDetailsDlg.h>
#include <PgsExt\Helpers.h>

#include <Atlddx.h>
#include <system\tokenizer.h>

#include "PGSuperUnits.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LEFT 0
#define RIGHT 1

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage property page

IMPLEMENT_DYNCREATE(CGirderDescGeneralPage, CPropertyPage)

CGirderDescGeneralPage::CGirderDescGeneralPage() : CPropertyPage(CGirderDescGeneralPage::IDD)
{
}

CGirderDescGeneralPage::~CGirderDescGeneralPage()
{
}

void CGirderDescGeneralPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CGirderDescGeneralPage)
   DDX_Control(pDX, IDC_CONSTRUCTION_EVENT, m_cbConstruction);
   DDX_Control(pDX, IDC_ERECTION_EVENT, m_cbErection);

   DDX_Control(pDX, IDC_EC,      m_ctrlEc);
   DDX_Control(pDX, IDC_ECI,     m_ctrlEci);
   DDX_Control(pDX, IDC_MOD_EC,  m_ctrlEcCheck);
   DDX_Control(pDX, IDC_MOD_ECI, m_ctrlEciCheck);
   DDX_Control(pDX, IDC_GIRDER_FC, m_ctrlFc);
   DDX_Control(pDX, IDC_FCI, m_ctrlFci);
   DDX_Control(pDX, IDC_TOP_FLANGE_THICKENING, m_ctrlTopFlangeThickening);
   //}}AFX_DATA_MAP

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,pParent->m_strGirderName);

   // concrete material
   ExchangeConcreteData(pDX);

   DDX_UnitValueAndTag( pDX, IDC_FCI, IDC_FCI_UNIT, pParent->m_pSegment->Material.Concrete.Fci, pDisplayUnits->GetStressUnit() );
   // Validation: 0 < f'ci <= f'c   
   DDV_UnitValueLimitOrLess( pDX, IDC_FCI, pParent->m_pSegment->Material.Concrete.Fci, pParent->m_pSegment->Material.Concrete.Fc, pDisplayUnits->GetStressUnit() );

   // Slab Offset
   DDX_Tag(pDX, IDC_START_SLAB_OFFSET_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_END_SLAB_OFFSET_UNIT,   pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_ASSUMED_EXCESS_CAMBER_UNIT,   pDisplayUnits->GetComponentDimUnit() );

   GET_IFACE2(pBroker,IBridgeDescription,pIBridge);
   const CBridgeDescription2* pBridgeDesc = pIBridge->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();
   if ( deckType != pgsTypes::sdtNone )
   {
      DDX_CBItemData(pDX, IDC_SLAB_OFFSET_TYPE, m_SlabOffsetType);
      DDX_UnitValueAndTag( pDX, IDC_START_SLAB_OFFSET, IDC_START_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit() );
      DDX_UnitValueAndTag( pDX, IDC_END_SLAB_OFFSET,   IDC_END_SLAB_OFFSET_UNIT,   m_SlabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit() );

      // validate slab offset... (must be greater or equal gross deck thickess)
      if ( pDX->m_bSaveAndValidate && m_SlabOffsetType == pgsTypes::sotSegment)
      {
         Float64 minSlabOffset = pBridgeDesc->GetMinSlabOffset();

         if (::IsLT(m_SlabOffset[pgsTypes::metStart], minSlabOffset))
         {
            pDX->PrepareEditCtrl(IDC_START_SLAB_OFFSET);
            CString msg;
            msg.Format(_T("The slab offset at the start of the girder must be at least equal to the slab depth of %s"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            pDX->Fail();
         }

         if (::IsLT(m_SlabOffset[pgsTypes::metEnd], minSlabOffset))
         {
            pDX->PrepareEditCtrl(IDC_END_SLAB_OFFSET);
            CString msg;
            msg.Format(_T("The slab offset at the end of the girder must be at least equal to the slab depth of %s"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            pDX->Fail();
         }
      }
   }

   pgsTypes::TopWidthType topWidthType;
   Float64 leftStart, rightStart, leftEnd, rightEnd;
   pParent->m_Girder.GetTopWidth(&topWidthType, &leftStart, &rightStart, &leftEnd, &rightEnd);
   DDX_CBItemData(pDX, IDC_TOP_WIDTH_TYPE, topWidthType);
   DDX_UnitValueAndTag(pDX, IDC_LEFT_TOP_WIDTH_START, IDC_LEFT_TOP_WIDTH_START_UNIT, leftStart, pDisplayUnits->GetXSectionDimUnit());
   DDX_UnitValueAndTag(pDX, IDC_RIGHT_TOP_WIDTH_START, IDC_RIGHT_TOP_WIDTH_START_UNIT, rightStart, pDisplayUnits->GetXSectionDimUnit());
   DDX_UnitValueAndTag(pDX, IDC_LEFT_TOP_WIDTH_END, IDC_LEFT_TOP_WIDTH_END_UNIT, leftEnd, pDisplayUnits->GetXSectionDimUnit());
   DDX_UnitValueAndTag(pDX, IDC_RIGHT_TOP_WIDTH_END, IDC_RIGHT_TOP_WIDTH_END_UNIT, rightEnd, pDisplayUnits->GetXSectionDimUnit());

   CComPtr<IBeamFactory> factory;
   pParent->m_Girder.GetGirderLibraryEntry()->GetBeamFactory(&factory);

   if (pDX->m_bSaveAndValidate && IsTopWidthSpacing(pParent->m_GirderSpacingType))
   {
      if (topWidthType == pgsTypes::twtAsymmetric)
      {
         DDV_UnitValueLimitOrMore(pDX, IDC_LEFT_TOP_WIDTH_START, leftStart, m_MinTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
         DDV_UnitValueLimitOrLess(pDX, IDC_LEFT_TOP_WIDTH_START, leftStart, m_MaxTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());

         DDV_UnitValueLimitOrMore(pDX, IDC_RIGHT_TOP_WIDTH_START, rightStart, m_MinTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit());
         DDV_UnitValueLimitOrLess(pDX, IDC_RIGHT_TOP_WIDTH_START, rightStart, m_MaxTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit());

         if (factory->CanTopWidthVary())
         {
            DDV_UnitValueLimitOrMore(pDX, IDC_LEFT_TOP_WIDTH_END, leftStart, m_MinTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
            DDV_UnitValueLimitOrLess(pDX, IDC_LEFT_TOP_WIDTH_END, leftStart, m_MaxTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());

            DDV_UnitValueLimitOrMore(pDX, IDC_RIGHT_TOP_WIDTH_END, rightStart, m_MinTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit());
            DDV_UnitValueLimitOrLess(pDX, IDC_RIGHT_TOP_WIDTH_END, rightStart, m_MaxTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit());
         }
      }
      else
      {
         DDV_UnitValueLimitOrMore(pDX, IDC_LEFT_TOP_WIDTH_START, leftStart, m_MinTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
         DDV_UnitValueLimitOrLess(pDX, IDC_LEFT_TOP_WIDTH_START, leftStart, m_MaxTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());

         if (factory->CanTopWidthVary())
         {
            DDV_UnitValueLimitOrMore(pDX, IDC_LEFT_TOP_WIDTH_END, leftEnd, m_MinTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
            DDV_UnitValueLimitOrLess(pDX, IDC_LEFT_TOP_WIDTH_END, leftEnd, m_MaxTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
         }
      }
      pParent->m_Girder.SetTopWidth(topWidthType,leftStart,rightStart,leftEnd,rightEnd);
   }

   if (factory->HasTopFlangeThickening())
   {
      DDX_CBItemData(pDX, IDC_TOP_FLANGE_THICKENING_TYPE, pParent->m_pSegment->TopFlangeThickeningType);
      DDX_UnitValueAndTag(pDX, IDC_TOP_FLANGE_THICKENING, IDC_TOP_FLANGE_THICKENING_UNIT, pParent->m_pSegment->TopFlangeThickening, pDisplayUnits->GetComponentDimUnit());
      if (pParent->m_pSegment->TopFlangeThickeningType != pgsTypes::tftNone)
      {
         DDV_UnitValueZeroOrMore(pDX, IDC_TOP_FLANGE_THICKENING, pParent->m_pSegment->TopFlangeThickening, pDisplayUnits->GetComponentDimUnit());
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_PRECAMBER, IDC_PRECAMBER_UNIT, pParent->m_pSegment->Precamber, pDisplayUnits->GetComponentDimUnit());
   if (pDX->m_bSaveAndValidate)
   {
      // precamber and tts are defined on different tabs.
      // we can change precamber without changing TTS and this will result in incompatible input
      if (!IsZero(pParent->m_pSegment->Precamber) && pParent->m_pSegment->Strands.GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned && pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary) != 0)
      {
         if (AfxMessageBox(_T("Temporary strands must be post-tensioned when girders are precambered.\nWould you like to post-tension temporary strands immediately before lifting?"), MB_YESNO) == IDYES)
         {
            pParent->m_pSegment->Strands.SetTemporaryStrandUsage(pgsTypes::ttsPTBeforeLifting);
         }
         else
         {
            pDX->PrepareCtrl(IDC_PRECAMBER);
            pDX->Fail();
         }
      }
   }

   if (m_bCanAssumedExcessCamberInputBeEnabled)
   {
      DDX_CBItemData(pDX, IDC_ASSUMED_EXCESS_CAMBER_TYPES, m_AssumedExcessCamberType);
      DDX_UnitValueAndTag(pDX, IDC_ASSUMED_EXCESS_CAMBER, IDC_ASSUMED_EXCESS_CAMBER_UNIT, m_AssumedExcessCamber, pDisplayUnits->GetComponentDimUnit());
   }

   DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, pParent->m_ConditionFactorType);
   DDX_Text(pDX,   IDC_CONDITION_FACTOR,      pParent->m_ConditionFactor);

   if (pDX->m_bSaveAndValidate && pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent && !pBridgeDesc->UseSameGirderForEntireBridge())
   {
      // the width of this girder must be compatible with other girders in this span
      if ( !pIBridge->IsCompatibleGirder(pParent->m_SegmentKey,pParent->m_strGirderName.c_str()))
      {
         CString strMsg;
         strMsg.Format(_T("%s does not have compatible dimensions with the other girders in this span."), pParent->m_strGirderName.c_str());
         AfxMessageBox(strMsg);
         pDX->Fail();
      }
   }
}

void CGirderDescGeneralPage::ExchangeConcreteData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int value;
      if ( !pDX->m_bSaveAndValidate )
      {
         value = pParent->m_pSegment->Material.Concrete.bBasePropertiesOnInitialValues ? 0 : 1;
      }
      
      DDX_Radio(pDX,IDC_FC1,value);

      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Material.Concrete.bBasePropertiesOnInitialValues = (value == 0 ? true : false);
      }
   }

   DDX_UnitValueAndTag( pDX, IDC_GIRDER_FC,  IDC_GIRDER_FC_UNIT,   pParent->m_pSegment->Material.Concrete.Fc , pDisplayUnits->GetStressUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_GIRDER_FC,pParent->m_pSegment->Material.Concrete.Fc, pDisplayUnits->GetStressUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_ECI, pParent->m_pSegment->Material.Concrete.bUserEci);
   DDX_UnitValueAndTag( pDX, IDC_ECI,  IDC_ECI_UNIT,   pParent->m_pSegment->Material.Concrete.Eci , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_ECI,pParent->m_pSegment->Material.Concrete.Eci, pDisplayUnits->GetModEUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_EC,  pParent->m_pSegment->Material.Concrete.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_EC,  IDC_EC_UNIT, pParent->m_pSegment->Material.Concrete.Ec , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_EC, pParent->m_pSegment->Material.Concrete.Ec, pDisplayUnits->GetModEUnit() );

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_ctrlEciCheck.GetCheck() == 1 )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }
}

BEGIN_MESSAGE_MAP(CGirderDescGeneralPage, CPropertyPage)
   //{{AFX_MSG_MAP(CGirderDescGeneralPage)
   ON_BN_CLICKED(IDC_MOD_ECI, OnUserEci)
   ON_BN_CLICKED(IDC_MOD_EC, OnUserEc)
   ON_COMMAND(ID_HELP, OnHelp)
   ON_EN_CHANGE(IDC_FCI, OnChangeFci)
   ON_EN_CHANGE(IDC_GIRDER_FC, OnChangeGirderFc)
   ON_EN_CHANGE(IDC_ECI, OnChangeEci)
   ON_EN_CHANGE(IDC_EC, OnChangeEc)
   ON_BN_CLICKED(IDC_MORE, OnMoreConcreteProperties)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_CBN_SELCHANGE(IDC_GIRDER_NAMEUSE,OnChangeSameGirderType)
   ON_CBN_SELCHANGE(IDC_SLAB_OFFSET_TYPE,OnChangeSlabOffsetType)
   ON_CBN_SELCHANGE(IDC_ASSUMED_EXCESS_CAMBER_TYPES,OnChangeAssumedExcessCamberType)
   ON_CBN_SELCHANGE(IDC_GIRDER_NAME,OnChangeGirderName)
   ON_CBN_DROPDOWN(IDC_GIRDER_NAME,OnBeforeChangeGirderName)
   ON_BN_CLICKED(IDC_FC1,OnConcreteStrength)
   ON_BN_CLICKED(IDC_FC2,OnConcreteStrength)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, OnConditionFactorTypeChanged)
   ON_CBN_SELCHANGE(IDC_CONSTRUCTION_EVENT, OnConstructionEventChanged)
   ON_CBN_DROPDOWN(IDC_CONSTRUCTION_EVENT, OnConstructionEventChanging)
   ON_CBN_SELCHANGE(IDC_ERECTION_EVENT, OnErectionEventChanged)
   ON_CBN_DROPDOWN(IDC_ERECTION_EVENT, OnErectionEventChanging)
   ON_CBN_SELCHANGE(IDC_TOP_WIDTH_TYPE, OnTopWidthTypeChanged)
   //}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_TOP_FLANGE_THICKENING_TYPE, &CGirderDescGeneralPage::OnTopFlangeThickeningTypeChanged)
   ON_STN_CLICKED(IDC_PRECAMBER_LABEL, &CGirderDescGeneralPage::OnStnClickedPrecamberLabel)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage message handlers

BOOL CGirderDescGeneralPage::OnInitDialog() 
{
   FillGirderComboBox();
   FillEventList();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   m_LossMethod = pLossParams->GetLossMethod();
   m_TimeDependentModel = pLossParams->GetTimeDependentModel();

   if ( m_LossMethod != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_CONSTRUCTION_EVENT)->EnableWindow(FALSE);
      GetDlgItem(IDC_ERECTION_EVENT)->EnableWindow(FALSE);
   }

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   // fill top flange thickening type combo box
   CComboBox* pcbTopFlangeThickening = (CComboBox*)GetDlgItem(IDC_TOP_FLANGE_THICKENING_TYPE);
   int idx = pcbTopFlangeThickening->AddString(_T("none"));
   pcbTopFlangeThickening->SetItemData(idx, (DWORD_PTR)pgsTypes::tftNone);

   idx = pcbTopFlangeThickening->AddString(_T("at ends"));
   pcbTopFlangeThickening->SetItemData(idx, (DWORD_PTR)pgsTypes::tftEnds);

   idx = pcbTopFlangeThickening->AddString(_T("at middle"));
   pcbTopFlangeThickening->SetItemData(idx, (DWORD_PTR)pgsTypes::tftMiddle);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   CComPtr<IBeamFactory> factory;
   pParent->m_Girder.GetGirderLibraryEntry()->GetBeamFactory(&factory);

   // fill top width combo box
   CComboBox* pcbTopWidthTypes = (CComboBox*)GetDlgItem(IDC_TOP_WIDTH_TYPE);
   std::vector<pgsTypes::TopWidthType> vTopWidthTypes = factory->GetSupportedTopWidthTypes();
   for (auto type : vTopWidthTypes)
   {
      int idx = pcbTopWidthTypes->AddString(GetTopWidthType(type));
      pcbTopWidthTypes->SetItemData(idx,(DWORD_PTR)type);
   }

   if (IsTopWidthSpacing(pParent->m_GirderSpacingType))
   {
      // if spacing is for the entire bridge, disable the top width controls
      BOOL bEnable = IsBridgeSpacing(pParent->m_GirderSpacingType) ? FALSE : TRUE;
      GetDlgItem(IDC_TOP_WIDTH_LABEL)->EnableWindow(bEnable);
      GetDlgItem(IDC_TOP_WIDTH_TYPE)->EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_LABEL)->EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_START)->EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_START_UNIT)->EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_END)->EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_END_UNIT)->EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_LABEL)->EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_START)->EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_START_UNIT)->EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_END)->EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_END_UNIT)->EnableWindow(bEnable);
      GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH)->EnableWindow(bEnable);
      GetDlgItem(IDC_TOP_WIDTH_START_LABEL)->EnableWindow(bEnable);
      GetDlgItem(IDC_TOP_WIDTH_END_LABEL)->EnableWindow(bEnable);

      if (!factory->CanTopWidthVary())
      {
         // if not a variable top width beam type
         // hide all the end width controls
         GetDlgItem(IDC_TOP_WIDTH_START_LABEL)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_TOP_WIDTH_END_LABEL)->ShowWindow(SW_HIDE);

         GetDlgItem(IDC_LEFT_TOP_WIDTH_END)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_LEFT_TOP_WIDTH_END_UNIT)->ShowWindow(SW_HIDE);

         GetDlgItem(IDC_RIGHT_TOP_WIDTH_START)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_RIGHT_TOP_WIDTH_START_UNIT)->ShowWindow(SW_HIDE);

         GetDlgItem(IDC_RIGHT_TOP_WIDTH_END)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_RIGHT_TOP_WIDTH_END_UNIT)->ShowWindow(SW_HIDE);
      }
   }
   else
   {
      // not a top width spacing type so hide all top width controls
      GetDlgItem(IDC_TOP_WIDTH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_WIDTH_TYPE)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_LEFT_TOP_WIDTH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_START)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_START_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_END)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_LEFT_TOP_WIDTH_END_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_START)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_START_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_END)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_END_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_WIDTH_START_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_WIDTH_END_LABEL)->ShowWindow(SW_HIDE);
   }

   if (factory->HasTopFlangeThickening())
   {
      GetDlgItem(IDC_TOP_FLANGE_THICKENING_LABEL)->EnableWindow(TRUE);
      GetDlgItem(IDC_TOP_FLANGE_THICKENING_TYPE)->EnableWindow(TRUE);
      GetDlgItem(IDC_TOP_FLANGE_THICKENING)->EnableWindow(TRUE);
      GetDlgItem(IDC_TOP_FLANGE_THICKENING_UNIT)->EnableWindow(TRUE);
   }
   else
   {
      GetDlgItem(IDC_TOP_FLANGE_THICKENING_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_FLANGE_THICKENING_TYPE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_FLANGE_THICKENING)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_FLANGE_THICKENING_UNIT)->ShowWindow(SW_HIDE);
   }

   if (!factory->CanPrecamber())
   {
      GetDlgItem(IDC_PRECAMBER_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PRECAMBER)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PRECAMBER_UNIT)->ShowWindow(SW_HIDE);
   }

   if (!IsTopWidthSpacing(pParent->m_GirderSpacingType) && !factory->HasTopFlangeThickening() && !factory->CanPrecamber())
   {
      GetDlgItem(IDC_GIRDER_MODIFIERS_NOTE)->SetWindowText(_T("This girder does not have modifiers"));
   }

   GET_IFACE2(pBroker,ISpecification, pSpec );
   m_bCanAssumedExcessCamberInputBeEnabled = pSpec->IsAssumedExcessCamberInputEnabled();

   if ( m_AssumedExcessCamberType == pgsTypes::aecBridge || m_AssumedExcessCamberType == pgsTypes::aecSpan )
   {
      m_AssumedExcessCamberTypeCache = pgsTypes::aecGirder;
   }
   else
   {
      m_AssumedExcessCamberTypeCache = pgsTypes::aecBridge;
   }

   m_strAssumedExcessCamberCache.Format(_T("%s"), FormatDimension(m_AssumedExcessCamber, pDisplayUnits->GetComponentDimUnit(), false));

   FillAssumedExcessCamberComboBox();


   // since the bridge model isn't accessable from here,
   // slab offset type and slab offset are initialized outside of this dialog
   m_strSlabOffsetCache[pgsTypes::metStart].Format(_T("%s"), FormatDimension(m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit(), false));
   m_strSlabOffsetCache[pgsTypes::metEnd].Format(_T("%s"), FormatDimension(m_SlabOffset[pgsTypes::metEnd], pDisplayUnits->GetComponentDimUnit(), false));

   FillSlabOffsetComboBox();

   CPropertyPage::OnInitDialog();

   m_ctrlTopFlangeThickening.ShowDefaultWhenDisabled(FALSE);
   OnTopFlangeThickeningTypeChanged();

   OnTopWidthTypeChanged();

   // initialize the event combo boxes
   EventIDType constructionEventID = pParent->m_TimelineMgr.GetSegmentConstructionEventID(pParent->m_SegmentID);
   EventIDType erectionEventID = pParent->m_TimelineMgr.GetSegmentErectionEventID(pParent->m_SegmentID);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_CONSTRUCTION_EVENT,constructionEventID);
   DDX_CBItemData(&dx,IDC_ERECTION_EVENT,erectionEventID);

   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      SegmentIDType segmentID = pParent->m_pSegment->GetID();
      ATLASSERT(segmentID != INVALID_ID);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
      EventIndexType eventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(segmentID);
      m_AgeAtRelease = pTimelineMgr->GetEventByIndex(eventIdx)->GetConstructSegmentsActivity().GetTotalCuringDuration();

      // hide regular text label
      GetDlgItem(IDC_FC_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FCI_LABEL)->ShowWindow(SW_HIDE);
   }
   else
   {
      // hide radio buttons
      GetDlgItem(IDC_FC1)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FC2)->ShowWindow(SW_HIDE);
   }

   OnConditionFactorTypeChanged();

   CComboBox* pcbSameGirderType = (CComboBox*)GetDlgItem(IDC_GIRDER_NAMEUSE);
   pcbSameGirderType->AddString(_T("This girder type is used in all spans"));
   pcbSameGirderType->AddString(_T("This girder type is assigned to this girder"));
   pcbSameGirderType->SetCurSel(m_bUseSameGirderType ? 0:1);
   UpdateGirderTypeControls();


   if ( m_strUserEc == _T("") )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
   
   if ( m_strUserEci == _T("") )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }

   OnUserEci();
   OnUserEc();

   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      if ( pParent->m_pSegment->Material.Concrete.bBasePropertiesOnInitialValues )
      {
         OnChangeFci();
      }
      else
      {
         OnChangeGirderFc();
      }
   }

   UpdateConcreteControls(true);

   EnableToolTips(TRUE);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   if (pIBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtNone)
   {
      // disable slab offset input if there isn't a deck
      GetDlgItem(IDC_SLAB_OFFSET_TYPE)->EnableWindow(FALSE);

      GetDlgItem(IDC_SLAB_OFFSET_LABEL)->EnableWindow(FALSE);

      GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_START_SLAB_OFFSET)->EnableWindow(FALSE);
      GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_END_SLAB_OFFSET)->EnableWindow(FALSE);
      GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_START_SLAB_OFFSET)->SetWindowText(_T(""));
      GetDlgItem(IDC_END_SLAB_OFFSET)->SetWindowText(_T(""));
   }

   if (!m_bCanAssumedExcessCamberInputBeEnabled)
   {
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPES)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER)->SetWindowText(_T(""));
   }

   UpdateAssumedExcessCamberControls();
   UpdateSlabOffsetControls();

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
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_GENERAL );
}

void CGirderDescGeneralPage::OnChangeFci() 
{
   UpdateEci();
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      UpdateFc();
      UpdateEc();
   }
}

void CGirderDescGeneralPage::OnChangeGirderFc() 
{
   UpdateEc();
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      UpdateFci();
      UpdateEci();
   }
}

void CGirderDescGeneralPage::OnChangeEc() 
{
   if ( m_LossMethod == pgsTypes::TIME_STEP && m_ctrlEcCheck.GetCheck() == TRUE)
   {
      UpdateEci();
   }
}

void CGirderDescGeneralPage::OnChangeEci() 
{
   if ( m_LossMethod == pgsTypes::TIME_STEP && m_ctrlEciCheck.GetCheck() == TRUE)
   {
      UpdateEc();
   }
}

void CGirderDescGeneralPage::UpdateEci()
{
   int method = 0;
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int nIDC = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected
      if ( nIDC == IDC_FC2 )
      {
         // concrete model is based on f'c
         if ( m_ctrlEcCheck.GetCheck() == TRUE )
         {
            // Ec box is checked... user has input a value for Ec
            // Eci is based on user value for Ec not f'ci
            method = 0;
         }
         else
         {
            // Ec box is not checked... Ec is computed from f'c, compute Eci from f'ci
            method = 1;
         }
      }
      else
      {
         if ( m_ctrlEciCheck.GetCheck() == FALSE )// not checked
         {
            method = 1;
         }
         else
         {
            method = -1; // don't compute... it has user input
         }
      }
   }
   else
   {
      if ( m_ctrlEciCheck.GetCheck() == FALSE )
      {
         method = 1;
      }
      else
      {
         method = -1;
      }
   }

   if ( method == 0 )
   {
      // Eci is based on the user input value of Ec and not f'ci
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strEc;
      m_ctrlEc.GetWindowText(strEc);
      Float64 Ec;
      WBFL::System::Tokenizer::ParseDouble(strEc,&Ec);
      Ec = WBFL::Units::ConvertToSysUnits(Ec,pDisplayUnits->GetModEUnit().UnitOfMeasure);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      Float64 Eci;
      if ( m_TimeDependentModel == pgsTypes::tdmAASHTO )
      {
         lrfdLRFDTimeDependentConcrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetA(pParent->m_pSegment->Material.Concrete.A);
         concrete.SetBeta(pParent->m_pSegment->Material.Concrete.B);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_pSegment->Material.Concrete.Fc);
         concrete.SetStrengthDensity(pParent->m_pSegment->Material.Concrete.StrengthDensity);
         Eci = concrete.GetEc(m_AgeAtRelease);
      }
      else if ( m_TimeDependentModel == pgsTypes::tdmACI209 )
      {
         WBFL::Materials::ACI209Concrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetA(pParent->m_pSegment->Material.Concrete.A);
         concrete.SetBeta(pParent->m_pSegment->Material.Concrete.B);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_pSegment->Material.Concrete.Fc);
         concrete.SetStrengthDensity(pParent->m_pSegment->Material.Concrete.StrengthDensity);
         Eci = concrete.GetEc(m_AgeAtRelease);
      }
      else
      {
         ATLASSERT(m_TimeDependentModel == pgsTypes::tdmCEBFIP);
         WBFL::Materials::CEBFIPConcrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_pSegment->Material.Concrete.Fc);
         concrete.SetStrengthDensity(pParent->m_pSegment->Material.Concrete.StrengthDensity);
         concrete.SetS(pParent->m_pSegment->Material.Concrete.S);
         concrete.SetBetaSc(pParent->m_pSegment->Material.Concrete.BetaSc);
         Eci = concrete.GetEc(m_AgeAtRelease);
      }

      CString strEci;
      strEci.Format(_T("%s"),FormatDimension(Eci,pDisplayUnits->GetModEUnit(),false));
      m_ctrlEci.SetWindowText(strEci);
   }
   else if ( method == 1)
   {
      // need to manually parse strength and density values
      CString strFci, strDensity, strK1, strK2;
      m_ctrlFci.GetWindowText(strFci);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_pSegment->Material.Concrete.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK1);
      strK2.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK2);

      CString strEci = CConcreteDetailsDlg::UpdateEc(pParent->m_pSegment->Material.Concrete.Type,strFci,strDensity,strK1,strK2);
      m_ctrlEci.SetWindowText(strEci);
   }
}

void CGirderDescGeneralPage::UpdateEc()
{
   int method = 0;
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int nIDC = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected

      if ( nIDC == IDC_FC1 )
      {
         // concrete model is based on f'ci
         if ( m_ctrlEciCheck.GetCheck() == TRUE )
         {
            // Eci box is checked... user has input a value for Eci
            // Ec is based on the user input value of Eci and not f'c
            method = 0;
         }
         else
         {
            // Eci box is not checked... Eci is computed from f'ci, compute Ec from f'c
            method = 1;
         }
      }
      else
      {
         if (m_ctrlEcCheck.GetCheck() == FALSE) // not checked
         {
            method = 1;
         }
         else
         {
            method = -1; // don't compute.... it is user input
         }
      }
   }
   else
   {
      if ( m_ctrlEcCheck.GetCheck() == FALSE )
      {
         method = 1;
      }
      else
      {
         method = -1;
      }
   }

   if ( method == 0 )
   {
      // Ec is based on the user input value of Eci and not f'c
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strEci;
      m_ctrlEci.GetWindowText(strEci);
      Float64 Eci;
      WBFL::System::Tokenizer::ParseDouble(strEci,&Eci);
      Eci = WBFL::Units::ConvertToSysUnits(Eci,pDisplayUnits->GetModEUnit().UnitOfMeasure);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      Float64 Ec;
      if ( m_TimeDependentModel == pgsTypes::tdmAASHTO )
      {
         Ec = lrfdLRFDTimeDependentConcrete::ComputeEc28(Eci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.A,pParent->m_pSegment->Material.Concrete.B);
      }
      else if (m_TimeDependentModel == pgsTypes::tdmACI209 )
      {
         Ec = WBFL::Materials::ACI209Concrete::ComputeEc28(Eci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.A,pParent->m_pSegment->Material.Concrete.B);
      }
      else
      {
         ATLASSERT( m_TimeDependentModel == pgsTypes::tdmCEBFIP );
         Ec = WBFL::Materials::CEBFIPConcrete::ComputeEc28(Eci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.S);
      }

      CString strEc;
      strEc.Format(_T("%s"),FormatDimension(Ec,pDisplayUnits->GetModEUnit(),false));
      m_ctrlEc.SetWindowText(strEc);
   }
   else if (method == 1)
   {
      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_pSegment->Material.Concrete.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK1);
      strK2.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK2);

      CString strEc = CConcreteDetailsDlg::UpdateEc(pParent->m_pSegment->Material.Concrete.Type,strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CGirderDescGeneralPage::UpdateFc()
{
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int nIDC = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected
      if ( nIDC == IDC_FC1 )
      {
         // concrete model is based on f'ci... compute f'c
         // Get f'ci from edit control
         CString strFci;
         m_ctrlFci.GetWindowText(strFci);

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         Float64 fci;
         WBFL::System::Tokenizer::ParseDouble(strFci, &fci);
         fci = WBFL::Units::ConvertToSysUnits(fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);

         CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
         Float64 fc;

         if ( m_TimeDependentModel == pgsTypes::tdmAASHTO )
         {
            fc = lrfdLRFDTimeDependentConcrete::ComputeFc28(fci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.A,pParent->m_pSegment->Material.Concrete.B);
         }
         else if ( m_TimeDependentModel == pgsTypes::tdmACI209 )
         {
            fc = WBFL::Materials::ACI209Concrete::ComputeFc28(fci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.A,pParent->m_pSegment->Material.Concrete.B);
         }
         else
         {
            ATLASSERT(m_TimeDependentModel == pgsTypes::tdmCEBFIP);
            fc = WBFL::Materials::CEBFIPConcrete::ComputeFc28(fci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.S);
         }

         CString strFc;
         strFc.Format(_T("%s"),FormatDimension(fc,pDisplayUnits->GetStressUnit(),false));
         m_ctrlFc.SetWindowText(strFc);
      }
   }
}

void CGirderDescGeneralPage::UpdateFci()
{
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int nIDC = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected
      if ( nIDC == IDC_FC2 )
      {
         // concrete model is based on f'ci... compute f'c
         // Get f'c from edit control
         CString strFc;
         m_ctrlFc.GetWindowText(strFc);

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         Float64 fc;
         WBFL::System::Tokenizer::ParseDouble(strFc, &fc);
         fc = WBFL::Units::ConvertToSysUnits(fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);

         CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

         Float64 fci;
         if ( m_TimeDependentModel == pgsTypes::tdmAASHTO )
         {
            lrfdLRFDTimeDependentConcrete concrete;
            concrete.SetTimeAtCasting(0);
            concrete.SetFc28(fc);
            concrete.SetA(pParent->m_pSegment->Material.Concrete.A);
            concrete.SetBeta(pParent->m_pSegment->Material.Concrete.B);
            fci = concrete.GetFc(m_AgeAtRelease);
         }
         else if ( m_TimeDependentModel == pgsTypes::tdmACI209 )
         {
            WBFL::Materials::ACI209Concrete concrete;
            concrete.SetTimeAtCasting(0);
            concrete.SetFc28(fc);
            concrete.SetA(pParent->m_pSegment->Material.Concrete.A);
            concrete.SetBeta(pParent->m_pSegment->Material.Concrete.B);
            fci = concrete.GetFc(m_AgeAtRelease);
         }
         else
         {
            ATLASSERT(m_TimeDependentModel == pgsTypes::tdmCEBFIP);
            WBFL::Materials::CEBFIPConcrete concrete;
            concrete.SetTimeAtCasting(0);
            concrete.SetFc28(fc);
            concrete.SetS(pParent->m_pSegment->Material.Concrete.S);
            concrete.SetBetaSc(pParent->m_pSegment->Material.Concrete.BetaSc);
            fci = concrete.GetFc(m_AgeAtRelease);
         }

         CString strFci;
         strFci.Format(_T("%s"),FormatDimension(fci,pDisplayUnits->GetStressUnit(),false));
         m_ctrlFci.SetWindowText(strFci);
      }
   }
}

void CGirderDescGeneralPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConcreteDetailsDlg dlg(true/*f'c*/);

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();


   EventIDType constructionEventID = pParent->m_TimelineMgr.GetSegmentConstructionEventID(pParent->m_pSegment->GetID());
   Float64 ageAtRelease = pParent->m_TimelineMgr.GetEventByID(constructionEventID)->GetConstructSegmentsActivity().GetTotalCuringDuration();
   dlg.m_TimeAtInitialStrength = WBFL::Units::ConvertToSysUnits(ageAtRelease,WBFL::Units::Measure::Day);

   dlg.m_fci = pParent->m_pSegment->Material.Concrete.Fci;
   dlg.m_Eci = pParent->m_pSegment->Material.Concrete.Eci;
   dlg.m_bUserEci = pParent->m_pSegment->Material.Concrete.bUserEci;

   dlg.m_fc28 = pParent->m_pSegment->Material.Concrete.Fc;
   dlg.m_Ec28 = pParent->m_pSegment->Material.Concrete.Ec;
   dlg.m_bUserEc28 = pParent->m_pSegment->Material.Concrete.bUserEc;

   dlg.m_General.m_Type        = pParent->m_pSegment->Material.Concrete.Type;
   dlg.m_General.m_AggSize     = pParent->m_pSegment->Material.Concrete.MaxAggregateSize;
   dlg.m_General.m_Ds          = pParent->m_pSegment->Material.Concrete.StrengthDensity;
   dlg.m_General.m_Dw          = pParent->m_pSegment->Material.Concrete.WeightDensity;

   dlg.m_PCIUHPC.m_ffc = pParent->m_pSegment->Material.Concrete.Ffc;
   dlg.m_PCIUHPC.m_frr = pParent->m_pSegment->Material.Concrete.Frr;
   dlg.m_PCIUHPC.m_FiberLength = pParent->m_pSegment->Material.Concrete.FiberLength;
   dlg.m_PCIUHPC.m_AutogenousShrinkage = pParent->m_pSegment->Material.Concrete.AutogenousShrinkage;
   dlg.m_PCIUHPC.m_bPCTT = pParent->m_pSegment->Material.Concrete.bPCTT;

   dlg.m_FHWAUHPC.m_ftcri = pParent->m_pSegment->Material.Concrete.ftcri;
   dlg.m_FHWAUHPC.m_ftcr = pParent->m_pSegment->Material.Concrete.ftcr;
   dlg.m_FHWAUHPC.m_ftloc = pParent->m_pSegment->Material.Concrete.ftloc;
   dlg.m_FHWAUHPC.m_etloc = pParent->m_pSegment->Material.Concrete.etloc;
   dlg.m_FHWAUHPC.m_alpha_u = pParent->m_pSegment->Material.Concrete.alpha_u;
   dlg.m_FHWAUHPC.m_ecu = pParent->m_pSegment->Material.Concrete.ecu;
   dlg.m_FHWAUHPC.m_bExperimental_ecu = pParent->m_pSegment->Material.Concrete.bExperimental_ecu;
   dlg.m_FHWAUHPC.m_FiberLength = pParent->m_pSegment->Material.Concrete.FiberLength;

   dlg.m_AASHTO.m_EccK1       = pParent->m_pSegment->Material.Concrete.EcK1;
   dlg.m_AASHTO.m_EccK2       = pParent->m_pSegment->Material.Concrete.EcK2;
   dlg.m_AASHTO.m_CreepK1     = pParent->m_pSegment->Material.Concrete.CreepK1;
   dlg.m_AASHTO.m_CreepK2     = pParent->m_pSegment->Material.Concrete.CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = pParent->m_pSegment->Material.Concrete.ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = pParent->m_pSegment->Material.Concrete.ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = pParent->m_pSegment->Material.Concrete.bHasFct;
   dlg.m_AASHTO.m_Fct         = pParent->m_pSegment->Material.Concrete.Fct;

   dlg.m_ACI.m_bUserParameters = pParent->m_pSegment->Material.Concrete.bACIUserParameters;
   dlg.m_ACI.m_A               = pParent->m_pSegment->Material.Concrete.A;
   dlg.m_ACI.m_B               = pParent->m_pSegment->Material.Concrete.B;
   dlg.m_ACI.m_CureMethod      = pParent->m_pSegment->Material.Concrete.CureMethod;
   dlg.m_ACI.m_CementType      = pParent->m_pSegment->Material.Concrete.ACI209CementType;

   dlg.m_CEBFIP.m_bUserParameters = pParent->m_pSegment->Material.Concrete.bCEBFIPUserParameters;
   dlg.m_CEBFIP.m_S               = pParent->m_pSegment->Material.Concrete.S;
   dlg.m_CEBFIP.m_BetaSc          = pParent->m_pSegment->Material.Concrete.BetaSc;
   dlg.m_CEBFIP.m_CementType      = pParent->m_pSegment->Material.Concrete.CEBFIPCementType;

   dlg.m_General.m_strUserEc  = m_strUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      pParent->m_pSegment->Material.Concrete.Fc               = dlg.m_fc28;
      pParent->m_pSegment->Material.Concrete.Ec               = dlg.m_Ec28;
      pParent->m_pSegment->Material.Concrete.bUserEc          = dlg.m_bUserEc28;

      pParent->m_pSegment->Material.Concrete.Type             = dlg.m_General.m_Type;
      pParent->m_pSegment->Material.Concrete.MaxAggregateSize = dlg.m_General.m_AggSize;
      pParent->m_pSegment->Material.Concrete.StrengthDensity  = dlg.m_General.m_Ds;
      pParent->m_pSegment->Material.Concrete.WeightDensity    = dlg.m_General.m_Dw;

      pParent->m_pSegment->Material.Concrete.Ffc = dlg.m_PCIUHPC.m_ffc;
      pParent->m_pSegment->Material.Concrete.Frr = dlg.m_PCIUHPC.m_frr;
      pParent->m_pSegment->Material.Concrete.FiberLength = dlg.m_PCIUHPC.m_FiberLength;
      pParent->m_pSegment->Material.Concrete.AutogenousShrinkage = dlg.m_PCIUHPC.m_AutogenousShrinkage;
      pParent->m_pSegment->Material.Concrete.bPCTT = dlg.m_PCIUHPC.m_bPCTT;

      pParent->m_pSegment->Material.Concrete.ftcri = dlg.m_FHWAUHPC.m_ftcri;
      pParent->m_pSegment->Material.Concrete.ftcr = dlg.m_FHWAUHPC.m_ftcr;
      pParent->m_pSegment->Material.Concrete.ftloc = dlg.m_FHWAUHPC.m_ftloc;
      pParent->m_pSegment->Material.Concrete.etloc = dlg.m_FHWAUHPC.m_etloc;
      pParent->m_pSegment->Material.Concrete.alpha_u = dlg.m_FHWAUHPC.m_alpha_u;
      pParent->m_pSegment->Material.Concrete.bExperimental_ecu = dlg.m_FHWAUHPC.m_bExperimental_ecu;
      pParent->m_pSegment->Material.Concrete.ecu = dlg.m_FHWAUHPC.m_ecu;
      pParent->m_pSegment->Material.Concrete.FiberLength = dlg.m_FHWAUHPC.m_FiberLength;

      pParent->m_pSegment->Material.Concrete.EcK1             = dlg.m_AASHTO.m_EccK1;
      pParent->m_pSegment->Material.Concrete.EcK2             = dlg.m_AASHTO.m_EccK2;
      pParent->m_pSegment->Material.Concrete.CreepK1          = dlg.m_AASHTO.m_CreepK1;
      pParent->m_pSegment->Material.Concrete.CreepK2          = dlg.m_AASHTO.m_CreepK2;
      pParent->m_pSegment->Material.Concrete.ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      pParent->m_pSegment->Material.Concrete.ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      pParent->m_pSegment->Material.Concrete.bHasFct          = dlg.m_AASHTO.m_bHasFct;
      pParent->m_pSegment->Material.Concrete.Fct              = dlg.m_AASHTO.m_Fct;

      pParent->m_pSegment->Material.Concrete.bACIUserParameters = dlg.m_ACI.m_bUserParameters;
      pParent->m_pSegment->Material.Concrete.A                  = dlg.m_ACI.m_A;
      pParent->m_pSegment->Material.Concrete.B                  = dlg.m_ACI.m_B;
      pParent->m_pSegment->Material.Concrete.CureMethod         = dlg.m_ACI.m_CureMethod;
      pParent->m_pSegment->Material.Concrete.ACI209CementType   = dlg.m_ACI.m_CementType;

      pParent->m_pSegment->Material.Concrete.bCEBFIPUserParameters = dlg.m_CEBFIP.m_bUserParameters;
      pParent->m_pSegment->Material.Concrete.S                     = dlg.m_CEBFIP.m_S;
      pParent->m_pSegment->Material.Concrete.BetaSc                = dlg.m_CEBFIP.m_BetaSc;
      pParent->m_pSegment->Material.Concrete.CEBFIPCementType      = dlg.m_CEBFIP.m_CementType;

      m_strUserEc  = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      dx.m_bSaveAndValidate = FALSE;
      ExchangeConcreteData(&dx);

      UpdateFci();
      UpdateFc();
      UpdateEci();
      UpdateEc();

      UpdateConcreteControls(true);
   }
   
}

void CGirderDescGeneralPage::UpdateConcreteControls(bool bSkipEcCheckBoxes)
{
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int nIDC = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected
      INT idFci[5] = {IDC_FCI,       IDC_FCI_UNIT,       IDC_MOD_ECI, IDC_ECI, IDC_ECI_UNIT};
      INT idFc[5]  = {IDC_GIRDER_FC, IDC_GIRDER_FC_UNIT, IDC_MOD_EC,  IDC_EC,  IDC_EC_UNIT };

      BOOL bEnableFci = (nIDC == IDC_FC1);

      for ( int j = 0; j < 5; j++ )
      {
         GetDlgItem(idFci[j])->EnableWindow(  bEnableFci );
         GetDlgItem(idFc[j] )->EnableWindow( !bEnableFci );
      }

      if ( !bSkipEcCheckBoxes )
      {
         if ( nIDC == IDC_FC1 ) // input based on f'ci
         {
            m_ctrlEciCheck.SetCheck(m_ctrlEcCheck.GetCheck());
            m_ctrlEcCheck.SetCheck(FALSE); // can't check Ec
         }

         if ( nIDC == IDC_FC2 ) // input is based on f'ci
         {
            m_ctrlEcCheck.SetCheck(m_ctrlEciCheck.GetCheck());
            m_ctrlEciCheck.SetCheck(FALSE); // can't check Eci
         }
      }
   }

   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   bEnable = m_ctrlEciCheck.GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);


   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   CString strLabel( ConcreteDescription(pParent->m_pSegment->Material.Concrete));
   GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText( strLabel );
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
      pTTT->hinst = nullptr;
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

   const WBFL::Units::DensityData& density = pDisplayUnits->GetDensityUnit();
   const WBFL::Units::LengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const WBFL::Units::StressData&  stress  = pDisplayUnits->GetStressUnit();
   const WBFL::Units::ScalarData&  scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pParent->m_pSegment->Material.Concrete.Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pParent->m_pSegment->Material.Concrete.StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pParent->m_pSegment->Material.Concrete.WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pParent->m_pSegment->Material.Concrete.MaxAggregateSize,aggsize)
      );

   if ( pParent->m_pSegment->Material.Concrete.Type != pgsTypes::Normal && pParent->m_pSegment->Material.Concrete.bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pParent->m_pSegment->Material.Concrete.Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   m_strTip = strTip;
}


void CGirderDescGeneralPage::FillGirderComboBox()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
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


void CGirderDescGeneralPage::OnChangeSameGirderType()
{
   m_bUseSameGirderType = !m_bUseSameGirderType;
   UpdateGirderTypeControls();
}

void CGirderDescGeneralPage::UpdateGirderTypeControls()
{
   GetDlgItem(IDC_GIRDER_NAME)->EnableWindow( m_bUseSameGirderType ? FALSE : TRUE );
}

pgsTypes::SlabOffsetType CGirderDescGeneralPage::GetCurrentSlabOffsetType()
{
   CComboBox* pcbSlabOffsetType = (CComboBox*)GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   int curSel = pcbSlabOffsetType->GetCurSel();
   return (pgsTypes::SlabOffsetType)pcbSlabOffsetType->GetItemData(curSel);
}

void CGirderDescGeneralPage::OnChangingSlabOffsetType()
{
   m_PrevSlabOffsetType = GetCurrentSlabOffsetType();
}

void CGirderDescGeneralPage::OnChangeSlabOffsetType()
{
   pgsTypes::SlabOffsetType slabOffsetType = GetCurrentSlabOffsetType();
   
   CWnd* pwndStart = GetDlgItem(IDC_START_SLAB_OFFSET);
   CWnd* pwndEnd   = GetDlgItem(IDC_END_SLAB_OFFSET);
   if (slabOffsetType == pgsTypes::sotSegment )
   {
      // going into girder by girder slab offset mode
      CString strTempStart = m_strSlabOffsetCache[pgsTypes::metStart];
      CString strTempEnd   = m_strSlabOffsetCache[pgsTypes::metEnd];

      pwndStart->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      pwndEnd->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      pwndStart->SetWindowText(strTempStart);
      pwndEnd->SetWindowText(strTempEnd);
   }
   else if (slabOffsetType == pgsTypes::sotBearingLine )
   {
      // Do nothing here... the same data for segments is used for bearing lines
   }
   else
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      GET_IFACE2(pBroker, IBridgeDescription, pIBridge);
      const CBridgeDescription2* pBridgeDesc = pIBridge->GetBridgeDescription();
      Float64 minSlabOffset = pBridgeDesc->GetMinSlabOffset();

      std::array<Float64,2> slabOffset;
      CDataExchange dx(this,TRUE);
      DDX_UnitValueAndTag(&dx, IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT, slabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,   slabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit());

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
            slab_offset = slabOffset[dlg.m_ItemIdx == 0 ? pgsTypes::metStart : pgsTypes::metEnd];
         }
         else
         {
            // roll back the edit... nothing is changing
            ComboBoxSelectByItemData(this, IDC_SLAB_OFFSET_TYPE, m_PrevSlabOffsetType);
            return;
         }
      }

      // when we switch to slab offset by bridge, the UI is disabled and you can't change the slab offset value
      // the slab offset must be valid before going on
      if (::IsLT(slab_offset, minSlabOffset))
      {
         CDataExchange dx(this, TRUE);
         dx.PrepareEditCtrl(IDC_START_SLAB_OFFSET);
         CString msg;
         msg.Format(_T("The slab offset must be at least equal to the slab depth of %s"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));
         AfxMessageBox(msg, MB_ICONERROR | MB_OK);

         // roll back the edit... nothing is changing
         ComboBoxSelectByItemData(this, IDC_SLAB_OFFSET_TYPE, m_PrevSlabOffsetType);

         dx.Fail();
      }

      GetDlgItem(IDC_START_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      GetDlgItem(IDC_END_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      GetDlgItem(IDC_START_SLAB_OFFSET)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
      GetDlgItem(IDC_END_SLAB_OFFSET)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
   }

   UpdateSlabOffsetControls();
}

void CGirderDescGeneralPage::OnChangeAssumedExcessCamberType()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTypes::AssumedExcessCamberType temp = m_AssumedExcessCamberType;
   m_AssumedExcessCamberType = m_AssumedExcessCamberTypeCache;
   m_AssumedExcessCamberTypeCache = temp;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CWnd* pwndAssumedExcessCamber = GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER);
   if ( m_AssumedExcessCamberType == pgsTypes::aecGirder )
   {
      // going into girder by girder slab offset mode
      CString strTemp = m_strAssumedExcessCamberCache;

      pwndAssumedExcessCamber->GetWindowText(m_strAssumedExcessCamberCache);
      pwndAssumedExcessCamber->SetWindowText(strTemp);
   }
   else if ( m_AssumedExcessCamberType == pgsTypes::aecSpan )
   {
      //pwndAssumedExcessCamber->SetWindowText(m_strAssumedExcessCamberCache);
   }
   else
   {
      Float64 assumedExcessCamber;
      CDataExchange dx(this,TRUE);
      DDX_UnitValueAndTag(&dx, IDC_ASSUMED_EXCESS_CAMBER,IDC_ASSUMED_EXCESS_CAMBER_UNIT, assumedExcessCamber, pDisplayUnits->GetComponentDimUnit());

      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER)->GetWindowText(m_strAssumedExcessCamberCache);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER)->SetWindowText( ::FormatDimension(assumedExcessCamber,pDisplayUnits->GetComponentDimUnit(),false) );
   }


   m_strAssumedExcessCamberCache.Format(_T("%s"), FormatDimension(m_AssumedExcessCamber, pDisplayUnits->GetComponentDimUnit(), false));
   FillAssumedExcessCamberComboBox();
   CComboBox* pcbAssumedExcessCamberType = (CComboBox*)GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPES);
   pcbAssumedExcessCamberType->SetCurSel(m_AssumedExcessCamberType == pgsTypes::aecGirder ? 1 : 0);

   UpdateAssumedExcessCamberControls();
}

void CGirderDescGeneralPage::UpdateSlabOffsetControls()
{
   // Enable/Disable Slab Offset controls
   pgsTypes::SlabOffsetType slabOffsetType = GetCurrentSlabOffsetType();
   BOOL bEnable = (slabOffsetType == pgsTypes::sotSegment ? TRUE : FALSE);

   GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);
}

void CGirderDescGeneralPage::UpdateAssumedExcessCamberControls()
{
   BOOL bEnable = (m_bCanAssumedExcessCamberInputBeEnabled && m_AssumedExcessCamberType == pgsTypes::aecGirder ? TRUE : FALSE);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER)->EnableWindow(bEnable);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNIT)->EnableWindow(bEnable);
}

void CGirderDescGeneralPage::OnBeforeChangeGirderName()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);
   m_GirderNameIdx = pCB->GetCurSel();
}

void CGirderDescGeneralPage::OnChangeGirderName()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);

   CString newName;
   pCB->GetWindowText(newName);

   // From the girder editing dialog, we don't have a way to change the overall spacing/layout type for the entire
   // bridge. Because of that, we have to make sure the new girder has a compatible spacing type with the other
   // girders. If it doesn't, balk and reset to the previous girder type.
   //
   // An alterative would be to not put girders in the drop down if they don't have compatible spacing. However
   // users will not like it if they can't see their girders. This approach shows the girders to the users and
   // provides and explaination as to why a particular girder can't be used.
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibrary, pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(newName);

   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   if (!factory->IsSupportedBeamSpacing(pParent->m_GirderSpacingType))
   {
      CString strMsg;
      strMsg.Format(_T("The current spacing type is \"%s\".\r\n%s is not compatible with this spacing type."), GetGirderSpacingType(pParent->m_GirderSpacingType, false/*not a spliced girder*/), newName);
      AfxMessageBox(strMsg, MB_ICONINFORMATION | MB_OK);
      pCB->SetCurSel(m_GirderNameIdx);
      return;
   }

   if (!factory->IsSupportedDeckType(deckType, pParent->m_GirderSpacingType))
   {
      CString strMsg;
      strMsg.Format(_T("The current deck type is \"%s\".\r\n%s is not compatible with this deck type."), GetDeckTypeName(deckType), newName);
      AfxMessageBox(strMsg, MB_ICONINFORMATION | MB_OK);
      pCB->SetCurSel(m_GirderNameIdx);
      return;
   }

   int result = AfxMessageBox(_T("Changing the girder type will reset the strands, stirrups, and longitudinal rebar to default values.\n\nIs that OK?"), MB_YESNO);
   if (result == IDNO)
   {
      pCB->SetCurSel(m_GirderNameIdx);
      return;
   }

   pParent->m_strGirderName = newName;


   // reset prestress data
   pParent->m_pSegment->Strands.ResetPrestressData();

   // Must change adjustable strand type if it is incompatible with library
   pgsTypes::AdjustableStrandType libAdjType = pGdrEntry->GetAdjustableStrandType();
   pgsTypes::AdjustableStrandType adjType = pParent->m_pSegment->Strands.GetAdjustableStrandType();
   if (adjType == pgsTypes::asHarped && libAdjType == pgsTypes::asStraight)
   {
      pParent->m_pSegment->Strands.SetAdjustableStrandType(pgsTypes::asStraight);
   }
   else if (adjType == pgsTypes::asStraight && libAdjType == pgsTypes::asHarped)
   {
      pParent->m_pSegment->Strands.SetAdjustableStrandType(pgsTypes::asHarped);
   }

   // reset stirrups to library
   pParent->m_Shear.m_CurGrdName = newName;
   pParent->m_Shear.DoRestoreDefaults();

   pParent->m_LongRebar.m_CurGrdName = newName;
   pParent->m_LongRebar.RestoreToLibraryDefaults(&(pParent->m_pSegment->LongitudinalRebarData));

   // add/remove property pages if needed
   GET_IFACE2( pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   bool bCanExtendStrands = pSpecEntry->AllowStraightStrandExtensions();
   bool bCanDebond = pGdrEntry->CanDebondStraightStrands();

   pParent->OnGirderTypeChanged(bCanExtendStrands,bCanDebond);
}

void CGirderDescGeneralPage::OnConcreteStrength()
{
   UpdateConcreteControls();
}

void CGirderDescGeneralPage::OnConditionFactorTypeChanged()
{
   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_CONDITION_FACTOR);
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);

   int idx = pcbConditionFactor->GetCurSel();
   switch(idx)
   {
   case 0:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("1.00"));
      break;
   case 1:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.95"));
      break;
   case 2:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.85"));
      break;
   case 3:
      pEdit->EnableWindow(TRUE);
      break;
   }
}

void CGirderDescGeneralPage::FillEventList()
{
   CComboBox* pcbConstruct = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION_EVENT);
   CComboBox* pcbErect = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);

   int constructIdx = pcbConstruct->GetCurSel();
   int erectIdx = pcbErect->GetCurSel();

   EventIDType constructEventID = (EventIDType)pcbConstruct->GetItemData(constructIdx);
   EventIDType erectionEventID  = (EventIDType)pcbConstruct->GetItemData(erectIdx);

   pcbConstruct->ResetContent();
   pcbErect->ResetContent();

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   EventIndexType nEvents = pParent->m_TimelineMgr.GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pParent->m_TimelineMgr.GetEventByIndex(eventIdx);
      EventIDType eventID = pTimelineEvent->GetID();

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      int idx = pcbConstruct->AddString(label);
      pcbConstruct->SetItemData(idx,eventID);
      if ( eventID == constructEventID )
      {
         pcbConstruct->SetCurSel(idx);
      }

      idx = pcbErect->AddString(label);
      pcbErect->SetItemData(idx,eventID);
      if ( eventID == erectionEventID )
      {
         pcbErect->SetCurSel(idx);
      }
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbConstruct->SetItemData(pcbConstruct->AddString(strNewEvent),CREATE_TIMELINE_EVENT);
   pcbErect->SetItemData(pcbErect->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( pcbConstruct->GetCurSel() == CB_ERR )
   {
      pcbConstruct->SetCurSel(0);
   }

   if ( pcbErect->GetCurSel() == CB_ERR )
   {
      pcbErect->SetCurSel(0);
   }
}

void CGirderDescGeneralPage::OnConstructionEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION_EVENT);
   m_PrevConstructionEventIdx = pCB->GetCurSel();
}

void CGirderDescGeneralPage::OnConstructionEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION_EVENT);
   int curSel = pCB->GetCurSel();
   EventIDType eventID = (EventIDType)pCB->GetItemData(curSel);
   if ( eventID == CREATE_TIMELINE_EVENT )
   {
      eventID = CreateEvent();
      if ( eventID == INVALID_ID )
      {
         pCB->SetCurSel(m_PrevConstructionEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_TimelineMgr.SetSegmentConstructionEventByID(pParent->m_SegmentID,eventID);

   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_CONSTRUCTION_EVENT,eventID);
}
   
void CGirderDescGeneralPage::OnErectionEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   m_PrevErectionEventIdx = pCB->GetCurSel();
}

void CGirderDescGeneralPage::OnErectionEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   int curSel = pCB->GetCurSel();
   EventIDType eventID = (EventIDType)pCB->GetItemData(curSel);
   if ( eventID == CREATE_TIMELINE_EVENT )
   {
      eventID = CreateEvent();
      if ( eventID == INVALID_ID )
      {
         pCB->SetCurSel(m_PrevErectionEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_TimelineMgr.SetSegmentErectionEventByID(pParent->m_SegmentID,eventID);

   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_ERECTION_EVENT,eventID);
}

EventIDType CGirderDescGeneralPage::CreateEvent()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   CTimelineEventDlg dlg(pParent->m_TimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      int result = pParent->m_TimelineMgr.AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      ATLASSERT(result == TLM_SUCCESS);
      EventIDType eventID = pParent->m_TimelineMgr.GetEventByIndex(eventIdx)->GetID();
      return eventID;
  }

   return INVALID_ID;
}

void CGirderDescGeneralPage::OnTopFlangeThickeningTypeChanged()
{
   CComboBox* pcbTopFlangeThickeningType = (CComboBox*)GetDlgItem(IDC_TOP_FLANGE_THICKENING_TYPE);
   int sel = pcbTopFlangeThickeningType->GetCurSel();
   pgsTypes::TopFlangeThickeningType thickeningType = (pgsTypes::TopFlangeThickeningType)(pcbTopFlangeThickeningType->GetItemData(sel));
   m_ctrlTopFlangeThickening.EnableWindow(thickeningType == pgsTypes::tftNone ? FALSE : TRUE);
}

void CGirderDescGeneralPage::OnTopWidthTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TOP_WIDTH_TYPE);
   int curSel = pCB->GetCurSel();
   pgsTypes::TopWidthType topWidthType = (pgsTypes::TopWidthType)(pCB->GetItemData(curSel));
 
   
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   const auto* pGirderLibraryEntry = pParent->m_Girder.GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGirderLibraryEntry->GetDimensions();
   CComPtr<IBeamFactory> factory;
   pGirderLibraryEntry->GetBeamFactory(&factory);
   factory->GetAllowableTopWidthRange(topWidthType, dimensions, &m_MinTopWidth[LEFT], &m_MaxTopWidth[LEFT], &m_MinTopWidth[RIGHT], &m_MaxTopWidth[RIGHT]);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CString strLabel;
   if (topWidthType == pgsTypes::twtAsymmetric)
   {
      strLabel.Format(_T("Allowable\nLeft (%s to %s)\nRight (%s to %s)"),
         FormatDimension(m_MinTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MaxTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MinTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MaxTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit()));
   }
   else
   {
      strLabel.Format(_T("Allowable\n(%s to %s)"),
         FormatDimension(m_MinTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MaxTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()));
   }

   GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH)->SetWindowText(strLabel);
   int nShowCommand = (topWidthType == pgsTypes::twtAsymmetric ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LEFT_TOP_WIDTH_LABEL)->ShowWindow(nShowCommand);
   GetDlgItem(IDC_RIGHT_TOP_WIDTH_LABEL)->ShowWindow(nShowCommand);
   GetDlgItem(IDC_RIGHT_TOP_WIDTH_START)->ShowWindow(nShowCommand);
   GetDlgItem(IDC_RIGHT_TOP_WIDTH_START_UNIT)->ShowWindow(nShowCommand);

   if (factory->CanTopWidthVary())
   {
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_END)->ShowWindow(nShowCommand);
      GetDlgItem(IDC_RIGHT_TOP_WIDTH_END_UNIT)->ShowWindow(nShowCommand);
   }
}

void CGirderDescGeneralPage::FillSlabOffsetComboBox()
{
   CComboBox* pcbSlabOffsetType = (CComboBox*)GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   pcbSlabOffsetType->ResetContent();

   if (m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotSegment)
   {
      int idx = pcbSlabOffsetType->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBridge, TRUE));
      pcbSlabOffsetType->SetItemData(idx, (DWORD_PTR)pgsTypes::sotBridge);
   }
   else
   {
      int idx = pcbSlabOffsetType->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine, TRUE));
      pcbSlabOffsetType->SetItemData(idx, (DWORD_PTR)pgsTypes::sotBearingLine);
   }
   int idx = pcbSlabOffsetType->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotSegment, TRUE));
   pcbSlabOffsetType->SetItemData(idx, (DWORD_PTR)pgsTypes::sotSegment);
}

void CGirderDescGeneralPage::FillAssumedExcessCamberComboBox()
{
   CComboBox* pcbAssumedExcessCamberType = (CComboBox*)GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPES);
   pcbAssumedExcessCamberType->ResetContent();

   if (m_AssumedExcessCamberType == pgsTypes::aecBridge || m_AssumedExcessCamberType == pgsTypes::aecGirder)
   {
      int idx = pcbAssumedExcessCamberType->AddString(_T("A single Assumed Excess Camber is used in all spans"));
      pcbAssumedExcessCamberType->SetItemData(idx, (DWORD_PTR)pgsTypes::aecBridge);
   }
   else
   {
      int idx = pcbAssumedExcessCamberType->AddString(_T("A unique Assumed Excess Camber is used in each span"));
      pcbAssumedExcessCamberType->SetItemData(idx, (DWORD_PTR)pgsTypes::aecSpan);
   }
   int idx = pcbAssumedExcessCamberType->AddString(_T("Assumed Excess Cambers are defined girder by girder"));
   pcbAssumedExcessCamberType->SetItemData(idx, (DWORD_PTR)pgsTypes::aecGirder);
}

void CGirderDescGeneralPage::OnStnClickedPrecamberLabel()
{
   // TODO: Add your control notification handler code here
}
