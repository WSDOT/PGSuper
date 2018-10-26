///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <PGSuperTypes.h>

template<class IteratorType>
ZoneIndexType GetZoneIndexAtLocation(Float64 location, Float64 girderLength, Float64 startSupportLoc, Float64 endSupportLoc,
                             bool bSymmetrical, IteratorType& rItBegin, IteratorType& rItEnd, ZoneIndexType collSize)
{
   CHECK(collSize>0);

   ZoneIndexType zone = 0;

   if (bSymmetrical)
   {
      bool on_left=true;

      if (location>girderLength/2.)  // zones are symmetric about mid-span
      {
         location = girderLength-location;
         on_left=false;
      }
      CHECK(location>=0);

      Float64 end_dist;
      if (on_left)
         end_dist = startSupportLoc;
      else
         end_dist = girderLength-endSupportLoc;

      // find zone
      Float64 ezloc=0.0;
      ZoneIndexType znum=0;
      bool found=false;
      // need to give value a little 'shove' so that pois inboard from the support
      // are assigned the zone to the left, and pois outboard from the support
      // are assigned the zone to the right.
      const Float64 tol = 1.0e-6;
      for (IteratorType it = rItBegin; it != rItEnd; it++)
      {
         ezloc += it->ZoneLength;
         if (location < end_dist+tol)
         {
            if (location+tol < ezloc)
            {
               found = true;
               break;
            }
         }
         else
         {
            if (location < ezloc+tol)
            {
               found = true;
               break;
            }
         }
         znum++;
      }

      if (found)
      {
         zone = znum;
      }
      else
      {
         // not found - must be in middle
         ATLASSERT(collSize != 0); // -1 will cause an underflow 
         zone = collSize-1;
      }
   }
   else
   {
      // not symmetrical
      // find zone
      Float64 ezloc=0.0;
      ZoneIndexType znum=0;
      bool found=false;
      // need to give value a little 'shove' so that pois inboard from the support
      // are assigned the zone to the left, and pois outboard from the support
      // are assigned the zone to the right.
      const Float64 tol = 1.0e-6;
      for (IteratorType it = rItBegin; it != rItEnd; it++)
      {
         ezloc += it->ZoneLength;
         if (location < startSupportLoc + tol)
         {
            if (location+tol<ezloc)
            {
               found = true;
               break;
            }
         }
         else if (location + tol > endSupportLoc)
         {
            if (location<ezloc+tol)
            {
               found = true;
               break;
            }
         }
         else
         {
            if (location<ezloc+tol)
            {
               found = true;
               break;
            }
         }

         znum++;
      }

      if (found)
      {
         zone = znum;
      }
      else
      {
         zone = collSize-1; // extend last zone to end of girder
      }
   }

   return zone;
}


// Various utility functions that operate on CONFIG objects in PGSuperTypes.h
enum PrimaryStirrupType {getVerticalStirrup, getHorizShearStirrup};

inline Float64 GetPrimaryStirrupAvs(const STIRRUPCONFIG& config, PrimaryStirrupType type, Float64 location, 
                                 Float64 gdrLength, Float64 leftSupportLoc,Float64 rgtSupportLoc,
                                 matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pNBars, Float64* pSpacing)
{
   // Use template function to do heavy work
   ZoneIndexType zone =  GetZoneIndexAtLocation(location, gdrLength, leftSupportLoc, rgtSupportLoc, config.bAreZonesSymmetrical, 
                                                config.ShearZones.begin(), config.ShearZones.end(), 
                                                config.ShearZones.size());

   if (zone>=0)
   {
      const STIRRUPCONFIG::SHEARZONEDATA& rzd = config.ShearZones[zone];
      *pSpacing = rzd.BarSpacing;
      *pSize = rzd.VertBarSize;
      *pSingleBarArea = rzd.VertABar;

      if (type==getVerticalStirrup)
      {
         *pNBars = rzd.nVertBars;
      }
      else // getHorizShearStirrup
      {
         *pNBars = rzd.nHorzInterfaceBars;
      }

      if (*pSpacing > 0.0)
      {
         return (*pSingleBarArea)*(*pNBars)/(*pSpacing);
      }
      else
      {
         return 0.0;
      }
   }
   else
   {
      ATLASSERT(0); // this should never happen
      *pSize = matRebar::bsNone;
      *pNBars = 0.0;
      *pSpacing = 0.0;
      return 0.0;
   }
}

inline Float64 GetAdditionalHorizInterfaceAvs(const STIRRUPCONFIG& config, Float64 location, 
                                              Float64 gdrLength, Float64 leftSupportLoc,Float64 rgtSupportLoc,
                                              matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pNBars, Float64* pSpacing)
{
   // Use template function to do heavy work
   ZoneIndexType zone =  GetZoneIndexAtLocation(location, gdrLength, leftSupportLoc, rgtSupportLoc, config.bAreZonesSymmetrical, 
                                                config.HorizontalInterfaceZones.begin(), config.HorizontalInterfaceZones.end(), 
                                                config.HorizontalInterfaceZones.size());
   if (zone>=0)
   {
      const STIRRUPCONFIG::HORIZONTALINTERFACEZONEDATA& rzd = config.HorizontalInterfaceZones[zone];
      *pSize = rzd.BarSize;
      *pNBars = rzd.nBars;
      *pSpacing = rzd.BarSpacing;
      *pSingleBarArea = rzd.ABar;

      if (rzd.BarSpacing > 0.0)
      {
         return rzd.ABar*rzd.nBars/rzd.BarSpacing;
      }
      else
      {
         return 0.0;
      }
   }
   else
   {
      ATLASSERT(0); // this should never happen
      *pSize = matRebar::bsNone;
      *pNBars = 0.0;
      *pSpacing = 0.0;
      return 0.0;
   }
}


inline void GetConfinementInfoFromStirrupConfig(const STIRRUPCONFIG& config, 
                                                Float64 reqdStartZl, matRebar::Size* pStartRBsiz, Float64* pStartZL, Float64* pStartS,
                                                Float64 reqdEndZl,   matRebar::Size* pEndRBsiz,   Float64* pEndZL,   Float64* pEndS)
{
   // Design for confinement will either use additional bars or primary bars
   if (config.ConfinementBarSize != matRebar::bsNone)
   {
      // Additional bars
      *pStartRBsiz = config.ConfinementBarSize;
      *pStartZL    = config.ConfinementZoneLength;
      *pStartS     = config.ConfinementBarSpacing;

      *pEndRBsiz = *pStartRBsiz;
      *pEndZL    = *pStartZL;
      *pEndS     = *pStartS;
   }
   else
   {
      // Primary bars are used for confinement
      *pStartRBsiz = matRebar::bsNone;
      *pEndRBsiz = matRebar::bsNone;

      // first, left end
      Float64 endloc=0.0;
      bool first=true;
      for(STIRRUPCONFIG::ShearZoneConstIterator itl = config.ShearZones.begin(); itl != config.ShearZones.end(); itl++)
      {
         if (endloc>=reqdStartZl)
         {
            break;
         }
         else
         {
            const STIRRUPCONFIG::SHEARZONEDATA& zd = *itl;
            if (first)
            {
               // Use bar size from first zone
               if (zd.ConfinementBarSize==matRebar::bsNone)
               {
                  break; // no use looking further - we have no confinement bars
               }

               *pStartRBsiz = zd.ConfinementBarSize;
               *pStartS = zd.BarSpacing;

               first = false;
            }
            else
            {
               *pStartS = max(*pStartS, zd.BarSpacing); // use max spacing of any zone
            }

            endloc += zd.ZoneLength;
         }
      }

      *pStartZL = endloc;

      // Next deal with left end
      if (config.bAreZonesSymmetrical)
      {
         // left is same as right
         *pEndRBsiz = *pStartRBsiz;
         *pEndZL    = *pStartZL;
         *pEndS     = *pStartS;
      }
      else
      {
         endloc=0.0;
         first=true;
         for(STIRRUPCONFIG::ShearZoneConstReverseIterator itr = config.ShearZones.rbegin(); itr != config.ShearZones.rend(); itr++)
         {
            if (endloc>=reqdEndZl)
            {
               break;
            }
            else
            {
               const STIRRUPCONFIG::SHEARZONEDATA& zd = *itr;
               if (first)
               {
                  // Use bar size from first zone
                  if (zd.ConfinementBarSize==matRebar::bsNone)
                  {
                     break; // no use looking further - we have no confinement bars
                  }

                  *pEndRBsiz = zd.ConfinementBarSize;
                  *pEndS     = zd.BarSpacing;

                  first = false;
               }
               else
               {
                  *pEndS = max(*pEndS, zd.BarSpacing); // use max spacing of any zone
               }

               endloc += zd.ZoneLength;
            }
         }

         *pEndZL = endloc;
      }
   }
}

inline Float64 GetPrimaryAvLeftEnd(const STIRRUPCONFIG& config, matRebar::Type barType, matRebar::Grade barGrade,
                                Float64 gdrLength, Float64 rangeLength)
{
   ATLASSERT(rangeLength<gdrLength/2.0); // This function was designed for splitting zone, which should never be too long

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   Float64 Av=0.0;
   Float64 endloc=0.0;
   for(STIRRUPCONFIG::ShearZoneConstIterator itl = config.ShearZones.begin(); itl != config.ShearZones.end(); itl++)
   {
      if (endloc>=rangeLength)
      {
         break; // done
      }
      else
      {
         const STIRRUPCONFIG::SHEARZONEDATA& zd = *itl;

         if (zd.VertBarSize==matRebar::bsNone)
         {
            break; // no use looking further - we have no bars
         }

         matRebar::Size size = zd.VertBarSize;
         const matRebar* pRebar = pool->GetRebar(barType,barGrade,size);

         Float64 Abars = pRebar->GetNominalArea() * zd.nVertBars;
         Float64 spacing = zd.BarSpacing;

         if (endloc+zd.ZoneLength > rangeLength)
         {
            // Range covers partial zone
            Float64 lastzl = rangeLength-endloc;
            Av += Abars*lastzl/spacing;
         }
         else
         {
            // Full length of zone is within range
            Av += Abars * zd.ZoneLength / spacing;
         }

         endloc += zd.ZoneLength;
      }
   }

   return Av;
}

inline Float64 GetPrimaryAvRightEnd(const STIRRUPCONFIG& config, matRebar::Type barType, matRebar::Grade barGrade,
                                    Float64 gdrLength, Float64 rangeLength)
{
   ATLASSERT(rangeLength<gdrLength/2.0); // This function was designed for splitting zone, which should never be too long

   Float64 Av=0.0;
   if (config.bAreZonesSymmetrical)
   {
      // Right end is same as left
      return GetPrimaryAvLeftEnd(config, barType, barGrade, gdrLength, rangeLength);
   }
   else
   {
      // Search in reverse from right end
      lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
      ATLASSERT(pool != NULL);

      Float64 endloc=0.0;
      for(STIRRUPCONFIG::ShearZoneConstReverseIterator itl = config.ShearZones.rbegin(); itl != config.ShearZones.rend(); itl++)
      {
         if (endloc>=rangeLength)
         {
            break; // done
         }
         else
         {
            const STIRRUPCONFIG::SHEARZONEDATA& zd = *itl;

            if (zd.VertBarSize==matRebar::bsNone)
            {
               break; // no use looking further - we have no bars
            }

            matRebar::Size size = zd.VertBarSize;
            const matRebar* pRebar = pool->GetRebar(barType,barGrade,size);

            Float64 Abars = pRebar->GetNominalArea() * zd.nVertBars;
            Float64 spacing = zd.BarSpacing;

            if (endloc+zd.ZoneLength > rangeLength)
            {
               // Range covers partial zone
               Float64 lastzl = rangeLength-endloc;
               Av += Abars*lastzl/spacing;
            }
            else
            {
               // Full length of zone is within range
               Av += Abars * zd.ZoneLength / spacing;
            }

            endloc += zd.ZoneLength;
         }
      }
   }

   return Av;
}

inline void GetSplittingAvFromStirrupConfig(const STIRRUPCONFIG& config, matRebar::Type barType, matRebar::Grade barGrade,
                                            Float64 gdrLength,
                                            Float64 reqdStartZl, Float64* pStartAv,
                                            Float64 reqdEndZl,   Float64* pEndAv)
{
   // Design for splitting can use both additional bars or primary bars
   *pStartAv = 0.0; 
   *pEndAv   = 0.0; 

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   if (config.SplittingBarSize != matRebar::bsNone)
   {
      // Additional bars
      matRebar::Size barSize = config.SplittingBarSize;
      const matRebar* pRebar = pool->GetRebar(barType,barGrade,barSize);
      Float64 Abar = pRebar->GetNominalArea();

      ATLASSERT(!IsZero(config.SplittingBarSpacing));

      Float64 Avs = Abar * config.nSplittingBars / config.SplittingBarSpacing;

      // start end
      Float64 zl = min(reqdStartZl, config.SplittingZoneLength);
      *pStartAv = Avs * zl;

      // end end
      zl = min(reqdEndZl, config.SplittingZoneLength);
      *pEndAv = Avs * zl;
   }

   if (config.bUsePrimaryForSplitting)
   {
      // Primary bars are used for splitting
      // first, left end then right
      *pStartAv += GetPrimaryAvLeftEnd(config, barType, barGrade, gdrLength, reqdStartZl);

      *pEndAv += GetPrimaryAvRightEnd(config, barType, barGrade, gdrLength, reqdEndZl);
   }
}

inline void WriteShearDataToStirrupConfig(const CShearData& rShearData, STIRRUPCONFIG& rConfig)
{
   rConfig.bAreZonesSymmetrical = rShearData.bAreZonesSymmetrical;
   rConfig.bIsRoughenedSurface = rShearData.bIsRoughenedSurface;
   rConfig.bUsePrimaryForSplitting = rShearData.bUsePrimaryForSplitting;

   lrfdRebarPool* prp = lrfdRebarPool::GetInstance();

   // Primary zones
   rConfig.ShearZones.clear();
   CShearData::ShearZoneConstIterator sz_it(rShearData.ShearZones.begin());
   CShearData::ShearZoneConstIterator sz_end(rShearData.ShearZones.end());
   while(sz_it != sz_end)
   {
      const CShearZoneData& sz_data = *sz_it;

      STIRRUPCONFIG::SHEARZONEDATA SZ_data;
      SZ_data.ZoneLength         = sz_data.ZoneLength;
      SZ_data.VertBarSize        = sz_data.VertBarSize;
      SZ_data.BarSpacing         = sz_data.BarSpacing;
      SZ_data.nVertBars          = sz_data.nVertBars;
      SZ_data.nHorzInterfaceBars = sz_data.nHorzInterfaceBars;
      SZ_data.ConfinementBarSize = sz_data.ConfinementBarSize;

      if(sz_data.VertBarSize!=matRebar::bsNone)
      {
         const matRebar* pRebar = prp->GetRebar(rShearData.ShearBarType,rShearData.ShearBarGrade,sz_data.VertBarSize);
         SZ_data.VertABar = pRebar->GetNominalArea();
      }
      else
      {
         SZ_data.VertABar = 0.0;
      }

      rConfig.ShearZones.push_back(SZ_data);

      sz_it++;
   }

   // Horizontal interface zones
   rConfig.HorizontalInterfaceZones.clear();
   CShearData::HorizontalInterfaceZoneConstIterator hi_it(rShearData.HorizontalInterfaceZones.begin());
   CShearData::HorizontalInterfaceZoneConstIterator hi_end(rShearData.HorizontalInterfaceZones.end());
   while(hi_it != hi_end)
   {
      const CHorizontalInterfaceZoneData& hi_data = *hi_it;

      STIRRUPCONFIG::HORIZONTALINTERFACEZONEDATA HI_data;
      HI_data.ZoneLength = hi_data.ZoneLength;
      HI_data.BarSpacing = hi_data.BarSpacing;
      HI_data.BarSize    = hi_data.BarSize;
      HI_data.nBars      = hi_data.nBars;

      if(hi_data.BarSize!=matRebar::bsNone)
      {
         const matRebar* pRebar = prp->GetRebar(rShearData.ShearBarType,rShearData.ShearBarGrade,hi_data.BarSize);
         HI_data.ABar = pRebar->GetNominalArea();
      }
      else
      {
         HI_data.ABar = 0.0;
      }

      rConfig.HorizontalInterfaceZones.push_back(HI_data);

      hi_it++;
   }

   // Splitting (additional)
   rConfig.SplittingBarSize = rShearData.SplittingBarSize;
   rConfig.SplittingBarSpacing = rShearData.SplittingBarSpacing;
   rConfig.SplittingZoneLength = rShearData.SplittingZoneLength;
   rConfig.nSplittingBars = rShearData.nSplittingBars;

   // Confinement (additional)
   rConfig.ConfinementBarSize = rShearData.ConfinementBarSize;
   rConfig.ConfinementBarSpacing = rShearData.ConfinementBarSpacing;
   rConfig.ConfinementZoneLength = rShearData.ConfinementZoneLength;
}
// NOTE: The method below could be useful to a design algorithm sometime in the future.
/*
inline void  WriteLongitudinalRebarDataToConfig(const CLongitudinalRebarData& rRebarData, LONGITUDINALREBARCONFIG& rConfig)
{
   rConfig.BarGrade = rRebarData.BarGrade;
   rConfig.BarType = rRebarData.BarType;

   rConfig.RebarRows.clear();

   std::vector<CLongitudinalRebarData::RebarRow>::const_iterator it(rRebarData.RebarRows.begin());
   std::vector<CLongitudinalRebarData::RebarRow>::const_iterator it_end(rRebarData.RebarRows.end());
   while(it != it_end)
   {
      const CLongitudinalRebarData::RebarRow& rwdata = *it;

      LONGITUDINALREBARCONFIG::RebarRow row;
      row.BarSize = rwdata.BarSize;
      row.BarSpacing = rwdata.BarSpacing;
      row.Cover = rwdata.Cover;
      row.Face = rwdata.Face;
      row.NumberOfBars = rwdata.NumberOfBars;

      rConfig.RebarRows.push_back(row);

      it++;
   }
}
*/