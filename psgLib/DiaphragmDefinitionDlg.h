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

#if !defined(AFX_DIAPHRAGMDEFINITIONDLG_H__DBEED739_8347_40B7_9623_23C5E1C0B4C8__INCLUDED_)
#define AFX_DIAPHRAGMDEFINITIONDLG_H__DBEED739_8347_40B7_9623_23C5E1C0B4C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiaphragmDefinitionDlg.h : header file
//

#include "resource.h"
#include <psgLib\GirderLibraryEntry.h>
#include <IFace\BeamFactory.h>

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmDefinitionDlg dialog

class CDiaphragmDefinitionDlg : public CDialog
{
// Construction
public:
	CDiaphragmDefinitionDlg(const GirderLibraryEntry& entry,const GirderLibraryEntry::DiaphragmLayoutRule& rule,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDiaphragmDefinitionDlg)
	enum { IDD = IDD_INTERMEDATE_DIAPHRAGM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

   const GirderLibraryEntry& m_Entry;
   CComPtr<IBeamFactory> m_pBeamFactory;
   bool m_bSplicedGirder;
   GirderLibraryEntry::DiaphragmLayoutRule m_Rule;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiaphragmDefinitionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   pgsTypes::DiaphragmType ConstructionTypeToDiaphragmType(GirderLibraryEntry::ConstructionType constructionType);
   void FillMeasurementTypeComboBox();
   void FillMeasurementDatumComboBox();

	// Generated message map functions
	//{{AFX_MSG(CDiaphragmDefinitionDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnMeasurementTypeChanged();
   afx_msg void OnConstructionTypeChanged();
   afx_msg void OnMethodChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedHelp();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIAPHRAGMDEFINITIONDLG_H__DBEED739_8347_40B7_9623_23C5E1C0B4C8__INCLUDED_)
