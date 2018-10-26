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

// GirderSegmentSpacingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "GirderSegmentSpacingPage.h"
#include "TemporarySupportDlg.h"
#include "PierDetailsDlg.h"
#include "SelectItemDlg.h"
#include "PGSuperColors.h"
#include "HtmlHelp\HelpTopics.hh"

#include <PGSuperUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentSpacingPage property page

IMPLEMENT_DYNCREATE(CGirderSegmentSpacingPage, CPropertyPage)

CGirderSegmentSpacingPage::CGirderSegmentSpacingPage() : CPropertyPage(CGirderSegmentSpacingPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderSegmentSpacingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderSegmentSpacingPage::~CGirderSegmentSpacingPage()
{
}

void CGirderSegmentSpacingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CGirderSegmentSpacingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
   DDX_Control(pDX, IDC_SEGMENT_SPACING_NOTE,   m_GirderSpacingHyperLink);
   DDX_Control(pDX, IDC_SPACING_MEASUREMENT,    m_cbGirderSpacingMeasurement);

   DDX_CBItemData(pDX, IDC_SPACING_MEASUREMENT, m_GirderSpacingMeasure);

   DDV_SpacingGrid(pDX,IDC_SPACING_GRID,&m_SpacingGrid);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_CBItemData(pDX, IDC_REF_GIRDER, m_RefGirderIdx);
   DDX_OffsetAndTag(pDX, IDC_REF_GIRDER_OFFSET,IDC_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset, pDisplayUnits->GetXSectionDimUnit());
   DDX_CBItemData(pDX, IDC_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType);
}


BEGIN_MESSAGE_MAP(CGirderSegmentSpacingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentSpacingPage)
	ON_WM_CTLCOLOR()
   ON_CBN_SELCHANGE(IDC_SPACING_MEASUREMENT,OnSpacingDatumChanged)
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderSpacing,OnChangeSameGirderSpacing)
   //ON_REGISTERED_MESSAGE(MsgChangeSlabOffset,OnChangeSlabOffset)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentSpacingPage message handlers
void CGirderSegmentSpacingPage::Init()
{
   const CGirderSpacing2& spacing = GetGirderSpacing();

   // Girder spacing
   SetGirderSpacingType(GetBridgeDescription().GetGirderSpacingType());
   SetGirderSpacingMeasurementLocation( GetBridgeDescription().GetMeasurementLocation() );

   m_GirderSpacingMeasure = HashGirderSpacing( spacing.GetMeasurementLocation(), spacing.GetMeasurementType() );
   m_GirderSpacingMeasureCache = m_GirderSpacingMeasure;

   // Girder Spacing Datum
   m_RefGirderIdx        = spacing.GetRefGirder();
   m_RefGirderOffset     = spacing.GetRefGirderOffset();
   m_RefGirderOffsetType = spacing.GetRefGirderOffsetType();
}

BOOL CGirderSegmentSpacingPage::OnInitDialog() 
{
   Init();

   FillGirderSpacingMeasurementComboBox();

   m_SpacingGrid.SubclassDlgItem(IDC_SPACING_GRID,this);
   m_SpacingGrid.CustomInit();

   m_SpacingCache = m_SpacingGrid.GetSpacingData();
	
   GetDlgItem(IDC_SPACING_LABEL)->SetWindowText(IsGirderSpacing(GetGirderSpacingType()) ? _T("Girder Spacing") : _T("Joint Spacing"));

   FillRefGirderComboBox();
   FillRefGirderOffsetTypeComboBox();

   SetGroupTitle();
   
   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CGirderSegmentSpacingPage::FillGirderSpacingMeasurementComboBox()
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(IDC_SPACING_MEASUREMENT);
   pSpacingType->ResetContent();

   int idx = pSpacingType->AddString(_T("Measured at and along the CL temporary support"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   
   idx = pSpacingType->AddString(_T("Measured normal to alignment at CL temporary support"));
   item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem);
   pSpacingType->SetItemData(idx,item_data);
}

void CGirderSegmentSpacingPage::FillRefGirderOffsetTypeComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE);
   int idx = pCB->AddString(_T("Alignment"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtAlignment);

   idx = pCB->AddString(_T("Bridge Line"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtBridge);
}

void CGirderSegmentSpacingPage::FillRefGirderComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REF_GIRDER);
   int curSel = pCB->GetCurSel();
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Center of Girders"));
   pCB->SetItemData(idx,INVALID_INDEX);

   GirderIndexType nGirders = GetBridgeDescription().GetGirderCount();
   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD)i);
   }

   pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel);
}

HBRUSH CGirderSegmentSpacingPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);

   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_SEGMENT_SPACING_NOTE:
      pDC->SetTextColor(HYPERLINK_COLOR);
      break;
   };

   return hbr;
}

BOOL CGirderSegmentSpacingPage::OnSetActive() 
{
   UpdateChildWindowState();
	BOOL bResult = CPropertyPage::OnSetActive();

   // if the connection type is continuous segment, the spacing is not defined at this
   // support (spacing is only defined at the ends of segments)
   int show = (IsContinuousSegment() ? SW_HIDE : SW_SHOW);
   m_GirderSpacingHyperLink.ShowWindow(show);
   m_cbGirderSpacingMeasurement.ShowWindow(show);
   m_SpacingGrid.ShowWindow(show);

   GetDlgItem(IDC_SPACING_LABEL)->ShowWindow(show);
   GetDlgItem(IDC_REF_GIRDER_LABEL)->ShowWindow(show);
   GetDlgItem(IDC_REF_GIRDER)->ShowWindow(show);
   GetDlgItem(IDC_REF_GIRDER_OFFSET)->ShowWindow(show);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_UNIT)->ShowWindow(show);
   GetDlgItem(IDC_REF_GIRDER_FROM)->ShowWindow(show);
   GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE)->ShowWindow(show);

   GetDlgItem(IDC_NO_SPACING_NOTE)->ShowWindow(IsContinuousSegment() ? SW_SHOW : SW_HIDE);

   return bResult;
}


void CGirderSegmentSpacingPage::DisableAll()
{
   CWnd* pWnd = GetTopWindow();
   while ( pWnd  )
   {
      pWnd->EnableWindow(FALSE);
      pWnd = pWnd->GetNextWindow();
   }

   m_SpacingGrid.Enable(FALSE);
}

void CGirderSegmentSpacingPage::UpdateChildWindowState()
{
#pragma Reminder("UPDATE: remove obsolete code")
   //// Generally enable/disable everything if there is a prev/next span
   //// The enable/disable will be refined later in this function

   //// Prev Span
   //BOOL bEnable = (m_pPrevSpan ? (m_bUseSameNumGirders ? FALSE : TRUE) : FALSE);
   //GetDlgItem(IDC_NUMGDR_PREV_SPAN_LABEL)->EnableWindow(        bEnable );
   //GetDlgItem(IDC_NUMGDR_PREV_SPAN)->EnableWindow(              bEnable );
   //GetDlgItem(IDC_NUMGDR_SPIN_PREV_SPAN)->EnableWindow(         bEnable );
   //GetDlgItem(IDC_PREV_SPAN_SPACING_LABEL)->EnableWindow(       m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT)->EnableWindow( m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_SPAN_SPACING_GRID)->EnableWindow(        m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->EnableWindow(         m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(               m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(        m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->EnableWindow(   m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->EnableWindow(          m_pPrevSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   m_pPrevSpan ? TRUE : FALSE );

   //bEnable = (m_pPrevSpan ? (m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotGirder ? FALSE : TRUE) : FALSE);
   //GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->EnableWindow(bEnable);
   //GetDlgItem(IDC_BACK_SLAB_OFFSET)->EnableWindow(bEnable);
   //GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);

   //// Next Span
   //bEnable = (m_pNextSpan ? (m_bUseSameNumGirders ? FALSE : TRUE) : FALSE);
   //GetDlgItem(IDC_NUMGDR_NEXT_SPAN_LABEL)->EnableWindow(         bEnable );
   //GetDlgItem(IDC_NUMGDR_NEXT_SPAN)->EnableWindow(               bEnable );
   //GetDlgItem(IDC_NUMGDR_SPIN_NEXT_SPAN)->EnableWindow(          bEnable );
   //GetDlgItem(IDC_NEXT_SPAN_SPACING_LABEL)->EnableWindow(        m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT)->EnableWindow(  m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_SPAN_SPACING_GRID)->EnableWindow(         m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->EnableWindow(          m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(                m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(         m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->EnableWindow(    m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->EnableWindow(           m_pNextSpan ? TRUE : FALSE );
   //GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(    m_pNextSpan ? TRUE : FALSE );
 
   UpdateGirderSpacingHyperLinkText();
   UpdateGirderSpacingState();
}

void CGirderSegmentSpacingPage::UpdateGirderSpacingState()
{
   BOOL bEnable = m_SpacingGrid.InputSpacing();

   GirderIndexType nGirders = GetBridgeDescription().GetGirderCount();
   if ( nGirders == 1 || IsBridgeSpacing( GetGirderSpacingType() ) )
   {
      // if there is only 1 girder or we are input the spacing for the whole bridge
      // (not span by span) then disable the input controls
      bEnable = FALSE;
   }

   m_cbGirderSpacingMeasurement.EnableWindow(bEnable);
   m_SpacingGrid.Enable(bEnable);

   GetDlgItem(IDC_SPACING_LABEL)->EnableWindow(            bEnable );
   GetDlgItem(IDC_REF_GIRDER_LABEL)->EnableWindow(         bEnable );
   GetDlgItem(IDC_REF_GIRDER)->EnableWindow(               bEnable );
   GetDlgItem(IDC_REF_GIRDER_OFFSET)->EnableWindow(        bEnable );
   GetDlgItem(IDC_REF_GIRDER_OFFSET_UNIT)->EnableWindow(   bEnable );
   GetDlgItem(IDC_REF_GIRDER_FROM)->EnableWindow(          bEnable );
   GetDlgItem(IDC_REF_GIRDER_OFFSET_TYPE)->EnableWindow(   bEnable );
}

void CGirderSegmentSpacingPage::ToggleGirderSpacingType()
{
   pgsTypes::SupportedBeamSpacing girderSpacingType = GetGirderSpacingType();
   if ( girderSpacingType == pgsTypes::sbsUniform )      // uniform to general
      SetGirderSpacingType(pgsTypes::sbsGeneral); 
   else if ( girderSpacingType == pgsTypes::sbsGeneral ) // general to uniform
      SetGirderSpacingType(pgsTypes::sbsUniform);
   else if ( girderSpacingType == pgsTypes::sbsUniformAdjacent ) // uniform adjacent to general adjacent
      SetGirderSpacingType(pgsTypes::sbsGeneralAdjacent);
   else if ( girderSpacingType == pgsTypes::sbsGeneralAdjacent ) // general adjacent to uniform adjacent
      SetGirderSpacingType(pgsTypes::sbsUniformAdjacent);
   else
      ATLASSERT(false); // is there a new spacing type???
}

LRESULT CGirderSegmentSpacingPage::OnChangeSameGirderSpacing(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // changing from uniform to general, or general to uniform spacing
   pgsTypes::SupportedBeamSpacing oldGirderSpacingType = GetGirderSpacingType();

   ToggleGirderSpacingType();

   pgsTypes::SupportedBeamSpacing newGirderSpacingType = GetGirderSpacingType();

   if ( newGirderSpacingType == pgsTypes::sbsUniform || newGirderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      // we are going from general to uniform spacing
      // if the grid has more than one spacing, we need to ask the user which one is to be
      // used for the entire bridge

      // determine if there is more than one spacing group
      CGirderSpacingData2 spacingData = m_SpacingGrid.GetSpacingData(); 
      GroupIndexType nSpacingGroups = spacingData.GetSpacingGroupCount();
      Float64 bridgeSpacing = 0;
      if ( 1 < nSpacingGroups )
      {
         // there is more than one group... get all the unique spacing values
         std::set<Float64> spacings;
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            Float64 space;
            spacingData.GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
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
            bridgeSpacing = spacingData.GetGirderSpacing(0);
         }
      }
      else
      {
         // there is only one unique spacing value.. get it
         bridgeSpacing = spacingData.GetGirderSpacing(0);
      }

      // join all the girder spacings into one
      spacingData.JoinAll(0);

      // set the spacing value
      spacingData.SetGirderSpacing(0,bridgeSpacing);

      //// cache the current data and apply the new spacing to the grids
      //if ( m_pPrevSpan )
      //{
      //   m_SpacingCache[pgsTypes::Back] = m_SpacingGrid[pgsTypes::Back].GetGirderSpacingData();
      //   m_SpacingGrid[pgsTypes::Back].SetGirderSpacingData(spacingData);
      //   m_SpacingGrid[pgsTypes::Back].FillGrid();

      //   CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_PREV_SPAN_SPACING_MEASUREMENT);
      //   m_GirderSpacingMeasureCache[pgsTypes::Back]  = pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );
      //}

      //if ( m_pNextSpan )
      //{
      //   m_SpacingCache[pgsTypes::Ahead] = m_SpacingGrid[pgsTypes::Ahead].GetGirderSpacingData();
      //   m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingData(spacingData);
      //   m_SpacingGrid[pgsTypes::Ahead].FillGrid();

      //   CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_NEXT_SPAN_SPACING_MEASUREMENT);
      //   m_GirderSpacingMeasureCache[pgsTypes::Ahead] = pcbStartOfSpanSpacingDatum->GetItemData( pcbStartOfSpanSpacingDatum->GetCurSel() );
      //}

      //backGirderSpacingDatum  = m_GirderSpacingMeasureCache[pierFace];
      //aheadGirderSpacingDatum = m_GirderSpacingMeasureCache[pierFace];
   }
   else
   {
      //// restore the girder spacing from the cached values
      //if ( m_pPrevSpan )
      //{
      //   m_SpacingGrid[pgsTypes::Back].SetGirderSpacingData(m_SpacingCache[pgsTypes::Back]);
      //   m_SpacingGrid[pgsTypes::Back].FillGrid();

      //   backGirderSpacingDatum = m_GirderSpacingMeasureCache[pgsTypes::Back];
      //}

      //if ( m_pNextSpan )
      //{
      //   m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingData(m_SpacingCache[pgsTypes::Ahead]);
      //   m_SpacingGrid[pgsTypes::Ahead].FillGrid();

      //   aheadGirderSpacingDatum = m_GirderSpacingMeasureCache[pgsTypes::Ahead];
      //}
   }

   //if ( m_pPrevSpan )
   //{
   //   m_SpacingGrid[pgsTypes::Back].SetGirderSpacingType(m_GirderSpacingType);

   //   CDataExchange dx(this,FALSE);
   //   DDX_CBItemData(&dx, IDC_PREV_SPAN_SPACING_MEASUREMENT, backGirderSpacingDatum);
   //}

   //if ( m_pNextSpan )
   //{
   //   m_SpacingGrid[pgsTypes::Ahead].SetGirderSpacingType(m_GirderSpacingType);

   //   CDataExchange dx(this,FALSE);
   //   DDX_CBItemData(&dx, IDC_NEXT_SPAN_SPACING_MEASUREMENT, aheadGirderSpacingDatum);
   //}

   UpdateChildWindowState();
   return 0;
}

void CGirderSegmentSpacingPage::OnSpacingDatumChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPACING_MEASUREMENT);

   int cursel = pCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pCB->GetItemData(cursel),&ml,&mt);

   m_SpacingGrid.SetMeasurementLocation(ml);
   m_SpacingGrid.SetMeasurementType(mt);
   m_SpacingGrid.FillGrid();
}

void CGirderSegmentSpacingPage::UpdateGirderSpacingHyperLinkText()
{
   CString strSpacingNote(_T(""));
   CString strConstantSpacingNote(_T(""));

   bool bInputSpacing;
   bInputSpacing = m_SpacingGrid.InputSpacing();

   CString strGirderSpacingURL;

   BOOL bEnable = TRUE;
   pgsTypes::SupportedBeamSpacing girderSpacingType = GetGirderSpacingType();
   if ( girderSpacingType == pgsTypes::sbsUniform )
   {
      strSpacingNote = _T("The same girder spacing is used for the entire bridge");
      bEnable = (bInputSpacing ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to define girder spacing span by span");
   }
   else if ( girderSpacingType == pgsTypes::sbsUniformAdjacent )
   {
      strSpacingNote = _T("The same joint spacing is used for the entire bridge");
      bEnable = (bInputSpacing ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to define joint spacing span by span");
   }
   else if ( girderSpacingType == pgsTypes::sbsConstantAdjacent )
   {
      strSpacingNote.Format(_T("The same girder spacing must be used for the entire bridge for %s girders"),
                            GetBridgeDescription().GetGirderFamilyName());

      bEnable = FALSE;

      strGirderSpacingURL = _T("Click to define girder spacing span by span");
   }
   else if ( girderSpacingType == pgsTypes::sbsGeneral )
   {
      strSpacingNote  = _T("Girder spacing is defined span by span");

      bEnable = (bInputSpacing ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to make girder spacing the same for all spans");
   }
   else if ( girderSpacingType == pgsTypes::sbsGeneralAdjacent )
   {
      strSpacingNote  = _T("Joint spacing is defined span by span");

      bEnable = (bInputSpacing ? TRUE : FALSE);

      strGirderSpacingURL = _T("Click to make joint spacing the same for all spans");
   }
   else
   {
      ATLASSERT(false); // is there a new spacing type???
   }

   GetDlgItem(IDC_CONSTANT_SPACING_NOTE)->SetWindowText(strConstantSpacingNote);

   m_GirderSpacingHyperLink.SetWindowText(strSpacingNote);
   m_GirderSpacingHyperLink.SetURL(strGirderSpacingURL);
   m_GirderSpacingHyperLink.EnableWindow(bEnable);
}

void CGirderSegmentSpacingPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_GIRDERSPACING );
}

bool CGirderSegmentSpacingPage::AllowConnectionChange(pgsTypes::PierFaceType side, const CString& connectionName)
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
   if (IsBridgeSpacing(GetGirderSpacingType()))
   {
      // same spacing for entire bridge - we cannot change from this dialog
      pgsTypes::MeasurementLocation ml;
      pgsTypes::MeasurementType mt;
      UnhashGirderSpacing(m_GirderSpacingMeasure,&ml,&mt);
      if (ml == pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG2);
         int result = ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
         if ( result == IDYES )
         {
            SetGirderSpacingMeasurementLocation(pgsTypes::AtPierLine);

#pragma Reminder("UPDATE: review this code... is is needed?")
            //// girder spacing is still measured at the bridge level. need to update
            //// the span by span girder measure to reflect the current bridge level measurement location
            //if ( m_pPrevSpan )
            //{
            //   pgsTypes::MeasurementLocation ml;
            //   pgsTypes::MeasurementType     mt;
            //   UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Back],&ml,&mt);
            //   m_GirderSpacingMeasure[pgsTypes::Back] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
            //   m_SpacingGrid[pgsTypes::Back].SetMeasurementLocation(m_GirderSpacingMeasurementLocation); // also update grid data
            //}

            //if ( m_pNextSpan )
            //{
            //   pgsTypes::MeasurementLocation ml;
            //   pgsTypes::MeasurementType     mt;
            //   UnhashGirderSpacing(m_GirderSpacingMeasure[pgsTypes::Ahead],&ml,&mt);
            //   m_GirderSpacingMeasure[pgsTypes::Ahead] = HashGirderSpacing(m_GirderSpacingMeasurementLocation,mt);
            //   m_SpacingGrid[pgsTypes::Ahead].SetMeasurementLocation(m_GirderSpacingMeasurementLocation); // also update grid data
            //}

            return true;
         }
         else if ( result == IDNO )
         {
            ToggleGirderSpacingType(); // change girder spacing type to span-by-span
            
            pgsTypes::MeasurementLocation ml;
            pgsTypes::MeasurementType     mt;
            UnhashGirderSpacing(m_GirderSpacingMeasure,&ml,&mt);
            m_GirderSpacingMeasure = HashGirderSpacing(pgsTypes::AtPierLine,mt);
            m_SpacingGrid.SetMeasurementLocation(pgsTypes::AtPierLine); // also update grid data

            m_SpacingGrid.SetGirderSpacingType(GetGirderSpacingType());

            ATLASSERT(IsSpanSpacing(GetGirderSpacingType()));

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
      UnhashGirderSpacing(m_GirderSpacingMeasure,&ml,&mt);
      if (ml == pgsTypes::AtCenterlineBearing)
      {
         CString msg;
         msg.LoadString(IDS_INCOMPATIBLE_BEARING_MSG);
         int st = ::AfxMessageBox(msg, MB_ICONQUESTION|MB_YESNO);
         if (st == IDYES)
         {
            m_GirderSpacingMeasure = HashGirderSpacing(pgsTypes::AtPierLine,mt);

            m_SpacingGrid.SetMeasurementLocation(pgsTypes::AtPierLine); // also update grid data
            m_GirderSpacingMeasureCache = m_GirderSpacingMeasure;
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

bool CGirderSegmentSpacingPage::IsContinuousSegment()
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   return pDlg->m_TemporarySupport.GetConnectionType() == pgsTypes::sctContinuousSegment ? true : false;
}

const CGirderSpacing2& CGirderSegmentSpacingPage::GetGirderSpacing()
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   return *pDlg->m_TemporarySupport.GetSegmentSpacing();
}

const CBridgeDescription2& CGirderSegmentSpacingPage::GetBridgeDescription()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   return *pIBridgeDesc->GetBridgeDescription();
}

Float64 CGirderSegmentSpacingPage::GetStation()
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   return pDlg->m_TemporarySupport.GetStation();
}

LPCTSTR CGirderSegmentSpacingPage::GetOrientation()
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   return pDlg->m_TemporarySupport.GetOrientation();
}

void CGirderSegmentSpacingPage::SetGroupTitle()
{
   CWnd* pWnd = GetDlgItem(IDC_SPACING_GROUP);
   CWnd* pParent = GetParent();
   pWnd->SetWindowText(_T("Spacing at centerline of temporary support"));
}

pgsTypes::SupportedBeamSpacing CGirderSegmentSpacingPage::GetGirderSpacingType()
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   return pDlg->m_GirderSpacingType;
}

void CGirderSegmentSpacingPage::SetGirderSpacingType(pgsTypes::SupportedBeamSpacing spacingType)
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   pDlg->m_GirderSpacingType = spacingType;
}

pgsTypes::MeasurementLocation CGirderSegmentSpacingPage::GetGirderSpacingMeasurementLocation()
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   return pDlg->m_GirderSpacingMeasurementLocation;
}

void CGirderSegmentSpacingPage::SetGirderSpacingMeasurementLocation(pgsTypes::MeasurementLocation location)
{
   CTemporarySupportDlg* pDlg = (CTemporarySupportDlg*)GetParent();
   pDlg->m_GirderSpacingMeasurementLocation = location;
}
