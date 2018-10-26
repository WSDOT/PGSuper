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

#if !defined(AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_)
#define AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanDetailsDlg.h : header file
//

#include <PgsExt\BridgeDescription2.h>
#include "SpanLayoutPage.h"
#include "GirderLayoutPage.h"
#include "PierConnectionsPage.h"
#include "EditSpan.h"

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

class CSpanDetailsDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CSpanDetailsDlg)

// Construction
public:
	CSpanDetailsDlg(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CSpanDetailsDlg();

// Attributes
public:
   const CBridgeDescription2* GetBridgeDescription();

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpanDetailsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:


   // Generated message map functions
protected:
	//{{AFX_MSG(CSpanDetailsDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void InitPages();
   void Init(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx);

   CBridgeDescription2 m_BridgeDesc; // this is the bridge we are operating on
   CPierData2* m_pPrevPier; // points to elements inside our private bridge model to make life easier
   CSpanData2* m_pSpanData;
   CPierData2* m_pNextPier;
   CGirderGroupData* m_pGirderGroup;

   pgsTypes::PierConnectionType m_PierConnectionType[2];
   pgsTypes::PierSegmentConnectionType m_SegmentConnectionType[2];

   void FillRefGirderOffsetTypeComboBox(pgsTypes::MemberEndType end);
   void FillRefGirderComboBox(pgsTypes::MemberEndType end);

   friend CSpanLayoutPage;
   friend CSpanGirderLayoutPage;

   CSpanLayoutPage m_SpanLayoutPage;
   CPierConnectionsPage m_StartPierPage;
   CPierConnectionsPage m_EndPierPage;
   CSpanGirderLayoutPage m_GirderLayoutPage;

   CString m_strStartPierTitle;
   CString m_strEndPierTitle;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_)
