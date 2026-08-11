///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include <WbflTypes.h>
#include <LRFD\LiveLoadDistributionFactorBase.h>
#include <PsgLib\PointLoadData.h>
#include <PsgLib\DistributedLoadData.h>
#include <PsgLib\MomentLoadData.h>

/*****************************************************************************
INTERFACE
   IEditByUI

   Interface used to invoke elements of the user interface so that the user
   can edit input data.

DESCRIPTION
   Interface used to invoke elements of the user interface so that the user
   can edit input data.

   Most of these methods were originally intended for use by core agents only. The EditXxxDescription
   methods that open a tabbed dialog (EditAlignmentDescription, EditBridgeDescription,
   EditPierDescription, EditSpanDescription, EditGirderDescription, EditSegmentDescription,
   EditTemporarySupportDescription, EditClosureJointDescription) are also safe for an extension
   agent to call, e.g. to add a menu command that opens straight to the agent's own extension page -
   pass the page name returned from your IExtensionPageCallback::GetPropertyPageName() override.
   See \ref creating_an_extension_agent "Creating an Extension Agent" for the pattern. The remaining
   methods on this interface (loads, timeline, live load factors, etc.) are still core-agent-only.

   Future versions of PGSuper will permit Agents to supply their own user interface
   and this will likely eliminate the need for this interface. This interface
   may be removed in the future.
*****************************************************************************/
// {E1CF3EAA-3E85-450a-9A67-D68FF321DC16}
DEFINE_GUID(IID_IEditByUI,
0xe1cf3eaa, 0x3e85, 0x450a, 0x9a, 0x67, 0xd6, 0x8f, 0xf3, 0x21, 0xdc, 0x16);
/// @brief Interface used to invoke elements of the user interface so that the user can edit input data.
///
/// Most methods are intended for core-agent use only. The EditXxxDescription methods that open a
/// tabbed dialog are also safe for an extension agent to call directly with a page name (its own,
/// via IExtensionPageCallback::GetPropertyPageName(), or one of the built-in XXXDLG_PAGE_* constants
/// in ExtendUI.h) - see \ref creating_an_extension_agent "Creating an Extension Agent".
///
/// Future versions of PGSuper will permit Agents to supply their own user interface
/// and this will likely eliminate the need for this interface. This interface
/// may be removed in the future.
class __declspec(uuid("{E1CF3EAA-3E85-450a-9A67-D68FF321DC16}")) IEditByUI
{
public:
   /// @brief Opens the Bridge Description dialog with the named page active (BRIDGEDLG_PAGE_xxx
   /// constants, IFace\ExtendUI.h, or an extension's own page name).
   virtual void EditBridgeDescription(LPCTSTR pageName) = 0;

   /// @brief Opens the Alignment Description dialog with the named page active. Built-in pages
   /// are named by the ALIGNMENTDLG_PAGE_xxx constants (IFace\ExtendUI.h); an extension agent's
   /// own page can be addressed the same way, by the name it was given when it was inserted
   /// (see CAlignmentDescriptionDlg::CreateExtensionPages).
   virtual void EditAlignmentDescription(LPCTSTR pageName) = 0;

   /// @brief Opens the Segment Description dialog with the named page active (GIRDERDLG_PAGE_xxx
   /// for PGSuper, SEGMENTDLG_PAGE_xxx for PGSplice, IFace\ExtendUI.h)
   /// @return Returns true if editing was successful
   virtual bool EditSegmentDescription(const CSegmentKey& segmentKey, LPCTSTR pageName) = 0;

   /// @brief Opens the Segment Description dialog for the currently selected segment
   /// @return Returns true if editing was successful
   virtual bool EditSegmentDescription() = 0;

   /// @brief Opens the Closure Joint Description dialog for the specifed closure joint
   /// @return Returns true if editing was successful
   virtual bool EditClosureJointDescription(const CClosureKey& closureKey, LPCTSTR pageName) = 0;

   /// @brief Opens the Girder Description dialog for the specified girder, with the named page
   /// active (GIRDERDLG_PAGE_xxx for PGSuper, SPLICEDGIRDERDLG_PAGE_xxx for PGSplice, IFace\ExtendUI.h)
   /// @return Returns true if editing was successful
   virtual bool EditGirderDescription(const CGirderKey& girderKey, LPCTSTR pageName) = 0;

   /// @brief Opens the Girder Description dialog for the currently selected girder
   /// @return Returns true if editing was successful
   virtual bool EditGirderDescription() = 0;

   /// @brief Opens the Span Description dialog for the specified span
   /// @return Returns true if editing was successful
   virtual bool EditSpanDescription(SpanIndexType spanIdx, LPCTSTR pageName) = 0;

   /// @brief Opens the Pier Description dialog for the specified pier
   /// @return Returns true if editing was successful
   virtual bool EditPierDescription(PierIndexType pierIdx, LPCTSTR pageName) = 0;

   /// @brief Opens the Temporary Support Dialog for the specified temporary support
   /// @return Returns true if editing was successful
   virtual bool EditTemporarySupportDescription(SupportIndexType tsIdx, LPCTSTR pageName) = 0;

   /// @brief Opens the Live Load editing dialog
   virtual void EditLiveLoads() = 0;

   /// @brief Opens the live load distribution factors dialog
   /// @param method Default value for distribution factor method
   /// @param roaAction Default value for range of applicability action
   virtual void EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,WBFL::LRFD::RangeOfApplicabilityAction roaAction) = 0;

   /// @brief Opens the Point Load editing dialog
   /// @param loadIdx Index of load to be edited
   /// @return Returns true if editing was successful
   virtual bool EditPointLoad(IndexType loadIdx) = 0;

   /// @brief Opens the Point Load editing dialog
   /// @param loadID ID of load to be edited
   /// @return Returns true if editing was successful
   virtual bool EditPointLoadByID(LoadIDType loadID) = 0;

   /// @brief Opens the Distributed Load editing dialog
   /// @param loadIdx Index of the load to be edited
   /// @return Returns true if editing was successful
   virtual bool EditDistributedLoad(IndexType loadIdx) = 0;

   /// @brief Opens the Distributed Load editing dialog
   /// @param loadID ID of the load to be edited
   /// @return Returns true if editing was successful
   virtual bool EditDistributedLoadByID(LoadIDType loadID) = 0;

   /// @brief Opens the Moment Load editing dialog
   /// @param loadIdx Index of the load to be edited
   /// @return Returns true if editing was successful
   virtual bool EditMomentLoad(IndexType loadIdx) = 0;
   
   /// @brief Opens the Moment Load editing dialog
   /// @param loadID ID of the load to be edited
   /// @return Returns true if editing was successful
   virtual bool EditMomentLoadByID(LoadIDType loadID) = 0;

   /// @brief Opens the timeline editor dialog
   /// @return Returns true if editing was successful
   virtual bool EditTimeline() = 0;

   /// @brief Opens the Cast Deck Activity dialog
   /// @return Returns true if editing was successful
   virtual bool EditCastDeckActivity() = 0;

   /// @brief Returns the ID of the standard toolbar
   virtual UINT GetStdToolBarID() = 0;

   /// @brief Returns the ID of the library editor toolbar
   virtual UINT GetLibToolBarID() = 0;

   /// @brief Returns the ID of the help toolbar
   virtual UINT GetHelpToolBarID() = 0;

   /// @brief Opens the Direct Section Prestressing editing dialog. NOTE: Strand fill type must be pgsTypes::sdtDirectSelection before entering this dialog
   /// @return Returns true if editing was successful
   virtual bool EditDirectSelectionPrestressing(const CSegmentKey& segmentKey) = 0;

   /// @brief Opens the Direct Row Input Prestressing editing dialog. NOTE: Strand fill type must be pgsTypes::sdtDirectRowInput before entering this dialog
   /// @return Returns true if editing was successful
   virtual bool EditDirectRowInputPrestressing(const CSegmentKey& segmentKey) = 0;

   /// @brief Opens the Direct Strand Input Prestressing editing dialog. NOTE: Strand fill type must be pgsTypes::sdtDirectStrandInput before entering this dialog
   /// @return Returns true if editing was successful
   virtual bool EditDirectStrandInputPrestressing(const CSegmentKey& segmentKey) = 0;

   /// @brief Adds a Point Load to the model
   /// @param loadData 
   virtual void AddPointLoad(const CPointLoadData& loadData) = 0;

   /// @brief Deletes a Point Load from the model
   /// @param loadIdx 
   virtual void DeletePointLoad(IndexType loadIdx) = 0;

   /// @brief Adds a Distributed Load to the model
   /// @param loadData 
   virtual void AddDistributedLoad(const CDistributedLoadData& loadData) = 0;

   /// @brief Deletes a Distributed Load from the model
   /// @param loadIdx 
   virtual void DeleteDistributedLoad(IndexType loadIdx) = 0;

   /// @brief Adds a Moment Load to the model
   /// @param loadData 
   virtual void AddMomentLoad(const CMomentLoadData& loadData) = 0;

   /// @brief Removes a Moment Load from the model
   /// @param loadIdx 
   virtual void DeleteMomentLoad(IndexType loadIdx) = 0;

   /// @brief Opens the Effective Flange Width dialog
   /// @return Returns true if editing was successful
   virtual bool EditEffectiveFlangeWidth() = 0;

   /// @brief Opens the project criteria selection dialog
   /// @return Returns true if editing was successful
   virtual bool SelectProjectCriteria() = 0;

   /// @brief Opens the Edit Bearings dialog
   /// @return Returns true if editing was successful
   virtual bool EditBearings() = 0;

   /// @brief Opens the Edit Haunch dialog
   /// @return Returns true if editing was successful
   virtual bool EditHaunch() = 0;
};
