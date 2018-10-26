///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

   DDX_Control(pDX, IDC_HYPERLINK, m_ctrlSlabOffsetHyperLink);

   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   std::_tstring strGirderName = pParent->m_pGirder->GetGirderName();
   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,strGirderName);
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_pGirder->SetGirderName(strGirderName.c_str());
   }

   DDX_Strand(pDX,IDC_STRAND,&pParent->m_pGirder->GetPostTensioning()->pStrand);

   DDV_GXGridWnd(pDX,&m_DuctGrid);
   DDX_PTData(pDX,IDC_DUCT_GRID,pParent->m_pGirder->GetPostTensioning());

   DDX_SlabOffsetGrid(pDX,IDC_SLABOFFSET_GRID);

   // Validate the timeline
   if ( pDX->m_bSaveAndValidate )
   {
      CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();

      DuctIndexType nDucts = pParent->m_pGirder->GetPostTensioning()->GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         pTimelineMgr->SetStressTendonEventByIndex(pParent->m_GirderKey,ductIdx,m_TendonStressingEvent[ductIdx]);
      }

      SegmentIndexType nCP = pParent->m_pGirder->GetClosureJointCount();
      for ( SegmentIndexType cpIdx = 0; cpIdx < nCP; cpIdx++ )
      {
         const CClosureJointData* pCP = pParent->m_pGirder->GetClosureJoint(cpIdx);
         IDType cpID = pCP->GetID();
         pTimelineMgr->SetCastClosureJointEventByIndex(cpID,m_CastClosureEvent[cpIdx]);
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


   Float64 conditionFactor;
   pgsTypes::ConditionFactorType conditionFactorType;
   if ( pDX->m_bSaveAndValidate )
   {
      // data coming out of dialog
      DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
      DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);

      pParent->m_pGirder->SetConditionFactor(conditionFactor);
      pParent->m_pGirder->SetConditionFactorType(conditionFactorType);
   }
   else
   {
      // data going into of dialog
      conditionFactor     = pParent->m_pGirder->GetConditionFactor();
      conditionFactorType = pParent->m_pGirder->GetConditionFactorType();

      DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
      DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);
   }
}


BEGIN_MESSAGE_MAP(CSplicedGirderGeneralPage, CPropertyPage)
   ON_BN_CLICKED(IDC_ADD, &CSplicedGirderGeneralPage::OnAddDuct)
   ON_BN_CLICKED(IDC_DELETE, &CSplicedGirderGeneralPage::OnDeleteDuct)
   ON_CBN_SELCHANGE(IDC_GRADE, &CSplicedGirderGeneralPage::OnStrandChanged)
   ON_CBN_SELCHANGE(IDC_TYPE, &CSplicedGirderGeneralPage::OnStrandChanged)
   ON_CBN_SELCHANGE(IDC_STRAND_SIZE, &CSplicedGirderGeneralPage::OnStrandSizeChanged)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, &CSplicedGirderGeneralPage::OnConditionFactorTypeChanged)
   ON_BN_CLICKED(IDHELP, &CSplicedGirderGeneralPage::OnHelp)
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffsetType,OnChangeSlabOffsetType)
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

   // subclass the schematic drawing of the tendons
   m_DrawTendons.SubclassDlgItem(IDC_TENDONS,this);
   m_DrawTendons.CustomInit(this);

   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();

   DuctIndexType nDucts = pParent->m_pGirder->GetPostTensioning()->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(pParent->m_GirderKey,ductIdx);
      m_TendonStressingEvent.push_back(eventIdx);
   }

   IndexType nClosureJoints = pParent->m_pGirder->GetClosureJointCount();
   for ( IndexType idx = 0; idx < nClosureJoints; idx++ )
   {
      const CPrecastSegmentData* pSegment = pParent->m_pGirder->GetSegment(idx);
      SegmentIDType segID = pSegment->GetID();
      EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(segID);
      m_CastClosureEvent.push_back(eventIdx);
   }


   FillStrandList(IDC_STRAND);

   FillGirderComboBox();

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   CPropertyPage::OnInitDialog();

   OnConditionFactorTypeChanged();

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
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
      eventIdx = m_TendonStressingEvent.back();

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
      pList->SetCurSel( new_cur_sel );
   else
      pList->SetCurSel( pList->GetCount()-1 );
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

const CSplicedGirderData* CSplicedGirderGeneralPage::GetGirder()
{
#pragma Reminder("UPDATE: is this really needed?")
   // this is for that callback interface for the drawing control... seems like
   // we could just initialize the drawing control with this data and get rid
   // of the interface
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   //pParent->m_pGirder->SetPostTensioning( m_DuctGrid.GetPTData() );
   return pParent->m_pGirder;
}

const CGirderKey& CSplicedGirderGeneralPage::GetGirderKey()
{
#pragma Reminder("UPDATE: is this really needed?")
   // this is for that callback interface for the drawing control... seems like
   // we could just initialize the drawing control with this data and get rid
   // of the interface
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)GetParent();
   return pParent->m_GirderKey;
}

void CSplicedGirderGeneralPage::OnStrandSizeChanged()
{
   m_DuctGrid.OnStrandChanged();
}

void CSplicedGirderGeneralPage::OnStrandChanged()
{
   CComboBox* pcbGrade = (CComboBox*)GetDlgItem(IDC_GRADE);
   CComboBox* pcbType  = (CComboBox*)GetDlgItem(IDC_TYPE);
   CComboBox* pList    = (CComboBox*)GetDlgItem(IDC_STRAND_SIZE);

   int cursel = pcbGrade->GetCurSel();
   matPsStrand::Grade grade = (matPsStrand::Grade)pcbGrade->GetItemData(cursel);

   cursel = pcbType->GetCurSel();
   matPsStrand::Type type = (matPsStrand::Type)pcbType->GetItemData(cursel);

   cursel = pList->GetCurSel();
   Uint32 key = (Uint32)pList->GetItemData(cursel);
   matPsStrand::Size size = lrfdStrandPool::GetInstance()->GetStrand(key)->GetSize();

   FillStrandList(pList,grade,type);

   int nItems = pList->GetCount();
   for ( int i = 0; i < nItems; i++ )
   {
      Uint32 key = (Uint32)pList->GetItemData(i);
      if (size == lrfdStrandPool::GetInstance()->GetStrand(key)->GetSize() )
      {
         pList->SetCurSel(i);
         break;
      }
   }

   m_DuctGrid.OnStrandChanged();
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
#pragma Reminder("IMPLEMENT")
   AfxMessageBox(_T("Add Help topic"));
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
//      if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
         m_ctrlSlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for the entire bridge"));
//      else
//         m_ctrlSlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for this span"));
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
