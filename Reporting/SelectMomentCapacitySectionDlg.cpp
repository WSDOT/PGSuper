///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// SelectPOIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectMomentCapacitySectionDlg.h"
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>
#include <PgsExt\GirderLabel.h>
#include <PGSuperUnits.h>
#include <EAF\EAFDocument.h>
#include "..\Documentation\PGSuper.hh"

// CSelectMomentCapacitySectionDlg dialog

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSelectMomentCapacitySectionDlg, CDialog)

CSelectMomentCapacitySectionDlg::CSelectMomentCapacitySectionDlg(IBroker* pBroker,std::shared_ptr<CMomentCapacityReportSpecification>& pRptSpec,CWnd* pParent /*=nullptr*/)
	: CDialog(CSelectMomentCapacitySectionDlg::IDD, pParent)
   , m_SliderPos(0)
   , m_pRptSpec(pRptSpec)
{
   m_pBroker = pBroker;
   m_MomentType = 0;
}

CSelectMomentCapacitySectionDlg::~CSelectMomentCapacitySectionDlg()
{
}

void CSelectMomentCapacitySectionDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_GROUP,  m_cbGroup);
   DDX_Control(pDX, IDC_GIRDER, m_cbGirder);
   DDX_Control(pDX, IDC_SLIDER, m_Slider);
   DDX_Control(pDX, IDC_LOCATION, m_Label);

   IndexType grpIdx,gdrIdx;
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_CBIndex(pDX, IDC_GROUP,  (int&)grpIdx);
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)gdrIdx);
      m_GirderKey = CGirderKey(grpIdx,gdrIdx);
   }
   else
   {
      grpIdx = m_GirderKey.groupIndex;
      gdrIdx = m_GirderKey.girderIndex;;
      DDX_CBIndex(pDX, IDC_GROUP,  (int&)grpIdx);
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)gdrIdx);
   }

   DDX_Slider(pDX, IDC_SLIDER, m_SliderPos);
   DDX_Radio(pDX, IDC_MOMENT1, m_MomentType);
}


BEGIN_MESSAGE_MAP(CSelectMomentCapacitySectionDlg, CDialog)
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(IDC_GROUP,  &CSelectMomentCapacitySectionDlg::OnGroupChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER, &CSelectMomentCapacitySectionDlg::OnGirderChanged)
   ON_COMMAND(IDC_HELPBTN,OnHelp)
END_MESSAGE_MAP()


// CSelectMomentCapacitySectionDlg message handlers

BOOL CSelectMomentCapacitySectionDlg::OnInitDialog()
{
   GET_IFACE( IBridge, pBridge );
   m_GirderKey = m_InitialPOI.GetSegmentKey();

   CDialog::OnInitDialog();

   CComboBox* pcbGroup = (CComboBox*)GetDlgItem( IDC_GROUP );
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strLabel(pgsGirderLabel::GetGroupLabel(grpIdx).c_str());
      pcbGroup->AddString(strLabel);
   }
   pcbGroup->SetCurSel((int)m_GirderKey.groupIndex);
   UpdateGirderComboBox();

   UpdatePOI();

   // initial the slider range
   m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 

   // initial the slider position to the current poi location
   IndexType pos = m_vPOI.size()/2; // default is mid-span
   IndexType cur_pos = 0;
   for ( const pgsPointOfInterest& poi : m_vPOI)
   {
      if ( poi.GetID() == m_InitialPOI.GetID() )
      {
         pos = cur_pos;
      }
      cur_pos++;
   }
   m_Slider.SetPos((int)pos);

   UpdateSliderLabel();

   if ( m_pRptSpec )
      InitFromRptSpec();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

pgsPointOfInterest CSelectMomentCapacitySectionDlg::GetPOI()
{
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_SliderPos];
   return poi;
}

void CSelectMomentCapacitySectionDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSelectMomentCapacitySectionDlg::UpdateGirderComboBox()
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

void CSelectMomentCapacitySectionDlg::UpdateSliderLabel()
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   CString strLabel;
   pgsPointOfInterest poi = m_vPOI[m_Slider.GetPos()];

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rptPointOfInterest rptPoi(&pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   rptPoi.SetValue(POI_SPAN,poi);
   rptPoi.PrefixAttributes(false); // put the attributes after the location
   rptPoi.IncludeSpanAndGirder(true);

   strLabel.Format(_T("Location = %s"),rptPoi.AsString().c_str());
   // remove the HTML tags
   strLabel.Replace(_T("<sub>"),_T(""));
   strLabel.Replace(_T("</sub>"),_T(""));

   m_Label.SetWindowText(strLabel);
}

void CSelectMomentCapacitySectionDlg::OnGroupChanged()
{
   m_GirderKey.groupIndex = m_cbGroup.GetCurSel();
   UpdateGirderComboBox();
   UpdatePOI();
   UpdateSliderLabel();
}

void CSelectMomentCapacitySectionDlg::OnGirderChanged()
{
   m_GirderKey.girderIndex = m_cbGirder.GetCurSel();
   UpdatePOI();
   UpdateSliderLabel();
}

void CSelectMomentCapacitySectionDlg::UpdatePOI()
{
   GET_IFACE(IPointOfInterest,pPOI);
   // Get available pois from report spec so both classes are in synch
   CSegmentKey segmentKey(m_GirderKey, ALL_SEGMENTS);
   m_vPOI = CMomentCapacityReportSpecification::GetMomentCapacityDetailsPois(pPOI, segmentKey);

   if (m_Slider.GetSafeHwnd() != nullptr )
   {
      m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 
                                                   // subtract one so we don't go past the end of the array
   }
}

void CSelectMomentCapacitySectionDlg::InitFromRptSpec()
{
   pgsPointOfInterest poi = m_pRptSpec->GetPOI();

   m_GirderKey = poi.GetSegmentKey();

   int pos = 0;
   for(const pgsPointOfInterest& p : m_vPOI)
   {
      if ( p.GetID() == poi.GetID() )
      {
         m_SliderPos = pos;
      }
      pos++;
   }

   m_MomentType = m_pRptSpec->IsPositiveMoment() ? 0 : 1;

   UpdateData(FALSE);
   UpdateSliderLabel();
}

void CSelectMomentCapacitySectionDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_MOMENT_CAPACITY_DETAILS_REPORT);
}