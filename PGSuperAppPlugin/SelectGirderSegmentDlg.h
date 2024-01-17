///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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


// CSelectGirderSegmentDlg dialog

class CSelectGirderSegmentDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectGirderSegmentDlg)

public:
	CSelectGirderSegmentDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectGirderSegmentDlg();

// Dialog Data
	enum { IDD = IDD_SELECT_GIRDERSEGMENT };

   GroupIndexType m_Group;
   GirderIndexType m_Girder;
   SegmentIndexType m_Segment;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog() override;
   afx_msg void OnGroupChanged();
};
