// FileSaveWarningDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "FileSaveWarningDlg.h"
#include <EAF\EAFDocument.h>


// CFileSaveWarningDlg dialog

IMPLEMENT_DYNAMIC(CFileSaveWarningDlg, CDialog)

CFileSaveWarningDlg::CFileSaveWarningDlg(LPCTSTR lpszAppName, LPCTSTR lpszFileName, LPCTSTR lpszCopyFileName, const CString& strOldVersion, const CString& strCurrentVersion, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_FILESAVEWARNINGDLG, pParent), m_strCopyFileName(lpszCopyFileName)
{
   m_CopyOption = FSW_COPY;
   m_DefaultCopyOption = FSW_COPY;
   m_bDontWarn = false;

   TCHAR title[_MAX_PATH];
   WORD cbBuf = _MAX_PATH;

   ::GetFileTitle(lpszFileName, title, cbBuf);
   m_strLabel.Format(_T("%s was created with %s version %s. It will be converted to version %s the next time it is saved. You will no longer be able to open it with an earlier version."),title,lpszAppName,strOldVersion,strCurrentVersion);
}

CFileSaveWarningDlg::~CFileSaveWarningDlg()
{
}

void CFileSaveWarningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   DDX_Radio(pDX, IDC_RB_COPY, m_CopyOption);
   DDX_Check_Bool(pDX, IDC_DONT_WARN, m_bDontWarn);
   DDX_CBIndex(pDX, IDC_DEFAULT_OPTIONS, m_DefaultCopyOption);
}


BEGIN_MESSAGE_MAP(CFileSaveWarningDlg, CDialog)
   ON_BN_CLICKED(IDC_DONT_WARN, &CFileSaveWarningDlg::OnClickedDontWarn)
   ON_BN_CLICKED(IDC_HELP, &CFileSaveWarningDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CFileSaveWarningDlg message handlers
BOOL CFileSaveWarningDlg::OnInitDialog()
{
   CWnd* pwndLabel = GetDlgItem(IDC_LABEL);
   pwndLabel->SetWindowText(m_strLabel);

   TCHAR title[_MAX_PATH];
   WORD cbBuf = _MAX_PATH;
   ::GetFileTitle(m_strCopyFileName, title, cbBuf);
   CString str;
   str.Format(_T("Make copy of this file before proceeding.\r\n%s"), title);
   CWnd* pRB = GetDlgItem(IDC_RB_COPY);
   pRB->SetWindowText(str);

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DEFAULT_OPTIONS);
   pCB->AddString(_T("Always make a copy when opening older-version files"));
   pCB->AddString(_T("Never make backup copy"));
   pCB->SetCurSel(0);
   
   CDialog::OnInitDialog();

   OnClickedDontWarn();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


void CFileSaveWarningDlg::OnClickedDontWarn()
{
   // If the don't warn box is checked, hide the combo box, otherwise show it
   auto btnState = IsDlgButtonChecked(IDC_DONT_WARN);
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DEFAULT_OPTIONS);
   pCB->ShowWindow(btnState == BST_CHECKED ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_RB_COPY)->ShowWindow(btnState == BST_UNCHECKED ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_RB_DONT_COPY)->ShowWindow(btnState == BST_UNCHECKED ? SW_SHOW : SW_HIDE);
}


void CFileSaveWarningDlg::OnBnClickedHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_FILE_COMPATIBILITY_WARNING);
}
