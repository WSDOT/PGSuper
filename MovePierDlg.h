///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#if !defined(AFX_MOVEPIERDLG_H__DF27287A_F489_4661_8E66_F17E6EFCC29D__INCLUDED_)
#define AFX_MOVEPIERDLG_H__DF27287A_F489_4661_8E66_F17E6EFCC29D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MovePierDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"

/////////////////////////////////////////////////////////////////////////////
// CMovePierDlg dialog

class CMovePierDlg : public CDialog
{
// Construction
public:
	CMovePierDlg(PierIndexType pierIdx,Float64 fromStation,Float64 toStation,Float64 prevPierStation,Float64 nextPierStation,SpanIndexType nSpans,const unitStationFormat& stationFormat,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMovePierDlg)
	enum { IDD = IDD_MOVEPIER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

   pgsTypes::MovePierOption m_Option;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMovePierDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   PierIndexType m_PierIdx;
   SpanIndexType m_nSpans;
   Float64 m_FromStation;
   Float64 m_ToStation;
   Float64 m_PrevPierStation;
   Float64 m_NextPierStation;
   const unitStationFormat& m_StationFormat;

	// Generated message map functions
	//{{AFX_MSG(CMovePierDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOVEPIERDLG_H__DF27287A_F489_4661_8E66_F17E6EFCC29D__INCLUDED_)
