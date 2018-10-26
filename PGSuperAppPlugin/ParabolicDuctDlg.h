#pragma once

#include "ParabolicDuctGrid.h"
#include "PGSuperAppPlugin\SplicedGirderGeneralPage.h"

// CParabolicDuctDlg dialog

class CParabolicDuctDlg : public CDialog, public CParabolicDuctGridCallback
{
	DECLARE_DYNAMIC(CParabolicDuctDlg)

public:
	CParabolicDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CWnd* pParent = NULL);   // standard constructor
	virtual ~CParabolicDuctDlg();

   CParabolicDuctGrid m_Grid;
   CParabolicDuctGeometry m_DuctGeometry; 

// Dialog Data
	enum { IDD = IDD_PARABOLIC_DUCT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   CSplicedGirderGeneralPage* m_pGirderlineDlg;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();

   virtual void OnDuctChanged();
};
