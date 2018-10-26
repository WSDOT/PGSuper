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

#if !defined(AFX_ANALYSISRESULTSVIEW_H__E2B376C7_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_ANALYSISRESULTSVIEW_H__E2B376C7_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AnalysisResultsView.h : header file
//
#include <EAF\EAFAutoCalcView.h>

#if !defined INCLUDED_GRAPHICSLIB_GRAPHXY_H_
#include <GraphicsLib\GraphXY.h>
#endif

#include "AnalysisResultsChildFrame.h"
#include <PgsExt\PointOfInterest.h>

interface IBroker;
class arvPhysicalConverter;

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsView view

class CAnalysisResultsView : public CView,
                             public CEAFAutoCalcViewMixin
{
protected:
	CAnalysisResultsView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAnalysisResultsView)

// Attributes
public:

// Operations
public:
   virtual bool DoResultsExist();
   virtual void UpdateFromBar(); // user interacted with bar
   virtual void UpdateNow();
   void UpdateGrid();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnalysisResultsView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
   virtual void OnInitialUpdate();
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
protected:

	virtual ~CAnalysisResultsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CAnalysisResultsView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
   afx_msg void DumpLBAM();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void UpdateUnits(ActionType action);
   void UpdateXAxisTitle(pgsTypes::Stage stage);
   void UpdateGraphTitle(SpanIndexType span,GirderIndexType girder,pgsTypes::Stage stage,ActionType action);

   void CombinedLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPOI,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void LiveLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPOI,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void VehicularLiveLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPOI,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void LimitStateLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPOI,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void ProductLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPOI,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void PrestressLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPOI,const std::vector<Float64>& xVals);
   void CyStressCapacityGraph(int graphIdx, SpanIndexType span,GirderIndexType girder);

   void InitializeGraph(int graphIdx,ActionType action,pgsTypes::Stage stage,bool bIsFinalShear,IndexType* pDataSeriesID,BridgeAnalysisType* pBAT,Uint16* pAnalysisTypeCount);
   void AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<Float64>& yvals);
   void AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<sysSectionValue>& yvals);
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
	void DrawBeam(CDC* pDC);
   void Update();
   void DoUpdateNow();

   void DrawSupport(pgsTypes::Stage stage,pgsTypes::PierConnectionType supportType,CPoint p,CDC* pDC);
   void DrawRoller(CPoint p,CDC* pDC);
   void DrawHinge(CPoint p,CDC* pDC);
   void DrawContinuous(CPoint p,CDC* pDC);
   void DrawIntegral(CPoint p,CDC* pDC);
   void DrawIntegralHingeBack(CPoint p,CDC* pDC);
   void DrawIntegralHingeAhead(CPoint p,CDC* pDC);

   CAnalysisResultsChildFrame* m_pFrame;

   bool m_bValidGraph;
   bool m_bUpdateError;
   std::_tstring m_ErrorMsg;

   CSize m_SupportSize;

   IBroker* m_pBroker;

   arvPhysicalConverter* m_pXFormat;
   arvPhysicalConverter* m_pYFormat;

   grGraphXY m_Graph;
   Float64   m_LeftEnd; // X value of left-most poi
   CRect m_PrintRect;
   bool m_IsPrinting;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANALYSISRESULTSVIEW_H__E2B376C7_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
