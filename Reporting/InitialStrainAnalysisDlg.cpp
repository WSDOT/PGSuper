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
//
// InitialStrainAnalysisDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "InitialStrainAnalysisDlg.h"

#include <PgsExt\GirderLabel.h>
#include <MFCTools\CustomDDX.h>

#include <IFace\Intervals.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CInitialStrainAnalysisDlg dialog

IMPLEMENT_DYNAMIC(CInitialStrainAnalysisDlg, CDialog)

CInitialStrainAnalysisDlg::CInitialStrainAnalysisDlg(IBroker* pBroker,std::shared_ptr<CInitialStrainAnalysisReportSpecification>& pRptSpec,const CGirderKey& initialGirderKey,IntervalIndexType intervalIdx,CWnd* pParent)
	: CDialog(CInitialStrainAnalysisDlg::IDD, pParent)
   , m_pRptSpec(pRptSpec)
{
   m_GirderKey = initialGirderKey;
   m_IntervalIdx = intervalIdx;
   m_pBroker = pBroker;
}

CInitialStrainAnalysisDlg::~CInitialStrainAnalysisDlg()
{
}

void CInitialStrainAnalysisDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_CBItemData(pDX, IDC_INTERVAL, m_IntervalIdx);

#pragma Reminder("UPDATE: Need UI and DDX for Girder")
   // right now, the girderKey is set and can't be changed
}

BEGIN_MESSAGE_MAP(CInitialStrainAnalysisDlg, CDialog)
END_MESSAGE_MAP()

BOOL CInitialStrainAnalysisDlg::OnInitDialog()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IIntervals, pIntervals);
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_INTERVAL);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
      int idx = pcbIntervals->AddString(strLabel);
      pcbIntervals->SetItemData(idx,(DWORD_PTR)intervalIdx);
   }

   CDialog::OnInitDialog();

   if ( m_pRptSpec )
      InitFromRptSpec();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

const CGirderKey& CInitialStrainAnalysisDlg::GetGirderKey()
{
   return m_GirderKey;
}

IntervalIndexType CInitialStrainAnalysisDlg::GetInterval()
{
   return m_IntervalIdx;
}

void CInitialStrainAnalysisDlg::InitFromRptSpec()
{
   m_GirderKey = m_pRptSpec->GetGirderKey();
   m_IntervalIdx = m_pRptSpec->GetInterval();
   UpdateData(FALSE);
}
