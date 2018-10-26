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
#include <DesignConfigUtil.h>

#include "GirderDescDlg.h" // for ReconcileDebonding

#define NO_DEBOND_FILL_COLOR BLUE //GREY90

// CGirderSegmentStrandsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentStrandsPage, CPropertyPage)

CGirderSegmentStrandsPage::CGirderSegmentStrandsPage()
	: CPropertyPage(CGirderSegmentStrandsPage::IDD)
{
	m_bSymmetricDebond = FALSE;

}

CGirderSegmentStrandsPage::~CGirderSegmentStrandsPage()
{
}

void CGirderSegmentStrandsPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescPrestressPage)
	//}}AFX_DATA_MAP

   bool bPjackUserInput[2];
   if ( !pDX->m_bSaveAndValidate )
   {
      bPjackUserInput[pgsTypes::Harped]   = !pSegment->Strands.bPjackCalculated[pgsTypes::Harped];
      bPjackUserInput[pgsTypes::Straight] = !pSegment->Strands.bPjackCalculated[pgsTypes::Straight];
   }

   DDX_Text(pDX, IDC_NUM_HS, pSegment->Strands.Nstrands[pgsTypes::Harped]);
   DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackUserInput[pgsTypes::Harped]);

   DDX_Text(pDX, IDC_NUM_SS, pSegment->Strands.Nstrands[pgsTypes::Straight]);
   DDX_Check_Bool(pDX, IDC_SS_JACK, bPjackUserInput[pgsTypes::Straight]);

   if ( pDX->m_bSaveAndValidate )
   {
      pSegment->Strands.bPjackCalculated[pgsTypes::Harped]   = !bPjackUserInput[pgsTypes::Harped];
      pSegment->Strands.bPjackCalculated[pgsTypes::Straight] = !bPjackUserInput[pgsTypes::Straight];
   }

   if (pDX->m_bSaveAndValidate && pSegment->Strands.bPjackCalculated[pgsTypes::Harped])
   {
      pSegment->Strands.Pjack[pgsTypes::Harped] = GetMaxPjack( pSegment->Strands.Nstrands[pgsTypes::Harped] );
   }
   else
   {
      DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, pSegment->Strands.Pjack[pgsTypes::Harped], pDisplayUnits->GetGeneralForceUnit() );
   }

   if (pDX->m_bSaveAndValidate && pSegment->Strands.bPjackCalculated[pgsTypes::Straight])
   {
      pSegment->Strands.Pjack[pgsTypes::Straight] = GetMaxPjack( pSegment->Strands.Nstrands[pgsTypes::Straight] );
   }
   else
   {
      DDX_UnitValueAndTag( pDX, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, pSegment->Strands.Pjack[pgsTypes::Straight], pDisplayUnits->GetGeneralForceUnit() );
   }

   DDV_UnitValueLimitOrLess( pDX, IDC_SS_JACK_FORCE, pSegment->Strands.Pjack[pgsTypes::Straight], GetUltPjack( pSegment->Strands.Nstrands[pgsTypes::Straight] ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
   DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, pSegment->Strands.Pjack[pgsTypes::Harped],   GetUltPjack( pSegment->Strands.Nstrands[pgsTypes::Harped] ),   pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
   UpdateStrandControls();

   if (pDX->m_bSaveAndValidate)
   {
      ConfigStrandFillVector strtvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);
      ReconcileDebonding(strtvec, pSegment->Strands.Debond[pgsTypes::Straight]); 
   }

   if ( pDX->m_bSaveAndValidate )
   {
      GET_IFACE2(pBroker, IBridge,pBridge);
      Float64 gdr_length2 = pBridge->GetSegmentLength(pParent->m_SegmentKey)/2.0;

      m_Grid.GetData(*pSegment);

      std::vector<CDebondData>::iterator iter(pSegment->Strands.Debond[pgsTypes::metStart].begin());
      std::vector<CDebondData>::iterator end(pSegment->Strands.Debond[pgsTypes::metStart].end());
      for ( ; iter != end; iter++ )
      {
         CDebondData& debond_info = *iter;
         if (debond_info.Length1 >= gdr_length2 || debond_info.Length2 >= gdr_length2)
         {
            HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	         AfxMessageBox( _T("Debond length cannot exceed one half of segment length."), MB_ICONEXCLAMATION);
	         pDX->Fail();
         }
      }

      pSegment->Strands.bSymmetricDebond           = m_bSymmetricDebond ? TRUE : FALSE;
   }

	DDX_CBIndex(pDX, IDC_STRAND_SIZE, m_StrandSizeIdx);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
      Int32 key = (Int32)pList->GetItemData( m_StrandSizeIdx );
      pSegment->Strands.StrandMaterial[pgsTypes::Straight] = pPool->GetStrand( key );
      pSegment->Strands.StrandMaterial[pgsTypes::Harped]   = pPool->GetStrand( key );
   }


	DDX_Check(pDX, IDC_SYMMETRIC_DEBOND, m_bSymmetricDebond);

}


BEGIN_MESSAGE_MAP(CGirderSegmentStrandsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentStrandsPage)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUM_SS_SPIN, OnNumStraightStrandsChanged)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUM_HS_SPIN, OnNumHarpedStrandsChanged)
	ON_BN_CLICKED(IDC_SS_JACK, OnUpdateStraightStrandPjEdit)
	ON_BN_CLICKED(IDC_HS_JACK, OnUpdateHarpedStrandPjEdit)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_STRAND_SIZE, OnStrandTypeChanged)
	ON_BN_CLICKED(IDC_SYMMETRIC_DEBOND, OnSymmetricDebond)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentStrandsPage message handlers

BOOL CGirderSegmentStrandsPage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_DEBOND_GRID, this);
   m_Grid.CustomInit(m_bSymmetricDebond ? TRUE : FALSE);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

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

   OnChange();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

StrandIndexType CGirderSegmentStrandsPage::StrandSpinnerInc(IStrandGeometry* pStrands, pgsTypes::StrandType type,StrandIndexType currNum, bool bAdd )
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();

   StrandIndexType nextnum;
   if ( bAdd )
   {
      nextnum = pStrands->GetNextNumStrands(pParent->m_SegmentKey, type, currNum);

      if (nextnum == INVALID_INDEX)
         nextnum = currNum; // no increment if we hit the top
   }
   else
   {
      nextnum = pStrands->GetPrevNumStrands(pParent->m_SegmentKey, type, currNum);

      if (nextnum == INVALID_INDEX)
         nextnum = currNum; // no increment if we hit the bottom
   }

   StrandIndexType increment = nextnum - currNum;

   return increment;
}

StrandIndexType CGirderSegmentStrandsPage::OnNumStrandsChanged(UINT nCheck,UINT nEdit,UINT nUnit,pgsTypes::StrandType strandType,int iPos,int& iDelta) 
{
   // NOTE: iDelta must be by reference because it is changed in this method. The change value has to get back to the operating system
   // Usually iDelta is set to 2 or 0 to indicate that we must step by 2 or not roll over
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   StrandIndexType inc = StrandSpinnerInc( pStrandGeom, strandType, iPos, iDelta > 0 );

   iDelta = (int)inc;

   StrandIndexType nStrands = StrandIndexType(iPos + iDelta);
   BOOL bUserPjack = IsDlgButtonChecked( nCheck );
   CWnd* pWnd = GetDlgItem( nEdit );
   pWnd->EnableWindow( nStrands == 0 ? FALSE : (bUserPjack ? TRUE : FALSE) );

   Float64 Pjack;
   if ( !bUserPjack || nStrands == 0 )
   {
      Pjack = GetMaxPjack( nStrands );
      CDataExchange dx(this,FALSE);
      DDX_UnitValueAndTag( &dx, nEdit, nUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }

   return nStrands;
}

void CGirderSegmentStrandsPage::OnNumHarpedStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
   StrandIndexType nStrands = OnNumStrandsChanged(IDC_HS_JACK,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Harped,pNMUpDown->iPos,pNMUpDown->iDelta);
	*pResult = 0;


   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   pSegment->Strands.Nstrands[pgsTypes::Harped] = nStrands;
}

void CGirderSegmentStrandsPage::OnNumStraightStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
   StrandIndexType nStrands = OnNumStrandsChanged(IDC_SS_JACK,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight,pNMUpDown->iPos,pNMUpDown->iDelta);
	*pResult = 0;

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
   pSegment->Strands.Nstrands[pgsTypes::Straight] = nStrands;

   OnChange();
   UpdateGrid();


   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rect;
   pPicture->GetWindowRect(rect);
   ScreenToClient(&rect);
   InvalidateRect(rect);
   UpdateWindow();
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

void CGirderSegmentStrandsPage::OnUpdateHarpedStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_HS_JACK,IDC_NUM_HS,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Harped);
}

void CGirderSegmentStrandsPage::OnUpdateStraightStrandPjEdit()
{
   OnUpdateStrandPjEdit(IDC_SS_JACK,IDC_NUM_SS,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
}

void CGirderSegmentStrandsPage::OnUpdateStrandPjEdit(UINT nCheck,UINT nStrandEdit,UINT nForceEdit,UINT nUnit,pgsTypes::StrandType strandType)
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   BOOL bEnable = IsDlgButtonChecked( nCheck ) ? TRUE : FALSE; // user defined value if checked
   Uint16 nStrands = GetDlgItemInt( nStrandEdit );
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
	// Each time this page is activated, we need to make sure the valid range for # of
   // strands is correct (i.e. in sync with the selected girder type on the Superstructure
   // page).
   //
   // If the current number of strands exceeds the max number of strands,  set the current
   // number of strands to the max number of strands and recompute the jacking forces.
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();

   CSpinButtonCtrl* pSpin;
   UDACCEL uda;
   StrandIndexType nStrandsMax;

   // Harped Strands
   nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_Girder.GetGirderName(), pgsTypes::Harped);

   pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
   uda;
   uda.nSec=0;
   uda.nInc=1;
   pSpin->SetAccel(1,&uda);
   pSpin->SetRange( 0, short(nStrandsMax) );

   // Straight
   nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_Girder.GetGirderName(), pgsTypes::Straight);

   pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_SS_SPIN );
   uda;
   uda.nSec=0;
   uda.nInc=1;
   pSpin->SetAccel(1,&uda);
   pSpin->SetRange( 0, short(nStrandsMax) );

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
   InitPjackEdits(IDC_HS_JACK,IDC_NUM_HS,IDC_HS_JACK_FORCE,IDC_HS_JACK_FORCE_UNIT,pgsTypes::Harped);
   InitPjackEdits(IDC_SS_JACK,IDC_NUM_SS,IDC_SS_JACK_FORCE,IDC_SS_JACK_FORCE_UNIT,pgsTypes::Straight);
}

void CGirderSegmentStrandsPage::InitPjackEdits(UINT nCalcPjack,UINT nNumStrands,UINT nPjackEdit,UINT nPjackUnit,pgsTypes::StrandType strandType)
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   CButton* chkbox = (CButton*)GetDlgItem(nCalcPjack);
   BOOL bEnable = FALSE; // true is user input model
   
   Uint16 nStrands = GetDlgItemInt( nNumStrands );

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


void CGirderSegmentStrandsPage::OnSymmetricDebond() 
{
   UINT checked = IsDlgButtonChecked(IDC_SYMMETRIC_DEBOND);
   m_Grid.CanDebond(true, checked != 0 );
}

void CGirderSegmentStrandsPage::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CProperyPage::OnPaint() for painting messages

   // Draw the girder cross section and label the strand locations
   // The basic logic from this code is take from
   // Programming Microsoft Visual C++, Fifth Edition
   // Kruglinski, Shepherd, and Wingo
   // Page 129
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   CRect redit;
   pWnd->GetClientRect(&redit);
   CRgn rgn;
   VERIFY(rgn.CreateRectRgn(redit.left,redit.top,redit.right,redit.bottom));
   CDC* pDC = pWnd->GetDC();
   pDC->SelectClipRgn(&rgn);
   pWnd->Invalidate();
   pWnd->UpdateWindow();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IShapes,pShapes);
   CComPtr<IShape> shape;

   pgsPointOfInterest poi(pParent->m_SegmentKey,0.00);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(pParent->m_SegmentKey);
   pShapes->GetSegmentShape(releaseIntervalIdx,poi,false,pgsTypes::scGirder,&shape);

   CComQIPtr<IXYPosition> position(shape);
   CComPtr<IPoint2d> lp;
   position->get_LocatorPoint(lpBottomCenter,&lp);
   lp->Move(0,0);
   position->put_LocatorPoint(lpBottomCenter,lp);


   // WORLD EXTENTS ARE THE FULL HEIGHT OF THE SEGMENT
   //// Get the world height to be equal to the height of the area 
   //// occupied by the strands
   //GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   //Float64 y_min =  DBL_MAX;
   //Float64 y_max = -DBL_MAX;
   //StrandIndexType nStrands = pSegment->Strands.Nstrands[pgsTypes::Straight];

   //ConfigStrandFillVector fillvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   //PRESTRESSCONFIG config;
   //config.SetStrandFill(pgsTypes::Straight, fillvec);

   //CComPtr<IPoint2dCollection> points;
   //pStrandGeometry->GetStrandPositionsEx(pParent->m_Girder.GetGirderName(),config,pgsTypes::Straight,pgsTypes::metStart,&points);
   //for ( StrandIndexType strIdx = 0; strIdx < nStrands; strIdx++ )
   //{
   //   CComPtr<IPoint2d> point;
   //   points->get_Item(strIdx,&point);
   //   Float64 y;
   //   point->get_Y(&y);
   //   y_min = _cpp_min(y,y_min);
   //   y_max = _cpp_max(y,y_max);
   //}
   gpSize2d size;
   
   GET_IFACE2(pBroker,IGirder,pGirder);
   size.Dx() = pGirder->GetBottomWidth(poi);
   size.Dy() = pGirder->GetHeight(poi);

   //size.Dy() = (y_max - y_min);
   //if ( IsZero(size.Dy()) )
   //   size.Dy() = size.Dx()/2;

   CSize csize = redit.Size();

   CComPtr<IRect2d> box;
   shape->get_BoundingBox(&box);

   CComPtr<IPoint2d> objOrg;
   box->get_BottomCenter(&objOrg);

   gpPoint2d org;
   Float64 x,y;
   objOrg->get_X(&x);
   objOrg->get_Y(&y);
   org.X() = x;
   org.Y() = y;

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx-10,csize.cy);
   mapper.SetDeviceOrg(csize.cx/2,csize.cy-5);

   CPen solid_pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush solid_brush(SEGMENT_FILL_COLOR);

   CPen void_pen(PS_SOLID,1,VOID_BORDER_COLOR);
   CBrush void_brush(GetSysColor(COLOR_WINDOW));

   CPen* pOldPen     = pDC->SelectObject(&solid_pen);
   CBrush* pOldBrush = pDC->SelectObject(&solid_brush);

   CComQIPtr<ICompositeShape> compshape(shape);
   if ( compshape )
   {
      CollectionIndexType nShapes;
      compshape->get_Count(&nShapes);
      for ( CollectionIndexType idx = 0; idx < nShapes; idx++ )
      {
         CComPtr<ICompositeShapeItem> item;
         compshape->get_Item(idx,&item);

         CComPtr<IShape> s;
         item->get_Shape(&s);

         VARIANT_BOOL bVoid;
         item->get_Void(&bVoid);

         if ( bVoid )
         {
            pDC->SelectObject(&void_pen);
            pDC->SelectObject(&void_brush);
         }
         else
         {
            pDC->SelectObject(&solid_pen);
            pDC->SelectObject(&solid_brush);
         }

         DrawShape(pDC,s,mapper);
      }
   }
   else
   {
      DrawShape(pDC,shape,mapper);
   }

   DrawStrands(pDC,mapper);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   pWnd->ReleaseDC(pDC);
}

void CGirderSegmentStrandsPage::DrawShape(CDC* pDC,IShape* shape,grlibPointMapper& mapper)
{
   CComPtr<IPoint2dCollection> objPoints;
   shape->get_PolyPoints(&objPoints);

   CollectionIndexType nPoints;
   objPoints->get_Count(&nPoints);

   CPoint* points = new CPoint[nPoints];

   CComPtr<IPoint2d> point;
   long dx,dy;

   long i = 0;
   CComPtr<IEnumPoint2d> enumPoints;
   objPoints->get__Enum(&enumPoints);
   while ( enumPoints->Next(1,&point,NULL) != S_FALSE )
   {
      mapper.WPtoDP(point,&dx,&dy);

      points[i] = CPoint(dx,dy);

      point.Release();
      i++;
   }

   pDC->Polygon(points,(int)nPoints);

   delete[] points;
}

void CGirderSegmentStrandsPage::DrawStrands(CDC* pDC,grlibPointMapper& mapper)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   CPen strand_pen(PS_SOLID,1,STRAND_BORDER_COLOR);
   CPen no_debond_pen(PS_SOLID,1,NO_DEBOND_FILL_COLOR);
   CPen debond_pen(PS_SOLID,1,DEBOND_FILL_COLOR);
   CPen extended_pen(PS_SOLID,1,EXTENDED_FILL_COLOR);
   CPen* old_pen = (CPen*)pDC->SelectObject(&strand_pen);

   CBrush strand_brush(STRAND_FILL_COLOR);
   CBrush no_debond_brush(NO_DEBOND_FILL_COLOR);
   CBrush debond_brush(DEBOND_FILL_COLOR);
   CBrush extended_brush(EXTENDED_FILL_COLOR);
   CBrush* old_brush = (CBrush*)pDC->SelectObject(&strand_brush);

   pDC->SetTextAlign(TA_CENTER);
   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);
   CFont* old_font = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   // Draw all the strands bonded
   StrandIndexType nStrands = pSegment->Strands.Nstrands[pgsTypes::Straight];

   ConfigStrandFillVector  straightStrandFill = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   PRESTRESSCONFIG config;
   config.SetStrandFill(pgsTypes::Straight, straightStrandFill);

   CComPtr<IPoint2dCollection> points;
   pStrandGeometry->GetStrandPositionsEx(pParent->m_Girder.GetGirderName(),config,pgsTypes::Straight,pgsTypes::metStart,&points);

   CComPtr<IIndexArray> debondables;
   pStrandGeometry->ListDebondableStrands(pParent->m_Girder.GetGirderName(), straightStrandFill,pgsTypes::Straight, &debondables); 

   const int strand_size = 2;
   for ( StrandIndexType strIdx = 0; strIdx <nStrands; strIdx++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strIdx,&point);

      StrandIndexType is_debondable = 0;
      debondables->get_Item(strIdx, &is_debondable);

      LONG dx,dy;
      mapper.WPtoDP(point,&dx,&dy);

      CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

      if (is_debondable)
      {
         pDC->SelectObject(&strand_pen);
         pDC->SelectObject(&strand_brush);
      }
      else
      {
         pDC->SelectObject(&no_debond_pen);
         pDC->SelectObject(&no_debond_brush);
      }

      pDC->Ellipse(&rect);

      CString strLabel;
      strLabel.Format(_T("%d"),strIdx+1);
      pDC->TextOut(dx,dy,strLabel);
   }

   // Redraw the debonded strands
   pDC->SelectObject(&debond_pen);
   pDC->SelectObject(&debond_brush);

   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_Girder.GetGirderName());

   m_Grid.GetData(*pSegment);
   std::vector<CDebondData>::iterator debond_iter(pSegment->Strands.Debond[pgsTypes::Straight].begin());
   std::vector<CDebondData>::iterator debond_iter_end(pSegment->Strands.Debond[pgsTypes::Straight].end());
   for ( ; debond_iter != debond_iter_end; debond_iter++ )
   {
      CDebondData& debond_info = *debond_iter;

      if ( debond_info.strandTypeGridIdx == INVALID_INDEX )
      {
         ATLASSERT(0); // we should be protecting against this
         continue;
      }

      // Library entry uses grid indexing (same as debonding)
      Float64 xs, xe, ys, ye;
      bool candb;
      pGdrEntry->GetStraightStrandCoordinates( debond_info.strandTypeGridIdx, &xs, &ys, &xe, &ye, &candb);

      long dx,dy;
      mapper.WPtoDP(xs, ys, &dx,&dy);

      CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

      pDC->Ellipse(&rect);

      if ( xs > 0.0 )
      {
         mapper.WPtoDP(-xs, ys, &dx,&dy);

         CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

         pDC->Ellipse(&rect);
      }
   }


   // Redraw the extended strands
   pDC->SelectObject(&extended_pen);
   pDC->SelectObject(&extended_brush);

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      const std::vector<GridIndexType>& extStrandsStart(pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,endType));
      std::vector<GridIndexType>::const_iterator ext_iter(extStrandsStart.begin());
      std::vector<GridIndexType>::const_iterator ext_iter_end(extStrandsStart.end());
      for ( ; ext_iter != ext_iter_end; ext_iter++ )
      {
         GridIndexType gridIdx = *ext_iter;

         // Library entry uses grid indexing (same as debonding)
         Float64 xs, xe, ys, ye;
         bool candb;
         pGdrEntry->GetStraightStrandCoordinates( gridIdx, &xs, &ys, &xe, &ye, &candb);

         long dx,dy;
         mapper.WPtoDP(xs, ys, &dx,&dy);

         CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

         pDC->Ellipse(&rect);

         if ( xs > 0.0 )
         {
            mapper.WPtoDP(-xs, ys, &dx,&dy);

            CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

            pDC->Ellipse(&rect);
         }
      }
   }

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
   pDC->SelectObject(old_font);
}

LPCTSTR CGirderSegmentStrandsPage::GetGirderName()
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   return pParent->m_Girder.GetGirderName();
}

void CGirderSegmentStrandsPage::OnChange() 
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   StrandIndexType nStrands = pSegment->Strands.Nstrands[pgsTypes::Straight];
   StrandIndexType ndbs = m_Grid.GetNumDebondedStrands();
   Float64 percent = 0.0;
   if (0 < nStrands)
      percent = 100.0 * (Float64)ndbs/(Float64)nStrands;

   CString str;
   str.Format(_T("Straight=%d"), nStrands);
   CWnd* pNs = GetDlgItem(IDC_NUMSTRAIGHT);
   pNs->SetWindowText(str);

   str.Format(_T("Debonded=%d (%.1f%%)"), ndbs,percent);
   CWnd* pNdb = GetDlgItem(IDC_NUM_DEBONDED);
   pNdb->SetWindowText(str);
}

const CSegmentKey& CGirderSegmentStrandsPage::GetSegmentKey()
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   return pParent->m_SegmentKey;
}

ConfigStrandFillVector CGirderSegmentStrandsPage::ComputeStrandFillVector(pgsTypes::StrandType type)
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   return pParent->ComputeStrandFillVector(type);
}

BOOL CGirderSegmentStrandsPage::OnSetActive() 
{
   // make sure we don't have more debonded strands than total strands
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   // Completely retarded that this has to happen here instead of DoDataExchange
   // But the call order is undefined, so we don't know if there is an active grid or not
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   StrandIndexType nStrands = pSegment->Strands.Nstrands[pgsTypes::Straight];
   ConfigStrandFillVector strtvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);

   m_Debondables.Release();
   pStrandGeometry->ListDebondableStrands(pParent->m_Girder.GetGirderName(), strtvec, pgsTypes::Straight, &m_Debondables);

   // Get rid of any debonded strands that aren't filled
   ReconcileDebonding(strtvec, pSegment->Strands.Debond[pgsTypes::Straight]); 

   for ( int i = 0; i < 2; i++ )
   {
      std::vector<GridIndexType> extStrands = pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i);
      bool bChanged = ReconcileExtendedStrands(strtvec, extStrands);

      if ( bChanged )
         pSegment->Strands.SetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i,extStrands);
   }

   BOOL enab = nStrands>0 ? TRUE:FALSE;
   GetDlgItem(IDC_SYMMETRIC_DEBOND)->EnableWindow(enab);

   OnChange();
   UpdateGrid();

   return CPropertyPage::OnSetActive();
	
}

BOOL CGirderSegmentStrandsPage::OnKillActive()
{
   this->SetFocus();  // prevents artifacts from grid list controls (not sure why)

   return CPropertyPage::OnKillActive();
}

void CGirderSegmentStrandsPage::UpdateGrid()
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   m_Grid.FillGrid(*pSegment);
   m_Grid.SelectRange(CGXRange().SetTable(), FALSE);
}
