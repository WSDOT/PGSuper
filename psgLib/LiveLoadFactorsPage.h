///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

// CLiveLoadFactorsPage dialog

class CLiveLoadFactorsPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CLiveLoadFactorsPage)

public:
   CLiveLoadFactorsPage(LPCTSTR strTitle,pgsTypes::LoadRatingType ratingType);
   CLiveLoadFactorsPage(LPCTSTR strTitle,pgsTypes::LoadRatingType ratingType,pgsTypes::SpecialPermitType permitType);
	virtual ~CLiveLoadFactorsPage();

// Dialog Data
	enum { IDD = IDD_LIVE_LOAD_FACTORS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   pgsTypes::LoadRatingType m_RatingType;
   pgsTypes::SpecialPermitType m_SpecialPermitType; // only used if rating type is lrPermit
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnLiveLoadFactorTypeChanged();
};
