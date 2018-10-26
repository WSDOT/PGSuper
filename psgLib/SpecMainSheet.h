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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPECMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_)
#define AFX_SPECMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SpecMainSheet.h : header file
//
#include "psgLib\SpecLibraryEntry.h"
#include "SpecDescrPage.h"
#include "SpecLiftingPage.h"
#include "SpecHaulingErectionPage.h"
#include "SpecCreepPage.h"
#include "SpecLossPage.h"
#include "SpecPSLimitPage.h"
#include "SpecLimitsPage.h"
#include "SpecDesignPage.h"
#include "SpecShearPage.h"
#include "SpecMomentPage.h"
#include "SpecLoadsPage.h"
#include "SpecGirderStressPage.h"
#include "SpecClosurePage.h"

class SpecLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CSpecMainSheet

class CSpecMainSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CSpecMainSheet)

// Construction
public:
	CSpecMainSheet( SpecLibraryEntry& rentry, UINT nIDCaption, 
      bool allowEditing,
      CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CSpecMainSheet( SpecLibraryEntry& rentry, LPCTSTR pszCaption, 
      bool allowEditing,
      CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
   CSpecDescrPage           m_SpecDescrPage;
   CSpecHaulingErectionPage m_SpecHaulingErectionPage;
   CSpecCreepPage           m_SpecCreepPage;
   CSpecLiftingPage         m_SpecLiftingPage;
   CSpecLossPage            m_SpecLossPage;
   CSpecStrandPage          m_SpecStrandStressPage;
   CSpecLimitsPage          m_SpecLimitsPage;
   CSpecDesignPage          m_SpecDesignPage;
   CSpecShearPage           m_SpecShearPage;
   CSpecMomentPage          m_SpecMomentPage;

   CSpecClosurePage         m_SpecClosurePage;

   CSpecLoadsPage          m_SpecLoadsPage;
   CSpecGirderStressPage m_SpecGirderStressPage;

   bool                m_AllowEditing;

   // work directly on an entry so we don't duplicate data.
   SpecLibraryEntry& m_Entry;
   CString m_Name;
   CString m_Description;

   lrfdVersionMgr::Version GetSpecVersion();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpecMainSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSpecMainSheet();
	// Generated message map functions
protected:
	//{{AFX_MSG(CSpecMainSheet)
	//}}AFX_MSG
   DECLARE_MESSAGE_MAP()
public:
// implementation stuff

   void ExchangeDescriptionData(CDataExchange* pDX);
   void ExchangeLoadsData(CDataExchange* pDX);
   void ExchangeGirderData(CDataExchange* pDX);
   void ExchangeLiftingData(CDataExchange* pDX);
   bool IsHaulingEnabled() const;
   void ExchangeWsdotHaulingData(CDataExchange* pDX);
   void ExchangeKdotHaulingData(CDataExchange* pDX);
   void ExchangeShearCapacityData(CDataExchange* pDX);
   void ExchangeMomentCapacityData(CDataExchange* pDX);
   void ExchangeCreepData(CDataExchange* pDX);
   void ExchangeLossData(CDataExchange* pDX);
   void ExchangeStrandData(CDataExchange* pDX);
   void ExchangeLimitsData(CDataExchange* pDX);
   void ExchangeClosureData(CDataExchange* pDX);

   void ExchangeDesignData(CDataExchange* pDX);


private:
   void Init();
   void CheckShearCapacityMethod();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_)
