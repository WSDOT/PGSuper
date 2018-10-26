///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEEDITORSECTIONPAGE_H__35E0E505_69C4_11D2_9D7D_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEEDITORSECTIONPAGE_H__35E0E505_69C4_11D2_9D7D_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeEditorSectionPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorSectionPage dialog

class CBridgeEditorSectionPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeEditorSectionPage)

// Construction
public:
	CBridgeEditorSectionPage();
	~CBridgeEditorSectionPage();

// Dialog Data
	//{{AFX_DATA(CBridgeEditorSectionPage)
	enum { IDD = IDD_BRIDGE_EDITOR_SECTION_PAGE };
	BOOL	m_LabelGirders;
	BOOL	m_ShowDimensions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeEditorSectionPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBridgeEditorSectionPage)
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEEDITORSECTIONPAGE_H__35E0E505_69C4_11D2_9D7D_00609710E6CE__INCLUDED_)
