#pragma once


// CClosureJointGeneralPage dialog

class CClosureJointGeneralPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CClosureJointGeneralPage)

public:
	CClosureJointGeneralPage();
	virtual ~CClosureJointGeneralPage();

// Dialog Data
	enum { IDD = IDD_CLOSURE_GENERAL };
	CEdit	   m_ctrlEc;
	CEdit	   m_ctrlEci;
	CButton	m_ctrlEcCheck;
	CButton	m_ctrlEciCheck;
	CEdit	   m_ctrlFc;
	CEdit  	m_ctrlFci;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void OnMoreConcreteProperties();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeFc();
	afx_msg void OnChangeEci();
	afx_msg void OnChangeEc();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHelp();
   afx_msg void OnEventChanged();
   afx_msg void OnEventChanging();
   afx_msg void OnConcreteStrength();

   void ExchangeConcreteData(CDataExchange* pDX);


	DECLARE_MESSAGE_MAP()

   void UpdateConcreteControls();
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void FillGirderComboBox();
   void FillEventList();
   EventIndexType CreateEvent();

   int m_PrevEventIdx;

   void UpdateEci();
   void UpdateEc();
   void UpdateFci();
   void UpdateFc();

   CString m_strUserEc;
   CString m_strUserEci;

   pgsTypes::SlabOffsetType m_SlabOffsetType;
   pgsTypes::SlabOffsetType m_SlabOffsetTypeCache;
   Float64 m_SlabOffset;
   CString m_strSlabOffsetCache;

   Float64 m_AgeAtContinuity;

public:
   virtual BOOL OnInitDialog();
};
