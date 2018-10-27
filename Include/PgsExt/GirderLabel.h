///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <PgsExt\PgsExtExp.h>

#include <string>

class CPierData2;
class CTemporarySupportData;
interface IEAFDisplayUnits;

#include <PsgLib\ConnectionLibraryEntry.h>
#include <PgsExt\ConcreteMaterial.h>
#include <PgsExt\Keys.h>

#define LABEL_GIRDER(_g_) pgsGirderLabel::GetGirderLabel(_g_).c_str()
#define LABEL_INDEX(_i_) (IndexType)((_i_)+1)
#define LABEL_GROUP(_g_) (GroupIndexType)LABEL_INDEX(_g_)
#define LABEL_SPAN(_s_) (SpanIndexType)LABEL_INDEX(_s_)
#define LABEL_PIER(_p_) (PierIndexType)LABEL_INDEX(_p_)
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

class PGSEXTCLASS pgsGirderLabel
{
public:
   static std::_tstring GetGirderLabel(GirderIndexType gdrIdx);
   static std::_tstring GetGirderLabel(const CGirderKey& girderKey);
   static std::_tstring GetSegmentLabel(const CSegmentKey& segmentKey);
   static std::_tstring GetClosureLabel(const CClosureKey& closureKey);
   static bool UseAlphaLabel();
   static bool UseAlphaLabel(bool bUseAlpha);

private:
   static bool ms_bUseAlpha;
   pgsGirderLabel(void);
   ~pgsGirderLabel(void);
};

class PGSEXTCLASS pgsAutoLabel
{
public:
   pgsAutoLabel() { m_bOldSetting = pgsGirderLabel::UseAlphaLabel(false); }
   ~pgsAutoLabel() { pgsGirderLabel::UseAlphaLabel(m_bOldSetting); }
private:
   bool m_bOldSetting;
};

LPCTSTR PGSEXTFUNC GetEndDistanceMeasureString(ConnectionLibraryEntry::EndDistanceMeasurementType type,bool bAbutment,bool bAbbreviation);
LPCTSTR PGSEXTFUNC GetBearingOffsetMeasureString(ConnectionLibraryEntry::BearingOffsetMeasurementType type,bool bAbutment,bool bAbbreviation);

CString PGSEXTFUNC GetLabel(const CPierData2* pPier,IEAFDisplayUnits* pDisplayUnits);
CString PGSEXTFUNC GetLabel(const CTemporarySupportData* pTS,IEAFDisplayUnits* pDisplayUnits);

CString PGSEXTFUNC ConcreteDescription(const CConcreteMaterial& concrete);

LPCTSTR PGSEXTFUNC GetLimitStateString(pgsTypes::LimitState limitState);
LPCTSTR PGSEXTFUNC GetStressLocationString(pgsTypes::StressLocation loc);
LPCTSTR PGSEXTFUNC GetStressTypeString(pgsTypes::StressType type);
