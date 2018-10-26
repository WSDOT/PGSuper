///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
// BridgeViewPrintJob.h: interface for the CBridgeViewPrintJob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BRIDGEVIEWPRINTJOB_H__E1B3DDE2_9D53_11D1_8BAC_0000B43382FE__INCLUDED_)
#define AFX_BRIDGEVIEWPRINTJOB_H__E1B3DDE2_9D53_11D1_8BAC_0000B43382FE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <MfcTools\PrinterJob.h>
#include "BridgePlanView.h"
#include "BridgeSectionView.h"

struct IBroker;

class CBridgeViewPrintJob : public CPrinterJob  
{
public:
	CBridgeViewPrintJob(CBridgeModelViewChildFrame* pFrame,CBridgePlanView* ppv, CBridgeSectionView* psv, IBroker* pBroker);
	virtual ~CBridgeViewPrintJob();

	// virtual overridden from base class; same meaning as CView's 
	void OnBeginPrinting(CDC * pDC, CPrintInfo * pInfo);
	void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	bool OnPreparePrinting(CPrintInfo* pInfo, bool bPrintPreview = false);
	void OnPrint(CDC* pDC, CPrintInfo* pInfo);


private:
	CBridgeViewPrintJob(); // no default construction

   // data members
	CRect m_rcMarginMM;	// contain the margins in millimeters

	CString	m_csFtPrint;// font type name
	int      m_iFtPrint;	// font size

   CBridgePlanView*    m_pPv;
   CBridgeSectionView* m_pSv;
   IBroker* m_pBroker;
   CBridgeModelViewChildFrame* m_pFrame;
};

#endif // !defined(AFX_BRIDGEVIEWPRINTJOB_H__E1B3DDE2_9D53_11D1_8BAC_0000B43382FE__INCLUDED_)
