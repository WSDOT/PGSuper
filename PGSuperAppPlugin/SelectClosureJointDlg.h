///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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


#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\BridgeDescription2.h>

// CSelectClosureJointDlg dialog

class CSelectClosureJointDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectClosureJointDlg)

public:
	CSelectClosureJointDlg(const CBridgeDescription2* pBridgeDesc,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectClosureJointDlg();

   PierIndexType m_PierIdx;
   SupportIDType m_TempSupportID;
   GirderIndexType m_GirderIdx;

// Dialog Data
	enum { IDD = IDD_SELECT_CLOSURE_JOINT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

   void FillSupportComboBox();
   void FillGirderComboBox(GroupIndexType grpIdx);

   CComPtr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnSupportChanged();
   virtual BOOL OnInitDialog() override;
};
