///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// BridgeDescPrestressPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "Resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescPrestressPage.h"
#include "GirderDescDlg.h"
#include "DebondDlg.h"

#include <Material\PsStrand.h>
#include <LRFD\StrandPool.h>

#include <MfcTools\CustomDDX.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#include <PsgLib\GirderLibraryEntry.h>

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescPrestressPage property page

IMPLEMENT_DYNCREATE(CGirderDescPrestressPage, CPropertyPage)

CGirderDescPrestressPage::CGirderDescPrestressPage() : 
CPropertyPage(CGirderDescPrestressPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescPrestressPage)
	m_StrandSizeIdx = -1;
	m_TempStrandSizeIdx = -1;
	//}}AFX_DATA_INIT
}

CGirderDescPrestressPage::~CGirderDescPrestressPage()
{
}


void CGirderDescPrestressPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescPrestressPage)
	//}}AFX_DATA_MAP
   DDX_Tag( pDX, IDC_HPOFFSET_END_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_HPOFFSET_HP_UNIT, pDisplayUnits->GetComponentDimUnit() );


   DDX_CBIndex(pDX, IDC_STRAND_INPUT_TYPE, pParent->m_GirderData.NumPermStrandsType);

   if (!pDX->m_bSaveAndValidate)
   {
      ShowHideNumStrandControls(pParent->m_GirderData.NumPermStrandsType);
   }

   if (pParent->m_GirderData.NumPermStrandsType == NPS_TOTAL_NUMBER)
   {
      DDX_Text(pDX, IDC_NUM_HS, pParent->m_GirderData.Nstrands[pgsTypes::Permanent]);
      DDX_Check_Bool(pDX, IDC_HS_JACK, pParent->m_GirderData.bPjackCalculated[pgsTypes::Permanent]);

      if (!pDX->m_bSaveAndValidate)
      {
         DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, pParent->m_GirderData.Pjack[pgsTypes::Permanent], pDisplayUnits->GetGeneralForceUnit() );
      }
      else
      {
         // value is dialog is chopped. recompute to get full precision
         if (pParent->m_GirderData.bPjackCalculated[pgsTypes::Permanent])
         {
            pParent->m_GirderData.Pjack[pgsTypes::Permanent] = GetMaxPjack( pParent->m_GirderData.Nstrands[pgsTypes::Permanent], pgsTypes::Permanent );
         }
         else
         {
            DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, pParent->m_GirderData.Pjack[pgsTypes::Permanent],   pDisplayUnits->GetGeneralForceUnit() );
         }
         DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, pParent->m_GirderData.Pjack[pgsTypes::Permanent], GetUltPjack( pParent->m_GirderData.Nstrands[pgsTypes::Permanent], pgsTypes::Permanent ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
      }

      // compute number of straight and harped based on num permanent for possible later use below
      pStrandGeometry->ComputeNumPermanentStrands( pParent->m_GirderData.Nstrands[pgsTypes::Permanent], 
                                                   pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                   &(pParent->m_GirderData.Nstrands[pgsTypes::Straight]), 
                                                   &(pParent->m_GirderData.Nstrands[pgsTypes::Harped]));

      UpdateStraightHarped(pParent->m_GirderData.Nstrands[pgsTypes::Straight], 
                           pParent->m_GirderData.Nstrands[pgsTypes::Harped]);

   }
   else 
   {
      ATLASSERT(pParent->m_GirderData.NumPermStrandsType == NPS_STRAIGHT_HARPED);

      DDX_Text(pDX, IDC_NUM_HS, pParent->m_GirderData.Nstrands[pgsTypes::Harped]);
      DDX_Check_Bool(pDX, IDC_HS_JACK, pParent->m_GirderData.bPjackCalculated[pgsTypes::Harped]);

      if (pDX->m_bSaveAndValidate && pParent->m_GirderData.bPjackCalculated[pgsTypes::Harped])
      {
         pParent->m_GirderData.Pjack[pgsTypes::Harped] = GetMaxPjack( pParent->m_GirderData.Nstrands[pgsTypes::Harped], pgsTypes::Harped );
      }
      else
      {
         DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, pParent->m_GirderData.Pjack[pgsTypes::Harped],   pDisplayUnits->GetGeneralForceUnit() );
      }

      DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, pParent->m_GirderData.Pjack[pgsTypes::Harped], GetUltPjack( pParent->m_GirderData.Nstrands[pgsTypes::Harped], pgsTypes::Harped ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s"));


	   DDX_Text(pDX, IDC_NUM_SS, pParent->m_GirderData.Nstrands[pgsTypes::Straight]);
	   DDX_Check_Bool(pDX, IDC_SS_JACK, pParent->m_GirderData.bPjackCalculated[pgsTypes::Straight]);

      if (pDX->m_bSaveAndValidate && pParent->m_GirderData.bPjackCalculated[pgsTypes::Straight])
      {
         pParent->m_GirderData.Pjack[pgsTypes::Straight] = GetMaxPjack( pParent->m_GirderData.Nstrands[pgsTypes::Straight], pgsTypes::Straight );
      }
      else
      {
         DDX_UnitValueAndTag( pDX, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, pParent->m_GirderData.Pjack[pgsTypes::Straight], pDisplayUnits->GetGeneralForceUnit() );
      }

      DDV_UnitValueLimitOrLess( pDX, IDC_SS_JACK_FORCE, pParent->m_GirderData.Pjack[pgsTypes::Straight], GetUltPjack( pParent->m_GirderData.Nstrands[pgsTypes::Straight], pgsTypes::Straight ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
   }

	DDX_Text(pDX, IDC_NUM_TEMP, pParent->m_GirderData.Nstrands[pgsTypes::Temporary]);
	DDX_Check_Bool(pDX, IDC_TEMP_JACK, pParent->m_GirderData.bPjackCalculated[pgsTypes::Temporary]);
   DDX_CBItemData(pDX, IDC_TTS_USE, pParent->m_GirderData.TempStrandUsage);

   if (pDX->m_bSaveAndValidate && pParent->m_GirderData.bPjackCalculated[pgsTypes::Temporary])
   {
      pParent->m_GirderData.Pjack[pgsTypes::Temporary] = GetMaxPjack( pParent->m_GirderData.Nstrands[pgsTypes::Temporary], pgsTypes::Temporary );
   }
   else
   {
      DDX_UnitValueAndTag( pDX, IDC_TEMP_JACK_FORCE, IDC_TEMP_JACK_FORCE_UNIT, pParent->m_GirderData.Pjack[pgsTypes::Temporary],  pDisplayUnits->GetGeneralForceUnit() );
   }

   DDV_UnitValueLimitOrLess( pDX, IDC_TEMP_JACK_FORCE, pParent->m_GirderData.Pjack[pgsTypes::Temporary], GetUltPjack( pParent->m_GirderData.Nstrands[pgsTypes::Temporary], pgsTypes::Temporary ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );

   // Set up pjack controls - values that are auto-computed will be refreshed
   UpdateStrandControls();

   // adjustment of harped strands at ends
   m_AllowEndAdjustment = 0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx);

   if (m_AllowEndAdjustment)
   {
      if (!pDX->m_bSaveAndValidate)
      {
         // must convert data from legacy data files
         if (pParent->m_GirderData.HsoEndMeasurement==hsoLEGACY)
         {
            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                                                   pParent->m_GirderData.Nstrands[pgsTypes::Harped],
                                                                                   pParent->m_GirderData.HsoEndMeasurement, 
                                                                                   pParent->m_GirderData.HpOffsetAtEnd);

            Float64 topcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(pParent->m_CurrentSpanIdx, 
                                                                                       pParent->m_CurrentGirderIdx, 
                                                                                       pParent->m_GirderData.Nstrands[pgsTypes::Harped], 
                                                                                       hsoCGFROMTOP, 
                                                                                       absol_offset);

            pParent->m_GirderData.HsoEndMeasurement = hsoCGFROMTOP;
            pParent->m_GirderData.HpOffsetAtEnd     = topcg_offset;
         }

         UpdateEndRangeLength(pParent->m_GirderData.HsoEndMeasurement,pParent->m_GirderData.Nstrands[pgsTypes::Harped]);
      }

      DDX_UnitValueAndTag( pDX, IDC_HPOFFSET_END, IDC_HPOFFSET_END_UNIT, pParent->m_GirderData.HpOffsetAtEnd, pDisplayUnits->GetComponentDimUnit() );
   	DDX_CBItemData(pDX, IDC_HP_COMBO_END, pParent->m_GirderData.HsoEndMeasurement);

      if ( pParent->m_GirderData.Nstrands[pgsTypes::Harped] <=0)
         HideEndOffsetControls(TRUE);
   }
   else
   {
      HideEndOffsetControls(TRUE);
   }

   m_AllowHpAdjustment = 0.0 <= pStrandGeometry->GetHarpedHpOffsetIncrement(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx);

   if (m_AllowHpAdjustment)
   {
      // must convert data from legacy data files
      if (!pDX->m_bSaveAndValidate)
      {
         if(pParent->m_GirderData.HsoHpMeasurement==hsoLEGACY)
         {
            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx,
                                                                                  pParent->m_GirderData.Nstrands[pgsTypes::Harped],
                                                                                  pParent->m_GirderData.HsoHpMeasurement, 
                                                                                  pParent->m_GirderData.HpOffsetAtHp);

            Float64 botcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                                                      pParent->m_GirderData.Nstrands[pgsTypes::Harped], 
                                                                                      hsoCGFROMBOTTOM, absol_offset);

            pParent->m_GirderData.HsoHpMeasurement = hsoCGFROMBOTTOM;
            pParent->m_GirderData.HpOffsetAtHp     = botcg_offset;
         }

         UpdateHpRangeLength(pParent->m_GirderData.HsoHpMeasurement,pParent->m_GirderData.Nstrands[pgsTypes::Harped]);
      }

      DDX_UnitValueAndTag( pDX, IDC_HPOFFSET_HP, IDC_HPOFFSET_HP_UNIT, pParent->m_GirderData.HpOffsetAtHp, pDisplayUnits->GetComponentDimUnit() );
   	DDX_CBItemData(pDX, IDC_HP_COMBO_HP, pParent->m_GirderData.HsoHpMeasurement);

      if ( pParent->m_GirderData.Nstrands[pgsTypes::Harped] <= 0)
         HideHpOffsetControls(TRUE);
   }
   else
   {
      HideHpOffsetControls(TRUE);
   }

   if (pDX->m_bSaveAndValidate)
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      GET_IFACE2(pBroker,IGirder,pGirder);

      // determine if offset strands are within girder bounds
      if (pParent->m_GirderData.Nstrands[pgsTypes::Harped] > 0)
      {
         // girder ends
         // first convert to abosolute offsets
         Float64 absol_offset;
         if (m_AllowEndAdjustment)
         {
            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                                                   pParent->m_GirderData.Nstrands[pgsTypes::Harped],
                                                                                   pParent->m_GirderData.HsoEndMeasurement, 
                                                                                   pParent->m_GirderData.HpOffsetAtEnd);

            Float64 max_end_offset, min_end_offset;
            pStrandGeometry->GetHarpedEndOffsetBoundsEx(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                        pParent->m_GirderData.Nstrands[pgsTypes::Harped], 
                                                        &min_end_offset, &max_end_offset);

            if( absol_offset > max_end_offset+TOLERANCE )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_HPOFFSET_END);
	            AfxMessageBox( _T("Harped strand offset is excessive at ends of girder - Strand lies above allowable cover"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }

            if( absol_offset < min_end_offset-TOLERANCE )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_HPOFFSET_END);
	            AfxMessageBox( _T("Harped strand offset is excessive at ends of girder - Strand lies below allowable cover"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }
         }

         // harping points
         if (m_AllowHpAdjustment)
         {
            absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                                          pParent->m_GirderData.Nstrands[pgsTypes::Harped],
                                                                          pParent->m_GirderData.HsoHpMeasurement, 
                                                                          pParent->m_GirderData.HpOffsetAtHp);

            Float64 max_hp_offset, min_hp_offset;
            pStrandGeometry->GetHarpedHpOffsetBoundsEx(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, 
                                                       pParent->m_GirderData.Nstrands[pgsTypes::Harped], 
                                                       &min_hp_offset, &max_hp_offset);

            if( absol_offset > max_hp_offset+TOLERANCE )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_HPOFFSET_HP);
	            AfxMessageBox( _T("Harped strand offset is excessive at harping points - Strand lies above allowable cover"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }

            if( absol_offset < min_hp_offset-TOLERANCE )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_HPOFFSET_HP);
	            AfxMessageBox( _T("Harped strand offset is excessive at harping points - Strand lies below allowable cover"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }
         }
      }

      // make sure we don't have more debonded strands than total strands
      StrandIndexType nStrands = pParent->GetStraightStrandCount();
      if (0 < nStrands)
      {
         std::vector<CDebondInfo>::iterator it=pParent->m_GirderData.Debond[pgsTypes::Straight].begin();
         while ( it!=pParent->m_GirderData.Debond[pgsTypes::Straight].end() )
         {
            if (it->idxStrand1 > nStrands || it->idxStrand2>nStrands)
            {
               it = pParent->m_GirderData.Debond[pgsTypes::Straight].erase(it);
            }
            else
            {
               it++;
            }
         }
      }
      else
      {
         pParent->m_GirderData.Debond[pgsTypes::Straight].clear();
      }
   }


	DDX_CBIndex(pDX, IDC_STRAND_SIZE, m_StrandSizeIdx);
	DDX_CBIndex(pDX, IDC_TEMP_STRAND_SIZE, m_TempStrandSizeIdx);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
      Int32 key = pList->GetItemData( m_StrandSizeIdx );
      pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Straight] = pPool->GetStrand( key );
      pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Harped] = pPool->GetStrand( key );

      pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
      key = pList->GetItemData( m_TempStrandSizeIdx );
      pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Temporary] = pPool->GetStrand( key );
   }
}



BEGIN_MESSAGE_MAP(CGirderDescPrestressPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescPrestressPage)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUM_SS_SPIN, OnNumStraightStrandsChanged)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUM_HS_SPIN, OnNumHarpedStrandsChanged)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUM_TEMP_SPIN, OnNumTempStrandsChanged)
	ON_BN_CLICKED(IDC_HS_JACK, OnUpdateHsPjEdit)
	ON_BN_CLICKED(IDC_SS_JACK, OnUpdateSsPjEdit)
	ON_BN_CLICKED(IDC_TEMP_JACK, OnUpdateTempPjEdit)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_HP_COMBO_HP, OnSelchangeHpComboHp)
	ON_CBN_SELCHANGE(IDC_HP_COMBO_END, OnSelchangeHpComboEnd)
	ON_CBN_SELCHANGE(IDC_STRAND_INPUT_TYPE, OnSelchangeStrandInputType)
	ON_WM_CTLCOLOR()
	ON_CBN_DROPDOWN(IDC_HP_COMBO_HP, OnDropdownHpComboHp)
	ON_CBN_DROPDOWN(IDC_HP_COMBO_END, OnDropdownHpComboEnd)
	ON_CBN_SELCHANGE(IDC_STRAND_SIZE, OnStrandTypeChanged)
	ON_CBN_SELCHANGE(IDC_TEMP_STRAND_SIZE, OnTempStrandTypeChanged)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescPrestressPage message handlers

BOOL CGirderDescPrestressPage::OnInitDialog() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   // Fill the strand size combo box.
   UpdateStrandList(IDC_STRAND_SIZE);
   UpdateStrandList(IDC_TEMP_STRAND_SIZE);

   // Select the strand size
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int32 target_key = pPool->GetStrandKey(pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Straight] );
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   int cStrands = pList->GetCount();
   for ( int i = 0; i < cStrands; i++ )
   {
      Int32 key = pList->GetItemData( i );
      if ( key == target_key )
      {
         m_StrandSizeIdx = i;
         break;
      }
   }

   target_key = pPool->GetStrandKey(pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Temporary] );
   pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
   cStrands = pList->GetCount();
   for ( int i = 0; i < cStrands; i++ )
   {
      Int32 key = pList->GetItemData( i );
      if ( key == target_key )
      {
         m_TempStrandSizeIdx = i;
         break;
      }
   }

   // All this work has to be done before CPropertyPage::OnInitDialog().
   // This code sets up the "current" selections which must be done prior to
   // calling DoDataExchange.  OnInitDialog() calls DoDataExchange().

   // Set the OK button as the default button
   SendMessage (DM_SETDEFID, IDOK);

   InitHarpStrandOffsetMeasureComboBox( (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP) );
   InitHarpStrandOffsetMeasureComboBox( (CComboBox*)GetDlgItem(IDC_HP_COMBO_END) );

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TTS_USE);
   int idx = pCB->AddString(_T("Temporary strands pretensioned with permanent strands"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::ttsPretensioned);

   idx = pCB->AddString(_T("Temporary strands post-tensioned before lifting"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::ttsPTBeforeLifting);

   idx = pCB->AddString(_T("Temporary strands post-tensioned immedately after lifting"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::ttsPTAfterLifting);

   idx = pCB->AddString(_T("Temporary strands post-tensioned immedately before shipping"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::ttsPTBeforeShipping);

   CPropertyPage::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetMaxStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx,pgsTypes::Temporary);
   if ( Nt == 0 )
   {
      // this girder can't have temporary strands so hide the input
      // this makes PGSuper look better to agencies that don't use temporary strands
      GetDlgItem(IDC_TEMP_TITLE)->ShowWindow(FALSE);
      GetDlgItem(IDC_NUM_TEMP)->ShowWindow(FALSE);
      GetDlgItem(IDC_NUM_TEMP_SPIN)->ShowWindow(FALSE);
      GetDlgItem(IDC_TEMP_JACK)->ShowWindow(FALSE);
      GetDlgItem(IDC_TEMP_JACK_FORCE)->ShowWindow(FALSE);
      GetDlgItem(IDC_TEMP_JACK_FORCE_UNIT)->ShowWindow(FALSE);
      GetDlgItem(IDC_TTS_USE)->ShowWindow(FALSE);
      GetDlgItem(IDC_TEMP_STRAND_SIZE)->ShowWindow(FALSE);
      GetDlgItem(IDC_TEMP_STRAND_SIZE_LABEL)->ShowWindow(FALSE);
   }

   OnDropdownHpComboHp();
   OnDropdownHpComboEnd();

   EnableToolTips(TRUE);


   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescPrestressPage::InitHarpStrandOffsetMeasureComboBox(CComboBox* pCB)
{
   pCB->Clear();
   int idx;
   
   idx = pCB->AddString(_T("Distance between CG of Harped Group and Girder Top"));
   pCB->SetItemData(idx,hsoCGFROMTOP);

   idx = pCB->AddString(_T("Distance between CG of Harped Group and Girder Bottom"));
   pCB->SetItemData(idx,hsoCGFROMBOTTOM);

   idx = pCB->AddString(_T("Distance between Top-Most Harped Strand and Girder Top"));
   pCB->SetItemData(idx,hsoTOP2TOP);

   idx = pCB->AddString(_T("Distance between Top-Most Harped Strand and Girder Bottom"));
   pCB->SetItemData(idx,hsoTOP2BOTTOM);

   idx = pCB->AddString(_T("Distance between Bottom-Most Harped Strand and Girder Bottom"));
   pCB->SetItemData(idx,hsoBOTTOM2BOTTOM);

   idx = pCB->AddString(_T("Eccentricity of Harped Strand Group (Non-Composite Section)"));
   pCB->SetItemData(idx,hsoECCENTRICITY);
}

StrandIndexType CGirderDescPrestressPage::PermStrandSpinnerInc(IStrandGeometry* pStrands, StrandIndexType currNum, bool bAdd )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nextnum;
   if ( bAdd )
   {
      nextnum = pStrands->GetNextNumPermanentStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, currNum);

      if (nextnum==-1)
         nextnum=currNum; // no increment if we hit the top
   }
   else
   {
      nextnum = pStrands->GetPreviousNumPermanentStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, currNum);

      if (nextnum==-1)
         nextnum=currNum; // no increment if we hit the bottom
   }

   StrandIndexType increment = nextnum - currNum;

   return increment;
}

StrandIndexType CGirderDescPrestressPage::StrandSpinnerInc(IStrandGeometry* pStrands, pgsTypes::StrandType type,StrandIndexType currNum, bool bAdd )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nextnum;
   if ( bAdd )
   {
      nextnum = pStrands->GetNextNumStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, type, currNum);

      if (nextnum==-1)
         nextnum=currNum; // no increment if we hit the top
   }
   else
   {
      nextnum = pStrands->GetPrevNumStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, type, currNum);

      if (nextnum==-1)
         nextnum=currNum; // no increment if we hit the bottom
   }

   StrandIndexType increment = nextnum - currNum;

   return increment;
}

void CGirderDescPrestressPage::OnNumStraightStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType inc = StrandSpinnerInc( pStrandGeom, pgsTypes::Straight, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );

   pNMUpDown->iDelta = inc;

   CWnd* pWnd = GetDlgItem( IDC_SS_JACK_FORCE );
   StrandIndexType nStrands = StrandIndexType(pNMUpDown->iPos + pNMUpDown->iDelta);
   BOOL bCalcPsForce = IsDlgButtonChecked( IDC_SS_JACK );
   pWnd->EnableWindow( nStrands == 0 ? FALSE : (bCalcPsForce ? FALSE : TRUE) );

   Float64 Pjack;
   if ( bCalcPsForce || nStrands == 0 )
   {
      Pjack = GetMaxPjack( nStrands, pgsTypes::Straight );
   }
   else
   {
      Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Straight];
   }

   CDataExchange dx(this,FALSE);
   DDX_UnitValueAndTag( &dx, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );

	*pResult = 0;
}

void CGirderDescPrestressPage::OnNumHarpedStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int cursel = box->GetCurSel();
   if (cursel==NPS_TOTAL_NUMBER)
   {
      pNMUpDown->iDelta  = PermStrandSpinnerInc( pStrandGeom, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );
   }
   else
   {
      pNMUpDown->iDelta  = StrandSpinnerInc( pStrandGeom, pgsTypes::Harped, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );
   }

   CWnd* pWnd = GetDlgItem( IDC_HS_JACK_FORCE );
   long nStrands = pNMUpDown->iPos + pNMUpDown->iDelta;
   BOOL bCalcPsForce = IsDlgButtonChecked( IDC_HS_JACK );
   pWnd->EnableWindow( nStrands == 0 ? FALSE : (bCalcPsForce ? FALSE : TRUE) );
   Float64 Pjack;
   if ( bCalcPsForce || nStrands == 0)
   {
      Pjack = GetMaxPjack( nStrands, pgsTypes::Harped );
   }
   else
   {
      if (cursel==NPS_TOTAL_NUMBER)
      {
         Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Permanent];
      }
      else
      {
         Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Harped];
      }
   }

   CDataExchange dx(this,FALSE);
   DDX_UnitValueAndTag( &dx, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );

   StrandIndexType numHarped;
   if (cursel==NPS_TOTAL_NUMBER)
   {
      StrandIndexType numStraight;
      bool was_valid = pStrandGeom->ComputeNumPermanentStrands(nStrands,pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, &numStraight, &numHarped);
      ATLASSERT(was_valid); // somehow got invalid number of strands into control

      UpdateStraightHarped(numStraight, numHarped);
   }
   else
   {
      numHarped  = nStrands;
   }

   // offsets
   UpdateHarpedOffsets(numHarped);

	*pResult = 0;
}

void CGirderDescPrestressPage::UpdateHarpedOffsets(StrandIndexType numHarped)
{
   if (m_AllowEndAdjustment)
   {
      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_END);
      int cursel = box->GetCurSel();
      HarpedStrandOffsetType measureType = (HarpedStrandOffsetType)box->GetItemData(cursel);

      UpdateEndRangeLength(measureType, numHarped);

      HideEndOffsetControls(numHarped<=0);
   }

   if (m_AllowHpAdjustment)
   {
      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP);
      int cursel = box->GetCurSel();
      HarpedStrandOffsetType measureType = (HarpedStrandOffsetType)box->GetItemData(cursel);

      UpdateHpRangeLength(measureType, numHarped);

      HideHpOffsetControls(numHarped<=0);
   }
}


void CGirderDescPrestressPage::OnNumTempStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   pNMUpDown->iDelta  = StrandSpinnerInc( pStrandGeom, pgsTypes::Temporary, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );

   CWnd* pWnd = GetDlgItem( IDC_TEMP_JACK_FORCE );
   StrandIndexType nStrands = pNMUpDown->iPos + pNMUpDown->iDelta;
   BOOL bCalcPsForce = IsDlgButtonChecked( IDC_TEMP_JACK );
   pWnd->EnableWindow( nStrands == 0 ? FALSE : (bCalcPsForce ? FALSE : TRUE) );
   Float64 Pjack;
   if ( bCalcPsForce || nStrands == 0)
   {
      Pjack = GetMaxPjack( nStrands, pgsTypes::Temporary );
   }
   else
   {
      Float64 Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Temporary];
   }

   CDataExchange dx(this,FALSE);
   DDX_UnitValueAndTag( &dx, IDC_TEMP_JACK_FORCE, IDC_TEMP_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );

	*pResult = 0;
}

void CGirderDescPrestressPage::InitPjackEditEx( UINT nCheckBox )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int cursel = box->GetCurSel();

   UINT nEdit;
   Uint16 nStrands;
   switch( nCheckBox )
   {
   case IDC_HS_JACK:
      nEdit = IDC_HS_JACK_FORCE;
      nStrands = GetDlgItemInt( IDC_NUM_HS );
      break;

   case IDC_SS_JACK:
      ATLASSERT(cursel!=NPS_TOTAL_NUMBER);
      nEdit = IDC_SS_JACK_FORCE;
      nStrands = GetDlgItemInt( IDC_NUM_SS );
      break;

   case IDC_TEMP_JACK:
      nEdit = IDC_TEMP_JACK_FORCE;
      nStrands = GetDlgItemInt( IDC_NUM_TEMP );
      break;
   }

   CButton* chkbox = (CButton*)GetDlgItem(nCheckBox);
   bool bEnable = false;

   if (  nStrands > 0 )
      bEnable = chkbox->GetCheck() == BST_UNCHECKED;

   CWnd* pWnd = GetDlgItem( nEdit );
   ASSERT( pWnd );
   pWnd->EnableWindow( bEnable );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CDataExchange dx(this,FALSE);

   // only update dialog values if they are auto-computed
   if (!bEnable)
   {
      Float64 Pjack = 0;
      switch( nCheckBox )
      {
      case IDC_HS_JACK:
         {
            if (cursel==NPS_TOTAL_NUMBER)
            {
               pParent->m_GirderData.Pjack[pgsTypes::Permanent] = GetMaxPjack(nStrands,pgsTypes::Harped);
               Pjack = pParent->m_GirderData.Pjack[pgsTypes::Permanent];
            }
            else
            {
               pParent->m_GirderData.Pjack[pgsTypes::Harped] = GetMaxPjack(nStrands,pgsTypes::Harped);
               Pjack = pParent->m_GirderData.Pjack[pgsTypes::Harped];
            }
            DDX_UnitValueAndTag( &dx, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         }
         break;

      case IDC_SS_JACK:
         pParent->m_GirderData.Pjack[pgsTypes::Straight] = GetMaxPjack(nStrands,pgsTypes::Straight);
         Pjack = pParent->m_GirderData.Pjack[pgsTypes::Straight];
         DDX_UnitValueAndTag( &dx, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         break;

      case IDC_TEMP_JACK:
         pParent->m_GirderData.Pjack[pgsTypes::Temporary] = GetMaxPjack(nStrands,pgsTypes::Temporary);
         Pjack = pParent->m_GirderData.Pjack[pgsTypes::Temporary];
         DDX_UnitValueAndTag( &dx, IDC_TEMP_JACK_FORCE, IDC_TEMP_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         break;
      }
   }
}

void CGirderDescPrestressPage::UpdatePjackEdit( UINT nCheckBox  )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int cursel = box->GetCurSel();

   UINT nEdit;
   Uint16 nStrands;
   switch( nCheckBox )
   {
   case IDC_HS_JACK:
      nEdit = IDC_HS_JACK_FORCE;
      nStrands = GetDlgItemInt( IDC_NUM_HS );
      break;

   case IDC_SS_JACK:
      nEdit = IDC_SS_JACK_FORCE;
      nStrands = GetDlgItemInt( IDC_NUM_SS );
      break;

   case IDC_TEMP_JACK:
      nEdit = IDC_TEMP_JACK_FORCE;
      nStrands = GetDlgItemInt( IDC_NUM_TEMP );
      break;
   }

   BOOL bEnable = IsDlgButtonChecked( nCheckBox ) ? FALSE : TRUE;
   if (  nStrands == 0 )
      bEnable = false; // don't enable if the number of strands is zero

   CWnd* pWnd = GetDlgItem( nEdit );
   ASSERT( pWnd );
   pWnd->EnableWindow( bEnable );

   Float64 Pjack = 0;
   if ( bEnable )
   {
      // Set the edit control value to the last user input force
      switch( nCheckBox )
      {
      case IDC_HS_JACK:
         {
            if (cursel==NPS_TOTAL_NUMBER)
            {
               Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Permanent];
            }
            else
            {
               Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Harped];
            }
         }
         break;

      case IDC_SS_JACK:
         Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Straight];
         break;

      case IDC_TEMP_JACK:
         Pjack = pParent->m_GirderData.LastUserPjack[pgsTypes::Temporary];
         break;
      }
   }
   else if ( nStrands != 0 )
   {
      CDataExchange dx(this,FALSE);
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      // Get the edit control value and save it as the last user input force
      CString val_as_text;
      pWnd->GetWindowText( val_as_text );
      Pjack = _tstof( val_as_text );
      Pjack = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
      
      switch( nCheckBox )
      {
      case IDC_HS_JACK:
         {
            if (cursel==NPS_TOTAL_NUMBER)
            {
               pParent->m_GirderData.LastUserPjack[pgsTypes::Permanent] = Pjack;
               Pjack = GetMaxPjack(nStrands,pgsTypes::Harped);
            }
            else
            {
               pParent->m_GirderData.LastUserPjack[pgsTypes::Harped] = Pjack;
               Pjack = GetMaxPjackHarped();
            }
         DDX_UnitValueAndTag( &dx, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         }
         break;

      case IDC_SS_JACK:
         pParent->m_GirderData.LastUserPjack[pgsTypes::Straight] = Pjack;
         Pjack = GetMaxPjackStraight();
         DDX_UnitValueAndTag( &dx, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         break;

      case IDC_TEMP_JACK:
         pParent->m_GirderData.LastUserPjack[pgsTypes::Temporary] = Pjack;
         Pjack = GetMaxPjackTemp();
         DDX_UnitValueAndTag( &dx, IDC_TEMP_JACK_FORCE, IDC_TEMP_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         break;
      }
   }
}

Float64 CGirderDescPrestressPage::GetMaxPjackStraight()
{
   CString txt;
   CWnd* pEdit = GetDlgItem( IDC_NUM_SS );
   pEdit->GetWindowText( txt );
   StrandIndexType nStrands = (StrandIndexType)_tstoi( txt );
   return GetMaxPjack( nStrands, pgsTypes::Straight );
}

Float64 CGirderDescPrestressPage::GetMaxPjackHarped()
{
   CString txt;
   CWnd* pEdit = GetDlgItem( IDC_NUM_HS );
   pEdit->GetWindowText( txt );
   StrandIndexType nStrands = (StrandIndexType)_tstoi( txt );
   return GetMaxPjack( nStrands, pgsTypes::Harped );
}


Float64 CGirderDescPrestressPage::GetMaxPjackTemp()
{
   CString txt;
   CWnd* pEdit = GetDlgItem( IDC_NUM_TEMP );
   pEdit->GetWindowText( txt );
   StrandIndexType nStrands = (StrandIndexType)_tstoi( txt );
   return GetMaxPjack( nStrands, pgsTypes::Temporary );
}

Float64 CGirderDescPrestressPage::GetMaxPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, IPrestressForce, pPrestress );

   if ( strandType == pgsTypes::Permanent )
      strandType = pgsTypes::Straight;


   // TRICK CODE
   // If strand stresses are limited immediate prior to transfer, prestress losses must be computed between jacking and prestress transfer in 
   // order to compute PjackMax. Losses are computed from transfer to final in one shot. The side of effect of this is that a bridge analysis
   // model must be built and in doing so, live load distribution factors must be computed. If the live load distribution factors cannot
   // be computed because of a range of applicability issue, an exception will be thrown.
   //
   // This exception adversely impacts the behavior of this dialog. To prevent these problems, capture the current ROA setting, change ROA to
   // "Ignore", compute PjackMax, and then restore the ROA setting.

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   LldfRangeOfApplicabilityAction action = pLiveLoads->GetLldfRangeOfApplicabilityAction();
   pLiveLoads->SetLldfRangeOfApplicabilityAction(roaIgnore);

   Float64 PjackMax;
   try
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
      PjackMax = pPrestress->GetPjackMax(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx,*pParent->m_GirderData.Material.pStrandMaterial[strandType], nStrands);
   }
   catch (... )
   {
      pLiveLoads->SetLldfRangeOfApplicabilityAction(action);
      pEvents->CancelPendingEvents();
      throw;
   }

   pLiveLoads->SetLldfRangeOfApplicabilityAction(action);
   pEvents->CancelPendingEvents();

   return PjackMax;
}

Float64 CGirderDescPrestressPage::GetUltPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType)
{
   if ( strandType == pgsTypes::Permanent )
      strandType = pgsTypes::Straight;

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   const matPsStrand& strand = *(pParent->m_GirderData.Material.pStrandMaterial[strandType]);

   // Ultimate strength of strand group
   Float64 ult = strand.GetUltimateStrength();
   Float64 area = strand.GetNominalArea();

   return nStrands*area*ult;
}

void CGirderDescPrestressPage::OnUpdateHsPjEdit() 
{
   UpdatePjackEdit( IDC_HS_JACK );
}

void CGirderDescPrestressPage::OnUpdateSsPjEdit() 
{
   UpdatePjackEdit( IDC_SS_JACK );
}

void CGirderDescPrestressPage::OnUpdateTempPjEdit() 
{
   UpdatePjackEdit( IDC_TEMP_JACK );
}

void CGirderDescPrestressPage::UpdateStrandControls() 
{
	// Each time this page is activated, we need to make sure the valid range for # of
   // strands is correct (i.e. in sync with the selected girder type on the Superstructure
   // page).
   //
   // If the current number of strands exceeds the max number of strands,  set the current
   // number of strands to the max number of strands and recompute the jacking forces.
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int cursel = box->GetCurSel();

   CSpinButtonCtrl* pSpin;
   UDACCEL uda;
   StrandIndexType nStrandsMax;

   // harped/straight or permanent strands
   if (cursel==NPS_TOTAL_NUMBER)
   {
      nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Permanent);

      pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
      uda;
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, short(nStrandsMax) );
   }
   else if  (cursel==NPS_STRAIGHT_HARPED)
   {
      nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight);

      pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_SS_SPIN );
      uda;
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, short(nStrandsMax) );

      nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Harped);

      pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, short(nStrandsMax) );
   }

   // temp strands
   nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Temporary);

   pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_TEMP_SPIN );
   uda.nSec=0;
   uda.nInc=1;
   pSpin->SetAccel(1,&uda);
   pSpin->SetRange( 0, short(nStrandsMax) );


   // Hide the "number of strands and jacking force" controls if the strand pattern
   // does not contain any strand positions
   // We have to do this here instead of in OnInitDialog because the strand pattern
   // objects are initialized here and not in OnInitDialog.  HideControls disables
   // the strand input controls if the pattern cannot accept any strands.
   HideControls(0);  // Straight Strands
   HideControls(1);  // Harped Strands
   HideControls(2);  // Temporary Strands

   InitPjackEdits();
}

HBRUSH CGirderDescPrestressPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr;

   CWnd* z2 = GetDlgItem(IDC_NUM_SS);
   ASSERT(z2);

   CWnd* z3 = GetDlgItem(IDC_NUM_HS);
   ASSERT(z3);

   CWnd* z4 = GetDlgItem(IDC_NUM_TEMP);
   ASSERT(z4);

   if (z2->m_hWnd == pWnd->m_hWnd ||
       z3->m_hWnd == pWnd->m_hWnd ||
       z4->m_hWnd == pWnd->m_hWnd)
   {
      COLORREF ccol = ::GetSysColor(COLOR_WINDOW);
      pDC->SetBkColor(ccol);
      hbr = ::CreateSolidBrush(ccol);
   }
   else
      hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);

	return hbr;
}

void CGirderDescPrestressPage::HideControls(int key)
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   int nTitle;
   int nNumStrand;  // control id for #of strands edit box
   int nNumStrandSpin;
   int nPjackCheck;
   int nPjackEdit;
   int nPjackUnit;
   long nStrandPoints; // Number of strand points

   switch( key )
   {
   case 0: // Straight strands
      nTitle         = IDC_SS_TITLE;
      nNumStrand     = IDC_NUM_SS;
      nNumStrandSpin = IDC_NUM_SS_SPIN;
      nPjackCheck    = IDC_SS_JACK;
      nPjackEdit     = IDC_SS_JACK_FORCE;
      nPjackUnit     = IDC_SS_JACK_FORCE_UNIT;

      nStrandPoints = pStrandGeom->GetMaxStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Straight);
      break;

   case 1: // Harped/Permanent Strands
      {
         nTitle         = IDC_HS_TITLE;
         nNumStrand     = IDC_NUM_HS;
         nNumStrandSpin = IDC_NUM_HS_SPIN;
         nPjackCheck    = IDC_HS_JACK;
         nPjackEdit     = IDC_HS_JACK_FORCE;
         nPjackUnit     = IDC_HS_JACK_FORCE_UNIT;

         CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
         int cursel = box->GetCurSel();
         if (cursel==NPS_TOTAL_NUMBER)
         {
            nStrandPoints = pStrandGeom->GetMaxNumPermanentStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx);
         }
         else
         {
            nStrandPoints = pStrandGeom->GetMaxStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Harped);
         }
      }
      break;

   case 2: // Temporary Strands
      nTitle         = IDC_TEMP_TITLE;
      nNumStrand     = IDC_NUM_TEMP;
      nNumStrandSpin = IDC_NUM_TEMP_SPIN;
      nPjackCheck    = IDC_TEMP_JACK;
      nPjackEdit     = IDC_TEMP_JACK_FORCE;
      nPjackUnit     = IDC_TEMP_JACK_FORCE_UNIT;

      nStrandPoints = pStrandGeom->GetMaxStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Temporary);
      break;

   default:
      // Should never get here... Bad key
      CHECK(false);
   }

   BOOL enable = nStrandPoints > 0 ? TRUE : FALSE;

   CWnd* pWnd = 0;

   pWnd = GetDlgItem( nTitle );
   ASSERT(pWnd);
   pWnd->EnableWindow( enable );

   pWnd = GetDlgItem( nNumStrand );
   ASSERT(pWnd);
   pWnd->EnableWindow( enable );

   pWnd = GetDlgItem( nNumStrandSpin );
   ASSERT(pWnd);
   pWnd->EnableWindow( enable );

   pWnd = GetDlgItem( nPjackCheck );
   ASSERT(pWnd);
   pWnd->EnableWindow( enable );

   pWnd = GetDlgItem( nPjackEdit );
   ASSERT(pWnd);
   pWnd->EnableWindow( enable );

   pWnd = GetDlgItem( nPjackUnit );
   ASSERT(pWnd);
   pWnd->EnableWindow( enable );

   if ( key == 2 ) // temporary strands
   {
      pWnd = GetDlgItem(IDC_TTS_USE );
      ASSERT( pWnd);
      pWnd->EnableWindow( enable );
   }
}

void CGirderDescPrestressPage::HideEndOffsetControls(BOOL hide)
{
   CWnd* pWnd = 0;

   BOOL show = hide==TRUE ? FALSE : TRUE;

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_END_TITLE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_END );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_END );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_END_UNIT );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_END_NOTE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );
}

void CGirderDescPrestressPage::HideHpOffsetControls(BOOL hide)
{
   CWnd* pWnd = 0;
   BOOL show = hide==TRUE ? FALSE : TRUE;

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_HP_TITLE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_HP );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP_UNIT );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );
}

void CGirderDescPrestressPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_PRESTRESS );
}


StrandIndexType CGirderDescPrestressPage::GetStraightStrandCount()
{

   StrandIndexType nStraightStrands;
   // see if we are alive yet 
   if (::IsWindow(m_hWnd))
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
      int cursel = box->GetCurSel();
      if (cursel==NPS_TOTAL_NUMBER)
      {
         StrandIndexType total_strands;
         CDataExchange DX(this,true);
	      DDX_Text(&DX, IDC_NUM_HS, total_strands);
         
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

         StrandIndexType numStraight, numHarped;
         pStrandGeometry->ComputeNumPermanentStrands( total_strands, pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, &numStraight, &numHarped);

         nStraightStrands = numStraight;
      }
      else
      {
         CDataExchange DX(this,true);
	      DDX_Text(&DX, IDC_NUM_SS, nStraightStrands);
      }
   }
   else
   {
      ATLASSERT(0);
   }

   return nStraightStrands;
}

StrandIndexType CGirderDescPrestressPage::GetHarpedStrandCount()
{

   StrandIndexType nHarpedStrands;
   if (::IsWindow(m_hWnd))
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
      int cursel = box->GetCurSel();
      if (cursel==NPS_TOTAL_NUMBER)
      {
         StrandIndexType total_strands;
         CDataExchange DX(this,true);
	      DDX_Text(&DX, IDC_NUM_HS, total_strands);

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

         StrandIndexType numStraight, numHarped;
         pStrandGeometry->ComputeNumPermanentStrands( total_strands, pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, &numStraight, &numHarped);

         nHarpedStrands = numHarped;
      }
      else
      {
         CDataExchange DX(this,true);
	      DDX_Text(&DX, IDC_NUM_HS, nHarpedStrands);
      }
   }
   else
   {
      ATLASSERT(0);
   }

   return nHarpedStrands;
}

void CGirderDescPrestressPage::UpdateEndRangeLength(HarpedStrandOffsetType measureType, StrandIndexType Nh)
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CString str;
   if (0 < Nh)
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 lowRange, highRange;
      pStrandGeom->ComputeValidHarpedOffsetForMeasurementTypeEnd(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, Nh, measureType, &lowRange, &highRange);

      lowRange  = ::ConvertFromSysUnits(lowRange, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      highRange = ::ConvertFromSysUnits(highRange,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      Float64 low  = min(lowRange, highRange);
      Float64 high = max(lowRange, highRange);

      if ( IS_SI_UNITS(pDisplayUnits) )
         str.Format(_T("(Valid Range %.1f to %.1f)"), low, high);
      else
         str.Format(_T("(Valid Range %.3f to %.3f)"), low, high);
   }

   CEdit* pEdit = (CEdit*)GetDlgItem( IDC_HPOFFSET_END_NOTE );
   pEdit->SetWindowText(str);
}

void CGirderDescPrestressPage::UpdateHpRangeLength(HarpedStrandOffsetType measureType, StrandIndexType Nh)
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CString str;
   if (0 < Nh)
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 lowRange, highRange;
      pStrandGeom->ComputeValidHarpedOffsetForMeasurementTypeHp(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, Nh, measureType, &lowRange, &highRange);


      lowRange = ::ConvertFromSysUnits(lowRange,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      highRange = ::ConvertFromSysUnits(highRange,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      Float64 low  = min(lowRange, highRange);
      Float64 high = max(lowRange, highRange);

      if ( IS_SI_UNITS(pDisplayUnits) )
         str.Format(_T("(Valid Range %.1f to %.1f)"), low, high);
      else
         str.Format(_T("(Valid Range %.3f to %.3f)"), low, high);
   }

   CEdit* pEdit = (CEdit*)GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   pEdit->SetWindowText(str);
}

void CGirderDescPrestressPage::UpdateStraightHarped(StrandIndexType Ns, StrandIndexType Nh)
{
   CString val_as_text;
   val_as_text.Format(_T("Number of Straight: %d, Harped: %d"),Ns, Nh);
   CWnd* pWnd = GetDlgItem( IDC_HARP_STRAIGHT );
   pWnd->SetWindowText( val_as_text );
}


void CGirderDescPrestressPage::OnSelchangeHpComboHp() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nh = GetHarpedStrandCount();

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP);
   int cursel = box->GetCurSel();
   HarpedStrandOffsetType measureType = (HarpedStrandOffsetType)box->GetItemData(cursel);

   SHORT keyState = GetKeyState(VK_CONTROL);
   if ( keyState < 0 )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strOffset;
      CWnd* pWnd = GetDlgItem(IDC_HPOFFSET_HP);
      pWnd->GetWindowText(strOffset);
      double offset = _tstof(strOffset);

      offset = ::ConvertToSysUnits(offset,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      offset = pStrandGeom->ConvertHarpedOffsetHp(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx,nh,m_OldHpMeasureType, offset, measureType);
      
      strOffset = ::FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false);
      pWnd->SetWindowText(strOffset);
   }

   UpdateHpRangeLength(measureType, nh);
}

void CGirderDescPrestressPage::OnSelchangeHpComboEnd() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   long nh = GetHarpedStrandCount();

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_END);
   int cursel = box->GetCurSel();
   HarpedStrandOffsetType measureType = (HarpedStrandOffsetType)box->GetItemData(cursel);

   SHORT keyState = GetKeyState(VK_CONTROL);
   if ( keyState < 0 )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strOffset;
      CWnd* pWnd = GetDlgItem(IDC_HPOFFSET_END);
      pWnd->GetWindowText(strOffset);
      double offset = _tstof(strOffset);
      offset = ::ConvertToSysUnits(offset,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      offset = pStrandGeom->ConvertHarpedOffsetEnd(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx,nh,m_OldEndMeasureType, offset, measureType);
      
      strOffset = FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false);
      pWnd->SetWindowText(strOffset);
   }

   UpdateEndRangeLength(measureType, nh);
}

void CGirderDescPrestressPage::ShowHideNumStrandControls(int numPermStrandsType)
{
   int show = numPermStrandsType==NPS_TOTAL_NUMBER ? SW_HIDE : SW_SHOW;

   int topids[] = {IDC_SS_TITLE, IDC_NUM_SS, IDC_NUM_SS_SPIN, IDC_SS_JACK, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, -12345};
   for (int id=0; topids[id]!=-12345; id++)
   {
      CWnd* pWnd = GetDlgItem( topids[id] );
      ASSERT( pWnd );
      pWnd->ShowWindow( show );
   }

   // num harped/straight indicator
   int nsshow = numPermStrandsType==NPS_TOTAL_NUMBER ? SW_SHOW : SW_HIDE;
   CWnd* pWnd = GetDlgItem( IDC_HARP_STRAIGHT );
   ASSERT( pWnd );
   pWnd->ShowWindow( nsshow );
   
   // label for strand spinner
   CString msg = numPermStrandsType==NPS_TOTAL_NUMBER ? _T("Total Number of Permanent Strands") : _T("Number of Harped Strands");
   pWnd = GetDlgItem( IDC_HS_TITLE );
   ASSERT( pWnd );
   pWnd->SetWindowText( msg );
}

void CGirderDescPrestressPage::OnSelchangeStrandInputType() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int cursel = box->GetCurSel();

   if (cursel==NPS_TOTAL_NUMBER)
   {
      // convert from num straight, num harped to num total. values might not be compatible
      StrandIndexType num_straight = GetDlgItemInt( IDC_NUM_SS );
      StrandIndexType num_harped   = GetDlgItemInt( IDC_NUM_HS );
      StrandIndexType num_total = num_straight + num_harped;


      StrandIndexType numStraight, numHarped;
      bool success = pStrandGeometry->ComputeNumPermanentStrands( num_total, pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, &numStraight, &numHarped);
      if (!success)
      {
         CString str;
         str.Format(_T("Current number of Permanent strands %d, is not compatible with the Girder Library. Number of strands will be set to zero"),num_total);
         ::AfxMessageBox(str, MB_OK | MB_ICONINFORMATION );

         num_total=0;
         numStraight=0;
         numHarped=0;
      }
      else if (num_straight!=numStraight || num_harped!=numHarped)
      {
         CString str;
         str.Format(_T("Current values of %d Straight and %d Harped are not compatible with the Girder Library. For %d Total strands you will have %d Straight and %d Harped"), num_straight, num_harped, num_total, numStraight, numHarped);
         ::AfxMessageBox(str, MB_OK | MB_ICONINFORMATION );
      }

      StrandIndexType nStrandsMax = pStrandGeometry->GetMaxNumPermanentStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx);

      CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
      UDACCEL uda;
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, short(nStrandsMax) );
      pSpin->SetPos(num_total);

      // convert pjack
      CButton* pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
      ASSERT( pbut );
      bool calc_harped   = pbut->GetCheck() != 0;

      pbut = (CButton*) GetDlgItem( IDC_SS_JACK );
      ASSERT( pbut );
      bool calc_straight = pbut->GetCheck() != 0;

      Float64 jack_straight = 0;
      if (calc_straight)
      {
         jack_straight = GetMaxPjack(numStraight,pgsTypes::Straight);
      }
      else
      {
         CWnd* pWnd = GetDlgItem( IDC_SS_JACK_FORCE );
         ASSERT( pWnd );
         CString val_as_text;
         pWnd->GetWindowText( val_as_text );
         Float64 Pjack = _tstof( val_as_text );
         jack_straight = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
         pParent->m_GirderData.LastUserPjack[pgsTypes::Straight] = Pjack;
      }

      Float64 jack_harped = 0;
      if (calc_harped)
      {
         jack_harped = GetMaxPjack(numHarped,pgsTypes::Harped);
      }
      else
      {
         CWnd* pWnd = GetDlgItem( IDC_HS_JACK_FORCE );
         ASSERT( pWnd );
         CString val_as_text;
         pWnd->GetWindowText( val_as_text );
         Float64 Pjack = _tstof( val_as_text );
         jack_harped = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
         pParent->m_GirderData.LastUserPjack[pgsTypes::Harped] = jack_harped;
      }

      pParent->m_GirderData.bPjackCalculated[pgsTypes::Permanent] = calc_straight&&calc_harped;

      if (!pParent->m_GirderData.bPjackCalculated[pgsTypes::Permanent])
      {
         pParent->m_GirderData.LastUserPjack[pgsTypes::Permanent] = jack_straight + jack_harped;
      }

      pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
      ASSERT( pbut );
      pbut->SetCheck( calc_straight&&calc_harped ? TRUE :FALSE);

      HideControls(1); 
      UpdateStraightHarped(numStraight, numHarped);
      UpdateHarpedOffsets(numHarped);
   }
   else if  (cursel==NPS_STRAIGHT_HARPED)
   {
      StrandIndexType total_strands = GetDlgItemInt( IDC_NUM_HS );

      StrandIndexType num_straight, num_harped;
      pStrandGeometry->ComputeNumPermanentStrands( total_strands, pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, &num_straight, &num_harped);

      long nStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Straight);

      CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_SS_SPIN );
      UDACCEL uda;
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, (short)nStrandsMax );
      pSpin->SetPos(num_straight);

      nStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Harped);

      pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, (short)nStrandsMax );
      pSpin->SetPos(num_harped);

      // convert pjack
      CButton* pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
      ASSERT( pbut );
      bool calc_permanent   = pbut->GetCheck() != 0;

      pbut = (CButton*) GetDlgItem( IDC_SS_JACK );
      ASSERT( pbut );
      pbut->SetCheck( calc_permanent ? 1 : 0);

      pParent->m_GirderData.bPjackCalculated[pgsTypes::Harped]   = calc_permanent;
      pParent->m_GirderData.bPjackCalculated[pgsTypes::Straight] = calc_permanent;

      if (!calc_permanent)
      {
         CWnd* pWnd = GetDlgItem( IDC_HS_JACK_FORCE );
         ASSERT( pWnd );
         CString val_as_text;
         pWnd->GetWindowText( val_as_text );
         Float64 Pjack = _tstof( val_as_text );
         Pjack = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );
         pParent->m_GirderData.LastUserPjack[pgsTypes::Permanent] = Pjack;

         if (total_strands>0)
         {
            pParent->m_GirderData.LastUserPjack[pgsTypes::Harped]   = Pjack * (Float64)num_harped/(Float64)total_strands;
            pParent->m_GirderData.LastUserPjack[pgsTypes::Straight] = Pjack * (Float64)num_straight/(Float64)total_strands;
         }
         else
         {
            pParent->m_GirderData.LastUserPjack[pgsTypes::Harped]   = 0.0;
            pParent->m_GirderData.LastUserPjack[pgsTypes::Straight] = 0.0;
         }
      }

      HideControls(0); 
      HideControls(1); 
   }
   else
      ATLASSERT(0);

   InitPjackEdits();

   ShowHideNumStrandControls(cursel);
}

void CGirderDescPrestressPage::OnDropdownHpComboHp() 
{
   CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP);
   int cursel = box->GetCurSel();
   m_OldHpMeasureType = (HarpedStrandOffsetType)box->GetItemData(cursel);
}

void CGirderDescPrestressPage::OnDropdownHpComboEnd() 
{
   CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_END);
   int cursel = box->GetCurSel();
   m_OldEndMeasureType = (HarpedStrandOffsetType)box->GetItemData(cursel);
}

void CGirderDescPrestressPage::UpdateStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   matPsStrand::Size cur_size = matPsStrand::D1270;
   if ( cur_sel != CB_ERR )
   {
      Int32 cur_key = pList->GetItemData( cur_sel );
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

void CGirderDescPrestressPage::OnStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CDataExchange DX(this,true);
	DDX_CBIndex(&DX, IDC_STRAND_SIZE, m_StrandSizeIdx);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   Int32 key = pList->GetItemData( m_StrandSizeIdx );

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Straight] = pPool->GetStrand( key );
   pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Harped]   = pPool->GetStrand( key );

   // Now we can update pjack values in dialog
   InitPjackEdits();
}

void CGirderDescPrestressPage::OnTempStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CDataExchange DX(this,true);
	DDX_CBIndex(&DX, IDC_TEMP_STRAND_SIZE, m_TempStrandSizeIdx);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
   Int32 key = pList->GetItemData( m_TempStrandSizeIdx );

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_GirderData.Material.pStrandMaterial[pgsTypes::Temporary] = pPool->GetStrand( key );

   // Now we can update pjack values in dialog
   InitPjackEdits();
}

BOOL CGirderDescPrestressPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_HP_COMBO_END:
      case IDC_HP_COMBO_HP:
         m_strTip = _T("Hold the CTRL key when making your selection to convert the harped strand location");
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.LockBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}

void CGirderDescPrestressPage::InitPjackEdits()
{
   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int cursel = box->GetCurSel();

   InitPjackEditEx( IDC_HS_JACK );

   if (cursel!=NPS_TOTAL_NUMBER)
      InitPjackEditEx( IDC_SS_JACK );

   InitPjackEditEx( IDC_TEMP_JACK );
}