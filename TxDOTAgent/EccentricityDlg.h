#pragma once


// CEccentricityDlg dialog

class CEccentricityDlg : public CDialog
{
	DECLARE_DYNAMIC(CEccentricityDlg)

public:
	CEccentricityDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEccentricityDlg();

// Dialog Data
	enum { IDD = IDD_ECC_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CString m_Message;
};
