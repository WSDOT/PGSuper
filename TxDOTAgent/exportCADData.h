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

#if !defined(AFX_EXPORTCADDATA_H__6F026124_5AB7_42E4_B64F_3BB2C5BD08B8__INCLUDED_)
#define AFX_EXPORTCADDATA_H__6F026124_5AB7_42E4_B64F_3BB2C5BD08B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// exportCADData.h : header file
//
#include "resource.h"

#include "SharedCTrls\MultiGirderSelectGrid.h" 

/////////////////////////////////////////////////////////////////////////////
// exportCADData dialog

class exportCADData : public CDialog
{
// Construction
public:
    exportCADData(IBroker* pBroker,CWnd* pParent = nullptr);
	~exportCADData(); 

// Dialog Data
	//{{AFX_DATA(exportCADData)
	enum { IDD = IDD_EXPORT_TXDOTCADDATA };

	enum cdtExportDataType { cdtGirderDesignData, cdtHaunchAndDeflectionData, cdtLegacyTextGirderData };
	cdtExportDataType m_ExportDataType;

   enum ctxFileFormatType { ctxExcel, ctxCSV, ctxLegacy };
   ctxFileFormatType	m_FileFormatType;
	//}}AFX_DATA

   std::vector<CGirderKey> m_GirderKeys;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(exportCADData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	CComPtr<IBroker> m_pBroker;

	// Generated message map functions
	//{{AFX_MSG(exportCADData)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnHelp();
	afx_msg void OnSelchangeSpan();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   CMultiGirderSelectGrid* m_pGrid;
public:
   afx_msg void OnBnClickedSelectAll();
   afx_msg void OnBnClickedClearAll();
	afx_msg void OnCbnSelchangeExportType();
};


   //{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTCADDATA_H__6F026124_5AB7_42E4_B64F_3BB2C5BD08B8__INCLUDED_)
