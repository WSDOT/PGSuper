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

// SpanLayoutPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "SpanLayoutPage.h"
#include "SpanDetailsDlg.h"
#include "SelectItemDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>
#include <IFace\Project.h>

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
   DDX_Control(pDX, IDC_ASSEXCESSCAMBER, m_ctrlAssExcessCamber);

   // display units no matter what
   DDX_Tag( pDX, IDC_ASSEXCESSCAMBER_UNIT,pDisplayUnits->GetComponentDimUnit() );


   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );

   DDX_UnitValueAndTag(pDX,IDC_SPAN_LENGTH,IDC_SPAN_LENGTH_UNIT,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueGreaterThanZero(pDX,IDC_SPAN_LENGTH,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());

   PierIndexType startPierIdx = pParent->m_pSpanData->GetPrevPier()->GetIndex();
   PierIndexType endPierIdx   = pParent->m_pSpanData->GetNextPier()->GetIndex();
   CGirderGroupData* pStartGroup = pParent->m_pSpanData->GetPrevPier()->GetGirderGroup(pgsTypes::Ahead);
   CGirderGroupData* pEndGroup   = pParent->m_pSpanData->GetNextPier()->GetGirderGroup(pgsTypes::Back);
   Float64 slabOffset[2] = {pStartGroup->GetSlabOffset(startPierIdx,0,m_InitialSlabOffsetType == pgsTypes::sotGirder ? true : false),
                            pEndGroup->GetSlabOffset(endPierIdx,0,m_InitialSlabOffsetType == pgsTypes::sotGirder ? true : false)};

   DDX_UnitValueAndTag(pDX,IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT,slabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,  slabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit());

   Float64 AssExcessCamber = 0;
   if (m_bCanAssExcessCamberInputBeEnabled)
   {
      AssExcessCamber = pParent->m_pSpanData->GetAssExcessCamber(0);
      DDX_UnitValueAndTag(pDX, IDC_ASSEXCESSCAMBER, IDC_ASSEXCESSCAMBER_UNIT, AssExcessCamber, pDisplayUnits->GetComponentDimUnit());
   }

   bool bHasCantilever[2] = { false,false };
   Float64 cantileverLength[2] = { 0.0,0.0 };
   if (m_bHasCantilevers)
   {
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
         bHasCantilever[end] = pParent->m_pSpanData->GetPier(end)->HasCantilever();
         cantileverLength[end] = pParent->m_pSpanData->GetPier(end)->GetCantileverLength();
      }

      DDX_Check_Bool(pDX, IDC_START_CANTILEVER, bHasCantilever[pgsTypes::metStart]);
      DDX_UnitValueAndTag(pDX, IDC_START_CANTILEVER_LENGTH, IDC_START_CANTILEVER_UNIT, cantileverLength[pgsTypes::metStart], pDisplayUnits->GetSpanLengthUnit());
      if (bHasCantilever[pgsTypes::metStart])
      {
         DDV_UnitValueGreaterThanZero(pDX, IDC_START_CANTILEVER_LENGTH, cantileverLength[pgsTypes::metStart], pDisplayUnits->GetSpanLengthUnit());
      }

      DDX_Check_Bool(pDX, IDC_END_CANTILEVER, bHasCantilever[pgsTypes::metEnd]);
      DDX_UnitValueAndTag(pDX, IDC_END_CANTILEVER_LENGTH, IDC_END_CANTILEVER_UNIT, cantileverLength[pgsTypes::metEnd], pDisplayUnits->GetSpanLengthUnit());
      if (bHasCantilever[pgsTypes::metEnd])
      {
         DDV_UnitValueGreaterThanZero(pDX, IDC_END_CANTILEVER_LENGTH, cantileverLength[pgsTypes::metEnd], pDisplayUnits->GetSpanLengthUnit());
      }
   }

   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.SetSpanLength(pParent->m_pSpanData->GetIndex(),m_SpanLength);

      if (pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
      {
         Float64 minA = pParent->m_BridgeDesc.GetDeckDescription()->GrossDepth;
         if (pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeSIP)
         {
            minA += pParent->m_BridgeDesc.GetDeckDescription()->PanelDepth;
         }

         cmpdim.SetValue(minA);
         CString strMinValError;
         strMinValError.Format(_T("Slab Offset value must be greater or equal to gross slab depth (%.4f %s)"), cmpdim.GetValue(true), cmpdim.GetUnitTag().c_str());

         if (slabOffset[pgsTypes::metStart] < minA)
         {
            pDX->PrepareCtrl(IDC_START_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         if (pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotPier && slabOffset[pgsTypes::metEnd] < minA)
         {
            pDX->PrepareCtrl(IDC_END_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         // Slab offset
         if (pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotBridge)
         {
            pParent->m_BridgeDesc.SetSlabOffset(slabOffset[pgsTypes::metStart]);
         }
         else if (pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotPier)
         {
            pStartGroup->SetSlabOffset(startPierIdx, slabOffset[pgsTypes::metStart]);
            pEndGroup->SetSlabOffset(endPierIdx, slabOffset[pgsTypes::metEnd]);
         }
         else
         {
            if (m_InitialSlabOffsetType != pgsTypes::sotGirder)
            {
               // Slab offset started off as Bridge or Pier and now it is Girder... this means the
               // slab offset applies to all girders in the Span
               CGirderGroupData* pGroup = pParent->m_BridgeDesc.GetGirderGroup(pParent->m_pSpanData);
               GirderIndexType nGirders = pGroup->GetGirderCount();
               for (PierIndexType pierIdx = m_PrevPierIdx; pierIdx <= m_NextPierIdx; pierIdx++)
               {
                  for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                  {
                     pGroup->SetSlabOffset(pierIdx, gdrIdx, slabOffset[pgsTypes::metStart]);
                  }
               }
            }
         }

         // excess camber
         if (m_bCanAssExcessCamberInputBeEnabled)
         {
            if (pParent->m_BridgeDesc.GetAssExcessCamberType() == pgsTypes::aecBridge)
            {
               pParent->m_BridgeDesc.SetAssExcessCamber(AssExcessCamber);
            }
            else if (pParent->m_BridgeDesc.GetAssExcessCamberType() == pgsTypes::aecSpan)
            {
               pParent->m_pSpanData->SetAssExcessCamber(AssExcessCamber);
            }
            else
            {
               if (m_InitialAssExcessCamberType != pgsTypes::aecGirder)
               {
                  GirderIndexType nGirders = pParent->m_pSpanData->GetGirderCount();
                  for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                  {
                     pParent->m_pSpanData->SetAssExcessCamber(gdrIdx, AssExcessCamber);
                  }
               }
            }
         }

         if (m_bHasCantilevers)
         {
            for (int i = 0; i < 2; i++)
            {
               pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
               pParent->m_pSpanData->GetPier(end)->HasCantilever(bHasCantilever[end]);
               pParent->m_pSpanData->GetPier(end)->SetCantileverLength(cantileverLength[end]);
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
   ON_BN_CLICKED(IDC_START_CANTILEVER, &CSpanLayoutPage::OnBnClickedStartCantilever)
   ON_BN_CLICKED(IDC_END_CANTILEVER, &CSpanLayoutPage::OnBnClickedEndCantilever)
   ON_CBN_SELCHANGE(IDC_ASSEXCESSCAMBER_COMBO, &CSpanLayoutPage::OnCbnSelchangeAssExcessCamberCombo)
   ON_CBN_SELCHANGE(IDC_SLABOFFSET_COMBO, &CSpanLayoutPage::OnChangeSlabOffset)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage message handlers
void CSpanLayoutPage::Init(CSpanDetailsDlg* pParent)
{
   m_SpanLength = pParent->m_pSpanData->GetSpanLength();
   m_InitialSlabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
   m_InitialAssExcessCamberType = pParent->m_BridgeDesc.GetAssExcessCamberType();
}

BOOL CSpanLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   m_SpanIdx = pParent->m_pSpanData->GetIndex();
   m_PrevPierIdx = (PierIndexType)m_SpanIdx;
   m_NextPierIdx = m_PrevPierIdx+1;

   if ( m_InitialSlabOffsetType == pgsTypes::sotGirder )
   {
      m_ctrlStartSlabOffset.ShowDefaultWhenDisabled(FALSE);
      m_ctrlEndSlabOffset.ShowDefaultWhenDisabled(FALSE);
   }
   else
   {
      m_ctrlStartSlabOffset.ShowDefaultWhenDisabled(TRUE);
      m_ctrlEndSlabOffset.ShowDefaultWhenDisabled(TRUE);
   }

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      ShowCantilevers(pParent->m_pSpanData->GetPrevPier()->GetPrevSpan() == nullptr ? TRUE : FALSE,
                      pParent->m_pSpanData->GetNextPier()->GetNextSpan() == nullptr ? TRUE : FALSE);
   }
   else
   {
      ShowCantilevers(FALSE,FALSE);
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISpecification, pSpec );

   m_bCanAssExcessCamberInputBeEnabled = pSpec->IsAssExcessCamberInputEnabled();
   
   CPropertyPage::OnInitDialog();

   CComboBox* pAssExcessCamberCB = (CComboBox*)this->GetDlgItem(IDC_ASSEXCESSCAMBER_COMBO);

   if (m_bCanAssExcessCamberInputBeEnabled)
   {
      if (m_InitialAssExcessCamberType == pgsTypes::aecGirder)
      {
         m_ctrlAssExcessCamber.ShowDefaultWhenDisabled(FALSE);
      }
      else
      {
         m_ctrlAssExcessCamber.ShowDefaultWhenDisabled(TRUE);
      }

      pgsTypes::AssExcessCamberType AssExcessCamberType = pParent->m_BridgeDesc.GetAssExcessCamberType();
      ATLASSERT(m_InitialAssExcessCamberType == AssExcessCamberType); // if this is not true, then 

      if (AssExcessCamberType == pgsTypes::aecBridge || AssExcessCamberType == pgsTypes::aecSpan)
      {
         pAssExcessCamberCB->AddString(_T("The same Assumed Excess Camber is used for the entire bridge"));
         pAssExcessCamberCB->AddString(_T("Assumed Excess Camber is defined span by span"));
         pAssExcessCamberCB->SetCurSel(AssExcessCamberType == pgsTypes::aecBridge ? 0 : 1);
      }
      else if (AssExcessCamberType == pgsTypes::aecGirder)
      {
         pAssExcessCamberCB->AddString(_T("A unique assumed excess camber is used for every girder"));
         pAssExcessCamberCB->AddString(_T("Assumed Excess Camber is defined span by span"));
         pAssExcessCamberCB->SetCurSel(0);
      }
      else
      {
         ATLASSERT(0);
      }
   }
   else
   {
      m_ctrlAssExcessCamber.EnableWindow(FALSE);
      pAssExcessCamberCB->EnableWindow(FALSE);
   }

   CComboBox* pSlabOffsetCB = (CComboBox*)this->GetDlgItem(IDC_SLABOFFSET_COMBO);

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   if (deckType != pgsTypes::sdtNone)
   {

      pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

      if (slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotPier)
      {
         pSlabOffsetCB->AddString(_T("The same slab offset is used for the entire bridge"));
         pSlabOffsetCB->AddString(_T("Slab offset is defined span by span"));
         pSlabOffsetCB->SetCurSel(slabOffsetType == pgsTypes::sotBridge ? 0:1);
      }
      else if ( slabOffsetType == pgsTypes::sotGirder )
      {
         pSlabOffsetCB->AddString(_T("A unique slab offset is used for every girder"));
         pSlabOffsetCB->AddString(_T("Slab offset is defined span by span"));
         pSlabOffsetCB->SetCurSel(0);
      }
      else
      {
         ATLASSERT(0);
      }
   }
   else
   {
      pSlabOffsetCB->EnableWindow(FALSE);
      m_ctrlStartSlabOffset.EnableWindow(FALSE);
      m_ctrlEndSlabOffset.EnableWindow(FALSE);
      this->GetDlgItem(IDC_SLABOFFSET_LABEL)->EnableWindow(FALSE);
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

   UpdateAssExcessCamberWindowState();

   OnBnClickedStartCantilever();
   OnBnClickedEndCantilever();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanLayoutPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_SPANDETAILS_GENERAL );
}

void CSpanLayoutPage::OnChangeSlabOffset()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

   if ( m_InitialSlabOffsetType == pgsTypes::sotBridge || m_InitialSlabOffsetType == pgsTypes::sotPier )
   {
      // if slab offset was bridge or pier when the dialog was created, toggle between
      // bridge and pier mode
      if ( slabOffsetType == pgsTypes::sotPier )
      {
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotBridge);
      }
      else
      {
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotPier);
      }
   }
   else
   {
      // if slab offset was girder when the dialog was created, toggle between
      // girder and pier
      if ( slabOffsetType == pgsTypes::sotPier )
      {
         // going from pier to girder so both ends of girder are the same. default values can be shown
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotGirder);
         m_ctrlStartSlabOffset.ShowDefaultWhenDisabled(TRUE);
         m_ctrlEndSlabOffset.ShowDefaultWhenDisabled(TRUE);
      }
      else
      {
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotPier);
      }
   }

   if ( slabOffsetType == pgsTypes::sotPier && pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotBridge )
   {
      // going from span-by-span to one for the entire bridge
      // need to check the span values and if they are different, ask the user which one is to
      // be used for the entire bridge

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      // get current values out of the controls
      Float64 slabOffset[2];
      CDataExchange dx(this,TRUE);
      DDX_UnitValueAndTag(&dx, IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT, slabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,   slabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit());

      // take start value as default
      Float64 slab_offset = slabOffset[pgsTypes::metStart];

      // check if start/end values are equal
      if ( !IsEqual(slabOffset[pgsTypes::metStart],slabOffset[pgsTypes::metEnd]) )
      {
         // nope... make the user select which slab offset to use
         CSelectItemDlg dlg;
         dlg.m_ItemIdx = 0;
         dlg.m_strTitle = _T("Select Slab Offset");
         dlg.m_strLabel = _T("A single slab offset will be used for the entire bridge. Select a value.");
         
         CString strItems;
         strItems.Format(_T("Start of Span (%s)\nEnd of Span (%s)"),
                         ::FormatDimension(slabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit()),
                         ::FormatDimension(slabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit()));

         dlg.m_strItems = strItems;
         if ( dlg.DoModal() == IDOK )
         {
            if ( dlg.m_ItemIdx == 0 )
            {
               slab_offset = slabOffset[pgsTypes::metStart];
            }
            else
            {
               slab_offset = slabOffset[pgsTypes::metEnd];
            }
         }
         else
         {
            // user cancelled... get the heck outta here
            return;
         }
      }

      // put the data back in the controls
      dx.m_bSaveAndValidate = FALSE;
      DDX_UnitValueAndTag(&dx, IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT, slab_offset, pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,   slab_offset, pDisplayUnits->GetComponentDimUnit());
   }

   UpdateSlabOffsetWindowState();
}

void CSpanLayoutPage::UpdateSlabOffsetWindowState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   BOOL bEnable = TRUE;
   if (deckType == pgsTypes::sdtNone || slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotGirder )
   {
      bEnable = FALSE;
   }

   m_ctrlStartSlabOffset.EnableWindow(bEnable);
   GetDlgItem(IDC_START)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);
   m_ctrlEndSlabOffset.EnableWindow(bEnable);
   GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_END)->EnableWindow(bEnable);
}

void CSpanLayoutPage::OnCbnSelchangeAssExcessCamberCombo()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::AssExcessCamberType AssExcessCamberType = pParent->m_BridgeDesc.GetAssExcessCamberType();

   if ( m_InitialAssExcessCamberType == pgsTypes::aecBridge || m_InitialAssExcessCamberType == pgsTypes::aecSpan )
   {
      // if camber was bridge or span when the dialog was created, toggle between
      // bridge and span mode
      if ( AssExcessCamberType == pgsTypes::aecSpan )
      {
         pParent->m_BridgeDesc.SetAssExcessCamberType(pgsTypes::aecBridge);
      }
      else
      {
         pParent->m_BridgeDesc.SetAssExcessCamberType(pgsTypes::aecSpan);
      }
   }
   else
   {
      // if camber was girder when the dialog was created, toggle between
      // girder and span
      if ( AssExcessCamberType == pgsTypes::aecSpan )
      {
         // going from pier to girder so both ends of girder are the same. default values can be shown
         pParent->m_BridgeDesc.SetAssExcessCamberType(pgsTypes::aecGirder);
         m_ctrlAssExcessCamber.ShowDefaultWhenDisabled(TRUE);
      }
      else
      {
         pParent->m_BridgeDesc.SetAssExcessCamberType(pgsTypes::aecSpan);
      }
   }

   UpdateAssExcessCamberWindowState();
}

void CSpanLayoutPage::UpdateAssExcessCamberWindowState()
{
   if (m_bCanAssExcessCamberInputBeEnabled)
   {
      CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
      pgsTypes::AssExcessCamberType AssExcessCamberType = pParent->m_BridgeDesc.GetAssExcessCamberType();

      BOOL bEnable = TRUE;
      if (AssExcessCamberType == pgsTypes::aecBridge || AssExcessCamberType == pgsTypes::aecGirder)
      {
         bEnable = FALSE;
      }

      m_ctrlAssExcessCamber.EnableWindow(bEnable);
      GetDlgItem(IDC_ASSEXCESSCAMBER_UNIT)->EnableWindow(bEnable);
      GetDlgItem(IDC_ASSEXCESSCAMBER_LABEL)->EnableWindow(bEnable);
   }
   else
   {
      GetDlgItem(IDC_ASSEXCESSCAMBER_COMBO)->EnableWindow(FALSE);
      m_ctrlAssExcessCamber.EnableWindow(FALSE);
      GetDlgItem(IDC_ASSEXCESSCAMBER_UNIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSEXCESSCAMBER_LABEL2)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSEXCESSCAMBER_LABEL)->EnableWindow(FALSE);
   }
}

void CSpanLayoutPage::ShowCantilevers(BOOL bShowStart,BOOL bShowEnd)
{
   GetDlgItem(IDC_START_CANTILEVER)       ->ShowWindow(bShowStart == TRUE ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_START_CANTILEVER_LABEL) ->ShowWindow(bShowStart == TRUE ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_START_CANTILEVER_LENGTH)->ShowWindow(bShowStart == TRUE ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_START_CANTILEVER_UNIT)  ->ShowWindow(bShowStart == TRUE ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_END_CANTILEVER)       ->ShowWindow(bShowEnd == TRUE ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_END_CANTILEVER_LABEL) ->ShowWindow(bShowEnd == TRUE ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_END_CANTILEVER_LENGTH)->ShowWindow(bShowEnd == TRUE ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_END_CANTILEVER_UNIT)  ->ShowWindow(bShowEnd == TRUE ? SW_SHOW : SW_HIDE);

   if ( bShowStart == FALSE && bShowEnd == FALSE )
   {
      GetDlgItem(IDC_CANTILEVER_GROUP)->ShowWindow(SW_HIDE);
   }

   m_bHasCantilevers = (bShowStart == TRUE || bShowEnd == TRUE) ? true : false;
}

void CSpanLayoutPage::OnBnClickedStartCantilever()
{
   // TODO: Add your control notification handler code here
   BOOL bEnable = ( IsDlgButtonChecked(IDC_START_CANTILEVER) == BST_CHECKED ? TRUE : FALSE);
   GetDlgItem(IDC_START_CANTILEVER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_CANTILEVER_LENGTH)->EnableWindow(bEnable);
   GetDlgItem(IDC_START_CANTILEVER_UNIT)->EnableWindow(bEnable);
}

void CSpanLayoutPage::OnBnClickedEndCantilever()
{
   // TODO: Add your control notification handler code here
   BOOL bEnable = ( IsDlgButtonChecked(IDC_END_CANTILEVER) == BST_CHECKED ? TRUE : FALSE);
   GetDlgItem(IDC_END_CANTILEVER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_CANTILEVER_LENGTH)->EnableWindow(bEnable);
   GetDlgItem(IDC_END_CANTILEVER_UNIT)->EnableWindow(bEnable);
}

