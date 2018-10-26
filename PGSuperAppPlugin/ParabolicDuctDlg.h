#pragma once

#include "ParabolicDuctGrid.h"
#include "EditGirderlineDlg.h"

// CParabolicDuctDlg dialog

class CParabolicDuctDlg : public CDialog, public CParabolicDuctGridCallback
{
	DECLARE_DYNAMIC(CParabolicDuctDlg)

public:
	CParabolicDuctDlg(CEditGirderlineDlg* pGdrDlg,CWnd* pParent = NULL);   // standard constructor
	virtual ~CParabolicDuctDlg();

   CParabolicDuctGrid m_Grid;
   CParabolicDuctGeometry m_DuctGeometry; 

// Dialog Data
	enum { IDD = IDD_PARABOLIC_DUCT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   CEditGirderlineDlg* m_pGirderlineDlg;

public:
   virtual BOOL OnInitDialog();

   virtual void OnDuctChanged();
};
