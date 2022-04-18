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

// LiveLoadDistFactorsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "LiveLoadDistFactorsDlg.h"

#include "LLDFFillDlg.h"

#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFDocument.h>

#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// functions for ordering distribution factor method
inline int GetIntForDfMethod(pgsTypes::DistributionFactorMethod method)
{
   if (method==pgsTypes::Calculated)
   {
      return 0;
   }
   else if (method==pgsTypes::DirectlyInput)
   {
      return 2;
   }
   else if (method==pgsTypes::LeverRule)
   {
      return 1;
   }
   else
   {
      ATLASSERT(false);
      return 0;
   }
}

inline pgsTypes::DistributionFactorMethod GetDfMethodForInt(int method)
{
   if (method==0)
   {
      return pgsTypes::Calculated;
   }
   else if (method==2)
   {
      return pgsTypes::DirectlyInput;
   }
   else if (method==1)
   {
      return pgsTypes::LeverRule;
   }
   else
   {
      ATLASSERT(false);
      return pgsTypes::Calculated;
   }
}


/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDistFactorsDlg dialog

CLiveLoadDistFactorsDlg::CLiveLoadDistFactorsDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CLiveLoadDistFactorsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLiveLoadDistFactorsDlg)
	//}}AFX_DATA_INIT
}

CLiveLoadDistFactorsDlg::~CLiveLoadDistFactorsDlg()
{
}


void CLiveLoadDistFactorsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLiveLoadDistFactorsDlg)
	//}}AFX_DATA_MAP

   // Translate df calculation method and action to get correct ordering
   int action, method;
   if ( !pDX->m_bSaveAndValidate )
   {
      action = GetIntForLldfAction(m_LldfRangeOfApplicabilityAction);
      method = GetIntForDfMethod(m_BridgeDesc.GetDistributionFactorMethod());
   }

   DDX_CBIndex(pDX, IDC_ROA_CB, action );
   DDX_Radio(pDX, IDC_LLDF_COMPUTE, method );

   pgsTypes::DistributionFactorMethod lldf_method = GetDfMethodForInt(method);

   if ( pDX->m_bSaveAndValidate )
   {
      m_LldfRangeOfApplicabilityAction = GetLldfActionForInt(action);

      m_BridgeDesc.SetDistributionFactorMethod(lldf_method);
   }

   SpanIndexType nspans = m_BridgeDesc.GetSpanCount();

   if ( pDX->m_bSaveAndValidate)
   {
      // only save user-input data if dialog closes using user input setting
      if ( lldf_method==pgsTypes::DirectlyInput )
      {
         for (GirderGridIterator it=m_GirderGrids.begin(); it!=m_GirderGrids.end(); it++)
         {
            it->get()->GetData(&m_BridgeDesc);
         }

         for (PierGridIterator it=m_PierGrids.begin(); it!=m_PierGrids.end(); it++)
         {
            it->get()->GetData(&m_BridgeDesc);
         }
      }
   }
   else
   {
      for (GirderGridIterator it=m_GirderGrids.begin(); it!=m_GirderGrids.end(); it++)
      {
         it->get()->FillGrid(&m_BridgeDesc);
      }

      for (PierGridIterator it=m_PierGrids.begin(); it!=m_PierGrids.end(); it++)
      {
         it->get()->FillGrid(&m_BridgeDesc);
      }
   }

	// validation routine for CGXTabWnd controls
	DDV_GXTabWnd(pDX, &m_LldfTabWnd);
}
 

BEGIN_MESSAGE_MAP(CLiveLoadDistFactorsDlg, CDialog)
	//{{AFX_MSG_MAP(CLiveLoadDistFactorsDlg)
	ON_BN_CLICKED(IDC_LLDF_COMPUTE, OnMethod)
	ON_BN_CLICKED(IDC_LLDF_INPUT, OnMethod)
	ON_BN_CLICKED(IDC_LLDF_LEVER_RULE, OnMethod)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_WM_NCACTIVATE()
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_LLDF_FILL2, &CLiveLoadDistFactorsDlg::OnBnClickedLldfFillButton)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDistFactorsDlg message handlers

BOOL CLiveLoadDistFactorsDlg::OnInitDialog() 
{
   m_LldfTabWnd.SubclassDlgItem(IDC_LLDF_TABW, this);

   GET_IFACE(IBridge,pBridge);

   // Init Girder grids
   SpanIndexType nspans = m_BridgeDesc.GetSpanCount();
   for (SpanIndexType ispan=0; ispan<nspans; ispan++)
   {
      // Create pier tab and grid
      CLLDFPierGrid* ppiergrid = new CLLDFPierGrid();

	   ppiergrid->Create(0, CRect(0,0,1,1), &m_LldfTabWnd, m_LldfTabWnd.GetNextID());

      CString pier_name;
      pier_name.Format(_T("%s"), LABEL_PIER_EX(ispan==0, ispan));
   	m_LldfTabWnd.AttachWnd(ppiergrid, pier_name);

      ppiergrid->CustomInit(ispan);

      m_PierGrids.push_back( std::shared_ptr<CLLDFPierGrid>(ppiergrid) );

      // Create span tab and grid
      CLLDFGrid* pgrid = new CLLDFGrid();

	   pgrid->Create(0, CRect(0,0,1,1), &m_LldfTabWnd, m_LldfTabWnd.GetNextID());

      CString span_name;
      span_name.Format(_T("Span %s"), LABEL_SPAN(ispan));
   	m_LldfTabWnd.AttachWnd(pgrid, span_name);

      bool bNegMoments = pBridge->ProcessNegativeMoments(ispan);

      pgrid->CustomInit(ispan,bNegMoments);

      m_GirderGrids.push_back( std::shared_ptr<CLLDFGrid>(pgrid) );
   }

   // Init end pier tab and grid
   CLLDFPierGrid* pgrid = new CLLDFPierGrid();

	pgrid->Create(0, CRect(0,0,1,1), &m_LldfTabWnd, m_LldfTabWnd.GetNextID());

   CString pier_name;
   pier_name.Format(_T("%s"), LABEL_PIER_EX(true,nspans));
   m_LldfTabWnd.AttachWnd(pgrid, pier_name);

   pgrid->CustomInit(nspans);

   m_PierGrids.push_back( std::shared_ptr<CLLDFPierGrid>(pgrid) );


	CDialog::OnInitDialog();

   // Deal with scroll bars
   // Max grid height will be the same for both piers and girders
   int growhgt(0);
   for (GirderGridIterator ig=m_GirderGrids.begin(); ig!=m_GirderGrids.end(); ig++)
   {
      ROWCOL nrows = ig->get()->GetRowCount();
      growhgt = Max(growhgt, ig->get()->CalcSumOfRowHeights(0,nrows-1)); 
   }

   m_LldfTabWnd.ShowScrollBar(SB_HORZ, FALSE);

   CRect trect;
   m_LldfTabWnd.GetInsideRect(trect);

   if (growhgt < trect.Height())
   {
      m_LldfTabWnd.ShowScrollBar(SB_VERT, FALSE);
   }

   m_LldfTabWnd.SwitchTab(1); // set to span 1
	m_LldfTabWnd.SetFocus();

   OnMethod();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLiveLoadDistFactorsDlg::OnMethod() 
{
   DealWithGridStates();

   BOOL bEnable = IsDlgButtonChecked(IDC_LLDF_COMPUTE);

   GetDlgItem(IDC_ROA_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_ROA_CB)->EnableWindow(bEnable);
}

void CLiveLoadDistFactorsDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DISTRIBUTION_FACTORS );
}

BOOL CLiveLoadDistFactorsDlg::OnNcActivate(BOOL bActive)
{
	if (GXDiscardNcActivate())
		return TRUE;

	return CDialog::OnNcActivate(bActive);
}

void CLiveLoadDistFactorsDlg::DealWithGridStates()
{
	BOOL bEnable = IsDlgButtonChecked(IDC_LLDF_INPUT);

   GetDlgItem(IDC_LLDF_FILL2)->EnableWindow(bEnable);
   GetDlgItem(IDC_USER_LLDF_NOTE)->EnableWindow(bEnable);

   m_LldfTabWnd.ShowWindow((SW_SHOW));

   CEnableGrid* pGrid = (CEnableGrid*)m_LldfTabWnd.GetActivePane();
   pGrid->Enable(bEnable);
   m_LldfTabWnd.GetBeam().EnableWindow(bEnable);
}

void CLiveLoadDistFactorsDlg::OnBnClickedLldfFillButton()
{
   CLLDFFillDlg dlg;
   dlg.m_LldfRangeOfApplicabilityAction = m_LldfRangeOfApplicabilityAction;
   dlg.m_pBridgeDesc = &m_BridgeDesc;
   if ( dlg.DoModal() == IDOK )
   {
      // dialog returns list of girders to be modified
      SpanKeyList spans = dlg.GetSpanKeys();
      PierGirderList piers = dlg.GetPierGirders();

      pgsTypes::DistributionFactorMethod method = dlg.GetDistributionFactorMethod();
      if (method==pgsTypes::DirectlyInput)
      {
         // Direct input - girders first
         for (SpanKeyIterator sit=spans.begin(); sit!=spans.end(); sit++)
         {
            const CSpanKey& spanKey = *sit;
            CLLDFGrid* pGrid = m_GirderGrids[spanKey.spanIndex].get();
            pGrid->SetGirderLLDF( spanKey.girderIndex, dlg.m_UserInputValue );
         }

         // Piers
         for (PierGirderIterator sit=piers.begin(); sit!=piers.end(); sit++)
         {
            const PierGirderType& sg = *sit;
            CLLDFPierGrid* pGrid = m_PierGrids[sg.Pier].get();
            pGrid->SetGirderLLDF( sg.Girder, dlg.m_UserInputValue );
         }
      }
      else
      {
         // Computed either by lever rule or LRFD
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
         GET_IFACE_NOCHECK(ILiveLoads,pLiveLoads);
         GET_IFACE(IProgress,pProgress);
         GET_IFACE(IEvents, pEvents);

         pEvents->HoldEvents(); // don't fire any changed events until all changes are done

         // This can take some time.
         CEAFAutoProgress ap(pProgress,0);
         pProgress->UpdateMessage(_T("Computing Distribution Factors..."));

         // save old method
         pgsTypes::DistributionFactorMethod old_method = pIBridgeDesc->GetLiveLoadDistributionFactorMethod();
         LldfRangeOfApplicabilityAction action, old_action;
         
         action = dlg.GetLldfRangeOfApplicabilityAction();

         try
         {
            pIBridgeDesc->SetLiveLoadDistributionFactorMethod(method);

            if (method==pgsTypes::Calculated)
            {
               old_action = pLiveLoads->GetLldfRangeOfApplicabilityAction();
               pLiveLoads->SetLldfRangeOfApplicabilityAction(action);
            }

            pEvents->FirePendingEvents();

            // First store all factors locally, then set them if we don't throw
            std::vector<SpanLLDF> span_lldfs;

            // Girders first
            for (SpanKeyIterator sit=spans.begin(); sit!=spans.end(); sit++)
            {
               const CSpanKey& spanKey = *sit;
               const CSpanData2* pSpan = m_BridgeDesc.GetSpan(spanKey.spanIndex);
               const CGirderGroupData* pGroup = m_BridgeDesc.GetGirderGroup(pSpan);
               GirderIndexType nGirders = pGroup->GetGirderCount();
               if(spanKey.girderIndex < nGirders)
               {
                  SpanLLDF lldf;
                  lldf.sgNMService= pLLDF->GetNegMomentDistFactor(spanKey, pgsTypes::ServiceI );
                  lldf.sgPMService= pLLDF->GetMomentDistFactor(spanKey, pgsTypes::ServiceI );
                  lldf.sgVService = pLLDF->GetShearDistFactor(spanKey, pgsTypes::ServiceI );
                  lldf.sgNMFatigue= pLLDF->GetNegMomentDistFactor(spanKey, pgsTypes::FatigueI );
                  lldf.sgPMFatigue= pLLDF->GetMomentDistFactor(spanKey, pgsTypes::FatigueI );
                  lldf.sgVFatigue = pLLDF->GetShearDistFactor(spanKey, pgsTypes::FatigueI );

                  span_lldfs.push_back(lldf);
               }
            }

            // Piers
            GirderIndexType npiers = m_BridgeDesc.GetPierCount();

            std::vector<PierLLDF> pier_lldfs;

            for (PierGirderIterator sit=piers.begin(); sit!=piers.end(); sit++)
            {
               const PierGirderType& pg = *sit;
               const CPierData2* pPier = m_BridgeDesc.GetPier(pg.Pier);

               // User-defined pier lldf's have no concept of ahead and back spans, so
               // use max distribution factors from ahead and back spans
               PierLLDF lldf_Back, lldf_Ahead;

               const CSpanData2* pSpanBack  = pPier->GetSpan(pgsTypes::Back);
               GirderIndexType nGirdersBack = INVALID_INDEX;
               if ( pSpanBack )
               {
                  const CGirderGroupData* pGroup = m_BridgeDesc.GetGirderGroup(pSpanBack);
                  nGirdersBack = pGroup->GetGirderCount();

               }

               const CSpanData2* pSpanAhead = pPier->GetSpan(pgsTypes::Ahead);
               GirderIndexType nGirdersAhead = INVALID_INDEX;
               if ( pSpanAhead )
               {
                  const CGirderGroupData* pGroup = m_BridgeDesc.GetGirderGroup(pSpanAhead);
                  nGirdersAhead = pGroup->GetGirderCount();
               }

               if( pSpanBack != nullptr && pg.Girder < nGirdersBack)
               {
                  lldf_Back.pgNMService= pLLDF->GetNegMomentDistFactorAtPier(pg.Pier, pg.Girder, pgsTypes::ServiceI,pgsTypes::Back);
                  lldf_Back.pgNMFatigue= pLLDF->GetNegMomentDistFactorAtPier(pg.Pier, pg.Girder, pgsTypes::FatigueI,pgsTypes::Back);
               }

               if( pSpanAhead != nullptr && pg.Girder < nGirdersAhead)
               {
                  lldf_Ahead.pgNMService= pLLDF->GetNegMomentDistFactorAtPier(pg.Pier, pg.Girder, pgsTypes::ServiceI,pgsTypes::Ahead);
                  lldf_Ahead.pgNMFatigue= pLLDF->GetNegMomentDistFactorAtPier(pg.Pier, pg.Girder, pgsTypes::FatigueI,pgsTypes::Ahead);
               }

               PierLLDF lldf;
               lldf.pgNMService= Max(lldf_Back.pgNMService, lldf_Ahead.pgNMService);
               lldf.pgNMFatigue= Max(lldf_Back.pgNMFatigue, lldf_Ahead.pgNMFatigue);

               pier_lldfs.push_back(lldf);

            }

            // Now that we've safely computed values, put them into the grids.
            int icnt=0;
            // Girders
            for (SpanKeyIterator sit=spans.begin(); sit!=spans.end(); sit++)
            {
               const CSpanKey& spanKey = *sit;
               CLLDFGrid* pGrid = m_GirderGrids[spanKey.spanIndex].get();
               pGrid->SetGirderLLDF( spanKey.girderIndex, span_lldfs[icnt] );

               icnt++;
            }

            // Piers
            icnt = 0;
            for (PierGirderIterator sit=piers.begin(); sit!=piers.end(); sit++)
            {
               const PierGirderType& sg = *sit;
               CLLDFPierGrid* pGrid = m_PierGrids[sg.Pier].get();
               pGrid->SetGirderLLDF( sg.Girder, pier_lldfs[icnt] );

               icnt++;
            }
         }
         catch(const CXUnwind* pe)
         {
            std::_tstring errmsg;
            pe->GetErrorMessage(&errmsg);

            std::_tostringstream os;
            os << errmsg << std::endl;
            os << _T("Grid input values will not be updated.") << std::endl;
            ::AfxMessageBox(os.str().c_str(), MB_OK);
         }
         catch(...)
         {
            ATLASSERT(false); // shouldn't get here
            ::AfxMessageBox(_T("An unknown error occurred while computing distribution factors. Grid input values will not be updated."), MB_OK);
         }

         // Restore original lldf computation method
         pEvents->HoldEvents();

         pIBridgeDesc->SetLiveLoadDistributionFactorMethod(old_method);
         if (method==pgsTypes::Calculated)
         {
            pLiveLoads->SetLldfRangeOfApplicabilityAction(old_action);
         }

         pEvents->FirePendingEvents(); // we really didn't make any changes
      }
   }
}
