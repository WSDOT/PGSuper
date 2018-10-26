///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "PierConnectionsPage.h"
#include "PierDetailsDlg.h"
#include "PGSuperColors.h"

#include "HtmlHelp\HelpTopics.hh"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

   m_PierConnectionType = pPier->GetPierConnectionType();

   for ( int i = 0; i < 2; i++ )
   {
      pPier->GetBearingOffset(pgsTypes::PierFaceType(i),&m_BearingOffset[i],&m_BearingOffsetMeasurementType);
      pPier->GetGirderEndDistance(pgsTypes::PierFaceType(i),&m_EndDistance[i],&m_EndDistanceMeasurementType);
      m_SupportWidth[i] = pPier->GetSupportWidth(pgsTypes::PierFaceType(i));

      m_DiaphragmHeight[i]       = pPier->GetDiaphragmHeight(pgsTypes::PierFaceType(i));
      m_DiaphragmWidth[i]        = pPier->GetDiaphragmWidth(pgsTypes::PierFaceType(i));
      m_DiaphragmLoadType[i]     = pPier->GetDiaphragmLoadType(pgsTypes::PierFaceType(i));
      m_DiaphragmLoadLocation[i] = pPier->GetDiaphragmLoadLocation(pgsTypes::PierFaceType(i));
   }
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
   DDX_Control(pDX,IDC_LEFT_SUPPORT_WIDTH,m_SupportWidthEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_RIGHT_SUPPORT_WIDTH,m_SupportWidthEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_BEARING_OFFSET_MEASURE,m_cbBearingOffsetMeasurementType);
   DDX_Control(pDX,IDC_END_DISTANCE_MEASURE,m_cbEndDistanceMeasurementType);
   DDX_Control(pDX,IDC_BACK_DIAPHRAGM_OFFSET,m_DiaphragmLoadLocationEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_AHEAD_DIAPHRAGM_OFFSET,m_DiaphragmLoadLocationEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_BACK_DIAPHRAGM_HEIGHT,m_DiaphragmHeightEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_BACK_DIAPHRAGM_WIDTH,m_DiaphragmWidthEdit[pgsTypes::Back]);
   DDX_Control(pDX,IDC_AHEAD_DIAPHRAGM_HEIGHT,m_DiaphragmHeightEdit[pgsTypes::Ahead]);
   DDX_Control(pDX,IDC_AHEAD_DIAPHRAGM_WIDTH,m_DiaphragmWidthEdit[pgsTypes::Ahead]);

   // Set the schematic connection image
   CString image_name = GetImageName(m_PierConnectionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,image_name, _T("Metafile") );

   // Boundary Conditions
   DDX_CBItemData(pDX,IDC_BOUNDARY_CONDITIONS,m_PierConnectionType);

   // Connection Dimensions
   if ( m_pPier->GetPrevSpan() )
   {
      DDX_UnitValueAndTag(pDX,IDC_LEFT_BEARING_OFFSET, IDC_LEFT_BEARING_OFFSET_T,  m_BearingOffset[pgsTypes::Back],  pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(pDX,IDC_LEFT_END_DISTANCE,   IDC_LEFT_END_DISTANCE_T,    m_EndDistance[pgsTypes::Back],    pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(pDX,IDC_LEFT_SUPPORT_WIDTH,  IDC_LEFT_SUPPORT_WIDTH_T,   m_SupportWidth[pgsTypes::Back],   pDisplayUnits->GetComponentDimUnit());
   }

   if ( m_pPier->GetNextSpan() )
   {
      DDX_UnitValueAndTag(pDX,IDC_RIGHT_BEARING_OFFSET,IDC_RIGHT_BEARING_OFFSET_T, m_BearingOffset[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(pDX,IDC_RIGHT_END_DISTANCE,  IDC_RIGHT_END_DISTANCE_T,   m_EndDistance[pgsTypes::Ahead],   pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(pDX,IDC_RIGHT_SUPPORT_WIDTH, IDC_RIGHT_SUPPORT_WIDTH_T,  m_SupportWidth[pgsTypes::Ahead],  pDisplayUnits->GetComponentDimUnit());
   }

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   // Validate connection dimensions
   if ( pDX->m_bSaveAndValidate )
   {
      if ( m_pPier->GetPrevSpan() )
      {
         DDV_UnitValueZeroOrMore(pDX, IDC_LEFT_END_DISTANCE,   m_EndDistance[pgsTypes::Back],   pDisplayUnits->GetComponentDimUnit() );

         // if end distance is measured from the datum line end distance cannot be greater than
         // the bearing offset
         if ( (m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierAlongGirder ||
               m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierNormalToPier)
               &&
               (m_BearingOffset[pgsTypes::Back] < m_EndDistance[pgsTypes::Back])
            )
         {
            pDX->PrepareEditCtrl(IDC_LEFT_END_DISTANCE);
            AfxMessageBox(_T("End Distance must be less than or equal to the Bearing Offset"),MB_OK | MB_ICONINFORMATION);
            pDX->Fail();
         }
      }

      if ( m_pPier->GetNextSpan() )
      {
         DDV_UnitValueZeroOrMore(pDX, IDC_RIGHT_END_DISTANCE,   m_EndDistance[pgsTypes::Ahead],   pDisplayUnits->GetComponentDimUnit() );

         // if end distance is measured from the datum line end distance cannot be greater than
         // the bearing offset
         if ( (m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierAlongGirder ||
               m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierNormalToPier)
               &&
               (m_BearingOffset[pgsTypes::Ahead] < m_EndDistance[pgsTypes::Ahead])
            )
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
   if ( m_pPier->GetPrevSpan() )
   {
      DDX_UnitValueAndTag(pDX,IDC_BACK_DIAPHRAGM_HEIGHT,IDC_BACK_DIAPHRAGM_HEIGHT_T,m_DiaphragmHeight[pgsTypes::Back],pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(pDX,IDC_BACK_DIAPHRAGM_WIDTH, IDC_BACK_DIAPHRAGM_WIDTH_T, m_DiaphragmWidth[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit());
      DDX_CBItemData(pDX,IDC_BACK_DIAPHRAGM_LOAD,m_DiaphragmLoadType[pgsTypes::Back]);
      DDX_Tag( pDX, IDC_BACK_DIAPHRAGM_OFFSET_UNITS, pDisplayUnits->GetComponentDimUnit() );
      if ( m_DiaphragmLoadType[pgsTypes::Back] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation )
      {
         DDX_UnitValue( pDX, IDC_BACK_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit() );
         DDV_UnitValueZeroOrMore(pDX, IDC_BACK_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Back], pDisplayUnits->GetComponentDimUnit() );
      }
   }

   if ( m_pPier->GetNextSpan() )
   {
      DDX_UnitValueAndTag(pDX,IDC_AHEAD_DIAPHRAGM_HEIGHT,IDC_AHEAD_DIAPHRAGM_HEIGHT_T,m_DiaphragmHeight[pgsTypes::Ahead],pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(pDX,IDC_AHEAD_DIAPHRAGM_WIDTH, IDC_AHEAD_DIAPHRAGM_WIDTH_T, m_DiaphragmWidth[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit());
      DDX_CBItemData(pDX,IDC_AHEAD_DIAPHRAGM_LOAD,m_DiaphragmLoadType[pgsTypes::Ahead]);
      DDX_Tag( pDX, IDC_AHEAD_DIAPHRAGM_OFFSET_UNITS, pDisplayUnits->GetComponentDimUnit() );
      if ( m_DiaphragmLoadType[pgsTypes::Ahead] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation )
      {
         DDX_UnitValue( pDX, IDC_AHEAD_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit() );
         DDV_UnitValueZeroOrMore(pDX, IDC_AHEAD_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit() );
      }
   }

   if ( pDX->m_bSaveAndValidate )
   {
      // all of the data has been extracted from the dialog controls and it has been validated
      // set the values on the actual pier object
      m_pPier->SetPierConnectionType(m_PierConnectionType);

      for ( int i = 0; i < 2; i++ )
      {
         m_pPier->SetBearingOffset(pgsTypes::PierFaceType(i),m_BearingOffset[i],m_BearingOffsetMeasurementType);
         m_pPier->SetGirderEndDistance(pgsTypes::PierFaceType(i),m_EndDistance[i],m_EndDistanceMeasurementType);
         m_pPier->SetSupportWidth(pgsTypes::PierFaceType(i),m_SupportWidth[i]);

         m_pPier->SetDiaphragmHeight(pgsTypes::PierFaceType(i),m_DiaphragmHeight[i]);
         m_pPier->SetDiaphragmWidth(pgsTypes::PierFaceType(i),m_DiaphragmWidth[i]);
         m_pPier->SetDiaphragmLoadType(pgsTypes::PierFaceType(i),m_DiaphragmLoadType[i]);
         m_pPier->SetDiaphragmLoadLocation(pgsTypes::PierFaceType(i),m_DiaphragmLoadLocation[i]);
      }
   }
}


BEGIN_MESSAGE_MAP(CPierConnectionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierConnectionsPage)
	ON_WM_CTLCOLOR()
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
   InitializeComboBoxes();

   CPropertyPage::OnInitDialog();

   CString strLabel;
   strLabel.Format(_T("Distance from %s Line to C.G. of Diaphragm"),IsAbutment() ? _T("Abutment") : _T("Pier"));
   GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_LABEL)->SetWindowText(strLabel);
   GetDlgItem(IDC_AHEAD_DIAPHRAGM_OFFSET_LABEL)->SetWindowText(strLabel);


   m_cbBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierConnectionsPage::InitializeComboBoxes()
{
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();
   FillBoundaryConditionComboBox();
   FillDiaphragmLoadComboBox();
}

void CPierConnectionsPage::FillBoundaryConditionComboBox()
{
   CBoundaryConditionComboBox* pcbBoundaryConditions = (CBoundaryConditionComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);

   std::vector<pgsTypes::PierConnectionType> connections( m_pPier->GetBridgeDescription()->GetPierConnectionTypes(m_PierIdx) );

   pcbBoundaryConditions->Initialize(connections);
}

void CPierConnectionsPage::FillDiaphragmLoadComboBox()
{
#pragma Reminder("UPDATE: option 2 is not available for Continuous Segment boundary conditions")
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD);
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm over CL Bearing")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtBearingCenterline));
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm to girder")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtSpecifiedLocation));
   pCB->SetItemData( pCB->AddString(_T("Ignore diaphragm weight")), DWORD_PTR(ConnectionLibraryEntry::DontApply));

   pCB = (CComboBox*)GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD);
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm over CL Bearing")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtBearingCenterline));
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm to girder")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtSpecifiedLocation));
   pCB->SetItemData( pCB->AddString(_T("Ignore diaphragm weight")), DWORD_PTR(ConnectionLibraryEntry::DontApply));
}

void CPierConnectionsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_CONNECTIONS );
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
   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);
   int curSel = pcbConnectionType->GetCurSel();
   pgsTypes::PierConnectionType connectionType = (pgsTypes::PierConnectionType)pcbConnectionType->GetItemData(curSel);

#pragma Reminder("UPDATE: clean up this code...")
   //BOOL bEnable = (connectionType == pgsTypes::ContinuousSegment ? FALSE : TRUE);
   BOOL bEnable = TRUE;
   if ( m_pPier->GetPrevSpan() )
   {
      GetDlgItem(IDC_LEFT_LABEL)->EnableWindow(bEnable);
      m_BearingOffsetEdit[pgsTypes::Back].EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_BEARING_OFFSET_T)->EnableWindow(bEnable);
      m_EndDistanceEdit[pgsTypes::Back].EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_END_DISTANCE_T)->EnableWindow(bEnable);
      m_SupportWidthEdit[pgsTypes::Back].EnableWindow(bEnable);
      GetDlgItem(IDC_LEFT_SUPPORT_WIDTH_T)->EnableWindow(bEnable);

      GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD)->EnableWindow(bEnable);

      OnBackDiaphragmLoadTypeChanged();
   }

   if ( m_pPier->GetNextSpan() )
   {
      GetDlgItem(IDC_RIGHT_LABEL)->EnableWindow(bEnable);
      m_BearingOffsetEdit[pgsTypes::Ahead].EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_BEARING_OFFSET_T)->EnableWindow(bEnable);
      m_EndDistanceEdit[pgsTypes::Ahead].EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_END_DISTANCE_T)->EnableWindow(bEnable);
      m_SupportWidthEdit[pgsTypes::Ahead].EnableWindow(bEnable);
      GetDlgItem(IDC_RIGHT_SUPPORT_WIDTH_T)->EnableWindow(bEnable);

      GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD)->EnableWindow(bEnable);

      OnAheadDiaphragmLoadTypeChanged();
   }

   GetDlgItem(IDC_COPY)->EnableWindow(bEnable);

   //if ( connectionType == pgsTypes::ContinuousSegment )
   //{
   //   m_SupportWidthEdit[pgsTypes::Back].EnableWindow(TRUE);
   //   GetDlgItem(IDC_LEFT_SUPPORT_WIDTH_T)->EnableWindow(TRUE);

   //   m_DiaphragmHeightEdit[pgsTypes::Ahead].EnableWindow(FALSE);
   //   GetDlgItem(IDC_AHEAD_DIAPHRAGM_HEIGHT_T)->EnableWindow(FALSE);
   //   m_DiaphragmWidthEdit[pgsTypes::Ahead].EnableWindow(FALSE);
   //   GetDlgItem(IDC_AHEAD_DIAPHRAGM_WIDTH_T)->EnableWindow(FALSE);
   //}
   //else
   {
      if ( m_pPier->GetPrevSpan() )
      {
         m_DiaphragmHeightEdit[pgsTypes::Back].EnableWindow(TRUE);
         GetDlgItem(IDC_BACK_DIAPHRAGM_HEIGHT_T)->EnableWindow(TRUE);
         m_DiaphragmWidthEdit[pgsTypes::Back].EnableWindow(TRUE);
         GetDlgItem(IDC_BACK_DIAPHRAGM_WIDTH_T)->EnableWindow(TRUE);
      }

      if ( m_pPier->GetNextSpan() )
      {
         m_DiaphragmHeightEdit[pgsTypes::Ahead].EnableWindow(TRUE);
         GetDlgItem(IDC_AHEAD_DIAPHRAGM_HEIGHT_T)->EnableWindow(TRUE);
         m_DiaphragmWidthEdit[pgsTypes::Ahead].EnableWindow(TRUE);
         GetDlgItem(IDC_AHEAD_DIAPHRAGM_WIDTH_T)->EnableWindow(TRUE);
      }
   }


   GetDlgItem(IDC_BEARING_OFFSET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_DISTANCE_LABEL)->EnableWindow(bEnable);
   m_cbBearingOffsetMeasurementType.EnableWindow(bEnable);
   m_cbEndDistanceMeasurementType.EnableWindow(bEnable);

   UpdateConnectionPicture();
}

void CPierConnectionsPage::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType ems = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType bms = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);
   curSel = pcbConnectionType->GetCurSel();
   pgsTypes::PierConnectionType connectionType = (pgsTypes::PierConnectionType)pcbConnectionType->GetItemData(curSel);

   CString image_name = GetImageName(connectionType,bms,ems);

	m_ConnectionPicture.SetImage(image_name, _T("Metafile") );
}

void CPierConnectionsPage::FillBearingOffsetComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   pCB->ResetContent();

   CString strLabel;
   strLabel.Format(_T("Normal to %s Line"),IsAbutment() ? _T("Abutment") : _T("Pier"));

   int idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::NormalToPier));

   idx = pCB->AddString(_T("Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::AlongGirder));
}

void CPierConnectionsPage::FillEndDistanceComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Measured from CL Bearing, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingAlongGirder));

   idx = pCB->AddString(_T("Measured from and Normal to CL Bearing"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingNormalToPier));

   CString strLabel;
   strLabel.Format(_T("Measured from %s Line, Along Girder"),IsAbutment() ? _T("Abutment") : _T("Pier"));
   idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierAlongGirder));

   strLabel.Format(_T("Measured from and Normal to %s Line"),IsAbutment() ? _T("Abutment") : _T("Pier"));
   idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierNormalToPier));
}

HBRUSH CPierConnectionsPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
#pragma Reminder("REVIEW: is this code needed?") // maybe for XP... everything seems on in Win7 (see span connections page)
   HBRUSH hBrush = CDialog::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_CONNECTION_MF )
   {
      return m_WhiteBrush;
   }

   return hBrush;
}

CString CPierConnectionsPage::GetImageName(pgsTypes::PierConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CSpanData2* pPrevSpan = m_pPier->GetPrevSpan();
   CSpanData2* pNextSpan = m_pPier->GetNextSpan();
   const int StartPier = PIERTYPE_START;
   const int IntPier   = PIERTYPE_INTERMEDIATE;
   const int EndPier   = PIERTYPE_END;
   int pierType = (pPrevSpan != NULL && pNextSpan != NULL ? IntPier : (pPrevSpan == NULL ? StartPier : EndPier));

   CString strName;
   if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGGDR_ENDALONGGDRFROMBRG");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGGDR_ENDALONGGDRFROMBRG");
         else
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGGDRFROMBRG");
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         else
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGGDR_ENDALONGGDRFROMPIER");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGGDR_ENDALONGGDRFROMPIER");
         else
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGGDRFROMPIER");
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         else
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         break;
      }
   }
   else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         else
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         else
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         else
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         if ( pierType == StartPier )
            strName = _T("SA_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         else if ( pierType == EndPier )
            strName = _T("EA_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         else
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         break;
      }
   }

   return strName;
}

bool CPierConnectionsPage::IsAbutment()
{
   return m_pPier->IsAbutment();
}

void CPierConnectionsPage::OnBackDiaphragmLoadTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD);
   int cursel = pCB->GetCurSel();
   ConnectionLibraryEntry::DiaphragmLoadType loadType = (ConnectionLibraryEntry::DiaphragmLoadType)pCB->GetItemData(cursel);

   BOOL bEnable = (loadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation) ? TRUE : FALSE;

   GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_LABEL)->EnableWindow(bEnable);
   m_DiaphragmLoadLocationEdit[pgsTypes::Back].EnableWindow(bEnable);
   GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_UNITS)->EnableWindow(bEnable);
}

void CPierConnectionsPage::OnAheadDiaphragmLoadTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD);
   int cursel = pCB->GetCurSel();
   ConnectionLibraryEntry::DiaphragmLoadType loadType = (ConnectionLibraryEntry::DiaphragmLoadType)pCB->GetItemData(cursel);

   BOOL bEnable = (loadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation) ? TRUE : FALSE;

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
	      CDataExchange dx(this,TRUE);
	      DoDataExchange(&dx);
	
	      std::_tstring name = names[result];
		
	      GET_IFACE2(pBroker,ILibrary,pLib);
	      const ConnectionLibraryEntry* pEntry = pLib->GetConnectionEntry(name.c_str());
	
	      for ( int i = 0; i < 2; i++ )
	      {
	         m_BearingOffset[i] = pEntry->GetGirderBearingOffset();
	         m_EndDistance[i]   = pEntry->GetGirderEndDistance();
	         m_SupportWidth[i]  = pEntry->GetSupportWidth();
	   
	         m_DiaphragmHeight[i]       = pEntry->GetDiaphragmHeight();
	         m_DiaphragmWidth[i]        = pEntry->GetDiaphragmWidth();
	         m_DiaphragmLoadType[i]     = pEntry->GetDiaphragmLoadType();
	         m_DiaphragmLoadLocation[i] = pEntry->GetDiaphragmLoadLocation();
	      }
	      m_EndDistanceMeasurementType   = pEntry->GetEndDistanceMeasurementType();
	      m_BearingOffsetMeasurementType = pEntry->GetBearingOffsetMeasurementType();
	
	      dx.m_bSaveAndValidate = FALSE;
	      DoDataExchange(&dx);

         OnBoundaryConditionChanged();
         OnBackDiaphragmLoadTypeChanged();
         OnAheadDiaphragmLoadTypeChanged();
	   }
   }
}

BOOL CPierConnectionsPage::OnSetActive()
{
   BOOL bResult = CPropertyPage::OnSetActive();

   if ( m_pPier->GetPrevSpan() == NULL )
   {
      // if there isn't a previous span, then only have input for the ahead side
      GetDlgItem(IDC_LEFT_LABEL)->EnableWindow(FALSE);
      m_BearingOffsetEdit[pgsTypes::Back].EnableWindow(FALSE);
      GetDlgItem(IDC_LEFT_BEARING_OFFSET_T)->EnableWindow(FALSE);
      m_EndDistanceEdit[pgsTypes::Back].EnableWindow(FALSE);
      GetDlgItem(IDC_LEFT_END_DISTANCE_T)->EnableWindow(FALSE);
      m_SupportWidthEdit[pgsTypes::Back].EnableWindow(FALSE);
      GetDlgItem(IDC_LEFT_SUPPORT_WIDTH_T)->EnableWindow(FALSE);

      m_DiaphragmHeightEdit[pgsTypes::Back].EnableWindow(FALSE);
      GetDlgItem(IDC_BACK_DIAPHRAGM_HEIGHT_T)->EnableWindow(FALSE);
      m_DiaphragmWidthEdit[pgsTypes::Back].EnableWindow(FALSE);
      GetDlgItem(IDC_BACK_DIAPHRAGM_WIDTH_T)->EnableWindow(FALSE);

      GetDlgItem(IDC_BACK_DIAPHRAGM_LOAD)->EnableWindow(FALSE);
      m_DiaphragmLoadLocationEdit[pgsTypes::Back].EnableWindow(FALSE);
      GetDlgItem(IDC_BACK_DIAPHRAGM_OFFSET_UNITS)->EnableWindow(FALSE);

      m_cbBoundaryCondition.SetPierType(PIERTYPE_START);
   }

   if ( m_pPier->GetNextSpan() == NULL )
   {
      GetDlgItem(IDC_RIGHT_LABEL)->EnableWindow(FALSE);
      m_BearingOffsetEdit[pgsTypes::Ahead].EnableWindow(FALSE);
      GetDlgItem(IDC_RIGHT_BEARING_OFFSET_T)->EnableWindow(FALSE);
      m_EndDistanceEdit[pgsTypes::Ahead].EnableWindow(FALSE);
      GetDlgItem(IDC_RIGHT_END_DISTANCE_T)->EnableWindow(FALSE);
      m_SupportWidthEdit[pgsTypes::Ahead].EnableWindow(FALSE);
      GetDlgItem(IDC_RIGHT_SUPPORT_WIDTH_T)->EnableWindow(FALSE);

      m_DiaphragmHeightEdit[pgsTypes::Ahead].EnableWindow(FALSE);
      GetDlgItem(IDC_AHEAD_DIAPHRAGM_HEIGHT_T)->EnableWindow(FALSE);
      m_DiaphragmWidthEdit[pgsTypes::Ahead].EnableWindow(FALSE);
      GetDlgItem(IDC_AHEAD_DIAPHRAGM_WIDTH_T)->EnableWindow(FALSE);

      GetDlgItem(IDC_AHEAD_DIAPHRAGM_LOAD)->EnableWindow(FALSE);
      m_DiaphragmLoadLocationEdit[pgsTypes::Ahead].EnableWindow(FALSE);
      GetDlgItem(IDC_AHEAD_DIAPHRAGM_OFFSET_UNITS)->EnableWindow(FALSE);

      m_cbBoundaryCondition.SetPierType(PIERTYPE_END);
   }

   OnBoundaryConditionChanged();
   OnBackDiaphragmLoadTypeChanged();
   OnAheadDiaphragmLoadTypeChanged();

   return bResult;
}
