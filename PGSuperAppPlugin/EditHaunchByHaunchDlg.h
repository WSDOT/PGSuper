///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#pragma once
#include "resource.h"
#include "HaunchDirectSegmentGrid.h"
#include "HaunchDirectSpansGrid.h"
#include "HaunchDirectSameAsGrid.h"
#include "HaunchDirectEntireBridgeGrid.h"

class CBridgeDescription2;

// CEditHaunchByHaunchDlg dialog

class CEditHaunchByHaunchDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditHaunchByHaunchDlg)

public:
   // constructor - holds on to bridge description while dialog is active
	CEditHaunchByHaunchDlg(CWnd* pParent = nullptr);
	virtual ~CEditHaunchByHaunchDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_HAUNCH_BY_HAUNCH};

   // Current selections for haunch depth spec
   pgsTypes::HaunchInputLocationType m_HaunchInputLocationType;
   pgsTypes::HaunchInputDistributionType m_HaunchInputDistributionType;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHaunchInputTypeChanged();
   afx_msg void OnHaunchInputDistributionTypeChanged();
   afx_msg void OnBnClickedHelp();

   CBridgeDescription2* GetBridgeDesc();
   pgsTypes::HaunchInputLocationType GetHaunchInputLocationType();
   pgsTypes::HaunchLayoutType GetHaunchLayoutType();
   pgsTypes::HaunchInputDistributionType GetHaunchInputDistributionType();
   pgsTypes::HaunchInputDepthType GetHaunchInputDepthType();

   void OnHaunchLayoutTypeChanged();

   // Functions for our grids for setting and getting haunch values. This will convert from haunch depth or haunch+deck depth as well
   Float64 GetValueFromGrid(CString cellValue,CDataExchange* pDX, ROWCOL row, ROWCOL col, CGXGridCore* pGrid);
   CString ConvertValueToGridString(Float64 haunchValue);

   void UpdateCurrentData();
   void UpdateActiveControls();

protected:
   void UpdateGroupBox();
   void UpdateLocationTypeControl(bool bIsPGSuper);
   void InitializeData();

private:
   bool m_bNeedsGroupTabs;

   // Below is all grids that this dialog can hold. Coding-wise it would have been better to have multiple embedded
   // dialogs, each containing its own grid. However, embedded grids eat up real-estate on the main dialog, so this
   // solution was chosen to save blank space.
   CGXTabWnd m_HaunchDirectSegmentTabWnd; // each group is in a tab for PGSplice documents
   CHaunchDirectSegmentGrid* m_pHaunchDirectSegmentGrid;

   CHaunchDirectSpansGrid* m_pHaunchDirectSpansGrid;

   CGXTabWnd m_HaunchDirectSameAsSegmentsTabWnd; // each group is in a tab for PGSplice documents with multiple groups
   CHaunchDirectSameAsGrid* m_pHaunchDirectSameAsSegmentsGrid;

   CHaunchDirectSameAsGrid* m_pHaunchDirectSameAsSpansGrid;

   CHaunchDirectEntireBridgeGrid* m_pHaunchEntireBridgeGrid;
   CGXGridWnd* m_pGrid;

   const WBFL::Units::LengthData* m_pUnit;
   Float64 m_DeckThickness;
};