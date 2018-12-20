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

// BridgeDescPrestressPage.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescPrestressPage.h"
#include "GirderDescDlg.h"
#include "DebondDlg.h"
#include "GirderSelectStrandsDlg.h"

#include "GirderSegmentStrandsPage.h" // replace with real dialog header when we have it

#include <PgsExt\DesignConfigUtil.h>

#include <GenericBridge\Helpers.h>

#include <Material\PsStrand.h>
#include <LRFD\StrandPool.h>

#include <MfcTools\CustomDDX.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>

#include <PsgLib\GirderLibraryEntry.h>

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
	m_StrandKey = -1;
	m_TempStrandKey = -1;
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
   DDX_Tag( pDX, IDC_HPOFFSET_START_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_HPOFFSET_HP1_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_HPOFFSET_HP2_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_HPOFFSET_END_UNIT, pDisplayUnits->GetComponentDimUnit() );

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

      bool bPjackUserInput = !pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Permanent);
      DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackUserInput);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Permanent,!bPjackUserInput);
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
   else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
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
   if (m_CurrStrandDefinitionType==CStrandData::sdtStraightHarped || m_CurrStrandDefinitionType==CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType==CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
   {
      // Harped
      bool bPjackUserInput = !pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped);
      DDX_Check_Bool(pDX, IDC_HS_JACK, bPjackUserInput);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Harped,!bPjackUserInput);
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
      bPjackUserInput = !pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight);
	   DDX_Check_Bool(pDX, IDC_SS_JACK, bPjackUserInput );
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Straight,!bPjackUserInput);
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
   if (m_CurrStrandDefinitionType != CStrandData::sdtDirectSelection && m_CurrStrandDefinitionType != CStrandData::sdtDirectRowInput && m_CurrStrandDefinitionType != CStrandData::sdtDirectStrandInput)
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

   bool bPjackUserInput = !pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary);
	DDX_Check_Bool(pDX, IDC_TEMP_JACK, bPjackUserInput);
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_pSegment->Strands.IsPjackCalculated(pgsTypes::Temporary,!bPjackUserInput);
   }

   pgsTypes::TTSUsage ttsUsage = pParent->m_pSegment->Strands.GetTemporaryStrandUsage();
   DDX_CBItemData(pDX, IDC_TTS_USE, ttsUsage );
   if ( pDX->m_bSaveAndValidate )
   {
      Float64 precamber = pParent->m_pSegment->Precamber;
      if (!IsZero(precamber) && ttsUsage == pgsTypes::ttsPretensioned && pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary) != 0)
      {
         AfxMessageBox(_T("Temporary top strands must be post-tensioned when girders are precambered"));
         pDX->PrepareCtrl(IDC_TTS_USE);
         pDX->Fail();
      }
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
   //m_AllowEndAdjustment = 0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(pParent->m_strGirderName.c_str());
   pgsTypes::AdjustableStrandType adjType = pParent->m_pSegment->Strands.GetAdjustableStrandType();

   m_AllowEndAdjustment = 0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(pParent->m_strGirderName.c_str(), adjType);

   if (m_AllowEndAdjustment)
   {
      if (!pDX->m_bSaveAndValidate)
      {
         // must convert data from legacy data files
         if (pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd()==hsoLEGACY)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_strGirderName.c_str(), endType,
                                                                                      adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                      harpFill,
                                                                                      pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                      pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(endType));

               Float64 topcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(pParent->m_strGirderName.c_str(), endType,
                                                                                          adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                          harpFill,
                                                                                          hsoCGFROMTOP, 
                                                                                          absol_offset);

               pParent->m_pSegment->Strands.SetHarpStrandOffsetAtEnd(endType,topcg_offset);
            }
            pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtEnd(hsoCGFROMTOP);
         }

         UpdateEndRangeLength(pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), nh);
      }

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

         Float64 offset = pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(endType);
         UINT nIDC     = (endType == pgsTypes::metStart ? IDC_HPOFFSET_START      : IDC_HPOFFSET_END);
         UINT nIDCUnit = (endType == pgsTypes::metStart ? IDC_HPOFFSET_START_UNIT : IDC_HPOFFSET_END_UNIT);
         DDX_UnitValueAndTag( pDX, nIDC, nIDCUnit, offset, pDisplayUnits->GetComponentDimUnit() );

         if ( pDX->m_bSaveAndValidate )
         {
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtEnd(endType,offset);
         }
      }

      if ( pDX->m_bSaveAndValidate )
      {
         GET_IFACE2(pBroker,IGirder,pGirder);
         if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
         {
            // if the segment is symmetric, the UI is only for the start half
            // copy the start half parameters to the end half
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtEnd(pgsTypes::metEnd,pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
         }
      }

      HarpedStrandOffsetType offsetType = pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd();
	   DDX_CBItemData(pDX, IDC_HP_COMBO_END, offsetType);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtEnd(offsetType);
      }

      if ( nh <= 0)
      {
         DisableEndOffsetControls(TRUE);
      }
   }
   else
   {
      DisableEndOffsetControls(TRUE);
   }

   m_AllowHpAdjustment = false;
   bool might_allow_hp_adjustment = false;
   if (m_LibraryAdjustableStrandType!=pgsTypes::asStraight)
   {
      m_AllowHpAdjustment = 0.0 <= pStrandGeometry->GetHarpedHpOffsetIncrement(pParent->m_strGirderName.c_str(), adjType);

      // Set variables for harping point adjustment up front if we might need them, not just if we need them now
      might_allow_hp_adjustment = m_AllowHpAdjustment || 
                                 (0.0 <= pStrandGeometry->GetHarpedHpOffsetIncrement(pParent->m_strGirderName.c_str(), pgsTypes::asHarped));
   }

   if (might_allow_hp_adjustment)
   {
      // must convert data from legacy data files
      if (!pDX->m_bSaveAndValidate)
      {
         if(pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint()==hsoLEGACY)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

               Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_strGirderName.c_str(), endType,
                                                                                     adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                     harpFill,
                                                                                     pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                                     pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(endType));

               Float64 botcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(pParent->m_strGirderName.c_str(), endType,
                                                                                         adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                         harpFill, 
                                                                                         hsoCGFROMBOTTOM, absol_offset);

               pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint( endType, botcg_offset );
            }
            pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint( hsoCGFROMBOTTOM );
         }

         UpdateHpRangeLength(pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), nh);
      }

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         Float64 offset = pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(endType);
         UINT nIDC     = (endType == pgsTypes::metStart ? IDC_HPOFFSET_HP1      : IDC_HPOFFSET_HP2);
         UINT nIDCUnit = (endType == pgsTypes::metStart ? IDC_HPOFFSET_HP1_UNIT : IDC_HPOFFSET_HP2_UNIT);
         DDX_UnitValueAndTag( pDX, nIDC, nIDCUnit, offset, pDisplayUnits->GetComponentDimUnit() );
         if ( pDX->m_bSaveAndValidate )
         {
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint(endType,offset);
         }
      }

      if ( pDX->m_bSaveAndValidate )
      {
         GET_IFACE2(pBroker,IGirder,pGirder);
         if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
         {
            // if the segment is symmetric, the UI is only for the start half
            // copy the start half parameters to the end half
            pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd,pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart));
         }
      }

      HarpedStrandOffsetType offsetType = pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint();
   	DDX_CBItemData(pDX, IDC_HP_COMBO_HP, offsetType);
      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint(offsetType);
      }

      if (nh <= 0)
      {
         DisableHpOffsetControls(TRUE);
      }
   }
   else
   {
      DisableHpOffsetControls(TRUE);
   }

   if (pDX->m_bSaveAndValidate && m_CurrStrandDefinitionType != CStrandData::sdtDirectRowInput && m_CurrStrandDefinitionType != CStrandData::sdtDirectStrandInput)
   {
      // determine if offset strands are within girder bounds
      if (0 < nh)
      {
         // But first, for Adj.-Straight strands, make adjustment at hp the same as at ends
         if( adjType==pgsTypes::asStraight && m_AllowEndAdjustment)
         {
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               pParent->m_pSegment->Strands.SetHarpStrandOffsetAtHarpPoint( endType, pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(endType) );
            }
            pParent->m_pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint( pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() );
         }

         // girder ends
         // first convert to abosolute offsets
         if (m_AllowEndAdjustment)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_strGirderName.c_str(), endType,
                                                                                      adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                                      harpFill,
                                                                                      pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                      pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(endType));

               Float64 max_end_offset, min_end_offset;
               pStrandGeometry->GetHarpedEndOffsetBoundsEx(pParent->m_strGirderName.c_str(), endType, adjType,
                                                           m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                           harpFill, &min_end_offset, &max_end_offset);

               UINT nIDC = (endType == pgsTypes::metStart ? IDC_HPOFFSET_START      : IDC_HPOFFSET_END);
               if( max_end_offset+TOLERANCE < absol_offset )
               {
                  HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	               AfxMessageBox( _T("Harped strand offset is excessive at ends of girder - Strand lies above allowable cover"), MB_ICONEXCLAMATION);
	               pDX->Fail();
               }

               if( absol_offset < min_end_offset-TOLERANCE )
               {
                  HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	               AfxMessageBox( _T("Harped strand offset is excessive at ends of girder - Strand lies below allowable cover"), MB_ICONEXCLAMATION);
	               pDX->Fail();
               }
            }
         }

         // harping points
         if (m_AllowHpAdjustment)
         {
            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_strGirderName.c_str(), endType,
                                                                             adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                             harpFill,
                                                                             pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                             pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(endType));

               Float64 max_hp_offset, min_hp_offset;
               pStrandGeometry->GetHarpedHpOffsetBoundsEx(pParent->m_strGirderName.c_str(),endType, 
                                                          adjType, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                          harpFill,
                                                          &min_hp_offset, &max_hp_offset);

               UINT nIDC = (endType == pgsTypes::metStart ? IDC_HPOFFSET_HP1 : IDC_HPOFFSET_HP2);
               if( max_hp_offset+TOLERANCE < absol_offset )
               {
                  HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	               AfxMessageBox( _T("Harped strand offset is excessive at harping points - Strand lies above allowable cover"), MB_ICONEXCLAMATION);
	               pDX->Fail();
               }

               if( absol_offset < min_hp_offset-TOLERANCE )
               {
                  HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	               AfxMessageBox( _T("Harped strand offset is excessive at harping points - Strand lies below allowable cover"), MB_ICONEXCLAMATION);
	               pDX->Fail();
               }
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

	DDX_CBItemData(pDX, IDC_STRAND_SIZE, m_StrandKey);
	DDX_CBItemData(pDX, IDC_TEMP_STRAND_SIZE, m_TempStrandKey);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight, pPool->GetStrand( m_StrandKey ));
      pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,   pPool->GetStrand( m_StrandKey ));

      pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand( m_TempStrandKey ));

      if ( m_CurrStrandDefinitionType == CStrandData::sdtTotal )
      {
         // Update Pjack for individual strand types
         StrandIndexType nPermStrands = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Permanent);
         if ( 0 < nPermStrands )
         {
            StrandIndexType nStraightStrands = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
            StrandIndexType nHarpedStrands = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
            Float64 Pjack = pParent->m_pSegment->Strands.GetPjack(pgsTypes::Permanent);
            pParent->m_pSegment->Strands.SetPjack(pgsTypes::Straight,Pjack*nStraightStrands/nPermStrands);
            pParent->m_pSegment->Strands.SetPjack(pgsTypes::Harped,Pjack*nHarpedStrands/nPermStrands);
         }
      }
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
	ON_CBN_SELCHANGE(IDC_STRAND_INPUT_TYPE, OnStrandInputTypeChanged)
	ON_WM_CTLCOLOR()
	ON_CBN_DROPDOWN(IDC_HP_COMBO_HP, OnDropdownHpComboHp)
	ON_CBN_DROPDOWN(IDC_HP_COMBO_END, OnDropdownHpComboEnd)
	ON_CBN_SELCHANGE(IDC_STRAND_SIZE, OnStrandTypeChanged)
	ON_CBN_SELCHANGE(IDC_TEMP_STRAND_SIZE, OnTempStrandTypeChanged)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_EDIT_STRAND_FILL, &CGirderDescPrestressPage::OnBnClickedEditStrandFill)
   ON_CBN_SELCHANGE(IDC_ADJUSTABLE_COMBO, &CGirderDescPrestressPage::OnCbnSelchangeAdjustableCombo)
   ON_BN_CLICKED(IDC_EPOXY,&CGirderDescPrestressPage::OnEpoxyChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescPrestressPage message handlers

BOOL CGirderDescPrestressPage::OnInitDialog() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   // Get key segment dimensions
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(pParent->m_SegmentKey);

   PoiList vPoi;
   pPoi->GetPointsOfInterest(pParent->m_SegmentKey, POI_START_FACE | POI_END_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 2);
   const pgsPointOfInterest& poiStart = vPoi.front();
   const pgsPointOfInterest& poiEnd = vPoi.back();

   m_HgStart = pSectProp->GetHg(intervalIdx,poiStart);
   m_HgEnd   = pSectProp->GetHg(intervalIdx,poiEnd);

   vPoi.clear();
   pPoi->GetPointsOfInterest(pParent->m_SegmentKey, POI_HARPINGPOINT, &vPoi);
   if ( 0 < vPoi.size() )
   {
      ATLASSERT(vPoi.size() == 1 || vPoi.size() == 2);
      const pgsPointOfInterest& poiHp1(vPoi.front());
      const pgsPointOfInterest& poiHp2(vPoi.back());

      m_HgHp1 = pSectProp->GetHg(intervalIdx,poiHp1);
      m_HgHp2 = pSectProp->GetHg(intervalIdx,poiHp2);
   }
   else
   {
      m_HgHp1 = m_HgStart;
      m_HgHp2 = m_HgEnd;
   }


   // Deal with adjustable strands 
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());
   m_LibraryAdjustableStrandType = pGdrEntry->GetAdjustableStrandType();

   // Select the strand size
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   m_StrandKey     = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight)); // straight and harped use the same strand type
   m_TempStrandKey = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary));

   if ( sysFlags<Int32>::IsSet(m_StrandKey,matPsStrand::GritEpoxy) )
   {
      CheckDlgButton(IDC_EPOXY,BST_CHECKED);
   }

   UpdateStrandList(IDC_STRAND_SIZE);
   UpdateStrandList(IDC_TEMP_STRAND_SIZE);

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
   idx = pCB->AddString(_T("Number of Permanent Strands"));
   pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtTotal);
   if( m_LibraryAdjustableStrandType == pgsTypes::asHarped )
   {
      idx = pCB->AddString(_T("Number of Straight and Harped Strands"));
      pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtStraightHarped);
      GetDlgItem(IDC_VERT_GROUP)->SetWindowText(_T("Vertical Location of Harped Strands"));
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Girder Ends:"));
   }
   else if( m_LibraryAdjustableStrandType == pgsTypes::asStraight )
   {    
      idx = pCB->AddString(_T("Number of Straight and Adjustable Straight Strands"));
      pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtStraightHarped);
      GetDlgItem(IDC_VERT_GROUP)->SetWindowText(_T("Vertical Location of Adjustable Straight Strands"));
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Along Girder:"));

      ShowHpOffsetControls(FALSE);
   }
   else
   {
      idx = pCB->AddString(_T("Number of Straight and Number of Adjustable Strands"));
      pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtStraightHarped);

      GetDlgItem(IDC_VERT_GROUP)->SetWindowText(_T("Vertical Location of Adjustable Strands"));
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Girder Ends"));
   }

   idx = pCB->AddString(_T("Strand Locations"));
   pCB->SetItemData(idx,(DWORD_PTR)CStrandData::sdtDirectSelection);

   idx = pCB->AddString(_T("Strand Rows"));
   pCB->SetItemData(idx, (DWORD_PTR)CStrandData::sdtDirectRowInput);

   idx = pCB->AddString(_T("Individual Strands"));
   pCB->SetItemData(idx, (DWORD_PTR)CStrandData::sdtDirectStrandInput);

   CPropertyPage::OnInitDialog();

   OnDropdownHpComboHp();
   OnDropdownHpComboEnd();

   // adjustable strand controls
   CComboBox* pAdjList = (CComboBox*)GetDlgItem( IDC_ADJUSTABLE_COMBO );
   pAdjList->SetCurSel((int)pParent->m_pSegment->Strands.GetAdjustableStrandType());
   if (m_LibraryAdjustableStrandType != pgsTypes::asStraightOrHarped)
   {
      pAdjList->EnableWindow(FALSE); // User can't choose if library doesn't allow
   }
   else
   {
      UpdateAdjustableStrandControls();
   }

   EnableToolTips(TRUE);

   if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
   {
      ShowEndOffsetControls(FALSE);
      ShowHpOffsetControls(FALSE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescPrestressPage::InitHarpStrandOffsetMeasureComboBox(CComboBox* pCB)
{
   pCB->Clear();
   int idx;

   CString strItem;

   strItem.Format(_T("Distance between CG of %s Group and Girder Top"), m_LibraryAdjustableStrandType == pgsTypes::asHarped ? _T("Harped") : _T("Adjustable"));
   idx = pCB->AddString(strItem); 
   pCB->SetItemData(idx,hsoCGFROMTOP);

   strItem.Format(_T("Distance between CG of %s Group and Girder Bottom"), m_LibraryAdjustableStrandType == pgsTypes::asHarped ? _T("Harped") : _T("Adjustable"));
   idx = pCB->AddString(strItem);
   pCB->SetItemData(idx,hsoCGFROMBOTTOM);

   strItem.Format(_T("Distance between Top-Most %s Strand and Girder Top"), m_LibraryAdjustableStrandType == pgsTypes::asHarped ? _T("Harped") : _T("Adjustable"));
   idx = pCB->AddString(strItem);
   pCB->SetItemData(idx,hsoTOP2TOP);

   strItem.Format(_T("Distance between Top-Most %s Strand and Girder Bottom"), m_LibraryAdjustableStrandType == pgsTypes::asHarped ? _T("Harped") : _T("Adjustable"));
   idx = pCB->AddString(strItem);
   pCB->SetItemData(idx,hsoTOP2BOTTOM);

   strItem.Format(_T("Distance between Bottom-Most %s Strand and Girder Bottom"), m_LibraryAdjustableStrandType == pgsTypes::asHarped ? _T("Harped") : _T("Adjustable"));
   idx = pCB->AddString(strItem);
   pCB->SetItemData(idx,hsoBOTTOM2BOTTOM);

   strItem.Format(_T("Eccentricity of %s Strand Group (Non-Composite Section)"), m_LibraryAdjustableStrandType == pgsTypes::asHarped ? _T("Harped") : _T("Adjustable"));
   idx = pCB->AddString(strItem);
   pCB->SetItemData(idx,hsoECCENTRICITY);
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

      DisableEndOffsetControls(numHarped<=0);
   }

   if (m_AllowHpAdjustment)
   {
      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP);
      int cursel = box->GetCurSel();
      HarpedStrandOffsetType measureType = (HarpedStrandOffsetType)box->GetItemData(cursel);

      UpdateHpRangeLength(measureType, numHarped);

      DisableHpOffsetControls(numHarped<=0);
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
   bool bIsDirect = (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput);
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
      bEnableUserInput= IsDlgButtonChecked( nCheckBox ) == BST_CHECKED ? TRUE : FALSE;
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

      pgsTypes::StrandType strandType;
      switch( nCheckBox )
      {
      case IDC_HS_JACK:
         {
            strandType = pgsTypes::Harped;
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
         strandType = pgsTypes::Straight;
         Pjack = GetMaxPjack( nStrands, pgsTypes::Straight );
         break;

      case IDC_TEMP_JACK:
         strandType = pgsTypes::Temporary;
         Pjack = GetMaxPjack(nStrands,pgsTypes::Temporary);
         break;
      }

      CDataExchange dx(this,FALSE);
      DDX_UnitValueAndTag( &dx, nEdit, nUnit, Pjack, pDisplayUnits->GetGeneralForceUnit() );

      if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
      {
         pParent->m_pSegment->Strands.SetPjack(strandType,Pjack);
      }
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

   if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
   {
      pgsTypes::StrandType strandType;
      switch( nCheckBox )
      {
      case IDC_HS_JACK: strandType = pgsTypes::Harped; break;
      case IDC_SS_JACK: strandType = pgsTypes::Straight; break;
      case IDC_TEMP_JACK: strandType = pgsTypes::Temporary; break;
      }

      pParent->m_pSegment->Strands.IsPjackCalculated(strandType,bEnableUserInput == TRUE ? false : true);
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
   if (m_CurrStrandDefinitionType == CStrandData::sdtTotal)
   {
      nStrandsMax = pStrandGeom->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Permanent);

      pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
      uda;
      uda.nSec=0;
      uda.nInc=1;
      pSpin->SetAccel(1,&uda);
      pSpin->SetRange( 0, short(nStrandsMax) );
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtStraightHarped)
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

void CGirderDescPrestressPage::DisableEndOffsetControls(BOOL hide)
{
   CWnd* pWnd = 0;

   BOOL show = hide==TRUE ? FALSE : TRUE;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   bool bSymmetric = pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey());

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_END_TITLE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_END );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_START );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_START_UNIT );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   if ( !bSymmetric )
   {
      pWnd = GetDlgItem( IDC_HPOFFSET_END );
      ASSERT( pWnd );
      pWnd->EnableWindow( show );

      pWnd = GetDlgItem( IDC_HPOFFSET_END_UNIT );
      ASSERT( pWnd );
      pWnd->EnableWindow( show );
   }

   pWnd = GetDlgItem( IDC_HPOFFSET_END_NOTE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );
}

void CGirderDescPrestressPage::ShowEndOffsetControls(BOOL shw)
{
   CWnd* pWnd = 0;

   int show = shw ? SW_SHOW : SW_HIDE;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   bool bSymmetric = pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey());

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_END_TITLE );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_END );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_START );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_START_UNIT );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );

   if ( !bSymmetric )
   {
      pWnd = GetDlgItem( IDC_HPOFFSET_END );
      ASSERT( pWnd );
      pWnd->ShowWindow( show );

      pWnd = GetDlgItem( IDC_HPOFFSET_END_UNIT );
      ASSERT( pWnd );
      pWnd->ShowWindow( show );
   }

   pWnd = GetDlgItem( IDC_HPOFFSET_END_NOTE );
   ASSERT( pWnd );
   pWnd->ShowWindow( show );
}

void CGirderDescPrestressPage::DisableHpOffsetControls(BOOL hide)
{
   CWnd* pWnd = 0;
   BOOL show = hide==TRUE ? FALSE : TRUE;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   bool bSymmetric = pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey());

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_HP_TITLE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HP_COMBO_HP );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP1 );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP1_UNIT );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );

   if ( !bSymmetric )
   {
      pWnd = GetDlgItem( IDC_HPOFFSET_HP2 );
      ASSERT( pWnd );
      pWnd->EnableWindow( show );

      pWnd = GetDlgItem( IDC_HPOFFSET_HP2_UNIT );
      ASSERT( pWnd );
      pWnd->EnableWindow( show );
   }

   pWnd = GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   ASSERT( pWnd );
   pWnd->EnableWindow( show );
}

void CGirderDescPrestressPage::ShowHpOffsetControls(BOOL show)
{
   CWnd* pWnd;

   int sShow = show ? SW_SHOW : SW_HIDE;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   bool bSymmetric = pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey());

   // These are the strand pattern offset controls
   pWnd = GetDlgItem( IDC_HPOFFSET_HP_TITLE );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );

   pWnd = GetDlgItem( IDC_HP_COMBO_HP );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP1 );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );

   pWnd = GetDlgItem( IDC_HPOFFSET_HP1_UNIT );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );

   if ( !bSymmetric )
   {
      pWnd = GetDlgItem( IDC_HPOFFSET_HP2 );
      ASSERT( pWnd );
      pWnd->ShowWindow( sShow );

      pWnd = GetDlgItem( IDC_HPOFFSET_HP2_UNIT );
      ASSERT( pWnd );
      pWnd->ShowWindow( sShow );
   }

   pWnd = GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );
}

void CGirderDescPrestressPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_STRANDS );
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
      else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
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
      else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
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

      Float64 low[2], high[2];

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

         Float64 lowRange, highRange;
         pStrandGeom->ComputeValidHarpedOffsetForMeasurementTypeEnd(pParent->m_strGirderName.c_str(),endType,
                                                                    pParent->m_pSegment->Strands.GetAdjustableStrandType(),
                                                                    m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                                    harpFill, measureType, &lowRange, &highRange);

         low[endType]  = Min(lowRange, highRange);
         high[endType] = Max(lowRange, highRange);

         // If offset value is blank, we need to fill it. Place at highest location in valid range
         UINT nIDC = (endType == pgsTypes::metStart ? IDC_HPOFFSET_START : IDC_HPOFFSET_END);
         CWnd* pWnd = GetDlgItem( nIDC );
         ASSERT( pWnd );
         CString txt;
         pWnd->GetWindowText(txt);
         if (txt.IsEmpty())
         {
            CDataExchange DX(this, FALSE);
            UINT nIDCUnit = (endType == pgsTypes::metStart ? IDC_HPOFFSET_START_UNIT : IDC_HPOFFSET_END_UNIT);
            DDX_UnitValueAndTag( &DX, nIDC, nIDCUnit, high[endType], pDisplayUnits->GetComponentDimUnit() );
         }
      }

      // Update message
      GET_IFACE2(pBroker,IGirder,pGirder);
      if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
      {
         str.Format(_T("Valid Range %s to %s"),
            ::FormatDimension(low[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(high[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false));
      }
      else
      {
         str.Format(_T("Valid Range %s to %s (Left) %s to %s (Right)"),
            ::FormatDimension(low[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(high[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(low[pgsTypes::metEnd],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(high[pgsTypes::metEnd],pDisplayUnits->GetComponentDimUnit(),false));
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


      Float64 low[2], high[2];

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

         Float64 lowRange, highRange;
         pStrandGeom->ComputeValidHarpedOffsetForMeasurementTypeHp(pParent->m_strGirderName.c_str(), endType, pParent->m_pSegment->Strands.GetAdjustableStrandType(), m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, harpFill, measureType, &lowRange, &highRange);


         low[endType]  = Min(lowRange, highRange);
         high[endType] = Max(lowRange, highRange);

         // If offset value is blank, we need to fill it. Place at lowest location in valid range
         UINT nIDC = (endType == pgsTypes::metStart ? IDC_HPOFFSET_HP1 : IDC_HPOFFSET_HP2);
         CWnd* pWnd = GetDlgItem( nIDC );
         ASSERT( pWnd );
         CString txt;
         pWnd->GetWindowText(txt);
         if (txt.IsEmpty())
         {
            CDataExchange DX(this, FALSE);
            UINT nIDCUnit = (endType == pgsTypes::metStart ? IDC_HPOFFSET_HP1_UNIT : IDC_HPOFFSET_HP2_UNIT);
            DDX_UnitValueAndTag( &DX, nIDC, nIDCUnit, low[endType], pDisplayUnits->GetComponentDimUnit() );
         }
      }

      // Update message
      GET_IFACE2(pBroker,IGirder,pGirder);
      if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
      {
         str.Format(_T("Valid Range %s to %s"),
            ::FormatDimension(low[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(high[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false));
      }
      else
      {
         str.Format(_T("Valid Range %s to %s (Left) %s to %s (Right)"),
            ::FormatDimension(low[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(high[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(low[pgsTypes::metEnd],pDisplayUnits->GetComponentDimUnit(),false),
            ::FormatDimension(high[pgsTypes::metEnd],pDisplayUnits->GetComponentDimUnit(),false));
      }
   }

   CEdit* pEdit = (CEdit*)GetDlgItem( IDC_HPOFFSET_HP_NOTE );
   pEdit->SetWindowText(str);
}

void CGirderDescPrestressPage::UpdateStraightHarped(StrandIndexType Ns, StrandIndexType Nh)
{
   CString val_as_text;
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   if (pParent->m_pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asStraight)
   {
      val_as_text.Format(_T("Number of Straight: %d, Adjustable Straight: %d"),Ns, Nh);
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

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

         CString strOffset;
         UINT nIDC = (endType == pgsTypes::metStart ? IDC_HPOFFSET_HP1 : IDC_HPOFFSET_HP2);
         CWnd* pWnd = GetDlgItem(nIDC);
         pWnd->GetWindowText(strOffset);
         Float64 offset = _tstof(strOffset);

         offset = ::ConvertToSysUnits(offset,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

         ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

         offset = pStrandGeom->ConvertHarpedOffsetHp(pParent->m_strGirderName.c_str(), endType,
                                                     pParent->m_pSegment->Strands.GetAdjustableStrandType(),
                                                     m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                     harpFill,  m_OldHpMeasureType, offset, measureType);
         
         strOffset = ::FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false);
         pWnd->SetWindowText(strOffset);
      }
   }

   StrandIndexType Nh = GetHarpedStrandCount();

   UpdateHpRangeLength(measureType, Nh);
}

void CGirderDescPrestressPage::OnSelchangeHpComboEnd() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_END);
   int cursel = box->GetCurSel();

   HarpedStrandOffsetType measureType;
   if (cursel != CB_ERR)
   {
      measureType = (HarpedStrandOffsetType)box->GetItemData(cursel);

      SHORT keyState = GetKeyState(VK_CONTROL);
      if ( keyState < 0 )
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

            CString strOffset;
            UINT nIDC = (endType == pgsTypes::metStart ? IDC_HPOFFSET_START : IDC_HPOFFSET_END);
            CWnd* pWnd = GetDlgItem(nIDC);
            pWnd->GetWindowText(strOffset);
            Float64 offset = _tstof(strOffset);
            offset = ::ConvertToSysUnits(offset,  pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

            ConfigStrandFillVector harpFill( ComputeHarpedStrandFillVector() );

            offset = pStrandGeom->ConvertHarpedOffsetEnd(pParent->m_strGirderName.c_str(), endType, 
                                                         pParent->m_pSegment->Strands.GetAdjustableStrandType(),
                                                         m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd,
                                                         harpFill, m_OldEndMeasureType, offset, measureType);

            strOffset = FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false);
            pWnd->SetWindowText(strOffset);
         }
      }
   }
   else
   {
      // No selection type set - just set first in list
      box->SetCurSel(0);
      measureType = (HarpedStrandOffsetType)box->GetItemData(0);
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


   if(strandDefinitionType == CStrandData::sdtDirectSelection || strandDefinitionType == CStrandData::sdtDirectRowInput || strandDefinitionType == CStrandData::sdtDirectStrandInput)
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
      else if (strandDefinitionType == CStrandData::sdtDirectRowInput)
      {
         GetDlgItem(IDC_EDIT_STRAND_FILL)->SetWindowText(_T("Define Strand Rows..."));
      }
      else if (strandDefinitionType == CStrandData::sdtDirectStrandInput)
      {
         GetDlgItem(IDC_EDIT_STRAND_FILL)->SetWindowText(_T("Define Individual Strands..."));
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

      if (pParent->m_pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asStraight)
      {
         msg.Format(_T("Number of Adj. Straight Strands = %d"), (int)pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped));
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
         if (pParent->m_pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asStraight)
         {
            msg = _T("Number of Adjustable Straight Strands");
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

void CGirderDescPrestressPage::OnStrandInputTypeChanged() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());

   CComboBox* box = (CComboBox*)GetDlgItem(IDC_STRAND_INPUT_TYPE);
   int idx = box->GetCurSel();
   CStrandData::StrandDefinitionType newStrandDefinitionType = (CStrandData::StrandDefinitionType)box->GetItemData(idx);

   // If strand definition type didnt change, do nothing
   if(newStrandDefinitionType == m_CurrStrandDefinitionType)
   {
      return;
   }

   // First get number of current strands
   StrandIndexType num_straight(0), num_harped(0), num_perm(0), num_temp(0);
   GetStrandCount(&num_straight, &num_harped, &num_temp, &num_perm);

   // Next: "To" new fill  type
   if (newStrandDefinitionType == CStrandData::sdtDirectSelection)
   {
      // Going to direct fill
      if (m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
      {
         CString str(_T("The total number of strands will remain the same, however the strands will be placed in pre-defined locations which may be different than the current strand locations. Additionally, debonding and strands extensions will be discarded. Would you like tor proceed?"));
         int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
         if (st==IDNO)
         {
            // Let user cancel operation
            box->SetCurSel(m_CurrStrandDefinitionType);
            return;
         }
         // clear debonding and strand extensions
         pParent->m_pSegment->Strands.ClearDebondData();
         pParent->m_pSegment->Strands.ClearExtendedStrands();
      }

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
         if(::IsGT(0.0,xs) || ::IsGT(0.0,xhp) || ::IsGT(0.0,xend) )
         {
            // We have case where odd # harped strands is utilized - inform user that number of strands is being changed
            CString str;
            str.Format(_T("The direct selection method does not support the \"Allow odd number of harped strands\" option. The current number of harped strands is %d. It will be changed to %d. Would you like to proceed?"),num_harped, num_harped+1);
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
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
               str.Format(_T("Current number of Temporary strands %d, is not compatible with the girder definition. Number of strands will be adjusted to fit. Would you like to proceed?"),num_temp);
               int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
               if (st==IDNO)
               {
                  // Let user cancel operation
                  box->SetCurSel(m_CurrStrandDefinitionType);
                  return;
               }

               num_temp = pStrandGeometry->GetNextNumStrands(pParent->m_SegmentKey,pgsTypes::Temporary,num_temp);
               if ( num_temp == INVALID_INDEX )
               {
                  num_temp = pStrandGeometry->GetPrevNumStrands(pParent->m_SegmentKey,pgsTypes::Temporary,num_temp);
                  ATLASSERT(num_temp != INVALID_INDEX);
                  if ( num_temp == INVALID_INDEX )
                  {
                     num_temp = 0;
                  }
               }
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
         GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);

         StrandIndexType newNumStraight, newNumHarped;
         bool bSuccess = pStrandGeometry->ComputeNumPermanentStrands(num_perm, pParent->m_strGirderName.c_str(), &newNumStraight, &newNumHarped);

         if (!bSuccess && m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
         {
            CString str;
            if (m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
            {
               str.Format(_T("Current number of Permanent strands %d, may not compatible with the girder definition. Number of strands will be adjusted to fit and debonded and extended strand information will be discarded. Would you like to proceed?"), num_perm);
            }
            else
            {
               str.Format(_T("Current number of Permanent strands %d, is not compatible with the girder definition. Number of strands will be adjusted to fit. Would you like to proceed?"), num_perm);
            }
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION);
            if (st == IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            if (!bSuccess)
            {
               num_perm = pStrandGeometry->GetNextNumPermanentStrands(pParent->m_strGirderName.c_str(), num_perm);
               if (num_perm == INVALID_INDEX)
               {
                  num_perm = pStrandGeometry->GetPreviousNumPermanentStrands(pParent->m_strGirderName.c_str(), num_perm);
                  ATLASSERT(num_perm != INVALID_INDEX);
                  if (num_perm == INVALID_INDEX)
                  {
                     num_perm = 0;
                  }
               }
            }

            bSuccess = pStrandGeometry->ComputeNumPermanentStrands( num_perm, pParent->m_strGirderName.c_str(), &newNumStraight, &newNumHarped);
            ATLASSERT(bSuccess);

            if (m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
            {
               // clear debonding and strand extensions
               pParent->m_pSegment->Strands.ClearDebondData();
               pParent->m_pSegment->Strands.ClearExtendedStrands();
            }
         }
         else if (num_straight!=newNumStraight || num_harped!=newNumHarped)
         {
            CString str;
            str.Format(_T("Current values of %d Straight and %d Harped are not compatible with the girder defintion. For %d Total strands you will have %d Straight and %d Harped. Would you like to proceed?"), num_straight, num_harped, num_perm, newNumStraight, newNumHarped);
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
         }
         else if (0 < num_perm && (m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput))
         {
            CString str(_T("The total number of strands will remain the same, however the strands will be placed in pre-defined locations which may be different than the current strand locations. Additionally, debonding and strands extensions will be discarded. Would you like to proceed?"));
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
            // clear debonding and strand extensions
            pParent->m_pSegment->Strands.ClearDebondData();
            pParent->m_pSegment->Strands.ClearExtendedStrands();
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
            str.Format(_T("Current number of Harped strands %d, is not compatible with the girder definition. Number of harped strands will be adjusted to fit. Would you like to proceed?"),num_harped);
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            num_harped = pStrandGeometry->GetNextNumStrands(pParent->m_SegmentKey,pgsTypes::Harped,num_harped);
            if ( num_harped == INVALID_INDEX )
            {
               num_harped = pStrandGeometry->GetPrevNumStrands(pParent->m_SegmentKey,pgsTypes::Harped,num_harped);
               ATLASSERT(num_harped != INVALID_INDEX);
               if ( num_harped == INVALID_INDEX )
               {
                  num_harped = 0;
               }
            }

            num_perm = num_harped + num_straight;
         }

         success = pStrandGeometry->IsValidNumStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight, num_straight);
         if (!success)
         {
            CString str;
            str.Format(_T("Current number of Straight strands %d, is not compatible with the girder definition. Number of Straight strands will be adjusted to fit. Would you like to proceed?"),num_straight);
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }

            num_straight = pStrandGeometry->GetNextNumStrands(pParent->m_strGirderName.c_str(),pgsTypes::Straight,num_straight);
            if ( num_harped == INVALID_INDEX )
            {
               num_straight = pStrandGeometry->GetPrevNumStrands(pParent->m_strGirderName.c_str(),pgsTypes::Straight,num_straight);
               ATLASSERT(num_straight != INVALID_INDEX);
               if ( num_straight == INVALID_INDEX )
               {
                  num_straight = 0;
               }
            }
            num_perm = num_harped + num_straight;
         }

         if  (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection )
         {
            CString str(_T("The total number of strands will remain the same, however the strands will be placed in pre-defined locations which may be different than the current strand locations. Would you like to proceed?"));
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
         }
         else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
         {
            CString str(_T("The total number of strands will remain the same, however the strands will be placed in pre-defined locations which may be different than the current strand locations. Additionally, debonding and strands extensions will be discarded. Would you like to proceed?"));
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION );
            if (st==IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
            // clear debonding and strand extensions
            pParent->m_pSegment->Strands.ClearDebondData();
            pParent->m_pSegment->Strands.ClearExtendedStrands();
         }

         StrandIndexType nStraightStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight);
         CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_SS_SPIN );
         UDACCEL uda;
         uda.nSec=0;
         uda.nInc=1;
         pSpin->SetAccel(1,&uda);
         pSpin->SetRange( 0, (short)nStraightStrandsMax );
         pSpin->SetPos((int)num_straight);

         StrandIndexType nHarpedStrandsMax = pStrandGeometry->GetMaxStrands(pParent->m_strGirderName.c_str(), pgsTypes::Harped);
         pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_NUM_HS_SPIN );
         uda.nSec=0;
         uda.nInc=1;
         pSpin->SetAccel(1,&uda);
         pSpin->SetRange( 0, (short)nHarpedStrandsMax );
         pSpin->SetPos((int)num_harped);

         HideControls(0, newStrandDefinitionType); 
         HideControls(1, newStrandDefinitionType); 
      }
      else if (newStrandDefinitionType == CStrandData::sdtDirectRowInput || newStrandDefinitionType == CStrandData::sdtDirectStrandInput)
      {
         CStrandRowCollection strandRows;
         if (m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
         {
            CString str(_T("Individual strands can't be converted to strand rows. Strands input be discarded. Would you like to proceed?"));
            int st = ::AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION);
            if (st == IDNO)
            {
               // Let user cancel operation
               box->SetCurSel(m_CurrStrandDefinitionType);
               return;
            }
            // clear debonding and strand extensions
            pParent->m_pSegment->Strands.ClearDebondData();
            pParent->m_pSegment->Strands.ClearExtendedStrands();
         }
         else
         {
            GET_IFACE2(pBroker,IPointOfInterest,pPoi);
            PoiList vPoi;
            pPoi->GetPointsOfInterest(pParent->m_SegmentKey, POI_START_FACE | POI_END_FACE, &vPoi);
            ATLASSERT(vPoi.size() == 2);
            const pgsPointOfInterest& startPoi = vPoi.front();
            const pgsPointOfInterest& endPoi   = vPoi.back();

            GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

            LPCTSTR lpszGirderName = pParent->m_Girder.GetGirderName();

            // convert strands
            // Create a prestressing config based on the current input
            PRESTRESSCONFIG config;
            config.AdjustableStrandType = pParent->m_pSegment->Strands.GetAdjustableStrandType();
            config.SetStrandFill(pgsTypes::Straight,pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(),pgsTypes::Straight,num_straight));
            config.SetStrandFill(pgsTypes::Harped,pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(),pgsTypes::Harped,num_harped));
            config.SetStrandFill(pgsTypes::Temporary,pStrandGeometry->ComputeStrandFill(pParent->m_strGirderName.c_str(),pgsTypes::Temporary,num_temp));

            config.EndOffset[pgsTypes::metStart] = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_SegmentKey,pgsTypes::metStart,config.GetStrandFill(pgsTypes::Harped), pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
            config.EndOffset[pgsTypes::metEnd]   = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(pParent->m_SegmentKey,pgsTypes::metStart,config.GetStrandFill(pgsTypes::Harped), pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), pParent->m_pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metEnd));
            config.HpOffset[pgsTypes::metStart]  = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_SegmentKey,pgsTypes::metStart,config.GetStrandFill(pgsTypes::Harped), pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart));
            config.HpOffset[pgsTypes::metEnd]    = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(pParent->m_SegmentKey,pgsTypes::metStart,config.GetStrandFill(pgsTypes::Harped), pParent->m_pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), pParent->m_pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd));

            for ( int i = 0; i < 3; i++ )
            {
               pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

               const ConfigStrandFillVector& vStrandFill(config.GetStrandFill(strandType));
               ConfigStrandFillVector::const_iterator iter(vStrandFill.begin());
               ConfigStrandFillVector::const_iterator end(vStrandFill.end());

               StrandIndexType strIdx1 = 0;
               StrandIndexType strIdx2 = 1;
               for ( ; iter != end; iter++ )
               {
                  GridIndexType gridIdx = std::distance(vStrandFill.begin(),iter);
                  StrandIndexType nStrandsThisRow = *iter;
                  if ( nStrandsThisRow == 0 )
                  {
                     // there is not a strand in this grid position
                     continue;
                  }
                  else if ( nStrandsThisRow == 1 )
                  {
                     strIdx2 = strIdx1;
                  }

                  CStrandRow strandRow;
                  strandRow.m_StrandType = strandType;

                  CComPtr<IPoint2d> pntStart1, pntStart2;
                  pStrandGeometry->GetStrandPositionEx(startPoi,strIdx1,strandType,config,&pntStart1);
                  pStrandGeometry->GetStrandPositionEx(startPoi,strIdx2,strandType,config,&pntStart2);

                  CComPtr<IPoint2d> pntEnd1, pntEnd2;
                  pStrandGeometry->GetStrandPositionEx(endPoi,strIdx1,strandType,config,&pntEnd1);
                  pStrandGeometry->GetStrandPositionEx(endPoi,strIdx2,strandType,config,&pntEnd2);

                  Float64 sx1,sy1, sx2,sy2;
                  pntStart1->Location(&sx1,&sy1);
                  pntStart2->Location(&sx2,&sy2);
                  ATLASSERT(::IsEqual(-sx1,sx2));
                  ATLASSERT(::IsEqual(sy1,sy2));
                  Float64 innerSpacing = sx2 - sx1;

                  Float64 ex1,ey1, ex2,ey2;
                  pntEnd1->Location(&ex1,&ey1);
                  pntEnd2->Location(&ex2,&ey2);
                  ATLASSERT(::IsEqual(-ex1,ex2));
                  ATLASSERT(::IsEqual(innerSpacing,(ex2-ex1)));
                  ATLASSERT(::IsEqual(ey2,ey1));

                  if (newStrandDefinitionType == CStrandData::sdtDirectStrandInput)
                  {
                     strandRow.m_Z = sx1;
                     strandRow.m_nStrands = 1;
                  }
                  else
                  {
                     strandRow.m_Z = innerSpacing;
                     strandRow.m_nStrands = (::IsZero(innerSpacing) ? 1 : 2);
                     ATLASSERT(strandRow.m_nStrands == *iter);
                  }
                  strandRow.m_Spacing = 0;

                  ATLASSERT(::IsEqual(sy1,ey1));
                  ATLASSERT(::IsEqual(sy2,ey2));

                  strandRow.m_Y[ZoneBreakType::Start] = -sy1;
                  strandRow.m_Face[ZoneBreakType::Start] = pgsTypes::TopFace;

                  strandRow.m_Y[ZoneBreakType::End] = -ey1;
                  strandRow.m_Face[ZoneBreakType::End] = pgsTypes::TopFace;

                  for ( int j = 0; j < 2; j++ )
                  {
                     pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)j;
                     strandRow.m_bIsExtendedStrand[endType] = pParent->m_pSegment->Strands.IsExtendedStrand(strandType,gridIdx,endType);

                     Float64 Ldebond;
                     if ( pParent->m_pSegment->Strands.IsDebonded(strandType,gridIdx,endType,&Ldebond) )
                     {
                        strandRow.m_bIsDebonded[endType] = true;
                        strandRow.m_DebondLength[endType] = Ldebond;
                     }
                  }

                  if ( strandType == pgsTypes::Harped )
                  {
                     vPoi.clear(); // recycle the list
                     pPoi->GetPointsOfInterest(pParent->m_SegmentKey,POI_HARPINGPOINT,&vPoi);
                     const pgsPointOfInterest& leftHPPoi(vPoi.front());
                     const pgsPointOfInterest& rightHPPoi(vPoi.back());

                     CComPtr<IPoint2d> pntLeftHP1;
                     pStrandGeometry->GetStrandPositionEx(leftHPPoi,strIdx1,strandType,config,&pntLeftHP1);
                     Float64 y;
                     pntLeftHP1->get_Y(&y);
                     strandRow.m_Y[ZoneBreakType::LeftBreak] = -y;
                     strandRow.m_Face[ZoneBreakType::LeftBreak] = pgsTypes::TopFace;

                     CComPtr<IPoint2d> pntRightHP1;
                     pStrandGeometry->GetStrandPositionEx(rightHPPoi,strIdx1,strandType,config,&pntRightHP1);
                     pntRightHP1->get_Y(&y);
                     strandRow.m_Y[ZoneBreakType::RightBreak] = -y;
                     strandRow.m_Face[ZoneBreakType::RightBreak] = pgsTypes::TopFace;
                  }

                  strandRows.push_back(strandRow);

                  if (newStrandDefinitionType == CStrandData::sdtDirectStrandInput && strIdx1 != strIdx2)
                  {
                     strandRow.m_Z = sx2;
                     strandRows.push_back(strandRow);
                  }


                  if ( nStrandsThisRow == 1 )
                  {
                     strIdx1++;
                     strIdx2 += 2;
                  }
                  else
                  {
                     ATLASSERT(nStrandsThisRow == 2);
                     strIdx1 += 2;
                     strIdx2 += 2;
                  }
               }
            }
         }

         pParent->m_pSegment->Strands.SetStrandRows(strandRows);

         HideControls(0, newStrandDefinitionType);
         HideControls(1, newStrandDefinitionType); 
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
   if ( m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
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

   if (m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
   {
      ShowEndOffsetControls(FALSE);
      ShowHpOffsetControls(FALSE);
   }
   else
   {
      ShowEndOffsetControls(TRUE);
      ShowHpOffsetControls(TRUE);
   }
}


void CGirderDescPrestressPage::ConvertPJackFromNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, IEAFDisplayUnits* pDisplayUnits) 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CButton* pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
   ASSERT( pbut );
   bool calc_permanent   = pbut->GetCheck() == BST_CHECKED;

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
   bool calc_straight = pbut->GetCheck() == BST_CHECKED;

   pbut = (CButton*) GetDlgItem( IDC_HS_JACK );
   ASSERT( pbut );
   bool calc_harped   = pbut->GetCheck() == BST_CHECKED;

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

void CGirderDescPrestressPage::OnEpoxyChanged()
{
   sysFlags<Int32>::Clear(&m_StrandKey,matPsStrand::None);
   sysFlags<Int32>::Clear(&m_StrandKey,matPsStrand::GritEpoxy);
   sysFlags<Int32>::Set(&m_StrandKey,IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? matPsStrand::GritEpoxy : matPsStrand::None);

   UpdateStrandList(IDC_STRAND_SIZE);

}

void CGirderDescPrestressPage::UpdateStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   Int32 cur_key = (Int32)pList->GetItemData( cur_sel );
   // remove the coating flag from the current key
   sysFlags<Int32>::Clear(&cur_key,matPsStrand::None);
   sysFlags<Int32>::Clear(&cur_key,matPsStrand::GritEpoxy);

   BOOL bIsEpoxy = FALSE;
   if ( nIDC == IDC_STRAND_SIZE )
   {
      bIsEpoxy = IsDlgButtonChecked(IDC_EPOXY) == BST_CHECKED ? TRUE : FALSE;
   }
   matPsStrand::Coating coating = (bIsEpoxy ? matPsStrand::GritEpoxy : matPsStrand::None);
   sysFlags<Int32>::Set(&cur_key,coating); // add the coating flag for the strand type we are changing to

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for ( int i = 0; i < 2; i++ )
   {
      matPsStrand::Grade grade = (i == 0 ? matPsStrand::Gr1725 : matPsStrand::Gr1860);
      for ( int j = 0; j < 2; j++ )
      {
         matPsStrand::Type type = (j == 0 ? matPsStrand::LowRelaxation : matPsStrand::StressRelieved);

         lrfdStrandIter iter(grade,type,coating);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const matPsStrand* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );
               
            Int32 key = pPool->GetStrandKey( pStrand );
            pList->SetItemData( idx, key );

            if ( key == cur_key )
            {
               new_cur_sel = sel_count;
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
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND_SIZE );
   int curSel = pList->GetCurSel();
   m_StrandKey = (Int32)pList->GetItemData(curSel);

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight, pPool->GetStrand( m_StrandKey ));
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,   pPool->GetStrand( m_StrandKey ));

   // Now we can update pjack values in dialog
   UpdatePjackEdits();
}

void CGirderDescPrestressPage::OnTempStrandTypeChanged() 
{
   // Very tricky code here - Update the strand material in order to compute new jacking forces
   // Strand material comes out of the strand pool
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_TEMP_STRAND_SIZE );
   int curSel = pList->GetCurSel();
   m_TempStrandKey = (Int32)pList->GetItemData(curSel);

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand( m_TempStrandKey ));

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
      pTTT->hinst = nullptr;
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
   else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput)
   {
      EditDirectRowInput();
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
   {
      EditDirectStrandInput();
   }
}

void CGirderDescPrestressPage::EditDirectSelect()
{
   if (!UpdateData(TRUE))
   {
      return;
   }

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
   Float64 hpOffsetAtStart(0.0), hpOffsetAtHp1(0.0), hpOffsetAtHp2(0.0), hpOffsetAtEnd(0.0);
   HarpedStrandOffsetType endMeasureType(hsoLEGACY), hpMeasureType(hsoLEGACY);
   CDataExchange DXf(this, TRUE);
   if (m_AllowEndAdjustment)
   {
      DDX_UnitValueAndTag( &DXf, IDC_HPOFFSET_START, IDC_HPOFFSET_START_UNIT, hpOffsetAtStart, pDisplayUnits->GetComponentDimUnit() );
      DDX_UnitValueAndTag( &DXf, IDC_HPOFFSET_END,   IDC_HPOFFSET_END_UNIT,   hpOffsetAtEnd,   pDisplayUnits->GetComponentDimUnit() );

      GET_IFACE2(pBroker,IGirder,pGirder);
      if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
      {
         hpOffsetAtEnd = hpOffsetAtStart;
      }

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_END);
      int cursel = box->GetCurSel();
      endMeasureType = (HarpedStrandOffsetType)box->GetItemData(cursel);
   }

   if (m_AllowHpAdjustment)
   {
      DDX_UnitValueAndTag( &DXf, IDC_HPOFFSET_HP1, IDC_HPOFFSET_HP1_UNIT, hpOffsetAtHp1, pDisplayUnits->GetComponentDimUnit() );
      DDX_UnitValueAndTag( &DXf, IDC_HPOFFSET_HP2, IDC_HPOFFSET_HP2_UNIT, hpOffsetAtHp2, pDisplayUnits->GetComponentDimUnit() );

      GET_IFACE2(pBroker,IGirder,pGirder);
      if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
      {
         hpOffsetAtHp2 = hpOffsetAtHp1;
      }

      CComboBox* box = (CComboBox*)GetDlgItem(IDC_HP_COMBO_HP);
      int cursel = box->GetCurSel();
      hpMeasureType = (HarpedStrandOffsetType)box->GetItemData(cursel);
   }

   // Max debond length is 1/2 girder length
   Float64 maxDebondLength = pBridge->GetSegmentLength(pParent->m_SegmentKey)/2.0;

   CGirderSelectStrandsDlg dlg;
   dlg.m_SelectStrandsPage.InitializeData(pParent->m_SegmentKey, &pParent->m_pSegment->Strands, pSpecEntry, pGdrEntry,
                      m_AllowEndAdjustment, m_AllowHpAdjustment, endMeasureType, hpMeasureType, 
                      hpOffsetAtStart, hpOffsetAtHp1, hpOffsetAtHp2, hpOffsetAtEnd, 
                      maxDebondLength,
                      m_HgStart,m_HgHp1,m_HgHp2,m_HgEnd);

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

void CGirderDescPrestressPage::EditDirectRowInput()
{
   if (!UpdateData(TRUE))
   {
      return;
   }

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight,pPool->GetStrand(m_StrandKey));
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,pPool->GetStrand(m_StrandKey));
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary,pPool->GetStrand(m_TempStrandKey));

   CPropertySheet dlg(_T("Define Strand Rows"),this);
   dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   CGirderSegmentStrandsPage page;
   page.Init(pParent->m_pSegment);
   dlg.AddPage(&page);
   if ( dlg.DoModal() == IDOK )
   {
      m_StrandKey     = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight));
      m_TempStrandKey = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary));

      CheckDlgButton(IDC_EPOXY,sysFlags<Int32>::IsSet(m_StrandKey,matPsStrand::GritEpoxy) ? BST_CHECKED : BST_UNCHECKED);

      UpdateStrandList(IDC_STRAND_SIZE);

      UpdateData(FALSE);
      ShowHideNumStrandControls(m_CurrStrandDefinitionType);
      UpdatePjackEdits();
   }
}

void CGirderDescPrestressPage::EditDirectStrandInput()
{
   if (!UpdateData(TRUE))
   {
      return;
   }

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Straight, pPool->GetStrand(m_StrandKey));
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Harped, pPool->GetStrand(m_StrandKey));
   pParent->m_pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary, pPool->GetStrand(m_TempStrandKey));

   CPropertySheet dlg(_T("Define Individual Strands"), this);
   dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   CGirderSegmentStrandsPage page;
   page.Init(pParent->m_pSegment);
   dlg.AddPage(&page);
   if (dlg.DoModal() == IDOK)
   {
      m_StrandKey = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Straight));
      m_TempStrandKey = pPool->GetStrandKey(pParent->m_pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary));

      CheckDlgButton(IDC_EPOXY, sysFlags<Int32>::IsSet(m_StrandKey, matPsStrand::GritEpoxy) ? BST_CHECKED : BST_UNCHECKED);

      UpdateStrandList(IDC_STRAND_SIZE);

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

void CGirderDescPrestressPage::OnCbnSelchangeAdjustableCombo()
{
   // AdjustableStrandType is kept up to date here
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   CComboBox* pAdjList = (CComboBox*)GetDlgItem( IDC_ADJUSTABLE_COMBO );
   pParent->m_pSegment->Strands.SetAdjustableStrandType((pgsTypes::AdjustableStrandType)pAdjList->GetCurSel());

   UpdateAdjustableStrandControls();
}

void CGirderDescPrestressPage::UpdateAdjustableStrandControls()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   pgsTypes::AdjustableStrandType adjType = pParent->m_pSegment->Strands.GetAdjustableStrandType();

   StrandIndexType nh = GetHarpedStrandCount();

   if (adjType == pgsTypes::asHarped)
   {
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Girder Ends:"));

      // harping point controls only for harped design
      m_AllowHpAdjustment = 0.0 <= pStrandGeometry->GetHarpedHpOffsetIncrement(pParent->m_strGirderName.c_str(), adjType);

      ShowHpOffsetControls(m_AllowHpAdjustment ? TRUE : FALSE );

      if (m_AllowHpAdjustment)
      {
         OnSelchangeHpComboHp(); // Update allowable range message
      }
   }
   else
   {
      GetDlgItem(IDC_HPOFFSET_END_TITLE)->SetWindowText(_T("Along Girder:"));
      m_AllowHpAdjustment = false;
      ShowHpOffsetControls(FALSE);
   }

   // End ctrls apply to both harped and straight
   m_AllowEndAdjustment = 0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(pParent->m_strGirderName.c_str(), adjType);

   ShowEndOffsetControls(m_AllowEndAdjustment ? TRUE : FALSE);

   if (m_AllowEndAdjustment)
   {
      DisableEndOffsetControls(m_AllowEndAdjustment && nh>0 ? FALSE : TRUE);
      OnSelchangeHpComboEnd(); // Update allowable range message
   }

   ShowOffsetControlGroup(m_AllowEndAdjustment || m_AllowHpAdjustment);
}

void CGirderDescPrestressPage::ShowOffsetControlGroup(BOOL show)
{
   CWnd* pWnd;

   int sShow = show ? SW_SHOW : SW_HIDE;

   pWnd = GetDlgItem( IDC_VERT_GROUP );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );

   pWnd = GetDlgItem( IDC_VERT_STATIC );
   ASSERT( pWnd );
   pWnd->ShowWindow( sShow );
}

BOOL CGirderDescPrestressPage::OnSetActive()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   UINT sw = SW_SHOW;
   if ( pGirder->IsSymmetricSegment(pParent->m_pSegment->GetSegmentKey()) )
   {
      sw = SW_HIDE;
   }

   GetDlgItem(IDC_HPOFFSET_START_LABEL)->ShowWindow(sw);
   GetDlgItem(IDC_HPOFFSET_END_LABEL)->ShowWindow(sw);
   GetDlgItem(IDC_HPOFFSET_END)->ShowWindow(sw);
   GetDlgItem(IDC_HPOFFSET_END_UNIT)->ShowWindow(sw);
   GetDlgItem(IDC_HPOFFSET_HP2)->ShowWindow(sw);
   GetDlgItem(IDC_HPOFFSET_HP2_UNIT)->ShowWindow(sw);

   return CPropertyPage::OnSetActive();
}

void CGirderDescPrestressPage::GetStrandCount(StrandIndexType* pNs, StrandIndexType* pNh, StrandIndexType* pNt,StrandIndexType* pNp)
{
   if (m_CurrStrandDefinitionType == CStrandData::sdtStraightHarped)
   {
      *pNs = GetDlgItemInt(IDC_NUM_SS);
      *pNh = GetDlgItemInt(IDC_NUM_HS);
      *pNt = GetDlgItemInt(IDC_NUM_TEMP);
      *pNp = *pNs + *pNh;
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtTotal)
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);
      *pNp = GetDlgItemInt(IDC_NUM_HS);
      pStrandGeometry->ComputeNumPermanentStrands(*pNp, pParent->m_strGirderName.c_str(), pNs, pNh);
      *pNt = GetDlgItemInt(IDC_NUM_TEMP);
   }
   else if (m_CurrStrandDefinitionType == CStrandData::sdtDirectSelection || 
            m_CurrStrandDefinitionType == CStrandData::sdtDirectRowInput || 
            m_CurrStrandDefinitionType == CStrandData::sdtDirectStrandInput)
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
      *pNs = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
      *pNh = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
      *pNt = pParent->m_pSegment->Strands.GetStrandCount(pgsTypes::Temporary);
      *pNp = *pNs + *pNh;
   }
   else
   {
      ATLASSERT(false); // new strand definition type?
   }
}
