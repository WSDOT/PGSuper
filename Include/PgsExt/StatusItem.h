///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// SYSTEM INCLUDES
//
#include <PgsExt\PgsExtExp.h>
#include <PsgLib\Keys.h>
#include <PgsExt\SpanGirderRelatedStatusItem.h>
#include <PgsExt\SegmentRelatedStatusItem.h>


// status for refined analysis
class PGSEXTCLASS pgsRefinedAnalysisStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsRefinedAnalysisStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

///////////////////////////
class PGSEXTCLASS pgsRefinedAnalysisStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsRefinedAnalysisStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};

// status for install error
class PGSEXTCLASS pgsInstallationErrorStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsInstallationErrorStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strComponent,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
   std::_tstring m_Component;
};

///////////////////////////
class PGSEXTCLASS pgsInstallationErrorStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsInstallationErrorStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};

// status for unknown error
class PGSEXTCLASS pgsUnknownErrorStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsUnknownErrorStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR file,long line,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
   std::_tstring m_File;
   long m_Line;
};

///////////////////////////
class PGSEXTCLASS pgsUnknownErrorStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsUnknownErrorStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};

// status informational message
class PGSEXTCLASS pgsInformationalStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsInformationalStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   ~pgsInformationalStatusItem();
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;

};

class PGSEXTCLASS pgsInformationalStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsInformationalStatusCallback(WBFL::EAF::StatusSeverityType severity,UINT helpID=0);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
   UINT m_HelpID;
};

/////////////////////////
class PGSEXTCLASS pgsProjectCriteriaStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsProjectCriteriaStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

class PGSEXTCLASS pgsProjectCriteriaStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsProjectCriteriaStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   UINT m_HelpID;
};

// status for girder input
class PGSEXTCLASS pgsGirderDescriptionStatusItem : public pgsSegmentRelatedStatusItem
{
public:
   pgsGirderDescriptionStatusItem(const CSegmentKey& segmentKey,Uint16 page,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;

   CSegmentKey m_SegmentKey;
   Uint16 m_Page; // page of girder input wizard
};


///////////////////////////
class PGSEXTCLASS pgsGirderDescriptionStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsGirderDescriptionStatusCallback(WBFL::EAF::StatusSeverityType severity);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
};

// status for structural analysis type
class PGSEXTCLASS pgsStructuralAnalysisTypeStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsStructuralAnalysisTypeStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

class PGSEXTCLASS pgsStructuralAnalysisTypeStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsStructuralAnalysisTypeStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};

// status for general bridge description input
class PGSEXTCLASS pgsBridgeDescriptionStatusItem : public WBFL::EAF::StatusItem
{
public:
   typedef enum IssueType { General, Framing, Railing, Deck, BoundaryConditions, Bearings, DeckCasting } IssueType;
   pgsBridgeDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,IssueType issueType,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;

   IssueType m_IssueType;
};

///////////////////////////
class PGSEXTCLASS pgsBridgeDescriptionStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsBridgeDescriptionStatusCallback(WBFL::EAF::StatusSeverityType severity);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
};

// status for distribution factor warnings
class PGSEXTCLASS pgsLldfWarningStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsLldfWarningStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

///////////////////////////
class PGSEXTCLASS pgsLldfWarningStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsLldfWarningStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
};

// status for effective flange width warnings
class PGSEXTCLASS pgsEffectiveFlangeWidthStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsEffectiveFlangeWidthStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

///////////////////////////
class PGSEXTCLASS pgsEffectiveFlangeWidthStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsEffectiveFlangeWidthStatusCallback(WBFL::EAF::StatusSeverityType severity);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
};


// status for timeline input
class PGSEXTCLASS pgsTimelineStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsTimelineStatusItem(StatusGroupIDType statusGroupID, StatusCallbackIDType callbackID, LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};


///////////////////////////
class PGSEXTCLASS pgsTimelineStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsTimelineStatusCallback(WBFL::EAF::StatusSeverityType severity);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
};


///////////////////////////
// status for connection geometry
class PGSEXTCLASS pgsConnectionGeometryStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsConnectionGeometryStatusItem(StatusGroupIDType statusGroupID, StatusCallbackIDType callbackID, PierIndexType pierIdx, LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;

   PierIndexType m_PierIdx;
};


class PGSEXTCLASS pgsConnectionGeometryStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsConnectionGeometryStatusCallback(WBFL::EAF::StatusSeverityType severity = WBFL::EAF::StatusSeverityType::Warning);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
};
