#pragma once

#include <Graphing/PointMapper.h>
#include <PsgLib\PrecastSegmentData.h>
#include <PsgLib\Keys.h>

// CDrawPrecastSegmentControl

interface IPrecastSegmentDataSource
{
public:
   virtual const CSplicedGirderData* GetGirder() const = 0;
   virtual const CSegmentKey& GetSegmentKey() const = 0;
   virtual SegmentIDType GetSegmentID() const = 0;
};

class CDrawPrecastSegmentControl : public CWnd
{
	DECLARE_DYNAMIC(CDrawPrecastSegmentControl)

public:
	CDrawPrecastSegmentControl();
	virtual ~CDrawPrecastSegmentControl();

   void CustomInit(IPrecastSegmentDataSource* pSource);

   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   IPrecastSegmentDataSource* m_pSource;

   void CreateSegmentShape(const CSegmentKey& segmentKey,IShape** ppShape,IPoint2dCollection** ppPoints);
   void DrawShape(CDC* pDC, WBFL::Graphing::PointMapper& mapper,IShape* pShape);
   void DrawBottomFlange(CDC* pDC, WBFL::Graphing::PointMapper& mapper,IPoint2dCollection* pPoints);
public:
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


