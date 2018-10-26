///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
//
// EquilibriumCheckDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "EquilibriumCheckDlg.h"

#include <PgsExt\GirderLabel.h>
#include <MFCTools\CustomDDX.h>

#include <IFace\Intervals.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>

// CEquilibriumCheckDlg dialog

IMPLEMENT_DYNAMIC(CEquilibriumCheckDlg, CDialog)

CEquilibriumCheckDlg::CEquilibriumCheckDlg(IBroker* pBroker,std::shared_ptr<CEquilibriumCheckReportSpecification>& pRptSpec,const pgsPointOfInterest& initialPoi,IntervalIndexType intervalIdx,CWnd* pParent)
	: CDialog(CEquilibriumCheckDlg::IDD, pParent)
   , m_SliderPos(0)
   , m_pRptSpec(pRptSpec)
{
   m_InitialPOI = initialPoi;
   m_GirderKey = m_InitialPOI.GetSegmentKey();
   m_IntervalIdx = intervalIdx;
   m_pBroker = pBroker;
}

CEquilibriumCheckDlg::~CEquilibriumCheckDlg()
{
}

void CEquilibriumCheckDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_SLIDER, m_Slider);
   DDX_Control(pDX, IDC_LOCATION, m_Label);

   DDX_CBItemData(pDX, IDC_INTERVAL, m_IntervalIdx);
   DDX_Slider(pDX, IDC_SLIDER, m_SliderPos);
}

BEGIN_MESSAGE_MAP(CEquilibriumCheckDlg, CDialog)
   ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CEquilibriumCheckDlg::OnInitDialog()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IIntervals, pIntervals);
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_INTERVAL);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      int idx = pcbIntervals->AddString(strLabel);
      pcbIntervals->SetItemData(idx,(DWORD_PTR)intervalIdx);
   }

   UpdatePOI();

   CDialog::OnInitDialog();

   // initial the slider range
   m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 

   // initial the slider position to the current poi location
   CollectionIndexType pos = m_vPOI.size()/2; // default is mid-span
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = m_vPOI.begin(); iter != m_vPOI.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      if ( poi.GetID() == m_InitialPOI.GetID() )
      {
         pos = (iter - m_vPOI.begin());
      }
   }
   m_Slider.SetPos((int)pos);

   UpdateSliderLabel();

   if ( m_pRptSpec )
   {
      InitFromRptSpec();
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

pgsPointOfInterest CEquilibriumCheckDlg::GetPOI()
{
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_SliderPos];
   return poi;
}

IntervalIndexType CEquilibriumCheckDlg::GetInterval()
{
   return m_IntervalIdx;
}

void CEquilibriumCheckDlg::UpdatePOI()
{
   GET_IFACE(IPointOfInterest,pPOI);
   m_vPOI = pPOI->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,m_GirderKey.girderIndex,ALL_SEGMENTS));
   if (m_Slider.GetSafeHwnd() != nullptr )
   {
      m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 
                                                   // subtract one so we don't go past the end of the array
   }
}

void CEquilibriumCheckDlg::InitFromRptSpec()
{
   pgsPointOfInterest poi = m_pRptSpec->GetPOI();

   m_GirderKey = poi.GetSegmentKey();

   std::vector<pgsPointOfInterest>::iterator iter(m_vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator end(m_vPOI.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& p = *iter;
      if ( p.GetID() == poi.GetID() )
      {
         m_SliderPos = (int)(iter - m_vPOI.begin());
      }
   }

   m_IntervalIdx = m_pRptSpec->GetInterval();

   UpdateData(FALSE);
   UpdateSliderLabel();
}

void CEquilibriumCheckDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CEquilibriumCheckDlg::UpdateSliderLabel()
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   CString strLabel;
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_Slider.GetPos()];

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rptPointOfInterest rptPoi(&pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   rptPoi.SetValue(POI_SPAN,poi);
   rptPoi.PrefixAttributes(false); // put the attributes after the location
   rptPoi.IncludeSpanAndGirder(true);

   strLabel.Format(_T("ID = %d: Location = %s"),poi.GetID(),rptPoi.AsString().c_str());
   // remove the HTML tags
   strLabel.Replace(_T("<sub>"),_T(""));
   strLabel.Replace(_T("</sub>"),_T(""));

   m_Label.SetWindowText(strLabel);
}
