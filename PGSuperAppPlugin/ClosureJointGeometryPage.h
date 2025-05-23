///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

#include <PsgLib\ConnectionLibraryEntry.h>
#include <PgsExt\BoundaryConditionComboBox.h>
#include <PgsExt\TimelineManager.h>

class CSegmentConnectionComboBox : public CComboBox
{
public:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

class CClosureJointBearingOffsetMeasureComboBox : public CComboBox
{
public:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

class CClosureJointEndDistanceMeasureComboBox : public CComboBox
{
public:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

// CClosureJointGeometryPage dialog

class CClosureJointGeometryPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CClosureJointGeometryPage)

public:
	CClosureJointGeometryPage();
	virtual ~CClosureJointGeometryPage();
   virtual BOOL OnInitDialog();

   void Init(const CTemporarySupportData* pTS);
   void Init(const CPierData2* pPierData);

// Dialog Data
	enum { IDD = IDD_CLOSURE_CONNECTION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   afx_msg void OnHelp();

   afx_msg HBRUSH OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

   void OnEndDistanceMeasureChanged();
   afx_msg void OnBearingOffsetMeasureChanged();
   afx_msg void OnConnectionTypeChanged();
   afx_msg void OnInstallationStageChanged();
   afx_msg void OnInstallationStageChanging();

   void UpdateConnectionPicture();
   void FillConnectionTypeComboBox();
   void FillBearingOffsetComboBox();
   void FillEndDistanceComboBox();
   void FillEventList();
   CString GetImageName(pgsTypes::TempSupportSegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType);
   CString GetImageName(pgsTypes::PierSegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType);

   void UpdateConnectionPicture(pgsTypes::TempSupportSegmentConnectionType connectionType);
   void UpdateConnectionPicture(pgsTypes::PierSegmentConnectionType connectionType);
   void UpdateConnectionPicture(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType);
   void UpdateConnectionPicture(ConnectionLibraryEntry::EndDistanceMeasurementType endType);

   CMetaFileStatic m_ConnectionPicture;
   CBrush m_WhiteBrush;
   CSegmentConnectionComboBox m_cbSegmentConnection;
   friend CSegmentConnectionComboBox;
   CClosureJointBearingOffsetMeasureComboBox m_cbBearingOffsetMeasure;
   friend CClosureJointBearingOffsetMeasureComboBox;
   CClosureJointEndDistanceMeasureComboBox m_cbEndDistMeasure;
   friend CClosureJointEndDistanceMeasureComboBox;

   bool m_bIsPier;
   CString m_strSupportLabel;

   CClosureKey m_ClosureKey;
   ClosureIDType m_ClosureID;

   Float64 m_EndDistance;
   Float64 m_BearingOffset;
   Float64 m_DiaphragmWidth;
   Float64 m_DiaphragmHeight;
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType;
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType;

   EventIndexType CreateEvent();

   EventIndexType m_PrevEventIdx;

   CTimelineManager* GetTimelineManager();

public:
   virtual BOOL OnSetActive();
};
