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
#include "HtmlHelp\HelpTopics.hh"

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
}

CSpanLayoutPage::~CSpanLayoutPage()
{
}

void CSpanLayoutPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_HYPERLINK,    m_SlabOffsetHyperLink);
   DDX_Control(pDX, IDC_START_SLAB_OFFSET, m_ctrlStartSlabOffset);
   DDX_Control(pDX, IDC_END_SLAB_OFFSET,   m_ctrlEndSlabOffset);

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag(pDX,IDC_SPAN_LENGTH,IDC_SPAN_LENGTH_UNIT,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());

   PierIndexType startPierIdx = pParent->m_pSpanData->GetPrevPier()->GetIndex();
   PierIndexType endPierIdx   = pParent->m_pSpanData->GetNextPier()->GetIndex();
   CGirderGroupData* pStartGroup = pParent->m_pSpanData->GetPrevPier()->GetGirderGroup(pgsTypes::Ahead);
   CGirderGroupData* pEndGroup   = pParent->m_pSpanData->GetNextPier()->GetGirderGroup(pgsTypes::Back);
   Float64 slabOffset[2] = {pStartGroup->GetSlabOffset(startPierIdx,0,m_InitialSlabOffsetType == pgsTypes::sotGirder ? true : false),
                            pEndGroup->GetSlabOffset(endPierIdx,0,m_InitialSlabOffsetType == pgsTypes::sotGirder ? true : false)};

   DDX_UnitValueAndTag(pDX,IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT,slabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,  slabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit());

   bool bHasCantilever[2];
   Float64 cantileverLength[2];
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
      bHasCantilever[end] = pParent->m_pSpanData->GetPier(end)->HasCantilever();
      cantileverLength[end] = pParent->m_pSpanData->GetPier(end)->GetCantileverLength();
   }

   DDX_Check_Bool(pDX,IDC_START_CANTILEVER,bHasCantilever[pgsTypes::metStart]);
   DDX_UnitValueAndTag(pDX,IDC_START_CANTILEVER_LENGTH,IDC_START_CANTILEVER_UNIT,cantileverLength[pgsTypes::metStart],pDisplayUnits->GetSpanLengthUnit());
   if ( bHasCantilever[pgsTypes::metStart] )
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_START_CANTILEVER_LENGTH,cantileverLength[pgsTypes::metStart],pDisplayUnits->GetSpanLengthUnit());
   }

   DDX_Check_Bool(pDX,IDC_END_CANTILEVER,bHasCantilever[pgsTypes::metEnd]);
   DDX_UnitValueAndTag(pDX,IDC_END_CANTILEVER_LENGTH,IDC_END_CANTILEVER_UNIT,cantileverLength[pgsTypes::metEnd],pDisplayUnits->GetSpanLengthUnit());
   if ( bHasCantilever[pgsTypes::metEnd] )
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_END_CANTILEVER_LENGTH,cantileverLength[pgsTypes::metEnd],pDisplayUnits->GetSpanLengthUnit());
   }

   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.SetSpanLength(pParent->m_pSpanData->GetIndex(),m_SpanLength);


      if ( pParent->m_BridgeDesc.GetSlabOffsetType()== pgsTypes::sotBridge )
      {
         pParent->m_BridgeDesc.SetSlabOffset(slabOffset[pgsTypes::metStart]);
      }
      else if ( pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotPier )
      {
         pStartGroup->SetSlabOffset(startPierIdx,slabOffset[pgsTypes::metStart]);
         pEndGroup->SetSlabOffset(  endPierIdx,  slabOffset[pgsTypes::metEnd]);
      }
      else
      {
         if ( m_InitialSlabOffsetType != pgsTypes::sotGirder )
         {
            // Slab offset started off as Bridge or Pier and now it is Girder... this means the
            // slab offset applies to all girders in the Span
            CGirderGroupData* pGroup = pParent->m_BridgeDesc.GetGirderGroup(pParent->m_pSpanData);
            GirderIndexType nGirders = pGroup->GetGirderCount();
            for ( PierIndexType pierIdx = m_PrevPierIdx; pierIdx <= m_NextPierIdx; pierIdx++ )
            {
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  pGroup->SetSlabOffset(pierIdx,gdrIdx,slabOffset[pgsTypes::metStart]);
               }
            }
         }
      }

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
         pParent->m_pSpanData->GetPier(end)->HasCantilever(bHasCantilever[end]);
         pParent->m_pSpanData->GetPier(end)->SetCantileverLength(cantileverLength[end]);
      }
   }
}


BEGIN_MESSAGE_MAP(CSpanLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanLayoutPage)
	ON_WM_CTLCOLOR()
	ON_COMMAND(ID_HELP, OnHelp)
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffset,OnChangeSlabOffset)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_START_CANTILEVER, &CSpanLayoutPage::OnBnClickedStartCantilever)
   ON_BN_CLICKED(IDC_END_CANTILEVER, &CSpanLayoutPage::OnBnClickedEndCantilever)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage message handlers
void CSpanLayoutPage::Init(CSpanDetailsDlg* pParent)
{
   m_SpanLength = pParent->m_pSpanData->GetSpanLength();
   m_InitialSlabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
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
      ShowCantilevers(pParent->m_pSpanData->GetPrevPier()->GetPrevSpan() == NULL ? TRUE : FALSE,
                      pParent->m_pSpanData->GetNextPier()->GetNextSpan() == NULL ? TRUE : FALSE);
   }
   else
   {
      ShowCantilevers(FALSE,FALSE);
   }
   
   CPropertyPage::OnInitDialog();

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

   OnBnClickedStartCantilever();
   OnBnClickedEndCantilever();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanLayoutPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPANDETAILS_GENERAL );
}

HBRUSH CSpanLayoutPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_SLAB_OFFSET_NOTE:
      pDC->SetTextColor(HYPERLINK_COLOR);
      break;
   };

   return hbr;
}

LRESULT CSpanLayoutPage::OnChangeSlabOffset(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

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
            return 0;
         }
      }

      // put the data back in the controls
      dx.m_bSaveAndValidate = FALSE;
      DDX_UnitValueAndTag(&dx, IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT, slab_offset, pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_END_SLAB_OFFSET,  IDC_END_SLAB_OFFSET_UNIT,   slab_offset, pDisplayUnits->GetComponentDimUnit());
   }

   UpdateSlabOffsetWindowState();

   return 0;
}

void CSpanLayoutPage::UpdateSlabOffsetHyperLinkText()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

   if (slabOffsetType == pgsTypes::sotBridge )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("The same slab offset is used for the entire bridge"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define slab offset span by span"));
   }
   else if ( slabOffsetType == pgsTypes::sotGirder )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A unique slab offset is used for every girder"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define slab offset span by span"));
   }
   else
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("Slab offset is defined span by span"));

      if ( m_InitialSlabOffsetType == pgsTypes::sotBridge )
      {
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this slab offset for the entire bridge"));
      }
      else if ( m_InitialSlabOffsetType == pgsTypes::sotGirder )
      {
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this slab offset for every girder in this span"));
      }
   }
}

void CSpanLayoutPage::UpdateSlabOffsetWindowState()
{
   UpdateSlabOffsetHyperLinkText();

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

   BOOL bEnable = TRUE;
   if ( slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotGirder )
   {
      bEnable = FALSE;
   }

   m_ctrlStartSlabOffset.EnableWindow(bEnable);
   m_ctrlEndSlabOffset.EnableWindow(bEnable);
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
