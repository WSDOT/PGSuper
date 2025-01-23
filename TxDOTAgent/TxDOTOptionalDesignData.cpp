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

#include "StdAfx.h"
#include "TxDOTOptionalDesignData.h"

#include <Units\Convert.h>
#include <WbflAtlExt.h>
#include <limits>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTxDOTOptionalDesignData
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTxDOTOptionalDesignData::CTxDOTOptionalDesignData():
m_OriginalDesignGirderData(this),
m_PrecasterDesignGirderData(this)
{
   ResetData();
}

CTxDOTOptionalDesignData::CTxDOTOptionalDesignData(const CTxDOTOptionalDesignData& rOther):
m_OriginalDesignGirderData(this),
m_PrecasterDesignGirderData(this)
{
   MakeCopy(rOther);
}

CTxDOTOptionalDesignData::~CTxDOTOptionalDesignData()
{
}

///////////////////////////////////////////////////////
// Resets all data to default values
void CTxDOTOptionalDesignData::ResetData()
{
   m_GirderEntryName.Empty();
   m_LeftConnectionEntryName.Empty();
   m_RightConnectionEntryName.Empty();
   m_PGSuperFileName.Empty();
   m_GirderConcreteUnitWeight = WBFL::Units::ConvertToSysUnits(150.0, WBFL::Units::Measure::LbfPerFeet3);

   m_Bridge.Empty();
   m_BridgeID.Empty();
   m_JobNumber.Empty();
   m_Engineer.Empty();
   m_Company.Empty();
   m_Comments.Empty();

   m_SpanNo.Empty();
   m_BeamNo.Empty();

   m_BeamType.Empty();

   // Set all floats to infinity
   m_BeamSpacing = Float64_Inf;
   m_SpanLength = Float64_Inf;
   m_SlabThickness = Float64_Inf;
   m_RelativeHumidity = Float64_Inf;
   m_LldfMoment = Float64_Inf;
   m_LldfShear = Float64_Inf;

   m_EcSlab = WBFL::Units::ConvertToSysUnits(5000.0, WBFL::Units::Measure::KSI);
   m_EcBeam = m_EcSlab;
   m_FcSlab = WBFL::Units::ConvertToSysUnits(4.0, WBFL::Units::Measure::KSI);

   m_Ft = Float64_Inf;
   m_Fb = Float64_Inf;
   m_Mu = Float64_Inf;

   m_WNonCompDc = 0.0;
   m_WCompDc    = 0.0;
   m_WCompDw    = 0.0;

   m_SelectedProjectCriteriaLibrary.Empty();

   m_OriginalDesignGirderData.ResetData();
   m_PrecasterDesignGirderData.ResetData();
}

void CTxDOTOptionalDesignData::ResetStrandNoData()
{
   m_OriginalDesignGirderData.ResetStrandNoData();
   m_PrecasterDesignGirderData.ResetStrandNoData();
}

//======================== OPERATORS  =======================================
CTxDOTOptionalDesignData& CTxDOTOptionalDesignData::operator= (const CTxDOTOptionalDesignData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//bool CTxDOTOptionalDesignData::operator==(const CTxDOTOptionalDesignData& rOther) const
//{
//   ASSERT(0);
//   return false;
//}
//
//
//bool CTxDOTOptionalDesignData::operator!=(const CTxDOTOptionalDesignData& rOther) const
//{
//   return !operator==(rOther);
//}

//======================== OPERATIONS =======================================
HRESULT CTxDOTOptionalDesignData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("TxDOTOptionalGirderData"),2.0);

   // Template Data
   pStrSave->BeginUnit(_T("TemplateData"),2.0);
   pStrSave->put_Property(_T("GirderEntryName"),         CComVariant(m_GirderEntryName));
   pStrSave->put_Property(_T("LeftConnectionEntryName"), CComVariant(m_LeftConnectionEntryName));
   pStrSave->put_Property(_T("RightConnectionEntryName"),CComVariant(m_RightConnectionEntryName));
   pStrSave->put_Property(_T("PGSuperFileName"), CComVariant(m_PGSuperFileName));
   pStrSave->put_Property(_T("GirderConcreteUnitWeight"), CComVariant(m_GirderConcreteUnitWeight));
   pStrSave->EndUnit();

   // Version: 2.0 - Added UseHigherCompressionAllowable
   //          3.0 - Changed UseHigherCompressionAllowable to SelectedProjectCriteriaLibrary
   pStrSave->BeginUnit(_T("BridgeInputData"),3.0);
   pStrSave->put_Property(_T("Bridge"),         CComVariant(m_Bridge));
   pStrSave->put_Property(_T("BridgeID"),         CComVariant(m_BridgeID));
   pStrSave->put_Property(_T("JobNumber"),         CComVariant(m_JobNumber));
   pStrSave->put_Property(_T("Engineer"),         CComVariant(m_Engineer));
   pStrSave->put_Property(_T("Company"),         CComVariant(m_Company));
   pStrSave->put_Property(_T("Comments"),         CComVariant(m_Comments));

   pStrSave->put_Property(_T("SpanNo"),         CComVariant(m_SpanNo));
   pStrSave->put_Property(_T("BeamNo"),         CComVariant(m_BeamNo));
   pStrSave->put_Property(_T("BeamType"),         CComVariant(m_BeamType));
   pStrSave->put_Property(_T("BeamSpacing"),         CComVariant(m_BeamSpacing));
   pStrSave->put_Property(_T("SpanLength"),         CComVariant(m_SpanLength));
   pStrSave->put_Property(_T("SlabThickness"),         CComVariant(m_SlabThickness));

   pStrSave->put_Property(_T("RelativeHumidity"),         CComVariant(m_RelativeHumidity));
   pStrSave->put_Property(_T("LldfMoment"),         CComVariant(m_LldfMoment));
   pStrSave->put_Property(_T("LldfShear"),         CComVariant(m_LldfShear));
   pStrSave->put_Property(_T("EcSlab"),         CComVariant(m_EcSlab));
   pStrSave->put_Property(_T("EcBeam"),         CComVariant(m_EcBeam));
   pStrSave->put_Property(_T("FcSlab"),         CComVariant(m_FcSlab));

   pStrSave->put_Property(_T("Ft"),         CComVariant(m_Ft));
   pStrSave->put_Property(_T("Fb"),         CComVariant(m_Fb));
   pStrSave->put_Property(_T("Mu"),         CComVariant(m_Mu));
   pStrSave->put_Property(_T("WNonCompDc"),         CComVariant(m_WNonCompDc));
   pStrSave->put_Property(_T("WCompDc"),         CComVariant(m_WCompDc));
   pStrSave->put_Property(_T("WCompDw"),         CComVariant(m_WCompDw));

   pStrSave->put_Property(_T("SelectedProjectCriteriaLibrary"), CComVariant(m_SelectedProjectCriteriaLibrary));

   pStrSave->EndUnit(); // BridgeInputData

   pStrSave->BeginUnit(_T("OriginalDesignGirderData"),1.0);
   m_OriginalDesignGirderData.Save(pStrSave, pProgress);
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("PrecasterDesignGirderData"),1.0);
   m_PrecasterDesignGirderData.Save(pStrSave, pProgress);
   pStrSave->EndUnit();

   pStrSave->EndUnit();

   return hr;
}

HRESULT CTxDOTOptionalDesignData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("TxDOTOptionalGirderData"));
      Float64 version;
      pStrLoad->get_Version(&version);

      // Template Data
      hr = pStrLoad->BeginUnit(_T("TemplateData"));

      Float64 lvers;
      pStrLoad->get_Version(&lvers);

      CComVariant var;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("GirderEntryName"), &var );
      m_GirderEntryName = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("LeftConnectionEntryName"), &var );
      m_LeftConnectionEntryName = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("RightConnectionEntryName"), &var );
      m_RightConnectionEntryName = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("PGSuperFileName"), &var );
      m_PGSuperFileName = var.bstrVal;

      if (1.0 < lvers)
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("GirderConcreteUnitWeight"), &var );
         m_GirderConcreteUnitWeight = var.dblVal;
      }

      hr = pStrLoad->EndUnit(); // end TemplateData

      // Bridge Input Data
      hr = pStrLoad->BeginUnit(_T("BridgeInputData"));

      Float64 brgversion;
      pStrLoad->get_Version(&brgversion);

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Bridge"), &var );
      m_Bridge = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BridgeID"), &var );
      m_BridgeID = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("JobNumber"), &var );
      m_JobNumber = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Engineer"), &var );
      m_Engineer = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Company"), &var );
      m_Company = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Comments"), &var );
      m_Comments = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("SpanNo"), &var );
      m_SpanNo = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BeamNo"), &var );
      m_BeamNo = var.bstrVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BeamType"), &var );
      m_BeamType = var.bstrVal;
      
      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("BeamSpacing"), &var );
      m_BeamSpacing = var.dblVal;
      
      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SpanLength"), &var );
      m_SpanLength = var.dblVal;
      
      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SlabThickness"), &var );
      m_SlabThickness = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("RelativeHumidity"), &var );
      m_RelativeHumidity = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LldfMoment"), &var );
      m_LldfMoment = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LldfShear"), &var );
      m_LldfShear = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("EcSlab"), &var );
      m_EcSlab = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("EcBeam"), &var );
      m_EcBeam = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("FcSlab"), &var );
      m_FcSlab = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Ft"), &var );
      m_Ft = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Fb"), &var );
      m_Fb = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Mu"), &var );
      m_Mu = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("WNonCompDc"), &var );
      m_WNonCompDc = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("WCompDc"), &var );
      m_WCompDc = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("WCompDw"), &var );
      m_WCompDw = var.dblVal;

      if (brgversion < 3.0)
      {
         bool useHigherCompressionAllowable(false);

         if (brgversion > 1.0)
         {
            var.Clear();
            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("UseHigherCompressionAllowable"), &var );
            useHigherCompressionAllowable = var.boolVal!=VARIANT_FALSE;
         }

         // Default names for project criteria with 0.60f'ci and 0.65f'ci release stress limits
         // This is for older files before the project criteria was selectable and an option was
         // provided to increase the allowable.
         const LPCTSTR sProjectCriteria_060=_T("TxDOT 2008");
         const LPCTSTR sProjectCriteria_065=_T("TxDOT 2010");

         m_SelectedProjectCriteriaLibrary = useHigherCompressionAllowable ? sProjectCriteria_065 : sProjectCriteria_060;
      }
      else
      {
         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("SelectedProjectCriteriaLibrary"), &var );
         m_SelectedProjectCriteriaLibrary = var.bstrVal;
      }

      hr = pStrLoad->EndUnit(); // end BridgeInputData

      // Girder Data
      hr = pStrLoad->BeginUnit(_T("OriginalDesignGirderData"));
      m_OriginalDesignGirderData.Load(pStrLoad, pProgress);
      hr = pStrLoad->EndUnit();

      hr = pStrLoad->BeginUnit(_T("PrecasterDesignGirderData"));
      m_PrecasterDesignGirderData.Load(pStrLoad, pProgress);
      hr = pStrLoad->EndUnit();

      // last unit - fires an error if this is called...?
      // 
      //hr = pStrLoad->EndUnit(); // end TxDOTOptionalGirderData
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   return hr;
}


//======================== ACCESS     =======================================
void CTxDOTOptionalDesignData::SetGirderEntryName(const CString& value)
{
   if (value != m_GirderEntryName)
   {
      m_GirderEntryName = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetGirderEntryName()
{
   return m_GirderEntryName;
}

void CTxDOTOptionalDesignData::SetLeftConnectionEntryName(const CString& value)
{
   if (value != m_LeftConnectionEntryName)
   {
      m_LeftConnectionEntryName = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetLeftConnectionEntryName()
{
   return m_LeftConnectionEntryName;
}

void CTxDOTOptionalDesignData::SetRightConnectionEntryName(const CString& value)
{
   if (value != m_RightConnectionEntryName)
   {
      m_RightConnectionEntryName = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetRightConnectionEntryName()
{
   return m_RightConnectionEntryName;
}

void CTxDOTOptionalDesignData::SetPGSuperFileName(const CString& value)
{
   if (value != m_PGSuperFileName)
   {
      m_PGSuperFileName = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetPGSuperFileName()
{
   return m_PGSuperFileName;
}

void CTxDOTOptionalDesignData::SetGirderConcreteUnitWeight(Float64 uw)
{
   if (uw != m_GirderConcreteUnitWeight)
   {
      m_GirderConcreteUnitWeight = uw;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetGirderConcreteUnitWeight()
{
   return m_GirderConcreteUnitWeight;
}

// Bridge Input Data

void CTxDOTOptionalDesignData::SetBridge(const CString& value)
{
   if (value != m_Bridge)
   {
      m_Bridge = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetBridge() const
{
   return m_Bridge;
}

void CTxDOTOptionalDesignData::SetBridgeID(const CString& value)
{
   if (value != m_BridgeID)
   {
      m_BridgeID = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetBridgeID() const
{
   return m_BridgeID;
}

void CTxDOTOptionalDesignData::SetJobNumber(const CString& value)
{
   if (value != m_JobNumber)
   {
      m_JobNumber = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetJobNumber() const
{
   return m_JobNumber;
}

CString CTxDOTOptionalDesignData::GetEngineer() const
{
   return m_Engineer;
}

void CTxDOTOptionalDesignData::SetEngineer(const CString& value)
{
   if (value != m_Engineer)
   {
      m_Engineer = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

void CTxDOTOptionalDesignData::SetCompany(const CString& value)
{
   if (value != m_Company)
   {
      m_Company = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetCompany() const
{
   return m_Company;
}

void CTxDOTOptionalDesignData::SetComments(const CString& value)
{
   if (value != m_Comments)
   {
      m_Comments = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetComments() const
{
   return m_Comments;
}

void CTxDOTOptionalDesignData::SetSpanNo(const CString& value)
{
   if (value != m_SpanNo)
   {
      m_SpanNo = value;
      FireChanged(ITxDataObserver::ctLocal);
   }
}

CString CTxDOTOptionalDesignData::GetSpanNo() const
{
   return m_SpanNo;
}

void CTxDOTOptionalDesignData::SetBeamNo(const CString& value)
{
   if (value != m_BeamNo)
   {
      m_BeamNo = value;
      FireChanged(ITxDataObserver::ctLocal);
   }
}

CString CTxDOTOptionalDesignData::GetBeamNo() const
{
   return m_BeamNo;
}

void CTxDOTOptionalDesignData::SetBeamType(const CString& value, bool doFire)
{
   if (value != m_BeamType)
   {
      m_BeamType = value;

      if (doFire)
         FireChanged(ITxDataObserver::ctTemplateFile); // could affect template
   }
}

CString CTxDOTOptionalDesignData::GetBeamType() const
{
   return m_BeamType;
}

void CTxDOTOptionalDesignData::SetBeamSpacing(Float64 value)
{
   if (value != m_BeamSpacing)
   {
      m_BeamSpacing = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetBeamSpacing() const
{
   return m_BeamSpacing;
}

void CTxDOTOptionalDesignData::SetSpanLength(Float64 value)
{
   if (value != m_SpanLength)
   {
      m_SpanLength = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetSpanLength() const
{
   return m_SpanLength;
}

void CTxDOTOptionalDesignData::SetSlabThickness(Float64 value)
{
   if (value != m_SlabThickness)
   {
      m_SlabThickness = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetSlabThickness() const
{
   return m_SlabThickness;
}

void CTxDOTOptionalDesignData::SetRelativeHumidity(Float64 value)
{
   if (value != m_RelativeHumidity)
   {
      m_RelativeHumidity = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetRelativeHumidity() const
{
   return m_RelativeHumidity;
}

void CTxDOTOptionalDesignData::SetLldfMoment(Float64 value)
{
   if (value != m_LldfMoment)
   {
      m_LldfMoment = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetLldfMoment() const
{
   return m_LldfMoment;
}

void CTxDOTOptionalDesignData::SetLldfShear(Float64 value)
{
   if (value != m_LldfShear)
   {
      m_LldfShear = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetLldfShear() const
{
   return m_LldfShear;
}

void CTxDOTOptionalDesignData::SetEcSlab(Float64 value)
{
   if (value != m_EcSlab)
   {
      m_EcSlab = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetEcSlab() const
{
   return m_EcSlab;
}

void CTxDOTOptionalDesignData::SetEcBeam(Float64 value)
{
   if (value != m_EcBeam)
   {
      m_EcBeam = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetEcBeam() const
{
   return m_EcBeam;
}

void CTxDOTOptionalDesignData::SetFcSlab(Float64 value)
{
   if (value != m_FcSlab)
   {
      m_FcSlab = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetFcSlab() const
{
   return m_FcSlab;
}

void CTxDOTOptionalDesignData::SetFt(Float64 value)
{
   if (value != m_Ft)
   {
      m_Ft = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetFt() const
{
   return m_Ft;
}

void CTxDOTOptionalDesignData::SetFb(Float64 value)
{
   if (value != m_Fb)
   {
      m_Fb = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetFb() const
{
   return m_Fb;
}

void CTxDOTOptionalDesignData::SetMu(Float64 value)
{
   if (value != m_Mu)
   {
      m_Mu = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetMu() const
{
   return m_Mu;
}

void CTxDOTOptionalDesignData::SetWNonCompDc(Float64 value)
{
   if (value != m_WNonCompDc)
   {
      m_WNonCompDc = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetWNonCompDc() const
{
   return m_WNonCompDc;
}

void CTxDOTOptionalDesignData::SetWCompDc(Float64 value)
{
   if (value != m_WCompDc)
   {
      m_WCompDc = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetWCompDc() const
{
   return m_WCompDc;
}

void CTxDOTOptionalDesignData::SetWCompDw(Float64 value)
{
   if (value != m_WCompDw)
   {
      m_WCompDw = value;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

Float64 CTxDOTOptionalDesignData::GetWCompDw() const
{
   return m_WCompDw;
}

void CTxDOTOptionalDesignData::SetSelectedProjectCriteriaLibrary(const CString& val)
{
   if (val != m_SelectedProjectCriteriaLibrary)
   {
      m_SelectedProjectCriteriaLibrary = val;
      FireChanged(ITxDataObserver::ctPGSuper);
   }
}

CString CTxDOTOptionalDesignData::GetSelectedProjectCriteriaLibrary() const
{
   return m_SelectedProjectCriteriaLibrary;
}

CTxDOTOptionalDesignGirderData* CTxDOTOptionalDesignData::GetOriginalDesignGirderData()
{
   return &m_OriginalDesignGirderData;
}

const CTxDOTOptionalDesignGirderData* CTxDOTOptionalDesignData::GetOriginalDesignGirderData() const
{
   return &m_OriginalDesignGirderData;
}


CTxDOTOptionalDesignGirderData* CTxDOTOptionalDesignData::GetPrecasterDesignGirderData()
{
   return &m_PrecasterDesignGirderData;
}

const CTxDOTOptionalDesignGirderData* CTxDOTOptionalDesignData::GetPrecasterDesignGirderData() const
{
   return &m_PrecasterDesignGirderData;
}

//======================== INQUIRY    =======================================
// Let our observers listen
void CTxDOTOptionalDesignData::Attach(ITxDataObserver* pObserver)
{
   ASSERT(pObserver!=nullptr);
   m_pObservers.insert(pObserver);
}

void CTxDOTOptionalDesignData::Detach(ITxDataObserver* pObserver)
{
   m_pObservers.erase(pObserver);
}

////////////////////////// PROTECTED  ///////////////////////////////////////
//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CTxDOTOptionalDesignData::MakeCopy(const CTxDOTOptionalDesignData& rOther)
{
   m_GirderEntryName = rOther.m_GirderEntryName;
   m_LeftConnectionEntryName = rOther.m_LeftConnectionEntryName;
   m_RightConnectionEntryName = rOther.m_RightConnectionEntryName;
   m_PGSuperFileName = rOther.m_PGSuperFileName;
   m_GirderConcreteUnitWeight = rOther.m_GirderConcreteUnitWeight;

   m_Bridge = rOther.m_Bridge;
   m_BridgeID = rOther.m_BridgeID;
   m_JobNumber = rOther.m_JobNumber;
   m_Engineer = rOther.m_Engineer;
   m_Company = rOther.m_Company;
   m_Comments = rOther.m_Comments;

   m_SpanNo = rOther.m_SpanNo;
   m_BeamNo = rOther.m_BeamNo;

   m_BeamType = rOther.m_BeamType;

   // Set all floats to infinity
   m_BeamSpacing = rOther.m_BeamSpacing;
   m_SpanLength = rOther.m_SpanLength;
   m_SlabThickness = rOther.m_SlabThickness;
   m_RelativeHumidity = rOther.m_RelativeHumidity;
   m_LldfMoment = rOther.m_LldfMoment;
   m_LldfShear = rOther.m_LldfShear;

   m_EcSlab = rOther.m_EcSlab;
   m_EcBeam = rOther.m_EcBeam;
   m_FcSlab = rOther.m_FcSlab;

   m_Ft = rOther.m_Ft;
   m_Fb = rOther.m_Fb;
   m_Mu = rOther.m_Mu;

   m_WNonCompDc = rOther.m_WNonCompDc;
   m_WCompDc = rOther.m_WCompDc;
   m_WCompDw = rOther.m_WCompDw;

   m_SelectedProjectCriteriaLibrary = rOther.m_SelectedProjectCriteriaLibrary;

   m_OriginalDesignGirderData = rOther.m_OriginalDesignGirderData;
   m_PrecasterDesignGirderData = rOther.m_PrecasterDesignGirderData;
}

void CTxDOTOptionalDesignData::MakeAssignment(const CTxDOTOptionalDesignData& rOther)
{
   MakeCopy( rOther );
}

void CTxDOTOptionalDesignData::FireChanged(ITxDataObserver::ChangeType change)
{
   // notify our observers we changed
   for(std::set<ITxDataObserver*>::iterator it=m_pObservers.begin(); it!=m_pObservers.end(); it++)
   {
      ITxDataObserver* pit = *it;
      pit->OnTxDotDataChanged(change);
   }
}
