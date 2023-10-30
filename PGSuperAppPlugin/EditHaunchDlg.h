///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include "EditHaunchACamberDlg.h"
#include "EditHaunchByHaunchDlg.h"
#include "TempSupportElevAdjustGrid.h"
#include <PgsExt\HaunchShapeComboBox.h>

class CBridgeDescription2;

// CEditHaunchDlg dialog


class CEditHaunchDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditHaunchDlg)

public:
   // constructor - holds on to bridge description while dialog is active
	CEditHaunchDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent = nullptr);
	virtual ~CEditHaunchDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_HAUNCH };

   pgsTypes::HaunchInputDepthType GetHaunchInputDepthType();
   pgsTypes::HaunchLayoutType GetHaunchLayoutType();

   // embedded dialogs for different haunch layouts
   std::unique_ptr<CEditHaunchACamberDlg> m_pEditHaunchACamberDlg;
   CEditHaunchByHaunchDlg m_EditHaunchByHaunchDlg;

   CTempSupportElevAdjustGrid* m_pTempSupportElevAdjustGrid;

   CBridgeDescription2 m_BridgeDesc;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedHelp();
   afx_msg void OnHaunchDepthTypeChanged();
   afx_msg void OnHaunchLayoutTypeChanged();

private:
   bool m_bIsInPGSuper; 
   Float64 m_Fillet;
   pgsTypes::HaunchShapeType m_HaunchShape;
   CHaunchShapeComboBox m_cbHaunchShape;
   pgsTypes::HaunchInputDepthType m_HaunchInputDepthType;

   void InitializeData();
};