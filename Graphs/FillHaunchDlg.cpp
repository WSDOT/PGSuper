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
// FillHaunchDlg.cpp 
//

#include "stdafx.h"
#include "resource.h"
#include "afxdialogex.h"
#include "FillHaunchDlg.h"

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <MFCTools\CustomDDX.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFDocument.h>
#include <EAF\EAFHelp.h>

#include <PgsExt/BridgeDescription2.h>
#include <PgsExt\HaunchDepthInputConversionTool.h>
#include "..\Documentation\PGSuper.hh"

// CFillHaunchDlg dialog

IMPLEMENT_DYNAMIC(CFillHaunchDlg, CDialog)

CFillHaunchDlg::CFillHaunchDlg(const CGirderKey& key, IBroker* pBroker,CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_FILL_HAUNCH, pParent),
	m_GirderKey(key),
	m_pBroker(pBroker),
	m_Method(mtCompute),
	m_HaunchInputDistributionType((int)pgsTypes::hidTenthPoints),
	m_AddedVal(0.0)
{
	//{{AFX_DATA_INIT(CFillHaunchDlg)
	//}}AFX_DATA_INIT
}

CFillHaunchDlg::~CFillHaunchDlg()
{
}

BOOL CFillHaunchDlg::OnInitDialog()
{
	GET_IFACE(IBridgeDescription,pIBridgeDesc);
	bool bBySpan = pgsTypes::hltAlongSpans == pIBridgeDesc->GetHaunchLayoutType();

	CString str;
	if (bBySpan)
	{
		if (m_GirderKey.groupIndex == ALL_SPANS)
		{
			str = _T("All Spans, ");
		}
		else
		{
			str.Format(_T("Span %s, "),LABEL_SPAN(m_GirderKey.groupIndex));
		}
	}
	else
	{
		if (m_GirderKey.groupIndex == ALL_GROUPS)
		{
			str = _T("All Groups, ");
		}
		else
		{
			str.Format(_T("Group %d, "),LABEL_GROUP(m_GirderKey.groupIndex));
		}
	}

	CString strgdr;
   strgdr.Format(_T("%sGirder %s"),str,LABEL_GIRDER(m_GirderKey.girderIndex));
	GetDlgItem(IDC_RADIO_SELECTED_GIRDER)->SetWindowText(strgdr);

	strgdr.Format(_T("%sAll Girders"),str);
	GetDlgItem(IDC_RADIO_SELECT_ALL_GIRDERS)->SetWindowText(strgdr);

	// select all girders if global bridge setting is so
	m_ToGirderSel = pgsTypes::hilPerEach == pIBridgeDesc->GetHaunchInputLocationType() ? 0 : 1;

	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_COMPUTE_HAUNCH_COMBO);
	if (bBySpan)
	{
		pBox->SetItemData(pBox->AddString(_T("1/4 points along Spans")),  (DWORD_PTR)pgsTypes::hidQuarterPoints);
		pBox->SetItemData(pBox->AddString(_T("1/10 points along Spans")), (DWORD_PTR)pgsTypes::hidTenthPoints);
	}
	else
	{
		pBox->SetItemData(pBox->AddString(_T("1/4 points along Segments")),  (DWORD_PTR)pgsTypes::hidQuarterPoints);
		pBox->SetItemData(pBox->AddString(_T("1/10 points along Segments")), (DWORD_PTR)pgsTypes::hidTenthPoints);
	}

	// determine girders where we can compute required haunch depths
	GirderIndexType gdrCnt;
	if (m_GirderKey.groupIndex == ALL_SPANS)
	{
		gdrCnt = pIBridgeDesc->GetBridgeDescription()->GetMaxGirderCount();
	}
	else
	{
		GET_IFACE(IBridge,pBridge);

		if (bBySpan)
		{
			gdrCnt = pBridge->GetGirderCountBySpan(m_GirderKey.groupIndex);
		}
		else
		{
			gdrCnt = pBridge->GetGirderCount(m_GirderKey.groupIndex);
		}
	}

	pBox = (CComboBox*)GetDlgItem(IDC_COMPUTE_HAUNCH_GIRDER);
	for (GirderIndexType gdrIdx = 0; gdrIdx < gdrCnt; gdrIdx++)
	{
		pBox->AddString(GIRDER_LABEL(gdrIdx));
	}

	m_ToBeComputedGirderIdx = (int)( m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : m_GirderKey.girderIndex );

	CDialog::OnInitDialog();
	return 0;
}

void CFillHaunchDlg::DoDataExchange(CDataExchange* pDX)
{
	GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

	//{{AFX_DATA_MAP(CFillHaunchDlg)
	//}}AFX_DATA_MAP

	CDialog::DoDataExchange(pDX);

	DDX_Radio(pDX,IDC_COMPUTE_HAUNCH_RADIO,m_Method);
	DDX_CBIndex(pDX,IDC_COMPUTE_HAUNCH_GIRDER,m_ToBeComputedGirderIdx);
	DDX_Radio(pDX,IDC_RADIO_SELECTED_GIRDER,m_ToGirderSel);
	DDX_CBItemData(pDX,IDC_COMPUTE_HAUNCH_COMBO,m_HaunchInputDistributionType);
	DDX_UnitValueAndTag(pDX,IDC_ADD_HAUNCH_EDIT,IDC_ADD_HAUNCH_UNIT,m_AddedVal,pDisplayUnits->GetComponentDimUnit());

	OnMethod();
}

BEGIN_MESSAGE_MAP(CFillHaunchDlg, CDialog)
	ON_BN_CLICKED(IDC_COMPUTE_HAUNCH_RADIO,OnMethod)
	ON_BN_CLICKED(IDC_ADD_HAUNCH_RADIO,OnMethod)
	ON_COMMAND(IDHELP,OnHelp)
END_MESSAGE_MAP()

// CFillHaunchDlg message handlers
void CFillHaunchDlg::OnMethod()
{
	BOOL bCompute = IsDlgButtonChecked(IDC_COMPUTE_HAUNCH_RADIO);
	GetDlgItem(IDC_COMPUTE_HAUNCH_COMBO)->EnableWindow(bCompute);
	GetDlgItem(IDC_COMPUTE_HAUNCH_GIRDER)->EnableWindow(bCompute);
	GetDlgItem(IDC_COMPUTE_HAUNCH_STATIC)->EnableWindow(bCompute);
	GetDlgItem(IDC_COMPUTE_HAUNCH_STATIC2)->EnableWindow(bCompute);
	GetDlgItem(IDC_COMPUTE_HAUNCH_STATIC3)->EnableWindow(bCompute);

	BOOL bAdd = IsDlgButtonChecked(IDC_ADD_HAUNCH_RADIO);
	GetDlgItem(IDC_ADD_HAUNCH_EDIT)->EnableWindow(bAdd);
	GetDlgItem(IDC_ADD_HAUNCH_UNIT)->EnableWindow(bAdd);
}

bool CFillHaunchDlg::ModifyBridgeDescription(CBridgeDescription2& rBridgeDescription2)
{
	if (pgsTypes::hidACamber == rBridgeDescription2.GetHaunchInputDepthType())
	{
		ATLASSERT(0); // we don't do "A" dim and assumed camber here
		return false;
	}

	switch ((MethodType)m_Method)
	{
	case mtCompute:
		return ModifyCompute(rBridgeDescription2);

	case mtAdd:
		return ModifyAdd(rBridgeDescription2);

	default:
		ATLASSERT(0);
	}

	return false;
}

bool CFillHaunchDlg::ModifyCompute(CBridgeDescription2& rBridgeDescription2)
{
	// Use the haunch conversion tool to perform the design and convert data to our need
	HaunchDepthInputConversionTool haunchTool(&rBridgeDescription2,m_pBroker,false);

	// Segments to be designed
	CSegmentKey designSegmentKey(m_GirderKey.groupIndex,m_GirderKey.girderIndex, ALL_SEGMENTS);

	GET_IFACE(IProgress,pProgress);
	CEAFAutoProgress ap(pProgress);
	pProgress->UpdateMessage(_T("Computing Haunch Design"));

	// Copy designed bridge descr
	rBridgeDescription2 = haunchTool.DesignHaunches(designSegmentKey,m_ToBeComputedGirderIdx, (pgsTypes::HaunchInputDistributionType)m_HaunchInputDistributionType, m_ToGirderSel == 1).second;

	return true;
}

// Utility struct for add operation
struct AddVal
{
	Float64 m_Val;
	Float64 m_Fillet;
	bool& m_rWasError;

	AddVal(Float64 v, Float64 fillet, bool& rWasErr) : m_Val(v),m_Fillet(fillet), m_rWasError(rWasErr) {};

	void operator()(Float64& elem) const
	{
		elem += m_Val;
		if (elem < m_Fillet) // we can't allow values less than fillet. Ceil this and set error
		{
			elem = m_Fillet;
			m_rWasError = true;
		}
	}
};

bool CFillHaunchDlg::ModifyAdd(CBridgeDescription2& rBridgeDescription)
{
	pgsTypes::HaunchInputLocationType haunchInputLocationType = rBridgeDescription.GetHaunchInputLocationType();
	pgsTypes::HaunchLayoutType haunchLayoutType = rBridgeDescription.GetHaunchLayoutType();
	pgsTypes::HaunchInputDistributionType haunchInputDistributionType = rBridgeDescription.GetHaunchInputDistributionType();

	Float64 fillet = rBridgeDescription.GetFillet();
	bool bWasFilletComprimized = false; // we won't cause any haunch depths to be less than fillet. Set flag if it happens

	if (haunchInputLocationType == pgsTypes::hilSame4Bridge)
	{
		// m_GirderKey doesn't matter
		std::vector<Float64> haunches = rBridgeDescription.GetDirectHaunchDepths();
		std::for_each(haunches.begin(),haunches.end(),AddVal(m_AddedVal,fillet,bWasFilletComprimized)); // Add value - capping to fillet
		rBridgeDescription.SetDirectHaunchDepths(haunches);
	}
	else if (haunchLayoutType == pgsTypes::hltAlongSpans)
	{
		// Tricky: we are setting groupIndex in m_GirderKey as user-selected span count
		SpanIndexType startSpan = m_GirderKey.groupIndex == ALL_SPANS ? 0 : m_GirderKey.groupIndex;
		SpanIndexType endSpan   = m_GirderKey.groupIndex==ALL_SPANS ? rBridgeDescription.GetSpanCount()-1 : m_GirderKey.groupIndex;
		for (SpanIndexType iSpan = startSpan; iSpan <= endSpan; iSpan++)
		{
			CSpanData2* pSpan = rBridgeDescription.GetSpan(iSpan);

			GirderIndexType startGirder,endGirder;
			if (pgsTypes::hilSame4AllGirders == haunchInputLocationType)
			{
				startGirder = 0;
				endGirder = 0;
			}
			else if (ALL_GIRDERS == m_GirderKey.girderIndex || m_ToGirderSel == 1)
			{
				startGirder = 0;
				endGirder = pSpan->GetGirderCount() - 1;
			}
			else
			{
				startGirder = m_GirderKey.girderIndex;
				endGirder =  m_GirderKey.girderIndex;
			}

			for (GirderIndexType iGirder = startGirder; iGirder <= endGirder; iGirder++)
			{
				std::vector<Float64> haunches = pSpan->GetDirectHaunchDepths(iGirder,true);
				std::for_each(haunches.begin(),haunches.end(),AddVal(m_AddedVal,fillet,bWasFilletComprimized)); // Add value - capping to fillet
				pSpan->SetDirectHaunchDepths(iGirder,haunches);
			}
		}
	}
	else if (haunchLayoutType == pgsTypes::hltAlongSegments)
	{
		GroupIndexType startGroup = m_GirderKey.groupIndex == ALL_GROUPS ? 0 : m_GirderKey.groupIndex;
		GroupIndexType endGroup = m_GirderKey.groupIndex == ALL_GROUPS ? rBridgeDescription.GetGirderGroupCount()-1 : m_GirderKey.groupIndex;
		for (GroupIndexType iGroup = startGroup; iGroup <= endGroup; iGroup++)
		{
			CGirderGroupData* pGroup = rBridgeDescription.GetGirderGroup(iGroup);

			GirderIndexType startGirder,endGirder;
			if (pgsTypes::hilSame4AllGirders == haunchInputLocationType)
			{
				startGirder = 0;
				endGirder = 0;
			}
			else if (ALL_GIRDERS == m_GirderKey.girderIndex || m_ToGirderSel == 1)
			{
				startGirder = 0;
				endGirder = pGroup->GetGirderCount() - 1;
			}
			else
			{
				startGirder = m_GirderKey.girderIndex;
				endGirder = m_GirderKey.girderIndex;
			}

			for (GirderIndexType iGirder = startGirder; iGirder <= endGirder; iGirder++)
			{
				CSplicedGirderData* pGirder = pGroup->GetGirder(iGirder);
				SegmentIndexType numSegs = pGirder->GetSegmentCount();
				for (SegmentIndexType segIdx = 0; segIdx < numSegs; segIdx++)
				{
					std::vector<Float64> haunches = pGirder->GetDirectHaunchDepths(segIdx);
					std::for_each(haunches.begin(),haunches.end(),AddVal(m_AddedVal,fillet,bWasFilletComprimized)); // Add value - capping to fillet
					pGirder->SetDirectHaunchDepths(segIdx,haunches);
				}
			}
		}
	}

	if (bWasFilletComprimized)
	{
		::AfxMessageBox(_T("Warning - The constant added to haunch depths caused some or all depths to be less than the fillet. This is not allowed. Comprimized haunch values were set to the fillet depth."),MB_OK | MB_ICONWARNING); 
	}

	return true;
}

void CFillHaunchDlg::OnHelp()
{
	EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_FILL_HAUNCH);
}
