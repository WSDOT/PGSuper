///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#if !defined(AFX_TrafficBarrierViewDIALOG_H__614DB3B9_39C8_43C0_BAB5_8A8BD46B37A8__INCLUDED_)
#define AFX_TrafficBarrierViewDIALOG_H__614DB3B9_39C8_43C0_BAB5_8A8BD46B37A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrafficBarrierViewDialog.h : header file
//

#include <GeomModel\CompositeShape.h>


#define HARPED_COLOR   RGB(80 ,200 ,80)
#define STRAIGHT_COLOR RGB(80 ,80   ,250)
#define TEMP_COLOR     RGB(200,80  ,80)
#define SHAPE_COLOR    RGB(255,255,0)
#define VOID_COLOR     RGB(255,255,255)

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierViewDialog dialog

class CTrafficBarrierViewDialog : public CDialog
{
// Construction
public:
   CTrafficBarrierViewDialog(IShape* pShape, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTrafficBarrierViewDialog)
	enum { IDD = IDD_TRAFFICBARRIERVIEW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrafficBarrierViewDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   CComPtr<IShape> m_pShape;

   void DrawShape(CDC* pDC,grlibPointMapper& Mapper);
   void DrawShape(CDC* pDC,grlibPointMapper& Mapper,IShape* pShape);

	// Generated message map functions
	//{{AFX_MSG(CTrafficBarrierViewDialog)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TrafficBarrierViewDIALOG_H__614DB3B9_39C8_43C0_BAB5_8A8BD46B37A8__INCLUDED_)
