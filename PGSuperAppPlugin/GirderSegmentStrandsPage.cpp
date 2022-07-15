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

// GirderSegmentStrandsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentStrandsPage.h"
#include "GirderSegmentDlg.h"

#include <GenericBridge\Helpers.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\Intervals.h>

#include <Materials/PsStrand.h>
#include <LRFD\StrandPool.h>

#include "PGSuperColors.h"
#include <PgsExt\DesignConfigUtil.h>

#include "GirderDescDlg.h" // for ReconcileDebonding

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CGirderSegmentStrandsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentStrandsPage, CPropertyPage)

void DDX_UnitValueChoice(CDataExchange* pDX, UINT nIDC, UINT nIDCUnit, Float64& value, const WBFL::Units::LengthData& lengthUnit)
{
   CComboBox* pCB = (CComboBox*)pDX->m_pDlgWnd->GetDlgItem(nIDCUnit);
   if (pDX->m_bSaveAndValidate)
   {
      // value is coming out of the control
      int curSel = pCB->GetCurSel();
      if (curSel == 0)
      {
         // value is a percentage
         Float64 v;
         DDX_Text(pDX, nIDC, v); // this is a number like 50.0 for 50%
         value = -1 * v / 100.; // we want -0.50 to be 50% (less than zero means fractional)
      }
      else
      {
         ASSERT(curSel == 1);
         DDX_UnitValueAndTag(pDX, nIDC, nIDCUnit, value, lengthUnit);
      }
   }
   else
   {
      // value is going into the control
      if (value < 0)
      {
         // value is fractional
         Float64 v = value;
         v *= -100; // want -0.50 to be 50.0 %
         DDX_Text(pDX, nIDC, v);
         pCB->SetCurSel(0);
      }
      else
      {
         // this is a unit value
         Float64 v = WBFL::Units::ConvertFromSysUnits(value, lengthUnit.UnitOfMeasure);
         DDX_Text(pDX, nIDC, v);
         pCB->SetCurSel(1);
      }
   }
}

void DDV_UnitValueChoice(CDataExchange* pDX, UINT nIDC, Float64& value, Float64 Ls, const WBFL::Units::LengthData& lengthUnit)
{
   if (pDX->m_bSaveAndValidate)
   {
      if (0 < value)
      {
         DDV_UnitValueLimitOrLess(pDX, nIDC, value, Ls, lengthUnit);
      }
      else if (value < -1.0)
      {
         pDX->PrepareEditCtrl(nIDC);
         AfxMessageBox(_T("Enter a value between 0% and 100%"));
         pDX->Fail();
      }
   }
}

CGirderSegmentStrandsPage::CGirderSegmentStrandsPage()
	: CPropertyPage(CGirderSegmentStrandsPage::IDD)
{
   m_pSegment = 0;
}

CGirderSegmentStrandsPage::~CGirderSegmentStrandsPage()
{
}

void CGirderSegmentStrandsPage::Init(CPrecastSegmentData* pSegment)
{
   m_pSegment = pSegment;
   m_Strands = m_pSegment->Strands;
   m_Tendons = m_pSegment->Tendons;

   if (m_Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
   {
      m_pGrid = std::make_unique<CPointStrandGrid>();
   }
   else
   {
      m_pGrid = std::make_unique<CRowStrandGrid>();
   }
}

void CGirderSegmentStrandsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);


   DDX_Control(pDX, IDC_STRAIGHT_STRAND_SIZE, m_cbStraight);
   DDX_Control(pDX, IDC_HARPED_STRAND_SIZE, m_cbHarped);
   DDX_Control(pDX, IDC_TEMP_STRAND_SIZE, m_cbTemporary);
   DDX_Control(pDX, IDC_SPACING, m_StrandSpacingImage);
   DDX_Control(pDX, IDC_HARP_POINT_LOCATION, m_HarpPointLocationImage);


   m_pGrid->UpdateStrandData(pDX,&(m_pSegment->Strands));
   ExchangeHarpPointLocations(pDX, &(m_pSegment->Strands));

	//{{AFX_DATA_MAP(CGirderDescPrestressPage)
	//}}AFX_DATA_MAP

   std::array<bool, 3> bPjackUserInput;
   if ( !pDX->m_bSaveAndValidate )
   {
      bPjackUserInput[pgsTypes::Straight]  = !m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight);
      bPjackUserInput[pgsTypes::Harped]    = !m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped);
      bPjackUserInput[pgsTypes::Temporary] = !m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary);
   }

   DDX_Check_Bool(pDX, IDC_SS_JACK, bPjackUserInput[pgsTypes::Straight]);
   DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackUserInput[pgsTypes::Harped]);
   DDX_Check_Bool(pDX, IDC_TS_JACK, bPjackUserInput[pgsTypes::Temporary]);

   if ( pDX->m_bSaveAndValidate )
   {
      m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight,  !bPjackUserInput[pgsTypes::Straight]);
      m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped,    !bPjackUserInput[pgsTypes::Harped]);
      m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary, !bPjackUserInput[pgsTypes::Temporary]);
   }


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   if (pDX->m_bSaveAndValidate && m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped))
   {
      m_pSegment->Strands.SetPjack(pgsTypes::Harped, GetMaxPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Harped), pgsTypes::Harped ) );
   }
   else
   {
      Float64 Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Harped);
      DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
      if ( pDX->m_bSaveAndValidate )
      {
         m_pSegment->Strands.SetPjack(pgsTypes::Harped,Pjack);
      }
   }

   if (pDX->m_bSaveAndValidate && m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight))
   {
      m_pSegment->Strands.SetPjack(pgsTypes::Straight, GetMaxPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Straight), pgsTypes::Straight) );
   }
   else
   {
      Float64 Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Straight);
      DDX_UnitValueAndTag( pDX, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
      if ( pDX->m_bSaveAndValidate )
      {
         m_pSegment->Strands.SetPjack(pgsTypes::Straight,Pjack);
      }
   }

   if (pDX->m_bSaveAndValidate && m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary))
   {
      m_pSegment->Strands.SetPjack(pgsTypes::Temporary, GetMaxPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary), pgsTypes::Temporary ) );
   }
   else
   {
      Float64 Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Temporary);
      DDX_UnitValueAndTag( pDX, IDC_TS_JACK_FORCE, IDC_TS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
      if ( pDX->m_bSaveAndValidate )
      {
         m_pSegment->Strands.SetPjack(pgsTypes::Temporary,Pjack);
      }
   }

   Float64 Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Straight);
   DDV_UnitValueLimitOrLess( pDX, IDC_SS_JACK_FORCE, Pjack,  GetUltPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Straight), pgsTypes::Straight ), pDisplayUnits->GetGeneralForceUnit(), _T("Pjack must be less than the ultimate value of %f %s") );
   
   Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Harped);
   DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, Pjack, GetUltPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Harped), pgsTypes::Harped ),   pDisplayUnits->GetGeneralForceUnit(), _T("Pjack must be less than the ultimate value of %f %s") );
   
   Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Temporary);
   DDV_UnitValueLimitOrLess( pDX, IDC_TS_JACK_FORCE, Pjack, GetUltPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary), pgsTypes::Temporary ),   pDisplayUnits->GetGeneralForceUnit(), _T("Pjack must be less than the ultimate value of %f %s") );

   DDX_CBItemData(pDX, IDC_STRAIGHT_STRAND_SIZE, m_StrandKey[pgsTypes::Straight]);
   DDX_CBItemData(pDX, IDC_HARPED_STRAND_SIZE, m_StrandKey[pgsTypes::Harped]);
   DDX_CBItemData(pDX, IDC_TEMP_STRAND_SIZE, m_StrandKey[pgsTypes::Temporary]);

   pgsTypes::TTSUsage ttsUsage = m_pSegment->Strands.GetTemporaryStrandUsage();
   DDX_CBItemData(pDX, IDC_TTS_USE, ttsUsage);
   if (pDX->m_bSaveAndValidate)
   {
      m_pSegment->Strands.SetTemporaryStrandUsage(ttsUsage);
   }

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      const auto* pStraightStrand = pPool->GetStrand(m_StrandKey[pgsTypes::Straight]);
      const auto* pHarpedStrand = pPool->GetStrand(m_StrandKey[pgsTypes::Harped]);
      const auto* pTemporaryStrand = pPool->GetStrand(m_StrandKey[pgsTypes::Temporary]);
      if (!pPool->CompareStrands(pStraightStrand, pHarpedStrand)/* || !pPool->CompareStrands(pStraightStrand, pTemporaryStrand)*/)
      {
         pDX->PrepareCtrl(IDC_STRAIGHT_STRAND_SIZE);
         AfxMessageBox(_T("Straight and harped strands must be the same Grade (250, 270, 300) and Type (low relaxation or stress relieved)"));
         pDX->Fail();
      }

      m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight, pStraightStrand);
      m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped, pHarpedStrand);
      m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pTemporaryStrand);

      // Update controls and UI elements
      UpdateStrandControls();
   }

#pragma Reminder("strand point model - need to verify strands are in the girder")
   // this may not be the best place since we use UpdateData from several locations
   // before this dialog is closed with [OK] need to validate
   // one "gotcha" is harped strands that are not over the web...
}


BEGIN_MESSAGE_MAP(CGirderSegmentStrandsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentStrandsPage)
	ON_BN_CLICKED(IDC_SS_JACK, OnUpdateStraightStrandPjEdit)
	ON_BN_CLICKED(IDC_HS_JACK, OnUpdateHarpedStrandPjEdit)
	ON_BN_CLICKED(IDC_TS_JACK, OnUpdateTemporaryStrandPjEdit)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_STRAIGHT_STRAND_SIZE, OnStraightStrandTypeChanged)
   ON_CBN_SELCHANGE(IDC_HARPED_STRAND_SIZE, OnHarpedStrandTypeChanged)
   ON_CBN_SELCHANGE(IDC_TEMP_STRAND_SIZE, OnTempStrandTypeChanged)
   ON_CBN_SELCHANGE(IDC_X1_MEASURE, OnChange)
   ON_CBN_SELCHANGE(IDC_X2_MEASURE, OnChange)
   ON_CBN_SELCHANGE(IDC_X3_MEASURE, OnChange)
   ON_CBN_SELCHANGE(IDC_X4_MEASURE, OnChange)
   ON_EN_CHANGE(IDC_X1, OnChange)
   ON_EN_CHANGE(IDC_X2, OnChange)
   ON_EN_CHANGE(IDC_X3, OnChange)
   ON_EN_CHANGE(IDC_X4, OnChange)
   ON_CBN_SELCHANGE(IDC_TTS_USE, &CGirderSegmentStrandsPage::OnChange)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_ADD, &CGirderSegmentStrandsPage::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &CGirderSegmentStrandsPage::OnBnClickedRemove)
   ON_BN_CLICKED(IDC_EPOXY, &CGirderSegmentStrandsPage::OnEpoxyChanged)
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentStrandsPage message handlers

BOOL CGirderSegmentStrandsPage::OnInitDialog() 
{
   m_pGrid->SubclassDlgItem(IDC_GRID, this);
   m_pGrid->CustomInit(m_pSegment);

   m_DrawStrands.SubclassDlgItem(IDC_DRAW_STRANDS,this);
   //m_DrawStrands.CustomInit(m_pSegment,&m_Strands,&m_Tendons); // we will do this in OnSetActive

   // Select the strand size
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      m_StrandKey[strandType] = pPool->GetStrandKey(m_pSegment->Strands.GetStrandMaterial(strandType));
   }

   if ( WBFL::System::Flags<Int64>::IsSet(m_StrandKey[pgsTypes::Straight],std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::GritEpoxy)) )
   {
      CheckDlgButton(IDC_EPOXY,BST_CHECKED);
   }

   UpdateStrandList(IDC_STRAIGHT_STRAND_SIZE);
   UpdateStrandList(IDC_HARPED_STRAND_SIZE);
   UpdateStrandList(IDC_TEMP_STRAND_SIZE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   Float64 L = pBridge->GetSegmentLength(m_pSegment->GetSegmentKey());
   CString strSegmentLengthLabel;
   strSegmentLengthLabel.Format(_T("%s Length: %s"),pDocType->IsPGSuperDocument() ? _T("Girder") : _T("Segment"),FormatDimension(L,pDisplayUnits->GetSpanLengthUnit()));
   GetDlgItem(IDC_SEGMENT_LENGTH_LABEL)->SetWindowText(strSegmentLengthLabel);

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TTS_USE);
   int idx = pCB->AddString(_T("Pretensioned with permanent strands"));
   pCB->SetItemData(idx, (DWORD)pgsTypes::ttsPretensioned);

   idx = pCB->AddString(_T("Post-tensioned before lifting"));
   pCB->SetItemData(idx, (DWORD)pgsTypes::ttsPTBeforeLifting);

   idx = pCB->AddString(_T("Post-tensioned immedately after lifting"));
   pCB->SetItemData(idx, (DWORD)pgsTypes::ttsPTAfterLifting);

   idx = pCB->AddString(_T("Post-tensioned immedately before shipping"));
   pCB->SetItemData(idx, (DWORD)pgsTypes::ttsPTBeforeShipping);

   FillHarpPointUnitComboBox(IDC_X1_MEASURE, pDisplayUnits->GetSpanLengthUnit());
   FillHarpPointUnitComboBox(IDC_X2_MEASURE, pDisplayUnits->GetSpanLengthUnit());
   FillHarpPointUnitComboBox(IDC_X3_MEASURE, pDisplayUnits->GetSpanLengthUnit());
   FillHarpPointUnitComboBox(IDC_X4_MEASURE, pDisplayUnits->GetSpanLengthUnit());


   // All this work has to be done before CPropertyPage::OnInitDialog().
   // This code sets up the "current" selections which must be done prior to
   // calling DoDataExchange.  OnInitDialog() calls DoDataExchange().

   // Set the OK button as the default button
   SendMessage (DM_SETDEFID, IDOK);

   CPropertyPage::OnInitDialog();

   m_StrandSpacingImage.SetImage(_T("StrandSpacing"),_T("Metafile"));
   m_HarpPointLocationImage.SetImage(_T("HarpPointLocation"), _T("Metafile"));

   EnableToolTips(TRUE);
   EnableRemoveButton(FALSE); // start off with the button disabled... it will get enabled when a row in the grid is selected

   if (m_pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
   {
      m_StrandSpacingImage.ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SPACING_LABEL)->ShowWindow(SW_HIDE);
   }


   
   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderSegmentStrandsPage::UpdateSectionDepth()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   Float64 L = pBridge->GetSegmentLength(m_pSegment->GetSegmentKey());

   GET_IFACE2(pBroker, IShapes, pShapes);
   std::array<Float64, 4> Xhp;
   GetHarpPointLocations(&Xhp[ZoneBreakType::Start], &Xhp[ZoneBreakType::LeftBreak], &Xhp[ZoneBreakType::RightBreak], &Xhp[ZoneBreakType::End]);
   std::array<CComPtr<IShape>, 4> shape;
   std::array<CComPtr<IRect2d>, 4> bounding_box;
   std::array<UINT, 4> nIDC{ IDC_HG_X1,IDC_HG_X2,IDC_HG_X3,IDC_HG_X4 };
   for (int i = 0; i < 4; i++)
   {
      if (Xhp[i] < 0)
      {
         Xhp[i] *= -L;
      }
      pShapes->GetSegmentShape(m_pSegment, Xhp[i], (i == 0 ? pgsTypes::sbRight : pgsTypes::sbLeft), &shape[i]);
      shape[i]->get_BoundingBox(&bounding_box[i]);
      Float64 Hg;
      bounding_box[i]->get_Height(&Hg);
      GetDlgItem(nIDC[i])->SetWindowText(::FormatDimension(Hg, pDisplayUnits->GetComponentDimUnit()));
   }
}

Float64 CGirderSegmentStrandsPage::GetMaxPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, IPretensionForce, pPrestress );

   // TRICKY CODE
   // If strand stresses are limited immediate prior to transfer, prestress losses must be computed between jacking and prestress transfer in 
   // order to compute PjackMax. Losses are computed from transfer to final in one shot. The side of effect of this is that a bridge analysis
   // model must be built and in doing so, live load distribution factors must be computed. If the live load distribution factors cannot
   // be computed because of a range of applicability issue, an exception will be thrown.
   //
   // This exception adversely impacts the behavior of this dialog. To prevent these problems, capture the current ROA setting, change ROA to
   // "Ignore", compute PjackMax, and then restore the ROA setting.
   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   LldfRangeOfApplicabilityAction action = pLiveLoads->GetLldfRangeOfApplicabilityAction();
   pLiveLoads->SetLldfRangeOfApplicabilityAction(roaIgnore);

   Float64 PjackMax;
   try
   {
      PjackMax = pPrestress->GetPjackMax(m_pSegment->GetSegmentKey(), *(m_Strands.GetStrandMaterial(strandType)), nStrands);
   }
   catch (... )
   {
      pLiveLoads->SetLldfRangeOfApplicabilityAction(action);
      throw;
   }

   pLiveLoads->SetLldfRangeOfApplicabilityAction(action);
   pEvents->CancelPendingEvents();

   return PjackMax;
}

Float64 CGirderSegmentStrandsPage::GetUltPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType)
{
   const auto& strand = *(m_Strands.GetStrandMaterial(strandType));

   // Ultimate strength of strand group
   Float64 ult = strand.GetUltimateStrength();
   Float64 area = strand.GetNominalArea();

   return nStrands*area*ult;
}

void CGirderSegmentStrandsPage::OnUpdateTemporaryStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_TS_JACK,IDC_TS_JACK_FORCE,IDC_TS_JACK_FORCE_UNIT,pgsTypes::Temporary);
}

void CGirderSegmentStrandsPage::OnUpdateHarpedStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_HS_JACK,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Harped);
}

void CGirderSegmentStrandsPage::OnUpdateStraightStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_SS_JACK,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
}

void CGirderSegmentStrandsPage::OnUpdateStrandPjEdit(UINT nCheck,UINT nForceEdit,UINT nUnit,pgsTypes::StrandType strandType)
{
   m_pGrid->UpdateStrandData(nullptr,&m_Strands);

   StrandIndexType nStrands = m_Strands.GetStrandCount(strandType);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   BOOL bEnable = IsDlgButtonChecked( nCheck ) ? TRUE : FALSE; // user defined value if checked
   if (  nStrands == 0 )
   {
      bEnable = FALSE; // don't enable if the number of strands is zero
   }

   CWnd* pWnd = GetDlgItem( nForceEdit );
   ASSERT( pWnd );
   pWnd->EnableWindow( bEnable );

   Float64 Pjack = 0;
   if ( bEnable )
   {
      // Set the edit control value to the last user input force
      Pjack = m_Strands.GetLastUserPjack(strandType);
   }
   else if ( nStrands != 0 )
   {
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      // Get the edit control value and save it as the last user input force
      CString val_as_text;
      pWnd->GetWindowText( val_as_text );
      Pjack = _tstof( val_as_text );
      Pjack = WBFL::Units::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
      
      m_Strands.SetLastUserPjack(strandType, Pjack);
      Pjack = GetMaxPjack(nStrands, strandType);
   }

   CDataExchange dx(this,FALSE);
   DDX_UnitValueAndTag( &dx, nForceEdit, nUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );
}

void CGirderSegmentStrandsPage::UpdateStrandControls() 
{
   m_DrawStrands.Invalidate();
   m_DrawStrands.UpdateWindow();

   InitPjackEdits();
}

void CGirderSegmentStrandsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), m_pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectRowInput ? IDH_GIRDER_STRAND_ROW_INPUT : IDH_GIRDER_INDIVIDUAL_STRAND_INPUT );
}

void CGirderSegmentStrandsPage::OnEpoxyChanged()
{
   WBFL::System::Flags<Int64>::Clear(&m_StrandKey[pgsTypes::Straight], std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::None));
   WBFL::System::Flags<Int64>::Clear(&m_StrandKey[pgsTypes::Straight], std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::GritEpoxy));
   WBFL::System::Flags<Int64>::Set(&m_StrandKey[pgsTypes::Straight],IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::GritEpoxy) : std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::None));

   WBFL::System::Flags<Int64>::Clear(&m_StrandKey[pgsTypes::Harped], std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::None));
   WBFL::System::Flags<Int64>::Clear(&m_StrandKey[pgsTypes::Harped], std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::GritEpoxy));
   WBFL::System::Flags<Int64>::Set(&m_StrandKey[pgsTypes::Harped], IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::GritEpoxy) : std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::None));

   UpdateStrandList(IDC_STRAIGHT_STRAND_SIZE);
   UpdateStrandList(IDC_HARPED_STRAND_SIZE);
}

void CGirderSegmentStrandsPage::UpdateStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   Int64 cur_key = (Int64)pList->GetItemData( cur_sel );
   // remove the coating flag from the current key
   WBFL::System::Flags<Int64>::Clear(&cur_key, std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::None));
   WBFL::System::Flags<Int64>::Clear(&cur_key, std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(WBFL::Materials::PsStrand::Coating::GritEpoxy));

   BOOL bIsEpoxy = FALSE;
   if ( nIDC == IDC_STRAIGHT_STRAND_SIZE || nIDC == IDC_HARPED_STRAND_SIZE)
   {
      bIsEpoxy = IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? TRUE : FALSE;
   }
  WBFL::Materials::PsStrand::Coating coating = (bIsEpoxy ? WBFL::Materials::PsStrand::Coating::GritEpoxy : WBFL::Materials::PsStrand::Coating::None);
   WBFL::System::Flags<Int64>::Set(&cur_key, std::underlying_type<WBFL::Materials::PsStrand::Coating>::type(coating)); // add the coating flag for the strand type we are changing to

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for (int i = 0; i < 3; i++)
   {
      WBFL::Materials::PsStrand::Grade grade = (i == 0 ? WBFL::Materials::PsStrand::Grade::Gr1725 :
                                                i == 1 ? WBFL::Materials::PsStrand::Grade::Gr1860 : WBFL::Materials::PsStrand::Grade::Gr2070);
      for ( int j = 0; j < 2; j++ )
      {
         WBFL::Materials::PsStrand::Type type = (j == 0 ? WBFL::Materials::PsStrand::Type::LowRelaxation : WBFL::Materials::PsStrand::Type::StressRelieved);

         lrfdStrandIter iter(grade,type,coating);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const auto* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );
               
            auto key = pPool->GetStrandKey( pStrand );
            pList->SetItemData( idx, key );

            if ( key == cur_key )
            {
               new_cur_sel = sel_count;
            }

            sel_count++;
         }
      }
   }

   // Attempt to re-select the strand.
   if ( 0 <= new_cur_sel )
   {
      pList->SetCurSel( new_cur_sel );
   }
   else
   {
      pList->SetCurSel( pList->GetCount()-1 );
   }
}

void CGirderSegmentStrandsPage::OnStrandTypeChanged(int nIDC,pgsTypes::StrandType strandType)
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   int curSel = pList->GetCurSel();
   m_StrandKey[strandType] = (Int64)pList->GetItemData(curSel);

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   m_pSegment->Strands.SetStrandMaterial(strandType, pPool->GetStrand(m_StrandKey[strandType]));

   // Now we can update pjack values in dialog
   InitPjackEdits();
}

void CGirderSegmentStrandsPage::OnStraightStrandTypeChanged() 
{
   OnStrandTypeChanged(IDC_STRAIGHT_STRAND_SIZE, pgsTypes::Straight);
}

void CGirderSegmentStrandsPage::OnHarpedStrandTypeChanged()
{
   OnStrandTypeChanged(IDC_HARPED_STRAND_SIZE, pgsTypes::Harped);
}

void CGirderSegmentStrandsPage::OnTempStrandTypeChanged() 
{
   OnStrandTypeChanged(IDC_TEMP_STRAND_SIZE, pgsTypes::Temporary);
}

void CGirderSegmentStrandsPage::InitPjackEdits()
{
   m_pGrid->UpdateStrandData(nullptr, &m_Strands); // do this here so we don't have to do it 3 times and get the exact same results
   InitPjackEdits(IDC_SS_JACK,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
   InitPjackEdits(IDC_HS_JACK,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Harped);
   InitPjackEdits(IDC_TS_JACK,IDC_TS_JACK_FORCE,IDC_TS_JACK_FORCE_UNIT,pgsTypes::Temporary);
}

void CGirderSegmentStrandsPage::InitPjackEdits(UINT nCalcPjack,UINT nPjackEdit,UINT nPjackUnit,pgsTypes::StrandType strandType)
{
   // call m_pGrid->UpdateStrandData(nullptr,&m_Strands) before calling this method

   StrandIndexType nStrands = m_Strands.GetStrandCount(strandType);

   CButton* chkbox = (CButton*)GetDlgItem(nCalcPjack);
   BOOL bEnable = FALSE; // true is user input model

   if (  0 < nStrands )
   {
      bEnable = (chkbox->GetCheck() == BST_CHECKED);
   }

   CWnd* pWnd = GetDlgItem( nPjackEdit );
   ASSERT( pWnd );
   pWnd->EnableWindow( bEnable );

   // only update dialog values if they are auto-computed
   if (!bEnable)
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
      CDataExchange dx(this,FALSE);

      Float64 Pjack = 0;
      m_Strands.SetPjack(strandType, GetMaxPjack(nStrands,strandType));
      Pjack = m_Strands.GetPjack(strandType);
      DDX_UnitValueAndTag( &dx, nPjackEdit, nPjackUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }
}

BOOL CGirderSegmentStrandsPage::OnSetActive() 
{
   // make sure segment geometry is up to date with what ever has been changed during
   // this editing session
   m_Strands = m_pSegment->Strands;
   m_Tendons = m_pSegment->Tendons;
   m_DrawStrands.CustomInit(m_pSegment,&m_Strands,&m_Tendons);

   OnChange();

   return CPropertyPage::OnSetActive();
}

BOOL CGirderSegmentStrandsPage::OnKillActive()
{
   //this->SetFocus();  // prevents artifacts from grid list controls (not sure why)

   return CPropertyPage::OnKillActive();
}

void CGirderSegmentStrandsPage::OnBnClickedAdd()
{
   m_pGrid->OnAddRow();
}

void CGirderSegmentStrandsPage::OnBnClickedRemove()
{
   m_pGrid->OnRemoveSelectedRows();
}

void CGirderSegmentStrandsPage::EnableRemoveButton(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnable);
}

void CGirderSegmentStrandsPage::OnChange()
{
   m_pGrid->UpdateStrandData(nullptr,&m_Strands);
   GetHarpPointLocations(&m_Strands);
   m_Strands.SetTemporaryStrandUsage(GetTemporaryStrandUsage());

   CString strLabel;
   strLabel.Format(_T("Straight Strands (%d)"), m_Strands.GetStrandCount(pgsTypes::Straight));
   GetDlgItem(IDC_SS_LABEL)->SetWindowText(strLabel);

   strLabel.Format(_T("Harped Strands (%d)"), m_Strands.GetStrandCount(pgsTypes::Harped));
   GetDlgItem(IDC_HS_LABEL)->SetWindowText(strLabel);

   strLabel.Format(_T("Temporary Strands (%d)"), m_Strands.GetStrandCount(pgsTypes::Temporary));
   GetDlgItem(IDC_TS_LABEL)->SetWindowText(strLabel);

   IndexType nExtendedStart = m_Strands.GetExtendedStrandCount(pgsTypes::Straight,pgsTypes::metStart);
   nExtendedStart += m_Strands.GetExtendedStrandCount(pgsTypes::Harped,pgsTypes::metStart);
   nExtendedStart += m_Strands.GetExtendedStrandCount(pgsTypes::Temporary,pgsTypes::metStart);

   IndexType nExtendedEnd = m_Strands.GetExtendedStrandCount(pgsTypes::Straight,pgsTypes::metEnd);
   nExtendedEnd += m_Strands.GetExtendedStrandCount(pgsTypes::Harped,pgsTypes::metEnd);
   nExtendedEnd += m_Strands.GetExtendedStrandCount(pgsTypes::Temporary,pgsTypes::metEnd);

   strLabel.Format(_T("Extended Strands (Left %d, Right %d)"),nExtendedStart,nExtendedEnd);
   GetDlgItem(IDC_EXTENDED_STRANDS_LABEL)->SetWindowText(strLabel);


   IndexType nDebondingStart = m_Strands.GetDebondCount(pgsTypes::Straight,pgsTypes::metStart,nullptr);
   nDebondingStart += m_Strands.GetDebondCount(pgsTypes::Harped,pgsTypes::metStart,nullptr);
   nDebondingStart += m_Strands.GetDebondCount(pgsTypes::Temporary,pgsTypes::metStart,nullptr);

   IndexType nDebondingEnd = m_Strands.GetDebondCount(pgsTypes::Straight,pgsTypes::metEnd,nullptr);
   nDebondingEnd += m_Strands.GetDebondCount(pgsTypes::Harped,pgsTypes::metEnd,nullptr);
   nDebondingEnd += m_Strands.GetDebondCount(pgsTypes::Temporary,pgsTypes::metEnd,nullptr);

   strLabel.Format(_T("Debonded Strands (Start %d, End %d)"),nDebondingStart,nDebondingEnd);
   GetDlgItem(IDC_DEBONDED_STRANDS_LABEL)->SetWindowText(strLabel);

   UpdateStrandControls();
   UpdateSectionDepth();
}

HBRUSH CGirderSegmentStrandsPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = CPropertyPage::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_TS_LABEL )
   {
      pDC->SetTextColor( TEMPORARY_FILL_COLOR );
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_EXTENDED_STRANDS_LABEL )
   {
      pDC->SetTextColor( EXTENDED_FILL_COLOR );
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_DEBONDED_STRANDS_LABEL )
   {
      pDC->SetTextColor( DEBOND_FILL_COLOR );
   }


   return hbr;
}

void CGirderSegmentStrandsPage::FillHarpPointUnitComboBox(UINT nIDC, const WBFL::Units::LengthData& lengthUnit)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(nIDC);
   pCB->AddString(_T("%"));
   pCB->AddString(lengthUnit.UnitOfMeasure.UnitTag().c_str());
}

void CGirderSegmentStrandsPage::ExchangeHarpPointLocations(CDataExchange* pDX,CStrandData* pStrands)
{
   if (pDX->m_bSaveAndValidate && pStrands->GetStrandCount(pgsTypes::Harped) == 0)
      return;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   Float64 Xstart, Xlhp, Xrhp, Xend;
   if (!pDX->m_bSaveAndValidate)
   {
      pStrands->GetHarpPoints(&Xstart, &Xlhp, &Xrhp, &Xend);
   }

   DDX_UnitValueChoice(pDX, IDC_X1, IDC_X1_MEASURE, Xstart, pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueChoice(pDX, IDC_X2, IDC_X2_MEASURE, Xlhp, pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueChoice(pDX, IDC_X3, IDC_X3_MEASURE, Xrhp, pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueChoice(pDX, IDC_X4, IDC_X4_MEASURE, Xend, pDisplayUnits->GetSpanLengthUnit());

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(m_pSegment->GetSegmentKey());
   DDV_UnitValueChoice(pDX, IDC_X1, Xstart, Ls, pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueChoice(pDX, IDC_X2, Xlhp, Ls, pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueChoice(pDX, IDC_X3, Xrhp, Ls, pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueChoice(pDX, IDC_X4, Xend, Ls, pDisplayUnits->GetSpanLengthUnit());

   if (pDX->m_bSaveAndValidate)
   {
      pStrands->SetHarpPoints(Xstart, Xlhp, Xrhp, Xend);
   }
}

void CGirderSegmentStrandsPage::GetHarpPointLocations(CStrandData* pStrands)
{
   Float64 Xstart, Xlhp, Xrhp, Xend;
   GetHarpPointLocations(&Xstart, &Xlhp, &Xrhp, &Xend);
   pStrands->SetHarpPoints(Xstart, Xlhp, Xrhp, Xend);
}

void CGirderSegmentStrandsPage::GetHarpPointLocations(Float64* pXstart, Float64* pXlhp, Float64* pXrhp, Float64* pXend)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CDataExchange dx(this, TRUE);
   DDX_UnitValueChoice(&dx, IDC_X1, IDC_X1_MEASURE, *pXstart, pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueChoice(&dx, IDC_X2, IDC_X2_MEASURE, *pXlhp, pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueChoice(&dx, IDC_X3, IDC_X3_MEASURE, *pXrhp, pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueChoice(&dx, IDC_X4, IDC_X4_MEASURE, *pXend, pDisplayUnits->GetSpanLengthUnit());
}

pgsTypes::TTSUsage CGirderSegmentStrandsPage::GetTemporaryStrandUsage()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TTS_USE);
   int curSel = pCB->GetCurSel();
   return (pgsTypes::TTSUsage)(pCB->GetItemData(curSel));
}