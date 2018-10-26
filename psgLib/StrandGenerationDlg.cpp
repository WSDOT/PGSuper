///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// StrandGenerationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StrandGenerationDlg.h"
#include <EAF\EAFApp.h>
#include "..\htmlhelp\HelpTopics.hh"

// CStrandGenerationDlg dialog

IMPLEMENT_DYNAMIC(CStrandGenerationDlg, CDialog)

CStrandGenerationDlg::CStrandGenerationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStrandGenerationDlg::IDD, pParent)
{
   m_bDelete = false;
}

CStrandGenerationDlg::~CStrandGenerationDlg()
{
}

void CStrandGenerationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX,IDC_START_X,IDC_START_X_UNIT,m_Xstart,pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX,IDC_START_Y,IDC_START_Y_UNIT,m_Ystart,pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX,IDC_END_X,  IDC_END_X_UNIT,  m_Xend,  pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX,IDC_END_Y,  IDC_END_Y_UNIT,  m_Yend,  pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX,IDC_START_X2,IDC_START_X_UNIT2,m_Xstart2,pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX,IDC_START_Y2,IDC_START_Y_UNIT2,m_Ystart2,pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX,IDC_END_X2,  IDC_END_X_UNIT2,  m_Xend2,  pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX,IDC_END_Y2,  IDC_END_Y_UNIT2,  m_Yend2,  pDisplayUnits->ComponentDim);

   DDX_Text(pDX,IDC_NSTRANDS_X,m_nStrandsX);
   DDX_Text(pDX,IDC_NSTRANDS_Y,m_nStrandsY);

   DDV_GreaterThanZero(pDX,IDC_NSTRANDS_X,m_nStrandsX);
   DDV_GreaterThanZero(pDX,IDC_NSTRANDS_Y,m_nStrandsY);

   DDX_CBEnum(pDX,IDC_STRAND_GENERATION,m_StrandGenerationType);
   DDX_CBEnum(pDX,IDC_LAYOUT,m_LayoutType);
   DDX_CBIndex(pDX,IDC_STRAND_TYPE,m_StrandType);

   DDX_Check_Bool(pDX,IDC_DELETE, m_bDelete);

   CString image_name = GetImageName(m_LayoutType,m_StrandGenerationType);
	DDX_MetaFileStatic(pDX, IDC_SCHEMATIC, m_Schematic, image_name, _T("Metafile") );
}


BEGIN_MESSAGE_MAP(CStrandGenerationDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_LAYOUT, &CStrandGenerationDlg::OnLayoutTypeChanged)
   ON_CBN_SELCHANGE(IDC_STRAND_GENERATION, &CStrandGenerationDlg::OnStrandGenerationTypeChanged)
   ON_CBN_SELCHANGE(IDC_STRAND_TYPE, &CStrandGenerationDlg::OnStrandTypeChanged)
   ON_BN_CLICKED(IDC_HELP, &CStrandGenerationDlg::OnHelp)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


// CStrandGenerationDlg message handlers

BOOL CStrandGenerationDlg::OnInitDialog()
{
   CComboBox* pcbStrandType = (CComboBox*)GetDlgItem(IDC_STRAND_TYPE);
   int idx = pcbStrandType->AddString("Straight");
   idx = pcbStrandType->AddString("Harped");
   pcbStrandType->SetCurSel(0);

   CComboBox* pcbGenerationTypes = (CComboBox*)GetDlgItem(IDC_STRAND_GENERATION);
   idx = pcbGenerationTypes->AddString("Sequential");
   pcbGenerationTypes->SetItemData(idx,(DWORD_PTR)sgSequential);
   idx = pcbGenerationTypes->AddString("Skipped");
   pcbGenerationTypes->SetItemData(idx,(DWORD_PTR)sgSkipped);

   CComboBox* pcbLayoutTypes = (CComboBox*)GetDlgItem(IDC_LAYOUT);
   idx = pcbLayoutTypes->AddString("Start Point and Horizontal and Vertical Spacing");
   pcbLayoutTypes->SetItemData(idx,(DWORD_PTR)ltSpacing);
   idx = pcbLayoutTypes->AddString("Start and End Points");
   pcbLayoutTypes->SetItemData(idx,(DWORD_PTR)ltStartEndPoint);

   CDialog::OnInitDialog();

   OnStrandTypeChanged();
   OnLayoutTypeChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CStrandGenerationDlg::OnLayoutTypeChanged()
{
   CComboBox* pcbLayoutTypes = (CComboBox*)GetDlgItem(IDC_LAYOUT);
   int curSel = pcbLayoutTypes->GetCurSel();
   LayoutType layout = (LayoutType)pcbLayoutTypes->GetItemData(curSel);
   if ( layout == ltSpacing )
   {
      GetDlgItem(IDC_END_LABEL)->SetWindowText(_T("Spacing"));
      GetDlgItem(IDC_END_LABEL2)->SetWindowText(_T("Spacing"));
   }
   else
   {
      GetDlgItem(IDC_END_LABEL)->SetWindowText(_T("End"));
      GetDlgItem(IDC_END_LABEL2)->SetWindowText(_T("End"));
   }

   UpdateSchematic();
}

void CStrandGenerationDlg::OnStrandGenerationTypeChanged()
{
   UpdateSchematic();
}

void CStrandGenerationDlg::OnStrandTypeChanged()
{
   CComboBox* pcbStrandType = (CComboBox*)GetDlgItem(IDC_STRAND_TYPE);
   int curSel = pcbStrandType->GetCurSel();

   BOOL bEnableHarped;
   if ( curSel == 0 )
   {
      // Straight
      bEnableHarped = FALSE;
      GetDlgItem(IDC_GROUP1)->SetWindowText(_T("Straight Strands"));
      GetDlgItem(IDC_GROUP2)->SetWindowText(_T(""));
      GetDlgItem(IDC_DELETE)->SetWindowText(_T("Delete previously defined straight strands"));
   }
   else
   {
      // Harped
      bEnableHarped = TRUE;
      GetDlgItem(IDC_GROUP1)->SetWindowText(_T("Harped Strands at End"));
      GetDlgItem(IDC_GROUP2)->SetWindowText(_T("Harped Strands at Harping Point"));
      GetDlgItem(IDC_DELETE)->SetWindowText(_T("Delete previously defined harped strands"));
   }

   GetDlgItem(IDC_GROUP2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_HEADING2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_X_LABEL2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_Y_LABEL2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_START_LABEL2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_START_X2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_START_X_UNIT2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_START_Y2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_START_Y_UNIT2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_END_LABEL2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_END_X2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_END_X_UNIT2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_END_Y2)->EnableWindow(bEnableHarped);
   GetDlgItem(IDC_END_Y_UNIT2)->EnableWindow(bEnableHarped);
}

CString CStrandGenerationDlg::GetImageName(LayoutType layoutType,StrandGenerationType generationType)
{
   CString strName;
   if ( layoutType == ltSpacing )
   {
      if ( generationType == sgSequential )
      {
         strName = "GENERATESTRANDS_STARTSPACING_SEQUENTIAL";
      }
      else
      {
         strName = "GENERATESTRANDS_STARTSPACING_SKIPPED";
      }
   }
   else
   {
      if ( generationType == sgSequential )
      {
         strName = "GENERATESTRANDS_STARTEND_SEQUENTIAL";
      }
      else
      {
         strName = "GENERATESTRANDS_STARTEND_SKIPPED";
      }
   }

   return strName;
}

void CStrandGenerationDlg::UpdateSchematic()
{
   CComboBox* pcbLayoutTypes = (CComboBox*)GetDlgItem(IDC_LAYOUT);
   int curSel = pcbLayoutTypes->GetCurSel();
   LayoutType layoutType = (LayoutType)pcbLayoutTypes->GetItemData(curSel);

   CComboBox* pcbGenerationTypes = (CComboBox*)GetDlgItem(IDC_STRAND_GENERATION);
   curSel = pcbGenerationTypes->GetCurSel();
   StrandGenerationType generationType = (StrandGenerationType)pcbGenerationTypes->GetItemData(curSel);

   CString strSchematic = GetImageName(layoutType,generationType);

	m_Schematic.SetImage(strSchematic, _T("Metafile") );
}

LRESULT CStrandGenerationDlg::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GENERATE_STRANDS );

   return TRUE;
}
