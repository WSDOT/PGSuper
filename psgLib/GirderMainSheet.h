///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_GIRDERMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_)
#define AFX_GIRDERMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderMainSheet.h : header file
//
#include <PsgLib\GirderLibraryEntry.h>

#include "GirderHarpedStrandPage.h"
#include "GirderStraightStrandPage.h"
#include <PsgLib\ShearSteelPage.h>
#include "LongSteelPage.h"
#include "GirderHarpPointPage.h"
#include "GirderDiaphragmPage.h"
#include "GirderDimensionsPage.h"
#include "GirderDebondCriteriaPage.h"
#include "GirderErrorDlg.h"
#include "ShearDesignPage.h"
#include "GirderHaunchAndCamberPage.h"

#include <units\Measure.h>

/////////////////////////////////////////////////////////////////////////////
// CGirderMainSheet

class CGirderMainSheet : public CPropertySheet, public CShearSteelPageParent
{
	DECLARE_DYNAMIC(CGirderMainSheet)

// Construction
public:
	CGirderMainSheet( GirderLibraryEntry& rentry,  
      bool allowEditing,
      int refCount,
      CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

   void SetBeamFactory(std::shared_ptr<PGS::Beams::BeamFactory> pBeamFactory);

   bool IsSplicedGirder();

   // CShearSteelPageParent
   virtual bool HasDeck() const override;
   virtual LPCTSTR GetIntentionalRougheningPrompt() const override;

// Attributes
public:
   CGirderDimensionsPage     m_GirderDimensionsPage;
   CGirderPermanentStrandPage   m_GirderPermanentStrandPage;
   CGirderTemporaryStrandPage m_GirderTemporaryStrandPage;
   CLongSteelPage            m_LongSteelPage;
   CShearSteelPage           m_ShearSteelPage;
   CGirderHarpPointPage      m_HarpPointPage;
   CGirderDiaphragmPage      m_DiaphragmPage;
   CGirderDebondCriteriaPage m_FlexureDesignPage;
   CShearDesignPage          m_ShearDesignPage;
   CGirderHaunchAndCamberPage   m_GirderHaunchAndCamberPage;

   CGirderErrorDlg       m_GirderErrorDlg;

   bool                m_bAllowEditing;

   // work directly on an entry so we don't duplicate data.
   int m_RefCount;
   GirderLibraryEntry& m_Entry;
   CString m_Name;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderMainSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderMainSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderMainSheet)
   afx_msg void OnApply( NMHDR * pNotifyStruct, LRESULT * result );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
// implementation stuff
   // exchange dimension dialog data
   void ExchangeDimensionData(CDataExchange* pDX);
   bool ExchangeStrandData(CDataExchange* pDX);
   void UploadStrandData();
   bool ExchangeTemporaryStrandData(CDataExchange* pDX);
   void UploadTemporaryStrandData();
   void ExchangeLongitudinalData(CDataExchange* pDX);
   void UploadLongitudinalData();
   void ExchangeDiaphragmData(CDataExchange* pDX);
   void ExchangeHarpPointData(CDataExchange* pDX);
   void ExchangeHaunchData(CDataExchange* pDX);
   void ExchangeFlexuralCriteriaData(CDataExchange* pDX);
   void ExchangeDebondCriteriaData(CDataExchange* pDX);
   void ExchangeFlexuralDesignStrategyCriteriaData(CDataExchange* pDX);
   void UploadShearDesignData(CDataExchange* pDX);
   void DownloadShearDesignData(CDataExchange* pDX);

   void MiscOnFractional();
   void MiscOnAbsolute();

   // strand data available to all but strand data page
   bool CanDoAllStraightDesign() const;
   bool CanHarpStrands() const;
   bool CanDebondStrands() const;

   void SetDebondTabName();


private:
   void Init();
   void UpdatePropertyPages();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_)
