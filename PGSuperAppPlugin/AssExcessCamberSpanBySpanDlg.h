///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "AssExcessCamberSpanGrid.h"

// CAssExcessCamberSpanBySpanDlg dialog

class CAssExcessCamberSpanBySpanDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssExcessCamberSpanBySpanDlg)

public:
	CAssExcessCamberSpanBySpanDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CAssExcessCamberSpanBySpanDlg();

// Dialog Data
	enum { IDD = IDD_ASSEXCESSCAMBER_SPAN_BY_SPAN };

   void UploadData(const HaunchInputData& rData);
   void DownloadData(HaunchInputData* pData, CDataExchange* pDX);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   CAssExcessCamberSpanGrid m_Grid;
   bool m_bFirstActive;

public:
   virtual BOOL OnInitDialog();

};
