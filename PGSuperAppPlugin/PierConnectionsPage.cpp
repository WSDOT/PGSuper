///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// PierConnectionsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "PierConnectionsPage.h"
#include "PierDetailsDlg.h"
#include "PGSuperColors.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CPierBearingOffsetMeasureComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   COLORREF oldTextColor = dc.GetTextColor();
   COLORREF oldBkColor = dc.GetBkColor();

   CString lpszText;
   GetLBText(lpDrawItemStruct->itemID, lpszText);

   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
      (lpDrawItemStruct->itemState & ODS_SELECTED))
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));

      // Tell the parent page to update the girder image
      CPierConnectionsPage* pParent = (CPierConnectionsPage*)GetParent();
      ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)GetItemData(lpDrawItemStruct->itemID);
      pParent->UpdateConnectionPicture(brgOffsetType);
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_WINDOW));
   }

   dc.DrawText(lpszText, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

   if (lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
   }

   dc.SetTextColor(oldTextColor);
   dc.SetBkColor(oldBkColor);

   dc.Detach();
}

void CPierEndDistanceMeasureComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   COLORREF oldTextColor = dc.GetTextColor();
   COLORREF oldBkColor = dc.GetBkColor();

   CString lpszText;
   GetLBText(lpDrawItemStruct->itemID, lpszText);

   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
      (lpDrawItemStruct->itemState & ODS_SELECTED))
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));

      // Tell the parent page to update the girder image
      CPierConnectionsPage* pParent = (CPierConnectionsPage*)GetParent();
      ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)GetItemData(lpDrawItemStruct->itemID);
      pParent->UpdateConnectionPicture(endType);
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_WINDOW));
   }

   dc.DrawText(lpszText, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

   if (lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
   }

   dc.SetTextColor(oldTextColor);
   dc.SetBkColor(oldBkColor);

   dc.Detach();
}

/////////////////////////////////////////////////////////////////////////////
// CPierConnectionsPage property page

IMPLEMENT_DYNCREATE(CPierConnectionsPage, CPropertyPage)

CPierConnectionsPage::CPierConnectionsPage() : CPropertyPage(CPierConnectionsPage::IDD)
{
	//{{AFX_DATA_INIT(CPierConnectionsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_WhiteBrush.CreateSolidBrush(METAFILE_BACKGROUND_COLOR);
}

CPierConnectionsPage::~CPierConnectionsPage()
{
}

void CPierConnectionsPage::Init(CPierData2* pPier)
{
   m_pPier = pPier;

   m_PierIdx = pPier->GetIndex();
}

void CPierConnectionsPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPierConnectionsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   // Setup controls
   DDX_Control(pDX,IDC_BOUNDARY_CONDITIONS,m_cbBoundaryCondition);
   DDX_Control(pDX,IDC_LEFT_BEARING_OFFSET,m_BearingOffsetEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_RIGHT_BEARING_OFFSET,m_BearingOffsetEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_LEFT_END_DISTANCE,m_EndDistanceEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_RIGHT_END_DISTANCE,m_EndDistanceEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_BEARING_OFFSET_MEASURE,m_cbBearingOffsetMeasure);
   DDX_Control(pDX,IDC_END_DISTANCE_MEASURE,m_cbEndDistanceMeasure);
   DDX_Control(pDX,IDC_BACK_DIAPHRAGM_OFFSET,m_DiaphragmLoadLocationEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_AHEAD_DIAPHRAGM_OFFSET,m_DiaphragmLoadLocationEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_BACK_DIAPHRAGM_HEIGHT,m_DiaphragmHeightEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_BACK_DIAPHRAGM_WIDTH,m_DiaphragmWidthEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_AHEAD_DIAPHRAGM_HEIGHT,m_DiaphragmHeightEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_AHEAD_DIAPHRAGM_WIDTH,m_DiaphragmWidthEdit[pgsTypes::Ahead]);


   // Set the schematic connection image
   CString image_name = GetImageName(m_BoundaryConditionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,image_name, _T("Metafile") );

   // Boundary Conditions
   DDX_CBItemData(pDX,IDC_BOUNDARY_CONDITIONS,m_BoundaryConditionType);

   // Connection Dimensions
   DDX_UnitValueAndTag(pDX,IDC_LEFT_BEARING_OFFSET, IDC_LEFT_BEARING_OFFSET_T,  m_BearingOffset[pgsTypes::Back],  pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_LEFT_END_DISTANCE,   IDC_LEFT_END_DISTANCE_T,    m_EndDistance[pgsTypes::Back],    pDisplayUnits->GetComponentDimUnit());

   DDX_UnitValueAndTag(pDX,IDC_RIGHT_BEARING_OFFSET,IDC_RIGHT_BEARING_OFFSET_T, m_BearingOffset[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_RIGHT_END_DISTANCE,  IDC_RIGHT_END_DISTANCE_T,   m_EndDistance[pgsTypes::Ahead],   pDisplayUnits->GetComponentDimUnit());

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   // Validate connection dimensions
   if ( pDX->m_bSaveAndValidate )
   {
      if ( m_pPier->GetPrevSpan() && m_pPier->IsAbutment())
      {
         DDV_UnitValueZeroOrMore(pDX, IDC_LEFT_END_DISTANCE,   m_EndDistance[pgsTypes::Back],   pDisplayUnits->GetComponentDimUnit() );

         // the end distance must be less than the bearing offset
         // if end distance is measured from the datum line, the end distance must be less than the bearing offset otherwise, 
         // the end of the girder is beyond the CL Bearing towards the span
         // if the end distance is measured from the CL Bearing, the end distance must be less than the bearing offset otherwise,
         // the end of the girder will go past the Alignment/Pier reference line
         // The only time the End Distance can be greater than the Bearing Offset is at end abutments when the End Distance is
         // measured from the CL Bearing... this is how cantilever spans are modeled
         // Note that this check doesn't occur if there isn't a previous span, which would be a situation where there could be a cantilever,
         // so only the first two cases need to be checked.

         if ((m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierAlongGirder ||
            m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierNormalToPier)
            && (m_BearingOffset[pgsTypes::Back] < m_EndDistance[pgsTypes::Back]))
         {
            pDX->PrepareEditCtrl(IDC_LEFT_END_DISTANCE);
            AfxMessageBox(_T("End Distance must be less than or equal to the Bearing Offset"),MB_OK | MB_ICONINFORMATION);
            pDX->Fail();
         }
      }

      if ( m_pPier->GetNextSpan() && m_pPier->IsAbutment())
      {
         DDV_UnitValueZeroOrMore(pDX, IDC_RIGHT_END_DISTANCE,   m_EndDistance[pgsTypes::Ahead],   pDisplayUnits->GetComponentDimUnit() );

         // See comment above about back side of pier
         if ((m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierAlongGirder ||
            m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierNormalToPier)
            && (m_BearingOffset[pgsTypes::Ahead] < m_EndDistance[pgsTypes::Ahead]))
         {
            pDX->PrepareEditCtrl(IDC_RIGHT_END_DISTANCE);
            AfxMessageBox(_T("End Distance must be less than or equal to the Bearing Offset"),MB_OK | MB_ICONINFORMATION);
            pDX->Fail();
         }
      }

      if ( m_pPier->GetPrevSpan() && m_pPier->GetNextSpan() )
      {
         // this is an intermediate pier
         if ( m_BearingOffset[pgsTypes::Back]+m_BearingOffset[pgsTypes::Ahead] < m_EndDistance[pgsTypes::Back]+m_EndDistance[pgsTypes::Ahead] )
         {
            pDX->PrepareEditCtrl(IDC_RIGHT_END_DISTANCE);
            AfxMessageBox(_T("Error: Girder ends overlap"),MB_OK | MB_ICONINFORMATION);
            pDX->Fail();
         }
      }
   }

   // Diaphragms
   DDX_KeywordUnitValueAndTag(pDX,IDC_BACK_DIAPHRAGM_HEIGHT,IDC_BACK_DIAPHRAGM_HEIGHT_T,_T("Compute"),m_DiaphragmHeight[pgsTypes::Back],pDisplayUnits->GetComponentDimUnit());
   DDX_KeywordUnitValueAndTag(pDX,IDC_BACK_DIAPHRAGM_WIDTH, IDC_BACK_DIAPHRAGM_WIDTH_T, _T("Compute"),m_DiaphragmWidth[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit());
   DDX_CBItemData(pDX,IDC_BACK_DIAPHRAGM_LOAD,m_DiaphragmLoadType[pgsTypes::Back]);
   DDX_Tag( pDX, IDC_BACK_DIAPHRAGM_OFFSET_UNITS, pDisplayUnits->GetComponentDimUnit() );
   if (m_pPier->GetPrevSpan() != nullptr && m_DiaphragmLoadType[pgsTypes::Back] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation )
   {
      DDX_UnitValue(pDX, IDC_BACK_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit());
      DDV_UnitValueZeroOrMore(pDX, IDC_BACK_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit() );
   }

   DDX_KeywordUnitValueAndTag(pDX,IDC_AHEAD_DIAPHRAGM_HEIGHT,IDC_AHEAD_DIAPHRAGM_HEIGHT_T,_T("Compute"),m_DiaphragmHeight[pgsTypes::Ahead],pDisplayUnits->GetComponentDimUnit());
   DDX_KeywordUnitValueAndTag(pDX,IDC_AHEAD_DIAPHRAGM_WIDTH, IDC_AHEAD_DIAPHRAGM_WIDTH_T, _T("Compute"),m_DiaphragmWidth[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit());
   DDX_CBItemData(pDX,IDC_AHEAD_DIAPHRAGM_LOAD,m_DiaphragmLoadType[pgsTypes::Ahead]);
   DDX_Tag( pDX, IDC_AHEAD_DIAPHRAGM_OFFSET_UNITS, pDisplayUnits->GetComponentDimUnit() );
   if (m_pPier->GetNextSpan() != nullptr && m_DiaphragmLoadType[pgsTypes::Ahead] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation )
   {
      DDX_UnitValue( pDX, IDC_AHEAD_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_AHEAD_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit() );
   }

   if ( pDX->m_bSaveAndValidate )
   {
      // all of the data has been extracted from the dialog controls and it has been validated
      // set the values on the actual pier object
      m_pPier->SetBoundaryConditionType(m_BoundaryConditionType);

      for ( int i = 0; i < 2; i++ )
      {
         m_pPier->SetBearingOffset(pgsTypes::PierFaceType(i),m_BearingOffset[i],m_BearingOffsetMeasurementType);
         m_pPier->SetGirderEndDistance(pgsTypes::PierFaceType(i),m_EndDistance[i],m_EndDistanceMeasurementType);

         m_pPier->SetDiaphragmHeight(pgsTypes::PierFaceType(i),m_DiaphragmHeight[i]);
         m_pPier->SetDiaphragmWidth(pgsTypes::PierFaceType(i),m_DiaphragmWidth[i]);
         m_pPier->SetDiaphragmLoadType(pgsTypes::PierFaceType(i),m_DiaphragmLoadType[i]);
         m_pPier->SetDiaphragmLoadLocation(pgsTypes::PierFaceType(i),m_DiaphragmLoadLocation[i]);
      }
   }
}


BEGIN_MESSAGE_MAP(CPierConnectionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierConnectionsPage)
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE, OnEndDistanceMeasureChanged)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnBearingOffsetMeasureChanged)
   ON_CBN_SELCHANGE(IDC_BOUNDARY_CONDITIONS, OnBoundaryConditionChanged)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_BACK_DIAPHRAGM_LOAD, &CPierConnectionsPage::OnBackDiaphragmLoadTypeChanged)
   ON_CBN_SELCHANGE(IDC_AHEAD_DIAPHRAGM_LOAD, &CPierConnectionsPage::OnAheadDiaphragmLoadTypeChanged)
   ON_BN_CLICKED(IDC_COPY, &CPierConnectionsPage::OnCopyFromLibrary)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierConnectionsPage message handlers

BOOL CPierConnectionsPage::OnInitDialog() 
{
   m_BoundaryConditionType = m_pPier->GetBoundaryConditionType();

   for ( int i = 0; i < 2; i++ )
   {
      std::tie(m_BearingOffset[i], m_BearingOffsetMeasurementType) = m_pPier->GetBearingOffset(pgsTypes::PierFaceType(i),true);
      std::tie(m_EndDistance[i], m_EndDistanceMeasurementType) = m_pPier->GetGirderEndDistance(pgsTypes::PierFaceType(i),true);

      m_DiaphragmHeight[i]       = m_pPier->GetDiaphragmHeight(pgsTypes::PierFaceType(i));
      m_DiaphragmWidth[i]        = m_pPier->GetDiaphragmWidth(pgsTypes::PierFaceType(i));
      m_DiaphragmLoadType[i]     = m_pPier->GetDiaphragmLoadType(pgsTypes::PierFaceType(i));
      m_DiaphragmLoadLocation[i] = m_pPier->GetDiaphragmLoadLocation(pgsTypes::PierFaceType(i));
   }

   InitializeComboBoxes();

   CPropertyPage::OnInitDialog();

   FillBoundaryConditionComboBox(); // must do this after OnInitDialog

   GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_LABEL)->SetWindowText(_T("Distance from CL Brg Line to C.G. of Diaphragm"));
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_OFFSET_LABEL)->SetWindowText(_T("Distance from CL Brg Line to C.G. of Diaphragm"));


   if (m_pPier->IsPier() /* not an abutment */)
   {
      m_cbBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);
   }
   else
   {
      ATLASSERT(m_pPier->IsAbutment());
      if ( m_pPier->GetIndex() == 0 )
      {
         m_cbBoundaryCondition.SetPierType(PIERTYPE_START);
      }
      else
      {
         m_cbBoundaryCondition.SetPierType(PIERTYPE_END);
      }
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierConnectionsPage::InitializeComboBoxes()
{
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();
   FillDiaphragmLoadComboBox();
}

void CPierConnectionsPage::FillBoundaryConditionComboBox()
{
   std::vector<pgsTypes::BoundaryConditionType> connections( m_pPier->GetBridgeDescription()->GetBoundaryConditionTypes(m_PierIdx) );
   m_cbBoundaryCondition.Initialize(m_pPier->IsBoundaryPier(),connections,IsNonstructuralDeck(m_pPier->GetBridgeDescription()->GetDeckDescription()->GetDeckType()));

   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_BOUNDARY_CONDITIONS,m_BoundaryConditionType);
}

void CPierConnectionsPage::FillDiaphragmLoadComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD);
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm over CL Bearing")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtBearingCenterline));
   if ( m_pPier->IsBoundaryPier() )
   {
      // this option is only available at boundary piers
      pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm to girder")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtSpecifiedLocation));
   }
   pCB->SetItemData( pCB->AddString(_T("Ignore diaphragm weight")), DWORD_PTR(ConnectionLibraryEntry::DontApply));

   pCB = (CComboBox*)GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD);
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm over CL Bearing")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtBearingCenterline));
   if ( m_pPier->IsBoundaryPier() )
   {
      // this option is only available at boundary piers
      pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm to girder")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtSpecifiedLocation));
   }
   pCB->SetItemData( pCB->AddString(_T("Ignore diaphragm weight")), DWORD_PTR(ConnectionLibraryEntry::DontApply));
}

void CPierConnectionsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PIERDETAILS_CONNECTIONS );
}

void CPierConnectionsPage::OnEndDistanceMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CPierConnectionsPage::OnBearingOffsetMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CPierConnectionsPage::OnBoundaryConditionChanged()
{
   BOOL bEnable = (m_pPier->GetPrevSpan() == nullptr ? FALSE : TRUE);

   // Connection properties on back side of pier
   GetDlgItem(IDC_LEFT_LABEL)->EnableWindow(bEnable);
   m_BearingOffsetEdit[pgsTypes::Back].EnableWindow(bEnable);
   GetDlgItem(IDC_LEFT_BEARING_OFFSET_T)->EnableWindow(bEnable);
   m_EndDistanceEdit[pgsTypes::Back].EnableWindow(bEnable);
   GetDlgItem(IDC_LEFT_END_DISTANCE_T)->EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_HEIGHT_LABEL)->EnableWindow(bEnable);
   m_DiaphragmHeightEdit[pgsTypes::Back].EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_HEIGHT_T)->EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_WIDTH_LABEL)->EnableWindow(bEnable);
   m_DiaphragmWidthEdit[pgsTypes::Back].EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_WIDTH_T)->EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD)->EnableWindow(bEnable);

   // Connection properties on ahead side of pier
   bEnable = (m_pPier->GetNextSpan() == nullptr ? FALSE : TRUE);
   GetDlgItem(IDC_RIGHT_LABEL)->EnableWindow(bEnable);
   m_BearingOffsetEdit[pgsTypes::Ahead].EnableWindow(bEnable);
   GetDlgItem(IDC_RIGHT_BEARING_OFFSET_T)->EnableWindow(bEnable);
   m_EndDistanceEdit[pgsTypes::Ahead].EnableWindow(bEnable);
   GetDlgItem(IDC_RIGHT_END_DISTANCE_T)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_HEIGHT_LABEL)->EnableWindow(bEnable);
   m_DiaphragmHeightEdit[pgsTypes::Ahead].EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_HEIGHT_T)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_WIDTH_LABEL)->EnableWindow(bEnable);
   m_DiaphragmWidthEdit[pgsTypes::Ahead].EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_WIDTH_T)->EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD)->EnableWindow(bEnable);

   OnBackDiaphragmLoadTypeChanged();
   OnAheadDiaphragmLoadTypeChanged();

   UpdateConnectionPicture();
}

void CPierConnectionsPage::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);
   curSel = pcbConnectionType->GetCurSel();
   pgsTypes::BoundaryConditionType connectionType = (pgsTypes::BoundaryConditionType)pcbConnectionType->GetItemData(curSel);

   CString image_name = GetImageName(connectionType,brgOffsetType,endType);

	m_ConnectionPicture.SetImage(image_name, _T("Metafile") );
}

void CPierConnectionsPage::UpdateConnectionPicture(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType)
{
   CComboBox* pcbEnd = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);
   curSel = pcbConnectionType->GetCurSel();
   pgsTypes::BoundaryConditionType connectionType = (pgsTypes::BoundaryConditionType)pcbConnectionType->GetItemData(curSel);

   CString image_name = GetImageName(connectionType, brgOffsetType, endType);

   m_ConnectionPicture.SetImage(image_name, _T("Metafile"));
}

void CPierConnectionsPage::UpdateConnectionPicture(ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CComboBox* pcbBrg = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   int curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);
   curSel = pcbConnectionType->GetCurSel();
   pgsTypes::BoundaryConditionType connectionType = (pgsTypes::BoundaryConditionType)pcbConnectionType->GetItemData(curSel);

   CString image_name = GetImageName(connectionType, brgOffsetType, endType);

   m_ConnectionPicture.SetImage(image_name, _T("Metafile"));
}

BOOL CPierConnectionsPage::CanMeasureBearingOffsetAlongGirder()
{
   bool bCanMeasureAlongGirder = true;
   if (m_pPier->GetPrevSpan())
   {
      auto* pSpacing = m_pPier->GetGirderSpacing(pgsTypes::Back);
      bCanMeasureAlongGirder &= pSpacing->GetMeasurementLocation() == pgsTypes::AtCenterlineBearing ? false : true;
   }

   if (m_pPier->GetNextSpan())
   {
      auto* pSpacing = m_pPier->GetGirderSpacing(pgsTypes::Ahead);
      bCanMeasureAlongGirder &= pSpacing->GetMeasurementLocation() == pgsTypes::AtCenterlineBearing ? false : true;
   }

   return bCanMeasureAlongGirder;
}

void CPierConnectionsPage::FillBearingOffsetComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   pCB->ResetContent();

   CString strType = pgsPierLabel::GetPierTypeLabelEx(m_pPier->IsAbutment(), m_pPier->GetIndex()).c_str();

   CString strLabel;
   strLabel.Format(_T("Normal to %s Line"),strType);

   int idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::NormalToPier));

   if (CanMeasureBearingOffsetAlongGirder())
   {
      idx = pCB->AddString(_T("Along Girder"));
      pCB->SetItemData(idx, DWORD(ConnectionLibraryEntry::AlongGirder));
   }
}

void CPierConnectionsPage::FillEndDistanceComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Measured from CL Bearing, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingAlongGirder));

   idx = pCB->AddString(_T("Measured from and Normal to CL Bearing"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingNormalToPier));

   CString strType = pgsPierLabel::GetPierTypeLabelEx(m_pPier->IsAbutment(), m_pPier->GetIndex()).c_str();

   CString strLabel;
   strLabel.Format(_T("Measured from %s Line, Along Girder"),strType);
   idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierAlongGirder));

   strLabel.Format(_T("Measured from and Normal to %s Line"),strType);
   idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierNormalToPier));
}

CString CPierConnectionsPage::GetImageName(pgsTypes::BoundaryConditionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CSpanData2* pPrevSpan = m_pPier->GetPrevSpan();
   CSpanData2* pNextSpan = m_pPier->GetNextSpan();
   const int StartPier = PIERTYPE_START;
   const int IntPier   = PIERTYPE_INTERMEDIATE;
   const int EndPier   = PIERTYPE_END;
   int pierType = (pPrevSpan != nullptr && pNextSpan != nullptr ? IntPier : (pPrevSpan == nullptr ? StartPier : EndPier));

   CString strName;
   if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGGDRFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGGDRFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGGDRFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGGDRFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGGDRFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGGDRFROMPIER");
         }
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         }
         break;
      }
   }
   else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         }
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         }
         break;
      }
   }

   return strName;
}

void CPierConnectionsPage::OnBackDiaphragmLoadTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD);
   int cursel = pCB->GetCurSel();
   ConnectionLibraryEntry::DiaphragmLoadType loadType = (ConnectionLibraryEntry::DiaphragmLoadType)pCB->GetItemData(cursel);

   BOOL bEnable = (m_pPier->GetPrevSpan() == nullptr ? FALSE : (loadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)) ? TRUE : FALSE;

   GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_LABEL)->EnableWindow(bEnable);
   m_DiaphragmLoadLocationEdit[pgsTypes::Back].EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_UNITS)->EnableWindow(bEnable);
}

void CPierConnectionsPage::OnAheadDiaphragmLoadTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD);
   int cursel = pCB->GetCurSel();
   ConnectionLibraryEntry::DiaphragmLoadType loadType = (ConnectionLibraryEntry::DiaphragmLoadType)pCB->GetItemData(cursel);

   BOOL bEnable = (m_pPier->GetNextSpan() == nullptr ? FALSE : (loadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)) ? TRUE : FALSE;

   GetDlgItem(IDC_AHEAD_DIAPHRAGM_OFFSET_LABEL)->EnableWindow(bEnable);
   m_DiaphragmLoadLocationEdit[pgsTypes::Ahead].EnableWindow(bEnable);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_OFFSET_UNITS)->EnableWindow(bEnable);
}

void CPierConnectionsPage::OnCopyFromLibrary()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames);
   std::vector<std::_tstring> names;
   pLibNames->EnumGdrConnectionNames(&names);

   if ( names.size() == 0 )
   {
      AfxMessageBox(_T("There are not any connections defined in the library"));
   }
   else
   {
	   CString strNames;
	   std::vector<std::_tstring>::iterator iter = names.begin();
	   strNames = (*iter).c_str();
       iter++;
	   for ( ; iter != names.end(); iter++ )
	   {
	      strNames += _T("\n");
	      CString strName( (*iter).c_str() );
	      strNames += strName;
	   }
	
	   CString strMsg(_T("Select a pre-defined connection from the list. Connection dimensions will be copied to both sides of intermediate piers."));
	   int result = AfxChoose(_T("Copy Connection Data from Library"),strMsg,strNames,0,TRUE);
	   if ( 0 <= result )
	   {
         std::_tstring name = names[result];
         GET_IFACE2(pBroker, ILibrary, pLib);
         const ConnectionLibraryEntry* pEntry = pLib->GetConnectionEntry(name.c_str());

         CDataExchange dx(this,TRUE);
	      DoDataExchange(&dx); // get all the current data
	
         // update with data from library
	      for ( int i = 0; i < 2; i++ )
	      {
	         m_BearingOffset[i] = pEntry->GetGirderBearingOffset();
	         m_EndDistance[i]   = pEntry->GetGirderEndDistance();
	   
	         m_DiaphragmHeight[i]       = pEntry->GetDiaphragmHeight();
	         m_DiaphragmWidth[i]        = pEntry->GetDiaphragmWidth();
	         m_DiaphragmLoadType[i]     = pEntry->GetDiaphragmLoadType();
	         m_DiaphragmLoadLocation[i] = pEntry->GetDiaphragmLoadLocation();
	      }
	      m_EndDistanceMeasurementType   = pEntry->GetEndDistanceMeasurementType();

         auto brgOffsetMeasurementType = pEntry->GetBearingOffsetMeasurementType();
         if( (CanMeasureBearingOffsetAlongGirder() && brgOffsetMeasurementType == ConnectionLibraryEntry::AlongGirder) || brgOffsetMeasurementType == ConnectionLibraryEntry::NormalToPier)
         {
            m_BearingOffsetMeasurementType = brgOffsetMeasurementType;
         }
         else
         {
            AfxMessageBox(_T("The Bearing Offset Measurement type in the library entry is not compatible with the girder spacing measurement type. The bearing offset measurement type will not be changed."), MB_ICONINFORMATION | MB_OK);
         }

         // put the new data back in the controls
	      dx.m_bSaveAndValidate = FALSE;
	      DoDataExchange(&dx);

         // update the images
         OnBoundaryConditionChanged();
         OnBackDiaphragmLoadTypeChanged();
         OnAheadDiaphragmLoadTypeChanged();
	   }
   }
}

BOOL CPierConnectionsPage::OnSetActive()
{
   BOOL bResult = CPropertyPage::OnSetActive();

   // re-get the boundary condition. it could have changed if
   // this page is in the span editing dialog and a cantilever was added
   m_BoundaryConditionType = m_pPier->GetBoundaryConditionType();
   FillBoundaryConditionComboBox();
   OnBoundaryConditionChanged();

   // update the bearing offset measurement options. they are dependent on how girder spacing is measured
   // girder spacing measurement could have changed on the girder spacing tab

   FillBearingOffsetComboBox(); // this is going to change the current selection that was set by DoDataExchange which will cause OnBearingOffsetMeasureChanged to crash

   // the the bearing offset measurement type and reset the control so OnBearingOffsetMeasureChanged will work property
   Float64 brgOffset;
   std::tie(brgOffset,m_BearingOffsetMeasurementType) = m_pPier->GetBearingOffset(m_pPier->GetNextSpan() == nullptr ? pgsTypes::Back : pgsTypes::Ahead,true);
   CDataExchange dx(this, FALSE);
   DDX_CBItemData(&dx, IDC_BEARING_OFFSET_MEASURE, m_BearingOffsetMeasurementType);

   OnBearingOffsetMeasureChanged();

   return bResult;
}
