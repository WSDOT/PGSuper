///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "SplicedGirderGeneralPage.h"
#include "..\SplicedGirderDescDlg.h"
#include <LRFD\StrandPool.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>


#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include <IFace\Project.h>

// CSplicedGirderGeneralPage dialog


void DDX_Strand(CDataExchange* pDX,UINT nIDC,const matPsStrand** ppStrand)
{
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)pDX->m_pDlgWnd->GetDlgItem( nIDC );

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      int curSel = pList->GetCurSel();
      Int32 key = (Int32)pList->GetItemData( curSel );
      *ppStrand = pPool->GetStrand( key );
   }
   else
   {
      Int32 target_key = pPool->GetStrandKey(*ppStrand );
      int cStrands = pList->GetCount();
      for ( int i = 0; i < cStrands; i++ )
      {
         Int32 key = (Int32)pList->GetItemData( i );
         if ( key == target_key )
         {
            pList->SetCurSel(i);
            break;
         }
      }
   }
}

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

void DDX_SlabOffsetGrid(CDataExchange* pDX,INT nIDC)
{
   CSlabOffsetGrid* pGrid = (CSlabOffsetGrid*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
   if ( pDX->m_bSaveAndValidate )
   {
      pGrid->UpdateSlabOffsetData();
   }
   else
   {
      pGrid->FillGrid();
   }
}

void DDX_FilletGrid(CDataExchange* pDX,INT nIDC)
{
   CFilletGrid* pGrid = (CFilletGrid*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
   if ( pDX->m_bSaveAndValidate )
   {
      pGrid->UpdateFilletData();
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

   DDX_Control(pDX, IDC_SLABOFFSET_HYPERLINK, m_ctrlSlabOffsetHyperLink);
   DDX_Control(pDX, IDC_FILLET_HYPERLINK, m_ctrlFilletHyperLink);
   DDX_Control(pDX, IDC_GIRDERTYPE_HYPERLINK, m_ctrlGirderTypeHyperLink);

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();

   std::_tstring strGirderName = pParent->m_pGirder->GetGirderName();
   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,strGirderName);
   if ( pDX->m_bSaveAndValidate )
   {
      GirderIndexType gdrIdx = pParent->m_pGirder->GetIndex();
      GroupIndexType girderTypeGroupIdx = pParent->m_pGirder->GetGirderGroup()->CreateGirderTypeGroup(gdrIdx,gdrIdx);
      pParent->m_pGirder->GetGirderGroup()->SetGirderName(girderTypeGroupIdx,strGirderName.c_str());
   }

   DDX_Strand(pDX,IDC_STRAND,&pParent->m_pGirder->GetPostTensioning()->pStrand);

   DDX_SlabOffsetGrid(pDX,IDC_SLABOFFSET_GRID);

   DDX_FilletGrid(pDX,IDC_FILLET_GRID);

   Float64 conditionFactor;
   pgsTypes::ConditionFactorType conditionFactorType;
   pgsTypes::DuctType ductType;
   pgsTypes::StrandInstallationType installationType;
   if ( pDX->m_bSaveAndValidate )
   {
      // data coming out of dialog

      DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
      DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);

      pParent->m_pGirder->SetConditionFactor(conditionFactor);
      pParent->m_pGirder->SetConditionFactorType(conditionFactorType);

      DDX_CBEnum(pDX, IDC_DUCT_TYPE, ductType);
      pParent->m_pGirder->GetPostTensioning()->DuctType = ductType;

      DDX_CBEnum(pDX, IDC_INSTALLATION_TYPE, installationType );
      pParent->m_pGirder->GetPostTensioning()->InstallationType = installationType;
   }
   else
   {
      // data going into of dialog
      conditionFactor     = pParent->m_pGirder->GetConditionFactor();
      conditionFactorType = pParent->m_pGirder->GetConditionFactorType();
      DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
      DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);

      ductType = pParent->m_pGirder->GetPostTensioning()->DuctType;
      DDX_CBEnum(pDX, IDC_DUCT_TYPE, ductType);

      installationType = pParent->m_pGirder->GetPostTensioning()->InstallationType;
      DDX_CBEnum(pDX, IDC_INSTALLATION_TYPE, installationType);
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
         CString strMsg = pTimelineMgr->GetErrorMessage(result);
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
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffsetType,OnChangeSlabOffsetType)
   ON_REGISTERED_MESSAGE(MsgChangeFilletType,OnChangeFilletType)
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderType,OnChangeGirderType)
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

   // initialize slab offset grid
   m_SlabOffsetGrid.SubclassDlgItem(IDC_SLABOFFSET_GRID,this);
   m_SlabOffsetGrid.CustomInit(pParent->m_pGirder);

   // initialize Fillet grid
   m_FilletGrid.SubclassDlgItem(IDC_FILLET_GRID,this);
   m_FilletGrid.CustomInit(pParent->m_pGirder);

   // subclass the schematic drawing of the tendons
   m_DrawTendons.SubclassDlgItem(IDC_TENDONS,this);
   m_DrawTendons.CustomInit(pParent->m_GirderKey,pParent->m_pGirder);

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

   m_ctrlGirderTypeHyperLink.ModifyLinkStyle(CHyperLink::StyleAutoSize,0);

   CPropertyPage::OnInitDialog();


   OnConditionFactorTypeChanged();

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   UpdateFilletHyperLink();
   UpdateFilletControls();

   UpdateGirderTypeHyperLink();
   UpdateGirderTypeControls();

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
   matPsStrand::Size cur_size = matPsStrand::D1270;
   if ( cur_sel != CB_ERR )
   {
      Int32 cur_key = (Int32)pList->GetItemData( cur_sel );
      const matPsStrand* pCurStrand = pPool->GetStrand( cur_key );
      cur_size = pCurStrand->GetSize();
   }

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for ( int i = 0; i < 2; i++ )
   {
      matPsStrand::Grade grade = (i == 0 ? matPsStrand::Gr1725 : matPsStrand::Gr1860);
      for ( int j = 0; j < 2; j++ )
      {
         matPsStrand::Type type = (j == 0 ? matPsStrand::LowRelaxation : matPsStrand::StressRelieved);

         lrfdStrandIter iter(grade,type);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const matPsStrand* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );

            if ( idx != CB_ERR )
            { 
               // if there wasn't an error adding the size, add a data item
               Int32 key;
               key = pPool->GetStrandKey( pStrand );

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

void CSplicedGirderGeneralPage::FillStrandList(CComboBox* pList,matPsStrand::Grade grade,matPsStrand::Type  type)
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
      const matPsStrand* pStrand = iter.GetCurrentStrand();
      std::_tstring size = matPsStrand::GetSize( pStrand->GetSize(), bUnitsUS );
      int idx = pList->AddString( size.c_str() );

      if ( idx != CB_ERR )
      { 
         // if there wasn't an error adding the size, add a data item
         Int32 key;
         key = pPool->GetStrandKey( pStrand );

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

const matPsStrand* CSplicedGirderGeneralPage::GetStrand()
{
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND );
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   int cursel = pList->GetCurSel();
   Int32 key = (Int32)pList->GetItemData(cursel);
   return pPool->GetStrand(key);
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

LRESULT CSplicedGirderGeneralPage::OnChangeSlabOffsetType(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   if ( pBridge->GetSlabOffsetType() == pgsTypes::sotBridge || pBridge->GetSlabOffsetType() == pgsTypes::sotPier )
   {
      pBridge->SetSlabOffsetType(pgsTypes::sotGirder);
   }
   else
   {
#pragma Reminder("UPDATE: need to deal with multiple offset values")
      // if switching from girder to bridge mode and the slab offsets are different, ask the user
      // which one is to be used for the entire bridge

      pBridge->SetSlabOffsetType(pgsTypes::sotBridge);
   }

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   return 0;
}

void CSplicedGirderGeneralPage::UpdateSlabOffsetHyperLink()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   if ( pBridge->GetSlabOffsetType() == pgsTypes::sotGirder )
   {
      // slab offset is by girder
      m_ctrlSlabOffsetHyperLink.SetWindowText(_T("Slab Offsets are defined girder by girder"));
      m_ctrlSlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for the entire bridge"));
   }
   else if ( pBridge->GetSlabOffsetType() == pgsTypes::sotBridge )
   {
      m_ctrlSlabOffsetHyperLink.SetWindowText(_T("A single Slab Offset is used for the entire bridge"));
      m_ctrlSlabOffsetHyperLink.SetURL(_T("Click to define Slab Offsets by girder"));
   }
   else
   {
      m_ctrlSlabOffsetHyperLink.SetWindowText(_T("A unique Slab Offset is used at each Pier"));
      m_ctrlSlabOffsetHyperLink.SetURL(_T("Click to define Slab Offsets by girder"));
   }
}

void CSplicedGirderGeneralPage::UpdateSlabOffsetControls()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   BOOL bEnable = ( pBridge->GetSlabOffsetType() == pgsTypes::sotGirder ? TRUE : FALSE );
   m_SlabOffsetGrid.EnableWindow(bEnable);
}

LRESULT CSplicedGirderGeneralPage::OnChangeFilletType(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   if ( pBridge->GetFilletType() == pgsTypes::fttBridge || pBridge->GetFilletType() == pgsTypes::fttSpan )
   {
      pBridge->SetFilletType(pgsTypes::fttGirder);
   }
   else
   {
      pBridge->SetFilletType(pgsTypes::fttBridge);
   }

   UpdateFilletHyperLink();
   UpdateFilletControls();

   return 0;
}

void CSplicedGirderGeneralPage::UpdateFilletHyperLink()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   if ( pBridge->GetFilletType() == pgsTypes::fttGirder )
   {
      // slab offset is by girder
      m_ctrlFilletHyperLink.SetWindowText(_T("Fillets are defined girder by girder"));
      m_ctrlFilletHyperLink.SetURL(_T("Click to use this Fillet for the entire bridge"));
   }
   else if ( pBridge->GetFilletType() == pgsTypes::fttBridge )
   {
      m_ctrlFilletHyperLink.SetWindowText(_T("A single Fillet is used for the entire bridge"));
      m_ctrlFilletHyperLink.SetURL(_T("Click to define Fillets by girder"));
   }
   else
   {
      m_ctrlFilletHyperLink.SetWindowText(_T("A unique Fillet is used at each Span"));
      m_ctrlFilletHyperLink.SetURL(_T("Click to define Fillets by girder"));
   }
}

void CSplicedGirderGeneralPage::UpdateFilletControls()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   BOOL bEnable = ( pBridge->GetFilletType() == pgsTypes::fttGirder ? TRUE : FALSE );
   m_FilletGrid.EnableWindow(bEnable);
}

LRESULT CSplicedGirderGeneralPage::OnChangeGirderType(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();
   pBridge->UseSameGirderForEntireBridge( !pBridge->UseSameGirderForEntireBridge() );

   UpdateGirderTypeHyperLink();
   UpdateGirderTypeControls();

   return 0;
}

void CSplicedGirderGeneralPage::UpdateGirderTypeHyperLink()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->m_pGirder->GetGirderGroup()->GetBridgeDescription();

   if ( pBridge->UseSameGirderForEntireBridge() )
   {
      // same girder type used for bridge
      m_ctrlGirderTypeHyperLink.SetWindowText(_T("The same segment type is used for the entire bridge"));
      m_ctrlGirderTypeHyperLink.SetURL(_T("Click to use a unique segment type for each girder"));
   }
   else
   {
      m_ctrlGirderTypeHyperLink.SetWindowText(_T("A unique segment type is used for each girder"));
      m_ctrlGirderTypeHyperLink.SetURL(_T("Click to use the same segment type for the entire bridge"));
   }
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
