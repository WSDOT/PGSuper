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

// GirderSegmentSpacingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "GirderSegmentSpacingPage.h"
#include "TemporarySupportDlg.h"
#include "PierDetailsDlg.h"
#include "SelectItemDlg.h"
#include "Utilities.h"

#include <PGSuperUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>



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

void CGirderSegmentSpacingPage::Init(CPierData2* pPierData)
{
   m_bIsPier = true;
   m_strSupportLabel = _T("Pier");

   // Only a single girder spacing is used at the pier. The back spacing is used in this case
   CGirderSpacing2* pSpacing = pPierData->GetGirderSpacing(pgsTypes::Back);
   CBridgeDescription2* pBridge = pPierData->GetBridgeDescription();

   CommonInit(pSpacing,pBridge);
}

void CGirderSegmentSpacingPage::Init(CTemporarySupportData* pTS)
{
   m_bIsPier = false;
   m_strSupportLabel = _T("Temporary Support");

   CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();

   CSpanData2* pSpan = pTS->GetSpan();
   CBridgeDescription2* pBridge = pSpan->GetBridgeDescription();

   CommonInit(pSpacing,pBridge);
}

void CGirderSegmentSpacingPage::CommonInit(CGirderSpacing2* pSpacing,CBridgeDescription2* pBridge)
{
   // Girder Spacing Options
   m_GirderSpacingMeasure = HashGirderSpacing( pSpacing->GetMeasurementLocation(), pSpacing->GetMeasurementType() );
   m_GirderSpacingMeasureCache = m_GirderSpacingMeasure;

   // Girder Spacing Datum
   m_RefGirderIdx        = pSpacing->GetRefGirder();
   m_RefGirderOffset     = pSpacing->GetRefGirderOffset();
   m_RefGirderOffsetType = pSpacing->GetRefGirderOffsetType();

   m_RefGirderIdxCache        = m_RefGirderIdx;
   m_RefGirderOffsetCache     = m_RefGirderOffset;
   m_RefGirderOffsetTypeCache = m_RefGirderOffsetType;

   // Fill spacing grid
   m_SpacingGrid.InitializeGridData( pSpacing );
   m_SpacingCache = *pSpacing;
}

void CGirderSegmentSpacingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CGirderSegmentSpacingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
   DDX_Control(pDX, IDC_SPACING_MEASUREMENT,    m_cbGirderSpacingMeasurement);
   DDX_Control(pDX, IDC_CB_SPACG_TYPE,          m_cbGirderSpacingType);

   DDX_CBItemData(pDX, IDC_SPACING_MEASUREMENT, m_GirderSpacingMeasure);

   DDV_SpacingGrid(pDX,IDC_SPACING_GRID,&m_SpacingGrid);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_CBItemData(pDX, IDC_REF_GIRDER, m_RefGirderIdx);
   DDX_OffsetAndTag(pDX, IDC_REF_GIRDER_OFFSET,IDC_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset, pDisplayUnits->GetXSectionDimUnit());
   DDX_CBItemData(pDX, IDC_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType);

   if ( pDX->m_bSaveAndValidate )
   {
      CGirderSpacing2* pSpacing = GetSpacing();
      pgsTypes::MeasurementLocation ml;
      pgsTypes::MeasurementType mt;
      UnhashGirderSpacing(m_GirderSpacingMeasure,&ml,&mt);
      pSpacing->SetMeasurementLocation(ml);
      pSpacing->SetMeasurementType(mt);
      pSpacing->SetRefGirder(m_RefGirderIdx);
      pSpacing->SetRefGirderOffset(m_RefGirderOffset);
      pSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType);

      if ( m_bIsPier )
      {
         // we are editing a pier...
         CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
         if ( pParent->m_pPier->IsInteriorPier() )
         {
            //... and it is an interior pier.
            // There is only one spacing for an interior pier so that precast segments line up.
            // We edit the back spacing so make the ahead spacing same as the back spacing.
            pParent->m_pPier->SetGirderSpacing(pgsTypes::Ahead,*(pParent->m_pPier->GetGirderSpacing(pgsTypes::Back)));
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CGirderSegmentSpacingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentSpacingPage)
   ON_CBN_SELCHANGE(IDC_SPACING_MEASUREMENT,OnSpacingDatumChanged)
   ON_CBN_SELCHANGE(IDC_CB_SPACG_TYPE,OnChangeSameGirderSpacing)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentSpacingPage message handlers
BOOL CGirderSegmentSpacingPage::OnInitDialog() 
{
   FillGirderSpacingMeasurementComboBox();

   CGirderSpacing2* pSpacing = GetSpacing();
   m_SpacingGrid.InitializeGridData(pSpacing);

   m_SpacingGrid.SubclassDlgItem(IDC_SPACING_GRID,this);
   m_SpacingGrid.CustomInit();
   m_SpacingGrid.SetSkewAngle(GetSkewAngle());
	
   pgsTypes::SupportedBeamSpacing spacingType = GetBridgeDescription()->GetGirderSpacingType();
   GetDlgItem(IDC_SPACING_LABEL)->SetWindowText(IsGirderSpacing(spacingType) ? _T("Girder Spacing") : _T("Joint Spacing"));

   FillRefGirderComboBox();
   FillRefGirderOffsetTypeComboBox();

   SetGroupTitle();
   
   CPropertyPage::OnInitDialog();

   if ( spacingType == pgsTypes::sbsUniform )
   {
      m_cbGirderSpacingType.AddString(_T("The same girder spacing is used for the entire bridge"));
      m_cbGirderSpacingType.AddString(_T("Girder spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(0);
   }
   else if ( spacingType == pgsTypes::sbsUniformAdjacent )
   {
      m_cbGirderSpacingType.AddString(_T("The same joint spacing is used for the entire bridge"));
      m_cbGirderSpacingType.AddString(_T("Joint spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(0);
   }
   else if ( spacingType == pgsTypes::sbsConstantAdjacent )
   {
      CString note;
      note.Format(_T("The same girder spacing must be used for the entire bridge for %s girders"), GetBridgeDescription()->GetGirderFamilyName());
      m_cbGirderSpacingType.AddString(note);
      m_cbGirderSpacingType.SetCurSel(0);
      m_cbGirderSpacingType.EnableWindow(FALSE);
   }
   else if ( spacingType == pgsTypes::sbsGeneral )
   {
      m_cbGirderSpacingType.AddString(_T("The same girder spacing is used for the entire bridge"));
      m_cbGirderSpacingType.AddString(_T("Girder spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(1);
   }
   else if ( spacingType == pgsTypes::sbsGeneralAdjacent )
   {
      m_cbGirderSpacingType.AddString(_T("The same joint spacing is used for the entire bridge"));
      m_cbGirderSpacingType.AddString(_T("Joint spacing is defined span by span"));
      m_cbGirderSpacingType.SetCurSel(1);
   }
   else
   {
      ATLASSERT(false); // is there a new spacing type???
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CGirderSegmentSpacingPage::FillGirderSpacingMeasurementComboBox()
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(IDC_SPACING_MEASUREMENT);
   pSpacingType->ResetContent();

   CString strLabel;
   strLabel.Format(_T("Measured at and along the CL %s"),m_strSupportLabel);
   int idx = pSpacingType->AddString(strLabel);
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   
   strLabel.Format(_T("Measured normal to alignment at CL %s"),m_strSupportLabel);
   idx = pSpacingType->AddString(strLabel);
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

   GirderIndexType nGirders = GetBridgeDescription()->GetGirderCount();
   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD)i);
   }

   pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel);
}

BOOL CGirderSegmentSpacingPage::OnSetActive() 
{
   UpdateChildWindowState();

   CGirderSpacing2* pSpacing = GetSpacing();
   m_SpacingGrid.InitializeGridData(pSpacing);

	BOOL bResult = CPropertyPage::OnSetActive();

   // if the connection type is continuous segment, the spacing is not defined at this
   // support (spacing is only defined at the ends of segments)
   int show = (IsContinuousSegment() ? SW_HIDE : SW_SHOW);
   m_cbGirderSpacingType.ShowWindow(show);
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
   UpdateGirderSpacingState();
}

void CGirderSegmentSpacingPage::UpdateGirderSpacingState()
{
   BOOL bEnable = m_SpacingGrid.InputSpacing();

   GirderIndexType nGirders = GetBridgeDescription()->GetGirderCount();
   pgsTypes::SupportedBeamSpacing spacingType = GetBridgeDescription()->GetGirderSpacingType();
   if ( nGirders == 1 || IsBridgeSpacing( spacingType ) )
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

void CGirderSegmentSpacingPage::OnChangeSameGirderSpacing()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // changing from uniform to general, or general to uniform spacing
   pgsTypes::SupportedBeamSpacing oldSpacingType = GetBridgeDescription()->GetGirderSpacingType();
   pgsTypes::SupportedBeamSpacing spacingType = ToggleGirderSpacingType(oldSpacingType);

   if ( spacingType == pgsTypes::sbsUniform || spacingType == pgsTypes::sbsUniformAdjacent )
   {
      // we are going from general to uniform spacing
      // if the grid has more than one spacing, we need to ask the user which one is to be
      // used for the entire bridge

      CComboBox* pcbSpacingDatum = (CComboBox*)GetDlgItem(IDC_SPACING_MEASUREMENT);
      m_GirderSpacingMeasureCache = (DWORD)pcbSpacingDatum->GetItemData( pcbSpacingDatum->GetCurSel() );

      // determine if there is more than one spacing group
      CGirderSpacing2* pSpacing = GetSpacing();
      m_SpacingCache = *pSpacing;

      GroupIndexType nSpacingGroups = pSpacing->GetSpacingGroupCount();
      Float64 bridgeSpacing = 0;
      if ( 1 < nSpacingGroups )
      {
         // there is more than one group... get all the unique spacing values
         std::set<Float64> spacings;
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            Float64 space;
            pSpacing->GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
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
            std::set<Float64>::iterator begin(spacings.begin());
            std::set<Float64>::iterator iter(begin);
            std::set<Float64>::iterator end(spacings.end());
            for ( ; iter != end; iter++ )
            {
               Float64 spacing = *iter;

               CString strItem;
               if ( IsGirderSpacing(oldSpacingType) )
                  strItem.Format(_T("%s"),FormatDimension(spacing,pDisplayUnits->GetXSectionDimUnit(),true));
               else
                  strItem.Format(_T("%s"),FormatDimension(spacing,pDisplayUnits->GetComponentDimUnit(),true));

               if ( iter != begin )
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
               return;
            }
         }
         else
         {
            // there is only one unique spacing value.. get it
            bridgeSpacing = pSpacing->GetGirderSpacing(0);
         }
      }
      else
      {
         // there is only one unique spacing value.. get it
         bridgeSpacing = pSpacing->GetGirderSpacing(0);
      }

      GetBridgeDescription()->SetGirderSpacing(bridgeSpacing);
   }
   else
   {
      // going from uniform to general.. restore the cached values
      SetSpacing(&m_SpacingCache);
   
      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_SPACING_MEASUREMENT, m_GirderSpacingMeasureCache);
   }

   GetBridgeDescription()->SetGirderSpacingType(spacingType);
   m_SpacingGrid.UpdateGrid();

   UpdateChildWindowState();
}

void CGirderSegmentSpacingPage::OnSpacingDatumChanged()
{
   m_SpacingGrid.UpdateGrid();
}


void CGirderSegmentSpacingPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), m_bIsPier ? IDH_PIERDETAILS_GIRDERSPACING : IDH_TSDETAILS_SPACING );
}

bool CGirderSegmentSpacingPage::IsContinuousSegment()
{
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      pgsTypes::PierSegmentConnectionType connection = pParent->m_pPier->GetSegmentConnectionType();
      return (connection == pgsTypes::psctContinuousSegment || connection == pgsTypes::psctIntegralSegment);
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      return pParent->m_pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment ? true : false;
   }
}

CBridgeDescription2* CGirderSegmentSpacingPage::GetBridgeDescription()
{
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      return pParent->GetBridgeDescription();
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      return pParent->GetBridgeDescription();
   }
}

CGirderSpacing2* CGirderSegmentSpacingPage::GetSpacing()
{
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      return pParent->m_pPier->GetGirderSpacing(pgsTypes::Back);
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      return pParent->m_pTS->GetSegmentSpacing();
   }
}

void CGirderSegmentSpacingPage::SetSpacing(CGirderSpacing2* pSpacing)
{
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      pParent->m_pPier->SetGirderSpacing(pgsTypes::Back,*pSpacing);
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      pParent->m_pTS->SetSegmentSpacing(*pSpacing);
   }
}

Float64 CGirderSegmentSpacingPage::GetStation()
{
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      return pParent->m_pPier->GetStation();
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      return pParent->m_pTS->GetStation();
   }
}

LPCTSTR CGirderSegmentSpacingPage::GetOrientation()
{
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      return pParent->m_pPier->GetOrientation();
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      return pParent->m_pTS->GetOrientation();
   }
}

Float64 CGirderSegmentSpacingPage::GetSkewAngle()
{
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   Float64 skewAngle;
   pBridge->GetSkewAngle(GetStation(),GetOrientation(),&skewAngle);
   return skewAngle;
}

void CGirderSegmentSpacingPage::SetGroupTitle()
{
   CWnd* pWnd = GetDlgItem(IDC_SPACING_GROUP);
   CString strLabel;
   strLabel.Format(_T("Spacing at centerline of %s"),m_strSupportLabel);
   pWnd->SetWindowText(strLabel);
}
