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

// TxDOTOptionalDesignStandardFillDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignStandardFillDlg.h"
#include <System\Tokenizer.h>
#include <MfcTools\CustomDDX.h>
#include <EAF\EAFDisplayUnits.h>
#include "EccentricityDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CTxDOTOptionalDesignStandardFillDlg dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignStandardFillDlg, CDialog)

CTxDOTOptionalDesignStandardFillDlg::CTxDOTOptionalDesignStandardFillDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CTxDOTOptionalDesignStandardFillDlg::IDD, pParent)
   , m_strNumStrands(_T(""))
   , m_NumStrands(0)
   , m_To(0)
   , m_bFirstActive(true)
{
   m_pGirderData = nullptr;
   m_pBrokerRetriever = nullptr;
}

CTxDOTOptionalDesignStandardFillDlg::~CTxDOTOptionalDesignStandardFillDlg()
{
}

BOOL CTxDOTOptionalDesignStandardFillDlg::OnInitDialog()
{
   // Load data into local members
   LoadDialogData();


   CDialog::OnInitDialog();


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignStandardFillDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_OPT_NUM_STRANDS, &CTxDOTOptionalDesignStandardFillDlg::OnCbnSelchangeOptNumStrands)
   ON_BN_CLICKED(IDC_OPT_COMPUTE, &CTxDOTOptionalDesignStandardFillDlg::OnBnClickedOptCompute)
   ON_WM_ERASEBKGND()
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CTxDOTOptionalDesignStandardFillDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      UpdateLibraryData();
   }

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
   if (pBroker==nullptr)
      return;

   DDX_CBStringExactCase(pDX, IDC_OPT_NUM_STRANDS, m_strNumStrands);

   bool st = WBFL::System::Tokenizer::ParseULong(m_strNumStrands, (unsigned long*)&m_NumStrands);  // save num strands as integral value as well
   ASSERT(st);

   // only parse To value if we have harped strands
   // it takes some effort here
   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(m_GirderEntryName));
   ASSERT(pGdrEntry!=nullptr);

   StrandIndexType numStraight(0), numHarped(0);
   if (pGdrEntry->GetPermStrandDistribution(m_NumStrands, &numStraight, &numHarped))
   {
      if (numHarped>0) 
      {
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
         DDX_UnitValueAndTag(pDX,IDC_OPT_TO, IDC_OPT_TO_UNITS, m_To, pDisplayUnits->GetComponentDimUnit() );

         if (pDX->m_bSaveAndValidate)
         {
            Float64 toLower, toUpper;
            m_pGirderData->ComputeToRange(pLib, m_NumStrands, &toLower, &toUpper);

            DDV_UnitValueRange( pDX,IDC_OPT_TO,m_To,toLower, toUpper, pDisplayUnits->GetComponentDimUnit() );
         }
      }
   }
   else
   {
      ATLASSERT(false); // should never happen
   }

   if (pDX->m_bSaveAndValidate)
   {
      SaveDialogData();
   }
   else
   {
      UpdateControls();
   }
}

void CTxDOTOptionalDesignStandardFillDlg::LoadDialogData()
{
   StrandIndexType ns = m_pGirderData->GetStrandCount();
   m_strNumStrands.Format(_T("%d"),ns);

   m_To = m_pGirderData->GetStrandTo();
}

void CTxDOTOptionalDesignStandardFillDlg::SaveDialogData()
{
   m_pGirderData->SetStrandFillType(CTxDOTOptionalDesignGirderData::sfStandard);
   m_pGirderData->SetNumStrands(m_NumStrands);
   m_pGirderData->SetStrandTo(m_To);
}

void CTxDOTOptionalDesignStandardFillDlg::Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* pBrokerRetriever, LPCTSTR entryName)
{
   m_pGirderData = pGirderData;
   m_pBrokerRetriever = pBrokerRetriever;
   m_GirderEntryName = entryName;
}

void CTxDOTOptionalDesignStandardFillDlg::SetGirderEntryName(LPCTSTR entryName)
{
   if (m_GirderEntryName != entryName)
   {
      m_GirderEntryName = entryName;

      UpdateLibraryData();
   }
}

// CTxDOTOptionalDesignStandardFillDlg message handlers

void CTxDOTOptionalDesignStandardFillDlg::OnCbnSelchangeOptNumStrands()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_OPT_NUM_STRANDS);
   int sel = pBox->GetCurSel();
   if (sel!=CB_ERR)
   {
      pBox->GetLBText(sel,m_strNumStrands);
      bool st = WBFL::System::Tokenizer::ParseULong(m_strNumStrands, (unsigned long*)&m_NumStrands);  // save num strands as integral value as well
      ASSERT(st);
   }
   else
   {
      m_strNumStrands=_T("0");
      m_NumStrands=0;
   }

   UpdateControls();
}

void CTxDOTOptionalDesignStandardFillDlg::UpdateControls()
{
   // Note - this function assumes that dialog data is updated

   // update number of depressed strands
   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(m_GirderEntryName));
   ASSERT(pGdrEntry!=nullptr);

   CWnd* pout = (CWnd*)GetDlgItem(IDC_OPT_NO_DEPRESSED);

   StrandIndexType numStraight(0), numHarped(0);
   if (!pGdrEntry->GetPermStrandDistribution(m_NumStrands, &numStraight, &numHarped))
   {
      ASSERT(0);
      pout->SetWindowText(_T("Error computing harped strands"));
   }
   else
   {
      CString msg;
      msg.Format(_T("(No. Depressed strands = %d)"), numHarped);
      pout->SetWindowText(msg);
   }

   // To Value range
   BOOL benable = numHarped>0 ? TRUE:FALSE;

   CWnd* pToEdit = GetDlgItem(IDC_OPT_TO);
   CWnd* pToUnit = GetDlgItem(IDC_OPT_TO_UNITS);
   CWnd* pToTag  = GetDlgItem(IDC_OPT_TO_TAG);
   CWnd* pToRange = GetDlgItem(IDC_OPT_TO_VALID_RANGE);
   CWnd* pCompute  = GetDlgItem(IDC_OPT_COMPUTE);

   pToEdit->EnableWindow(benable);
   pToUnit->EnableWindow(benable);
   pToTag->EnableWindow(benable);
   pToRange->ShowWindow(benable?SW_SHOW:SW_HIDE);

   if (benable)
   {
      CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 toLower, toUpper;
      m_pGirderData->ComputeToRange(pLib, m_NumStrands, &toLower, &toUpper);
      toLower = WBFL::Units::ConvertFromSysUnits(toLower, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      toUpper = WBFL::Units::ConvertFromSysUnits(toUpper, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      CString umsg;
      umsg.Format(_T("(Valid Range: %.3f to %.3f)"),toLower, toUpper);

      pToRange->SetWindowText(umsg);
   }

   // ecc compute button
   pCompute->EnableWindow(m_NumStrands>0 ? TRUE:FALSE);

}

void  CTxDOTOptionalDesignStandardFillDlg::UpdateLibraryData()
{
   InitStrandNoCtrl();
}

void CTxDOTOptionalDesignStandardFillDlg::InitStrandNoCtrl()
{
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_OPT_NUM_STRANDS );
   pList->ResetContent();

   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();
   CString str;
   std::vector<StrandIndexType> strands = m_pGirderData->ComputeAvailableNumStrands(pLib);
   for (std::vector<StrandIndexType>::iterator iter = strands.begin(); iter != strands.end(); iter++)
   {
      str.Format(_T("%d"), *iter);
      pList->AddString(str);
   }
}

void CTxDOTOptionalDesignStandardFillDlg::OnBnClickedOptCompute()
{
   if (!UpdateData(TRUE))
      return;

   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();

   Float64 eccEnds, eccCL;
   m_pGirderData->ComputeEccentricities(pLib,m_NumStrands,m_To,&eccEnds,&eccCL);

   eccEnds = WBFL::Units::ConvertFromSysUnits(eccEnds,WBFL::Units::Measure::Inch);
   eccCL = WBFL::Units::ConvertFromSysUnits(eccCL,WBFL::Units::Measure::Inch);

   CEccentricityDlg dlg;
   if (eccCL != eccEnds)
   {
      dlg.m_Message.Format(_T("e, CL = %.3f in\n e, girder ends = %.3f in"), eccCL, eccEnds);
   }
   else
   {
      dlg.m_Message.Format(_T("e = %.3f in"), eccCL);
   }

   dlg.DoModal();
}

// We want to play like a property page, but we aren't one
BOOL CTxDOTOptionalDesignStandardFillDlg::OnFillSetActive()
{
   if (m_bFirstActive)
   {
      m_bFirstActive = false;
      return TRUE;
   }
   else
   {
      LoadDialogData();

      return UpdateData(FALSE);
   }
}

BOOL CTxDOTOptionalDesignStandardFillDlg::OnFillKillActive()
{
   return UpdateData(TRUE);
}

BOOL CTxDOTOptionalDesignStandardFillDlg::OnEraseBkgnd(CDC* pDC)
{
    CRect r;
    GetClientRect(&r);

    CBrush brush;
    brush.CreateSolidBrush(TXDOT_BACK_COLOR);

    pDC->FillRect(&r, &brush);
    return TRUE;
}

HBRUSH CTxDOTOptionalDesignStandardFillDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

   TCHAR classname[MAX_PATH];
   if(::GetClassName(pWnd->m_hWnd, classname, MAX_PATH) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("EDIT")) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("COMBOBOX")) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("COMBOLBOX")) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("LISTBOX")) == 0)
     return hbr;

   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}
