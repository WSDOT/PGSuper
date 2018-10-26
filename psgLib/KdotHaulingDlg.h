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
   virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   afx_msg void OnBnClickedCheckHaulingTensMax();
};
