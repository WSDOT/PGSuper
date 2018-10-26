///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
// BridgeViewPrintJob.cpp: implementation of the CBridgeViewPrintJob class.
//
//////////////////////////////////////////////////////////////////////

#include "PGSuperAppPlugin\stdafx.h"
#include "BridgeViewPrintJob.h"
#include "PGSuperCalculationSheet.h"
#include "PGSuperUnits.h"
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBridgeViewPrintJob::CBridgeViewPrintJob(CBridgeModelViewChildFrame* pFrame,CBridgePlanView* ppv, CBridgeSectionView* psv, IBroker* pBroker)
{
   ATLASSERT(ppv!=0);
   ATLASSERT(psv!=0);
   m_pPlanView=ppv;
   m_pSectionView=psv;
   m_pFrame = pFrame;
   m_pBroker = pBroker;

	m_rcMarginMM = CRect(10,10,10,10);

	m_csFtPrint = "Arial";
	m_iFtPrint = 120;

	strTitle = "Bridge View";
}

CBridgeViewPrintJob::~CBridgeViewPrintJob()
{
}

bool CBridgeViewPrintJob::OnPreparePrinting(CPrintInfo* pInfo, bool bPrintPreview)
{
	pInfo->SetMinPage(1);
   pInfo->SetMaxPage(1);

	return CPrinterJob::OnPreparePrinting(pInfo, bPrintPreview);
}

void CBridgeViewPrintJob::OnBeginPrinting(CDC * pDC, CPrintInfo * pInfo)
{
}

void CBridgeViewPrintJob::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{

}



void CBridgeViewPrintJob::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	int obkm = pDC->SetBkMode(TRANSPARENT);

   // print calc sheet border
   PGSuperCalculationSheet border(m_pBroker);
   CDocument* pdoc = m_pPlanView->GetDocument();
   CString path = pdoc->GetPathName();
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

   // print plan view on top half of page
   CPoint tl(rcPrint.TopLeft());
   CSize rsz(rcPrint.Size());
   rsz.cy = 6*rsz.cy/10;
   CRect top(tl, rsz);

   CRect pvrect(top);
   pvrect.DeflateRect(offsetx, offsety);
   pDC->SetMapMode(MM_ISOTROPIC);
   pDC->SetWindowOrg(0,0);
   pDC->SetViewportOrg(0,0);
   pDC->SetWindowExt(1,1);
   pDC->SetViewportExt(1,1);

   CRect orig_clip;
   pDC->GetClipBox(&orig_clip);

   CRgn top_clip;
   top_clip.CreateRectRgnIndirect(&top);
   pDC->SelectClipRgn(&top_clip);

   m_pPlanView->DoPrint(pDC,pInfo,pvrect);

   CRgn old_clip;
   old_clip.CreateRectRgnIndirect(&orig_clip);
   pDC->SelectClipRgn(&old_clip);


   // print section view on bottom half of page
   tl.y += rsz.cy;
   rsz.cy = rcPrint.Size().cy - rsz.cy;
   CRect svrect(tl, rsz);
   svrect.DeflateRect(offsetx, offsety,offsetx, offsety);
   
   CRgn bot_clip;
   bot_clip.CreateRectRgnIndirect(&svrect);
   pDC->SelectClipRgn(&bot_clip);

   m_pSectionView->DoPrint(pDC, pInfo, svrect); 

   CRgn bot_old_clip;
   bot_old_clip.CreateRectRgnIndirect(&orig_clip);
   pDC->SelectClipRgn(&bot_old_clip);


	pDC->SetBkMode(obkm);
   pDC->SetMapMode(oldmode);
}


