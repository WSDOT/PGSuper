///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LiveLoadDistFactorsDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"
#include <PgsExt\BridgeDescription.h>
#include <LRFD\LRFD.h>
#include "afxwin.h"

struct SpanGirderType
{
   SpanIndexType Span;
   GirderIndexType Girder;

   SpanGirderType(SpanIndexType span, GirderIndexType girder):
      Span(span), Girder(girder)
   {;}
};

typedef std::vector<SpanGirderType> SpanGirderList;
typedef SpanGirderList::iterator SpanGirderIterator;

struct PierGirderType
{
   PierIndexType Pier;
   GirderIndexType Girder;

   PierGirderType(PierIndexType pier, GirderIndexType girder):
      Pier(pier), Girder(girder)
   {;}

};

typedef std::vector<PierGirderType> PierGirderList;
typedef PierGirderList::iterator PierGirderIterator;

// CLLDFFillDlg dialog

class CLLDFFillDlg : public CDialog
{
	DECLARE_DYNAMIC(CLLDFFillDlg)

public:
	CLLDFFillDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLLDFFillDlg();

// Functions to get data after dialog closes
   SpanGirderList GetSpanGirders();
   PierGirderList GetPierGirders();

   pgsTypes::DistributionFactorMethod GetDistributionFactorMethod();
   LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction();

// Dialog Data
	enum { IDD = IDD_LLDF_FILL };

   const CBridgeDescription* m_pBridgeDesc;
   LldfRangeOfApplicabilityAction m_LldfRangeOfApplicabilityAction;

   GirderIndexType m_MaxNumGirders; // max girders in all spans

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void ComputeMaxNumGirders();

	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMethod();

   CComboBox m_SpanCB;
   CComboBox m_GirderGirderCB;
   CComboBox m_PierCB;
   CComboBox m_PierGirderCB;
   afx_msg void OnCbnSelchangeGirderSpan();
   afx_msg void OnCbnSelchangePier();
   double m_UserInputValue;
   int m_Method;
   int m_GIRDER_SPAN_INT;
   int m_GIRDER_GIRDER_INT;
   int m_PIER_INT;
   int m_PIER_GIRDER_INT;
   int m_ROA_INT;
};
