#pragma once

#include <PsgLib\ConnectionLibraryEntry.h>

// CTemporarySupportGeometryPage dialog

class CTemporarySupportGeometryPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CTemporarySupportGeometryPage)

public:
	CTemporarySupportGeometryPage();
	virtual ~CTemporarySupportGeometryPage();
   virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_TS_GEOMETRY };

   pgsTypes::SegmentConnectionType m_ConnectionType;
   Float64 m_GirderEndDistance;
   Float64 m_GirderBearingOffset;
   Float64 m_SupportWidth;
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType;
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg HBRUSH OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

   void OnEndDistanceMeasureChanged();
   void OnBearingOffsetMeasureChanged();
   void OnConnectionTypeChanged();
   void UpdateConnectionPicture();
   void FillConnectionTypeComboBox();
   void FillBearingOffsetComboBox();
   void FillEndDistanceComboBox();
   void FillEventList();
   CString GetImageName(pgsTypes::SegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType);

   CMetaFileStatic m_ConnectionPicture;
   CBrush m_WhiteBrush;
public:
   virtual BOOL OnSetActive();
};
