///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// BridgeDescGeneralPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSpliceDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescGeneralPage.h"
#include "BridgeDescDlg.h"
#include "Hints.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DistFactorEngineer.h>

#include <EAF\EAFMainFrame.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescGeneralPage property page

IMPLEMENT_DYNCREATE(CBridgeDescGeneralPage, CPropertyPage)

CBridgeDescGeneralPage::CBridgeDescGeneralPage() : CPropertyPage(CBridgeDescGeneralPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_MinGirderCount = 2;
   m_CacheGirderConnectivityIdx = CB_ERR;
   m_CacheDeckTypeIdx = CB_ERR;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_GirderSpacingTolerance = pow(10.0,-pDisplayUnits->GetXSectionDimUnit().Precision);

   m_bSetActive = false;
}

CBridgeDescGeneralPage::~CBridgeDescGeneralPage()
{
}

void CBridgeDescGeneralPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeDescGeneralPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_NUMGDR_SPIN, m_NumGdrSpinner);
   DDX_Control(pDX, IDC_ALIGNMENTOFFSET_FMT, m_AlignmentOffsetFormat);
	//}}AFX_DATA_MAP

   ////////////////////////////////////////////////
   // Girders
   ////////////////////////////////////////////////
   if (!pDX->m_bSaveAndValidate)
   {
      FillGirderSpacingMeasurementComboBox();
   }


   DDX_Check_Bool(pDX, IDC_SAME_NUM_GIRDERLINES, m_bSameNumberOfGirders);
   DDX_Check_Bool(pDX, IDC_SAME_GIRDERNAME,      m_bSameGirderName);

   DDX_CBStringExactCase(pDX, IDC_BEAM_FAMILIES, m_GirderFamilyName );
   DDX_CBStringExactCase(pDX, IDC_GDR_TYPE, m_GirderName );
   DDX_CBItemData(pDX, IDC_GIRDER_ORIENTATION,  m_GirderOrientation);
   DDX_CBItemData(pDX, IDC_GIRDER_SPACING_TYPE, m_GirderSpacingType);

   
   if ( m_bSameNumberOfGirders )
   {
      DDX_Text(pDX, IDC_NUMGDR, m_nGirders );
      DDV_MinMaxLongLong(pDX, m_nGirders, m_MinGirderCount, MAX_GIRDERS_PER_SPAN );
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      // girder spacing
      DDX_Tag(pDX,IDC_SPACING_UNIT,pDisplayUnits->GetXSectionDimUnit());
      if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && (m_GirderSpacingType == pgsTypes::sbsUniform || m_GirderSpacingType == pgsTypes::sbsConstantAdjacent)) )
      {
         DDX_UnitValueAndTag(pDX,IDC_SPACING,IDC_SPACING_UNIT,m_GirderSpacing,pDisplayUnits->GetXSectionDimUnit());
      }
   }
   else
   {
      // joint spacing
      DDX_Tag(pDX,IDC_SPACING_UNIT,pDisplayUnits->GetComponentDimUnit());
      if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && m_GirderSpacingType == pgsTypes::sbsUniformAdjacent) )
      {
         DDX_UnitValueAndTag(pDX,IDC_SPACING,IDC_SPACING_UNIT,m_GirderSpacing,pDisplayUnits->GetComponentDimUnit());
      }
   }

   if ( m_GirderSpacingType == pgsTypes::sbsUniform || m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
   {
      // check girder spacing
      if ( IsEqual(m_GirderSpacing,m_MinGirderSpacing,m_GirderSpacingTolerance) )
      {
         m_GirderSpacing = m_MinGirderSpacing;
      }

      if ( IsEqual(m_GirderSpacing,m_MaxGirderSpacing,m_GirderSpacingTolerance) )
      {
         m_GirderSpacing = m_MaxGirderSpacing;
      }

      DDV_UnitValueLimitOrMore(pDX, IDC_SPACING, m_GirderSpacing, m_MinGirderSpacing, pDisplayUnits->GetXSectionDimUnit() );
      DDV_UnitValueLimitOrLess(pDX, IDC_SPACING, m_GirderSpacing, m_MaxGirderSpacing, pDisplayUnits->GetXSectionDimUnit() );
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      // check joint spacing
      DDV_UnitValueLimitOrLess(pDX, IDC_SPACING, m_GirderSpacing, m_MaxGirderSpacing-m_MinGirderSpacing, pDisplayUnits->GetComponentDimUnit() );
   }

   if ( pDX->m_bSaveAndValidate )
   {
      DWORD dw;
      DDX_CBItemData(pDX, IDC_GIRDER_SPACING_MEASURE, dw);
      UnhashGirderSpacing(dw,&m_GirderSpacingMeasurementLocation,&m_GirderSpacingMeasurementType);
   }
   else
   {
      DWORD dw = HashGirderSpacing(m_GirderSpacingMeasurementLocation,m_GirderSpacingMeasurementType);
      DDX_CBItemData(pDX, IDC_GIRDER_SPACING_MEASURE, dw);
   }

   DDX_CBItemData(pDX,IDC_REF_GIRDER,m_RefGirderIdx);
   DDX_OffsetAndTag(pDX,IDC_REF_GIRDER_OFFSET,IDC_REF_GIRDER_OFFSET_UNIT,m_RefGirderOffset,pDisplayUnits->GetXSectionDimUnit() );
   DDX_CBItemData(pDX,IDC_REF_GIRDER_OFFSET_TYPE,m_RefGirderOffsetType);

   ////////////////////////////////////////////////
   // Deck and Railing System
   ////////////////////////////////////////////////
	DDX_CBItemData(pDX, IDC_DECK_TYPE, m_Deck.DeckType);
 	DDX_CBItemData(pDX, IDC_GIRDER_CONNECTIVITY, m_Deck.TransverseConnectivity);

   if ( pDX->m_bSaveAndValidate )
   {
      UpdateBridgeDescription();
   }

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   if ( !pDX->m_bSaveAndValidate )
   {
      m_AlignmentOffset = pParent->m_BridgeDesc.GetAlignmentOffset();
   }

   DDX_OffsetAndTag(pDX, IDC_ALIGNMENTOFFSET, IDC_ALIGNMENTOFFSET_UNIT, m_AlignmentOffset, pDisplayUnits->GetAlignmentLengthUnit() );
   
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.SetAlignmentOffset(m_AlignmentOffset);
   }

}


BEGIN_MESSAGE_MAP(CBridgeDescGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescGeneralPage)
	ON_BN_CLICKED(IDC_SAME_NUM_GIRDERLINES, OnSameNumGirders)
	ON_BN_CLICKED(IDC_SAME_GIRDERNAME, OnSameGirderName)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUMGDR_SPIN, OnNumGirdersChanged)
	ON_CBN_SELCHANGE(IDC_BEAM_FAMILIES, OnGirderFamilyChanged)
	ON_CBN_SELCHANGE(IDC_GDR_TYPE, OnGirderNameChanged)
   ON_CBN_DROPDOWN(IDC_GDR_TYPE,OnBeforeChangeGirderName)
	ON_CBN_SELCHANGE(IDC_DECK_TYPE, OnDeckTypeChanged)
	ON_CBN_SELCHANGE(IDC_GIRDER_CONNECTIVITY, OnGirderConnectivityChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER_SPACING_MEASURE,OnSpacingDatumChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER_SPACING_TYPE,OnGirderSpacingTypeChanged)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescGeneralPage message handlers

BOOL CBridgeDescGeneralPage::OnInitDialog() 
{
   EnableToolTips(TRUE);

   Init();

   FillGirderFamilyComboBox();

   FillGirderNameComboBox();

   FillGirderOrientationComboBox();

   FillGirderSpacingTypeComboBox();

   FillDeckTypeComboBox();

   FillGirderConnectivityComboBox();

   FillRefGirderOffsetTypeComboBox();
   
   FillRefGirderComboBox();

   UpdateGirderSpacingLimits();

   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      EnableGirderSpacing(FALSE,FALSE);
   }

	CPropertyPage::OnInitDialog();
   
   m_NumGdrSpinner.SetRange32((int)m_MinGirderCount,(int)MAX_GIRDERS_PER_SPAN);

   UDACCEL accel[2];
   accel[0].nInc = 1;
   accel[0].nSec = 5;
   accel[1].nInc = 5;
   accel[1].nSec = 5;
   m_NumGdrSpinner.SetAccel(2,accel);

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      //// hide the edit controls not used for spliced girder structures
      //GetDlgItem(IDC_GDR_TYPE_LABEL)->ShowWindow(SW_HIDE);
      //GetDlgItem(IDC_GDR_TYPE)->ShowWindow(SW_HIDE);
      //GetDlgItem(IDC_SAME_GIRDERNAME)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_SAME_NUM_GIRDERLINES)->SetWindowText(_T("Use same number of girders in all groups"));
   }

   CString fmt;
   fmt.LoadString( IDS_ALIGNMENTOFFSET_FMT);
   m_AlignmentOffsetFormat.SetWindowText(fmt);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescGeneralPage::Init()
{
   // initialize the page data members with values from the bridge model
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   m_bSameNumberOfGirders = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups();
   m_bSameGirderName      = pParent->m_BridgeDesc.UseSameGirderForEntireBridge();
   m_nGirders = pParent->m_BridgeDesc.GetGirderGroup((GroupIndexType)0)->GetGirderCount();

   m_strCacheNumGirders.Format(_T("%d"),m_nGirders);

   m_GirderFamilyName = pParent->m_BridgeDesc.GetGirderFamilyName();

   if ( m_bSameGirderName )
   {
      m_GirderName = pParent->m_BridgeDesc.GetGirderName();
   }
   else
   {
      ASSERT(1 <= pParent->m_BridgeDesc.GetGirderGroup((GroupIndexType)0)->GetGirderTypeGroupCount());
      m_GirderName = pParent->m_BridgeDesc.GetGirderGroup((GroupIndexType)0)->GetGirderName(0);
   }
   UpdateGirderFactory();

   m_GirderOrientation = pParent->m_BridgeDesc.GetGirderOrientation();
   m_GirderSpacingType = pParent->m_BridgeDesc.GetGirderSpacingType();

   if ( IsBridgeSpacing(m_GirderSpacingType) )
   {
      m_GirderSpacing                    = pParent->m_BridgeDesc.GetGirderSpacing();
      m_GirderSpacingMeasurementLocation = pParent->m_BridgeDesc.GetMeasurementLocation();
      m_GirderSpacingMeasurementType     = pParent->m_BridgeDesc.GetMeasurementType();

      m_RefGirderIdx        = pParent->m_BridgeDesc.GetRefGirder();
      m_RefGirderOffset     = pParent->m_BridgeDesc.GetRefGirderOffset();
      m_RefGirderOffsetType = pParent->m_BridgeDesc.GetRefGirderOffsetType();

   }
   else
   {
      const CGirderGroupData* pGroup = pParent->m_BridgeDesc.GetGirderGroup(GroupIndexType(0));
      const CGirderSpacing2* pSpacing = pGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);

      m_GirderSpacing                    = pSpacing->GetGirderSpacing(0);
      m_GirderSpacingMeasurementLocation = pSpacing->GetMeasurementLocation();
      m_GirderSpacingMeasurementType     = pSpacing->GetMeasurementType();

      m_RefGirderIdx        = pSpacing->GetRefGirder();
      m_RefGirderOffset     = pSpacing->GetRefGirderOffset();
      m_RefGirderOffsetType = pSpacing->GetRefGirderOffsetType();
   }

   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      m_strCacheGirderSpacing.Format(_T("%s"),FormatDimension(m_GirderSpacing,pDisplayUnits->GetXSectionDimUnit(), false));
      m_strCacheJointSpacing.Format( _T("%s"),FormatDimension(0,              pDisplayUnits->GetComponentDimUnit(),false));
   }
   else
   {
      m_strCacheGirderSpacing.Format(_T("%s"),FormatDimension(m_MinGirderSpacing,pDisplayUnits->GetXSectionDimUnit(), false));
      m_strCacheJointSpacing.Format( _T("%s"),FormatDimension(m_GirderSpacing,   pDisplayUnits->GetComponentDimUnit(),false));
   }

   int sign = ::Sign(m_RefGirderOffset);
   LPTSTR strOffset = (sign == 0 ? _T("") : sign < 0 ? _T("L") : _T("R"));
   m_strCacheRefGirderOffset.Format(_T("%s %s"),FormatDimension(fabs(m_RefGirderOffset),pDisplayUnits->GetXSectionDimUnit(),false),strOffset);

   m_Deck = *pParent->m_BridgeDesc.GetDeckDescription();
   m_CacheDeckEdgePoints = m_Deck.DeckEdgePoints;

   UpdateMinimumGirderCount();
}

void CBridgeDescGeneralPage::UpdateBridgeDescription()
{
   // put the page data values into the bridge model
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups( m_bSameNumberOfGirders );
   pParent->m_BridgeDesc.UseSameGirderForEntireBridge( m_bSameGirderName );
   pParent->m_BridgeDesc.SetGirderCount(m_nGirders);


   bool bNewGirderFamily = (pParent->m_BridgeDesc.GetGirderFamilyName() != m_GirderFamilyName);

   pParent->m_BridgeDesc.SetGirderFamilyName(m_GirderFamilyName);
   pParent->m_BridgeDesc.SetGirderOrientation(m_GirderOrientation);

   if ( m_bSameGirderName || bNewGirderFamily )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2( pBroker, ILibrary, pLib );
   
      const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(m_GirderName);
      pParent->m_BridgeDesc.SetGirderLibraryEntry(pGdrEntry); // must do this before SetGirderName
      pParent->m_BridgeDesc.SetGirderName(m_GirderName);
   }

   if ( bNewGirderFamily )
   {
      UpdateGirderSpacingLimits();
   }

   pParent->m_BridgeDesc.SetGirderSpacingType(m_GirderSpacingType);
   pParent->m_BridgeDesc.SetGirderSpacing(m_GirderSpacing);

   pParent->m_BridgeDesc.SetMeasurementLocation(m_GirderSpacingMeasurementLocation);
   pParent->m_BridgeDesc.SetMeasurementType(m_GirderSpacingMeasurementType);

   pParent->m_BridgeDesc.SetRefGirder(m_RefGirderIdx);
   pParent->m_BridgeDesc.SetRefGirderOffset(m_RefGirderOffset);
   pParent->m_BridgeDesc.SetRefGirderOffsetType(m_RefGirderOffsetType);

   *pParent->m_BridgeDesc.GetDeckDescription() = m_Deck;

   if ( bNewGirderFamily )
   {
      pParent->m_BridgeDesc.CopyDown(true,true,true,true,true);
   }
}

void CBridgeDescGeneralPage::OnSameNumGirders() 
{
	CButton* pBtn = (CButton*)GetDlgItem(IDC_SAME_NUM_GIRDERLINES);

   // grab the new state of the check box
   m_bSameNumberOfGirders = (pBtn->GetCheck() == BST_CHECKED ? true : false);

   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_NUMGDR);
   pEdit->SetWindowText(m_strCacheNumGirders);

   CString strText; // hint dialog text
   if ( m_bSameNumberOfGirders )
   {
      // box was just checked so restore the number of girders from the cache
      CEAFDocument* pDoc = EAFGetDocument();
      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         strText = CString(_T("By checking this box, all girder groups will have the same number of girders."));
      }
      else
      {
         strText = CString(_T("By checking this box, all spans will have the same number of girders."));
      }
   }
   else
   {
      // box was just unchecked so cache the number of girders
      CEAFDocument* pDoc = EAFGetDocument();
#pragma Reminder("UPDATE: confirm this instructional message is accurate")
      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         strText = CString(_T("By unchecking this box, each girder group can have a different number of girders. To define the number of girders in a girder group, edit the Group Details for each group.\n\nGroup Details can be edited by selecting the Layout tab and then pressing the edit button for a group."));
      }
      else
      {
         strText = CString(_T("By unchecking this box, each span can have a different number of girders. To define the number of girders in a span, edit the Span Details for each span.\n\nSpan Details can be edited by selecting the Layout tab and then pressing the edit button for a span."));
      }
   }
   // enable/disable the associated controls
   EnableNumGirderLines(m_bSameNumberOfGirders);

   UpdateBridgeDescription();

   // Show the hint dialog
   UIHint(strText,UIHINT_SAME_NUMBER_OF_GIRDERS);
}

void CBridgeDescGeneralPage::EnableNumGirderLines(BOOL bEnable)
{
   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_NUMGDR);
   pEdit->EnableWindow(bEnable);

   if ( bEnable )
   {
      CString strNum;
      strNum.Format(_T("%d"),m_nGirders);
      pEdit->SetWindowText(strNum);
   }
   else
   {
      pEdit->SetWindowText(_T(""));
   }
   
   GetDlgItem(IDC_NUMGDR_SPIN)->EnableWindow(bEnable);
}

void CBridgeDescGeneralPage::OnSameGirderName() 
{
	CButton* pBtn = (CButton*)GetDlgItem(IDC_SAME_GIRDERNAME);

   m_bSameGirderName = (pBtn->GetCheck() == BST_CHECKED ? true : false);

   CComboBox* pcbGirderName = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);
   CString strText;
   if ( m_bSameGirderName )
   {
      // button was just checked
      pcbGirderName->SetCurSel(m_CacheGirderNameIdx);
      strText = CString(_T("By checking this box, the same girder type will be used throughout the bridge."));
   }
   else
   {
      // was was just unchecked
      m_CacheGirderNameIdx = pcbGirderName->GetCurSel();
      CEAFDocument* pDoc = EAFGetDocument();
#pragma Reminder("UPDATE: confirm this instructional message is accurate")
      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         strText = CString(_T("By unchecking this box, a different girder type can be assigned to each girder. To do this, edit the Group Details for each group.\n\nGroup Details can be edited by selecting the Layout tab and then pressing the edit button for a group."));
      }
      else
      {
         strText = CString(_T("By unchecking this box, a different girder type can be assigned to each girder. To do this, edit the Span Details for each span.\n\nSpan Details can be edited by selecting the Layout tab and then pressing the edit button for a span."));
      }
   }

   EnableGirderName(m_bSameGirderName);
   UpdateGirderSpacingLimits();

   UpdateBridgeDescription();

   // Show the hint dialog
   UIHint(strText,UIHINT_SAME_GIRDER_NAME);
}

void CBridgeDescGeneralPage::EnableGirderName(BOOL bEnable)
{
   CComboBox* pcbGirderName        = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);

   pcbGirderName->EnableWindow(bEnable);

   if ( !bEnable )
   {
      pcbGirderName->SetCurSel(-1);
   }
}

void CBridgeDescGeneralPage::EnableGirderSpacing(BOOL bEnable,BOOL bClearControls)
{
   CWnd* pLabel = GetDlgItem(IDC_GIRDER_SPACING_LABEL);
   pLabel->EnableWindow(bEnable);

   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SPACING);
   pEdit->EnableWindow(bEnable);

   CWnd* pTag = GetDlgItem(IDC_SPACING_UNIT);
   pTag->EnableWindow(bEnable);
   pTag->ShowWindow(bClearControls ? SW_HIDE : SW_SHOW);

   CWnd* pAllowable = GetDlgItem(IDC_ALLOWABLE_SPACING);
   pAllowable->EnableWindow(bEnable);
   pAllowable->ShowWindow(bClearControls ? SW_HIDE : SW_SHOW);

   if ( !bEnable && bClearControls )
   {
      pEdit->SetSel(0,-1);
      pEdit->Clear();
   }
}

void CBridgeDescGeneralPage::OnNumGirdersChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   // this is what the count will be
   int new_count = pNMUpDown->iPos + pNMUpDown->iDelta;

   // if it goes over the top
   if ( MAX_GIRDERS_PER_SPAN < new_count )
   {
      new_count = (int)m_MinGirderCount; // make it the min value
   }

   // if it goes under the bottom
   if ( new_count < int(m_MinGirderCount) )
   {
      new_count = MAX_GIRDERS_PER_SPAN; // make it the max value
   }

   m_nGirders = new_count;

   // disable girder spacing if girder count is 1
   // this could happen for U-beams, Voided Slabs, Triple-Tees, or other multi-web beams
   if ( IsBridgeSpacing(m_GirderSpacingType) )
   {
      EnableGirderSpacing(new_count != 1,FALSE);
   }

   *pResult = 0;

   FillRefGirderComboBox();
   UpdateBridgeDescription();
}

void CBridgeDescGeneralPage::FillGirderFamilyComboBox()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;

   CComboBox* pcbGirderFamilies = (CComboBox*)GetDlgItem(IDC_BEAM_FAMILIES);
   std::vector<std::_tstring> girderFamilyNames;
   pLibNames->EnumGirderFamilyNames(&girderFamilyNames);

   for ( iter = girderFamilyNames.begin(); iter != girderFamilyNames.end(); iter++ )
   {
      std::_tstring strGirderFamilyName = *iter;
      pLibNames->EnumGirderNames(strGirderFamilyName.c_str(), &names );

      // only add a girder family name into the combo box if the library
      // has girders of this type
      if ( names.size() != 0 )
      {
         pcbGirderFamilies->AddString(strGirderFamilyName.c_str());
      }
   }

   int curSel = pcbGirderFamilies->FindStringExact(-1,m_GirderFamilyName);
   pcbGirderFamilies->SetCurSel(curSel);
}

void CBridgeDescGeneralPage::FillGirderNameComboBox()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   
   CComboBox* pcbGirders = (CComboBox*)GetDlgItem( IDC_GDR_TYPE );
   pcbGirders->ResetContent();

   int curSel = CB_ERR;
   pLibNames->EnumGirderNames(m_GirderFamilyName, &names );

   std::vector<std::_tstring>::iterator iter(names.begin());
   std::vector<std::_tstring>::iterator iterEnd(names.end());
   for ( ; iter != iterEnd; iter++ )
   {
      std::_tstring& name = *iter;

      int idx = pcbGirders->AddString( name.c_str() );
      if ( CString(name.c_str()) == m_GirderName )
      {
         curSel = idx;
      }
   }

   m_CacheGirderNameIdx = (curSel == CB_ERR ? 0 : curSel);
   pcbGirders->SetCurSel(m_CacheGirderNameIdx);
}

void CBridgeDescGeneralPage::FillGirderOrientationComboBox()
{
   // Girder Orientation
   CComboBox* pOrientation = (CComboBox*)GetDlgItem(IDC_GIRDER_ORIENTATION);
   int idx = pOrientation->AddString(_T("Plumb")); 
   pOrientation->SetItemData(idx,pgsTypes::Plumb);

   idx = pOrientation->AddString(_T("Normal to roadway at Start of Bridge"));
   pOrientation->SetItemData(idx,pgsTypes::StartNormal);

   idx = pOrientation->AddString(_T("Normal to roadway at Mid-span of Bridge"));
   pOrientation->SetItemData(idx,pgsTypes::MidspanNormal);

   idx = pOrientation->AddString(_T("Normal to roadway at End of Bridge"));
   pOrientation->SetItemData(idx,pgsTypes::EndNormal);

   int curSel;
   switch( m_GirderOrientation )
   {
   case pgsTypes::Plumb:
      curSel = 0;
      break;

   case pgsTypes::StartNormal:
      curSel = 1;
      break;

   case pgsTypes::MidspanNormal:
      curSel = 2;
      break;

   case pgsTypes::EndNormal:
      curSel = 3;
      break;

   default:
      ASSERT(FALSE); // shouldn't get here
   }

   pOrientation->SetCurSel(curSel);
}

void CBridgeDescGeneralPage::FillGirderSpacingTypeComboBox()
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_TYPE);
   pSpacingType->ResetContent();

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CEAFDocument* pDoc = EAFGetDocument();

   pgsTypes::SupportedBeamSpacings sbs = m_Factory->GetSupportedBeamSpacings();
   pgsTypes::SupportedBeamSpacings::iterator iter;
   int idx;
   int curSel = CB_ERR;
   for ( iter = sbs.begin(); iter != sbs.end(); iter++ )
   {
      pgsTypes::SupportedBeamSpacing spacingType = *iter;

      switch( spacingType )
      {
      case pgsTypes::sbsUniform:
         idx = pSpacingType->AddString(_T("Same spacing for all girders"));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsGeneral:
         if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
         {
            idx = pSpacingType->AddString(_T("Unique spacing for each precast segment"));
         }
         else
         {
            idx = pSpacingType->AddString(_T("Unique spacing for each girder"));
         }
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsUniformAdjacent:
         if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
         {
            idx = pSpacingType->AddString(_T("Adjacent girders with same joint spacing in all groups"));
         }
         else
         {
            idx = pSpacingType->AddString(_T("Adjacent girders with same joint spacing in all spans"));
         }
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsGeneralAdjacent:
         if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
         {
            idx = pSpacingType->AddString(_T("Adjacent girders with unique joint spacing for each group"));
         }
         else
         {
            idx = pSpacingType->AddString(_T("Adjacent girders with unique joint spacing for each span"));
         }
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsConstantAdjacent:
         idx = pSpacingType->AddString(_T("Adjacent girders with the same width for all girders"));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      default:
         ATLASSERT(false); // is there a new spacing type???
      }

      if ( m_GirderSpacingType == spacingType )
      {
         curSel = idx;
      }
   }

   if ( curSel == CB_ERR )
   {
      pSpacingType->SetCurSel(0);
      m_GirderSpacingType = sbs.front();
   }
   else
   {
      pSpacingType->SetCurSel(curSel);
      m_GirderSpacingType = sbs[curSel];
   }

   // update the unit tag that goes with the spacing input box
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CDataExchange dx(this,FALSE);
   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      DDX_Tag(&dx,IDC_SPACING_UNIT,pDisplayUnits->GetXSectionDimUnit());
   }
   else
   {
      ASSERT(IsJointSpacing(m_GirderSpacingType));
      DDX_Tag(&dx,IDC_SPACING_UNIT,pDisplayUnits->GetComponentDimUnit());
   }
}

void CBridgeDescGeneralPage::FillGirderSpacingMeasurementComboBox()
{
   m_CacheGirderSpacingMeasureIdx = 0;

   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_MEASURE);
   pSpacingType->ResetContent();

   DWORD current_value = HashGirderSpacing(m_GirderSpacingMeasurementLocation,m_GirderSpacingMeasurementType);

   int idx = pSpacingType->AddString(_T("Measured at and along the abutment/pier line"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   m_CacheGirderSpacingMeasureIdx = (item_data == current_value ? idx : m_CacheGirderSpacingMeasureIdx );
   
   idx = pSpacingType->AddString(_T("Measured normal to alignment at abutment/pier line"));
   item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem);
   pSpacingType->SetItemData(idx,item_data);
   m_CacheGirderSpacingMeasureIdx = (item_data == current_value ? idx : m_CacheGirderSpacingMeasureIdx );

   // Cannot measure along CL Bearing if any bearing offsets are measured along girder
   // If the girders are not parallel, then there is not a single unique bearing line.
   if (!AreAnyBearingsMeasuredAlongGirder())
   {
      idx = pSpacingType->AddString(_T("Measured at and along the CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem);
      pSpacingType->SetItemData(idx,item_data);
      m_CacheGirderSpacingMeasureIdx = (item_data == current_value ? idx : m_CacheGirderSpacingMeasureIdx );

      idx = pSpacingType->AddString(_T("Measured normal to alignment at CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem);
      pSpacingType->SetItemData(idx,item_data);
      m_CacheGirderSpacingMeasureIdx = (item_data == current_value ? idx : m_CacheGirderSpacingMeasureIdx );
   }
}

bool CBridgeDescGeneralPage::AreAnyBearingsMeasuredAlongGirder()
{
   bool test=false;
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   SpanIndexType nSpans   = pParent->m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CSpanData2* pSpan = pParent->m_BridgeDesc.GetSpan(spanIdx);
      const CPierData2* pPrevPier = pSpan->GetPrevPier();
      const CPierData2* pNextPier = pSpan->GetNextPier();

      Float64 offset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType mbs, mbe;
      pPrevPier->GetBearingOffset(pgsTypes::Ahead,&offset,&mbs);
      pNextPier->GetBearingOffset(pgsTypes::Back, &offset,&mbe);

      if (mbs == ConnectionLibraryEntry::AlongGirder || mbe == ConnectionLibraryEntry::AlongGirder )
      {
         test = true;
         break;
      }
   }

   return test;
}

void CBridgeDescGeneralPage::FillDeckTypeComboBox()
{
   pgsTypes::SupportedDeckTypes deckTypes = m_Factory->GetSupportedDeckTypes(m_GirderSpacingType);

   CComboBox* pcbDeck = (CComboBox*)GetDlgItem(IDC_DECK_TYPE);
   int cursel = pcbDeck->GetCurSel();
   pgsTypes::SupportedDeckType deckType;
   if ( cursel != CB_ERR )
   {
      deckType = (pgsTypes::SupportedDeckType)pcbDeck->GetItemData(cursel);
   }
   else
   {
      deckType = m_Deck.DeckType;
   }

   pcbDeck->ResetContent();

   cursel = CB_ERR;
   int selidx = 0;
   pgsTypes::SupportedDeckTypes::iterator iter;
   for ( iter = deckTypes.begin(); iter != deckTypes.end(); iter++ )
   {
      CString typestr = GetDeckString(*iter);

      selidx = pcbDeck->AddString(typestr);

      pcbDeck->SetItemData(selidx,(DWORD)*iter);

      if ( *iter == deckType )
      {
         cursel = selidx;
      }
   }

   if ( cursel != CB_ERR )
   {
      pcbDeck->SetCurSel(cursel);
   }
   else
   {
      pcbDeck->SetCurSel(0);
      m_Deck.DeckType = deckTypes.front();
      OnDeckTypeChanged();
   }
}

void CBridgeDescGeneralPage::FillRefGirderOffsetTypeComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE);
   int idx = pCB->AddString(_T("Alignment"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtAlignment);

   idx = pCB->AddString(_T("Bridge Line"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtBridge);
}

void CBridgeDescGeneralPage::FillRefGirderComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REF_GIRDER);
   int curSel = pCB->GetCurSel();
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Center of Girders"));
   pCB->SetItemData(idx,INVALID_INDEX);

   for ( GirderIndexType i = 0; i < m_nGirders; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD)i);
   }

   pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel);
}

void CBridgeDescGeneralPage::FillGirderConnectivityComboBox()
{
   CComboBox* pcbConnectivity = (CComboBox*)GetDlgItem(IDC_GIRDER_CONNECTIVITY);
   int cursel = pcbConnectivity->GetCurSel();
   pcbConnectivity->ResetContent();

   bool bConnectivity = (m_GirderSpacingType == pgsTypes::sbsUniformAdjacent || m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent || m_GirderSpacingType == pgsTypes::sbsConstantAdjacent);
   if ( bConnectivity )
   {
      int idx = pcbConnectivity->AddString(_T("Sufficient to make girders act as a unit"));
      pcbConnectivity->SetItemData(idx,pgsTypes::atcConnectedAsUnit);

      idx =     pcbConnectivity->AddString(_T("Prevents relative vertical displacement"));
      pcbConnectivity->SetItemData(idx,pgsTypes::atcConnectedRelativeDisplacement);

      if ( cursel != CB_ERR ) // if there was a previous selection
      {
         pcbConnectivity->SetCurSel(cursel);
      }
      else
      {
         pcbConnectivity->SetCurSel(0); /// just select index 0
      }
   }
}

void CBridgeDescGeneralPage::UpdateGirderFactory()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   m_Factory.Release();
   pLibNames->GetBeamFactory(std::_tstring(m_GirderFamilyName),std::_tstring(m_GirderName),&m_Factory);
}

CString CBridgeDescGeneralPage::GetDeckString(pgsTypes::SupportedDeckType deckType)
{
   switch ( deckType )
   {
      case pgsTypes::sdtCompositeCIP:
         return CString(_T("Composite Cast-In-Place Deck"));

      case pgsTypes::sdtCompositeSIP:
         return CString(_T("Composite Stay-In-Place Deck Panels"));

      case pgsTypes::sdtCompositeOverlay:
         return CString(_T("Composite Cast-In-Place Overlay"));

      case pgsTypes::sdtNone:
         return CString(_T("No deck"));
   }

   ATLASSERT(false);
   return CString(_T("Unknown Deck Type"));
}

void CBridgeDescGeneralPage::OnGirderFamilyChanged() 
{
   CComboBox* pBeamFamilies = (CComboBox*)GetDlgItem(IDC_BEAM_FAMILIES);
   int curSel = pBeamFamilies->GetCurSel();

   if ( curSel == CB_ERR )
   {
      ASSERT(false); // this shouldn't happen
      return; // just return if we are in a release build
   }

   pBeamFamilies->GetLBText(curSel,m_GirderFamilyName);

   InitGirderName();          // sets the current girder name to the first girder of the family
   UpdateGirderFactory();     // gets the new factory for this girder family
   FillGirderNameComboBox();  // fills the girder name combo box with girders from this family
   FillGirderSpacingTypeComboBox(); // get new spacing options for this girder family
   FillDeckTypeComboBox();          // set deck type options to match this girder family
   UpdateGirderConnectivity();      // fills the combo box and enables/disables

   UpdateBridgeDescription();

   if ( !UpdateGirderSpacingLimits() || m_NumGdrSpinner.GetPos() == 1)
   {
      EnableGirderSpacing(FALSE,FALSE);
   }
   else
   {
      EnableGirderSpacing(TRUE,FALSE);
   }
}

void CBridgeDescGeneralPage::UpdateMinimumGirderCount()
{
   m_MinGirderCount = m_Factory->GetMinimumBeamCount();

   if ( m_NumGdrSpinner.GetSafeHwnd() != NULL )
   {
      m_NumGdrSpinner.SetRange(short(m_MinGirderCount),MAX_GIRDERS_PER_SPAN);
   }
}

void CBridgeDescGeneralPage::OnBeforeChangeGirderName()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);
   m_GirderNameIdx = pCB->GetCurSel();
}

void CBridgeDescGeneralPage::OnGirderNameChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);
   int result = AfxMessageBox(_T("Changing the girder type will reset the strands, stirrups, and longitudinal rebar to default values.\n\nIs that OK?"),MB_YESNO);
   if ( result == IDNO )
   {
      pCB->SetCurSel((int)m_GirderNameIdx);
      return;
   }

   CComboBox* pCBGirders = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);
   int sel = pCBGirders->GetCurSel();
   pCBGirders->GetLBText(sel,m_GirderName);

   UpdateGirderFactory();

   UpdateMinimumGirderCount();


   UpdateBridgeDescription();

   if ( !UpdateGirderSpacingLimits() || m_NumGdrSpinner.GetPos() == 1)
   {
      EnableGirderSpacing(FALSE,FALSE);
   }
   else
   {
      EnableGirderSpacing(TRUE,FALSE);
   }


   UpdateSuperstructureDescription();
   FillDeckTypeComboBox();
}

void CBridgeDescGeneralPage::OnGirderConnectivityChanged() 
{
   UpdateSuperstructureDescription();

   UpdateBridgeDescription();

   UpdateSuperstructureDescription();
}

void CBridgeDescGeneralPage::OnSpacingDatumChanged()
{
   UpdateGirderSpacingLimits();

   UpdateBridgeDescription();

   UpdateSuperstructureDescription();
}

void CBridgeDescGeneralPage::OnGirderSpacingTypeChanged() 
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CComboBox* pcbSpacingType = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_TYPE);
   int curSel = pcbSpacingType->GetCurSel();
   m_GirderSpacingType = (pgsTypes::SupportedBeamSpacing)(pcbSpacingType->GetItemData(curSel));

   FillDeckTypeComboBox(); // update the available deck types for this spacing type
                           // this must be done before the girder spacing limits are determined

   FillGirderConnectivityComboBox();

   BOOL specify_spacing = UpdateGirderSpacingLimits();
   // spacing only needs to be specified if it can take on multiple values

   CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_MEASURE);

   CString strWndTxt;
   BOOL bEnable = FALSE;
   switch ( m_GirderSpacingType )
   {
      case pgsTypes::sbsUniform:
      case pgsTypes::sbsUniformAdjacent:
      case pgsTypes::sbsConstantAdjacent:

         // girder spacing is uniform for the entire bridge
         // enable the controls, unless there is only one girder then spacing doesn't apply
         bEnable = (m_NumGdrSpinner.GetPos() == 1 ? FALSE : specify_spacing);

         // restore the cached values for girder spacing and measurement type
         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            GetDlgItem(IDC_SPACING)->SetWindowText(m_strCacheGirderSpacing);
         }
         else
         {
            GetDlgItem(IDC_SPACING)->SetWindowText(m_strCacheJointSpacing);
         }
         pcbMeasure->SetCurSel(m_CacheGirderSpacingMeasureIdx);

         GetDlgItem(IDC_REF_GIRDER_OFFSET)->SetWindowText(m_strCacheRefGirderOffset);

      break;

      case pgsTypes::sbsGeneral:
      case pgsTypes::sbsGeneralAdjacent:

         // girder spacing is general (aka, defined span by span)
         // disable the controls
         bEnable = FALSE;

         // cache the current value of girder spacing and measurement type
         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            GetDlgItem(IDC_SPACING)->GetWindowText(strWndTxt);
            GetDlgItem(IDC_SPACING)->SetWindowText(_T(""));

            if ( strWndTxt != _T("") )
            {
               m_strCacheGirderSpacing = strWndTxt;
            }

         }
         else
         {
            GetDlgItem(IDC_SPACING)->GetWindowText(strWndTxt);
            GetDlgItem(IDC_SPACING)->SetWindowText(_T(""));

            if ( strWndTxt != _T("") )
            {
               m_strCacheJointSpacing = strWndTxt;
            }
         }

         GetDlgItem(IDC_REF_GIRDER_OFFSET)->GetWindowText(strWndTxt);
         GetDlgItem(IDC_REF_GIRDER_OFFSET)->SetWindowText(_T(""));
         if ( strWndTxt != _T("") )
         {
            m_strCacheRefGirderOffset = strWndTxt;
         }

         curSel = pcbMeasure->GetCurSel();
         pcbMeasure->SetCurSel(-1);

         if ( curSel != CB_ERR )
         {
            m_CacheGirderSpacingMeasureIdx = curSel;
         }

         
         pParent->m_BridgeDesc.SetGirderSpacing(m_GirderSpacing);

      break;

      default:
         ATLASSERT(false); // is there a new spacing type????
   }

   GetDlgItem(IDC_GIRDER_SPACING_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_SPACING)->EnableWindow(bEnable);
   GetDlgItem(IDC_SPACING_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_ALLOWABLE_SPACING)->EnableWindow(bEnable);


   if ( ::IsBridgeSpacing(m_GirderSpacingType) )
   {
      bEnable = TRUE;
   }
   else
   {
      bEnable = FALSE;
   }

   GetDlgItem(IDC_REF_GIRDER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);
   GetDlgItem(IDC_GIRDER_SPACING_MEASURE)->EnableWindow(bEnable);

   // update the the unit of measure
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CWnd* pWnd = GetDlgItem(IDC_SPACING_UNIT);
   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      pWnd->SetWindowText( pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   }
   else
   {
      pWnd->SetWindowText( pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   }

   if ( !m_bSetActive && IsSpanSpacing(m_GirderSpacingType) )
   {
      CString strText;
      CEAFDocument* pDoc = EAFGetDocument();
      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         strText = _T("By selecting this option, a different spacing can be used between each girder. To do this, edit the Group Details for each group.\n\nGroup Details can be edited by selecting the Layout tab and then pressing the edit button for a group.");
      }
      else
      {
         strText = _T("By selecting this option, a different spacing can be used between each girder. To do this, edit the Span Details for each span.\n\nSpan Details can be edited by selecting the Layout tab and then pressing the edit button for a span.");
      }
      UIHint(strText,UIHINT_SAME_GIRDER_SPACING);
   }

   OnDeckTypeChanged();

   // UpdateBridgeDescription(); // called by OnDeckTypeChanged()
}

void CBridgeDescGeneralPage::UpdateGirderConnectivity()
{
   // connectivity is only applicable for adjacent spacing
   bool bConnectivity = (m_GirderSpacingType == pgsTypes::sbsUniformAdjacent || m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent || m_GirderSpacingType == pgsTypes::sbsConstantAdjacent);
   GetDlgItem(IDC_GIRDER_CONNECTIVITY)->EnableWindow(bConnectivity);
   GetDlgItem(IDC_GIRDER_CONNECTIVITY_S)->EnableWindow(bConnectivity);
   FillGirderConnectivityComboBox();
}

void CBridgeDescGeneralPage::OnDeckTypeChanged() 
{
   UpdateGirderConnectivity();

   // make sure deck dimensions are consistent with deck type
   CComboBox* pcbDeckType = (CComboBox*)GetDlgItem(IDC_DECK_TYPE);
   int curSel = pcbDeckType->GetCurSel();
   pgsTypes::SupportedDeckType newDeckType = (pgsTypes::SupportedDeckType)pcbDeckType->GetItemData(curSel);

   if (IsAdjustableWidthDeck(newDeckType))
   {
      if ( m_CacheDeckEdgePoints.size() == 0 )
      {
         // moving to an adjustable width deck, but there aren't
         // any deck points
         //
         // Create a default deck point
         CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
         const CGirderGroupData* pGroup = pParent->m_BridgeDesc.GetGirderGroup((GroupIndexType)0);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         Float64 w = 0;
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            const GirderLibraryEntry* pEntry = pGroup->GetGirderLibraryEntry(gdrIdx);
            w += Max(pEntry->GetBeamWidth(pgsTypes::metStart),pEntry->GetBeamWidth(pgsTypes::metEnd));
         }

         CDeckPoint deckPoint;
         deckPoint.Station   = 0;
         deckPoint.LeftEdge  = w/2;
         deckPoint.RightEdge = w/2;

         m_CacheDeckEdgePoints.push_back(deckPoint);
      }

      m_Deck.DeckEdgePoints = m_CacheDeckEdgePoints;
   }
   else
   {
      m_CacheDeckEdgePoints = m_Deck.DeckEdgePoints;
      m_Deck.DeckEdgePoints.clear();
   }
   
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   m_Deck.DeckType = newDeckType;
   pParent->m_BridgeDesc.GetDeckDescription()->DeckType = newDeckType; // need to updated deck type here so calls to GetMaxFillet() work properly

   if ( m_Deck.DeckType == pgsTypes::sdtCompositeCIP || m_Deck.DeckType == pgsTypes::sdtCompositeOverlay )
   {
      Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();
      Float64 fillet = pParent->m_BridgeDesc.GetMaxFillet();
      if ( minSlabOffset < m_Deck.GrossDepth )
      {
         m_Deck.GrossDepth = minSlabOffset;

         // Since we are changing deck type here, data could be whacky. So use lrfd 9.7.1.1—Minimum Depth and Cover to
         // insure that we have a reasonable slab depth
         if (m_Deck.GrossDepth <= 0.0)
         {
            m_Deck.GrossDepth = Max(::ConvertToSysUnits(7.0, unitMeasure::Inch),fillet);
         }
      }
   }
   else if ( m_Deck.DeckType == pgsTypes::sdtCompositeSIP )
   {
      Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();
      Float64 fillet = pParent->m_BridgeDesc.GetMaxFillet();
      if ( minSlabOffset < m_Deck.GrossDepth + m_Deck.PanelDepth )
      {
         m_Deck.GrossDepth = minSlabOffset - m_Deck.PanelDepth; // decrease the cast depth

         // Since we are changing deck type here, data could be wacky. So use lrfd 9.7.1.1—Minimum Depth and Cover to
         // insure that we have a reasonable slab depth
         if (m_Deck.GrossDepth <= 0.0)
         {
            m_Deck.GrossDepth = Max(::ConvertToSysUnits(7.0, unitMeasure::Inch) - m_Deck.PanelDepth, fillet);
         }
      }
   }

   UpdateBridgeDescription();

   UpdateSuperstructureDescription();
}

BOOL CBridgeDescGeneralPage::UpdateGirderSpacingLimits()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   if ( IsSpanSpacing(m_GirderSpacingType) )
   {
      GetDlgItem(IDC_ALLOWABLE_SPACING)->SetWindowText(_T(""));
      return FALSE; // girder spacing isn't input in this dialog
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge,       pBridge);

   // need a spacing range that works for every girder in every span
   m_MinGirderSpacing = -MAX_GIRDER_SPACING;
   m_MaxGirderSpacing =  MAX_GIRDER_SPACING;
   GroupIndexType nGroups = pParent->m_BridgeDesc.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pParent->m_BridgeDesc.GetGirderGroup(grpIdx);

      const CPierData2* pPrevPier = pGroup->GetPier(pgsTypes::metStart);
      const CPierData2* pNextPier = pGroup->GetPier(pgsTypes::metEnd);

      // get skew information so spacing ranges can be skew adjusted
      Float64 prevSkewAngle;
      pBridge->GetSkewAngle(pPrevPier->GetStation(),pPrevPier->GetOrientation(),&prevSkewAngle);

      Float64 nextSkewAngle;
      pBridge->GetSkewAngle(pNextPier->GetStation(),pNextPier->GetOrientation(),&nextSkewAngle);

      Float64 startSkewCorrection, endSkewCorrection;
      if ( m_GirderSpacingMeasurementType == pgsTypes::NormalToItem )
      {
         startSkewCorrection = 1;
         endSkewCorrection = 1;
      }
      else
      {
         startSkewCorrection = fabs(1.0/cos(prevSkewAngle));
         endSkewCorrection   = fabs(1.0/cos(nextSkewAngle));
      }

      // check spacing limits for each girder group... grab the controlling values
      GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();
      for ( GroupIndexType gdrTypeGrpIdx = 0; gdrTypeGrpIdx < nGirderTypeGroups; gdrTypeGrpIdx++ )
      {
         GirderIndexType firstGdrIdx, lastGdrIdx;
         std::_tstring strGdrName;

         pGroup->GetGirderTypeGroup(gdrTypeGrpIdx,&firstGdrIdx,&lastGdrIdx,&strGdrName);

         const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(firstGdrIdx);
         const IBeamFactory::Dimensions& dimensions = pGdrEntry->GetDimensions();

         // don't use m_Factory because if we have a cross section with mixed beam types
         // (ie, I-beams and NU beams) the dimensions list and the factory wont match up
         // and GetAllowableSpacingRange will be all messed up.
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         Float64 min, max;
         factory->GetAllowableSpacingRange(dimensions,m_Deck.DeckType,m_GirderSpacingType,&min,&max);

         Float64 min1 = min*startSkewCorrection;
         Float64 max1 = max*startSkewCorrection;
         Float64 min2 = min*endSkewCorrection;
         Float64 max2 = max*endSkewCorrection;

         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            // girder spacing
            m_MinGirderSpacing = Max(Max(min1,min2),m_MinGirderSpacing);
            m_MaxGirderSpacing = Min(Min(max1,max2),m_MaxGirderSpacing);
         }
         else
         {
            // joint spacing
            m_MinGirderSpacing = 0;
            m_MaxGirderSpacing = Min(Min(max1-min1,max2-min2),m_MaxGirderSpacing);
         }
      }
   } // group loop

   BOOL specify_spacing = !IsEqual(m_MinGirderSpacing,m_MaxGirderSpacing) ? TRUE : FALSE;

   ATLASSERT( m_MinGirderSpacing <= m_MaxGirderSpacing );

   // label for beam spacing
   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      GetDlgItem(IDC_GIRDER_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
   }
   else
   {
      GetDlgItem(IDC_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
   }

   CString label;
   if (specify_spacing)
   {
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      if (m_MaxGirderSpacing < MAX_GIRDER_SPACING)
      {
         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            label.Format(_T("( %s to %s )"), 
               FormatDimension(m_MinGirderSpacing,pDisplayUnits->GetXSectionDimUnit()),
               FormatDimension(m_MaxGirderSpacing,pDisplayUnits->GetXSectionDimUnit()));
         }
         else
         {
            // this is actually joint spacing
            label.Format(_T("( 0 to %s )"), 
               FormatDimension(m_MaxGirderSpacing - m_MinGirderSpacing,pDisplayUnits->GetComponentDimUnit()));
         }
      }
      else
      {
         label.Format(_T("( %s or more )"), 
            FormatDimension(m_MinGirderSpacing,pDisplayUnits->GetXSectionDimUnit()));
      }
   }

   GetDlgItem(IDC_ALLOWABLE_SPACING)->SetWindowText(label);

   // if spacing is out of range fix it
   if (m_GirderSpacing < m_MinGirderSpacing || m_MaxGirderSpacing < m_GirderSpacing )
   {
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      m_GirderSpacing = m_MinGirderSpacing;

      CDataExchange dx(this,FALSE);
      DDX_UnitValueAndTag( &dx, IDC_SPACING,  IDC_SPACING_UNIT, m_GirderSpacing, pDisplayUnits->GetXSectionDimUnit() );

      if ( IsGirderSpacing(m_GirderSpacingType) )
      {
         m_strCacheGirderSpacing.Format(_T("%s"),FormatDimension(m_GirderSpacing,pDisplayUnits->GetXSectionDimUnit(), false));
         m_strCacheJointSpacing.Format( _T("%s"),FormatDimension(0,              pDisplayUnits->GetComponentDimUnit(),false));
      }
      else
      {
         m_strCacheGirderSpacing.Format(_T("%s"),FormatDimension(m_MinGirderSpacing,pDisplayUnits->GetXSectionDimUnit(), false));
         m_strCacheJointSpacing.Format( _T("%s"),FormatDimension(m_GirderSpacing,   pDisplayUnits->GetComponentDimUnit(),false));
      }
   }

   return specify_spacing;
}

void CBridgeDescGeneralPage::UpdateSuperstructureDescription()
{
   CString description;

   // girder name
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibrary, pLib );

   CComboBox* pCBGirders = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);
   int sel = pCBGirders->GetCurSel();

   const GirderLibraryEntry* pGdrEntry = NULL;
   if ( sel == CB_ERR )
   {
      pGdrEntry = pLib->GetGirderEntry(m_GirderName);
   }
   else
   {
      CString strGirder;
      pCBGirders->GetLBText(sel,strGirder);
      pGdrEntry = pLib->GetGirderEntry(strGirder);
   }


   std::_tstring name = pGdrEntry->GetSectionName();
   description = name.c_str();

   // deck type
   CComboBox* box = (CComboBox*)GetDlgItem(IDC_DECK_TYPE);
   int cursel = box->GetCurSel();
   pgsTypes::SupportedDeckType deckType = (pgsTypes::SupportedDeckType)box->GetItemData(cursel);

   description += _T(", ") + GetDeckString(deckType);

   pgsTypes::AdjacentTransverseConnectivity connect = pgsTypes::atcConnectedAsUnit;

   // connectivity if adjacent
   if (deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone)
   {

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_GIRDER_CONNECTIVITY);
      // good a place as any to cache connectivity
      m_CacheGirderConnectivityIdx = box->GetCurSel();
      ATLASSERT(m_CacheGirderConnectivityIdx!=-1);

      connect = (pgsTypes::AdjacentTransverseConnectivity)m_CacheGirderConnectivityIdx;

      description += _T(". Girders are spaced adjacently, and are connected transversely ");

      if (connect == pgsTypes::atcConnectedAsUnit)
      {
         description +=_T("sufficiently to act as a unit.");
      }
      else
      {
         description += _T("only enough to prevent relative vertical displacement at interface.");
      }
   }
   else
   {
      if (pgsTypes::sbsConstantAdjacent==m_GirderSpacingType || pgsTypes::sbsGeneralAdjacent==m_GirderSpacingType || pgsTypes::sbsUniformAdjacent==m_GirderSpacingType)
      {
         description += _T(", Girders at adjacent spacing.");
      }
      else
      {
         description += _T(", Girders at spread spacing.");
      }
   }


   GET_IFACE2( pBroker, IBridgeDescription, pIBridgeDesc );
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      description += _T("\r\n\r\nDistribution factors from Direct User Input");
   }
   else if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::LeverRule )
   {
      description += _T("\r\n\r\nDistribution factors computed using lever rule. Specification was overridden");
   }
   else
   {
#pragma Reminder("UPDATE: Assuming same section used for all spans/girders")
      std::_tstring entry_name = pGdrEntry->GetName();


      CComPtr<IDistFactorEngineer> dfEngineer;
      m_Factory->CreateDistFactorEngineer(pBroker, -1, &m_GirderSpacingType, &deckType, &connect, &dfEngineer);
      std::_tstring dfmethod = dfEngineer->GetComputationDescription(CGirderKey(0,0), entry_name, deckType, connect );

      description += _T("\r\n\r\nDistribution factors computed using ");
      description += dfmethod.c_str();
   }

   CEdit* pedit = (CEdit*)GetDlgItem(IDC_SUPERSTRUCTURE_DESCRIPTION);
   pedit->SetWindowText(description);
}



BOOL CBridgeDescGeneralPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      BOOL bShowTip = false;
      switch(nID)
      {
      case IDC_SAME_NUM_GIRDERLINES:
         if ( IsDlgButtonChecked(IDC_SAME_NUM_GIRDERLINES) )
         {
            if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
            {
               m_strToolTipText = _T("Uncheck this option to define a different number of girders in each group.");
            }
            else
            {
               m_strToolTipText = _T("Uncheck this option to define a different number of girders in each span.");
            }
         }
         else
         {
            if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
            {
               m_strToolTipText = _T("Check this option to define the number of girders in all groups. Otherwise, open the Layout tab and click on the Edit Group Details button to define the number of girders in a particular group.");
            }
            else
            {
               m_strToolTipText = _T("Check this option to define the number of girders in all spans. Otherwise, open the Layout tab and click on the Edit Span Details button to define the number of girders in a particular span.");
            }
         }

         bShowTip = TRUE;
         break;

      case IDC_SAME_GIRDERNAME:
         if ( IsDlgButtonChecked(IDC_SAME_GIRDERNAME) )
         {
            m_strToolTipText = _T("Uncheck this option to use a different girder type for each girder.");
         }
         else
         {
            if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
            {
               m_strToolTipText = _T("Check this option to use the same girder in all groups. Otherwise, open the Layout tab and click on the Edit Group Details button to assign a different girder type to each girder in a particular group.");
            }
            else
            {
               m_strToolTipText = _T("Check this option to use the same girder in all spans. Otherwise, open the Layout tab and click on the Edit Span Details button to assign a different girder type to each girder in a particular span.");
            }
         }

         bShowTip = TRUE;
         break;

      case IDC_REF_GIRDER_OFFSET:
         m_strToolTipText.LoadString(IDS_ALIGNMENTOFFSET_FMT);
         bShowTip = TRUE;
         break;

      default:
         return FALSE;
      }

      if ( bShowTip )
      {
         ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
         ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
         pTTT->lpszText = m_strToolTipText.LockBuffer();
         pTTT->hinst = NULL;
         return TRUE;
      }
      else
      {
         return FALSE;
      }

   }
   return FALSE;
}

void CBridgeDescGeneralPage::UIHint(const CString& strText,UINT mask)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   Uint32 hintSettings = pDoc->GetUIHintSettings();
   if ( sysFlags<Uint32>::IsClear(hintSettings,mask) )
   {
      if ( EAFShowUIHints(strText) )
      {
         sysFlags<Uint32>::Set(&hintSettings,mask);
         pDoc->SetUIHintSettings(hintSettings);
      }
   }
}

BOOL CBridgeDescGeneralPage::OnSetActive() 
{
   m_bSetActive = true;

   if ( !m_bFirstSetActive )
   {
      Init();
   }

	BOOL bResult = CPropertyPage::OnSetActive(); // calls DoDataExchange if not the first time page is active

   //OnGirderNameChanged(); // Available slab types are a function of girder type
   OnDeckTypeChanged();

   EnableGirderName(m_bSameGirderName);
   EnableNumGirderLines(m_bSameNumberOfGirders);

   OnGirderSpacingTypeChanged(); // enables/disables the girder spacing input based on current value

   UpdateSuperstructureDescription();
	
   m_bSetActive = false;

   return bResult;
}


void CBridgeDescGeneralPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_GENERAL );
}

void CBridgeDescGeneralPage::InitGirderName()
{
   // Gets the first girder name for the current girder family
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   pLibNames->EnumGirderNames(m_GirderFamilyName, &names );
   m_GirderName = names.front().c_str();
}

