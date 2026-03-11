///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include "OffsetDuctGrid.h"
#include "SplicedGirderGeneralPage.h"

// COffsetDuctDlg dialog

class COffsetDuctDlg : public CDialog, public COffsetDuctGridCallback
{
	DECLARE_DYNAMIC(COffsetDuctDlg)

public:
	COffsetDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,DuctIndexType ductIdx,CWnd* pParent = nullptr);   // standard constructor
	virtual ~COffsetDuctDlg();

   void EnableDeleteBtn(BOOL bEnable);

   COffsetDuctGrid m_Grid;
   COffsetDuctGeometry m_DuctGeometry; 

// Dialog Data
	enum { IDD = IDD_OFFSET_DUCT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   CSplicedGirderGeneralPage* m_pGirderlineDlg;

   DuctIndexType RefDuctIdx;
   DuctIndexType m_DuctIdx;

   DECLARE_MESSAGE_MAP()

   virtual void OnDuctChanged();

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddPoint();
   afx_msg void OnDeletePoint();
};
