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

// BridgeDescPrestressPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "Resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescPrestressPage.h"
#include "GirderDescDlg.h"
#include "DebondDlg.h"
#include "GirderSelectStrandsDlg.h"

#include "PGSuperAppPlugin\GirderSegmentStrandsPage.h" // replace with real dialog header when we have it

#include <PgsExt\DesignConfigUtil.h>

#include <Material\PsStrand.h>
#include <LRFD\StrandPool.h>

#include <MfcTools\CustomDDX.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>

#include <PsgLib\GirderLibraryEntry.h>

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma Reminder("UPDATE: problems with this dialog")
// 1) Reverse how the Pjack check box works so that it is consistent with the DirectSelect and DirectInput input methods

/////////////////////////////////////////////////////////////////////////////
// CGirderDescPrestressPage property page

IMPLEMENT_DYNCREATE(CGirderDescPrestressPage, CPropertyPage)

CGirderDescPrestressPage::CGirderDescPrestressPage() : 
CPropertyPage(CGirderDescPrestressPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescPrestressPage)
	m_StrandSizeIdx = INVALID_INDEX;
	m_TempStrandSizeIdx = INVALID_INDEX;
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

   if (!pDX->m_bSaveAndValidate)
   {
      m_CurrStrandDefinitionType = pParent->m_pSegment->Strands.GetStrandDefinitionType();
   }

   DDX_CBItemData(pDX, IDC_STRAND_INPUT_TYPE, m_CurrStrandDefinitionType);

   // First deal with various ways to describe number of strands
   StrandIndexType ns(0), nh(0), nt(0);
   if (m_CurrStrandDefinitionType == CStrandData::sdtTotal)
   {
      StrandIndexType np;
      if (!pDX->m_bSaveAndValidate)
      {
         np = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Permanent);
      }

      DDX_Text(pDX, IDC_NUM_HS, np);

      bool bPjackCalc = pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Permanent);
      DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackCalc);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Permanent,bPjackCalc);
      }

      if (!pDX->m_bSaveAndValidate)
      {
         Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Permanent);
         DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
      }
      else
      {
         // value is dialog is chopped. recompute to get full precision
         if (pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Permanent))
         {
            pParent->m_pSegment->Strands.SetPjack(pgsTypes::Permanent,GetMaxPjack(np, pgsTypes::Permanent));
         }
         else
         {
            ATLASSERT(pDX->m_bSaveAndValidate);
            Float64 Pjack;
            DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack,   pDisplayUnits->GetGeneralForceUnit() );
            pParent->m_pSegment->Strands.SetPjack(pgsTypes::Permanent,Pjack);
         }
         Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Permanent);
         DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, Pjack, GetUltPjack(np, pgsTypes::Permanent ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
      }

      // compute number of straight and harped based on num permanent for possible later use below
      pStrandGeometry->ComputeNumPermanentStrands( np, pParent->m_strGirderName.c_str(), &ns, &nh);

      if (pDX->m_bSaveAndValidate)
      {
         pParent->m_pSegment->Strands.SetTotalPermanentNstrands(np, ns, nh);
      }

      UpdateStraightHarped(ns, nh);
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtStraightHarped)
   {
      if (!pDX->m_bSaveAndValidate)
      {
         nh = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
      }

      DDX_Text(pDX, IDC_NUM_HS, nh);

      if (!pDX->m_bSaveAndValidate)
      {
         ns = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
      }

	   DDX_Text(pDX, IDC_NUM_SS, ns);

      if (pDX->m_bSaveAndValidate)
      {
         pParent->m_pSegment->Strands.SetHarpedStraightNstrands(ns, nh);
      }
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectInput)
   {
      // Data is kept up to date in pParent->m_GirderData.PrestressData
      nh = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
      ns = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
      nt = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary);
   }
   else
   {
      ATLASSERT(false);
   }

   // Jacking force controls for harped/straight are same for CStrandData::sdtStraightHarped and CStrandData::sdtDirectSelection
   if (m_CurrStrandDefinitionType==CStrandData::sdtStraightHarped || m_CurrStrandDefinitionType==CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType==CStrandData::sdtDirectInput)
   {
      // Harped
      bool bPjackCalc = pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped);
      DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackCalc);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped,bPjackCalc);
      }

      if (pDX->m_bSaveAndValidate && pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped))
      {
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Harped, GetMaxPjack(nh, pgsTypes::Harped));
      }
      else
      {
         Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Harped);
         DDX_UnitValueAndTag( pDX, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack,   pDisplayUnits->GetGeneralForceUnit() );
         if ( pDX->m_bSaveAndValidate )
         {
            pParent->m_pSegment->Strands.SetPjack(pgsTypes::Harped,Pjack);
         }
      }

      Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Harped);
      DDV_UnitValueLimitOrLess( pDX, IDC_HS_JACK_FORCE, Pjack, 
                                GetUltPjack(nh, pgsTypes::Harped), pDisplayUnits->GetGeneralForceUnit(), 
                                _T("PJack must be less than the ultimate value of %f %s"));

      // Straight
      bPjackCalc = pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight);
	   DDX_Check_Bool(pDX, IDC_SS_JACK, bPjackCalc );
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight,bPjackCalc);
      }

      if (pDX->m_bSaveAndValidate && pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight) )
      {
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Straight,GetMaxPjack(ns, pgsTypes::Straight));
      }
      else
      {
         Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Straight);
         DDX_UnitValueAndTag( pDX, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
         if ( pDX->m_bSaveAndValidate )
         {
            pParent->m_pSegment->Strands.SetPjack(pgsTypes::Straight,Pjack);
         }
      }

      Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Straight);
      DDV_UnitValueLimitOrLess( pDX, IDC_SS_JACK_FORCE, Pjack, GetUltPjack(ns, pgsTypes::Straight ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );
   }

   // Temporary
   if (m_CurrStrandDefinitionType != CStrandData::sdtDirectSelection && m_CurrStrandDefinitionType != CStrandData::sdtDirectInput)
   {
      if (!pDX->m_bSaveAndValidate)
      {
         nt = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary);
      }

	   DDX_Text(pDX, IDC_NUM_TEMP, nt);

      if (pDX->m_bSaveAndValidate)
      {
         pParent->m_pSegment->Strands.SetTemporaryNstrands(nt);
      }
   }

   if (!pDX->m_bSaveAndValidate)
   {
      ShowHideNumStrandControls(m_CurrStrandDefinitionType);
   }

   bool bPjackCalc = pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary);
	DDX_Check_Bool(pDX, IDC_TEMP_JACK, bPjackCalc);
   if ( pDX->m_bSaveAndValidate )
   {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary,bPjackCalc);
   }

   pgsTypes::TTSUsage ttsUsage = pParent->m_pSegment->Strands.GetTemporaryStrandUsage();
   DDX_CBItemData(pDX, IDC_TTS_USE, ttsUsage );
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_pSegment->Strands.SetTemporaryStrandUsage(ttsUsage);
   }

   if (pDX->m_bSaveAndValidate && pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary))
   {
      pParent->m_pSegment->Strands.SetPjack(pgsTypes::Temporary,GetMaxPjack(nt, pgsTypes::Temporary));
   }
   else
   {
      Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Temporary);
      DDX_UnitValueAndTag( pDX, IDC_TEMP_JACK_FORCE, IDC_TEMP_JACK_FORCE_UNIT, Pjack,  pDisplayUnits->GetGeneralForceUnit() );
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Temporary,Pjack);
      }
   }

   Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Temporary);
   DDV_UnitValueLimitOrLess( pDX, IDC_TEMP_JACK_FORCE, Pjack, GetUltPjack(nt, pgsTypes::Temporary ), pDisplayUnits->GetGeneralForceUnit(), _T("PJack must be less than the ultimate value of %f %s") );

   // Set up pjack controls - values that are auto-computed will be refreshed
   UpdateStrandControls();

   // adjustment of harped strands at ends
   m_AllowEndAdjustment = 0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(pParent->m_strGirderName.c_str());

   if (m_AllowEndAdjustment)
   {
      if (!pDX->m_bSaveAndValidate)
      {
         // must convert data from legacy data files
         if (pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd()==hsoLEGACY)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_strGirderName.c_str(), 
                                                                                   m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                   harpFill,
                                                                                   pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                   pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd());

            Float64 topcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(pParent->m_strGirderName.c_str(), 
                                                                                       m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                       harpFill,
                                                                                       hsoCGFROMTOP, 
                                                                                       absol_offset);

            pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtEnd(hsoCGFROMTOP);
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtEnd(topcg_offset);
         }

         UpdateEndRangeLength(pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), nh);
      }

      Float64 offset = pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd();
      HarpedStrandOffsetType offsetType = pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd();
      DDX_UnitValueAndTag( pDX, IDC_HPOFFSET_END, IDC_HPOFFSET_END_UNIT, offset, pDisplayUnits->GetComponentDimUnit() );
   	DDX_CBItemData(pDX, IDC_HP_COMBO_END, offsetType);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.SetHarpStrandOffsetAtEnd(offset);
         pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtEnd(offsetType);
      }

      if ( nh <=0)
      {
         HideEndOffsetControls(TRUE);
      }
   }
   else
   {
      HideEndOffsetControls(TRUE);
   }

   m_AllowHpAdjustment = 0.0 <= pStrandGeometry->GetHarpedHpOffsetIncrement(pParent->m_strGirderName.c_str());

   if (m_AllowHpAdjustment)
   {
      // must convert data from legacy data files
      if (!pDX->m_bSaveAndValidate)
      {
         if(pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint()==hsoLEGACY)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_strGirderName.c_str(),
                                                                                  m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                  harpFill,
                                                                                  pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                                  pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint());

            Float64 botcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(pParent->m_strGirderName.c_str(), 
                                                                                      m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                      harpFill, 
                                                                                      hsoCGFROMBOTTOM, absol_offset);

            pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint( hsoCGFROMBOTTOM );
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint( botcg_offset );
         }

         UpdateHpRangeLength(pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), nh);
      }

      Float64 offset = pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint();
      HarpedStrandOffsetType offsetType = pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint();
      DDX_UnitValueAndTag( pDX, IDC_HPOFFSET_HP, IDC_HPOFFSET_HP_UNIT, offset, pDisplayUnits->GetComponentDimUnit() );
   	DDX_CBItemData(pDX, IDC_HP_COMBO_HP, offsetType);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint(offset);
         pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint(offsetType);
      }

      if ( nh <= 0)
      {
         HideHpOffsetControls(TRUE);
      }
   }
   else
   {
      HideHpOffsetControls(TRUE);
   }

   if (pDX->m_bSaveAndValidate)
   {
      // determine if offset strands are within girder bounds
      if (0 < nh)
      {

         // But first, for straight-web strands, make adjustment at hp the same as at ends
         if( m_bAreHarpedStrandsForcedStraight && m_AllowEndAdjustment)
         {
            ATLASSERT(m_AllowHpAdjustment); // should always be true because we must be able to adjust both locations
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint( pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd() );
            pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint( pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() );
         }

         // girder ends
         // first convert to abosolute offsets
         Float64 absol_offset;
         if (m_AllowEndAdjustment)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_strGirderName.c_str(), 
                                                                                   m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                   harpFill,
                                                                                   pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                   pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd());

            Float64 max_end_offset, min_end_offset;
            pStrandGeometry->GetHarpedEndOffsetBoundsEx(pParent->m_strGirderName.c_str(), 
                                                        m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                        harpFill, &min_end_offset, &max_end_offset);

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
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_strGirderName.c_str(), 
                                                                          m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                          harpFill,
                                                                          pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                          pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint());

            Float64 max_hp_offset, min_hp_offset;
            pStrandGeometry->GetHarpedHpOffsetBoundsEx(pParent->m_strGirderName.c_str(), 
                                                       m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                       harpFill,
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

      // Get rid of any debonded strands that aren't filled
      ConfigStrandFillVector strtvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);
      ReconcileDebonding(strtvec, pParent->m_pSegment->Strands.GetDebonding(pgsTypes::Straight)); 

      for ( int i = 0; i < 2; i++ )
      {
         std::vector<GridIndexType> extStrands = pParent->m_pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i);
         bool bChanged = ReconcileExtendedStrands(strtvec, extStrands);

         if ( bChanged )
         {
            pParent->m_pSegment->Strands.SetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i,extStrands);
         }
      }
   }

	DDX_CBIndex(pDX, IDC_STRAND_SIZE, (int&)m_StrandSizeIdx);
	DDX_CBIndex(pDX, IDC_TEMP_STRAND_SIZE, (int&)m_TempStrandSizeIdx);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
      Int32 key = (Int32)pList->GetItemData( (int)m_StrandSizeIdx );
      pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight, pPool->GetStrand( key ));
      pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,   pPool->GetStrand( key ));

      pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
      key = (Int32)pList->GetItemData( (int)m_TempStrandSizeIdx );
      pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand( key ));
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
   ON_BN_CLICKED(IDC_EDIT_STRAND_FILL, &CGirderDescPrestressPage::OnBnClickedEditStrandFill)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescPrestressPage message handlers

BOOL CGirderDescPrestressPage::OnInitDialog() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   // Get key segment dimensions
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(pParent->m_SegmentKey);

   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest(pParent->m_SegmentKey,POI_RELEASED_SEGMENT | POI_0L);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiStart(vPoi.front());

   vPoi = pIPoi->GetPointsOfInterest(pParent->m_SegmentKey,POI_RELEASED_SEGMENT | POI_10L);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiEnd(vPoi.front());

   m_HgStart = pSectProp->GetHg(intervalIdx,poiStart);
   m_HgEnd   = pSectProp->GetHg(intervalIdx,poiEnd);

   vPoi = pIPoi->GetPointsOfInterest(pParent->m_SegmentKey,POI_HARPINGPOINT);
   if ( 0 < vPoi.size() )
   {
      ATLASSERT(vPoi.size() == 1 || vPoi.size() == 2);
      pgsPointOfInterest poiHp1(vPoi.front());
      pgsPointOfInterest poiHp2(vPoi.back());

      m_HgHp1 = pSectProp->GetHg(intervalIdx,poiHp1);
      m_HgHp2 = pSectProp->GetHg(intervalIdx,poiHp2);
   }
   else
   {
      m_HgHp1 = m_HgStart;
      m_HgHp2 = m_HgEnd;
   }


   // This value is used throughout
   m_bAreHarpedStrandsForcedStraight = pStrandGeom->GetAreHarpedStrandsForcedStraightEx(pParent->m_strGirderName.c_str());

   // Fill the strand size combo box.
   UpdateStrandList(IDC_STRAND_SIZE);
   UpdateStrandList(IDC_TEMP_STRAND_SIZE);

   // Select the strand size
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int32 target_key = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight));
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   int cStrands = pList->GetCount();
   for ( int i = 0; i < cStrands; i++ )
   {
      Int32 key = (Int32)pList->GetItemData( i );
      if ( key == target_key )
      {
         m_StrandSizeIdx = i;
         break;
      }
   }

   target_key = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary));
   pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
   cStrands = pList->GetCount();
   for ( int i = 0; i < cStrands; i++ )
   {
      Int32 key = (Int32)pList->GetItemData( i );
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

   pCB = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   idx = pCB->AddString(_T("Total Number of Permanent Strands"));
   pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtTotal);
   if(m_bAreHarpedStrandsForcedStraight)
   {
      idx = pCB->AddString(_T("Number of Straight and Number of Straight-Web"));
      pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtStraightHarped);

      GetDlgItem(IDC_VERT_GROUP)->SetWindowText(_T("Vertical Location of Straight-Web Strands"));
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Along Girder"));

      DisappearHpOffsetControls();
   }
   else
   {
      idx = pCB->AddString(_T("Number of Straight and Number of Harped"));
      pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtStraightHarped);

      GetDlgItem(IDC_VERT_GROUP)->SetWindowText(_T("Vertical Location of Harped Strands"));
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Girder Ends"));
   }

   idx = pCB->AddString(_T("Direct Selection of Strand Locations"));
   pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtDirectSelection);

   idx = pCB->AddString(_T("Direct Input of Strand Locations"));
   pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtDirectInput);

   CPropertyPage::OnInitDialog();

   OnDropdownHpComboHp();
   OnDropdownHpComboEnd();

   EnableToolTips(TRUE);

   if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectInput )
   {
      DisappearEndOffsetControls();
      DisappearHpOffsetControls();
      GetDlgItem(IDC_HP_NOTE)->ShowWindow(SW_HIDE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescPrestressPage::InitHarpStrandOffsetMeasureComboBox(CComboBox* pCB)
{
   pCB->Clear();
   int idx;

   if(!m_bAreHarpedStrandsForcedStraight)
   {
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
   else
   {
      idx = pCB->AddString(_T("Distance between CG of Straight-Web Group and Girder Top"));
      pCB->SetItemData(idx,hsoCGFROMTOP);

      idx = pCB->AddString(_T("Distance between CG of Straight-Web Group and Girder Bottom"));
      pCB->SetItemData(idx,hsoCGFROMBOTTOM);

      idx = pCB->AddString(_T("Distance between Top-Most Straight-Web Strand and Girder Top"));
      pCB->SetItemData(idx,hsoTOP2TOP);

      idx = pCB->AddString(_T("Distance between Top-Most Straight-Web Strand and Girder Bottom"));
      pCB->SetItemData(idx,hsoTOP2BOTTOM);

      idx = pCB->AddString(_T("Distance between Bottom-Most Straight-Web Strand and Girder Bottom"));
      pCB->SetItemData(idx,hsoBOTTOM2BOTTOM);

      idx = pCB->AddString(_T("Eccentricity of Straight-Web Strand Group (Non-Composite Section)"));
      pCB->SetItemData(idx,hsoECCENTRICITY);
   }
}

StrandIndexType CGirderDescPrestressPage::PermStrandSpinnerInc(IStrandGeometry* pStrands, StrandIndexType currNum, bool bAdd )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nextnum;
   if ( bAdd )
   {
      nextnum = pStrands->GetNextNumPermanentStrands(pParent->m_strGirderName.c_str(), currNum);

      if (nextnum == INVALID_INDEX)
      {
         nextnum=currNum; // no increment if we hit the top
      }
   }
   else
   {
      nextnum = pStrands->GetPreviousNumPermanentStrands(pParent->m_strGirderName.c_str(), currNum);

      if (nextnum == INVALID_INDEX)
      {
         nextnum=currNum; // no increment if we hit the bottom
      }
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
      nextnum = pStrands->GetNextNumStrands(pParent->m_strGirderName.c_str(), type, currNum);

      if (nextnum == INVALID_INDEX)
      {
         nextnum=currNum; // no increment if we hit the top
      }
   }
   else
   {
      nextnum = pStrands->GetPrevNumStrands(pParent->m_strGirderName.c_str(), type, currNum);

      if (nextnum == INVALID_INDEX)
      {
         nextnum=currNum; // no increment if we hit the bottom
      }
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

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType inc = StrandSpinnerInc( pStrandGeom, pgsTypes::Straight, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );

   pNMUpDown->iDelta = (int)inc;

   StrandIndexType nStrands = StrandIndexType(pNMUpDown->iPos + pNMUpDown->iDelta);

   UpdatePjackEditEx(nStrands, IDC_SS_JACK );

   *pResult = 0;
}

void CGirderDescPrestressPage::OnNumHarpedStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   if (m_CurrStrandDefinitionType==CStrandData::sdtTotal)
   {
      pNMUpDown->iDelta  = (int)PermStrandSpinnerInc( pStrandGeom, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );
   }
   else if (m_CurrStrandDefinitionType==CStrandData::sdtStraightHarped)
   {
      pNMUpDown->iDelta  = (int)StrandSpinnerInc( pStrandGeom, pgsTypes::Harped, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );
   }
   else
   {
      ATLASSERT(false); // shouldn't be here for direct fill
   }


   StrandIndexType nStrands = StrandIndexType(pNMUpDown->iPos + pNMUpDown->iDelta);

   UpdatePjackEditEx(nStrands, IDC_HS_JACK );

   StrandIndexType numHarped;
   if (m_CurrStrandDefinitionType==CStrandData::sdtTotal)
   {
      StrandIndexType numStraight;
      bool was_valid = pStrandGeom->ComputeNumPermanentStrands(nStrands,pParent->m_strGirderName.c_str(), &numStraight, &numHarped);
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

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   pNMUpDown->iDelta  = (int)StrandSpinnerInc( pStrandGeom, pgsTypes::Temporary, pNMUpDown->iPos, pNMUpDown->iDelta > 0 );

   StrandIndexType nStrands = pNMUpDown->iPos + pNMUpDown->iDelta;

   UpdatePjackEditEx(nStrands, IDC_TEMP_JACK );

	*pResult = 0;
}

void CGirderDescPrestressPage::UpdatePjackEdits()
{
   UpdatePjackEdit( IDC_HS_JACK );

   if (m_CurrStrandDefinitionType != CStrandData::sdtTotal)
   {
      UpdatePjackEdit( IDC_SS_JACK );
   }

   UpdatePjackEdit( IDC_TEMP_JACK );
}

void CGirderDescPrestressPage::UpdatePjackEdit( UINT nCheckBox  )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nStrands;
   bool bIsDirect = (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectInput);
   switch( nCheckBox )
   {
   case IDC_HS_JACK:
      nStrands = bIsDirect ? pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped) : GetDlgItemInt( IDC_NUM_HS );
      break;

   case IDC_SS_JACK:
      nStrands = bIsDirect ? pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight) : GetDlgItemInt( IDC_NUM_SS );
      break;

   case IDC_TEMP_JACK:
      nStrands = bIsDirect ? pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary) : GetDlgItemInt( IDC_NUM_TEMP );
      break;
   }

   UpdatePjackEditEx(nStrands, nCheckBox);
}

void CGirderDescPrestressPage::UpdatePjackEditEx(StrandIndexType nStrands, UINT nCheckBox  )
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   UINT nEdit, nUnit;
   switch( nCheckBox )
   {
   case IDC_HS_JACK:
      nEdit = IDC_HS_JACK_FORCE;
      nUnit = IDC_HS_JACK_FORCE_UNIT;
      break;

   case IDC_SS_JACK:
      nEdit = IDC_SS_JACK_FORCE;
      nUnit = IDC_SS_JACK_FORCE_UNIT;
      break;

   case IDC_TEMP_JACK:
      nEdit = IDC_TEMP_JACK_FORCE;
      nUnit = IDC_TEMP_JACK_FORCE_UNIT;
      break;
   }

   BOOL bEnableUserInput;
   CWnd* pCheck = GetDlgItem( nCheckBox );
   if (  nStrands == 0 )
   {
      pCheck->EnableWindow( FALSE );
      bEnableUserInput = FALSE; // don't enable controls if the number of strands is zero
   }
   else
   {
      pCheck->EnableWindow( TRUE );
      bEnableUserInput= IsDlgButtonChecked( nCheckBox )!=BST_CHECKED ? TRUE : FALSE;
   }

   CWnd* pWnd = GetDlgItem( nEdit );
   pWnd->EnableWindow( bEnableUserInput );
   CWnd* pUnitWnd = GetDlgItem( nUnit );
   pUnitWnd->EnableWindow( bEnableUserInput );

   Float64 Pjack = 0;
   if ( !bEnableUserInput && nStrands != 0 )
   {
      // Compute pjack and fill in value
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
            if (m_CurrStrandDefinitionType == CStrandData::sdtTotal)
            {
               Pjack = GetMaxPjack(nStrands,pgsTypes::Permanent);
            }
            else
            {
               Pjack = GetMaxPjack(nStrands,pgsTypes::Harped);
            }
         }
         break;

      case IDC_SS_JACK:
         Pjack = GetMaxPjack( nStrands, pgsTypes::Straight );
         break;

      case IDC_TEMP_JACK:
         Pjack =GetMaxPjack(nStrands,pgsTypes::Temporary);
         break;
      }

      CDataExchange dx(this,FALSE);
      DDX_UnitValueAndTag( &dx, nEdit, nUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }
   else if (nStrands == 0)
   {
      // zero out pjack
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 jack=0.0;
      CDataExchange dx(this,FALSE);
      DDX_UnitValueAndTag( &dx, nEdit, nUnit, jack, pDisplayUnits->GetGeneralForceUnit() );
   }
}

Float64 CGirderDescPrestressPage::GetMaxPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, IPretensionForce, pPSForce );

   if ( strandType == pgsTypes::Permanent )
   {
      strandType = pgsTypes::Straight;
   }


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
      PjackMax = pPSForce->GetPjackMax(pParent->m_SegmentKey,*pParent->m_pSegment->Strands.GetStrandMaterial(strandType), nStrands);
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
   {
      strandType = pgsTypes::Straight;
   }

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   const matPsStrand* pStrand = pParent->m_pSegment->Strands.GetStrandMaterial(strandType);

   // Ultimate strength of strand group
   Float64 ult  = pStrand->GetUltimateStrength();
   Float64 area = pStrand->GetNominalArea();

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

   CSpinButtonCtrl* pSpin;
   UDACCEL uda;
   StrandIndexType nStrandsMax;

   // harped/straight or permanent strands
   if (m_CurrStrandDefinitionType==CStrandData::sdtTotal)
   {
      nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Permanent);

      pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
      uda;
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, short(nStrandsMax) );
   }
   else if  (m_CurrStrandDefinitionType==CStrandData::sdtStraightHarped)
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
   HideControls(0,m_CurrStrandDefinitionType);  // Straight Strands
   HideControls(1,m_CurrStrandDefinitionType);  // Harped Strands
   HideControls(2,m_CurrStrandDefinitionType);  // Temporary Strands

   UpdatePjackEdits();
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
   {
      hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
   }

	return hbr;
}

void CGirderDescPrestressPage::HideControls(int key, CStrandData::StrandDefinitionType numPermStrandsType)
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
   StrandIndexType nStrandPoints; // Number of strand points

   switch( key )
   {
   case 0: // Straight strands
      nTitle         = IDC_SS_TITLE;
      nNumStrand     = IDC_NUM_SS;
      nNumStrandSpin = IDC_NUM_SS_SPIN;
      nPjackCheck    = IDC_SS_JACK;
      nPjackEdit     = IDC_SS_JACK_FORCE;
      nPjackUnit     = IDC_SS_JACK_FORCE_UNIT;

      nStrandPoints = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight);
      break;

   case 1: // Harped/Permanent Strands
      {
         nTitle         = IDC_HS_TITLE;
         nNumStrand     = IDC_NUM_HS;
         nNumStrandSpin = IDC_NUM_HS_SPIN;
         nPjackCheck    = IDC_HS_JACK;
         nPjackEdit     = IDC_HS_JACK_FORCE;
         nPjackUnit     = IDC_HS_JACK_FORCE_UNIT;

         if (numPermStrandsType==CStrandData::sdtTotal)
         {
            nStrandPoints = pStrandGeom->GetMaxNumPermanentStrands(pParent->m_strGirderName.c_str());
         }
         else
         {
            nStrandPoints = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Harped);
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

      nStrandPoints = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Temporary);
      break;

   default:
      // Should never get here... Bad key
      ATLASSERT(false);
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

void CGirderDescPrestressPage::DisappearHpOffsetControls(int show)
{
   CWnd* pWnd;

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_HP_TITLE );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_HP );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP_UNIT );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );
}

void CGirderDescPrestressPage::DisappearEndOffsetControls(int show)
{
   CWnd* pWnd;

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_END_TITLE );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_END );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_END );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_END_UNIT );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_END_NOTE );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );
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

      if (m_CurrStrandDefinitionType == CStrandData::sdtTotal )
      {
         StrandIndexType total_strands;
         CDataExchange DX(this,FALSE);
	      DDX_Text(&DX, IDC_NUM_HS, total_strands);
         
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

         StrandIndexType numStraight, numHarped;
         pStrandGeometry->ComputeNumPermanentStrands( total_strands, pParent->m_strGirderName.c_str(), &numStraight, &numHarped);

         nStraightStrands = numStraight;
      }
      else if (m_CurrStrandDefinitionType == CStrandData::sdtStraightHarped )
      {
         CDataExchange DX(this,FALSE);
	      DDX_Text(&DX, IDC_NUM_SS, nStraightStrands);
      }
      else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection )
      {
         // data is stored when direct input dialog is closed
        nStraightStrands = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
      }
      else
      {
         ATLASSERT(false);
      }
   }
   else
   {
      ATLASSERT(false);
   }

   return nStraightStrands;
}

StrandIndexType CGirderDescPrestressPage::GetHarpedStrandCount()
{

   StrandIndexType nHarpedStrands;
   if (::IsWindow(m_hWnd))
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      if (m_CurrStrandDefinitionType == CStrandData::sdtTotal)
      {
         StrandIndexType total_strands;
         CDataExchange DX(this,TRUE);
	      DDX_Text(&DX, IDC_NUM_HS, total_strands);

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

         StrandIndexType numStraight, numHarped;
         pStrandGeometry->ComputeNumPermanentStrands( total_strands, pParent->m_strGirderName.c_str(), &numStraight, &numHarped);

         nHarpedStrands = numHarped;
      }
      else if (m_CurrStrandDefinitionType == CStrandData::sdtStraightHarped)
      {
         CDataExchange DX(this,TRUE);
	      DDX_Text(&DX, IDC_NUM_HS, nHarpedStrands);
      }
      else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectInput)
      {
         // data is stored when direct input dialog is closed
         nHarpedStrands = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
      }
      else
      {
         ATLASSERT(false);
      }
   }
   else
   {
      ATLASSERT(false);
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

      // use passed nh value if continuous fill (control might not have data yet)
      ConfigStrandFillVector harpFill = m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection ? 
                                        ComputeHarpedStrandFillVector() :
                                        pStrandGeom->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Harped, Nh);

      Float64 lowRange, highRange;
      pStrandGeom->ComputeValidHarpedOffsetForMeasurementTypeEnd(pParent->m_strGirderName.c_str(),
                                                                 m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                 harpFill, measureType, &lowRange, &highRange);

      lowRange  = ::ConvertFromSysUnits(lowRange, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      highRange = ::ConvertFromSysUnits(highRange,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      Float64 low  = Min(lowRange, highRange);
      Float64 high = Max(lowRange, highRange);

      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         str.Format(_T("(Valid Range %.1f to %.1f)"), low, high);
      }
      else
      {
         str.Format(_T("(Valid Range %.3f to %.3f)"), low, high);
      }
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

      // use passed nh value if continuous fill (control might not have data yet)
      ConfigStrandFillVector harpFill = m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection ? 
                                        ComputeHarpedStrandFillVector() :
                                        pStrandGeom->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Harped, Nh);

      Float64 lowRange, highRange;
      pStrandGeom->ComputeValidHarpedOffsetForMeasurementTypeHp(pParent->m_strGirderName.c_str(), m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, harpFill, measureType, &lowRange, &highRange);


      lowRange = ::ConvertFromSysUnits(lowRange,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      highRange = ::ConvertFromSysUnits(highRange,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      Float64 low  = Min(lowRange, highRange);
      Float64 high = Max(lowRange, highRange);

      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         str.Format(_T("(Valid Range %.1f to %.1f)"), low, high);
      }
      else
      {
         str.Format(_T("(Valid Range %.3f to %.3f)"), low, high);
      }
   }

   CEdit* pEdit = (CEdit*)GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   pEdit->SetWindowText(str);
}

void CGirderDescPrestressPage::UpdateStraightHarped(StrandIndexType Ns, StrandIndexType Nh)
{
   CString val_as_text;
   if (m_bAreHarpedStrandsForcedStraight)
   {
      val_as_text.Format(_T("Number of Straight: %d, Straight-Web: %d"),Ns, Nh);
   }
   else
   {
      val_as_text.Format(_T("Number of Straight: %d, Harped: %d"),Ns, Nh);
   }

   CWnd* pWnd = GetDlgItem( IDC_HARP_STRAIGHT );
   pWnd->SetWindowText( val_as_text );
}


void CGirderDescPrestressPage::OnSelchangeHpComboHp() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

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
      Float64 offset = _tstof(strOffset);

      offset = ::ConvertToSysUnits(offset,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

      offset = pStrandGeom->ConvertHarpedOffsetHp(pParent->m_strGirderName.c_str(), 
                                                  m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                  harpFill,  m_OldHpMeasureType, offset, measureType);
      
      strOffset = ::FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false);
      pWnd->SetWindowText(strOffset);
   }

   StrandIndexType Nh = GetHarpedStrandCount();

   UpdateHpRangeLength(measureType, Nh);
}

void CGirderDescPrestressPage::OnSelchangeHpComboEnd() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

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
      Float64 offset = _tstof(strOffset);
      offset = ::ConvertToSysUnits(offset,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

      offset = pStrandGeom->ConvertHarpedOffsetEnd(pParent->m_strGirderName.c_str(), 
                                                   m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                   harpFill,  m_OldEndMeasureType, offset, measureType);
      
      strOffset = FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false);
      pWnd->SetWindowText(strOffset);
   }

   StrandIndexType Nh = GetHarpedStrandCount();

   UpdateEndRangeLength(measureType, Nh);
}

void CGirderDescPrestressPage::ShowHideNumStrandControls(CStrandData::StrandDefinitionType strandDefinitionType)
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   // Temporary strands
   StrandIndexType Nt = pStrandGeometry->GetMaxStrands(pParent->m_strGirderName.c_str(),pgsTypes::Temporary);
   int show = Nt==0 ? SW_HIDE : SW_SHOW;

   int tstopids[] = {IDC_TEMP_TITLE,IDC_NUM_TEMP,IDC_NUM_TEMP_SPIN,IDC_TEMP_JACK,IDC_TEMP_JACK_FORCE,IDC_TEMP_JACK_FORCE_UNIT,
                     IDC_TTS_USE,IDC_TEMP_STRAND_SIZE,IDC_TEMP_STRAND_SIZE_LABEL,-12345};
   for (int id=0; tstopids[id]!=-12345; id++)
   {
      CWnd* pWnd = GetDlgItem( tstopids[id] );
      ASSERT( pWnd );
      pWnd->ShowWindow( show );
   }


   if(strandDefinitionType == CStrandData::sdtDirectSelection || strandDefinitionType == CStrandData::sdtDirectInput)
   {
      // hide spinners if direct-select
      int topids[] = {IDC_NUM_HS_SPIN, IDC_NUM_HS, IDC_NUM_SS_SPIN, IDC_NUM_SS, IDC_NUM_TEMP_SPIN, IDC_NUM_TEMP, -12345};
      for (int id=0; topids[id]!=-12345; id++)
      {
         CWnd* pWnd = GetDlgItem( topids[id] );
         ASSERT( pWnd );
         pWnd->ShowWindow( SW_HIDE );
      }

      GetDlgItem(IDC_HARP_STRAIGHT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_STRAND_FILL)->ShowWindow(SW_SHOW);
      if ( strandDefinitionType == CStrandData::sdtDirectSelection )
      {
         GetDlgItem(IDC_EDIT_STRAND_FILL)->SetWindowText(_T("Select Strands..."));
      }
      else
      {
         GetDlgItem(IDC_EDIT_STRAND_FILL)->SetWindowText(_T("Define Strands..."));
      }

      // always show ss title and jacking info
      GetDlgItem(IDC_SS_TITLE)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_SS_JACK)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_SS_JACK_FORCE)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_SS_JACK_FORCE_UNIT)->ShowWindow(SW_SHOW);

      // labels by strand spinners
      CString msg;
      msg.Format(_T("Number of Straight Strands = %d"), (int)pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight) );
      CWnd* pWnd = GetDlgItem( IDC_SS_TITLE );
      pWnd->SetWindowText( msg);

      if (m_bAreHarpedStrandsForcedStraight)
      {
         msg.Format(_T("Number of Straight-Web Strands = %d"), (int)pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped));
      }
      else
      {
         msg.Format(_T("Number of Harped Strands = %d"), (int)pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped));
      }

      pWnd = GetDlgItem( IDC_HS_TITLE );
      pWnd->SetWindowText( msg );

      msg.Format(_T("Number of Temporary Strands = %d"), (int)pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary));
      pWnd = GetDlgItem( IDC_TEMP_TITLE );
      pWnd->SetWindowText( msg );
   }
   else
   {
      GetDlgItem(IDC_EDIT_STRAND_FILL)->ShowWindow(SW_HIDE);

      int hstopids[] = {IDC_NUM_HS_SPIN, IDC_NUM_HS, -12345};
      for (int id=0; hstopids[id]!=-12345; id++)
      {
         CWnd* pWnd = GetDlgItem( hstopids[id] );
         ASSERT( pWnd );
         pWnd->ShowWindow( SW_SHOW );
      }

      int show = strandDefinitionType == CStrandData::sdtTotal ? SW_HIDE : SW_SHOW;

      int topids[] = {IDC_SS_TITLE, IDC_NUM_SS, IDC_NUM_SS_SPIN, IDC_SS_JACK, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, -12345};
      for (int id=0; topids[id]!=-12345; id++)
      {
         CWnd* pWnd = GetDlgItem( topids[id] );
         ASSERT( pWnd );
         pWnd->ShowWindow( show );
      }

      // num harped/straight indicator
      int nsshow = strandDefinitionType == CStrandData::sdtTotal ? SW_SHOW : SW_HIDE;
      CWnd* pWnd = GetDlgItem( IDC_HARP_STRAIGHT );
      ASSERT( pWnd );
      pWnd->ShowWindow( nsshow );
      
      // labels for strand spinners
      pWnd = GetDlgItem( IDC_SS_TITLE );
      pWnd->SetWindowText( _T("Number of Straight Strands") );

      CString msg;
      if(strandDefinitionType == CStrandData::sdtTotal)
      {
         msg = _T("Total Number of Permanent Strands");
      }
      else
      {
         if (m_bAreHarpedStrandsForcedStraight)
         {
            msg = _T("Number of Straight-Web Strands");
         }
         else
         {
            msg = _T("Number of Harped Strands");
         }
      } 

      pWnd = GetDlgItem( IDC_HS_TITLE );
      pWnd->SetWindowText( msg );

      pWnd = GetDlgItem( IDC_TEMP_TITLE );
      pWnd->SetWindowText( _T("Number of Temporary Strands") );
   }
}

void CGirderDescPrestressPage::OnSelchangeStrandInputType() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int idx = box->GetCurSel();
   CStrandData::StrandDefinitionType newStrandDefinitionType = (CStrandData::StrandDefinitionType)box->GetItemData(idx);

   // If fill type didnt change, do nothing
   if(newStrandDefinitionType == m_CurrStrandDefinitionType)
   {
      return;
   }

   // First get number of current strands we are converting from
   StrandIndexType num_straight(0), num_harped(0), num_perm(0), num_temp(0);

   if  (m_CurrStrandDefinitionType == CStrandData::sdtStraightHarped)
   {
      // convert from num straight, num harped 
      num_straight = GetDlgItemInt( IDC_NUM_SS );
      num_harped   = GetDlgItemInt( IDC_NUM_HS );
      num_temp     = GetDlgItemInt( IDC_NUM_TEMP );
      num_perm     = num_straight + num_harped;
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtTotal )
   {
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
      num_perm = GetDlgItemInt( IDC_NUM_HS );
      pStrandGeometry->ComputeNumPermanentStrands( num_perm, pParent->m_strGirderName.c_str(), &num_straight, &num_harped);
      num_temp     = GetDlgItemInt( IDC_NUM_TEMP );
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectInput)
   {
      // convert from num straight, num harped 
      num_straight = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
      num_harped   = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
      num_temp     = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary );
      num_perm     = num_straight + num_harped;
   }
   else
   {
      ATLASSERT(false); // new fill type?
   }

   // show hidden controls
   DisappearEndOffsetControls(SW_SHOW);
   DisappearHpOffsetControls(SW_SHOW);
   GetDlgItem(IDC_HP_NOTE)->ShowWindow(SW_SHOW);

#pragma Reminder("IMPLEMENT: if converting from direct input to a library strand fill type, ask user if it is ok to throw away all strands")
   // direct input is not going to be, in general, compatible with a library.

   // Next: "To" new fill  type
   if (newStrandDefinitionType == CStrandData::sdtDirectSelection)
   {
      // Going from sequential strand fill to direct fill
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
      ConfigStrandFillVector straight_fill = pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Straight, num_straight);
      ConfigStrandFillVector harped_fill   = pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Harped, num_harped);
      ConfigStrandFillVector temp_fill     = pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Temporary, num_temp);

      // Deal with allow odd number of strands - direct fill does not support this
      if (pGdrEntry->OddNumberOfHarpedStrands() && !harped_fill.empty() && harped_fill[0]==1)
      {
         // Check if two strand locations at first strand location
         Float64 xs, ys, xhp, yhp, xend, yend;
         pGdrEntry->GetHarpedStrandCoordinates(0, &xs, &ys, &xhp, &yhp, &xend, &yend);
         if(xs>0.0 || xhp>0.0 || xend>0.0)
         {
            // We have case where odd # harped strands is utilized - inform user that number of strands is being changed
            CString str;
            str.Format(_T("The direct selection method does not support the \"Allow odd number of harped strands\" library option. The current number of harped strands is %d. It will be changed to %d. Is this Ok?"),num_harped, num_harped+1);
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONINFORMATION );
            if (st == IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            harped_fill[0]=2;
            num_harped++;
         }
      }

      ATLASSERT(PRESTRESSCONFIG::CountStrandsInFill(straight_fill) == num_straight);
      ATLASSERT(PRESTRESSCONFIG::CountStrandsInFill(harped_fill) == num_harped);
      ATLASSERT(PRESTRESSCONFIG::CountStrandsInFill(temp_fill) == num_temp);

      pParent->m_pSegment->Strands.SetDirectStrandFillStraight( ConvertConfigToDirectStrandFill(straight_fill) );
      pParent->m_pSegment->Strands.SetDirectStrandFillHarped( ConvertConfigToDirectStrandFill(harped_fill) );
      pParent->m_pSegment->Strands.SetDirectStrandFillTemporary( ConvertConfigToDirectStrandFill(temp_fill) );

      HideControls(0, newStrandDefinitionType); 
      HideControls(1, newStrandDefinitionType); 
   }
   else 
   {
      // Going to a sequential strand fill
      // See if we are coming from direct selection - if so, we need to convert strand data
      if(m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection)
      {
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
         if (0 < num_temp)
         {
            bool success = pStrandGeometry->IsValidNumStrands( pParent->m_strGirderName.c_str(), pgsTypes::Temporary, num_temp);
            if (!success)
            {
               CString str;
               str.Format(_T("Current number of Temporary strands %d, is not compatible with the Girder Library. Number of strands will be set to zero. Is this Ok?"),num_temp);
               int st = ::AfxMessageBox(str, MB_OKCANCEL | MB_ICONINFORMATION );
               if (st==IDCANCEL)
               {
                  // Let user cancel operation
                  box->SetCurSel(m_CurrStrandDefinitionType);
                  return;
               }

               num_temp = 0;
            }
         }

         StrandIndexType nStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Temporary );

         CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_TEMP_SPIN );
         UDACCEL uda;
         uda.nSec=0;
         uda.nInc=1;
         pSpin->SetAccel(1,&uda);
         pSpin->SetRange( 0, short(nStrandsMax) );
         pSpin->SetPos((int)num_temp);
      }

      // Deal with permanent strands
      if (newStrandDefinitionType == CStrandData::sdtTotal)
      {
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
         StrandIndexType newNumStraight, newNumHarped;
         bool success = pStrandGeometry->ComputeNumPermanentStrands( num_perm, pParent->m_strGirderName.c_str(), &newNumStraight, &newNumHarped);
         if (!success)
         {
            CString str;
            str.Format(_T("Current number of Permanent strands %d, is not compatible with the Girder Library. Number of strands will be set to zero. Is this Ok?"),num_perm);
            int st = ::AfxMessageBox(str, MB_OKCANCEL | MB_ICONINFORMATION );
            if (st==IDCANCEL)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            num_perm = 0;
         }
         else if (num_straight!=newNumStraight || num_harped!=newNumHarped)
         {
            CString str;
            str.Format(_T("Current values of %d Straight and %d Harped are not compatible with the Girder Library. For %d Total strands you will have %d Straight and %d Harped. Is this Ok?"), num_straight, num_harped, num_perm, newNumStraight, newNumHarped);
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONINFORMATION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
         }
         else if (0 < num_perm && m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection)
         {
            CString str(_T("Strands from direct selection may not be in the same locations in the new fill mode. Is this Ok?"));
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONINFORMATION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
         }

         StrandIndexType nStrandsMax = pStrandGeometry->GetMaxNumPermanentStrands(pParent->m_strGirderName.c_str());

         CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
         UDACCEL uda;
         uda.nSec=0;
         uda.nInc=1;
         pSpin->SetAccel(1,&uda);
         pSpin->SetRange( 0, short(nStrandsMax) );
         pSpin->SetPos((int)num_perm);

         HideControls(1, newStrandDefinitionType); 
         UpdateStraightHarped(newNumStraight, newNumHarped);
         UpdateHarpedOffsets(newNumHarped);
      }
      else if  (newStrandDefinitionType == CStrandData::sdtStraightHarped)
      {
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
         bool success = pStrandGeometry->IsValidNumStrands(pParent->m_strGirderName.c_str(), pgsTypes::Harped, num_harped);
         if (!success)
         {
            CString str;
            str.Format(_T("Current number of Harped strands %d, is not compatible with the Girder Library. Number of harped strands will be set to zero. Is this Ok?"),num_harped);
            int st = ::AfxMessageBox(str, MB_OKCANCEL | MB_ICONINFORMATION );
            if (st==IDCANCEL)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            num_harped = 0;
            num_perm = num_straight;
         }

         success = pStrandGeometry->IsValidNumStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight, num_straight);
         if (!success)
         {
            CString str;
            str.Format(_T("Current number of Straight strands %d, is not compatible with the Girder Library. Number of Straight strands will be set to zero. Is this Ok?"),num_harped);
            int st = ::AfxMessageBox(str, MB_OKCANCEL | MB_ICONINFORMATION );
            if (st==IDCANCEL)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            num_straight = 0;
            num_perm = num_harped;
         }

         if  (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection)
         {
            CString str(_T("Strands from direct selection may not be in the same locations in the new fill mode. Is this Ok?"));
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONINFORMATION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
         }

         StrandIndexType nStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight);

         CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_SS_SPIN );
         UDACCEL uda;
         uda.nSec=0;
         uda.nInc=1;
         pSpin->SetAccel(1,&uda);
         pSpin->SetRange( 0, (short)nStrandsMax );
         pSpin->SetPos((int)num_straight);

         nStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Harped);

         pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
         uda.nSec=0;
         uda.nInc=1;
         pSpin->SetAccel(1,&uda);
         pSpin->SetRange( 0, (short)nStrandsMax );
         pSpin->SetPos((int)num_harped);

         HideControls(0, newStrandDefinitionType); 
         HideControls(1, newStrandDefinitionType); 
      }
      else if  (newStrandDefinitionType == CStrandData::sdtDirectInput)
      {
#pragma Reminder("IMPLEMENT - need to convert library strands into user defined strands")
         HideControls(0, newStrandDefinitionType); 
         HideControls(1, newStrandDefinitionType); 
         DisappearEndOffsetControls();
         DisappearHpOffsetControls();
         GetDlgItem(IDC_HP_NOTE)->ShowWindow(SW_HIDE);
      }
      else
      {
         ATLASSERT(false); // new type?
      }
   }

   // convert user-input pjack if coming from or going to total strands
   if  (m_CurrStrandDefinitionType == CStrandData::sdtTotal)
   {
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
      ConvertPJackFromNumPerm(num_straight, num_harped, pDisplayUnits);
   }
   else if  (newStrandDefinitionType == CStrandData::sdtTotal)
   {
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
      ConvertPJackToNumPerm(num_straight, num_harped, pDisplayUnits);
   }

   // Strands have changed - take care of any invalid debonding
   ConfigStrandFillVector fillvec = ComputeStraightStrandFillVector(num_straight);
    
   ReconcileDebonding(fillvec, pParent->m_pSegment->Strands.GetDebonding(pgsTypes::Straight));

   for ( int i = 0; i < 2; i++ )
   {
      std::vector<GridIndexType> extStrands = pParent->m_pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i);
      bool bChanged = ReconcileExtendedStrands(fillvec, extStrands);

      if ( bChanged )
      {
         pParent->m_pSegment->Strands.SetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i,extStrands);
      }
   }

   // Save new adjustment type
   m_CurrStrandDefinitionType = newStrandDefinitionType;
   pParent->m_pSegment->Strands.SetStrandDefinitionType(newStrandDefinitionType);

   UpdatePjackEdits();

   ShowHideNumStrandControls(newStrandDefinitionType);

   // show/hide the Extend Strands tab on the parent dialog
   if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectInput )
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
      pParent->OnGirderTypeChanged(false,false);
   }
   else
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
      
      // add/remove property pages if needed
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2( pBroker, ILibrary, pLib );
      GET_IFACE2( pBroker, ISpecification, pSpec);
      const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

      bool bCanExtendStrands = pSpecEntry->AllowStraightStrandExtensions();
      bool bCanDebond = pGdrEntry->CanDebondStraightStrands();

      pParent->OnGirderTypeChanged(bCanExtendStrands,bCanDebond);
   }
}

void CGirderDescPrestressPage::ConvertPJackFromNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, IEAFDisplayUnits* pDisplayUnits) 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CButton* pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
   ASSERT( pbut );
   bool calc_permanent   = pbut->GetCheck() != 0;

   pbut = (CButton*) GetDlgItem( IDC_SS_JACK );
   ASSERT( pbut );
   pbut->SetCheck( calc_permanent ? 1 : 0);

   pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped,   calc_permanent);
   pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight, calc_permanent);

   if (!calc_permanent)
   {
      CWnd* pWnd = GetDlgItem( IDC_HS_JACK_FORCE );
      ASSERT( pWnd );
      CString val_as_text;
      pWnd->GetWindowText( val_as_text );
      Float64 Pjack = _tstof( val_as_text );
      Pjack = ::ConvertToSysUnits( Pjack, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure );

      StrandIndexType num_perm = numStraight+numHarped;
      if (num_perm>0)
      {
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Harped,   Pjack * (Float64)numHarped/(Float64)num_perm);
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Straight, Pjack * (Float64)numStraight/(Float64)num_perm);
      }
      else
      {
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Harped,   0.0);
         pParent->m_pSegment->Strands.SetPjack(pgsTypes::Straight, 0.0);
      }

      CDataExchange dx(this,FALSE);
      Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Harped);
      DDX_UnitValueAndTag( &dx, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );

      Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Straight);
      DDX_UnitValueAndTag( &dx, IDC_SS_JACK_FORCE, IDC_SS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }
}

void CGirderDescPrestressPage::ConvertPJackToNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, IEAFDisplayUnits* pDisplayUnits) 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CButton* pbut = (CButton*) GetDlgItem( IDC_SS_JACK );
   ASSERT( pbut );
   bool calc_straight = pbut->GetCheck() != 0;

   pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
   ASSERT( pbut );
   bool calc_harped   = pbut->GetCheck() != 0;

   // Only convert user input if both pjacs are user input, otherwise calculate
   bool user_input = !calc_straight && !calc_harped;

   pbut->SetCheck( user_input ? 0 : 1);

   pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Permanent, !user_input);

   if (user_input)
   {
      // Sum hs and ss jacking forces to permanent
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
      }

      pParent->m_pSegment->Strands.SetPjack(pgsTypes::Permanent, jack_straight + jack_harped);

      CDataExchange dx(this,FALSE);
      Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Permanent);
      DDX_UnitValueAndTag( &dx, IDC_HS_JACK_FORCE, IDC_HS_JACK_FORCE_UNIT, Pjack, pDisplayUnits->GetGeneralForceUnit() );
   }
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

void CGirderDescPrestressPage::OnStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CDataExchange DX(this,true);
	DDX_CBIndex(&DX, IDC_STRAND_SIZE, (int&)m_StrandSizeIdx);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   Int32 key = (Int32)pList->GetItemData( (int)m_StrandSizeIdx );

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight, pPool->GetStrand( key ));
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,   pPool->GetStrand( key ));

   // Now we can update pjack values in dialog
   UpdatePjackEdits();
}

void CGirderDescPrestressPage::OnTempStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CDataExchange DX(this,true);
	DDX_CBIndex(&DX, IDC_TEMP_STRAND_SIZE, (int&)m_TempStrandSizeIdx);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
   Int32 key = (Int32)pList->GetItemData( (int)m_TempStrandSizeIdx );

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand( key ));

   // Now we can update pjack values in dialog
   UpdatePjackEdits();
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

void CGirderDescPrestressPage::OnBnClickedEditStrandFill()
{
   if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection )
   {
      EditDirectSelect();
   }
   else
   {
      ASSERT(m_CurrStrandDefinitionType == CStrandData::sdtDirectInput);
      EditDirectInput();
   }
}

void CGirderDescPrestressPage::EditDirectSelect()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibrary, pLib );
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());

   GET_IFACE2( pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // Get current offset input values - dialog will force in bounds if needed
   Float64 hpOffsetAtEnd(0.0), hpOffsetAtHp(0.0);
   HarpedStrandOffsetType endMeasureType(hsoLEGACY), hpMeasureType(hsoLEGACY);
   CDataExchange DXf(this, TRUE);
   if (m_AllowEndAdjustment)
   {
      DDX_UnitValueAndTag( &DXf, IDC_HPOFFSET_END, IDC_HPOFFSET_END_UNIT, hpOffsetAtEnd, pDisplayUnits->GetComponentDimUnit() );

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_END);
      int cursel = box->GetCurSel();
      endMeasureType = (HarpedStrandOffsetType)box->GetItemData(cursel);
   }

   if (m_AllowHpAdjustment)
   {
      DDX_UnitValueAndTag( &DXf, IDC_HPOFFSET_HP, IDC_HPOFFSET_HP_UNIT, hpOffsetAtHp, pDisplayUnits->GetComponentDimUnit() );

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP);
      int cursel = box->GetCurSel();
      hpMeasureType = (HarpedStrandOffsetType)box->GetItemData(cursel);
   }

   // Max debond length is 1/2 girder length
   Float64 maxDebondLength = pBridge->GetSegmentLength(pParent->m_SegmentKey)/2.0;

   CGirderSelectStrandsDlg dlg;
   dlg.m_SelectStrandsPage.InitializeData(pParent->m_SegmentKey, &pParent->m_pSegment->Strands, pSpecEntry, pGdrEntry,
                      m_AllowEndAdjustment, m_AllowHpAdjustment, endMeasureType, hpMeasureType, hpOffsetAtEnd, hpOffsetAtHp, maxDebondLength);

   if ( dlg.DoModal() == IDOK )
   {
      UpdateData(FALSE);

      //// put harped strand offsets into controls
      //CDataExchange DX(this, FALSE);
      //if (m_AllowEndAdjustment)
      //{
      //   Float64 offset = pParent->m_pSegment->Strands.HpOffsetAtEnd;
      //   DDX_UnitValueAndTag( &DX, IDC_HPOFFSET_END, IDC_HPOFFSET_END_UNIT, offset, pDisplayUnits->GetComponentDimUnit() );
      //}

      //if (m_AllowHpAdjustment)
      //{
      //   Float64 offset = pParent->m_pSegment->Strands.HpOffsetAtHp;
      //   DDX_UnitValueAndTag( &DX, IDC_HPOFFSET_HP, IDC_HPOFFSET_HP_UNIT, offset, pDisplayUnits->GetComponentDimUnit() );
      //}

      // update pjack and display elements
      ShowHideNumStrandControls(m_CurrStrandDefinitionType);
      UpdatePjackEdits();
      UpdateHarpedOffsets(pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped));
   }
}

void CGirderDescPrestressPage::EditDirectInput()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CPropertySheet dlg(_T("Define Strands"),this);
   CGirderSegmentStrandsPage page;
   page.Init(pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex));
   dlg.AddPage(&page);
   if ( dlg.DoModal() == IDOK )
   {
      UpdateData(FALSE);
      ShowHideNumStrandControls(m_CurrStrandDefinitionType);
      UpdatePjackEdits();
   }
}

ConfigStrandFillVector CGirderDescPrestressPage::ComputeStraightStrandFillVector(StrandIndexType Ns)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection)
   {
      // Fill is stored locally in compacted form
      ATLASSERT(pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight)==Ns);

      ConfigStrandFillVector vec = ConvertDirectToConfigFill(pStrandGeometry, pgsTypes::Straight, 
                                   pParent->m_strGirderName.c_str(), 
                                   *(pParent->m_pSegment->Strands.GetDirectStrandFillStraight()));

      return vec;
   }
   else
   {
      return pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Straight, Ns);
   }
}

ConfigStrandFillVector CGirderDescPrestressPage::ComputeHarpedStrandFillVector()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection)
   {
      ConfigStrandFillVector vec = ConvertDirectToConfigFill(pStrandGeometry, pgsTypes::Harped, 
                                   pParent->m_strGirderName.c_str(), 
                                   *(pParent->m_pSegment->Strands.GetDirectStrandFillHarped()));

      return vec;
   }
   else
   {
      // Num harped is in a control
      StrandIndexType Nh = CGirderDescPrestressPage::GetHarpedStrandCount();

      return pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(), pgsTypes::Harped, Nh);
   }
}


