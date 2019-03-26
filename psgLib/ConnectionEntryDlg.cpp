///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// ConnectionEntryDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConnectionEntryDlg.h"
#include <MfcTools\CustomDDX.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#include "PGSuperColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionEntryDlg dialog


CConnectionEntryDlg::CConnectionEntryDlg(bool allowEditing,
                                         CWnd* pParent /*=nullptr*/)
	: CDialog(CConnectionEntryDlg::IDD, pParent),
   m_bAllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CConnectionEntryDlg)
	m_Name = _T("");
	//}}AFX_DATA_INIT

   m_WhiteBrush.CreateSolidBrush(METAFILE_BACKGROUND_COLOR);
}


void CConnectionEntryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CString image_name = GetImageName(m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);

	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,image_name, _T("Metafile"), EMF_FIT );

   DDX_UnitValueAndTag(pDX, IDC_END_DISTANCE, IDC_END_DISTANCE_T, m_GirderEndDistance, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_DISTANCE, m_GirderEndDistance, pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_BEARING_OFFSET, IDC_BEARING_OFFSET_T, m_GirderBearingOffset, pDisplayUnits->ComponentDim );

   DDX_KeywordUnitValueAndTag(pDX, IDC_DIAPHRAGM_HEIGHT, IDC_DIAPHRAGM_HEIGHT_T, _T("Compute"), m_DiaphragmHeight, pDisplayUnits->ComponentDim );
   DDX_KeywordUnitValueAndTag(pDX, IDC_DIAPHRAGM_WIDTH, IDC_DIAPHRAGM_WIDTH_T, _T("Compute"), m_DiaphragmWidth, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag( pDX, IDC_DIAPHRAGM_OFFSET, IDC_DIAPHRAGM_OFFSET_UNITS, m_DiaphragmLoadLocation, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation, pDisplayUnits->ComponentDim );

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   //{{AFX_DATA_MAP(CConnectionEntryDlg)
	DDX_Text(pDX, IDC_NAME, m_Name);
	//}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate)
   {
      if (m_Name.IsEmpty())
      {
         AfxMessageBox(_T("Connection Name cannot be blank"));
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CConnectionEntryDlg, CDialog)
	//{{AFX_MSG_MAP(CConnectionEntryDlg)
	ON_BN_CLICKED(ID_HELP,OnHelp)
	ON_BN_CLICKED(IDC_APPLY_DR_TO_BEARING, OnApplyDrToBearing)
	ON_BN_CLICKED(IDC_APPLY_DR_TO_BEAM, OnApplyDrToBeam)
	ON_BN_CLICKED(IDC_DONT_APPLY, OnDontApply)
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE, OnSelchangeEndDistanceMeasure)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnSelchangeBearingOffsetMeasure)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectionEntryDlg message handlers
void CConnectionEntryDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_CONNECTION_DIALOG );
}

BOOL CConnectionEntryDlg::OnInitDialog() 
{
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();

	CDialog::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Name;
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);

   UpdateButtons();

   UpdateConnectionPicture();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConnectionEntryDlg::UpdateButtons()
{
	CButton* pdir = (CButton*)GetDlgItem(IDC_APPLY_DR_TO_BEARING);
   ASSERT(pdir);
	CButton* pbeam = (CButton*)GetDlgItem(IDC_APPLY_DR_TO_BEAM);
   ASSERT(pbeam);
	CButton* pdont = (CButton*)GetDlgItem(IDC_APPLY_DR_TO_BEARING2);
   ASSERT(pdont);

   switch (m_DiaphragmLoadType)
   {
   case ConnectionLibraryEntry::ApplyAtBearingCenterline:
      pdir->SetCheck(1);
      pbeam->SetCheck(0);
      pdont->SetCheck(0);
      EnableDLEdit(false);
      break;
   case ConnectionLibraryEntry::ApplyAtSpecifiedLocation:
      pdir->SetCheck(0);
      pbeam->SetCheck(1);
      pdont->SetCheck(0);
      EnableDLEdit(true);
      break;
   case ConnectionLibraryEntry::DontApply:
      pdir->SetCheck(0);
      pbeam->SetCheck(0);
      pdont->SetCheck(1);
      EnableDLEdit(false);
      break;
   default:
      ASSERT(0);
   }
}

void CConnectionEntryDlg::EnableDLEdit(bool flag)
{
	CEdit* pedit = (CEdit*)GetDlgItem(IDC_DIAPHRAGM_OFFSET);
   ASSERT(pedit);
   pedit->EnableWindow(flag?TRUE:FALSE);


	CWnd* punit = GetDlgItem(IDC_DIAPHRAGM_OFFSET_UNITS);
   ASSERT(punit);
   punit->EnableWindow(flag?TRUE:FALSE);
}

void CConnectionEntryDlg::OnApplyDrToBearing() 
{
   m_DiaphragmLoadType = ConnectionLibraryEntry::ApplyAtBearingCenterline;
   UpdateButtons();
}

void CConnectionEntryDlg::OnApplyDrToBeam() 
{
   m_DiaphragmLoadType = ConnectionLibraryEntry::ApplyAtSpecifiedLocation;
   UpdateButtons();
}

void CConnectionEntryDlg::OnDontApply() 
{
   m_DiaphragmLoadType = ConnectionLibraryEntry::DontApply;
   UpdateButtons();
}

void CConnectionEntryDlg::OnSelchangeEndDistanceMeasure() 
{
   UpdateConnectionPicture();
}

void CConnectionEntryDlg::OnSelchangeBearingOffsetMeasure() 
{
   UpdateConnectionPicture();
}

void CConnectionEntryDlg::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType ems = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType bms = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CString image_name = GetImageName(bms,ems);

	m_ConnectionPicture.SetImage(image_name, _T("Metafile") );
}

void CConnectionEntryDlg::FillBearingOffsetComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(ConnectionLibraryEntry::GetBearingOffsetMeasurementType(ConnectionLibraryEntry::NormalToPier));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::NormalToPier));

   idx = pCB->AddString(ConnectionLibraryEntry::GetBearingOffsetMeasurementType(ConnectionLibraryEntry::AlongGirder));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::AlongGirder));
}

void CConnectionEntryDlg::FillEndDistanceComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(ConnectionLibraryEntry::GetEndDistanceMeasurementType(ConnectionLibraryEntry::FromBearingAlongGirder));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingAlongGirder));

   idx = pCB->AddString(ConnectionLibraryEntry::GetEndDistanceMeasurementType(ConnectionLibraryEntry::FromBearingNormalToPier));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingNormalToPier));

   idx = pCB->AddString(ConnectionLibraryEntry::GetEndDistanceMeasurementType(ConnectionLibraryEntry::FromPierAlongGirder));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierAlongGirder));

   idx = pCB->AddString(ConnectionLibraryEntry::GetEndDistanceMeasurementType(ConnectionLibraryEntry::FromPierNormalToPier));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierNormalToPier));
}

HBRUSH CConnectionEntryDlg::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hBrush = CDialog::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_CONNECTION_MF )
   {
      return m_WhiteBrush;
   }

   return hBrush;
}

CString CConnectionEntryDlg::GetImageName(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CString strName;
   if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         strName = _T("CONNECTION_BRGALONGGDR_ENDALONGGDRFROMBRG");
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         strName = _T("CONNECTION_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         strName = _T("CONNECTION_BRGALONGGDR_ENDALONGGDRFROMPIER");
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         strName = _T("CONNECTION_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         break;
      }
   }
   else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         strName = _T("CONNECTION_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         strName = _T("CONNECTION_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         strName = _T("CONNECTION_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         strName = _T("CONNECTION_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         break;
      }
   }

   return strName;
}
