///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// SplicedGirderGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "SplicedGirderGeneralPage.h"
#include "SplicedGirderDescDlg.h"
#include <LRFD\StrandPool.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <PgsExt\CustomDDX.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CSplicedGirderGeneralPage dialog

void DDX_PTData(CDataExchange* pDX,INT nIDC,CPTData* ptData)
{
   CDuctGrid* pGrid = (CDuctGrid*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
   if ( pDX->m_bSaveAndValidate )
   {
      pGrid->UpdatePTData();

      // If Pjack is defined with a user-input force, the value of Pj
      // becomes the last Pj input by the user. Update the data structure here.
      DuctIndexType nDucts = ptData->GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         CDuctData* pDuct = ptData->GetDuct(ductIdx);
         if ( !pDuct->bPjCalc )
         {
            pDuct->LastUserPj = pDuct->Pj;
         }
      }
   }
   else
   {
      pGrid->FillGrid();
   }
}

IMPLEMENT_DYNAMIC(CSplicedGirderGeneralPage, CPropertyPage)

CSplicedGirderGeneralPage::CSplicedGirderGeneralPage()
	: CPropertyPage(CSplicedGirderGeneralPage::IDD)
{
}

CSplicedGirderGeneralPage::~CSplicedGirderGeneralPage()
{
}

void CSplicedGirderGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();

   int sameGirderType = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription()->UseSameGirderForEntireBridge() ? 0 : 1;
   DDX_CBIndex(pDX,IDC_CB_SAMEGIRDER,sameGirderType);
   std::_tstring strGirderName = pParent->m_pGirder->GetGirderName();
   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,strGirderName);
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription()->UseSameGirderForEntireBridge(sameGirderType == 0 ? true : false);
      if ( sameGirderType == 0 )
      {
         pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription()->SetGirderName(strGirderName.c_str());
      }
      else
      {
         GirderIndexType gdrIdx = pParent->m_pGirder->GetIndex();
         GroupIndexType girderTypeGroupIdx = pParent->m_pGirder->GetGirderGroup()->CreateGirderTypeGroup(gdrIdx,gdrIdx);
         pParent->m_pGirder->GetGirderGroup()->SetGirderName(girderTypeGroupIdx,strGirderName.c_str());
      }
   }

   DDX_Strand(pDX,IDC_STRAND,&pParent->m_pGirder->GetPostTensioning()->pStrand);

   Float64 conditionFactor = pParent->m_pGirder->GetConditionFactor();
   pgsTypes::ConditionFactorType conditionFactorType = pParent->m_pGirder->GetConditionFactorType();
   pgsTypes::DuctType ductType   = pParent->m_pGirder->GetPostTensioning()->DuctType; 
   pgsTypes::StrandInstallationType installationType = pParent->m_pGirder->GetPostTensioning()->InstallationType;
   DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
   DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);
   DDX_CBEnum(pDX, IDC_DUCT_TYPE, ductType);
   DDX_CBItemData(pDX, IDC_INSTALLATION_TYPE, installationType );
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_pGirder->SetConditionFactor(conditionFactor);
      pParent->m_pGirder->SetConditionFactorType(conditionFactorType);
      pParent->m_pGirder->GetPostTensioning()->DuctType = ductType;
      pParent->m_pGirder->GetPostTensioning()->InstallationType = installationType;
   }

   // NOTE: must come after DDX for strand installation type
   DDV_GXGridWnd(pDX,&m_DuctGrid);
   DDX_PTData(pDX,IDC_DUCT_GRID,pParent->m_pGirder->GetPostTensioning());

   // Validate the timeline
   if ( pDX->m_bSaveAndValidate )
   {
      CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();

      DuctIndexType nDucts = pParent->m_pGirder->GetPostTensioning()->GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         pTimelineMgr->SetStressTendonEventByIndex(pParent->m_GirderID,ductIdx,m_TendonStressingEvent[ductIdx]);
      }

      int result = pTimelineMgr->Validate();
      if ( result != TLM_SUCCESS )
      {
         pDX->PrepareCtrl(IDC_GIRDER_GRID);
         CString strMsg = pTimelineMgr->GetErrorMessage(result).c_str();
         AfxMessageBox(strMsg);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CSplicedGirderGeneralPage, CPropertyPage)
   ON_BN_CLICKED(IDC_ADD, &CSplicedGirderGeneralPage::OnAddDuct)
   ON_BN_CLICKED(IDC_DELETE, &CSplicedGirderGeneralPage::OnDeleteDuct)
   ON_CBN_SELCHANGE(IDC_STRAND, &CSplicedGirderGeneralPage::OnStrandChanged)
   ON_CBN_SELCHANGE(IDC_INSTALLATION_TYPE, &CSplicedGirderGeneralPage::OnInstallationTypeChanged)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, &CSplicedGirderGeneralPage::OnConditionFactorTypeChanged)
   ON_COMMAND(ID_HELP, &CSplicedGirderGeneralPage::OnHelp)
   ON_CBN_SELCHANGE(IDC_CB_SAMEGIRDER, &CSplicedGirderGeneralPage::OnChangeGirderType)
   ON_CBN_SELCHANGE(IDC_GIRDER_NAME, &CSplicedGirderGeneralPage::OnChangedGirderName)
   ON_BN_CLICKED(IDC_SCHEMATIC, &CSplicedGirderGeneralPage::OnSchematicButton)
END_MESSAGE_MAP()


// CSplicedGirderGeneralPage message handlers

BOOL CSplicedGirderGeneralPage::OnInitDialog()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();

   EnableToolTips(TRUE);

   // initialize girder girder
	m_GirderGrid.SubclassDlgItem(IDC_GIRDER_GRID, this);
   m_GirderGrid.CustomInit();

   // initialize duct grid
	m_DuctGrid.SubclassDlgItem(IDC_DUCT_GRID, this);
   m_DuctGrid.CustomInit(pParent->m_pGirder);

   // subclass the schematic drawing of the tendons
   m_DrawTendons.SubclassDlgItem(IDC_TENDONS,this);
   m_DrawTendons.CustomInit(pParent->m_GirderKey,pParent->m_pGirder,pParent->m_pGirder->GetPostTensioning());

   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();
   DuctIndexType nDucts = pParent->m_pGirder->GetPostTensioning()->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(pParent->m_GirderID,ductIdx);
      m_TendonStressingEvent.push_back(eventIdx);
   }

   FillStrandList(IDC_STRAND);

   FillGirderComboBox();

   FillDuctType();
   FillInstallationType();

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   CComboBox* pcbSameGirder = (CComboBox*)GetDlgItem(IDC_CB_SAMEGIRDER);
   pcbSameGirder->AddString(_T("Use same girder type for entire bridge"));
   pcbSameGirder->AddString(_T("Use a unique girder type for each girder"));

   CPropertyPage::OnInitDialog();

   OnConditionFactorTypeChanged();

   UpdateGirderTypeControls();

   HINSTANCE hInstance = AfxGetInstanceHandle();
   CButton* pSchematicBtn = (CButton*)GetDlgItem(IDC_SCHEMATIC);
   pSchematicBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_SCHEMATIC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSplicedGirderGeneralPage::EventCreated()
{
   m_TendonStressingEvent.clear();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();
   DuctIndexType nDucts = pParent->m_pGirder->GetPostTensioning()->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(pParent->m_GirderID,ductIdx);
      m_TendonStressingEvent.push_back(eventIdx);
   }

   m_DuctGrid.EventCreated();
}

void CSplicedGirderGeneralPage::OnConditionFactorTypeChanged()
{
   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_CONDITION_FACTOR);
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);

   int idx = pcbConditionFactor->GetCurSel();
   switch(idx)
   {
   case 0:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("1.00"));
      break;
   case 1:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.95"));
      break;
   case 2:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.85"));
      break;
   case 3:
      pEdit->EnableWindow(TRUE);
      break;
   }
}

void CSplicedGirderGeneralPage::OnAddDuct()
{
   EventIndexType eventIdx = 0;
   if ( m_TendonStressingEvent.size() != 0 )
   {
      eventIdx = m_TendonStressingEvent.back();
   }

   m_TendonStressingEvent.push_back(eventIdx);
   m_DuctGrid.AddDuct(eventIdx);
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}

void CSplicedGirderGeneralPage::OnDeleteDuct()
{
   m_TendonStressingEvent.pop_back();
   m_DuctGrid.DeleteDuct();
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}

void CSplicedGirderGeneralPage::FillGirderComboBox()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strGirderFamilyName = pBridgeDesc->GetGirderFamilyName();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;
   
   pLibNames->EnumGirderNames(strGirderFamilyName.c_str(), &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::_tstring& name = *iter;

      pCB->AddString( name.c_str() );
   }
}

void CSplicedGirderGeneralPage::FillStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   WBFL::Materials::PsStrand::Size cur_size = WBFL::Materials::PsStrand::Size::D1270;
   if ( cur_sel != CB_ERR )
   {
      Int64 cur_key = (Int64)pList->GetItemData( cur_sel );
      const auto* pCurStrand = pPool->GetStrand( cur_key );
      cur_size = pCurStrand->GetSize();
   }

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for (int i = 0; i < 3; i++)
   {
      WBFL::Materials::PsStrand::Grade grade = (i == 0 ? WBFL::Materials::PsStrand::Grade::Gr1725 :
                                  i == 1 ? WBFL::Materials::PsStrand::Grade::Gr1860 : WBFL::Materials::PsStrand::Grade::Gr2070);
      for ( int j = 0; j < 2; j++ )
      {
         WBFL::Materials::PsStrand::Type type = (j == 0 ? WBFL::Materials::PsStrand::Type::LowRelaxation : WBFL::Materials::PsStrand::Type::StressRelieved);

         lrfdStrandIter iter(grade,type);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const auto* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );

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
      }
   }

   // Attempt to re-select the strand.
   if ( 0 <= new_cur_sel )
   {
      pList->SetCurSel( new_cur_sel );
   }
   else
   {
      pList->SetCurSel( pList->GetCount()-1 );
   }
}

void CSplicedGirderGeneralPage::FillStrandList(CComboBox* pList,WBFL::Materials::PsStrand::Grade grade,WBFL::Materials::PsStrand::Type  type)
{
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   bool bUnitsUS = IS_US_UNITS(pDisplayUnits);

   pList->ResetContent();

   lrfdStrandIter iter( grade, type );
   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   for ( iter.Begin(); iter; iter.Next() )
   {
      const auto* pStrand = iter.GetCurrentStrand();
      std::_tstring size = WBFL::Materials::PsStrand::GetSize( pStrand->GetSize(), bUnitsUS );
      int idx = pList->AddString( size.c_str() );

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
      }

      sel_count++;
   }
}

const WBFL::Materials::PsStrand* CSplicedGirderGeneralPage::GetStrand()
{
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND );
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   int cursel = pList->GetCurSel();
   Int64 key = (Int64)pList->GetItemData(cursel);
   return pPool->GetStrand(key);
}

WBFL::Graphing::PointMapper::MapMode CSplicedGirderGeneralPage::GetTendonControlMapMode() const
{
   return m_DrawTendons.GetMapMode();
}

CSplicedGirderData* CSplicedGirderGeneralPage::GetGirder()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   return pParent->m_pGirder;
}

pgsTypes::StrandInstallationType CSplicedGirderGeneralPage::GetInstallationType()
{
   CComboBox* pList = (CComboBox*)GetDlgItem(IDC_INSTALLATION_TYPE);
   int idx = pList->GetCurSel();
   pgsTypes::StrandInstallationType installationType = (pgsTypes::StrandInstallationType)(pList->GetItemData(idx));
   return installationType;
}

void CSplicedGirderGeneralPage::FillDuctType()
{
   // LRFD 5.4.6.1
   CComboBox* pcbDuctType = (CComboBox*)GetDlgItem(IDC_DUCT_TYPE);
   pcbDuctType->AddString(_T("Galvanized ferrous metal"));
   pcbDuctType->AddString(_T("Polyethylene"));
   pcbDuctType->AddString(_T("Formed in concrete with removable cores"));
}

void CSplicedGirderGeneralPage::FillInstallationType()
{
   // LRFD 5.4.6.2
   CComboBox* pcbInstallType = (CComboBox*)GetDlgItem(IDC_INSTALLATION_TYPE);
   int idx = pcbInstallType->AddString(_T("Push"));
   pcbInstallType->SetItemData(idx,(DWORD_PTR)pgsTypes::sitPush);
   idx = pcbInstallType->AddString(_T("Pull"));
   pcbInstallType->SetItemData(idx,(DWORD_PTR)pgsTypes::sitPull);
}

void CSplicedGirderGeneralPage::OnStrandChanged()
{
   m_DuctGrid.OnStrandChanged();
}

void CSplicedGirderGeneralPage::OnInstallationTypeChanged()
{
   m_DuctGrid.OnInstallationTypeChanged();
}

void CSplicedGirderGeneralPage::OnDuctChanged()
{
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}


int CSplicedGirderGeneralPage::GetDuctCount()
{
   return m_DuctGrid.GetRowCount();
}

void CSplicedGirderGeneralPage::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_SPLICED_GIRDER_GENERAL);
}

void CSplicedGirderGeneralPage::OnChangeGirderType()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();
   pBridge->UseSameGirderForEntireBridge( !pBridge->UseSameGirderForEntireBridge() );

   UpdateGirderTypeControls();
}

void CSplicedGirderGeneralPage::UpdateGirderTypeControls()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   BOOL bEnable = ( pBridge->UseSameGirderForEntireBridge() ? FALSE : TRUE );
   CWnd* pWnd = GetDlgItem(IDC_GIRDER_NAME);
   pWnd->EnableWindow(bEnable);

   pWnd->SetWindowText(pParent->m_pGirder->GetGirderName());
}

void CSplicedGirderGeneralPage::OnChangedGirderName()
{
   CComboBox* pcbGirderName = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);
   int curSel = pcbGirderName->GetCurSel();
   CString strName;
   pcbGirderName->GetLBText(curSel,strName);

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   if ( pParent->m_pGirder->GetGirderName() != strName )
   {
      pParent->m_pGirder->SetGirderName(strName);

      // make sure the segment variation type is valid
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,ILibrary,pLib);
      const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strName);
      pParent->m_pGirder->SetGirderLibraryEntry(pGirderEntry);

      SegmentIndexType nSegments = pParent->m_pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CPrecastSegmentData* pSegment = pParent->m_pGirder->GetSegment(segIdx);
         std::vector<pgsTypes::SegmentVariationType> segmentVariations(pParent->m_pGirder->GetSupportedSegmentVariations(pGirderEntry));
         std::vector<pgsTypes::SegmentVariationType>::iterator found = std::find(segmentVariations.begin(),segmentVariations.end(),pSegment->GetVariationType());
         if ( found == segmentVariations.end() )
         {
            pSegment->SetVariationType(segmentVariations.front());
         }
      } // next segment
   }
}

void CSplicedGirderGeneralPage::OnSchematicButton()
{
   auto mm = m_DrawTendons.GetMapMode();
   mm = (mm == WBFL::Graphing::PointMapper::MapMode::Isotropic ? WBFL::Graphing::PointMapper::MapMode::Anisotropic : WBFL::Graphing::PointMapper::MapMode::Isotropic);
   m_DrawTendons.SetMapMode(mm);
}
