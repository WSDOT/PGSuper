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

// SpanGirderLayoutPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "GirderLayoutPage.h"
#include "SpanDetailsDlg.h"
#include "SelectItemDlg.h"
#include "ResolveGirderSpacingDlg.h"
#include "PGSuperColors.h"
#include "HtmlHelp\HelpTopics.hh"

#include <PGSuperUnits.h>

#include <MfcTools\CustomDDX.h>
#include <PgsExt\BridgeDescription.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

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
   if (pConEntry)
   {
      *pStartMeasure = pConEntry->GetBearingOffsetMeasurementType();
   }
   else
   {
      *pStartMeasure = ConnectionLibraryEntry::NormalToPier;
   }

   pConEntry = pLib->GetConnectionEntry(endConnectionName);
   if (pConEntry)
   {
      *pEndMeasure = pConEntry->GetBearingOffsetMeasurementType();
   }
   else
   {
      *pEndMeasure = ConnectionLibraryEntry::NormalToPier;
   }
}


/////////////////////////////////////////////////////////////////////////////
// CSpanGirderLayoutPage property page

IMPLEMENT_DYNCREATE(CSpanGirderLayoutPage, CPropertyPage)

CSpanGirderLayoutPage::CSpanGirderLayoutPage() : CPropertyPage(CSpanGirderLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CSpanGirderLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_MinGirderCount = 2;
}

CSpanGirderLayoutPage::~CSpanGirderLayoutPage()
{
}

void CSpanGirderLayoutPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanGirderLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_NUMGDR_SPIN, m_NumGdrSpinner);
	//}}AFX_DATA_MAP

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if (!pDX->m_bSaveAndValidate)
   {
      ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
      GetBearingMeasurementTypes(pParent->m_PrevPierConnectionName[pgsTypes::Ahead], pParent->m_NextPierConnectionName[pgsTypes::Back], &start_measure, &end_measure);

      if (m_bUseSameSpacingAtBothEnds)
      {
         if (start_measure==ConnectionLibraryEntry::AlongGirder || end_measure==ConnectionLibraryEntry::AlongGirder)
         {
            start_measure = ConnectionLibraryEntry::AlongGirder;
            end_measure   = ConnectionLibraryEntry::AlongGirder;
         }
      }

      FillGirderSpacingMeasurementComboBox(IDC_PREV_PIER_GIRDER_SPACING_MEASURE,start_measure);
      FillGirderSpacingMeasurementComboBox(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE,end_measure);
   }

   DDX_Control(pDX, IDC_NUMGDR_NOTE,         m_NumGirdersHyperLink);
   DDX_Control(pDX, IDC_GIRDER_SPACING_NOTE, m_GirderSpacingHyperLink);
   DDX_Control(pDX, IDC_GIRDERGRID_NOTE,     m_GirderTypeHyperLink);
   DDX_Control(pDX, IDC_SLAB_OFFSET_NOTE,    m_SlabOffsetHyperLink);

   DDX_Text( pDX, IDC_NUMGDR, m_nGirders );
   DDV_MinMaxLong(pDX, (long)m_nGirders, (long)m_MinGirderCount, MAX_GIRDERS_PER_SPAN );

   DDX_Check_Bool(pDX, IDC_SAME_GIRDER_SPACING, m_bUseSameSpacingAtBothEnds );

   DDX_CBItemData(pDX, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::Ahead]);
   DDX_CBItemData(pDX, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::Back]);

   DDV_SpacingGrid(pDX,IDC_PREV_PIER_SPACING_GRID,&m_SpacingGrid[pgsTypes::Ahead]);
   DDV_SpacingGrid(pDX,IDC_NEXT_PIER_SPACING_GRID,&m_SpacingGrid[pgsTypes::Back]);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER, m_RefGirderIdx[pgsTypes::Ahead]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER, m_RefGirderIdx[pgsTypes::Back]);

   DDX_OffsetAndTag(pDX, IDC_PREV_REF_GIRDER_OFFSET,IDC_PREV_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::Ahead], pDisplayUnits->GetXSectionDimUnit());
   DDX_OffsetAndTag(pDX, IDC_NEXT_REF_GIRDER_OFFSET,IDC_NEXT_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::Back], pDisplayUnits->GetXSectionDimUnit());

   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::Ahead]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::Back]);

   DDX_Tag(pDX, IDC_START_SLAB_OFFSET_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_END_SLAB_OFFSET_UNIT,   pDisplayUnits->GetComponentDimUnit() );
   if ( pParent->m_pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   {
      if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && m_SlabOffsetType == pgsTypes::sotSpan) )
      {
         DDX_UnitValueAndTag(pDX, IDC_START_SLAB_OFFSET, IDC_START_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit() );
         DDX_UnitValueAndTag(pDX, IDC_END_SLAB_OFFSET,   IDC_END_SLAB_OFFSET_UNIT,   m_SlabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit() );

         if ( pDX->m_bSaveAndValidate )
         {
            GET_IFACE2(pBroker,IBridge,pBridge);
            Float64 Lg = pBridge->GetGirderLength(pParent->m_pSpanData->GetSpanIndex(),0);
            pgsPointOfInterest poiStart(pParent->m_pSpanData->GetSpanIndex(),0,0.0);
            pgsPointOfInterest poiEnd(pParent->m_pSpanData->GetSpanIndex(),0,Lg);
            Float64 grossDeckThicknessStart = pBridge->GetGrossSlabDepth(poiStart) + pBridge->GetFillet();
            Float64 grossDeckThicknessEnd   = pBridge->GetGrossSlabDepth(poiEnd) + pBridge->GetFillet();
            
            m_SlabOffset[pgsTypes::metStart] = (IsEqual(m_SlabOffset[pgsTypes::metStart],grossDeckThicknessStart) ? grossDeckThicknessStart : m_SlabOffset[pgsTypes::metStart]);
            m_SlabOffset[pgsTypes::metEnd]   = (IsEqual(m_SlabOffset[pgsTypes::metEnd],  grossDeckThicknessEnd  ) ? grossDeckThicknessEnd   : m_SlabOffset[pgsTypes::metEnd]);

            if ( m_SlabOffset[pgsTypes::metStart] < grossDeckThicknessStart )
            {
               pDX->PrepareEditCtrl(IDC_START_SLAB_OFFSET);
               CString msg;
               msg.Format(_T("The slab offset at the start of the girder must be at equal to the slab + fillet depth of %s"),FormatDimension(grossDeckThicknessStart,pDisplayUnits->GetComponentDimUnit()));
               AfxMessageBox(msg,MB_ICONEXCLAMATION);
               pDX->Fail();
            }

            if ( m_SlabOffset[pgsTypes::metEnd] < grossDeckThicknessEnd )
            {
               pDX->PrepareEditCtrl(IDC_END_SLAB_OFFSET);
               CString msg;
               msg.Format(_T("The slab offset at the end of the girder must be at equal to the slab + fillet depth of %s"),FormatDimension(grossDeckThicknessEnd,pDisplayUnits->GetComponentDimUnit()));
               AfxMessageBox(msg,MB_ICONEXCLAMATION);
               pDX->Fail();
            }
         }

      }
   }
}


BEGIN_MESSAGE_MAP(CSpanGirderLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanGirderLayoutPage)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUMGDR_SPIN, OnNumGirdersChanged)
	ON_BN_CLICKED(IDC_SAME_GIRDER_SPACING, OnSameGirderSpacing)
	ON_CBN_SELCHANGE(IDC_PREV_PIER_GIRDER_SPACING_MEASURE, OnPrevPierGirderSpacingMeasureChanged)
	ON_CBN_SELCHANGE(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, OnNextPierGirderSpacingMeasureChanged)
	ON_WM_CTLCOLOR()
   ON_REGISTERED_MESSAGE(MsgChangeSameNumberOfGirders,OnChangeSameNumberOfGirders)
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderSpacing,OnChangeSameGirderSpacing)
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderType,OnChangeSameGirderType)
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffset,OnChangeSlabOffset)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanGirderLayoutPage message handlers
void CSpanGirderLayoutPage::Init(const CSpanData* pSpan)
{
   m_GirderSpacingType  = pSpan->GetBridgeDescription()->GetGirderSpacingType();
   m_GirderSpacingMeasurementLocation = pSpan->GetBridgeDescription()->GetMeasurementLocation();
   m_bUseSameNumGirders = pSpan->GetBridgeDescription()->UseSameNumberOfGirdersInAllSpans();
   m_bUseSameGirderType = pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge();

   m_GirderNameGrid.m_GirderTypes = *pSpan->GetGirderTypes();

   pgsTypes::MeasurementLocation ml = pSpan->GetGirderSpacing(pgsTypes::metStart)->GetMeasurementLocation();
   pgsTypes::MeasurementType     mt = pSpan->GetGirderSpacing(pgsTypes::metStart)->GetMeasurementType();
   m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(ml,mt);

   ml = pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetMeasurementLocation();
   mt = pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetMeasurementType();
   m_GirderSpacingMeasure[pgsTypes::Back] = HashGirderSpacing(ml,mt);
   
   m_CacheGirderSpacingMeasure[pgsTypes::Back]  = m_GirderSpacingMeasure[pgsTypes::Back];
   m_CacheGirderSpacingMeasure[pgsTypes::Ahead] = m_GirderSpacingMeasure[pgsTypes::Ahead];

   m_RefGirderIdx[pgsTypes::Ahead]        = pSpan->GetGirderSpacing(pgsTypes::metStart)->GetRefGirder();
   m_RefGirderOffset[pgsTypes::Ahead]     = pSpan->GetGirderSpacing(pgsTypes::metStart)->GetRefGirderOffset();
   m_RefGirderOffsetType[pgsTypes::Ahead] = pSpan->GetGirderSpacing(pgsTypes::metStart)->GetRefGirderOffsetType();

   m_CacheRefGirderIdx[pgsTypes::Ahead]        = m_RefGirderIdx[pgsTypes::Ahead];
   m_CacheRefGirderOffset[pgsTypes::Ahead]     = m_RefGirderOffset[pgsTypes::Ahead];
   m_CacheRefGirderOffsetType[pgsTypes::Ahead] = m_RefGirderOffsetType[pgsTypes::Ahead];

   m_RefGirderIdx[pgsTypes::Back]        = pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetRefGirder();
   m_RefGirderOffset[pgsTypes::Back]     = pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetRefGirderOffset();
   m_RefGirderOffsetType[pgsTypes::Back] = pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetRefGirderOffsetType();

   m_CacheRefGirderIdx[pgsTypes::Back]        = m_RefGirderIdx[pgsTypes::Back];
   m_CacheRefGirderOffset[pgsTypes::Back]     = m_RefGirderOffset[pgsTypes::Back];
   m_CacheRefGirderOffsetType[pgsTypes::Back] = m_RefGirderOffsetType[pgsTypes::Back];

   m_nGirders = pSpan->GetGirderCount();

   m_bUseSameSpacingAtBothEnds = pSpan->UseSameSpacingAtBothEndsOfSpan();

   double skew_angle = 0; // dummy value

   // back of pier (end of span)
   CGirderSpacingGridData gridData;
   gridData.m_GirderSpacing = *pSpan->GetGirderSpacing(pgsTypes::metEnd);
   gridData.m_GirderTypes   = *pSpan->GetGirderTypes();
   gridData.m_PierFace      = pgsTypes::Back;
   m_GirderSpacingCache[pgsTypes::Back] = gridData;

   m_SpacingGrid[pgsTypes::Back].Init( m_GirderSpacingType,
                                       m_bUseSameNumGirders,
                                       pSpan->GetGirderSpacing(pgsTypes::metEnd),
                                       pSpan->GetGirderTypes(),
                                       pgsTypes::Back,
                                       pSpan->GetNextPier()->GetPierIndex(),
                                       skew_angle,
                                       pSpan->GetNextPier()->GetNextSpan() == NULL ? true : false);


   // ahead of pier (start of span)
   gridData.m_GirderSpacing = *pSpan->GetGirderSpacing(pgsTypes::metStart);
   gridData.m_GirderTypes   = *pSpan->GetGirderTypes();
   gridData.m_PierFace      = pgsTypes::Ahead;
   m_GirderSpacingCache[pgsTypes::Ahead] = gridData;

   m_SpacingGrid[pgsTypes::Ahead].Init( m_GirderSpacingType,
                                        m_bUseSameNumGirders,
                                        pSpan->GetGirderSpacing(pgsTypes::metStart),
                                        pSpan->GetGirderTypes(),
                                        pgsTypes::Ahead,
                                        pSpan->GetPrevPier()->GetPierIndex(),
                                        skew_angle,
                                        pSpan->GetPrevPier()->GetPrevSpan() == NULL ? true : false);

   // slab offset
   m_SlabOffsetType = pSpan->GetBridgeDescription()->GetSlabOffsetType();
   if ( m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotGirder )
      m_SlabOffsetTypeCache = pgsTypes::sotSpan;
   else
      m_SlabOffsetTypeCache = pgsTypes::sotBridge;

   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      // slab offset is unique for each girder... which one do we use??? For now, use girder 0
      m_SlabOffset[pgsTypes::metStart] = pSpan->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metStart);
      m_SlabOffset[pgsTypes::metEnd]   = pSpan->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metEnd);
   }
   else
   {
      m_SlabOffset[pgsTypes::metStart] = pSpan->GetSlabOffset(pgsTypes::metStart);
      m_SlabOffset[pgsTypes::metEnd]   = pSpan->GetSlabOffset(pgsTypes::metEnd);
   }
}

void CSpanGirderLayoutPage::GetPierSkewAngles(double& skew1,double& skew2)
{
   CSpanDetailsDlg* pParent   = (CSpanDetailsDlg*)GetParent();
   const CPierData* pPrevPier = pParent->m_pSpanData->GetPrevPier();
   const CPierData* pNextPier = pParent->m_pSpanData->GetNextPier();

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   double skew_angle_1;
   pBridge->GetSkewAngle(pPrevPier->GetStation(),pPrevPier->GetOrientation(),&skew_angle_1);

   double skew_angle_2;
   pBridge->GetSkewAngle(pNextPier->GetStation(),pNextPier->GetOrientation(),&skew_angle_2);

   if (m_bUseSameSpacingAtBothEnds)
   {
      double max_skew = _cpp_max(skew_angle_1,skew_angle_2);
      skew_angle_1    = max_skew;
      skew_angle_2    = max_skew;
   }

   skew1 = skew_angle_1;
   skew2 = skew_angle_2;
}

BOOL CSpanGirderLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   SpanIndexType spanIdx = pParent->m_pSpanData->GetSpanIndex();

   double skew_angle_1, skew_angle_2;
   GetPierSkewAngles(skew_angle_1,skew_angle_2);

   // set up the girder name grid
	m_GirderNameGrid.SubclassDlgItem(IDC_GIRDERGRID, this);
   m_GirderNameGrid.CustomInit(pParent->m_pSpanData);
   m_GirderTypesCache = m_GirderNameGrid.m_GirderTypes;


   // set up the previous pier girder spacing input
   m_SpacingGrid[pgsTypes::Ahead].SubclassDlgItem(IDC_PREV_PIER_SPACING_GRID,this);
   m_SpacingGrid[pgsTypes::Ahead].CustomInit();
   m_SpacingGrid[pgsTypes::Ahead].SetPierSkewAngle(skew_angle_1);


   // set up the next pier girder spacing input
   m_SpacingGrid[pgsTypes::Back].SubclassDlgItem(IDC_NEXT_PIER_SPACING_GRID,this);
   m_SpacingGrid[pgsTypes::Back].CustomInit();
   m_SpacingGrid[pgsTypes::Back].SetPierSkewAngle(skew_angle_2);

   FillRefGirderComboBox(pgsTypes::Back);
   FillRefGirderComboBox(pgsTypes::Ahead);
   FillRefGirderOffsetTypeComboBox(pgsTypes::Back);
   FillRefGirderOffsetTypeComboBox(pgsTypes::Ahead);

   CPropertyPage::OnInitDialog();

   // Initialize the spinner control for # of girder lines
   m_MinGirderCount = GetMinGirderCount();
   m_NumGdrSpinner.SetRange(short(m_MinGirderCount),MAX_GIRDERS_PER_SPAN);

   // restrict the spinner to only go one girder at a time.
   UDACCEL accel;
   accel.nInc = 1;
   accel.nSec = 0;
   m_NumGdrSpinner.SetAccel(0,&accel);

   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
      GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
      GetDlgItem(IDC_SAME_GIRDER_SPACING)->SetWindowText(_T("Use same girder spacing at both ends of span"));
   }
   else
   {
      GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
      GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
      GetDlgItem(IDC_SAME_GIRDER_SPACING)->SetWindowText(_T("Use same joint spacing at both ends of span"));
   }

   if ( m_bUseSameSpacingAtBothEnds )
   {
      m_SpacingGrid[pgsTypes::Ahead].SetLinkedGrid(&m_SpacingGrid[pgsTypes::Back]);
   }

  
   UpdateChildWindowState();

   //CComPtr<IBroker> pBroker;
   //EAFGetBroker(&pBroker);
   //GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   //if ( pBridgeDesc->IsPostTensioningModeled() )
   //{
   //   // disable elements of the UI that can't be changed when the bridge model has PT
   //   GetDlgItem(IDC_NUMGDR_NOTE)->EnableWindow(FALSE);
   //}

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanGirderLayoutPage::OnNumGirdersChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   int nGirders = pNMUpDown->iPos + pNMUpDown->iDelta;
   if ( nGirders < int(m_MinGirderCount) )
   {
      // rolling past the bottom... going back up to MAX_GIRDERS_PER_SPAN
      AddGirders( GirderIndexType(MAX_GIRDERS_PER_SPAN - pNMUpDown->iPos) );
   }
   else if ( MAX_GIRDERS_PER_SPAN < nGirders )
   {
      RemoveGirders( GirderIndexType(pNMUpDown->iPos - m_MinGirderCount) );
   }
   else
   {
	   if ( pNMUpDown->iDelta < 0 )
         RemoveGirders(GirderIndexType(-pNMUpDown->iDelta));
      else
         AddGirders(GirderIndexType(pNMUpDown->iDelta));
   }

   *pResult = 0;
}

void CSpanGirderLayoutPage::AddGirders(GirderIndexType nGirders)
{
   m_nGirders += nGirders;

   m_GirderNameGrid.AddGirders(nGirders);
   m_SpacingGrid[pgsTypes::Ahead].AddGirders(nGirders);
   m_SpacingGrid[pgsTypes::Back].AddGirders(nGirders);
   UpdateGirderSpacingState();
}

void CSpanGirderLayoutPage::RemoveGirders(GirderIndexType nGirders)
{
   m_nGirders -= nGirders;

   m_GirderNameGrid.RemoveGirders(nGirders);
   m_SpacingGrid[pgsTypes::Ahead].RemoveGirders(nGirders);
   m_SpacingGrid[pgsTypes::Back].RemoveGirders(nGirders);

   UpdateGirderSpacingState();
}

void CSpanGirderLayoutPage::FillGirderSpacingMeasurementComboBox(int nIDC, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure)
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

void CSpanGirderLayoutPage::FillRefGirderOffsetTypeComboBox(pgsTypes::PierFaceType pierFace)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(pierFace == pgsTypes::Back ? IDC_PREV_REF_GIRDER_OFFSET_TYPE : IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   int idx = pCB->AddString(_T("Alignment"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtAlignment);

   idx = pCB->AddString(_T("Bridge Line"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtBridge);
}

void CSpanGirderLayoutPage::FillRefGirderComboBox(pgsTypes::PierFaceType pierFace)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(pierFace == pgsTypes::Back ? IDC_PREV_REF_GIRDER : IDC_NEXT_REF_GIRDER);
   int curSel = pCB->GetCurSel();
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Center of Girders"));
   pCB->SetItemData(idx,(DWORD_PTR)INVALID_INDEX);

   for ( GirderIndexType i = 0; i < m_nGirders; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD_PTR)i);
   }

   pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel);
}

void CSpanGirderLayoutPage::OnSameGirderSpacing() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if ( m_GirderSpacingType == pgsTypes::sbsGeneral || m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent )
   {
      if ( 0 < IsDlgButtonChecked(IDC_SAME_GIRDER_SPACING) )
      {
         CacheEndSpacing();
      }
      else
      {
         RestoreEndSpacing();
      }
   }
}

void CSpanGirderLayoutPage::CacheEndSpacing()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   m_bUseSameSpacingAtBothEnds = true;

   // make the end of span spacing grid display the start of span spacing
   m_GirderSpacingCache[pgsTypes::Back] = m_SpacingGrid[pgsTypes::Back].GetGirderSpacingData();

   m_SpacingGrid[pgsTypes::Ahead].SetLinkedGrid(&m_SpacingGrid[pgsTypes::Back]);

   // disable the end of span spacing grid
   m_SpacingGrid[pgsTypes::Back].Enable(FALSE);

   CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);

   // cache the current value of the end of span spacing datum
   m_CacheGirderSpacingMeasure[pgsTypes::Back]  = (long)pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );

   // cache the reference girder
   CComboBox* pcbStartRefGirder = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER);
   CComboBox* pcbEndRefGirder   = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER);
   m_CacheRefGirderIdx[pgsTypes::Back] = (long)pcbEndRefGirder->GetItemData( pcbEndRefGirder->GetCurSel() );

   // cache the reference girder offset
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CDataExchange dx(this,TRUE);
   DDX_OffsetAndTag(&dx, IDC_NEXT_REF_GIRDER_OFFSET,IDC_NEXT_REF_GIRDER_OFFSET_UNIT, m_CacheRefGirderOffset[pgsTypes::Back], pDisplayUnits->GetXSectionDimUnit());

   // cache the reference girder offset type
   CComboBox* pcbStartRefGirderOffsetType = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE);
   CComboBox* pcbEndRefGirderOffsetType   = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   m_CacheRefGirderOffsetType[pgsTypes::Back] = (pgsTypes::OffsetMeasurementType)pcbEndRefGirderOffsetType->GetItemData( pcbEndRefGirderOffsetType->GetCurSel() );

   // Tricky here - we can have different connections at each end. Must use setting for common denominator when combining
   ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
   GetBearingMeasurementTypes(pParent->m_PrevPierConnectionName[pgsTypes::Ahead], pParent->m_NextPierConnectionName[pgsTypes::Back], &start_measure, &end_measure);

   if (start_measure == ConnectionLibraryEntry::AlongGirder &&
       end_measure != ConnectionLibraryEntry::AlongGirder
       ) 
   {
      FillGirderSpacingMeasurementComboBox(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE,ConnectionLibraryEntry::AlongGirder);
   }
   else if (end_measure==ConnectionLibraryEntry::AlongGirder) 
   {
      FillGirderSpacingMeasurementComboBox(IDC_PREV_PIER_GIRDER_SPACING_MEASURE,ConnectionLibraryEntry::AlongGirder);
      pcbStartOfSpanSpacingDatum->SetCurSel( pcbEndOfSpanSpacingDatum->GetCurSel() );
      m_SpacingGrid[pgsTypes::Ahead].SetMeasurementLocation(pgsTypes::AtCenterlinePier);
   }

   // set the end of span spacing datum equal to the start of span
   pcbEndOfSpanSpacingDatum->SetCurSel( pcbStartOfSpanSpacingDatum->GetCurSel() );

   // disable the combo box
   pcbEndOfSpanSpacingDatum->EnableWindow(FALSE);

   // set the end ref girder equal to the start
   pcbEndRefGirder->SetCurSel( pcbStartRefGirder->GetCurSel() );
   pcbEndRefGirder->EnableWindow(FALSE);

   pcbEndRefGirderOffsetType->SetCurSel( pcbStartRefGirderOffsetType->GetCurSel() );
   pcbEndRefGirderOffsetType->EnableWindow(FALSE);

   CString strWndTxt;
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->GetWindowText(strWndTxt);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->SetWindowText(strWndTxt);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(FALSE);


   double skew_angle_1, skew_angle_2;
   GetPierSkewAngles(skew_angle_1,skew_angle_2);
   m_SpacingGrid[pgsTypes::Ahead].SetPierSkewAngle(skew_angle_1);
   m_SpacingGrid[pgsTypes::Back].SetPierSkewAngle(skew_angle_2);
}

void CSpanGirderLayoutPage::RestoreEndSpacing()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   m_bUseSameSpacingAtBothEnds = false;
   m_SpacingGrid[pgsTypes::Back].Enable(TRUE);
   m_SpacingGrid[pgsTypes::Ahead].SetLinkedGrid(NULL);
   m_SpacingGrid[pgsTypes::Ahead].FillGrid();

   m_SpacingGrid[pgsTypes::Back].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::Back]);
   m_SpacingGrid[pgsTypes::Back].FillGrid();


   ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
   GetBearingMeasurementTypes(pParent->m_PrevPierConnectionName[pgsTypes::Ahead], pParent->m_NextPierConnectionName[pgsTypes::Back], &start_measure, &end_measure);
   FillGirderSpacingMeasurementComboBox(IDC_PREV_PIER_GIRDER_SPACING_MEASURE,start_measure);
   FillGirderSpacingMeasurementComboBox(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE,end_measure);

   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::Ahead]);
   DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::Back]);

   GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(TRUE);

   DDX_CBItemData(&dx, IDC_NEXT_REF_GIRDER, m_CacheRefGirderIdx[pgsTypes::Back]);
   GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(TRUE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   DDX_OffsetAndTag(&dx, IDC_NEXT_REF_GIRDER_OFFSET,IDC_NEXT_REF_GIRDER_OFFSET_UNIT, m_CacheRefGirderOffset[pgsTypes::Back], pDisplayUnits->GetXSectionDimUnit());
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(TRUE);

   DDX_CBItemData(&dx, IDC_NEXT_REF_GIRDER_OFFSET_TYPE, m_CacheRefGirderOffsetType[pgsTypes::Back]);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(TRUE);

   double skew_angle_1, skew_angle_2;
   GetPierSkewAngles(skew_angle_1,skew_angle_2);
   m_SpacingGrid[pgsTypes::Ahead].SetPierSkewAngle(skew_angle_1);
   m_SpacingGrid[pgsTypes::Back].SetPierSkewAngle(skew_angle_2);
}

void CSpanGirderLayoutPage::OnPrevPierGirderSpacingMeasureChanged() 
{
   CComboBox* pPrevCB = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   int cursel = pPrevCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pPrevCB->GetItemData(cursel),&ml,&mt);

   m_SpacingGrid[pgsTypes::Ahead].SetMeasurementType(mt);
   m_SpacingGrid[pgsTypes::Ahead].SetMeasurementLocation(ml);
   m_SpacingGrid[pgsTypes::Ahead].FillGrid();

   if ( 0 < IsDlgButtonChecked(IDC_SAME_GIRDER_SPACING) )
   {
      CComboBox* pNextCB = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);

      pNextCB->SetCurSel( pPrevCB->GetCurSel() );
      OnNextPierGirderSpacingMeasureChanged();
   }
}

void CSpanGirderLayoutPage::OnNextPierGirderSpacingMeasureChanged() 
{
   CComboBox* pNextCB = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   int cursel = pNextCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pNextCB->GetItemData(cursel),&ml,&mt);

   m_SpacingGrid[pgsTypes::Back].SetMeasurementType(mt);
   m_SpacingGrid[pgsTypes::Back].SetMeasurementLocation(ml);
   m_SpacingGrid[pgsTypes::Back].FillGrid();
}

HBRUSH CSpanGirderLayoutPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_NUMGDR_NOTE:
   case IDC_GIRDER_SPACING_NOTE:
   case IDC_GIRDERGRID_NOTE:
   case IDC_SLAB_OFFSET_NOTE:
      pDC->SetTextColor(HYPERLINK_COLOR);
      break;
   };

   return hbr;
}

GirderIndexType CSpanGirderLayoutPage::GetMinGirderCount()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   // It is ok to use girder 0 here because all the girders within the span
   // are of the same family. All the girders in the span will have the
   // same factory
   std::_tstring strGdrName = pParent->m_pSpanData->GetGirderTypes()->GetGirderName(0);

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



void CSpanGirderLayoutPage::UpdateGirderSpacingState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   BOOL bEnable = TRUE;

   if ( m_nGirders == 1 || IsBridgeSpacing(m_GirderSpacingType) )
   {
      // if there is only 1 girder or we are inputting spacing for the whole bridge
      // (not span by span) then disable the input controls
      bEnable = FALSE;
   }

   
   GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(bEnable);
   m_SpacingGrid[pgsTypes::Ahead].Enable(bEnable);

   GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);

   bEnable = ( 0 < IsDlgButtonChecked(IDC_SAME_GIRDER_SPACING) ? FALSE : bEnable);
   GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(bEnable);
   m_SpacingGrid[pgsTypes::Back].Enable(bEnable);

   GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);

   if ( m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
      GetDlgItem(IDC_GIRDER_SPACING_NOTE)->EnableWindow(FALSE);
   else
      GetDlgItem(IDC_GIRDER_SPACING_NOTE)->EnableWindow(m_nGirders == 1 ? FALSE : TRUE);
}

LRESULT CSpanGirderLayoutPage::OnChangeSameNumberOfGirders(WPARAM wParam,LPARAM lParam)
{
   m_bUseSameNumGirders = !m_bUseSameNumGirders;
   m_GirderNameGrid.UseSameNumGirders(m_bUseSameNumGirders);
   UpdateChildWindowState();
   return 0;
}

LRESULT CSpanGirderLayoutPage::OnChangeSameGirderSpacing(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
      return 0;

   // changing from uniform to general, or general to uniform spacing
   pgsTypes::SupportedBeamSpacing oldGirderSpacingType = m_GirderSpacingType;
   ToggleGirderSpacingType();

   if ( m_GirderSpacingType == pgsTypes::sbsUniform || m_GirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      // we are going from general to uniform spacing
      // if the grid has more than one spacing, we need to ask the user which one is to be
      // used for the entire bridge

      // cache the girder spacing data
      m_GirderSpacingCache[pgsTypes::Back]  = m_SpacingGrid[pgsTypes::Back].GetGirderSpacingData();
      m_GirderSpacingCache[pgsTypes::Ahead] = m_SpacingGrid[pgsTypes::Ahead].GetGirderSpacingData();

      CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
      CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
      m_CacheGirderSpacingMeasure[pgsTypes::Ahead] = (long)pcbStartOfSpanSpacingDatum->GetItemData( pcbStartOfSpanSpacingDatum->GetCurSel() );
      m_CacheGirderSpacingMeasure[pgsTypes::Back]  = (long)pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );

      // determine if there is more than one spacing group
      std::set<double> spacings;
      double bridgeSpacing = 0;
      long datum = 0;
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::PierFaceType pierFace = (i == 0 ? pgsTypes::Ahead : pgsTypes::Back);

         CGirderSpacingGridData spacingData = m_SpacingGrid[pierFace].GetGirderSpacingData(); 
         GroupIndexType nSpacingGroups = spacingData.m_GirderSpacing.GetSpacingGroupCount();
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            double space;
            spacingData.m_GirderSpacing.GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
            spacings.insert( space );
         }
      }

      if ( 1 < spacings.size() || (m_CacheGirderSpacingMeasure[pgsTypes::Ahead] != m_CacheGirderSpacingMeasure[pgsTypes::Back]))
      {
         // there is more than one unique girder spacing... which one do we want to use
         // for the entire bridge???
         CComPtr<IBroker> broker;
         EAFGetBroker(&broker);
         GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);

         CResolveGirderSpacingDlg dlg;
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

         // check connections to see what spacing locations we can use
         CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
         ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
         GetBearingMeasurementTypes(pParent->m_PrevPierConnectionName[pgsTypes::Ahead], pParent->m_NextPierConnectionName[pgsTypes::Back], &start_measure, &end_measure);
         dlg.m_RestrictSpacing = start_measure==ConnectionLibraryEntry::AlongGirder || end_measure==ConnectionLibraryEntry::AlongGirder;

         dlg.m_strSpacings = strItems;
         dlg.m_MeasurementDatum = 0;

         if ( dlg.DoModal() == IDOK )
         {
            iter = spacings.begin();
            for ( int i = 0; i < dlg.m_ItemIdx; i++ )
               iter++;

            bridgeSpacing = *iter;

            datum = dlg.m_MeasurementDatum;
         }
         else
         {
            return 0;
         }
      }
      else
      {
         // there is only one unique spacing value.. get it
         bridgeSpacing = m_SpacingGrid[pgsTypes::Back].GetGirderSpacingData().m_GirderSpacing.GetGirderSpacing(0);
         datum = m_CacheGirderSpacingMeasure[pgsTypes::Ahead];
      }

      // join all the girder spacings into one
      CGirderSpacingGridData spacingData = m_SpacingGrid[pgsTypes::Back].GetGirderSpacingData(); 
      spacingData.m_GirderSpacing.JoinAll(0);

      // set the spacing value
      spacingData.m_GirderSpacing.SetGirderSpacing(0,bridgeSpacing);

      // apply the new spacing to the grids
      spacingData.m_GirderSpacing.SetSpan(NULL);
      m_SpacingGrid[pgsTypes::Back].SetGirderSpacingData(spacingData);
      m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingData(spacingData);
      m_SpacingGrid[pgsTypes::Back].FillGrid();
      m_SpacingGrid[pgsTypes::Ahead].FillGrid();

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, datum);
      DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, datum);
   }
   else
   {
      // restore the girder spacing from the cached values
      GirderIndexType nGirders = m_SpacingGrid[pgsTypes::Back].GetGirderSpacingData().m_GirderTypes.GetGirderCount();

      m_SpacingGrid[pgsTypes::Back].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::Back]);
      m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::Ahead]);

      m_SpacingGrid[pgsTypes::Back].SetGirderCount(nGirders);
      m_SpacingGrid[pgsTypes::Ahead].SetGirderCount(nGirders);

      m_SpacingGrid[pgsTypes::Back].FillGrid();
      m_SpacingGrid[pgsTypes::Ahead].FillGrid();

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::Ahead]);
      DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::Back]);
   }

   m_SpacingGrid[pgsTypes::Back].SetGirderSpacingType(m_GirderSpacingType);
   m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingType(m_GirderSpacingType);

   UpdateChildWindowState();
   return 0;
}

LRESULT CSpanGirderLayoutPage::OnChangeSameGirderType(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   m_bUseSameGirderType = !m_bUseSameGirderType;
   
   if ( m_bUseSameGirderType )
   {
      // cache the current grid data
      m_GirderTypesCache = m_GirderNameGrid.m_GirderTypes;

      // if the grid has more than one girder type, we need to ask the user which one is to be
      // used for the entire bridge
      CGirderTypes girderTypes = m_GirderTypesCache;
      GroupIndexType nGroups = girderTypes.GetGirderGroupCount();
      std::_tstring strBridgeGirderName;
      if ( 1 < nGroups )
      {
         // there is more than one group... get all the unique girder names
         std::set<std::_tstring> girderNames;
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            std::_tstring strGirderName;
            girderTypes.GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strGirderName);
            girderNames.insert(strGirderName);
         }

         if ( 1 < girderNames.size() )
         {
            // there is more than one unique girder type... which one do we want to use
            // for the entire bridge???
            CSelectItemDlg dlg;
            dlg.m_strLabel = _T("Select the girder to be used for the entire bridge");
            dlg.m_strTitle = _T("Select Girder");
            dlg.m_ItemIdx = 0;

            CString strItems;
            std::set<std::_tstring>::iterator iter;
            for ( iter = girderNames.begin(); iter != girderNames.end(); iter++ )
            {
               std::_tstring strGirderName = *iter;

               if ( iter != girderNames.begin() )
                  strItems += _T("\n");

               strItems += CString(strGirderName.c_str());
            }

            dlg.m_strItems = strItems;
            if ( dlg.DoModal() == IDOK )
            {
               iter = girderNames.begin();
               for ( IndexType i = 0; i < dlg.m_ItemIdx; i++ )
                  iter++;

               strBridgeGirderName = *iter;
            }
            else
            {
               m_bUseSameGirderType = !m_bUseSameGirderType;
               UpdateChildWindowState();
               return 0;
            }
         }
         else
         {
            strBridgeGirderName = girderTypes.GetGirderName(0);
         }
      }
      else
      {
         strBridgeGirderName = girderTypes.GetGirderName(0);
      }

      // join all the girder types into one
      girderTypes.JoinAll(0);

      // set the girder name value
      girderTypes.SetGirderName(0,strBridgeGirderName.c_str());

      m_GirderNameGrid.m_GirderTypes = girderTypes;
   }
   else
   {
      // restore the cached values
      GirderIndexType nGirders = m_GirderNameGrid.m_GirderTypes.GetGirderCount();
      m_GirderNameGrid.m_GirderTypes = m_GirderTypesCache;
      m_GirderNameGrid.m_GirderTypes.SetGirderCount(nGirders);
   }

   m_GirderNameGrid.UseSameGirderName(m_bUseSameGirderType);
   UpdateChildWindowState();
   return 0;
}

LRESULT CSpanGirderLayoutPage::OnChangeSlabOffset(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
   {
      Float64 slabOffset[2];
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

      GetDlgItem(IDC_START_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      GetDlgItem(IDC_END_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      GetDlgItem(IDC_END_SLAB_OFFSET)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
      GetDlgItem(IDC_START_SLAB_OFFSET)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
   }
   else if ( m_SlabOffsetTypeCache == pgsTypes::sotGirder )
   {
      GetDlgItem(IDC_END_SLAB_OFFSET)->SetWindowText( _T("") );
      GetDlgItem(IDC_START_SLAB_OFFSET)->SetWindowText( _T("") );
   }
   else
   {
      // going to span by span slab offset
      GetDlgItem(IDC_START_SLAB_OFFSET)->SetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      GetDlgItem(IDC_END_SLAB_OFFSET)->SetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);
   }

   // swap slab offset type
   pgsTypes::SlabOffsetType temp = m_SlabOffsetType;
   m_SlabOffsetType = m_SlabOffsetTypeCache;
   m_SlabOffsetTypeCache = temp;

   UpdateChildWindowState();

   return 0;
}

BOOL CSpanGirderLayoutPage::OnSetActive() 
{
   UpdateChildWindowState();
	BOOL bResult = CPropertyPage::OnSetActive();

   GetDlgItem(IDC_START_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
   GetDlgItem(IDC_END_SLAB_OFFSET)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      GetDlgItem(IDC_START_SLAB_OFFSET)->SetWindowText(_T(""));
      GetDlgItem(IDC_END_SLAB_OFFSET)->SetWindowText(_T(""));
   }

   return bResult;
}

void CSpanGirderLayoutPage::UpdateChildWindowState()
{
   UpdateGirderCountHyperLinkText();
   UpdateGirderTypeHyperLinkText();
   UpdateGirderSpacingHyperLinkText();
   UpdateSlabOffsetHyperLinkText();

   UpdateGirderSpacingState();

   // Update slab offset state
   BOOL bEnable = FALSE;
   if ( m_SlabOffsetType == pgsTypes::sotSpan )
   {
      bEnable = TRUE;
   }

   GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   m_SlabOffsetHyperLink.EnableWindow( pParent->m_pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone ? FALSE : TRUE );
}

void CSpanGirderLayoutPage::ToggleGirderSpacingType()
{
   if ( m_GirderSpacingType == pgsTypes::sbsUniform )      // uniform to general
      m_GirderSpacingType = pgsTypes::sbsGeneral; 
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneral ) // general to uniform
      m_GirderSpacingType = pgsTypes::sbsUniform;
   else if ( m_GirderSpacingType == pgsTypes::sbsUniformAdjacent ) // uniform adjacent to general adjacent
      m_GirderSpacingType = pgsTypes::sbsGeneralAdjacent;
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent ) // general adjacent to uniform adjacent
      m_GirderSpacingType = pgsTypes::sbsUniformAdjacent;
}

void CSpanGirderLayoutPage::UpdateGirderCountHyperLinkText()
{
   if ( m_bUseSameNumGirders  )
   {
      GetDlgItem(IDC_NUMGDR_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_NUMGDR_SPIN)->EnableWindow(FALSE);
      GetDlgItem(IDC_NUMGDR)->EnableWindow(FALSE);
      m_NumGirdersHyperLink.SetWindowText(_T("The same number of girder are used in all spans"));
      m_NumGirdersHyperLink.SetURL(_T("Click to define number of girder span by span"));
   }
   else
   {
      m_NumGirdersHyperLink.SetWindowText(_T("The number of girders is defined span by span"));
      m_NumGirdersHyperLink.SetURL(_T("Click to use this number of girders in all spans"));
      GetDlgItem(IDC_NUMGDR_LABEL)->EnableWindow(TRUE);
      GetDlgItem(IDC_NUMGDR_SPIN)->EnableWindow(TRUE);
      GetDlgItem(IDC_NUMGDR)->EnableWindow(TRUE);
   }
}

void CSpanGirderLayoutPage::UpdateGirderTypeHyperLinkText()
{
   if ( m_bUseSameGirderType )
   {
      // girder name is shared with the entire bridge
      // must be edited on the Bridge Description General Tab
      // Girder name is simply displayed here for information
      m_GirderNameGrid.Enable(FALSE);

      m_GirderTypeHyperLink.SetWindowText(_T("This type of girder is used for the entire bridge"));
      m_GirderTypeHyperLink.SetURL(_T("Click to defined girders span by span"));
   }
   else
   {
      m_GirderTypeHyperLink.SetWindowText(_T("Girder types are defined span by span"));
      m_GirderTypeHyperLink.SetURL(_T("Click to use this girder for the entire bridge"));
      m_GirderNameGrid.Enable(TRUE);
   }
}

void CSpanGirderLayoutPage::UpdateGirderSpacingHyperLinkText()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   BOOL bEnable = TRUE;
   if ( m_GirderSpacingType == pgsTypes::sbsUniform )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("The same girder spacing is used for the entire bridge"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to define girder spacing span by span"));

      bEnable = (m_SpacingGrid[pgsTypes::Ahead].InputSpacing() && m_SpacingGrid[pgsTypes::Back].InputSpacing() ? TRUE : FALSE);

      GetDlgItem(IDC_SAME_GIRDER_SPACING)->EnableWindow(FALSE);
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("The same joint spacing is used for the entire bridge"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to define joint spacing span by span"));

      bEnable = (m_SpacingGrid[pgsTypes::Ahead].InputSpacing() && m_SpacingGrid[pgsTypes::Back].InputSpacing() ? TRUE : FALSE);

      GetDlgItem(IDC_SAME_GIRDER_SPACING)->EnableWindow(FALSE);
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
   {
      CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

      CString strWindowText;
      strWindowText.Format(_T("The same girder spacing must be used for the entire bridge for %s girders"),
                            pParent->m_pSpanData->GetBridgeDescription()->GetGirderFamilyName());

      m_GirderSpacingHyperLink.SetWindowText(strWindowText);
      m_GirderSpacingHyperLink.SetURL(_T(""));

      bEnable = TRUE;

      GetDlgItem(IDC_SAME_GIRDER_SPACING)->EnableWindow(FALSE);
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneral )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("Girder spacing is defined span by span"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to make girder spacing the same for all spans"));

      bEnable = TRUE;

      GetDlgItem(IDC_SAME_GIRDER_SPACING)->EnableWindow(TRUE);
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("Joint spacing is defined span by span"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to make joint spacing the same for all spans"));

      bEnable = TRUE;

      GetDlgItem(IDC_SAME_GIRDER_SPACING)->EnableWindow(TRUE);
   }
   else
   {
      ATLASSERT( false ); // is there a new spacing type???
   }

   m_GirderSpacingHyperLink.EnableWindow(bEnable);
}

void CSpanGirderLayoutPage::UpdateSlabOffsetHyperLinkText()
{
   if (m_SlabOffsetType == pgsTypes::sotBridge )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("The same slab offset is used for the entire bridge"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define slab offset span by span"));
   }
   else if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A unique slab offset is used for every girder"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define slab offset span by span"));
   }
   else
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("Slab offset is defined span by span"));

      if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this slab offset for the entire bridge"));
      else if ( m_SlabOffsetTypeCache == pgsTypes::sotGirder )
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this slab offset for every girder in this span"));
   }
}

void CSpanGirderLayoutPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPANDETAILS_GIRDERSPACING );
}

bool CSpanGirderLayoutPage::AllowConnectionChange(pgsTypes::PierFaceType side, const CString& connectionName)
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
            pgsTypes::MeasurementLocation ml;
            pgsTypes::MeasurementType     mt;
            UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Back],&ml,&mt);
            m_GirderSpacingMeasure[pgsTypes::Back] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
            m_SpacingGrid[pgsTypes::Back].SetMeasurementLocation(m_GirderSpacingMeasurementLocation);

            UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Ahead],&ml,&mt);
            m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
            m_SpacingGrid[pgsTypes::Ahead].SetMeasurementLocation(m_GirderSpacingMeasurementLocation);

            return true;
         }
         else if ( result == IDNO )
         {
            ToggleGirderSpacingType(); // change girder spacing type to span-by-span
            
            pgsTypes::MeasurementLocation ml;
            pgsTypes::MeasurementType     mt;
            UnhashGirderSpacing(m_GirderSpacingMeasure[side],&ml,&mt);
            m_GirderSpacingMeasure[side] = HashGirderSpacing(pgsTypes::AtCenterlinePier,mt);
            m_SpacingGrid[side].SetMeasurementLocation(pgsTypes::AtCenterlinePier);

            m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingType( m_GirderSpacingType );
            m_SpacingGrid[pgsTypes::Back].SetGirderSpacingType( m_GirderSpacingType );

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
   else if (m_bUseSameSpacingAtBothEnds)
   {
      // have to check and change both ends if needed
      pgsTypes::MeasurementLocation mla, mlb;
      pgsTypes::MeasurementType mt;
      UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Ahead],&mla,&mt);
      UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Back],&mlb,&mt);
      if (mla==pgsTypes::AtCenterlineBearing || mlb==pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG);
         int st = ::AfxMessageBox(msg, MB_ICONQUESTION|MB_YESNO);
         if (st==IDYES)
         {
            m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(pgsTypes::AtCenterlinePier,mt);
            m_GirderSpacingMeasure[pgsTypes::Back] = m_GirderSpacingMeasure[pgsTypes::Ahead];

            // Real data set back is in grid, must also update this
            m_SpacingGrid[pgsTypes::Ahead].SetMeasurementLocation(pgsTypes::AtCenterlinePier);
            m_SpacingGrid[pgsTypes::Back ].SetMeasurementLocation(pgsTypes::AtCenterlinePier);

            // also the cached spacing may not be compatible with the new 
            m_CacheGirderSpacingMeasure[pgsTypes::Ahead] = m_GirderSpacingMeasure[pgsTypes::Ahead];
            m_CacheGirderSpacingMeasure[pgsTypes::Back]  = m_GirderSpacingMeasure[pgsTypes::Ahead];

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

            m_SpacingGrid[side].SetMeasurementLocation(pgsTypes::AtCenterlinePier); // also update grid data
            m_CacheGirderSpacingMeasure[side]  = m_GirderSpacingMeasure[side];
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
