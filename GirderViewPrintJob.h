///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
// GirderViewPrintJob.h: interface for the CGirderViewPrintJob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GirderViewPRINTJOB_H__E1B3DDE2_9D53_11D1_8BAC_0000B43382FE__INCLUDED_)
#define AFX_GirderViewPRINTJOB_H__E1B3DDE2_9D53_11D1_8BAC_0000B43382FE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <MfcTools\PrinterJob.h>
#include "GirderModelChildFrame.h"

// Forward declarations
struct IBroker; 

class CGirderViewPrintJob : public CPrinterJob  
{
public:
	CGirderViewPrintJob(CGirderModelElevationView* ppv, CGirderModelSectionView* psv, CGirderModelChildFrame* pframe, IBroker* pBroker);
	virtual ~CGirderViewPrintJob();

	// virtual overridden from base class; same meaning as CView's 
	void OnBeginPrinting(CDC * pDC, CPrintInfo * pInfo);
	void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	bool OnPreparePrinting(CPrintInfo* pInfo, bool bPrintPreview = false);
	void OnPrint(CDC* pDC, CPrintInfo* pInfo);

private:
	CGirderViewPrintJob(); // no default construction

   // data members
	CRect m_rcMarginMM;	// contain the margins in millimeters

	CString	m_csFtPrint;// font type name
	int      m_iFtPrint;	// font size

   CGirderModelElevationView*    m_pEv;
   CGirderModelSectionView* m_pSv;
   CGirderModelChildFrame* m_pFrame;
   IBroker* m_pBroker;
};

#endif // !defined(AFX_GirderViewPRINTJOB_H__E1B3DDE2_9D53_11D1_8BAC_0000B43382FE__INCLUDED_)
