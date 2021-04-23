///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <System\Transaction.h>
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

enum PierCBLocType { pcblLeftBoundaryPier, pcblInteriorPier, pcblContinousSegment, pcblRightBoundaryPier};

////////////////////////////////////////////////////////////////////////
/////// Utility classes for comparing pier data and transactions ///////
////////////////////////////////////////////////////////////////////////

class PierConnectionData
{
public:
   // members
   PierCBLocType m_PierLocType;
   pgsTypes::BoundaryConditionType m_BoundaryConditionType;
   std::array<Float64, 2> m_GirderEndDistance;
   std::array<ConnectionLibraryEntry::EndDistanceMeasurementType, 2> m_EndDistanceMeasurementType;
   std::array<Float64, 2> m_GirderBearingOffset;
   std::array<ConnectionLibraryEntry::BearingOffsetMeasurementType, 2> m_BearingOffsetMeasurementType;

   PierConnectionData()
   {
      ;
   }

   // Granddaddy constructor
   PierConnectionData(PierCBLocType PierType, pgsTypes::BoundaryConditionType boundaryConditionType,
                        Float64 backEndDist, ConnectionLibraryEntry::EndDistanceMeasurementType backEndDistMeasure,
                        Float64 backGirderBearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType backBearingOffsetMeasurementType,
                        Float64 aheadEndDist, ConnectionLibraryEntry::EndDistanceMeasurementType aheadEndDistMeasure,
                        Float64 aheadGirderBearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType aheadBearingOffsetMeasurementType);

   // IsEqual operator takes location into account. boundary piers can be equal to interior piers under the right conditions
   bool AreConnectionsEqual(const PierConnectionData& rOther) const;

private:

   bool IsSideEqual(pgsTypes::PierFaceType myFace, pgsTypes::PierFaceType otherFace, const PierConnectionData& rOther) const;
   void CopyFace(pgsTypes::PierFaceType myFace, pgsTypes::PierFaceType otherFace, const PierConnectionData& rFromOther);
};

class PierDiaphragmData
{
public:
   // members.
   PierCBLocType m_PierLocType;
   std::array<Float64, 2> m_Height;
   std::array<Float64, 2> m_Width;
   std::array<Float64, 2> m_LoadLocation;
   std::array<ConnectionLibraryEntry::DiaphragmLoadType, 2> m_DiaphragmLoadType;

   PierDiaphragmData()
   {
      ;
   }
   // Granddaddy constructor
   PierDiaphragmData::PierDiaphragmData(PierCBLocType PierType,
                                       Float64 backHeight, Float64 backWidth, ConnectionLibraryEntry::DiaphragmLoadType backLoadType, Float64 backLoadLocation,
                                       Float64 aheadHeight, Float64 aheadWidth, ConnectionLibraryEntry::DiaphragmLoadType aheadLoadType, Float64 aheadLoadLocation);

   // IsEqual operator takes location into account. boundary piers can be equal to interior piers under the right conditions
   bool AreDiaphragmsEqual(const PierDiaphragmData& rOther) const;

private:
   bool IsSideEqual(pgsTypes::PierFaceType myFace, pgsTypes::PierFaceType otherFace, const PierDiaphragmData& rOther) const;
   void CopyFace(pgsTypes::PierFaceType myFace, pgsTypes::PierFaceType otherFace, const PierDiaphragmData& rFromOther);
};

class PierModelData
{
public:
   pgsTypes::PierModelType m_PierModelType; 
   // This data only used if m_PierModelType is pgsTypes::pmtPhysical
   CConcreteMaterial m_Concrete;

   // arrays by pgsTypes::SideType
   ColumnIndexType m_RefColumnIdx;
   Float64 m_TransverseOffset;
   pgsTypes::OffsetMeasurementType m_TransverseOffsetMeasurement;

   // Cross Beam Dimensions (array index is SideType enum)
   std::array<Float64, 2> m_XBeamHeight;
   std::array<Float64, 2> m_XBeamTaperHeight;
   std::array<Float64, 2> m_XBeamTaperLength;
   std::array<Float64, 2> m_XBeamEndSlopeOffset;
   std::array<Float64, 2> m_XBeamOverhang;
   Float64 m_XBeamWidth;

   // Column Dimensions and Layout
   CColumnData::ColumnHeightMeasurementType m_ColumnHeightType; // Note this value is per-column in column data, but is contstrained by the UI to be the same for all columns. We will treat per UI
   pgsTypes::ColumnLongitudinalBaseFixityType m_ColumnFixity;
   std::vector<Float64> m_ColumnSpacing;
   std::vector<CColumnData> m_Columns;

   // Idealized model
   PierModelData::PierModelData() :
      m_PierModelType(pgsTypes::pmtIdealized)
   {
   }

   // Constructor for physical piers. Creates a single column
   PierModelData::PierModelData(const CConcreteMaterial& rConcrete, ColumnIndexType refColumnIdx, Float64 transverseOffset, pgsTypes::OffsetMeasurementType transverseOffsetMeasurement,
                                 Float64 leftXBeamHeight, Float64 leftXBeamTaperHeight, Float64 leftXBeamTaperLength, Float64 leftXBeamEndSlopeOffset, Float64 leftXBeamOverhang,
                                 Float64 rightXBeamHeight, Float64 rightXBeamTaperHeight, Float64 rightXBeamTaperLength, Float64 rightXBeamEndSlopeOffset, Float64 rightXBeamOverhang,
                                 Float64 XBeamWidth, CColumnData::ColumnHeightMeasurementType columnHeightType, pgsTypes::ColumnLongitudinalBaseFixityType columnFixity, const CColumnData& rColumnData);

   void AddColumn(Float64 spacing, const CColumnData& rColumnData);
      
   bool AreModelsEqual(const PierModelData& rOther) const;
   bool ArePierMaterialsEqual(const PierModelData& rOther) const;
   bool ArePierCapDimensionsEqual(const PierModelData& rOther) const;
   bool AreColumnLayoutsEqual(const PierModelData& rOther) const;

private:
};

// Class for comparing all pier data. Is composed of our previous comparison classes
class PierAllData
{
public:
   PierAllData()
   {
      ;
   }
   
   PierModelData m_PierModelData;
   PierDiaphragmData m_PierDiaphragmData;
   PierConnectionData m_PierConnectionData;

   // Comparison function
   bool ArePiersEqual(const PierAllData& rOther) const;

};

////////////////////////////////////////////////////////////////////////
//////////////////////   Transactions //////////////////////////////////
////////////////////////////////////////////////////////////////////////

class txnCopyPierAllProperties :  public pgsMacroTxn
{
public:
   txnCopyPierAllProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers);
   virtual ~txnCopyPierAllProperties();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   PierIndexType m_FromPierIdx;
   std::vector<PierIndexType> m_ToPiers;
};


class txnCopyPierConnectionProperties :  public txnTransaction
{
public:
   txnCopyPierConnectionProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers);
   virtual ~txnCopyPierConnectionProperties();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   PierIndexType m_FromPierIdx;
   std::vector<PierIndexType> m_ToPiers;
   std::vector<PierConnectionData> m_PierConnectionData;
   bool m_DidDoCopy;
};

class txnCopyPierDiaphragmProperties :  public txnTransaction
{
public:
   txnCopyPierDiaphragmProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers);
   virtual ~txnCopyPierDiaphragmProperties();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   PierIndexType m_FromPierIdx;
   std::vector<PierIndexType> m_ToPiers;
   std::vector<PierDiaphragmData> m_PierDiaphragmData;
};

class txnCopyPierModelProperties :  public txnTransaction
{
public:
   txnCopyPierModelProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers);
   virtual ~txnCopyPierModelProperties();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   PierIndexType m_FromPierIdx;
   std::vector<PierIndexType> m_ToPiers;
   std::vector<CPierData2> m_PierData;
};

////////////////////////////////////////////////////////////////////////////

class CCopyPierAllProperties : public ICopyPierPropertiesCallback
{
public:
   CCopyPierAllProperties();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual txnTransaction* CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual UINT GetPierEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
};

class CCopyPierConnectionProperties : public ICopyPierPropertiesCallback
{
public:
   CCopyPierConnectionProperties();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual txnTransaction* CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual UINT GetPierEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
};

class CCopyPierDiaphragmProperties : public ICopyPierPropertiesCallback
{
public:
   CCopyPierDiaphragmProperties();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual txnTransaction* CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual UINT GetPierEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
};

class CCopyPierModelProperties : public ICopyPierPropertiesCallback
{
public:
   CCopyPierModelProperties();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual txnTransaction* CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
   virtual UINT GetPierEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) override;
};
