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

#if !defined(AFX_SECTIONVIEWDIALOG_H__614DB3B9_39C8_43C0_BAB5_8A8BD46B37A8__INCLUDED_)
#define AFX_SECTIONVIEWDIALOG_H__614DB3B9_39C8_43C0_BAB5_8A8BD46B37A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SectionViewDialog.h : header file
//

#include <GeomModel\CompositeShape.h>


/////////////////////////////////////////////////////////////////////////////
// CSectionViewDialog dialog

class CSectionViewDialog : public CDialog
{
// Construction
public:
	CSectionViewDialog(const GirderLibraryEntry* pShape,bool isEnd,CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSectionViewDialog)
	enum { IDD = IDD_SECTIONVIEW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSectionViewDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   //}}AFX_VIRTUAL

// Implementation
protected:
   CComPtr<IShape>     m_pShape;
   CComPtr<IShapeProperties> m_ShapeProps;
   const GirderLibraryEntry* m_pGirderEntry;
   bool                m_IsEnd;
   bool                m_DrawNumbers;
   Float64             m_Radius;

#ifdef _DEBUG
   std::vector< CComPtr<IShape> > m_RegionShapes; // for debugging the strandmover
#endif

   void DrawShape(CDC* pDC,grlibPointMapper& Mapper);
   void DrawShape(CDC* pDC,grlibPointMapper& Mapper,IShape* pShape);
   void DrawShape(CDC* pDC,grlibPointMapper& mapper,ICompositeShape* pCompositeShape,CBrush& solidBrush,CBrush& voidBrush);
   void DrawStrands(CDC* pDC, grlibPointMapper& Mapper, bool isEnd);
   StrandIndexType DrawStrand(CDC* pDC, grlibPointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index);


	// Generated message map functions
	//{{AFX_MSG(CSectionViewDialog)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnClickNumbers();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECTIONVIEWDIALOG_H__614DB3B9_39C8_43C0_BAB5_8A8BD46B37A8__INCLUDED_)
