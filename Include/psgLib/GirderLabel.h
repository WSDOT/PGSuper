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

#include "PsgLibLib.h"
#include <PsgLib\ConcreteMaterial.h>
#include <PsgLib\Keys.h>
#include <psgLib/ConnectionLibraryEntry.h>
#include <string>

class CPierData2;
class CTemporarySupportData;
class CBridgeDescription2;
class CPointLoadData;
class CDistributedLoadData;
class CMomentLoadData;
class IEAFDisplayUnits;


#define LABEL_GIRDER(_g_) pgsGirderLabel::GetGirderLabel(_g_).c_str()
#define LABEL_INDEX(_i_) (IndexType)((_i_)+1)
#define LABEL_GROUP(_g_) (GroupIndexType)LABEL_INDEX(_g_)
#define LABEL_SPAN(_s_) pgsPierLabel::GetPierLabel(_s_).c_str()
#define LABEL_PIER(_p_) pgsPierLabel::GetPierLabel(_p_).c_str()
#define LABEL_PIER_EX(_ia_,_p_) pgsPierLabel::GetPierLabelEx(_ia_, _p_).c_str()
#define LABEL_TEMPORARY_SUPPORT(_ts_) (SupportIndexType)LABEL_INDEX(_ts_ )
#define LABEL_SEGMENT(_s_) (SegmentIndexType)LABEL_INDEX(_s_)
#define LABEL_EVENT(_e_) (EventIndexType)LABEL_INDEX(_e_)
#define LABEL_STIRRUP_ZONE(_z_) (ZoneIndexType)LABEL_INDEX(_z_)
#define LABEL_DUCT(_d_) (DuctIndexType)LABEL_INDEX(_d_)
#define LABEL_INTERVAL(_i_) (IntervalIndexType)LABEL_INDEX(_i_)
#define LABEL_ROW(_r_) (RowIndexType)LABEL_INDEX(_r_)
#define LABEL_COLUMN(_c_) (ColumnIndexType)LABEL_INDEX(_c_)

#define GIRDER_LABEL(_g_) pgsGirderLabel::GetGirderLabel(_g_).c_str()
#define SEGMENT_LABEL(_s_) pgsGirderLabel::GetSegmentLabel(_s_).c_str()
#define CLOSURE_LABEL(_c_) pgsGirderLabel::GetClosureLabel(_c_).c_str()

// Return string describing type of harped strands
inline LPCTSTR LABEL_HARP_TYPE(bool bAreHarpedStraight) { return bAreHarpedStraight ? _T("Adjustable Straight") : _T("Harped"); }
inline LPCTSTR ABR_LABEL_HARP_TYPE(bool bAreHarpedStraight) { return bAreHarpedStraight ? _T("Adj. Straight") : _T("Harped"); }

class PSGLIBCLASS pgsGirderLabel
{
public:
   static std::_tstring GetGirderLabel(GirderIndexType gdrIdx);
   // Can force "Span" regardless of document type if desired
   static std::_tstring GetGirderLabel(const CGirderKey& girderKey, bool forceSpan=false);
   static std::_tstring GetSegmentLabel(const CSegmentKey& segmentKey, bool forceSpan=false);
   static std::_tstring GetGroupLabel(GroupIndexType grpIdx, bool forceSpan=false);
   static std::_tstring GetClosureLabel(const CClosureKey& closureKey);
   static bool UseAlphaLabel();
   static bool UseAlphaLabel(bool bUseAlpha);

private:
   static bool ms_bUseAlpha;
   pgsGirderLabel(void);
   ~pgsGirderLabel(void);
};

class PSGLIBCLASS pgsAutoGirderLabel
{
public:
   pgsAutoGirderLabel() { m_bOldSetting = pgsGirderLabel::UseAlphaLabel(false); }
   ~pgsAutoGirderLabel() { pgsGirderLabel::UseAlphaLabel(m_bOldSetting); }
private:
   bool m_bOldSetting;
};

// Class used for pier and span labelling. 
// settings are saved in static members (not BridgeAgent) for performance since they are called millions of times potentially 
class PSGLIBCLASS pgsAutoPierLabel;

class PSGLIBCLASS pgsPierLabel
{
   friend pgsAutoPierLabel;

public:
   // numeric label only
   static std::_tstring GetPierLabel(PierIndexType pierIdx);
   // text and numeric label (i.e., "Pier 3")
   static std::_tstring GetPierLabelEx(bool bIsAbutment, PierIndexType pierIdx);
   static std::_tstring GetPierTypeLabelEx(bool bIsAbutment, PierIndexType pierIdx);

   static void SetPierLabelSettings(pgsTypes::DisplayEndSupportType displayStartSupportType, pgsTypes::DisplayEndSupportType displayEndSupportType, PierIndexType startingPierNumber);

   // utility function to create label independently of static data values
   static std::_tstring CreatePierLabel(bool bIsAbutment, PierIndexType pierIdx, pgsTypes::DisplayEndSupportType displayStartSupportType, pgsTypes::DisplayEndSupportType displayEndSupportType, PierIndexType startingPierNumber);

   // create label based on current CBridgeDescription2
   static std::_tstring CreatePierLabel(const CBridgeDescription2& bridgeDescr, PierIndexType pierIdx);

private:
   static pgsTypes::DisplayEndSupportType m_DisplayStartSupportType;
   static pgsTypes::DisplayEndSupportType m_DisplayEndSupportType;
   static PierIndexType m_StartingPierNumber;

   pgsPierLabel(void);
   ~pgsPierLabel(void);
};

class PSGLIBCLASS pgsAutoPierLabel
{
public:
   pgsAutoPierLabel() : m_OldDisplayStartSupportType(pgsPierLabel::m_DisplayStartSupportType), m_OldDisplayEndSupportType(pgsPierLabel::m_DisplayEndSupportType),m_OldStartingPierNumber(pgsPierLabel::m_StartingPierNumber)
   {;}

   ~pgsAutoPierLabel() 
   {
      pgsPierLabel::m_DisplayStartSupportType = m_OldDisplayStartSupportType;
      pgsPierLabel::m_DisplayEndSupportType = m_OldDisplayEndSupportType;
      pgsPierLabel::m_StartingPierNumber = m_OldStartingPierNumber;
   }

private:
   pgsTypes::DisplayEndSupportType m_OldDisplayStartSupportType;
   pgsTypes::DisplayEndSupportType m_OldDisplayEndSupportType;
   PierIndexType m_OldStartingPierNumber;
};

// At piers
LPCTSTR PSGLIBFUNC GetEndDistanceMeasureString(ConnectionLibraryEntry::EndDistanceMeasurementType type,bool bAbutment,bool bAbbreviation);
LPCTSTR PSGLIBFUNC GetBearingOffsetMeasureString(ConnectionLibraryEntry::BearingOffsetMeasurementType type,bool bAbutment,bool bAbbreviation);

// At temporary supports
LPCTSTR PSGLIBFUNC GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::EndDistanceMeasurementType type,bool bAbbreviation);
LPCTSTR PSGLIBFUNC GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::BearingOffsetMeasurementType type,bool bAbbreviation);

CString PSGLIBFUNC GetLabel(const CPierData2* pPier,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
CString PSGLIBFUNC GetLabel(const CTemporarySupportData* pTS,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

CString PSGLIBFUNC GetLoadDescription(const CPointLoadData* pLoad);
CString PSGLIBFUNC GetLoadDescription(const CDistributedLoadData* pLoad);
CString PSGLIBFUNC GetLoadDescription(const CMomentLoadData* pLoad);

CString PSGLIBFUNC ConcreteDescription(const CConcreteMaterial& concrete);

LPCTSTR PSGLIBFUNC GetLimitStateString(pgsTypes::LimitState limitState);
LPCTSTR PSGLIBFUNC GetStressLocationString(pgsTypes::StressLocation loc);
LPCTSTR PSGLIBFUNC GetStressTypeString(pgsTypes::StressType type);
