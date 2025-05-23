///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#pragma once

#include <PsgLib\ShearSteelPage.h>

// CGirderSegmentStirrupsPage dialog

class CGirderSegmentStirrupsPage : public CShearSteelPage
{
	DECLARE_DYNCREATE(CGirderSegmentStirrupsPage)

// Construction
public:
	CGirderSegmentStirrupsPage();
	~CGirderSegmentStirrupsPage();


// Dialog Data
	//{{AFX_DATA(CGirderDescShearPage)
	//}}AFX_DATA

   //CGirderDescShearGrid m_Grid;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderSegmentStirrupsPage)
	public:
      virtual void GetLastZoneName(CString& strSymmetric, CString& strEnd) override;

protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderSegmentStirrupsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual UINT GetHelpID() { return IDH_GIRDERDETAILS_TRANSV_REBAR; }
};
