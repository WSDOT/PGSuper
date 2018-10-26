///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#pragma once;

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
interface IBridgeDescription;
interface IBridge;
interface IGirder;
interface IBarriers;

// MISCELLANEOUS

/*****************************************************************************
CLASS 
   pgsBarrierSidewalkLoadDistributionTool

   Encapsulates the geometry computations for distributin of barrier and sidewalk loads
   for a given span


DESCRIPTION
   Encapsulates the geometry computations for distributin of barrier and sidewalk loads
   for a given span

LOG
   rdp : 1.12.2012 : Created file 
*****************************************************************************/

class pgsBarrierSidewalkLoadDistributionTool 
{
public:
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Default constructor
   pgsBarrierSidewalkLoadDistributionTool(SHARED_LOGFILE lf,IBridgeDescription* pBridgeDesc, IBridge* pBridge, IGirder* pGirder, IBarriers* pBarriers);

   void Initialize(GroupIndexType grpIdx, SegmentIndexType segIdx, pgsTypes::TrafficBarrierDistribution distType, GirderIndexType nMaxDistributed);

   void GetTrafficBarrierLoadFraction(GirderIndexType gdrIdx,
                                      Float64* pFraExtLeft, Float64* pFraIntLeft,
                                      Float64* pFraExtRight,Float64* pFraIntRight);

   void GetSidewalkLoadFraction(GirderIndexType gdrIdx, Float64* pFraSwLeft, Float64* pFraSwRight);

private:
   // GMSWs array below is indexed by these enum's
   enum BarrSwType {LeftExteriorBarrier, LeftSidewalk, LeftInteriorBarrier,
                    RightExteriorBarrier, RightSidewalk, RightInteriorBarrier,
                    BarrSwSize};

   // GROUP: OPERATIONS
   void Compute();
   void BuildGeometryModel();
   void ComputeBarrierLoadDistribution(pgsTypes::TrafficBarrierOrientation orientation, Float64 BrOffset, BarrSwType barrswType, bool oppositeExists);
   void ComputeSidewalkLoadDistribution(pgsTypes::TrafficBarrierOrientation orientation, Float64 extEdgeOffset, Float64 intEdgeOffset, BarrSwType barrswType, bool oppositeExists);
   bool DistributeBSWLoadEvenly(BarrSwType barrswType, bool oppositeExists);
   bool DistributeSidewalkLoadUnderSw(pgsTypes::TrafficBarrierOrientation orientation, Float64 extEdgeOffset, Float64 intEdgeOffset, BarrSwType barrswType);
   void DistributeBSWLoadToNNearest(pgsTypes::TrafficBarrierOrientation orientation, Float64 bSwOffset, BarrSwType barrswType);

   // ACCESS
   //////////
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // Local Data
   GroupIndexType m_GroupIdx;
   SegmentIndexType m_SegmentIdx;
   pgsTypes::TrafficBarrierDistribution m_DistType;
   GirderIndexType m_nMaxDistributed;

   bool m_DidCompute;

   // Weak reference to interface pointers
   IBridgeDescription* m_pIBridgeDesc;
   IBridge* m_pIBridge;
   IGirder* m_pIGirder;
   IBarriers* m_pIBarriers;

   /////////////////////////////////////////////////////////////////////////////////////////////
   // The data structures below store the number of load fractions that go to each girder in a 
   // given span.
   struct GlfData
   {
      // # of GMSWs (Girders, Mating Surfaces, or Webs) loaded by this load on this girder
      IndexType GMSWs[BarrSwSize]; // each value in array is number of loadings for given sw/bar on this girder

      GlfData()
      {
         // zero loads to start with
         memset(GMSWs, 0, BarrSwSize*sizeof(IndexType));
      }
   };

   struct SegmentLoadFractionData
   {
      IndexType m_TotalGMSWs;     // Total Girder, mating surface, or webs in this span
      IndexType m_GMSWsAppliedTo[BarrSwSize]; // Girder, mating surface, or webs that load is disributed to for this sw/bar
      std::vector<GlfData> m_GirderLoadFractions; // number of GMSW's loaded for indiv girders

      SegmentLoadFractionData()
      {
         Init();
      }

      void Init()
      {
         m_TotalGMSWs = 0;
         memset(m_GMSWsAppliedTo, 0, BarrSwSize*sizeof(IndexType));
         m_GirderLoadFractions.clear();
      }
   };

   SegmentLoadFractionData m_SegmentLoadFractionData;

   /////////////////////////////////////////////////////////////////////////////////////////////
   // The data below defines the geometry model used to find the N nearest girders to GMSW's
   CComPtr<ILine2d> m_RefLine; // line bisecting through mid-span of our span
   CComPtr<ILine2d> m_LeftSlabEdgeLine, m_RightSlabEdgeLine; // Lines tangent to slab edges at intersections to m_RefLine on left and right side of bridge

   // List of points where GMSW lines intersect m_RefLine
   struct GMSWInterSectionPoint
   {
      GirderIndexType m_Gdr; // Girder where intersection occurs
      IndexType m_MsW;       // Index of web or mating surface on this girder that's intersected
      CComPtr<IPoint2d> m_IntersectionPoint;

      GMSWInterSectionPoint(GirderIndexType gdr, IndexType msw, CComPtr<IPoint2d> intersectionPoint):
         m_Gdr(gdr), m_MsW(msw), m_IntersectionPoint(intersectionPoint)
      {;}
   };

   std::vector<GMSWInterSectionPoint> m_GMSWInterSectionPoints;
   typedef std::vector<GMSWInterSectionPoint>::iterator GMSWInterSectionIterator;

   CComPtr<IGeomUtil2d> m_GeomUtil;

private:
	DECLARE_SHARED_LOGFILE;

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

