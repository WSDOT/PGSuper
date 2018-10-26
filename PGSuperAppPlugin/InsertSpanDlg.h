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
///////////////////////////////////////////////////////////////////////

#pragma once

// InsertSpanDlg.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include <PgsExt\BridgeDescription2.h>

/////////////////////////////////////////////////////////////////////////////
// CInsertSpanDlg dialog

class CInsertSpanDlg : public CDialog
{
// Construction
public:
   CInsertSpanDlg(const CBridgeDescription2* pBridgeDesc,CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSpecDlg)
	enum { IDD = IDD_INSERT_SPAN };
	//}}AFX_DATA

   Float64 m_SpanLength; // length of the new span
   PierIndexType m_RefPierIdx; // pier where the new span is located
   pgsTypes::PierFaceType m_PierFace; // face of pier where new span is inserted
   bool m_bCreateNewGroup; // if true, a new girder group is created
   EventIndexType m_EventIndex; // event when the pier is constructed

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertSpanDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   const CBridgeDescription2* m_pBridgeDesc;
   int m_LocationIdx;
   std::vector<std::pair<PierIndexType,pgsTypes::PierFaceType>> m_Keys;

	// Generated message map functions
	//{{AFX_MSG(CInsertSpanDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
   afx_msg void OnPierChanged();
   afx_msg void OnEventChanged();
   afx_msg void OnEventChanging();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void FillEventList();
   EventIndexType CreateEvent();

   int m_PrevEventIdx;

public:
   afx_msg void OnBnClickedNewGroup();
};
