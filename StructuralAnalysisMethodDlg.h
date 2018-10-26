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

#if !defined(AFX_STRUCTURALANALYSISMETHODDLG_H__52D610B9_244C_474F_B0BF_4D40DF22BF47__INCLUDED_)
#define AFX_STRUCTURALANALYSISMETHODDLG_H__52D610B9_244C_474F_B0BF_4D40DF22BF47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StructuralAnalysisMethodDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"

/////////////////////////////////////////////////////////////////////////////
// CStructuralAnalysisMethodDlg dialog

class CStructuralAnalysisMethodDlg : public CDialog
{
// Construction
public:
	CStructuralAnalysisMethodDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStructuralAnalysisMethodDlg)
	enum { IDD = IDD_STRUCTURAL_ANALYSIS_METHOD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
   pgsTypes::AnalysisType m_AnalysisType;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStructuralAnalysisMethodDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStructuralAnalysisMethodDlg)
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STRUCTURALANALYSISMETHODDLG_H__52D610B9_244C_474F_B0BF_4D40DF22BF47__INCLUDED_)
