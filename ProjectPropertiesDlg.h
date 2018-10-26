///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#if !defined(AFX_PROJECTPROPERTIESDLG_H__59D50427_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
#define AFX_PROJECTPROPERTIESDLG_H__59D50427_265C_11D2_8EB0_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProjectPropertiesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProjectPropertiesDlg dialog

class CProjectPropertiesDlg : public CDialog
{
// Construction
public:
	CProjectPropertiesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProjectPropertiesDlg)
	enum { IDD = IDD_PROJECTPROPERTIES };
	//}}AFX_DATA
	std::string	m_Bridge;
	std::string	m_BridgeID;
	std::string	m_Comments;
	std::string	m_Company;
	std::string	m_Engineer;
	std::string	m_JobNumber;
	bool	m_bShowProjectProperties;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProjectPropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProjectPropertiesDlg)
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROJECTPROPERTIESDLG_H__59D50427_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
