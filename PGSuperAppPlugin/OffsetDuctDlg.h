#pragma once

#include "OffsetDuctGrid.h"
#include "EditGirderlineDlg.h"

// COffsetDuctDlg dialog

class COffsetDuctDlg : public CDialog, public COffsetDuctGridCallback
{
	DECLARE_DYNAMIC(COffsetDuctDlg)

public:
	COffsetDuctDlg(CEditGirderlineDlg* pGdrDlg,CWnd* pParent = NULL);   // standard constructor
	virtual ~COffsetDuctDlg();

   void EnableDeleteBtn(BOOL bEnable);

   COffsetDuctGrid m_Grid;
   COffsetDuctGeometry m_DuctGeometry; 

// Dialog Data
	enum { IDD = IDD_OFFSET_DUCT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   CEditGirderlineDlg* m_pGirderlineDlg;

   DuctIndexType RefDuctIdx;

   DECLARE_MESSAGE_MAP()

   virtual void OnDuctChanged();

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddPoint();
   afx_msg void OnDeletePoint();
};
