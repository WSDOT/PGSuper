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


// CTimeStepPropertiesPage dialog

class CTimeStepPropertiesPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CTimeStepPropertiesPage)

public:
	CTimeStepPropertiesPage();
	virtual ~CTimeStepPropertiesPage();

// Dialog Data
	enum { IDD = IDD_TIMESTEP_PARAMETERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
   afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()
public:
   bool m_bIgnoreCreepEffects;
   bool m_bIgnoreShrinkageEffects;
   bool m_bIgnoreRelaxationEffects;
};
