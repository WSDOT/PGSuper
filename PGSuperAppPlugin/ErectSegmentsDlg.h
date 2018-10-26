#pragma once

#include <PgsExt\SegmentActivity.h>
#include <PgsExt\TimelineManager.h>

// CErectSegmentsDlg dialog

class CErectSegmentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CErectSegmentsDlg)

public:
	CErectSegmentsDlg(const CTimelineManager* pTimelineMgr,EventIndexType eventIdx,CWnd* pParent = NULL);   // standard constructor
	virtual ~CErectSegmentsDlg();

// Dialog Data
	enum { IDD = IDD_ERECT_SEGMENTS };
   CErectSegmentActivity m_ErectSegments;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillSourceList();
   void FillTargetList();

   const CTimelineManager* m_pTimelineMgr;
   const CBridgeDescription2* m_pBridgeDesc;
   EventIndexType m_EventIndex;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveRight();
   afx_msg void OnMoveLeft();
};
