#pragma once

#include <EAF\EAFAboutDlg.h>
#include <afxlinkctrl.h>

class CAboutDlg : public CDialog
{
   DECLARE_DYNAMIC(CAboutDlg)

public:
   CAboutDlg(UINT nResourceID,UINT nIDTemplate=0,CWnd* pParent=NULL);
   virtual ~CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
   afx_msg void OnAppListSelChanged();
   afx_msg void OnMoreInfo();
   afx_msg HBRUSH OnCtlColor(CDC*, CWnd*, UINT);

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   UINT m_ResourceID;
   CMFCLinkCtrl m_WSDOT;
   CMFCLinkCtrl m_TxDOT;
   CMFCLinkCtrl m_KDOT;
   CMFCLinkCtrl m_BridgeSight;

   CListBox m_AppList;
   CStatic m_Description;
};
