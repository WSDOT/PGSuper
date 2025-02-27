///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
// TxDOTOptionalDesignGirderInputPage.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignGirderInputPage.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <MfcTools\CustomDDX.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <LRFD\StrandPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



static const int NUM_TYPES = 4;

// Static data to deal with strand types in controls
struct TxDOTStrandType
{
   CString            Name;
   WBFL::Materials::PsStrand::Type  Type;
   WBFL::Materials::PsStrand::Grade Grade;
  WBFL::Materials::PsStrand::Coating Coating;
};

static TxDOTStrandType StrandTypeList[NUM_TYPES] = { {_T("Grade 250, Low Relaxation"),WBFL::Materials::PsStrand::Type::LowRelaxation, WBFL::Materials::PsStrand::Grade::Gr1725,WBFL::Materials::PsStrand::Coating::None},
                                                   {_T("Grade 250, Stress Relieved"), WBFL::Materials::PsStrand::Type::StressRelieved,WBFL::Materials::PsStrand::Grade::Gr1725,WBFL::Materials::PsStrand::Coating::None},
                                                   {_T("Grade 270, Low Relaxation"),  WBFL::Materials::PsStrand::Type::LowRelaxation, WBFL::Materials::PsStrand::Grade::Gr1860,WBFL::Materials::PsStrand::Coating::None},
                                                   {_T("Grade 270, Stress Relieved"), WBFL::Materials::PsStrand::Type::StressRelieved,WBFL::Materials::PsStrand::Grade::Gr1860,WBFL::Materials::PsStrand::Coating::None} };

static int GetStrandTypeIndex(WBFL::Materials::PsStrand::Type type, WBFL::Materials::PsStrand::Grade grade, WBFL::Materials::PsStrand::Coating coating)
{
   for(int i=0; i<NUM_TYPES; i++)
   {
      TxDOTStrandType& st = StrandTypeList[i];
      if (type==st.Type && grade==st.Grade && coating==st.Coating)
      {
         return i;
      }
   }
   
   ASSERT(0);
   return 0;
}

// CTxDOTOptionalDesignGirderInputPage dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignGirderInputPage, CPropertyPage)

CTxDOTOptionalDesignGirderInputPage::CTxDOTOptionalDesignGirderInputPage()
	: CPropertyPage(CTxDOTOptionalDesignGirderInputPage::IDD),
   m_pBrokerRetriever(nullptr),
   m_pData(nullptr),
   m_OrigStrandFillType( CTxDOTOptionalDesignGirderData::sfStandard),
   m_OptStrandFillType( CTxDOTOptionalDesignGirderData::sfStandard),
   m_GirderTypeChanged(false)
{
}

CTxDOTOptionalDesignGirderInputPage::~CTxDOTOptionalDesignGirderInputPage()
{
}

BOOL CTxDOTOptionalDesignGirderInputPage::OnInitDialog()
{
   // Listen for data changes
   m_pData->Attach(this);

   // Load data into local members
   LoadDialogData();


   // Fill Type combo boxes
   InitFillTypeCtrls();

   // Embed dialogs for strand editing into current. A discription may be found at
   // http://www.codeproject.com/KB/dialog/embedded_dialog.aspx

   // Precaster optional design embedded dialogs
   {
      CWnd* pBox = GetDlgItem(IDC_OPT_BOX);
      pBox->ShowWindow(SW_HIDE);

      CRect boxRect;
      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      m_OptStandardDlg.Init(m_pData->GetPrecasterDesignGirderData(), m_pBrokerRetriever, m_pData->GetGirderEntryName());
      VERIFY(m_OptStandardDlg.Create(CTxDOTOptionalDesignStandardFillDlg::IDD, this));
      VERIFY(m_OptStandardDlg.SetWindowPos( GetDlgItem(IDC_OPT_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      m_OptNonStandardDlg.Init(m_pData->GetPrecasterDesignGirderData(), m_pBrokerRetriever);
      m_OptNonStandardDlg.m_pGirderData = m_pData->GetPrecasterDesignGirderData();
      VERIFY(m_OptNonStandardDlg.Create(CTxDOTOptionalDesignNonStandardFillDlg::IDD, this));
      VERIFY(m_OptNonStandardDlg.SetWindowPos( GetDlgItem(IDC_OPT_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      m_OptDirectFillDlg.Init(m_pData->GetPrecasterDesignGirderData(), m_pBrokerRetriever, m_pData->GetGirderEntryName(), TOGA_FABR_GDR);
      VERIFY(m_OptDirectFillDlg.Create(CTogaDirectFillDlg::IDD, this));
      VERIFY(m_OptDirectFillDlg.SetWindowPos( GetDlgItem(IDC_OPT_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));
   }

   // Original design embedded dialogs
   {
      CWnd* pBox = GetDlgItem(IDC_ORIG_BOX);
      pBox->ShowWindow(SW_HIDE);

      CRect boxRect;
      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      m_OrigStandardDlg.Init(m_pData->GetOriginalDesignGirderData(), m_pBrokerRetriever, m_pData->GetGirderEntryName());
      VERIFY(m_OrigStandardDlg.Create(CTxDOTOptionalDesignStandardFillDlg::IDD, this));
      VERIFY(m_OrigStandardDlg.SetWindowPos( GetDlgItem(IDC_OPT_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      m_OrigNonStandardDlg.Init(m_pData->GetOriginalDesignGirderData(), m_pBrokerRetriever);
      m_OrigNonStandardDlg.m_pGirderData = m_pData->GetOriginalDesignGirderData();
      VERIFY(m_OrigNonStandardDlg.Create(CTxDOTOptionalDesignNonStandardFillDlg::IDD, this));
      VERIFY(m_OrigNonStandardDlg.SetWindowPos( GetDlgItem(IDC_OPT_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      m_OrigDirectFillDlg.Init(m_pData->GetOriginalDesignGirderData(), m_pBrokerRetriever, m_pData->GetGirderEntryName(), TOGA_ORIG_GDR);
      VERIFY(m_OrigDirectFillDlg.Create(CTogaDirectFillDlg::IDD, this));
      VERIFY(m_OrigDirectFillDlg.SetWindowPos( GetDlgItem(IDC_OPT_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));
   }

   // ---
   CPropertyPage::OnInitDialog();

   // Strand types and sizes
   InitStrandSizeTypeCtrls();

   // Show embedded dialogs
   OnCbnSelchangeOptFilltypeCombo();
   OnCbnSelchangeOrgFilltypeCombo();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTxDOTOptionalDesignGirderInputPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
   if (pBroker==nullptr)
      return;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   Float64 min_fc  = WBFL::Units::ConvertToSysUnits( 4.0,  WBFL::Units::Measure::KSI); 
   Float64 max_fci = WBFL::Units::ConvertToSysUnits(10.0,  WBFL::Units::Measure::KSI); 
   Float64 max_fc  = WBFL::Units::ConvertToSysUnits(15.0,  WBFL::Units::Measure::KSI); 

   // precaster opt
   DDX_CBItemData(pDX, IDC_OPT_FILLTYPE_COMBO, (int&)m_OptStrandFillType);

   DDX_UnitValueAndTag(pDX,IDC_OPT_FC, IDC_OPT_FC_UNITS, m_OptFc, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX,IDC_OPT_FC,m_OptFc,min_fc, max_fc, pDisplayUnits->GetStressUnit() );

   DDX_UnitValueAndTag(pDX,IDC_OPT_FCI, IDC_OPT_FCI_UNITS, m_OptFci, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX,IDC_OPT_FCI,m_OptFci,min_fc, max_fci, pDisplayUnits->GetStressUnit() );

   if (pDX->m_bSaveAndValidate)
   {
      if (m_OptFc < m_OptFci)
      {
         ::AfxMessageBox(_T("f'ci may not be larger than f'c"),MB_OK | MB_ICONEXCLAMATION);
         pDX->PrepareCtrl(IDC_OPT_FCI);
         pDX->Fail();
      }
   }

   // original
   DDX_CBItemData(pDX, IDC_ORG_FILLTYPE_COMBO, (int&)m_OrigStrandFillType);

   DDX_UnitValueAndTag(pDX,IDC_ORIG_FC, IDC_ORIG_FC_UNITS, m_OrigFc, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX,IDC_ORIG_FC,m_OrigFc,min_fc, max_fc, pDisplayUnits->GetStressUnit() );

   DDX_UnitValueAndTag(pDX,IDC_ORIG_FCI, IDC_ORIG_FCI_UNITS, m_OrigFci, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX,IDC_ORIG_FCI,m_OrigFci,min_fc, max_fci, pDisplayUnits->GetStressUnit() );

   if (pDX->m_bSaveAndValidate)
   {
      if (m_OrigFc < m_OrigFci)
      {
         ::AfxMessageBox(_T("f'ci may not be larger than f'c"),MB_OK | MB_ICONEXCLAMATION);
         pDX->PrepareCtrl(IDC_ORIG_FCI);
         pDX->Fail();
      }
   }

   if (pDX->m_bSaveAndValidate)
   {
      SaveDialogData();
   }
}

// Bastardized override of updatedata
BOOL CTxDOTOptionalDesignGirderInputPage::UpdateData(BOOL bSaveAndValidate)
{
   // local data and embedded control data
   if (!__super::UpdateData(bSaveAndValidate))
      return FALSE;

   // precaster optional data
   if (CTxDOTOptionalDesignGirderData::sfStandard == m_OptStrandFillType)
   {
      BOOL st = m_OptStandardDlg.UpdateData(bSaveAndValidate);
      if (!st)
         return FALSE;
   }
   else if (CTxDOTOptionalDesignGirderData::sfHarpedRows == m_OptStrandFillType)
   {
      BOOL st = m_OptNonStandardDlg.UpdateData(bSaveAndValidate);
      if (!st)
         return FALSE;
   }
   else if (CTxDOTOptionalDesignGirderData::sfDirectFill == m_OptStrandFillType)
   {
      BOOL st = m_OptDirectFillDlg.UpdateData(bSaveAndValidate);
      if (!st)
         return FALSE;
   }
   else
      ATLASSERT(false);

   // original
   if (CTxDOTOptionalDesignGirderData::sfStandard == m_OrigStrandFillType)
   {
      BOOL st = m_OrigStandardDlg.UpdateData(bSaveAndValidate);
      if (!st)
         return FALSE;
   }
   else if (CTxDOTOptionalDesignGirderData::sfHarpedRows == m_OrigStrandFillType)
   {
      BOOL st = m_OrigNonStandardDlg.UpdateData(bSaveAndValidate);
      if (!st)
         return FALSE;
   }
   else if (CTxDOTOptionalDesignGirderData::sfDirectFill == m_OrigStrandFillType)
   {
      BOOL st = m_OrigDirectFillDlg.UpdateData(bSaveAndValidate);
      if (!st)
         return FALSE;
   }
   else
      ATLASSERT(false);

   return TRUE;
}

BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignGirderInputPage, CPropertyPage)
   ON_CBN_SELCHANGE(IDC_OPT_STRAND_TYPE, &CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOptStrandType)
   ON_CBN_SELCHANGE(IDC_ORIG_STRAND_TYPE, &CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOrigStrandType)
   ON_WM_ERASEBKGND()
   ON_WM_CTLCOLOR()
   ON_COMMAND(ID_HELP, &CTxDOTOptionalDesignGirderInputPage::OnHelpFinder)
   ON_COMMAND(ID_HELP_FINDER, &CTxDOTOptionalDesignGirderInputPage::OnHelpFinder)
   ON_CBN_SELCHANGE(IDC_ORG_FILLTYPE_COMBO, &CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOrgFilltypeCombo)
   ON_CBN_SELCHANGE(IDC_OPT_FILLTYPE_COMBO, &CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOptFilltypeCombo)
END_MESSAGE_MAP()

void CTxDOTOptionalDesignGirderInputPage::OnTxDotDataChanged(int change)
{
   if ( (change & ITxDataObserver::ctTemplateFile) == ITxDataObserver::ctTemplateFile)
   {
      // girder type changed - we need to update our library data
      m_GirderTypeChanged = true;
   }
}

void CTxDOTOptionalDesignGirderInputPage::LoadDialogData()
{
   // precaster optional
   CTxDOTOptionalDesignGirderData* pOptGirderData = m_pData->GetPrecasterDesignGirderData();

   m_OptFc = pOptGirderData->GetFc();
   m_OptFci = pOptGirderData->GetFci();

   m_OptStrandFillType = pOptGirderData->GetStrandFillType();
   m_strOptNoStrands.Format(_T("%d"), pOptGirderData->GetStrandCount() );

   // original
   CTxDOTOptionalDesignGirderData* pOrigGirderData = m_pData->GetOriginalDesignGirderData();

   m_OrigFc = pOrigGirderData->GetFc();
   m_OrigFci = pOrigGirderData->GetFci();

   m_OrigStrandFillType = pOrigGirderData->GetStrandFillType();
   m_strOrigNoStrands.Format(_T("%d"), pOrigGirderData->GetStrandCount() );
}

void CTxDOTOptionalDesignGirderInputPage::SaveDialogData()
{
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();

   // Optional Design
   CTxDOTOptionalDesignGirderData* pOptGirderData = m_pData->GetPrecasterDesignGirderData();

   pOptGirderData->SetStrandFillType(m_OptStrandFillType);

   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_OPT_STRAND_SIZE );
   int sel = pList->GetCurSel();
   ASSERT(sel!=CB_ERR);
   DWORD_PTR key = pList->GetItemData( sel );
   const auto* pmat = pPool->GetStrand( (Int32)key );

   pOptGirderData->SetStrandData(pmat->GetGrade(), pmat->GetType(), pmat->GetCoating(), pmat->GetSize());

   pOptGirderData->SetFc(m_OptFc);
   pOptGirderData->SetFci(m_OptFci);

   // Original Design
   CTxDOTOptionalDesignGirderData* pOrigGirderData = m_pData->GetOriginalDesignGirderData();

   pOrigGirderData->SetStrandFillType(m_OrigStrandFillType);

   pList = (CComboBox*)GetDlgItem( IDC_ORIG_STRAND_SIZE );
   sel = pList->GetCurSel();
   ASSERT(sel!=CB_ERR);
   key = pList->GetItemData( sel );
   pmat = pPool->GetStrand( (Int32)key );

   pOrigGirderData->SetStrandData(pmat->GetGrade(), pmat->GetType(), pmat->GetCoating(), pmat->GetSize());

   pOrigGirderData->SetFc(m_OrigFc);
   pOrigGirderData->SetFci(m_OrigFci);
}

void CTxDOTOptionalDesignGirderInputPage::InitStrandTypeCtrl(long TypeCtrlID)
{
   CComboBox* pList = (CComboBox*)GetDlgItem( TypeCtrlID );

   for (long i=0; i<NUM_TYPES; i++)
   {
      pList->AddString( StrandTypeList[i].Name );
   }
}

void CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOptStrandType()
{
   OnStrandTypeChanged(IDC_OPT_STRAND_SIZE, IDC_OPT_STRAND_TYPE);
}

void CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOrigStrandType()
{
   OnStrandTypeChanged(IDC_ORIG_STRAND_SIZE, IDC_ORIG_STRAND_TYPE);
}

void CTxDOTOptionalDesignGirderInputPage::InitStrandSizeTypeCtrls()
{
   InitOptStrandSizeTypeCtrls();
   InitOrigStrandSizeTypeCtrls();
}

void CTxDOTOptionalDesignGirderInputPage::InitOptStrandSizeTypeCtrls()
{
   // Precaster Optional Design
   CTxDOTOptionalDesignGirderData* pOptionalGirderData = m_pData->GetPrecasterDesignGirderData();

   // strand type, grade, size
   WBFL::Materials::PsStrand::Type type;
   WBFL::Materials::PsStrand::Grade grade;
  WBFL::Materials::PsStrand::Coating coating;
   WBFL::Materials::PsStrand::Size size;
   pOptionalGirderData->GetStrandData(&grade,&type,&coating,&size);

   // set type control
   InitStrandTypeCtrl(IDC_OPT_STRAND_TYPE); // fill with grade/type

   int strandTypeIdx = GetStrandTypeIndex(type, grade, coating);
   CComboBox* pTypeList = (CComboBox*)GetDlgItem( IDC_OPT_STRAND_TYPE );
   pTypeList->SetCurSel(strandTypeIdx);

   // Set strand size
   UpdateStrandSizeList( IDC_OPT_STRAND_SIZE, grade, type, coating, size);	
}

void CTxDOTOptionalDesignGirderInputPage::InitOrigStrandSizeTypeCtrls()
{
   // Precaster Optional Design
   CTxDOTOptionalDesignGirderData* pOriginalGirderData = m_pData->GetOriginalDesignGirderData();

   // strand type, grade, size
   WBFL::Materials::PsStrand::Type type;
   WBFL::Materials::PsStrand::Grade grade;
  WBFL::Materials::PsStrand::Coating coating;
   WBFL::Materials::PsStrand::Size size;
   pOriginalGirderData->GetStrandData(&grade,&type,&coating,&size);

   // set type control
   InitStrandTypeCtrl(IDC_ORIG_STRAND_TYPE); // fill with grade/type

   int strandTypeIdx = GetStrandTypeIndex(type, grade, coating);
   CComboBox* pTypeList = (CComboBox*)GetDlgItem( IDC_ORIG_STRAND_TYPE );
   pTypeList->SetCurSel(strandTypeIdx);

   // Set strand size
   UpdateStrandSizeList( IDC_ORIG_STRAND_SIZE, grade, type, coating, size);	
}

void CTxDOTOptionalDesignGirderInputPage::InitFillTypeCtrls()
{
   // Fill-Type combo boxes
   CString girder =  m_pData->GetGirderEntryName();
   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(girder));
   ASSERT(pGdrEntry!=nullptr);

   // Options depend on whether harped strands can exist
   bool hasHarped = pGdrEntry->GetMaxHarpedStrands() > 0;

   // add cb data
   //original
   CComboBox* pCBox = (CComboBox*)GetDlgItem(IDC_ORG_FILLTYPE_COMBO);
   pCBox->ResetContent();

   int idx = pCBox->AddString(_T("Standard sequential strand fill"));
   pCBox->SetItemData(idx, DWORD(CTxDOTOptionalDesignGirderData::sfStandard));

   if(hasHarped)
   {
      idx = pCBox->AddString(_T("Non-standard strand fill with depressed strands"));
      pCBox->SetItemData(idx, DWORD(CTxDOTOptionalDesignGirderData::sfHarpedRows));
   }

   idx = pCBox->AddString(_T("Non-standard direct strand fill of straight strands"));
   pCBox->SetItemData(idx, DWORD(CTxDOTOptionalDesignGirderData::sfDirectFill));

   //optional
   pCBox = (CComboBox*)GetDlgItem(IDC_OPT_FILLTYPE_COMBO);
   pCBox->ResetContent();

   idx = pCBox->AddString(_T("Standard sequential strand fill"));
   pCBox->SetItemData(idx, DWORD(CTxDOTOptionalDesignGirderData::sfStandard));

   if(hasHarped)
   {
      idx = pCBox->AddString(_T("Non-standard strand fill with depressed strands"));
      pCBox->SetItemData(idx, DWORD(CTxDOTOptionalDesignGirderData::sfHarpedRows));
   }

   idx = pCBox->AddString(_T("Non-standard direct strand fill of straight strands"));
   pCBox->SetItemData(idx, DWORD(CTxDOTOptionalDesignGirderData::sfDirectFill));

   // it is possible that fill type uses harprd strands and we switched to a girder with none
   if(!hasHarped)
   {
      if(m_OrigStrandFillType == CTxDOTOptionalDesignGirderData::sfHarpedRows)
      {
         pCBox = (CComboBox*)GetDlgItem(IDC_ORG_FILLTYPE_COMBO);
         pCBox->SetCurSel(0);
         OnCbnSelchangeOrgFilltypeCombo();
      }

      if(m_OptStrandFillType == CTxDOTOptionalDesignGirderData::sfHarpedRows)
      {
         pCBox = (CComboBox*)GetDlgItem(IDC_OPT_FILLTYPE_COMBO);
         pCBox->SetCurSel(0);
         OnCbnSelchangeOptFilltypeCombo();
      }
   }
}

void CTxDOTOptionalDesignGirderInputPage::UpdateStrandSizeList(long StrandSizeListCtrlID, WBFL::Materials::PsStrand::Grade grade,WBFL::Materials::PsStrand::Type type, WBFL::Materials::PsStrand::Coating coating, WBFL::Materials::PsStrand::Size size)
{
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();

   CComboBox* pList = (CComboBox*)GetDlgItem( StrandSizeListCtrlID );

   // Retain information about the current selection so we can attempt to re-select the
   // same size after the combo box is updated.
   int cur_sel = pList->GetCurSel();
   WBFL::Materials::PsStrand::Size cur_size = size;
   if ( cur_sel != CB_ERR )
   {
      DWORD_PTR cur_key = pList->GetItemData( cur_sel );
      const auto* pCurStrand = pPool->GetStrand( (Int32)cur_key );
      cur_size = pCurStrand->GetSize();
   }

   pList->ResetContent();
   WBFL::LRFD::StrandIter iter( grade, type, coating );
   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for ( iter.Begin(); iter; iter.Next() )
   {
      const auto* pStrand = iter.GetCurrentStrand();
      CString size = get_strand_size( pStrand->GetSize() );
      int idx = pList->AddString( size );

      if ( idx != CB_ERR )
      { 
         // if there wasn't an error adding the size, add a data item
         auto key = pPool->GetStrandKey( pStrand );

         if ( pList->SetItemData( idx, key ) == CB_ERR )
         {
            // if there was an error adding the data item, remove the size
            idx = pList->DeleteString( idx );
            ASSERT( idx != CB_ERR ); // make sure it got removed.
         }
         else
         {
            // data item added successfully.
            if ( pStrand->GetSize() == cur_size )
            {
               // We just found the one we want to select.
               new_cur_sel = sel_count;
            }
         }
      }

      sel_count++;
   }

   // Attempt to re-select the strand.
   if ( new_cur_sel >= 0 )
      pList->SetCurSel( new_cur_sel );
   else
      pList->SetCurSel( pList->GetCount()-1 );
}

void CTxDOTOptionalDesignGirderInputPage::OnStrandTypeChanged(long SizeCtrlID, long TypeCtrlID) 
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(TypeCtrlID);
   ASSERT(ptype_ctrl!=0);
   int idx = ptype_ctrl->GetCurSel();
   ASSERT(idx!=CB_ERR);

   WBFL::Materials::PsStrand::Grade grade = StrandTypeList[idx].Grade;
   WBFL::Materials::PsStrand::Type  type  = StrandTypeList[idx].Type;
   WBFL::Materials::PsStrand::Coating coating = StrandTypeList[idx].Coating;

   UpdateStrandSizeList( SizeCtrlID, grade, type, coating, WBFL::Materials::PsStrand::Size::D635);
}

BOOL CTxDOTOptionalDesignGirderInputPage::OnSetActive()
{
   if (m_GirderTypeChanged)
   {
      // fill type can change
      InitFillTypeCtrls();

      // Need to alert our embedded dialogs when girder entry changes
      CString name = m_pData->GetGirderEntryName();

      m_OptStandardDlg.SetGirderEntryName(name);
      m_OrigStandardDlg.SetGirderEntryName(name);
      m_OptDirectFillDlg.SetGirderEntryName(name);
      m_OrigDirectFillDlg.SetGirderEntryName(name);

      m_GirderTypeChanged = false;
   }

   m_OptDirectFillDlg.SetSpanLength(m_pData->GetSpanLength());
   m_OrigDirectFillDlg.SetSpanLength(m_pData->GetSpanLength());

   if (!m_OptStandardDlg.OnFillSetActive())
   {
      return false;
   }

   if (!m_OptNonStandardDlg.OnFillSetActive())
   {
      return false;
   }

   if (!m_OrigStandardDlg.OnFillSetActive())
   {
      return false;
   }

   if (!m_OrigNonStandardDlg.OnFillSetActive())
   {
      return false;
   }

   return CPropertyPage::OnSetActive();
}

BOOL CTxDOTOptionalDesignGirderInputPage::OnKillActive()
{
   // Data exchange for embedded dialogs
   // Only get data from active dialog, else ddv's will cause trouble
   if (CTxDOTOptionalDesignGirderData::sfStandard == m_OptStrandFillType)
   {
      if (!m_OptStandardDlg.OnFillKillActive())
      {
         return false;
      }
   }
   else if (CTxDOTOptionalDesignGirderData::sfHarpedRows == m_OptStrandFillType)
   {
      if (!m_OptNonStandardDlg.OnFillKillActive())
      {
         return false;
      }
   }
   else if (CTxDOTOptionalDesignGirderData::sfDirectFill == m_OptStrandFillType)
   {
      if (!m_OptDirectFillDlg.OnFillKillActive())
      {
         return false;
      }
   }
   else
   {
      ATLASSERT(false);
   }

   if (CTxDOTOptionalDesignGirderData::sfStandard == m_OrigStrandFillType)
   {
      if (!m_OrigStandardDlg.OnFillKillActive())
      {
         return false;
      }
   }
   else if (CTxDOTOptionalDesignGirderData::sfHarpedRows == m_OrigStrandFillType)
   {
      if (!m_OrigNonStandardDlg.OnFillKillActive())
      {
         return false;
      }
   }
   else if (CTxDOTOptionalDesignGirderData::sfDirectFill == m_OrigStrandFillType)
   {
      if (!m_OrigDirectFillDlg.OnFillKillActive())
      {
         return false;
      }
   }
   else
   {
      ATLASSERT(false);
   }

   return CPropertyPage::OnKillActive();
}


void CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOrgFilltypeCombo()
{
   // Show correct embedded dialog
   CComboBox* pCBox = (CComboBox*)GetDlgItem(IDC_ORG_FILLTYPE_COMBO);
   int sel = pCBox->GetCurSel();

   m_OrigStrandFillType = (CTxDOTOptionalDesignGirderData::StrandFillType) pCBox->GetItemData(sel);

   if (CTxDOTOptionalDesignGirderData::sfStandard == m_OrigStrandFillType)
   {
      m_OrigNonStandardDlg.ShowWindow(SW_HIDE);
      m_OrigDirectFillDlg.ShowWindow(SW_HIDE);

      m_OrigStandardDlg.ShowWindow(SW_SHOW);
      m_OrigStandardDlg.OnFillSetActive();
   }
   else if (CTxDOTOptionalDesignGirderData::sfHarpedRows == m_OrigStrandFillType)
   {
      m_OrigStandardDlg.ShowWindow(SW_HIDE);
      m_OrigDirectFillDlg.ShowWindow(SW_HIDE);

      m_OrigNonStandardDlg.ShowWindow(SW_SHOW);
      m_OrigNonStandardDlg.OnFillSetActive();

      m_OrigNonStandardDlg.DoUseDepressed(true);
   }
   else if (CTxDOTOptionalDesignGirderData::sfDirectFill == m_OrigStrandFillType)
   {
      m_OrigStandardDlg.ShowWindow(SW_HIDE);
      m_OrigNonStandardDlg.ShowWindow(SW_HIDE);

      m_OrigDirectFillDlg.ShowWindow(SW_SHOW);
      m_OrigDirectFillDlg.OnFillSetActive();
   }
}

void CTxDOTOptionalDesignGirderInputPage::OnCbnSelchangeOptFilltypeCombo()
{
   // Show correct embedded dialog
   CComboBox* pCBox = (CComboBox*)GetDlgItem(IDC_OPT_FILLTYPE_COMBO);
   int sel = pCBox->GetCurSel();

   m_OptStrandFillType = (CTxDOTOptionalDesignGirderData::StrandFillType) pCBox->GetItemData(sel);

   if (CTxDOTOptionalDesignGirderData::sfStandard == m_OptStrandFillType)
   {
      m_OptNonStandardDlg.ShowWindow(SW_HIDE);
      m_OptDirectFillDlg.ShowWindow(SW_HIDE);

      m_OptStandardDlg.ShowWindow(SW_SHOW);
      m_OptStandardDlg.OnFillSetActive();
   }
   else if (CTxDOTOptionalDesignGirderData::sfHarpedRows == m_OptStrandFillType)
   {
      m_OptStandardDlg.ShowWindow(SW_HIDE);
      m_OptDirectFillDlg.ShowWindow(SW_HIDE);

      m_OptNonStandardDlg.ShowWindow(SW_SHOW);
      m_OptNonStandardDlg.OnFillSetActive();

      m_OptNonStandardDlg.DoUseDepressed(true);
   }
   else if (CTxDOTOptionalDesignGirderData::sfDirectFill == m_OptStrandFillType)
   {
      m_OptStandardDlg.ShowWindow(SW_HIDE);
      m_OptNonStandardDlg.ShowWindow(SW_HIDE);

      m_OptDirectFillDlg.ShowWindow(SW_SHOW);
      m_OptDirectFillDlg.OnFillSetActive();
   }
}

BOOL CTxDOTOptionalDesignGirderInputPage::OnEraseBkgnd(CDC* pDC)
{
   // Set brush to dialog background color
   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   // Save old brush
   CBrush* pOldBrush = pDC->SelectObject(&backBrush);

   CRect rect;
   pDC->GetClipBox(&rect);     // Erase the area needed

   pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
       PATCOPY);
   pDC->SelectObject(pOldBrush);

   return true;
}

HBRUSH CTxDOTOptionalDesignGirderInputPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}


void CTxDOTOptionalDesignGirderInputPage::OnHelpFinder()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_INPUT );
}
