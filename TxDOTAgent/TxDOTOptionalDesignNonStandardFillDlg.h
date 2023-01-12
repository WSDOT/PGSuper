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

#include "StrandRowGrid.h"
#include "TxDOTOptionalDesignGirderData.h"
#include "afxwin.h"

// CTxDOTOptionalDesignNonStandardFillDlg dialog

class CTxDOTOptionalDesignNonStandardFillDlg : public CDialog, public StrandRowGridEventHandler
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignNonStandardFillDlg)

public:
	CTxDOTOptionalDesignNonStandardFillDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTxDOTOptionalDesignNonStandardFillDlg();

   void Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* pBrokerRetriever);

// Dialog Data
	enum { IDD = IDD_NONSTANDARD_FILL_DLG };

   CTxDOTOptionalDesignGirderData* m_pGirderData;
   ITxDOTBrokerRetriever* m_pBrokerRetriever;
   Float64 m_yBottom;

private:
   CStrandRowGrid m_GridAtCL;
   CStrandRowGrid m_GridAtEnds;

   bool m_UseDepressed; // was used in previous version of toga to show only one grid if all straight strands
   bool m_bFirstActive;
protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog() override;

   // We aren't a property page, but we need to act like one
   BOOL OnFillSetActive();
   BOOL OnFillKillActive();

   void DoUseDepressed(bool useDepr);


   // listen to our grids
   void OnGridDataChanged();

private:
   void UpdateNoStrandsCtrls();
   void DisplayEndCtrls();

   CStatic m_CLNoStrandsCtrl;
   CStatic m_EndsNoStrandsCtrl;
public:
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
