///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// SectionCutDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "GirderModelChildFrame.h"
#include "SectionCutDlgEx.h"
#include <ostream>

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSectionCutDlgEx dialog

CSectionCutDlgEx::CSectionCutDlgEx(IBroker* pBroker,const CGirderKey& girderKey,const pgsPointOfInterest& initialPoi,CWnd* pParent) 
: CDialog(CSectionCutDlgEx::IDD, pParent),
m_pBroker(pBroker),
m_SliderPos(0)
{
   m_InitialPOI = initialPoi;
   m_GirderKey = girderKey;
}

void CSectionCutDlgEx::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSectionCutDlgEx) 
	//}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_SLIDER, m_Slider);
   DDX_Control(pDX, IDC_LOCATION, m_Label);
   DDX_Slider(pDX, IDC_SLIDER, m_SliderPos);

}

BEGIN_MESSAGE_MAP(CSectionCutDlgEx, CDialog)
	//{{AFX_MSG_MAP(CSectionCutDlgEx)
   ON_WM_HSCROLL()
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSectionCutDlgEx message handlers

BOOL CSectionCutDlgEx::OnInitDialog() 
{
   UpdatePOI();

   CDialog::OnInitDialog();

   // initial the slider range
   m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 

   // initial the slider position to the current poi location
   int pos = (int)m_vPOI.size()/2; // default is mid-span
   int cur_pos = 0;
   for( const pgsPointOfInterest& poi : m_vPOI)
   {
      if ( poi.GetID() == m_InitialPOI.GetID() )
      {
         pos = cur_pos;
      }
      cur_pos++;
   }
   m_Slider.SetPos(pos);

   UpdateSliderLabel();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSectionCutDlgEx::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_SECTIONCUT );
}

pgsPointOfInterest CSectionCutDlgEx::GetPOI()
{
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_SliderPos];
   return poi;
}

void CSectionCutDlgEx::UpdatePOI()
{
   GET_IFACE(IPointOfInterest,pPoi);
   m_vPOI.clear();
   pPoi->GetPointsOfInterest(CSegmentKey(m_GirderKey, ALL_SEGMENTS), &m_vPOI);
   if (m_Slider.GetSafeHwnd() != nullptr )
   {
      m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 
                                                   // subtract one so we don't go past the end of the array
   }
}

void CSectionCutDlgEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSectionCutDlgEx::UpdateSliderLabel()
{
/*
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
*/
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IDocumentType,pDocType);

   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_Slider.GetPos()];

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CString strLabel1;
   if ( pDocType->IsPGSuperDocument() )
   {
      strLabel1.Format(_T("Location from Start of Span %s Girder %s, %s"),LABEL_SPAN(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex),
         ::FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit()));
   }
   else
   {
      strLabel1.Format(_T("Location from Start of Segment %d, %s"),LABEL_SEGMENT(segmentKey.segmentIndex),
         ::FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit()));
   }
   
   std::_tstring strAttr = poi.GetAttributes(POI_ERECTED_SEGMENT, false);
   if (!strAttr.empty())
   {
      CString strAttribute;
      strAttribute.Format(_T(" (%s)"), strAttr.c_str());
      strLabel1 += strAttribute;
   }

   Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);
   CString strLabel2;
   strLabel2.Format(_T("Location from Start of Girder Line, %s"),::FormatDimension(Xgl,pDisplayUnits->GetSpanLengthUnit()));

   CString strLabel3;
   if ( pDocType->IsPGSpliceDocument() )
   {
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      strLabel3.Format(_T("Location from Start of Span %s, %s"),LABEL_SPAN(spanKey.spanIndex),::FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()));

      std::_tstring strAttr = poi.GetAttributes(POI_SPAN, false);
      if (!strAttr.empty())
      {
         CString strAttribute;
         strAttribute.Format(_T(" (%s)"), strAttr.c_str());
         strLabel3 += strAttribute;
      }
   }

   CString strLabel;
   strLabel.Format(_T("%s\r\n%s\r\n%s"),strLabel1,strLabel2,strLabel3);

   m_Label.SetWindowText(strLabel);
}
