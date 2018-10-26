// GirderSegmentStirrupsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentStirrupsPage.h"
#include "GirderSegmentDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>


#include "HtmlHelp\HelpTopics.hh"

#include <LRFD\RebarPool.h>

// CGirderSegmentStirrupsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentStirrupsPage, CShearSteelPage)

CGirderSegmentStirrupsPage::CGirderSegmentStirrupsPage()
{
	//{{AFX_DATA_INIT(CGirderSegmentStirrupsPage)
	//}}AFX_DATA_INIT
}

CGirderSegmentStirrupsPage::~CGirderSegmentStirrupsPage()
{
}


BEGIN_MESSAGE_MAP(CGirderSegmentStirrupsPage, CShearSteelPage)
	//{{AFX_MSG_MAP(CGirderSegmentStirrupsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentStirrupsPage message handlers

BOOL CGirderSegmentStirrupsPage::OnInitDialog() 
{
	CShearSteelPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
