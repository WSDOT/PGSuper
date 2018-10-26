// EccentricityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EccentricityDlg.h"


// CEccentricityDlg dialog

IMPLEMENT_DYNAMIC(CEccentricityDlg, CDialog)

CEccentricityDlg::CEccentricityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEccentricityDlg::IDD, pParent)
   , m_Message(_T(""))
{

}

CEccentricityDlg::~CEccentricityDlg()
{
}

void CEccentricityDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_ECC_TEXT, m_Message);
}


BEGIN_MESSAGE_MAP(CEccentricityDlg, CDialog)
END_MESSAGE_MAP()


// CEccentricityDlg message handlers
