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

// BridgeDescLiftingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "BridgeDescLiftingPage.h"

#include "GirderDescDlg.h"
#include "PGSuperAppPlugin\GirderSegmentDlg.h"


#include "HtmlHelp\HelpTopics.hh"

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
   CPrecastSegmentData* pSegment = NULL;
   CWnd* pWnd = GetParent();
   CSegmentKey segmentKey;
   if ( pWnd->IsKindOf(RUNTIME_CLASS(CGirderDescDlg)) )
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)pWnd;
      pSegment = &pParent->m_Segment;
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

   DDX_MetaFileStatic(pDX, IDC_HAULINGOVERHANGS, m_Picture, _T("HaulingOverhangs"), _T("Metafile") );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag( pDX, IDC_LIFTING_LOOP_LOCATION, IDC_LIFTING_LOOP_LOCATION_UNITS, pSegment->HandlingData.LeftLiftPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag( pDX, IDC_STORAGE_LOCATION, IDC_STORAGE_LOCATION_UNITS, pSegment->HandlingData.LeftStoragePoint, pDisplayUnits->GetSpanLengthUnit() );
   if ( pDX->m_bSaveAndValidate )
   {
      pSegment->HandlingData.RightLiftPoint    = pSegment->HandlingData.LeftLiftPoint;
      pSegment->HandlingData.RightStoragePoint = pSegment->HandlingData.LeftStoragePoint;
   }

   DDV_UnitValueZeroOrMore( pDX, IDC_LIFTING_LOOP_LOCATION, pSegment->HandlingData.LeftLiftPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_STORAGE_LOCATION, pSegment->HandlingData.LeftStoragePoint, pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag( pDX, IDC_LEADINGOVERHANG, IDC_LEADINGOVERHANG_UNITS, pSegment->HandlingData.LeadingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_LEADINGOVERHANG, pSegment->HandlingData.LeadingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag( pDX, IDC_TRAILINGOVERHANG, IDC_TRAILINGOVERHANG_UNITS, pSegment->HandlingData.TrailingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_TRAILINGOVERHANG, pSegment->HandlingData.TrailingSupportPoint, pDisplayUnits->GetSpanLengthUnit() );

   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 gdrlength = pBridge->GetSegmentLength(segmentKey);
   DDX_UnitValueAndTag( pDX, IDC_GIRDERLENGTH, IDC_GIRDERLENGTH_UNIT, gdrlength, pDisplayUnits->GetSpanLengthUnit() );

#pragma Reminder("IMPLEMENT: Check clear span... make sure it is positive")
   Float64 clearspan = gdrlength -pSegment->HandlingData.LeadingSupportPoint - pSegment->HandlingData.TrailingSupportPoint;
//   DDV_UnitValueZeroOrMore( pDX, clearspan, bUnitsSI, usLength, siLength );
#pragma Reminder("STATUS ITEM: If leading overhang exceeds max, post status item")
//    // this should be done in an agent
//   // let the user exceed the maximum so as not to limit the program
}

BEGIN_MESSAGE_MAP(CGirderDescLiftingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescLiftingPage)
		// NOTE: the ClassWizard will add message map macros here
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLiftingPage message handlers

BOOL CGirderDescLiftingPage::OnSetActive() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   if(!pGirderLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      int cntrls[] = {IDC_STATIC_LL,IDC_STATIC_LLE,IDC_LIFTING_LOOP_LOCATION,
                      IDC_LIFTING_LOOP_LOCATION_UNITS};

      int ic=0;
      while(true)
      {
         CWnd* pwnd = GetDlgItem(cntrls[ic]);
         ASSERT(pwnd);
         pwnd->EnableWindow(FALSE);

         if (cntrls[ic++] == IDC_LIFTING_LOOP_LOCATION_UNITS)
            break;
      }
   }
   
   if (!pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      int cntrls[] = {IDC_HAULINGOVERHANGS,IDC_STATIC_TLSO,IDC_TRAILINGOVERHANG,IDC_TRAILINGOVERHANG_UNITS,
                      IDC_STATIC_SLLO,IDC_LEADINGOVERHANG,IDC_LEADINGOVERHANG_UNITS,IDC_STATICSLGL,
                      IDC_GIRDERLENGTH,IDC_GIRDERLENGTH_UNIT,
                      IDC_STATIC_SL};

      int ic=0;
      while(true)
      {
         CWnd* pwnd = GetDlgItem(cntrls[ic]);
         ASSERT(pwnd);
         pwnd->EnableWindow(FALSE);

         if (cntrls[ic++] == IDC_STATIC_SL)
            break;
      }
   }

	return CPropertyPage::OnSetActive();
}


void CGirderDescLiftingPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_LIFTING );
}
