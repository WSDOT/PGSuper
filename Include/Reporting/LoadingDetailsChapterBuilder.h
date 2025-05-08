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

#pragma once

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

class IBridge;
class IProductLoads;
class IEAFDisplayUnits;
class IRatingSpecification;

class REPORTINGCLASS CLoadingDetailsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CLoadingDetailsChapterBuilder(bool bDesign,bool bRating,bool bSelect);

   // use this constructor if a simplified (shortened) version of the chapter is desired
   CLoadingDetailsChapterBuilder(bool SimplifiedVersion,bool bDesign,bool bRating,bool bSelect);

   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

protected:
   void ReportPedestrianLoad(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& thisSegmentKey) const;
   void ReportSlabLoad(std::shared_ptr<WBFL::EAF::Broker> pBroker,rptChapter* pChapter,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& thisSegmentKey) const;
   void ReportOverlayLoad(rptChapter* pChapter,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,bool bRating,const CSegmentKey& thisSegmentKey) const;
   void ReportConstructionLoad(rptChapter* pChapter,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& thisSegmentKey) const;
   void ReportLongitudinalJointLoad(rptChapter* pChapter, std::shared_ptr<IBridge> pBridge, std::shared_ptr<IProductLoads> pProdLoads, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const CSegmentKey& thisSegmentKey) const;
   void ReportShearKeyLoad(rptChapter* pChapter,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& thisSegmentKey,bool& one_girder_has_shear_key) const;
   void ReportPrecastDiaphragmLoad(rptChapter* pChapter,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& thisSegmentKey) const;
   void ReportCastInPlaceDiaphragmLoad(rptChapter* pChapter,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IProductLoads> pProdLoads,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSpanKey& spanKey) const;
   
   void ReportLiveLoad(rptChapter* pChapter,bool bDesign,bool bRating,std::shared_ptr<IRatingSpecification> pRatingSpec,bool& bPermit) const;
   void ReportLimitStates(rptChapter* pChapter,bool bDesign,bool bRating,bool bPermit,bool one_girder_has_shear_key,std::shared_ptr<IRatingSpecification> pRatingSpec) const;
   void ReportEquivPretensionLoads(rptChapter* pChatper,bool bRating,std::shared_ptr<IBridge> pBridge,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CGirderKey& girderKey) const;
   void ReportEquivSegmentPostTensioningLoads(rptChapter* pChapter, bool bRating, std::shared_ptr<IBridge> pBridge, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const CGirderKey& girderKey) const;

   rptParagraph* CreatePointLoadTable(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSpanKey& spanKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, Uint16 level) const;
   rptParagraph* CreateDistributedLoadTable(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSpanKey& spanKey, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, Uint16 level) const;
   rptParagraph* CreateMomentLoadTable(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSpanKey& spanKey, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, Uint16 level) const;

private:
   // set this to true for a shortened version of the chapter
   bool m_bSimplifiedVersion;
   bool m_bDesign;
   bool m_bRating;
};
