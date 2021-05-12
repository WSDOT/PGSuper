///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "Resource.h"

#include <PgsExt\BoundaryConditionComboBox.h>

// CBoundaryConditionDlg dialog

class CBoundaryConditionDlg : public CDialog
{
	DECLARE_DYNAMIC(CBoundaryConditionDlg)

public:
	CBoundaryConditionDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CBoundaryConditionDlg();

   pgsTypes::BoundaryConditionType m_BoundaryCondition;
   PierIndexType m_PierIdx;

// Dialog Data
	enum { IDD = IDD_BOUNDARY_CONDITIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

   CBoundaryConditionComboBox m_cbBoundaryCondition;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog() override;
};
