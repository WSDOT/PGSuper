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

#if !defined(AFX_CrownSlopePage_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_)
#define AFX_CrownSlopePage_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CrownSlopePage.h : header file
//

#include "resource.h"
#include "CrownSlopeGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopePage dialog

class CCrownSlopePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CCrownSlopePage)

// Construction
public:
	CCrownSlopePage();
	~CCrownSlopePage();

// Dialog Data
	//{{AFX_DATA(CCrownSlopePage)
	enum { IDD = IDD_SUPERELEVATIONS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   RoadwaySectionData m_RoadwaySectionData;
   IBroker* GetBroker();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCrownSlopePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
public:
   void OnGridTemplateClicked(IndexType templIdx); 

protected:
	// Generated message map functions
	//{{AFX_MSG(CCrownSlopePage)
	afx_msg void OnAdd();
   afx_msg void OnUpdateAdd(CCmdUI* pCmdUI);
	afx_msg void OnRemove();
	afx_msg void OnSort();
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangeNumsegmentsCombo();
   afx_msg void OnCbnSelchangeRidgeptCombo();
   afx_msg void OnCbnSelchangeProfileGradePointCombo();
   afx_msg void OnUpdateRemove(CCmdUI* pCmdUI);
   afx_msg void OnUpdateSort(CCmdUI* pCmdUI);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

	//}}AFX_MSG

   afx_msg void OnHelp();

   void OnChange();
   void FillNumSegsCtrl();
   void UpdateNumSegsCtrl();
   void UpdateRidgeptData();
   void UpdateProfilePointData();
   void UpdateViewSpinner();

   CCrownSlopeGrid m_Grid;
   friend CCrownSlopeGrid;

	CSpinButtonCtrl m_SelTemplateSpinner;
   IndexType m_SelectedTemplate;

   int GetTemplateCount();

	DECLARE_MESSAGE_MAP()

   // tools for drawing section
   bool GetSelectedTemplate(RoadwaySectionTemplate* pTemplate);
   WBFL::Geometry::Rect2d GetRidgePointBounds();
   std::vector<WBFL::Geometry::Point2d> m_DrawnRidgePoints;
public:
   afx_msg void OnDeltaposViewTemplateSpin(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnCbnSelchangeSlopeMeasureType();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CrownSlopePage_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_)
