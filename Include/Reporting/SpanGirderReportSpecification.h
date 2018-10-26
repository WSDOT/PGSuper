///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\Keys.h>
#include <PgsExt\PointOfInterest.h>

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

class REPORTINGCLASS CGirderLineReportHint : public CReportHint
{
public:
   CGirderLineReportHint();
   CGirderLineReportHint(GroupIndexType grpIdx,GirderIndexType gdrIdx);

   void SetGroupIndex(GroupIndexType grpIdx);
   GroupIndexType GetGroupIndex() const;

   void SetGirderIndex(GirderIndexType gdrIdx);
   GirderIndexType GetGirderIndex() const;

   static int IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec);

protected:
   GroupIndexType m_GroupIdx;
   GirderIndexType m_GirderIdx;
};

class REPORTINGCLASS CGirderReportHint : public CReportHint
{
public:
   CGirderReportHint();
   CGirderReportHint(const CGirderKey& girderKey,Uint32 lHint);

   void SetHint(Uint32 lHint);
   Uint32 GetHint();

   void SetGirderKey(const CGirderKey& girderKey);
   const CGirderKey& GetGirderKey() const;

   static int IsMyGirder(CReportHint* pHint,CReportSpecification* pRptSpec);

protected:
   CGirderKey m_GirderKey;
   Uint32 m_Hint; // one of GCH_xxx constants in IFace\Project.h
};

class REPORTINGCLASS CSpanReportSpecification :
   public CBrokerReportSpecification
{
public:
   CSpanReportSpecification(LPCTSTR strReportName,IBroker* pBroker,SpanIndexType spanIdx);
   CSpanReportSpecification(const CBrokerReportSpecification& other,SpanIndexType spanIdx);
   CSpanReportSpecification(const CSpanReportSpecification& other);
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
   CGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const CGirderKey& girderKey);
   CGirderReportSpecification(const CGirderReportSpecification& other);
   ~CGirderReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetGroupIndex(GroupIndexType grpIdx);
   GroupIndexType GetGroupIndex() const;

   void SetGirderIndex(GirderIndexType gdrIdx);
   GirderIndexType GetGirderIndex() const;

   void SetGirderKey(const CGirderKey& girderKey);
   const CGirderKey& GetGirderKey() const;

   virtual HRESULT Validate() const;

protected:
   CGirderKey m_GirderKey;
};


class REPORTINGCLASS CGirderLineReportSpecification :
   public CBrokerReportSpecification
{
public:
   CGirderLineReportSpecification(LPCTSTR strReportName,IBroker* pBroker,GirderIndexType gdrIdx);
   CGirderLineReportSpecification(const CGirderLineReportSpecification& other);
   ~CGirderLineReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetGirderIndex(GirderIndexType gdrIdx);
   GirderIndexType GetGirderIndex() const;

   CGirderKey GetGirderKey() const;

   virtual HRESULT Validate() const;

protected:
   GirderIndexType m_GirderIdx;
};

class REPORTINGCLASS CMultiGirderReportSpecification :
   public CBrokerReportSpecification
{
public:
   CMultiGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const std::vector<CGirderKey>& girdersKeys);
   CMultiGirderReportSpecification(const CMultiGirderReportSpecification& other);
   ~CMultiGirderReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetGirderKeys(const std::vector<CGirderKey>& girdersKeys);
   const std::vector<CGirderKey>& GetGirderKeys() const;

   bool IsMyGirder(const CGirderKey& girderKey) const;

   virtual HRESULT Validate() const;

protected:
   std::vector<CGirderKey> m_GirderKeys;
};

//////////////////////////////////////////////////////////
// CMultiViewSpanGirderReportSpecification - open a new window for each girder specified
//////////////////////////////////////////////////////////
class REPORTINGCLASS CMultiViewSpanGirderReportSpecification :
   public CBrokerReportSpecification
{
public:

   CMultiViewSpanGirderReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const std::vector<CGirderKey>& girderKeys);
   CMultiViewSpanGirderReportSpecification(const CMultiViewSpanGirderReportSpecification& other);
   ~CMultiViewSpanGirderReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetGirderKeys(const std::vector<CGirderKey>& girderKeys);
   const std::vector<CGirderKey>& GetGirderKeys() const;

   int IsMyGirder(const CGirderKey& girderKey) const;

   virtual HRESULT Validate() const;

protected:
   std::vector<CGirderKey> m_GirderKeys;
};


class REPORTINGCLASS CPointOfInterestReportSpecification :
   public CBrokerReportSpecification
{
public:
   CPointOfInterestReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const pgsPointOfInterest& poi);
   CPointOfInterestReportSpecification(const CPointOfInterestReportSpecification& other);
   ~CPointOfInterestReportSpecification(void);

   virtual std::_tstring GetReportTitle() const;

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   virtual HRESULT Validate() const;

protected:
   pgsPointOfInterest m_POI;
};
