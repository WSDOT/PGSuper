///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "PierDetailsBearingsPage.h"
#include "EditBearingDlg.h"
#include "PierDetailsDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// PierDetailsBearingsPage.cpp : implementation file
//
   static const int hdrctls[] ={IDC_STATIC_B1, IDC_STATIC_B2,  IDC_BRG_SPACING_STATIC, IDC_BRG_SPACING_UNIT, 
                               IDC_BRG_LENGTH_STATIC, IDC_BRG_LENGTH_UNIT, IDC_BRG_WIDTH_STATIC, IDC_BRG_WIDTH_UNIT, 
                               IDC_STATIC_B3, IDC_BRG_HEIGHT_UNIT, IDC_STATIC_B4, IDC_BRG_RECESS_UNIT, 
                               IDC_STATIC_B6, IDC_BRG_SOLEPLATE_UNIT, IDC_STATIC_B5, IDC_BRG_RECESS_LENGTH_UNIT, -1 };

   static const int ctls1[] =  {IDC_BRG_SHAPE_1, IDC_BRG_SPACING_1,  IDC_BRG_LENGTH_1, IDC_BRG_WIDTH_1, IDC_BRG_HEIGHT_1,
                               IDC_BRG_RECESS_1, IDC_BRG_SOLEPLATE_1, IDC_BRG_COUNT_1, IDC_BRG_RECESS_LENGTH_1, IDC_BRG1_STATIC, -1 };

   static const int ctls2[] =  {IDC_BRG_SHAPE_2, IDC_BRG_SPACING_2,  IDC_BRG_LENGTH_2, IDC_BRG_WIDTH_2, IDC_BRG_HEIGHT_2,
                               IDC_BRG_RECESS_2, IDC_BRG_SOLEPLATE_2, IDC_BRG_COUNT_2, IDC_BRG_RECESS_LENGTH_2, IDC_BRG2_STATIC, -1 };

IMPLEMENT_DYNAMIC(CPierDetailsBearingsPage, CPropertyPage)

CPierDetailsBearingsPage::CPierDetailsBearingsPage()
	: CPropertyPage(IDD_PIER_BEARINGS_PAGE),
   m_IsMsgFromMe(false)
{
}

CPierDetailsBearingsPage::~CPierDetailsBearingsPage()
{
}

void CPierDetailsBearingsPage::Initialize(const CBridgeDescription2* pBridge, const CPierData2* pPier)
{
   m_PierIdx = pPier->GetIndex();
   m_IsBoundaryPier = pPier->IsBoundaryPier();
   m_IsAbutment = pPier->IsAbutment();

   // Load data from parent
   m_BearingInputData.CopyFromBridgeDescription(pBridge);

   // need to display data in first bearing line for aesthetics
   m_bStoredSingleBL = m_IsAbutment || !m_IsBoundaryPier;

   UpdateLocalData();
}

void CPierDetailsBearingsPage::UpdateLocalData()
{
   // make copy into local data structures for this pier
   for (const auto& brpierdat : m_BearingInputData.m_Bearings)
   {
      if (brpierdat.m_PierIndex == m_PierIdx)
      {
         if (m_bStoredSingleBL || brpierdat.m_BPDType != BearingPierData::bpdAhead )
         {
            // pier data is in girderline 0
            m_Bearings[0] = brpierdat.m_BearingsForGirders[0]; 
         }
         else
         {
            m_Bearings[1] = brpierdat.m_BearingsForGirders[0];
         }
      }
   }
}

void CPierDetailsBearingsPage::SaveData()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   m_BearingInputData.m_BearingType = (pgsTypes::BearingType)pBox->GetItemData(pBox->GetCurSel());

   // Only save bearing data in controls if bearing type is pier. However, we will need to save all
   // data unless a call to OnEdit() took place previously.
   if (m_BearingInputData.m_BearingType == pgsTypes::brtPier)
   {
      // Opposite of Initialize above
      for (auto& brpierdat : m_BearingInputData.m_Bearings)
      {
         if (brpierdat.m_PierIndex == m_PierIdx)
         {
            if (m_bStoredSingleBL || brpierdat.m_BPDType != BearingPierData::bpdAhead)
            {
               // copy pier data to all girders in bearing line
               brpierdat.m_BearingsForGirders.assign(brpierdat.m_BearingsForGirders.size(), m_Bearings[0]);
            }
            else
            {
               brpierdat.m_BearingsForGirders.assign(brpierdat.m_BearingsForGirders.size(), m_Bearings[1]);
            }
         }
      }
   }

   if (!m_IsMsgFromMe)
   {
      // Dialog is closing, copy data to bridge
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CPierDetailsDlg)));

      m_BearingInputData.CopyToBridgeDescription(&pParent->m_BridgeDesc);
   }
}

BOOL CPierDetailsBearingsPage::OnInitDialog()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CPierDetailsDlg)) );

   // load bearing counts
   CString cntstr;
   CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT_1); // top
   for(int i=1; i<=MAX_BEARING_CNT; i++)
   {
      cntstr.Format(_T("%d"), i);
      int idx = pctrl->AddString(cntstr);
      pctrl->SetItemData(idx, i);
   }

   pctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT_2); // bottom
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

   ShowCtrls();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierDetailsBearingsPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_MetaFileStatic(pDX, IDC_BEARING, m_Bearing, _T("BEARINGDIMENSIONS"), _T("Metafile"), EMF_FIT);

   DDX_CBItemData(pDX, IDC_BRG_TYPE, m_BearingInputData.m_BearingType);

   // First line of data
   CBearingData2& B1Dat = m_Bearings[0];

   DDX_CBIndex(pDX, IDC_BRG_SHAPE_1, (int&)B1Dat.Shape);
   DDX_CBItemData(pDX, IDC_BRG_COUNT_1, B1Dat.BearingCount);
   if (!(pDX->m_bSaveAndValidate && B1Dat.BearingCount==1))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_SPACING_1, IDC_BRG_SPACING_UNIT, B1Dat.Spacing, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SPACING_1, B1Dat.Spacing, pDisplayUnits->GetComponentDimUnit());
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_LENGTH_1, IDC_BRG_LENGTH_UNIT, B1Dat.Length, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueGreaterThanZero(pDX, IDC_BRG_LENGTH_1, B1Dat.Length, pDisplayUnits->GetComponentDimUnit() );
   if (!(pDX->m_bSaveAndValidate && B1Dat.Shape==bsRound))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_WIDTH_1, IDC_BRG_WIDTH_UNIT, B1Dat.Width, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_WIDTH_1, B1Dat.Width, pDisplayUnits->GetComponentDimUnit() );
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_HEIGHT_1, IDC_BRG_HEIGHT_UNIT, B1Dat.Height, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_HEIGHT_1, B1Dat.Height, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS_1, IDC_BRG_RECESS_UNIT, B1Dat.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS_1, B1Dat.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS_LENGTH_1, IDC_BRG_RECESS_LENGTH_UNIT, B1Dat.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS_LENGTH_1, B1Dat.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_SOLEPLATE_1, IDC_BRG_SOLEPLATE_UNIT, B1Dat.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SOLEPLATE_1, B1Dat.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );

   // Second line of data
   CBearingData2& B2Dat = m_Bearings[1];

   DDX_CBIndex(pDX, IDC_BRG_SHAPE_2, (int&)B2Dat.Shape);
   DDX_CBItemData(pDX, IDC_BRG_COUNT_2, B2Dat.BearingCount);
   if (!(pDX->m_bSaveAndValidate && B2Dat.BearingCount==2))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_SPACING_2, IDC_BRG_SPACING_UNIT, B2Dat.Spacing, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SPACING_2, B2Dat.Spacing, pDisplayUnits->GetComponentDimUnit());
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_LENGTH_2, IDC_BRG_LENGTH_UNIT, B2Dat.Length, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueGreaterThanZero(pDX, IDC_BRG_LENGTH_2, B2Dat.Length, pDisplayUnits->GetComponentDimUnit() );
   if (!(pDX->m_bSaveAndValidate && B2Dat.Shape==bsRound))
   {
      DDX_UnitValueAndTag( pDX, IDC_BRG_WIDTH_2, IDC_BRG_WIDTH_UNIT, B2Dat.Width, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore(pDX, IDC_BRG_WIDTH_2, B2Dat.Width, pDisplayUnits->GetComponentDimUnit() );
   }

   DDX_UnitValueAndTag( pDX, IDC_BRG_HEIGHT_2, IDC_BRG_HEIGHT_UNIT, B2Dat.Height, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_HEIGHT_2, B2Dat.Height, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS_2, IDC_BRG_RECESS_UNIT, B2Dat.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS_2, B2Dat.RecessHeight, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_RECESS_LENGTH_2, IDC_BRG_RECESS_LENGTH_UNIT, B2Dat.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_RECESS_LENGTH_2, B2Dat.RecessLength, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_BRG_SOLEPLATE_2, IDC_BRG_SOLEPLATE_UNIT, B2Dat.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BRG_SOLEPLATE_2, B2Dat.SolePlateHeight, pDisplayUnits->GetComponentDimUnit() );

	CPropertyPage::DoDataExchange(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      ShowCtrls();
   }
   else
   {
      // save to bearing data, and maybe to bridge
      SaveData();
   }
 }

BEGIN_MESSAGE_MAP(CPierDetailsBearingsPage, CPropertyPage)
   ON_BN_CLICKED(IDC_EDIT_BEARINGS, &CPierDetailsBearingsPage::OnBnClickedEditBearings)
   ON_CBN_SELCHANGE(IDC_BRG_TYPE, &CPierDetailsBearingsPage::OnCbnSelchangeBrgType)
   ON_CBN_SELCHANGE(IDC_BRG_SHAPE_1, &CPierDetailsBearingsPage::OnCbnSelchangeBrgShape1)
   ON_CBN_SELCHANGE(IDC_BRG_COUNT_1, &CPierDetailsBearingsPage::OnCbnSelchangeBrgCount1)
   ON_CBN_SELCHANGE(IDC_BRG_SHAPE_2, &CPierDetailsBearingsPage::OnCbnSelchangeBrgShape2)
   ON_CBN_SELCHANGE(IDC_BRG_COUNT_2, &CPierDetailsBearingsPage::OnCbnSelchangeBrgCount2)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CPierDetailsBearingsPage message handlers

void CPierDetailsBearingsPage::OnBnClickedEditBearings()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   m_BearingInputData.m_BearingType = (pgsTypes::BearingType)pBox->GetItemData(pBox->GetCurSel());
   if (m_BearingInputData.m_BearingType == pgsTypes::brtPier)
   {
      // save current bearing data if for pier
      SimpleMutex mutex(m_IsMsgFromMe); // dodataexchange does not save data to bridge, only locally
      UpdateData(TRUE);
   }

   CEditBearingDlg dlg(&m_BearingInputData);
   if (dlg.DoModal() == IDOK)
   {
      m_BearingInputData = dlg.m_BearingInputData;
      UpdateLocalData();
      UpdateData(FALSE);
   }
}

void CPierDetailsBearingsPage::OnCbnSelchangeBrgType()
{
   ShowCtrls();
}

void  CPierDetailsBearingsPage::ShowCtrls()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   m_BearingInputData.m_BearingType = (pgsTypes::BearingType)pBox->GetItemData(pBox->GetCurSel());

   int swh = m_BearingInputData.m_BearingType == pgsTypes::brtPier ? SW_SHOW : SW_HIDE; // show header for all pier configurations
   int sw1 = swh; // first line always shown
   int sw2 = (swh == SW_SHOW && !m_bStoredSingleBL) ? SW_SHOW : SW_HIDE; //  show second line of data for boundary piers and first abutment

   int i = 0;
   while (hdrctls[i] != -1)
   {
      CWnd* pWnd = this->GetDlgItem(hdrctls[i++]);
      pWnd->ShowWindow(swh);
   }

   i = 0;
   while (ctls1[i] != -1)
   {
      CWnd* pWnd = this->GetDlgItem(ctls1[i++]);
      pWnd->ShowWindow(sw1);
   }

   i = 0;
   while (ctls2[i] != -1)
   {
      CWnd* pWnd = this->GetDlgItem(ctls2[i++]);
      pWnd->ShowWindow(sw2);
   }

   // Titles for bearing lines
   CWnd* pWnd1 =(CComboBox*)GetDlgItem(IDC_BRG1_STATIC);
   CWnd* pWnd2 =(CComboBox*)GetDlgItem(IDC_BRG2_STATIC);
   if (!m_IsBoundaryPier)
   {
      pWnd1->SetWindowText(_T("C.L.")); // single bearing line at interior piers
   }
   else if (m_IsAbutment && m_PierIdx == 0)
   {
      pWnd2->SetWindowText(_T("Back"));
   }
   else
   {
      pWnd1->SetWindowText(_T("Back"));
      pWnd2->SetWindowText(_T("Ahead"));
   }

   if (sw1 == SW_SHOW)
   {
      OnCbnSelchangeBrgShape1();
      OnCbnSelchangeBrgCount1();
   }

   if (sw2 == SW_SHOW)
   {
      OnCbnSelchangeBrgShape2();
      OnCbnSelchangeBrgCount2();
   }

   GetDlgItem(IDC_BEARING)->ShowWindow(swh);

   // Edit button is opposite
   int sb = swh == SW_SHOW ? SW_HIDE : SW_SHOW;
   this->GetDlgItem(IDC_EDIT_BEARINGS)->ShowWindow(sb);
}

void CPierDetailsBearingsPage::OnCbnSelchangeBrgShape1()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_BRG_SHAPE_1);
   BearingShape shp = (BearingShape)ptype_ctrl->GetCurSel();
   ATLASSERT(shp!=CB_ERR);

   // Hide width for circular bearings
   BOOL benable = (shp == bsRectangular) ? TRUE : FALSE;
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_BRG_WIDTH_1);
   pwnd->EnableWindow(benable);
}

void CPierDetailsBearingsPage::OnCbnSelchangeBrgCount1()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT_1);
   int idx = ptype_ctrl->GetCurSel();
   ATLASSERT(idx!=CB_ERR);
   Uint32 cnt = (Uint32)ptype_ctrl->GetItemData(idx);

   BOOL benable = (cnt > 1) ? TRUE : FALSE;
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_BRG_SPACING_1);
   pwnd->EnableWindow(benable);
}

void CPierDetailsBearingsPage::OnCbnSelchangeBrgShape2()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_BRG_SHAPE_2);
   BearingShape shp = (BearingShape)ptype_ctrl->GetCurSel();
   ATLASSERT(shp!=CB_ERR);

   // Hide width for circular bearings
   BOOL benable = (shp == bsRectangular) ? TRUE : FALSE;
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_BRG_WIDTH_2);
   pwnd->EnableWindow(benable);
}

void CPierDetailsBearingsPage::OnCbnSelchangeBrgCount2()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_BRG_COUNT_2);
   int idx = ptype_ctrl->GetCurSel();
   ATLASSERT(idx!=CB_ERR);
   Uint32 cnt = (Uint32)ptype_ctrl->GetItemData(idx);

   BOOL benable = (cnt > 1) ? TRUE : FALSE;
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_BRG_SPACING_2);
   pwnd->EnableWindow(benable);
}

void CPierDetailsBearingsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_BEARINGS);
}