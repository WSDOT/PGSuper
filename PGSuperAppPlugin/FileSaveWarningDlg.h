#pragma once


// CFileSaveWarningDlg dialog

// copy options
#define FSW_COPY 0
#define FSW_DONT_COPY 1

class CFileSaveWarningDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileSaveWarningDlg)

public:
	CFileSaveWarningDlg(LPCTSTR lpszAppName,LPCTSTR lpszFileName,LPCTSTR lpszCopyFileName,const CString& strOldVersion,const CString& strCurrentVersion,CWnd* pParent = NULL);   // standard constructor
	virtual ~CFileSaveWarningDlg();

   int m_CopyOption;
   bool m_bDontWarn;
   int m_DefaultCopyOption;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILESAVEWARNINGDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog() override;

   CString m_strLabel;
   CString m_strCopyFileName;

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnClickedDontWarn();
   afx_msg void OnBnClickedHelp();
};
