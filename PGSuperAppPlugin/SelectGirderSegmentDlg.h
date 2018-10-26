#pragma once


// CSelectGirderSegmentDlg dialog

class CSelectGirderSegmentDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectGirderSegmentDlg)

public:
	CSelectGirderSegmentDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectGirderSegmentDlg();

// Dialog Data
	enum { IDD = IDD_SELECT_GIRDERSEGMENT };

   GroupIndexType m_Group;
   GirderIndexType m_Girder;
   SegmentIndexType m_Segment;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnGroupChanged();
};
