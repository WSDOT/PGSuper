///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// HaunchSame4Bridge.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "FilletByGirderDlg.h"


// CFilletByGirderDlg dialog

IMPLEMENT_DYNAMIC(CFilletByGirderDlg, CDialog)

CFilletByGirderDlg::CFilletByGirderDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CFilletByGirderDlg::IDD, pParent),
   m_bFirstActive(true)
{

}

CFilletByGirderDlg::~CFilletByGirderDlg()
{
}

void CFilletByGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFilletByGirderDlg, CDialog)
END_MESSAGE_MAP()

BOOL CFilletByGirderDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

	m_Grid.SubclassDlgItem(IDC_HAUNCH_GRID, this);
   m_Grid.CustomInit(m_NumSpans,m_MaxGirdersPerSpan);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CFilletByGirderDlg::UploadData(const HaunchInputData& rData)
{
   m_Grid.FillGrid(rData);
}

void CFilletByGirderDlg::DownloadData(HaunchInputData* pData, CDataExchange* pDX)
{
   m_Grid.GetData(pData, pDX);
}

void CFilletByGirderDlg::InitSize(const HaunchInputData& data)
{
   m_NumSpans = data.m_FilletSpans.size();
   m_MaxGirdersPerSpan = data.m_MaxGirdersPerSpan;
}