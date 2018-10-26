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

#if !defined(AFX_BRIDGEEDITORSETTINGSSHEET_H__35E0E503_69C4_11D2_9D7D_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEEDITORSETTINGSSHEET_H__35E0E503_69C4_11D2_9D7D_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeEditorSettingsSheet.h : header file
//
#include "BridgeEditorPlanSettingsPage.h"
#include "BridgeEditorSectionPage.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorSettingsSheet

class CBridgeEditorSettingsSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CBridgeEditorSettingsSheet)

// Construction
public:
	CBridgeEditorSettingsSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CBridgeEditorSettingsSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeEditorSettingsSheet)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBridgeEditorSettingsSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBridgeEditorSettingsSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   void SetSettings(UINT settings);
   UINT GetSettings()const;

private:
   void Init();

   CBridgeEditorPlanSettingsPage m_BridgeEditorPlanSettingsPage;
   CBridgeEditorSectionPage      m_BridgeEditorSectionPage;
   UINT m_Settings;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEEDITORSETTINGSSHEET_H__35E0E503_69C4_11D2_9D7D_00609710E6CE__INCLUDED_)
