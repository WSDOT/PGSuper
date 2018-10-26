///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "resource.h"

#define PROJECT_IMPORTER_PAGE 0
#define DATA_IMPORTER_PAGE 1
#define DATA_EXPORTER_PAGE 2
#define EXTENSION_AGENT_PAGE 3

// CPluginPage dialog
class CPluginPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CPluginPage)

public:
	CPluginPage();
	virtual ~CPluginPage();

// Dialog Data
	enum { IDD = IDD_MANAGE_PLUGINS };

   void Init(int pageType);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   bool InitList(const CATID& catid);

   int m_PageType;

	CCheckListBox 	m_ctlPluginList;
   std::vector<CString> m_CLSIDs;

   afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   virtual void OnOK();
};
