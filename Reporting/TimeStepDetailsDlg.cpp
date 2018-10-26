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
// TimeStepDetailsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "TimeStepDetailsDlg.h"

#include <PgsExt\GirderLabel.h>
#include <MFCTools\CustomDDX.h>

#include <IFace\Intervals.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>

// CTimeStepDetailsDlg dialog

IMPLEMENT_DYNAMIC(CTimeStepDetailsDlg, CDialog)

CTimeStepDetailsDlg::CTimeStepDetailsDlg(IBroker* pBroker,boost::shared_ptr<CTimeStepDetailsReportSpecification>& pRptSpec,const pgsPointOfInterest& initialPoi,IntervalIndexType intervalIdx,CWnd* pParent)
	: CDialog(CTimeStepDetailsDlg::IDD, pParent)
   , m_SliderPos(0)
   , m_pRptSpec(pRptSpec)
{
   m_InitialPOI = initialPoi;
   m_GirderKey = m_InitialPOI.GetSegmentKey();
   m_IntervalIdx = intervalIdx;
   m_bUseAllLocations = false;
   m_pBroker = pBroker;
}

CTimeStepDetailsDlg::~CTimeStepDetailsDlg()
{
}

void CTimeStepDetailsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_SLIDER, m_Slider);
   DDX_Control(pDX, IDC_LOCATION, m_Location);

   DDX_CBIndex(pDX, IDC_GIRDERLINE, m_GirderKey.girderIndex);
   DDX_CBItemData(pDX, IDC_INTERVAL, m_IntervalIdx);
   DDX_Slider(pDX, IDC_SLIDER, m_SliderPos);
   DDX_Check_Bool(pDX, IDC_ALL_LOCATIONS, m_bUseAllLocations);
}

BEGIN_MESSAGE_MAP(CTimeStepDetailsDlg, CDialog)
   ON_WM_HSCROLL()
   ON_BN_CLICKED(IDC_ALL_LOCATIONS,OnClickedAllLocations)
   ON_CBN_SELCHANGE(IDC_GIRDERLINE,OnGirderLineChanged)
END_MESSAGE_MAP()

BOOL CTimeStepDetailsDlg::OnInitDialog()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridge,pBridge);
   CComboBox* pcbGirders = (CComboBox*)GetDlgItem(IDC_GIRDERLINE);
   IndexType nGirderLines = pBridge->GetGirderlineCount();
   for ( IndexType gdrIdx = 0; gdrIdx < nGirderLines; gdrIdx++ )
   {
      CString str;
      str.Format(_T("%s"),LABEL_GIRDER(gdrIdx));
      pcbGirders->AddString(str);
   }

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
   int idx = pcbIntervals->AddString(_T("All Intervals"));
   pcbIntervals->SetItemData(idx,(DWORD_PTR)INVALID_INDEX);

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

   if ( m_pRptSpec )
   {
      InitFromRptSpec();
   }

   UpdateSliderLabel();
   OnClickedAllLocations();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

bool CTimeStepDetailsDlg::UseAllLocations()
{
   return m_bUseAllLocations;
}

pgsPointOfInterest CTimeStepDetailsDlg::GetPOI()
{
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_SliderPos];
   return poi;
}

IntervalIndexType CTimeStepDetailsDlg::GetInterval()
{
   return m_IntervalIdx;
}

void CTimeStepDetailsDlg::UpdatePOI()
{
   GET_IFACE(IPointOfInterest,pPOI);
   m_vPOI = pPOI->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,m_GirderKey.girderIndex,ALL_SEGMENTS));
   if (m_Slider.GetSafeHwnd() != NULL )
   {
      m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 
                                                   // subtract one so we don't go past the end of the array
   }
}

void CTimeStepDetailsDlg::InitFromRptSpec()
{
   m_bUseAllLocations = m_pRptSpec->ReportAtAllLocations();
   pgsPointOfInterest poi = m_pRptSpec->GetPointOfInterest();

   m_GirderKey = poi.GetSegmentKey();

   std::vector<pgsPointOfInterest>::iterator iter(m_vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator end(m_vPOI.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& p = *iter;
      if ( p.AtSamePlace(poi) )
      {
         m_SliderPos = (int)(iter - m_vPOI.begin());
         break;
      }
   }

   m_IntervalIdx = m_pRptSpec->GetInterval();

   UpdateData(FALSE);
}

void CTimeStepDetailsDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CTimeStepDetailsDlg::UpdateSliderLabel()
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

#if defined _DEBUG || defined _BETA_VERSION
   strLabel.Format(_T("Point Of Interest: ID = %d %s"),poi.GetID(),rptPoi.AsString().c_str());
#else
   strLabel.Format(_T("%s"),rptPoi.AsString().c_str());
#endif
   // remove the HTML tags
   strLabel.Replace(_T("<sub>"),_T(""));
   strLabel.Replace(_T("</sub>"),_T(""));

   m_Location.SetWindowText(strLabel);
}

void CTimeStepDetailsDlg::OnClickedAllLocations()
{
   int show = IsDlgButtonChecked(IDC_ALL_LOCATIONS) ? SW_HIDE : SW_SHOW;
   GetDlgItem(IDC_LABEL)->ShowWindow(show);
   m_Slider.ShowWindow(show);
   m_Location.ShowWindow(show);
}

void CTimeStepDetailsDlg::OnGirderLineChanged()
{
   CComboBox* pcbGirders = (CComboBox*)GetDlgItem(IDC_GIRDERLINE);
   GirderIndexType gdrIdx = (GirderIndexType)pcbGirders->GetCurSel();
   m_GirderKey.girderIndex = gdrIdx;
   UpdatePOI();
   UpdateSliderLabel();
}