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
//
// TimeStepDetailsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "BearingTimeStepDetailsDlg.h"

#include <PgsExt\GirderLabel.h>
#include <MFCTools\CustomDDX.h>

#include <IFace\Intervals.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CBearingTimeStepDetailsDlg dialog

IMPLEMENT_DYNAMIC(CBearingTimeStepDetailsDlg, CDialog)

CBearingTimeStepDetailsDlg::CBearingTimeStepDetailsDlg(IBroker* pBroker,std::shared_ptr<CBearingTimeStepDetailsReportSpecification>& pRptSpec,
    const ReactionLocation& initialReactionLocation,IntervalIndexType intervalIdx,CWnd* pParent)
	: CDialog(CBearingTimeStepDetailsDlg::IDD, pParent)
   , m_SliderPos(0)
   , m_pBTsRptSpec(pRptSpec)
{
   m_InitialReactionLocation = initialReactionLocation;
   m_GirderKey = m_InitialReactionLocation.GirderKey;
   m_IntervalIdx = intervalIdx;
   m_bUseAllLocations = false;
   m_pBroker = pBroker;
}

CBearingTimeStepDetailsDlg::~CBearingTimeStepDetailsDlg()
{
}

void CBearingTimeStepDetailsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_SLIDER_SHEAR_DEF, m_Slider);
   DDX_Control(pDX, IDC_LOCATION_SHEAR_DEF, m_Location);

   DDX_CBIndex(pDX, IDC_GIRDERLINE_SHEAR_DEF, m_GirderKey.girderIndex);
   DDX_CBItemData(pDX, IDC_INTERVAL_SHEAR_DEF, m_IntervalIdx);
   DDX_Slider(pDX, IDC_SLIDER_SHEAR_DEF, m_SliderPos);
   DDX_Check_Bool(pDX, IDC_ALL_LOCATIONS_SHEAR_DEF, m_bUseAllLocations);
}

BEGIN_MESSAGE_MAP(CBearingTimeStepDetailsDlg, CDialog)
   ON_WM_HSCROLL()
   ON_BN_CLICKED(IDC_ALL_LOCATIONS_SHEAR_DEF,OnClickedAllLocations)
   ON_CBN_SELCHANGE(IDC_GIRDERLINE_SHEAR_DEF,OnGirderLineChanged)
END_MESSAGE_MAP()

BOOL CBearingTimeStepDetailsDlg::OnInitDialog()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridge,pBridge);
   CComboBox* pcbGirders = (CComboBox*)GetDlgItem(IDC_GIRDERLINE_SHEAR_DEF);
   IndexType nGirderLines = pBridge->GetGirderlineCount();
   for ( IndexType gdrIdx = 0; gdrIdx < nGirderLines; gdrIdx++ )
   {
      CString str;
      str.Format(_T("%s"),LABEL_GIRDER(gdrIdx));
      pcbGirders->AddString(str);
   }

   GET_IFACE( IIntervals, pIntervals);
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_INTERVAL_SHEAR_DEF);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();


   
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetLastSegmentErectionInterval(m_GirderKey);


   for ( IntervalIndexType intervalIdx = erectSegmentIntervalIdx + 1; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
      int idx = pcbIntervals->AddString(strLabel);
      pcbIntervals->SetItemData(idx,(DWORD_PTR)intervalIdx);
   }

   int idx = pcbIntervals->AddString(_T("All Intervals"));
   pcbIntervals->SetItemData(idx,(DWORD_PTR)INVALID_INDEX);

   UpdateReactionLocation();

   CDialog::OnInitDialog();

   //   Reaction Locations

   // initial the slider range
   m_Slider.SetRange(0,(int)(m_vReactionLocations.size()-1)); // the range is number of spaces along slider... 

   // initial the slider position to the current reaction location
   int pos = 0; // default Pier 1
   int cur_pos = 0;
   for (const ReactionLocation& location : m_vReactionLocations)
   {
      if ( location.PierIdx == m_InitialReactionLocation.PierIdx && location.Face == m_InitialReactionLocation.Face)
      {
         pos = cur_pos;
      }
      cur_pos++;
   }
   m_Slider.SetPos(pos);

   InitFromBearingTimeStepRptSpec();

   UpdateSliderLabel();
   OnClickedAllLocations();

   SetWindowText(_T("Bearing Shear Deformation Time Step Details"));

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

bool CBearingTimeStepDetailsDlg::UseAllLocations()
{
   return m_bUseAllLocations;
}

ReactionLocation CBearingTimeStepDetailsDlg::GetReactionLocation()
{
   ASSERT((int)m_SliderPos < (int)m_vReactionLocations.size());
   ReactionLocation reactionLocation = m_vReactionLocations[m_SliderPos];
   return reactionLocation;
}

IntervalIndexType CBearingTimeStepDetailsDlg::GetInterval()
{
   return m_IntervalIdx;
}

void CBearingTimeStepDetailsDlg::UpdateReactionLocation()
{
   m_vReactionLocations.clear();

   GET_IFACE(IBearingDesign, pBearingDesign);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IBridge, pBridge);

   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   std::unique_ptr<CmbLsBearingDesignReactionAdapter> pForces(std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, m_GirderKey));
   m_vReactionLocations = pForces->GetBearingReactionLocations(lastCompositeDeckIntervalIdx, m_GirderKey, pBridge, pBearingDesign);
   if (m_Slider.GetSafeHwnd() != nullptr )
   {
      m_Slider.SetRange(0,(int)(m_vReactionLocations.size()-1)); // the range is number of spaces along slider... 
                                                   // subtract one so we don't go past the end of the array
   }
}

void CBearingTimeStepDetailsDlg::InitFromBearingTimeStepRptSpec()
{
   if (!m_pBTsRptSpec)
   {
      return;
   }

   m_bUseAllLocations = m_pBTsRptSpec->ReportAtAllLocations();
   ReactionLocation location = m_pBTsRptSpec->GetReactionLocation();

   m_GirderKey = location.GirderKey;

   int cur_pos = 0;
   for(const ReactionLocation& l : m_vReactionLocations)
   {
      if ( l.PierIdx && l.Face)
      {
         m_SliderPos = cur_pos;
         break;
      }
      cur_pos++;
   }

   m_IntervalIdx = m_pBTsRptSpec->GetInterval();

   UpdateData(FALSE);
}

void CBearingTimeStepDetailsDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CBearingTimeStepDetailsDlg::UpdateSliderLabel()
{

   ASSERT((int)m_SliderPos < (int)m_vReactionLocations.size());
   ReactionLocation location = m_vReactionLocations[m_Slider.GetPos()];

   CString strLabel{ location.PierLabel.c_str() };

   m_Location.SetWindowText(strLabel);
}

void CBearingTimeStepDetailsDlg::OnClickedAllLocations()
{
   int show = IsDlgButtonChecked(IDC_ALL_LOCATIONS) ? SW_HIDE : SW_SHOW;
   GetDlgItem(IDC_LABEL)->ShowWindow(show);
   m_Slider.ShowWindow(show);
   m_Location.ShowWindow(show);
}

void CBearingTimeStepDetailsDlg::OnGirderLineChanged()
{
   CComboBox* pcbGirders = (CComboBox*)GetDlgItem(IDC_GIRDERLINE_SHEAR_DEF);
   GirderIndexType gdrIdx = (GirderIndexType)pcbGirders->GetCurSel();
   m_GirderKey.girderIndex = gdrIdx;
   UpdateReactionLocation();
   UpdateSliderLabel();
}