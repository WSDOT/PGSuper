///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#if !defined(AFX_DESIGNGIRDERDLG_H__051156F4_8FA6_11D2_889E_006097C68A9C__INCLUDED_)
#define AFX_DESIGNGIRDERDLG_H__051156F4_8FA6_11D2_889E_006097C68A9C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DesignGirderDlg.h : header file
//

interface IBroker;
#include "PGSuperAppPlugin\resource.h"
#include "MultiGirderSelectDlg.h"

#include <IFace\Artifact.h>
/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg dialog

class CDesignGirderDlg : public CDialog
{
// Construction
public:
	CDesignGirderDlg(SpanIndexType span,GirderIndexType girder, bool enableA, bool designA, IBroker* pBroker, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDesignGirderDlg)
	enum { IDD = IDD_DESIGN_GIRDER };
	GirderIndexType		m_Girder;
	SpanIndexType   		m_Span;
	BOOL	m_DesignForFlexure;
	BOOL	m_DesignForShear;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDesignGirderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// return design options
public:
   bool m_DesignA;
   std::vector<SpanGirderHashType> m_GirderList;

// Implementation
private:
   IBroker* m_pBroker;

   bool m_EnableA; // if true, we can ask if user wants A design

   void UpdateGirderComboBox(SpanIndexType spanIdx);
   void UpdateADimCtrl();

protected:
	// Generated message map functions
	//{{AFX_MSG(CDesignGirderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnSpanChanged();
	afx_msg void OnDesignFlexure();
	//}}AFX_MSG
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

   CString m_strToolTip;
public:
   afx_msg void OnBnClickedSelectGirders();
   afx_msg void OnBnClickedRadio();
   int m_DesignRadioNum;
   BOOL m_StartWithCurrentStirrupLayout;
   afx_msg void OnBnClickedDesignShear();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DESIGNGIRDERDLG_H__051156F4_8FA6_11D2_889E_006097C68A9C__INCLUDED_)
