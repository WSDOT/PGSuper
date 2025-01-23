///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PierGirderSpacingPage.h"
#include "PierDetailsDlg.h"
#include "SelectItemDlg.h"
#include "PGSuperColors.h"
#include "Utilities.h"

#include <PGSuperUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\BeamFactory.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT AheadControls[] =
{
   IDC_AHEADGROUP,
   IDC_NUMGDR_SPIN_NEXT_SPAN,
   IDC_NUMGDR_NEXT_SPAN,
   IDC_NUMGDR_NEXT_SPAN_LABEL,
   IDC_NEXT_SPAN_SPACING_LABEL,
   IDC_NEXT_SPAN_SPACING_MEASUREMENT,
   IDC_NEXT_SPAN_SPACING_GRID,
   IDC_AHEAD_COPY,
   IDC_NEXT_REF_GIRDER_LABEL,
   IDC_NEXT_REF_GIRDER,
   IDC_NEXT_REF_GIRDER_OFFSET,
   IDC_NEXT_REF_GIRDER_OFFSET_UNIT,
   IDC_NEXT_REF_GIRDER_FROM,
   IDC_NEXT_REF_GIRDER_OFFSET_TYPE
};

UINT BackControls[] =
{
   IDC_BACKGROUP,
   IDC_NUMGDR_SPIN_PREV_SPAN,
   IDC_NUMGDR_PREV_SPAN,
   IDC_NUMGDR_PREV_SPAN_LABEL,
   IDC_PREV_SPAN_SPACING_LABEL,
   IDC_PREV_SPAN_SPACING_MEASUREMENT,
   IDC_PREV_SPAN_SPACING_GRID,
   IDC_BACK_COPY,
   IDC_PREV_REF_GIRDER_LABEL,
   IDC_PREV_REF_GIRDER,
   IDC_PREV_REF_GIRDER_OFFSET,
   IDC_PREV_REF_GIRDER_OFFSET_UNIT,
   IDC_PREV_REF_GIRDER_FROM,
   IDC_PREV_REF_GIRDER_OFFSET_TYPE
};

/////////////////////////////////////////////////////////////////////////////
// CPierGirderSpacingPage property page

IMPLEMENT_DYNCREATE(CPierGirderSpacingPage, CPropertyPage)

CPierGirderSpacingPage::CPierGirderSpacingPage() : CPropertyPage(CPierGirderSpacingPage::IDD)
{
	//{{AFX_DATA_INIT(CPierGirderSpacingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_MinGirderCount[pgsTypes::Back]  = 2;
   m_MinGirderCount[pgsTypes::Ahead] = 2;
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
      Float64 offset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType back_measure, ahead_measure;
      std::tie(offset,back_measure) = pParent->m_pPier->GetBearingOffset(pgsTypes::Back, true);
      std::tie(offset,ahead_measure) = pParent->m_pPier->GetBearingOffset(pgsTypes::Ahead, true);
      FillGirderSpacingMeasurementComboBox(IDC_PREV_SPAN_SPACING_MEASUREMENT,back_measure);
      FillGirderSpacingMeasurementComboBox(IDC_NEXT_SPAN_SPACING_MEASUREMENT,ahead_measure);
   }


	//{{AFX_DATA_MAP(CPierGirderSpacingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_NUMGDR_SPIN_PREV_SPAN,                  m_NumGdrSpinner[pgsTypes::Back]);
	DDX_Control(pDX, IDC_NUMGDR_SPIN_NEXT_SPAN,                  m_NumGdrSpinner[pgsTypes::Ahead]);
   DDX_Control(pDX, IDC_CB_SPACG_TYPE,                          m_cbGirderSpacingType);
   DDX_Control(pDX, IDC_PREV_SPAN_SPACING_MEASUREMENT,          m_cbGirderSpacingMeasurement[pgsTypes::Back]);
   DDX_Control(pDX, IDC_NEXT_SPAN_SPACING_MEASUREMENT,          m_cbGirderSpacingMeasurement[pgsTypes::Ahead]);
   DDX_Control(pDX, IDC_CB_NUMGDRS,                             m_cbNumGdrType);

   DDX_CBItemData(pDX, IDC_PREV_SPAN_SPACING_MEASUREMENT, m_GirderSpacingMeasure[pgsTypes::Back]);
   DDX_CBItemData(pDX, IDC_NEXT_SPAN_SPACING_MEASUREMENT, m_GirderSpacingMeasure[pgsTypes::Ahead]);

   DDX_Text( pDX, IDC_NUMGDR_PREV_SPAN, m_nGirders[pgsTypes::Back] );
   DDV_MinMaxLongLong(pDX, m_nGirders[pgsTypes::Back], m_MinGirderCount[pgsTypes::Back], MAX_GIRDERS_PER_SPAN );

   DDX_Text( pDX, IDC_NUMGDR_NEXT_SPAN, m_nGirders[pgsTypes::Ahead] );
   DDV_MinMaxLongLong(pDX, m_nGirders[pgsTypes::Ahead], m_MinGirderCount[pgsTypes::Ahead], MAX_GIRDERS_PER_SPAN );

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

   if ( pDX->m_bSaveAndValidate )
   {
      pgsTypes::MeasurementLocation ml;
      pgsTypes::MeasurementType     mt;

      CGirderSpacing2* pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Back);
      UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Back],&ml,&mt);
      pGirderSpacing->SetMeasurementLocation(ml);
      pGirderSpacing->SetMeasurementType(mt);
      pGirderSpacing->SetRefGirder(m_RefGirderIdx[pgsTypes::Back]);
      pGirderSpacing->SetRefGirderOffset(m_RefGirderOffset[pgsTypes::Back]);
      pGirderSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType[pgsTypes::Back]);

      pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Ahead);
      UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Ahead],&ml,&mt);
      pGirderSpacing->SetMeasurementLocation(ml);
      pGirderSpacing->SetMeasurementType(mt);
      pGirderSpacing->SetRefGirder(m_RefGirderIdx[pgsTypes::Ahead]);
      pGirderSpacing->SetRefGirderOffset(m_RefGirderOffset[pgsTypes::Ahead]);
      pGirderSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType[pgsTypes::Ahead]);
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
   ON_CBN_SELCHANGE(IDC_CB_NUMGDRS,OnChangeSameNumberOfGirders)
   ON_CBN_SELCHANGE(IDC_CB_SPACG_TYPE,OnChangeSameGirderSpacing)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierGirderSpacingPage message handlers
void CPierGirderSpacingPage::Init(CPierDetailsDlg* pParent)
{
   CBridgeDescription2* pBridgeDesc = &pParent->m_BridgeDesc;

   m_GirderSpacingMeasurementLocation = pBridgeDesc->GetMeasurementLocation();

   if ( pParent->m_pPier->IsInteriorPier() )
   {
      // Pier is inside of a group
      if ( IsContinuousSegment() )
      {
         // Segments are continuous over this pier, so spacing is not defined here
         m_SpacingTypeMode = None;
      }
      else
      {
         // spacing is defined here, however the spacing is the same on both
         // sides of the pier
         m_SpacingTypeMode = Single;
      }
   }
   else
   {
      // Pier is at the start or end of a group
      // Spacing is defined on both sides of the pier (unless it is the first or last pier in the bridge)
      if ( pParent->m_pSpan[pgsTypes::Back] && pParent->m_pSpan[pgsTypes::Ahead] )
      {
         m_SpacingTypeMode = Both;
      }
      else if ( pParent->m_pSpan[pgsTypes::Back] && !pParent->m_pSpan[pgsTypes::Ahead] )
      {
         m_SpacingTypeMode = Back;
      }
      else if ( !pParent->m_pSpan[pgsTypes::Back] && pParent->m_pSpan[pgsTypes::Ahead] )
      {
         m_SpacingTypeMode = Ahead;
      }
   }

   if ( m_SpacingTypeMode == None )
   {
      InitSpacingBack(pParent,false);
      InitSpacingAhead(pParent,false);
   }
   else if ( m_SpacingTypeMode == Single )
   {
      InitSpacingBack(pParent,true);
      InitSpacingAhead(pParent,false);
   }
   else if ( m_SpacingTypeMode == Ahead )
   {
      InitSpacingBack(pParent,false);
      InitSpacingAhead(pParent,true);
   }
   else if ( m_SpacingTypeMode == Back )
   {
      InitSpacingBack(pParent,true);
      InitSpacingAhead(pParent,false);
   }
   else
   {
      ATLASSERT(m_SpacingTypeMode == Both);
      InitSpacingBack(pParent,true);
      InitSpacingAhead(pParent,true);
   }

   m_nGirdersCache[pgsTypes::Back]  = m_nGirders[pgsTypes::Back];
   m_nGirdersCache[pgsTypes::Ahead] = m_nGirders[pgsTypes::Ahead];

   m_GirderSpacingMeasureCache[pgsTypes::Back]  = m_GirderSpacingMeasure[pgsTypes::Back];
   m_GirderSpacingMeasureCache[pgsTypes::Ahead] = m_GirderSpacingMeasure[pgsTypes::Ahead];
}

BOOL CPierGirderSpacingPage::OnInitDialog() 
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   CPierData2* pPier = pParent->m_pPier;

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   Float64 skew_angle;
   pBridge->GetSkewAngle(pPier->GetStation(),pPier->GetOrientation(),&skew_angle);

   m_GirderSpacingGrid[pgsTypes::Back].SubclassDlgItem(IDC_PREV_SPAN_SPACING_GRID,this);
   m_GirderSpacingGrid[pgsTypes::Ahead].SubclassDlgItem(IDC_NEXT_SPAN_SPACING_GRID,this);

   if ( m_SpacingTypeMode == None )
   {
      // Hide all controls
      HideBackGroup();
      HideAheadGroup();

      // Show the group box for "Back side of Pier"
      // Change the label
      CWnd* pWnd = GetDlgItem(IDC_BACKGROUP);
      pWnd->ShowWindow(SW_SHOW);
      CString strTxt;
      strTxt.Format(_T("Spacing at %s Line"),IsAbutment() ? _T("Abutment") : _T("Pier"));
      pWnd->SetWindowText(strTxt);
      
      // Get the rect for the group box and deflate it... use this rect for the no spacing note
      CRect rect;
      pWnd->GetWindowRect(&rect);
      rect.DeflateRect(10,10);
      rect.OffsetRect(5,5);
      ScreenToClient(rect);


      // create a static control with note defining why girder spacing is not defined here
      m_NoSpacingNote.Create(_T("Segments are continuous over this support. Spacing is defined at segment ends"),
                             WS_CHILD | WS_VISIBLE | SS_CENTER, rect, this);

      m_NoSpacingNote.SetFont(pWnd->GetFont());
   }
   else if ( m_SpacingTypeMode == Single )
   {
      HideAheadGroup();

      // Show the group box for "Back side of Pier"
      // Change the label
      CWnd* pWnd = GetDlgItem(IDC_BACKGROUP);
      pWnd->ShowWindow(SW_SHOW);
      pWnd->SetWindowText(_T("Spacing at centerline of pier"));

      // Don't need the copy button
      CWnd* pCopy = GetDlgItem(IDC_BACK_COPY);
      pCopy->ShowWindow(SW_HIDE);

      m_nGirders[pgsTypes::Ahead] = 2;
      m_GirderSpacingGrid[pgsTypes::Ahead].Initialize();
      m_GirderSpacingMeasure[pgsTypes::Ahead] = 0;
   }
   else if ( m_SpacingTypeMode == Ahead )
   {
      HideBackGroup();
      MoveAheadGroup();
   
      CString strTxt;
      strTxt.Format(_T("Ahead side of %s = Start of Span %s"), LABEL_PIER_EX(IsAbutment(),pParent->m_pPier->GetIndex()), LABEL_SPAN(pParent->m_pSpan[pgsTypes::Ahead]->GetIndex()));
      GetDlgItem(IDC_AHEADGROUP)->SetWindowText(strTxt);

      // Don't need the copy button
      CWnd* pCopy = GetDlgItem(IDC_AHEAD_COPY);
      pCopy->ShowWindow(SW_HIDE);
   }
   else if ( m_SpacingTypeMode == Back )
   {
      HideAheadGroup();

      CString strTxt;
      strTxt.Format(_T("Back side of %s = End of Span %s"), LABEL_PIER_EX(IsAbutment(), pParent->m_pPier->GetIndex()),LABEL_SPAN(pParent->m_pSpan[pgsTypes::Back]->GetIndex()));
      GetDlgItem(IDC_BACKGROUP)->SetWindowText(strTxt);

      // Don't need the copy button
      CWnd* pCopy = GetDlgItem(IDC_BACK_COPY);
      pCopy->ShowWindow(SW_HIDE);

      m_nGirders[pgsTypes::Ahead] = 2;
      m_GirderSpacingGrid[pgsTypes::Ahead].Initialize();
      m_GirderSpacingMeasure[pgsTypes::Ahead] = 0;
   }
   else
   {
      ATLASSERT(m_SpacingTypeMode == Both);

      CString strTxt;
      strTxt.Format(_T("Back side of %s = End of Span %s"),LABEL_PIER_EX(IsAbutment(), pParent->m_pPier->GetIndex()),LABEL_SPAN(pParent->m_pSpan[pgsTypes::Back]->GetIndex()));
      GetDlgItem(IDC_BACKGROUP)->SetWindowText(strTxt);

      strTxt.Format(_T("Ahead side of %s = Start of Span %s"),LABEL_PIER_EX(IsAbutment(),pParent->m_pPier->GetIndex()),LABEL_SPAN(pParent->m_pSpan[pgsTypes::Ahead]->GetIndex()));
      GetDlgItem(IDC_AHEADGROUP)->SetWindowText(strTxt);
   }

   // Initialize grids... use Initialize() if grid is not is use
   // otherwise use CustomInit()
   if ( m_SpacingTypeMode == None )
   {
      m_GirderSpacingGrid[pgsTypes::Back].Initialize();
      m_GirderSpacingGrid[pgsTypes::Ahead].Initialize();
   }
   else if ( m_SpacingTypeMode == Single || m_SpacingTypeMode == Back )
   {
      m_GirderSpacingGrid[pgsTypes::Back].CustomInit();
      m_GirderSpacingGrid[pgsTypes::Back].SetPierSkewAngle(skew_angle);
      m_GirderSpacingGrid[pgsTypes::Ahead].Initialize();
   }
   else if ( m_SpacingTypeMode == Ahead )
   {
      m_GirderSpacingGrid[pgsTypes::Back].Initialize();
      m_GirderSpacingGrid[pgsTypes::Ahead].CustomInit();
      m_GirderSpacingGrid[pgsTypes::Ahead].SetPierSkewAngle(skew_angle);
   }
   else
   {
      ATLASSERT(m_SpacingTypeMode == Both);
      m_GirderSpacingGrid[pgsTypes::Back].CustomInit();
      m_GirderSpacingGrid[pgsTypes::Back].SetPierSkewAngle(skew_angle);

      m_GirderSpacingGrid[pgsTypes::Ahead].CustomInit();
      m_GirderSpacingGrid[pgsTypes::Ahead].SetPierSkewAngle(skew_angle);
   }

   FillRefGirderComboBox(pgsTypes::Back);
   FillRefGirderComboBox(pgsTypes::Ahead);
   FillRefGirderOffsetTypeComboBox(pgsTypes::Back);
   FillRefGirderOffsetTypeComboBox(pgsTypes::Ahead);

   
   CPropertyPage::OnInitDialog();

   bool bUseSameGirderType = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups();

   m_cbNumGdrType.AddString(_T("The same number of girders are used in all spans"));
   m_cbNumGdrType.AddString(_T("The number of girders is defined span by span"));
   m_cbNumGdrType.SetCurSel(bUseSameGirderType?0:1);

   // Initialize the spinner control for # of girder lines
   m_MinGirderCount[pgsTypes::Back]  = GetMinGirderCount(pgsTypes::Back);
   m_MinGirderCount[pgsTypes::Ahead] = GetMinGirderCount(pgsTypes::Ahead);
   m_NumGdrSpinner[pgsTypes::Back].SetRange( short(m_MinGirderCount[pgsTypes::Back]), MAX_GIRDERS_PER_SPAN);
   m_NumGdrSpinner[pgsTypes::Ahead].SetRange(short(m_MinGirderCount[pgsTypes::Ahead]),MAX_GIRDERS_PER_SPAN);

   // restrict the spinner to only go one girder at a time.
   UDACCEL accel;
   accel.nInc = 1;
   accel.nSec = 0;
   m_NumGdrSpinner[pgsTypes::Back].SetAccel(0,&accel);
   m_NumGdrSpinner[pgsTypes::Ahead].SetAccel(0,&accel);
	
   pgsTypes::SupportedBeamSpacing spacingType = pParent->m_BridgeDesc.GetGirderSpacingType();

   if ( spacingType == pgsTypes::sbsUniform )
   {
      m_cbGirderSpacingType.AddString(_T("The same girder spacing is used in all spans"));
      m_cbGirderSpacingType.AddString(_T("Girder spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(0);
   }
   else if ( spacingType == pgsTypes::sbsUniformAdjacent )
   {
      m_cbGirderSpacingType.AddString(_T("The same joint spacing is used in all spans"));
      m_cbGirderSpacingType.AddString(_T("Joint spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(0);
   }
   else if ( spacingType == pgsTypes::sbsConstantAdjacent )
   {
      CString note;
      note.Format(_T("The same girder spacing must be used for the entire bridge for %s girders"), pParent->m_pPier->GetBridgeDescription()->GetGirderFamilyName());
      m_cbGirderSpacingType.AddString(note);
      m_cbGirderSpacingType.SetCurSel(0);
      m_cbGirderSpacingType.EnableWindow(FALSE);
   }
   else if ( spacingType == pgsTypes::sbsGeneral )
   {
      m_cbGirderSpacingType.AddString(_T("The same girder spacing is used in all spans"));
      m_cbGirderSpacingType.AddString(_T("Girder spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(1);
   }
   else if ( spacingType == pgsTypes::sbsGeneralAdjacent )
   {
      m_cbGirderSpacingType.AddString(_T("The same joint spacing is used in all spans"));
      m_cbGirderSpacingType.AddString(_T("Joint spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(1);
   }
   else if (spacingType == pgsTypes::sbsUniformAdjacentWithTopWidth || spacingType == pgsTypes::sbsGeneralAdjacentWithTopWidth)
   {
      m_cbGirderSpacingType.AddString(_T("The same top flange width and joint spacing is used in all spans"));
      m_cbGirderSpacingType.AddString(_T("Top flange width and joint spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(spacingType == pgsTypes::sbsUniformAdjacentWithTopWidth ? 0 : 1);
   }
   else
   {
      ATLASSERT(false); // is there a new spacing type???
   }

   if ( IsGirderSpacing(spacingType) )
   {
      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
   }
   else
   {
      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CPierGirderSpacingPage::FillGirderSpacingMeasurementComboBox(int nIDC, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure)
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(nIDC);
   pSpacingType->ResetContent();

   int idx = pSpacingType->AddString(IsAbutment() ? _T("Measured at and along abutment line") : _T("Measured at and along pier line"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   
   idx = pSpacingType->AddString(IsAbutment() ? _T("Measured normal to alignment at abutment line") : _T("Measured normal to alignment at pier line"));
   item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem);
   pSpacingType->SetItemData(idx,item_data);

   // if the bearing offset is measured along the CL girder, the girder spacing cannot be measured at the CL bearing
   // this is because there would not be a unique CL bearing line common to all girders if the girders are not parallel to one another
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
   pCB->SetItemData(idx,INVALID_INDEX);

   for ( GirderIndexType i = 0; i < m_nGirders[pierFace]; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD)i);
   }

   if ( pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
   {
      pCB->SetCurSel(0);
   }
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

   // update the girder spacing cache
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   m_GirderSpacingCache[pierFace] = *(pParent->m_pPier->GetGirderSpacing(pierFace));
   FillRefGirderComboBox(pierFace);

   UpdateGirderSpacingState(pierFace);
   UpdateCopyButtonState(m_nGirders[pgsTypes::Ahead] == m_nGirders[pgsTypes::Back]);

   *pResult = 0;
}

void CPierGirderSpacingPage::AddGirders(GirderIndexType nGirders,pgsTypes::PierFaceType pierFace)
{
   m_nGirders[pierFace] += nGirders;

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   pParent->m_pPier->GetGirderGroup(pierFace)->AddGirders(nGirders);

   m_GirderSpacingGrid[pierFace].UpdateGrid();
}

void CPierGirderSpacingPage::RemoveGirders(GirderIndexType nGirders,pgsTypes::PierFaceType pierFace)
{
   m_nGirders[pierFace] -= nGirders;

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   pParent->m_pPier->GetGirderGroup(pierFace)->RemoveGirders(nGirders);

   m_GirderSpacingGrid[pierFace].UpdateGrid();
}

BOOL CPierGirderSpacingPage::OnSetActive() 
{
   UpdateChildWindowState();
	BOOL bResult = CPropertyPage::OnSetActive();

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
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   BOOL bEnable = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups() ? FALSE : TRUE;
   GetDlgItem(IDC_NUMGDR_PREV_SPAN_LABEL)->EnableWindow(        bEnable );
   GetDlgItem(IDC_NUMGDR_PREV_SPAN)->EnableWindow(              bEnable );
   GetDlgItem(IDC_NUMGDR_SPIN_PREV_SPAN)->EnableWindow(         bEnable );

   GetDlgItem(IDC_NUMGDR_NEXT_SPAN_LABEL)->EnableWindow(        bEnable );
   GetDlgItem(IDC_NUMGDR_NEXT_SPAN)->EnableWindow(              bEnable );
   GetDlgItem(IDC_NUMGDR_SPIN_NEXT_SPAN)->EnableWindow(         bEnable );

   UpdateGirderSpacingText();

   UpdateGirderSpacingState(pgsTypes::Back);
   UpdateGirderSpacingState(pgsTypes::Ahead);
   UpdateCopyButtonState(m_NumGdrSpinner[pgsTypes::Back].GetPos() == m_NumGdrSpinner[pgsTypes::Ahead].GetPos());
}

void CPierGirderSpacingPage::OnCopyToAheadSide() 
{
   CComboBox* pBackMeasure = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
   int curSel = pBackMeasure->GetCurSel();

   CComboBox* pAheadMeasure = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
   pAheadMeasure->SetCurSel(curSel);

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   CGirderSpacing2* pSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Back);
   pParent->m_pPier->SetGirderSpacing(pgsTypes::Ahead,*pSpacing);
   m_GirderSpacingGrid[pgsTypes::Ahead].UpdateGrid();

   ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER))->GetCurSel() );
   ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE))->GetCurSel() );

   CString strWndTxt;
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->GetWindowText( strWndTxt );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->SetWindowText( strWndTxt );
}

void CPierGirderSpacingPage::OnCopyToBackSide() 
{
   CComboBox* pAheadMeasure = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
   int curSel = pAheadMeasure->GetCurSel();

   CComboBox* pBackMeasure = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
   pBackMeasure->SetCurSel(curSel);

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   CGirderSpacing2* pSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Ahead);
   pParent->m_pPier->SetGirderSpacing(pgsTypes::Back,*pSpacing);
   m_GirderSpacingGrid[pgsTypes::Back].UpdateGrid();

   ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER))->GetCurSel() );
   ((CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE))->SetCurSel( ((CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE))->GetCurSel() );

   CString strWndTxt;
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->GetWindowText( strWndTxt );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->SetWindowText( strWndTxt );
}

GirderIndexType CPierGirderSpacingPage::GetMinGirderCount(pgsTypes::PierFaceType pierFace)
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   CGirderGroupData* pGirderGroup = pParent->m_pPier->GetGirderGroup(pierFace);

   if ( pGirderGroup == nullptr )
   {
      return 2;
   }

   // It is ok to use girder 0 here because all the girders within the span
   // are of the same family. All the girders in the span will have the
   // same factory
   const GirderLibraryEntry* pGdrEntry = pGirderGroup->GetGirderLibraryEntry(0);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   return factory->GetMinimumBeamCount();
}

void CPierGirderSpacingPage::UpdateGirderSpacingState(pgsTypes::PierFaceType pierFace)
{
   BOOL bEnable = m_GirderSpacingGrid[pierFace].InputSpacing();

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   if ( m_nGirders[pierFace] == 1 || IsBridgeSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) )
   {
      // if there is only 1 girder or we are input the spacing for the whole bridge
      // (not span by span) then disable the input controls
      bEnable = FALSE;
   }

   m_cbGirderSpacingMeasurement[pierFace].EnableWindow(bEnable);
   m_GirderSpacingGrid[pierFace].Enable(bEnable);

   if ( IsBridgeSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) )
      bEnable = FALSE;
   else
      bEnable = TRUE;

   if ( pierFace == pgsTypes::Back )
   {
      GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->EnableWindow(       bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->EnableWindow(         bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(               bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->EnableWindow(   bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->EnableWindow(          bEnable );
      GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );
   }
   else
   {
      GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->EnableWindow(       bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->EnableWindow(         bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(               bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->EnableWindow(   bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->EnableWindow(          bEnable );
      GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );
   }
}

void CPierGirderSpacingPage::UpdateCopyButtonState(BOOL bEnable)
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   if ( IsBridgeSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) )
   {
      bEnable = FALSE; // nothing to copy if same girder spacing for entire bridge
   }

   GetDlgItem(IDC_BACK_COPY)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_COPY)->EnableWindow(bEnable);
}

void CPierGirderSpacingPage::OnChangeSameNumberOfGirders()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   bool bUseSameNumGirders = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups();
   // Toggle
   bUseSameNumGirders = !bUseSameNumGirders;

   if ( bUseSameNumGirders )
   {
      GirderIndexType nGirders = (pParent->m_pSpan[pgsTypes::Back] ? m_nGirders[pgsTypes::Back] : m_nGirders[pgsTypes::Ahead]);
      if ( pParent->m_pSpan[pgsTypes::Back] && pParent->m_pSpan[pgsTypes::Ahead] && m_nGirders[pgsTypes::Back] != m_nGirders[pgsTypes::Ahead] )
      {
         // There are spans on both sides of this pier and there is a different number of girders
         // in those spans. Ask the user which number of girders should be used for the entire bridge
         CSelectItemDlg dlg;
         dlg.m_ItemIdx = 0;
         dlg.m_strTitle = _T("Select Number of Girders");
         dlg.m_strLabel = _T("A single number of girders will be used for the entire bridge. Select a value.");
         
         CString strItems;
         strItems.Format(_T("%d\n%d"),m_nGirders[pgsTypes::Back],m_nGirders[pgsTypes::Ahead]);

         dlg.m_strItems = strItems;
         if ( dlg.DoModal() == IDOK )
         {
            if ( dlg.m_ItemIdx == 0 )
               nGirders = m_nGirders[pgsTypes::Back];
            else
               nGirders = m_nGirders[pgsTypes::Ahead];
         }
         else
         {
            // User pressed Cancel button... leave now without making any changes
            return;
         }
      }
      
      pParent->m_BridgeDesc.SetGirderCount(nGirders);
      m_nGirders[pgsTypes::Back]  = nGirders;
      m_nGirders[pgsTypes::Ahead] = nGirders;
      CDataExchange dx(this,FALSE);
      DDX_Text( &dx, IDC_NUMGDR_PREV_SPAN, m_nGirders[pgsTypes::Back] );
      DDX_Text( &dx, IDC_NUMGDR_NEXT_SPAN, m_nGirders[pgsTypes::Ahead] );
   }

   pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups(bUseSameNumGirders);

   m_GirderSpacingGrid[pgsTypes::Back].UpdateGrid();
   m_GirderSpacingGrid[pgsTypes::Ahead].UpdateGrid();

   UpdateChildWindowState();
}

void CPierGirderSpacingPage::OnChangeSameGirderSpacing()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // changing from uniform to general, or general to uniform spacing
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   pgsTypes::SupportedBeamSpacing oldSpacingType = pParent->m_BridgeDesc.GetGirderSpacingType();
   pgsTypes::SupportedBeamSpacing spacingType = ToggleGirderSpacingType(oldSpacingType);

   long backGirderSpacingDatum, aheadGirderSpacingDatum;

   pgsTypes::PierFaceType pierFace = pParent->m_pSpan[pgsTypes::Ahead] ? pgsTypes::Ahead : pgsTypes::Back;
   if (IsBridgeSpacing(spacingType))
   {
      // we are going from general to uniform spacing
      // if the grid has more than one spacing, we need to ask the user which one is to be
      // used for the entire bridge

      // cache the current settings before we change
      if ( pParent->m_pSpan[pgsTypes::Back] )
      {
         CGirderSpacing2* pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Back);
         m_GirderSpacingCache[pgsTypes::Back] = *pGirderSpacing;

         CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
         m_GirderSpacingMeasureCache[pgsTypes::Back]  = (long)pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );
      }

      if ( pParent->m_pSpan[pgsTypes::Ahead] )
      {
         CGirderSpacing2* pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Ahead);
         m_GirderSpacingCache[pgsTypes::Ahead] = *pGirderSpacing;

         CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
         m_GirderSpacingMeasureCache[pgsTypes::Ahead] = (long)pcbStartOfSpanSpacingDatum->GetItemData( pcbStartOfSpanSpacingDatum->GetCurSel() );
      }

      // determine if there is more than one spacing group
      CGirderSpacing2* pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pierFace);
      GroupIndexType nSpacingGroups = pGirderSpacing->GetSpacingGroupCount();
      Float64 bridgeSpacing = 0;
      if ( 1 < nSpacingGroups )
      {
         // there is more than one group... get all the unique spacing values
         std::set<Float64> spacings;
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            Float64 space;
            pGirderSpacing->GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
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
            dlg.m_strLabel = _T("Select the spacing to be used for the entire bridge");
            dlg.m_strTitle = _T("Select spacing");
            dlg.m_ItemIdx = 0;

            CString strItems;
            std::set<Float64>::iterator iter;
            for ( iter = spacings.begin(); iter != spacings.end(); iter++ )
            {
               Float64 spacing = *iter;

               CString strItem;
               if (IsGirderSpacing(oldSpacingType))
               {
                  strItem.Format(_T("%s"), FormatDimension(spacing, pDisplayUnits->GetXSectionDimUnit(), true));
               }
               else
               {
                  strItem.Format(_T("%s"), FormatDimension(spacing, pDisplayUnits->GetComponentDimUnit(), true));
               }

               if (iter != spacings.begin())
               {
                  strItems += _T("\n");
               }

               strItems += strItem;
            }

            dlg.m_strItems = strItems;
            if ( dlg.DoModal() == IDOK )
            {
               iter = spacings.begin();
               for (IndexType i = 0; i < dlg.m_ItemIdx; i++)
               {
                  iter++;
               }

               bridgeSpacing = *iter;
            }
            else
            {
               return;
            }
         }
         else
         {
            // there is only one unique spacing value.. get it
            bridgeSpacing = pGirderSpacing->GetGirderSpacing(0);
         }
      }
      else
      {
         // there is only one unique spacing value.. get it
         bridgeSpacing = pGirderSpacing->GetGirderSpacing(0);
      }

      pParent->m_BridgeDesc.SetGirderSpacing(bridgeSpacing);

      backGirderSpacingDatum  = m_GirderSpacingMeasureCache[pierFace];
      aheadGirderSpacingDatum = m_GirderSpacingMeasureCache[pierFace];
   }
   else
   {
      // restore the girder spacing from the cached values
      if ( pParent->m_pSpan[pgsTypes::Back] )
      {
         pParent->m_pPier->SetGirderSpacing(pgsTypes::Back,m_GirderSpacingCache[pgsTypes::Back]);
         backGirderSpacingDatum = m_GirderSpacingMeasureCache[pgsTypes::Back];
      }

      if ( pParent->m_pSpan[pgsTypes::Ahead] )
      {
         pParent->m_pPier->SetGirderSpacing(pgsTypes::Ahead,m_GirderSpacingCache[pgsTypes::Ahead]);
         aheadGirderSpacingDatum = m_GirderSpacingMeasureCache[pgsTypes::Ahead];
      }
   }

   if ( pParent->m_pSpan[pgsTypes::Back] )
   {
      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_SPAN_SPACING_MEASUREMENT, backGirderSpacingDatum);
   }

   if ( pParent->m_pSpan[pgsTypes::Ahead] )
   {
      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_NEXT_SPAN_SPACING_MEASUREMENT, aheadGirderSpacingDatum);
   }

   pParent->m_BridgeDesc.SetGirderSpacingType(spacingType);
   m_GirderSpacingGrid[pgsTypes::Ahead].UpdateGrid();
   m_GirderSpacingGrid[pgsTypes::Back].UpdateGrid();

   UpdateChildWindowState();
   return;
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
   UnhashGirderSpacing(pCB->GetItemData(cursel),&ml,&mt);

   m_GirderSpacingGrid[pierFace].UpdateGrid();
}

void CPierGirderSpacingPage::UpdateGirderSpacingText()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   bool bInputSpacing[2];
   bInputSpacing[pgsTypes::Ahead] = m_GirderSpacingGrid[pgsTypes::Ahead].InputSpacing();
   bInputSpacing[pgsTypes::Back]  = m_GirderSpacingGrid[pgsTypes::Back].InputSpacing();

   if ( pParent->m_pSpan[pgsTypes::Back] == nullptr )
      bInputSpacing[pgsTypes::Back] = true;

   if ( pParent->m_pSpan[pgsTypes::Ahead] == nullptr )
      bInputSpacing[pgsTypes::Ahead] = true;

   BOOL bEnable = TRUE;
   if ( ::IsBridgeSpacing( pParent->m_BridgeDesc.GetGirderSpacingType() ) )
   {
      bEnable = FALSE;
   }

   GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->EnableWindow(         bEnable );
   GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(               bEnable );
   GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->EnableWindow(          bEnable );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->EnableWindow(   bEnable );
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );

   GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->EnableWindow(         bEnable );
   GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(               bEnable );
   GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->EnableWindow(          bEnable );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->EnableWindow(   bEnable );
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );
}

void CPierGirderSpacingPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PIERDETAILS_GIRDERSPACING );
}

bool CPierGirderSpacingPage::IsAbutment()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   return pParent->m_pPier->IsAbutment();
}

bool CPierGirderSpacingPage::IsContinuousSegment()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   if ( pParent->m_pPier->IsInteriorPier() )
      return false; // segments are not continuous (as far as spacing is concerned)
   else
      return pParent->m_pPier->IsContinuousConnection() ? true : false;
}

void CPierGirderSpacingPage::HideBackGroup(bool bHide)
{
   HideGroup(BackControls,sizeof(BackControls)/sizeof(UINT),bHide);
}

void CPierGirderSpacingPage::HideAheadGroup(bool bHide)
{
   HideGroup(AheadControls,sizeof(AheadControls)/sizeof(UINT),bHide);
}

void CPierGirderSpacingPage::HideGroup(UINT* nIDs,UINT nControls,bool bHide)
{
   for (UINT i = 0; i < nControls; i++ )
   {
      UINT nID = nIDs[i];
      CWnd* pWnd = GetDlgItem(nID);
      pWnd->ShowWindow(bHide ? SW_HIDE : SW_SHOW);
   }
}

void CPierGirderSpacingPage::MoveAheadGroup()
{
   // Moves the ahead group to the top of the dialog
   CWnd* pBackGroup = GetDlgItem(IDC_BACKGROUP);
   CRect backGroupRect;
   pBackGroup->GetWindowRect(&backGroupRect);
   CPoint backTopLeft(backGroupRect.TopLeft());

   CWnd* pAheadGroup = GetDlgItem(IDC_AHEADGROUP);
   CRect aheadGroupRect;
   pAheadGroup->GetWindowRect(&aheadGroupRect);
   CPoint aheadTopLeft(aheadGroupRect.TopLeft());

   CPoint offset;
   offset.x = backTopLeft.x - aheadTopLeft.x;
   offset.y = backTopLeft.y - aheadTopLeft.y;

   UINT nControls = sizeof(AheadControls)/sizeof(UINT);
   for (UINT i = 0; i < nControls; i++ )
   {
      UINT nID = AheadControls[i];
      CWnd* pWnd = GetDlgItem(nID);
      CRect rect;
      pWnd->GetWindowRect(&rect);
      rect.OffsetRect(offset);
      ScreenToClient(&rect);
      pWnd->MoveWindow(rect);
   }
}

void CPierGirderSpacingPage::InitSpacingBack(CPierDetailsDlg* pParent,bool bUse)
{
   if ( bUse )
   {
      // use a dummy skew angle for basic initialization... set it to the correct value OnInitialDialog
      Float64 skew_angle = 0;

      CGirderSpacing2* pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Back);
      m_GirderSpacingCache[pgsTypes::Back] = *pGirderSpacing;

      m_nGirders[pgsTypes::Back]             = pParent->m_pPier->GetPrevGirderGroup()->GetGirderCount();
      pgsTypes::MeasurementLocation ml       = pGirderSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mt       = pGirderSpacing->GetMeasurementType();
      m_GirderSpacingMeasure[pgsTypes::Back] = HashGirderSpacing(ml,mt);

      m_GirderSpacingGrid[pgsTypes::Back].InitializeGridData( pGirderSpacing,
                                                              pParent->m_pPier->GetPrevGirderGroup(),
                                                              pgsTypes::Back,
                                                              pParent->m_pPier->GetIndex(),
                                                              skew_angle,
                                                              pParent->m_pSpan[pgsTypes::Ahead] == nullptr ? true : false,
                                                              pParent->m_pPier->GetBridgeDescription()->GetDeckDescription()->GetDeckType());

      m_RefGirderIdx[pgsTypes::Back]        = pGirderSpacing->GetRefGirder();
      m_RefGirderOffset[pgsTypes::Back]     = pGirderSpacing->GetRefGirderOffset();
      m_RefGirderOffsetType[pgsTypes::Back] = pGirderSpacing->GetRefGirderOffsetType();
   }
   else
   {
      m_nGirders[pgsTypes::Back] = 2;
      m_GirderSpacingMeasure[pgsTypes::Back] = 0;

      m_RefGirderIdx[pgsTypes::Back]        = INVALID_INDEX;
      m_RefGirderOffset[pgsTypes::Back]     = 0;
      m_RefGirderOffsetType[pgsTypes::Back] = pgsTypes::omtBridge;
   }
}

void CPierGirderSpacingPage::InitSpacingAhead(CPierDetailsDlg* pParent,bool bUse)
{
   if ( bUse )
   {
      // use a dummy skew angle for basic initialization... set it to the correct value OnInitialDialog
      Float64 skew_angle = 0;

      CGirderSpacing2* pGirderSpacing = pParent->m_pPier->GetGirderSpacing(pgsTypes::Ahead);
      m_GirderSpacingCache[pgsTypes::Ahead] = *pGirderSpacing;

      m_nGirders[pgsTypes::Ahead]             = pParent->m_pPier->GetNextGirderGroup()->GetGirderCount();
      pgsTypes::MeasurementLocation ml        = pGirderSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mt        = pGirderSpacing->GetMeasurementType();
      m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(ml,mt);

      m_GirderSpacingGrid[pgsTypes::Ahead].InitializeGridData( pGirderSpacing,
                                                               pParent->m_pPier->GetNextGirderGroup(),
                                                               pgsTypes::Ahead,
                                                               pParent->m_pPier->GetIndex(),
                                                               skew_angle,
                                                               pParent->m_pSpan[pgsTypes::Back] == nullptr ? true : false,
                                                               pParent->m_pPier->GetBridgeDescription()->GetDeckDescription()->GetDeckType());

      m_RefGirderIdx[pgsTypes::Ahead]        = pGirderSpacing->GetRefGirder();
      m_RefGirderOffset[pgsTypes::Ahead]     = pGirderSpacing->GetRefGirderOffset();
      m_RefGirderOffsetType[pgsTypes::Ahead] = pGirderSpacing->GetRefGirderOffsetType();
   }
   else 
   {
      m_nGirders[pgsTypes::Ahead] = 2;
      m_GirderSpacingMeasure[pgsTypes::Ahead] = 0;

      m_RefGirderIdx[pgsTypes::Ahead]        = INVALID_INDEX;
      m_RefGirderOffset[pgsTypes::Ahead]     = 0;
      m_RefGirderOffsetType[pgsTypes::Ahead] = pgsTypes::omtBridge;
   }
}
