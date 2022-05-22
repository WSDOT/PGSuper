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
//
// GirderViewPrintJob.cpp: implementation of the CGirderViewPrintJob class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "GirderViewPrintJob.h"
#include "PGSuperCalculationSheet.h"
#include "GirderModelElevationView.h"
#include "GirderModelSectionView.h"

#include <IFace\VersionInfo.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGirderViewPrintJob::CGirderViewPrintJob(CGirderModelElevationView* pev, 
                                         CGirderModelSectionView* psv, 
                                         CGirderModelChildFrame* pframe,
                                         IBroker* pBroker)
{
   ATLASSERT(pev!=0);
   ATLASSERT(psv!=0);
   m_pElevationView=pev;
   m_pSectionView=psv;
   m_pBroker = pBroker;
   m_pFrame = pframe;

	m_rcMarginMM = CRect(10,10,10,10);

	m_csFtPrint = "Arial";
	m_iFtPrint = 120;

	strTitle = "Girder View";
}

CGirderViewPrintJob::~CGirderViewPrintJob()
{
}

bool CGirderViewPrintJob::OnPreparePrinting(CPrintInfo* pInfo, bool bPrintPreview)
{
	pInfo->SetMinPage(1);
   pInfo->SetMaxPage(1);

	return CPrinterJob::OnPreparePrinting(pInfo, bPrintPreview);
}

void CGirderViewPrintJob::OnBeginPrinting(CDC * pDC, CPrintInfo * pInfo)
{
}

void CGirderViewPrintJob::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{

}

void CGirderViewPrintJob::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	int obkm = pDC->SetBkMode(TRANSPARENT);

   // print calc sheet border
   PGSuperCalculationSheet border(m_pBroker);
   CString title;

   CDocument* pDoc = (CDocument*)(m_pSectionView->GetDocument());

   GET_IFACE(IVersionInfo,pVerInfo);
   GET_IFACE(IDocumentType,pDocType);

   const CGirderKey& girderKey = m_pFrame->GetSelection();
   if ( pDocType->IsPGSuperDocument() )
   {
      if ( girderKey.groupIndex == ALL_GROUPS )
         title.Format(_T("Girder %s - PGSuper™ Version %s, Copyright © %4d, WSDOT, All rights reserved"), LABEL_GIRDER(girderKey.girderIndex), pVerInfo->GetVersion(), WBFL::System::Date().Year());  
      else
         title.Format(_T("Span %s Girder %s - PGSuper™ Version %s, Copyright © %4d, WSDOT, All rights reserved"), LABEL_SPAN(girderKey.groupIndex), LABEL_GIRDER(girderKey.girderIndex), pVerInfo->GetVersion(), WBFL::System::Date().Year());
   }
   else
   {
      if ( girderKey.groupIndex == ALL_GROUPS )
         title.Format(_T("Girder %s - PGSplice™ Version %s, Copyright © %4d, WSDOT, All rights reserved"), LABEL_GIRDER(girderKey.girderIndex), pVerInfo->GetVersion(), WBFL::System::Date().Year());
      else
         title.Format(_T("Group %d Girder %s - PGSplice™ Version %s, Copyright © %4d, WSDOT, All rights reserved"), LABEL_GROUP(girderKey.groupIndex), LABEL_GIRDER(girderKey.girderIndex), pVerInfo->GetVersion(), WBFL::System::Date().Year());
   }

   border.SetTitle(title);

   CString path = pDoc->GetPathName();
   border.SetFileName(path);
   CRect rcPrint = border.Print(pDC, 1);

   if (rcPrint.IsRectEmpty())
   {
      CHECKX(0,_T("Can't print border - page too small?"));
      rcPrint = pInfo->m_rectDraw;
   }

   // want to offset picture away from borders - get device units for 10mm
   int oldmode = pDC->SetMapMode(MM_LOMETRIC);
   POINT offset[2] = { {0,0}, {100,-100}};
   pDC->LPtoDP(offset,2);
   int offsetx = offset[1].x - offset[0].x;
   int offsety = offset[1].y - offset[0].y;

   // print elevation view on top half of page
   CPoint tl(rcPrint.TopLeft());
   CSize rsz(rcPrint.Size());
   rsz.cy = rsz.cy*4/10;
   CRect top(tl, rsz);

   CRect pvrect(top);
   pvrect.DeflateRect(offsetx, offsety);
   pDC->SetMapMode(MM_TEXT);
   pDC->SetWindowOrg(0,0);
   pDC->SetViewportOrg(0,0);
   CRect orig_clip;
   pDC->GetClipBox(&orig_clip);

   CRgn top_clip;
   top_clip.CreateRectRgnIndirect(&top);
   pDC->SelectClipRgn(&top_clip);

   m_pElevationView->DoPrint(pDC,pInfo);

   CRgn old_clip;
   old_clip.CreateRectRgnIndirect(&orig_clip);
   pDC->SelectClipRgn(&old_clip);

   // label
   CFont pvf;
   pvf.CreatePointFont(m_iFtPrint, m_csFtPrint, pDC);
   CFont* oldfont = pDC->SelectObject(&pvf);
   pDC->SetTextAlign(TA_LEFT|TA_TOP);
   GET_IFACE(IEventMap,pEventMap);
   CString tstr = pEventMap->GetEventName(m_pFrame->GetEvent());
   CString topcap = _T("Elevation View (") + tstr + _T(")");
   CSize csiz = pDC->GetTextExtent( topcap );
   int x = (rcPrint.left+rcPrint.right)/2 - csiz.cx/2;
   int y = pvrect.bottom + 2*csiz.cy;
   pDC->TextOut(x,y,topcap);
   pDC->SelectObject(oldfont);

   // print section view on bottom half of page
   tl.y += rsz.cy;
   rsz.cy = rcPrint.Size().cy - rsz.cy;
   CRect svrect(tl, rsz);
   svrect.DeflateRect(offsetx, offsety,offsetx, offsety);
   CRgn bot_clip;
   bot_clip.CreateRectRgnIndirect(&svrect);
   pDC->SelectClipRgn(&bot_clip);

   m_pSectionView->DoPrint(pDC, pInfo);

   CRgn bot_old_clip;
   bot_old_clip.CreateRectRgnIndirect(&orig_clip);
   pDC->SelectClipRgn(&bot_old_clip);

   // label
   // get length unit for labelling
   GET_IFACE(IEAFDisplayUnits,pdisp_units);
   const WBFL::Units::LengthData& rlen = pdisp_units->GetSpanLengthUnit();
   WBFL::System::NumericFormatTool nf(rlen.Format, rlen.Width, rlen.Precision);
   Float64 dist = WBFL::Units::ConvertFromSysUnits(m_pFrame->GetCurrentCutLocation(), rlen.UnitOfMeasure);
   CString msg;
   msg.Format(_T("Section Cut At %s %s"),nf.AsString(dist).c_str(), rlen.UnitOfMeasure.UnitTag().c_str());

   CFont svf;
   svf.CreatePointFont(m_iFtPrint, m_csFtPrint, pDC);
   oldfont = pDC->SelectObject(&svf);
   pDC->SetTextAlign(TA_LEFT|TA_TOP);
   csiz = pDC->GetTextExtent( msg );
   x = (rcPrint.left+rcPrint.right)/2 - csiz.cx/2;
   y = svrect.bottom;
   pDC->TextOut(x,y,msg);
   pDC->SelectObject(oldfont);

	pDC->SetBkMode(obkm);
   pDC->SetMapMode(oldmode);
}


