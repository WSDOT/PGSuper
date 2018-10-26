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

// PierGirderSpacingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "PierGirderSpacingPage.h"
#include "PierDetailsDlg.h"
#include "SelectItemDlg.h"
#include "PGSuperColors.h"
#include "HtmlHelp\HelpTopics.hh"

#include <PGSuperUnits.h>

#include <PgsExt\BridgeDescription.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#include <MfcTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


inline void GetBearingMeasurementTypes(const CString& startConnectionName, const CString& endConnectionName, ConnectionLibraryEntry::BearingOffsetMeasurementType* pStartMeasure, ConnectionLibraryEntry::BearingOffsetMeasurementType* pEndMeasure)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLib);

   const ConnectionLibraryEntry* pConEntry = pLib->GetConnectionEntry(startConnectionName);
   *pStartMeasure = pConEntry->GetBearingOffsetMeasurementType();

   pConEntry = pLib->GetConnectionEntry(endConnectionName);
   *pEndMeasure = pConEntry->GetBearingOffsetMeasurementType();
}


/////////////////////////////////////////////////////////////////////////////
// CPierGirderSpacingPage property page

IMPLEMENT_DYNCREATE(CPierGirderSpacingPage, CPropertyPage)

CPierGirderSpacingPage::CPierGirderSpacingPage() : CPropertyPage(CPierGirderSpacingPage::IDD)
{
   m_pPier = 0;

	//{{AFX_DATA_INIT(CPierGirderSpacingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_MinGirderCount[pgsTypes::Back]  = 2;
   m_MinGirderCount[pgsTypes::Ahead] = 2;

   m_NumGirdersHyperLink[pgsTypes::Back].m_wParam  = (WPARAM)pgsTypes::Back;
   m_NumGirdersHyperLink[pgsTypes::Ahead].m_wParam = (WPARAM)pgsTypes::Ahead;

   m_GirderSpacingHyperLink[pgsTypes::Back].m_wParam  = (WPARAM)pgsTypes::Back;
   m_GirderSpacingHyperLink[pgsTypes::Ahead].m_wParam = (WPARAM)pgsTypes::Ahead;

   m_SlabOffsetHyperLink[pgsTypes::Back].m_wParam  = (WPARAM)pgsTypes::Back;
   m_SlabOffsetHyperLink[pgsTypes::Ahead].m_wParam = (WPARAM)pgsTypes::Ahead;
}

CPierGirderSpacingPage::~CPierGirderSpacingPage()
{
}

void CPierGirderSpacingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   if (!pDX->m_bSaveAndValidate)
   {
      ConnectionLibraryEntry::BearingOffsetMeasurementType back_measure, ahead_measure;
      GetBearingMeasurementTypes(pParent->m_ConnectionName[pgsTypes::Back], pParent->m_ConnectionName[pgsTypes::Ahead], &back_measure, &ahead_measure);

      FillGirderSpacingMeasurementComboBox(IDC_PREV_SPAN_SPACING_MEASUREMENT,back_measure);
      FillGirderSpacingMeasurementComboBox(IDC_NEXT_SPAN_SPACING_MEASUREMENT,ahead_measure);
   }


	//{{AFX_DATA_MAP(CPierGirderSpacingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_NUMGDR_SPIN_PREV_SPAN,                  m_NumGdrSpinner[pgsTypes::Back]);
	DDX_Control(pDX, IDC_NUMGDR_SPIN_NEXT_SPAN,                  m_NumGdrSpinner[pgsTypes::Ahead]);
   DDX_Control(pDX, IDC_NUM_GIRDER_PREV_SPAN_NOTE,              m_NumGirdersHyperLink[pgsTypes::Back]);
   DDX_Control(pDX, IDC_NUM_GIRDER_NEXT_SPAN_NOTE,              m_NumGirdersHyperLink[pgsTypes::Ahead]);
   DDX_Control(pDX, IDC_GIRDER_SPACING_NOTE_END_OF_PREV_SPAN,   m_GirderSpacingHyperLink[pgsTypes::Back]);
   DDX_Control(pDX, IDC_GIRDER_SPACING_NOTE_START_OF_NEXT_SPAN, m_GirderSpacingHyperLink[pgsTypes::Ahead]);
   DDX_Control(pDX, IDC_BACK_SLAB_OFFSET_NOTE,                  m_SlabOffsetHyperLink[pgsTypes::Back]);
   DDX_Control(pDX, IDC_AHEAD_SLAB_OFFSET_NOTE,                 m_SlabOffsetHyperLink[pgsTypes::Ahead]);
   DDX_Control(pDX, IDC_PREV_SPAN_SPACING_MEASUREMENT,          m_cbGirderSpacingMeasurement[pgsTypes::Back]);
   DDX_Control(pDX, IDC_NEXT_SPAN_SPACING_MEASUREMENT,          m_cbGirderSpacingMeasurement[pgsTypes::Ahead]);

   DDX_CBItemData(pDX, IDC_PREV_SPAN_SPACING_MEASUREMENT, m_GirderSpacingMeasure[pgsTypes::Back]);
   DDX_CBItemData(pDX, IDC_NEXT_SPAN_SPACING_MEASUREMENT, m_GirderSpacingMeasure[pgsTypes::Ahead]);

   DDX_Text( pDX, IDC_NUMGDR_PREV_SPAN, m_nGirders[pgsTypes::Back] );
   DDV_MinMaxLong(pDX, (long)m_nGirders[pgsTypes::Back], (long)m_MinGirderCount[pgsTypes::Back], MAX_GIRDERS_PER_SPAN );

   DDX_Text( pDX, IDC_NUMGDR_NEXT_SPAN, m_nGirders[pgsTypes::Ahead] );
   DDV_MinMaxLong(pDX, (long)m_nGirders[pgsTypes::Ahead], (long)m_MinGirderCount[pgsTypes::Ahead], MAX_GIRDERS_PER_SPAN );

   DDV_SpacingGrid(pDX,IDC_NEXT_SPAN_SPACING_GRID,&m_GirderSpacingGrid[pgsTypes::Ahead]);
   DDV_SpacingGrid(pDX,IDC_PREV_SPAN_SPACING_GRID,&m_GirderSpacingGrid[pgsTypes::Back]);


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER, m_RefGirderIdx[pgsTypes::Back]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER, m_RefGirderIdx[pgsTypes::Ahead]);

   DDX_OffsetAndTag(pDX, IDC_PREV_REF_GIRDER_OFFSET,IDC_PREV_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::Back], pDisplayUnits->GetXSectionDimUnit());
   DDX_OffsetAndTag(pDX, IDC_NEXT_REF_GIRDER_OFFSET,IDC_NEXT_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::Ahead], pDisplayUnits->GetXSectionDimUnit());

   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::Back]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::Ahead]);

   DDX_Tag(pDX, IDC_BACK_SLAB_OFFSET_UNIT,  pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_AHEAD_SLAB_OFFSET_UNIT, pDisplayUnits->GetComponentDimUnit() );
   if ( pParent->m_pBridge->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   {
      if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && m_SlabOffsetType == pgsTypes::sotSpan) )
      {
         DDX_UnitValueAndTag(pDX, IDC_BACK_SLAB_OFFSET,  IDC_BACK_SLAB_OFFSET_UNIT,  m_SlabOffset[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit() );
         DDX_UnitValueAndTag(pDX, IDC_AHEAD_SLAB_OFFSET, IDC_AHEAD_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit() );
      }
   }
}


BEGIN_MESSAGE_MAP(CPierGirderSpacingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierGirderSpacingPage)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUMGDR_SPIN_PREV_SPAN, OnNumGirdersPrevSpanChanged)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUMGDR_SPIN_NEXT_SPAN, OnNumGirdersNextSpanChanged)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BACK_COPY, OnCopyToAheadSide)
	ON_BN_CLICKED(IDC_AHEAD_COPY, OnCopyToBackSide)
   ON_CBN_SELCHANGE(IDC_PREV_SPAN_SPACING_MEASUREMENT,OnBackPierSpacingDatumChanged)
   ON_CBN_SELCHANGE(IDC_NEXT_SPAN_SPACING_MEASUREMENT,OnAheadPierSpacingDatumChanged)
   ON_REGISTERED_MESSAGE(MsgChangeSameNumberOfGirders,OnChangeSameNumberOfGirders)
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderSpacing,OnChangeSameGirderSpacing)
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffset,OnChangeSlabOffset)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierGirderSpacingPage message handlers
void CPierGirderSpacingPage::Init(const CPierData* pPier)
{
   m_pPier = pPier;
   m_pPrevSpan = m_pPier->GetPrevSpan();
   m_pNextSpan = m_pPier->GetNextSpan();

   m_GirderSpacingType  = pPier->GetBridgeDescription()->GetGirderSpacingType();
   m_GirderSpacingMeasurementLocation = pPier->GetBridgeDescription()->GetMeasurementLocation();
   m_bUseSameNumGirders = pPier->GetBridgeDescription()->UseSameNumberOfGirdersInAllSpans();


   // slab offset
   m_SlabOffsetType = pPier->GetBridgeDescription()->GetSlabOffsetType();
   if ( m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotGirder )
      m_SlabOffsetTypeCache = pgsTypes::sotSpan;
   else
      m_SlabOffsetTypeCache = pgsTypes::sotBridge;

   // use a dummy skew angle for basic initialization... set it to the correct value OnInitialDialog
   double skew_angle = 0;

   if ( m_pPrevSpan )
   {
      const CGirderSpacing* pGirderSpacing = m_pPrevSpan->GetGirderSpacing(pgsTypes::metEnd);

      m_nGirders[pgsTypes::Back]             = m_pPrevSpan->GetGirderCount();
      pgsTypes::MeasurementLocation ml       = pGirderSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mt       = pGirderSpacing->GetMeasurementType();
      m_GirderSpacingMeasure[pgsTypes::Back] = HashGirderSpacing(ml,mt);

      m_GirderSpacingGrid[pgsTypes::Back].Init(m_GirderSpacingType,
                                               m_bUseSameNumGirders,
                                               pGirderSpacing,
                                               m_pPrevSpan->GetGirderTypes(),
                                               pgsTypes::Back,
                                               m_pPier->GetPierIndex(),
                                               skew_angle,
                                               m_pNextSpan == NULL ? true : false);

      m_RefGirderIdx[pgsTypes::Back]        = pGirderSpacing->GetRefGirder();
      m_RefGirderOffset[pgsTypes::Back]     = pGirderSpacing->GetRefGirderOffset();
      m_RefGirderOffsetType[pgsTypes::Back] = pGirderSpacing->GetRefGirderOffsetType();

      if ( m_SlabOffsetType == pgsTypes::sotGirder )
      {
         // slab offset is unique for each girder... which one do we use??? For now, use girder 0
         m_SlabOffset[pgsTypes::Back]  = m_pPrevSpan->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metEnd);
      }
      else
      {
         m_SlabOffset[pgsTypes::Back]  = m_pPrevSpan->GetSlabOffset(pgsTypes::metEnd);
      }
   }
   else
   {
      m_nGirders[pgsTypes::Back] = 0;
      m_GirderSpacingMeasure[pgsTypes::Back] = 0;

      m_RefGirderIdx[pgsTypes::Back]        = INVALID_INDEX;
      m_RefGirderOffset[pgsTypes::Back]     = 0;
      m_RefGirderOffsetType[pgsTypes::Back] = pgsTypes::omtBridge;
   }

   if ( m_pNextSpan )
   {
      const CGirderSpacing* pGirderSpacing = m_pNextSpan->GetGirderSpacing(pgsTypes::metStart);

      m_nGirders[pgsTypes::Ahead]             = m_pNextSpan->GetGirderCount();
      pgsTypes::MeasurementLocation ml        = pGirderSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mt        = pGirderSpacing->GetMeasurementType();
      m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(ml,mt);

      m_GirderSpacingGrid[pgsTypes::Ahead].Init(m_GirderSpacingType,
                                                m_bUseSameNumGirders,
                                                pGirderSpacing,
                                                m_pNextSpan->GetGirderTypes(),
                                                pgsTypes::Ahead,
                                                m_pPier->GetPierIndex(),
                                                skew_angle,
                                                m_pPrevSpan == NULL ? true : false);

      m_RefGirderIdx[pgsTypes::Ahead]        = pGirderSpacing->GetRefGirder();
      m_RefGirderOffset[pgsTypes::Ahead]     = pGirderSpacing->GetRefGirderOffset();
      m_RefGirderOffsetType[pgsTypes::Ahead] = pGirderSpacing->GetRefGirderOffsetType();

      if ( m_SlabOffsetType == pgsTypes::sotGirder )
      {
         // slab offset is unique for each girder... which one do we use??? For now, use girder 0
         m_SlabOffset[pgsTypes::Ahead]  = m_pNextSpan->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metStart);
      }
      else
      {
         m_SlabOffset[pgsTypes::Ahead]  = m_pNextSpan->GetSlabOffset(pgsTypes::metStart);
      }
   }
   else 
   {
      m_nGirders[pgsTypes::Ahead] = 0;
      m_GirderSpacingMeasure[pgsTypes::Ahead] = 0;

      m_RefGirderIdx[pgsTypes::Ahead]        = INVALID_INDEX;
      m_RefGirderOffset[pgsTypes::Ahead]     = 0;
      m_RefGirderOffsetType[pgsTypes::Ahead] = pgsTypes::omtBridge;
   }

   m_nGirdersCache[pgsTypes::Back]  = m_nGirders[pgsTypes::Back];
   m_nGirdersCache[pgsTypes::Ahead] = m_nGirders[pgsTypes::Ahead];

   m_GirderSpacingMeasureCache[pgsTypes::Back]  = m_GirderSpacingMeasure[pgsTypes::Back];
   m_GirderSpacingMeasureCache[pgsTypes::Ahead] = m_GirderSpacingMeasure[pgsTypes::Ahead];
}

BOOL CPierGirderSpacingPage::OnInitDialog() 
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   double skew_angle;
   pBridge->GetSkewAngle(pParent->GetStation(),pParent->GetOrientation(),&skew_angle);

   m_GirderSpacingGrid[pgsTypes::Back].SubclassDlgItem(IDC_PREV_SPAN_SPACING_GRID,this);
   if ( m_pPrevSpan )
   {
      const CGirderSpacing* pGirderSpacing = m_pPrevSpan->GetGirderSpacing(pgsTypes::metEnd);
      m_GirderSpacingGrid[pgsTypes::Back].CustomInit();
      m_GirderSpacingGrid[pgsTypes::Back].SetPierSkewAngle(skew_angle);

      CString strTxt;
      strTxt.Format(_T("Back side of %s %d = End of Span %d"),m_pNextSpan ? _T("Pier") :_T("Abutment"),LABEL_SPAN(m_pPier->GetPierIndex()),LABEL_SPAN(m_pPrevSpan->GetSpanIndex()));
      GetDlgItem(IDC_BACKGROUP)->SetWindowText(strTxt);
   }
   else
   {
      m_GirderSpacingGrid[pgsTypes::Back].Initialize();

      // hide the spacing on the back side of pier
      GetDlgItem(IDC_BACKGROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUMGDR_SPIN_PREV_SPAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUMGDR_PREV_SPAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUMGDR_PREV_SPAN_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_SPAN_SPACING_GRID)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_GIRDER_SPACING_NOTE_END_OF_PREV_SPAN )->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUM_GIRDER_PREV_SPAN_NOTE )->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_BACK_COPY)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_REF_GIRDER)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_BACK_SLAB_OFFSET_NOTE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);

      // move the ahead side input
      CRect rClient;
      GetDlgItem(IDC_BACKGROUP)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEADGROUP)->MoveWindow(rClient);

      GetDlgItem(IDC_NUMGDR_SPIN_PREV_SPAN)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NUMGDR_SPIN_NEXT_SPAN)->MoveWindow(rClient);

      GetDlgItem(IDC_NUMGDR_PREV_SPAN)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NUMGDR_NEXT_SPAN)->MoveWindow(rClient);

      GetDlgItem(IDC_NUMGDR_PREV_SPAN_LABEL)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NUMGDR_NEXT_SPAN_LABEL)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_SPAN_SPACING_GRID)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_SPAN_SPACING_GRID)->MoveWindow(rClient);

      GetDlgItem(IDC_GIRDER_SPACING_NOTE_END_OF_PREV_SPAN )->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_GIRDER_SPACING_NOTE_START_OF_NEXT_SPAN)->MoveWindow(rClient);

      GetDlgItem(IDC_NUM_GIRDER_PREV_SPAN_NOTE )->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NUM_GIRDER_NEXT_SPAN_NOTE)->MoveWindow(rClient);

      GetDlgItem(IDC_AHEAD_COPY)->ShowWindow(SW_HIDE);


      GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_REF_GIRDER)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_REF_GIRDER)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->MoveWindow(rClient);

      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->MoveWindow(rClient);

      GetDlgItem(IDC_BACK_SLAB_OFFSET_NOTE)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_NOTE)->MoveWindow(rClient);

      GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->MoveWindow(rClient);

      GetDlgItem(IDC_BACK_SLAB_OFFSET)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->MoveWindow(rClient);

      GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->GetWindowRect(rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->MoveWindow(rClient);
   }


   m_GirderSpacingGrid[pgsTypes::Ahead].SubclassDlgItem(IDC_NEXT_SPAN_SPACING_GRID,this);
   if ( m_pNextSpan )
   {
      const CGirderSpacing* pGirderSpacing = m_pNextSpan->GetGirderSpacing(pgsTypes::metStart);
      m_GirderSpacingGrid[pgsTypes::Ahead].CustomInit();
      m_GirderSpacingGrid[pgsTypes::Ahead].SetPierSkewAngle(skew_angle);
   
      CString strTxt;
      strTxt.Format(_T("Ahead side of %s %d = Start of Span %d"),m_pPrevSpan ? _T("Pier") :_T("Abutment"),LABEL_SPAN(m_pPier->GetPierIndex()),LABEL_SPAN(m_pNextSpan->GetSpanIndex()));
      GetDlgItem(IDC_AHEADGROUP)->SetWindowText(strTxt);
   }
   else 
   {
      GetDlgItem(IDC_AHEADGROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUMGDR_SPIN_NEXT_SPAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUMGDR_NEXT_SPAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUMGDR_NEXT_SPAN_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_SPAN_SPACING_GRID)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_GIRDER_SPACING_NOTE_START_OF_NEXT_SPAN )->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NUM_GIRDER_NEXT_SPAN_NOTE )->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_BACK_COPY)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_AHEAD_COPY)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_REF_GIRDER)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_NOTE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);

      m_nGirders[pgsTypes::Ahead] = 0;
      m_GirderSpacingGrid[pgsTypes::Ahead].Initialize();
      m_GirderSpacingMeasure[pgsTypes::Ahead] = 0;
   }


   m_GirderSpacingCache[pgsTypes::Back]  = m_GirderSpacingGrid[pgsTypes::Back].GetGirderSpacingData();
   m_GirderSpacingCache[pgsTypes::Ahead] = m_GirderSpacingGrid[pgsTypes::Ahead].GetGirderSpacingData();

   FillRefGirderComboBox(pgsTypes::Back);
   FillRefGirderComboBox(pgsTypes::Ahead);
   FillRefGirderOffsetTypeComboBox(pgsTypes::Back);
   FillRefGirderOffsetTypeComboBox(pgsTypes::Ahead);

   
   CPropertyPage::OnInitDialog();
	


   // Initialize the spinner control for # of girder lines
   m_MinGirderCount[pgsTypes::Back]  = GetMinGirderCount(m_pPrevSpan);
   m_MinGirderCount[pgsTypes::Ahead] = GetMinGirderCount(m_pNextSpan);
   m_NumGdrSpinner[pgsTypes::Back].SetRange( short(m_MinGirderCount[pgsTypes::Back]), MAX_GIRDERS_PER_SPAN);
   m_NumGdrSpinner[pgsTypes::Ahead].SetRange(short(m_MinGirderCount[pgsTypes::Ahead]),MAX_GIRDERS_PER_SPAN);

   // restrict the spinner to only go one girder at a time.
   UDACCEL accel;
   accel.nInc = 1;
   accel.nSec = 0;
   m_NumGdrSpinner[pgsTypes::Back].SetAccel(0,&accel);
   m_NumGdrSpinner[pgsTypes::Ahead].SetAccel(0,&accel);
	
   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
   }
   else
   {
      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
   }

   //CComPtr<IBroker> pBroker;
   //EAFGetBroker(&pBroker);
   //GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   //if ( pBridgeDesc->IsPostTensioningModeled() )
   //{
   //   // disable elements of the UI that can't be changed when the bridge model has PT
   //   GetDlgItem(IDC_NUM_GIRDER_PREV_SPAN_NOTE)->EnableWindow(FALSE);
   //   GetDlgItem(IDC_NUM_GIRDER_NEXT_SPAN_NOTE)->EnableWindow(FALSE);
   //}

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CPierGirderSpacingPage::FillGirderSpacingMeasurementComboBox(int nIDC, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure)
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(nIDC);
   pSpacingType->ResetContent();

   int idx = pSpacingType->AddString(_T("Measured at and along the CL pier"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtCenterlinePier,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   
   idx = pSpacingType->AddString(_T("Measured normal to alignment at CL pier"));
   item_data = HashGirderSpacing(pgsTypes::AtCenterlinePier,pgsTypes::NormalToItem);
   pSpacingType->SetItemData(idx,item_data);

   if (bearingMeasure!=ConnectionLibraryEntry::AlongGirder)
   {
      idx = pSpacingType->AddString(_T("Measured at and along the CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem);
      pSpacingType->SetItemData(idx,item_data);

      idx = pSpacingType->AddString(_T("Measured normal to alignment at CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem);
      pSpacingType->SetItemData(idx,item_data);
   }
}

void CPierGirderSpacingPage::FillRefGirderOffsetTypeComboBox(pgsTypes::PierFaceType pierFace)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(pierFace == pgsTypes::Back ? IDC_PREV_REF_GIRDER_OFFSET_TYPE : IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   int idx = pCB->AddString(_T("Alignment"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtAlignment);

   idx = pCB->AddString(_T("Bridge Line"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtBridge);
}

void CPierGirderSpacingPage::FillRefGirderComboBox(pgsTypes::PierFaceType pierFace)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(pierFace == pgsTypes::Back ? IDC_PREV_REF_GIRDER : IDC_NEXT_REF_GIRDER);
   int curSel = pCB->GetCurSel();
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Center of Girders"));
   pCB->SetItemData(idx,(DWORD_PTR)INVALID_INDEX);

   for ( GirderIndexType i = 0; i < m_nGirders[pierFace]; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD_PTR)i);
   }

   pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel);
}

void CPierGirderSpacingPage::OnNumGirdersPrevSpanChanged(NMHDR* pNMHDR,LRESULT* pResult)
{
   OnNumGirdersChanged(pNMHDR,pResult,pgsTypes::Back);
}

void CPierGirderSpacingPage::OnNumGirdersNextSpanChanged(NMHDR* pNMHDR,LRESULT* pResult)
{
   OnNumGirdersChanged(pNMHDR,pResult,pgsTypes::Ahead);
}

void CPierGirderSpacingPage::OnNumGirdersChanged(NMHDR* pNMHDR,LRESULT* pResult,pgsTypes::PierFaceType pierFace)
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   int nGirders = pNMUpDown->iPos + pNMUpDown->iDelta;
   if ( nGirders < int(m_MinGirderCount[pierFace]) )
   {
      // rolling past the bottom... going back up to MAX_GIRDERS_PER_SPAN
      AddGirders( MAX_GIRDERS_PER_SPAN - m_nGirders[pierFace], pierFace );
   }
   else if ( MAX_GIRDERS_PER_SPAN < nGirders )
   {
      RemoveGirders( m_nGirders[pierFace] - m_MinGirderCount[pierFace], pierFace );
   }
   else
   {
	   if ( pNMUpDown->iDelta < 0 )
         RemoveGirders(-pNMUpDown->iDelta, pierFace);
      else
         AddGirders(pNMUpDown->iDelta, pierFace);
   }

   UpdateGirderSpacingState(pierFace);
   UpdateCopyButtonState(m_nGirders[pgsTypes::Ahead] == m_nGirders[pgsTypes::Back]);

   *pResult = 0;
}

void CPierGirderSpacingPage::AddGirders(GirderIndexType nGirders,pgsTypes::PierFaceType pierFace)
{
   ASSERT( !m_bUseSameNumGirders );

   m_nGirders[pierFace] += nGirders;
   m_GirderSpacingGrid[pierFace].AddGirders(nGirders);
   m_GirderSpacingCache[pierFace]  = m_GirderSpacingGrid[pierFace].GetGirderSpacingData();
}

void CPierGirderSpacingPage::RemoveGirders(GirderIndexType nGirders,pgsTypes::PierFaceType pierFace)
{
   ASSERT( !m_bUseSameNumGirders );
   m_nGirders[pierFace] -= nGirders;
   m_GirderSpacingGrid[pierFace].RemoveGirders(nGirders);
   m_GirderSpacingCache[pierFace]  = m_GirderSpacingGrid[pierFace].GetGirderSpacingData();
}

void CPierGirderSpacingPage::SetGirderCount(GirderIndexType nGirders,pgsTypes::PierFaceType pierFace)
{
   m_nGirders[pierFace] = nGirders;
   m_GirderSpacingGrid[pierFace].SetGirderCount(nGirders);
   m_NumGdrSpinner[pierFace].SetPos((int)nGirders);
   m_GirderSpacingCache[pierFace]  = m_GirderSpacingGrid[pierFace].GetGirderSpacingData();
}

HBRUSH CPierGirderSpacingPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);

   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_NUM_GIRDER_PREV_SPAN_NOTE:
   case IDC_NUM_GIRDER_NEXT_SPAN_NOTE:
   case IDC_GIRDER_SPACING_NOTE_END_OF_PREV_SPAN:
   case IDC_GIRDER_SPACING_NOTE_START_OF_NEXT_SPAN:
   case IDC_BACK_SLAB_OFFSET_NOTE:
   case IDC_AHEAD_SLAB_OFFSET_NOTE:
      pDC->SetTextColor(HYPERLINK_COLOR);
      break;
   };

   return hbr;
}

BOOL CPierGirderSpacingPage::OnSetActive() 
{
   UpdateChildWindowState();
	BOOL bResult = CPropertyPage::OnSetActive();

   GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::Ahead]);
   GetDlgItem(IDC_BACK_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::Back]);
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->SetWindowText(_T(""));
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->SetWindowText(_T(""));
   }

   return bResult;
}


void CPierGirderSpacingPage::DisableAll()
{
   CWnd* pWnd = GetTopWindow();
   while ( pWnd  )
   {
      pWnd->EnableWindow(FALSE);
      pWnd = pWnd->GetNextWindow();
   }

   m_GirderSpacingGrid[pgsTypes::Ahead].Enable(FALSE);
   m_GirderSpacingGrid[pgsTypes::Back].Enable(FALSE);
}

void CPierGirderSpacingPage::UpdateChildWindowState()
{
   // Generally enable/disable everything if there is a prev/next span
   // The enable/disable will be refined later in this function

   // Prev Span
   BOOL bEnable = (m_pPrevSpan ? (m_bUseSameNumGirders ? FALSE : TRUE) : FALSE);
   GetDlgItem(IDC_NUMGDR_PREV_SPAN_LABEL)->EnableWindow(        bEnable );
   GetDlgItem(IDC_NUMGDR_PREV_SPAN)->EnableWindow(              bEnable );
   GetDlgItem(IDC_NUMGDR_SPIN_PREV_SPAN)->EnableWindow(         bEnable );
   GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->EnableWindow(       m_pPrevSpan ? TRUE : FALSE );
   GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT)->EnableWindow( m_pPrevSpan ? TRUE : FALSE );
   GetDlgItem(IDC_PREV_SPAN_SPACING_GRID)->EnableWindow(        m_pPrevSpan ? TRUE : FALSE );
   GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(               m_pPrevSpan ? TRUE : FALSE );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(        m_pPrevSpan ? TRUE : FALSE );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   m_pPrevSpan ? TRUE : FALSE );

   bEnable = (m_pPrevSpan ? (m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotGirder ? FALSE : TRUE) : FALSE);
   GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);

   // Next Span
   bEnable = (m_pNextSpan ? (m_bUseSameNumGirders ? FALSE : TRUE) : FALSE);
   GetDlgItem(IDC_NUMGDR_NEXT_SPAN_LABEL)->EnableWindow(         bEnable );
   GetDlgItem(IDC_NUMGDR_NEXT_SPAN)->EnableWindow(               bEnable );
   GetDlgItem(IDC_NUMGDR_SPIN_NEXT_SPAN)->EnableWindow(          bEnable );
   GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->EnableWindow(       m_pNextSpan ? TRUE : FALSE );
   GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT)->EnableWindow( m_pNextSpan ? TRUE : FALSE );
   GetDlgItem(IDC_NEXT_SPAN_SPACING_GRID)->EnableWindow(        m_pNextSpan ? TRUE : FALSE );
   GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(                m_pNextSpan ? TRUE : FALSE );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(         m_pNextSpan ? TRUE : FALSE );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(    m_pNextSpan ? TRUE : FALSE );

   bEnable = (m_pNextSpan ? (m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotGirder ? FALSE : TRUE) : FALSE);
   GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);
 
   UpdateGirderSpacingHyperLinkText();
   UpdateNumGirdersHyperLinkText();
   UpdateSlabOffsetHyperLinkText();

   //////////// Back Side of Pier
   if ( m_pPrevSpan )
      UpdateGirderSpacingState(pgsTypes::Back);

   //////////// Ahead Side of Pier
   if ( m_pNextSpan )
      UpdateGirderSpacingState(pgsTypes::Ahead);

   UpdateCopyButtonState(m_NumGdrSpinner[pgsTypes::Back].GetPos() == m_NumGdrSpinner[pgsTypes::Ahead].GetPos());
}

void CPierGirderSpacingPage::OnCopyToAheadSide() 
{
   CComboBox* pBackMeasure = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
   int curSel = pBackMeasure->GetCurSel();

   CComboBox* pAheadMeasure = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
   pAheadMeasure->SetCurSel(curSel);

   m_GirderSpacingGrid[pgsTypes::Ahead].SetGirderSpacingData( m_GirderSpacingGrid[pgsTypes::Back].GetGirderSpacingData() );
   m_GirderSpacingGrid[pgsTypes::Ahead].FillGrid();

   ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER))->GetCurSel() );
   ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE))->GetCurSel() );

   CString strWndTxt;
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->GetWindowText( strWndTxt );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->SetWindowText( strWndTxt );

   if ( m_GirderSpacingType == pgsTypes::sotSpan )
   {
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->GetWindowText( strWndTxt );
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->SetWindowText( strWndTxt );
   }
}

void CPierGirderSpacingPage::OnCopyToBackSide() 
{
   CComboBox* pAheadMeasure = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
   int curSel = pAheadMeasure->GetCurSel();

   CComboBox* pBackMeasure = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
   pBackMeasure->SetCurSel(curSel);

   m_GirderSpacingGrid[pgsTypes::Back].SetGirderSpacingData( m_GirderSpacingGrid[pgsTypes::Ahead].GetGirderSpacingData() );
   m_GirderSpacingGrid[pgsTypes::Back].FillGrid();

   ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER))->GetCurSel() );
   ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE))->GetCurSel() );

   CString strWndTxt;
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->GetWindowText( strWndTxt );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->SetWindowText( strWndTxt );

   if ( m_GirderSpacingType == pgsTypes::sotSpan )
   {
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->GetWindowText( strWndTxt );
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->SetWindowText( strWndTxt );
   }
}

int CPierGirderSpacingPage::GetMinGirderCount(const CSpanData* pSpan)
{
   if ( !pSpan )
      return 0;

   // It is ok to use girder 0 here because all the girders within the span
   // are of the same family. All the girders in the span will have the
   // same factory
   std::_tstring strGdrName = pSpan->GetGirderTypes()->GetGirderName(0);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGdrName.c_str());

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   CComPtr<IGirderSection> section;
   factory->CreateGirderSection(pBroker,0,INVALID_INDEX,INVALID_INDEX,pGdrEntry->GetDimensions(),&section);

   WebIndexType nWebs;
   section->get_WebCount(&nWebs);

   return (1 < nWebs ? 1 : 2);
}

void CPierGirderSpacingPage::UpdateGirderSpacingState(pgsTypes::PierFaceType pierFace)
{
   BOOL bEnable = m_GirderSpacingGrid[pierFace].InputSpacing();

   if ( m_nGirders[pierFace] == 1 || IsBridgeSpacing(m_GirderSpacingType) )
   {
      // if there is only 1 girder or we are input the spacing for the whole bridge
      // (not span by span) then disable the input controls
      bEnable = FALSE;
   }

   m_cbGirderSpacingMeasurement[pierFace].EnableWindow(bEnable);
   m_GirderSpacingGrid[pierFace].Enable(bEnable);

   if ( pierFace == pgsTypes::Back )
   {
      GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(               bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );
   }
   else
   {
      GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(               bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );
   }
}

void CPierGirderSpacingPage::UpdateCopyButtonState(BOOL bEnable)
{
   if ( IsBridgeSpacing(m_GirderSpacingType) )
   {
      bEnable = FALSE; // nothing to copy if same girder spacing for entire bridge
   }

   if ( !m_GirderSpacingGrid[pgsTypes::Ahead].InputSpacing() || !m_GirderSpacingGrid[pgsTypes::Back].InputSpacing() )
      bEnable = FALSE; // spacing range is zero so there isn't any spacing to input


   GetDlgItem(IDC_BACK_COPY)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_COPY)->EnableWindow(bEnable);
}

LRESULT CPierGirderSpacingPage::OnChangeSameNumberOfGirders(WPARAM wParam,LPARAM lParam)
{
   // reverse the flag
   m_bUseSameNumGirders = !m_bUseSameNumGirders;

   GirderIndexType nGirders[2];
   if ( !m_bUseSameNumGirders )
   {
      // span by span input was just enabled
      nGirders[pgsTypes::Back]  = m_nGirdersCache[pgsTypes::Back];
      nGirders[pgsTypes::Ahead] = m_nGirdersCache[pgsTypes::Ahead];
   }
   else
   {
      // span by span input was just disabled

      // cache the current values
      m_nGirdersCache[pgsTypes::Ahead] = m_nGirders[pgsTypes::Ahead];
      m_nGirdersCache[pgsTypes::Back]  = m_nGirders[pgsTypes::Back];

      pgsTypes::PierFaceType pierFace = (pgsTypes::PierFaceType)(wParam);
      nGirders[pgsTypes::Back]  = m_nGirders[pierFace];
      nGirders[pgsTypes::Ahead] = nGirders[pgsTypes::Back];
   }


   if ( m_pPrevSpan )
   {
      m_GirderSpacingGrid[pgsTypes::Back].SharedGirderCount(m_bUseSameNumGirders);
      SetGirderCount(nGirders[pgsTypes::Back],pgsTypes::Back);
   }

   if ( m_pNextSpan )
   {
      m_GirderSpacingGrid[pgsTypes::Ahead].SharedGirderCount(m_bUseSameNumGirders);
      SetGirderCount(nGirders[pgsTypes::Ahead],pgsTypes::Ahead);
   }

   UpdateChildWindowState();
   return 0;
}

void CPierGirderSpacingPage::ToggleGirderSpacingType()
{
   if ( m_GirderSpacingType == pgsTypes::sbsUniform )      // uniform to general
      m_GirderSpacingType = pgsTypes::sbsGeneral; 
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneral ) // general to uniform
      m_GirderSpacingType = pgsTypes::sbsUniform;
   else if ( m_GirderSpacingType == pgsTypes::sbsUniformAdjacent ) // uniform adjacent to general adjacent
      m_GirderSpacingType = pgsTypes::sbsGeneralAdjacent;
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent ) // general adjacent to uniform adjacent
      m_GirderSpacingType = pgsTypes::sbsUniformAdjacent;
   else
      ATLASSERT(false); // is there a new spacing type???
}

LRESULT CPierGirderSpacingPage::OnChangeSameGirderSpacing(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // changing from uniform to general, or general to uniform spacing
   pgsTypes::SupportedBeamSpacing oldGirderSpacingType = m_GirderSpacingType;

   ToggleGirderSpacingType();

   long backGirderSpacingDatum, aheadGirderSpacingDatum;


   pgsTypes::PierFaceType pierFace = (pgsTypes::PierFaceType)(wParam);
   if ( m_GirderSpacingType == pgsTypes::sbsUniform || m_GirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      // we are going from general to uniform spacing
      // if the grid has more than one spacing, we need to ask the user which one is to be
      // used for the entire bridge

      // determine if there is more than one spacing group
      CGirderSpacingGridData spacingData = m_GirderSpacingGrid[pierFace].GetGirderSpacingData(); 
      GroupIndexType nSpacingGroups = spacingData.m_GirderSpacing.GetSpacingGroupCount();
      double bridgeSpacing = 0;
      if ( 1 < nSpacingGroups )
      {
         // there is more than one group... get all the unique spacing values
         std::set<double> spacings;
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            double space;
            spacingData.m_GirderSpacing.GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
            spacings.insert( space );
         }


         if ( 1 < spacings.size() )
         {
            // there is more than one unique girder spacing... which one do we want to use
            // for the entire bridge???
            CComPtr<IBroker> broker;
            EAFGetBroker(&broker);
            GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);

            CSelectItemDlg dlg;
            dlg.m_strLabel = "Select the spacing to be used for the entire bridge";
            dlg.m_strTitle = "Select spacing";
            dlg.m_ItemIdx = 0;

            CString strItems;
            std::set<double>::iterator iter;
            for ( iter = spacings.begin(); iter != spacings.end(); iter++ )
            {
               double spacing = *iter;

               CString strItem;
               if ( IsGirderSpacing(oldGirderSpacingType) )
                  strItem.Format(_T("%s"),FormatDimension(spacing,pDisplayUnits->GetXSectionDimUnit(),true));
               else
                  strItem.Format(_T("%s"),FormatDimension(spacing,pDisplayUnits->GetComponentDimUnit(),true));

               if ( iter != spacings.begin() )
                  strItems += _T("\n");

               strItems += strItem;
            }

            dlg.m_strItems = strItems;
            if ( dlg.DoModal() == IDOK )
            {
               iter = spacings.begin();
               for ( IndexType i = 0; i < dlg.m_ItemIdx; i++ )
                  iter++;

               bridgeSpacing = *iter;
            }
            else
            {
               return 0;
            }
         }
         else
         {
            // there is only one unique spacing value.. get it
            bridgeSpacing = spacingData.m_GirderSpacing.GetGirderSpacing(0);
         }
      }
      else
      {
         // there is only one unique spacing value.. get it
         bridgeSpacing = spacingData.m_GirderSpacing.GetGirderSpacing(0);
      }

      // join all the girder spacings into one
      spacingData.m_GirderSpacing.JoinAll(0);

      // set the spacing value
      spacingData.m_GirderSpacing.SetGirderSpacing(0,bridgeSpacing);

      // cache the current data and apply the new spacing to the grids
      if ( m_pPrevSpan )
      {
         m_GirderSpacingCache[pgsTypes::Back] = m_GirderSpacingGrid[pgsTypes::Back].GetGirderSpacingData();
         m_GirderSpacingGrid[pgsTypes::Back].SetGirderSpacingData(spacingData);
         m_GirderSpacingGrid[pgsTypes::Back].FillGrid();

         CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
         m_GirderSpacingMeasureCache[pgsTypes::Back] = (long)pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );
      }

      if ( m_pNextSpan )
      {
         m_GirderSpacingCache[pgsTypes::Ahead] = m_GirderSpacingGrid[pgsTypes::Ahead].GetGirderSpacingData();
         m_GirderSpacingGrid[pgsTypes::Ahead].SetGirderSpacingData(spacingData);
         m_GirderSpacingGrid[pgsTypes::Ahead].FillGrid();

         CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
         m_GirderSpacingMeasureCache[pgsTypes::Ahead] = (long)pcbStartOfSpanSpacingDatum->GetItemData( pcbStartOfSpanSpacingDatum->GetCurSel() );
      }

      backGirderSpacingDatum  = m_GirderSpacingMeasureCache[pierFace];
      aheadGirderSpacingDatum = m_GirderSpacingMeasureCache[pierFace];
   }
   else
   {
      // restore the girder spacing from the cached values
      if ( m_pPrevSpan )
      {
         m_GirderSpacingGrid[pgsTypes::Back].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::Back]);
         m_GirderSpacingGrid[pgsTypes::Back].FillGrid();

         backGirderSpacingDatum = m_GirderSpacingMeasureCache[pgsTypes::Back];
      }

      if ( m_pNextSpan )
      {
         m_GirderSpacingGrid[pgsTypes::Ahead].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::Ahead]);
         m_GirderSpacingGrid[pgsTypes::Ahead].FillGrid();

         aheadGirderSpacingDatum = m_GirderSpacingMeasureCache[pgsTypes::Ahead];
      }
   }

   if ( m_pPrevSpan )
   {
      m_GirderSpacingGrid[pgsTypes::Back].SetGirderSpacingType(m_GirderSpacingType);

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_SPAN_SPACING_MEASUREMENT, backGirderSpacingDatum);
   }

   if ( m_pNextSpan )
   {
      m_GirderSpacingGrid[pgsTypes::Ahead].SetGirderSpacingType(m_GirderSpacingType);

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_NEXT_SPAN_SPACING_MEASUREMENT, aheadGirderSpacingDatum);
   }

   UpdateChildWindowState();
   return 0;
}

LRESULT CPierGirderSpacingPage::OnChangeSlabOffset(WPARAM wParam,LPARAM lParam)
{
   pgsTypes::PierFaceType pierFace = (pgsTypes::PierFaceType)(wParam);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CWnd* pWnd = GetDlgItem(pierFace == pgsTypes::Back ? IDC_BACK_SLAB_OFFSET : IDC_AHEAD_SLAB_OFFSET);

   if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
   {
      // going to a single slab offset for the entire bridge

      // get current value from UI
      CString strWndTxt;
      pWnd->GetWindowText(strWndTxt);

      CString strBackTxt, strAheadTxt;
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::Back]);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::Ahead]);

      GetDlgItem(IDC_BACK_SLAB_OFFSET)->SetWindowText(strWndTxt);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->SetWindowText(strWndTxt);
   }
   else if ( m_SlabOffsetTypeCache == pgsTypes::sotGirder )
   {
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->SetWindowText(_T(""));
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->SetWindowText(_T(""));
   }
   else
   {
      // going to span by span slab offset
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->SetWindowText( m_strSlabOffsetCache[pgsTypes::Back] );
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->SetWindowText( m_strSlabOffsetCache[pgsTypes::Ahead] );
   }

   // swap slab offset type
   pgsTypes::SlabOffsetType temp = m_SlabOffsetType;
   m_SlabOffsetType = m_SlabOffsetTypeCache;
   m_SlabOffsetTypeCache = temp;


   UpdateChildWindowState();
   return 0;
}

void CPierGirderSpacingPage::OnAheadPierSpacingDatumChanged()
{
   OnPierSpacingDatumChanged(IDC_NEXT_SPAN_SPACING_MEASUREMENT,pgsTypes::Ahead);
}

void CPierGirderSpacingPage::OnBackPierSpacingDatumChanged()
{
   OnPierSpacingDatumChanged(IDC_PREV_SPAN_SPACING_MEASUREMENT,pgsTypes::Back);
}

void CPierGirderSpacingPage::OnPierSpacingDatumChanged(UINT nIDC,pgsTypes::PierFaceType pierFace)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(nIDC);

   int cursel = pCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing((DWORD)pCB->GetItemData(cursel),&ml,&mt);

   m_GirderSpacingGrid[pierFace].SetMeasurementLocation(ml);
   m_GirderSpacingGrid[pierFace].SetMeasurementType(mt);
   m_GirderSpacingGrid[pierFace].FillGrid();
}

void CPierGirderSpacingPage::UpdateNumGirdersHyperLinkText()
{
   if ( m_bUseSameNumGirders )
   {
      m_NumGirdersHyperLink[pgsTypes::Back].SetWindowText(_T("The same number of girders are used in all spans"));
      m_NumGirdersHyperLink[pgsTypes::Back].SetURL(_T("Click to define the number of girders span by span")); // tooltip text
      m_NumGirdersHyperLink[pgsTypes::Ahead].SetWindowText(_T("The same number of girders are used in all spans"));
      m_NumGirdersHyperLink[pgsTypes::Ahead].SetURL(_T("Click to define the number of girders span by span") ); // tooltip text
   }
   else
   {
      m_NumGirdersHyperLink[pgsTypes::Back].SetWindowText(_T("The number of girders is defined span by span"));
      m_NumGirdersHyperLink[pgsTypes::Back].SetURL(_T("Click to use this number of girders in all spans")); // tooltip text
      m_NumGirdersHyperLink[pgsTypes::Ahead].SetWindowText(_T("The number of girders is defined span by span"));
      m_NumGirdersHyperLink[pgsTypes::Ahead].SetURL(_T("Click to use this number of girders in all spans") ); // tooltip text
   }
}

void CPierGirderSpacingPage::UpdateGirderSpacingHyperLinkText()
{
   CString strBackSpacingNote(_T(""));
   CString strAheadSpacingNote(_T(""));
   CString strBackSpanConstantSpacingNote(_T(""));
   CString strAheadSpanConstantSpacingNote(_T(""));

   bool bInputSpacing[2];
   bInputSpacing[pgsTypes::Ahead] = m_GirderSpacingGrid[pgsTypes::Ahead].InputSpacing();
   bInputSpacing[pgsTypes::Back]  = m_GirderSpacingGrid[pgsTypes::Back].InputSpacing();

   if ( m_pPrevSpan == NULL )
      bInputSpacing[pgsTypes::Back] = true;

   if ( m_pNextSpan == NULL )
      bInputSpacing[pgsTypes::Ahead] = true;


   CString strGirderSpacingURL;

   BOOL bEnable = TRUE;
   if ( m_GirderSpacingType == pgsTypes::sbsUniform )
   {
      strBackSpacingNote = _T("The same girder spacing is used for the entire bridge");
      strAheadSpacingNote = strBackSpacingNote;
      bEnable = (bInputSpacing[pgsTypes::Back] && bInputSpacing[pgsTypes::Ahead] ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to define girder spacing span by span");
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      strBackSpacingNote = _T("The same joint spacing is used for the entire bridge");
      strAheadSpacingNote = strBackSpacingNote;
      bEnable = (bInputSpacing[pgsTypes::Back] && bInputSpacing[pgsTypes::Ahead] ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to define joint spacing span by span");
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
   {
      strBackSpacingNote.Format(_T("The same girder spacing must be used for the entire bridge for %s girders"),
                            m_pPier->GetBridgeDescription()->GetGirderFamilyName());

      strAheadSpacingNote = strBackSpacingNote;
      bEnable = FALSE;

      strGirderSpacingURL = _T("Click to define girder spacing span by span");
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneral )
   {
      strBackSpacingNote  = _T("Girder spacing is defined span by span");
      strAheadSpacingNote = _T("Girder spacing is defined span by span");

      bEnable = (bInputSpacing[pgsTypes::Back] && bInputSpacing[pgsTypes::Ahead] ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to make girder spacing the same for all spans");
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent )
   {
      strBackSpacingNote  = _T("Joint spacing is defined span by span");
      strAheadSpacingNote = _T("Joint spacing is defined span by span");

      bEnable = (bInputSpacing[pgsTypes::Back] && bInputSpacing[pgsTypes::Ahead] ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to make joint spacing the same for all spans");
   }
   else
   {
      ATLASSERT(false); // is there a new spacing type???
   }
   
   if ( m_pPrevSpan && m_pPrevSpan->UseSameSpacingAtBothEndsOfSpan() )
   {
      if ( ::IsSpreadSpacing(m_pPrevSpan->GetBridgeDescription()->GetGirderSpacingType()) )
         strBackSpanConstantSpacingNote.Format(_T("The same girder spacing is used at both ends of Span %d."),LABEL_SPAN(m_pPrevSpan->GetSpanIndex()));
      else
         strBackSpanConstantSpacingNote.Format(_T("The same joint spacing is used at both ends of Span %d."),LABEL_SPAN(m_pPrevSpan->GetSpanIndex()));
   }

   if ( m_pNextSpan && m_pNextSpan->UseSameSpacingAtBothEndsOfSpan() )
   {
      if ( ::IsSpreadSpacing(m_pNextSpan->GetBridgeDescription()->GetGirderSpacingType()) )
         strBackSpanConstantSpacingNote.Format(_T("The same girder spacing is used at both ends of Span %d."),LABEL_SPAN(m_pNextSpan->GetSpanIndex()));
      else
         strAheadSpanConstantSpacingNote.Format(_T("The same joint spacing is used at both ends of Span %d."),LABEL_SPAN(m_pNextSpan->GetSpanIndex()));
   }

   GetDlgItem(IDC_PREV_SPAN_CONSTANT_SPACING_NOTE)->SetWindowText(strBackSpanConstantSpacingNote);
   GetDlgItem(IDC_NEXT_SPAN_CONSTANT_SPACING_NOTE)->SetWindowText(strAheadSpanConstantSpacingNote);

   m_GirderSpacingHyperLink[pgsTypes::Back].SetWindowText(strBackSpacingNote);
   m_GirderSpacingHyperLink[pgsTypes::Back].SetURL(strGirderSpacingURL);
   m_GirderSpacingHyperLink[pgsTypes::Back].EnableWindow(bEnable);

   m_GirderSpacingHyperLink[pgsTypes::Ahead].SetWindowText(strAheadSpacingNote);
   m_GirderSpacingHyperLink[pgsTypes::Ahead].SetURL(strGirderSpacingURL);
   m_GirderSpacingHyperLink[pgsTypes::Ahead].EnableWindow(bEnable);
}

void CPierGirderSpacingPage::UpdateSlabOffsetHyperLinkText()
{
   CString strSlabOffsetNote;
   CString strSlabOffsetURL;
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      // slab offset is by girder
      strSlabOffsetNote = _T("Slab Offsets are defined girder by girder");
      strSlabOffsetURL = _T("Click to use this Slab Offset for this span");
   }
   else if ( m_SlabOffsetType == pgsTypes::sotBridge )
   {
      strSlabOffsetNote = _T("A single Slab Offset is used for the entire bridge");
      strSlabOffsetURL = _T("Click to use this Slab Offset for this span");
   }
   else
   {
      strSlabOffsetNote = _T("A unique Slab Offset is used in each span");
      if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
         strSlabOffsetURL = _T("Click to use this Slab Offset for the entire bridge");
      else
         strSlabOffsetURL = _T("Click to use this Slab Offset for the girders in this span");
   }

   m_SlabOffsetHyperLink[pgsTypes::Ahead].SetWindowText(strSlabOffsetNote);
   m_SlabOffsetHyperLink[pgsTypes::Ahead].SetURL(strSlabOffsetURL);

   m_SlabOffsetHyperLink[pgsTypes::Back].SetWindowText(strSlabOffsetNote);
   m_SlabOffsetHyperLink[pgsTypes::Back].SetURL(strSlabOffsetURL);

   // enable/disable and cache slab offsets as needed
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   m_SlabOffsetHyperLink[pgsTypes::Back].EnableWindow(  pParent->m_pBridge->GetDeckDescription()->DeckType == pgsTypes::sdtNone ? FALSE : TRUE );
   m_SlabOffsetHyperLink[pgsTypes::Ahead].EnableWindow( pParent->m_pBridge->GetDeckDescription()->DeckType == pgsTypes::sdtNone ? FALSE : TRUE );
}

void CPierGirderSpacingPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_GIRDERSPACING );
}

bool CPierGirderSpacingPage::AllowConnectionChange(pgsTypes::PierFaceType side, const CString& connectionName)
{
   // See if we need to change our current spacing for this connection
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const ConnectionLibraryEntry* pConEntry = pLib->GetConnectionEntry(connectionName);

   ConnectionLibraryEntry::BearingOffsetMeasurementType measure_type = pConEntry->GetBearingOffsetMeasurementType();
   if (measure_type!=ConnectionLibraryEntry::AlongGirder)
   {
      // Bearing location not measured along girder, we can accept any spacing type
      return true;
   }

   // Bearing location is measured is along girder, see if we need to change spacing type for this
   if (IsBridgeSpacing(m_GirderSpacingType))
   {
      // same spacing for entire bridge - we cannot change from this dialog
      pgsTypes::MeasurementLocation ml;
      pgsTypes::MeasurementType mt;
      UnhashGirderSpacing(m_GirderSpacingMeasure[side],&ml,&mt);
      if (ml == pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG2);
         int result = ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
         if ( result == IDYES )
         {
            m_GirderSpacingMeasurementLocation = pgsTypes::AtCenterlinePier;

            // girder spacing is still measured at the bridge level. need to update
            // the span by span girder measure to reflect the current bridge level measurement location
            if ( m_pPrevSpan )
            {
               pgsTypes::MeasurementLocation ml;
               pgsTypes::MeasurementType     mt;
               UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Back],&ml,&mt);
               m_GirderSpacingMeasure[pgsTypes::Back] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
               m_GirderSpacingGrid[pgsTypes::Back].SetMeasurementLocation(m_GirderSpacingMeasurementLocation); // also update grid data
            }

            if ( m_pNextSpan )
            {
               pgsTypes::MeasurementLocation ml;
               pgsTypes::MeasurementType     mt;
               UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Ahead],&ml,&mt);
               m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
               m_GirderSpacingGrid[pgsTypes::Ahead].SetMeasurementLocation(m_GirderSpacingMeasurementLocation); // also update grid data
            }

            return true;
         }
         else if ( result == IDNO )
         {
            ToggleGirderSpacingType(); // change girder spacing type to span-by-span
            
            pgsTypes::MeasurementLocation ml;
            pgsTypes::MeasurementType     mt;
            UnhashGirderSpacing(m_GirderSpacingMeasure[side],&ml,&mt);
            m_GirderSpacingMeasure[side] = HashGirderSpacing(pgsTypes::AtCenterlinePier,mt);
            m_GirderSpacingGrid[side].SetMeasurementLocation(pgsTypes::AtCenterlinePier); // also update grid data

            m_GirderSpacingGrid[pgsTypes::Ahead].SetGirderSpacingType(m_GirderSpacingType);
            m_GirderSpacingGrid[pgsTypes::Back].SetGirderSpacingType(m_GirderSpacingType);

            ATLASSERT(IsSpanSpacing(m_GirderSpacingType));

            return true;
         }
         else
         {
            return false; // don't allow connection to change
         }
      }
      else
      {
         return true; // no problem
      }
   }
   else
   {
      // spacing is unique at each end
      pgsTypes::MeasurementLocation ml;
      pgsTypes::MeasurementType mt;
      UnhashGirderSpacing(m_GirderSpacingMeasure[side],&ml,&mt);
      if (ml==pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG);
         int st = ::AfxMessageBox(msg, MB_ICONQUESTION|MB_YESNO);
         if (st==IDYES)
         {
            m_GirderSpacingMeasure[side] = HashGirderSpacing(pgsTypes::AtCenterlinePier,mt);

            m_GirderSpacingGrid[side].SetMeasurementLocation(pgsTypes::AtCenterlinePier); // also update grid data
            m_GirderSpacingMeasureCache[side]  = m_GirderSpacingMeasure[side];
            return true;
         }
         else
         {
            return false;
         }
      }
      else
      {
         return true;
      }
   }
}
