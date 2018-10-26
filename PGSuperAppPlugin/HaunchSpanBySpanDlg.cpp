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
#include "HaunchSpanBySpanDlg.h"


// CHaunchSpanBySpanDlg dialog

IMPLEMENT_DYNAMIC(CHaunchSpanBySpanDlg, CDialog)

CHaunchSpanBySpanDlg::CHaunchSpanBySpanDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHaunchSpanBySpanDlg::IDD, pParent),
   m_bFirstActive(true)
{

}

CHaunchSpanBySpanDlg::~CHaunchSpanBySpanDlg()
{
}

void CHaunchSpanBySpanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHaunchSpanBySpanDlg, CDialog)
END_MESSAGE_MAP()


BOOL CHaunchSpanBySpanDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

	m_Grid.SubclassDlgItem(IDC_HAUNCH_GRID, this);
   m_Grid.CustomInit();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CHaunchSpanBySpanDlg::UploadData(const HaunchInputData& rData)
{
   m_Grid.FillGrid(rData);
}

HaunchInputData CHaunchSpanBySpanDlg::DownloadData(Float64 minA,CString& minValError, CDataExchange* pDX)
{
   return m_Grid.GetData(minA, minValError, pDX);
}


