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

// BridgeDescDeckDetailsPagePage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"

#include "PGSpliceDoc.h"
#include "PGSuperDoc.h"
#include "BridgeDescDeckDetailsPage.h"
#include "BridgeDescDlg.h"
#include "ConcreteDetailsDlg.h"
#include "PGSuperAppPlugin\TimelineEventDlg.h"
#include "UIHintsDlg.h"
#include "Hints.h"

#include <System\Flags.h>

#include "HtmlHelp\HelpTopics.hh"

#include <WBFLGenericBridge.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#include <EAF\EAFMainFrame.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckDetailsPage property page

IMPLEMENT_DYNCREATE(CBridgeDescDeckDetailsPage, CPropertyPage)

CBridgeDescDeckDetailsPage::CBridgeDescDeckDetailsPage() : CPropertyPage(CBridgeDescDeckDetailsPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescDeckDetailsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CBridgeDescDeckDetailsPage::~CBridgeDescDeckDetailsPage()
{
}

void CBridgeDescDeckDetailsPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
	
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CEAFDocument* pDoc = EAFGetDocument();
   

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->DeckType;

   CPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CBridgeDescDeckDetailsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	DDX_Control(pDX, IDC_MOD_E,   m_ctrlEcCheck);
	DDX_Control(pDX, IDC_SLAB_FC, m_ctrlFc);
	//}}AFX_DATA_MAP

   // make sure unit tags are always displayed (even if disabled)
   DDX_Tag( pDX, IDC_GROSS_DEPTH_UNIT,    pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag( pDX, IDC_FILLET_UNIT,         pDisplayUnits->GetComponentDimUnit() );
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
      DDX_UnitValueAndTag( pDX, IDC_FILLET, IDC_FILLET_UNIT, pParent->m_BridgeDesc.GetDeckDescription()->Fillet, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueZeroOrMore( pDX, IDC_FILLET,pParent->m_BridgeDesc.GetDeckDescription()->Fillet, pDisplayUnits->GetComponentDimUnit() );

      // slab offset
      DDX_Check_Bool(pDX,IDC_SAMESLABOFFSET,m_bSlabOffsetWholeBridge);
      if ( m_bSlabOffsetWholeBridge )
      {
         DDX_UnitValueAndTag( pDX, IDC_ADIM, IDC_ADIM_UNIT, m_SlabOffset, pDisplayUnits->GetComponentDimUnit() );
      }

      // overhang
      DDX_UnitValueAndTag( pDX, IDC_OVERHANG_DEPTH, IDC_OVERHANG_DEPTH_UNIT, pParent->m_BridgeDesc.GetDeckDescription()->OverhangEdgeDepth, pDisplayUnits->GetComponentDimUnit() );
      DDX_CBItemData( pDX, IDC_OVERHANG_TAPER, pParent->m_BridgeDesc.GetDeckDescription()->OverhangTaper );

      // deck panel
      DDX_UnitValueAndTag( pDX, IDC_PANEL_DEPTH, IDC_PANEL_DEPTH_UNIT,  pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth, pDisplayUnits->GetComponentDimUnit() );
      if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeSIP ) // SIP
      {
         DDV_UnitValueGreaterThanZero( pDX, IDC_PANEL_DEPTH, pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth, pDisplayUnits->GetXSectionDimUnit() );
      }

      DDX_UnitValueAndTag( pDX, IDC_PANEL_SUPPORT, IDC_PANEL_SUPPORT_UNIT,  pParent->m_BridgeDesc.GetDeckDescription()->PanelSupport, pDisplayUnits->GetComponentDimUnit() );

      // slab material
      ExchangeConcreteData(pDX);
   }

   // sacrificial depth
   DDX_UnitValueAndTag( pDX, IDC_SACDEPTH,      IDC_SACDEPTH_UNIT,     pParent->m_BridgeDesc.GetDeckDescription()->SacrificialDepth, pDisplayUnits->GetComponentDimUnit() );
   if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   {
      if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeSIP ) // SIP
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
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      EventIndexType overlayEventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetOverlayLoadEventIndex();
      DDX_CBItemData(pDX,IDC_OVERLAY_EVENT,overlayEventIdx);

      if ( pDX->m_bSaveAndValidate )
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
            VERIFY(pParent->m_BridgeDesc.GetTimelineManager()->RemoveOverlayLoadEvent() == TLM_SUCCESS);
         }
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
         if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType != pgsTypes::sdtNone )
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
      if ( m_bSlabOffsetWholeBridge )
      {
         // Slab offset applies to the entire bridge... have users adjust Slab offset if it doesn't
         // fit with the slab depth

         DDV_UnitValueLimitOrMore( pDX, IDC_ADIM,m_SlabOffset, pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetDeckDescription()->Fillet, pDisplayUnits->GetComponentDimUnit() );
         pParent->m_BridgeDesc.SetSlabOffset(m_SlabOffset);
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotBridge);

         Float64 grossDepth;
         bool bCheckDepth = false;
         CString strMsg1;
         CString strMsg2;

         if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeCIP ||
              pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeOverlay ) // CIP deck
         {
            grossDepth = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth;
            strMsg1 = _T("Slab Offset must be larger than the gross slab depth");
            strMsg2 = _T("Overhang edge depth must be less than the gross slab depth");
            bCheckDepth = true;
         }
         else if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeSIP ) // SIP
         {
            grossDepth = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth + pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth;
            strMsg1 = _T("Slab Offset must be larger than the cast depth + panel depth");
            strMsg2 = _T("Overhang edge depth must be less than the cast depth + panel depth");
            bCheckDepth = true;
         }
         else
         {
            ATLASSERT(false);
            // should not get here
         }

         if ( bCheckDepth && m_SlabOffset < grossDepth )
         {
            AfxMessageBox(strMsg1);
            pDX->PrepareEditCtrl(IDC_ADIM);
            pDX->Fail();
         }
      }
      else
      {
         // Slab offset is span-by-span or girder-by-girder. Have user adjust the slab depth if it doesn't
         // fit with the current values for slab offset.
         
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotGirder); // force to girder-by-girder

         Float64 maxSlabOffset = pParent->m_BridgeDesc.GetMaxSlabOffset();

         if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeCIP || 
              pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeOverlay )
         {
            pDX->PrepareEditCtrl(IDC_GROSS_DEPTH);
            if ( maxSlabOffset - pParent->m_BridgeDesc.GetDeckDescription()->Fillet < pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth )
            {
               CString msg;
               msg.Format(_T("Gross slab depth must less than %s to accomodate the %s fillet and maximum slab offset of %s"),
                            FormatDimension(maxSlabOffset - pParent->m_BridgeDesc.GetDeckDescription()->Fillet,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(pParent->m_BridgeDesc.GetDeckDescription()->Fillet,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(maxSlabOffset,pDisplayUnits->GetComponentDimUnit()));
               AfxMessageBox(msg,MB_ICONEXCLAMATION);
               pDX->Fail();
            }
         }
         else if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeSIP )
         {
            pDX->PrepareEditCtrl(IDC_PANEL_DEPTH);
            if ( maxSlabOffset -  pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth - pParent->m_BridgeDesc.GetDeckDescription()->Fillet < pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth )
            {
               CString msg;
               msg.Format(_T("Cast slab depth must less than %s to accomodate the %s panel, %s fillet and maximum slab offset of %s"),
                            FormatDimension(maxSlabOffset - pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth - pParent->m_BridgeDesc.GetDeckDescription()->Fillet,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(pParent->m_BridgeDesc.GetDeckDescription()->Fillet,pDisplayUnits->GetComponentDimUnit()),
                            FormatDimension(maxSlabOffset,pDisplayUnits->GetComponentDimUnit()));
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
   }

   if ( pDX->m_bSaveAndValidate )
   {
      DDV_GXGridWnd(pDX,&m_Grid);
      pParent->m_BridgeDesc.GetDeckDescription()->DeckEdgePoints = m_Grid.GetEdgePoints();
   }
   else
   {
      m_Grid.FillGrid(pParent->m_BridgeDesc.GetDeckDescription());
   }

   DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, pParent->m_BridgeDesc.GetDeckDescription()->Condition);
   DDX_Text(pDX,   IDC_CONDITION_FACTOR,      pParent->m_BridgeDesc.GetDeckDescription()->ConditionFactor);

   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) && !pDX->m_bSaveAndValidate )
   {
      EventIndexType deckEventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastDeckEventIndex();
      DDX_CBItemData(pDX,IDC_DECK_EVENT,deckEventIdx);
   }
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


   if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType != pgsTypes::sdtNone && pDX->m_bSaveAndValidate )
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
   ON_BN_CLICKED(IDC_SAMESLABOFFSET, &CBridgeDescDeckDetailsPage::OnBnClickedSameslaboffset)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, &CBridgeDescDeckDetailsPage::OnConditionFactorTypeChanged)
   ON_CBN_SELCHANGE(IDC_DECK_EVENT, OnDeckEventChanged)
   ON_CBN_DROPDOWN(IDC_DECK_EVENT, OnDeckEventChanging)
   ON_CBN_SELCHANGE(IDC_OVERLAY_EVENT, OnOverlayEventChanged)
   ON_CBN_DROPDOWN(IDC_OVERLAY_EVENT, OnOverlayEventChanging)
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
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   EventIndexType eventIdx = pTimelineMgr->GetCastDeckEventIndex();
   m_AgeAtContinuity = pTimelineMgr->GetEventByIndex(eventIdx)->GetCastDeckActivity().GetConcreteAgeAtContinuity();


   // set density/weight labels
   CStatic* pStatic = (CStatic*)GetDlgItem( IDC_OLAY_DENSITY_LABEL );
   if ( IS_SI_UNITS(pDisplayUnits) )
   {
      pStatic->SetWindowText( _T("Overlay Density") );
   }
   else
   {
      pStatic->SetWindowText( _T("Overlay Weight") );
   }

   // fill up slab overhang taper options
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_OVERHANG_TAPER);
   int idx = pCB->AddString(_T("Taper overhang to top of top flange"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::dotTopTopFlange);

   idx = pCB->AddString(_T("Taper overhang to bottom of top flange"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::dotBottomTopFlange);

   idx = pCB->AddString(_T("Don't taper overhang"));
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

   // disable wearing surface stage if it doesn't apply
   pCB = (CComboBox*)GetDlgItem(IDC_OVERLAY_EVENT);
   if ( pParent->m_BridgeDesc.GetDeckDescription()->WearingSurface == pgsTypes::wstSacrificialDepth )
   {
      pCB->EnableWindow(FALSE);
   }

   m_SlabOffset = pParent->m_BridgeDesc.GetSlabOffset();
   m_bSlabOffsetWholeBridge = pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotBridge ? true : false;
   m_strSlabOffsetCache.Format(_T("%s"),FormatDimension(m_SlabOffset,pDisplayUnits->GetComponentDimUnit(), false));

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   CPropertyPage::OnInitDialog();

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

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescDeckDetailsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGEDESC_DECKDETAILS );
}

BOOL CBridgeDescDeckDetailsPage::OnSetActive() 
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->DeckType;

   CString strDeckType;
   switch ( deckType )
   {
      case pgsTypes::sdtCompositeCIP:
         strDeckType = _T("Composite Cast-In-Place Deck");
         break;

      case pgsTypes::sdtCompositeSIP:
         strDeckType = _T("Composite Stay-In-Place Deck Panels");
         break;

      case pgsTypes::sdtCompositeOverlay:
         strDeckType = _T("Composite Cast-In-Place Overlay");
         break;

      case pgsTypes::sdtNone:
         strDeckType = _T("No deck");
   }
   GetDlgItem(IDC_DECK_TYPE)->SetWindowText(strDeckType);


   CWnd* pWnd = GetDlgItem(IDC_GROSS_DEPTH_LABEL);
   pWnd->SetWindowText(deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeOverlay ? _T("Gross Depth") : _T("Cast Depth"));

   GetDlgItem(IDC_GROSS_DEPTH_LABEL)->EnableWindow( deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_GROSS_DEPTH)->EnableWindow(       deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_GROSS_DEPTH_UNIT)->EnableWindow(  deckType != pgsTypes::sdtNone);

   GetDlgItem(IDC_OVERHANG_DEPTH_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_OVERHANG_DEPTH)->EnableWindow(       deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_OVERHANG_DEPTH_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_OVERHANG_TAPER)->EnableWindow(       deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);

   GetDlgItem(IDC_FILLET_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_FILLET)->EnableWindow(       deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_FILLET_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);

   if ( deckType == pgsTypes::sdtCompositeOverlay )
   {
      GetDlgItem(IDC_FILLET)->SetWindowText(_T("0.00"));
   }

   UpdateSlabOffsetControls();

   GetDlgItem(IDC_SAMESLABOFFSET)->EnableWindow( deckType != pgsTypes::sdtNone );

   GetDlgItem(IDC_SLAB_FC_LABEL)->EnableWindow( deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_SLAB_FC)->EnableWindow(       deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_SLAB_FC_UNIT)->EnableWindow(  deckType != pgsTypes::sdtNone);
   GetDlgItem(IDC_MORE)->EnableWindow(  deckType != pgsTypes::sdtNone);
   
   GetDlgItem(IDC_ADD)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );
   GetDlgItem(IDC_REMOVE)->EnableWindow( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );
   m_Grid.Enable( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );

   GetDlgItem(IDC_SACDEPTH_LABEL)->EnableWindow( deckType != pgsTypes::sdtNone );
   GetDlgItem(IDC_SACDEPTH)->EnableWindow( deckType != pgsTypes::sdtNone );
   GetDlgItem(IDC_SACDEPTH_UNIT)->EnableWindow( deckType != pgsTypes::sdtNone );

   GetDlgItem(IDC_SIP_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeSIP);

   GetDlgItem(IDC_PANEL_DEPTH_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_PANEL_DEPTH)->EnableWindow(       deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_PANEL_DEPTH_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeSIP);

   GetDlgItem(IDC_PANEL_SUPPORT_LABEL)->EnableWindow( deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_PANEL_SUPPORT)->EnableWindow(       deckType == pgsTypes::sdtCompositeSIP);
   GetDlgItem(IDC_PANEL_SUPPORT_UNIT)->EnableWindow(  deckType == pgsTypes::sdtCompositeSIP);
	
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

   return CPropertyPage::OnSetActive();
}

void CBridgeDescDeckDetailsPage::OnUserEc()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   BOOL bEnable = m_ctrlEcCheck.GetCheck();

   // can't be enabled if there is no deck
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->DeckType;

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

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->DeckType;

   CEAFDocument* pDoc = EAFGetDocument();
   bool bPGSuperDoc = false;
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      bPGSuperDoc = true;
   }

   pgsTypes::WearingSurfaceType ws = (pgsTypes::WearingSurfaceType)(pCB->GetItemData(idx));
   if ( ws == pgsTypes::wstSacrificialDepth )
   {
      bSacDepth               = TRUE;
      bOverlayEvent           = FALSE;
      bOverlayLabel           = FALSE;
      bOverlayWeight          = FALSE;
      bOverlayDepthAndDensity = FALSE;
   }
   else if ( ws == pgsTypes::wstFutureOverlay )
   {
      ATLASSERT(bPGSuperDoc == true); // future overlay only used for PGSuper... should not get here if PGSplice
      bSacDepth               = TRUE;
      bOverlayEvent           = FALSE;
      bOverlayLabel           = TRUE;
      bOverlayWeight          = (iOption == IDC_OLAY_WEIGHT_LABEL ? TRUE : FALSE);
      bOverlayDepthAndDensity = (iOption == IDC_OLAY_DEPTH_LABEL  ? TRUE : FALSE);
   }
   else
   {
      bSacDepth               = (bPGSuperDoc ? FALSE : TRUE); // If regular overlay in PGSuper, no sacrifical depth. In PGSplice, sacrifical depth applies until overlay is applied
      bOverlayEvent           = (bPGSuperDoc ? FALSE : TRUE);
      bOverlayLabel           = TRUE;
      bOverlayWeight          = (iOption == IDC_OLAY_WEIGHT_LABEL ? TRUE : FALSE);
      bOverlayDepthAndDensity = (iOption == IDC_OLAY_DEPTH_LABEL  ? TRUE : FALSE);
   }

   GetDlgItem(IDC_OVERLAY_EVENT)->EnableWindow( bOverlayEvent );

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
}

void CBridgeDescDeckDetailsPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CConcreteDetailsDlg dlg;

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();

   dlg.m_General.m_Type    = pDeck->Concrete.Type;
   dlg.m_General.m_Fc      = pDeck->Concrete.Fc;
   dlg.m_General.m_AggSize = pDeck->Concrete.MaxAggregateSize;
   dlg.m_General.m_bUserEc = pDeck->Concrete.bUserEc;
   dlg.m_General.m_Ds      = pDeck->Concrete.StrengthDensity;
   dlg.m_General.m_Dw      = pDeck->Concrete.WeightDensity;
   dlg.m_General.m_Ec      = pDeck->Concrete.Ec;
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
   dlg.m_ACI.m_CementType      = pDeck->Concrete.CementType;

   matACI209Concrete concrete;
   concrete.SetTimeAtCasting(0);
   concrete.SetFc28(pDeck->Concrete.Fc);
   concrete.SetA(pDeck->Concrete.A);
   concrete.SetBeta(pDeck->Concrete.B);
   Float64 fci = concrete.GetFc(m_AgeAtContinuity);
   dlg.m_ACI.m_TimeAtInitialStrength = ::ConvertToSysUnits(m_AgeAtContinuity,unitMeasure::Day);
   dlg.m_ACI.m_fci = fci;
   dlg.m_ACI.m_fc28 = pDeck->Concrete.Fc;

#pragma Reminder("UPDATE: deal with CEB-FIP concrete models")

   if ( dlg.DoModal() == IDOK )
   {
      pDeck->Concrete.Type             = dlg.m_General.m_Type;
      pDeck->Concrete.Fc               = dlg.m_General.m_Fc;
      pDeck->Concrete.MaxAggregateSize = dlg.m_General.m_AggSize;
      pDeck->Concrete.bUserEc          = dlg.m_General.m_bUserEc;
      pDeck->Concrete.StrengthDensity  = dlg.m_General.m_Ds;
      pDeck->Concrete.WeightDensity    = dlg.m_General.m_Dw;
      pDeck->Concrete.Ec               = dlg.m_General.m_Ec;
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
      pDeck->Concrete.CementType         = dlg.m_ACI.m_CementType;

#pragma Reminder("UPDATE: deal with CEB-FIP concrete models")

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
      pTTT->hinst = NULL;
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
      _T("Type"), matConcrete::GetTypeName((matConcrete::Type)pDeck->Concrete.Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pDeck->Concrete.StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pDeck->Concrete.WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pDeck->Concrete.MaxAggregateSize,aggsize)
      );

   //if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   //{
   //   // add K1 parameter
   //   CString strK1;
   //   strK1.Format(_T("\r\n%-20s %s"),
   //      _T("K1"),FormatScalar(pDeck->SlabK1,scalar));

   //   strTip += strK1;
   //}

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

void CBridgeDescDeckDetailsPage::OnBnClickedSameslaboffset()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_SAMESLABOFFSET);
   CString strHint;
   if ( bEnable )
   {
      strHint = _T("By checking this box, the same slab offset will be used for the entire bridge");
   }
   else
   {
      strHint = _T("By unchecking this box, the slab offset can be specified by span or by girder. Span by span slab offsets can be defined on either the Span or Pier Connections page. Girder by girder slab offsets can be defined on the Girder Spacing or Girder dialogs");
   }

   // Show the hint dialog
   UIHint(strHint,UIHINT_SINGLE_SLAB_OFFSET);

   CWnd* pWnd = GetDlgItem(IDC_ADIM);
   if ( bEnable )   
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
}

void CBridgeDescDeckDetailsPage::UpdateSlabOffsetControls()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_SAMESLABOFFSET) ? TRUE : FALSE;

   // enable/disable slab offset controls
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->DeckType;

   if ( bEnable )
   {
      bEnable = (deckType != pgsTypes::sdtNone); // if enabled, disable if deck type is none
   }

   GetDlgItem(IDC_ADIM_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_UNIT)->EnableWindow(bEnable);
}

void CBridgeDescDeckDetailsPage::UIHint(const CString& strText,UINT mask)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)EAFGetDocument();

   Uint32 hintSettings = pDoc->GetUIHintSettings();
   if ( sysFlags<Uint32>::IsClear(hintSettings,mask) )
   {
      CUIHintsDlg dlg;
      dlg.m_strTitle = _T("Hint");
      dlg.m_strText = strText;
      dlg.DoModal();
      if ( dlg.m_bDontShowAgain )
      {
         sysFlags<Uint32>::Set(&hintSettings,mask);
         pDoc->SetUIHintSettings(hintSettings);
      }
   }
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
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      GetDlgItem(IDC_DECK_EVENT_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DECK_EVENT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_OVERLAY_EVENT)->ShowWindow(SW_HIDE);

      return;
   }

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

   if ( overlayEventIdx != CB_ERR )
   {
      pcbOverlayEvent->SetCurSel(overlayEventIdx);
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
      CTimelineEventDlg dlg(pParent->m_BridgeDesc.GetTimelineManager(),FALSE);
      if ( dlg.DoModal() == IDOK )
      {
         bool bDone = false;
         bool bAdjustTimeline = false;
         while ( !bDone )
         {
            int result = pParent->m_BridgeDesc.GetTimelineManager()->AddTimelineEvent(dlg.m_TimelineEvent,bAdjustTimeline,&eventIdx);
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
   CTimelineEventDlg dlg(pParent->m_BridgeDesc.GetTimelineManager(),FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      int result = pParent->m_BridgeDesc.GetTimelineManager()->AddTimelineEvent(dlg.m_TimelineEvent,true,&eventIdx);
      return eventIdx;
  }

   return INVALID_INDEX;
}
