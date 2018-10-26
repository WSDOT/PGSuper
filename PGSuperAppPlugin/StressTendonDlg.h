#pragma once

#include <PgsExt\StressTendonActivity.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TimelineItemListBox.h>

// CStressTendonDlg dialog

class CStressTendonDlg : public CDialog
{
	DECLARE_DYNAMIC(CStressTendonDlg)

public:
	CStressTendonDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent = NULL);   // standard constructor
	virtual ~CStressTendonDlg();

// Dialog Data
	enum { IDD = IDD_STRESS_TENDON };

   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillLists();

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
};
