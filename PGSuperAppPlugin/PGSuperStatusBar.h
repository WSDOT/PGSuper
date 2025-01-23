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

#if !defined(AFX_PGSUPERSTATUSBAR_H__B22464C6_7694_4E37_B401_4DB13D79B0CF__INCLUDED_)
#define AFX_PGSUPERSTATUSBAR_H__B22464C6_7694_4E37_B401_4DB13D79B0CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PGSuperStatusBar.h : header file
//

#include <EAF\EAFAutoCalcStatusBar.h>

/////////////////////////////////////////////////////////////////////////////
// CPGSuperStatusBar window

class CPGSuperStatusBar : public CEAFAutoCalcStatusBar
{
public:
	CPGSuperStatusBar();
	virtual ~CPGSuperStatusBar();

   virtual void GetStatusIndicators(const UINT** lppIDArray,int* pnIDCount) override;
   virtual BOOL SetStatusIndicators(const UINT* lpIDArray, int nIDCount) override;

   int GetProjectCriteriaPaneIndex();
   int GetAnalysisModePaneIndex();

   virtual void Reset() override;

   void SetProjectCriteria(LPCTSTR lpszCriteria);
   void SetAnalysisTypeStatusIndicator(pgsTypes::AnalysisType analysisType);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperStatusBar)
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CPGSuperStatusBar)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG


private:
   int m_ProjectCriteriaPaneIdx;
   int m_AnalysisModePaneIdx;

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPERSTATUSBAR_H__B22464C6_7694_4E37_B401_4DB13D79B0CF__INCLUDED_)
