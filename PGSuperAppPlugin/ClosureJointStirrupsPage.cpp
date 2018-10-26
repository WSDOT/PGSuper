
// ClosureJointStirrupsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosureJointStirrupsPage.h"
#include "ClosureJointDlg.h"

#include <EAF\EAFDisplayUnits.h>

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

// CClosureJointStirrupsPage dialog

IMPLEMENT_DYNAMIC(CClosureJointStirrupsPage, CShearSteelPage2)

CClosureJointStirrupsPage::CClosureJointStirrupsPage()
{

}

CClosureJointStirrupsPage::~CClosureJointStirrupsPage()
{
}


BEGIN_MESSAGE_MAP(CClosureJointStirrupsPage, CShearSteelPage2)
END_MESSAGE_MAP()


// CClosureJointStirrupsPage message handlers

BOOL CClosureJointStirrupsPage::OnInitDialog()
{
   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   m_ShearData = pParent->m_ClosureJoint.GetStirrups();

   CShearSteelPage2::OnInitDialog();
   
   EnableClosureJointMode();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosureJointStirrupsPage::DoDataExchange(CDataExchange* pDX)
{
   CShearSteelPage2::DoDataExchange(pDX);
   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_ClosureJoint.SetStirrups(m_ShearData);
   }
}
