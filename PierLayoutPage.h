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

#if !defined(AFX_PIERLAYOUTPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
#define AFX_PIERLAYOUTPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierLayoutPage.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include <PgsExt\PierData2.h>

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage dialog
class CPierLayoutPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPierLayoutPage)

// Construction
public:
	CPierLayoutPage();
	~CPierLayoutPage();

// Dialog Data
	//{{AFX_DATA(CPierLayoutPage)
	enum { IDD = IDD_PIER_LAYOUT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   void Init(CPierData2* pPier);
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPierLayoutPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	CEdit	m_ctrlEc;
	CButton m_ctrlEcCheck;
	CEdit	m_ctrlFc;
   CString m_strUserEc;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPierLayoutPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
   afx_msg void OnChangeFc();
   afx_msg void OnUserEc();
   afx_msg void OnMoreProperties();
   afx_msg void OnPierModelTypeChanged();
   afx_msg void OnColumnShapeChanged();
   afx_msg void OnColumnCountChanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CMetaFileStatic m_LayoutPicture;
   CCacheEdit m_SpacingControl;

   void FillPierModelTypeComboBox();
   void FillRefColumnComboBox();
   void FillHeightMeasureComboBox();
   void FillColumnShapeComboBox();
   void FillTransverseLocationComboBox();

   void UpdateColumnSpacingControls();
   void UpdateEcControls();

   CPierData2* m_pPier;
   PierIndexType m_PierIdx;

   pgsTypes::PierModelType m_PierModelType;

   void UpdateConcreteTypeLabel();
   void UpdateEc();

   ColumnIndexType m_nColumns;
   ColumnIndexType m_RefColumnIdx;
   Float64 m_TransverseOffset;
   pgsTypes::OffsetMeasurementType m_TransverseOffsetMeasurement;
   Float64 m_XBeamWidth;
   Float64 m_XBeamHeight[2];
   Float64 m_XBeamTaperHeight[2];
   Float64 m_XBeamTaperLength[2];
   Float64 m_XBeamOverhang[2];

   CColumnData::ColumnHeightMeasurementType m_ColumnHeightMeasurementType;
   Float64 m_ColumnHeight;
   Float64 m_ColumnSpacing;

   CColumnData::ColumnShapeType m_ColumnShape;
   Float64 m_B;
   Float64 m_D;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERLAYOUTPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
