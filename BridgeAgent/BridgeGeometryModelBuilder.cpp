///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include "BridgeGeometryModelBuilder.h"
#include "BridgeHelpers.h"

#include <PgsExt\ClosureJointData.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <IFace\Project.h>

IDType CBridgeGeometryModelBuilder::AlignmentID = 999;
IDType CBridgeGeometryModelBuilder::ProfileID = 999;
IDType CBridgeGeometryModelBuilder::SurfaceID = 999;

// utility function to get midpoint between two points
inline void GetMidPoint(IPoint2d* P1, IPoint2d* P2, IPoint2d** pMid)
{
   Float64 X1, X2, Y1, Y2;
   P1->get_X(&X1);
   P2->get_X(&X2);
   P1->get_Y(&Y1);
   P2->get_Y(&Y2);

   Float64 Xmid = (X1 + X2) / 2.0;
   Float64 Ymid = (Y1 + Y2) / 2.0;

   CComPtr<IPoint2d> midPt;
   midPt.CoCreateInstance(CLSID_Point2d);
   midPt->Move(Xmid, Ymid);

   midPt.CopyTo(pMid);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBridgeGeometryModelBuilder::CBridgeGeometryModelBuilder()
{
}

bool CBridgeGeometryModelBuilder::BuildBridgeGeometryModel(const CBridgeDescription2* pBridgeDesc,ICogoModel* pCogoModel,IAlignment* pAlignment,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& orientationCollection)
{
   orientationCollection.clear();

   // Associate an alignment with the bridge model (bridges can be associated with many alignments)
   pBridgeGeometry->AddAlignment(AlignmentID,pAlignment);

   // Set the bridge line offset
   pBridgeGeometry->put_BridgeLineOffset(pBridgeDesc->GetAlignmentOffset());

   // Set the ID of the main bridge alignment (all other alignments are secondary)
   pBridgeGeometry->put_BridgeAlignmentID(AlignmentID);

   if ( !LayoutPiers(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   if ( !LayoutTemporarySupports(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   if ( !LayoutGirderLines(pBridgeDesc,pBridgeGeometry, orientationCollection) )
   {
      return false;
   }

   if ( !LayoutDiaphragmLines(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutPiers(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   PierIndexType pierIdx = 0;
   while ( pPier )
   {
      PierIDType pierID = ::GetPierLineID(pierIdx);

      CComPtr<IAngle> skew;
      CComPtr<IDirection> direction;
      GetPierDirection(alignment,pPier,&skew,&direction);

      Float64 skew_angle;
      skew->get_Value(&skew_angle);

      Float64 pier_length;
      Float64 left_end_offset; // offset from the alignment to the left hand-side of the pier, measured along the pier

      if ( pPier->GetPierModelType() == pgsTypes::pmtIdealized )
      {
         // get maximum girder spacing width (back and ahead side of piers)
         // width is to be measured along CL pier
         // also get the offset from the alignment, measured along the CL pier, to the left-most girder
         Float64 back_width  = 0;
         Float64 ahead_width = 0;

         Float64 back_offset  = 0;
         Float64 ahead_offset = 0;

         const CSpanData2* pPrevSpan = pPier->GetPrevSpan();
         const CSpanData2* pNextSpan = pPier->GetNextSpan();

         if ( pPrevSpan )
         {
            const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Back);
            GetPierLineProperties(pBridgeDesc,pSpacing,skew_angle,&back_width,&back_offset);
         }

         if ( pNextSpan )
         {
            const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
            GetPierLineProperties(pBridgeDesc,pSpacing,skew_angle,&ahead_width,&ahead_offset);
         }

         if ( back_width < ahead_width )
         {
            pier_length     = ahead_width;
            left_end_offset = ahead_offset;
         }
         else
         {
            pier_length     = back_width;
            left_end_offset = back_offset;
         }
      }
      else
      {
         Float64 w = pPier->GetColumnSpacingWidth();
         Float64 x5,x6;
         pPier->GetXBeamOverhangs(&x5,&x6);
         pier_length = w + x5 + x6;

         // update offset, which is the offset to the left edge of the pier from the alignment
         // measured along the CL Pier
         ColumnIndexType refColIdx;
         Float64 refColOffset;
         pgsTypes::OffsetMeasurementType refColOffsetMeasure;
         pPier->GetTransverseOffset(&refColIdx,&refColOffset,&refColOffsetMeasure);

         if ( refColOffsetMeasure == pgsTypes::omtBridge )
         {
            Float64 blo = pBridgeDesc->GetAlignmentOffset();
            refColOffset += blo;
         }

         left_end_offset = refColOffset - x5 - pPier->GetColumnSpacingWidthToColumn(refColIdx);
      }

      Float64 station = pPier->GetStation();
      std::_tstring strOrientation = pPier->GetOrientation();

      CComPtr<ISinglePierLineFactory> factory;
      factory.CoCreateInstance(CLSID_SinglePierLineFactory);
      factory->put_PierLineID(pierID);
      factory->put_AlignmentID(AlignmentID);
      factory->put_Station(CComVariant(station));
      factory->put_Direction(CComBSTR(strOrientation.c_str()));
      factory->put_Length(pier_length);
      factory->put_Offset(left_end_offset);
     
      CPierData2::PierConnectionFlags conFlag = pPier->IsConnectionDataAvailable();

      // Layout connection geometry on back side of pier
      if ( CPierData2::pcfBackOnly==conFlag || CPierData2::pcfBothFaces == conFlag)
      {
         auto [offset, offsetMeasureType] = pPier->GetBearingOffset(pgsTypes::Back);
         factory->put_BearingOffset( pfBack, offset );
         factory->put_BearingOffsetMeasurementType( pfBack, offsetMeasureType == ConnectionLibraryEntry::AlongGirder ? mtAlongItem : mtNormal );

         auto [endDist,endDistMeasureType] = pPier->GetGirderEndDistance(pgsTypes::Back);
         factory->put_EndDistance( pfBack, endDist );
         if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingAlongGirder )
         {
            factory->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
            factory->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingNormalToPier )
         {
            factory->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
            factory->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierAlongGirder )
         {
            factory->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
            factory->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierNormalToPier )
         {
            factory->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
            factory->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         }
      }

      // Layout connection geometry on ahead side of pier
      if (CPierData2::pcfAheadOnly == conFlag || CPierData2::pcfBothFaces == conFlag)
      {
         auto [offset, offsetMeasureType] = pPier->GetBearingOffset(pgsTypes::Ahead);
         factory->put_BearingOffset( pfAhead, offset );
         factory->put_BearingOffsetMeasurementType( pfAhead, offsetMeasureType == ConnectionLibraryEntry::AlongGirder ? mtAlongItem : mtNormal );

         auto [endDist, endDistMeasureType] = pPier->GetGirderEndDistance(pgsTypes::Ahead);
         factory->put_EndDistance( pfAhead, endDist );
         if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingAlongGirder )
         {
            factory->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
            factory->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingNormalToPier )
         {
            factory->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
            factory->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierAlongGirder )
         {
            factory->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
            factory->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierNormalToPier )
         {
            factory->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
            factory->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
         }
      }

      pBridgeGeometry->AddPierLineFactory(factory);

      const CSpanData2* pSpan = pPier->GetNextSpan();
      if ( pSpan )
      {
         pPier = pSpan->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }

      pierIdx++;
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutTemporarySupports(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
#pragma Reminder("UPDATE: Find a way to use the same Temporary Support ID for the bridge and geometry models")
      SupportIDType tsID = ::GetTempSupportLineID(tsIdx); // this is the ID of the line that represents the temporary
                                                          // support in the geometry model, not the ID of the temporary support
                                                          // in the bridge data model

      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

      Float64 station = pTS->GetStation();

      Float64 pier_width;
      Float64 left_end_offset;
      if ( pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment )
      {
         ATLASSERT(!pTS->HasSpacing());
         // no spacing at the TS... need to base width on spacing at ends of segment
         // also need to account for the fact that the ref girder can be different at each end
         // once ref girder is based on girder index 0, the ref girder offset at this TS can be computed
         CSegmentKey segmentKey, rightSegmentKey;
         pBridgeDesc->GetSegmentsAtTemporarySupport(pTS->GetIndex(),&segmentKey,&rightSegmentKey);
         ATLASSERT(segmentKey == rightSegmentKey);

         // we need a specific segment key... this segment key is for all girders
         ATLASSERT(segmentKey.girderIndex == ALL_GIRDERS);
         segmentKey.girderIndex = 0;
         const CPrecastSegmentData* pSegment = pBridgeDesc->GetSegment(segmentKey);

         const CPierData2* pStartPier;
         const CTemporarySupportData* pStartTS;
         pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

         const CGirderSpacing2* pStartSpacing;
         Float64 start_station, start_skew_angle;
         if ( pStartPier )
         {
            start_station = pStartPier->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetPierDirection(alignment,pStartPier,&skew,&direction);
            skew->get_Value(&start_skew_angle);

            pStartSpacing = pStartPier->GetGirderSpacing(pgsTypes::Ahead);
         }
         else
         {
            ATLASSERT(pStartTS);
            start_station = pStartTS->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetTempSupportDirection(alignment,pStartTS,&skew,&direction);
            skew->get_Value(&start_skew_angle);

            pStartSpacing = pStartTS->GetSegmentSpacing();
         }

         const CPierData2* pEndPier;
         const CTemporarySupportData* pEndTS;
         pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);

         const CGirderSpacing2* pEndSpacing;
         Float64 end_station, end_skew_angle;
         if ( pEndPier )
         {
            end_station = pEndPier->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetPierDirection(alignment,pEndPier,&skew,&direction);
            skew->get_Value(&end_skew_angle);

            pEndSpacing = pEndPier->GetGirderSpacing(pgsTypes::Back);
         }
         else
         {
            ATLASSERT(pEndTS);
            end_station = pEndTS->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetTempSupportDirection(alignment,pEndTS,&skew,&direction);
            skew->get_Value(&end_skew_angle);

            pEndSpacing = pEndTS->GetSegmentSpacing();
         }

         Float64 start_width, start_offset;
         Float64 end_width, end_offset;
         GetPierLineProperties(pBridgeDesc,pStartSpacing,start_skew_angle,&start_width,&start_offset);
         GetPierLineProperties(pBridgeDesc,pEndSpacing,  end_skew_angle,  &end_width,  &end_offset);

         pier_width      = ::LinInterp(station - start_station,start_width, end_width, end_station-start_station);
         left_end_offset = ::LinInterp(station - start_station,start_offset,end_offset,end_station-start_station);
      }
      else
      {
         // spacing is measured at TS
         ATLASSERT(pTS->HasSpacing());

         CComPtr<IAngle> skew;
         CComPtr<IDirection> direction;
         GetTempSupportDirection(alignment,pTS,&skew,&direction);

         Float64 skew_angle;
         skew->get_Value(&skew_angle);

         const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();
         GetPierLineProperties(pBridgeDesc,pSpacing,skew_angle,&pier_width,&left_end_offset);
      }

      std::_tstring strOrientation = pTS->GetOrientation();

      CComPtr<ISinglePierLineFactory> factory;
      factory.CoCreateInstance(CLSID_SinglePierLineFactory);
      factory->put_PierLineID(tsID);
      factory->put_AlignmentID(AlignmentID);
      factory->put_Station(CComVariant(station));
      factory->put_Direction(CComBSTR(strOrientation.c_str()));
      factory->put_Length(pier_width);
      factory->put_Offset(left_end_offset);

      auto [brgOffset,brgOffsetMeasure] = pTS->GetBearingOffset();
      factory->put_BearingOffset( pfBack, brgOffset);
      factory->put_BearingOffsetMeasurementType( pfBack, (MeasurementType)brgOffsetMeasure );
      factory->put_BearingOffset( pfAhead, brgOffset);
      factory->put_BearingOffsetMeasurementType( pfAhead, (MeasurementType)brgOffsetMeasure );

      // layout connection geometry on both sides of temporary support
      auto [endDistance, measureType] = pTS->GetGirderEndDistance();
      factory->put_EndDistance( pfBack, endDistance );
      factory->put_EndDistance( pfAhead, endDistance );
      if ( measureType == ConnectionLibraryEntry::FromBearingAlongGirder )
      {
         factory->put_EndDistanceMeasurementLocation(pfBack, mlCenterlineBearing);
         factory->put_EndDistanceMeasurementType(pfBack, mtAlongItem);
         factory->put_EndDistanceMeasurementLocation(pfAhead, mlCenterlineBearing);
         factory->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
      }
      else if ( measureType == ConnectionLibraryEntry::FromBearingNormalToPier )
      {
         factory->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
         factory->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         factory->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
         factory->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
      }
      else if ( measureType == ConnectionLibraryEntry::FromPierAlongGirder )
      {
         factory->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
         factory->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         factory->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
         factory->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
      }
      else if ( measureType == ConnectionLibraryEntry::FromPierNormalToPier )
      {
         factory->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
         factory->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         factory->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
         factory->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
      }

      pBridgeGeometry->AddPierLineFactory(factory);
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& orientationCollection)
{
   // girder layout lines are geometric construction lines that are used
   // to create the actual girder line geometry

   bool bRetVal = false;
   switch( pBridgeDesc->GetGirderSpacingType() )
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsUniformAdjacent:
   case pgsTypes::sbsConstantAdjacent:
   case pgsTypes::sbsUniformAdjacentWithTopWidth:
      bRetVal = LayoutUniformGirderLines(pBridgeDesc,pBridgeGeometry, orientationCollection);
      break;

   case pgsTypes::sbsGeneral:
   case pgsTypes::sbsGeneralAdjacent:
   case pgsTypes::sbsGeneralAdjacentWithTopWidth:
      bRetVal = LayoutGeneralGirderLines(pBridgeDesc,pBridgeGeometry, orientationCollection);
      break;

   default:
      ATLASSERT(false); // is there a new spacing type?
   }

   return bRetVal;
}

bool CBridgeGeometryModelBuilder::LayoutUniformGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& orientationCollection)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IRoadwayData, pIAlignment);
   bool bAnglePointInAlignment = false;
   const AlignmentData2& alignment_data = pIAlignment->GetAlignmentData2();
   for (const auto& hc : alignment_data.CompoundCurves)
   {
      if (IsZero(hc.Radius))
      {
         bAnglePointInAlignment = true;
         break;
      }
   }

   pgsTypes::MeasurementLocation measureLocation = pBridgeDesc->GetMeasurementLocation();
   pgsTypes::MeasurementType measureType = pBridgeDesc->GetMeasurementType();

   if ( !pBridgeDesc->UseSameNumberOfGirdersInAllGroups() ||
        !pBridgeDesc->UseSameGirderForEntireBridge()      || 
        measureType == pgsTypes::AlongItem                ||
        bAnglePointInAlignment                            ||
        pBridgeDesc->GetWorkPointLocation() != pgsTypes::wplTopGirder
      )
   {
      // if there is a different number of girders in each group, but the spacing is uniform, this is the
      // same as general spacing... use the general spacing function and return

      // OR
      
      // if a different girder types are used, the girders aren't necessarily the same width so
      // this is the same as general spacing

      // OR
      
      // the girder spacing is measured along the CL pier or CL bearing. if the piers are skewed or
      // if the alignment is curved, the girder spacing along the alignment normal will not be uniform

      // OR
      
      // this is an angle point in the alignment. the simple layout doesn't work because of how parallel
      // alignment paths get created. use the more general layout. Note, we only really need to do this
      // if there is an angle point within the limits of the bridge, but it is just easier to do the more
      // general layout if any angle points exist

      // OR 
      
      // Workpoint is not at top of girder

      return LayoutGeneralGirderLines(pBridgeDesc,pBridgeGeometry, orientationCollection);
   }

#pragma Reminder("UPDATE: adjust ref girder offset for spacing measurement type")
#pragma Reminder("UPDATE: adjust girder spacing for spacing measurement type and location")

   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();
   Float64 spacing;
   if (IsTopWidthSpacing(spacingType))
   {
      spacing = pBridgeDesc->GetGirderTopWidth() + pBridgeDesc->GetGirderSpacing();
   }
   else
   {
      spacing = pBridgeDesc->GetGirderSpacing();
   }

   if ( !IsTopWidthSpacing(spacingType) && IsJointSpacing(spacingType) )
   {
      // the spacing is a joint spacing and beam has a fixed top width (top width is not input)
      // need to add beam width

      ATLASSERT(pBridgeDesc->UseSameGirderForEntireBridge() == true);
      const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(GroupIndexType(0))->GetGirder(0);
      const GirderLibraryEntry* pGdrLibEntry = pGirder->GetGirderLibraryEntry();
      Float64 w1 = pGdrLibEntry->GetBeamWidth(pgsTypes::metStart);
      Float64 w2 = pGdrLibEntry->GetBeamWidth(pgsTypes::metEnd);
      Float64 w  = Max(w1,w2);

      spacing += w;
   }

   GirderIndexType nGirders = pBridgeDesc->GetGirderCount();

   GirderIndexType refGirderIdx = pBridgeDesc->GetRefGirder();
   Float64 refGirderOffset = pBridgeDesc->GetRefGirderOffset();
   pgsTypes::OffsetMeasurementType refGirderOffsetType = pBridgeDesc->GetRefGirderOffsetType();

   // convert ref girder offset into an offset to the left-most girder
   if ( refGirderIdx == INVALID_INDEX )
   {
      // reference girder is the center of the spacing group
      refGirderOffset -= spacing*(nGirders-1)/2;
      refGirderIdx = 0;
   }
   else
   {
      // a specific girder is the reference girder
      refGirderOffset -= spacing*refGirderIdx;
      refGirderIdx = 0;
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      // alignment offset is always measured normal to the alignment
      refGirderOffset += pBridgeDesc->GetAlignmentOffset();
   }


   // Create a girder layout line
   // Layout line is parallel to and offset from the alignment
   // Layout line IDs are the girder index
   CComPtr<IAlignmentOffsetLayoutLineFactory> factory;
   factory.CoCreateInstance(CLSID_AlignmentOffsetLayoutLineFactory);
   factory->put_LayoutLineID(0); // ID of first layout line
   factory->put_LayoutLineIDIncrement(1);
   factory->put_LayoutLineCount(nGirders);
   factory->put_Offset(refGirderOffset);  
   factory->put_OffsetIncrement(spacing); 
   factory->put_AlignmentID(AlignmentID);
   pBridgeGeometry->AddLayoutLineFactory(factory);

   //
   // Build Girder Lines
   //
   Float64 alignmentOffset = pBridgeDesc->GetAlignmentOffset();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CComPtr<ISingleGirderLineFactory> glFactory;
            HRESULT hr = glFactory.CoCreateInstance(CLSID_SingleGirderLineFactory);
            ATLASSERT(SUCCEEDED(hr));

            IDType segID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);
            glFactory->put_GirderLineID( segID );   // girder line ID
            glFactory->put_LayoutLineID( gdrIdx );  // layout line used to layout this girder line
            glFactory->put_Type(glChord);           // layout as a chord
            glFactory->put_StartMeasurementType( measureType == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_StartMeasurementLocation( measureLocation == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );
            glFactory->put_EndMeasurementType( measureType == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_EndMeasurementLocation( measureLocation == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            PierIDType startID, endID;
            GetPierID(pSegment,&startID,&endID);

            glFactory->put_StartPierID(startID);
            glFactory->put_EndPierID(endID);

            pBridgeGeometry->AddGirderLineFactory(glFactory);
         } // segment loop
      } // girder loop
   } // group loop

   // Need to compute and store girder orientations for later use in BridgeAgent
   pgsTypes::GirderOrientationType orientType = pBridgeDesc->GetGirderOrientation();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      // Rotation is defined at start of first segment and end of last segment in group
      std::vector<CComPtr<IPoint2dCollection>> startGirderPoints(2), endGirderPoints(2); // first, last
      if (nSegments == 1)
      {
         const CPrecastSegmentData* pStartSegment = pGirder->GetSegment(0);
         ResolveSegmentSpacing(pBridgeGeometry, alignmentOffset, pStartSegment, &startGirderPoints[0], &endGirderPoints[1]);
      }
      else
      {
         const CPrecastSegmentData* pStartSegment = pGirder->GetSegment(0);
         ResolveSegmentSpacing(pBridgeGeometry, alignmentOffset, pStartSegment, &startGirderPoints[0], &endGirderPoints[0]);

         const CPrecastSegmentData* pEndSegment = pGirder->GetSegment(nSegments - 1);
         ResolveSegmentSpacing(pBridgeGeometry, alignmentOffset, pEndSegment, &startGirderPoints[1], &endGirderPoints[1]);
      }

      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();

         CComPtr<IPoint2d> startPoint, endPoint;
         startGirderPoints[0]->get_Item(gdrIdx, &startPoint);
         endGirderPoints[1]->get_Item(gdrIdx, &endPoint);

         const CSplicedGirderData* pGdr = pGroup->GetGirder(gdrIdx);
         Float64 orientation = ComputeGirderOrientation(orientType, pGdr, pBridgeGeometry, startPoint, endPoint);

         // store orientation data. Note that segment shift is always zero because this function can never be called
         // if the work point is not located at the top of girder
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            orientationCollection.insert(std::make_pair(CSegmentKey(grpIdx, gdrIdx, segIdx), GirderOrientationShift(orientation, 0.0)));
         }
      }
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutGeneralGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry, GirderOrientationCollection& orientationCollection)
{
   //
   // Do some preliminary setup for the function
   //
   CComPtr<IAlignment> alignment;
   pBridgeGeometry->GetAlignment(AlignmentID,&alignment);

   Float64 alignmentOffset = pBridgeDesc->GetAlignmentOffset();

   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CComPtr<ISimpleLayoutLineFactory> factory;
      factory.CoCreateInstance(CLSID_SimpleLayoutLineFactory);

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(0); // all girders in a group have the same number of segments... girder 0 is just as good as any other girder in this group
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      GirderIndexType nGirders = pGroup->GetGirderCount();

      std::vector<CComPtr<IPoint2dCollection>> girderPoints(2*nSegments);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         ResolveSegmentSpacing(pBridgeGeometry,alignmentOffset,pSegment,&girderPoints[2*segIdx],&girderPoints[2*segIdx+1]);
      }

   #if defined _DEBUG
      // verify all girder offset arrays are the same length
      for ( IndexType i = 0; i < nSegments; i++ )
      {
         IndexType size1;
         girderPoints[2*i]->get_Count(&size1);

         IndexType size2;
         girderPoints[2*i+1]->get_Count(&size2);

         ATLASSERT(size1 == size2);
      }
   #endif // _DEBUG

      // At this point we have the end points for the girder layout lines in 2D space as defined by the girder spacing.
      // The bridge geometry models in the WBFL and in PGSuper assume that the work point is at the top of the girder. 
      // We have not yet considered the effects of work point location. If the girder is not plumb, and the work point
      // is not at the top of the girder, we must offset (shift) the layout line so we have an equivalent layout.

      // Store girder orientations and layout shift (offset) for each girder
      pgsTypes::GirderOrientationType orientType = pBridgeDesc->GetGirderOrientation();
      if (orientType == pgsTypes::Plumb)
      {
         // No work to do. Girders built here are always plumb (zero orientation angle and zero layout line shift
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
            {
               orientationCollection.insert(std::make_pair(CSegmentKey(grpIdx, gdrIdx, segIdx), GirderOrientationShift()));
            }
         }
      }
      else
      {
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            // get start and end point for group
            CComPtr<IPoint2d> startPoint, endPoint;
            girderPoints[0]->get_Item(gdrIdx, &startPoint);
            girderPoints[2 * nSegments - 1]->get_Item(gdrIdx, &endPoint);
            const CSplicedGirderData* pGdr = pGroup->GetGirder(gdrIdx);

            Float64 orientation = ComputeGirderOrientation(orientType, pGdr, pBridgeGeometry, startPoint, endPoint);

            Float64 shift(0.0);
            pgsTypes::WorkPointLocation wploc = pBridgeDesc->GetWorkPointLocation();
            if ( wploc != pgsTypes::wplTopGirder)
            {
               // this should be caught. work point must be at top of beam for variable depth girders
               ATLASSERT(!pGdr->GetGirderLibraryEntry()->IsVariableDepthSectionEnabled()); 

               if (wploc == pgsTypes::wplBottomGirder)
               {
                  Float64 beamHeight = pGdr->GetGirderLibraryEntry()->GetBeamHeight(pgsTypes::metStart);

                  shift = orientation * beamHeight;
               }
            }

            // Store orientation and shift. Same for all segments along a girder
            for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
            {
               orientationCollection.insert(std::make_pair(CSegmentKey(grpIdx, gdrIdx, segIdx), GirderOrientationShift(orientation, shift)));
            }
         }
      }

      // 
      // Create paths for layout lines
      //
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CComPtr<IPoint2d> pntStart, pntEnd;
            girderPoints[2*segIdx]->get_Item(gdrIdx,&pntStart);
            girderPoints[2*segIdx+1]->get_Item(gdrIdx,&pntEnd);

            CComPtr<IPathSegment> lineSegment;
            lineSegment.CoCreateInstance(CLSID_PathSegment);
            lineSegment->ThroughPoints(pntStart,pntEnd);

            // Shift layout line if workpoint is not at top for non-plumb girder
            CSegmentKey segkey(grpIdx, gdrIdx, segIdx);
            auto found = orientationCollection.find(segkey);
            if (found != orientationCollection.end())
            {
               lineSegment->Offset(found->second.m_LayoutLineShift);
            }
            else
            {
               ATLASSERT(0);
            }

            CComPtr<IPath> path;
            path.CoCreateInstance(CLSID_Path);
            CComQIPtr<IPathElement> element(lineSegment);
            path->Add(element);

            IDType ID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);
            factory->AddPath(ID,path);
         } // end of segment loop
      } // end of girder loop

      // create the layout lines
      // there is a layout line for each segment. The layout line ID is derived from grpIdx/gdrIdx/segIdx
      pBridgeGeometry->AddLayoutLineFactory(factory);

      //
      // Create the girder lines
      //
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            IDType ID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            PierIDType startID, endID;
            GetPierID(pSegment,&startID,&endID);

            const CPierData2* pStartPier = nullptr;
            const CTemporarySupportData* pStartTS = nullptr;
            pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

            const CPierData2* pEndPier = nullptr;
            const CTemporarySupportData* pEndTS = nullptr;
            pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);

            const CGirderSpacing2* pStartSpacing = nullptr;
            const CGirderSpacing2* pEndSpacing   = nullptr;
            if ( pStartPier )
            {
               pStartSpacing = pStartPier->GetGirderSpacing(pgsTypes::Ahead);
            }
            else
            {
               pStartSpacing = pStartTS->GetSegmentSpacing();
            }

            if ( pEndPier )
            {
               pEndSpacing = pEndPier->GetGirderSpacing(pgsTypes::Back);
            }
            else
            {
               pEndSpacing = pEndTS->GetSegmentSpacing();
            }


            CComPtr<ISingleGirderLineFactory> glFactory;
            glFactory.CoCreateInstance(CLSID_SingleGirderLineFactory);
            glFactory->put_GirderLineID(ID); // girder line ID
            glFactory->put_LayoutLineID(ID); // layout line used to layout this girder line
            glFactory->put_StartPierID( startID );
            glFactory->put_EndPierID(   endID   );
            glFactory->put_Type(glChord);
            glFactory->put_StartMeasurementType( pStartSpacing->GetMeasurementType() == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_StartMeasurementLocation( pStartSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );
            glFactory->put_EndMeasurementType( pEndSpacing->GetMeasurementType() == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_EndMeasurementLocation( pEndSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );
            pBridgeGeometry->AddGirderLineFactory(glFactory);
         }
      }
   } // end of group loop

   return true;
}

void CBridgeGeometryModelBuilder::GetPierID(const CPrecastSegmentData* pSegment,PierIDType* pStartID,PierIDType* pEndID)
{
   const CClosureJointData* pStartClosure  = pSegment->GetClosureJoint(pgsTypes::metStart);
   const CClosureJointData* pEndClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

   if ( pStartClosure == nullptr )
   {
      *pStartID = ::GetPierLineID( pSegment->GetGirder()->GetPier(pgsTypes::metStart)->GetIndex() );
   }
   else
   {
      if ( pStartClosure->GetPier() )
      {
         *pStartID = ::GetPierLineID( pStartClosure->GetPier()->GetIndex() );
      }
      else
      {
         *pStartID = ::GetTempSupportLineID( pStartClosure->GetTemporarySupport()->GetIndex() );
      }
   }


   if ( pEndClosure == nullptr )
   {
      *pEndID = ::GetPierLineID( pSegment->GetGirder()->GetPier(pgsTypes::metEnd)->GetIndex() );
   }
   else
   {
      if ( pEndClosure->GetPier() )
      {
         *pEndID = ::GetPierLineID( pEndClosure->GetPier()->GetIndex() );
      }
      else
      {
         *pEndID = ::GetTempSupportLineID( pEndClosure->GetTemporarySupport()->GetIndex() );
      }
   }
}

void CBridgeGeometryModelBuilder::GetPierDirection(IAlignment* pAlignment,const CPierData2* pPier,IAngle** ppSkew,IDirection** ppDirection)
{
   Float64 station = pPier->GetStation();
   LPCTSTR strOrientation = pPier->GetOrientation();

   GetSkewAndDirection(pAlignment,station,strOrientation,ppSkew,ppDirection);
}

void CBridgeGeometryModelBuilder::GetTempSupportDirection(IAlignment* pAlignment,const CTemporarySupportData* pTS,IAngle** ppSkew,IDirection** ppDirection)
{
   Float64 station = pTS->GetStation();
   LPCTSTR strOrientation = pTS->GetOrientation();
   GetSkewAndDirection(pAlignment,station,strOrientation,ppSkew,ppDirection);
}

void CBridgeGeometryModelBuilder::GetSkewAndDirection(IAlignment* pAlignment,Float64 station,LPCTSTR strOrientation,IAngle** ppSkew,IDirection** ppDirection)
{
   // get pier direction and skew angle
   // this is needed to determine the pier width and offset measured along CL pier
   pAlignment->GetDirection(CComVariant(station),CComBSTR(strOrientation),ppDirection);

   CComPtr<IDirection> normal; // normal to the alignment
   pAlignment->GetNormal(CComVariant(station),&normal);
   normal->IncrementBy(CComVariant(M_PI)); // We want the normal to the left... Increment by 180 degrees

   (*ppDirection)->AngleBetween(normal,ppSkew);
}

bool CBridgeGeometryModelBuilder::LayoutDiaphragmLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
#pragma Reminder("IMPLEMENT")
   // need to add diaphragms to the bridge geometry model
   // though there is no point in doing this unless the generic bridge model or
   // the bridge agent make use of the diaphragm geometry.
   //
   // also, the software can be updated to accommodate more general diaphragm layouts including
   // staggered diaphragm lines.
   return true;
}

void CBridgeGeometryModelBuilder::GetSpacingDataAtPier(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CPierData2* pPier,pgsTypes::PierFaceType pierFace,
                                                       Float64* pMeasureStation,IDirection** ppMeasureDirection,IDirection** ppSupportDirection,const CGirderSpacing2** ppSpacing)
{
   const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pierFace);
   Float64 measureStation;
   Float64 pierStation = pPier->GetStation();

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   CComPtr<IAngle> skew;
   CComPtr<IDirection> pierDirection;
   GetPierDirection(alignment,pPier,&skew,&pierDirection);

   if ( pSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine )
   {
      // Spacing measured at centerline of pier
      measureStation = pierStation;
   }
   else
   {
      // Spacing measured at centerline of bearing
      // Support station is where a line parallel to the CL Pier, offset by +/- bearing offset, 
      // intersects the alignment

      // Bearing offset must be adjusted for how it is measured
      auto [brgOffset, measureType] = pPier->GetBearingOffset(pierFace);

      if ( measureType == ConnectionLibraryEntry::AlongGirder)
      {
         // assumes CL girder is parallel to the alignment
         Float64 skew_angle;
         skew->get_Value(&skew_angle);
         brgOffset *= cos(skew_angle);
      }

      PierIDType pierID = ::GetPierLineID(pPier->GetIndex());
      CComPtr<IPierLine> pierLine;
      pBridgeGeometry->FindPierLine( pierID, &pierLine );

      CComPtr<ILine2d> centerlinePier;
      pierLine->get_Centerline(&centerlinePier);

      CComPtr<IGeomUtil2d> geomUtil;
      geomUtil.CoCreateInstance(CLSID_GeomUtil);

      Float64 sign = (pierFace == pgsTypes::Back ? -1 : 1);

      CComPtr<ILine2d> brgLine;
      geomUtil->CreateParallelLine(centerlinePier,sign*brgOffset,&brgLine);

      CComPtr<IPoint2d> pntAlignment;
      pierLine->get_AlignmentPoint(&pntAlignment);

      CComPtr<IPoint2d> pnt;
      alignment->Intersect(brgLine,pntAlignment,&pnt);

      CComPtr<IStation> station;
      Float64 offset;
      alignment->StationAndOffset(pnt,&station,&offset);

      ATLASSERT(IsZero(offset));
      alignment->ConvertToNormalizedStation(CComVariant(station), &measureStation);
   }

   CComPtr<IDirection> measureDirection;
   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // spacing is measured normal to the alignment
      alignment->GetNormal(CComVariant(measureStation),&measureDirection);
   }
   else
   {
      // spacing is measured along CL Pier or CL Bearing
      // CL Bearing is parallel to CL Pier so always measure direction at the CL Pier station
      alignment->GetDirection(CComVariant(pierStation),CComBSTR(pPier->GetOrientation()),&measureDirection);
   }

   *pMeasureStation = measureStation;
   measureDirection.CopyTo(ppMeasureDirection);
   pierDirection.CopyTo(ppSupportDirection);
   *ppSpacing = pSpacing;
}

void CBridgeGeometryModelBuilder::GetSpacingDataAtTempSupport(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CTemporarySupportData* pTS,
                                                       Float64* pMeasureStation,IDirection** ppMeasureDirection,IDirection** ppSupportDirection,const CGirderSpacing2** ppSpacing)
{
   const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();

   // spacing is always measured at CL temporary support
   ATLASSERT(pSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine);

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   CComPtr<IAngle> skew;
   CComPtr<IDirection> tsDirection;
   GetTempSupportDirection(alignment,pTS,&skew,&tsDirection);

   Float64 measureStation = pTS->GetStation();
   CComPtr<IDirection> measureDirection;
   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // spacing is measured normal to the alignment
      alignment->GetNormal(CComVariant(measureStation),&measureDirection);
   }
   else
   {
      // spacing is measured along CL Temp Support
      measureDirection = tsDirection;
   }

   *pMeasureStation = measureStation;
   measureDirection.CopyTo(ppMeasureDirection);
   tsDirection.CopyTo(ppSupportDirection);
   *ppSpacing = pSpacing;
}

void CBridgeGeometryModelBuilder::ResolveSegmentSpacing(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CPrecastSegmentData* pSegment,IPoint2dCollection** ppStartPoints,IPoint2dCollection** ppEndPoints)
{
   const CGirderSpacing2* pStartSpacing;
   const CGirderSpacing2* pEndSpacing;

   Float64 startMeasureStation, endMeasureStation;
   CComPtr<IDirection> startMeasureDirection, endMeasureDirection;
   CComPtr<IDirection> startSupportDirection, endSupportDirection;


   const CPierData2* pStartPier = nullptr;
   const CTemporarySupportData* pStartTS = nullptr;
   pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);
   if ( pStartPier )
   {
      GetSpacingDataAtPier(pBridgeGeometry,alignmentOffset,pStartPier,pgsTypes::Ahead,&startMeasureStation,&startMeasureDirection,&startSupportDirection,&pStartSpacing);
   }
   else
   {
      GetSpacingDataAtTempSupport(pBridgeGeometry,alignmentOffset,pStartTS,&startMeasureStation,&startMeasureDirection,&startSupportDirection,&pStartSpacing);
   }

   const CPierData2* pEndPier = nullptr;
   const CTemporarySupportData* pEndTS = nullptr;
   pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);
   if ( pEndPier )
   {
      GetSpacingDataAtPier(pBridgeGeometry,alignmentOffset,pEndPier,pgsTypes::Back,&endMeasureStation,&endMeasureDirection,&endSupportDirection,&pEndSpacing);
   }
   else
   {
      GetSpacingDataAtTempSupport(pBridgeGeometry,alignmentOffset,pEndTS,&endMeasureStation,&endMeasureDirection,&endSupportDirection,&pEndSpacing);
   }

   ResolveSegmentSpacing(pBridgeGeometry,alignmentOffset,
                         startMeasureStation,
                         startMeasureDirection,
                         startSupportDirection,
                         pStartSpacing,
                         endMeasureStation,
                         endMeasureDirection,
                         endSupportDirection,
                         pEndSpacing,
                         ppStartPoints,
                         ppEndPoints);
}

Float64 CBridgeGeometryModelBuilder::GetSkewAngle(IAlignment* pAlignment,Float64 measureStation,IDirection* pMeasureDirection)
{
   CComPtr<IDirection> normal;
   pAlignment->GetNormal(CComVariant(measureStation),&normal);
   normal->IncrementBy(CComVariant(M_PI));

   CComPtr<IAngle> angle;
   normal->AngleBetween(pMeasureDirection,&angle);

   Float64 skew;
   angle->get_Value(&skew);
   return skew;
}

Float64 CBridgeGeometryModelBuilder::GetLeftGirderOffset(IAlignment* pAlignment,
                                                         Float64 alignmentOffset,
                                                         Float64 measureStation,
                                                         IDirection* pMeasureDirection,
                                                         const CGirderSpacing2* pSpacing)
{
   // returns the offset from the alignment to the left-most girder

   Float64 skew = GetSkewAngle(pAlignment, measureStation, pMeasureDirection);

   GirderIndexType refGirderIdx = pSpacing->GetRefGirder();
   Float64 refGirderOffset = pSpacing->GetRefGirderOffset();
   pgsTypes::OffsetMeasurementType refGirderOffsetType = pSpacing->GetRefGirderOffsetType();

   // convert ref girder offset into an offset to the left-most girder
   if ( refGirderIdx == INVALID_INDEX )
   {
      // reference girder is the center of the spacing group
      Float64 width = pSpacing->GetSpacingWidth(skew);
      refGirderOffset -= width/2;
      refGirderIdx = 0;
   }
   else
   {
      // a specific girder is the reference girder
      Float64 width = pSpacing->GetSpacingWidthToGirder(refGirderIdx,skew);
      refGirderOffset -= width;
      refGirderIdx = 0;
   }

   // convert offset so that it is measured along the centerline of the support
   pgsTypes::MeasurementType measureType = pSpacing->GetMeasurementType();
   if ( measureType == pgsTypes::NormalToItem )
   {
      // spacing is normal to alignment
      // get skew angle and adjust the ref girder offset
      refGirderOffset /= cos(skew);
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      refGirderOffset += alignmentOffset/cos(skew);
   }

   return refGirderOffset;
}

void CBridgeGeometryModelBuilder::ResolveSegmentSpacing(IBridgeGeometry* pBridgeGeometry,
                                                        Float64 alignmentOffset,
                                                        Float64 startMeasureStation,
                                                        IDirection* pStartMeasureDirection,
                                                        IDirection* pStartSupportDirection,
                                                        const CGirderSpacing2* pStartSpacing,
                                                        Float64 endMeasureStation,
                                                        IDirection* pEndMeasureDirection,
                                                        IDirection* pEndSupportDirection,
                                                        const CGirderSpacing2* pEndSpacing,
                                                        IPoint2dCollection** ppStartPoints,
                                                        IPoint2dCollection** ppEndPoints)
{
   // NOTE: supportStation is the station where the girder spacing is measured. It could be at the
   // center of the support or at the centerline of bearing. You must check the girder spacing object
   // and use the appropriate value

   if ( *ppStartPoints == nullptr )
   {
      // if array is not given, create it
      CComPtr<IPoint2dCollection> points;
      points.CoCreateInstance(CLSID_Point2dCollection);
      points.CopyTo(ppStartPoints);
   }
   else
   {
      // otherwise clear it
      (*ppStartPoints)->Clear();
   }

   if ( *ppEndPoints == nullptr )
   {
      // if array is not given, create it
      CComPtr<IPoint2dCollection> points;
      points.CoCreateInstance(CLSID_Point2dCollection);
      points.CopyTo(ppEndPoints);
   }
   else
   {
      // otherwise clear it
      (*ppEndPoints)->Clear();
   }

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   //
   // Get distance from alignment to left-most girder
   //

   Float64 startRefGirderOffset = GetLeftGirderOffset(alignment, alignmentOffset, startMeasureStation, pStartMeasureDirection, pStartSpacing);
   Float64 endRefGirderOffset   = GetLeftGirderOffset(alignment, alignmentOffset, endMeasureStation,   pEndMeasureDirection,   pEndSpacing);


   //
   // Using distance from the alignment to each girder and the support direction, locate
   // the intersection of the centerline of the support and girder
   //

   CComPtr<ICogoEngine> cogoEngine;
   cogoEngine.CoCreateInstance(CLSID_CogoEngine);

   CComPtr<ILocate2> locate;
   cogoEngine->get_Locate(&locate);

   CComPtr<IPoint2d> startAlignmentPnt;
   alignment->LocatePoint(CComVariant(startMeasureStation),omtNormal,0.0,CComVariant(0),&startAlignmentPnt);

   CComPtr<IPoint2d> pntOnStartMeasurementLine;
   locate->ByDistDir(startAlignmentPnt,-startRefGirderOffset,CComVariant(pStartMeasureDirection),0.0,&pntOnStartMeasurementLine);


   CComPtr<IPoint2d> endAlignmentPnt;
   alignment->LocatePoint(CComVariant(endMeasureStation),omtNormal,0.0,CComVariant(0),&endAlignmentPnt);

   CComPtr<IPoint2d> pntOnEndMeasurementLine;
   locate->ByDistDir(endAlignmentPnt,-endRefGirderOffset,CComVariant(pEndMeasureDirection),0.0,&pntOnEndMeasurementLine);

   CComPtr<IMeasure2> measure;
   cogoEngine->get_Measure(&measure);
   CComPtr<IDirection> dirGirder;
   measure->Direction(pntOnStartMeasurementLine,pntOnEndMeasurementLine,&dirGirder);

   CComPtr<IIntersect2> intersect;
   cogoEngine->get_Intersect(&intersect);

   // intersect a line passing through the two points on the girder line with the measurement lines
   CComPtr<IPoint2d> startPnt;
   intersect->Bearings(pntOnStartMeasurementLine,CComVariant(dirGirder),0.0,startAlignmentPnt,CComVariant(pStartSupportDirection),0.0,&startPnt);
   (*ppStartPoints)->Add(startPnt);

   CComPtr<IPoint2d> endPnt;
   intersect->Bearings(pntOnEndMeasurementLine,CComVariant(dirGirder),0.0,endAlignmentPnt,CComVariant(pEndSupportDirection),0.0,&endPnt);
   (*ppEndPoints)->Add(endPnt);

   const CBridgeDescription2* pBridgeDesc = pStartSpacing->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   Float64 start_offset = startRefGirderOffset;
   Float64 end_offset   = endRefGirderOffset;
   Float64 start_skew = (pStartSpacing->GetMeasurementType() == pgsTypes::NormalToItem ? GetSkewAngle(alignment,startMeasureStation,pStartMeasureDirection) : 0);
   Float64 end_skew   = (pEndSpacing->GetMeasurementType()   == pgsTypes::NormalToItem ? GetSkewAngle(alignment,endMeasureStation,  pEndMeasureDirection) : 0);
   Float64 start_cosine_skew = cos(start_skew);
   Float64 end_cosine_skew   = cos(end_skew);
   SpacingIndexType nSpaces = pStartSpacing->GetSpacingCount();
   ATLASSERT(nSpaces == pEndSpacing->GetSpacingCount());
   for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
   {
      Float64 start_space = pStartSpacing->GetGirderSpacing(spaceIdx);
      start_space /= start_cosine_skew;
      start_offset += start_space;

      Float64 end_space = pEndSpacing->GetGirderSpacing(spaceIdx);
      end_space /= end_cosine_skew;
      end_offset += end_space;

      if ( ::IsJointSpacing(spacingType) )
      {
         // spacing is a joint spacing
         // need to add the left girder right width and the right girder left width on each side of the joint
         // (can't assume layout is based on width/2 because of asymmetric girders)
         GirderIndexType leftGdrIdx  = spaceIdx;
         GirderIndexType rightGdrIdx = leftGdrIdx + 1;

         const CGirderGroupData* pGroup = GetGirderGroup(pStartSpacing);

         const CSplicedGirderData* pLeftGirder  = pGroup->GetGirder(leftGdrIdx);
         const CSplicedGirderData* pRightGirder = pGroup->GetGirder(rightGdrIdx);

         Float64 left_girder_left_width[2], left_girder_right_width[2];
         Float64 right_girder_left_width[2], right_girder_right_width[2];
         GetGirderWidth(pLeftGirder,  pgsTypes::metStart, &left_girder_left_width[pgsTypes::metStart],  &left_girder_right_width[pgsTypes::metStart]);
         GetGirderWidth(pRightGirder, pgsTypes::metStart, &right_girder_left_width[pgsTypes::metStart], &right_girder_right_width[pgsTypes::metStart]);

         GetGirderWidth(pLeftGirder, pgsTypes::metEnd, &left_girder_left_width[pgsTypes::metEnd], &left_girder_right_width[pgsTypes::metEnd]);
         GetGirderWidth(pRightGirder, pgsTypes::metEnd, &right_girder_left_width[pgsTypes::metEnd], &right_girder_right_width[pgsTypes::metEnd]);

         Float64 start_width = left_girder_right_width[pgsTypes::metStart] + right_girder_left_width[pgsTypes::metStart];
         Float64 end_width   = left_girder_right_width[pgsTypes::metEnd] + right_girder_left_width[pgsTypes::metEnd];


         // this is not the skew angle we want... width is measured normal to the CL segment
         // the skew angle we need is the angle between the support line and the CL segment
         // this skew angle is the one the measure line makes with the alignment

         // use startPnt and endPnt to get the direction of the girder
         // then get the angle between the girder line direction and the pStartMeasureDirection and pEndMeasureDirection
         // this is the skew angle we want
         //CComPtr<IMeasure2> measure;
         //cogoEngine->get_Measure(&measure);

         CComPtr<IDirection> gdrDirection;
         measure->Direction(startPnt,endPnt,&gdrDirection);
         
         // we want the angle between the normal to the measurement line and the CL Girder.
         // It is easier to rotate the CL girder by 90 degrees, than to rotate the two
         // measurement lines... additionally, the measurement lines are used for other purposes
         // so we would have to rotate them back as well.
         gdrDirection->IncrementBy(CComVariant(PI_OVER_2));

         CComPtr<IAngle> startAngle, endAngle;
         pStartMeasureDirection->AngleBetween(gdrDirection,&startAngle);
         pEndMeasureDirection->AngleBetween(gdrDirection,&endAngle);

         Float64 start_girder_skew;
         startAngle->get_Value(&start_girder_skew);

         Float64 end_girder_skew;
         endAngle->get_Value(&end_girder_skew);

         start_offset += start_width/cos(start_girder_skew);
         end_offset   += end_width/cos(end_girder_skew);
      }

      pntOnStartMeasurementLine.Release();
      locate->ByDistDir(startAlignmentPnt,-start_offset,CComVariant(pStartMeasureDirection),0.0,&pntOnStartMeasurementLine);

      pntOnEndMeasurementLine.Release();
      locate->ByDistDir(endAlignmentPnt,-end_offset,CComVariant(pEndMeasureDirection),0.0,&pntOnEndMeasurementLine);

      dirGirder.Release();
      measure->Direction(pntOnStartMeasurementLine,pntOnEndMeasurementLine,&dirGirder);

      startPnt.Release();
      intersect->Bearings(pntOnStartMeasurementLine,CComVariant(dirGirder),0.0,startAlignmentPnt,CComVariant(pStartSupportDirection),0.0,&startPnt);

      (*ppStartPoints)->Add(startPnt);


      pntOnEndMeasurementLine.Release();
      locate->ByDistDir(endAlignmentPnt,-end_offset,CComVariant(pEndMeasureDirection),0.0,&pntOnEndMeasurementLine);

      endPnt.Release();
      intersect->Bearings(pntOnEndMeasurementLine,CComVariant(dirGirder),0.0,endAlignmentPnt,CComVariant(pEndSupportDirection),0.0,&endPnt);

      (*ppEndPoints)->Add(endPnt);
   }
}

Float64 CBridgeGeometryModelBuilder::ComputeGirderOrientation(pgsTypes::GirderOrientationType orientType, const CSplicedGirderData* pGirder, IBridgeGeometry* pBridgeGeometry,
                                                              IPoint2d* pStartPoint, IPoint2d* pEndPoint)
{
   Float64 orientation(0.0);

   CComPtr<IAlignment> pAlignment;
   pBridgeGeometry->GetAlignment(AlignmentID,&pAlignment);
   CComPtr<IProfile> pProfile;
   pAlignment->GetProfile(ProfileID,&pProfile);

   // Orientation angle is based on elevations at top flange edges of the girder group
   // Create line segment along girder. We will offset this to get locations for elevation measurements
   // Don't mess with this object because it owns our caller's points
   CComPtr<ILineSegment2d> gdrLine;
   gdrLine.CoCreateInstance(CLSID_LineSegment2d);
   gdrLine->put_StartPoint(pStartPoint);
   gdrLine->put_EndPoint(pEndPoint);

   // Create a working line we can change
   CComPtr<ILineSegment2d> offsetLine;
   gdrLine->Clone(&offsetLine);

   // Get elevations we will compute slope from
   if (pgsTypes::Plumb == orientType)
   {
      return 0.0;
   }
   else if (pgsTypes::Balanced == orientType)
   {
      // Want top flange to hit at high points at both ends
      Float64 leftWidthStart, rightWidthStart, leftWidthEnd, rightWidthEnd;;
      pGirder->GetGirderLibraryEntry()->GetBeamTopWidth(pgsTypes::metStart, &leftWidthStart, &rightWidthStart);
      pGirder->GetGirderLibraryEntry()->GetBeamTopWidth(pgsTypes::metEnd, &leftWidthEnd, &rightWidthEnd);
      // Use an average width to normalize slope
      Float64 leftWidth = (leftWidthStart + leftWidthEnd) / 2.0;
      Float64 rightWidth = (rightWidthStart + rightWidthEnd) / 2.0;

      // Start end
      CComPtr<IStation> start_station;
      Float64 start_offset;
      pAlignment->StationAndOffset(pStartPoint, &start_station, &start_offset);

      Float64 start_leftElevation, start_ClElevation, start_rightElevation;
      pProfile->Elevation(SurfaceID, CComVariant(start_station), start_offset - leftWidth , &start_leftElevation);
      pProfile->Elevation(SurfaceID, CComVariant(start_station), start_offset             , &start_ClElevation);
      pProfile->Elevation(SurfaceID, CComVariant(start_station), start_offset + rightWidth, &start_rightElevation);

      // we want elevation changes relative to CL girder
      start_leftElevation -= start_ClElevation;
      start_rightElevation -= start_ClElevation;

      // End end
      CComPtr<IStation> end_station;
      Float64 end_offset;
      pAlignment->StationAndOffset(pEndPoint, &end_station, &end_offset);

      Float64 end_leftElevation, end_ClElevation, end_rightElevation;
      pProfile->Elevation(SurfaceID, CComVariant(end_station), end_offset - leftWidth, &end_leftElevation);
      pProfile->Elevation(SurfaceID, CComVariant(end_station), end_offset, &end_ClElevation);
      pProfile->Elevation(SurfaceID, CComVariant(end_station), end_offset + rightWidth, &end_rightElevation);

      end_leftElevation -= end_ClElevation;
      end_rightElevation -= end_ClElevation;

      // Lowest elevation at either end is where girder will top out
      Float64 leftElevation = min(start_leftElevation,   end_leftElevation);
      Float64 rightElevation = min(start_rightElevation, end_rightElevation);

      // Now we can compute slope (orientation)
      Float64 distApart = leftWidth + rightWidth;
      if (IsZero(distApart))
      {
         ATLASSERT(false); // this is likely a symptom of a problem
         pProfile->CrossSlope(SurfaceID, CComVariant(start_station), start_offset, &orientation);
      }
      else
      {
         orientation = (leftElevation - rightElevation) / distApart;
      }
   }
   else
   {
      // Orientation is normal at start, mid, or end. Treat similarly
      CComPtr<IStation> station;
      Float64 offset;
      Float64 leftWidth, rightWidth;

      if (pgsTypes::StartNormal == orientType)
      {
         // Orientation defined at start of girder
         pGirder->GetGirderLibraryEntry()->GetBeamTopWidth(pgsTypes::metStart, &leftWidth, &rightWidth);

         // station and offset at start of girder
         pAlignment->StationAndOffset(pStartPoint, &station, &offset);
      }
      else if (pgsTypes::EndNormal == orientType)
      {
         // Orientation defined at end of girder
         pGirder->GetGirderLibraryEntry()->GetBeamTopWidth(pgsTypes::metEnd, &leftWidth, &rightWidth);
         pAlignment->StationAndOffset(pEndPoint, &station, &offset);
      }
      else if (pgsTypes::MidspanNormal == orientType)
      {
         // Orientation defined at mid-span of girder. use average width
         Float64 leftWidthStart, rightWidthStart, leftWidthEnd, rightWidthEnd;;
         pGirder->GetGirderLibraryEntry()->GetBeamTopWidth(pgsTypes::metStart, &leftWidthStart, &rightWidthStart);
         pGirder->GetGirderLibraryEntry()->GetBeamTopWidth(pgsTypes::metEnd, &leftWidthEnd, &rightWidthEnd);
         leftWidth = (leftWidthStart + leftWidthEnd) / 2.0;
         rightWidth = (rightWidthStart + rightWidthEnd) / 2.0;;

         // Mid point along girder line
         CComPtr<IPoint2d> pMidPoint;
         GetMidPoint(pStartPoint, pEndPoint, &pMidPoint);

         pAlignment->StationAndOffset(pMidPoint, &station, &offset);
      }
      else
      {
         ATLASSERT(0); // new orientation type?
      }

      // compute slope based on elevations at left and right side
      Float64 leftElevation, rightElevation;
      pProfile->Elevation(SurfaceID, CComVariant(station), offset - leftWidth, &leftElevation);
      pProfile->Elevation(SurfaceID, CComVariant(station), offset + rightWidth, &rightElevation);

      Float64 distApart = leftWidth + rightWidth;
      if (IsZero(distApart))
      {
         pProfile->CrossSlope(SurfaceID, CComVariant(station), offset, &orientation);
      }
      else
      {
         orientation = (leftElevation - rightElevation) / distApart;
      }
   }

   return orientation;
}


const CGirderGroupData* CBridgeGeometryModelBuilder::GetGirderGroup(const CGirderSpacing2* pSpacing)
{
   const CSpanData2* pSpan;
   const CPierData2* pPier = pSpacing->GetPier();
   if ( pPier )
   {
      if ( pPier->GetGirderSpacing(pgsTypes::Back) == pSpacing )
      {
         pSpan = pPier->GetPrevSpan();
      }
      else
      {
         pSpan = pPier->GetNextSpan();
      }
   }
   else
   {
      const CTemporarySupportData* pTS = pSpacing->GetTemporarySupport();
      pSpan = pTS->GetSpan();
   }

   const CGirderGroupData* pGroup = pSpacing->GetBridgeDescription()->GetGirderGroup(pSpan);
   return pGroup;
}

void CBridgeGeometryModelBuilder::GetGirderWidth(const CSplicedGirderData* pGirder,pgsTypes::MemberEndType endType,Float64* pLeft,Float64* pRight)
{
   if (IsTopWidthSpacing(pGirder->GetGirderGroup()->GetBridgeDescription()->GetGirderSpacingType()))
   {
      pGirder->GetTopWidth(endType, pLeft, pRight);
   }
   else
   {
      const GirderLibraryEntry* pLibEntry = pGirder->GetGirderLibraryEntry();
      Float64 w = pLibEntry->GetBeamWidth(endType);
      *pLeft = w / 2;
      *pRight = *pLeft;
   }
}

void CBridgeGeometryModelBuilder::GetPierLineProperties(const CBridgeDescription2* pBridgeDesc,const CGirderSpacing2* pSpacing,Float64 skewAngle,Float64* pWidth,Float64* pOffset)
{
   Float64 width = pSpacing->GetSpacingWidth(skewAngle);

   Float64 cosine_skew_angle = cos(skewAngle);

   GirderIndexType refGirderIdx = pSpacing->GetRefGirder();
   Float64 refGirderOffset = pSpacing->GetRefGirderOffset();
   pgsTypes::OffsetMeasurementType refGirderOffsetType = pSpacing->GetRefGirderOffsetType();

   // convert ref girder offset into an offset to the left-most girder
   if ( refGirderIdx == INVALID_INDEX )
   {
      // reference girder is the center of the spacing group
      refGirderOffset -= width/2;
      refGirderIdx = 0;
   }
   else
   {
      // a specific girder is the reference girder
      refGirderOffset -= pSpacing->GetSpacingWidthToGirder(refGirderIdx, skewAngle);
      refGirderIdx = 0;
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      Float64 alignment_offset = pBridgeDesc->GetAlignmentOffset();

      if ( pSpacing->GetMeasurementType() == pgsTypes::AlongItem )
      {
         alignment_offset /= cosine_skew_angle;
      }

      refGirderOffset += alignment_offset;
   }

   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // convert spacing to along pier
      width           /= cosine_skew_angle;
      refGirderOffset /= cosine_skew_angle;
   }

   *pWidth = width;
   *pOffset = refGirderOffset;
}
