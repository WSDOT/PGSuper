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
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_)
#define AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanDetailsDlg.h : header file
//

#include <PgsExt\BridgeDescription.h>
#include "SpanLayoutPage.h"
#include "GirderLayoutPage.h"
#include "SpanConnectionsPage.h"
#include "EditSpan.h"

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

class CSpanDetailsDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CSpanDetailsDlg)

// Construction
public:
	CSpanDetailsDlg(const CSpanData* pSpanData = NULL,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
   void SetSpanData(const CSpanData* pSpan);


// Attributes
public:

// Operations
public:
   txnEditSpanData GetEditSpanData();

   double GetSpanLength();

   pgsTypes::PierConnectionType GetConnectionType(pgsTypes::PierFaceType pierFace);
   const char* GetPrevPierConnection(pgsTypes::PierFaceType pierFace);
   const char* GetNextPierConnection(pgsTypes::PierFaceType pierFace);
   pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   bool UseSameGirderType();
   bool UseSameNumGirders();
   bool UseSameGirderSpacingAtEachEnd();
   CGirderSpacing GetGirderSpacing(pgsTypes::PierFaceType pierFace);
   CGirderTypes GetGirderTypes();
   GirderIndexType GetGirderCount();
   pgsTypes::MeasurementLocation GetMeasurementLocation(pgsTypes::PierFaceType pierFace);
   pgsTypes::MeasurementType GetMeasurementType(pgsTypes::PierFaceType pierFace);

   GirderIndexType GetRefGirder(pgsTypes::PierFaceType pierFace);
   double GetRefGirderOffset(pgsTypes::PierFaceType pierFace);
   pgsTypes::OffsetMeasurementType GetRefGirderOffsetType(pgsTypes::PierFaceType pierFace);

   pgsTypes::MeasurementLocation GetMeasurementLocation(); // for the entire bridge

   pgsTypes::SlabOffsetType GetSlabOffsetType();
   Float64 GetSlabOffset(pgsTypes::MemberEndType end);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpanDetailsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSpanDetailsDlg();


   // Generated message map functions
protected:
	//{{AFX_MSG(CSpanDetailsDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void Init();
   bool AllowConnectionChange(pgsTypes::PierFaceType side, const CString& conectionName);

   const CBridgeDescription* m_pBridgeDesc;
   const CPierData* m_pPrevPier;
   const CSpanData* m_pSpanData;
   const CPierData* m_pNextPier;

   void FillRefGirderOffsetTypeComboBox(pgsTypes::PierFaceType pierFace);
   void FillRefGirderComboBox(pgsTypes::PierFaceType pierFace);

   // connections
   // index is pgsTypes::PierFaceType
   // start of span is pgsTypes::Ahead, end of span is pgsTypes::Back
   pgsTypes::PierConnectionType m_ConnectionType[2];
   CString m_PrevPierConnectionName[2];
   CString m_NextPierConnectionName[2];


   friend CSpanLayoutPage;
   friend CSpanConnectionsPage;
   friend CSpanGirderLayoutPage;

   CSpanLayoutPage m_SpanLayoutPage;
   CSpanConnectionsPage m_SpanConnectionsPage;
   CSpanGirderLayoutPage m_GirderLayoutPage;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_)
