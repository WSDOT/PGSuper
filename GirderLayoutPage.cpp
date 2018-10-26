///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "PGSuperDoc.h"
#include "GirderLayoutPage.h"
#include "SpanDetailsDlg.h"
#include "SelectItemDlg.h"
#include "ResolveGirderSpacingDlg.h"
#include "PGSuperColors.h"
#include "HtmlHelp\HelpTopics.hh"

#include <PGSuperUnits.h>


#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
      Float64 offset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
      pParent->m_pPrevPier->GetBearingOffset(pgsTypes::Ahead, &offset,&start_measure);
      pParent->m_pNextPier->GetBearingOffset(pgsTypes::Back,  &offset,&end_measure);

      FillGirderSpacingMeasurementComboBox(IDC_PREV_PIER_GIRDER_SPACING_MEASURE,pgsTypes::metStart,start_measure);
      FillGirderSpacingMeasurementComboBox(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE,pgsTypes::metEnd,  end_measure);
   }

   DDX_Control(pDX, IDC_NUMGDR_NOTE,         m_NumGirdersHyperLink);
   DDX_Control(pDX, IDC_GIRDER_SPACING_NOTE, m_GirderSpacingHyperLink);
   DDX_Control(pDX, IDC_GIRDERGRID_NOTE,     m_GirderTypeHyperLink);
   DDX_Control(pDX, IDC_SLAB_OFFSET_NOTE,    m_SlabOffsetHyperLink);

   DDX_Text( pDX, IDC_NUMGDR, m_nGirders );
   DDV_MinMaxLongLong(pDX, m_nGirders, m_MinGirderCount, MAX_GIRDERS_PER_SPAN );

   DDX_CBItemData(pDX, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::metStart]);
   DDX_CBItemData(pDX, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::metEnd]);

   DDV_SpacingGrid(pDX,IDC_PREV_PIER_SPACING_GRID,&m_SpacingGrid[pgsTypes::metStart]);
   DDV_SpacingGrid(pDX,IDC_NEXT_PIER_SPACING_GRID,&m_SpacingGrid[pgsTypes::metEnd]);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER, m_RefGirderIdx[pgsTypes::metStart]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER, m_RefGirderIdx[pgsTypes::metEnd]);

   DDX_OffsetAndTag(pDX, IDC_PREV_REF_GIRDER_OFFSET,IDC_PREV_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit());
   DDX_OffsetAndTag(pDX, IDC_NEXT_REF_GIRDER_OFFSET,IDC_NEXT_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::metEnd], pDisplayUnits->GetXSectionDimUnit());

   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::metStart]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::metEnd]);

   DDX_Tag(pDX, IDC_START_SLAB_OFFSET_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_END_SLAB_OFFSET_UNIT,   pDisplayUnits->GetComponentDimUnit() );
   if ( pParent->m_pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   {
      CEAFDocument* pDoc = EAFGetDocument();
      BOOL bExchange;
      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
      {
         // for PGSuper, only take slab offset out of dialog box controls if
         // the slab offset type is Group... that is, slab offset is defined span by span
         bExchange = (m_SlabOffsetType == pgsTypes::sotGroup);
      }
      else
      {
         // for PGSplice, only take the slab offset out of dialog box controls if
         // the slab offset typs is Group or Segment... For group, slab offset is defined at piers
         // for Segment, slab offset is defined at the ends of all segments which would be the start
         // and end pier of a girder, and at the closure pours
         bExchange = (m_SlabOffsetType == pgsTypes::sotGroup || m_SlabOffsetType == pgsTypes::sotSegment);
      }

      if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && bExchange) )
      {
         DDX_UnitValueAndTag(pDX, IDC_START_SLAB_OFFSET, IDC_START_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit() );
         DDX_UnitValueAndTag(pDX, IDC_END_SLAB_OFFSET,   IDC_END_SLAB_OFFSET_UNIT,   m_SlabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit() );

         // Validate slab offset
         if ( pDX->m_bSaveAndValidate )
         {
            // Span index is group index for precast girder bridges
            CSegmentKey segmentKey(pParent->m_pSpanData->GetIndex(),0,0);

            GET_IFACE2(pBroker,IBridge,pBridge);
            Float64 Lg = pBridge->GetSegmentLength(segmentKey);

            pgsPointOfInterest poiStart(segmentKey,0.0);
            pgsPointOfInterest poiEnd(segmentKey,Lg);

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
   ON_BN_CLICKED(IDC_COPY_TO_END,OnCopySpacingToEnd)
   ON_BN_CLICKED(IDC_COPY_TO_START,OnCopySpacingToStart)
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
void CSpanGirderLayoutPage::Init(const CSpanData2* pSpan)
{
   const CBridgeDescription2* pBridgeDesc = pSpan->GetBridgeDescription();
   const CGirderGroupData* pGroup         = pBridgeDesc->GetGirderGroup(pSpan);
   m_GirderSpacingType                    = pBridgeDesc->GetGirderSpacingType();
   m_GirderSpacingMeasurementLocation     = pBridgeDesc->GetMeasurementLocation();
   m_bUseSameNumGirders                   = pBridgeDesc->UseSameNumberOfGirdersInAllGroups();
   m_bUseSameGirderType                   = pBridgeDesc->UseSameGirderForEntireBridge();

   const CPierData2* pPrevPier = pSpan->GetPrevPier();
   const CPierData2* pNextPier = pSpan->GetNextPier();

   // This dialog page is only used for PGSuper documents (Precast Girder Bridges)
   // The span and the girder group must have the same piers
   ATLASSERT(EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)));
   ATLASSERT(pGroup->GetPier(pgsTypes::metStart) == pPrevPier);
   ATLASSERT(pGroup->GetPier(pgsTypes::metEnd)   == pNextPier);

   m_GirderNameGrid.m_GirderGroup = *pGroup;

   const CGirderSpacing2* pStartSpacing = pPrevPier->GetGirderSpacing(pgsTypes::Ahead);
   pgsTypes::MeasurementLocation ml = pStartSpacing->GetMeasurementLocation();
   pgsTypes::MeasurementType     mt = pStartSpacing->GetMeasurementType();
   m_GirderSpacingMeasure[pgsTypes::metStart] = HashGirderSpacing(ml,mt);

   m_RefGirderIdx[pgsTypes::metStart]        = pStartSpacing->GetRefGirder();
   m_RefGirderOffset[pgsTypes::metStart]     = pStartSpacing->GetRefGirderOffset();
   m_RefGirderOffsetType[pgsTypes::metStart] = pStartSpacing->GetRefGirderOffsetType();

   m_CacheRefGirderIdx[pgsTypes::metStart]        = m_RefGirderIdx[pgsTypes::metStart];
   m_CacheRefGirderOffset[pgsTypes::metStart]     = m_RefGirderOffset[pgsTypes::metStart];
   m_CacheRefGirderOffsetType[pgsTypes::metStart] = m_RefGirderOffsetType[pgsTypes::metStart];

   const CGirderSpacing2* pEndSpacing = pNextPier->GetGirderSpacing(pgsTypes::Back);
   ml = pEndSpacing->GetMeasurementLocation();
   mt = pEndSpacing->GetMeasurementType();
   m_GirderSpacingMeasure[pgsTypes::metEnd] = HashGirderSpacing(ml,mt);
   
   m_CacheGirderSpacingMeasure[pgsTypes::metEnd]  = m_GirderSpacingMeasure[pgsTypes::metEnd];
   m_CacheGirderSpacingMeasure[pgsTypes::metStart] = m_GirderSpacingMeasure[pgsTypes::metStart];

   m_RefGirderIdx[pgsTypes::metEnd]        = pEndSpacing->GetRefGirder();
   m_RefGirderOffset[pgsTypes::metEnd]     = pEndSpacing->GetRefGirderOffset();
   m_RefGirderOffsetType[pgsTypes::metEnd] = pEndSpacing->GetRefGirderOffsetType();

   m_CacheRefGirderIdx[pgsTypes::metEnd]        = m_RefGirderIdx[pgsTypes::metEnd];
   m_CacheRefGirderOffset[pgsTypes::metEnd]     = m_RefGirderOffset[pgsTypes::metEnd];
   m_CacheRefGirderOffsetType[pgsTypes::metEnd] = m_RefGirderOffsetType[pgsTypes::metEnd];

   m_nGirders = pGroup->GetGirderCount();

   Float64 skew_angle = 0; // dummy value (on purpose!)

   // ahead of pier (start of span)
   CGirderSpacingGridData gridData;
   gridData.m_GirderSpacing = *pStartSpacing;
   gridData.m_GirderGroup   = *pGroup;
   gridData.m_PierFace      = pgsTypes::Ahead;
   m_GirderSpacingCache[pgsTypes::metStart] = gridData;

   m_SpacingGrid[pgsTypes::metStart].Init( m_GirderSpacingType,
                                         m_bUseSameNumGirders,
                                         pStartSpacing,
                                         pGroup,
                                         pgsTypes::Ahead,
                                         pSpan->GetPrevPier()->GetIndex(),
                                         skew_angle,
                                         pSpan->GetPrevPier()->GetNextSpan() == NULL ? true : false,
                                         pBridgeDesc->GetDeckDescription()->DeckType);


   // back of pier (end of span)
   gridData.m_GirderSpacing = *pEndSpacing;
   gridData.m_GirderGroup   = *pGroup;
   gridData.m_PierFace      = pgsTypes::Back;
   m_GirderSpacingCache[pgsTypes::metEnd] = gridData;

   m_SpacingGrid[pgsTypes::metEnd].Init( m_GirderSpacingType,
                                           m_bUseSameNumGirders,
                                           pEndSpacing,
                                           pGroup,
                                           pgsTypes::Back,
                                           pSpan->GetNextPier()->GetIndex(),
                                           skew_angle,
                                           pSpan->GetNextPier()->GetPrevSpan() == NULL ? true : false,
                                           pBridgeDesc->GetDeckDescription()->DeckType);

   // slab offset
   m_SlabOffsetType = pSpan->GetBridgeDescription()->GetSlabOffsetType();
   if ( m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotSegment )
      m_SlabOffsetTypeCache = pgsTypes::sotGroup;
   else
      m_SlabOffsetTypeCache = pgsTypes::sotBridge;

   if ( m_SlabOffsetType == pgsTypes::sotSegment )
   {
      // slab offset is unique for each girder... which one do we use??? For now, use girder 0
      m_SlabOffset[pgsTypes::metStart] = pGroup->GetSlabOffset(0,pgsTypes::metStart);
      m_SlabOffset[pgsTypes::metEnd]   = pGroup->GetSlabOffset(0,pgsTypes::metEnd);
   }
   else
   {
      m_SlabOffset[pgsTypes::metStart] = pGroup->GetSlabOffset(INVALID_INDEX,pgsTypes::metStart);
      m_SlabOffset[pgsTypes::metEnd]   = pGroup->GetSlabOffset(INVALID_INDEX,pgsTypes::metEnd);
   }
}

void CSpanGirderLayoutPage::GetPierSkewAngles(Float64& skew1,Float64& skew2)
{
   CSpanDetailsDlg* pParent   = (CSpanDetailsDlg*)GetParent();
   const CPierData2* pPrevPier = pParent->m_pSpanData->GetPrevPier();
   const CPierData2* pNextPier = pParent->m_pSpanData->GetNextPier();

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   Float64 skew_angle_1;
   pBridge->GetSkewAngle(pPrevPier->GetStation(),pPrevPier->GetOrientation(),&skew_angle_1);

   Float64 skew_angle_2;
   pBridge->GetSkewAngle(pNextPier->GetStation(),pNextPier->GetOrientation(),&skew_angle_2);

   skew1 = skew_angle_1;
   skew2 = skew_angle_2;
}

BOOL CSpanGirderLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   Float64 skew_angle_1, skew_angle_2;
   GetPierSkewAngles(skew_angle_1,skew_angle_2);

   // set up the girder name grid
	m_GirderNameGrid.SubclassDlgItem(IDC_GIRDERGRID, this);
   m_GirderNameGrid.CustomInit(pParent->m_pGirderGroup);
   m_GirderGroupCache = m_GirderNameGrid.m_GirderGroup;


   // set up the previous pier girder spacing input
   m_SpacingGrid[pgsTypes::metStart].SubclassDlgItem(IDC_PREV_PIER_SPACING_GRID,this);
   m_SpacingGrid[pgsTypes::metStart].CustomInit();
   m_SpacingGrid[pgsTypes::metStart].SetPierSkewAngle(skew_angle_1);


   // set up the next pier girder spacing input
   m_SpacingGrid[pgsTypes::metEnd].SubclassDlgItem(IDC_NEXT_PIER_SPACING_GRID,this);
   m_SpacingGrid[pgsTypes::metEnd].CustomInit();
   m_SpacingGrid[pgsTypes::metEnd].SetPierSkewAngle(skew_angle_2);

   FillRefGirderComboBox(pgsTypes::metStart);
   FillRefGirderComboBox(pgsTypes::metEnd);
   FillRefGirderOffsetTypeComboBox(pgsTypes::metStart);
   FillRefGirderOffsetTypeComboBox(pgsTypes::metEnd);

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
   }
   else
   {
      GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
      GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
   }
  
   UpdateChildWindowState();

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
   m_SpacingGrid[pgsTypes::metStart].AddGirders(nGirders);
   m_SpacingGrid[pgsTypes::metEnd].AddGirders(nGirders);
   UpdateGirderSpacingState();
}

void CSpanGirderLayoutPage::RemoveGirders(GirderIndexType nGirders)
{
   m_nGirders -= nGirders;

   m_GirderNameGrid.RemoveGirders(nGirders);
   m_SpacingGrid[pgsTypes::metStart].RemoveGirders(nGirders);
   m_SpacingGrid[pgsTypes::metEnd].RemoveGirders(nGirders);

   UpdateGirderSpacingState();
}

void CSpanGirderLayoutPage::FillGirderSpacingMeasurementComboBox(int nIDC, pgsTypes::MemberEndType end,ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure)
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(nIDC);
   pSpacingType->ResetContent();

   int idx = pSpacingType->AddString(IsAbutment(end) ? _T("Measured at and along abutment line") : _T("Measured at and along the pier line"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   
   idx = pSpacingType->AddString(IsAbutment(end) ? _T("Measured normal to alignment at abutment line") : _T("Measured normal to alignment at pier line"));
   item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem);
   pSpacingType->SetItemData(idx,item_data);
   
   if (bearingMeasure != ConnectionLibraryEntry::AlongGirder)
   {
      idx = pSpacingType->AddString(_T("Measured at and along the CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem);
      pSpacingType->SetItemData(idx,item_data);

      idx = pSpacingType->AddString(_T("Measured normal to alignment at CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem);
      pSpacingType->SetItemData(idx,item_data);
   }
}

void CSpanGirderLayoutPage::FillRefGirderOffsetTypeComboBox(pgsTypes::MemberEndType end)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(end == pgsTypes::metStart ? IDC_PREV_REF_GIRDER_OFFSET_TYPE : IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   int idx = pCB->AddString(_T("Alignment"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtAlignment);

   idx = pCB->AddString(_T("Bridge Line"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtBridge);
}

void CSpanGirderLayoutPage::FillRefGirderComboBox(pgsTypes::MemberEndType end)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(end == pgsTypes::metStart ? IDC_PREV_REF_GIRDER : IDC_NEXT_REF_GIRDER);
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

void CSpanGirderLayoutPage::OnPrevPierGirderSpacingMeasureChanged() 
{
   CComboBox* pPrevCB = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   int cursel = pPrevCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pPrevCB->GetItemData(cursel),&ml,&mt);

   m_SpacingGrid[pgsTypes::metStart].SetMeasurementType(mt);
   m_SpacingGrid[pgsTypes::metStart].SetMeasurementLocation(ml);
   m_SpacingGrid[pgsTypes::metStart].FillGrid();
}

void CSpanGirderLayoutPage::OnNextPierGirderSpacingMeasureChanged() 
{
   CComboBox* pNextCB = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   int cursel = pNextCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pNextCB->GetItemData(cursel),&ml,&mt);

   m_SpacingGrid[pgsTypes::metEnd].SetMeasurementType(mt);
   m_SpacingGrid[pgsTypes::metEnd].SetMeasurementLocation(ml);
   m_SpacingGrid[pgsTypes::metEnd].FillGrid();
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
   const CBridgeDescription2* pBridgeDesc = pParent->m_pSpanData->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pParent->m_pSpanData);

   // It is ok to use girder 0 here because all the girders within the span
   // are of the same family. All the girders in the span will have the
   // same factory
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(0);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<IGirderSection> section;
   factory->CreateGirderSection(pBroker,0,pGdrEntry->GetDimensions(),-1,-1,&section);

   WebIndexType nWebs;
   section->get_WebCount(&nWebs);

   return (1 < nWebs ? 1 : 2);
}

void CSpanGirderLayoutPage::UpdateGirderSpacingState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   BOOL bPrevEnable = m_SpacingGrid[pgsTypes::metStart].InputSpacing();
   BOOL bNextEnable = m_SpacingGrid[pgsTypes::metEnd].InputSpacing();

   if ( m_nGirders == 1 || IsBridgeSpacing(m_GirderSpacingType) )
   {
      // if there is only 1 girder or we are inputting spacing for the whole bridge
      // (not span by span) then disable the input controls
      bPrevEnable = FALSE;
      bNextEnable = FALSE;
   }

   // Prev Pier
   GetDlgItem(IDC_PREV_PIER_LABEL)->EnableWindow(bPrevEnable);
   GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->EnableWindow(bPrevEnable);
   GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(bPrevEnable);
   m_SpacingGrid[pgsTypes::metStart].Enable(bPrevEnable);

   // Next Pier
   GetDlgItem(IDC_NEXT_PIER_LABEL)->EnableWindow(bNextEnable);
   GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->EnableWindow(bNextEnable);
   GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(bNextEnable);
   m_SpacingGrid[pgsTypes::metEnd].Enable(bNextEnable);

   if ( m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
      GetDlgItem(IDC_GIRDER_SPACING_NOTE)->EnableWindow(FALSE);
   else
      GetDlgItem(IDC_GIRDER_SPACING_NOTE)->EnableWindow(m_nGirders == 1 ? FALSE : TRUE);


   BOOL bEnable;
   if ( ::IsBridgeSpacing(m_GirderSpacingType) )
      bEnable = FALSE;
   else
      bEnable = TRUE;

   GetDlgItem(IDC_COPY_TO_END)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);

   GetDlgItem(IDC_COPY_TO_START)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);
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
      m_GirderSpacingCache[pgsTypes::metEnd]  = m_SpacingGrid[pgsTypes::metEnd].GetGirderSpacingData();
      m_GirderSpacingCache[pgsTypes::metStart] = m_SpacingGrid[pgsTypes::metStart].GetGirderSpacingData();

      CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
      CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
      m_CacheGirderSpacingMeasure[pgsTypes::metStart] = (DWORD)pcbStartOfSpanSpacingDatum->GetItemData( pcbStartOfSpanSpacingDatum->GetCurSel() );
      m_CacheGirderSpacingMeasure[pgsTypes::metEnd]   = (DWORD)pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );

      // determine if there is more than one spacing group
      std::set<Float64> spacings;
      Float64 bridgeSpacing = 0;
      long datum = 0;
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);

         CGirderSpacingGridData spacingData = m_SpacingGrid[end].GetGirderSpacingData(); 
         GroupIndexType nSpacingGroups = spacingData.m_GirderSpacing.GetSpacingGroupCount();
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            Float64 space;
            spacingData.m_GirderSpacing.GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
            spacings.insert( space );
         }
      }

      if ( 1 < spacings.size() || (m_CacheGirderSpacingMeasure[pgsTypes::metStart] != m_CacheGirderSpacingMeasure[pgsTypes::metEnd]))
      {
         // there is more than one unique girder spacing... which one do we want to use
         // for the entire bridge???
         CComPtr<IBroker> broker;
         EAFGetBroker(&broker);
         GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);

         CResolveGirderSpacingDlg dlg;
         CString strItems;
         std::set<Float64>::iterator iter;
         for ( iter = spacings.begin(); iter != spacings.end(); iter++ )
         {
            Float64 spacing = *iter;

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
         Float64 offset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
         pParent->m_pPrevPier->GetBearingOffset(pgsTypes::Ahead,&offset,&start_measure);
         pParent->m_pNextPier->GetBearingOffset(pgsTypes::Back, &offset,&end_measure);
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
         bridgeSpacing = m_SpacingGrid[pgsTypes::metEnd].GetGirderSpacingData().m_GirderSpacing.GetGirderSpacing(0);
         datum = m_CacheGirderSpacingMeasure[pgsTypes::metStart];
      }

      // join all the girder spacings into one
      CGirderSpacingGridData spacingData = m_SpacingGrid[pgsTypes::metEnd].GetGirderSpacingData(); 
      spacingData.m_GirderSpacing.JoinAll(0);

      // set the spacing value
      spacingData.m_GirderSpacing.SetGirderSpacing(0,bridgeSpacing);

      // apply the new spacing to the grids
      //spacingData.m_GirderSpacing.SetSpan(NULL);
      m_SpacingGrid[pgsTypes::metEnd].SetGirderSpacingData(spacingData);
      m_SpacingGrid[pgsTypes::metStart].SetGirderSpacingData(spacingData);
      m_SpacingGrid[pgsTypes::metEnd].FillGrid();
      m_SpacingGrid[pgsTypes::metStart].FillGrid();

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, datum);
      DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, datum);
   }
   else
   {
      // restore the girder spacing from the cached values
      GirderIndexType nGirders = m_SpacingGrid[pgsTypes::metEnd].GetGirderSpacingData().m_GirderGroup.GetGirderCount();

      m_SpacingGrid[pgsTypes::metEnd].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::metEnd]);
      m_SpacingGrid[pgsTypes::metStart].SetGirderSpacingData(m_GirderSpacingCache[pgsTypes::metStart]);

      m_SpacingGrid[pgsTypes::metEnd].SetGirderCount(nGirders);
      m_SpacingGrid[pgsTypes::metStart].SetGirderCount(nGirders);

      m_SpacingGrid[pgsTypes::metEnd].FillGrid();
      m_SpacingGrid[pgsTypes::metStart].FillGrid();

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::metStart]);
      DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::metEnd]);
   }

   m_SpacingGrid[pgsTypes::metEnd].SetGirderSpacingType(m_GirderSpacingType);
   m_SpacingGrid[pgsTypes::metStart].SetGirderSpacingType(m_GirderSpacingType);

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
      m_GirderGroupCache = m_GirderNameGrid.m_GirderGroup;;

      // if the grid has more than one girder type, we need to ask the user which one is to be
      // used for the entire bridge
      CGirderGroupData girderGroup = m_GirderGroupCache;
      GroupIndexType nGirderTypeGroups = girderGroup.GetGirderTypeGroupCount();
      std::_tstring strBridgeGirderName;
      if ( 1 < nGirderTypeGroups )
      {
         // there is more than one group... get all the unique girder names
         std::set<std::_tstring> girderNames;
         for ( GroupIndexType girderGroupTypeIdx = 0; girderGroupTypeIdx < nGirderTypeGroups; girderGroupTypeIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            std::_tstring strGirderName;
            girderGroup.GetGirderTypeGroup(girderGroupTypeIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);
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
            strBridgeGirderName = girderGroup.GetGirderName(0);
         }
      }
      else
      {
         strBridgeGirderName = girderGroup.GetGirderName(0);
      }

      // join all the girder types into one
      girderGroup.JoinAll(0);

      // set the girder name value
      girderGroup.SetGirderName(0,strBridgeGirderName.c_str());

      m_GirderNameGrid.m_GirderGroup = girderGroup;
   }
   else
   {
      // restore the cached values
      GirderIndexType nGirders = m_GirderNameGrid.m_GirderGroup.GetGirderCount();
      m_GirderNameGrid.m_GirderGroup = m_GirderGroupCache;
      m_GirderNameGrid.m_GirderGroup.SetGirderCount(nGirders);
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
   else if ( m_SlabOffsetTypeCache == pgsTypes::sotSegment )
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
   if ( m_SlabOffsetType == pgsTypes::sotSegment )
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
   if ( m_SlabOffsetType == pgsTypes::sotGroup )
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

      bEnable = (m_SpacingGrid[pgsTypes::metStart].InputSpacing() && m_SpacingGrid[pgsTypes::metEnd].InputSpacing() ? TRUE : FALSE);
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("The same joint spacing is used for the entire bridge"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to define joint spacing span by span"));

      bEnable = (m_SpacingGrid[pgsTypes::metStart].InputSpacing() && m_SpacingGrid[pgsTypes::metEnd].InputSpacing() ? TRUE : FALSE);
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
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneral )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("Girder spacing is defined span by span"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to make girder spacing the same for all spans"));

      bEnable = TRUE;
   }
   else if ( m_GirderSpacingType == pgsTypes::sbsGeneralAdjacent )
   {
      m_GirderSpacingHyperLink.SetWindowText(_T("Joint spacing is defined span by span"));
      m_GirderSpacingHyperLink.SetURL(_T("Click to make joint spacing the same for all spans"));

      bEnable = TRUE;
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
   else if ( m_SlabOffsetType == pgsTypes::sotSegment )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A unique slab offset is used for every girder"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define slab offset span by span"));
   }
   else
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("Slab offset is defined span by span"));

      if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this slab offset for the entire bridge"));
      else if ( m_SlabOffsetTypeCache == pgsTypes::sotSegment )
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this slab offset for every girder in this span"));
   }
}

void CSpanGirderLayoutPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPANDETAILS_GIRDERSPACING );
}

bool CSpanGirderLayoutPage::AllowConnectionChange(pgsTypes::MemberEndType end, const CString& connectionName)
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
      UnhashGirderSpacing(m_GirderSpacingMeasure[end],&ml,&mt);
      if (ml == pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG2);
         int result = ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
         if ( result == IDYES )
         {
            m_GirderSpacingMeasurementLocation = pgsTypes::AtPierLine;

            // girder spacing is still measured at the bridge level. need to update
            // the span by span girder measure to reflect the current bridge level measurement location
            pgsTypes::MeasurementLocation ml;
            pgsTypes::MeasurementType     mt;
            UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::metEnd],&ml,&mt);
            m_GirderSpacingMeasure[pgsTypes::metEnd] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
            m_SpacingGrid[pgsTypes::metEnd].SetMeasurementLocation(m_GirderSpacingMeasurementLocation);

            UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::metStart],&ml,&mt);
            m_GirderSpacingMeasure[pgsTypes::metStart] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
            m_SpacingGrid[pgsTypes::metStart].SetMeasurementLocation(m_GirderSpacingMeasurementLocation);

            return true;
         }
         else if ( result == IDNO )
         {
            ToggleGirderSpacingType(); // change girder spacing type to span-by-span
            
            pgsTypes::MeasurementLocation ml;
            pgsTypes::MeasurementType     mt;
            UnhashGirderSpacing(m_GirderSpacingMeasure[end],&ml,&mt);
            m_GirderSpacingMeasure[end] = HashGirderSpacing(pgsTypes::AtPierLine,mt);
            m_SpacingGrid[end].SetMeasurementLocation(pgsTypes::AtPierLine);

            m_SpacingGrid[pgsTypes::metStart].SetGirderSpacingType( m_GirderSpacingType );
            m_SpacingGrid[pgsTypes::metEnd].SetGirderSpacingType( m_GirderSpacingType );

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
      UnhashGirderSpacing(m_GirderSpacingMeasure[end],&ml,&mt);
      if (ml==pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG);
         int st = ::AfxMessageBox(msg, MB_ICONQUESTION|MB_YESNO);
         if (st==IDYES)
         {
            m_GirderSpacingMeasure[end] = HashGirderSpacing(pgsTypes::AtPierLine,mt);

            m_SpacingGrid[end].SetMeasurementLocation(pgsTypes::AtPierLine); // also update grid data
            m_CacheGirderSpacingMeasure[end]  = m_GirderSpacingMeasure[end];
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

bool CSpanGirderLayoutPage::IsAbutment(pgsTypes::MemberEndType end)
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if ( end == pgsTypes::metStart && pParent->m_pPrevPier->GetPrevSpan() == NULL )
      return true;
   else if ( end == pgsTypes::metEnd && pParent->m_pNextPier->GetNextSpan() == NULL )
      return true;
   else
      return false;
}

void CSpanGirderLayoutPage::OnCopySpacingToEnd()
{
   // Copy girder spacing from the start of the span to the end of the span
   // Prev Pier, Ahead side --> Next Pier, Back side

   // Girder spacing measurement type
   CComboBox* pcbBackMeasure  = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   CComboBox* pcbAheadMeasure = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   pcbBackMeasure->SetCurSel(pcbAheadMeasure->GetCurSel());

   // Spacing Grid
   m_SpacingGrid[pgsTypes::metEnd].SetGirderSpacingData(m_SpacingGrid[pgsTypes::metStart].GetGirderSpacingData());
   m_SpacingGrid[pgsTypes::metEnd].FillGrid();

   // Spacing Location

   // reference girder index
   CComboBox* pcbBackRefGirder  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER);
   CComboBox* pcbAheadRefGirder = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER);
   pcbAheadRefGirder->SetCurSel(pcbBackRefGirder->GetCurSel());

   // reference girder offset
   CWnd* pwndBackRefGirderOffset  = GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET);
   CWnd* pwndAheadRefGirderOffset = GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET);
   CString strText;
   pwndBackRefGirderOffset->GetWindowText(strText);
   pwndAheadRefGirderOffset->SetWindowText(strText);

   // reference girder offset type
   CComboBox* pcbBackRefGirderOffsetType  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE);
   CComboBox* pcbAheadRefGirderOffsetType = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   pcbAheadRefGirderOffsetType->SetCurSel(pcbBackRefGirderOffsetType->GetCurSel());
}

void CSpanGirderLayoutPage::OnCopySpacingToStart()
{
   // Copy girder spacing from the end of the span to the start of the span
   // Next Pier, Back side --> Prev Pier, Ahead side

   // Girder spacing measurement type
   CComboBox* pcbBackMeasure  = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   CComboBox* pcbAheadMeasure = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   pcbAheadMeasure->SetCurSel(pcbBackMeasure->GetCurSel());

   // Spacing Grid
   m_SpacingGrid[pgsTypes::metStart].SetGirderSpacingData(m_SpacingGrid[pgsTypes::metEnd].GetGirderSpacingData());
   m_SpacingGrid[pgsTypes::metStart].FillGrid();

   // Spacing Location

   // reference girder index
   CComboBox* pcbBackRefGirder  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER);
   CComboBox* pcbAheadRefGirder = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER);
   pcbBackRefGirder->SetCurSel(pcbAheadRefGirder->GetCurSel());

   // reference girder offset
   CWnd* pwndBackRefGirderOffset  = GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET);
   CWnd* pwndAheadRefGirderOffset = GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET);
   CString strText;
   pwndAheadRefGirderOffset->GetWindowText(strText);
   pwndBackRefGirderOffset->SetWindowText(strText);

   // reference girder offset type
   CComboBox* pcbBackRefGirderOffsetType  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE);
   CComboBox* pcbAheadRefGirderOffsetType = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   pcbBackRefGirderOffsetType->SetCurSel(pcbAheadRefGirderOffsetType->GetCurSel());
}