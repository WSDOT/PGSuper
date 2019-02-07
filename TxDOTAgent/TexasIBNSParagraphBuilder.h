///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_TEXASIBNSPARAGRAPHBUILDER_H_
#define INCLUDED_TEXASIBNSPARAGRAPHBUILDER_H_

#include <PgsExt\DebondUtil.h>
#include "TxDOTOptionalDesignUtilities.h"

#include <PgsExt\Keys.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CTexasIBNSParagraphBuilder

   Paragraph builder for Texas IBNS sheet


DESCRIPTION
   Paragraph builder for Texas IBNS sheet. This is a customized output paragraph
   for TxDOT.

LOG
   rdp : 06.13.2006 : Created file
*****************************************************************************/

class CTexasIBNSParagraphBuilder
{
public:
   CTexasIBNSParagraphBuilder();

   rptParagraph* Build(IBroker* pBroker,const std::vector<CSegmentKey>& segmentKeys,
                       IEAFDisplayUnits* pDisplayUnits,
                       Uint16 level, bool& rbEjectPage) const;

protected:

private:
   // Prevent accidental copying and assignment
   CTexasIBNSParagraphBuilder(const CTexasIBNSParagraphBuilder&) = delete;
   CTexasIBNSParagraphBuilder& operator=(const CTexasIBNSParagraphBuilder&) = delete;

   void WriteDebondTable(rptParagraph* pPara, IBroker* pBroker, const CSegmentKey& segmentKey, IEAFDisplayUnits* pDisplayUnits) const;
};

/****************************************************************************
CLASS	TxDOTIBNSDebondWriter
   Writes debond table to reports
****************************************************************************/

class TxDOTIBNSDebondWriter : public TxDOTDebondTool
{
public:
   TxDOTIBNSDebondWriter(const CSegmentKey& segmentKey, Float64 girderLength, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(segmentKey, girderLength, pStrandGeometry)
   {;}

   void WriteDebondData(rptParagraph* pPara,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const std::_tstring& optionalName);
};

#endif // INCLUDED_TEXASIBNSParagraphBUILDER_H_
