#pragma once

#include <PgsExt\SegmentActivity.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TimelineItemListBox.h>

// CConstructSegmentsDlg dialog

class CConstructSegmentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CConstructSegmentsDlg)

public:
	CConstructSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent = NULL);   // standard constructor
	virtual ~CConstructSegmentsDlg();

   // Dialog Data
	enum { IDD = IDD_CONSTRUCT_SEGMENT };

   CTimelineManager m_TimelineMgr;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillLists();

   const CBridgeDescription2* m_pBridgeDesc;
   EventIndexType m_EventIndex;

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
};
