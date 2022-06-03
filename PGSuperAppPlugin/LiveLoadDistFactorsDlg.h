///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#if !defined(AFX_LIVELOADDISTFACTORSDLG_H__F7E4757E_C70D_4E53_AA9C_6579704372A8__INCLUDED_)
#define AFX_LIVELOADDISTFACTORSDLG_H__F7E4757E_C70D_4E53_AA9C_6579704372A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LiveLoadDistFactorsDlg.h : header file
//
#include "resource.h"
#include "LLDFGrid.h"
#include "LLDFPierGrid.h"
#include <PgsExt\BridgeDescription2.h>
#include <LRFD\LRFD.h>

inline GirderIndexType GetPierGirderCount(const CPierData2* pPier)
{
   // Number of girders at pier is max in attached spans
   GirderIndexType nPrevGirders(0), nNextGirders(0);

   const CSpanData2* pPrevSpan = pPier->GetPrevSpan();

   if ( pPrevSpan )
   {
      const CBridgeDescription2* pBridgeDesc = pPrevSpan->GetBridgeDescription();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pPrevSpan);
      nPrevGirders = pGroup->GetGirderCount();
   }

   const CSpanData2* pNextSpan = pPier->GetNextSpan();
   if ( pNextSpan )
   {
      const CBridgeDescription2* pBridgeDesc = pNextSpan->GetBridgeDescription();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pNextSpan);
      nNextGirders = pGroup->GetGirderCount();
   }

   GirderIndexType nGirders = Max(nPrevGirders, nNextGirders);
   return nGirders;
}

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDistFactorsDlg dialog

class CLiveLoadDistFactorsDlg : public CDialog
{
// Construction
public:
	CLiveLoadDistFactorsDlg(CWnd* pParent = nullptr);   // standard constructor
	~CLiveLoadDistFactorsDlg();

// Dialog Data
	//{{AFX_DATA(CLiveLoadDistFactorsDlg)
	enum { IDD = IDD_LLDF };
	//}}AFX_DATA

   CBridgeDescription2 m_BridgeDesc;
   LldfRangeOfApplicabilityAction m_LldfRangeOfApplicabilityAction;
   IBroker* m_pBroker;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLiveLoadDistFactorsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

   // User-input lldf grid
   CGXTabWnd m_LldfTabWnd;

   typedef std::vector< std::shared_ptr<CLLDFGrid> > GirderGridList;
   typedef GirderGridList::iterator GirderGridIterator;
   GirderGridList m_GirderGrids;

   typedef std::vector< std::shared_ptr<CLLDFPierGrid> > PierGridList;
   typedef PierGridList::iterator PierGridIterator;
   PierGridList m_PierGrids;

	// Generated message map functions
	//{{AFX_MSG(CLiveLoadDistFactorsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnMethod();
	afx_msg void OnHelp();
	afx_msg BOOL OnNcActivate(BOOL bActive);
   afx_msg void OnBnClickedLldfFillButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   void DealWithGridStates();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIVELOADDISTFACTORSDLG_H__F7E4757E_C70D_4E53_AA9C_6579704372A8__INCLUDED_)
