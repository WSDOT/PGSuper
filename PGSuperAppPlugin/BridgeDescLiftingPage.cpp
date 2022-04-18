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

// BridgeDescLiftingPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "BridgeDescLiftingPage.h"

#include "GirderDescDlg.h"
#include "GirderSegmentDlg.h"

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLiftingPage property page

IMPLEMENT_DYNCREATE(CGirderDescLiftingPage, CPropertyPage)

CGirderDescLiftingPage::CGirderDescLiftingPage() : CPropertyPage(CGirderDescLiftingPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescLiftingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderDescLiftingPage::~CGirderDescLiftingPage()
{
}

void CGirderDescLiftingPage::DoDataExchange(CDataExchange* pDX)
{
   CPrecastSegmentData* pSegment = nullptr;
   CWnd* pWnd = GetParent();
   CSegmentKey segmentKey;
   if ( pWnd->IsKindOf(RUNTIME_CLASS(CGirderDescDlg)) )
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)pWnd;
      pSegment = pParent->m_pSegment;
      segmentKey = pParent->m_SegmentKey;
   }
   else if ( pWnd->IsKindOf(RUNTIME_CLASS(CGirderSegmentDlg)) )
   {
      CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)pWnd;
      pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);
      segmentKey = pParent->m_SegmentKey;
   }
   else
   {
      ATLASSERT(false); // should never get here
      // is there a new parent dialog???
   }

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescLiftingPage)
	//}}AFX_DATA_MAP

   DDX_MetaFileStatic(pDX, IDC_HAULINGOVERHANGS, m_Picture, _T("HaulingOverhangs"), _T("Metafile"), EMF_FIT );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
   DDX_UnitValueAndTag( pDX, IDC_GIRDERLENGTH, IDC_GIRDERLENGTH_UNIT, segmentLength, pDisplayUnits->GetSpanLengthUnit() );

   DDX_KeywordUnitValueAndTag(pDX,IDC_RELEASE_LOCATION,IDC_RELEASE_LOCATION_UNITS,_T("END|BRG"),pSegment->HandlingData.LeftReleasePoint, pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag( pDX, IDC_LIFTING_LOOP_LOCATION, IDC_LIFTING_LOOP_LOCATION_UNITS, pSegment->HandlingData.LeftLiftPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDX_KeywordUnitValueAndTag(pDX,IDC_STORAGE_LOCATION,IDC_STORAGE_LOCATION_UNITS,_T("BRG"),pSegment->HandlingData.LeftStoragePoint, pDisplayUnits->GetSpanLengthUnit() );

   if ( pDX->m_bSaveAndValidate )
   {
      // these are always the same at both ends, even thought the implementation in this
      // software permits them to be different at each end. make the right end the
      // same as the left end
      pSegment->HandlingData.RightReleasePoint = pSegment->HandlingData.LeftReleasePoint;
      pSegment->HandlingData.RightLiftPoint    = pSegment->HandlingData.LeftLiftPoint;
      pSegment->HandlingData.RightStoragePoint = pSegment->HandlingData.LeftStoragePoint;
   }

   if ( pSegment->HandlingData.LeftReleasePoint != -1 && pSegment->HandlingData.LeftReleasePoint != -2)
   {
      DDV_UnitValueZeroOrMore( pDX, IDC_RELEASE_LOCATION, pSegment->HandlingData.LeftReleasePoint, pDisplayUnits->GetSpanLengthUnit() );
      DDV_UnitValueLessThanLimit(pDX, IDC_RELEASE_LOCATION, pSegment->HandlingData.LeftReleasePoint, segmentLength/2, pDisplayUnits->GetSpanLengthUnit(),_T("Release support location must be less than half the segment length. Please enter a value that is less than %f %s") );
   }

   DDV_UnitValueZeroOrMore( pDX, IDC_LIFTING_LOOP_LOCATION, pSegment->HandlingData.LeftLiftPoint, pDisplayUnits->GetSpanLengthUnit() );

   if ( pSegment->HandlingData.LeftStoragePoint != -1 )
   {
      DDV_UnitValueZeroOrMore( pDX, IDC_STORAGE_LOCATION, pSegment->HandlingData.LeftStoragePoint, pDisplayUnits->GetSpanLengthUnit() );
      DDV_UnitValueLessThanLimit(pDX, IDC_STORAGE_LOCATION, pSegment->HandlingData.LeftStoragePoint, segmentLength/2, pDisplayUnits->GetSpanLengthUnit(),_T("Storage support location must be less than half the segment length. Please enter a value that is less than %f %s") );
   }

   DDX_UnitValueAndTag( pDX, IDC_LEADINGOVERHANG, IDC_LEADINGOVERHANG_UNITS, pSegment->HandlingData.LeadingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_LEADINGOVERHANG, pSegment->HandlingData.LeadingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag( pDX, IDC_TRAILINGOVERHANG, IDC_TRAILINGOVERHANG_UNITS, pSegment->HandlingData.TrailingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_TRAILINGOVERHANG, pSegment->HandlingData.TrailingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );

   Float64 clearspan = segmentLength - pSegment->HandlingData.LeadingSupportPoint - pSegment->HandlingData.TrailingSupportPoint;
   DDV_UnitValueZeroOrMore( pDX, IDC_LEADINGOVERHANG, clearspan, pDisplayUnits->GetSpanLengthUnit() );

   DDX_Control(pDX, IDC_HAUL_TRUCKS, m_HaulTruckCB);
   DDX_CBStringExactCase(pDX,IDC_HAUL_TRUCKS,pSegment->HandlingData.HaulTruckName);
}

BEGIN_MESSAGE_MAP(CGirderDescLiftingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescLiftingPage)
		// NOTE: the ClassWizard will add message map macros here
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLiftingPage message handlers
BOOL CGirderDescLiftingPage::OnInitDialog()
{
   CComboBox* pcbHaulTrucks = (CComboBox*)GetDlgItem(IDC_HAUL_TRUCKS);
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   pLibNames->EnumHaulTruckNames( &names );
   for (const auto& name : names)
   {
      pcbHaulTrucks->AddString( name.c_str() );
   }

	CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CGirderDescLiftingPage::OnSetActive() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);

   if(!pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      int cntrls[] = {IDC_STATIC_LLE,IDC_LIFTING_LOOP_LOCATION,
                      IDC_LIFTING_LOOP_LOCATION_UNITS};

      int nCtrls = sizeof(cntrls)/sizeof(int);
      for ( int ic = 0; ic < nCtrls; ic++ )
      {
         CWnd* pwnd = GetDlgItem(cntrls[ic]);
         ASSERT(pwnd);
         pwnd->EnableWindow(FALSE);
      }
   }
   
   if (!pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      int cntrls[] = {IDC_HAULINGOVERHANGS,IDC_STATIC_TLSO,IDC_TRAILINGOVERHANG,IDC_TRAILINGOVERHANG_UNITS,
                      IDC_STATIC_SLLO,IDC_LEADINGOVERHANG,IDC_LEADINGOVERHANG_UNITS,IDC_STATICSLGL,
                      IDC_GIRDERLENGTH,IDC_GIRDERLENGTH_UNIT,IDC_HAUL_TRUCKS
                      };

      int nCtrls = sizeof(cntrls)/sizeof(int);
      for ( int ic = 0; ic < nCtrls; ic++ )
      {
         CWnd* pwnd = GetDlgItem(cntrls[ic]);
         ASSERT(pwnd);
         pwnd->EnableWindow(FALSE);
      }
   }

	return CPropertyPage::OnSetActive();
}


void CGirderDescLiftingPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_TEMPORARY_CONDITIONS );
}
