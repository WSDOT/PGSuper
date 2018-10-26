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
#include "PierConnectionsPage.h"
#include "EditSpan.h"
#include <IFace\ExtendUI.h>

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

class CSpanDetailsDlg : public CPropertySheet, public IPierConnectionsParent, public IEditSpanData
{
	DECLARE_DYNAMIC(CSpanDetailsDlg)

// Construction
public:
	CSpanDetailsDlg(const CSpanData* pSpan,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CSpanDetailsDlg(const CSpanData* pSpan,const std::set<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
   void SetSpanData(const CSpanData* pSpan);

   //interface IPierConnectionsParent
   virtual pgsTypes::PierConnectionType GetConnectionType(PierIndexType pierIdx);
   virtual void SetConnectionType(PierIndexType pierIdx,pgsTypes::PierConnectionType type);
   virtual const CSpanData* GetPrevSpan(PierIndexType pierIdx);
   virtual const CSpanData* GetNextSpan(PierIndexType pierIdx);
   virtual const CBridgeDescription* GetBridgeDescription();

   // IEditSpanData
   virtual SpanIndexType GetSpanCount() { return m_pBridgeDesc->GetSpanCount(); }
   virtual SpanIndexType GetSpan() { return m_pSpanData->GetSpanIndex(); }
   virtual pgsTypes::PierConnectionType GetConnectionType(pgsTypes::MemberEndType end);
   virtual GirderIndexType GetGirderCount();

// Attributes
public:

// Operations
public:
   txnEditSpanData GetEditSpanData();

   // General Layout
   Float64 GetSpanLength();

   // Connections
   Float64 GetDiaphragmHeight(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetDiaphragmWidth(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   ConnectionLibraryEntry::DiaphragmLoadType GetDiaphragmLoadType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetDiaphragmLoadLocation(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   ConnectionLibraryEntry::EndDistanceMeasurementType GetEndDistanceMeasurementType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetEndDistance(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   ConnectionLibraryEntry::BearingOffsetMeasurementType GetBearingOffsetMeasurementType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetBearingOffset(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);
   Float64 GetSupportWidth(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face);


   pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   bool UseSameGirderType();
   bool UseSameNumGirders();
   //bool UseSameGirderSpacingAtEachEnd();
   CGirderSpacing GetGirderSpacing(pgsTypes::MemberEndType end);
   CGirderTypes GetGirderTypes();
   //GirderIndexType GetGirderCount();
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

   virtual INT_PTR DoModal();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

   // Generated message map functions
protected:
	//{{AFX_MSG(CSpanDetailsDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   void Init();
   void Init(const std::set<EditBridgeExtension>& editBridgeExtensions);
   void CommonInit();
   void CreateExtensionPages();
   void CreateExtensionPages(const std::set<EditBridgeExtension>& editBridgeExtensions);
   void DestroyExtensionPages();

   bool AllowConnectionChange(pgsTypes::MemberEndType end, const CString& conectionName);


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


   friend CSpanLayoutPage;
   friend CSpanGirderLayoutPage;

   CSpanLayoutPage m_SpanLayoutPage;
   CPierConnectionsPage m_StartPierPage;
   CPierConnectionsPage m_EndPierPage;
   CSpanGirderLayoutPage m_GirderLayoutPage;

   txnMacroTxn m_Macro;
   std::vector<std::pair<IEditSpanCallback*,CPropertyPage*>> m_ExtensionPages;
   std::set<EditBridgeExtension> m_BridgeExtensionPages;
   void NotifyExtensionPages();
   void NotifyBridgeExtensionPages();

   CString m_strStartPierTitle;
   CString m_strEndPierTitle;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPANDETAILSDLG_H__FE19ADCD_C53C_4556_89C3_C24327C63F62__INCLUDED_)
