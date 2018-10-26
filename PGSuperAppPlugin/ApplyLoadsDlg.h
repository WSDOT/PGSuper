#pragma once

#include <PgsExt\ApplyLoadActivity.h>
#include "afxcmn.h"

// CApplyLoadsDlg dialog

class CApplyLoadsDlg : public CDialog
{
	DECLARE_DYNAMIC(CApplyLoadsDlg)

public:
	CApplyLoadsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CApplyLoadsDlg();

// Dialog Data
	enum { IDD = IDD_APPLYLOADS };
   CApplyLoadActivity m_ApplyLoads;
   EventIndexType m_ThisEventIdx;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   CListCtrl m_ctrlUserLoads;

   EventIndexType m_RailingSystemEventIdx;
   EventIndexType m_OverlayEventIdx;
   EventIndexType m_LiveLoadEventIdx;

   void InitUserLoads();
   void AddDistributedLoad(int rowIdx,LoadIDType loadID);
   void AddPointLoad(int rowIdx,LoadIDType loadID);
   void AddMomentLoad(int rowIdx,LoadIDType loadID);

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnRailingSystemClicked();
   afx_msg void OnOverlayClicked();
   afx_msg void OnLiveloadClicked();
};
