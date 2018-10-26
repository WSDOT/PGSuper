///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <MFCTools\MFCTools.h>

// CStrandGenerationDlg dialog

class CStrandGenerationDlg : public CDialog
{
	DECLARE_DYNAMIC(CStrandGenerationDlg)

public:
	CStrandGenerationDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CStrandGenerationDlg();

// Dialog Data
	enum { IDD = IDD_STRAND_GENERATION };

   enum StrandGenerationType
   {
      sgSequential,
      sgSkipped
   };

   enum LayoutType
   {
      ltSpacing,
      ltStartEndPoint
   };

   int m_StrandType; // 0 = straight, 1 = harped
   Float64 m_Xstart, m_Ystart;
   Float64 m_Xend,   m_Yend; // end point is spacing if m_LayoutType is ltSpacing
   Float64 m_Xstart2, m_Ystart2; // "2" version used for harped strands at harp point
   Float64 m_Xend2,   m_Yend2; // end point is spacing if m_LayoutType is ltSpacing
   Int16 m_nStrandsX, m_nStrandsY;
   StrandGenerationType m_StrandGenerationType;
   LayoutType m_LayoutType;
   bool m_bDelete;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CMetaFileStatic m_Schematic;

   CString GetImageName(LayoutType layoutType,StrandGenerationType generationType);
   void UpdateSchematic();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnLayoutTypeChanged();
   afx_msg void OnStrandGenerationTypeChanged();
   afx_msg void OnStrandTypeChanged();
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
};
