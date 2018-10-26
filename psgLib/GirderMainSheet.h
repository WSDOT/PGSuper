///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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
#include "ShearSteelPage.h"
#include "LongSteelPage.h"
#include "GirderHarpPointPage.h"
#include "GirderDiaphragmPage.h"
#include "GirderDimensionsPage.h"
#include "GirderDebondCriteriaPage.h"
#include "GirderErrorDlg.h"

#if !defined INCLUDED_LIBRARYFW_UNITSMODE_H_
#include <LibraryFw\UnitsMode.h>
#endif

#include <units\Measure.h>

/////////////////////////////////////////////////////////////////////////////
// CGirderMainSheet

class CGirderMainSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CGirderMainSheet)

// Construction
public:
	CGirderMainSheet( GirderLibraryEntry& rentry, UINT nIDCaption, 
      libUnitsMode::Mode mode,  bool allowEditing,
      CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CGirderMainSheet( GirderLibraryEntry& rentry, LPCTSTR pszCaption, 
      libUnitsMode::Mode mode,  bool allowEditing,
      CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
   CGirderDimensionsPage     m_GirderDimensionsPage;
   CGirderHarpedStrandPage   m_GirderHarpedStrandPage;
   CGirderStraightStrandPage m_GirderStraightStrandPage;
   CLongSteelPage            m_LongSteelPage;
   CShearSteelPage           m_ShearSteelPage;
   CGirderHarpPointPage      m_HarpPointPage;
   CGirderDiaphragmPage      m_DiaphragmPage;
   CGirderDebondCriteriaPage m_GirderDebondCriteriaPage;

   CGirderErrorDlg       m_GirderErrorDlg;

   libUnitsMode::Mode  m_Mode;
   bool                m_AllowEditing;

   unitLength          m_LongLengthUnit;
   CString             m_LongLengthUnitString;
   unitLength          m_ShortLengthUnit;
   CString             m_ShortLengthUnitString;

   // work directly on an entry so we don't duplicate data.
   GirderLibraryEntry& m_Entry;
   CString m_Name;

   // top flange bar spacing;
   Float64 m_TfBarSpacing;

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
   void ExchangeTransverseData(CDataExchange* pDX);
   void UploadTransverseData();
   void ExchangeDiaphragmData(CDataExchange* pDX);
   void ExchangeHarpPointData(CDataExchange* pDX);
   void ExchangeDebondCriteriaData(CDataExchange* pDX);
   CString GetLongLengthUnitString() const { return m_LongLengthUnitString; }
   unitLength GetLongLengthUnit() const { return m_LongLengthUnit; }
   CString GetShortLengthUnitString() const { return m_ShortLengthUnitString; }
   unitLength GetShortLengthUnit() const { return m_ShortLengthUnit; }

   void MiscOnFractional();
   void MiscOnAbsolute();

private:
   void Init();

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMAINSHEET_H__0186F183_31F0_11D2_9D3F_00609710E6CE__INCLUDED_)
