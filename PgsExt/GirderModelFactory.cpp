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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\GirderModelFactory.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\AnalysisResults.h>
#include <iterator>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PoiIDType pgsGirderModelFactory::ms_FemModelPoiID = 0;

pgsGirderModelFactory::pgsGirderModelFactory(void)
{
}

pgsGirderModelFactory::~pgsGirderModelFactory(void)
{
}

void pgsGirderModelFactory::CreateGirderModel(IBroker* pBroker, // broker to access PGSuper data
                                 IntervalIndexType intervalIdx, // used for looking up section properties and section transition POIs
                                 const CSegmentKey& segmentKey, // this is the segment that the modeling is build for
                                 Float64 leftSupportLoc,        // distance from the left end of the model to the left support location
                                 Float64 rightSupportLoc,       // distance from the right end of the model to the right support location
                                 Float64 segmentLength,         // length of the segment
                                 Float64 E,                     // modulus of elasticity
                                 LoadCaseIDType lcidGirder,     // load case ID that is to be used to define the girder dead load
                                 bool bModelLeftCantilever,     // if true, the cantilever defined by leftSupportLoc is modeled
                                 bool bModelRightCantilever,    // if true, the cantilever defined by rightSupportLoc is modeled
                                 const PoiList& vPoi,           // vector of PGSuper POIs that are to be modeld in the Fem2d Model
                                 IFem2dModel** ppModel,         // the Fem2d Model
                                 pgsPoiPairMap* pPoiMap         // a mapping of PGSuper POIs to Fem2d POIs
                                 )
{
#if defined _DEBUG
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 _SegmentLength = pBridge->GetSegmentLength(segmentKey);
   ATLASSERT(IsEqual(segmentLength, _SegmentLength));
#endif

   // Build the model... always model the cantilevers in the geometry of the FEM model
   BuildModel(pBroker, intervalIdx, segmentKey, segmentLength, leftSupportLoc, rightSupportLoc, E, lcidGirder, vPoi, ppModel);

   ApplyLoads(pBroker, segmentKey, segmentLength, leftSupportLoc, rightSupportLoc, E, lcidGirder, bModelLeftCantilever, bModelRightCantilever, vPoi, ppModel);

   ApplyPointsOfInterest(pBroker, segmentKey, leftSupportLoc, rightSupportLoc, E, lcidGirder, bModelLeftCantilever, bModelRightCantilever, vPoi, ppModel, pPoiMap);
}

void pgsGirderModelFactory::BuildModel(IBroker* pBroker, IntervalIndexType intervalIdx, const CSegmentKey& segmentKey,
   Float64 segmentLength, Float64 leftSupportLoc, Float64 rightSupportLoc, Float64 E,
   LoadCaseIDType lcidGirder, const PoiList& vPOI, IFem2dModel** ppModel)
{
   if (*ppModel)
   {
      (*ppModel)->Clear();
   }
   else
   {
      CComPtr<IFem2dModel> model;
      model.CoCreateInstance(CLSID_Fem2dModel);
      (*ppModel) = model;
      (*ppModel)->AddRef();
   }

   // This is the same tolerance as used to build LBAM's in CGirderModelManager::BuildLBAM()
   (*ppModel)->put_ForceEquilibriumTolerance(::ConvertToSysUnits(0.25, unitMeasure::Kip));
   (*ppModel)->put_MomentEquilibriumTolerance(::ConvertToSysUnits(0.25, unitMeasure::KipFeet));

   // get all the cross section changes
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   PoiList xsPOI;
   pPoi->GetPointsOfInterest(segmentKey, POI_SECTCHANGE,&xsPOI);
   pPoi->RemovePointsOfInterest(xsPOI,POI_ERECTED_SEGMENT,POI_CANTILEVER);

   xsPOI.erase(std::remove_if(xsPOI.begin(), xsPOI.end(), [Ls = segmentLength](const pgsPointOfInterest& poi) {return !InRange(0.0, poi.GetDistFromStart(), Ls); }), xsPOI.end());// make sure no out of bound poi's sneak in

   // sometimes we loose the released segment POI at 0L and 1.0L in the call to RemovePointsOfInterest above
   // these are key POI so include them here so we are guarenteed to have them.
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE,&vPoi);
   ATLASSERT(vPoi.size() == 1);
   if ( xsPOI.empty() || !vPoi.front().get().AtExactSamePlace(xsPOI.front()) ) // don't add duplicates
   {
      xsPOI.insert(xsPOI.begin(),vPoi.front());
   }

   vPoi.clear();
   pPoi->GetPointsOfInterest(segmentKey, POI_END_FACE,&vPoi);
   ATLASSERT(vPoi.size() == 1);
   if ( !vPoi.front().get().AtExactSamePlace(xsPOI.back()) ) // don't add duplicates
   {
      xsPOI.push_back(vPoi.front());
   }

   // add support locations if there aren't already POIs at those locations
   std::vector<pgsPointOfInterest> vXSPoi;
   MakePoiVector(xsPOI,&vXSPoi);

   bool bLeftSupportLoc = true;
   bool bRightSupportLoc = true;
   std::vector<pgsPointOfInterest>::iterator xsIter(vXSPoi.begin());
   std::vector<pgsPointOfInterest>::iterator xsIterEnd(vXSPoi.end());
   for (; xsIter != xsIterEnd && (bLeftSupportLoc == true || bRightSupportLoc == true); xsIter++)
   {
      pgsPointOfInterest& poi(*xsIter);
      if (IsEqual(leftSupportLoc, poi.GetDistFromStart()))
      {
         bLeftSupportLoc = false;
         PoiAttributeType attribute = poi.GetNonReferencedAttributes();
         attribute |= POI_INTERMEDIATE_PIER;
         poi.SetNonReferencedAttributes(attribute);
      }

      if (IsEqual(rightSupportLoc, poi.GetDistFromStart()))
      {
         bRightSupportLoc = false;
         PoiAttributeType attribute = poi.GetNonReferencedAttributes();
         attribute |= POI_INTERMEDIATE_PIER;
         poi.SetNonReferencedAttributes(attribute);
      }
   }

   if (bLeftSupportLoc)
   {
      vXSPoi.push_back(pgsPointOfInterest(segmentKey, leftSupportLoc,POI_INTERMEDIATE_PIER));
   }

   if (bRightSupportLoc)
   {
      vXSPoi.push_back(pgsPointOfInterest(segmentKey, rightSupportLoc,POI_INTERMEDIATE_PIER));
   }

   // sort the POI
   std::sort(vXSPoi.begin(), vXSPoi.end());

   // This is the same tolerance as used in the LBAM's use of LAYOUT_TOLERANCE
   Float64 otol = vXSPoi.back().GetDistFromStart() / 10000.00;

   // eliminate any very close duplicates, but not support members
   auto rightIter(std::begin(vXSPoi));
   auto leftIter(rightIter++);
   auto end(std::end(vXSPoi));
   while(rightIter != end)
   {
      bool didErase = false;
      PoiIDType lid = leftIter->GetID();
      PoiIDType rid = rightIter->GetID();
      if (IsEqual(leftIter->GetDistFromStart(), rightIter->GetDistFromStart(), otol))
      {
         // POI's are very close, blast the one that's not a support or a duplicate
         if (0 <= lid && lid == rid)
         {
            // Weed duplicates out. Should be blocked from code above
            ATLASSERT(false);
            vXSPoi.erase(leftIter);
            didErase = true;
         }
         else if (!leftIter->HasAttribute(POI_INTERMEDIATE_PIER) &&  !leftIter->HasAttribute(POI_BOUNDARY_PIER) && !leftIter->HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) )
         {
            rightIter->MergeAttributes(*leftIter);
            vXSPoi.erase(leftIter);
            didErase = true;
         }
         else if (!rightIter->HasAttribute(POI_INTERMEDIATE_PIER) &&  !rightIter->HasAttribute(POI_BOUNDARY_PIER) && !rightIter->HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) )
         {
            leftIter->MergeAttributes(*rightIter);
            vXSPoi.erase(rightIter);
            didErase = true;
         }
         else if (leftIter->HasAttribute(POI_SECTCHANGE_LEFTFACE) && rightIter->HasAttribute(POI_SECTCHANGE_RIGHTFACE))
         {
            leftIter->MergeAttributes(*rightIter);
            vXSPoi.erase(rightIter);
            didErase = true;
         }
      }

      if (didErase)
      {
         leftIter = std::begin(vXSPoi); // inefficient, but have to restart to recoup left iterator in proper order
         rightIter = leftIter;
         rightIter++;
         end = std::end(vXSPoi);
      }
      else
      {
         leftIter++;
         rightIter++;
      }
   }

   // The decision to create fem members for cantilevers is based on numerical stability and not on whether loads are to be applied
   bool bDoModelLeftCantilever  = otol < leftSupportLoc ? true : false;
   bool bDoModelRightCantilever = otol < segmentLength - rightSupportLoc ? true : false;


   // layout the joints
   CComPtr<IFem2dJoint> jnt;
   bool bFoundLeftSupport(false), bFoundRightSupport(false);
   JointIDType jntID = 0;
   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);
   for( const auto& poi : vXSPoi)
   {
      Float64 Xpoi = poi.GetDistFromStart();
      if ( (!bDoModelLeftCantilever && (Xpoi < leftSupportLoc)) || 
           (!bDoModelRightCantilever && (rightSupportLoc < Xpoi)) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next joint
         continue;
      }

      jnt.Release();
      joints->Create(jntID++,Xpoi,0,&jnt);

      // set boundary conditions if this is a support joint
      if ( !bFoundLeftSupport && IsEqual(Xpoi,leftSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtFx);
         jnt->ReleaseDof(jrtMz);
         bFoundLeftSupport = true;
      }
      else if ( !bFoundRightSupport && IsEqual(Xpoi,rightSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtMz);
         bFoundRightSupport = true;
      }
   }

   ATLASSERT(bFoundLeftSupport && bFoundRightSupport); // model will be unstable

   // create members
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   // for consistancy with all structural analysis models, sections properties are based on the mid-span location of segments
   PoiList vMyPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
   ATLASSERT( vMyPoi.size() == 1 );
   const pgsPointOfInterest& spPoi = vMyPoi.front();
   ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

   Float64 Ixx = pSectProp->GetIxx(intervalIdx, spPoi);
   Float64 Iyy = pSectProp->GetIyy(intervalIdx, spPoi);
   Float64 Ixy = pSectProp->GetIxy(intervalIdx, spPoi);
   Float64 Ag = pSectProp->GetAg(intervalIdx,spPoi);
   Float64 EI = E*(Ixx*Iyy - Ixy*Ixy) / Iyy;
   Float64 EA = E*Ag;

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dMember> member;
   MemberIDType mbrID = 0;
   JointIDType prevJntID = 0;
   jntID = prevJntID + 1;
   auto prevJointIter( vXSPoi.begin() );
   auto jointIter = prevJointIter;
   jointIter++;
   auto jointIterEnd = vXSPoi.end();
   for ( ; jointIter < jointIterEnd; jointIter++, prevJointIter++ )
   {
      pgsPointOfInterest& prevPoi( *prevJointIter );
      pgsPointOfInterest& poi( *jointIter );

      if ( (!bDoModelLeftCantilever  && (prevPoi.GetDistFromStart() < leftSupportLoc)) || 
           (!bDoModelRightCantilever && (rightSupportLoc <= prevPoi.GetDistFromStart())) )
      {
         // location is before or after the left/right support and we aren't modeling
         // the cantilevers... next member
         continue;
      }

      member.Release();
      members->Create(mbrID++,prevJntID,jntID,EA,EI,&member);

      prevJntID++;
      jntID++;
   }
}

void pgsGirderModelFactory::ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,
                                       Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                       bool bModelLeftCantilever, bool bModelRightCantilever,const PoiList& vPOI,
                                       IFem2dModel** ppModel)
{
   // apply loads
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   (*ppModel)->get_Loadings(&loadings);
   loadings->Create(lcidGirder,&loading);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);

   std::vector<SegmentLoad> segLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   std::vector<ClosureJointLoad> cjLoads;
   pProductLoads->GetSegmentSelfWeightLoad(segmentKey,&segLoads,&diaphLoads,&cjLoads);

   // apply girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   for (const auto& segLoad : segLoads)
   {
      Float64 wStart = segLoad.Wstart;
      Float64 wEnd   = segLoad.Wend;
      Float64 start  = segLoad.Xstart;
      Float64 end    = segLoad.Xend;

      if ( !bModelLeftCantilever && ::IsLT(start,leftSupportLoc) )
      {
         // this load segment begins before the left support and we are ignoring loads out there
  
         // compute load intensity at the left support
         wStart = ::LinInterp(leftSupportLoc,wStart,wEnd,end-start);
         start = leftSupportLoc;
      }

      if ( !bModelRightCantilever && ::IsLT(rightSupportLoc,end) )
      {
         // this load segment ends after the right support and we are ignoring loads out there

         // compute load intensity at the right support
         wEnd = ::LinInterp(rightSupportLoc-start,wStart,wEnd,end-start);
         end = rightSupportLoc;
      }

      // apply the loading
      MemberIDType mbrIDStart; // member ID at the start of the load
      MemberIDType mbrIDEnd;   // member ID at the end of the load
      Float64 xStart; // distance from start of member mbrIDStart to the start of the load
      Float64 xEnd;   // distance from start of member mbrIDEnd to end of the load
      FindMember(*ppModel,start,&mbrIDStart,&xStart);
      FindMember(*ppModel,end,  &mbrIDEnd,  &xEnd);

      if (mbrIDStart == mbrIDEnd && IsEqual(xStart, xEnd))
      {
         continue;
      }

      CComPtr<IFem2dDistributedLoad> distLoad;
      if ( mbrIDStart == mbrIDEnd )
      {
         // load is contained on a single member
         distLoad.Release();
         distributedLoads->Create(loadID++,mbrIDStart,loadDirFy,xStart,xEnd,wStart,wEnd,lotMember,&distLoad);
      }
      else
      {
         // load straddles two or more members
         CComPtr<IFem2dMember> mbr;
         CComPtr<IFem2dJoint> jntStart, jntEnd;
         for ( MemberIDType mbrID = mbrIDStart; mbrID <= mbrIDEnd; mbrID++ )
         {
            Float64 w1,w2; // start and end load intensity on this member
            Float64 x1,x2; // start and end load location from the start of this member

            Float64 Lmbr;
            mbr.Release();
            members->Find(mbrID,&mbr);
            mbr->get_Length(&Lmbr); 

            JointIDType jntIDStart,jntIDEnd;
            mbr->get_StartJoint(&jntIDStart);
            mbr->get_EndJoint(&jntIDEnd);

            jntStart.Release();
            jntEnd.Release();
            joints->Find(jntIDStart,&jntStart);
            joints->Find(jntIDEnd,  &jntEnd);

            Float64 xMbrStart, xMbrEnd;
            jntStart->get_X(&xMbrStart);
            jntEnd->get_X(&xMbrEnd);

            if ( mbrID == mbrIDStart )
            {
               w1 = wStart;
               x1 = xStart;
            }
            else
            {
               w1 = ::LinInterp(xMbrStart-start,wStart,wEnd,end-start);
               x1 = 0; // start of member
            }

            if (mbrID == mbrIDEnd )
            {
               w2 = wEnd;
               x2 = xEnd;
            }
            else
            {
               w2 = ::LinInterp(xMbrEnd-start,wStart,wEnd,end-start);
               x2 = Lmbr; // end of member
            }

            if ( !IsEqual(x1,x2) )
            {
               // no need to add the laod if its length is 0
               distLoad.Release();
               distributedLoads->Create(loadID++,mbrID,loadDirFy,x1,x2,w1,w2,lotMember,&distLoad);
            }
         }
      }
   }

   // apply diaphragm loads for precast diaphragms
   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);


   CComPtr<IFem2dJoint> prevJoint, nextJoint;
   CComPtr<IFem2dPointLoad> pointLoad;
   for(const auto& diaphragmLoad : diaphLoads)
   {
      Float64 x = 0;

      mbrID = 0;
      bool bApplyLoad = false;

      CollectionIndexType nJoints;
      joints->get_Count(&nJoints);
      for ( CollectionIndexType jntIdx = 1; jntIdx < nJoints; jntIdx++, mbrID++ )
      {
         prevJoint.Release();
         nextJoint.Release();
         joints->get_Item(jntIdx-1,&prevJoint);
         joints->get_Item(jntIdx,&nextJoint);

         Float64 xPrev, xNext;
         prevJoint->get_X(&xPrev);
         nextJoint->get_X(&xNext);

         if ((!bModelLeftCantilever  && ::IsLT(xPrev,leftSupportLoc) ) ||
             (!bModelRightCantilever && ::IsLT(rightSupportLoc,xPrev) ) )
         {
            // location is before or after the left/right support and we arn't modeling
            // the cantilevers... next member
            continue;
         }

         if ( InRange(xPrev,diaphragmLoad.Loc,xNext) )
         {
            x = diaphragmLoad.Loc - xPrev;
            bApplyLoad = true;
            break;
         }
      }

      if ( bApplyLoad )
      {
         pointLoad.Release();
         pointLoads->Create(loadID++,mbrID,x,0,diaphragmLoad.Load,0,lotMember,&pointLoad);
      }
   }
}

void pgsGirderModelFactory::ApplyPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                                  bool bModelLeftCantilever, bool bModelRightCantilever,const PoiList& vPOI,
                                                  IFem2dModel** ppModel,pgsPoiPairMap* pPoiMap)
{
   // layout poi on fem model
   pPoiMap->Clear();
   auto poiIDs = pgsGirderModelFactory::AddPointsOfInterest(*ppModel,vPOI);
   auto poiIDiter(poiIDs.cbegin());
   auto poiIDiterEnd(poiIDs.cend());
   auto poiIter(vPOI.cbegin());
   auto poiIterEnd(vPOI.cend());
   for ( ; poiIDiter != poiIDiterEnd && poiIter != poiIterEnd; poiIDiter++, poiIter++ )
   {
      pPoiMap->AddMap(*poiIter,*poiIDiter);
   }
}

void pgsGirderModelFactory::FindMember(IFem2dModel* pModel,Float64 distFromStartOfModel,MemberIDType* pMbrID,Float64* pDistFromStartOfMbr)
{
   CComPtr<IFem2dMemberCollection> members;
   pModel->get_Members(&members);

   CollectionIndexType mbrcnt;
   members->get_Count(&mbrcnt);

   CComPtr<IFem2dJointCollection> joints;
   pModel->get_Joints(&joints);

   CComPtr<IFem2dEnumMember> enumMembers;
   members->get__EnumElements(&enumMembers);

   CollectionIndexType idx = 0;
   CComPtr<IFem2dJoint> j1, j2;
   CComPtr<IFem2dMember> mbr;
   while ( enumMembers->Next(1,&mbr,nullptr) != S_FALSE )
   {
      JointIDType jntID1, jntID2;
      mbr->get_StartJoint(&jntID1);
      mbr->get_EndJoint(&jntID2);

      j1.Release();
      j2.Release();
      joints->Find(jntID1,&j1);
      joints->Find(jntID2,&j2);

      Float64 x1,x2;
      j1->get_X(&x1);
      j2->get_X(&x2);

      if ( InRange(x1,distFromStartOfModel,x2) )
      {
         mbr->get_ID(pMbrID);
         *pDistFromStartOfMbr = distFromStartOfModel - x1;
         return;
      }
      else if (idx==0 && distFromStartOfModel<x1) // next cases are for short cantilevers where fem model is not generated
      {
         mbr->get_ID(pMbrID);
         *pDistFromStartOfMbr = 0.0;
         return;
      }
      else if (idx==mbrcnt-1 && distFromStartOfModel>x2)
      {
         mbr->get_ID(pMbrID);
         *pDistFromStartOfMbr = x2-x1;
         return;
      }

      mbr.Release();
      idx++;
   }

   ATLASSERT(false); // didn't find a solution
}

PoiIDPairType pgsGirderModelFactory::AddPointOfInterest(IFem2dModel* pModel,const pgsPointOfInterest& poi)
{
   // layout poi on fem model
   CComPtr<IFem2dJointCollection> joints;
   pModel->get_Joints(&joints);
   CollectionIndexType nJoints;
   joints->get_Count(&nJoints);

   CComPtr<IFem2dPOICollection> pois;
   pModel->get_POIs(&pois);

   Float64 dist_from_start_of_member;
   MemberIDType mbrID = 0;
   
   CollectionIndexType jntIdx = 0;

   CComPtr<IFem2dJoint> prevJnt;
   joints->get_Item(jntIdx++,&prevJnt);
   Float64 prevLocation;
   prevJnt->get_X(&prevLocation);

   Float64 poi_dist_from_start = poi.GetDistFromStart();

   bool is_dual_pois = false; // if poi straddles a support location, add fem pois at either side
   CComPtr<IFem2dJoint> jnt;
   for ( ; jntIdx < nJoints; jntIdx++, mbrID++ )
   {
      jnt.Release();
      joints->get_Item(jntIdx,&jnt);
      Float64 location;
      jnt->get_X(&location);
      VARIANT_BOOL is_support;
      jnt->IsSupport(&is_support);

      if (is_support && IsEqual(poi_dist_from_start,location) && jntIdx!=nJoints-1 )
      {
         // poi is directly over an interior support joint. we need fem pois on either side
         CComPtr<IFem2dPOI> objPOI_left,objPOI_right;

         PoiIDType femID_left = ms_FemModelPoiID++;
         PoiIDType femID_right = ms_FemModelPoiID++;

         HRESULT hr = pois->Create(femID_left, mbrID, -1.0, &objPOI_left);
         ATLASSERT(SUCCEEDED(hr));
         hr = pois->Create(femID_right, mbrID+1, 0.0, &objPOI_right);
         ATLASSERT(SUCCEEDED(hr));

         return PoiIDPairType(femID_left, femID_right);
      }
      else if ( InRange(prevLocation, poi_dist_from_start, location) )
      {
         dist_from_start_of_member = poi_dist_from_start - prevLocation;

         CComPtr<IFem2dPOI> objPOI;

         PoiIDType femID = ms_FemModelPoiID++;

         HRESULT hr = pois->Create(femID,mbrID,dist_from_start_of_member,&objPOI);
         ATLASSERT(SUCCEEDED(hr));

         return PoiIDPairType(femID, femID);
      }

      prevLocation = location;
   }

   // POI is not on model. This can happen if the POI is before or after the support and cantilevers are not modelled
   // First check left end
   Float64 location;
   CComPtr<IFem2dJoint> lftjnt;
   joints->get_Item(0,&lftjnt);
   lftjnt->get_X(&location);

   if (poi_dist_from_start < location)
   {
      VARIANT_BOOL is_support;
      lftjnt->IsSupport(&is_support);
      ATLASSERT(is_support); // remember, we are assuming no cantilever here

      CComPtr<IFem2dPOI> objPOI;
      PoiIDType femID = ms_FemModelPoiID++;

      HRESULT hr = pois->Create(femID,0,0.0,&objPOI); // left end of first member
      ATLASSERT(SUCCEEDED(hr));
      return PoiIDPairType(femID, femID);
   }

   // Now check right end
   CComPtr<IFem2dJoint> rgtjnt;
   joints->get_Item(nJoints-1,&rgtjnt);
   rgtjnt->get_X(&location);

   if (location < poi_dist_from_start)
   {
      VARIANT_BOOL is_support;
      rgtjnt->IsSupport(&is_support);
      ATLASSERT(is_support); // remember, we are assuming no cantilever here

      CComPtr<IFem2dPOI> objPOI;
      PoiIDType femID = ms_FemModelPoiID++;

      HRESULT hr = pois->Create(femID,nJoints-2,-1.0,&objPOI); // left end of first member
      ATLASSERT(SUCCEEDED(hr));
      return PoiIDPairType(femID, femID);
   }

   // poi not found. should never happen
   ATLASSERT(0); 
   return PoiIDPairType(INVALID_ID,INVALID_ID);
}

std::vector<PoiIDPairType> pgsGirderModelFactory::AddPointsOfInterest(IFem2dModel* pModel,const PoiList& vPoi)
{
   std::vector<PoiIDPairType> femIDs;

   for(const pgsPointOfInterest& poi : vPoi)
   {
      femIDs.push_back(pgsGirderModelFactory::AddPointOfInterest(pModel,poi));
   }

   return femIDs;
}

/////////////////////////////////////////////////////////////////////
/////////// class pgsKdotHaulingGirderModelFactory //////////////////
/////////////////////////////////////////////////////////////////////
pgsKdotHaulingGirderModelFactory::pgsKdotHaulingGirderModelFactory(Float64 overhangFactor, Float64 interiorFactor):
m_OverhangFactor(overhangFactor), m_InteriorFactor(interiorFactor)
{
}

pgsKdotHaulingGirderModelFactory::~pgsKdotHaulingGirderModelFactory(void)
{
}

void pgsKdotHaulingGirderModelFactory::ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,
                                                  LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,
                                                  const PoiList& vPOI,IFem2dModel** ppModel)
{
   ATLASSERT(bModelLeftCantilever && bModelRightCantilever); // kdot method should always include cantilevers

   // apply  loads
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   (*ppModel)->get_Loadings(&loadings);
   loadings->Create(lcidGirder,&loading);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);

   std::vector<SegmentLoad> segLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   std::vector<ClosureJointLoad> cjLoads;
   pProductLoads->GetSegmentSelfWeightLoad(segmentKey,&segLoads,&diaphLoads,&cjLoads);

   // apply dynamically factored girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<SegmentLoad>::iterator segLoadIter(segLoads.begin());
   std::vector<SegmentLoad>::iterator segLoadIterEnd(segLoads.end());
   for ( ; segLoadIter != segLoadIterEnd; segLoadIter++ )
   {
      SegmentLoad& segLoad = *segLoadIter;

      Float64 wStart = segLoad.Wstart;
      Float64 wEnd   = segLoad.Wend;
      Float64 start  = segLoad.Xstart;
      Float64 end    = segLoad.Xend;

      // apply the loading
      MemberIDType mbrIDStart; // member ID at the start of the load
      MemberIDType mbrIDEnd;   // member ID at the end of the load
      Float64 xStart; // distance from start of member mbrIDStart to the start of the load
      Float64 xEnd;   // distance from start of member mbrIDEnd to end of the load
      FindMember(*ppModel,start,&mbrIDStart,&xStart);
      FindMember(*ppModel,end,  &mbrIDEnd,  &xEnd);

      if ( mbrIDStart == mbrIDEnd )
      {
         // load is contained on a single member and is all interior
         wStart *= m_InteriorFactor;
         wEnd   *= m_InteriorFactor;

         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrIDStart,loadDirFy,xStart,xEnd,wStart,wEnd,lotMember,&distLoad);
      }
      else
      {
         // load straddles two or more members
         for ( MemberIDType mbrID = mbrIDStart; mbrID <= mbrIDEnd; mbrID++ )
         {
            Float64 w1,w2; // start and end load intensity on this member
            Float64 x1,x2; // start and end load location from the start of this member

            Float64 Lmbr;
            CComPtr<IFem2dMember> mbr;
            members->Find(mbrID,&mbr);
            mbr->get_Length(&Lmbr); 

            JointIDType jntIDStart,jntIDEnd;
            mbr->get_StartJoint(&jntIDStart);
            mbr->get_EndJoint(&jntIDEnd);

            CComPtr<IFem2dJoint> jntStart, jntEnd;
            joints->Find(jntIDStart,&jntStart);
            joints->Find(jntIDEnd,  &jntEnd);

            Float64 xMbrStart, xMbrEnd;
            jntStart->get_X(&xMbrStart);
            jntEnd->get_X(&xMbrEnd);

            if ( mbrID == mbrIDStart )
            {
               w1 = wStart;
               x1 = xStart;
            }
            else
            {
               w1 = ::LinInterp(xMbrStart,wStart,wEnd,end-start);
               x1 = 0; // start of member
            }

            if (mbrID == mbrIDEnd )
            {
               w2 = wEnd;
               x2 = xEnd;
            }
            else
            {
               w2 = ::LinInterp(xMbrEnd,wStart,wEnd,end-start);
               x2 = Lmbr; // end of member
            }

            // Factor loads depending on whether they are on cantilever, or interior
            bool onLeft  = IsLE(xMbrStart, leftSupportLoc)  && IsLE(xMbrEnd, leftSupportLoc);
            bool onRight = IsLE(rightSupportLoc, xMbrStart) && IsLE(rightSupportLoc, xMbrEnd);
            if ( onLeft || onRight)
            {
               w1 *= this->m_OverhangFactor;
               w2 *= this->m_OverhangFactor;
            }
            else
            {
               w1 *= this->m_InteriorFactor;
               w2 *= this->m_InteriorFactor;
            }

            CComPtr<IFem2dDistributedLoad> distLoad;
            distributedLoads->Create(loadID++,mbrID,loadDirFy,x1,x2,w1,w2,lotMember,&distLoad);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////
/////////// class pgsDesignHaunchLoadGirderModelFactory //////////////////
/////////////////////////////////////////////////////////////////////
pgsDesignHaunchLoadGirderModelFactory::pgsDesignHaunchLoadGirderModelFactory( const std::vector<SlabLoad>& slabLoads, LoadCaseIDType slabLoadCase, LoadCaseIDType slabPadLoadCase):
m_SlabLoads(slabLoads), m_SlabLoadCase(slabLoadCase), m_SlabPadLoadCase(slabPadLoadCase)
{
}

pgsDesignHaunchLoadGirderModelFactory::~pgsDesignHaunchLoadGirderModelFactory(void)
{
}

void pgsDesignHaunchLoadGirderModelFactory::ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,
                                                  LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,
                                                  const PoiList& vPOI,IFem2dModel** ppModel)
{
   // apply  loads
   // We dont need girder self weight, so don't use it
   CComPtr<IFem2dLoadingCollection> loadings;
   (*ppModel)->get_Loadings(&loadings);

   CComPtr<IFem2dLoading> slabPadLoading, slabLoading;

   loadings->Create(m_SlabLoadCase,   &slabLoading);
   loadings->Create(m_SlabPadLoadCase,&slabPadLoading);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);

   CComPtr<IFem2dDistributedLoadCollection> slabDistributedLoads, slabPadDistributedLoads;
   slabLoading->get_DistributedLoads(&slabDistributedLoads);
   slabPadLoading->get_DistributedLoads(&slabPadDistributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;

   std::vector<SlabLoad>::iterator slabLoadIter(m_SlabLoads.begin());
   std::vector<SlabLoad>::iterator slabLoadIterEnd(m_SlabLoads.end());

   // load information is at individual locations. Need to get get first value to get ball rolling
   SlabLoad& startSlabLoad = *slabLoadIter;
   Float64 start = startSlabLoad.Loc;
   Float64 wslabStart = startSlabLoad.MainSlabLoad + startSlabLoad.PanelLoad;
   Float64 wslabPadStart = startSlabLoad.PadLoad;
   slabLoadIter++;

   CComPtr<IFem2dDistributedLoad> slabDistLoad, slabPadDistLoad;
   for ( ; slabLoadIter != slabLoadIterEnd; slabLoadIter++ )
   {
      SlabLoad& slabLoad = *slabLoadIter;

      Float64 end = slabLoad.Loc;
      Float64 wslabEnd = slabLoad.MainSlabLoad + slabLoad.PanelLoad;
      Float64 wslabPadEnd = slabLoad.PadLoad;


      if ( !bModelLeftCantilever && ::IsLT(start,leftSupportLoc) )
      {
         // this load segment begins before the left support and we are ignoring loads out there
  
         // compute load intensity at the left support
         wslabStart = ::LinInterp(leftSupportLoc,wslabStart,wslabEnd,end-start);
         wslabPadStart = ::LinInterp(leftSupportLoc,wslabPadStart,wslabPadEnd,end-start);
         start = leftSupportLoc;
      }

      if ( !bModelRightCantilever && ::IsLT(rightSupportLoc,end) )
      {
         // this load segment ends after the right support and we are ignoring loads out there

         // compute load intensity at the right support
         wslabEnd = ::LinInterp(rightSupportLoc-start,wslabStart,wslabEnd,end-start);
         wslabPadEnd = ::LinInterp(rightSupportLoc-start,wslabPadStart,wslabPadEnd,end-start);
         end = rightSupportLoc;
      }

      // apply the loading
      MemberIDType mbrIDStart; // member ID at the start of the load
      MemberIDType mbrIDEnd;   // member ID at the end of the load
      Float64 xStart; // distance from start of member mbrIDStart to the start of the load
      Float64 xEnd;   // distance from start of member mbrIDEnd to end of the load
      FindMember(*ppModel,start,&mbrIDStart,&xStart);
      FindMember(*ppModel,end,  &mbrIDEnd,  &xEnd);

      if ( mbrIDStart == mbrIDEnd )
      {
         // load is contained on a single member and is all interior
         if (!IsEqual(xStart, xEnd)) // No use creating a load if it's zero length
         {
         slabDistLoad.Release();
         slabPadDistLoad.Release();
            slabDistributedLoads->Create(loadID++, mbrIDStart, loadDirFy, xStart, xEnd, wslabStart, wslabEnd, lotMember, &slabDistLoad);
            slabPadDistributedLoads->Create(loadID++, mbrIDStart, loadDirFy, xStart, xEnd, wslabPadStart, wslabPadEnd, lotMember, &slabPadDistLoad);
         }
      }
      else
      {
         // load straddles two or more members
         CComPtr<IFem2dMember> mbr;
         CComPtr<IFem2dJoint> jntStart, jntEnd;
         for ( MemberIDType mbrID = mbrIDStart; mbrID <= mbrIDEnd; mbrID++ )
         {
            Float64 wsl1, wsl2, wsp1, wsp2; // start and end load intensity of slab and slab pad on this member
            Float64 x1,x2; // start and end load location from the start of this member

            Float64 Lmbr;
            mbr.Release();
            members->Find(mbrID,&mbr);
            mbr->get_Length(&Lmbr); 

            JointIDType jntIDStart,jntIDEnd;
            mbr->get_StartJoint(&jntIDStart);
            mbr->get_EndJoint(&jntIDEnd);

            jntStart.Release();
            jntEnd.Release();
            joints->Find(jntIDStart,&jntStart);
            joints->Find(jntIDEnd,  &jntEnd);

            Float64 xMbrStart, xMbrEnd;
            jntStart->get_X(&xMbrStart);
            jntEnd->get_X(&xMbrEnd);

            if ( mbrID == mbrIDStart )
            {
               wsl1 = wslabStart;
               wsp1 = wslabPadStart;
               x1 = xStart;
            }
            else
            {
               wsl1 = ::LinInterp(xMbrStart,wslabStart,   wslabEnd,   end-start);
               wsp1 = ::LinInterp(xMbrStart,wslabPadStart,wslabPadEnd,end-start);
               x1 = 0; // start of member
            }

            if (mbrID == mbrIDEnd )
            {
               wsl2 = wslabEnd;
               wsp2 = wslabPadEnd;
               x2 = xEnd;
            }
            else
            {
               wsl2 = ::LinInterp(xMbrEnd,wslabStart,   wslabEnd,   end-start);
               wsp2 = ::LinInterp(xMbrEnd,wslabPadStart,wslabPadEnd,end-start);
               x2 = Lmbr; // end of member
            }

            if (!IsEqual(x1, x2)) // No use creating a load if it's zero length
            {
            slabDistLoad.Release();
            slabPadDistLoad.Release();
               slabDistributedLoads->Create(loadID++, mbrID, loadDirFy, x1, x2, wsl1, wsl2, lotMember, &slabDistLoad);
               slabPadDistributedLoads->Create(loadID++, mbrID, loadDirFy, x1, x2, wsp1, wsp2, lotMember, &slabPadDistLoad);
            }
         }
      }

      // cycle
      start = end;
      wslabStart = wslabEnd;
      wslabPadStart = wslabPadEnd;
   }
}