///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperApp.h"
#include "PGSpliceDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescGeneralPage.h"
#include "BridgeDescDlg.h"
#include "Hints.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DistFactorEngineer.h>
#include <IFace\DocumentType.h>

#include <EAF\EAFMainFrame.h>

#include <PgsExt\ConcreteDetailsDlg.h>
#include <PgsExt\Helpers.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LEFT 0
#define RIGHT 1

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescGeneralPage property page

IMPLEMENT_DYNCREATE(CBridgeDescGeneralPage, CPropertyPage)

CBridgeDescGeneralPage::CBridgeDescGeneralPage() : CPropertyPage(CBridgeDescGeneralPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_TopWidthType = pgsTypes::twtSymmetric;
   m_LeftTopWidth = 0;
   m_RightTopWidth = 0;
   m_MinGirderTopWidth[LEFT] = 0;
   m_MaxGirderTopWidth[LEFT] = 0;
   m_MinGirderTopWidth[RIGHT] = 0;
   m_MaxGirderTopWidth[RIGHT] = 0;

   m_MinGirderSpacing = 0;
   m_MaxGirderSpacing = 0;

   m_MinGirderCount = 2;
   m_CacheGirderConnectivityIdx = CB_ERR;
   m_CacheDeckTypeIdx = CB_ERR;
   m_CacheWorkPointTypeIdx = CB_ERR;

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

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   DDX_Control(pDX, IDC_EC, m_ctrlEc);
   DDX_Control(pDX, IDC_EC_LABEL, m_ctrlEcCheck);
   DDX_Control(pDX, IDC_FC, m_ctrlFc);

	//{{AFX_DATA_MAP(CBridgeDescGeneralPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_NUMGDR_SPIN, m_NumGdrSpinner);
   DDX_Control(pDX, IDC_ALIGNMENTOFFSET_FMT, m_AlignmentOffsetFormat);
   //}}AFX_DATA_MAP

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);


   ////////////////////////////////////////////////
   // BridgeLink/Alignment offset
   ////////////////////////////////////////////////
   if (!pDX->m_bSaveAndValidate)
   {
      m_AlignmentOffset = pParent->m_BridgeDesc.GetAlignmentOffset();
   }

   DDX_OffsetAndTag(pDX, IDC_ALIGNMENTOFFSET, IDC_ALIGNMENTOFFSET_UNIT, m_AlignmentOffset, pDisplayUnits->GetAlignmentLengthUnit());

   if (pDX->m_bSaveAndValidate)
   {
      pParent->m_BridgeDesc.SetAlignmentOffset(m_AlignmentOffset);
   }

   ////////////////////////////////////////////////
   // Girders
   ////////////////////////////////////////////////
   //if (!pDX->m_bSaveAndValidate)
   //{
   //   FillGirderSpacingMeasurementComboBox();
   //}

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

   ////////////////////////////////////////////////
   // Girder/Joint Spacing
   ////////////////////////////////////////////////
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
      if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && IsBridgeSpacing(m_GirderSpacingType) && IsAdjacentSpacing(m_GirderSpacingType)) )
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

   ////////////////////////////////////////////////
   // Girder/Joint measurement type, location, and datum
   ////////////////////////////////////////////////
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
   DDX_CBItemData(pDX,IDC_WORKPOINT_TYPE,m_WorkPointLocation);

   ////////////////////////////////////////////////
   // Top Width
   ////////////////////////////////////////////////
   if (!pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && IsBridgeSpacing(m_GirderSpacingType) && IsTopWidthSpacing(m_GirderSpacingType)))
   {
      DDX_CBItemData(pDX, IDC_TOP_WIDTH_TYPE, m_TopWidthType);
      DDX_UnitValueAndTag(pDX, IDC_LEFT_TOP_WIDTH, IDC_LEFT_TOP_WIDTH_UNIT, m_LeftTopWidth, pDisplayUnits->GetXSectionDimUnit());
      DDX_UnitValueAndTag(pDX, IDC_RIGHT_TOP_WIDTH, IDC_RIGHT_TOP_WIDTH_UNIT, m_RightTopWidth, pDisplayUnits->GetXSectionDimUnit());
   }

   if (IsBridgeSpacing(m_GirderSpacingType) && IsTopWidthSpacing(m_GirderSpacingType))
   {
      if (m_TopWidthType == pgsTypes::twtAsymmetric)
      {
         Float64 topWidth = m_LeftTopWidth + m_RightTopWidth;
         DDV_UnitValueLimitOrMore(pDX, IDC_LEFT_TOP_WIDTH, m_LeftTopWidth, m_MinGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
         DDV_UnitValueLimitOrLess(pDX, IDC_LEFT_TOP_WIDTH, m_LeftTopWidth, m_MaxGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());

         DDV_UnitValueLimitOrMore(pDX, IDC_RIGHT_TOP_WIDTH, m_RightTopWidth, m_MinGirderTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit());
         DDV_UnitValueLimitOrLess(pDX, IDC_RIGHT_TOP_WIDTH, m_RightTopWidth, m_MaxGirderTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit());
      }
      else
      {
         DDV_UnitValueLimitOrMore(pDX, IDC_LEFT_TOP_WIDTH, m_LeftTopWidth, m_MinGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
         DDV_UnitValueLimitOrLess(pDX, IDC_LEFT_TOP_WIDTH, m_LeftTopWidth, m_MaxGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit());
      }
   }

   ////////////////////////////////////////////////
   // Longitudinal Joint Material
   ////////////////////////////////////////////////
   DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_UNIT, m_JointConcrete.Fc, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, m_JointConcrete.Ec, pDisplayUnits->GetModEUnit());
   DDX_Check_Bool(pDX, IDC_EC_LABEL, m_JointConcrete.bUserEc);
   if (pDX->m_bSaveAndValidate)
   {
      pParent->m_BridgeDesc.SetLongitudinalJointMaterial(m_JointConcrete);
   }
   else
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      m_strUserEc.Format(_T("%s"), FormatDimension(m_JointConcrete.Ec, pDisplayUnits->GetModEUnit(), false));
   }




   ////////////////////////////////////////////////
   // Deck 
   ////////////////////////////////////////////////
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
	DDX_CBItemData(pDX, IDC_DECK_TYPE, deckType);
   if (pDX->m_bSaveAndValidate)
   {
      pParent->m_BridgeDesc.GetDeckDescription()->SetDeckType(deckType);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      UpdateBridgeDescription();
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
   ON_CBN_SELCHANGE(IDC_TOP_WIDTH_TYPE, OnTopWidthTypeChanged)
   ON_BN_CLICKED(IDC_MORE_PROPERTIES, OnMoreProperties)
   ON_BN_CLICKED(IDC_EC_LABEL, OnBnClickedEc)
   ON_EN_CHANGE(IDC_FC, OnChangeFc)
   ON_EN_CHANGE(IDC_SPACING,OnChangeSpacing)
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

   FillGirderSpacingMeasurementComboBox();

   UpdateGirderSpacingLimits();

   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      EnableGirderSpacing(FALSE,FALSE);
   }

   FillTopWidthComboBox();
   EnableTopWidth(IsTopWidthSpacing(m_GirderSpacingType));
   UpdateGirderTopWidthSpacingLimits();

   EnableLongitudinalJointMaterial();

   FillWorkPointLocationComboBox();

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
      GetDlgItem(IDC_SAME_NUM_GIRDERLINES)->SetWindowText(_T("Use same number of girders in all groups"));
   }

   CString fmt;
   fmt.LoadString(IDS_ALIGNMENTOFFSET_FMT);
   m_AlignmentOffsetFormat.SetWindowText(fmt);

   UpdateConcreteTypeLabel();
   OnBnClickedEc();
   OnTopWidthTypeChanged();

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

      pParent->m_BridgeDesc.GetGirderTopWidth(&m_TopWidthType, &m_LeftTopWidth, &m_RightTopWidth);
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

      Float64 leftEnd, rightEnd;
      pGroup->GetGirder(0)->GetTopWidth(&m_TopWidthType, &m_LeftTopWidth, &m_RightTopWidth, &leftEnd, &rightEnd);
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

   if (IsTopWidthSpacing(m_GirderSpacingType))
   {
      m_strCacheLeftTopWidth.Format(_T("%s"), FormatDimension(m_LeftTopWidth, pDisplayUnits->GetXSectionDimUnit(), false));
      m_strCacheRightTopWidth.Format(_T("%s"), FormatDimension(m_RightTopWidth, pDisplayUnits->GetXSectionDimUnit(), false));
   }
   else
   {
      m_strCacheLeftTopWidth.Format(_T("%s"), FormatDimension(0, pDisplayUnits->GetXSectionDimUnit(), false));
      m_strCacheRightTopWidth.Format(_T("%s"), FormatDimension(0, pDisplayUnits->GetXSectionDimUnit(), false));
   }

   m_WorkPointLocation = pParent->m_BridgeDesc.GetWorkPointLocation();

   int sign = ::Sign(m_RefGirderOffset);
   LPTSTR strOffset = (sign == 0 ? _T("") : sign < 0 ? _T("L") : _T("R"));
   m_strCacheRefGirderOffset.Format(_T("%s %s"),FormatDimension(fabs(m_RefGirderOffset),pDisplayUnits->GetXSectionDimUnit(),false),strOffset);

   m_CacheDeckEdgePoints = pParent->m_BridgeDesc.GetDeckDescription()->DeckEdgePoints;

   m_TransverseConnectivity = pParent->m_BridgeDesc.GetDeckDescription()->TransverseConnectivity;
   m_JointConcrete = pParent->m_BridgeDesc.GetLongitudinalJointMaterial();

   UpdateMinimumGirderCount();

   // there are a couple controls that we don't want to be empty so put some dummy text in them
   GetDlgItem(IDC_LEFT_TOP_WIDTH)->SetWindowText(_T("0"));
   GetDlgItem(IDC_RIGHT_TOP_WIDTH)->SetWindowText(_T("0"));
   GetDlgItem(IDC_SPACING)->SetWindowText(_T("0"));
}

void CBridgeDescGeneralPage::UpdateBridgeDescription()
{
   // put the page data values into the bridge model
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups( m_bSameNumberOfGirders );
   pParent->m_BridgeDesc.UseSameGirderForEntireBridge( m_bSameGirderName );
   pParent->m_BridgeDesc.SetGirderCount(m_nGirders);


   bool bNewGirderFamily = (pParent->m_BridgeDesc.GetGirderFamilyName() != m_GirderFamilyName);

   pParent->m_BridgeDesc.SetGirderFamilyName(m_GirderFamilyName);
   pParent->m_BridgeDesc.SetGirderOrientation(m_GirderOrientation);

   if ( m_bSameGirderName || bNewGirderFamily )
   {
      GET_IFACE2( pBroker, ILibrary, pLib );
   
      const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(m_GirderName);
      pParent->m_BridgeDesc.SetGirderLibraryEntry(pGdrEntry); // must do this before SetGirderName
      pParent->m_BridgeDesc.SetGirderName(m_GirderName);
   }

   if ( bNewGirderFamily )
   {
      UpdateGirderSpacingLimits();
      UpdateGirderTopWidthSpacingLimits();
   }

   if (!m_Factory->IsSupportedBeamSpacing(m_GirderSpacingType))
   {
      const IBeamFactory::Dimensions& dimensions = pParent->m_BridgeDesc.GetGirderLibraryEntry()->GetDimensions();
      pgsTypes::SupportedBeamSpacing newSpacingType;
      Float64 newSpacing, topWidth;
      VERIFY(m_Factory->ConvertBeamSpacing(dimensions, m_GirderSpacingType, m_GirderSpacing, &newSpacingType, &newSpacing, &topWidth));
      m_GirderSpacingType = newSpacingType;
      m_GirderSpacing = newSpacing;
      m_LeftTopWidth = topWidth;
      m_RightTopWidth = 0.0;

      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      if (IsGirderSpacing(m_GirderSpacingType))
      {
         m_strCacheGirderSpacing.Format(_T("%s"), FormatDimension(m_GirderSpacing, pDisplayUnits->GetXSectionDimUnit(), false));
         m_strCacheJointSpacing.Format(_T("%s"), FormatDimension(0, pDisplayUnits->GetComponentDimUnit(), false));
      }
      else
      {
         m_strCacheGirderSpacing.Format(_T("%s"), FormatDimension(m_MinGirderSpacing, pDisplayUnits->GetXSectionDimUnit(), false));
         m_strCacheJointSpacing.Format(_T("%s"), FormatDimension(m_GirderSpacing, pDisplayUnits->GetComponentDimUnit(), false));
      }

      if (IsTopWidthSpacing(m_GirderSpacingType))
      {
         m_strCacheLeftTopWidth.Format(_T("%s"), FormatDimension(m_LeftTopWidth, pDisplayUnits->GetXSectionDimUnit(), false));
         m_strCacheRightTopWidth.Format(_T("%s"), FormatDimension(m_RightTopWidth, pDisplayUnits->GetXSectionDimUnit(), false));
      }
   }
   pParent->m_BridgeDesc.SetGirderSpacingType(m_GirderSpacingType);
   pParent->m_BridgeDesc.SetGirderSpacing(m_GirderSpacing);
   pParent->m_BridgeDesc.SetGirderTopWidth(m_TopWidthType, m_LeftTopWidth, m_RightTopWidth);

   pParent->m_BridgeDesc.SetMeasurementLocation(m_GirderSpacingMeasurementLocation);
   pParent->m_BridgeDesc.SetMeasurementType(m_GirderSpacingMeasurementType);

   pParent->m_BridgeDesc.SetWorkPointLocation(m_WorkPointLocation);

   pParent->m_BridgeDesc.SetRefGirder(m_RefGirderIdx);
   pParent->m_BridgeDesc.SetRefGirderOffset(m_RefGirderOffset);
   pParent->m_BridgeDesc.SetRefGirderOffsetType(m_RefGirderOffsetType);

   pParent->m_BridgeDesc.GetDeckDescription()->TransverseConnectivity = m_TransverseConnectivity;

   if ( bNewGirderFamily )
   {
      pParent->m_BridgeDesc.CopyDown(true,true,true,true,true,true,true);
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
   UpdateGirderTopWidthSpacingLimits();

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

void CBridgeDescGeneralPage::EnableTopWidth(BOOL bEnable)
{
   CWnd* pLabel = GetDlgItem(IDC_TOP_WIDTH_LABEL);
   CWnd* pType = GetDlgItem(IDC_TOP_WIDTH_TYPE);
   CWnd* pLeftLabel = GetDlgItem(IDC_LEFT_TOP_WIDTH_LABEL);
   CWnd* pLeftEdit = GetDlgItem(IDC_LEFT_TOP_WIDTH);
   CWnd* pLeftUnit = GetDlgItem(IDC_LEFT_TOP_WIDTH_UNIT);
   CWnd* pRightLabel = GetDlgItem(IDC_RIGHT_TOP_WIDTH_LABEL);
   CWnd* pRightEdit = GetDlgItem(IDC_RIGHT_TOP_WIDTH);
   CWnd* pRightUnit = GetDlgItem(IDC_RIGHT_TOP_WIDTH_UNIT);
   CWnd* pAllowable = GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH);
   int nShowCommand = (bEnable ? SW_SHOW : SW_HIDE);
   pLabel->ShowWindow(nShowCommand);
   pType->ShowWindow(nShowCommand);
   pLeftLabel->ShowWindow(nShowCommand);
   pLeftEdit->ShowWindow(nShowCommand);
   pLeftUnit->ShowWindow(nShowCommand);
   pRightLabel->ShowWindow(nShowCommand);
   pRightEdit->ShowWindow(nShowCommand);
   pRightUnit->ShowWindow(nShowCommand);
   pAllowable->ShowWindow(nShowCommand);
}

void CBridgeDescGeneralPage::EnableLongitudinalJointMaterial()
{
   CWnd* pGroupBox = GetDlgItem(IDC_LONGITUDINAL_JOINT_MATERIAL_GROUP);
   CWnd* pConcreteTypeLabel = GetDlgItem(IDC_CONCRETE_TYPE_LABEL);
   CWnd* pfcLabel = GetDlgItem(IDC_FC_LABEL);
   CWnd* pfc = GetDlgItem(IDC_FC);
   CWnd* pfcUnit = GetDlgItem(IDC_FC_UNIT);
   CWnd* pEcLabel = GetDlgItem(IDC_EC_LABEL);
   CWnd* pEc = GetDlgItem(IDC_EC);
   CWnd* pEcUnit = GetDlgItem(IDC_EC_UNIT);
   CWnd* pMoreProperties = GetDlgItem(IDC_MORE_PROPERTIES);

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   int nShowCommand = (pParent->m_BridgeDesc.HasStructuralLongitudinalJoints() ? SW_SHOW : SW_HIDE);

   pGroupBox->ShowWindow(nShowCommand);
   pConcreteTypeLabel->ShowWindow(nShowCommand);
   pfcLabel->ShowWindow(nShowCommand);
   pfc->ShowWindow(nShowCommand);
   pfcUnit->ShowWindow(nShowCommand);
   pEcLabel->ShowWindow(nShowCommand);
   pEc->ShowWindow(nShowCommand);
   pEcUnit->ShowWindow(nShowCommand);
   pMoreProperties->ShowWindow(nShowCommand);
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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);

   CString strGrp = (bIsSplicedGirder ? _T("Group") : _T("Span"));

   CString strOrientation[5];
   strOrientation[0] = _T("Plumb");
   strOrientation[1].Format(_T("Normal to roadway at Start of %s"), strGrp);
   strOrientation[2].Format(_T("Normal to roadway at Middle of %s"), strGrp);
   strOrientation[3].Format(_T("Normal to roadway at End of %s"), strGrp);
   strOrientation[4] = _T("Balanced to minimize haunch depth");

   // Girder Orientation
   CComboBox* pOrientation = (CComboBox*)GetDlgItem(IDC_GIRDER_ORIENTATION);
   int curSel = pOrientation->GetCurSel();
   // Assume plumb if no current selection
   pgsTypes::GirderOrientationType curOrientation = (curSel!=CB_ERR) ? (pgsTypes::GirderOrientationType)pOrientation->GetItemData(curSel) : pgsTypes::Plumb;

   // Refill combo box
   pOrientation->ResetContent();
   int numInList = 0;
   std::vector<pgsTypes::GirderOrientationType> vTypes = m_Factory->GetSupportedGirderOrientation();
   for (auto type : vTypes)
   {
      int idx = pOrientation->AddString(strOrientation[type]);
      pOrientation->SetItemData(idx, (DWORD_PTR)type);
      numInList++;
   }

   // There is a chance that value set in old list is not available in new list
   bool wasSet(false);
   if (curSel != CB_ERR)
   {
      for (int isel = 0; isel < numInList; isel++)
      {
         pgsTypes::GirderOrientationType orientation = (pgsTypes::GirderOrientationType)pOrientation->GetItemData(isel);
         if (orientation == curOrientation)
         {
            if (pOrientation->SetCurSel(isel) == CB_ERR)
            {
               ATLASSERT(0); // should not happen
            }
            else
            {
               wasSet = true; // success
               break;
            }
         }
      }
   }

   if (!wasSet)
   {
      // Default to Plumb if all else fails
      pOrientation->SetCurSel(0);

      if (curSel != CB_ERR)
      {
         // If value was previously set and we get here, then we need to modify underlying data as well. (i.e., DoDataExchange will not be called).
         m_GirderOrientation = pgsTypes::Plumb;
      }
   }
}

void CBridgeDescGeneralPage::FillGirderSpacingTypeComboBox()
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_TYPE);
   pSpacingType->ResetContent();

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CEAFDocument* pDoc = EAFGetDocument();
   bool bSplicedGirder = pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) == TRUE ? true : false;

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
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType,bSplicedGirder));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsGeneral:
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType, bSplicedGirder));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsUniformAdjacent:
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType, bSplicedGirder));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsGeneralAdjacent:
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType, bSplicedGirder));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsConstantAdjacent:
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType, bSplicedGirder));
         pSpacingType->SetItemData(idx,(DWORD)spacingType);
         break;

      case pgsTypes::sbsUniformAdjacentWithTopWidth:
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType, bSplicedGirder));
         pSpacingType->SetItemData(idx, (DWORD)spacingType);
         break;

      case pgsTypes::sbsGeneralAdjacentWithTopWidth:
         idx = pSpacingType->AddString(GetGirderSpacingType(spacingType, bSplicedGirder));
         pSpacingType->SetItemData(idx, (DWORD)spacingType);
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

void CBridgeDescGeneralPage::FillWorkPointLocationComboBox()
{
   CComboBox* pWorkPointCB = (CComboBox*)GetDlgItem(IDC_WORKPOINT_TYPE);

   int oldSel = pWorkPointCB->GetCurSel();

   // Fill control
   pWorkPointCB->ResetContent();
   pgsTypes::WorkPointLocations wpls = m_Factory->GetSupportedWorkPointLocations(m_GirderSpacingType);
   int idx;
   for ( auto & rwpl : wpls )
   {
      switch( rwpl )
      {
      case pgsTypes::wplTopGirder:
         idx = pWorkPointCB->AddString(_T("Top Centerline of Girders"));
         pWorkPointCB->SetItemData(idx,(DWORD)rwpl);
         break;

      case pgsTypes::wplBottomGirder:
         idx = pWorkPointCB->AddString(_T("Bottom Centerline of Girders"));
         pWorkPointCB->SetItemData(idx,(DWORD)rwpl);
         break;

      default:
         ATLASSERT(false); // is there a new type???
      }
   }

   // Disable control if only one item in it. This means that only top is allowed
   BOOL benable = wpls.size() > 1 ? 1 : 0; 

   // Cache last enabled index if we are switching from disabled -> enabled
   int curSel = max(0, oldSel);
   if (benable)
   {
      if (!pWorkPointCB->IsWindowEnabled())
      {
         curSel = m_CacheWorkPointTypeIdx;
      }
   }
   else
   {
      curSel = 0; // Top work point location available only
      m_CacheWorkPointTypeIdx = oldSel; // cache old setting
   }

   if ( curSel != CB_ERR )
   {
      pWorkPointCB->SetCurSel(curSel);
   }

   pWorkPointCB->EnableWindow(benable);
   GetDlgItem(IDC_WORKPOINT_STATIC)->EnableWindow(benable);

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

void CBridgeDescGeneralPage::FillTopWidthComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TOP_WIDTH_TYPE);
   pCB->ResetContent();
   std::vector<pgsTypes::TopWidthType> vTopWidthTypes = m_Factory->GetSupportedTopWidthTypes();
#if defined _DEBUG
   if (IsTopWidthSpacing(m_GirderSpacingType))
   {
      ATLASSERT(1 <= vTopWidthTypes.size()); // there must be at least on supported type
   }
   else
   {
      ATLASSERT(vTopWidthTypes.size() == 0);
   }
#endif

   for (auto type : vTopWidthTypes)
   {
      int idx = pCB->AddString(GetTopWidthType(type));
      pCB->SetItemData(idx, (DWORD_PTR)type);
   }
   pCB->SetCurSel(0);
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
      pPrevPier->GetBearingOffset(pgsTypes::Ahead,&offset,&mbs,true);
      pNextPier->GetBearingOffset(pgsTypes::Back, &offset,&mbe,true);

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
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

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
      deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   }

   pcbDeck->ResetContent();

   cursel = CB_ERR;
   int selidx = 0;
   for ( const auto& thisDeckType : deckTypes)
   {
      CString strDeckType = GetDeckTypeName(thisDeckType);

      selidx = pcbDeck->AddString(strDeckType );

      pcbDeck->SetItemData(selidx,(DWORD)thisDeckType);

      if (thisDeckType == deckType )
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
      pParent->m_BridgeDesc.GetDeckDescription()->SetDeckType(deckTypes.front());
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

   bool bConnectivity = IsAdjacentSpacing(m_GirderSpacingType) ? true : false;
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
         cursel = (m_TransverseConnectivity == pgsTypes::atcConnectedAsUnit ? 0 : 1);
         pcbConnectivity->SetCurSel(cursel);
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

   // when the girder family changes, we must change the girder layout so that all girders are the same
   m_bSameGirderName = true;
   CheckDlgButton(IDC_SAME_GIRDERNAME, BST_CHECKED);

   InitGirderName();          // sets the current girder name to the first girder of the family
   UpdateGirderFactory();     // gets the new factory for this girder family
   FillGirderOrientationComboBox(); // fills the girder orientation combo box
   FillGirderNameComboBox();  // fills the girder name combo box with girders from this family
   FillGirderSpacingTypeComboBox(); // get new spacing options for this girder family
   FillDeckTypeComboBox();          // set deck type options to match this girder family
   UpdateGirderConnectivity();      // fills the combo box and enables/disables

   UpdateBridgeDescription();

   pgsTypes::SupportedBeamSpacing oldSpacingType = m_GirderSpacingType;

   if ( !UpdateGirderSpacingLimits() || m_NumGdrSpinner.GetPos() == 1)
   {
      EnableGirderSpacing(FALSE,FALSE);
   }
   else
   {
      EnableGirderSpacing(TRUE,FALSE);
   }

   EnableTopWidth(IsTopWidthSpacing(m_GirderSpacingType));
   UpdateGirderTopWidthSpacingLimits();

   UpdateSuperstructureDescription();
   FillDeckTypeComboBox();
   FillGirderSpacingTypeComboBox();
   FillTopWidthComboBox();
   FillWorkPointLocationComboBox();

   UpdateData(FALSE);


   //if (oldSpacingType != m_GirderSpacingType)
   {
      OnGirderSpacingTypeChanged();
   }

   EnableTopWidth(IsTopWidthSpacing(m_GirderSpacingType));
   OnTopWidthTypeChanged();

   EnableLongitudinalJointMaterial();
}

void CBridgeDescGeneralPage::UpdateMinimumGirderCount()
{
   m_MinGirderCount = m_Factory->GetMinimumBeamCount();

   if ( m_NumGdrSpinner.GetSafeHwnd() != nullptr )
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
   int result = AfxMessageBox(_T("Changing the girder type will reset the strands, stirrups, and longitudinal rebar to default values.\n\nIs that OK?"), MB_YESNO);
   if (result == IDNO)
   {
      pCB->SetCurSel((int)m_GirderNameIdx);
      return;
   }

   CComboBox* pCBGirders = (CComboBox*)GetDlgItem(IDC_GDR_TYPE);
   int sel = pCBGirders->GetCurSel();
   pCBGirders->GetLBText(sel,m_GirderName);

   UpdateGirderFactory();

   UpdateMinimumGirderCount();

   pgsTypes::SupportedBeamSpacing oldSpacingType = m_GirderSpacingType;

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
   FillGirderSpacingTypeComboBox();
   FillTopWidthComboBox();
   FillGirderOrientationComboBox(); // orientation can differ between girder types although it probably should not

   UpdateData(FALSE);


   if (oldSpacingType != m_GirderSpacingType)
   {
      OnGirderSpacingTypeChanged();
   }

   EnableTopWidth(IsTopWidthSpacing(m_GirderSpacingType));
   OnTopWidthTypeChanged();

   EnableLongitudinalJointMaterial();
}

void CBridgeDescGeneralPage::OnGirderConnectivityChanged() 
{
   CComboBox* pcbConnectivity = (CComboBox*)GetDlgItem(IDC_GIRDER_CONNECTIVITY);
   int curSel = pcbConnectivity->GetCurSel();
   m_TransverseConnectivity = (pgsTypes::AdjacentTransverseConnectivity)(pcbConnectivity->GetItemData(curSel));

   UpdateBridgeDescription();

   EnableLongitudinalJointMaterial();

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

   pgsTypes::SupportedBeamSpacing old_spacing_type = m_GirderSpacingType;

   CComboBox* pcbSpacingType = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_TYPE);
   int curSel = pcbSpacingType->GetCurSel();
   m_GirderSpacingType = (pgsTypes::SupportedBeamSpacing)(pcbSpacingType->GetItemData(curSel));

   FillDeckTypeComboBox(); // update the available deck types for this spacing type
                           // this must be done before the girder spacing limits are determined

   FillGirderConnectivityComboBox();

   FillWorkPointLocationComboBox();

   BOOL specify_spacing = UpdateGirderSpacingLimits();
   // spacing only needs to be specified if it can take on multiple values

   UpdateGirderTopWidthSpacingLimits();

   CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_GIRDER_SPACING_MEASURE);

   CString strWndTxt;
   BOOL bEnableSpacing = FALSE;
   BOOL bEnableTopWidth = FALSE;
   switch ( m_GirderSpacingType )
   {
      case pgsTypes::sbsUniform:
      case pgsTypes::sbsUniformAdjacent:
      case pgsTypes::sbsConstantAdjacent:
      case pgsTypes::sbsUniformAdjacentWithTopWidth:

         // girder spacing is uniform for the entire bridge
         // enable the controls, unless there is only one girder then spacing doesn't apply
         bEnableSpacing = (m_NumGdrSpinner.GetPos() == 1 ? FALSE : specify_spacing);

         // restore the cached values for girder spacing and measurement type
         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            GetDlgItem(IDC_SPACING)->SetWindowText(m_strCacheGirderSpacing);
         }
         else
         {
            GetDlgItem(IDC_SPACING)->SetWindowText(m_strCacheJointSpacing);
         }

         if (IsTopWidthSpacing(m_GirderSpacingType))
         {
            GetDlgItem(IDC_LEFT_TOP_WIDTH)->SetWindowText(m_strCacheLeftTopWidth);
            GetDlgItem(IDC_RIGHT_TOP_WIDTH)->SetWindowText(m_strCacheRightTopWidth);
            bEnableTopWidth = TRUE;
         }

         pcbMeasure->SetCurSel(m_CacheGirderSpacingMeasureIdx);

         GetDlgItem(IDC_REF_GIRDER_OFFSET)->SetWindowText(m_strCacheRefGirderOffset);

      break;

      case pgsTypes::sbsGeneral:
      case pgsTypes::sbsGeneralAdjacent:
      case pgsTypes::sbsGeneralAdjacentWithTopWidth:

         // girder spacing is general (aka, defined span by span)
         // disable the controls
         bEnableSpacing = FALSE;

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

         if (IsTopWidthSpacing(m_GirderSpacingType))
         {
            GetDlgItem(IDC_LEFT_TOP_WIDTH)->GetWindowText(strWndTxt);
            GetDlgItem(IDC_LEFT_TOP_WIDTH)->SetWindowText(_T(""));
            if (strWndTxt != _T(""))
            {
               m_strCacheLeftTopWidth = strWndTxt;
            }

            GetDlgItem(IDC_RIGHT_TOP_WIDTH)->GetWindowText(strWndTxt);
            GetDlgItem(IDC_RIGHT_TOP_WIDTH)->SetWindowText(_T(""));
            if (strWndTxt != _T(""))
            {
               m_strCacheRightTopWidth = strWndTxt;
            }
            bEnableTopWidth = FALSE;
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

         if ( m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent && old_spacing_type == pgsTypes::sbsGeneral)
         {
            // oddball case changing from general spread to general adjacent. Need to change spacing values to 
            // something reasonable
            m_GirderSpacing = m_MaxGirderSpacing; // get some spread since we a coming from that world
            pParent->m_BridgeDesc.SetGirderSpacing(m_GirderSpacing);
            pParent->m_BridgeDesc.CopyDown(false,false,true,false,false,false,false);
         }
         else if (m_GirderSpacingType == pgsTypes::sbsGeneral && old_spacing_type == pgsTypes::sbsUniformAdjacent)
         {
            // oddball case changing from uniform adjacent to general. Need to change spacing values to 
            // something reasonable
            m_GirderSpacing = m_MinGirderSpacing; // get some spread since we a coming from that world
            pParent->m_BridgeDesc.SetGirderSpacing(m_GirderSpacing);
            pParent->m_BridgeDesc.CopyDown(false, false, true, false, false, false, false);
         }
         else
         {
            pParent->m_BridgeDesc.SetGirderSpacing(m_GirderSpacing);
         }

      break;

      default:
         ATLASSERT(false); // is there a new spacing type????
   }

   GetDlgItem(IDC_GIRDER_SPACING_LABEL)->EnableWindow(bEnableSpacing);
   GetDlgItem(IDC_SPACING)->EnableWindow(bEnableSpacing);
   GetDlgItem(IDC_SPACING_UNIT)->EnableWindow(bEnableSpacing);
   GetDlgItem(IDC_ALLOWABLE_SPACING)->EnableWindow(bEnableSpacing);

   GetDlgItem(IDC_TOP_WIDTH_LABEL)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_TOP_WIDTH_TYPE)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_LEFT_TOP_WIDTH_LABEL)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_LEFT_TOP_WIDTH)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_LEFT_TOP_WIDTH_UNIT)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_RIGHT_TOP_WIDTH_LABEL)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_RIGHT_TOP_WIDTH)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_RIGHT_TOP_WIDTH_UNIT)->EnableWindow(bEnableTopWidth);
   GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH)->EnableWindow(bEnableTopWidth);


   BOOL bEnableRefGirder = FALSE;
   if ( ::IsBridgeSpacing(m_GirderSpacingType) )
   {
      bEnableRefGirder = TRUE;
   }

   GetDlgItem(IDC_REF_GIRDER_LABEL)->EnableWindow(bEnableRefGirder);
   GetDlgItem(IDC_REF_GIRDER)->EnableWindow(bEnableRefGirder);
   GetDlgItem(IDC_REF_GIRDER_OFFSET)->EnableWindow(bEnableRefGirder);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_UNIT)->EnableWindow(bEnableRefGirder);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE_LABEL)->EnableWindow(bEnableRefGirder);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnableRefGirder);
   GetDlgItem(IDC_GIRDER_SPACING_MEASURE)->EnableWindow(bEnableRefGirder); // should this be bEnableSpacing?

   // update the unit of measure
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

   // If girder spacing is adjacent, force haunch shape to square
   if ( IsAdjacentSpacing(m_GirderSpacingType) )
   {
      pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape = pgsTypes::hsSquare;
   }

   OnDeckTypeChanged();

   EnableLongitudinalJointMaterial();
}

void CBridgeDescGeneralPage::OnTopWidthTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TOP_WIDTH_TYPE);
   int curSel = pCB->GetCurSel();
   pgsTypes::TopWidthType type = (pgsTypes::TopWidthType)pCB->GetItemData(curSel);
   int nShowCommand = (type == pgsTypes::twtAsymmetric ? SW_SHOW : SW_HIDE);
   CWnd* pLeftLabel = GetDlgItem(IDC_LEFT_TOP_WIDTH_LABEL);
   CWnd* pRightLabel = GetDlgItem(IDC_RIGHT_TOP_WIDTH_LABEL);
   CWnd* pEdit = GetDlgItem(IDC_RIGHT_TOP_WIDTH);
   CWnd* pUnit = GetDlgItem(IDC_RIGHT_TOP_WIDTH_UNIT);
   pLeftLabel->ShowWindow(nShowCommand);
   pRightLabel->ShowWindow(nShowCommand);
   pEdit->ShowWindow(nShowCommand);
   pUnit->ShowWindow(nShowCommand);

   UpdateGirderTopWidthSpacingLimits();
}

void CBridgeDescGeneralPage::UpdateGirderConnectivity()
{
   // connectivity is only applicable for adjacent spacing
   bool bConnectivity = IsAdjacentSpacing(m_GirderSpacingType);
   GetDlgItem(IDC_GIRDER_CONNECTIVITY)->EnableWindow(bConnectivity);
   GetDlgItem(IDC_GIRDER_CONNECTIVITY_S)->EnableWindow(bConnectivity);
   FillGirderConnectivityComboBox();
}

void CBridgeDescGeneralPage::OnDeckTypeChanged() 
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

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

      pParent->m_BridgeDesc.GetDeckDescription()->DeckEdgePoints = m_CacheDeckEdgePoints;
   }
   else
   {
      m_CacheDeckEdgePoints = pParent->m_BridgeDesc.GetDeckDescription()->DeckEdgePoints;
      pParent->m_BridgeDesc.GetDeckDescription()->DeckEdgePoints.clear();
   }

   CTimelineManager* pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
   
   pParent->m_BridgeDesc.GetDeckDescription()->SetDeckType(newDeckType);

   if (pTimelineMgr->GetCastDeckEventIndex() == INVALID_INDEX && newDeckType != pgsTypes::sdtNone)
   {
      // we are changing to a "deck" option and there isn't a deck construction activity in the time line
      // create one now
      const CTimelineEvent* pEventBeforeDeckCasting;
      EventIndexType castDiaphragmEventIdx = pTimelineMgr->GetIntermediateDiaphragmsLoadEventIndex();
      EventIndexType castLongitudinalJointEventIdx = pTimelineMgr->GetCastLongitudinalJointEventIndex();
      if (castLongitudinalJointEventIdx == INVALID_INDEX)
      {
         pEventBeforeDeckCasting = pTimelineMgr->GetEventByIndex(castDiaphragmEventIdx);
      }
      else
      {
         pEventBeforeDeckCasting = pTimelineMgr->GetEventByIndex(castLongitudinalJointEventIdx);
      }

      CTimelineEvent castDeckEvent;
      castDeckEvent.SetDay(pEventBeforeDeckCasting->GetDay() + pEventBeforeDeckCasting->GetDuration());
      castDeckEvent.GetCastDeckActivity().Enable();
      castDeckEvent.SetDescription(GetCastDeckEventName(newDeckType));
      castDeckEvent.GetCastDeckActivity().Enable();
      castDeckEvent.GetCastDeckActivity().SetCastingType(CCastDeckActivity::Continuous);
      castDeckEvent.GetCastDeckActivity().SetTotalCuringDuration(28.0); // day
      castDeckEvent.GetCastDeckActivity().SetActiveCuringDuration(28.0); // day
      
      EventIndexType castDeckEventIdx;
      pTimelineMgr->AddTimelineEvent(castDeckEvent, true, &castDeckEventIdx);
   }

   if (newDeckType == pgsTypes::sdtCompositeCIP || IsOverlayDeck(newDeckType))
   {
      Float64 minSlabOffset = pParent->m_BridgeDesc.GetLeastSlabOffset();
      if ( minSlabOffset < pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth )
      {
         pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth = minSlabOffset;

         // Since we are changing deck type here, data could be wacky. So use LRFD 9.7.1.1—Minimum Depth and Cover to
         // insure that we have a reasonable slab depth
         if (pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth <= 0.0)
         {
            Float64 fillet = pParent->m_BridgeDesc.GetFillet();
            pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth = Max(WBFL::Units::ConvertToSysUnits(7.0, WBFL::Units::Measure::Inch),fillet);
         }
      }
   }
   else if (newDeckType == pgsTypes::sdtCompositeSIP )
   {
      Float64 minSlabOffset = pParent->m_BridgeDesc.GetLeastSlabOffset();
      if ( minSlabOffset < pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth )
      {
         pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth = minSlabOffset - pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth; // decrease the cast depth

         // Since we are changing deck type here, data could be wacky. So use LRFD 9.7.1.1—Minimum Depth and Cover to
         // insure that we have a reasonable slab depth
         if (pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth <= 0.0)
         {
            Float64 fillet = pParent->m_BridgeDesc.GetFillet();
            pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth = Max(WBFL::Units::ConvertToSysUnits(7.0, WBFL::Units::Measure::Inch) - pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth, fillet);
         }
      }
   }

   UpdateBridgeDescription();

   UpdateSuperstructureDescription();
}

void CBridgeDescGeneralPage::UpdateGirderTopWidthSpacingLimits()
{
   if (IsSpanSpacing(m_GirderSpacingType) )
   {
      GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH)->SetWindowText(_T(""));
      return;
   }

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TOP_WIDTH_TYPE);
   int curSel = pCB->GetCurSel();
   pgsTypes::TopWidthType topWidthType = (pgsTypes::TopWidthType)pCB->GetItemData(curSel);

   // need a top width range that works for every girder in every span
   Float64 minGirderTopWidth[2] = { m_MinGirderTopWidth[LEFT], m_MinGirderTopWidth[RIGHT] };
   Float64 maxGirderTopWidth[2] = { m_MaxGirderTopWidth[LEFT], m_MaxGirderTopWidth[RIGHT] };

   GroupIndexType nGroups = pParent->m_BridgeDesc.GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      // check top width limits for each girder group... grab the controlling values
      const CGirderGroupData* pGroup = pParent->m_BridgeDesc.GetGirderGroup(grpIdx);
      GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();
      for (GroupIndexType gdrTypeGrpIdx = 0; gdrTypeGrpIdx < nGirderTypeGroups; gdrTypeGrpIdx++)
      {
         GirderIndexType firstGdrIdx, lastGdrIdx;
         std::_tstring strGdrName;
         pGroup->GetGirderTypeGroup(gdrTypeGrpIdx, &firstGdrIdx, &lastGdrIdx, &strGdrName);

         const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(firstGdrIdx);
         const IBeamFactory::Dimensions& dimensions = pGdrEntry->GetDimensions();

         // don't use m_Factory because if we have a cross section with mixed beam types
         // (i.e., I-beams and NU beams) the dimensions list and the factory wont match up
         // and GetAllowableTopWidthRange will be all messed up.
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         Float64 Wmin[2], Wmax[2];
         factory->GetAllowableTopWidthRange(topWidthType, dimensions, &Wmin[LEFT], &Wmax[LEFT], &Wmin[RIGHT], &Wmax[RIGHT]);
         if (gdrTypeGrpIdx == 0)
         {
            minGirderTopWidth[LEFT] = Wmin[LEFT];
            maxGirderTopWidth[LEFT] = Wmax[LEFT];

            minGirderTopWidth[RIGHT] = Wmin[RIGHT];
            maxGirderTopWidth[RIGHT] = Wmax[RIGHT];
         }
         else
         {
            // note, the use of Max and Min for the min and max values is correct
            // even though it looks backwards.
            // Consider two girder types... one with top width range 4.5' - 9' and one with
            // top width range 6.5' - 14'. The valid range is 6.5'-9'. 
            // We want the max of the min values and the min of the max values
            minGirderTopWidth[LEFT] = Max(minGirderTopWidth[LEFT], Wmin[LEFT]);
            maxGirderTopWidth[LEFT] = Min(maxGirderTopWidth[LEFT], Wmax[LEFT]);
            minGirderTopWidth[RIGHT] = Max(minGirderTopWidth[RIGHT], Wmin[RIGHT]);
            maxGirderTopWidth[RIGHT] = Min(maxGirderTopWidth[RIGHT], Wmax[RIGHT]);
         }
      }
   }

   m_MinGirderTopWidth[LEFT] = minGirderTopWidth[LEFT];
   m_MaxGirderTopWidth[LEFT] = maxGirderTopWidth[LEFT];
   m_MinGirderTopWidth[RIGHT] = minGirderTopWidth[RIGHT];
   m_MaxGirderTopWidth[RIGHT] = maxGirderTopWidth[RIGHT];

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CString strLabel;
   if (topWidthType == pgsTypes::twtAsymmetric)
   {
      strLabel.Format(_T("Left (%s to %s), Right (%s to %s)"),
         FormatDimension(m_MinGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MaxGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MinGirderTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MaxGirderTopWidth[RIGHT], pDisplayUnits->GetXSectionDimUnit()));
   }
   else
   {
      strLabel.Format(_T("(%s to %s)"),
         FormatDimension(m_MinGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()),
         FormatDimension(m_MaxGirderTopWidth[LEFT], pDisplayUnits->GetXSectionDimUnit()));
   }

   GetDlgItem(IDC_ALLOWABLE_TOP_WIDTH)->SetWindowText(strLabel);
}

BOOL CBridgeDescGeneralPage::UpdateGirderSpacingLimits()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge,       pBridge);

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();


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
         // (i.e. , I-beams and NU beams) the dimensions list and the factory wont match up
         // and GetAllowableSpacingRange will be all messed up.
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         Float64 min, max;
         factory->GetAllowableSpacingRange(dimensions, deckType,m_GirderSpacingType,&min,&max);

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
            m_MinGirderSpacing = Max(min1, min2, m_MinGirderSpacing);
            m_MaxGirderSpacing = Min(max1, max2, m_MaxGirderSpacing);
         }
      }
   } // group loop

   BOOL specify_spacing = !IsEqual(m_MinGirderSpacing,m_MaxGirderSpacing) ? TRUE : FALSE;

   ATLASSERT( m_MinGirderSpacing <= m_MaxGirderSpacing );

   if ( IsSpanSpacing(m_GirderSpacingType) )
   {
      GetDlgItem(IDC_ALLOWABLE_SPACING)->SetWindowText(_T(""));
      return FALSE; // girder spacing isn't input in this dialog
   }

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
            label.Format(_T("(%s to %s)"), 
               FormatDimension(m_MinGirderSpacing,pDisplayUnits->GetXSectionDimUnit()),
               FormatDimension(m_MaxGirderSpacing,pDisplayUnits->GetXSectionDimUnit()));
         }
         else
         {
            // this is actually joint spacing
            label.Format(_T("(%s to %s)"),
               FormatDimension(m_MinGirderSpacing, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(m_MaxGirderSpacing, pDisplayUnits->GetComponentDimUnit()));
         }
      }
      else
      {
         if (IsGirderSpacing(m_GirderSpacingType))
         {
            label.Format(_T("( %s or more )"), FormatDimension(m_MinGirderSpacing, pDisplayUnits->GetXSectionDimUnit()));
         }
         else
         {
            label.Format(_T("( %s or more )"), FormatDimension(m_MinGirderSpacing, pDisplayUnits->GetComponentDimUnit()));
         }
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

   const GirderLibraryEntry* pGdrEntry = nullptr;
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

   description += _T(", Deck: ") + CString(GetDeckTypeName(deckType));

   pgsTypes::AdjacentTransverseConnectivity connect = pgsTypes::atcConnectedAsUnit;

   // connectivity if adjacent
   if (IsAdjacentSpacing(m_GirderSpacingType))
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
      if ( IsAdjacentSpacing(m_GirderSpacingType) )
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
      // only a single girder family is used for the bridge. therefore, the same distribution factor engineering object
      // is used for all spans and girders. Getting the factory for any girder in the family will give us the
      // correct distribution factor engineer.
      std::_tstring entry_name = pGdrEntry->GetName();

      CComPtr<IDistFactorEngineer> dfEngineer;
      m_Factory->CreateDistFactorEngineer(pBroker, -1, &m_GirderSpacingType, &deckType, &connect, &dfEngineer);
      std::_tstring dfmethod = dfEngineer->GetComputationDescription(CGirderKey(0,0), entry_name, deckType, connect );

      description += _T("\r\n\r\nDistribution factors computed using ");
      description += dfmethod.c_str();
   }

   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SUPERSTRUCTURE_DESCRIPTION);
   pEdit->SetWindowText(description);
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
         m_strToolTipText = _T("Locates the girders, in plan view, relative to the Alignment or Bridge Line");
         bShowTip = TRUE;
         break;

      default:
         return FALSE;
      }

      if ( bShowTip )
      {
         ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
         ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tool tip
         pTTT->lpszText = m_strToolTipText.LockBuffer();
         pTTT->hinst = nullptr;
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
   if ( WBFL::System::Flags<Uint32>::IsClear(hintSettings,mask) && EAFShowUIHints(strText))
   {
      WBFL::System::Flags<Uint32>::Set(&hintSettings,mask);
      pDoc->SetUIHintSettings(hintSettings);
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


void CBridgeDescGeneralPage::UpdateConcreteTypeLabel()
{
   CString strLabel;
   strLabel.Format(_T("%s"), lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)m_JointConcrete.Type, true).c_str());
   GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText(strLabel);
}

void CBridgeDescGeneralPage::OnBnClickedEc()
{
   BOOL bEnable = m_ctrlEcCheck.GetCheck();

   GetDlgItem(IDC_EC_LABEL)->EnableWindow(TRUE);

   if (bEnable == FALSE)
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
      UpdateEc();
   }
   else
   {
      m_ctrlEc.SetWindowText(m_strUserEc);
   }

   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}

void CBridgeDescGeneralPage::OnMoreProperties()
{
   UpdateData(TRUE);
   CConcreteDetailsDlg dlg(true  /*properties are based on f'c*/,
      true /*include UHPC*/,
      false /*don't enable Compute Time Parameters option*/,
      false /*hide the CopyFromLibrary button*/);

   dlg.m_fc28 = m_JointConcrete.Fc;
   dlg.m_Ec28 = m_JointConcrete.Ec;
   dlg.m_bUserEc28 = m_JointConcrete.bUserEc;

   dlg.m_General.m_Type = m_JointConcrete.Type;
   dlg.m_General.m_AggSize = m_JointConcrete.MaxAggregateSize;
   dlg.m_General.m_Ds = m_JointConcrete.StrengthDensity;
   dlg.m_General.m_Dw = m_JointConcrete.WeightDensity;
   dlg.m_General.m_strUserEc = m_strUserEc;

   dlg.m_AASHTO.m_EccK1 = m_JointConcrete.EcK1;
   dlg.m_AASHTO.m_EccK2 = m_JointConcrete.EcK2;
   dlg.m_AASHTO.m_CreepK1 = m_JointConcrete.CreepK1;
   dlg.m_AASHTO.m_CreepK2 = m_JointConcrete.CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = m_JointConcrete.ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = m_JointConcrete.ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct = m_JointConcrete.bHasFct;
   dlg.m_AASHTO.m_Fct = m_JointConcrete.Fct;

   dlg.m_PCIUHPC.m_ffc = m_JointConcrete.Ffc;
   dlg.m_PCIUHPC.m_frr = m_JointConcrete.Frr;
   dlg.m_PCIUHPC.m_FiberLength = m_JointConcrete.FiberLength;
   dlg.m_PCIUHPC.m_AutogenousShrinkage = m_JointConcrete.AutogenousShrinkage;
   dlg.m_PCIUHPC.m_bPCTT = m_JointConcrete.bPCTT;

   dlg.m_UHPC.m_ftcri = m_JointConcrete.ftcri;
   dlg.m_UHPC.m_ftcr = m_JointConcrete.ftcr;
   dlg.m_UHPC.m_ftloc = m_JointConcrete.ftloc;
   dlg.m_UHPC.m_etloc = m_JointConcrete.etloc;
   dlg.m_UHPC.m_alpha_u = m_JointConcrete.alpha_u;
   dlg.m_UHPC.m_ecu = m_JointConcrete.ecu;
   dlg.m_UHPC.m_bExperimental_ecu = m_JointConcrete.bExperimental_ecu;
   dlg.m_UHPC.m_gamma_u = m_JointConcrete.gamma_u;
   dlg.m_UHPC.m_FiberLength = m_JointConcrete.FiberLength;

   if (dlg.DoModal() == IDOK)
   {
      m_JointConcrete.Fc = dlg.m_fc28;
      m_JointConcrete.Ec = dlg.m_Ec28;
      m_JointConcrete.bUserEc = dlg.m_bUserEc28;

      m_JointConcrete.Type = dlg.m_General.m_Type;
      m_JointConcrete.MaxAggregateSize = dlg.m_General.m_AggSize;
      m_JointConcrete.StrengthDensity = dlg.m_General.m_Ds;
      m_JointConcrete.WeightDensity = dlg.m_General.m_Dw;
      m_JointConcrete.EcK1 = dlg.m_AASHTO.m_EccK1;
      m_JointConcrete.EcK2 = dlg.m_AASHTO.m_EccK2;
      m_JointConcrete.CreepK1 = dlg.m_AASHTO.m_CreepK1;
      m_JointConcrete.CreepK2 = dlg.m_AASHTO.m_CreepK2;
      m_JointConcrete.ShrinkageK1 = dlg.m_AASHTO.m_ShrinkageK1;
      m_JointConcrete.ShrinkageK2 = dlg.m_AASHTO.m_ShrinkageK2;
      m_JointConcrete.bHasFct = dlg.m_AASHTO.m_bHasFct;
      m_JointConcrete.Fct = dlg.m_AASHTO.m_Fct;

      m_JointConcrete.Ffc = dlg.m_PCIUHPC.m_ffc;
      m_JointConcrete.Frr = dlg.m_PCIUHPC.m_frr;
      m_JointConcrete.FiberLength = dlg.m_PCIUHPC.m_FiberLength;
      m_JointConcrete.AutogenousShrinkage = dlg.m_PCIUHPC.m_AutogenousShrinkage;
      m_JointConcrete.bPCTT = dlg.m_PCIUHPC.m_bPCTT;

      m_JointConcrete.ftcri = dlg.m_UHPC.m_ftcri;
      m_JointConcrete.ftcr = dlg.m_UHPC.m_ftcr;
      m_JointConcrete.ftloc = dlg.m_UHPC.m_ftloc;
      m_JointConcrete.etloc = dlg.m_UHPC.m_etloc;
      m_JointConcrete.alpha_u = dlg.m_UHPC.m_alpha_u;
      m_JointConcrete.bExperimental_ecu = dlg.m_UHPC.m_bExperimental_ecu;
      m_JointConcrete.ecu = dlg.m_UHPC.m_ecu;
      m_JointConcrete.gamma_u = dlg.m_UHPC.m_gamma_u;
      m_JointConcrete.FiberLength = dlg.m_UHPC.m_FiberLength;

      m_strUserEc = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      UpdateData(FALSE);
      OnBnClickedEc();
      UpdateConcreteTypeLabel();
   }
}

void CBridgeDescGeneralPage::OnChangeSpacing()
{
   if (IsJointSpacing(m_GirderSpacingType) && IsBridgeSpacing(m_GirderSpacingType))
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      CDataExchange dx(this, TRUE);
      DDX_UnitValueAndTag(&dx, IDC_SPACING, IDC_SPACING_UNIT, m_GirderSpacing, pDisplayUnits->GetComponentDimUnit());
      UpdateBridgeDescription();
      EnableLongitudinalJointMaterial();
   }
}

void CBridgeDescGeneralPage::OnChangeFc()
{
   UpdateEc();
}

void CBridgeDescGeneralPage::UpdateEc()
{
   // update modulus
   if (m_ctrlEcCheck.GetCheck() == 0)
   {
      // blank out ec
      CString strEc;
      m_ctrlEc.SetWindowText(strEc);

      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      strDensity.Format(_T("%s"), FormatDimension(m_JointConcrete.StrengthDensity, pDisplayUnits->GetDensityUnit(), false));
      strK1.Format(_T("%f"), m_JointConcrete.EcK1);
      strK2.Format(_T("%f"), m_JointConcrete.EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(m_JointConcrete.Type,strFc, strDensity, strK1, strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}
