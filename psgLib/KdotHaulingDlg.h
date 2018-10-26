///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// CKdotHaulingDlg dialog

class CKdotHaulingDlg : public CDialog
{
	DECLARE_DYNAMIC(CKdotHaulingDlg)

public:
	CKdotHaulingDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CKdotHaulingDlg();

// Dialog Data
	enum { IDD = IDD_KDOT_HAULINGD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   static BOOL CALLBACK EnableWindows(HWND hwnd,LPARAM lParam);

public:
   void HideControls(bool hide);
	void DoCheckMax();
	void DoCheckMinLocation();

   bool m_IsHaulingEnabled;
   afx_msg void OnBnClickedCheckHaulingTensMax();
   afx_msg void OnBnClickedIsSupportLessThan();
protected:
   virtual void OnOK();
   virtual void OnCancel();
};
