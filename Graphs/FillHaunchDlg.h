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
#include "afxdialogex.h"

// CFillHaunchDlg dialog

class CFillHaunchDlg : public CDialog
{
	enum MethodType { mtCompute,mtAdd };

	DECLARE_DYNAMIC(CFillHaunchDlg)

public:
	CFillHaunchDlg(const CGirderKey& key, IBroker* pBroker, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFillHaunchDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILL_HAUNCH };
#endif

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	// Generated message map functions
	//{{AFX_MSG(CFillHaunchDlg)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnMethod();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();

public:
	// Change bridge based on results in dialog. return true if succeeded
	bool ModifyBridgeDescription(CBridgeDescription2& rBridgeDescription2);

private:
	bool ModifyCompute(CBridgeDescription2& rBridgeDescription2);
	bool ModifyAdd(CBridgeDescription2& rBridgeDescription2);

	// Tricky: we are setting groupIndex in m_GirderKey as user-selected span count for haunch-spans setting
   CGirderKey m_GirderKey;
	IBroker* m_pBroker;
	int m_Method; // radio settings (set to MethodType value)
	int m_ToGirderSel; // 0==girder selected, 1==all girders
	Float64 m_AddedVal; // value to be added if MethodType==mtAdd
	int m_ToBeComputedGirderIdx;
	int m_HaunchInputDistributionType; // set to pgsTypes::HaunchInputDistributionType
};
