///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "GirderPropertiesGraphController.h"
#include <Graphing\GirderPropertiesGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\Bridge.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CGirderPropertiesGraphController,CIntervalGirderGraphControllerBase)

CGirderPropertiesGraphController::CGirderPropertiesGraphController():
CIntervalGirderGraphControllerBase(true/*all groups*/),
m_PropertyType(CGirderPropertiesGraphBuilder::Height),
m_SectionPropertyType(pgsTypes::sptTransformed)
{
}

BEGIN_MESSAGE_MAP(CGirderPropertiesGraphController, CIntervalGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CGirderPropertiesGraphController)
   ON_CBN_SELCHANGE( IDC_PROPERTY, OnPropertyChanged )
   ON_BN_CLICKED(IDC_TRANSFORMED, OnSectionPropertiesChanged)
   ON_BN_CLICKED(IDC_GROSS, OnSectionPropertiesChanged)
   ON_BN_CLICKED(IDC_NET_GIRDER, OnSectionPropertiesChanged)
   ON_BN_CLICKED(IDC_NET_DECK, OnSectionPropertiesChanged)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CGirderPropertiesGraphController::OnInitDialog()
{
   CIntervalGirderGraphControllerBase::OnInitDialog();

   FillPropertyCtrl();

   CComboBox* pcbProperties = (CComboBox*)GetDlgItem(IDC_PROPERTY);
   pcbProperties->SetCurSel(0);
   m_PropertyType = (CGirderPropertiesGraphBuilder::PropertyType)(pcbProperties->GetItemData(0));

   GET_IFACE(ISectionProperties,pSectProp);
   m_SectionPropertyType = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed );

   CheckRadioButton(IDC_TRANSFORMED,IDC_NET_DECK,(m_SectionPropertyType == pgsTypes::spmGross ? 1 : 0) + IDC_TRANSFORMED);

   UpdateSectionPropertyTypeControls();

   return TRUE;
}

CGirderPropertiesGraphBuilder::PropertyType CGirderPropertiesGraphController::GetPropertyType()
{
   return m_PropertyType;
}

pgsTypes::SectionPropertyType CGirderPropertiesGraphController::GetSectionPropertyType()
{
   return m_SectionPropertyType;
}

IndexType CGirderPropertiesGraphController::GetGraphCount()
{
   return 1;
}

void CGirderPropertiesGraphController::OnPropertyChanged()
{
   CComboBox* pcbProperties = (CComboBox*)GetDlgItem(IDC_PROPERTY);
   int curSel = pcbProperties->GetCurSel();
   CGirderPropertiesGraphBuilder::PropertyType propertyType = (CGirderPropertiesGraphBuilder::PropertyType)(pcbProperties->GetItemData(curSel));
   if ( m_PropertyType != propertyType )
   {
      m_PropertyType = propertyType;

      UpdateSectionPropertyTypeControls();

      UpdateGraph();
   }
}

void CGirderPropertiesGraphController::OnSectionPropertiesChanged()
{
   int i = GetCheckedRadioButton(IDC_TRANSFORMED,IDC_NET_DECK);
   pgsTypes::SectionPropertyType spType;
   if ( i == IDC_TRANSFORMED )
   {
      spType = pgsTypes::sptTransformed;
   }
   else if ( i == IDC_GROSS )
   {
      spType = pgsTypes::sptGross;
   }
   else if ( i == IDC_NET_GIRDER )
   {
      spType = pgsTypes::sptNetGirder;
   }
   else if ( i == IDC_NET_DECK )
   {
      spType = pgsTypes::sptNetDeck;
   }

   if ( spType != m_SectionPropertyType )
   {
      m_SectionPropertyType = spType;
      UpdateGraph();
   }
}

void CGirderPropertiesGraphController::FillPropertyCtrl()
{
   CComboBox* pcbProperties = (CComboBox*)GetDlgItem(IDC_PROPERTY);
   int curSel = pcbProperties->GetCurSel();

   pcbProperties->ResetContent();

   CGirderPropertiesGraphBuilder* pGraphBuilder = (CGirderPropertiesGraphBuilder*)GetGraphBuilder();

   GET_IFACE(IDocumentType,pDocType);
   bool bPGSuperDoc = pDocType->IsPGSuperDocument();

   int idx;
   for ( int i = 0; i < int(CGirderPropertiesGraphBuilder::PropertyTypeCount); i++ )
   {
      CGirderPropertiesGraphBuilder::PropertyType propertyType = (CGirderPropertiesGraphBuilder::PropertyType)i;

      if ( bPGSuperDoc && (propertyType == CGirderPropertiesGraphBuilder::TendonEccentricity || propertyType == CGirderPropertiesGraphBuilder::TendonProfile) )
      {
         continue;
      }

      idx = pcbProperties->AddString(pGraphBuilder->GetPropertyLabel(propertyType));
      pcbProperties->SetItemData(idx,(DWORD_PTR)propertyType);
   }
}

void CGirderPropertiesGraphController::UpdateSectionPropertyTypeControls()
{
   BOOL bEnable = TRUE;
   if ( m_PropertyType == CGirderPropertiesGraphBuilder::Height ||
        m_PropertyType == CGirderPropertiesGraphBuilder::TendonProfile ||
        m_PropertyType == CGirderPropertiesGraphBuilder::EffectiveFlangeWidth ||
        m_PropertyType == CGirderPropertiesGraphBuilder::Fc ||
        m_PropertyType == CGirderPropertiesGraphBuilder::Ec
      )
   {
      bEnable = FALSE;
   }

   GetDlgItem(IDC_TRANSFORMED)->EnableWindow(bEnable);
   GetDlgItem(IDC_GROSS)->EnableWindow(bEnable);
   GetDlgItem(IDC_NET_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_NET_DECK)->EnableWindow(bEnable);
}

#ifdef _DEBUG
void CGirderPropertiesGraphController::AssertValid() const
{
	CIntervalGirderGraphControllerBase::AssertValid();
}

void CGirderPropertiesGraphController::Dump(CDumpContext& dc) const
{
	CIntervalGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
