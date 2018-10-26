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
#include "PGSuperAppPlugin\resource.h"
#include "EditRatingCriteria.h"


// CPermitRatingPage dialog

class CPermitRatingPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CPermitRatingPage)

public:
	CPermitRatingPage();
	virtual ~CPermitRatingPage();

	CCheckListBox 	m_ctlRoutineLL;
	CCheckListBox 	m_ctlSpecialLL;

   std::vector<std::_tstring> m_AllNames;
   txnPermitRatingData m_Data;

// Dialog Data
	enum { IDD = IDD_PERMIT_RATING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);

   CString m_strTip; // buffer that holds the tooltip text

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnCheckYieldingClicked();
   afx_msg void OnPermitTypeChanged();
   afx_msg void OnHelp();
   virtual BOOL OnSetActive();

   pgsTypes::SpecialPermitType GetSpecialPermitType();
   afx_msg void OnRateForStressChanged();
};
