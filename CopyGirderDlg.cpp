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

// CopyGirderDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperDoc.h"
#include "CopyGirderDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>

#include <PgsExt\BridgeDescription.h>

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog


CCopyGirderDlg::CCopyGirderDlg(IBroker* pBroker, CPGSuperDoc* pDoc, CWnd* pParent /*=NULL*/)
	: CDialog(CCopyGirderDlg::IDD, pParent),
   m_pDoc(pDoc),
   m_pBroker(pBroker)
{
	//{{AFX_DATA_INIT(CCopyGirderDlg)
	//}}AFX_DATA_INIT
}


void CCopyGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyGirderDlg)
	DDX_Control(pDX, IDC_COPY_GIRDER, m_DoCopyGirder);
	DDX_Control(pDX, IDC_COPY_MATERIAL, m_DoCopyMaterial);
	DDX_Control(pDX, IDC_COPY_TRANSVERSE, m_DoCopyTransverse);
	DDX_Control(pDX, IDC_COPY_PRESTRESSING, m_DoCopyPrestressing);
	DDX_Control(pDX, IDC_COPY_HANDLING, m_DoCopyHandling);
	DDX_Control(pDX, IDC_COPY_SLABOFFSET, m_DoCopySlabOffset);
   DDX_Control(pDX, IDC_LONGITUDINAL_REBAR, m_DoCopyLongitudinalRebar);

   DDX_Control(pDX, IDC_FROM_SPAN,   m_FromSpan);
   DDX_Control(pDX, IDC_FROM_GIRDER, m_FromGirder);
   DDX_Control(pDX, IDC_TO_SPAN,     m_ToSpan);
   DDX_Control(pDX, IDC_TO_GIRDER,   m_ToGirder);
	//}}AFX_DATA_MAP

   DDX_Check(pDX, IDC_COPY_GIRDER,       m_bCopyGirder);
   DDX_Check(pDX, IDC_COPY_TRANSVERSE,   m_bCopyTransverse);
   DDX_Check(pDX, IDC_COPY_PRESTRESSING, m_bCopyPrestressing);
   DDX_Check(pDX, IDC_COPY_HANDLING,     m_bCopyHandling);
   DDX_Check(pDX, IDC_COPY_MATERIAL,     m_bCopyMaterial);
   DDX_Check(pDX, IDC_LONGITUDINAL_REBAR, m_bCopyLongitudinalRebar);
   DDX_Check(pDX, IDC_COPY_SLABOFFSET,   m_bCopySlabOffset);

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromSpanGirderHashValue = GetFromSpanGirder();
      m_ToSpanGirderHashValues  = GetToSpanGirders();
   }
   else
   {
      // start with combo box for To girder
      CButton* pBut = (CButton*)GetDlgItem(IDC_RADIO1);
      pBut->SetCheck(BST_CHECKED);
      GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(false);
   }
}

BEGIN_MESSAGE_MAP(CCopyGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyGirderDlg)
   ON_CBN_SELCHANGE(IDC_FROM_SPAN,OnFromSpanChanged)
   ON_CBN_SELCHANGE(IDC_TO_SPAN,OnToSpanChanged)
   ON_CBN_SELCHANGE(IDC_TO_GIRDER,OnToGirderChanged)
	ON_BN_CLICKED(IDC_COPY_PRESTRESSING, OnCopyPrestressing)
	ON_BN_CLICKED(IDC_COPY_HANDLING, OnCopyHandling)
	ON_BN_CLICKED(IDC_COPY_SLABOFFSET, OnCopySlabOffset)
	ON_BN_CLICKED(IDC_COPY_TRANSVERSE, OnCopyTransverse)
   ON_BN_CLICKED(IDC_LONGITUDINAL_REBAR, OnCopyLongitudinalRebar)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
	ON_BN_CLICKED(IDC_COPY_GIRDER, OnCopyGirder)
   ON_BN_CLICKED(IDC_RADIO1, &CCopyGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CCopyGirderDlg::OnBnClickedRadio)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SELECT_GIRDERS, &CCopyGirderDlg::OnBnClickedSelectGirders)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg message handlers

BOOL CCopyGirderDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // prestressing and longitudinal rebar can only be copied if
   // the source and destination girders are the same time
   UINT chkMainReinforcement = 1;

   if ( pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      m_DoCopyGirder.SetCheck(0);
      m_DoCopyGirder.EnableWindow(FALSE);

      chkMainReinforcement = 1;
   }
   else
   {
      m_DoCopyGirder.SetCheck(1);
      chkMainReinforcement = 0;
   }

   m_DoCopyPrestressing.SetCheck(chkMainReinforcement);
   m_DoCopyLongitudinalRebar.SetCheck(chkMainReinforcement);
   m_DoCopyTransverse.SetCheck(1);
   m_DoCopyHandling.SetCheck(1);
   m_DoCopyMaterial.SetCheck(1);

   if ( pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
   {
      m_DoCopySlabOffset.SetCheck(0);
      m_DoCopySlabOffset.EnableWindow(FALSE);
   }
   else
   {
      m_DoCopySlabOffset.SetCheck(1);
   }

   FillComboBoxes(m_FromSpan,m_FromGirder,false);
   FillComboBoxes(m_ToSpan,  m_ToGirder,  true );

   GET_IFACE(ISelection,pSelection);
   SpanIndexType currSpan = pSelection->GetSpanIdx();
   GirderIndexType currGirder = pSelection->GetGirderIdx();
   if ( currSpan != INVALID_INDEX && currGirder != INVALID_INDEX )
   {
      m_FromSpan.SetCurSel((int)currSpan);
      OnFromSpanChanged();
      m_FromGirder.SetCurSel((int)currGirder);

      m_ToSpan.SetCurSel((int)currSpan+1);
      OnToSpanChanged();
   }
	
   UpdateApply();


   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyGirderDlg::FillComboBoxes(CComboBox& cbSpan,CComboBox& cbGirder, bool bIncludeAllSpanGirder)
{
   cbSpan.ResetContent();

   if ( bIncludeAllSpanGirder )
   {
      int idx = cbSpan.AddString(_T("All Spans"));
      cbSpan.SetItemData(idx,ALL_SPANS);
   }

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CString str;
      str.Format(_T("Span %d"),LABEL_SPAN(spanIdx));

      int idx = cbSpan.AddString(str);
      cbSpan.SetItemData(idx,spanIdx);
   }

   cbSpan.SetCurSel(0);

   FillGirderComboBox(cbGirder,0,bIncludeAllSpanGirder);
}

void CCopyGirderDlg::FillGirderComboBox(CComboBox& cbGirder,SpanIndexType spanIdx,bool bIncludeAll)
{
   int curSel = cbGirder.GetCurSel();

   cbGirder.ResetContent();

   if ( bIncludeAll )
   {
      int idx = cbGirder.AddString(_T("All Girders"));
      cbGirder.SetItemData(idx,ALL_GIRDERS);
   }

   GET_IFACE(IBridge,pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx == ALL_SPANS ? 0 : spanIdx);
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));

      int idx = cbGirder.AddString(str);
      cbGirder.SetItemData(idx,gdrIdx);
   }

   if ( curSel != CB_ERR )
      curSel = cbGirder.SetCurSel(curSel);

   if ( curSel == CB_ERR )
      cbGirder.SetCurSel(0);
}

void CCopyGirderDlg::OnFromSpanChanged() 
{
   int curSel = m_FromSpan.GetCurSel();
   if ( curSel != CB_ERR )
   {
      SpanIndexType spanIdx = (SpanIndexType)m_FromSpan.GetItemData(curSel);
      FillGirderComboBox(m_FromGirder,spanIdx,false);
   }
   else
   {
      FillGirderComboBox(m_FromGirder,0,false);
   }
}

void CCopyGirderDlg::OnToSpanChanged() 
{
   int curSel = m_ToSpan.GetCurSel();
   if ( curSel != CB_ERR )
   {
      SpanIndexType spanIdx = (SpanIndexType)m_ToSpan.GetItemData(curSel);
      FillGirderComboBox(m_ToGirder,spanIdx,true);
   }
   else
   {
      FillGirderComboBox(m_ToGirder,0,true);
   }

   CopyToSelectionChanged();
}

void CCopyGirderDlg::OnToGirderChanged()
{
   CopyToSelectionChanged();
}

void CCopyGirderDlg::CopyToSelectionChanged() 
{
   // if the source and any of the destination girders are not the same type
   // the prestressing and longitudinal reinforcement data must be copied
   SpanGirderHashType copyFrom = GetFromSpanGirder();
   std::vector<SpanGirderHashType> copyTo = GetToSpanGirders();

   SpanIndexType fromSpan;
   GirderIndexType fromGirder;
   UnhashSpanGirder(copyFrom,&fromSpan,&fromGirder);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::_tstring strFromGirder = pBridgeDesc->GetSpan(fromSpan)->GetGirderTypes()->GetGirderName(fromGirder);

   BOOL bCanCopy = TRUE;

   std::vector<SpanGirderHashType>::iterator iter;
   for ( iter = copyTo.begin(); iter != copyTo.end(); iter++ )
   {
      SpanGirderHashType dwTo = *iter;
      SpanIndexType toSpan;
      GirderIndexType toGirder;
      UnhashSpanGirder(dwTo,&toSpan,&toGirder);
      std::_tstring strToGirder = pBridgeDesc->GetSpan(toSpan)->GetGirderTypes()->GetGirderName(toGirder);

      if ( strFromGirder != strToGirder )
      {
         bCanCopy = FALSE;
         break;
      }
   }
   m_DoCopyPrestressing.SetCheck(bCanCopy ? 1 : 0);
   m_DoCopyPrestressing.EnableWindow(bCanCopy);
   m_DoCopyLongitudinalRebar.SetCheck(bCanCopy ? 1 : 0);
   m_DoCopyLongitudinalRebar.EnableWindow(bCanCopy);

   UpdateApply();
}

void CCopyGirderDlg::UpdateApply()
{
   BOOL enable = TRUE;
   if (m_DoCopyTransverse.GetCheck()==1   ||
       m_DoCopyGirder.GetCheck()==1 ||
       m_DoCopyPrestressing.GetCheck()==1 ||
       m_DoCopyHandling.GetCheck()==1     ||
       m_DoCopyMaterial.GetCheck()==1     ||
       m_DoCopySlabOffset.GetCheck()==1     ||
       m_DoCopyLongitudinalRebar.GetCheck() == 1)
   {
      enable = TRUE;
   }
   else
   {
      enable = FALSE;
   }
   CWnd* butok = GetDlgItem(IDOK);
   ASSERT(butok!=0);
   butok->EnableWindow(enable);
}

SpanGirderHashType CCopyGirderDlg::GetFromSpanGirder()
{
   SpanIndexType spanIdx = 0;
   int sel = m_FromSpan.GetCurSel();
   if ( sel != CB_ERR )
   {
      spanIdx = (SpanIndexType)m_FromSpan.GetItemData(sel);
   }

   GirderIndexType gdrIdx = 0;
   sel = m_FromGirder.GetCurSel();
   if( sel != CB_ERR )
   {
      gdrIdx = (GirderIndexType)m_FromGirder.GetItemData(sel);
   }

   return HashSpanGirder(spanIdx,gdrIdx);
}

std::vector<SpanGirderHashType> CCopyGirderDlg::GetToSpanGirders()
{
   std::vector<SpanGirderHashType> vec;

   // See which control to get data from
   BOOL enab_combo = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;

   if (enab_combo)
   {
      GET_IFACE(IBridge,pBridge);
      SpanIndexType firstSpan, lastSpan;
      int sel = m_ToSpan.GetCurSel();
      firstSpan = (SpanIndexType)m_ToSpan.GetItemData(sel);
      if ( firstSpan == ALL_SPANS )
      {
         firstSpan = 0;
         lastSpan = pBridge->GetSpanCount()-1;
      }
      else
      {
         lastSpan = firstSpan;
      }

      for ( SpanIndexType spanIdx = firstSpan; spanIdx <= lastSpan; spanIdx++ )
      {
         GirderIndexType firstGdr, lastGdr;
         sel = m_ToGirder.GetCurSel();
         firstGdr = (GirderIndexType)m_ToGirder.GetItemData(sel);
         if ( firstGdr == ALL_GIRDERS )
         {
            firstGdr = 0;
            lastGdr = pBridge->GetGirderCount(spanIdx)-1;
         }
         else
         {
            lastGdr = firstGdr;
         }

         for (GirderIndexType gdrIdx = firstGdr; gdrIdx <= lastGdr; gdrIdx++ )
         {
            GirderIndexType realGdrIdx = gdrIdx;
            GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
            if ( nGirders <= gdrIdx )
               realGdrIdx = nGirders-1;

            vec.push_back( HashSpanGirder(spanIdx,realGdrIdx) );
         }
      }
   }
   else
   {
      // data is in grid
      vec = m_MultiDialogSelections;
   }

   return vec;
}

void CCopyGirderDlg::OnCopyPrestressing() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyHandling() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopySlabOffset() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyTransverse()  
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyLongitudinalRebar()
{
   UpdateApply();
}

void CCopyGirderDlg::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_COPYGDRPROPERTIES );
}

void CCopyGirderDlg::OnCopyMaterial() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyGirder() 
{
   UpdateApply();	
}

void CCopyGirderDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_TO_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_TO_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(enab_mpl);

   CopyToSelectionChanged();
}

void CCopyGirderDlg::OnBnClickedSelectGirders()
{
   CMultiGirderSelectDlg dlg;
   dlg.m_SelGdrs = m_MultiDialogSelections;

   if (dlg.DoModal()==IDOK)
   {
      // update button text
      CString msg;
      msg.Format(_T("Select Girders\n(%d Selected)"), dlg.m_SelGdrs.size());
      GetDlgItem(IDC_SELECT_GIRDERS)->SetWindowText(msg);

      m_MultiDialogSelections = dlg.m_SelGdrs;

      CopyToSelectionChanged();
   }
}
