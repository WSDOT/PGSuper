#pragma once

#include "resource.h"

// CKdotHaulingDlg dialog

class CKdotHaulingDlg : public CDialog
{
	DECLARE_DYNAMIC(CKdotHaulingDlg)

public:
	CKdotHaulingDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CKdotHaulingDlg();

// Dialog Data
	enum { IDD = IDD_KDOT_HAULINGD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   static BOOL CALLBACK EnableWindows(HWND hwnd,LPARAM lParam);

public:
   void HideControls(bool hide);
	void DoCheckMax();
	void DoCheckMinLocation();

   bool m_IsHaulingEnabled;
   afx_msg void OnBnClickedCheckHaulingTensMax();
   afx_msg void OnBnClickedIsSupportLessThan();
protected:
   virtual void OnOK();
   virtual void OnCancel();
};
