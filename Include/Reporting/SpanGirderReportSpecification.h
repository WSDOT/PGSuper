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

#pragma once
#include <Reporting\ReportingExp.h>
#include <Reporting\BrokerReportSpecification.h>
#include <ReportManager\ReportManager.h>
#include <ReportManager\ReportHint.h>
#include <WBFLCore.h>

class REPORTINGCLASS CSpanReportHint : public CReportHint
{
public:
   CSpanReportHint();
   CSpanReportHint(SpanIndexType spanIdx);

   void SetSpan(SpanIndexType spanIdx);
   SpanIndexType GetSpan();

   static int IsMySpan(CReportHint* pHint,CReportSpecification* pRptSpec);

protected:
   SpanIndexType m_SpanIdx;
};

class REPORTINGCLASS CGirderReportHint : public CReportHint
{
public:
   CGirderReportHint();
   CGirderReportHint(GirderIndexType gdrIdx);

   void SetGirder(GirderIndexType gdrIdx);
   GirderIndexType GetGirder();

   static int IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec);

protected:
   GirderIndexType m_GdrIdx;
};

class REPORTINGCLASS CSpanGirderReportHint : public CReportHint
{
public:
   CSpanGirderReportHint();
   CSpanGirderReportHint(SpanIndexType spanIdx,GirderIndexType gdrIdx,Uint32 lHint);

   void SetHint(Uint32 lHint);
   Uint32 GetHint();

   void SetGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   void GetGirder(SpanIndexType& spanIdx,GirderIndexType& gdrIdx);

   static int IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec);

protected:
   SpanIndexType m_SpanIdx;
   GirderIndexType m_GdrIdx;
   Uint32 m_Hint; // one of GCH_xxx constants in IFace\Project.h
};

class REPORTINGCLASS CSpanReportSpecification :
   public CBrokerReportSpecification
{
public:
   CSpanReportSpecification(LPCTSTR strReportName,IBroker* pBroker,SpanIndexType spanIdx);
   ~CSpanReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetSpan(SpanIndexType spanIdx);
   SpanIndexType GetSpan() const;

   virtual HRESULT Validate() const;

protected:
   SpanIndexType m_Span;
};


class REPORTINGCLASS CGirderReportSpecification :
   public CBrokerReportSpecification
{
public:
   CGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,GirderIndexType gdrIdx);
   ~CGirderReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetGirder(GirderIndexType gdrIdx);
   GirderIndexType GetGirder() const;

   virtual HRESULT Validate() const;

protected:
   GirderIndexType m_Girder;
};


class REPORTINGCLASS CSpanGirderReportSpecification :
   public CSpanReportSpecification
{
public:
   CSpanGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx);
   ~CSpanGirderReportSpecification(void);
   
   virtual std::_tstring GetReportTitle() const;

   void SetGirder(GirderIndexType gdrIdx);
   GirderIndexType GetGirder() const;

   virtual HRESULT Validate() const;

protected:
   GirderIndexType m_Girder;
};

class REPORTINGCLASS CMultiGirderReportSpecification :
   public CBrokerReportSpecification
{
public:

   CMultiGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const std::vector<SpanGirderHashType>& girderlist);
   ~CMultiGirderReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetGirderList(const std::vector<SpanGirderHashType>& girderlist);
   std::vector<SpanGirderHashType> GetGirderList() const;

   int IsMyGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx) const;

   virtual HRESULT Validate() const;

protected:
   std::vector<SpanGirderHashType> m_GirderList;
};
