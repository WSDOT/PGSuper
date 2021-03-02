#pragma once

#include <PgsExt\PrecastSegmentData.h>
#include "DrawPrecastSegmentControl.h"
#include <MfcTools\WideDropDownComboBox.h>

// CGirderSegmentGeneralPage dialog

class CGirderSegmentGeneralPage : public CPropertyPage, public IPrecastSegmentDataSource
{
	DECLARE_DYNAMIC(CGirderSegmentGeneralPage)

public:
	CGirderSegmentGeneralPage();
	virtual ~CGirderSegmentGeneralPage();

// Dialog Data
	enum { IDD = IDD_SEGMENT_GENERAL };
   CWideDropDownComboBox m_cbConstruction;
   CWideDropDownComboBox m_cbErection;
   CEdit	   m_ctrlEc;
	CEdit	   m_ctrlEci;
	CButton	m_ctrlEcCheck;
	CButton	m_ctrlEciCheck;
	CEdit	   m_ctrlFc;
	CEdit  	m_ctrlFci;
   CDrawPrecastSegmentControl m_ctrlDrawSegment;

   std::array<CCacheEdit, 4> m_ctrlSectionLength;
   std::array<CCacheEdit, 4> m_ctrlSectionHeight;
   std::array<CCacheEdit, 4> m_ctrlBottomFlangeDepth;

   Float64 GetBottomFlangeDepth(pgsTypes::SegmentZoneType segZone);
   Float64 GetHeight(pgsTypes::SegmentZoneType segZone);
   Float64 GetLength(pgsTypes::SegmentZoneType segZone);
   Float64 GetSegmentLength();
   pgsTypes::SegmentVariationType GetSegmentVariation();

   // IPrecastSegmentDataSource
   virtual const CSplicedGirderData* GetGirder() const;
   virtual const CSegmentKey& GetSegmentKey() const;
   virtual SegmentIDType GetSegmentID() const;

   bool m_bWasEventCreated;

   Float64 m_MinSlabOffset;
   pgsTypes::SlabOffsetType m_SlabOffsetType;
   pgsTypes::SlabOffsetType m_PrevSlabOffsetType;
   std::array<Float64, 2> m_SlabOffset;
   std::array<CString, 2> m_strSlabOffsetCache;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeFc();
   afx_msg void OnChangeEc();
   afx_msg void OnChangeEci();
	afx_msg void OnMoreConcreteProperties();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnVariationTypeChanged();
   afx_msg void OnConstructionEventChanged();
   afx_msg void OnConstructionEventChanging();
   afx_msg void OnErectionEventChanged();
   afx_msg void OnErectionEventChanging();
   afx_msg void OnChangingSlabOffsetType();
   afx_msg void OnChangeSlabOffsetType();

   void UpdateConcreteControls(bool bSkipEcCheckBoxes = false);
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void ExchangeConcreteData(CDataExchange* pDX);

   void FillVariationTypeComboBox();
   void GetSectionVariationControlState(BOOL* pbEnable);
   void GetSectionVariationControlState(pgsTypes::SegmentVariationType variationType,BOOL* pbEnable);
   void UpdateSegmentVariationParameters(pgsTypes::SegmentVariationType variationType);

   void FillSlabOffsetComboBox();

   void UpdateFc();
   void UpdateFci();
   void UpdateEci();
   void UpdateEc();

   pgsTypes::SlabOffsetType GetCurrentSlabOffsetType();
   void UpdateSlabOffsetControls();

   CString m_strUserEc;
   CString m_strUserEci;

   Float64 GetValue(UINT nIDC,const unitmgtLengthData& lengthUnit);

   void FillEventList();
   EventIDType CreateEvent();
	
   int m_PrevConstructionEventIdx;
   int m_PrevErectionEventIdx; // capture the erection stage when the combo box drops down so we can restore the value if CreateEvent fails
   DECLARE_MESSAGE_MAP()

   int m_LossMethod;
   int m_TimeDependentModel;
   Float64 m_AgeAtRelease;

   void InitBottomFlangeDepthControls();
   void InitEndBlockControls();

public:
   afx_msg void OnSegmentChanged();
   afx_msg void OnConcreteStrength();
   afx_msg void OnBnClickedBottomFlangeDepth();
   afx_msg void OnHelp();
};
