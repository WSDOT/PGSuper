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

// GirderSegmentStrandsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentStrandsPage.h"
#include "GirderSegmentDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <EAF\EAFDisplayUnits.h>

#include <IFace\Intervals.h>

#include <Material\PsStrand.h>
#include <LRFD\StrandPool.h>

#include "HtmlHelp\HelpTopics.hh"

#include "PGSuperColors.h"
#include <PgsExt\DesignConfigUtil.h>

#include "GirderDescDlg.h" // for ReconcileDebonding

// CGirderSegmentStrandsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentStrandsPage, CPropertyPage)

CGirderSegmentStrandsPage::CGirderSegmentStrandsPage()
	: CPropertyPage(CGirderSegmentStrandsPage::IDD)
{
}

CGirderSegmentStrandsPage::~CGirderSegmentStrandsPage()
{
}

void CGirderSegmentStrandsPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

	CPropertyPage::DoDataExchange(pDX);

   m_Grid.UpdateStrandData(pDX,&(pSegment->Strands));

	//{{AFX_DATA_MAP(CGirderDescPrestressPage)
	//}}AFX_DATA_MAP

   bool bPjackUserInput[3]; // this has to be 3 because we are using the constants for straight and temporary (0 and 2)
   if ( !pDX->m_bSaveAndValidate )
   {
      bPjackUserInput[pgsTypes::Straight]  = !pSegment->Strands.bPjackCalculated[pgsTypes::Straight];
      bPjackUserInput[pgsTypes::Temporary] = !pSegment->Strands.bPjackCalculated[pgsTypes::Temporary];
   }

   DDX_Check_Bool(pDX, IDC_SS_JACK, bPjackUserInput[pgsTypes::Straight]);
   DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackUserInput[pgsTypes::Temporary]);

   if ( pDX->m_bSaveAndValidate )
   {
      pSegment->Strands.bPjackCalculated[pgsTypes::Straight]  = !bPjackUserInput[pgsTypes::Straight];
      pSegment->Strands.bPjackCalculated[pgsTypes::Temporary] = !bPjackUserInput[pgsTypes::Temporary];
   }

   if (pDX->m_bSaveAndValidate && pSegment->Strands.bPjackCalculated[pgsTypes::Temporary])
   {
      pSegment->Strands.Pjack[pgsTypes::Temporary] = GetMaxPjack( pSegment->Strands.Nstrands[pgsTypes::Temporary] );
   }
   else
   {
      DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, pSegment->Strands.Pjack[pgsTypes::Temporary], pDisplayUnits->GetGeneralForceUnit() );
   }

   if (pDX->m_bSaveAndValidate && pSegment->Strands.bPjackCalculated[pgsTypes::Straight])
   {
      pSegment->Strands.Pjack[pgsTypes::Straight] = GetMaxPjack( pSegment->Strands.Nstrands[pgsTypes::Straight] );
   }
   else
   {
      DDX_UnitValueAndTag( pDX, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, pSegment->Strands.Pjack[pgsTypes::Straight], pDisplayUnits->GetGeneralForceUnit() );
   }

   DDV_UnitValueLimitOrLess( pDX, IDC_SS_JACK_FORCE, pSegment->Strands.Pjack[pgsTypes::Straight],  GetUltPjack( pSegment->Strands.Nstrands[pgsTypes::Straight] ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
   DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, pSegment->Strands.Pjack[pgsTypes::Temporary], GetUltPjack( pSegment->Strands.Nstrands[pgsTypes::Harped] ),   pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );

	DDX_CBIndex(pDX, IDC_STRAND_SIZE, m_StrandSizeIdx);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
      Int32 key = (Int32)pList->GetItemData( m_StrandSizeIdx );
      pSegment->Strands.StrandMaterial[pgsTypes::Straight]  = pPool->GetStrand( key );
      pSegment->Strands.StrandMaterial[pgsTypes::Temporary] = pPool->GetStrand( key );

      // Update controls and UI elements
      UpdateStrandControls();
   }
}


BEGIN_MESSAGE_MAP(CGirderSegmentStrandsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentStrandsPage)
	ON_BN_CLICKED(IDC_SS_JACK, OnUpdateStraightStrandPjEdit)
	ON_BN_CLICKED(IDC_HS_JACK, OnUpdateTemporaryStrandPjEdit)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_STRAND_SIZE, OnStrandTypeChanged)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_ADD, &CGirderSegmentStrandsPage::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &CGirderSegmentStrandsPage::OnBnClickedRemove)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentStrandsPage message handlers

BOOL CGirderSegmentStrandsPage::OnInitDialog() 
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   m_Grid.SubclassDlgItem(IDC_GRID, this);
   m_Grid.CustomInit(pSegment);

   m_DrawStrands.SubclassDlgItem(IDC_DRAW_STRANDS,this);
   m_DrawStrands.CustomInit(pSegment);

   // Fill the strand size combo box.
   UpdateStrandList(IDC_STRAND_SIZE);

   // Select the strand size
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int32 target_key = pPool->GetStrandKey(pSegment->Strands.StrandMaterial[pgsTypes::Straight] );
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   int cStrands = pList->GetCount();
   for ( int i = 0; i < cStrands; i++ )
   {
      Int32 key = (Int32)pList->GetItemData( i );
      if ( key == target_key )
      {
         m_StrandSizeIdx = i;
         break;
      }
   }

   // All this work has to be done before CPropertyPage::OnInitDialog().
   // This code sets up the "current" selections which must be done prior to
   // calling DoDataExchange.  OnInitDialog() calls DoDataExchange().

   // Set the OK button as the default button
   SendMessage (DM_SETDEFID, IDOK);

   CPropertyPage::OnInitDialog();

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

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

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
      PjackMax = pPrestress->GetPjackMax(pParent->m_SegmentKey,*pSegment->Strands.StrandMaterial[pgsTypes::Straight], nStrands);
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
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   const matPsStrand& strand = *(pSegment->Strands.StrandMaterial[pgsTypes::Straight]);

   // Ultimate strength of strand group
   Float64 ult = strand.GetUltimateStrength();
   Float64 area = strand.GetNominalArea();

   return nStrands*area*ult;
}

void CGirderSegmentStrandsPage::OnUpdateTemporaryStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_HS_JACK,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Temporary);
}

void CGirderSegmentStrandsPage::OnUpdateStraightStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_SS_JACK,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
}

void CGirderSegmentStrandsPage::OnUpdateStrandPjEdit(UINT nCheck,UINT nForceEdit,UINT nUnit,pgsTypes::StrandType strandType)
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   m_Grid.UpdateStrandData(NULL,&(pSegment->Strands));

   StrandIndexType nStrands = pSegment->Strands.GetNstrands(strandType);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   BOOL bEnable = IsDlgButtonChecked( nCheck ) ? TRUE : FALSE; // user defined value if checked
   if (  nStrands == 0 )
      bEnable = FALSE; // don't enable if the number of strands is zero

   CWnd* pWnd = GetDlgItem( nForceEdit );
   ASSERT( pWnd );
   pWnd->EnableWindow( bEnable );

   Float64 Pjack = 0;
   if ( bEnable )
   {
      // Set the edit control value to the last user input force
      Pjack = pSegment->Strands.LastUserPjack[strandType];
   }
   else if ( nStrands != 0 )
   {
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      // Get the edit control value and save it as the last user input force
      CString val_as_text;
      pWnd->GetWindowText( val_as_text );
      Pjack = _tstof( val_as_text );
      Pjack = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
      
      pSegment->Strands.LastUserPjack[strandType] = Pjack;
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
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_PRESTRESS );
}

void CGirderSegmentStrandsPage::UpdateStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   matPsStrand::Size cur_size = matPsStrand::D1270;
   if ( cur_sel != CB_ERR )
   {
      Int32 cur_key = (Int32)pList->GetItemData( cur_sel );
      const matPsStrand* pCurStrand = pPool->GetStrand( cur_key );
      cur_size = pCurStrand->GetSize();
   }

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for ( int i = 0; i < 2; i++ )
   {
      matPsStrand::Grade grade = (i == 0 ? matPsStrand::Gr1725 : matPsStrand::Gr1860);
      for ( int j = 0; j < 2; j++ )
      {
         matPsStrand::Type type = (j == 0 ? matPsStrand::LowRelaxation : matPsStrand::StressRelieved);

         lrfdStrandIter iter(grade,type);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const matPsStrand* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );

            if ( idx != CB_ERR )
            { 
               // if there wasn't an error adding the size, add a data item
               Int32 key;
               key = pPool->GetStrandKey( pStrand );

               if ( pList->SetItemData( idx, key ) == CB_ERR )
               {
                  // if there was an error adding the data item, remove the size
                  idx = pList->DeleteString( idx );
                  ASSERT( idx != CB_ERR ); // make sure it got removed.
               }
               else
               {
                  // data item added successfully.
                  if ( pStrand->GetSize() == cur_size )
                  {
                     // We just found the one we want to select.
                     new_cur_sel = sel_count;
                  }
               }
            }

            sel_count++;
         }
      }
   }

   // Attempt to re-select the strand.
   if ( 0 <= new_cur_sel )
      pList->SetCurSel( new_cur_sel );
   else
      pList->SetCurSel( pList->GetCount()-1 );
}

void CGirderSegmentStrandsPage::OnStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CDataExchange DX(this,true);
	DDX_CBIndex(&DX, IDC_STRAND_SIZE, m_StrandSizeIdx);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   Int32 key = (Int32)pList->GetItemData( m_StrandSizeIdx );

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   pSegment->Strands.StrandMaterial[pgsTypes::Straight] = pPool->GetStrand( key );
   pSegment->Strands.StrandMaterial[pgsTypes::Harped]   = pPool->GetStrand( key );

   // Now we can update pjack values in dialog
   InitPjackEdits();
}

void CGirderSegmentStrandsPage::InitPjackEdits()
{
   InitPjackEdits(IDC_SS_JACK,IDC_NUM_SS,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
   InitPjackEdits(IDC_HS_JACK,IDC_NUM_HS,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Temporary);
}

void CGirderSegmentStrandsPage::InitPjackEdits(UINT nCalcPjack,UINT nNumStrands,UINT nPjackEdit,UINT nPjackUnit,pgsTypes::StrandType strandType)
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   m_Grid.UpdateStrandData(NULL,&(pSegment->Strands));

   StrandIndexType nStrands = pSegment->Strands.GetNstrands(strandType);

   CButton* chkbox = (CButton*)GetDlgItem(nCalcPjack);
   BOOL bEnable = FALSE; // true is user input model

   if (  0 < nStrands )
      bEnable = (chkbox->GetCheck() == BST_CHECKED);

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
      pSegment->Strands.Pjack[strandType] = GetMaxPjack(nStrands);
      Pjack = pSegment->Strands.Pjack[strandType];
      DDX_UnitValueAndTag( &dx, nPjackEdit, nPjackUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }
}

BOOL CGirderSegmentStrandsPage::OnSetActive() 
{
   // make sure segment geometry is up to date with what ever has been changed during
   // this editing session
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   m_DrawStrands.CustomInit(pSegment);

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
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   m_Grid.UpdateStrandData(NULL,&(pSegment->Strands));

   CString strLabel;
   strLabel.Format(_T("Permanent Strands (%d)"),pSegment->Strands.GetNstrands(pgsTypes::Straight));
   GetDlgItem(IDC_SS_LABEL)->SetWindowText(strLabel);

   strLabel.Format(_T("Temporary Strands (%d)"),pSegment->Strands.GetNstrands(pgsTypes::Temporary));
   GetDlgItem(IDC_HS_LABEL)->SetWindowText(strLabel);

   m_DrawStrands.Invalidate();
   m_DrawStrands.UpdateWindow();

   UpdateStrandControls();
}
