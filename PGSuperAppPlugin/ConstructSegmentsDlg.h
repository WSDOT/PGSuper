#pragma once

#include <PgsExt\SegmentActivity.h>
#include <PgsExt\TimelineManager.h>

// CConstructSegmentsDlg dialog

class CConstructSegmentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CConstructSegmentsDlg)

public:
	CConstructSegmentsDlg(const CTimelineManager* pTimelineMgr,EventIndexType eventIdx,CWnd* pParent = NULL);   // standard constructor
	virtual ~CConstructSegmentsDlg();

   // Dialog Data
	enum { IDD = IDD_CONSTRUCT_SEGMENT };
   CConstructSegmentActivity m_ConstructSegments;

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
