///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// GirderDescDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperDoc.h"
#include "GirderDescDlg.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_CHECKBOX 100

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDlg

IMPLEMENT_DYNAMIC(CGirderDescDlg, CPropertySheet)

CGirderDescDlg::CGirderDescDlg(const CSegmentKey& segmentKey,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   m_SegmentKey = segmentKey;

   ATLASSERT(m_SegmentKey.segmentIndex == 0); // this is a PGSuper dialog

   CString strTitle;
   strTitle.Format(_T("Girder Details for Span %d, Girder %s"),LABEL_SPAN(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex));
   SetTitle(strTitle);

   Init();
}

CGirderDescDlg::~CGirderDescDlg()
{
}

void CGirderDescDlg::Init()
{
   m_bApplyToAll = false;

   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags   |= PSP_HASHELP;
   m_Prestress.m_psp.dwFlags |= PSP_HASHELP;
   m_Shear.m_psp.dwFlags     |= PSP_HASHELP;
   m_LongRebar.m_psp.dwFlags |= PSP_HASHELP;
   m_Lifting.m_psp.dwFlags   |= PSP_HASHELP;
   m_Debond.m_psp.dwFlags    |= PSP_HASHELP;

   AddPage( &m_General );
   AddPage( &m_Prestress );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   AddAdditionalPropertyPages( pSpecEntry->AllowStraightStrandExtensions(), pStrandGeom->CanDebondStrands(m_SegmentKey,pgsTypes::Straight) );
}

BEGIN_MESSAGE_MAP(CGirderDescDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CGirderDescDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
      WBFL_ON_PROPSHEET_OK
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDlg message handlers


void CGirderDescDlg::DoUpdate()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IShear,pShear);
   GET_IFACE2(pBroker,ILongitudinalRebar,pLongitudinaRebar);
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_SegmentKey.girderIndex);
   m_Segment = *pGirder->GetSegment(m_SegmentKey.segmentIndex);
   m_strGirderName = pGroup->GetGirderName(m_SegmentKey.girderIndex);

   m_ConditionFactorType = pGirder->GetConditionFactorType();
   m_ConditionFactor     = pGirder->GetConditionFactor();

   // Setup girder data for our pages
   m_General.m_bUseSameGirderType = pBridgeDesc->UseSameGirderForEntireBridge();
   m_General.m_SlabOffsetType = pBridgeDesc->GetSlabOffsetType();
   pIBridgeDesc->GetSlabOffset(m_SegmentKey,&m_General.m_SlabOffset[pgsTypes::metStart],&m_General.m_SlabOffset[pgsTypes::metEnd]);

   // shear page
   m_Shear.m_CurGrdName = pGirder->GetGirderName();
   m_Shear.m_ShearData  = m_Segment.ShearData;

   // longitudinal rebar page
   m_LongRebar.m_CurGrdName = m_Shear.m_CurGrdName;
}

BOOL CGirderDescDlg::OnOK()
{
   UpdateData(TRUE); // calls DoDataExchange
   return FALSE; // MUST RETURN FALSE!!!!
}

void CGirderDescDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bApplyToAll);
}

BOOL CGirderDescDlg::OnInitDialog() 
{
	DoUpdate();

	BOOL bResult = CPropertySheet::OnInitDialog();

   // Set the dialog icon
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_EDIT_GIRDER),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   SetDebondTabName();

   // Build the OK button
   CWnd* pOK = GetDlgItem(IDOK);
   CRect rOK;
   pOK->GetWindowRect(&rOK);

   CRect wndPage;
   GetActivePage()->GetWindowRect(&wndPage);

   CRect rect;
   rect.left = wndPage.left;
   rect.top = rOK.top;
   rect.bottom = rOK.bottom;
   rect.right = rOK.left - 7;
   ScreenToClient(&rect);
   m_CheckBox.Create(_T("Copy to all girders in this span"),WS_CHILD | WS_VISIBLE | BS_LEFTTEXT | BS_RIGHT | BS_AUTOCHECKBOX,rect,this,IDC_CHECKBOX);
   m_CheckBox.SetFont(GetFont());

   UpdateData(FALSE); // calls DoDataExchange

   return bResult;
}

void CGirderDescDlg::SetDebondTabName()
{
   int index = GetPageIndex(&m_Debond);
   if ( index < 0 )
      return; // not using the debond tab

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CTabCtrl* pTab = GetTabControl();
   TC_ITEM ti;
   ti.mask = TCIF_TEXT;

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   bool bCanDebond = pStrandGeometry->CanDebondStrands(m_strGirderName.c_str(),pgsTypes::Straight);
   if ( bCanDebond )
   {
      ti.pszText = _T("Debonding");
   }
   else
   {
      ti.pszText = _T("Strand Extensions");
   }
   
   pTab->SetItem(index,&ti);
}

StrandIndexType CGirderDescDlg::GetStraightStrandCount()
{
   return m_Segment.Strands.Nstrands[pgsTypes::Straight];
}

StrandIndexType CGirderDescDlg::GetHarpedStrandCount()
{
   return m_Segment.Strands.Nstrands[pgsTypes::Harped];
}

void CGirderDescDlg::SetSegment(const CPrecastSegmentData& segment)
{
   m_Segment = segment;
   m_Shear.m_ShearData = m_Segment.ShearData;
}

const CPrecastSegmentData& CGirderDescDlg::GetSegment()
{
   m_Segment.ShearData = m_Shear.m_ShearData;
   return m_Segment;
}

void CGirderDescDlg::SetConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor)
{
   m_ConditionFactorType = conditionFactorType;
   m_ConditionFactor     = conditionFactor;
}

pgsTypes::ConditionFactorType CGirderDescDlg::GetConditionFactorType()
{
   return m_ConditionFactorType;
}

Float64 CGirderDescDlg::GetConditionFactor()
{
   return m_ConditionFactor;
}

ConfigStrandFillVector CGirderDescDlg::ComputeStrandFillVector(pgsTypes::StrandType type)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   if (m_Segment.Strands.NumPermStrandsType == CStrandData::npsDirectSelection)
   {
      // first get in girderdata format
      const DirectStrandFillCollection* pDirectFillData(NULL);
      if (type==pgsTypes::Straight)
      {
         pDirectFillData = m_Segment.Strands.GetDirectStrandFillStraight();
      }
      else if (type==pgsTypes::Harped)
      {
         pDirectFillData = m_Segment.Strands.GetDirectStrandFillHarped();
      }
      if (type==pgsTypes::Temporary)
      {
         pDirectFillData = m_Segment.Strands.GetDirectStrandFillTemporary();
      }

      // Convert girderdata to config
      // Start with unfilled grid 
      ConfigStrandFillVector vec(pStrandGeometry->ComputeStrandFill(m_strGirderName.c_str(), type, 0));
      StrandIndexType gridsize = vec.size();

      if(pDirectFillData!=NULL)
      {
         DirectStrandFillCollection::const_iterator it = pDirectFillData->begin();
         DirectStrandFillCollection::const_iterator itend = pDirectFillData->end();
         while(it != itend)
         {
            StrandIndexType idx = it->permStrandGridIdx;
            if (idx<gridsize)
            {
               vec[idx] = it->numFilled;
            }
            else
               ATLASSERT(0); 

            it++;
         }
      }

      return vec;
   }
   else
   {
      // Continuous fill
      StrandIndexType Ns = m_Segment.Strands.GetNstrands(type);

      return pStrandGeometry->ComputeStrandFill(m_SegmentKey, type, Ns);
   }
}

void CGirderDescDlg::AddAdditionalPropertyPages(bool bAllowExtendedStrands,bool bIsDebonding)
{
   if ( bAllowExtendedStrands || bIsDebonding )
   {
      AddPage( &m_Debond );
   }

   AddPage(&m_LongRebar);
   AddPage( &m_Shear );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   // don't add page if both hauling and lifting checks are disabled
   if (pGirderLiftingSpecCriteria->IsLiftingAnalysisEnabled() || pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      AddPage( &m_Lifting );
   }
}

void CGirderDescDlg::OnGirderTypeChanged(bool bAllowExtendedStrands,bool bIsDebonding)
{
   // Remove all but the first two pages
   int nps = GetPageCount();
   for (int ip=nps-1; ip>1; ip--)
   {
      RemovePage(ip);
   }

   AddAdditionalPropertyPages(bAllowExtendedStrands,bIsDebonding);
   SetDebondTabName();
}
