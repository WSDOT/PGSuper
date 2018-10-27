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
