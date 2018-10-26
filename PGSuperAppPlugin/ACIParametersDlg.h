#pragma once


// CACIParametersDlg dialog

class CACIParametersDlg : public CDialog
{
	DECLARE_DYNAMIC(CACIParametersDlg)

public:
	CACIParametersDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CACIParametersDlg();

   Float64 m_t1;
   Float64 m_t2;
   Float64 m_fc1;
   Float64 m_fc2;
   Float64 m_A;
   Float64 m_B;

// Dialog Data
	enum { IDD = IDD_ACI_PARAMETERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void UpdateParameters();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
};
