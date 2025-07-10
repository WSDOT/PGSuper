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

// ConcreteDetailsDlg.cpp : implementation file
//

///////////////////////////////////////////////////////////////////////////
// NOTE: Duplicate code warning
//
// This dialog along with all its property pages are basically repeated in
// the PGSuperLibrary project. I could not get a single implementation to
// work because of issues with the module resources.
//
// If changes are made here, the same changes are likely needed in
// the other location.

#include "stdafx.h"
#include "BearingDetailsDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>

#include <psgLib\BearingCriteria.h>
#include <AgentTools.h>


/////////////////////////////////////////////////////////////////////////////
// CBearingDetailsDlg dialog


CBearingDetailsDlg::CBearingDetailsDlg(CWnd* pParent)
	:CDialog(IDD_BEARING_DETAIL, pParent)
{

   Init();

}


void CBearingDetailsDlg::SetBearingDetailDlg(const CBearingData2& brg)
{
	m_BearingData = brg;
}

CBearingData2 CBearingDetailsDlg::GetBearingDetails() const
{
	return m_BearingData;
}

Float64 CBearingDetailsDlg::GetComputedHeight() const
{
	return m_ComputedHeight;
}

BEGIN_MESSAGE_MAP(CBearingDetailsDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_BEARING_LENGTH, &CBearingDetailsDlg::OnEnChangeBearingInput)
	ON_EN_CHANGE(IDC_EDIT_BEARING_WIDTH, &CBearingDetailsDlg::OnEnChangeBearingInput)
	ON_EN_CHANGE(IDC_EDIT_INT_ELASTOMER_THICK, &CBearingDetailsDlg::OnEnChangeBearingInput)
	ON_EN_CHANGE(IDC_NUMBER_INT_LAYERS, &CBearingDetailsDlg::OnEnChangeBearingInput)
	ON_EN_CHANGE(IDC_EDIT_COVER_ELASTOMER_THICK, &CBearingDetailsDlg::OnEnChangeBearingInput)
	ON_EN_CHANGE(IDC_STEEL_SHIM_THICKNESS, &CBearingDetailsDlg::OnEnChangeBearingInput)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBearingDetailsDlg message handlers

BOOL CBearingDetailsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	auto pBroker = EAFGetBroker();
	GET_IFACE2(pBroker, ISpecification, pSpec);
	GET_IFACE2(pBroker, ILibrary, pLib);
	pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
	const auto pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
	const WBFL::EngTools::BearingProjectCriteria& criteria = pSpecEntry->GetBearingCriteria();

	if (criteria.AnalysisMethod == WBFL::EngTools::BearingAnalysisMethod::MethodA)
	{
		MethodAControls(SW_SHOW);
		MethodBControls(SW_HIDE);
	}
	else
	{
		MethodAControls(SW_HIDE);
		MethodBControls(SW_SHOW);
	}

	UpdateOptimizationResults();

    return TRUE;
}

void CBearingDetailsDlg::UpdateOptimizationResults()
{

	WBFL::EngTools::Bearing brg;
	WBFL::EngTools::BearingCalculator calc;


	brg.SetLength(m_BearingData.Length);
	brg.SetWidth(m_BearingData.Width);
	brg.SetIntermediateLayerThickness(m_BearingData.ElastomerThickness);
	brg.SetCoverThickness(m_BearingData.CoverThickness);
	brg.SetSteelShimThickness(m_BearingData.ShimThickness);
	brg.SetNumIntLayers(m_BearingData.NumIntLayers);

	auto pBroker = EAFGetBroker();
	GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

	m_ComputedHeight = calc.ComputeBearingHeight(brg);

	CString height;
	height.Format(_T("Computed Height: %s"), FormatDimension(m_ComputedHeight, pDisplayUnits->GetComponentDimUnit()));
	
	GetDlgItem(IDC_STATIC_BRG_HEIGHT)->SetWindowText(height);

	Float64 value;
	CString unit;

	if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
	{
		unit = _T("lbs");
		value = brg.GetBearingWeight() * 0.225;
	}
	else
	{
		unit = _T("N");
		value = brg.GetBearingWeight();
	}

	CString weight;
	weight.Format(_T("Estimated Weight: %.1f %s"), value, unit);

	GetDlgItem(IDC_BRG_WEIGHT)->SetWindowText(weight);

}


void CBearingDetailsDlg::MethodAControls(int s)
{
	//GetDlgItem()->ShowWindow(s);
}

void CBearingDetailsDlg::MethodBControls(int s)
{
	GetDlgItem(IDC_CHECK_FIXED_X_TRANS)->ShowWindow(s);
	GetDlgItem(IDC_CHECK_FIXED_Y_TRANS)->ShowWindow(s);
	GetDlgItem(IDC_CHECK_EXT_BONDED_PLATES)->ShowWindow(s);
}


void CBearingDetailsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	auto pBroker = EAFGetBroker();
	GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

	DDX_UnitValueAndTag(pDX, IDC_EDIT_BEARING_LENGTH, 
		IDC_EDIT_BEARING_LENGTH_UNIT, m_BearingData.Length, pDisplayUnits->GetComponentDimUnit());
	DDX_UnitValueAndTag(pDX, IDC_EDIT_BEARING_WIDTH,
		IDC_EDIT_BEARING_WIDTH_UNIT, m_BearingData.Width, pDisplayUnits->GetComponentDimUnit());

	DDX_UnitValueAndTag(pDX, IDC_EDIT_INT_ELASTOMER_THICK,
		IDC_EDIT_INT_ELASTOMER_THICK_UNIT, m_BearingData.ElastomerThickness, pDisplayUnits->GetComponentDimUnit());
	DDX_UnitValueAndTag(pDX, IDC_EDIT_COVER_ELASTOMER_THICK,
		IDC_EDIT_COVER_ELASTOMER_THICK_UNIT, m_BearingData.CoverThickness, pDisplayUnits->GetComponentDimUnit());
	DDX_Text(pDX, IDC_NUMBER_INT_LAYERS, m_BearingData.NumIntLayers);
	DDX_UnitValueAndTag(pDX, IDC_STEEL_SHIM_THICKNESS,
		IDC_STEEL_SHIM_THICKNESS_UNIT, m_BearingData.ShimThickness, pDisplayUnits->GetComponentDimUnit());

	CEdit* editWindow = (CEdit*)GetDlgItem(IDC_EDIT_SHEAR_DEFORMATION);
	CString strValue;
	editWindow->GetWindowText(strValue);
	strValue.Trim();
	
	if (m_BearingData.ShearDeformationOverride == -1 || (pDX->m_bSaveAndValidate && strValue.IsEmpty()))  //Compute when blank or -1
	{
		DDX_KeywordUnitValueAndTag(pDX, IDC_EDIT_SHEAR_DEFORMATION,
			IDC_EDIT_SHEAR_DEFORMATION_UNIT, _T("Compute"), m_BearingData.ShearDeformationOverride,
			pDisplayUnits->GetComponentDimUnit());
	}
	else // accept positive or negative input
	{
		DDX_UnitValueAndTag(pDX, IDC_EDIT_SHEAR_DEFORMATION,
			IDC_EDIT_SHEAR_DEFORMATION_UNIT, m_BearingData.ShearDeformationOverride,
			pDisplayUnits->GetComponentDimUnit());
	}
	
	DDX_Check_Bool(pDX, IDC_CHECK_FIXED_X_TRANS, m_BearingData.FixedX);
	DDX_Check_Bool(pDX, IDC_CHECK_FIXED_Y_TRANS, m_BearingData.FixedY);
	DDX_Check_Bool(pDX, IDC_CHECK_EXT_BONDED_PLATES, m_BearingData.UseExtPlates);

}

void CBearingDetailsDlg::Init()
{

}

void CBearingDetailsDlg::OnEnChangeBearingInput()
{
	const UINT editIDs[] = {
		IDC_EDIT_BEARING_LENGTH,
		IDC_EDIT_BEARING_WIDTH,
		IDC_EDIT_INT_ELASTOMER_THICK,
		IDC_NUMBER_INT_LAYERS,
		IDC_EDIT_COVER_ELASTOMER_THICK,
		IDC_STEEL_SHIM_THICKNESS
	};

	for (UINT id : editIDs)
	{
		CString text;
		GetDlgItemText(id, text);
		if (text.Trim().IsEmpty())
		{
			// If any field is empty, skip processing
			return;
		}
	}

	UpdateData();

	UpdateOptimizationResults();
}


