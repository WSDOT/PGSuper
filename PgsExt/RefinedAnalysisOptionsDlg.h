///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#if !defined(AFX_REFINEDANALYSISOPTIONSDLG_H__33C52EC4_342D_4CA2_8929_0F2A6BA3F900__INCLUDED_)
#define AFX_REFINEDANALYSISOPTIONSDLG_H__33C52EC4_342D_4CA2_8929_0F2A6BA3F900__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RefinedAnalysisOptionsDlg.h : header file
//

#include "Resource.h"

/////////////////////////////////////////////////////////////////////////////
// CRefinedAnalysisOptionsDlg dialog

class CRefinedAnalysisOptionsDlg : public CDialog
{
public:
   // options offered by this dialog
   enum RefinedAnalysisOption {lldfDefault=-1,lldfDirectInput, lldfIgnore, lldfIgnoreLever, lldfForceLever};
// Construction
public:
	CRefinedAnalysisOptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRefinedAnalysisOptionsDlg)
	enum { IDD = IDD_REFINEDANALYSIS };
	RefinedAnalysisOption		m_Choice;
	//}}AFX_DATA
   CString m_strDescription;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRefinedAnalysisOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRefinedAnalysisOptionsDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REFINEDANALYSISOPTIONSDLG_H__33C52EC4_342D_4CA2_8929_0F2A6BA3F900__INCLUDED_)
