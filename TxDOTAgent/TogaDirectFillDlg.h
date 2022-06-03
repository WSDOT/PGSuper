///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
// CTogaDirectFillDlg dialog

#include "TxDOTOptionalDesignGirderData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

#pragma once

class CTogaDirectFillDlg : public CDialog
{
	DECLARE_DYNAMIC(CTogaDirectFillDlg)

public:
	CTogaDirectFillDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTogaDirectFillDlg();

// Dialog Data
	enum { IDD = IDD_DIRECT_FILL_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support


	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedSelectStrands();
   virtual BOOL OnInitDialog() override;

   // We aren't a property page, but we need to act like one
   BOOL OnFillSetActive();
   BOOL OnFillKillActive();
   void SetGirderEntryName(LPCTSTR entryName);
   void SetSpanLength(Float64 length);

   void Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* m_pBrokerRetriever, LPCTSTR entryName, GirderIndexType girderIdx);

private:
   void UpdateNoStrandsCtrls();
   bool UpdateLibraryData();

   // keep a copy of the current girder library entry. we may modify this if needed.
   GirderLibraryEntry m_GirderLibraryEntry;

   bool m_bFirstActive;

   CTxDOTOptionalDesignGirderData* m_pGirderData;
   CString m_GirderEntryName;
   ITxDOTBrokerRetriever* m_pBrokerRetriever;
   GirderIndexType        m_GirderIdx;
   Float64 m_SpanLength;

};
