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

// BearingSame4BridgeDlg.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "BearingSame4BridgeDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CBearingSame4BridgeDlg dialog

IMPLEMENT_DYNAMIC(CBearingSame4BridgeDlg, CDialog)

CBearingSame4BridgeDlg::CBearingSame4BridgeDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CBearingSame4BridgeDlg::IDD, pParent)
{

}

CBearingSame4BridgeDlg::~CBearingSame4BridgeDlg()
{
}

BOOL CBearingSame4BridgeDlg::OnInitDialog()
{
   // load bearing count
   CString cntstr;
   CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT);
   for(int i=1; i<=MAX_BEARING_CNT; i++)
   {
      cntstr.Format(_T("%d"), i);
      int idx = pctrl->AddString(cntstr);
      pctrl->SetItemData(idx, i);
   }

   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


void CBearingSame4BridgeDlg::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_CBIndex(pDX, IDC_BRG_SHAPE, (int&)m_BearingData.Shape);
   DDX_CBItemData(pDX, IDC_BRG_COUNT, m_BearingData.BearingCount);
   if (!(pDX->m_bSaveAndValidate && m_BearingData.BearingCount==1))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_SPACING, IDC_BRG_SPACING_UNIT, m_BearingData.Spacing, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SPACING, m_BearingData.Spacing, pDisplayUnits->GetComponentDimUnit());
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_LENGTH, IDC_BRG_LENGTH_UNIT, m_BearingData.Length, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueGreaterThanZero(pDX, IDC_BRG_LENGTH, m_BearingData.Length, pDisplayUnits->GetComponentDimUnit() );
   if (!(pDX->m_bSaveAndValidate && m_BearingData.Shape==bsRound))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_WIDTH, IDC_BRG_WIDTH_UNIT, m_BearingData.Width, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_WIDTH, m_BearingData.Width, pDisplayUnits->GetComponentDimUnit() );
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_HEIGHT, IDC_BRG_HEIGHT_UNIT, m_BearingData.Height, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_HEIGHT, m_BearingData.Height, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS, IDC_BRG_RECESS_UNIT, m_BearingData.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS, m_BearingData.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS_LENGTH, IDC_BRG_RECESS_LENGTH_UNIT, m_BearingData.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS_LENGTH, m_BearingData.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_SOLEPLATE, IDC_BRG_SOLEPLATE_UNIT, m_BearingData.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SOLEPLATE, m_BearingData.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );

	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CBearingSame4BridgeDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_BRG_SHAPE, &CBearingSame4BridgeDlg::OnCbnSelchangeBrgShape)
   ON_CBN_SELCHANGE(IDC_BRG_COUNT, &CBearingSame4BridgeDlg::OnCbnSelchangeBrgCount)
END_MESSAGE_MAP()

void CBearingSame4BridgeDlg::UploadData(const BearingInputData& rData)
{
   m_BearingData = rData.m_SingleBearing;

   UpdateData(FALSE);

   OnCbnSelchangeBrgShape();
   OnCbnSelchangeBrgCount();
}

void CBearingSame4BridgeDlg::DownloadData(BearingInputData* pData,CDataExchange* pDX)
{
   if (!UpdateData(TRUE))
   {
      // pass failure up the chain
      pDX->Fail();
   }

   pData->m_BearingType = pgsTypes::brtBridge;
   pData->m_SingleBearing = m_BearingData;
}

void CBearingSame4BridgeDlg::OnCbnSelchangeBrgShape()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_BRG_SHAPE);
   BearingShape shp = (BearingShape)ptype_ctrl->GetCurSel();
   ATLASSERT(shp!=CB_ERR);

   // Hide width for circular bearings
   int show = (shp == bsRectangular) ? SW_SHOW : SW_HIDE;
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_BRG_WIDTH);
   pwnd->ShowWindow(show);
   pwnd = (CWnd*)GetDlgItem(IDC_BRG_WIDTH_UNIT);
   pwnd->ShowWindow(show);
   pwnd = (CWnd*)GetDlgItem(IDC_BRG_WIDTH_STATIC);
   pwnd->ShowWindow(show);


   // Change label for length depending on shape
   pwnd = (CWnd*)GetDlgItem(IDC_BRG_LENGTH_STATIC);
   if (shp == bsRectangular)
   {
      pwnd->SetWindowText(_T("Length"));
   }
   else
   {
      pwnd->SetWindowText(_T("Diameter"));
   }
}

void CBearingSame4BridgeDlg::OnCbnSelchangeBrgCount()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT);
   int idx = ptype_ctrl->GetCurSel();
   ATLASSERT(idx!=CB_ERR);
   Uint32 cnt = (Uint32)ptype_ctrl->GetItemData(idx);

   // Hide width for circular bearings
   BOOL benable = (cnt > 1) ? SW_SHOW : SW_HIDE;
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_BRG_SPACING);
   pwnd->EnableWindow(benable);
   pwnd = (CWnd*)GetDlgItem(IDC_BRG_SPACING_UNIT);
   pwnd->EnableWindow(benable);
   pwnd = (CWnd*)GetDlgItem(IDC_BRG_SPACING_STATIC);
   pwnd->EnableWindow(benable);
}
