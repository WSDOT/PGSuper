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

// PierLayoutPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "PierLayoutPage.h"
#include "PierDetailsDlg.h"
#include <PgsExt\ConcreteDetailsDlg.h>
#include <PsgLib\GirderLabel.h>

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>





/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage property page

IMPLEMENT_DYNCREATE(CPierLayoutPage, CPropertyPage)

CPierLayoutPage::CPierLayoutPage() : CPropertyPage(CPierLayoutPage::IDD)
{
    //{{AFX_DATA_INIT(CPierLayoutPage)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT


}

CPierLayoutPage::~CPierLayoutPage()
{
}

void CPierLayoutPage::Init(CPierData2* pPier)
{
    m_pPier = pPier;

    m_PierIdx = pPier->GetIndex();
    
}

void CPierLayoutPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPierConnectionsPage)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_EC, m_ctrlEc);
    DDX_Control(pDX, IDC_EC_LABEL, m_ctrlEcCheck);
    DDX_Control(pDX, IDC_FC, m_ctrlFc);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    // Pier Model
    DDX_CBItemData(pDX, IDC_PIER_MODEL_TYPE, m_PierModelType);
    DDX_CBEnum(pDX, IDC_PIER_LAYOUT_TYPE, m_PierLayoutType);

    CConcreteMaterial& concrete = m_pPier->GetConcrete();
    DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_UNIT, concrete.Fc, pDisplayUnits->GetStressUnit());
    DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, concrete.Ec, pDisplayUnits->GetModEUnit());
    DDX_Check_Bool(pDX, IDC_EC_LABEL, concrete.bUserEc);

    if (pDX->m_bSaveAndValidate)
    {
        // Only validate and save data owned by this page here.
        // Embedded child dialogs are validated and committed from OnApply/OnKillActive
        // so we do not recurse into UpdateData(TRUE) from inside DoDataExchange.
        m_pPier->SetPierModelType(m_PierModelType);
        m_pPier->SetPierLayoutType(m_PierLayoutType);
    }
}


bool CPierLayoutPage::CommitCommonPierLayout()
{

    if (!m_CommonPierLayoutDlg.UpdateData(TRUE))
    {
        return false;
    }

    m_pPier->SetTransverseOffset(m_CommonPierLayoutDlg.m_RefColumnIdx, 
    m_CommonPierLayoutDlg.m_TransverseOffset, m_CommonPierLayoutDlg.m_TransverseOffsetMeasurement);
    m_pPier->SetXBeamWidth(m_CommonPierLayoutDlg.m_XBeamWidth);

    for (int i = 0; i < 2; i++)
    {
        pgsTypes::SideType side = (pgsTypes::SideType)i;
        m_pPier->SetXBeamDimensions(side, m_CommonPierLayoutDlg.m_XBeamHeight[side], m_CommonPierLayoutDlg.m_XBeamTaperHeight[side],
        m_CommonPierLayoutDlg.m_XBeamTaperLength[side], m_CommonPierLayoutDlg.m_XBeamEndSlopeOffset[side]);
        m_pPier->SetXBeamOverhang(side, m_CommonPierLayoutDlg.m_XBeamOverhang[side]);
    }

    m_pPier->SetColumnFixity(m_CommonPierLayoutDlg.m_ColumnFixity);
    m_CommonPierLayoutDlg.m_ColumnLayoutGrid.GetColumnData(*m_pPier);

    ColumnIndexType nColumns = m_pPier->GetColumnCount();
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        CColumnData column = m_pPier->GetColumnData(colIdx);
        column.SetColumnHeightMeasurementType(m_CommonPierLayoutDlg.m_ColumnHeightMeasurementType);
        m_pPier->SetColumnData(colIdx, column);
    }

    return true;
}

bool CPierLayoutPage::CommitScallopedPierLayout()
{

    if (!m_ScallopedPierLayoutDlg.UpdateData(TRUE))
    {
        return false;
    }

    m_pPier->SetTransverseOffset(m_ScallopedPierLayoutDlg.m_RefColumnIdx,
    m_ScallopedPierLayoutDlg.m_TransverseOffset, m_ScallopedPierLayoutDlg.m_TransverseOffsetMeasurement);
    m_pPier->SetXBeamWidth(m_ScallopedPierLayoutDlg.m_XBeamWidth);

    for (int i = 0; i < 2; i++)
    {
        pgsTypes::SideType side = (pgsTypes::SideType)i;
        m_pPier->SetXBeamDimensions(side, m_ScallopedPierLayoutDlg.m_XBeamHeight[side], 0.0,
        0.0, m_ScallopedPierLayoutDlg.m_XBeamEndSlopeOffset[side]);
        m_pPier->SetXBeamOverhangRadius(side, m_ScallopedPierLayoutDlg.m_XBeamOverhangRadius[side]);
    }

    m_pPier->SetColumnFixity(m_ScallopedPierLayoutDlg.m_ColumnFixity);
    m_ScallopedPierLayoutDlg.m_ColumnLayoutGrid.GetColumnData(*m_pPier);

    ColumnIndexType nColumns = m_pPier->GetColumnCount();
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        CColumnData column = m_pPier->GetColumnData(colIdx);
        column.SetColumnHeightMeasurementType(m_ScallopedPierLayoutDlg.m_ColumnHeightMeasurementType);
        m_pPier->SetColumnData(colIdx, column);
    }

    return true;
}

bool CPierLayoutPage::CommitUserDefinedPierLayout()
{

    if (!m_UserDefinedPierLayoutDlg.UpdateData(TRUE))
    {
        return false;
    }

    m_pPier->SetTransverseOffset(m_UserDefinedPierLayoutDlg.m_RefColumnIdx,
        m_UserDefinedPierLayoutDlg.m_TransverseOffset, m_UserDefinedPierLayoutDlg.m_TransverseOffsetMeasurement);
    m_pPier->SetXBeamWidth(m_UserDefinedPierLayoutDlg.m_XBeamWidth);

    for (int i = 0; i < 2; i++)
    {
        pgsTypes::SideType side = (pgsTypes::SideType)i;
        m_pPier->SetXBeamDimensions(side, m_UserDefinedPierLayoutDlg.m_XBeamHeight[side], m_UserDefinedPierLayoutDlg.m_XBeamTaperHeight[side],
            m_UserDefinedPierLayoutDlg.m_XBeamTaperLength[side], m_UserDefinedPierLayoutDlg.m_XBeamEndSlopeOffset[side]);
        m_pPier->SetXBeamOverhang(side, m_UserDefinedPierLayoutDlg.m_XBeamOverhang[side]);
    }

    m_pPier->SetColumnFixity(m_UserDefinedPierLayoutDlg.m_ColumnFixity);
    m_UserDefinedPierLayoutDlg.m_ColumnLayoutGrid.GetColumnData(*m_pPier);

    ColumnIndexType nColumns = m_pPier->GetColumnCount();
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        CColumnData column = m_pPier->GetColumnData(colIdx);
        column.SetColumnHeightMeasurementType(m_UserDefinedPierLayoutDlg.m_ColumnHeightMeasurementType);
        m_pPier->SetColumnData(colIdx, column);
    }

    m_UserDefinedPierLayoutDlg.m_PierPointGrid.GetPierPointData(*m_pPier);

    PierPointIndexType nPierPoints = m_pPier->GetPierPointCount();
    for (PierPointIndexType ppIdx = 0; ppIdx < nPierPoints; ppIdx++)
    {
        CPierPointData pierPoint = m_pPier->GetPierPointData(ppIdx);
        m_pPier->SetPierPointData(ppIdx, pierPoint);
    }

    return true;
}

BOOL CPierLayoutPage::OnKillActive()
{
    if (!UpdateData(TRUE))
    {
        return FALSE;
    }

    if (m_PierModelType == pgsTypes::pmtPhysical)
    {
        if (m_PierLayoutType == pgsTypes::pltCommon)
        {
            if (!CommitCommonPierLayout())
            {
                return FALSE;
            }
        }
        else if (m_PierLayoutType == pgsTypes::pltScalloped)
        {
            if (!CommitScallopedPierLayout())
            {
                return FALSE;
            }
        }
        else if (m_PierLayoutType == pgsTypes::pltUserDefined)
        {
            if (!CommitUserDefinedPierLayout())
            {
                return FALSE;
            }
        }
    }

    return CPropertyPage::OnKillActive();
}

BOOL CPierLayoutPage::OnApply()
{
    if (!UpdateData(TRUE))
    {
        return FALSE;
    }

    if (m_PierModelType == pgsTypes::pmtPhysical)
    {
        if (m_PierLayoutType == pgsTypes::pltCommon)
        {
            if (!CommitCommonPierLayout())
            {
                return FALSE;
            }
        }
        else if (m_PierLayoutType == pgsTypes::pltScalloped)
        {
            if (!CommitScallopedPierLayout())
            {
                return FALSE;
            }
        }
        else if (m_PierLayoutType == pgsTypes::pltUserDefined)
        {
            if (!CommitUserDefinedPierLayout())
            {
                return FALSE;
            }
        }
    }

    SetModified(FALSE);
    return CPropertyPage::OnApply();
}

BEGIN_MESSAGE_MAP(CPierLayoutPage, CPropertyPage)
    //{{AFX_MSG_MAP(CPierLayoutPage)
    ON_BN_CLICKED(IDC_EC_LABEL, OnUserEc)
    ON_EN_CHANGE(IDC_FC, OnChangeFc)
    ON_BN_CLICKED(IDC_MORE_PROPERTIES, OnMoreProperties)
    ON_CBN_SELCHANGE(IDC_PIER_MODEL_TYPE, OnPierModelTypeChanged)
    ON_CBN_SELCHANGE(IDC_PIER_LAYOUT_TYPE, OnPierLayoutTypeChanged)
    ON_BN_CLICKED(IDC_LAYOUT_GRAPHIC, OnShowLiveClicked)
    ON_MESSAGE(WM_PIER_LAYOUT_CHANGED, OnPierLayoutChanged)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage message handlers

BOOL CPierLayoutPage::OnInitDialog()
{

    m_PierModelType = m_pPier->GetPierModelType();
    m_PierLayoutType = m_pPier->GetPierLayoutType();

    FillPierModelTypeComboBox();
    FillPierLayoutTypeComboBox();

    CWnd* pBox = GetDlgItem(IDC_STATIC_BOUNDS);
    pBox->ShowWindow(SW_HIDE);

    CRect boxRect;
    pBox->GetWindowRect(&boxRect);
    ScreenToClient(boxRect);

    m_pPier->SetPierLayoutType(m_PierLayoutType);

    m_CommonPierLayoutDlg.SetPierModelType(m_PierModelType);
    m_CommonPierLayoutDlg.SetPierData(*m_pPier);
    VERIFY(m_CommonPierLayoutDlg.Create(IDD_PIER_LAYOUT_COMMON, this));
    VERIFY(m_CommonPierLayoutDlg.SetWindowPos(GetDlgItem(IDC_STATIC_BOUNDS), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

    m_ScallopedPierLayoutDlg.SetPierModelType(m_PierModelType);
    m_ScallopedPierLayoutDlg.SetPierData(*m_pPier);
    VERIFY(m_ScallopedPierLayoutDlg.Create(IDD_PIER_LAYOUT_SCALLOPED, this));
    VERIFY(m_ScallopedPierLayoutDlg.SetWindowPos(GetDlgItem(IDC_STATIC_BOUNDS), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));
    
    m_UserDefinedPierLayoutDlg.SetPierModelType(m_PierModelType);
    m_UserDefinedPierLayoutDlg.SetPierData(*m_pPier);
    VERIFY(m_UserDefinedPierLayoutDlg.Create(IDD_PIER_LAYOUT_USERDEFINED, this));
    VERIFY(m_UserDefinedPierLayoutDlg.SetWindowPos(GetDlgItem(IDC_STATIC_BOUNDS), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

    CPropertyPage::OnInitDialog();

    UpdateConcreteTypeLabel();
    if (m_strUserEc == _T(""))
    {
        m_ctrlEc.GetWindowText(m_strUserEc);
    }
    OnUserEc();

    OnPierModelTypeChanged();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierLayoutPage::UpdateConcreteTypeLabel()
{
    CString strLabel;
    if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims)
    {
        switch (m_pPier->GetConcrete().Type)
        {
        case pgsTypes::Normal:
            strLabel = _T("Normal Weight Concrete");
            break;

        case pgsTypes::AllLightweight:
            strLabel = _T("All Lightweight Concrete");
            break;

        case pgsTypes::SandLightweight:
            strLabel = _T("Sand Lightweight Concrete");
            break;

        case pgsTypes::PCI_UHPC: // UI should prevent UHPC for piers
        case pgsTypes::UHPC:
        default:
            ATLASSERT(false); // should never get here
            strLabel = _T("Concrete Type Label Error");
        }
    }
    else
    {
        // LRFD 2016 and later there is only normal and lightweight concrete. We
        //use the SandLightweight enum to mean "lightweight"
        ATLASSERT(m_pPier->GetConcrete().Type == pgsTypes::Normal || m_pPier->GetConcrete().Type == pgsTypes::SandLightweight);
        switch (m_pPier->GetConcrete().Type)
        {
        case pgsTypes::Normal:
            strLabel = _T("Normal Weight Concrete");
            break;

        case pgsTypes::SandLightweight:
            strLabel = _T("Lightweight Concrete");
            break;

        case pgsTypes::PCI_UHPC: // UI should prevent UHPC for piers
        case pgsTypes::UHPC:
        default:
            ATLASSERT(false); // should never get here
            strLabel = _T("Concrete Type Label Error");
        }
    }

    GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText(strLabel);
}



void CPierLayoutPage::FillPierModelTypeComboBox()
{
    CComboBox* pcbPierModel = (CComboBox*)GetDlgItem(IDC_PIER_MODEL_TYPE);
    pcbPierModel->ResetContent();

    int idx = pcbPierModel->AddString(_T("Idealized"));
    pcbPierModel->SetItemData(idx, (DWORD_PTR)pgsTypes::pmtIdealized);

    idx = pcbPierModel->AddString(_T("Physical"));
    pcbPierModel->SetItemData(idx, (DWORD_PTR)pgsTypes::pmtPhysical);
}

void CPierLayoutPage::FillPierLayoutTypeComboBox()
{
    CComboBox* pcbPierModel = (CComboBox*)GetDlgItem(IDC_PIER_LAYOUT_TYPE);
    pcbPierModel->ResetContent();

    int idx = pcbPierModel->AddString(_T("Common"));
    pcbPierModel->SetItemData(idx, (DWORD_PTR)pgsTypes::pltCommon);

    idx = pcbPierModel->AddString(_T("Scalloped"));
    pcbPierModel->SetItemData(idx, (DWORD_PTR)pgsTypes::pltScalloped);

    idx = pcbPierModel->AddString(_T("User-Defined"));
    pcbPierModel->SetItemData(idx, (DWORD_PTR)pgsTypes::pltUserDefined);

}

void CPierLayoutPage::OnHelp()
{
    EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PIERDETAILS_LAYOUT);
}

void CPierLayoutPage::OnUserEc()
{
    BOOL bEnable = m_ctrlEcCheck.GetCheck();

    GetDlgItem(IDC_EC_LABEL)->EnableWindow(TRUE);

    if (bEnable == FALSE)
    {
        m_ctrlEc.GetWindowText(m_strUserEc);
        UpdateEc();
    }
    else
    {
        m_ctrlEc.SetWindowText(m_strUserEc);
    }

    GetDlgItem(IDC_EC)->EnableWindow(bEnable);
    GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}

void CPierLayoutPage::OnMoreProperties()
{
    UpdateData(TRUE);
    CConcreteDetailsDlg dlg(true/*f'c*/, false/*no uhpc*/, false/*don't enable Compute Time Parameters option*/);
    CConcreteMaterial& concrete = m_pPier->GetConcrete();

    dlg.m_fc28 = concrete.Fc;
    dlg.m_Ec28 = concrete.Ec;
    dlg.m_bUserEc28 = concrete.bUserEc;

    dlg.m_General.m_Type = concrete.Type;
    dlg.m_General.m_AggSize = concrete.MaxAggregateSize;
    dlg.m_General.m_Ds = concrete.StrengthDensity;
    dlg.m_General.m_Dw = concrete.WeightDensity;
    dlg.m_General.m_strUserEc = m_strUserEc;

    dlg.m_AASHTO.m_EccK1 = concrete.EcK1;
    dlg.m_AASHTO.m_EccK2 = concrete.EcK2;
    dlg.m_AASHTO.m_CreepK1 = concrete.CreepK1;
    dlg.m_AASHTO.m_CreepK2 = concrete.CreepK2;
    dlg.m_AASHTO.m_ShrinkageK1 = concrete.ShrinkageK1;
    dlg.m_AASHTO.m_ShrinkageK2 = concrete.ShrinkageK2;
    dlg.m_AASHTO.m_bHasFct = concrete.bHasFct;
    dlg.m_AASHTO.m_Fct = concrete.Fct;

    if (dlg.DoModal() == IDOK)
    {
        concrete.Fc = dlg.m_fc28;
        concrete.Ec = dlg.m_Ec28;
        concrete.bUserEc = dlg.m_bUserEc28;

        concrete.Type = dlg.m_General.m_Type;
        concrete.MaxAggregateSize = dlg.m_General.m_AggSize;
        concrete.StrengthDensity = dlg.m_General.m_Ds;
        concrete.WeightDensity = dlg.m_General.m_Dw;
        concrete.EcK1 = dlg.m_AASHTO.m_EccK1;
        concrete.EcK2 = dlg.m_AASHTO.m_EccK2;
        concrete.CreepK1 = dlg.m_AASHTO.m_CreepK1;
        concrete.CreepK2 = dlg.m_AASHTO.m_CreepK2;
        concrete.ShrinkageK1 = dlg.m_AASHTO.m_ShrinkageK1;
        concrete.ShrinkageK2 = dlg.m_AASHTO.m_ShrinkageK2;
        concrete.bHasFct = dlg.m_AASHTO.m_bHasFct;
        concrete.Fct = dlg.m_AASHTO.m_Fct;

        m_strUserEc = dlg.m_General.m_strUserEc;
        m_ctrlEc.SetWindowText(m_strUserEc);

        UpdateData(FALSE);
        OnUserEc();
        UpdateConcreteTypeLabel();
    }
}

void CPierLayoutPage::OnChangeFc()
{
    UpdateEc();
}

void CPierLayoutPage::UpdateEc()
{
    // update modulus
    if (m_ctrlEcCheck.GetCheck() == 0)
    {
        // blank out ec
        CString strEc;
        m_ctrlEc.SetWindowText(strEc);

        // need to manually parse strength and density values
        CString strFc, strDensity, strK1, strK2;
        m_ctrlFc.GetWindowText(strFc);


        auto pBroker = EAFGetBroker();
        GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

        strDensity.Format(_T("%s"), FormatDimension(m_pPier->GetConcrete().StrengthDensity, pDisplayUnits->GetDensityUnit(), false));
        strK1.Format(_T("%f"), m_pPier->GetConcrete().EcK1);
        strK2.Format(_T("%f"), m_pPier->GetConcrete().EcK2);

        strEc = CConcreteDetailsDlg::UpdateEc(m_pPier->GetConcrete().Type, strFc, strDensity, strK1, strK2);
        m_ctrlEc.SetWindowText(strEc);
    }
}

void CPierLayoutPage::OnPierModelTypeChanged()
{
    CComboBox* pcbPierModel = (CComboBox*)GetDlgItem(IDC_PIER_MODEL_TYPE);
    int curSel = pcbPierModel->GetCurSel();
    m_PierModelType = (pgsTypes::PierModelType)pcbPierModel->GetItemData(curSel);

    const bool bPhysical = (m_PierModelType == pgsTypes::pmtPhysical);
    int nShow = bPhysical ? SW_SHOW : SW_HIDE;

    CWnd* pWnd = pcbPierModel->GetNextWindow(GW_HWNDNEXT);
    while (pWnd)
    {
        int nID = pWnd->GetDlgCtrlID();

        bool bIsEmbeddedPierDlg =
            (pWnd->GetSafeHwnd() == m_CommonPierLayoutDlg.GetSafeHwnd()) ||
            (pWnd->GetSafeHwnd() == m_ScallopedPierLayoutDlg.GetSafeHwnd()) ||
            (pWnd->GetSafeHwnd() == m_UserDefinedPierLayoutDlg.GetSafeHwnd());

        if (!bIsEmbeddedPierDlg &&
            nID != IDC_PIER_MODEL_LABEL &&
            nID != IDC_PIER_MODEL_TYPE)
        {
            if (nID == IDC_EC_LABEL)
            {
                m_ctrlEcCheck.ShowWindow(nShow);
            }
            else
            {
                pWnd->ShowWindow(nShow);
            }
        }

        pWnd = pWnd->GetNextWindow(GW_HWNDNEXT);
    }

    // Manage embedded dialogs explicitly
    if (bPhysical)
    {
        SwapDialogs(); // show only the selected physical layout dialog
    }
    else
    {
        m_CommonPierLayoutDlg.ShowWindow(SW_HIDE);
        m_ScallopedPierLayoutDlg.ShowWindow(SW_HIDE);
        m_UserDefinedPierLayoutDlg.ShowWindow(SW_HIDE);
    }

    SetModified(TRUE);
}



void CPierLayoutPage::OnPierLayoutTypeChanged()
{
    CComboBox* pcbPierModel = (CComboBox*)GetDlgItem(IDC_PIER_LAYOUT_TYPE);
    int curSel = pcbPierModel->GetCurSel();
    if (curSel == CB_ERR)
        return;

    pgsTypes::PierLayoutType layoutType = (pgsTypes::PierLayoutType)pcbPierModel->GetItemData(curSel);
    if (layoutType == m_PierLayoutType)
        return;

    bool saved = false;
    switch (m_PierLayoutType)
    {
    case pgsTypes::pltCommon:
        saved = SaveColumnLayout(m_CommonPierLayoutDlg, m_CommonPierLayoutDlg.m_ColumnLayoutGrid);
        break;
    case pgsTypes::pltScalloped:
        saved = SaveColumnLayout(m_ScallopedPierLayoutDlg, m_ScallopedPierLayoutDlg.m_ColumnLayoutGrid);
        break;
    case pgsTypes::pltUserDefined:
        saved = SaveColumnLayout(m_UserDefinedPierLayoutDlg, m_UserDefinedPierLayoutDlg.m_ColumnLayoutGrid);
        break;
    }

    if (!saved)
    {
        CDataExchange dx(this, FALSE);
        DDX_CBEnum(&dx, IDC_PIER_LAYOUT_TYPE, m_PierLayoutType);
        return;
    }

    m_PierLayoutType = layoutType;
    switch (m_PierLayoutType)
    {
    case pgsTypes::pltCommon:
        LoadColumnLayout(m_CommonPierLayoutDlg, m_CommonPierLayoutDlg.m_ColumnLayoutGrid, m_CommonPierLayoutDlg.m_Pier);
        break;
    case pgsTypes::pltScalloped:
        LoadColumnLayout(m_ScallopedPierLayoutDlg, m_ScallopedPierLayoutDlg.m_ColumnLayoutGrid, m_ScallopedPierLayoutDlg.m_Pier);
        break;
    case pgsTypes::pltUserDefined:
        LoadColumnLayout(m_UserDefinedPierLayoutDlg, m_UserDefinedPierLayoutDlg.m_ColumnLayoutGrid, m_UserDefinedPierLayoutDlg.m_Pier);
        break;
    }

    SwapDialogs();

    if (m_pPierLayoutPopout && ::IsWindow(m_pPierLayoutPopout->GetSafeHwnd()))
    {
        ShowPierLayoutPopout();
    }
}

bool CPierLayoutPage::SaveColumnLayout(CDialog& dialog, CColumnLayoutGrid& grid)
{
    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    ColumnIndexType refColumnIdx;
    Float64 offset;
    pgsTypes::OffsetMeasurementType offsetType;
    m_pPier->GetTransverseOffset(&refColumnIdx, &offset, &offsetType);
    pgsTypes::ColumnLongitudinalBaseFixityType fixity = m_pPier->GetColumnFixity();
    CColumnData::ColumnHeightMeasurementType heightMeasure = m_pPier->GetColumnData(0).GetColumnHeightMeasurementType();

    try
    {
        CDataExchange dx(&dialog, TRUE);
        CColumnLayoutGrid::DDV_ColumnGrid(&dx, grid);
        DDX_CBIndex(&dx, IDC_REFCOLUMN, refColumnIdx);
        DDX_OffsetAndTag(&dx, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, offset, pDisplayUnits->GetSpanLengthUnit());
        DDX_CBItemData(&dx, IDC_REFCOLUMN_MEASUREMENT, offsetType);
        DDX_CBItemData(&dx, IDC_FIXITY, fixity);
        DDX_CBItemData(&dx, IDC_HEIGHT_MEASURE, heightMeasure);
    }
    catch (CUserException* pException)
    {
        pException->Delete();
        return false;
    }

    // Save only the shared column layout; cap inputs belong to each layout dialog.
    grid.GetColumnData(*m_pPier);
    m_pPier->SetTransverseOffset(refColumnIdx, offset, offsetType);
    m_pPier->SetColumnFixity(fixity);
    for (ColumnIndexType colIdx = 0; colIdx < m_pPier->GetColumnCount(); colIdx++)
    {
        CColumnData column = m_pPier->GetColumnData(colIdx);
        column.SetColumnHeightMeasurementType(heightMeasure);
        m_pPier->SetColumnData(colIdx, column);
    }
    return true;
}

void CPierLayoutPage::LoadColumnLayout(CDialog& dialog, CColumnLayoutGrid& grid, CPierData2& pier)
{
    ColumnIndexType nColumns = m_pPier->GetColumnCount();
    pier.SetColumnCount(nColumns);
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        pier.SetColumnData(colIdx, m_pPier->GetColumnData(colIdx));
        pier.SetColumnScallopDepth(colIdx, m_pPier->GetColumnScallopDepth(colIdx));
        if (colIdx + 1 < nColumns)
        {
            pier.SetColumnSpacing(colIdx, m_pPier->GetColumnSpacing(colIdx));
            pier.SetColumnScallopRadius(colIdx, m_pPier->GetColumnScallopRadius(colIdx));
        }
    }

    ColumnIndexType refColumnIdx;
    Float64 offset;
    pgsTypes::OffsetMeasurementType offsetType;
    m_pPier->GetTransverseOffset(&refColumnIdx, &offset, &offsetType);
    pier.SetTransverseOffset(refColumnIdx, offset, offsetType);
    pgsTypes::ColumnLongitudinalBaseFixityType fixity = m_pPier->GetColumnFixity();
    pier.SetColumnFixity(fixity);
    CColumnData::ColumnHeightMeasurementType heightMeasure = pier.GetColumnData(0).GetColumnHeightMeasurementType();
    grid.SetColumnData(pier);
    grid.SetHeightMeasurementType(heightMeasure);

    CComboBox* pcbRefColumn = (CComboBox*)dialog.GetDlgItem(IDC_REFCOLUMN);
    pcbRefColumn->ResetContent();
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        CString label;
        label.Format(_T("Column %d"), LABEL_COLUMN(colIdx));
        pcbRefColumn->AddString(label);
    }

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    CDataExchange dx(&dialog, FALSE);
    DDX_CBIndex(&dx, IDC_REFCOLUMN, refColumnIdx);
    DDX_OffsetAndTag(&dx, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, offset, pDisplayUnits->GetSpanLengthUnit());
    DDX_CBItemData(&dx, IDC_REFCOLUMN_MEASUREMENT, offsetType);
    DDX_CBItemData(&dx, IDC_FIXITY, fixity);
    DDX_CBItemData(&dx, IDC_HEIGHT_MEASURE, heightMeasure);
}

void CPierLayoutPage::SwapDialogs() // call UpdateData(TRUE) on these?
{
    if (m_PierModelType == pgsTypes::pmtPhysical)
    {
        if (m_PierLayoutType == pgsTypes::pltCommon)
        {
            m_CommonPierLayoutDlg.ShowWindow(SW_SHOW);
            m_ScallopedPierLayoutDlg.ShowWindow(SW_HIDE);
            m_UserDefinedPierLayoutDlg.ShowWindow(SW_HIDE);
        }
        else if (m_PierLayoutType == pgsTypes::pltScalloped)
        {
            m_CommonPierLayoutDlg.ShowWindow(SW_HIDE);
            m_ScallopedPierLayoutDlg.ShowWindow(SW_SHOW);
            m_UserDefinedPierLayoutDlg.ShowWindow(SW_HIDE);
        }
        else if (m_PierLayoutType == pgsTypes::pltUserDefined)
        {
            m_CommonPierLayoutDlg.ShowWindow(SW_HIDE);
            m_ScallopedPierLayoutDlg.ShowWindow(SW_HIDE);
            m_UserDefinedPierLayoutDlg.ShowWindow(SW_SHOW);
        }
        else
        {
            m_CommonPierLayoutDlg.ShowWindow(SW_HIDE);
            m_ScallopedPierLayoutDlg.ShowWindow(SW_HIDE);
            m_UserDefinedPierLayoutDlg.ShowWindow(SW_HIDE);
        }

    }

}

LRESULT CPierLayoutPage::OnPierLayoutChanged(WPARAM, LPARAM)
{
    RefreshPierLayoutPopout();
    return 0;
}

void CPierLayoutPage::RefreshPierLayoutPopout()
{
    if (m_pPierLayoutPopout &&
        ::IsWindow(m_pPierLayoutPopout->GetSafeHwnd()))
    {
        m_pPierLayoutPopout->UpdateDisplayObjects();
        m_pPierLayoutPopout->Invalidate();
		m_pPierLayoutPopout->UpdateWindow();
    }
}




void CPierLayoutPage::ShowPierLayoutPopout()
{
    // Select the correct data source based on current model/layout
    IPierLayoutDataSource* pSource = nullptr;

    if (m_PierModelType == pgsTypes::pmtPhysical)
    {
        switch (m_PierLayoutType)
        {
        case pgsTypes::pltCommon:
            pSource = &m_CommonPierLayoutDlg;
            break;
        case pgsTypes::pltScalloped:
            pSource = &m_ScallopedPierLayoutDlg;
            break;
        case pgsTypes::pltUserDefined:
            pSource = &m_UserDefinedPierLayoutDlg;
            break;
        default:
            // No popout for custom/unknown layouts
            pSource = nullptr;
            break;
        }
    }
    else
    {
        // Idealized model has no physical layout to draw
        pSource = nullptr;
    }

    if (pSource == nullptr)
    {
        // nothing to show
        return;
    }

    // If popout doesn't exist, create it with the chosen source.
    if (m_pPierLayoutPopout == nullptr || !::IsWindow(m_pPierLayoutPopout->GetSafeHwnd()))
    {
        m_pPierLayoutPopout = new CDrawPierLayoutControl();
        if (!m_pPierLayoutPopout->CreatePopout(pSource, this))
        {
            delete m_pPierLayoutPopout;
            m_pPierLayoutPopout = nullptr;
            return;
        }
    }
    else
    {
        // Popout already exists — re-bind to the new data source.
        m_pPierLayoutPopout->CustomInit(pSource);
    }

    // Show and refresh
    pSource->ChangePierLayout();
    m_pPierLayoutPopout->ShowWindow(SW_SHOW);
    m_pPierLayoutPopout->UpdateDisplayObjects();
    m_pPierLayoutPopout->Invalidate();
    m_pPierLayoutPopout->UpdateWindow();
}


void CPierLayoutPage::OnShowLiveClicked()
{
    // Determine the active embedded dialog (data source) based on current model/layout
    CDialog* pActiveDlg = nullptr;
    if (m_PierModelType == pgsTypes::pmtPhysical)
    {
        switch (m_PierLayoutType)
        {
        case pgsTypes::pltCommon:
            pActiveDlg = &m_CommonPierLayoutDlg;
            break;
        case pgsTypes::pltScalloped:
            pActiveDlg = &m_ScallopedPierLayoutDlg;
            break;
        case pgsTypes::pltUserDefined:
            pActiveDlg = &m_UserDefinedPierLayoutDlg;
            break;
        default:
            pActiveDlg = nullptr;
            break;
        }
    }

    // If there is no valid active dialog, nothing to do.
    if (pActiveDlg == nullptr || !::IsWindow(pActiveDlg->GetSafeHwnd()))
    {
        return;
    }

    // hide the embedded control in the dialog (we use the popout for live)
    if (CWnd* pEmbedded = pActiveDlg->GetDlgItem(IDC_PIER_LAYOUT))
        pEmbedded->ShowWindow(SW_HIDE);

    // Create / show a popout bound to the currently active dialog's data source
    ShowPierLayoutPopout();

}




