///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "BridgeDescrBearings.h"
#include "BridgeDescDlg.h"
#include "EditBearingDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include "PGSuperUnits.h"

// CBridgeDescrBearings dialog

IMPLEMENT_DYNAMIC(CBridgeDescrBearings, CPropertyPage)


CBridgeDescrBearings::CBridgeDescrBearings()
	: CPropertyPage(IDD_BRIDGEDESC_BEARING),
   m_IsMsgFromMe(false)
{
}

CBridgeDescrBearings::~CBridgeDescrBearings()
{
}

BOOL CBridgeDescrBearings::OnInitDialog()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   // Load data from parent
   m_BearingInputData.CopyFromBridgeDescription(&pParent->m_BridgeDesc);

   // load bearing count
   CString cntstr;
   CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT);
   for(int i=1; i<=MAX_BEARING_CNT; i++)
   {
      cntstr.Format(_T("%d"), i);
      int idx = pctrl->AddString(cntstr);
      pctrl->SetItemData(idx, i);
   }

   // Fill options in Slab offset type combo
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   int sqidx = pBox->AddString( BearingTypeAsString(pgsTypes::brtBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::brtBridge);
   sqidx = pBox->AddString( BearingTypeAsString(pgsTypes::brtPier));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::brtPier);
   sqidx = pBox->AddString( BearingTypeAsString(pgsTypes::brtGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::brtGirder);

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescrBearings::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_CBItemData(pDX, IDC_BRG_TYPE, m_BearingInputData.m_BearingType);

   DDX_CBIndex(pDX, IDC_BRG_SHAPE, (int&)m_BearingInputData.m_SingleBearing.Shape);
   DDX_CBItemData(pDX, IDC_BRG_COUNT, m_BearingInputData.m_SingleBearing.BearingCount);
   if (!(pDX->m_bSaveAndValidate && m_BearingInputData.m_SingleBearing.BearingCount==1))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_SPACING, IDC_BRG_SPACING_UNIT, m_BearingInputData.m_SingleBearing.Spacing, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SPACING, m_BearingInputData.m_SingleBearing.Spacing, pDisplayUnits->GetComponentDimUnit());
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_LENGTH, IDC_BRG_LENGTH_UNIT, m_BearingInputData.m_SingleBearing.Length, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueGreaterThanZero(pDX, IDC_BRG_LENGTH, m_BearingInputData.m_SingleBearing.Length, pDisplayUnits->GetComponentDimUnit() );
   if (!(pDX->m_bSaveAndValidate && m_BearingInputData.m_SingleBearing.Shape==bsRound))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_WIDTH, IDC_BRG_WIDTH_UNIT, m_BearingInputData.m_SingleBearing.Width, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_WIDTH, m_BearingInputData.m_SingleBearing.Width, pDisplayUnits->GetComponentDimUnit() );
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_HEIGHT, IDC_BRG_HEIGHT_UNIT, m_BearingInputData.m_SingleBearing.Height, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_HEIGHT, m_BearingInputData.m_SingleBearing.Height, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS, IDC_BRG_RECESS_UNIT, m_BearingInputData.m_SingleBearing.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS, m_BearingInputData.m_SingleBearing.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS_LENGTH, IDC_BRG_RECESS_LENGTH_UNIT, m_BearingInputData.m_SingleBearing.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS_LENGTH, m_BearingInputData.m_SingleBearing.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_SOLEPLATE, IDC_BRG_SOLEPLATE_UNIT, m_BearingInputData.m_SingleBearing.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SOLEPLATE, m_BearingInputData.m_SingleBearing.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );

	CPropertyPage::DoDataExchange(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      OnCbnSelchangeBrgType();
   }
   else
   {
      if (!m_IsMsgFromMe)
      {
         // page headed out of scope - save data
         CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
         ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

         m_BearingInputData.CopyToBridgeDescription(&pParent->m_BridgeDesc);
      }
   }
}

BEGIN_MESSAGE_MAP(CBridgeDescrBearings, CPropertyPage)
   ON_BN_CLICKED(IDC_EDIT_BEARINGS, &CBridgeDescrBearings::OnBnClickedEditBearings)
   ON_CBN_SELCHANGE(IDC_BRG_TYPE, &CBridgeDescrBearings::OnCbnSelchangeBrgType)
   ON_CBN_SELCHANGE(IDC_BRG_COUNT, &CBridgeDescrBearings::OnCbnSelchangeBrgCount)
   ON_CBN_SELCHANGE(IDC_BRG_SHAPE, &CBearingSame4BridgeDlg::OnCbnSelchangeBrgShape)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CBridgeDescrBearings message handlers
void CBridgeDescrBearings::OnBnClickedEditBearings()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   m_BearingInputData.m_BearingType = (pgsTypes::BearingType)pBox->GetItemData(pBox->GetCurSel());
   if (m_BearingInputData.m_BearingType == pgsTypes::sotBridge)
   {
      SimpleMutex mutex(m_IsMsgFromMe); // dodataexchange does not save data to bridge, only locally
      UpdateData(TRUE);
   }

   CEditBearingDlg dlg(&m_BearingInputData);
   if (dlg.DoModal() == IDOK)
   {
      m_BearingInputData = dlg.m_BearingInputData;
      UpdateData(FALSE);
   }
}

void CBridgeDescrBearings::OnCbnSelchangeBrgType()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   m_BearingInputData.m_BearingType = (pgsTypes::BearingType)pBox->GetItemData(pBox->GetCurSel());

   int sw = m_BearingInputData.m_BearingType == pgsTypes::sotBridge ? SW_SHOW : SW_HIDE;

   const int ctls[] = { IDC_BRG_SHAPE, IDC_STATIC_B1, IDC_STATIC_B2, IDC_BRG_SPACING, IDC_BRG_SPACING_STATIC, IDC_BRG_SPACING_UNIT, IDC_BRG_LENGTH,
                IDC_BRG_LENGTH_STATIC, IDC_BRG_LENGTH_UNIT, IDC_BRG_WIDTH, IDC_BRG_WIDTH_STATIC, IDC_BRG_WIDTH_UNIT, IDC_BRG_HEIGHT,
                IDC_STATIC_B3, IDC_BRG_HEIGHT_UNIT, IDC_BRG_RECESS, IDC_STATIC_B4, IDC_BRG_RECESS_UNIT, IDC_BRG_SOLEPLATE,
                IDC_STATIC_B6, IDC_BRG_SOLEPLATE_UNIT, IDC_BRG_COUNT, IDC_BRG_RECESS_LENGTH, IDC_STATIC_B5, IDC_BRG_RECESS_LENGTH_UNIT, -1 };
   int i = 0;
   while (ctls[i] != -1)
   {
      CWnd* pWnd = this->GetDlgItem(ctls[i++]);
      pWnd->ShowWindow(sw);
   }

   if (sw == SW_SHOW)
   {
      OnCbnSelchangeBrgShape();
      OnCbnSelchangeBrgCount();
   }

   // Edit button is opposite
   int sb = sw == SW_SHOW ? SW_HIDE : SW_SHOW;
   this->GetDlgItem(IDC_EDIT_BEARINGS)->ShowWindow(sb);
}

void CBridgeDescrBearings::OnCbnSelchangeBrgShape()
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

void CBridgeDescrBearings::OnCbnSelchangeBrgCount()
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

void CBridgeDescrBearings::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_BEARINGS);
}
