#pragma once
#include "Resource.h"

#include <PgsExt\BoundaryConditionComboBox.h>

// CBoundaryConditionDlg dialog

class CBoundaryConditionDlg : public CDialog
{
	DECLARE_DYNAMIC(CBoundaryConditionDlg)

public:
	CBoundaryConditionDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBoundaryConditionDlg();

   pgsTypes::PierConnectionType m_BoundaryCondition;
   PierIndexType m_PierIdx;

// Dialog Data
	enum { IDD = IDD_BOUNDARY_CONDITIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CBoundaryConditionComboBox m_cbBoundaryCondition;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
};
