#pragma once

#include <PgsExt\SegmentActivity.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TimelineItemListBox.h>

// CErectSegmentsDlg dialog

class CErectSegmentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CErectSegmentsDlg)

public:
	CErectSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = NULL);   // standard constructor
	virtual ~CErectSegmentsDlg();

// Dialog Data
	enum { IDD = IDD_ERECT_SEGMENTS };

   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillLists();

   const CBridgeDescription2* m_pBridgeDesc;

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

   BOOL m_bReadOnly;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
