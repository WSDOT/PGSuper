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

#if !defined(AFX_GIRDERDIMENSIONSPAGE_H__3772ACC5_3124_11D2_9D3E_00609710E6CE__INCLUDED_)
#define AFX_GIRDERDIMENSIONSPAGE_H__3772ACC5_3124_11D2_9D3E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderDimensionsPage.h : header file
//
#if !defined INCLUDED_MFCTOOLS_METAFILESTATIC_H_
#include <MfcTools\MetaFileStatic.h>
#endif

#if !defined NOGRID
// for the grid
#ifndef _GXALL_H_
#include "gxwnd.h"
#include "gxctrl.h"
#endif
#endif // NOGRID

#include "BeamDimensionGrid.h"

// Want to be able to update the girder image as the user scrolls
// over the list of girders... the only way to do this is with
// an owner draw combo box.
class CGirderComboBox : public CComboBox
{
public:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

/////////////////////////////////////////////////////////////////////////////
// CGirderDimensionsPage dialog

class CGirderDimensionsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDimensionsPage)

// Construction
public:
	CGirderDimensionsPage();
	~CGirderDimensionsPage();

// Dialog Data
	//{{AFX_DATA(CGirderDimensionsPage)
	enum { IDD = IDD_GIRDER_DIMENSIONS };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDimensionsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
   void ViewSection(bool isEnd);

protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDimensionsPage)
   afx_msg void OnBeamTypesChanging();
	afx_msg void OnBeamTypeChanged();
   afx_msg void OnDestroy();
	//}}AFX_MSG
   afx_msg void OnViewSection();
   afx_msg void OnViewSectionMid();
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   BOOL OnInitDialog();

private:
   CMetaFileStatic m_GirderPicture;
   CBeamDimensionGrid m_Grid;
   CGirderComboBox m_cbGirder;
   int m_LastBeamType;

   void UpdateGirderImage(const CLSID& factoryCLSID);

   friend CGirderMainSheet;
   friend CGirderComboBox;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERDIMENSIONSPAGE_H__3772ACC5_3124_11D2_9D3E_00609710E6CE__INCLUDED_)
