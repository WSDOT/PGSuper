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

#include "stdafx.h"
#include "CopyTempSupportPropertiesCallbacks.h"

#include <EAF\EAFUtilities.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <IFace\DocumentType.h>

#include <PgsExt\BridgeDescription2.h>

#include <Reporter\ReportingUtils.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////// reporting utilities
inline void ColorFromRow(rptRcTable* p_table, RowIndexType row, ColumnIndexType nCols)
{
   // Color background of From row
   for (ColumnIndexType col = 0; col < nCols; col++)
   {
      rptRiStyle::FontColor color = rptRiStyle::Yellow;
      (*p_table)(row, col) << new rptRcBgColor(color);
      (*p_table)(row, col).SetFillBackGroundColor(color);
   }
}

// Common function to write comparison cell
inline void WriteCompareCell(rptRcTable* p_table, RowIndexType row, ColumnIndexType col, bool isFrom, bool isEqual)
{
   if (!isFrom)
   {
      if (isEqual)
      {
         (*p_table)(row, col) << color(Green) << _T("Yes") << color(Black);
      }
      else
      {
         (*p_table)(row, col) << color(Red) << _T("No") << color(Black);
      }
   }
   else
   {
      (*p_table)(row, col) << symbol(NBSP);
   }
}

static TempSupportConnectionData MakeTempSupportConnectionData(const CTemporarySupportData* pTempSupport)
{
   pgsTypes::TempSupportSegmentConnectionType tsType = pTempSupport->GetConnectionType();
   Float64 brgOffset;
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
   pTempSupport->GetBearingOffset(&brgOffset,&brgOffsetMeasure);

   Float64 endDist;
   ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
   pTempSupport->GetGirderEndDistance(&endDist, &endDistMeasure);

   return TempSupportConnectionData(tsType, endDist, endDistMeasure, brgOffset, brgOffsetMeasure);
}

static bool CanCopyConnectionData(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTemporarySupportData* pFromTempSupport = pBridgeDesc->GetTemporarySupport(fromTempSupportIdx);

   // cannot copy To or From continuous segments
   if (pgsTypes::tsctContinuousSegment == pFromTempSupport->GetConnectionType())
   {
      return false;
   }

   for (auto TempSupportIdx : toTempSupports)
   {
      const CTemporarySupportData* pTempSupport = pBridgeDesc->GetTemporarySupport(TempSupportIdx);
      if (pgsTypes::tsctContinuousSegment == pTempSupport->GetConnectionType())
      {
         return false;
      }
   }

   return true;
}

// Declaration of comparison reports
static void TempSupportConnectionPropertiesComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers);

////////////////////////////////////////////////////
//////////////////// Transaction Classes ////////////
////////////////////////////////////////////////////

txnCopyTempSupportConnectionProperties::txnCopyTempSupportConnectionProperties(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports)
{
   m_FromTempSupportIdx = fromTempSupportIdx;
   m_ToTempSupports  = toTempSupports;
}

txnCopyTempSupportConnectionProperties::~txnCopyTempSupportConnectionProperties()
{
}

bool txnCopyTempSupportConnectionProperties::Execute()
{
   // Do nothing if data cannot be copied. This saves second guessing when we are part of a composite
   if (!CanCopyConnectionData(m_FromTempSupportIdx, m_ToTempSupports))
   {
      m_DidDoCopy = false;
   }
   else
   {
      m_DidDoCopy = true;

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker, IEvents, pEvents);
      pEvents->HoldEvents(); // Large bridges can take a long time. Don't fire any changed events until all changes are done

      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      m_TempSupportConnectionData.clear();

      PierIndexType nTS = pBridgeDesc->GetTemporarySupportCount();

      // Use utility class to get and store from data
      const CTemporarySupportData* pFromTempSupport = pBridgeDesc->GetTemporarySupport(m_FromTempSupportIdx);
      TempSupportConnectionData fromData(MakeTempSupportConnectionData(pFromTempSupport));

      pgsTypes::TempSupportSegmentConnectionType  fromConnType = pFromTempSupport->GetConnectionType();

      for (const auto toTempSupportIdx : m_ToTempSupports)
      {
         CTemporarySupportData toTempSupport = *(pIBridgeDesc->GetTemporarySupport(toTempSupportIdx));

         // Store old data
         m_TempSupportConnectionData.push_back(MakeTempSupportConnectionData(&toTempSupport));

         pgsTypes::TempSupportSegmentConnectionType  toConnType = toTempSupport.GetConnectionType();

         if (toTempSupportIdx != m_FromTempSupportIdx &&  fromConnType != pgsTypes::tsctContinuousSegment && toConnType != pgsTypes::tsctContinuousSegment ) // don't set data from continuous segments
         {
            toTempSupport.SetBearingOffset(fromData.m_GirderBearingOffset, fromData.m_BearingOffsetMeasurementType);
            toTempSupport.SetGirderEndDistance(fromData.m_GirderEndDistance, fromData.m_EndDistanceMeasurementType);

            // Set new data
            pIBridgeDesc->SetTemporarySupportByIndex(toTempSupportIdx, toTempSupport);
         }
      }

      pEvents->FirePendingEvents();
   }

   return true;
}

void txnCopyTempSupportConnectionProperties::Undo()
{
   if (m_DidDoCopy)  
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

      std::vector<TempSupportConnectionData>::iterator iterPCD = m_TempSupportConnectionData.begin();
      for (const auto toTempSupportIdx : m_ToTempSupports)
      {
         TempSupportConnectionData& toData = *iterPCD;

         CTemporarySupportData toTempSupport = *(pIBridgeDesc->GetTemporarySupport(toTempSupportIdx));

         toTempSupport.SetBearingOffset(toData.m_GirderBearingOffset, toData.m_BearingOffsetMeasurementType);
         toTempSupport.SetGirderEndDistance(toData.m_GirderEndDistance, toData.m_EndDistanceMeasurementType);

         pIBridgeDesc->SetTemporarySupportByIndex(toTempSupportIdx, toTempSupport);

         iterPCD++;
      }
   }
}

std::unique_ptr<CEAFTransaction> txnCopyTempSupportConnectionProperties::CreateClone() const
{
   return std::make_unique<txnCopyTempSupportConnectionProperties>(m_FromTempSupportIdx,m_ToTempSupports);
}

std::_tstring txnCopyTempSupportConnectionProperties::Name() const
{
   return _T("txnCopyTempSupportConnectionProperties");
}


///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Callback Classes    /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


CCopyTempSupportConnectionProperties::CCopyTempSupportConnectionProperties()
{
}

LPCTSTR CCopyTempSupportConnectionProperties::GetName()
{
   return _T("Connection Geometry");
}

BOOL CCopyTempSupportConnectionProperties::CanCopy(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports)
{
   return CanCopyConnectionData(fromTempSupportIdx, toTempSupports);
}

std::unique_ptr<CEAFTransaction> CCopyTempSupportConnectionProperties::CreateCopyTransaction(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports)
{
   return std::make_unique<txnCopyTempSupportConnectionProperties>(fromTempSupportIdx, toTempSupports);
}

UINT CCopyTempSupportConnectionProperties::GetTempSupportEditorTabIndex()
{
   return ETS_CONNECTION;
}

rptParagraph* CCopyTempSupportConnectionProperties::BuildComparisonReportParagraph(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   TempSupportConnectionPropertiesComparison(pPara, pBroker, fromTempSupportIdx, toTempSupports);

   return pPara;
}


/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Reporting functions /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void TempSupportConnectionPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false );

   ColumnIndexType nCols = 7;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Temporary Support Connections"));
   *pPara << p_table;

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 1;
   (*p_table)(0, col++) << _T("Connection") << rptNewLine << _T("Type");
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Temp Support?");
   (*p_table)(0,col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(0,col++) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*p_table)(0,col++) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(0,col++) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   RowIndexType irow = p_table->GetNumberOfHeaderRows();

   PierIndexType nTempSupports = pBridgeDesc->GetTemporarySupportCount();
   if (fromTempSupportIdx > nTempSupports - 1)
   {
      ATLASSERT(0); // this should never happen
   }

   const CTemporarySupportData* pFromTempSupport = pBridgeDesc->GetTemporarySupport(fromTempSupportIdx);

   // use utility class to store data and for comparisons
   TempSupportConnectionData fromTempSupportConnectionData( MakeTempSupportConnectionData(pFromTempSupport) );

   for (PierIndexType TempSupportIdx = 0; TempSupportIdx<nTempSupports; TempSupportIdx++)
   {
      col = 0;

      bool isFrom = TempSupportIdx == fromTempSupportIdx;
      if (isFrom)
      {
         ColorFromRow(p_table, irow, nCols);
      }

      const CTemporarySupportData* pTempSupport = pBridgeDesc->GetTemporarySupport(TempSupportIdx);

      CString strts;
      strts.Format(_T("%s %d"), CTemporarySupportData::AsString(pTempSupport->GetSupportType()), LABEL_TEMPORARY_SUPPORT( TempSupportIdx));
      (*p_table)(irow, col++) << strts;

      pgsTypes::TempSupportSegmentConnectionType connType = pTempSupport->GetConnectionType();
      (*p_table)(irow, col++) << CTemporarySupportData::AsString(connType);

      // use utility class to store data and for comparisons
      TempSupportConnectionData TempSupportConnectionData( MakeTempSupportConnectionData(pTempSupport) );

      bool isEqual = fromTempSupportConnectionData.AreConnectionsEqual(TempSupportConnectionData);

      WriteCompareCell(p_table, irow, col++, isFrom, isEqual);

      if (pgsTypes::tsctContinuousSegment == connType)
      {
         p_table->SetColumnSpan(irow,col,nCols-col);
         (*p_table)(irow, col++) << _T("Segment is continuous over Temporary Support");
      }
      else
      {
         (*p_table)(irow, col++) << cmpdim.SetValue(TempSupportConnectionData.m_GirderBearingOffset);
         (*p_table)(irow, col++) << GetTempSupportBearingOffsetMeasureString(TempSupportConnectionData.m_BearingOffsetMeasurementType, true);

         (*p_table)(irow, col++) << cmpdim.SetValue(TempSupportConnectionData.m_GirderEndDistance);
         (*p_table)(irow, col++) << GetTempSupportEndDistanceMeasureString(TempSupportConnectionData.m_EndDistanceMeasurementType, true);
      }

      irow++;
   }

   *pPara << rptNewLine;
   *pPara << Underline(Bold(_T("Legend:"))) << rptNewLine;
   *pPara << Bold(_T("Bearing Offset Measure")) << rptNewLine;
   *pPara << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, true) << _T(" = ") << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, false) << rptNewLine;
   *pPara << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true) << _T(" = ") << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << Bold(_T("End Distance Measure")) << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder, false) << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false) << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, false) << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, false) << rptNewLine;
   *pPara << rptNewLine;
}

////////////////////////////////////////////////////////////////////////
/////// Classes for comparing pier data and transactions ///////////////
////////////////////////////////////////////////////////////////////////

TempSupportConnectionData::TempSupportConnectionData(pgsTypes::TempSupportSegmentConnectionType connType,
                        Float64 endDist, ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure,
                        Float64 girderBearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingOffsetMeasurement)
{
   m_TempSupportSegmentConnectionType = connType;
   m_GirderEndDistance            = endDist;
   m_EndDistanceMeasurementType   = endDistMeasure;
   m_GirderBearingOffset          = girderBearingOffset;
   m_BearingOffsetMeasurementType = bearingOffsetMeasurement;
}

bool TempSupportConnectionData::AreConnectionsEqual(const TempSupportConnectionData& rOther) const
{
   return m_TempSupportSegmentConnectionType == rOther.m_TempSupportSegmentConnectionType &&
      IsEqual(m_GirderEndDistance, rOther.m_GirderEndDistance) &&
      m_EndDistanceMeasurementType == rOther.m_EndDistanceMeasurementType &&
      IsEqual(m_GirderBearingOffset, rOther.m_GirderBearingOffset) &&
      m_BearingOffsetMeasurementType == rOther.m_BearingOffsetMeasurementType;
}

