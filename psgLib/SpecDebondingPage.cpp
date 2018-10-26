// SpecDebondingPage.cpp : implementation file
//

#include "stdafx.h"
#include "psglib\psglib.h"
#include "SpecDebondingPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDebondingPage property page

IMPLEMENT_DYNCREATE(CSpecDebondingPage, CPropertyPage)

CSpecDebondingPage::CSpecDebondingPage() : CPropertyPage(CSpecDebondingPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecDebondingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecDebondingPage::~CSpecDebondingPage()
{
}

void CSpecDebondingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecDebondingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDebondingData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecDebondingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecDebondingPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDebondingPage message handlers

LRESULT CSpecDebondingPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DEBONDING_TAB );
   return TRUE;
}
