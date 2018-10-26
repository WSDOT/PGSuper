#pragma once

#include <PgsExt\ErectPiersActivity.h>
#include <PgsExt\TimelineManager.h>
#include <EAF\EAFDisplayUnits.h>

// Pier and Temporary Support indicies are stored in the ItemData member of the
// list box controls. To differentiate between a Pier index and a Temporary Support index
// the Temporary Support indices are encoded/decoded with the following methods
#include <WBFLTypes.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\TemporarySupportData.h>
bool IsTSIndex(SupportIndexType tsIdx);
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx);
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx);
CString GetLabel(const CPierData2* pPier,IEAFDisplayUnits* pDisplayUnits);
CString GetLabel(const CTemporarySupportData* pTS,IEAFDisplayUnits* pDisplayUnits);

// CCastClosurePourDlg dialog

class CCastClosurePourDlg : public CDialog
{
	DECLARE_DYNAMIC(CCastClosurePourDlg)

public:
	CCastClosurePourDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent = NULL);   // standard constructor
	virtual ~CCastClosurePourDlg();

// Dialog Data
	enum { IDD = IDD_CAST_CLOSOURE_POUR };
   CCastClosurePourActivity m_CastClosurePours;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillSourceList();
   void FillTargetList();

	DECLARE_MESSAGE_MAP()

   const CTimelineManager* m_pTimelineMgr;
   const CBridgeDescription2* m_pBridgeDesc;
   CComPtr<IEAFDisplayUnits> m_pDisplayUnits;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveRight();
   afx_msg void OnMoveLeft();
};
