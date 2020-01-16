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

#if !defined(AFX_PIERCONNECTIONSPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
#define AFX_PIERCONNECTIONSPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierConnectionsPage.h : header file
//

#include "resource.h"
#include <PgsExt\PierData2.h>
#include <PgsExt\BoundaryConditionComboBox.h>


class CPierBearingOffsetMeasureComboBox : public CCacheComboBox
{
public:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

class CPierEndDistanceMeasureComboBox : public CCacheComboBox
{
public:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

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

   void Init(CPierData2* pPier);
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPierConnectionsPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPierConnectionsPage)
	virtual BOOL OnInitDialog() override;
   afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CBoundaryConditionComboBox m_cbBoundaryCondition;

   CPierBearingOffsetMeasureComboBox m_cbBearingOffsetMeasure;
   friend CPierBearingOffsetMeasureComboBox;
   CPierEndDistanceMeasureComboBox m_cbEndDistanceMeasure;
   friend CPierEndDistanceMeasureComboBox;

   void OnEndDistanceMeasureChanged();
   void OnBearingOffsetMeasureChanged();
   void OnBoundaryConditionChanged();

   void UpdateConnectionPicture();
   void UpdateConnectionPicture(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType);
   void UpdateConnectionPicture(ConnectionLibraryEntry::EndDistanceMeasurementType endType);
   void FillBearingOffsetComboBox();
   void FillEndDistanceComboBox();
   void FillDiaphragmLoadComboBox();
   void FillBoundaryConditionComboBox();
   CString GetImageName(pgsTypes::BoundaryConditionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType);

   CPierData2* m_pPier;

   PierIndexType m_PierIdx;

   CMetaFileStatic m_ConnectionPicture;
   std::array<CCacheEdit,2> m_BearingOffsetEdit;
   std::array<CCacheEdit,2> m_EndDistanceEdit;
   std::array<CCacheEdit,2> m_DiaphragmHeightEdit;
   std::array<CCacheEdit,2> m_DiaphragmWidthEdit;
   std::array<CCacheEdit,2> m_DiaphragmLoadLocationEdit;

   CBrush m_WhiteBrush;

   pgsTypes::BoundaryConditionType m_BoundaryConditionType;

   std::array<Float64,2> m_DiaphragmHeight;
   std::array<Float64,2> m_DiaphragmWidth;
   std::array<ConnectionLibraryEntry::DiaphragmLoadType,2> m_DiaphragmLoadType;
   std::array<Float64,2> m_DiaphragmLoadLocation;

   std::array<Float64,2> m_BearingOffset;
   std::array<Float64,2> m_EndDistance;
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType;
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType;

   void InitializeComboBoxes();

   void DisableAll();

   friend class CPierDetailsDlg;
   friend class CSpanDetailsDlg;

public:
   afx_msg void OnBackDiaphragmLoadTypeChanged();
   afx_msg void OnAheadDiaphragmLoadTypeChanged();
   afx_msg void OnCopyFromLibrary();
   virtual BOOL OnSetActive() override;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERCONNECTIONSPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
