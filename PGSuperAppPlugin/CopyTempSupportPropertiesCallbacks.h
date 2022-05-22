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

#pragma once

#include <IFace\ExtendUI.h>
#include <EAF\EAFTransaction.h>
#include <PgsExt\MacroTxn.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\HandlingData.h>
#include <PgsExt\GirderMaterial.h>
#include <PgsExt\PTData.h>
#include <PgsExt\SegmentPTData.h>
#include <PgsExt\ColumnData.h>

class rptParagraph;

////////////////////////////////////////////////////////////////////////
/////// Utility classes for comparing temporary support data and transactions ///////
////////////////////////////////////////////////////////////////////////

class TempSupportConnectionData
{
public:
   // members
   pgsTypes::TempSupportSegmentConnectionType m_TempSupportSegmentConnectionType;
   Float64   m_GirderEndDistance;
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType;
   Float64 m_GirderBearingOffset;
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType;

   TempSupportConnectionData()
   {
      ;
   }

   // Granddaddy constructor
   TempSupportConnectionData(pgsTypes::TempSupportSegmentConnectionType connType,
                        Float64 endDist, ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure,
                        Float64 girderBearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingOffsetMeasurement);

   // IsEqual operator takes location into account. boundary TempSupports can be equal to interior TempSupports under the right conditions
   bool AreConnectionsEqual(const TempSupportConnectionData& rOther) const;

};


////////////////////////////////////////////////////////////////////////
//////////////////////   Transactions //////////////////////////////////
////////////////////////////////////////////////////////////////////////


class txnCopyTempSupportConnectionProperties :  public CEAFTransaction
{
public:
   txnCopyTempSupportConnectionProperties(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports);
   virtual ~txnCopyTempSupportConnectionProperties();
   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   PierIndexType m_FromTempSupportIdx;
   std::vector<PierIndexType> m_ToTempSupports;
   std::vector<TempSupportConnectionData> m_TempSupportConnectionData;
   bool m_DidDoCopy;
};

////////////////////////////////////////////////////////////////////////////

class CCopyTempSupportConnectionProperties : public ICopyTemporarySupportPropertiesCallback
{
public:
   CCopyTempSupportConnectionProperties();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) override;
   virtual UINT GetTempSupportEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) override;
};

