// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/paint_worklet_style_property_map.h"

#include <memory>
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_computed_style_declaration.h"
#include "third_party/blink/renderer/core/css/css_test_helpers.h"
#include "third_party/blink/renderer/core/css/cssom/css_keyword_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_unit_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_unsupported_color_value.h"
#include "third_party/blink/renderer/core/css/cssom/paint_worklet_input.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"

namespace blink {

class PaintWorkletStylePropertyMapTest : public PageTestBase {
 public:
  PaintWorkletStylePropertyMapTest() = default;

  void SetUp() override { PageTestBase::SetUp(IntSize()); }

  Node* PageNode() { return GetDocument().documentElement(); }

  void ShutDown(base::WaitableEvent* waitable_event) {
    DCHECK(!IsMainThread());
    waitable_event->Signal();
  }

  void ShutDownThread() {
    base::WaitableEvent waitable_event;
    PostCrossThreadTask(
        *thread_->GetTaskRunner(), FROM_HERE,
        CrossThreadBindOnce(&PaintWorkletStylePropertyMapTest::ShutDown,
                            CrossThreadUnretained(this),
                            CrossThreadUnretained(&waitable_event)));
    waitable_event.Wait();
  }

  void CheckCrossThreadData(base::WaitableEvent* waitable_event,
                            scoped_refptr<PaintWorkletInput> input) {
    DCHECK(!IsMainThread());

    PaintWorkletStylePropertyMap* map =
        MakeGarbageCollected<PaintWorkletStylePropertyMap>(
            input->StyleMapData());
    DCHECK(map);

    const PaintWorkletStylePropertyMap::CrossThreadData& data =
        map->StyleMapDataForTest();
    EXPECT_EQ(data.size(), 5u);
    EXPECT_EQ(data.at("--foo")->ToCSSStyleValue()->GetType(),
              CSSStyleValue::StyleValueType::kUnitType);
    EXPECT_EQ(To<CSSUnitValue>(data.at("--foo")->ToCSSStyleValue())->value(),
              134);
    EXPECT_EQ(To<CSSUnitValue>(data.at("--foo")->ToCSSStyleValue())->unit(),
              "px");
    EXPECT_EQ(data.at("--bar")->ToCSSStyleValue()->GetType(),
              CSSStyleValue::StyleValueType::kUnitType);
    EXPECT_EQ(To<CSSUnitValue>(data.at("--bar")->ToCSSStyleValue())->value(),
              42);
    EXPECT_EQ(data.at("--loo")->ToCSSStyleValue()->GetType(),
              CSSStyleValue::StyleValueType::kKeywordType);
    EXPECT_EQ(To<CSSKeywordValue>(data.at("--loo")->ToCSSStyleValue())->value(),
              "test");
    EXPECT_EQ(data.at("--gar")->ToCSSStyleValue()->GetType(),
              CSSStyleValue::StyleValueType::kUnsupportedColorType);
    EXPECT_EQ(To<CSSUnsupportedColorValue>(data.at("--gar")->ToCSSStyleValue())
                  ->Value(),
              Color(0, 255, 0));
    EXPECT_EQ(data.at("display")->ToCSSStyleValue()->GetType(),
              CSSStyleValue::StyleValueType::kKeywordType);
    waitable_event->Signal();
  }

 protected:
  std::unique_ptr<blink::Thread> thread_;
};

TEST_F(PaintWorkletStylePropertyMapTest, CreateSupportedCrossThreadData) {
  Vector<CSSPropertyID> native_properties({CSSPropertyID::kDisplay});
  Vector<AtomicString> custom_properties({"--foo", "--bar", "--loo", "--gar"});
  css_test_helpers::RegisterProperty(GetDocument(), "--foo", "<length>",
                                     "134px", false);
  css_test_helpers::RegisterProperty(GetDocument(), "--bar", "<number>", "42",
                                     false);
  css_test_helpers::RegisterProperty(GetDocument(), "--loo", "test", "test",
                                     false);
  css_test_helpers::RegisterProperty(GetDocument(), "--gar", "<color>",
                                     "rgb(0, 255, 0)", false);

  UpdateAllLifecyclePhasesForTest();
  Node* node = PageNode();

  Vector<std::unique_ptr<CrossThreadStyleValue>> input_arguments;
  CompositorPaintWorkletInput::PropertyKeys input_property_keys;
  auto data = PaintWorkletStylePropertyMap::BuildCrossThreadData(
      GetDocument(), node->GetLayoutObject()->UniqueId(),
      node->ComputedStyleRef(), native_properties, custom_properties,
      input_property_keys);

  EXPECT_TRUE(data.has_value());
  std::vector<cc::PaintWorkletInput::PropertyKey> property_keys;
  scoped_refptr<PaintWorkletInput> input =
      base::MakeRefCounted<PaintWorkletInput>(
          "test", FloatSize(100, 100), 1.0f, 1.0f, 1, std::move(data.value()),
          std::move(input_arguments), std::move(property_keys));
  DCHECK(input);

  thread_ = blink::Thread::CreateThread(
      ThreadCreationParams(WebThreadType::kTestThread).SetSupportsGC(true));
  base::WaitableEvent waitable_event;
  PostCrossThreadTask(
      *thread_->GetTaskRunner(), FROM_HERE,
      CrossThreadBindOnce(
          &PaintWorkletStylePropertyMapTest::CheckCrossThreadData,
          CrossThreadUnretained(this), CrossThreadUnretained(&waitable_event),
          std::move(input)));
  waitable_event.Wait();

  ShutDownThread();
}

TEST_F(PaintWorkletStylePropertyMapTest, UnsupportedCrossThreadData) {
  Vector<CSSPropertyID> native_properties1;
  Vector<AtomicString> custom_properties1({"--foo", "--bar", "--loo"});
  css_test_helpers::RegisterProperty(GetDocument(), "--foo", "<url>",
                                     "url(https://google.com)", false);
  css_test_helpers::RegisterProperty(GetDocument(), "--bar", "<number>", "42",
                                     false);
  css_test_helpers::RegisterProperty(GetDocument(), "--loo", "test", "test",
                                     false);

  UpdateAllLifecyclePhasesForTest();
  Node* node = PageNode();

  Vector<std::unique_ptr<CrossThreadStyleValue>> input_arguments;
  CompositorPaintWorkletInput::PropertyKeys input_property_keys;
  auto data1 = PaintWorkletStylePropertyMap::BuildCrossThreadData(
      GetDocument(), node->GetLayoutObject()->UniqueId(),
      node->ComputedStyleRef(), native_properties1, custom_properties1,
      input_property_keys);

  EXPECT_FALSE(data1.has_value());

  Vector<CSSPropertyID> native_properties2(
      {CSSPropertyID::kDisplay, CSSPropertyID::kColor});
  Vector<AtomicString> custom_properties2;

  auto data2 = PaintWorkletStylePropertyMap::BuildCrossThreadData(
      GetDocument(), node->GetLayoutObject()->UniqueId(),
      node->ComputedStyleRef(), native_properties2, custom_properties2,
      input_property_keys);

  EXPECT_FALSE(data2.has_value());
}

}  // namespace blink
