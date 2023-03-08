#pragma once

#include "CoreMinimal.h"
#include "Input/CursorReply.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "ITimeSlider.h"
#include "AnimTimeline/SAnimTimeline.h"

class FSlateWindowElementList;
struct FContextMenuSuppressor;
struct FScrubRangeToScreen;
struct FSlateBrush;
class FAnimModel;

// struct FPaintSectionAreaViewArgs {
//   FPaintSectionAreaViewArgs()
//       : bDisplayTickLines(false), bDisplayScrubPosition(false) {}
//
//   /** Whether to display tick lines */
//   bool bDisplayTickLines;
//   /** Whether to display the scrub position */
//   bool bDisplayScrubPosition;
//   /** Optional Paint args for the playback range*/
//   TOptional<FPaintPlaybackRangeArgs> PlaybackRangeArgs;
// };

/**
 * A time slider controller for the anim timeline
 */
class FCreateCharacterSliderController : public ITimeSliderController {
 public:
  /** 开始 ITimeSliderController 接口 */
  virtual int32 OnPaintTimeSlider(bool bMirrorLabels, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override{};
  virtual int32 OnPaintViewArea(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bEnabled, const FPaintViewAreaArgs& Args) const override{};
  virtual FReply OnMouseButtonDown(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override{};
  virtual FReply OnMouseButtonUp(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override{};
  virtual FReply OnMouseMove(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override{};
  virtual FReply OnMouseWheel(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override{};
  virtual FCursorReply OnCursorQuery(TSharedRef<const SWidget> WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override{};
  virtual void SetViewRange(double NewRangeMin, double NewRangeMax, EViewRangeInterpolation Interpolation) override{};
  virtual void SetClampRange(double NewRangeMin, double NewRangeMax) override{};
  virtual void SetPlayRange(FFrameNumber RangeStart, int32 RangeDuration) override{};
  virtual FFrameRate GetDisplayRate() const override { return TimeSliderArgs.DisplayRate.Get(); }
  virtual FFrameRate GetTickResolution() const override { return TimeSliderArgs.TickResolution.Get(); }
  virtual FAnimatedRange GetViewRange() const override { return TimeSliderArgs.ViewRange.Get(); }
  virtual FAnimatedRange GetClampRange() const override { return TimeSliderArgs.ClampRange.Get(); }
  virtual TRange<FFrameNumber> GetPlayRange() const override { return TimeSliderArgs.PlaybackRange.Get(TRange<FFrameNumber>()); }
  virtual FFrameTime GetScrubPosition() const override { return TimeSliderArgs.ScrubPosition.Get(); }
  virtual void SetScrubPosition(FFrameTime InTime, bool bEvaluate) override { TimeSliderArgs.OnScrubPositionChanged.ExecuteIfBound(InTime, false, bEvaluate); }
  /** 结束 ITimeSliderController 接口 */

 private:
  FTimeSliderArgs TimeSliderArgs;
};