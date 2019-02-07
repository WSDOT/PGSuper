///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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


// CPretensioningPage dialog

class CPretensioningPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CPretensioningPage)

public:
	CPretensioningPage();
	virtual ~CPretensioningPage();

// Dialog Data
	enum { IDD = IDD_PRETENSION_PARAMETERS };

   bool    bUseLumpSumLosses;
   Float64 BeforeXferLosses;
   Float64 AfterXferLosses;
   Float64 LiftingLosses;
   Float64 ShippingLosses;
   Float64 BeforeTempStrandRemovalLosses;
   Float64 AfterTempStrandRemovalLosses;
   Float64 AfterDeckPlacementLosses;
   Float64 AfterSIDLLosses;
   Float64 FinalLosses;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
   afx_msg void OnHelp();
   virtual BOOL OnInitDialog() override;

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnUseLumpSumLosses();
};
