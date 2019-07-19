///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// SpanLayoutPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "SpanLayoutPage.h"
#include "SpanDetailsDlg.h"
#include "SelectItemDlg.h"
#include "Utilities.h"

#include <IFace\DocumentType.h>

#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage property page

IMPLEMENT_DYNCREATE(CSpanLayoutPage, CPropertyPage)

CSpanLayoutPage::CSpanLayoutPage() : CPropertyPage(CSpanLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CSpanLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_SpanLength = -1;
   m_bHasSlabOffset = true;
}

CSpanLayoutPage::~CSpanLayoutPage()
{
}

void CSpanLayoutPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   DDX_Control(pDX, IDC_START_SLAB_OFFSET, m_ctrlStartSlabOffset);
   DDX_Control(pDX, IDC_END_SLAB_OFFSET,   m_ctrlEndSlabOffset);
   DDX_Control(pDX, IDC_ASSUMED_EXCESS_CAMBER, m_ctrlAssumedExcessCamber);

   // display units no matter what
   DDX_Tag( pDX, IDC_ASSUMED_EXCESS_CAMBER_UNIT,pDisplayUnits->GetComponentDimUnit() );

   DDX_UnitValueAndTag(pDX,IDC_SPAN_LENGTH,IDC_SPAN_LENGTH_UNIT,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueGreaterThanZero(pDX,IDC_SPAN_LENGTH,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());

   DDX_CBItemData(pDX, IDC_SLAB_OFFSET_TYPE, m_SlabOffsetType);

   CPierData2* pStartPier = pParent->m_pSpanData->GetPrevPier();
   CPierData2* pEndPier = pParent->m_pSpanData->GetNextPier();
   if (pStartPier->HasSlabOffset())
   {
      DDX_UnitValueAndTag(pDX, IDC_START_SLAB_OFFSET, IDC_START_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit());
   }
   
   if(pEndPier->HasSlabOffset())
   {
      DDX_UnitValueAndTag(pDX, IDC_END_SLAB_OFFSET, IDC_END_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::metEnd], pDisplayUnits->GetComponentDimUnit());
   };

   Float64 AssumedExcessCamber = 0;
   if (m_bCanAssumedExcessCamberInputBeEnabled)
   {
      AssumedExcessCamber = pParent->m_pSpanData->GetAssumedExcessCamber(0);
      DDX_UnitValueAndTag(pDX, IDC_ASSUMED_EXCESS_CAMBER, IDC_ASSUMED_EXCESS_CAMBER_UNIT, AssumedExcessCamber, pDisplayUnits->GetComponentDimUnit());
   }

   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.SetSpanLength(pParent->m_pSpanData->GetIndex(),m_SpanLength);

      if (pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone && m_SlabOffsetType != pgsTypes::sotSegment && m_bHasSlabOffset)
      {
         Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();

         CString strMinValError;
         strMinValError.Format(_T("Slab Offset value must be greater or equal to (%s)"), FormatDimension(minSlabOffset,pDisplayUnits->GetComponentDimUnit()));

         if (pStartPier->HasSlabOffset() && m_SlabOffset[pgsTypes::metStart] < minSlabOffset)
         {
            pDX->PrepareCtrl(IDC_START_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         if (pEndPier->HasSlabOffset() && m_SlabOffset[pgsTypes::metEnd] < minSlabOffset)
         {
            pDX->PrepareCtrl(IDC_END_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         // Slab offset
         pParent->m_BridgeDesc.SetSlabOffsetType(m_SlabOffsetType);
         if (m_SlabOffsetType == pgsTypes::sotBridge)
         {
            pParent->m_BridgeDesc.SetSlabOffset(m_SlabOffset[pStartPier->HasSlabOffset() ? pgsTypes::metStart : pgsTypes::metEnd]);
         }
         else 
         {
            // this is not a context where we can change individual segment slab offsets
            // so the slab offset type must sotBearingLine here
            ASSERT(m_SlabOffsetType == pgsTypes::sotBearingLine);
            if (pStartPier->HasSlabOffset())
            {
               pStartPier->SetSlabOffset(pgsTypes::Ahead, m_SlabOffset[pgsTypes::metStart]);
            }

            if (pEndPier->HasSlabOffset())
            {
               pEndPier->SetSlabOffset(pgsTypes::Back, m_SlabOffset[pgsTypes::metEnd]);
            }
         }

         // excess camber
         if (m_bCanAssumedExcessCamberInputBeEnabled)
         {
            if (pParent->m_BridgeDesc.GetAssumedExcessCamberType() == pgsTypes::aecBridge)
            {
               pParent->m_BridgeDesc.SetAssumedExcessCamber(AssumedExcessCamber);
            }
            else if (pParent->m_BridgeDesc.GetAssumedExcessCamberType() == pgsTypes::aecSpan)
            {
               pParent->m_pSpanData->SetAssumedExcessCamber(AssumedExcessCamber);
            }
            else
            {
               if (m_InitialAssumedExcessCamberType != pgsTypes::aecGirder)
               {
                  GirderIndexType nGirders = pParent->m_pSpanData->GetGirderCount();
                  for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                  {
                     pParent->m_pSpanData->SetAssumedExcessCamber(gdrIdx, AssumedExcessCamber);
                  }
               }
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CSpanLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanLayoutPage)
	ON_WM_CTLCOLOR()
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_ASSUMED_EXCESS_CAMBER_TYPE, &CSpanLayoutPage::OnCbnSelchangeAssumedExcessCamberCombo)
   ON_CBN_DROPDOWN(IDC_SLAB_OFFSET_TYPE, &CSpanLayoutPage::OnChangingSlabOffset)
   ON_CBN_SELCHANGE(IDC_SLAB_OFFSET_TYPE, &CSpanLayoutPage::OnChangeSlabOffset)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage message handlers

BOOL CSpanLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   m_SpanIdx = pParent->m_pSpanData->GetIndex();
   m_PrevPierIdx = (PierIndexType)m_SpanIdx;
   m_NextPierIdx = m_PrevPierIdx+1;

   m_SpanLength = pParent->m_pSpanData->GetSpanLength();
   m_InitialAssumedExcessCamberType = pParent->m_BridgeDesc.GetAssumedExcessCamberType();


   m_SlabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   BOOL bIsPGSuper = (pDocType->IsPGSuperDocument() ? TRUE : FALSE);
   CComboBox* pSlabOffsetCB = (CComboBox*)this->GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   if (m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotBearingLine)
   {
      // in the context of this dialog, all we can edit is bridge or pier level values
      int idx = pSlabOffsetCB->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBridge, bIsPGSuper));
      pSlabOffsetCB->SetItemData(idx, pgsTypes::sotBridge);
      idx = pSlabOffsetCB->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine, bIsPGSuper));
      pSlabOffsetCB->SetItemData(idx, pgsTypes::sotBearingLine);
   }
   else
   {
      // we go to this dialog with the slab offset by segment
      // since segment values can't be edited here, the only choice we have to to edit the bearing line values
      // however, we need the by segment option so the UI shows the current state. If by segment is
      // selected when the dialog closes, the slab offset information is not updated
      ASSERT(m_SlabOffsetType == pgsTypes::sotSegment);
      int idx = pSlabOffsetCB->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine, bIsPGSuper));
      pSlabOffsetCB->SetItemData(idx, pgsTypes::sotBearingLine);
      idx = pSlabOffsetCB->AddString(GetSlabOffsetTypeAsString(pgsTypes::sotSegment, bIsPGSuper));
      pSlabOffsetCB->SetItemData(idx, pgsTypes::sotSegment);
   }

   if (m_SlabOffsetType == pgsTypes::sotSegment)
   {
      // since the context of this dialog is such that we can't edit segment level slab offsets
      // don't show the numbers in the controls when segment is the selected type
      m_ctrlStartSlabOffset.ShowDefaultWhenDisabled(FALSE);
      m_ctrlEndSlabOffset.ShowDefaultWhenDisabled(FALSE);
   }
   else
   {
      m_ctrlStartSlabOffset.ShowDefaultWhenDisabled(TRUE);
      m_ctrlEndSlabOffset.ShowDefaultWhenDisabled(TRUE);
   }

   // not all piers have slab offsets... hide controls if piers don't have it
   const CPierData2* pPrevPier = pParent->GetBridgeDescription()->GetPier(m_PrevPierIdx);
   const CPierData2* pNextPier = pParent->GetBridgeDescription()->GetPier(m_NextPierIdx);
   m_bHasSlabOffset = pPrevPier->HasSlabOffset() || pNextPier->HasSlabOffset();
   if (pPrevPier->HasSlabOffset())
   {
      m_SlabOffset[pgsTypes::metStart] = pPrevPier->GetSlabOffset(pgsTypes::Ahead, m_SlabOffsetType == pgsTypes::sotSegment ? true : false);
   }
   else
   {
      GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_START_SLAB_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);
   }

   if (pNextPier->HasSlabOffset())
   {
      m_SlabOffset[pgsTypes::metEnd] = pNextPier->GetSlabOffset(pgsTypes::Back, m_SlabOffsetType == pgsTypes::sotSegment ? true : false);
   }
   else
   {
      GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_END_SLAB_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);
   }

   if (!m_bHasSlabOffset)
   {
      GetDlgItem(IDC_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SLAB_OFFSET_TYPE)->ShowWindow(SW_HIDE);
   }

   GET_IFACE2(pBroker,ISpecification, pSpec );

   m_bCanAssumedExcessCamberInputBeEnabled = pSpec->IsAssumedExcessCamberInputEnabled();
   
   CPropertyPage::OnInitDialog();

   CComboBox* pAssumedExcessCamberCB = (CComboBox*)this->GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPE);

   if (m_bCanAssumedExcessCamberInputBeEnabled)
   {
      if (m_InitialAssumedExcessCamberType == pgsTypes::aecGirder)
   {
         m_ctrlAssumedExcessCamber.ShowDefaultWhenDisabled(FALSE);
      }
      else
      {
         m_ctrlAssumedExcessCamber.ShowDefaultWhenDisabled(TRUE);
      }

      pgsTypes::AssumedExcessCamberType AssumedExcessCamberType = pParent->m_BridgeDesc.GetAssumedExcessCamberType();
      ATLASSERT(m_InitialAssumedExcessCamberType == AssumedExcessCamberType); // if this is not true, then 

      if (AssumedExcessCamberType == pgsTypes::aecBridge || AssumedExcessCamberType == pgsTypes::aecSpan)
      {
         pAssumedExcessCamberCB->AddString(_T("The same Assumed Excess Camber is used for the entire bridge"));
         pAssumedExcessCamberCB->AddString(_T("Assumed Excess Camber is defined span by span"));
         pAssumedExcessCamberCB->SetCurSel(AssumedExcessCamberType == pgsTypes::aecBridge ? 0 : 1);
      }
      else if (AssumedExcessCamberType == pgsTypes::aecGirder)
      {
         pAssumedExcessCamberCB->AddString(_T("A unique assumed excess camber is used for every girder"));
         pAssumedExcessCamberCB->AddString(_T("Assumed Excess Camber is defined span by span"));
         pAssumedExcessCamberCB->SetCurSel(0);
      }
      else
      {
         ATLASSERT(0);
      }
   }
   else
   {
      m_ctrlAssumedExcessCamber.EnableWindow(FALSE);
      pAssumedExcessCamberCB->EnableWindow(FALSE);
   }

   const CSpanData2* pPrevSpan = pParent->m_pSpanData->GetPrevPier()->GetPrevSpan();
   const CSpanData2* pNextSpan = pParent->m_pSpanData->GetNextPier()->GetNextSpan();

   CString strPrevPierType(pParent->m_pSpanData->GetPrevPier()->IsAbutment() ? _T("Abutment") : _T("Pier"));
   CString strNextPierType(pParent->m_pSpanData->GetNextPier()->IsAbutment() ? _T("Abutment") : _T("Pier"));

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %d"),LABEL_SPAN(m_SpanIdx));
   GetDlgItem(IDC_SPAN_LABEL)->SetWindowText(strSpanLabel);

   CString strSpanLengthBasis;
   strSpanLengthBasis.Format(_T("Span length is measured along the alignment between the %s Line at %s %d and the %s Line at %s %d."),strPrevPierType,strPrevPierType,LABEL_PIER(m_PrevPierIdx),strNextPierType,strNextPierType,LABEL_PIER(m_NextPierIdx));
   GetDlgItem(IDC_SPAN_LENGTH_BASIS)->SetWindowText(strSpanLengthBasis);

   CString strSpanLengthNote;
   strSpanLengthNote.Format(_T("The length of Span %d is changed by moving all piers after %s %d. Only the length of Span %d is changed."),LABEL_SPAN(m_SpanIdx),strPrevPierType,LABEL_PIER(m_PrevPierIdx),LABEL_SPAN(m_SpanIdx));
   GetDlgItem(IDC_SPAN_LENGTH_NOTE)->SetWindowText(strSpanLengthNote);

   UpdateSlabOffsetWindowState();

   UpdateAssumedExcessCamberWindowState();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanLayoutPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_SPANDETAILS_GENERAL );
}

void CSpanLayoutPage::OnChangingSlabOffset()
{
   m_PrevSlabOffsetType = GetCurrentSlabOffsetType();
}

void CSpanLayoutPage::OnChangeSlabOffset()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ASSERT(m_bHasSlabOffset);

   pgsTypes::SlabOffsetType slabOffsetType = GetCurrentSlabOffsetType();
  
   if (slabOffsetType == pgsTypes::sotBridge)
   {
      // just switch to slab offset by bridge
      // need to deal with possibly 2 values that aren't equal mapping to a single value

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
      const CPierData2* pPrevPier = pParent->GetBridgeDescription()->GetPier(m_PrevPierIdx);
      const CPierData2* pNextPier = pParent->GetBridgeDescription()->GetPier(m_NextPierIdx);
      if (pPrevPier->HasSlabOffset() && pNextPier->HasSlabOffset())
      {
         // both piers can have slab offsets so this gives rise to the possibility that
         // there are two unequal values mapping into a single value for the bridge

         // get current values out of the controls
         std::array<Float64,2> slabOffset;
         CDataExchange dx(this,TRUE);
         DDX_UnitValueAndTag(&dx, IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT, slabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit());
         DDX_UnitValueAndTag(&dx, IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,   slabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit());
         if (!IsEqual(slabOffset[pgsTypes::metStart], slabOffset[pgsTypes::metEnd]))
         {
            // slab offsets are different... which one should we use? Ask the user.
            CSelectItemDlg dlg;
            dlg.m_ItemIdx = 0;
            dlg.m_strTitle = _T("Select Slab Offset");
            dlg.m_strLabel = _T("A single slab offset will be used for the entire bridge. Select a value.");
                  
            CString strItems;
            strItems.Format(_T("Start of Span (%s)\nEnd of Span (%s)"),
                              ::FormatDimension(slabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit()),
                              ::FormatDimension(slabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit()));

            dlg.m_strItems = strItems;
            Float64 slab_offset;
            if ( dlg.DoModal() == IDOK )
            {
               slab_offset = slabOffset[dlg.m_ItemIdx == 0 ? pgsTypes::metStart : pgsTypes::metEnd];
            }
            else
            {
               // user cancelled... get the heck outta here
               ComboBoxSelectByItemData(this, IDC_SLAB_OFFSET_TYPE, m_PrevSlabOffsetType);
               return;
            }

            Float64 minSlabOffset = pParent->GetBridgeDescription()->GetMinSlabOffset();
            if (::IsLT(slab_offset, minSlabOffset))
            {
               CDataExchange dx(this, TRUE);
               dx.PrepareEditCtrl(pPrevPier->HasSlabOffset() ? IDC_START_SLAB_OFFSET : IDC_END_SLAB_OFFSET);
               CString msg;
               msg.Format(_T("The slab offset must be at least equal to the slab depth of %s"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));
               AfxMessageBox(msg, MB_ICONERROR | MB_OK);

               ComboBoxSelectByItemData(this, IDC_SLAB_OFFSET_TYPE, m_PrevSlabOffsetType);

               dx.Fail();
            }

            // put the data back in the controls
            dx.m_bSaveAndValidate = FALSE;
            DDX_UnitValueAndTag(&dx, IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT, slab_offset, pDisplayUnits->GetComponentDimUnit());
            DDX_UnitValueAndTag(&dx, IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,   slab_offset, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         // this may look silly, but it updates the default value for the edit control to the current value
         // and the default value is displayed when the control is disabled, which will happen when we
         // call UpdateSlabOffsetWindowState below
         UINT nIDC, nIDCUnit;
         if (pPrevPier->HasSlabOffset())
         {
            nIDC = IDC_START_SLAB_OFFSET;
            nIDCUnit = IDC_START_SLAB_OFFSET_UNIT;
         }
         else
         {
            ASSERT(pNextPier->HasSlabOffset());
            nIDC = IDC_END_SLAB_OFFSET;
            nIDCUnit = IDC_END_SLAB_OFFSET_UNIT;
         }

         Float64 slab_offset;
         CDataExchange dx(this, TRUE);
         DDX_UnitValueAndTag(&dx, nIDC, nIDCUnit, slab_offset, pDisplayUnits->GetComponentDimUnit());

         Float64 minSlabOffset = pParent->GetBridgeDescription()->GetMinSlabOffset();
         if (::IsLT(slab_offset,minSlabOffset))
         {
            CDataExchange dx(this, TRUE);
            dx.PrepareEditCtrl(nIDC);
            CString msg;
            msg.Format(_T("The slab offset must be at least equal to the slab depth of %s"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg, MB_ICONERROR | MB_OK);

            ComboBoxSelectByItemData(this, IDC_SLAB_OFFSET_TYPE, m_PrevSlabOffsetType);

            dx.Fail();
         }

         dx.m_bSaveAndValidate = FALSE;
         DDX_UnitValueAndTag(&dx, nIDC, nIDCUnit, slab_offset, pDisplayUnits->GetComponentDimUnit());
      }
   }
   UpdateSlabOffsetWindowState();
}

pgsTypes::SlabOffsetType CSpanLayoutPage::GetCurrentSlabOffsetType()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SLAB_OFFSET_TYPE);
   int curSel = pCB->GetCurSel();
   pgsTypes::SlabOffsetType slabOffsetType = (pgsTypes::SlabOffsetType)(pCB->GetItemData(curSel));
   return slabOffsetType;
}

void CSpanLayoutPage::UpdateSlabOffsetWindowState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   pgsTypes::SlabOffsetType slabOffsetType = GetCurrentSlabOffsetType();

   BOOL bEnable = TRUE;
   if (deckType == pgsTypes::sdtNone || slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotSegment )
   {
      bEnable = FALSE;
   }

   m_ctrlStartSlabOffset.EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);

   m_ctrlEndSlabOffset.EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);
}

void CSpanLayoutPage::OnCbnSelchangeAssumedExcessCamberCombo()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::AssumedExcessCamberType AssumedExcessCamberType = pParent->m_BridgeDesc.GetAssumedExcessCamberType();

   if ( m_InitialAssumedExcessCamberType == pgsTypes::aecBridge || m_InitialAssumedExcessCamberType == pgsTypes::aecSpan )
   {
      // if camber was bridge or span when the dialog was created, toggle between
      // bridge and span mode
      if ( AssumedExcessCamberType == pgsTypes::aecSpan )
      {
         pParent->m_BridgeDesc.SetAssumedExcessCamberType(pgsTypes::aecBridge);
      }
      else
      {
         pParent->m_BridgeDesc.SetAssumedExcessCamberType(pgsTypes::aecSpan);
      }
   }
   else
   {
      // if camber was girder when the dialog was created, toggle between
      // girder and span
      if ( AssumedExcessCamberType == pgsTypes::aecSpan )
      {
         // going from pier to girder so both ends of girder are the same. default values can be shown
         pParent->m_BridgeDesc.SetAssumedExcessCamberType(pgsTypes::aecGirder);
         m_ctrlAssumedExcessCamber.ShowDefaultWhenDisabled(TRUE);
      }
      else
      {
         pParent->m_BridgeDesc.SetAssumedExcessCamberType(pgsTypes::aecSpan);
      }
   }

   UpdateAssumedExcessCamberWindowState();
}

void CSpanLayoutPage::UpdateAssumedExcessCamberWindowState()
{
   if (m_bCanAssumedExcessCamberInputBeEnabled)
   {
      CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
      pgsTypes::AssumedExcessCamberType AssumedExcessCamberType = pParent->m_BridgeDesc.GetAssumedExcessCamberType();

      BOOL bEnable = TRUE;
      if (AssumedExcessCamberType == pgsTypes::aecBridge || AssumedExcessCamberType == pgsTypes::aecGirder)
      {
         bEnable = FALSE;
      }

      m_ctrlAssumedExcessCamber.EnableWindow(bEnable);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNIT)->EnableWindow(bEnable);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL)->EnableWindow(bEnable);
   }
   else
   {
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_TYPE)->EnableWindow(FALSE);
      m_ctrlAssumedExcessCamber.EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL2)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL)->EnableWindow(FALSE);
   }
}

