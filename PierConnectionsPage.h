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

#if !defined(AFX_PIERCONNECTIONSPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
#define AFX_PIERCONNECTIONSPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierConnectionsPage.h : header file
//

#include <PgsExt\PierData.h>
#include "ConnectionComboBox.h"

/////////////////////////////////////////////////////////////////////////////
// CPierConnectionsPage dialog

class CPierConnectionsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPierConnectionsPage)

// Construction
public:
	CPierConnectionsPage();
	~CPierConnectionsPage();

// Dialog Data
	//{{AFX_DATA(CPierConnectionsPage)
	enum { IDD = IDD_PIERCONNECTIONS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   void Init(const CPierData* pPier);
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPierConnectionsPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPierConnectionsPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
	afx_msg void OnAheadChanged();
	afx_msg void OnBackChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CConnectionComboBox m_cbConnection;

   int  m_CacheConnectionSelection;
   int  m_CacheBoundaryConditionSelection;

   void InitializeComboBoxes();
   void FillWithConnections(CComboBox* pCB);
   void FillWithBoundaryConditions(CComboBox* pCB,bool bIncludeContinuity);
   void CopyConnection(int idcSourceConnection,int idcSourceBC,int idcTargetConnection,int idcTargetBC);

   void DisableAll();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERCONNECTIONSPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
