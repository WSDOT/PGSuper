#pragma once

#include <PgsExt\SegmentActivity.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TimelineItemListBox.h>

// CConstructSegmentsDlg dialog

class CConstructSegmentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CConstructSegmentsDlg)

public:
	CConstructSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CConstructSegmentsDlg();

   // Dialog Data
	enum { IDD = IDD_CONSTRUCT_SEGMENT };

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
