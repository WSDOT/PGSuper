#pragma once

#include <PgsExt\StressTendonActivity.h>

// CStressTendonDlg dialog

class CStressTendonDlg : public CDialog
{
	DECLARE_DYNAMIC(CStressTendonDlg)

public:
	CStressTendonDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CStressTendonDlg();

// Dialog Data
	enum { IDD = IDD_STRESS_TENDON };
   CStressTendonActivity m_StressTendons;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillSourceList();
   void FillTargetList();

   std::vector<std::pair<CGirderKey,DuctIndexType>> m_SourceTendons;
   std::vector<std::pair<CGirderKey,DuctIndexType>> m_TargetTendons;

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveRight();
   afx_msg void OnMoveLeft();
};
