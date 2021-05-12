///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

void CGirderPropertiesGraphController::DoDataExchange(CDataExchange* pDX)
{
   DDX_Control(pDX, IDC_INTERVAL, m_cbDropList);

   CIntervalGirderGraphControllerBase::DoDataExchange(pDX);
}

BOOL CGirderPropertiesGraphController::OnInitDialog()
{
   CIntervalGirderGraphControllerBase::OnInitDialog();

   FillPropertyCtrl();

   CComboBox* pcbProperties = (CComboBox*)GetDlgItem(IDC_PROPERTY);
   pcbProperties->SetCurSel(0);
   m_PropertyType = (CGirderPropertiesGraphBuilder::PropertyType)(pcbProperties->GetItemData(0));

   GET_IFACE(ISectionProperties,pSectProp);
   m_SectionPropertyType = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed );

   CheckRadioButton(IDC_TRANSFORMED,IDC_NET_DECK,(m_SectionPropertyType == pgsTypes::sptGross ? 1 : 0) + IDC_TRANSFORMED);

   UpdateSectionPropertyTypeControls();


   return TRUE;
}

bool CGirderPropertiesGraphController::SetPropertyType(CGirderPropertiesGraphBuilder::PropertyType propertyType)
{
   GET_IFACE(IDocumentType, pDocType);
   bool bPGSuperDoc = pDocType->IsPGSuperDocument();

   if (bPGSuperDoc && (propertyType == CGirderPropertiesGraphBuilder::TendonEccentricity || propertyType == CGirderPropertiesGraphBuilder::TendonProfile))
   {
      return false; // invalid property type
   }

   if (m_PropertyType != propertyType)
   {
      m_PropertyType = propertyType;

      CComboBox* pcbProperties = (CComboBox*)GetDlgItem(IDC_PROPERTY);
      int count = pcbProperties->GetCount();
      for (int i = 0; i < count; i++)
      {
         CGirderPropertiesGraphBuilder::PropertyType propType = (CGirderPropertiesGraphBuilder::PropertyType)(pcbProperties->GetItemData(i));
         if (propType == m_PropertyType)
         {
            pcbProperties->SetCurSel(i);
            break;
         }
      }

      ATLASSERT(pcbProperties->GetCurSel() != CB_ERR);

      UpdateSectionPropertyTypeControls();
      UpdateGraph();
   }

   return true;
}

CGirderPropertiesGraphBuilder::PropertyType CGirderPropertiesGraphController::GetPropertyType() const
{
   return m_PropertyType;
}

bool CGirderPropertiesGraphController::SetSectionPropertyType(pgsTypes::SectionPropertyType type)
{
   if (m_SectionPropertyType != type)
   {
      m_SectionPropertyType = type;
      int nIDC = GetSectionPropertyControlID(m_SectionPropertyType);
      if (nIDC < 0)
      {
         return false;
      }

      CheckRadioButton(IDC_TRANSFORMED, IDC_NET_DECK, nIDC);
      UpdateSectionPropertyTypeControls();
      UpdateGraph();
   }

   return true;
}

pgsTypes::SectionPropertyType CGirderPropertiesGraphController::GetSectionPropertyType() const
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

int CGirderPropertiesGraphController::GetSectionPropertyControlID(pgsTypes::SectionPropertyType type)
{
   int nIDC = -1;
   switch (type)
   {
   case pgsTypes::sptTransformed: nIDC = IDC_TRANSFORMED; break;
   case pgsTypes::sptGross: nIDC = IDC_GROSS; break;
   case pgsTypes::sptNetGirder: nIDC = IDC_NET_GIRDER; break;
   case pgsTypes::sptNetDeck: nIDC = IDC_NET_DECK; break;
   }
   return nIDC;
}

pgsTypes::SectionPropertyType CGirderPropertiesGraphController::GetSectionPropertyType(int nIDC)
{
   pgsTypes::SectionPropertyType spType = pgsTypes::sptGross;
   if (nIDC == IDC_TRANSFORMED)
   {
      spType = pgsTypes::sptTransformed;
   }
   else if (nIDC == IDC_GROSS)
   {
      spType = pgsTypes::sptGross;
   }
   else if (nIDC == IDC_NET_GIRDER)
   {
      spType = pgsTypes::sptNetGirder;
   }
   else if (nIDC == IDC_NET_DECK)
   {
      spType = pgsTypes::sptNetDeck;
   }
#if defined _DEBUG
   else
   {
      ATLASSERT(false); // should never get here
   }
#endif
   return spType;
}

void CGirderPropertiesGraphController::OnSectionPropertiesChanged()
{
   int nIDC = GetCheckedRadioButton(IDC_TRANSFORMED,IDC_NET_DECK);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   pgsTypes::SectionPropertyType spType = GetSectionPropertyType(nIDC);

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

bool CGirderPropertiesGraphController::IsInvariantProperty(CGirderPropertiesGraphBuilder::PropertyType propertyType) const
{
   // this properties don't depend on section properties type (gross, transformed, etc)
   if (propertyType == CGirderPropertiesGraphBuilder::Height ||
      propertyType == CGirderPropertiesGraphBuilder::TendonProfile ||
      propertyType == CGirderPropertiesGraphBuilder::EffectiveFlangeWidth ||
      propertyType == CGirderPropertiesGraphBuilder::Fc ||
      propertyType == CGirderPropertiesGraphBuilder::Ec
      )
   {
      return true;
   }
   return false;
}

void CGirderPropertiesGraphController::UpdateSectionPropertyTypeControls()
{
   BOOL bEnable = TRUE;
   if ( IsInvariantProperty(m_PropertyType) )
   {
      bEnable = FALSE;
   }

   GetDlgItem(IDC_TRANSFORMED)->EnableWindow(bEnable);
   GetDlgItem(IDC_GROSS)->EnableWindow(bEnable);
   GetDlgItem(IDC_NET_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_NET_DECK)->EnableWindow(bEnable);

   GET_IFACE(IBridge, pBridge);
   if (IsNonstructuralDeck(pBridge->GetDeckType()))
   {
      GetDlgItem(IDC_NET_DECK)->EnableWindow(FALSE);
   }
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
