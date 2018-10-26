#pragma once

#include <PgsExt\PrecastSegmentData.h>
#include "DrawPrecastSegmentControl.h"

// CGirderSegmentGeneralPage dialog

class CGirderSegmentGeneralPage : public CPropertyPage, public IPrecastSegmentDataSource
{
	DECLARE_DYNAMIC(CGirderSegmentGeneralPage)

public:
	CGirderSegmentGeneralPage();
	virtual ~CGirderSegmentGeneralPage();

// Dialog Data
	enum { IDD = IDD_SEGMENT_GENERAL };
	CEdit	   m_ctrlEc;
	CEdit	   m_ctrlEci;
	CButton	m_ctrlEcCheck;
	CButton	m_ctrlEciCheck;
	CEdit	   m_ctrlFc;
	CEdit  	m_ctrlFci;
   CDrawPrecastSegmentControl m_ctrlDrawSegment;

   CCacheEdit m_ctrlSectionLength[4];
   CCacheEdit m_ctrlSectionHeight[4];
   CCacheEdit m_ctrlBottomFlangeDepth[4];

   Float64 GetBottomFlangeDepth(pgsTypes::SegmentZoneType segZone);
   Float64 GetHeight(pgsTypes::SegmentZoneType segZone);
   Float64 GetLength(pgsTypes::SegmentZoneType segZone);
   Float64 GetSegmentLength();
   pgsTypes::SegmentVariationType GetSegmentVariation();

   // IPrecastSegmentDataSource
   virtual const CSplicedGirderData* GetGirder() const;
   virtual const CSegmentKey& GetSegmentKey() const;
   virtual SegmentIDType GetSegmentID() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeGirderFc();
   afx_msg void OnChangeEc();
   afx_msg void OnChangeEci();
	afx_msg void OnMoreConcreteProperties();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnVariationTypeChanged();
   afx_msg void OnConstructionEventChanged();
   afx_msg void OnConstructionEventChanging();
   afx_msg void OnErectionEventChanged();
   afx_msg void OnErectionEventChanging();

   void UpdateConcreteControls();
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void ExchangeConcreteData(CDataExchange* pDX);

   void FillVariationTypeComboBox();
   void GetSectionVariationControlState(BOOL* pbEnable);
   void GetSectionVariationControlState(pgsTypes::SegmentVariationType variationType,BOOL* pbEnable);
   void UpdateSegmentVariationParameters(pgsTypes::SegmentVariationType variationType);

   void UpdateFc();
   void UpdateFci();
   void UpdateEci();
   void UpdateEc();

   CString m_strUserEc;
   CString m_strUserEci;

   Float64 GetValue(UINT nIDC,const unitmgtLengthData& lengthUnit);

   void FillEventList();
   EventIndexType CreateEvent();
	
   int m_PrevConstructionEventIdx;
   int m_PrevErectionEventIdx; // capture the erection stage when the combo box drops down so we can restore the value if CreateEvent fails
   DECLARE_MESSAGE_MAP()

   Float64 m_AgeAtRelease;

   void InitBottomFlangeDepthControls();
   void InitEndBlockControls();

public:
   afx_msg void OnSegmentChanged();
   afx_msg void OnConcreteStrength();
   afx_msg void OnBnClickedBottomFlangeDepth();
};
