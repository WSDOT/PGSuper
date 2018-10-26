#pragma once

#include <PgsExt\StressTendonActivity.h>
#include <PgsExt\TimelineManager.h>

// CStressTendonDlg dialog

class CStressTendonDlg : public CDialog
{
	DECLARE_DYNAMIC(CStressTendonDlg)

public:
	CStressTendonDlg(const CTimelineManager* pTimelineMgr,EventIndexType eventIdx,CWnd* pParent = NULL);   // standard constructor
	virtual ~CStressTendonDlg();

// Dialog Data
	enum { IDD = IDD_STRESS_TENDON };
   CStressTendonActivity m_StressTendonActivity;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillSourceList();
   void FillTargetList();

   std::vector<std::pair<CGirderKey,DuctIndexType>> m_SourceTendons; // all unstressed tendons
   std::vector<std::pair<CGirderKey,DuctIndexType>> m_TargetTendons; // all tendons stress during this activity

   const CTimelineManager* m_pTimelineMgr;
   EventIndexType m_EventIndex;

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveRight();
   afx_msg void OnMoveLeft();
};
