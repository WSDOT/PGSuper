///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// BridgeEditorAlignmentSettingsPage.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorAlignmentSettingsPage dialog

class CBridgeEditorAlignmentSettingsPage : public CPropertyPage
{
// Construction
public:
	CBridgeEditorAlignmentSettingsPage(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBridgeEditorAlignmentSettingsPage)
	enum { IDD = IDD_BRIDGE_EDITOR_ALIGNMENT_PAGE };
   BOOL m_DrawBridge;
	BOOL m_NorthUp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeEditorAlignmentSettingsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBridgeEditorAlignmentSettingsPage)
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
