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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

// RatingMainSheet.h : header file
//
#include "psgLib\RatingLibraryEntry.h"

#include <units\Measure.h>
#include "RatingDescriptionPage.h"
#include "LiveLoadFactorsPage.h"

class RatingLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CRatingDialog

class CRatingDialog : public CPropertySheet
{
	DECLARE_DYNAMIC(CRatingDialog)

// Construction
public:
	CRatingDialog( RatingLibraryEntry& rentry,
      bool allowEditing,
      CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

// Attributes
public:
   bool                m_bAllowEditing;

   // work directly on an entry so we don't duplicate data.
   RatingLibraryEntry& m_Entry;
   CString m_Name;
   CString m_Description;

   //lrfrVersionMgr::Version GetSpecVersion();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRatingDialog)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRatingDialog();
	// Generated message map functions
protected:
	//{{AFX_MSG(CRatingDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   CRatingDescriptionPage m_RatingDescriptionPage;

   // for LRFR before 2013
   CLiveLoadFactorsPage*   m_LiveLoadFactorsPage1[5];
   CLiveLoadFactorsPage*   m_PermitLiveLoadFactorsPage1[4];

   // for LRFR 2013 and later
   CLiveLoadFactorsPage*   m_LiveLoadFactorsPage2[5];
   CLiveLoadFactorsPage*   m_PermitLiveLoadFactorsPage2[4];

   void ExchangeDescriptionData(CDataExchange* pDX);

   void ExchangeLoadFactorData(CDataExchange* pDX,pgsTypes::LoadRatingType ratingType);
   void ExchangeLoadFactorData(CDataExchange* pDX,pgsTypes::SpecialPermitType permitType);

   void UpdatePageLayout();

private:
   void Init();

   void ExchangeLoadFactorData(CDataExchange* pDX,CLiveLoadFactorModel* pModel);
   void ExchangeLoadFactorData(CDataExchange* pDX,CLiveLoadFactorModel2* pModel);

   void UpdatePageLayout(lrfrVersionMgr::Version version);
};
