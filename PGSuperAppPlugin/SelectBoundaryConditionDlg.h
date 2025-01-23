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

#pragma once

#include <PgsExt\BoundaryConditionComboBox.h>

// CSelectBoundaryConditionDlg dialog

class CSelectBoundaryConditionDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectBoundaryConditionDlg)

public:
	CSelectBoundaryConditionDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectBoundaryConditionDlg();

   pgsTypes::BoundaryConditionType m_BoundaryCondition;

   CString m_strPrompt;
   std::vector<pgsTypes::BoundaryConditionType> m_Connections;
   bool m_bIsBoundaryPier;
   bool m_bIsNoDeck;
   int m_PierType;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SELECT_BOUNDARY_CONDITION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CBoundaryConditionComboBox m_cbBoundaryCondition;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
};
