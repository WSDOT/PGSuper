///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentStrandsPage.h"
#include "GirderSegmentDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\Intervals.h>

#include <Material\PsStrand.h>
#include <LRFD\StrandPool.h>

#include "PGSuperColors.h"
#include <PgsExt\DesignConfigUtil.h>

#include "GirderDescDlg.h" // for ReconcileDebonding

// CGirderSegmentStrandsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentStrandsPage, CPropertyPage)

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
}

void CGirderSegmentStrandsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   m_Grid.UpdateStrandData(pDX,&(m_pSegment->Strands));

	//{{AFX_DATA_MAP(CGirderDescPrestressPage)
	//}}AFX_DATA_MAP

   bool bPjackUserInput[3];
   if ( !pDX->m_bSaveAndValidate )
   {
      bPjackUserInput[pgsTypes::Straight]  = !m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight);
      bPjackUserInput[pgsTypes::Harped]    = !m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped);
      bPjackUserInput[pgsTypes::Temporary] = !m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary);
   }

   DDX_Control(pDX,IDC_SPACING, m_Picture);

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
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if (pDX->m_bSaveAndValidate && m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped))
   {
      m_pSegment->Strands.SetPjack(pgsTypes::Harped, GetMaxPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Harped) ) );
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
      m_pSegment->Strands.SetPjack(pgsTypes::Straight, GetMaxPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Straight)) );
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
      m_pSegment->Strands.SetPjack(pgsTypes::Temporary, GetMaxPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary) ) );
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
   DDV_UnitValueLimitOrLess( pDX, IDC_SS_JACK_FORCE, Pjack,  GetUltPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Straight) ), pDisplayUnits->GetGeneralForceUnit(), _T("Pjack must be less than the ultimate value of %f %s") );
   
   Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Harped);
   DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, Pjack, GetUltPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Harped) ),   pDisplayUnits->GetGeneralForceUnit(), _T("Pjack must be less than the ultimate value of %f %s") );
   
   Pjack = m_pSegment->Strands.GetPjack(pgsTypes::Temporary);
   DDV_UnitValueLimitOrLess( pDX, IDC_TS_JACK_FORCE, Pjack, GetUltPjack( m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary) ),   pDisplayUnits->GetGeneralForceUnit(), _T("Pjack must be less than the ultimate value of %f %s") );

   DDX_CBItemData(pDX, IDC_STRAND_SIZE, m_StrandKey);
   DDX_CBItemData(pDX, IDC_TEMP_STRAND_SIZE, m_TempStrandKey);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight,  pPool->GetStrand( m_StrandKey ));
      m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,    pPool->GetStrand( m_StrandKey ));

      m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand( m_TempStrandKey ));

      // Update controls and UI elements
      UpdateStrandControls();
   }
}


BEGIN_MESSAGE_MAP(CGirderSegmentStrandsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentStrandsPage)
	ON_BN_CLICKED(IDC_SS_JACK, OnUpdateStraightStrandPjEdit)
	ON_BN_CLICKED(IDC_HS_JACK, OnUpdateHarpedStrandPjEdit)
	ON_BN_CLICKED(IDC_TS_JACK, OnUpdateTemporaryStrandPjEdit)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_STRAND_SIZE, OnStrandTypeChanged)
	ON_CBN_SELCHANGE(IDC_TEMP_STRAND_SIZE, OnTempStrandTypeChanged)
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
   m_Grid.SubclassDlgItem(IDC_GRID, this);
   m_Grid.CustomInit(m_pSegment);

   m_DrawStrands.SubclassDlgItem(IDC_DRAW_STRANDS,this);
   m_DrawStrands.CustomInit(m_pSegment);

   // Select the strand size
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   m_StrandKey     = pPool->GetStrandKey(m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight) );
   m_TempStrandKey = pPool->GetStrandKey(m_pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary) );

   if ( sysFlags<Int32>::IsSet(m_StrandKey,matPsStrand::GritEpoxy) )
   {
      CheckDlgButton(IDC_EPOXY,BST_CHECKED);
   }

   UpdateStrandList(IDC_STRAND_SIZE);
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


   // All this work has to be done before CPropertyPage::OnInitDialog().
   // This code sets up the "current" selections which must be done prior to
   // calling DoDataExchange.  OnInitDialog() calls DoDataExchange().

   // Set the OK button as the default button
   SendMessage (DM_SETDEFID, IDOK);

   CPropertyPage::OnInitDialog();

   m_Picture.SetImage(_T("StrandSpacing"),_T("Metafile"));

   EnableToolTips(TRUE);
   EnableRemoveButton(FALSE); // start off with the button disabled... it will get enabled when a row in the grid is selected

   OnChange();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

Float64 CGirderSegmentStrandsPage::GetMaxPjack(StrandIndexType nStrands)
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
      PjackMax = pPrestress->GetPjackMax(m_pSegment->GetSegmentKey(),*m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight), nStrands);
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

Float64 CGirderSegmentStrandsPage::GetUltPjack(StrandIndexType nStrands)
{
   const matPsStrand& strand = *(m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight));

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
   m_Grid.UpdateStrandData(NULL,&(m_pSegment->Strands));

   StrandIndexType nStrands = m_pSegment->Strands.GetStrandCount(strandType);

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
      Pjack = m_pSegment->Strands.GetLastUserPjack(strandType);
   }
   else if ( nStrands != 0 )
   {
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      // Get the edit control value and save it as the last user input force
      CString val_as_text;
      pWnd->GetWindowText( val_as_text );
      Pjack = _tstof( val_as_text );
      Pjack = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
      
      m_pSegment->Strands.SetLastUserPjack(strandType, Pjack);
      Pjack = GetMaxPjack(nStrands);
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
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_DIRECT_STRAND_INPUT );
}

void CGirderSegmentStrandsPage::OnEpoxyChanged()
{
   sysFlags<Int32>::Clear(&m_StrandKey,matPsStrand::None);
   sysFlags<Int32>::Clear(&m_StrandKey,matPsStrand::GritEpoxy);
   sysFlags<Int32>::Set(&m_StrandKey,IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? matPsStrand::GritEpoxy : matPsStrand::None);

   UpdateStrandList(IDC_STRAND_SIZE);
}

void CGirderSegmentStrandsPage::UpdateStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   Int32 cur_key = (Int32)pList->GetItemData( cur_sel );
   // remove the coating flag from the current key
   sysFlags<Int32>::Clear(&cur_key,matPsStrand::None);
   sysFlags<Int32>::Clear(&cur_key,matPsStrand::GritEpoxy);

   BOOL bIsEpoxy = FALSE;
   if ( nIDC == IDC_STRAND_SIZE )
   {
      bIsEpoxy = IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? TRUE : FALSE;
   }
   matPsStrand::Coating coating = (bIsEpoxy ? matPsStrand::GritEpoxy : matPsStrand::None);
   sysFlags<Int32>::Set(&cur_key,coating); // add the coating flag for the strand type we are changing to

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for ( int i = 0; i < 2; i++ )
   {
      matPsStrand::Grade grade = (i == 0 ? matPsStrand::Gr1725 : matPsStrand::Gr1860);
      for ( int j = 0; j < 2; j++ )
      {
         matPsStrand::Type type = (j == 0 ? matPsStrand::LowRelaxation : matPsStrand::StressRelieved);

         lrfdStrandIter iter(grade,type,coating);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const matPsStrand* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );
               
            Int32 key = pPool->GetStrandKey( pStrand );
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

void CGirderSegmentStrandsPage::OnStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   int curSel = pList->GetCurSel();
   m_StrandKey = (Int32)pList->GetItemData( curSel );

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight,  pPool->GetStrand( m_StrandKey ));
   m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,    pPool->GetStrand( m_StrandKey ));

   // Now we can update pjack values in dialog
   InitPjackEdits();
}

void CGirderSegmentStrandsPage::OnTempStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
   int curSel = pList->GetCurSel();
   m_TempStrandKey = (Int32)pList->GetItemData( curSel );

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand( m_TempStrandKey ));

   // Now we can update pjack values in dialog
   InitPjackEdits();
}

void CGirderSegmentStrandsPage::InitPjackEdits()
{
   InitPjackEdits(IDC_SS_JACK,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
   InitPjackEdits(IDC_HS_JACK,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Harped);
   InitPjackEdits(IDC_TS_JACK,IDC_TS_JACK_FORCE,IDC_TS_JACK_FORCE_UNIT,pgsTypes::Temporary);
}

void CGirderSegmentStrandsPage::InitPjackEdits(UINT nCalcPjack,UINT nPjackEdit,UINT nPjackUnit,pgsTypes::StrandType strandType)
{
   m_Grid.UpdateStrandData(NULL,&(m_pSegment->Strands));

   StrandIndexType nStrands = m_pSegment->Strands.GetStrandCount(strandType);

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
      m_pSegment->Strands.SetPjack(strandType, GetMaxPjack(nStrands));
      Pjack = m_pSegment->Strands.GetPjack(strandType);
      DDX_UnitValueAndTag( &dx, nPjackEdit, nPjackUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }
}

BOOL CGirderSegmentStrandsPage::OnSetActive() 
{
   // make sure segment geometry is up to date with what ever has been changed during
   // this editing session
   m_DrawStrands.CustomInit(m_pSegment);

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
   m_Grid.OnAddRow();
}

void CGirderSegmentStrandsPage::OnBnClickedRemove()
{
   m_Grid.OnRemoveSelectedRows();
}

void CGirderSegmentStrandsPage::EnableRemoveButton(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnable);
}

void CGirderSegmentStrandsPage::OnChange()
{
   m_Grid.UpdateStrandData(NULL,&(m_pSegment->Strands));

   CString strLabel;
   strLabel.Format(_T("Straight Strands (%d)"),m_pSegment->Strands.GetStrandCount(pgsTypes::Straight));
   GetDlgItem(IDC_SS_LABEL)->SetWindowText(strLabel);

   strLabel.Format(_T("Harped Strands (%d)"),m_pSegment->Strands.GetStrandCount(pgsTypes::Harped));
   GetDlgItem(IDC_HS_LABEL)->SetWindowText(strLabel);

   strLabel.Format(_T("Temporary Strands (%d)"),m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary));
   GetDlgItem(IDC_TS_LABEL)->SetWindowText(strLabel);

   IndexType nExtendedStart = m_pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,pgsTypes::metStart).size();
   nExtendedStart += m_pSegment->Strands.GetExtendedStrands(pgsTypes::Harped,pgsTypes::metStart).size();
   nExtendedStart += m_pSegment->Strands.GetExtendedStrands(pgsTypes::Temporary,pgsTypes::metStart).size();

   IndexType nExtendedEnd = m_pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,pgsTypes::metEnd).size();
   nExtendedEnd += m_pSegment->Strands.GetExtendedStrands(pgsTypes::Harped,pgsTypes::metEnd).size();
   nExtendedEnd += m_pSegment->Strands.GetExtendedStrands(pgsTypes::Temporary,pgsTypes::metEnd).size();

   strLabel.Format(_T("Extended Strands (Left %d, Right %d)"),nExtendedStart,nExtendedEnd);
   GetDlgItem(IDC_EXTENDED_STRANDS_LABEL)->SetWindowText(strLabel);


   IndexType nDebondingStart = m_pSegment->Strands.GetDebondCount(pgsTypes::Straight,pgsTypes::metStart,NULL);
   nDebondingStart += m_pSegment->Strands.GetDebondCount(pgsTypes::Harped,pgsTypes::metStart,NULL);
   nDebondingStart += m_pSegment->Strands.GetDebondCount(pgsTypes::Temporary,pgsTypes::metStart,NULL);

   IndexType nDebondingEnd = m_pSegment->Strands.GetDebondCount(pgsTypes::Straight,pgsTypes::metEnd,NULL);
   nDebondingEnd += m_pSegment->Strands.GetDebondCount(pgsTypes::Harped,pgsTypes::metEnd,NULL);
   nDebondingEnd += m_pSegment->Strands.GetDebondCount(pgsTypes::Temporary,pgsTypes::metEnd,NULL);

   strLabel.Format(_T("Debonded Strands (Start %d, End %d)"),nDebondingStart,nDebondingEnd);
   GetDlgItem(IDC_DEBONDED_STRANDS_LABEL)->SetWindowText(strLabel);

   UpdateStrandControls();
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
