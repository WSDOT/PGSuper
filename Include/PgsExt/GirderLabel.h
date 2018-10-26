///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#define LABEL_GIRDER(_g_) pgsGirderLabel::GetGirderLabel(_g_).c_str()
#define LABEL_GROUP(_g_) (GroupIndexType)(_g_ + 1)
#define LABEL_SPAN(_s_) (SpanIndexType)(_s_ + 1)
#define LABEL_PIER(_p_) (PierIndexType)(_p_ + 1)
#define LABEL_TEMPORARY_SUPPORT(_ts_) (SupportIndexType)(_ts_ + 1)
#define LABEL_SEGMENT(_s_) (SegmentIndexType)(_s_ + 1)
#define LABEL_EVENT(_e_) (EventIndexType)(_e_+1)
#define LABEL_STIRRUP_ZONE(_z_) (ZoneIndexType)(_z_ + 1)
#define LABEL_DUCT(_d_) (DuctIndexType)(_d_ + 1)
#define LABEL_INTERVAL(_i_) (IntervalIndexType)(_i_ + 1)
#define LABEL_ROW(_r_) (RowIndexType)(_r_ + 1)

// Return string describing type of harped strands
inline LPCTSTR LABEL_HARP_TYPE(bool bAreHarpedStraight) { return bAreHarpedStraight ? _T("Straight-Web") : _T("Harped"); }

class PGSEXTCLASS pgsGirderLabel
{
public:
   static std::_tstring GetGirderLabel(GirderIndexType gdrIdx);
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

std::_tstring PGSEXTFUNC GetEndDistanceMeasureString(ConnectionLibraryEntry::EndDistanceMeasurementType type,bool bAbutment);
std::_tstring PGSEXTFUNC GetBearingOffsetMeasureString(ConnectionLibraryEntry::BearingOffsetMeasurementType type,bool bAbutment);

CString PGSEXTFUNC GetLabel(const CPierData2* pPier,IEAFDisplayUnits* pDisplayUnits);
CString PGSEXTFUNC GetLabel(const CTemporarySupportData* pTS,IEAFDisplayUnits* pDisplayUnits);

CString PGSEXTFUNC ConcreteDescription(const CConcreteMaterial& concrete);
