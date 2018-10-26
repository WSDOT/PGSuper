
// ClosurePourStirrupsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosurePourStirrupsPage.h"
#include "ClosurePourDlg.h"

#include <EAF\EAFDisplayUnits.h>

#include "HtmlHelp\HelpTopics.hh"

#include <LRFD\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////
// NOTE
// The implementation of this class is very similar to CGirderDescShearPage
// Changes made to this class are probably needed there as well.
//////////////////////////////////////////////////////////////////////////////////////

// CClosurePourStirrupsPage dialog

IMPLEMENT_DYNAMIC(CClosurePourStirrupsPage, CShearSteelPage2)

CClosurePourStirrupsPage::CClosurePourStirrupsPage()
{

}

CClosurePourStirrupsPage::~CClosurePourStirrupsPage()
{
}


BEGIN_MESSAGE_MAP(CClosurePourStirrupsPage, CShearSteelPage2)
END_MESSAGE_MAP()


// CClosurePourStirrupsPage message handlers

BOOL CClosurePourStirrupsPage::OnInitDialog()
{
   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();
   m_ShearData = pParent->m_ClosurePour.GetStirrups();

   CShearSteelPage2::OnInitDialog();
   
   EnableClosurePourMode();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosurePourStirrupsPage::DoDataExchange(CDataExchange* pDX)
{
   CShearSteelPage2::DoDataExchange(pDX);
   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_ClosurePour.SetStirrups(m_ShearData);
   }
}
