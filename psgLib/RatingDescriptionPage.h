///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2009  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// RatingDescriptionPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRatingDescriptionPage dialog

class CRatingDescriptionPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRatingDescriptionPage)

// Construction
public:
	CRatingDescriptionPage();
	~CRatingDescriptionPage();

// Dialog Data
	//{{AFX_DATA(CSpecDescrPage)
	enum { IDD = IDD_RATING_DESCR };
	//}}AFX_DATA


   lrfrVersionMgr::Version GetSpecVersion();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRatingDescriptionPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRatingDescriptionPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnSpecificationChanged();
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

};
