///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

class rptParagraph;

// Class to copy "all" properties. Note that it doesn't exactly do this. It is meant to copy the girder type
// and clear out data for subsequent copy girder properties transactions
class txnCopyGirderAllProperties :  public CEAFTransaction
{
public:
   txnCopyGirderAllProperties(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys);
   virtual ~txnCopyGirderAllProperties();
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;

   std::vector<std::_tstring> m_strOldNames;
};

class txnCopyGirderStirrups :  public CEAFTransaction
{
public:
   txnCopyGirderStirrups(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys);
   virtual ~txnCopyGirderStirrups();
   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;
   std::vector<CShearData2> m_OldShearData;
};

class txnCopyGirderPrestressing :  public CEAFTransaction
{
public:
   txnCopyGirderPrestressing(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys);
   virtual ~txnCopyGirderPrestressing();
   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;
   std::vector<CStrandData> m_OldPrestressData;
   std::vector<CSegmentPTData> m_OldSegmentPTData;
   std::vector<CPTData> m_OldPTData;
};

class txnCopyGirderHandling :  public CEAFTransaction
{
public:
   txnCopyGirderHandling(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys);
   virtual ~txnCopyGirderHandling();
   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;
   std::vector<CHandlingData> m_OldHandlingData;
};

class txnCopyGirderMaterial :  public CEAFTransaction
{
public:
   txnCopyGirderMaterial(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys);
   virtual ~txnCopyGirderMaterial();
   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;
   std::vector<CGirderMaterial> m_OldMaterialData;
};

class txnCopyGirderRebar :  public CEAFTransaction
{
public:
   txnCopyGirderRebar(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys);
   virtual ~txnCopyGirderRebar();
   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const { return true; }
   virtual bool IsRepeatable() const { return false; }

private:
   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;
   std::vector<CLongitudinalRebarData> m_OldRebarData;
};

//////////// Girder Copy Callback classes ////////////////////////////////////////////////////////////////


class CCopyGirderAllProperties : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderAllProperties();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual UINT GetGirderEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) override;
};

class CCopyGirderStirrups : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderStirrups();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual UINT GetGirderEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) override;
};

class CCopyGirderPrestressing : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderPrestressing();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual UINT GetGirderEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) override;
};

class CCopyGirderHandling : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderHandling();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual UINT GetGirderEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) override;
};

class CCopyGirderMaterial : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderMaterial();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual UINT GetGirderEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) override;
};

class CCopyGirderRebar : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderRebar();
   virtual LPCTSTR GetName() override;
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) override;
   virtual UINT GetGirderEditorTabIndex() override;
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) override;
};