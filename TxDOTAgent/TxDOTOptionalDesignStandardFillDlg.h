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

#include "TxDOTOptionalDesignGirderData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

class CTxDOTOptionalDesignGirderInputPage;
// CTxDOTOptionalDesignStandardFillDlg dialog

class CTxDOTOptionalDesignStandardFillDlg : public CDialog
{
public:
   friend CTxDOTOptionalDesignGirderInputPage;

	DECLARE_DYNAMIC(CTxDOTOptionalDesignStandardFillDlg)

public:
	CTxDOTOptionalDesignStandardFillDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTxDOTOptionalDesignStandardFillDlg();

// Dialog Data
	enum { IDD = IDD_STANDARD_FILL_DLG };

   CTxDOTOptionalDesignGirderData* m_pGirderData;
   CString m_GirderEntryName;
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

   CString m_strNumStrands;
   StrandIndexType m_NumStrands;
   Float64 m_To;

public:
   void LoadDialogData();
   void SaveDialogData();
   void Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* m_pBrokerRetriever, LPCTSTR entryName);
   void UpdateLibraryData();
   void InitStrandNoCtrl();
   void UpdateControls();

   // common girder dialog calls
   void SetGirderEntryName(LPCTSTR entryName);
   BOOL OnFillSetActive();
   BOOL OnFillKillActive();

   bool m_bFirstActive;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnCbnSelchangeOptNumStrands();
   virtual BOOL OnInitDialog() override;
   afx_msg void OnBnClickedOptCompute();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
