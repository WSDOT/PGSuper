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

// BridgeDescDeckDetailsPagePage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"

#include "PGSpliceDoc.h"
#include "PGSuperDoc.h"
#include "BridgeDescDeckDetailsPage.h"
#include "BridgeDescDlg.h"
#include <PgsExt\ConcreteDetailsDlg.h>
#include <PgsExt\Helpers.h>

#include "PGSuperAppPlugin\TimelineEventDlg.h"
#include "PGSuperAppPlugin\EditHaunchDlg.h"
#include "Hints.h"

#include <System\Flags.h>

#include <WBFLGenericBridge.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#include <EAF\EAFMainFrame.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// template function to directly set item data in a combo box
template <class T>
bool SetCBItemData( CWnd* pWnd, int nIDC, T itemdata )
{
   CComboBox* pCB = (CComboBox*)pWnd->GetDlgItem(nIDC);

   bool succ=false;
   int count = pCB->GetCount();
   for ( int i = 0; i < count; i++ )
   {
      if ( ((T)pCB->GetItemData(i)) == itemdata )
      {
         pCB->SetCurSel(i);
         succ=true;
         break;
      }
   }

   return succ;
}


/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckDetailsPage property page

IMPLEMENT_DYNCREATE(CBridgeDescDeckDetailsPage, CPropertyPage)

CBridgeDescDeckDetailsPage::CBridgeDescDeckDetailsPage() : CPropertyPage(CBridgeDescDeckDetailsPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescDeckDetailsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_AgeAtContinuity = 0;
}

CBridgeDescDeckDetailsPage::~CBridgeDescDeckDetailsPage()
{
}

void CBridgeDescDeckDetailsPage::DoDataExchange(CDataExchange* pDX)
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   bool doAssExcessCamber = m_bCanAssExcessCamberInputBeEnabled && deckType != pgsTypes::sdtNone;
	
   CPropertyPage::DoDataExchange(pDX);

   //{{AFX_DATA_MAP(CBridgeDescDeckDetailsPage)
   // NOTE: the ClassWizard will add DDX and DDV calls here
   DDX_Control(pDX, IDC_EC, m_ctrlEc);
   DDX_Control(pDX, IDC_MOD_E, m_ctrlEcCheck);
   DDX_Control(pDX, IDC_SLAB_FC, m_ctrlFc);
   DDX_Control(pDX, IDC_HAUNCH_SHAPE2, m_cbHaunchShape);
   //}}AFX_DATA_MAP
   DDX_Control(pDX, IDC_OVERHANG_DEPTH, m_ctrlOverhangEdgeDepth);
   DDX_Control(pDX, IDC_OVERHANG_TAPER, m_ctrlOverhangTaper);
   DDX_Control(pDX, IDC_PANEL_DEPTH, m_ctrlPanelDepth);
   DDX_Control(pDX, IDC_PANEL_SUPPORT, m_ctrlPanelSupportWidth);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
	
   CEAFDocument* pDoc = EAFGetDocument();
   
   // Slab offset data
   if (!pDX->m_bSaveAndValidate)
   {
      m_Fillet = pParent->m_BridgeDesc.GetFillet();

      m_SlabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
      m_SlabOffset = pParent->m_BridgeDesc.GetSlabOffset();
      m_strSlabOffsetCache.Format(_T("%s"),FormatDimension(m_SlabOffset,pDisplayUnits->GetComponentDimUnit(), false));

      m_AssExcessCamberType = doAssExcessCamber ? pParent->m_BridgeDesc.GetAssExcessCamberType() : pgsTypes::aecBridge;
      m_AssExcessCamber = pParent->m_BridgeDesc.GetAssExcessCamber();
      m_strAssExcessCamberCache.Format(_T("%s"),FormatDimension(m_AssExcessCamber,pDisplayUnits->GetComponentDimUnit(), false));
   }

   DDX_CBItemData(pDX, IDC_HAUNCH_SHAPE2,  pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape);

   // make sure unit tags are always displayed (even if disabled)
   DDX_Tag( pDX, IDC_GROSS_DEPTH_UNIT,    pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_FILLET_UNIT,         pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_ASSEXCESSCAMBER_UNIT,pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_ADIM_UNIT,           pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_OVERHANG_DEPTH_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_PANEL_DEPTH_UNIT,    pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_PANEL_SUPPORT_UNIT,  pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_SACDEPTH_UNIT,       pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_SLAB_FC_UNIT,        pDisplayUnits->GetStressUnit() );
   DDX_Tag( pDX, IDC_EC_UNIT,             pDisplayUnits->GetModEUnit() );

   if ( deckType != pgsTypes::sdtNone )
   {
      // gross or cast depth
      DDX_UnitValueAndTag( pDX, IDC_GROSS_DEPTH,   IDC_GROSS_DEPTH_UNIT,  pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueGreaterThanZero( pDX, IDC_GROSS_DEPTH,pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth, pDisplayUnits->GetComponentDimUnit() );

      // fillet
      DDX_UnitValueAndTag( pDX, IDC_FILLET, IDC_FILLET_UNIT, m_Fillet, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore( pDX, IDC_FILLET, m_Fillet, pDisplayUnits->GetComponentDimUnit() );

      if (pDX->m_bSaveAndValidate)
      {
         pParent->m_BridgeDesc.SetFillet(m_Fillet);
      }

      // AssExcessCamber
      if (!pDX->m_bSaveAndValidate)
      {
         m_AssExcessCamber = pParent->m_BridgeDesc.GetAssExcessCamber();
      }

      // AssExcessCamber
      DDX_CBItemData(pDX, IDC_ASSEXCESSCAMBERTYPE, m_AssExcessCamberType);
      if ( doAssExcessCamber)
      {
         DDX_UnitValueAndTag( pDX, IDC_ASSEXCESSCAMBER, IDC_ASSEXCESSCAMBER_UNIT, m_AssExcessCamber, pDisplayUnits->GetComponentDimUnit() );
         DDV_UnitValueZeroOrMore( pDX, IDC_ASSEXCESSCAMBER, m_AssExcessCamber, pDisplayUnits->GetComponentDimUnit() );
      }

      // slab offset
      DDX_CBItemData(pDX, IDC_SAMESLABOFFSET, m_SlabOffsetType);
      if ( m_SlabOffsetType==pgsTypes::sotBridge)
      {
         DDX_UnitValueAndTag( pDX, IDC_ADIM, IDC_ADIM_UNIT, m_SlabOffset, pDisplayUnits->GetComponentDimUnit() );
      }

      // overhang
      DDX_UnitValueAndTag( pDX, IDC_OVERHANG_DEPTH, IDC_OVERHANG_DEPTH_UNIT, pParent->m_BridgeDesc.GetDeckDescription()->OverhangEdgeDepth, pDisplayUnits->GetComponentDimUnit() );
      DDX_CBItemData( pDX, IDC_OVERHANG_TAPER, pParent->m_BridgeDesc.GetDeckDescription()->OverhangTaper );

      // deck panel
      DDX_UnitValueAndTag( pDX, IDC_PANEL_DEPTH, IDC_PANEL_DEPTH_UNIT,  pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth, pDisplayUnits->GetComponentDimUnit() );
      if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeSIP ) // SIP
      {
         DDV_UnitValueGreaterThanZero( pDX, IDC_PANEL_DEPTH, pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth, pDisplayUnits->GetXSectionDimUnit() );
      }

      DDX_UnitValueAndTag( pDX, IDC_PANEL_SUPPORT, IDC_PANEL_SUPPORT_UNIT,  pParent->m_BridgeDesc.GetDeckDescription()->PanelSupport, pDisplayUnits->GetComponentDimUnit() );

      // slab material
      ExchangeConcreteData(pDX);
   }

   // sacrificial depth
   DDX_UnitValueAndTag( pDX, IDC_SACDEPTH,      IDC_SACDEPTH_UNIT,     pParent->m_BridgeDesc.GetDeckDescription()->SacrificialDepth, pDisplayUnits->GetComponentDimUnit() );
   if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone )
   {
      if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeSIP ) // SIP
      {
         DDV_UnitValueLessThanLimit(pDX, IDC_SACDEPTH,pParent->m_BridgeDesc.GetDeckDescription()->SacrificialDepth, pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth, pDisplayUnits->GetComponentDimUnit(), _T("Please enter a sacrificial depth that is less than %f %s") );
      }
      else // all others
      {
         DDV_UnitValueLessThanLimit(pDX, IDC_SACDEPTH,pParent->m_BridgeDesc.GetDeckDescription()->SacrificialDepth, pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth, pDisplayUnits->GetComponentDimUnit(), _T("Please enter a sacrificial depth that is less than %f %s") );
      }
   }

   // wearing surface
   DDX_CBItemData( pDX, IDC_WEARINGSURFACETYPE, pParent->m_BridgeDesc.GetDeckDescription()->WearingSurface );

   EventIndexType overlayEventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetOverlayLoadEventIndex();
   if ( overlayEventIdx != INVALID_INDEX )
   {
      DDX_CBItemData(pDX,IDC_OVERLAY_EVENT,overlayEventIdx);
   }
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) && pDX->m_bSaveAndValidate )
   {
      // saving data - dialog is closing

      if ( pParent->m_BridgeDesc.GetDeckDescription()->WearingSurface != pgsTypes::wstSacrificialDepth )
      {
         // there is an overlay
         if ( overlayEventIdx == INVALID_INDEX ) // the choice is invalid... alert the user
         {
            pDX->PrepareCtrl(IDC_OVERLAY_EVENT);
            AfxMessageBox(_T("Select the event when the overlay is installed."));
            pDX->Fail();
         }

         EventIndexType railingSystemEventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetRailingSystemLoadEventIndex();
         if ( overlayEventIdx < railingSystemEventIdx )
         {
            pDX->PrepareCtrl(IDC_OVERLAY_EVENT);
            AfxMessageBox(_T("The overlay cannot be installed prior to the railing system."));
            pDX->Fail();
         }
      }
      else
      {
         // there is not an overlay... remove the overlay activity from the timeline
         pParent->m_BridgeDesc.GetTimelineManager()->RemoveOverlayLoadEvent();
      }
   }

   int bInputAsDepthAndDensity;
   if ( !pDX->m_bSaveAndValidate )
   {
      bInputAsDepthAndDensity = (pParent->m_BridgeDesc.GetDeckDescription()->bInputAsDepthAndDensity ? 1 : 0);
   }
   
   DDX_Radio(pDX, IDC_OLAY_WEIGHT_LABEL, bInputAsDepthAndDensity );

   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.GetDeckDescription()->bInputAsDepthAndDensity = (bInputAsDepthAndDensity == 1 ? true : false);
   }
   
   DDX_UnitValueAndTag( pDX, IDC_OLAY_WEIGHT,   IDC_OLAY_WEIGHT_UNIT,  pParent->m_BridgeDesc.GetDeckDescription()->OverlayWeight, pDisplayUnits->GetOverlayWeightUnit() );

   DDX_UnitValueAndTag( pDX, IDC_OLAY_DEPTH,    IDC_OLAY_DEPTH_UNIT,   pParent->m_BridgeDesc.GetDeckDescription()->OverlayDepth, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag( pDX, IDC_OLAY_DENSITY,  IDC_OLAY_DENSITY_UNIT, pParent->m_BridgeDesc.GetDeckDescription()->OverlayDensity, pDisplayUnits->GetDensityUnit() );

   if ( pDX->m_bSaveAndValidate )
   {
      if ( pParent->m_BridgeDesc.GetDeckDescription()->WearingSurface == pgsTypes::wstSacrificialDepth )
      {
         DDV_UnitValueZeroOrMore( pDX, IDC_SACDEPTH,pParent->m_BridgeDesc.GetDeckDescription()->SacrificialDepth, pDisplayUnits->GetComponentDimUnit() );
         if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone )
         {
            DDV_UnitValueLessThanLimit( pDX, IDC_SACDEPTH,pParent->m_BridgeDesc.GetDeckDescription()->SacrificialDepth, pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth, pDisplayUnits->GetComponentDimUnit() );
         }
      }
      else
      {
         if ( pParent->m_BridgeDesc.GetDeckDescription()->bInputAsDepthAndDensity )
         {
            DDV_UnitValueZeroOrMore( pDX, IDC_OLAY_DEPTH,pParent->m_BridgeDesc.GetDeckDescription()->OverlayDepth, pDisplayUnits->GetComponentDimUnit() );
            DDV_UnitValueZeroOrMore( pDX, IDC_OLAY_DENSITY,pParent->m_BridgeDesc.GetDeckDescription()->OverlayDensity, pDisplayUnits->GetDensityUnit());

            Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
            pParent->m_BridgeDesc.GetDeckDescription()->OverlayWeight = pParent->m_BridgeDesc.GetDeckDescription()->OverlayDepth * pParent->m_BridgeDesc.GetDeckDescription()->OverlayDensity * g;
         }
         else
         {
            DDV_UnitValueZeroOrMore( pDX, IDC_OLAY_WEIGHT,pParent->m_BridgeDesc.GetDeckDescription()->OverlayWeight, pDisplayUnits->GetOverlayWeightUnit() );
         }
      }
   }
      
   if ( pDX->m_bSaveAndValidate && deckType != pgsTypes::sdtNone )
   {
      // Validate slab depth
      if ( m_SlabOffsetType==pgsTypes::sotBridge )
      {
         // Slab offset applies to the entire bridge... have users adjust Slab offset if it doesn't
         // fit with the slab depth

         DDV_UnitValueLimitOrMore(pDX, IDC_ADIM, m_SlabOffset, pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetFillet(), pDisplayUnits->GetComponentDimUnit());
         pParent->m_BridgeDesc.SetSlabOffset(m_SlabOffset);
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotBridge);

         Float64 grossDepth;
         bool bCheckDepth = false;
         CString strMsg;

         if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeCIP || // CIP deck or overlay
              IsOverlayDeck(pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType())) 
         {
            grossDepth = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetFillet();
            strMsg = _T("Slab Offset must be larger than the gross slab depth + fillet");
            bCheckDepth = true;
         }
         else if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeSIP ) // SIP
         {
            grossDepth = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetFillet() + pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth;
            strMsg = _T("Slab Offset must be larger than the cast depth + fillet + panel depth");
            bCheckDepth = true;
         }
         else
         {
            ATLASSERT(false);
            // should not get here
         }

         if ( bCheckDepth && m_SlabOffset < grossDepth )
         {
            AfxMessageBox(strMsg);
            pDX->PrepareEditCtrl(IDC_ADIM);
            pDX->Fail();
         }
      }
      else
      {
         // Slab offset is span-by-span or girder-by-girder. Have user adjust the slab depth if it doesn't
         // fit with the current values for slab offset.
         Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();

         if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeCIP || 
              IsOverlayDeck(pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType()))
         {
            pDX->PrepareEditCtrl(IDC_GROSS_DEPTH);
            Float64 maxGrossDepth = minSlabOffset;
            Float64 grossDepth = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth;
            if ( ::IsLT(maxGrossDepth,grossDepth) )
            {
               CString msg;
               msg.Format(_T("Gross slab depth must less than %s to accomodate minimum slab offset of %s"),
                            FormatDimension(maxGrossDepth,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(minSlabOffset,pDisplayUnits->GetComponentDimUnit()));
               AfxMessageBox(msg,MB_ICONEXCLAMATION);
               pDX->Fail();
            }
         }
         else if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeSIP )
         {
            pDX->PrepareEditCtrl(IDC_PANEL_DEPTH);
            Float64 maxCastDepth = minSlabOffset - pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth;
            Float64 castDepth = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth;
            if ( ::IsLT(maxCastDepth,castDepth) )
            {
               CString msg;
               msg.Format(_T("Cast slab depth must less than %s to accomodate the %s panel and minimum slab offset of %s"),
                            FormatDimension(maxCastDepth,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(minSlabOffset,pDisplayUnits->GetComponentDimUnit()));
               AfxMessageBox(msg,MB_ICONEXCLAMATION);
               pDX->Fail();
            }
         }
         else
         {
            ATLASSERT(false);
            // should not get here
         }
      }

      // Set AssExcessCamber
      if ( doAssExcessCamber && m_AssExcessCamberType==pgsTypes::aecBridge)
      {
         pParent->m_BridgeDesc.SetAssExcessCamberType(pgsTypes::aecBridge);
         pParent->m_BridgeDesc.SetAssExcessCamber(m_AssExcessCamber);
      }
   }

   if ( pDX->m_bSaveAndValidate && (deckType== pgsTypes::sdtCompositeCIP ||  deckType== pgsTypes::sdtCompositeSIP) )
   {
      DDV_DeckPointGrid(pDX,IDC_GRID,&m_Grid);
      pParent->m_BridgeDesc.GetDeckDescription()->DeckEdgePoints = m_Grid.GetEdgePoints();
   }
   else
   {
      m_Grid.FillGrid(pParent->m_BridgeDesc.GetDeckDescription());
   }

   DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, pParent->m_BridgeDesc.GetDeckDescription()->Condition);
   DDX_Text(pDX,   IDC_CONDITION_FACTOR,      pParent->m_BridgeDesc.GetDeckDescription()->ConditionFactor);

   EventIndexType deckEventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastDeckEventIndex();
   DDX_CBItemData(pDX,IDC_DECK_EVENT,deckEventIdx);
}

void CBridgeDescDeckDetailsPage::ExchangeConcreteData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   DDX_UnitValueAndTag( pDX, IDC_SLAB_FC, IDC_SLAB_FC_UNIT, pParent->m_BridgeDesc.GetDeckDescription()->Concrete.Fc, pDisplayUnits->GetStressUnit() );

   DDX_Check_Bool(pDX,IDC_MOD_E, pParent->m_BridgeDesc.GetDeckDescription()->Concrete.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_EC,  IDC_EC_UNIT, pParent->m_BridgeDesc.GetDeckDescription()->Concrete.Ec, pDisplayUnits->GetModEUnit() );


   if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone && pDX->m_bSaveAndValidate )
   {
      DDV_UnitValueGreaterThanZero( pDX, IDC_SLAB_FC, pParent->m_BridgeDesc.GetDeckDescription()->Concrete.Fc, pDisplayUnits->GetStressUnit() );
      DDV_UnitValueGreaterThanZero( pDX, IDC_EC,      pParent->m_BridgeDesc.GetDeckDescription()->Concrete.Ec, pDisplayUnits->GetModEUnit() );
   }

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
}

BEGIN_MESSAGE_MAP(CBridgeDescDeckDetailsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescDeckDetailsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnUserEc)
	ON_EN_CHANGE(IDC_SLAB_FC, OnChangeSlabFc)
   ON_CBN_SELCHANGE(IDC_WEARINGSURFACETYPE,OnWearingSurfaceTypeChanged)
	ON_BN_CLICKED(IDC_MORE, OnMoreConcreteProperties)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_BN_CLICKED(IDC_ADD,OnAddDeckEdgePoint)
   ON_BN_CLICKED(IDC_REMOVE,OnRemoveDeckEdgePoint)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_OLAY_WEIGHT_LABEL, &CBridgeDescDeckDetailsPage::OnBnClickedOlayWeightLabel)
   ON_BN_CLICKED(IDC_OLAY_DEPTH_LABEL, &CBridgeDescDeckDetailsPage::OnBnClickedOlayDepthLabel)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, &CBridgeDescDeckDetailsPage::OnConditionFactorTypeChanged)
   ON_CBN_SELCHANGE(IDC_DECK_EVENT, OnDeckEventChanged)
   ON_CBN_DROPDOWN(IDC_DECK_EVENT, OnDeckEventChanging)
   ON_CBN_SELCHANGE(IDC_OVERLAY_EVENT, OnOverlayEventChanged)
   ON_CBN_DROPDOWN(IDC_OVERLAY_EVENT, OnOverlayEventChanging)
   ON_BN_CLICKED(IDC_EDIT_HAUNCH_BUTTON, &CBridgeDescDeckDetailsPage::OnBnClickedEditHaunchButton)
   ON_BN_CLICKED(IDC_EDIT_HAUNCH_BUTTON2, &CBridgeDescDeckDetailsPage::OnBnClickedEditHaunchButton)
   ON_CBN_SELCHANGE(IDC_SAMESLABOFFSET, &CBridgeDescDeckDetailsPage::OnCbnSelchangeSameslaboffset)
   ON_CBN_SELCHANGE(IDC_HAUNCH_SHAPE2, &CBridgeDescDeckDetailsPage::OnCbnSelchangeHaunchShape2)
   ON_CBN_SELCHANGE(IDC_ASSEXCESSCAMBERTYPE, &CBridgeDescDeckDetailsPage::OnCbnSelchangeAssExcessCamberType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckDetailsPage message handlers

BOOL CBridgeDescDeckDetailsPage::OnInitDialog() 
{
   m_Grid.SubclassDlgItem(IDC_GRID, this);
   m_Grid.CustomInit();

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CEAFDocument* pDoc = EAFGetDocument();

   FillEventList();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_DECK_EVENT)->EnableWindow(FALSE);
      GetDlgItem(IDC_OVERLAY_EVENT)->EnableWindow(FALSE);
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if ( pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
   {
      const CTimelineManager* pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
      EventIndexType eventIdx = pTimelineMgr->GetCastDeckEventIndex();
      m_AgeAtContinuity = pTimelineMgr->GetEventByIndex(eventIdx)->GetCastDeckActivity().GetConcreteAgeAtContinuity();
   }



   // set density/weight labels
   CStatic* pStatic = (CStatic*)GetDlgItem( IDC_OLAY_DENSITY_LABEL );
   if ( IS_SI_UNITS(pDisplayUnits) )
   {
      pStatic->SetWindowText( _T("Overlay Density") );
   }
   else
   {
      pStatic->SetWindowText( _T("Overlay Unit Wgt") );
   }

   // fill up slab overhang taper options
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_OVERHANG_TAPER);
   int idx = pCB->AddString(_T("Taper overhang to top of top flange"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::dotTopTopFlange);

   idx = pCB->AddString(_T("Taper overhang to bottom of top flange"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::dotBottomTopFlange);

   idx = pCB->AddString(_T("Constant depth overhang"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::dotNone);

   // wearing surface types
   pCB = (CComboBox*)GetDlgItem(IDC_WEARINGSURFACETYPE);

   idx = pCB->AddString(_T("Sacrificial Depth"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::wstSacrificialDepth);

   idx = pCB->AddString(_T("Overlay"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::wstOverlay);

   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      idx = pCB->AddString(_T("Future Overlay"));
      pCB->SetItemData(idx,(DWORD)pgsTypes::wstFutureOverlay);
   }

   // Slab offset type combo
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_SAMESLABOFFSET);
   int sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotBridge);
   sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotPier));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotPier);
   sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotGirder);

   // See if there is any chance we'll need to input excess camber
   GET_IFACE2(pBroker,ISpecification, pSpec );
   m_bCanAssExcessCamberInputBeEnabled = pSpec->IsAssExcessCamberInputEnabled(false);

   // AssExcessCamber type combo
   pBox =(CComboBox*)GetDlgItem(IDC_ASSEXCESSCAMBERTYPE);
   sqidx = pBox->AddString( AssExcessCamberTypeAsString(pgsTypes::aecBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecBridge);
   sqidx = pBox->AddString( AssExcessCamberTypeAsString(pgsTypes::aecSpan));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecSpan);
   sqidx = pBox->AddString( AssExcessCamberTypeAsString(pgsTypes::aecGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecGirder);

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   // If girder spacing is adjacent, force haunch shape to square
   if ( IsAdjacentSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) )
   {
      pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape = pgsTypes::hsSquare;
   }

   CPropertyPage::OnInitDialog();

   m_ctrlOverhangEdgeDepth.ShowDefaultWhenDisabled(FALSE);
   m_ctrlPanelDepth.ShowDefaultWhenDisabled(FALSE);
   m_ctrlPanelSupportWidth.ShowDefaultWhenDisabled(FALSE);

   if ( m_strUserEc == _T("") )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
	
   OnWearingSurfaceTypeChanged();
   UpdateSlabOffsetControls();
   UpdateConcreteControls();

   EnableToolTips(TRUE);

   EnableRemove(FALSE);
	
   OnConditionFactorTypeChanged();

   m_cbHaunchShape.Initialize(pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_HAUNCH_SHAPE2,pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescDeckDetailsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_DECKDETAILS );
}

BOOL CBridgeDescDeckDetailsPage::OnSetActive() 
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   CString strDeckType = GetDeckTypeName(deckType);
   GetDlgItem(IDC_DECK_TYPE)->SetWindowText(strDeckType);


   CWnd* pWnd = GetDlgItem(IDC_GROSS_DEPTH_LABEL);
   pWnd->SetWindowText(deckType == pgsTypes::sdtCompositeCIP || IsOverlayDeck(deckType) ? _T("Gross Depth") : _T("Cast Depth"));

   GetDlgItem(IDC_GROSS_DEPTH_LABEL)->EnableWindow( deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_GROSS_DEPTH)->EnableWindow(       deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_GROSS_DEPTH_UNIT)->EnableWindow(  deckType != pgsTypes::sdtNone);

   GetDlgItem(IDC_OVERHANG_DEPTH_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   m_ctrlOverhangEdgeDepth.EnableWindow(       deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_OVERHANG_DEPTH_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   m_ctrlOverhangTaper.EnableWindow(       deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);


   UpdateDeckRelatedControls();
   UpdateSlabOffsetControls();
   UpdateAssExcessCamberControls();

   BOOL bEnableSlabConcrete = (deckType == pgsTypes::sdtNone /*|| deckType == pgsTypes::sdtNonstructuralOverlay*/ ? FALSE : TRUE);
   // need deck concrete properties for Nonstructural Overlay... need the density of material for dead load
   GetDlgItem(IDC_SLAB_FC_LABEL)->EnableWindow(bEnableSlabConcrete);
   GetDlgItem(IDC_SLAB_FC)->EnableWindow(bEnableSlabConcrete);
   GetDlgItem(IDC_SLAB_FC_UNIT)->EnableWindow(bEnableSlabConcrete);
   GetDlgItem(IDC_MORE)->EnableWindow(bEnableSlabConcrete);
   
   GetDlgItem(IDC_ADD)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );
   GetDlgItem(IDC_REMOVE)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );
   m_Grid.Enable( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );

   GetDlgItem(IDC_SACDEPTH_LABEL)->EnableWindow( deckType != pgsTypes::sdtNone );
   GetDlgItem(IDC_SACDEPTH)->EnableWindow( deckType != pgsTypes::sdtNone );
   GetDlgItem(IDC_SACDEPTH_UNIT)->EnableWindow( deckType != pgsTypes::sdtNone );

   GetDlgItem(IDC_SIP_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeSIP);

   GetDlgItem(IDC_PANEL_DEPTH_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeSIP);
   m_ctrlPanelDepth.EnableWindow(       deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_PANEL_DEPTH_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeSIP);

   GetDlgItem(IDC_PANEL_SUPPORT_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeSIP);
   m_ctrlPanelSupportWidth.EnableWindow(       deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_PANEL_SUPPORT_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeSIP);

   BOOL bEnableConditionFactor = (deckType == pgsTypes::sdtNone || deckType == pgsTypes::sdtNonstructuralOverlay ? FALSE : TRUE); // if no deck, or overlay is not structural, there isn't a condition state
   GetDlgItem(IDC_CONDITION_FACTOR_TYPE)->EnableWindow(bEnableConditionFactor);
   GetDlgItem(IDC_CONDITION_FACTOR)->EnableWindow(bEnableConditionFactor);

   OnUserEc();
   OnWearingSurfaceTypeChanged();

   // blank out deck related inputs
   if ( deckType == pgsTypes::sdtNone )
   {
      GetDlgItem(IDC_GROSS_DEPTH)->SetWindowText(_T(""));
      GetDlgItem(IDC_OVERHANG_DEPTH)->SetWindowText(_T(""));
      GetDlgItem(IDC_FILLET)->SetWindowText(_T(""));
      GetDlgItem(IDC_ADIM)->SetWindowText(_T(""));
      GetDlgItem(IDC_PANEL_DEPTH)->SetWindowText(_T(""));
      GetDlgItem(IDC_PANEL_SUPPORT)->SetWindowText(_T(""));
      GetDlgItem(IDC_SLAB_FC)->SetWindowText(_T(""));
      GetDlgItem(IDC_EC)->SetWindowText(_T(""));
      GetDlgItem(IDC_OVERHANG_TAPER)->SetWindowText(_T("")); // this is a combobox
   }

   if (!m_bCanAssExcessCamberInputBeEnabled || deckType == pgsTypes::sdtNone)
   {
      GetDlgItem(IDC_ASSEXCESSCAMBER)->SetWindowText(_T(""));
   }


   return CPropertyPage::OnSetActive();
}

void CBridgeDescDeckDetailsPage::OnUserEc()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   BOOL bEnable = m_ctrlEcCheck.GetCheck();

   // can't be enabled if there is no deck
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   if ( deckType == pgsTypes::sdtNone )
   {
      bEnable = FALSE;
      GetDlgItem(IDC_MOD_E)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_MOD_E)->EnableWindow(TRUE);

      if (bEnable==FALSE)
      {
         m_ctrlEc.GetWindowText(m_strUserEc);
         UpdateEc();
      }
      else
      {
         m_ctrlEc.SetWindowText(m_strUserEc);
      }
   }

   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}

void CBridgeDescDeckDetailsPage::OnChangeSlabFc() 
{
   UpdateEc();
}

void CBridgeDescDeckDetailsPage::UpdateEc()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   // update modulus
   if (m_ctrlEcCheck.GetCheck() == 0)
   {
      // blank out ec
      CString strEc;
      m_ctrlEc.SetWindowText(strEc);

      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_BridgeDesc.GetDeckDescription()->Concrete.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_BridgeDesc.GetDeckDescription()->Concrete.EcK1);
      strK2.Format(_T("%f"),pParent->m_BridgeDesc.GetDeckDescription()->Concrete.EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CBridgeDescDeckDetailsPage::OnWearingSurfaceTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_WEARINGSURFACETYPE);
   int idx = pCB->GetCurSel();

   BOOL bSacDepth;
   BOOL bOverlayEvent;
   BOOL bOverlayLabel;
   BOOL bOverlayWeight;
   BOOL bOverlayDepthAndDensity;

   int iOption = GetCheckedRadioButton(IDC_OLAY_WEIGHT_LABEL,IDC_OLAY_DEPTH_LABEL);
   ATLASSERT(iOption != 0); // 0 means nothing is selected

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   pgsTypes::LossMethod lossMethod = pLossParams->GetLossMethod();

   CTimelineManager* pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();

   pgsTypes::WearingSurfaceType ws = (pgsTypes::WearingSurfaceType)(pCB->GetItemData(idx));
   if ( ws == pgsTypes::wstSacrificialDepth )
   {
      bSacDepth               = TRUE;
      bOverlayEvent           = FALSE;
      bOverlayLabel           = FALSE;
      bOverlayWeight          = FALSE;
      bOverlayDepthAndDensity = FALSE;

      // no overlay so remove the overlay loading event from the time line
      pTimelineMgr->SetOverlayLoadEventByIndex(INVALID_INDEX);
   }
   else if ( ws == pgsTypes::wstFutureOverlay )
   {
      ATLASSERT(lossMethod != pgsTypes::TIME_STEP); // future overlay only used for PGSuper... should not get here if PGSplice
      bSacDepth               = TRUE;
      bOverlayEvent           = FALSE;
      bOverlayLabel           = TRUE;
      bOverlayWeight          = (iOption == IDC_OLAY_WEIGHT_LABEL ? TRUE : FALSE);
      bOverlayDepthAndDensity = (iOption == IDC_OLAY_DEPTH_LABEL  ? TRUE : FALSE);

      // create a loading event in the timeline for the overlay load
      pTimelineMgr->SetOverlayLoadEventByIndex(pTimelineMgr->GetLiveLoadEventIndex()-1);
   }
   else
   {
      bSacDepth               = (lossMethod == pgsTypes::TIME_STEP ? TRUE : FALSE); // For time step analysis sacrifical depth applies until overlay is applied. For other loss methods, sacrifical depth doesn't apply for overlays
      bOverlayEvent           = (lossMethod == pgsTypes::TIME_STEP ? TRUE : FALSE);
      bOverlayLabel           = TRUE;
      bOverlayWeight          = (iOption == IDC_OLAY_WEIGHT_LABEL ? TRUE : FALSE);
      bOverlayDepthAndDensity = (iOption == IDC_OLAY_DEPTH_LABEL  ? TRUE : FALSE);

      if ( lossMethod != pgsTypes::TIME_STEP )
      {
         // create a loading event in the timeline for the overlay load... only do this for non-timestep analysis
         // The user has to explicitly select the event for the overlay load event for time-step analysis
         pTimelineMgr->SetOverlayLoadEventByIndex(pTimelineMgr->GetRailingSystemLoadEventIndex());
      }
   }

   GetDlgItem(IDC_OVERLAY_EVENT)->EnableWindow( bOverlayEvent );
   GetDlgItem(IDC_OVERLAY_EVENT)->ShowWindow(ws == pgsTypes::wstSacrificialDepth ? SW_HIDE : SW_SHOW);

   GetDlgItem(IDC_OLAY_WEIGHT_LABEL)->EnableWindow( bOverlayLabel );
   GetDlgItem(IDC_OLAY_WEIGHT)->EnableWindow( bOverlayWeight );
   GetDlgItem(IDC_OLAY_WEIGHT_UNIT)->EnableWindow( bOverlayWeight );

   GetDlgItem(IDC_OLAY_DEPTH_LABEL)->EnableWindow( bOverlayLabel );
   GetDlgItem(IDC_OLAY_DEPTH)->EnableWindow( bOverlayDepthAndDensity );
   GetDlgItem(IDC_OLAY_DEPTH_UNIT)->EnableWindow( bOverlayDepthAndDensity );

   GetDlgItem(IDC_OLAY_DENSITY_LABEL)->EnableWindow( bOverlayDepthAndDensity );
   GetDlgItem(IDC_OLAY_DENSITY)->EnableWindow( bOverlayDepthAndDensity );
   GetDlgItem(IDC_OLAY_DENSITY_UNIT)->EnableWindow( bOverlayDepthAndDensity );

   GetDlgItem(IDC_SACDEPTH_LABEL)->EnableWindow( bSacDepth );
   GetDlgItem(IDC_SACDEPTH)->EnableWindow( bSacDepth );
   GetDlgItem(IDC_SACDEPTH_UNIT)->EnableWindow( bSacDepth );

   CDataExchange dx(this, FALSE);
   EventIndexType overlayEventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetOverlayLoadEventIndex();
   if (overlayEventIdx != INVALID_INDEX)
   {
      DDX_CBItemData(&dx, IDC_OVERLAY_EVENT, overlayEventIdx);
   }
}

void CBridgeDescDeckDetailsPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CConcreteDetailsDlg dlg(true/*f'c*/,false/*don't enable Compute Time Parameters option*/);

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();

   dlg.m_fc28 = pDeck->Concrete.Fc;
   dlg.m_Ec28 = pDeck->Concrete.Ec;
   dlg.m_bUserEc28 = pDeck->Concrete.bUserEc;

   dlg.m_General.m_Type    = pDeck->Concrete.Type;
   dlg.m_General.m_AggSize = pDeck->Concrete.MaxAggregateSize;
   dlg.m_General.m_Ds      = pDeck->Concrete.StrengthDensity;
   dlg.m_General.m_Dw      = pDeck->Concrete.WeightDensity;
   dlg.m_General.m_strUserEc  = m_strUserEc;

   dlg.m_AASHTO.m_EccK1       = pDeck->Concrete.EcK1;
   dlg.m_AASHTO.m_EccK2       = pDeck->Concrete.EcK2;
   dlg.m_AASHTO.m_CreepK1     = pDeck->Concrete.CreepK1;
   dlg.m_AASHTO.m_CreepK2     = pDeck->Concrete.CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = pDeck->Concrete.ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = pDeck->Concrete.ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = pDeck->Concrete.bHasFct;
   dlg.m_AASHTO.m_Fct         = pDeck->Concrete.Fct;

   dlg.m_ACI.m_bUserParameters = pDeck->Concrete.bACIUserParameters;
   dlg.m_ACI.m_A               = pDeck->Concrete.A;
   dlg.m_ACI.m_B               = pDeck->Concrete.B;
   dlg.m_ACI.m_CureMethod      = pDeck->Concrete.CureMethod;
   dlg.m_ACI.m_CementType      = pDeck->Concrete.ACI209CementType;

   matACI209Concrete concrete;
   concrete.SetTimeAtCasting(0);
   concrete.SetFc28(pDeck->Concrete.Fc);
   concrete.SetA(pDeck->Concrete.A);
   concrete.SetBeta(pDeck->Concrete.B);
   Float64 fci = concrete.GetFc(m_AgeAtContinuity);
   dlg.m_TimeAtInitialStrength = ::ConvertToSysUnits(m_AgeAtContinuity,unitMeasure::Day);
   dlg.m_fci = fci;
   dlg.m_fc28 = pDeck->Concrete.Fc;

   dlg.m_CEBFIP.m_bUserParameters = pDeck->Concrete.bCEBFIPUserParameters;
   dlg.m_CEBFIP.m_S               = pDeck->Concrete.S;
   dlg.m_CEBFIP.m_BetaSc          = pDeck->Concrete.BetaSc;
   dlg.m_CEBFIP.m_CementType      = pDeck->Concrete.CEBFIPCementType;

   if ( dlg.DoModal() == IDOK )
   {
      pDeck->Concrete.Fc               = dlg.m_fc28;
      pDeck->Concrete.Ec               = dlg.m_Ec28;
      pDeck->Concrete.bUserEc          = dlg.m_bUserEc28;

      pDeck->Concrete.Type             = dlg.m_General.m_Type;
      pDeck->Concrete.MaxAggregateSize = dlg.m_General.m_AggSize;
      pDeck->Concrete.StrengthDensity  = dlg.m_General.m_Ds;
      pDeck->Concrete.WeightDensity    = dlg.m_General.m_Dw;
      pDeck->Concrete.EcK1             = dlg.m_AASHTO.m_EccK1;
      pDeck->Concrete.EcK2             = dlg.m_AASHTO.m_EccK2;
      pDeck->Concrete.CreepK1          = dlg.m_AASHTO.m_CreepK1;
      pDeck->Concrete.CreepK2          = dlg.m_AASHTO.m_CreepK2;
      pDeck->Concrete.ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      pDeck->Concrete.ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      pDeck->Concrete.bHasFct          = dlg.m_AASHTO.m_bHasFct;
      pDeck->Concrete.Fct              = dlg.m_AASHTO.m_Fct;

      pDeck->Concrete.bACIUserParameters = dlg.m_ACI.m_bUserParameters;
      pDeck->Concrete.A                  = dlg.m_ACI.m_A;
      pDeck->Concrete.B                  = dlg.m_ACI.m_B;
      pDeck->Concrete.CureMethod         = dlg.m_ACI.m_CureMethod;
      pDeck->Concrete.ACI209CementType   = dlg.m_ACI.m_CementType;

      pDeck->Concrete.bCEBFIPUserParameters = dlg.m_CEBFIP.m_bUserParameters;
      pDeck->Concrete.S                     = dlg.m_CEBFIP.m_S;
      pDeck->Concrete.BetaSc                = dlg.m_CEBFIP.m_BetaSc;
      pDeck->Concrete.CEBFIPCementType      = dlg.m_CEBFIP.m_CementType;

      m_strUserEc  = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      UpdateConcreteControls();
   }
}

void CBridgeDescDeckDetailsPage::UpdateConcreteControls()
{
   // the concrete details were updated in the details dialog
   // update f'c and Ec in this dialog
   CDataExchange dx(this,FALSE);
   ExchangeConcreteData(&dx);
   UpdateEc();

   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   const CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();

   CString strLabel(ConcreteDescription(pDeck->Concrete));
   GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText( strLabel );
}

BOOL CBridgeDescDeckDetailsPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_MORE:
         UpdateConcreteParametersToolTip();
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CBridgeDescDeckDetailsPage::UpdateConcreteParametersToolTip()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();
   const unitmgtStressData&  stress  = pDisplayUnits->GetStressUnit();

   const CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), lrfdConcreteUtil::GetTypeName((matConcrete::Type)pDeck->Concrete.Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pDeck->Concrete.StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pDeck->Concrete.WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pDeck->Concrete.MaxAggregateSize,aggsize)
      );

   if ( pDeck->Concrete.Type != pgsTypes::Normal && pDeck->Concrete.bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pDeck->Concrete.Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   m_strTip = strTip;
}

void CBridgeDescDeckDetailsPage::OnAddDeckEdgePoint()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();

   pDeck->DeckEdgePoints = m_Grid.GetEdgePoints();

   CDeckPoint newPoint;
   if ( pDeck->DeckEdgePoints.size() == 0 )
   {
      newPoint.LeftEdge = ::ConvertToSysUnits(15.0,unitMeasure::Feet);
      newPoint.RightEdge = ::ConvertToSysUnits(15.0,unitMeasure::Feet);
      newPoint.Station = ::ConvertToSysUnits(100.0,unitMeasure::Feet);
   }
   else
   {
      newPoint = pDeck->DeckEdgePoints.back();
      newPoint.Station += ::ConvertToSysUnits(100.0,unitMeasure::Feet);
   }
   pDeck->DeckEdgePoints.push_back(newPoint);

   m_Grid.FillGrid(pDeck);
}

void CBridgeDescDeckDetailsPage::OnRemoveDeckEdgePoint()
{
   m_Grid.RemoveSelectedRows();
}

void CBridgeDescDeckDetailsPage::EnableRemove(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnable);
}

void CBridgeDescDeckDetailsPage::OnBnClickedOlayWeightLabel()
{
   OnWearingSurfaceTypeChanged();
}

void CBridgeDescDeckDetailsPage::OnBnClickedOlayDepthLabel()
{
   OnWearingSurfaceTypeChanged();
}

void CBridgeDescDeckDetailsPage::OnCbnSelchangeSameslaboffset()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_SAMESLABOFFSET);
   int idx = pBox->GetCurSel();

   m_SlabOffsetType = (pgsTypes::SlabOffsetType)pBox->GetItemData(idx);

   // TRICKY: But this takes care of the case when the offset type is changed and never edited:
   // Use logic in the edit haunch dialog to change the bridge description
   CEditHaunchDlg dlg(&(pParent->m_BridgeDesc));
   dlg.ForceToSlabOffsetType(m_SlabOffsetType);
   dlg.ModifyBridgeDescr(&(pParent->m_BridgeDesc));

   bool bEntireBridge = m_SlabOffsetType==pgsTypes::sotBridge; 

   CWnd* pWnd = GetDlgItem(IDC_ADIM);
   if ( bEntireBridge )   
   {
      // box was checked on so put the cache value into the window
      pWnd->SetWindowText(m_strSlabOffsetCache);
   }
   else
   {
      // box was checked off so read the current value from the window and cache it
      pWnd->GetWindowText(m_strSlabOffsetCache);
      pWnd->SetWindowText(_T(""));
   }

   UpdateSlabOffsetControls();

   // Show dialog 
   if ( !bEntireBridge )   
   {
      OnBnClickedEditHaunchButton();
   }
}

void CBridgeDescDeckDetailsPage::OnCbnSelchangeAssExcessCamberType()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_ASSEXCESSCAMBERTYPE);
   int idx = pBox->GetCurSel();

   m_AssExcessCamberType = (pgsTypes::AssExcessCamberType)pBox->GetItemData(idx);

   // TRICKY: But this takes care of the case when the offset type is changed and never edited:
   // Use logic in the edit haunch dialog to change the bridge description
   CEditHaunchDlg dlg(&(pParent->m_BridgeDesc));
   dlg.ForceToAssExcessCamberType(m_AssExcessCamberType);
   dlg.ModifyBridgeDescr(&(pParent->m_BridgeDesc));

   bool bEntireBridge = m_AssExcessCamberType==pgsTypes::aecBridge; 

   CWnd* pWnd = GetDlgItem(IDC_ASSEXCESSCAMBER);
   if ( bEntireBridge )   
   {
      // box was checked on so put the cache value into the window
      pWnd->SetWindowText(m_strAssExcessCamberCache);
   }
   else
   {
      // box was checked off so read the current value from the window and cache it
      pWnd->GetWindowText(m_strAssExcessCamberCache);
      pWnd->SetWindowText(_T(""));
   }

   UpdateAssExcessCamberControls();

   // Show dialog 
   if ( !bEntireBridge )   
   {
      OnBnClickedEditHaunchButton();
   }
}

void CBridgeDescDeckDetailsPage::UpdateSlabOffsetControls()
{
   // Which controls to show
   int ShowInput  = m_SlabOffsetType==pgsTypes::sotBridge ? SW_SHOW : SW_HIDE;
   int ShowButton = ShowInput == SW_HIDE ?  SW_SHOW : SW_HIDE;

   // enable/disable slab offset controls
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   BOOL bEnable = (deckType != pgsTypes::sdtNone) ? TRUE : FALSE; // if enabled, disable if deck type is none

   GetDlgItem(IDC_SAMESLABOFFSET)->EnableWindow( bEnable );
   GetDlgItem(IDC_ADIM_LABEL)->ShowWindow(ShowInput);
   GetDlgItem(IDC_ADIM)->ShowWindow(ShowInput);
   GetDlgItem(IDC_ADIM_UNIT)->ShowWindow(ShowInput);
   GetDlgItem(IDC_ADIM_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_EDIT_HAUNCH_BUTTON)->ShowWindow(ShowButton);
   GetDlgItem(IDC_EDIT_HAUNCH_BUTTON)->EnableWindow(bEnable);
}

void CBridgeDescDeckDetailsPage::UpdateDeckRelatedControls()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   // enable/disable slab offset controls
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   BOOL bEnable = deckType == pgsTypes::sdtNone ? FALSE : TRUE; // no fillet for "no deck" or overlay
   // haunch shape not applicable to adjacent beams
   BOOL enableShape = (bEnable && !::IsAdjacentSpacing(pParent->m_BridgeDesc.GetGirderSpacingType())) ? TRUE:FALSE;
   
   GetDlgItem(IDC_FILLET)->EnableWindow(bEnable);
   GetDlgItem(IDC_FILLET_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_FILLET_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_HAUNCH_SHAPE2)->EnableWindow(enableShape);
}

void CBridgeDescDeckDetailsPage::UpdateAssExcessCamberControls()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   // Which controls to show
   int ShowInput  = m_AssExcessCamberType==pgsTypes::aecBridge ? SW_SHOW : SW_HIDE;
   int ShowButton = ShowInput == SW_HIDE ?  SW_SHOW : SW_HIDE;

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   BOOL bEnable = (deckType != pgsTypes::sdtNone && m_bCanAssExcessCamberInputBeEnabled) ? TRUE : FALSE; // if enabled, disable if deck type is none

   GetDlgItem(IDC_ASSEXCESSCAMBERTYPE)->EnableWindow( bEnable );
   GetDlgItem(IDC_ASSEXCESSCAMBER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ASSEXCESSCAMBER)->ShowWindow(ShowInput);
   GetDlgItem(IDC_ASSEXCESSCAMBER)->EnableWindow(bEnable);
   GetDlgItem(IDC_ASSEXCESSCAMBER_UNIT)->ShowWindow(ShowInput);
   GetDlgItem(IDC_ASSEXCESSCAMBER_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_EDIT_HAUNCH_BUTTON2)->ShowWindow(ShowButton);
   GetDlgItem(IDC_EDIT_HAUNCH_BUTTON2)->EnableWindow(bEnable);
}

void CBridgeDescDeckDetailsPage::OnConditionFactorTypeChanged()
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



void CBridgeDescDeckDetailsPage::FillEventList()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();


   CComboBox* pcbDeckEvent = (CComboBox*)GetDlgItem(IDC_DECK_EVENT);
   CComboBox* pcbOverlayEvent = (CComboBox*)GetDlgItem(IDC_OVERLAY_EVENT);

   int deckEventIdx = pcbDeckEvent->GetCurSel();
   int overlayEventIdx = pcbOverlayEvent->GetCurSel();

   pcbDeckEvent->ResetContent();
   pcbOverlayEvent->ResetContent();

   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbDeckEvent->SetItemData(pcbDeckEvent->AddString(label),eventIdx);
      pcbOverlayEvent->SetItemData(pcbOverlayEvent->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbDeckEvent->SetItemData(pcbDeckEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);
   pcbOverlayEvent->SetItemData(pcbOverlayEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( deckEventIdx != CB_ERR )
   {
      pcbDeckEvent->SetCurSel(deckEventIdx);
   }
   else
   {
      pcbDeckEvent->SetCurSel(0);
   }

   if ( overlayEventIdx != CB_ERR )
   {
      pcbOverlayEvent->SetCurSel(overlayEventIdx);
   }
   else
   {
      pcbOverlayEvent->SetCurSel(0);
   }
}

void CBridgeDescDeckDetailsPage::OnDeckEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DECK_EVENT);
   m_PrevDeckEventIdx = pCB->GetCurSel();
}

void CBridgeDescDeckDetailsPage::OnDeckEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DECK_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType eventIdx = (EventIndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
   }

   if (eventIdx != INVALID_INDEX)
   {
      CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

      bool bDone = false;
      bool bAdjustTimeline = false;
      while ( !bDone )
      {
         int result = pParent->m_BridgeDesc.GetTimelineManager()->SetCastDeckEventByIndex(eventIdx,bAdjustTimeline);
         if ( result == TLM_SUCCESS )
         {
            eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastDeckEventIndex();
            bDone = true;
         }
         else
         {
            CString strProblem;
            if (result == TLM_OVERLAPS_PREVIOUS_EVENT )
            {
               strProblem = _T("This event begins before the activities in the previous event have completed.");
            }
            else
            {
               strProblem = _T("The activities in this event end after the next event begins.");
            }

            CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));

            CString strMsg;
            strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
            if ( AfxMessageBox(strMsg,MB_OKCANCEL | MB_ICONQUESTION) == IDOK )
            {
               bAdjustTimeline = true;
            }
            else
            {
               pParent->m_BridgeDesc.GetTimelineManager()->RemoveEventByIndex(eventIdx);
               pParent->m_BridgeDesc.GetTimelineManager()->SetCastDeckEventByIndex(m_PrevDeckEventIdx,false);
               pCB->SetCurSel((int)m_PrevDeckEventIdx);
               return;
            }

         }
      }

      FillEventList();

      pCB->SetCurSel((int)eventIdx);
   }
   else
   {
      pCB->SetCurSel((int)m_PrevDeckEventIdx);
   }
}

void CBridgeDescDeckDetailsPage::OnOverlayEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_OVERLAY_EVENT);
   m_PrevOverlayEventIdx = pCB->GetCurSel();
}

void CBridgeDescDeckDetailsPage::OnOverlayEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_OVERLAY_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType eventIdx = (EventIndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
      CTimelineEventDlg dlg(*pParent->m_BridgeDesc.GetTimelineManager(),INVALID_INDEX,FALSE);
      if ( dlg.DoModal() == IDOK )
      {
         bool bDone = false;
         bool bAdjustTimeline = false;
         while ( !bDone )
         {
            int result = pParent->m_BridgeDesc.GetTimelineManager()->AddTimelineEvent(*dlg.m_pTimelineEvent,bAdjustTimeline,&eventIdx);
            if ( result == TLM_SUCCESS )
            {
               bDone = true;
            }
            else
            {
               CString strProblem;
               if (result == TLM_OVERLAPS_PREVIOUS_EVENT )
               {
                  strProblem = _T("This event begins before the activities in the previous event have completed.");
               }
               else
               {
                  strProblem = _T("The activities in this event end after the next event begins.");
               }

               CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));

               CString strMsg;
               strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
               if ( AfxMessageBox(strMsg,MB_OKCANCEL | MB_ICONQUESTION) == IDOK )
               {
                  bAdjustTimeline = true;
               }
               else
               {
                  bDone = true;
               }
            }
         }
     }
   }

   if (eventIdx != INVALID_INDEX)
   {
      CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

      pParent->m_BridgeDesc.GetTimelineManager()->SetOverlayLoadEventByIndex(eventIdx);

      FillEventList();

      pCB->SetCurSel((int)eventIdx);
   }
   else
   {
      pCB->SetCurSel((int)m_PrevOverlayEventIdx);
   }
}

EventIndexType CBridgeDescDeckDetailsPage::CreateEvent()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   CTimelineEventDlg dlg(*pParent->m_BridgeDesc.GetTimelineManager(),INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      int result = pParent->m_BridgeDesc.GetTimelineManager()->AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      return eventIdx;
  }

   return INVALID_INDEX;
}

void CBridgeDescDeckDetailsPage::OnBnClickedEditHaunchButton()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CEditHaunchDlg dlg(&(pParent->m_BridgeDesc));
   dlg.ForceToSlabOffsetType(m_SlabOffsetType);
   dlg.ForceToAssExcessCamberType(m_AssExcessCamberType);
   if ( dlg.DoModal() == IDOK )
   {
      // Dialog modifies bridge descr
      dlg.ModifyBridgeDescr(&(pParent->m_BridgeDesc));

      // Upload pertinent changed data back into this dialog
      bool st = SetCBItemData(this, IDC_HAUNCH_SHAPE2, pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape);
      ATLASSERT(st);

      // slab offset 
      m_SlabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
      st = SetCBItemData(this, IDC_SAMESLABOFFSET, m_SlabOffsetType);
      ATLASSERT(st);

      // Dialog can change back to single A for whole bridge. Set local values if needed
      if (m_SlabOffsetType==pgsTypes::sotBridge)
      {
         m_SlabOffset = pParent->m_BridgeDesc.GetSlabOffset();
         m_strSlabOffsetCache.Format(_T("%s"),FormatDimension(m_SlabOffset,pDisplayUnits->GetComponentDimUnit(), false));
         CWnd* pWnd = GetDlgItem(IDC_ADIM);
         pWnd->SetWindowText(m_strSlabOffsetCache);
      }

      // AssExcessCamber
      m_AssExcessCamberType = pParent->m_BridgeDesc.GetAssExcessCamberType();
      st = SetCBItemData(this, IDC_ASSEXCESSCAMBERTYPE, m_AssExcessCamberType);
      ATLASSERT(st);

      // Dialog can change back to single AssExcessCamber for whole bridge. Set local values if needed
      if (m_AssExcessCamberType==pgsTypes::aecBridge)
      {
         m_AssExcessCamber = pParent->m_BridgeDesc.GetAssExcessCamber();
         m_strAssExcessCamberCache.Format(_T("%s"),FormatDimension(m_AssExcessCamber,pDisplayUnits->GetComponentDimUnit(), false));
         CWnd* pWnd = GetDlgItem(IDC_ASSEXCESSCAMBER);
         pWnd->SetWindowText(m_strAssExcessCamberCache);
      }

      UpdateSlabOffsetControls();
      UpdateDeckRelatedControls();
      UpdateAssExcessCamberControls();
   }
}

void CBridgeDescDeckDetailsPage::OnCbnSelchangeHaunchShape2()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   // keep haunch shape up to date
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_HAUNCH_SHAPE2);
   int idx = pBox->GetCurSel();
   pParent->m_BridgeDesc.GetDeckDescription()->HaunchShape = (pgsTypes::HaunchShapeType)pBox->GetItemData(idx);
}
