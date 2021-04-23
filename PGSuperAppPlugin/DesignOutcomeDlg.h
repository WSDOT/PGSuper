///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#if !defined(AFX_DESIGNOUTCOMEDLG_H__4C702540_6E3F_11D4_AF04_00105A9AF985__INCLUDED_)
#define AFX_DESIGNOUTCOMEDLG_H__4C702540_6E3F_11D4_AF04_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DesignOutcomeDlg.h : header file
//

#include "resource.h"
#include <PgsExt\GirderDesignArtifact.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include "DesignGirder.h"
#include <memory>
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CDesignOutcomeDlg dialog

class CDesignOutcomeDlg : public CDialog
{
// Construction
public:
   CDesignOutcomeDlg(std::shared_ptr<CMultiGirderReportSpecification>& pRptSpec,const std::vector<CGirderKey>& girderKeys, arSlabOffsetDesignType designADim, CWnd* pParent=nullptr);

// Dialog Data
	//{{AFX_DATA(CDesignOutcomeDlg)
	enum { IDD = IDD_DESIGN_OUTCOME };
	CStatic	m_Static;
	CButton	m_Ok;
	CButton	m_Cancel;
	CButton	m_Print;
	CButton	m_Help;
   CButton m_ADesignCheckBox;
   CComboBox m_ADesignFromCombo;
   CStatic m_ADesignStatic;
   CComboBox m_ADesignToCombo;
	//}}AFX_DATA

// Operations
   // Returns true if A design exists, and returns method for setting A for design. 
   // for sotSegment, use the individual A's per girder designed.
   bool GetSlabOffsetDesign(SlabOffsetDesignSelectionType* pSoSelectionType, SpanIndexType* pFromSpan, GirderIndexType* pFromGirder);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDesignOutcomeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void CleanUp();

   std::shared_ptr<CMultiGirderReportSpecification> m_pRptSpec;
   std::shared_ptr<CReportBrowser> m_pBrowser; // this is the actual browser window that displays the report
   const std::vector<CGirderKey> m_GirderKeys;
   arSlabOffsetDesignType m_DesignADimType;

	// Generated message map functions
	//{{AFX_MSG(CDesignOutcomeDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPrint();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnHelp();
   afx_msg void OnCmenuSelected(UINT id);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedCheckAdesign();
   afx_msg void OnCbnSelchangeDesignaFrom();

private:
   SlabOffsetDesignSelectionType m_SoSelectionType;
   SpanIndexType m_FromSpanIdx;
   GirderIndexType m_FromGirderIdx;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DESIGNOUTCOMEDLG_H__4C702540_6E3F_11D4_AF04_00105A9AF985__INCLUDED_)
