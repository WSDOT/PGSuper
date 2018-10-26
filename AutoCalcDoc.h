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

#if !defined(AFX_AUTOCALCDOC_H__0A979192_6385_11D2_8EDC_006097DF3C68__INCLUDED_)
#define AFX_AUTOCALCDOC_H__0A979192_6385_11D2_8EDC_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AutoCalcDoc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcDoc document

class CAutoCalcDoc : public CDocument
{
protected:
	CAutoCalcDoc();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
   virtual bool IsAutoCalcEnabled() const = 0;
   virtual void EnableAutoCalc(bool bEnable) = 0;

   void OnUpdateNow();
	void OnUpdateUpdateNow(CCmdUI* pCmdUI);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoCalcDoc)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAutoCalcDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CAutoCalcDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOCALCDOC_H__0A979192_6385_11D2_8EDC_006097DF3C68__INCLUDED_)
