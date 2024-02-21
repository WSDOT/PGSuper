///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// The moment Capacity Details report was contributed by BridgeSight Inc.

// SelectCrackedSectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectCrackedSectionDlg.h"
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>
#include <PgsExt\GirderLabel.h>
#include <PGSuperUnits.h>
#include <EAF\EAFDocument.h>
#include "..\Documentation\PGSuper.hh"

// CSelectCrackedSectionDlg dialog

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSelectCrackedSectionDlg, CDialog)

CSelectCrackedSectionDlg::CSelectCrackedSectionDlg(IBroker* pBroker,std::shared_ptr<CCrackedSectionReportSpecification>& pRptSpec,CWnd* pParent /*=nullptr*/)
	: CDialog(CSelectCrackedSectionDlg::IDD, pParent)
   , m_SliderPos(0)
   , m_pRptSpec(pRptSpec)
{
   m_pBroker = pBroker;
   m_MomentType = 0;
}

CSelectCrackedSectionDlg::~CSelectCrackedSectionDlg()
{
}

void CSelectCrackedSectionDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_SPAN, m_cbGroup);
   DDX_Control(pDX, IDC_GIRDER, m_cbGirder);
   DDX_Control(pDX, IDC_SLIDER, m_Slider);
   DDX_Control(pDX, IDC_LOCATION, m_Label);

   GroupIndexType grpIdx;
   GirderIndexType gdrIdx;
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_CBIndex(pDX, IDC_SPAN,   (int&)grpIdx);
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)gdrIdx);
      m_GirderKey = CGirderKey(grpIdx, gdrIdx);

   }
   else
   {
      grpIdx = m_GirderKey.groupIndex;
      gdrIdx = m_GirderKey.girderIndex;;
      DDX_CBIndex(pDX, IDC_SPAN,   (int&)grpIdx);
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)gdrIdx);
   }


   DDX_Slider(pDX, IDC_SLIDER, m_SliderPos);
   DDX_Radio(pDX, IDC_MOMENT1, m_MomentType);
}


BEGIN_MESSAGE_MAP(CSelectCrackedSectionDlg, CDialog)
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(IDC_SPAN, &CSelectCrackedSectionDlg::OnSpanChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER, &CSelectCrackedSectionDlg::OnGirderChanged)
   ON_COMMAND(IDC_HELPBTN,OnHelp)
END_MESSAGE_MAP()


// CSelectCrackedSectionDlg message handlers

BOOL CSelectCrackedSectionDlg::OnInitDialog()
{
   GET_IFACE( IBridge, pBridge );
   m_GirderKey = m_InitialPOI.GetSegmentKey();

   CDialog::OnInitDialog();

   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   GroupIndexType cGroup = pBridge->GetGirderGroupCount();
   for ( GroupIndexType i = 0; i < cGroup; i++ )
   {
      CString strSpan(pgsGirderLabel::GetGroupLabel(i).c_str());
      pSpanBox->AddString(strSpan);
   }
   pSpanBox->SetCurSel((int)m_GirderKey.groupIndex);
   UpdateGirderComboBox();

   UpdatePOI();

   // initial the slider range
   m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 

   // initial the slider position to the current poi location
   int pos = (int)m_vPOI.size()/2; // default is mid-span
   int cur_pos = 0;
   for(const pgsPointOfInterest& poi : m_vPOI)
   {
      if ( poi.GetID() == m_InitialPOI.GetID() )
      {
         pos = cur_pos;
         break;
      }
      cur_pos++;
   }
   m_Slider.SetPos((int)pos);
   m_SliderPos = pos;

   UpdateSliderLabel();

   if ( m_pRptSpec )
      InitFromRptSpec();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

pgsPointOfInterest CSelectCrackedSectionDlg::GetPOI()
{
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_SliderPos];
   return poi;
}

void CSelectCrackedSectionDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSelectCrackedSectionDlg::UpdateGirderComboBox()
{
   GET_IFACE(IBridge, pBridge);

   int curSel = m_cbGirder.GetCurSel();
   if (curSel == CB_ERR)
      curSel = (int)m_GirderKey.girderIndex;

   m_cbGirder.ResetContent();

   GirderIndexType cGirder = pBridge->GetGirderCount(m_GirderKey.groupIndex);
   for (GirderIndexType j = 0; j < cGirder; j++)
   {
      CString strGdr;
      strGdr.Format(_T("Girder %s"), LABEL_GIRDER(j));
      m_cbGirder.AddString(strGdr);
   }

   if (m_cbGirder.SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR)
      m_cbGirder.SetCurSel(0);

   m_GirderKey.girderIndex = (IndexType)m_cbGirder.GetCurSel();
}

void CSelectCrackedSectionDlg::UpdateSliderLabel()
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   CString strLabel;
   int sliderPos = m_Slider.GetPos();
   if ((int)(m_vPOI.size() - 1) < sliderPos)
   {
      sliderPos = (int)m_vPOI.size() / 2; 
      m_Slider.SetPos(sliderPos);
   }

   pgsPointOfInterest poi = m_vPOI[sliderPos];

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rptPointOfInterest rptPoi(&pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   rptPoi.SetValue(POI_SPAN,poi);
   rptPoi.PrefixAttributes(false); // put the attributes after the location

   strLabel.Format(_T("Distance from Left Support = %s"),rptPoi.AsString().c_str());
   strLabel.Replace(_T("<sub>"),_T(""));
   strLabel.Replace(_T("</sub>"),_T(""));

   m_Label.SetWindowText(strLabel);
}

void CSelectCrackedSectionDlg::OnSpanChanged()
{
   m_GirderKey.groupIndex = m_cbGroup.GetCurSel();
   UpdateGirderComboBox();
   UpdatePOI();
   UpdateSliderLabel();
}

void CSelectCrackedSectionDlg::OnGirderChanged()
{
   m_GirderKey.girderIndex = m_cbGirder.GetCurSel();
   UpdatePOI();
   UpdateSliderLabel();
}

void CSelectCrackedSectionDlg::UpdatePOI()
{
   GET_IFACE(IPointOfInterest,pPOI);

   m_vPOI.clear();
   pPOI->GetPointsOfInterest(CSegmentKey(m_GirderKey, ALL_SEGMENTS), &m_vPOI);
   if (m_Slider.GetSafeHwnd() != nullptr )
   {
      m_Slider.SetRange(0,int(m_vPOI.size()-1)); // the range is number of spaces along slider... 
                                                 // subtract one so we don't go past the end of the array
   }
}

void CSelectCrackedSectionDlg::InitFromRptSpec()
{
   pgsPointOfInterest poi = m_pRptSpec->GetPOI();

   m_GirderKey = poi.GetSegmentKey();

   int cur_pos = 0;
   for(const pgsPointOfInterest& p : m_vPOI)
   {
      if ( p.GetID() == poi.GetID() )
      {
         m_SliderPos = cur_pos;
      }
      cur_pos++;
   }

   m_MomentType = m_pRptSpec->IsPositiveMoment() ? 0 : 1;

   UpdateData(FALSE);
   UpdateSliderLabel();
}

void CSelectCrackedSectionDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_CRACKED_SECTION_DETAILS_REPORT);
}