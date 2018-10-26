///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <PgsExt\BridgeDescription2.h>
#include "SpanLayoutPage.h"
#include "GirderLayoutPage.h"
#include "PierConnectionsPage.h"
#include "EditSpan.h"

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

class CSpanDetailsDlg : public CPropertySheet, public IPierConnectionsParent
{
	DECLARE_DYNAMIC(CSpanDetailsDlg)

// Construction
public:
	CSpanDetailsDlg(const CSpanData2* pSpanData = NULL,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
   void SetSpanData(const CSpanData2* pSpan);

   //interface IPierConnectionsParent
   virtual pgsTypes::PierConnectionType GetPierConnectionType(PierIndexType pierIdx);
   virtual void SetPierConnectionType(PierIndexType pierIdx,pgsTypes::PierConnectionType type);
   virtual pgsTypes::PierSegmentConnectionType GetSegmentConnectionType(PierIndexType pierIdx);
   virtual void SetSegmentConnectionType(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType type);
   virtual const CSpanData2* GetPrevSpan(PierIndexType pierIdx);
   virtual const CSpanData2* GetNextSpan(PierIndexType pierIdx);
   virtual const CBridgeDescription2* GetBridgeDescription();

// Attributes
public:

// Operations
public:
   txnEditSpanData GetEditSpanData();

   // General Layout
   Float64 GetSpanLength();

   // Connections
   pgsTypes::PierConnectionType GetConnectionType(pgsTypes::MemberEndType end);
   Float64 GetDiaphragmHeight(pgsTypes::MemberEndType end);
   Float64 GetDiaphragmWidth(pgsTypes::MemberEndType end);
   ConnectionLibraryEntry::DiaphragmLoadType GetDiaphragmLoadType(pgsTypes::MemberEndType end);
   Float64 GetDiaphragmLoadLocation(pgsTypes::MemberEndType end);
   ConnectionLibraryEntry::EndDistanceMeasurementType GetEndDistanceMeasurementType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetEndDistance(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   ConnectionLibraryEntry::BearingOffsetMeasurementType GetBearingOffsetMeasurementType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetBearingOffset(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetSupportWidth(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);


   pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   bool UseSameGirderType();
   bool UseSameNumGirders();
   CGirderSpacing2 GetGirderSpacing(pgsTypes::MemberEndType end);
   const CGirderGroupData& GetGirderGroup() const;
   GirderIndexType GetGirderCount() const;
   pgsTypes::MeasurementLocation GetMeasurementLocation(pgsTypes::MemberEndType end);
   pgsTypes::MeasurementType GetMeasurementType(pgsTypes::MemberEndType end);

   GirderIndexType GetRefGirder(pgsTypes::MemberEndType end);
   Float64 GetRefGirderOffset(pgsTypes::MemberEndType end);
   pgsTypes::OffsetMeasurementType GetRefGirderOffsetType(pgsTypes::MemberEndType end);

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
   bool AllowConnectionChange(pgsTypes::MemberEndType end, const CString& conectionName);

   const CBridgeDescription2* m_pBridgeDesc;
   const CPierData2* m_pPrevPier;
   const CSpanData2* m_pSpanData;
   const CPierData2* m_pNextPier;
   const CGirderGroupData* m_pGirderGroup;

   pgsTypes::PierConnectionType m_PierConnectionType[2];
   pgsTypes::PierSegmentConnectionType m_SegmentConnectionType[2];

   void FillRefGirderOffsetTypeComboBox(pgsTypes::MemberEndType end);
   void FillRefGirderComboBox(pgsTypes::MemberEndType end);

   friend CSpanLayoutPage;
   friend CSpanGirderLayoutPage;

   CSpanLayoutPage m_SpanLayoutPage;
   CPierConnectionsPage m_StartPierPage;
   CPierConnectionsPage m_EndPierPage;
   CSpanGirderLayoutPage m_GirderLayoutPage;

   CString m_strStartPierTitle;
   CString m_strEndPierTitle;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_)
