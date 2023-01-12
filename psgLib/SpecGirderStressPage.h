///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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


// CSpecGirderStressPage dialog

class CSpecGirderStressPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSpecGirderStressPage)

public:
	CSpecGirderStressPage();
	virtual ~CSpecGirderStressPage();

// Dialog Data
	enum { IDD = IDD_SPEC_GIRDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void OnHelp();
   afx_msg void OnCheckServiceITensileStress();
   afx_msg void OnCheckReleaseTensionMax();
   afx_msg void OnCheckTSRemovalTensionMax();
   afx_msg void OnCheckAfterDeckTensionMax();
	afx_msg void OnCheckServiceITensionMax();
	afx_msg void OnCheckServiceIIITensionMax();
	afx_msg void OnCheckSevereServiceIIITensionMax();
   afx_msg void OnCheckTemporaryStresses();

   DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();
};
