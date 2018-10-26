#pragma once

#include <PgsExt\SegmentActivity.h>
#include <PgsExt\TimelineManager.h>

// CConstructSegmentsDlg dialog

class CConstructSegmentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CConstructSegmentsDlg)

public:
	CConstructSegmentsDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent = NULL);   // standard constructor
	virtual ~CConstructSegmentsDlg();

   // Dialog Data
	enum { IDD = IDD_CONSTRUCT_SEGMENT };
   CConstructSegmentActivity m_ConstructSegments;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   const CTimelineManager* m_pTimelineMgr;
   const CBridgeDescription2* m_pBridgeDesc;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
};
