#pragma once

#include <PgsExt\ErectPiersActivity.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\TimelineManager.h>

#include <EAF\EAFDisplayUnits.h>



// CErectPiersDlg dialog

class CErectPiersDlg : public CDialog
{
	DECLARE_DYNAMIC(CErectPiersDlg)

public:
	CErectPiersDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent = NULL);   // standard constructor
	virtual ~CErectPiersDlg();

// Dialog Data
	enum { IDD = IDD_ERECT_PIERS };
   CErectPiersActivity m_ErectPiers;
   const CTimelineManager* m_pTimelineMgr;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void FillSourceList();
   void FillTargetList();

	DECLARE_MESSAGE_MAP()

   CComPtr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveRight();
   afx_msg void OnMoveLeft();
};
