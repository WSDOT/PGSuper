///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PsgLib\BearingData2.h>
#include <IFace\BearingDesignParameters.h>

// BearingDetailsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBearingDetailsDlg dialog

class CBearingDetailsDlg : public CDialog
{

// Construction
public:
	CBearingDetailsDlg(CWnd* pParent = nullptr);

	void MethodAControls(int s);
	void MethodBControls(int s);

	void SetBearingDetailDlg(const CBearingData2& data);

	CBearingData2 GetBearingDetails() const;

	Float64 GetComputedHeight() const;

   // Implementation
protected:
	void DoDataExchange(CDataExchange* pDX);
	void Init();


	virtual BOOL OnInitDialog() override;

	void UpdateOptimizationResults();

	DECLARE_MESSAGE_MAP()


private:

	CBearingData2 m_BearingData;
	Float64 m_ComputedHeight{0.0};

public:
	afx_msg void OnEnChangeBearingInput();
};


