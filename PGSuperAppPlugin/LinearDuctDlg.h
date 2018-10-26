#pragma once

#include "LinearDuctGrid.h"
#include "PGSuperAppPlugin\SplicedGirderGeneralPage.h"

// CLinearDuctDlg dialog

class CLinearDuctDlg : public CDialog, public CLinearDuctGridCallback
{
	DECLARE_DYNAMIC(CLinearDuctDlg)

public:
	CLinearDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CWnd* pParent = NULL);   // standard constructor
	virtual ~CLinearDuctDlg();

   void EnableDeleteBtn(BOOL bEnable);

// Dialog Data
	enum { IDD = IDD_LINEAR_DUCT };

   CLinearDuctGeometry m_DuctGeometry; 

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CLinearDuctGrid m_Grid;
   CSplicedGirderGeneralPage* m_pGirderlineDlg;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddPoint();
   afx_msg void OnDeletePoint();

   virtual void OnDuctChanged();
};
