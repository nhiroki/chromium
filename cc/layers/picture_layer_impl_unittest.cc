// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/picture_layer_impl.h"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

#include "cc/base/math_util.h"
#include "cc/layers/append_quads_data.h"
#include "cc/layers/picture_layer.h"
#include "cc/quads/draw_quad.h"
#include "cc/quads/tile_draw_quad.h"
#include "cc/test/begin_frame_args_test.h"
#include "cc/test/fake_content_layer_client.h"
#include "cc/test/fake_impl_proxy.h"
#include "cc/test/fake_layer_tree_host_impl.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/fake_picture_layer_impl.h"
#include "cc/test/fake_picture_pile_impl.h"
#include "cc/test/geometry_test_utils.h"
#include "cc/test/impl_side_painting_settings.h"
#include "cc/test/layer_test_common.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "cc/test/test_web_graphics_context_3d.h"
#include "cc/trees/layer_tree_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace cc {
namespace {

#define EXPECT_BOTH_EQ(expression, x)         \
  do {                                        \
    EXPECT_EQ(x, pending_layer_->expression); \
    EXPECT_EQ(x, active_layer_->expression);  \
  } while (false)

#define EXPECT_BOTH_NE(expression, x)         \
  do {                                        \
    EXPECT_NE(x, pending_layer_->expression); \
    EXPECT_NE(x, active_layer_->expression);  \
  } while (false)

class MockCanvas : public SkCanvas {
 public:
  explicit MockCanvas(int w, int h) : SkCanvas(w, h) {}

  void drawRect(const SkRect& rect, const SkPaint& paint) override {
    // Capture calls before SkCanvas quickReject() kicks in.
    rects_.push_back(rect);
  }

  std::vector<SkRect> rects_;
};

class NoLowResTilingsSettings : public ImplSidePaintingSettings {};

class LowResTilingsSettings : public ImplSidePaintingSettings {
 public:
  LowResTilingsSettings() { create_low_res_tiling = true; }
};

class PictureLayerImplTest : public testing::Test {
 public:
  PictureLayerImplTest()
      : proxy_(base::MessageLoopProxy::current()),
        host_impl_(LowResTilingsSettings(), &proxy_, &shared_bitmap_manager_),
        root_id_(6),
        id_(7),
        pending_layer_(nullptr),
        old_pending_layer_(nullptr),
        active_layer_(nullptr) {
    host_impl_.SetViewportSize(gfx::Size(10000, 10000));
  }

  explicit PictureLayerImplTest(const LayerTreeSettings& settings)
      : proxy_(base::MessageLoopProxy::current()),
        host_impl_(settings, &proxy_, &shared_bitmap_manager_),
        root_id_(6),
        id_(7) {
    host_impl_.SetViewportSize(gfx::Size(10000, 10000));
  }

  virtual ~PictureLayerImplTest() {
  }

  void SetUp() override { InitializeRenderer(); }

  virtual void InitializeRenderer() {
    host_impl_.InitializeRenderer(FakeOutputSurface::Create3d());
  }

  void SetupDefaultTrees(const gfx::Size& layer_bounds) {
    gfx::Size tile_size(100, 100);

    scoped_refptr<FakePicturePileImpl> pending_pile =
        FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
    scoped_refptr<FakePicturePileImpl> active_pile =
        FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

    SetupTrees(pending_pile, active_pile);
  }

  void SetupDefaultTreesWithInvalidation(const gfx::Size& layer_bounds,
                                         const Region& invalidation) {
    gfx::Size tile_size(100, 100);

    scoped_refptr<FakePicturePileImpl> pending_pile =
        FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
    scoped_refptr<FakePicturePileImpl> active_pile =
        FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

    SetupTreesWithInvalidation(pending_pile, active_pile, invalidation);
  }

  void ActivateTree() {
    host_impl_.ActivateSyncTree();
    CHECK(!host_impl_.pending_tree());
    CHECK(host_impl_.recycle_tree());
    old_pending_layer_ = pending_layer_;
    pending_layer_ = nullptr;
    active_layer_ = static_cast<FakePictureLayerImpl*>(
        host_impl_.active_tree()->LayerById(id_));

    host_impl_.active_tree()->UpdateDrawProperties();
  }

  void SetupDefaultTreesWithFixedTileSize(const gfx::Size& layer_bounds,
                                          const gfx::Size& tile_size,
                                          const Region& invalidation) {
    gfx::Size pile_tile_size(100, 100);

    scoped_refptr<FakePicturePileImpl> pending_pile =
        FakePicturePileImpl::CreateFilledPile(pile_tile_size, layer_bounds);
    scoped_refptr<FakePicturePileImpl> active_pile =
        FakePicturePileImpl::CreateFilledPile(pile_tile_size, layer_bounds);

    SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size,
                                invalidation);
  }

  void SetupTrees(
      scoped_refptr<PicturePileImpl> pending_pile,
      scoped_refptr<PicturePileImpl> active_pile) {
    SetupPendingTree(active_pile);
    ActivateTree();
    SetupPendingTreeInternal(pending_pile, gfx::Size(), Region());
  }

  void SetupTreesWithInvalidation(scoped_refptr<PicturePileImpl> pending_pile,
                                  scoped_refptr<PicturePileImpl> active_pile,
                                  const Region& pending_invalidation) {
    SetupPendingTreeInternal(active_pile, gfx::Size(), Region());
    ActivateTree();
    SetupPendingTreeInternal(pending_pile, gfx::Size(), pending_invalidation);
  }

  void SetupTreesWithFixedTileSize(scoped_refptr<PicturePileImpl> pending_pile,
                                   scoped_refptr<PicturePileImpl> active_pile,
                                   const gfx::Size& tile_size,
                                   const Region& pending_invalidation) {
    SetupPendingTreeInternal(active_pile, tile_size, Region());
    ActivateTree();
    SetupPendingTreeInternal(pending_pile, tile_size, pending_invalidation);
  }

  void SetupPendingTree(scoped_refptr<RasterSource> raster_source) {
    SetupPendingTreeInternal(raster_source, gfx::Size(), Region());
  }

  void SetupPendingTreeWithInvalidation(
      scoped_refptr<RasterSource> raster_source,
      const Region& invalidation) {
    SetupPendingTreeInternal(raster_source, gfx::Size(), invalidation);
  }

  void SetupPendingTreeWithFixedTileSize(
      scoped_refptr<RasterSource> raster_source,
      const gfx::Size& tile_size,
      const Region& invalidation) {
    SetupPendingTreeInternal(raster_source, tile_size, invalidation);
  }

  void SetupPendingTreeInternal(scoped_refptr<RasterSource> raster_source,
                                const gfx::Size& tile_size,
                                const Region& invalidation) {
    host_impl_.CreatePendingTree();
    host_impl_.pending_tree()->PushPageScaleFromMainThread(1.f, 0.25f, 100.f);
    LayerTreeImpl* pending_tree = host_impl_.pending_tree();

    // Steal from the recycled tree if possible.
    scoped_ptr<LayerImpl> pending_root = pending_tree->DetachLayerTree();
    scoped_ptr<FakePictureLayerImpl> pending_layer;
    DCHECK_IMPLIES(pending_root, pending_root->id() == root_id_);
    if (!pending_root) {
      pending_root = LayerImpl::Create(pending_tree, root_id_);
      pending_layer = FakePictureLayerImpl::Create(pending_tree, id_);
      if (!tile_size.IsEmpty())
        pending_layer->set_fixed_tile_size(tile_size);
      pending_layer->SetDrawsContent(true);
    } else {
      pending_layer.reset(static_cast<FakePictureLayerImpl*>(
          pending_root->RemoveChild(pending_root->children()[0]).release()));
      if (!tile_size.IsEmpty())
        pending_layer->set_fixed_tile_size(tile_size);
    }
    pending_root->SetHasRenderSurface(true);
    // The bounds() just mirror the pile size.
    pending_layer->SetBounds(raster_source->GetSize());
    pending_layer->SetContentBounds(raster_source->GetSize());
    pending_layer->SetRasterSourceOnPending(raster_source, invalidation);

    pending_root->AddChild(pending_layer.Pass());
    pending_tree->SetRootLayer(pending_root.Pass());

    pending_layer_ = static_cast<FakePictureLayerImpl*>(
        host_impl_.pending_tree()->LayerById(id_));

    // Add tilings/tiles for the layer.
    host_impl_.pending_tree()->UpdateDrawProperties();
  }

  void SetupDrawPropertiesAndUpdateTiles(FakePictureLayerImpl* layer,
                                         float ideal_contents_scale,
                                         float device_scale_factor,
                                         float page_scale_factor,
                                         float maximum_animation_contents_scale,
                                         bool animating_transform_to_screen) {
    layer->draw_properties().ideal_contents_scale = ideal_contents_scale;
    layer->draw_properties().device_scale_factor = device_scale_factor;
    layer->draw_properties().page_scale_factor = page_scale_factor;
    layer->draw_properties().maximum_animation_contents_scale =
        maximum_animation_contents_scale;
    layer->draw_properties().screen_space_transform_is_animating =
        animating_transform_to_screen;
    bool resourceless_software_draw = false;
    layer->UpdateTiles(Occlusion(), resourceless_software_draw);
  }
  static void VerifyAllTilesExistAndHavePile(
      const PictureLayerTiling* tiling,
      PicturePileImpl* pile) {
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             tiling->contents_scale(),
             gfx::Rect(tiling->tiling_size()));
         iter;
         ++iter) {
      EXPECT_TRUE(*iter);
      EXPECT_EQ(pile, iter->raster_source());
    }
  }

  void SetContentsScaleOnBothLayers(float contents_scale,
                                    float device_scale_factor,
                                    float page_scale_factor,
                                    float maximum_animation_contents_scale,
                                    bool animating_transform) {
    SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                      contents_scale,
                                      device_scale_factor,
                                      page_scale_factor,
                                      maximum_animation_contents_scale,
                                      animating_transform);

    SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                      contents_scale,
                                      device_scale_factor,
                                      page_scale_factor,
                                      maximum_animation_contents_scale,
                                      animating_transform);
  }

  void ResetTilingsAndRasterScales() {
    pending_layer_->ReleaseResources();
    active_layer_->ReleaseResources();
    if (pending_layer_)
      EXPECT_EQ(0u, pending_layer_->tilings()->num_tilings());
    if (active_layer_)
      EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
  }

  void AssertAllTilesRequired(PictureLayerTiling* tiling) {
    std::vector<Tile*> tiles = tiling->AllTilesForTesting();
    for (size_t i = 0; i < tiles.size(); ++i)
      EXPECT_TRUE(tiles[i]->required_for_activation()) << "i: " << i;
    EXPECT_GT(tiles.size(), 0u);
  }

  void AssertNoTilesRequired(PictureLayerTiling* tiling) {
    std::vector<Tile*> tiles = tiling->AllTilesForTesting();
    for (size_t i = 0; i < tiles.size(); ++i)
      EXPECT_FALSE(tiles[i]->required_for_activation()) << "i: " << i;
    EXPECT_GT(tiles.size(), 0u);
  }

 protected:
  void TestQuadsForSolidColor(bool test_for_solid);

  FakeImplProxy proxy_;
  TestSharedBitmapManager shared_bitmap_manager_;
  FakeLayerTreeHostImpl host_impl_;
  int root_id_;
  int id_;
  FakePictureLayerImpl* pending_layer_;
  FakePictureLayerImpl* old_pending_layer_;
  FakePictureLayerImpl* active_layer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PictureLayerImplTest);
};

class NoLowResPictureLayerImplTest : public PictureLayerImplTest {
 public:
  NoLowResPictureLayerImplTest()
      : PictureLayerImplTest(NoLowResTilingsSettings()) {}
};

TEST_F(PictureLayerImplTest, TileGridAlignment) {
  // Layer to span 4 raster tiles in x and in y
  ImplSidePaintingSettings settings;
  gfx::Size layer_size(settings.default_tile_size.width() * 7 / 2,
                       settings.default_tile_size.height() * 7 / 2);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(layer_size, layer_size);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(layer_size, layer_size);

  SetupTrees(pending_pile, active_pile);

  // Add 1x1 rects at the centers of each tile, then re-record pile contents
  active_layer_->tilings()->tiling_at(0)->CreateAllTilesForTesting();
  std::vector<Tile*> tiles =
      active_layer_->tilings()->tiling_at(0)->AllTilesForTesting();
  EXPECT_EQ(16u, tiles.size());
  std::vector<SkRect> rects;
  std::vector<Tile*>::const_iterator tile_iter;
  for (tile_iter = tiles.begin(); tile_iter < tiles.end(); tile_iter++) {
    gfx::Point tile_center = (*tile_iter)->content_rect().CenterPoint();
    gfx::Rect rect(tile_center.x(), tile_center.y(), 1, 1);
    active_pile->add_draw_rect(rect);
    rects.push_back(SkRect::MakeXYWH(rect.x(), rect.y(), 1, 1));
  }
  // Force re-raster with newly injected content
  active_pile->RemoveRecordingAt(0, 0);
  active_pile->AddRecordingAt(0, 0);

  std::vector<SkRect>::const_iterator rect_iter = rects.begin();
  for (tile_iter = tiles.begin(); tile_iter < tiles.end(); tile_iter++) {
    MockCanvas mock_canvas(1000, 1000);
    active_pile->PlaybackToSharedCanvas(&mock_canvas,
                                        (*tile_iter)->content_rect(), 1.0f);

    // This test verifies that when drawing the contents of a specific tile
    // at content scale 1.0, the playback canvas never receives content from
    // neighboring tiles which indicates that the tile grid embedded in
    // SkPicture is perfectly aligned with the compositor's tiles.
    EXPECT_EQ(1u, mock_canvas.rects_.size());
    EXPECT_EQ(*rect_iter, mock_canvas.rects_[0]);
    rect_iter++;
  }
}

TEST_F(PictureLayerImplTest, CloneNoInvalidation) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTreesWithInvalidation(pending_pile, active_pile, Region());

  EXPECT_EQ(pending_layer_->tilings()->num_tilings(),
            active_layer_->tilings()->num_tilings());

  const PictureLayerTilingSet* tilings = pending_layer_->tilings();
  EXPECT_GT(tilings->num_tilings(), 0u);
  for (size_t i = 0; i < tilings->num_tilings(); ++i)
    VerifyAllTilesExistAndHavePile(tilings->tiling_at(i), pending_pile.get());
}

TEST_F(PictureLayerImplTest, ExternalViewportRectForPrioritizingTiles) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTreesWithInvalidation(pending_pile, active_pile, Region());

  SetupDrawPropertiesAndUpdateTiles(active_layer_, 1.f, 1.f, 1.f, 1.f, false);

  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  // Update tiles with viewport for tile priority as (0, 0, 100, 100) and the
  // identify transform for tile priority.
  bool resourceless_software_draw = false;
  gfx::Rect viewport = gfx::Rect(layer_bounds),
            viewport_rect_for_tile_priority = gfx::Rect(0, 0, 100, 100);
  gfx::Transform transform, transform_for_tile_priority;

  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport_rect_for_tile_priority,
                                        transform_for_tile_priority,
                                        resourceless_software_draw);
  host_impl_.active_tree()->UpdateDrawProperties();

  gfx::Rect viewport_rect_for_tile_priority_in_view_space =
      viewport_rect_for_tile_priority;

  // Verify the viewport rect for tile priority is used in picture layer tiling.
  EXPECT_EQ(viewport_rect_for_tile_priority_in_view_space,
            active_layer_->GetViewportForTilePriorityInContentSpace());
  PictureLayerTilingSet* tilings = active_layer_->tilings();
  for (size_t i = 0; i < tilings->num_tilings(); i++) {
    PictureLayerTiling* tiling = tilings->tiling_at(i);
    EXPECT_EQ(
        tiling->GetCurrentVisibleRectForTesting(),
        gfx::ScaleToEnclosingRect(viewport_rect_for_tile_priority_in_view_space,
                                  tiling->contents_scale()));
  }

  // Update tiles with viewport for tile priority as (200, 200, 100, 100) in
  // screen space and the transform for tile priority is translated and
  // rotated. The actual viewport for tile priority used by PictureLayerImpl
  // should be (200, 200, 100, 100) applied with the said transform.
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  viewport_rect_for_tile_priority = gfx::Rect(200, 200, 100, 100);
  transform_for_tile_priority.Translate(100, 100);
  transform_for_tile_priority.Rotate(45);
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport_rect_for_tile_priority,
                                        transform_for_tile_priority,
                                        resourceless_software_draw);
  host_impl_.active_tree()->UpdateDrawProperties();

  gfx::Transform screen_to_view(gfx::Transform::kSkipInitialization);
  bool success = transform_for_tile_priority.GetInverse(&screen_to_view);
  EXPECT_TRUE(success);

  // Note that we don't clip this to the layer bounds, since it is expected that
  // the rect will sometimes be outside of the layer bounds. If we clip to
  // bounds, then tile priorities will end up being incorrect in cases of fully
  // offscreen layer.
  viewport_rect_for_tile_priority_in_view_space =
      gfx::ToEnclosingRect(MathUtil::ProjectClippedRect(
          screen_to_view, viewport_rect_for_tile_priority));

  EXPECT_EQ(viewport_rect_for_tile_priority_in_view_space,
            active_layer_->GetViewportForTilePriorityInContentSpace());
  tilings = active_layer_->tilings();
  for (size_t i = 0; i < tilings->num_tilings(); i++) {
    PictureLayerTiling* tiling = tilings->tiling_at(i);
    EXPECT_EQ(
        tiling->GetCurrentVisibleRectForTesting(),
        gfx::ScaleToEnclosingRect(viewport_rect_for_tile_priority_in_view_space,
                                  tiling->contents_scale()));
  }
}

TEST_F(PictureLayerImplTest, InvalidViewportForPrioritizingTiles) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTreesWithInvalidation(pending_pile, active_pile, Region());

  SetupDrawPropertiesAndUpdateTiles(active_layer_, 1.f, 1.f, 1.f, 1.f, false);

  // UpdateTiles with valid viewport. Should update tile viewport.
  // Note viewport is considered invalid if and only if in resourceless
  // software draw.
  bool resourceless_software_draw = false;
  gfx::Rect viewport = gfx::Rect(layer_bounds);
  gfx::Transform transform;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);
  active_layer_->draw_properties().visible_content_rect = viewport;
  active_layer_->draw_properties().screen_space_transform = transform;
  active_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  gfx::Rect visible_rect_for_tile_priority =
      active_layer_->visible_rect_for_tile_priority();
  EXPECT_FALSE(visible_rect_for_tile_priority.IsEmpty());
  gfx::Transform screen_space_transform_for_tile_priority =
      active_layer_->screen_space_transform();

  // Expand viewport and set it as invalid for prioritizing tiles.
  // Should update viewport and transform, but not update visible rect.
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  resourceless_software_draw = true;
  viewport = gfx::ScaleToEnclosingRect(viewport, 2);
  transform.Translate(1.f, 1.f);
  active_layer_->draw_properties().visible_content_rect = viewport;
  active_layer_->draw_properties().screen_space_transform = transform;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);
  active_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  // Transform for tile priority is updated.
  EXPECT_TRANSFORMATION_MATRIX_EQ(transform,
                                  active_layer_->screen_space_transform());
  // Visible rect for tile priority retains old value.
  EXPECT_EQ(visible_rect_for_tile_priority,
            active_layer_->visible_rect_for_tile_priority());

  // Keep expanded viewport but mark it valid. Should update tile viewport.
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  resourceless_software_draw = false;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);
  active_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  EXPECT_TRANSFORMATION_MATRIX_EQ(transform,
                                  active_layer_->screen_space_transform());
  EXPECT_EQ(viewport, active_layer_->visible_rect_for_tile_priority());
}

TEST_F(PictureLayerImplTest, ClonePartialInvalidation) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);
  gfx::Rect layer_invalidation(150, 200, 30, 180);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> lost_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupPendingTreeWithFixedTileSize(lost_pile, gfx::Size(50, 50), Region());
  ActivateTree();
  // Add a non-shared tiling on the active tree.
  PictureLayerTiling* tiling = active_layer_->AddTiling(3.f);
  tiling->CreateAllTilesForTesting();
  // Then setup a new pending tree and activate it.
  SetupTreesWithFixedTileSize(pending_pile, active_pile, gfx::Size(50, 50),
                              layer_invalidation);

  EXPECT_EQ(2u, pending_layer_->num_tilings());
  EXPECT_EQ(3u, active_layer_->num_tilings());

  const PictureLayerTilingSet* tilings = pending_layer_->tilings();
  EXPECT_GT(tilings->num_tilings(), 0u);
  for (size_t i = 0; i < tilings->num_tilings(); ++i) {
    const PictureLayerTiling* tiling = tilings->tiling_at(i);
    gfx::Rect content_invalidation = gfx::ScaleToEnclosingRect(
        layer_invalidation,
        tiling->contents_scale());
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             tiling->contents_scale(),
             gfx::Rect(tiling->tiling_size()));
         iter;
         ++iter) {
      EXPECT_TRUE(*iter);
      EXPECT_FALSE(iter.geometry_rect().IsEmpty());
      EXPECT_EQ(pending_pile.get(), iter->raster_source());
    }
  }

  tilings = active_layer_->tilings();
  EXPECT_GT(tilings->num_tilings(), 0u);
  for (size_t i = 0; i < tilings->num_tilings(); ++i) {
    const PictureLayerTiling* tiling = tilings->tiling_at(i);
    gfx::Rect content_invalidation =
        gfx::ScaleToEnclosingRect(layer_invalidation, tiling->contents_scale());
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             tiling->contents_scale(),
             gfx::Rect(tiling->tiling_size()));
         iter;
         ++iter) {
      EXPECT_TRUE(*iter);
      EXPECT_FALSE(iter.geometry_rect().IsEmpty());
      if (iter.geometry_rect().Intersects(content_invalidation))
        EXPECT_EQ(active_pile.get(), iter->raster_source());
      else if (!active_layer_->GetPendingOrActiveTwinTiling(tiling))
        EXPECT_EQ(active_pile.get(), iter->raster_source());
      else
        EXPECT_EQ(pending_pile.get(), iter->raster_source());
    }
  }
}

TEST_F(PictureLayerImplTest, CloneFullInvalidation) {
  gfx::Size tile_size(90, 80);
  gfx::Size layer_bounds(300, 500);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTreesWithInvalidation(pending_pile, active_pile,
                             gfx::Rect(layer_bounds));

  EXPECT_EQ(pending_layer_->tilings()->num_tilings(),
            active_layer_->tilings()->num_tilings());

  const PictureLayerTilingSet* tilings = pending_layer_->tilings();
  EXPECT_GT(tilings->num_tilings(), 0u);
  for (size_t i = 0; i < tilings->num_tilings(); ++i)
    VerifyAllTilesExistAndHavePile(tilings->tiling_at(i), pending_pile.get());
}

TEST_F(PictureLayerImplTest, ManageTilingsCreatesTilings) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  EXPECT_LT(low_res_factor, 1.f);

  active_layer_->ReleaseResources();
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());

  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    6.f,  // ideal contents scale
                                    3.f,  // device scale
                                    2.f,  // page scale
                                    1.f,  // maximum animation scale
                                    false);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(6.f * low_res_factor,
                  active_layer_->tilings()->tiling_at(1)->contents_scale());

  // If we change the page scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    6.6f,  // ideal contents scale
                                    3.f,   // device scale
                                    2.2f,  // page scale
                                    1.f,   // maximum animation scale
                                    false);
  ASSERT_EQ(4u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.6f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(6.6f * low_res_factor,
                  active_layer_->tilings()->tiling_at(2)->contents_scale());

  // If we change the device scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    7.26f,  // ideal contents scale
                                    3.3f,   // device scale
                                    2.2f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(6u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(7.26f * low_res_factor,
                  active_layer_->tilings()->tiling_at(3)->contents_scale());

  // If we change the device scale factor, but end up at the same total scale
  // factor somehow, then we don't get new tilings.
  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    7.26f,  // ideal contents scale
                                    2.2f,   // device scale
                                    3.3f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(6u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(7.26f * low_res_factor,
                  active_layer_->tilings()->tiling_at(3)->contents_scale());
}

TEST_F(PictureLayerImplTest, PendingLayerOnlyHasHighAndLowResTiling) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  EXPECT_LT(low_res_factor, 1.f);

  pending_layer_->ReleaseResources();
  EXPECT_EQ(0u, pending_layer_->tilings()->num_tilings());

  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    6.f,  // ideal contents scale
                                    3.f,  // device scale
                                    2.f,  // page scale
                                    1.f,  // maximum animation scale
                                    false);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(6.f * low_res_factor,
                  pending_layer_->tilings()->tiling_at(1)->contents_scale());

  // If we change the page scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    6.6f,  // ideal contents scale
                                    3.f,   // device scale
                                    2.2f,  // page scale
                                    1.f,   // maximum animation scale
                                    false);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.6f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(6.6f * low_res_factor,
                  pending_layer_->tilings()->tiling_at(1)->contents_scale());

  // If we change the device scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    7.26f,  // ideal contents scale
                                    3.3f,   // device scale
                                    2.2f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(7.26f * low_res_factor,
                  pending_layer_->tilings()->tiling_at(1)->contents_scale());

  // If we change the device scale factor, but end up at the same total scale
  // factor somehow, then we don't get new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    7.26f,  // ideal contents scale
                                    2.2f,   // device scale
                                    3.3f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(7.26f * low_res_factor,
                  pending_layer_->tilings()->tiling_at(1)->contents_scale());
}

TEST_F(PictureLayerImplTest, CreateTilingsEvenIfTwinHasNone) {
  // This test makes sure that if a layer can have tilings, then a commit makes
  // it not able to have tilings (empty size), and then a future commit that
  // makes it valid again should be able to create tilings.
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> empty_pile =
      FakePicturePileImpl::CreateEmptyPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> valid_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupPendingTree(valid_pile);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());

  ActivateTree();
  SetupPendingTree(empty_pile);
  EXPECT_FALSE(pending_layer_->CanHaveTilings());
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());
  ASSERT_EQ(0u, pending_layer_->tilings()->num_tilings());

  ActivateTree();
  EXPECT_FALSE(active_layer_->CanHaveTilings());
  ASSERT_EQ(0u, active_layer_->tilings()->num_tilings());

  SetupPendingTree(valid_pile);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());
  ASSERT_EQ(0u, active_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, ZoomOutCrash) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  // Set up the high and low res tilings before pinch zoom.
  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);
  ResetTilingsAndRasterScales();
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
  SetContentsScaleOnBothLayers(32.0f, 1.0f, 32.0f, 1.0f, false);
  EXPECT_EQ(32.f, active_layer_->HighResTiling()->contents_scale());
  host_impl_.PinchGestureBegin();
  SetContentsScaleOnBothLayers(1.0f, 1.0f, 1.0f, 1.0f, false);
  SetContentsScaleOnBothLayers(1.0f, 1.0f, 1.0f, 1.0f, false);
  EXPECT_EQ(active_layer_->tilings()->NumHighResTilings(), 1);
}

TEST_F(PictureLayerImplTest, PinchGestureTilings) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  // Set up the high and low res tilings before pinch zoom.
  SetupTrees(pending_pile, active_pile);
  ResetTilingsAndRasterScales();

  SetContentsScaleOnBothLayers(2.f, 1.0f, 2.f, 1.0f, false);
  EXPECT_BOTH_EQ(num_tilings(), 2u);
  EXPECT_BOTH_EQ(tilings()->tiling_at(0)->contents_scale(), 2.f);
  EXPECT_BOTH_EQ(tilings()->tiling_at(1)->contents_scale(),
                 2.f * low_res_factor);

  // Start a pinch gesture.
  host_impl_.PinchGestureBegin();

  // Zoom out by a small amount. We should create a tiling at half
  // the scale (2/kMaxScaleRatioDuringPinch).
  SetContentsScaleOnBothLayers(1.8f, 1.0f, 1.8f, 1.0f, false);
  EXPECT_EQ(3u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(2.0f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(1.0f,
                  active_layer_->tilings()->tiling_at(1)->contents_scale());
  EXPECT_FLOAT_EQ(2.0f * low_res_factor,
                  active_layer_->tilings()->tiling_at(2)->contents_scale());

  // Zoom out further, close to our low-res scale factor. We should
  // use that tiling as high-res, and not create a new tiling.
  SetContentsScaleOnBothLayers(low_res_factor * 2.1f, 1.0f,
                               low_res_factor * 2.1f, 1.0f, false);
  EXPECT_EQ(3u, active_layer_->tilings()->num_tilings());

  // Zoom in a lot now. Since we increase by increments of
  // kMaxScaleRatioDuringPinch, this will create a new tiling at 4.0.
  SetContentsScaleOnBothLayers(3.8f, 1.0f, 3.8f, 1.f, false);
  EXPECT_EQ(4u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(4.0f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
}

TEST_F(PictureLayerImplTest, SnappedTilingDuringZoom) {
  gfx::Size tile_size(300, 300);
  gfx::Size layer_bounds(2600, 3800);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  ResetTilingsAndRasterScales();
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());

  // Set up the high and low res tilings before pinch zoom.
  SetContentsScaleOnBothLayers(0.24f, 1.0f, 0.24f, 1.0f, false);
  EXPECT_EQ(2u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(0.24f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(0.0625f,
                  active_layer_->tilings()->tiling_at(1)->contents_scale());

  // Start a pinch gesture.
  host_impl_.PinchGestureBegin();

  // Zoom out by a small amount. We should create a tiling at half
  // the scale (1/kMaxScaleRatioDuringPinch).
  SetContentsScaleOnBothLayers(0.2f, 1.0f, 0.2f, 1.0f, false);
  EXPECT_EQ(3u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(0.24f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_FLOAT_EQ(0.12f,
                  active_layer_->tilings()->tiling_at(1)->contents_scale());
  EXPECT_FLOAT_EQ(0.0625,
                  active_layer_->tilings()->tiling_at(2)->contents_scale());

  // Zoom out further, close to our low-res scale factor. We should
  // use that tiling as high-res, and not create a new tiling.
  SetContentsScaleOnBothLayers(0.1f, 1.0f, 0.1f, 1.0f, false);
  EXPECT_EQ(3u, active_layer_->tilings()->num_tilings());

  // Zoom in. 0.25(desired_scale) should be snapped to 0.24 during zoom-in
  // because 0.25(desired_scale) is within the ratio(1.2).
  SetContentsScaleOnBothLayers(0.25f, 1.0f, 0.25f, 1.0f, false);
  EXPECT_EQ(3u, active_layer_->tilings()->num_tilings());

  // Zoom in a lot. Since we move in factors of two, we should get a scale that
  // is a power of 2 times 0.24.
  SetContentsScaleOnBothLayers(1.f, 1.0f, 1.f, 1.0f, false);
  EXPECT_EQ(4u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(1.92f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
}

TEST_F(PictureLayerImplTest, CleanUpTilings) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  std::vector<PictureLayerTiling*> used_tilings;

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  EXPECT_LT(low_res_factor, 1.f);

  float scale = 1.f;
  float page_scale = 1.f;

  SetupTrees(pending_pile, active_pile);
  EXPECT_EQ(2u, active_layer_->tilings()->num_tilings());
  EXPECT_EQ(1.f, active_layer_->HighResTiling()->contents_scale());

  // We only have ideal tilings, so they aren't removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  EXPECT_EQ(2u, active_layer_->tilings()->num_tilings());

  host_impl_.PinchGestureBegin();

  // Changing the ideal but not creating new tilings.
  scale = 1.5f;
  page_scale = 1.5f;
  SetContentsScaleOnBothLayers(scale, 1.f, page_scale, 1.f, false);
  EXPECT_EQ(2u, active_layer_->tilings()->num_tilings());

  // The tilings are still our target scale, so they aren't removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());

  host_impl_.PinchGestureEnd();

  // Create a 1.2 scale tiling. Now we have 1.0 and 1.2 tilings. Ideal = 1.2.
  scale = 1.2f;
  page_scale = 1.2f;
  SetContentsScaleOnBothLayers(1.2f, 1.f, page_scale, 1.f, false);
  ASSERT_EQ(4u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(
      1.f,
      active_layer_->tilings()->tiling_at(1)->contents_scale());
  EXPECT_FLOAT_EQ(
      1.f * low_res_factor,
      active_layer_->tilings()->tiling_at(3)->contents_scale());

  // Mark the non-ideal tilings as used. They won't be removed.
  used_tilings.clear();
  used_tilings.push_back(active_layer_->tilings()->tiling_at(1));
  used_tilings.push_back(active_layer_->tilings()->tiling_at(3));
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(4u, active_layer_->tilings()->num_tilings());

  // Now move the ideal scale to 0.5. Our target stays 1.2.
  SetContentsScaleOnBothLayers(0.5f, 1.f, page_scale, 1.f, false);

  // The high resolution tiling is between target and ideal, so is not
  // removed.  The low res tiling for the old ideal=1.0 scale is removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(3u, active_layer_->tilings()->num_tilings());

  // Now move the ideal scale to 1.0. Our target stays 1.2.
  SetContentsScaleOnBothLayers(1.f, 1.f, page_scale, 1.f, false);

  // All the tilings are between are target and the ideal, so they are not
  // removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(3u, active_layer_->tilings()->num_tilings());

  // Now move the ideal scale to 1.1 on the active layer. Our target stays 1.2.
  SetupDrawPropertiesAndUpdateTiles(active_layer_, 1.1f, 1.f, page_scale, 1.f,
                                    false);

  // Because the pending layer's ideal scale is still 1.0, our tilings fall
  // in the range [1.0,1.2] and are kept.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(3u, active_layer_->tilings()->num_tilings());

  // Move the ideal scale on the pending layer to 1.1 as well. Our target stays
  // 1.2 still.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_, 1.1f, 1.f, page_scale, 1.f,
                                    false);

  // Our 1.0 tiling now falls outside the range between our ideal scale and our
  // target raster scale. But it is in our used tilings set, so nothing is
  // deleted.
  used_tilings.clear();
  used_tilings.push_back(active_layer_->tilings()->tiling_at(1));
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(3u, active_layer_->tilings()->num_tilings());

  // If we remove it from our used tilings set, it is outside the range to keep
  // so it is deleted.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, DontAddLowResDuringAnimation) {
  // Make sure this layer covers multiple tiles, since otherwise low
  // res won't get created because it is too small.
  gfx::Size tile_size(host_impl_.settings().default_tile_size);
  // Avoid max untiled layer size heuristics via fixed tile size.
  gfx::Size layer_bounds(tile_size.width() + 1, tile_size.height() + 1);
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, Region());

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  float contents_scale = 1.f;
  float device_scale = 1.f;
  float page_scale = 1.f;
  float maximum_animation_scale = 1.f;
  bool animating_transform = true;

  ResetTilingsAndRasterScales();

  // Animating, so don't create low res even if there isn't one already.
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 1.f);
  EXPECT_BOTH_EQ(num_tilings(), 1u);

  // Stop animating, low res gets created.
  animating_transform = false;
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 1.f);
  EXPECT_BOTH_EQ(LowResTiling()->contents_scale(), low_res_factor);
  EXPECT_BOTH_EQ(num_tilings(), 2u);

  // Page scale animation, new high res, but no low res. We still have
  // a tiling at the previous scale, it's just not marked as low res on the
  // active layer. The pending layer drops non-ideal tilings.
  contents_scale = 2.f;
  page_scale = 2.f;
  maximum_animation_scale = 2.f;
  animating_transform = true;
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 2.f);
  EXPECT_FALSE(active_layer_->LowResTiling());
  EXPECT_FALSE(pending_layer_->LowResTiling());
  EXPECT_EQ(3u, active_layer_->num_tilings());
  EXPECT_EQ(1u, pending_layer_->num_tilings());

  // Stop animating, new low res gets created for final page scale.
  animating_transform = false;
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 2.f);
  EXPECT_BOTH_EQ(LowResTiling()->contents_scale(), 2.f * low_res_factor);
  EXPECT_EQ(4u, active_layer_->num_tilings());
  EXPECT_EQ(2u, pending_layer_->num_tilings());
}

TEST_F(PictureLayerImplTest, DontAddLowResForSmallLayers) {
  gfx::Size layer_bounds(host_impl_.settings().default_tile_size);
  gfx::Size tile_size(100, 100);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  float device_scale = 1.f;
  float page_scale = 1.f;
  float maximum_animation_scale = 1.f;
  bool animating_transform = false;

  // Contents exactly fit on one tile at scale 1, no low res.
  float contents_scale = 1.f;
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), contents_scale);
  EXPECT_BOTH_EQ(num_tilings(), 1u);

  ResetTilingsAndRasterScales();

  // Contents that are smaller than one tile, no low res.
  contents_scale = 0.123f;
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), contents_scale);
  EXPECT_BOTH_EQ(num_tilings(), 1u);

  ResetTilingsAndRasterScales();

  // Any content bounds that would create more than one tile will
  // generate a low res tiling.
  contents_scale = 2.5f;
  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), contents_scale);
  EXPECT_BOTH_EQ(LowResTiling()->contents_scale(),
                 contents_scale * low_res_factor);
  EXPECT_BOTH_EQ(num_tilings(), 2u);

  // Mask layers dont create low res since they always fit on one tile.
  scoped_ptr<FakePictureLayerImpl> mask =
      FakePictureLayerImpl::CreateMaskWithRasterSource(
          host_impl_.pending_tree(), 3, pending_pile);
  mask->SetBounds(layer_bounds);
  mask->SetContentBounds(layer_bounds);
  mask->SetDrawsContent(true);

  SetupDrawPropertiesAndUpdateTiles(mask.get(), contents_scale, device_scale,
                                    page_scale, maximum_animation_scale,
                                    animating_transform);
  EXPECT_EQ(mask->HighResTiling()->contents_scale(), contents_scale);
  EXPECT_EQ(mask->num_tilings(), 1u);
}

TEST_F(PictureLayerImplTest, HugeMasksGetScaledDown) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  scoped_refptr<FakePicturePileImpl> valid_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTree(valid_pile);

  scoped_ptr<FakePictureLayerImpl> mask_ptr =
      FakePictureLayerImpl::CreateMaskWithRasterSource(
          host_impl_.pending_tree(), 3, valid_pile);
  mask_ptr->SetBounds(layer_bounds);
  mask_ptr->SetContentBounds(layer_bounds);
  mask_ptr->SetDrawsContent(true);
  pending_layer_->SetMaskLayer(mask_ptr.Pass());
  pending_layer_->SetHasRenderSurface(true);

  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  FakePictureLayerImpl* pending_mask =
      static_cast<FakePictureLayerImpl*>(pending_layer_->mask_layer());

  EXPECT_EQ(1.f, pending_mask->HighResTiling()->contents_scale());
  EXPECT_EQ(1u, pending_mask->num_tilings());

  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(
      pending_mask->HighResTiling()->AllTilesForTesting());

  ActivateTree();

  FakePictureLayerImpl* active_mask =
      static_cast<FakePictureLayerImpl*>(active_layer_->mask_layer());

  // Mask layers have a tiling with a single tile in it.
  EXPECT_EQ(1u, active_mask->HighResTiling()->AllTilesForTesting().size());
  // The mask resource exists.
  ResourceProvider::ResourceId mask_resource_id;
  gfx::Size mask_texture_size;
  active_mask->GetContentsResourceId(&mask_resource_id, &mask_texture_size);
  EXPECT_NE(0u, mask_resource_id);
  EXPECT_EQ(active_mask->bounds(), mask_texture_size);

  // Drop resources and recreate them, still the same.
  pending_mask->ReleaseResources();
  active_mask->ReleaseResources();
  SetupDrawPropertiesAndUpdateTiles(active_mask, 1.f, 1.f, 1.f, 1.f, false);
  active_mask->HighResTiling()->CreateAllTilesForTesting();
  EXPECT_EQ(1u, active_mask->HighResTiling()->AllTilesForTesting().size());
  EXPECT_NE(0u, mask_resource_id);
  EXPECT_EQ(active_mask->bounds(), mask_texture_size);

  // Resize larger than the max texture size.
  int max_texture_size = host_impl_.GetRendererCapabilities().max_texture_size;
  gfx::Size huge_bounds(max_texture_size + 1, 10);
  scoped_refptr<FakePicturePileImpl> huge_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, huge_bounds);

  SetupPendingTree(huge_pile);
  pending_mask->SetBounds(huge_bounds);
  pending_mask->SetContentBounds(huge_bounds);
  pending_mask->SetRasterSourceOnPending(huge_pile, Region());

  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  // The mask tiling gets scaled down.
  EXPECT_LT(pending_mask->HighResTiling()->contents_scale(), 1.f);
  EXPECT_EQ(1u, pending_mask->num_tilings());

  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(
      pending_mask->HighResTiling()->AllTilesForTesting());

  ActivateTree();

  // Mask layers have a tiling with a single tile in it.
  EXPECT_EQ(1u, active_mask->HighResTiling()->AllTilesForTesting().size());
  // The mask resource exists.
  active_mask->GetContentsResourceId(&mask_resource_id, &mask_texture_size);
  EXPECT_NE(0u, mask_resource_id);
  gfx::Size expected_size = active_mask->bounds();
  expected_size.SetToMin(gfx::Size(max_texture_size, max_texture_size));
  EXPECT_EQ(expected_size, mask_texture_size);

  // Drop resources and recreate them, still the same.
  pending_mask->ReleaseResources();
  active_mask->ReleaseResources();
  SetupDrawPropertiesAndUpdateTiles(active_mask, 1.f, 1.f, 1.f, 1.f, false);
  active_mask->HighResTiling()->CreateAllTilesForTesting();
  EXPECT_EQ(1u, active_mask->HighResTiling()->AllTilesForTesting().size());
  EXPECT_NE(0u, mask_resource_id);
  EXPECT_EQ(expected_size, mask_texture_size);

  // Do another activate, the same holds.
  SetupPendingTree(huge_pile);
  ActivateTree();
  EXPECT_EQ(1u, active_mask->HighResTiling()->AllTilesForTesting().size());
  active_layer_->GetContentsResourceId(&mask_resource_id, &mask_texture_size);
  EXPECT_EQ(expected_size, mask_texture_size);
  EXPECT_EQ(0u, mask_resource_id);

  // Resize even larger, so that the scale would be smaller than the minimum
  // contents scale. Then the layer should no longer have any tiling.
  float min_contents_scale = host_impl_.settings().minimum_contents_scale;
  gfx::Size extra_huge_bounds(max_texture_size / min_contents_scale + 1, 10);
  scoped_refptr<FakePicturePileImpl> extra_huge_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, extra_huge_bounds);

  SetupPendingTree(extra_huge_pile);
  pending_mask->SetBounds(extra_huge_bounds);
  pending_mask->SetContentBounds(extra_huge_bounds);
  pending_mask->SetRasterSourceOnPending(extra_huge_pile, Region());

  EXPECT_FALSE(pending_mask->CanHaveTilings());

  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  EXPECT_EQ(0u, pending_mask->num_tilings());
}

TEST_F(PictureLayerImplTest, ScaledMaskLayer) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  host_impl_.SetDeviceScaleFactor(1.3f);

  scoped_refptr<FakePicturePileImpl> valid_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTree(valid_pile);

  scoped_ptr<FakePictureLayerImpl> mask_ptr =
      FakePictureLayerImpl::CreateMaskWithRasterSource(
          host_impl_.pending_tree(), 3, valid_pile);
  mask_ptr->SetBounds(layer_bounds);
  mask_ptr->SetContentBounds(layer_bounds);
  mask_ptr->SetDrawsContent(true);
  pending_layer_->SetMaskLayer(mask_ptr.Pass());
  pending_layer_->SetHasRenderSurface(true);

  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  FakePictureLayerImpl* pending_mask =
      static_cast<FakePictureLayerImpl*>(pending_layer_->mask_layer());

  // Masks are scaled, and do not have a low res tiling.
  EXPECT_EQ(1.3f, pending_mask->HighResTiling()->contents_scale());
  EXPECT_EQ(1u, pending_mask->num_tilings());

  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(
      pending_mask->HighResTiling()->AllTilesForTesting());

  ActivateTree();

  FakePictureLayerImpl* active_mask =
      static_cast<FakePictureLayerImpl*>(active_layer_->mask_layer());

  // Mask layers have a tiling with a single tile in it.
  EXPECT_EQ(1u, active_mask->HighResTiling()->AllTilesForTesting().size());
  // The mask resource exists.
  ResourceProvider::ResourceId mask_resource_id;
  gfx::Size mask_texture_size;
  active_mask->GetContentsResourceId(&mask_resource_id, &mask_texture_size);
  EXPECT_NE(0u, mask_resource_id);
  gfx::Size expected_mask_texture_size =
      gfx::ToCeiledSize(gfx::ScaleSize(active_mask->bounds(), 1.3f));
  EXPECT_EQ(mask_texture_size, expected_mask_texture_size);
}

TEST_F(PictureLayerImplTest, ReleaseResources) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);
  EXPECT_EQ(2u, pending_layer_->tilings()->num_tilings());

  // All tilings should be removed when losing output surface.
  active_layer_->ReleaseResources();
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
  pending_layer_->ReleaseResources();
  EXPECT_EQ(0u, pending_layer_->tilings()->num_tilings());

  // This should create new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    1.f,  // ideal contents scale
                                    1.f,  // device scale
                                    1.f,  // page scale
                                    1.f,  // maximum animation scale
                                    false);
  EXPECT_EQ(2u, pending_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, ClampTilesToToMaxTileSize) {
  // The default max tile size is larger than 400x400.
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(5000, 5000);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);
  EXPECT_GE(pending_layer_->tilings()->num_tilings(), 1u);

  pending_layer_->tilings()->tiling_at(0)->CreateAllTilesForTesting();

  // The default value.
  EXPECT_EQ(gfx::Size(256, 256).ToString(),
            host_impl_.settings().default_tile_size.ToString());

  Tile* tile = pending_layer_->tilings()->tiling_at(0)->AllTilesForTesting()[0];
  EXPECT_EQ(gfx::Size(256, 256).ToString(),
            tile->content_rect().size().ToString());

  ResetTilingsAndRasterScales();

  // Change the max texture size on the output surface context.
  scoped_ptr<TestWebGraphicsContext3D> context =
      TestWebGraphicsContext3D::Create();
  context->set_max_texture_size(140);
  host_impl_.DidLoseOutputSurface();
  host_impl_.InitializeRenderer(
      FakeOutputSurface::Create3d(context.Pass()).Pass());

  SetupDrawPropertiesAndUpdateTiles(pending_layer_, 1.f, 1.f, 1.f, 1.f, false);
  ASSERT_EQ(2u, pending_layer_->tilings()->num_tilings());

  pending_layer_->tilings()->tiling_at(0)->CreateAllTilesForTesting();

  // Verify the tiles are not larger than the context's max texture size.
  tile = pending_layer_->tilings()->tiling_at(0)->AllTilesForTesting()[0];
  EXPECT_GE(140, tile->content_rect().width());
  EXPECT_GE(140, tile->content_rect().height());
}

TEST_F(PictureLayerImplTest, ClampSingleTileToToMaxTileSize) {
  // The default max tile size is larger than 400x400.
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(500, 500);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);
  EXPECT_GE(pending_layer_->tilings()->num_tilings(), 1u);

  pending_layer_->tilings()->tiling_at(0)->CreateAllTilesForTesting();

  // The default value. The layer is smaller than this.
  EXPECT_EQ(gfx::Size(512, 512).ToString(),
            host_impl_.settings().max_untiled_layer_size.ToString());

  // There should be a single tile since the layer is small.
  PictureLayerTiling* high_res_tiling = pending_layer_->tilings()->tiling_at(0);
  EXPECT_EQ(1u, high_res_tiling->AllTilesForTesting().size());

  ResetTilingsAndRasterScales();

  // Change the max texture size on the output surface context.
  scoped_ptr<TestWebGraphicsContext3D> context =
      TestWebGraphicsContext3D::Create();
  context->set_max_texture_size(140);
  host_impl_.DidLoseOutputSurface();
  host_impl_.InitializeRenderer(
      FakeOutputSurface::Create3d(context.Pass()).Pass());

  SetupDrawPropertiesAndUpdateTiles(pending_layer_, 1.f, 1.f, 1.f, 1.f, false);
  ASSERT_LE(1u, pending_layer_->tilings()->num_tilings());

  pending_layer_->tilings()->tiling_at(0)->CreateAllTilesForTesting();

  // There should be more than one tile since the max texture size won't cover
  // the layer.
  high_res_tiling = pending_layer_->tilings()->tiling_at(0);
  EXPECT_LT(1u, high_res_tiling->AllTilesForTesting().size());

  // Verify the tiles are not larger than the context's max texture size.
  Tile* tile = pending_layer_->tilings()->tiling_at(0)->AllTilesForTesting()[0];
  EXPECT_GE(140, tile->content_rect().width());
  EXPECT_GE(140, tile->content_rect().height());
}

TEST_F(PictureLayerImplTest, DisallowTileDrawQuads) {
  scoped_ptr<RenderPass> render_pass = RenderPass::Create();

  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  gfx::Rect layer_invalidation(150, 200, 30, 180);
  SetupTreesWithInvalidation(pending_pile, active_pile, layer_invalidation);

  active_layer_->draw_properties().visible_content_rect =
      gfx::Rect(layer_bounds);

  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_RESOURCELESS_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  ASSERT_EQ(1U, render_pass->quad_list.size());
  EXPECT_EQ(DrawQuad::PICTURE_CONTENT,
            render_pass->quad_list.front()->material);
}

TEST_F(PictureLayerImplTest, SolidColorLayerHasVisibleFullCoverage) {
  scoped_ptr<RenderPass> render_pass = RenderPass::Create();

  gfx::Size tile_size(1000, 1000);
  gfx::Size layer_bounds(1500, 1500);
  gfx::Rect visible_rect(250, 250, 1000, 1000);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateEmptyPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateEmptyPile(tile_size, layer_bounds);

  pending_pile->set_is_solid_color(true);
  active_pile->set_is_solid_color(true);

  SetupTrees(pending_pile, active_pile);

  active_layer_->draw_properties().visible_content_rect = visible_rect;

  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  Region remaining = visible_rect;
  for (const auto& quad : render_pass->quad_list) {
    EXPECT_TRUE(visible_rect.Contains(quad->rect));
    EXPECT_TRUE(remaining.Contains(quad->rect));
    remaining.Subtract(quad->rect);
  }

  EXPECT_TRUE(remaining.IsEmpty());
}

TEST_F(PictureLayerImplTest, TileScalesWithSolidColorPile) {
  gfx::Size layer_bounds(200, 200);
  gfx::Size tile_size(host_impl_.settings().default_tile_size);
  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateEmptyPileThatThinksItHasRecordings(
          tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateEmptyPileThatThinksItHasRecordings(
          tile_size, layer_bounds);

  pending_pile->set_is_solid_color(false);
  active_pile->set_is_solid_color(true);
  SetupTrees(pending_pile, active_pile);
  // Solid color pile should not allow tilings at any scale.
  EXPECT_FALSE(active_layer_->CanHaveTilings());
  EXPECT_EQ(0.f, active_layer_->ideal_contents_scale());

  // Activate non-solid-color pending pile makes active layer can have tilings.
  ActivateTree();
  EXPECT_TRUE(active_layer_->CanHaveTilings());
  EXPECT_GT(active_layer_->ideal_contents_scale(), 0.f);
}

TEST_F(NoLowResPictureLayerImplTest, MarkRequiredOffscreenTiles) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);

  gfx::Transform transform;
  gfx::Transform transform_for_tile_priority;
  bool resourceless_software_draw = false;
  gfx::Rect viewport(0, 0, 100, 200);
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());

  EXPECT_EQ(1u, pending_layer_->num_tilings());
  EXPECT_EQ(viewport, pending_layer_->visible_rect_for_tile_priority());

  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  pending_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  int num_visible = 0;
  int num_offscreen = 0;

  for (PictureLayerTiling::TilingRasterTileIterator iter(
           pending_layer_->HighResTiling());
       iter; ++iter) {
    const Tile* tile = *iter;
    DCHECK(tile);
    if (tile->priority(PENDING_TREE).distance_to_visible == 0.f) {
      EXPECT_TRUE(tile->required_for_activation());
      num_visible++;
    } else {
      EXPECT_FALSE(tile->required_for_activation());
      num_offscreen++;
    }
  }

  EXPECT_GT(num_visible, 0);
  EXPECT_GT(num_offscreen, 0);
}

TEST_F(NoLowResPictureLayerImplTest,
       TileOutsideOfViewportForTilePriorityNotRequired) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);
  gfx::Rect external_viewport_for_tile_priority(400, 200);
  gfx::Rect visible_content_rect(200, 400);

  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size, Region());

  ASSERT_EQ(1u, pending_layer_->num_tilings());
  ASSERT_EQ(1.f, pending_layer_->HighResTiling()->contents_scale());

  // Set external viewport for tile priority.
  gfx::Rect viewport = gfx::Rect(layer_bounds);
  gfx::Transform transform;
  gfx::Transform transform_for_tile_priority;
  bool resourceless_software_draw = false;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        external_viewport_for_tile_priority,
                                        transform_for_tile_priority,
                                        resourceless_software_draw);
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  // Set visible content rect that is different from
  // external_viewport_for_tile_priority.
  pending_layer_->draw_properties().visible_content_rect = visible_content_rect;
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  pending_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  // Intersect the two rects. Any tile outside should not be required for
  // activation.
  gfx::Rect viewport_for_tile_priority =
      pending_layer_->GetViewportForTilePriorityInContentSpace();
  viewport_for_tile_priority.Intersect(pending_layer_->visible_content_rect());

  int num_inside = 0;
  int num_outside = 0;
  for (PictureLayerTiling::CoverageIterator iter(
           pending_layer_->HighResTiling(), 1.f, gfx::Rect(layer_bounds));
       iter; ++iter) {
    if (!*iter)
      continue;
    Tile* tile = *iter;
    if (viewport_for_tile_priority.Intersects(iter.geometry_rect())) {
      num_inside++;
      // Mark everything in viewport for tile priority as ready to draw.
      TileDrawInfo& draw_info = tile->draw_info();
      draw_info.SetSolidColorForTesting(SK_ColorRED);
    } else {
      num_outside++;
      EXPECT_FALSE(tile->required_for_activation());
    }
  }

  EXPECT_GT(num_inside, 0);
  EXPECT_GT(num_outside, 0);

  // Activate and draw active layer.
  host_impl_.ActivateSyncTree();
  host_impl_.active_tree()->UpdateDrawProperties();
  active_layer_->draw_properties().visible_content_rect = visible_content_rect;

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  // All tiles in activation rect is ready to draw.
  EXPECT_EQ(0u, data.num_missing_tiles);
  EXPECT_EQ(0u, data.num_incomplete_tiles);
  EXPECT_FALSE(active_layer_->only_used_low_res_last_append_quads());
}

TEST_F(PictureLayerImplTest, HighResTileIsComplete) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());
  ActivateTree();

  // All high res tiles have resources.
  std::vector<Tile*> tiles =
      active_layer_->tilings()->tiling_at(0)->AllTilesForTesting();
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(tiles);

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  // All high res tiles drew, nothing was incomplete.
  EXPECT_EQ(9u, render_pass->quad_list.size());
  EXPECT_EQ(0u, data.num_missing_tiles);
  EXPECT_EQ(0u, data.num_incomplete_tiles);
  EXPECT_FALSE(active_layer_->only_used_low_res_last_append_quads());
}

TEST_F(PictureLayerImplTest, HighResTileIsIncomplete) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());
  ActivateTree();

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  EXPECT_EQ(1u, render_pass->quad_list.size());
  EXPECT_EQ(1u, data.num_missing_tiles);
  EXPECT_EQ(0u, data.num_incomplete_tiles);
  EXPECT_TRUE(active_layer_->only_used_low_res_last_append_quads());
}

TEST_F(PictureLayerImplTest, HighResTileIsIncompleteLowResComplete) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());
  ActivateTree();

  std::vector<Tile*> low_tiles =
      active_layer_->tilings()->tiling_at(1)->AllTilesForTesting();
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(low_tiles);

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  EXPECT_EQ(1u, render_pass->quad_list.size());
  EXPECT_EQ(0u, data.num_missing_tiles);
  EXPECT_EQ(1u, data.num_incomplete_tiles);
  EXPECT_TRUE(active_layer_->only_used_low_res_last_append_quads());
}

TEST_F(PictureLayerImplTest, LowResTileIsIncomplete) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());
  ActivateTree();

  // All high res tiles have resources except one.
  std::vector<Tile*> high_tiles =
      active_layer_->tilings()->tiling_at(0)->AllTilesForTesting();
  high_tiles.erase(high_tiles.begin());
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(high_tiles);

  // All low res tiles have resources.
  std::vector<Tile*> low_tiles =
      active_layer_->tilings()->tiling_at(1)->AllTilesForTesting();
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(low_tiles);

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  // The missing high res tile was replaced by a low res tile.
  EXPECT_EQ(9u, render_pass->quad_list.size());
  EXPECT_EQ(0u, data.num_missing_tiles);
  EXPECT_EQ(1u, data.num_incomplete_tiles);
  EXPECT_FALSE(active_layer_->only_used_low_res_last_append_quads());
}

TEST_F(PictureLayerImplTest,
       HighResAndIdealResTileIsCompleteWhenRasterScaleIsNotIdeal) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);
  gfx::Size viewport_size(400, 400);

  host_impl_.SetViewportSize(viewport_size);
  host_impl_.SetDeviceScaleFactor(2.f);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size, Region());

  // One ideal tile exists, this will get used when drawing.
  std::vector<Tile*> ideal_tiles;
  EXPECT_EQ(2.f, active_layer_->HighResTiling()->contents_scale());
  ideal_tiles.push_back(active_layer_->HighResTiling()->TileAt(0, 0));
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(
      ideal_tiles);

  // Due to layer scale throttling, the raster contents scale is changed to 1,
  // while the ideal is still 2.
  SetupDrawPropertiesAndUpdateTiles(active_layer_, 1.f, 1.f, 1.f, 1.f, false);
  SetupDrawPropertiesAndUpdateTiles(active_layer_, 2.f, 1.f, 1.f, 1.f, false);

  EXPECT_EQ(1.f, active_layer_->HighResTiling()->contents_scale());
  EXPECT_EQ(1.f, active_layer_->raster_contents_scale());
  EXPECT_EQ(2.f, active_layer_->ideal_contents_scale());

  // Both tilings still exist.
  EXPECT_EQ(2.f, active_layer_->tilings()->tiling_at(0)->contents_scale());
  EXPECT_EQ(1.f, active_layer_->tilings()->tiling_at(1)->contents_scale());

  // All high res tiles have resources.
  std::vector<Tile*> high_tiles =
      active_layer_->HighResTiling()->AllTilesForTesting();
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(high_tiles);

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  // All high res tiles drew, and the one ideal res tile drew.
  ASSERT_GT(render_pass->quad_list.size(), 9u);
  EXPECT_EQ(gfx::SizeF(99.f, 99.f),
            TileDrawQuad::MaterialCast(render_pass->quad_list.front())
                ->tex_coord_rect.size());
  EXPECT_EQ(gfx::SizeF(49.5f, 49.5f),
            TileDrawQuad::MaterialCast(render_pass->quad_list.ElementAt(1))
                ->tex_coord_rect.size());

  // Neither the high res nor the ideal tiles were considered as incomplete.
  EXPECT_EQ(0u, data.num_missing_tiles);
  EXPECT_EQ(0u, data.num_incomplete_tiles);
  EXPECT_FALSE(active_layer_->only_used_low_res_last_append_quads());
}

TEST_F(PictureLayerImplTest, HighResRequiredWhenUnsharedActiveAllReady) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  // No tiles shared.
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size,
                                     gfx::Rect(layer_bounds));

  active_layer_->SetAllTilesReady();

  // No shared tiles and all active tiles ready, so pending can only
  // activate with all high res tiles.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertAllTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, HighResRequiredWhenMissingHighResFlagOn) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  // All tiles shared (no invalidation).
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, Region());

  // Verify active tree not ready.
  Tile* some_active_tile =
      active_layer_->HighResTiling()->AllTilesForTesting()[0];
  EXPECT_FALSE(some_active_tile->IsReadyToDraw());

  // When high res are required, even if the active tree is not ready,
  // the high res tiles must be ready.
  host_impl_.SetRequiresHighResToDraw();

  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertAllTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, AllHighResRequiredEvenIfShared) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, Region());

  Tile* some_active_tile =
      active_layer_->HighResTiling()->AllTilesForTesting()[0];
  EXPECT_FALSE(some_active_tile->IsReadyToDraw());

  // All tiles shared (no invalidation), so even though the active tree's
  // tiles aren't ready, the high res tiles are required for activation.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertAllTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, DisallowRequiredForActivation) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, Region());

  Tile* some_active_tile =
      active_layer_->HighResTiling()->AllTilesForTesting()[0];
  EXPECT_FALSE(some_active_tile->IsReadyToDraw());

  pending_layer_->HighResTiling()->set_can_require_tiles_for_activation(false);
  pending_layer_->LowResTiling()->set_can_require_tiles_for_activation(false);

  // If we disallow required for activation, no tiles can be required.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertNoTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, NothingRequiredIfActiveMissingTiles) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  // This pile will create tilings, but has no recordings so will not create any
  // tiles.  This is attempting to simulate scrolling past the end of recorded
  // content on the active layer, where the recordings are so far away that
  // no tiles are created.
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateEmptyPileThatThinksItHasRecordings(
          tile_size, layer_bounds);

  SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size, Region());

  // Active layer has tilings, but no tiles due to missing recordings.
  EXPECT_TRUE(active_layer_->CanHaveTilings());
  EXPECT_EQ(active_layer_->tilings()->num_tilings(), 2u);
  EXPECT_EQ(active_layer_->HighResTiling()->AllTilesForTesting().size(), 0u);

  // Since the active layer has no tiles at all, the pending layer doesn't
  // need content in order to activate.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertNoTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, HighResRequiredIfActiveCantHaveTiles) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateEmptyPile(tile_size, layer_bounds);
  SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size, Region());

  // Active layer can't have tiles.
  EXPECT_FALSE(active_layer_->CanHaveTilings());

  // All high res tiles required.  This should be considered identical
  // to the case where there is no active layer, to avoid flashing content.
  // This can happen if a layer exists for a while and switches from
  // not being able to have content to having content.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertAllTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, HighResRequiredWhenActiveHasDifferentBounds) {
  gfx::Size pending_layer_bounds(400, 400);
  gfx::Size active_layer_bounds(200, 200);
  gfx::Size tile_size(100, 100);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, pending_layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, active_layer_bounds);

  SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size, Region());

  // Since the active layer has different bounds, the pending layer needs all
  // high res tiles in order to activate.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertAllTilesRequired(pending_layer_->HighResTiling());
  AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(PictureLayerImplTest, ActivateUninitializedLayer) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);
  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  host_impl_.CreatePendingTree();
  LayerTreeImpl* pending_tree = host_impl_.pending_tree();

  scoped_ptr<FakePictureLayerImpl> pending_layer =
      FakePictureLayerImpl::CreateWithRasterSource(pending_tree, id_,
                                                   pending_pile);
  pending_layer->SetDrawsContent(true);
  pending_tree->SetRootLayer(pending_layer.Pass());

  pending_layer_ = static_cast<FakePictureLayerImpl*>(
      host_impl_.pending_tree()->LayerById(id_));

  // Set some state on the pending layer, make sure it is not clobbered
  // by a sync from the active layer.  This could happen because if the
  // pending layer has not been post-commit initialized it will attempt
  // to sync from the active layer.
  float raster_page_scale = 10.f * pending_layer_->raster_page_scale();
  pending_layer_->set_raster_page_scale(raster_page_scale);

  host_impl_.ActivateSyncTree();

  active_layer_ = static_cast<FakePictureLayerImpl*>(
      host_impl_.active_tree()->LayerById(id_));

  EXPECT_EQ(0u, active_layer_->num_tilings());
  EXPECT_EQ(raster_page_scale, active_layer_->raster_page_scale());
}

TEST_F(PictureLayerImplTest, ShareTilesOnNextFrame) {
  gfx::Size layer_bounds(1500, 1500);
  gfx::Size tile_size(100, 100);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupPendingTree(pending_pile);

  PictureLayerTiling* tiling = pending_layer_->HighResTiling();
  gfx::Rect first_invalidate = tiling->TilingDataForTesting().TileBounds(0, 0);
  first_invalidate.Inset(tiling->TilingDataForTesting().border_texels(),
                         tiling->TilingDataForTesting().border_texels());
  gfx::Rect second_invalidate = tiling->TilingDataForTesting().TileBounds(1, 1);
  second_invalidate.Inset(tiling->TilingDataForTesting().border_texels(),
                          tiling->TilingDataForTesting().border_texels());

  ActivateTree();

  // Make a pending tree with an invalidated raster tile 0,0.
  SetupPendingTreeWithInvalidation(pending_pile, first_invalidate);

  // Activate and make a pending tree with an invalidated raster tile 1,1.
  ActivateTree();

  SetupPendingTreeWithInvalidation(pending_pile, second_invalidate);

  PictureLayerTiling* pending_tiling = pending_layer_->tilings()->tiling_at(0);
  PictureLayerTiling* active_tiling = active_layer_->tilings()->tiling_at(0);

  // pending_tiling->CreateAllTilesForTesting();

  // Tile 0,0 should be shared, but tile 1,1 should not be.
  EXPECT_EQ(active_tiling->TileAt(0, 0), pending_tiling->TileAt(0, 0));
  EXPECT_EQ(active_tiling->TileAt(1, 0), pending_tiling->TileAt(1, 0));
  EXPECT_EQ(active_tiling->TileAt(0, 1), pending_tiling->TileAt(0, 1));
  EXPECT_NE(active_tiling->TileAt(1, 1), pending_tiling->TileAt(1, 1));
  EXPECT_TRUE(pending_tiling->TileAt(0, 0)->is_shared());
  EXPECT_TRUE(pending_tiling->TileAt(1, 0)->is_shared());
  EXPECT_TRUE(pending_tiling->TileAt(0, 1)->is_shared());
  EXPECT_FALSE(pending_tiling->TileAt(1, 1)->is_shared());

  // Drop the tiles on the active tree and recreate them. The same tiles
  // should be shared or not.
  active_tiling->ComputeTilePriorityRects(gfx::Rect(), 1.f, 1.0, Occlusion());
  EXPECT_TRUE(active_tiling->AllTilesForTesting().empty());
  active_tiling->CreateAllTilesForTesting();

  // Tile 0,0 should be shared, but tile 1,1 should not be.
  EXPECT_EQ(active_tiling->TileAt(0, 0), pending_tiling->TileAt(0, 0));
  EXPECT_EQ(active_tiling->TileAt(1, 0), pending_tiling->TileAt(1, 0));
  EXPECT_EQ(active_tiling->TileAt(0, 1), pending_tiling->TileAt(0, 1));
  EXPECT_NE(active_tiling->TileAt(1, 1), pending_tiling->TileAt(1, 1));
  EXPECT_TRUE(pending_tiling->TileAt(0, 0)->is_shared());
  EXPECT_TRUE(pending_tiling->TileAt(1, 0)->is_shared());
  EXPECT_TRUE(pending_tiling->TileAt(0, 1)->is_shared());
  EXPECT_FALSE(pending_tiling->TileAt(1, 1)->is_shared());
}

TEST_F(PictureLayerImplTest, ShareTilesWithNoInvalidation) {
  SetupDefaultTrees(gfx::Size(1500, 1500));

  EXPECT_GE(active_layer_->num_tilings(), 1u);
  EXPECT_GE(pending_layer_->num_tilings(), 1u);

  // No invalidation, so all tiles are shared.
  PictureLayerTiling* active_tiling = active_layer_->tilings()->tiling_at(0);
  PictureLayerTiling* pending_tiling = pending_layer_->tilings()->tiling_at(0);
  ASSERT_TRUE(active_tiling);
  ASSERT_TRUE(pending_tiling);

  EXPECT_TRUE(active_tiling->TileAt(0, 0));
  EXPECT_TRUE(active_tiling->TileAt(1, 0));
  EXPECT_TRUE(active_tiling->TileAt(0, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1));

  EXPECT_TRUE(pending_tiling->TileAt(0, 0));
  EXPECT_TRUE(pending_tiling->TileAt(1, 0));
  EXPECT_TRUE(pending_tiling->TileAt(0, 1));
  EXPECT_TRUE(pending_tiling->TileAt(1, 1));

  EXPECT_EQ(active_tiling->TileAt(0, 0), pending_tiling->TileAt(0, 0));
  EXPECT_TRUE(active_tiling->TileAt(0, 0)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(1, 0), pending_tiling->TileAt(1, 0));
  EXPECT_TRUE(active_tiling->TileAt(1, 0)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(0, 1), pending_tiling->TileAt(0, 1));
  EXPECT_TRUE(active_tiling->TileAt(0, 1)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(1, 1), pending_tiling->TileAt(1, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1)->is_shared());
}

TEST_F(PictureLayerImplTest, ShareInvalidActiveTreeTiles) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1500, 1500);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupTreesWithInvalidation(pending_pile, active_pile, gfx::Rect(1, 1));
  // Activate the invalidation.
  ActivateTree();
  // Make another pending tree without any invalidation in it.
  scoped_refptr<FakePicturePileImpl> pending_pile2 =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTree(pending_pile2);

  EXPECT_GE(active_layer_->num_tilings(), 1u);
  EXPECT_GE(pending_layer_->num_tilings(), 1u);

  // The active tree invalidation was handled by the active tiles, so they
  // can be shared with the pending tree.
  PictureLayerTiling* active_tiling = active_layer_->tilings()->tiling_at(0);
  PictureLayerTiling* pending_tiling = pending_layer_->tilings()->tiling_at(0);
  ASSERT_TRUE(active_tiling);
  ASSERT_TRUE(pending_tiling);

  EXPECT_TRUE(active_tiling->TileAt(0, 0));
  EXPECT_TRUE(active_tiling->TileAt(1, 0));
  EXPECT_TRUE(active_tiling->TileAt(0, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1));

  EXPECT_TRUE(pending_tiling->TileAt(0, 0));
  EXPECT_TRUE(pending_tiling->TileAt(1, 0));
  EXPECT_TRUE(pending_tiling->TileAt(0, 1));
  EXPECT_TRUE(pending_tiling->TileAt(1, 1));

  EXPECT_EQ(active_tiling->TileAt(0, 0), pending_tiling->TileAt(0, 0));
  EXPECT_TRUE(active_tiling->TileAt(0, 0)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(1, 0), pending_tiling->TileAt(1, 0));
  EXPECT_TRUE(active_tiling->TileAt(1, 0)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(0, 1), pending_tiling->TileAt(0, 1));
  EXPECT_TRUE(active_tiling->TileAt(0, 1)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(1, 1), pending_tiling->TileAt(1, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1)->is_shared());
}

TEST_F(PictureLayerImplTest, RecreateInvalidPendingTreeTiles) {
  // Set some invalidation on the pending tree. We should replace raster tiles
  // that touch this.
  SetupDefaultTreesWithInvalidation(gfx::Size(1500, 1500), gfx::Rect(1, 1));

  EXPECT_GE(active_layer_->num_tilings(), 1u);
  EXPECT_GE(pending_layer_->num_tilings(), 1u);

  // The pending tree invalidation means tiles can not be shared with the
  // active tree.
  PictureLayerTiling* active_tiling = active_layer_->tilings()->tiling_at(0);
  PictureLayerTiling* pending_tiling = pending_layer_->tilings()->tiling_at(0);
  ASSERT_TRUE(active_tiling);
  ASSERT_TRUE(pending_tiling);

  EXPECT_TRUE(active_tiling->TileAt(0, 0));
  EXPECT_TRUE(active_tiling->TileAt(1, 0));
  EXPECT_TRUE(active_tiling->TileAt(0, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1));

  EXPECT_TRUE(pending_tiling->TileAt(0, 0));
  EXPECT_TRUE(pending_tiling->TileAt(1, 0));
  EXPECT_TRUE(pending_tiling->TileAt(0, 1));
  EXPECT_TRUE(pending_tiling->TileAt(1, 1));

  EXPECT_NE(active_tiling->TileAt(0, 0), pending_tiling->TileAt(0, 0));
  EXPECT_FALSE(active_tiling->TileAt(0, 0)->is_shared());
  EXPECT_FALSE(pending_tiling->TileAt(0, 0)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(1, 0), pending_tiling->TileAt(1, 0));
  EXPECT_TRUE(active_tiling->TileAt(1, 0)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(0, 1), pending_tiling->TileAt(0, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1)->is_shared());
  EXPECT_EQ(active_tiling->TileAt(1, 1), pending_tiling->TileAt(1, 1));
  EXPECT_TRUE(active_tiling->TileAt(1, 1)->is_shared());
}

TEST_F(PictureLayerImplTest, SyncTilingAfterGpuRasterizationToggles) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(10, 10);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  EXPECT_TRUE(pending_layer_->tilings()->FindTilingWithScale(1.f));
  EXPECT_TRUE(active_layer_->tilings()->FindTilingWithScale(1.f));

  // Gpu rasterization is disabled by default.
  EXPECT_FALSE(host_impl_.use_gpu_rasterization());
  // Toggling the gpu rasterization clears all tilings on both trees.
  host_impl_.SetUseGpuRasterization(true);
  EXPECT_EQ(0u, pending_layer_->tilings()->num_tilings());
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());

  // Make sure that we can still add tiling to the pending layer,
  // that gets synced to the active layer.
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();
  EXPECT_TRUE(pending_layer_->tilings()->FindTilingWithScale(1.f));

  ActivateTree();
  EXPECT_TRUE(active_layer_->tilings()->FindTilingWithScale(1.f));

  SetupPendingTree(pending_pile);
  EXPECT_TRUE(pending_layer_->tilings()->FindTilingWithScale(1.f));

  // Toggling the gpu rasterization clears all tilings on both trees.
  EXPECT_TRUE(host_impl_.use_gpu_rasterization());
  host_impl_.SetUseGpuRasterization(false);
  EXPECT_EQ(0u, pending_layer_->tilings()->num_tilings());
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, HighResCreatedWhenBoundsShrink) {
  gfx::Size tile_size(100, 100);

  // Put 0.5 as high res.
  host_impl_.SetDeviceScaleFactor(0.5f);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, gfx::Size(10, 10));
  SetupPendingTree(pending_pile);

  // Sanity checks.
  EXPECT_EQ(1u, pending_layer_->tilings()->num_tilings());
  EXPECT_TRUE(pending_layer_->tilings()->FindTilingWithScale(0.5f));

  ActivateTree();

  // Now, set the bounds to be 1x1, so that minimum contents scale becomes 1.
  pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, gfx::Size(1, 1));
  SetupPendingTree(pending_pile);

  // Another sanity check.
  EXPECT_EQ(1.f, pending_layer_->MinimumContentsScale());

  // Since the MinContentsScale is 1, the 0.5 tiling should be replaced by a 1.0
  // tiling.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_, 0.5f, 1.f, 1.f, 1.f, false);

  EXPECT_EQ(1u, pending_layer_->tilings()->num_tilings());
  PictureLayerTiling* tiling =
      pending_layer_->tilings()->FindTilingWithScale(1.0f);
  ASSERT_TRUE(tiling);
  EXPECT_EQ(HIGH_RESOLUTION, tiling->resolution());
}

TEST_F(PictureLayerImplTest, LowResTilingWithoutGpuRasterization) {
  gfx::Size default_tile_size(host_impl_.settings().default_tile_size);
  gfx::Size layer_bounds(default_tile_size.width() * 4,
                         default_tile_size.height() * 4);

  host_impl_.SetUseGpuRasterization(false);

  SetupDefaultTrees(layer_bounds);
  EXPECT_FALSE(host_impl_.use_gpu_rasterization());
  // Should have a low-res and a high-res tiling.
  EXPECT_EQ(2u, pending_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, NoLowResTilingWithGpuRasterization) {
  gfx::Size default_tile_size(host_impl_.settings().default_tile_size);
  gfx::Size layer_bounds(default_tile_size.width() * 4,
                         default_tile_size.height() * 4);

  host_impl_.SetUseGpuRasterization(true);

  SetupDefaultTrees(layer_bounds);
  EXPECT_TRUE(host_impl_.use_gpu_rasterization());
  // Should only have the high-res tiling.
  EXPECT_EQ(1u, pending_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, NoTilingIfDoesNotDrawContent) {
  // Set up layers with tilings.
  SetupDefaultTrees(gfx::Size(10, 10));
  SetContentsScaleOnBothLayers(1.f, 1.f, 1.f, 1.f, false);
  pending_layer_->PushPropertiesTo(active_layer_);
  EXPECT_TRUE(pending_layer_->DrawsContent());
  EXPECT_TRUE(pending_layer_->CanHaveTilings());
  EXPECT_GE(pending_layer_->num_tilings(), 0u);
  EXPECT_GE(active_layer_->num_tilings(), 0u);

  // Set content to false, which should make CanHaveTilings return false.
  pending_layer_->SetDrawsContent(false);
  EXPECT_FALSE(pending_layer_->DrawsContent());
  EXPECT_FALSE(pending_layer_->CanHaveTilings());

  // No tilings should be pushed to active layer.
  pending_layer_->PushPropertiesTo(active_layer_);
  EXPECT_EQ(0u, active_layer_->num_tilings());
}

TEST_F(PictureLayerImplTest, FirstTilingDuringPinch) {
  SetupDefaultTrees(gfx::Size(10, 10));

  // We start with a tiling at scale 1.
  EXPECT_EQ(1.f, pending_layer_->HighResTiling()->contents_scale());

  // When we scale up by 2.3, we get a new tiling that is a power of 2, in this
  // case 4.
  host_impl_.PinchGestureBegin();
  float high_res_scale = 2.3f;
  SetContentsScaleOnBothLayers(high_res_scale, 1.f, 1.f, 1.f, false);
  EXPECT_EQ(4.f, pending_layer_->HighResTiling()->contents_scale());
}

TEST_F(PictureLayerImplTest, PinchingTooSmall) {
  SetupDefaultTrees(gfx::Size(10, 10));

  // We start with a tiling at scale 1.
  EXPECT_EQ(1.f, pending_layer_->HighResTiling()->contents_scale());

  host_impl_.PinchGestureBegin();
  float high_res_scale = 0.0001f;
  EXPECT_LT(high_res_scale, pending_layer_->MinimumContentsScale());

  SetContentsScaleOnBothLayers(high_res_scale, 1.f, high_res_scale, 1.f, false);
  EXPECT_FLOAT_EQ(pending_layer_->MinimumContentsScale(),
                  pending_layer_->HighResTiling()->contents_scale());
}

TEST_F(PictureLayerImplTest, PinchingTooSmallWithContentsScale) {
  SetupDefaultTrees(gfx::Size(10, 10));

  ResetTilingsAndRasterScales();

  float contents_scale = 0.15f;
  SetContentsScaleOnBothLayers(contents_scale, 1.f, 1.f, 1.f, false);

  ASSERT_GE(pending_layer_->num_tilings(), 0u);
  EXPECT_FLOAT_EQ(contents_scale,
                  pending_layer_->HighResTiling()->contents_scale());

  host_impl_.PinchGestureBegin();

  float page_scale = 0.0001f;
  EXPECT_LT(page_scale * contents_scale,
            pending_layer_->MinimumContentsScale());

  SetContentsScaleOnBothLayers(contents_scale * page_scale, 1.f, page_scale,
                               1.f, false);
  ASSERT_GE(pending_layer_->num_tilings(), 0u);
  EXPECT_FLOAT_EQ(pending_layer_->MinimumContentsScale(),
                  pending_layer_->HighResTiling()->contents_scale());
}

class DeferredInitPictureLayerImplTest : public PictureLayerImplTest {
 public:
  void InitializeRenderer() override {
    bool delegated_rendering = false;
    host_impl_.InitializeRenderer(FakeOutputSurface::CreateDeferredGL(
        scoped_ptr<SoftwareOutputDevice>(new SoftwareOutputDevice),
        delegated_rendering));
  }

  void SetUp() override {
    PictureLayerImplTest::SetUp();

    // Create some default active and pending trees.
    gfx::Size tile_size(100, 100);
    gfx::Size layer_bounds(400, 400);

    scoped_refptr<FakePicturePileImpl> pending_pile =
        FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
    scoped_refptr<FakePicturePileImpl> active_pile =
        FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

    SetupTrees(pending_pile, active_pile);
  }
};

// This test is really a LayerTreeHostImpl test, in that it makes sure
// that trees need update draw properties after deferred initialization.
// However, this is also a regression test for PictureLayerImpl in that
// not having this update will cause a crash.
TEST_F(DeferredInitPictureLayerImplTest, PreventUpdateTilesDuringLostContext) {
  host_impl_.pending_tree()->UpdateDrawProperties();
  host_impl_.active_tree()->UpdateDrawProperties();
  EXPECT_FALSE(host_impl_.pending_tree()->needs_update_draw_properties());
  EXPECT_FALSE(host_impl_.active_tree()->needs_update_draw_properties());

  FakeOutputSurface* fake_output_surface =
      static_cast<FakeOutputSurface*>(host_impl_.output_surface());
  ASSERT_TRUE(fake_output_surface->InitializeAndSetContext3d(
      TestContextProvider::Create()));

  // These will crash PictureLayerImpl if this is not true.
  ASSERT_TRUE(host_impl_.pending_tree()->needs_update_draw_properties());
  ASSERT_TRUE(host_impl_.active_tree()->needs_update_draw_properties());
  host_impl_.active_tree()->UpdateDrawProperties();
}

TEST_F(PictureLayerImplTest, HighResTilingDuringAnimationForCpuRasterization) {
  gfx::Size viewport_size(1000, 1000);
  host_impl_.SetViewportSize(viewport_size);

  gfx::Size layer_bounds(100, 100);
  SetupDefaultTrees(layer_bounds);

  float contents_scale = 1.f;
  float device_scale = 1.f;
  float page_scale = 1.f;
  float maximum_animation_scale = 1.f;
  bool animating_transform = false;

  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 1.f);

  // Since we're CPU-rasterizing, starting an animation should cause tiling
  // resolution to get set to the maximum animation scale factor.
  animating_transform = true;
  maximum_animation_scale = 3.f;
  contents_scale = 2.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 3.f);

  // Further changes to scale during the animation should not cause a new
  // high-res tiling to get created.
  contents_scale = 4.f;
  maximum_animation_scale = 5.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 3.f);

  // Once we stop animating, a new high-res tiling should be created.
  animating_transform = false;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 4.f);

  // When animating with an unknown maximum animation scale factor, a new
  // high-res tiling should be created at a source scale of 1.
  animating_transform = true;
  contents_scale = 2.f;
  maximum_animation_scale = 0.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), page_scale * device_scale);

  // Further changes to scale during the animation should not cause a new
  // high-res tiling to get created.
  contents_scale = 3.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), page_scale * device_scale);

  // Once we stop animating, a new high-res tiling should be created.
  animating_transform = false;
  contents_scale = 4.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 4.f);

  // When animating with a maxmium animation scale factor that is so large
  // that the layer grows larger than the viewport at this scale, a new
  // high-res tiling should get created at a source scale of 1, not at its
  // maximum scale.
  animating_transform = true;
  contents_scale = 2.f;
  maximum_animation_scale = 11.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), page_scale * device_scale);

  // Once we stop animating, a new high-res tiling should be created.
  animating_transform = false;
  contents_scale = 11.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 11.f);

  // When animating with a maxmium animation scale factor that is so large
  // that the layer grows larger than the viewport at this scale, and where
  // the intial source scale is < 1, a new high-res tiling should get created
  // at source scale 1.
  animating_transform = true;
  contents_scale = 0.1f;
  maximum_animation_scale = 11.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), device_scale * page_scale);

  // Once we stop animating, a new high-res tiling should be created.
  animating_transform = false;
  contents_scale = 12.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 12.f);

  // When animating toward a smaller scale, but that is still so large that the
  // layer grows larger than the viewport at this scale, a new high-res tiling
  // should get created at source scale 1.
  animating_transform = true;
  contents_scale = 11.f;
  maximum_animation_scale = 11.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), device_scale * page_scale);

  // Once we stop animating, a new high-res tiling should be created.
  animating_transform = false;
  contents_scale = 11.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 11.f);
}

TEST_F(PictureLayerImplTest, HighResTilingDuringAnimationForGpuRasterization) {
  gfx::Size layer_bounds(100, 100);
  gfx::Size viewport_size(1000, 1000);
  SetupDefaultTrees(layer_bounds);
  host_impl_.SetViewportSize(viewport_size);
  host_impl_.SetUseGpuRasterization(true);

  float contents_scale = 1.f;
  float device_scale = 1.3f;
  float page_scale = 1.4f;
  float maximum_animation_scale = 1.f;
  bool animating_transform = false;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 1.f);

  // Since we're GPU-rasterizing, starting an animation should cause tiling
  // resolution to get set to the current contents scale.
  animating_transform = true;
  contents_scale = 2.f;
  maximum_animation_scale = 4.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 2.f);

  // Further changes to scale during the animation should cause a new high-res
  // tiling to get created.
  contents_scale = 3.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 3.f);

  // Since we're re-rasterizing during the animation, scales smaller than 1
  // should be respected.
  contents_scale = 0.25f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 0.25f);

  // Once we stop animating, a new high-res tiling should be created.
  contents_scale = 4.f;
  animating_transform = false;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 4.f);
}

TEST_F(PictureLayerImplTest, TilingSetRasterQueue) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  host_impl_.SetViewportSize(gfx::Size(500, 500));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupPendingTree(pending_pile);
  EXPECT_EQ(2u, pending_layer_->num_tilings());

  scoped_ptr<TilingSetRasterQueue> queue =
      pending_layer_->CreateRasterQueue(false);

  std::set<Tile*> unique_tiles;
  bool reached_prepaint = false;
  size_t non_ideal_tile_count = 0u;
  size_t low_res_tile_count = 0u;
  size_t high_res_tile_count = 0u;
  queue = pending_layer_->CreateRasterQueue(false);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();
    TilePriority priority = tile->priority(PENDING_TREE);

    EXPECT_TRUE(tile);

    // Non-high res tiles only get visible tiles. Also, prepaint should only
    // come at the end of the iteration.
    if (priority.resolution != HIGH_RESOLUTION)
      EXPECT_EQ(TilePriority::NOW, priority.priority_bin);
    else if (reached_prepaint)
      EXPECT_NE(TilePriority::NOW, priority.priority_bin);
    else
      reached_prepaint = priority.priority_bin != TilePriority::NOW;

    non_ideal_tile_count += priority.resolution == NON_IDEAL_RESOLUTION;
    low_res_tile_count += priority.resolution == LOW_RESOLUTION;
    high_res_tile_count += priority.resolution == HIGH_RESOLUTION;

    unique_tiles.insert(tile);
    queue->Pop();
  }

  EXPECT_TRUE(reached_prepaint);
  EXPECT_EQ(0u, non_ideal_tile_count);
  EXPECT_EQ(0u, low_res_tile_count);
  EXPECT_EQ(16u, high_res_tile_count);
  EXPECT_EQ(low_res_tile_count + high_res_tile_count + non_ideal_tile_count,
            unique_tiles.size());

  // No NOW tiles.
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  pending_layer_->draw_properties().visible_content_rect =
      gfx::Rect(1100, 1100, 500, 500);
  bool resourceless_software_draw = false;
  pending_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  unique_tiles.clear();
  high_res_tile_count = 0u;
  queue = pending_layer_->CreateRasterQueue(false);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();
    TilePriority priority = tile->priority(PENDING_TREE);

    EXPECT_TRUE(tile);

    // Non-high res tiles only get visible tiles.
    EXPECT_EQ(HIGH_RESOLUTION, priority.resolution);
    EXPECT_NE(TilePriority::NOW, priority.priority_bin);

    high_res_tile_count += priority.resolution == HIGH_RESOLUTION;

    unique_tiles.insert(tile);
    queue->Pop();
  }

  EXPECT_EQ(16u, high_res_tile_count);
  EXPECT_EQ(high_res_tile_count, unique_tiles.size());

  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  pending_layer_->draw_properties().visible_content_rect =
      gfx::Rect(0, 0, 500, 500);
  pending_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  std::vector<Tile*> high_res_tiles =
      pending_layer_->HighResTiling()->AllTilesForTesting();
  for (std::vector<Tile*>::iterator tile_it = high_res_tiles.begin();
       tile_it != high_res_tiles.end();
       ++tile_it) {
    Tile* tile = *tile_it;
    TileDrawInfo& draw_info = tile->draw_info();
    draw_info.SetSolidColorForTesting(SK_ColorRED);
  }

  non_ideal_tile_count = 0;
  low_res_tile_count = 0;
  high_res_tile_count = 0;
  queue = pending_layer_->CreateRasterQueue(true);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();
    TilePriority priority = tile->priority(PENDING_TREE);

    EXPECT_TRUE(tile);

    non_ideal_tile_count += priority.resolution == NON_IDEAL_RESOLUTION;
    low_res_tile_count += priority.resolution == LOW_RESOLUTION;
    high_res_tile_count += priority.resolution == HIGH_RESOLUTION;
    queue->Pop();
  }

  EXPECT_EQ(0u, non_ideal_tile_count);
  EXPECT_EQ(1u, low_res_tile_count);
  EXPECT_EQ(0u, high_res_tile_count);
}

TEST_F(PictureLayerImplTest, TilingSetEvictionQueue) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);
  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;

  host_impl_.SetViewportSize(gfx::Size(500, 500));

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  // TODO(vmpstr): Add a test with tilings other than high/low res on the active
  // tree.
  SetupPendingTree(pending_pile);
  EXPECT_EQ(2u, pending_layer_->num_tilings());

  std::vector<Tile*> all_tiles;
  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    std::vector<Tile*> tiles = tiling->AllTilesForTesting();
    all_tiles.insert(all_tiles.end(), tiles.begin(), tiles.end());
  }

  std::set<Tile*> all_tiles_set(all_tiles.begin(), all_tiles.end());

  bool mark_required = false;
  size_t number_of_marked_tiles = 0u;
  size_t number_of_unmarked_tiles = 0u;
  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             pending_layer_->contents_scale_x(),
             pending_layer_->visible_content_rect());
         iter;
         ++iter) {
      if (mark_required) {
        number_of_marked_tiles++;
        iter->set_required_for_activation(true);
      } else {
        number_of_unmarked_tiles++;
      }
      mark_required = !mark_required;
    }
  }

  // Sanity checks.
  EXPECT_EQ(17u, all_tiles.size());
  EXPECT_EQ(17u, all_tiles_set.size());
  EXPECT_GT(number_of_marked_tiles, 1u);
  EXPECT_GT(number_of_unmarked_tiles, 1u);

  // Tiles don't have resources yet.
  scoped_ptr<TilingSetEvictionQueue> queue =
      pending_layer_->CreateEvictionQueue(SAME_PRIORITY_FOR_BOTH_TREES);
  EXPECT_TRUE(queue->IsEmpty());

  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(all_tiles);

  std::set<Tile*> unique_tiles;
  float expected_scales[] = {low_res_factor, 1.f};
  size_t scale_index = 0;
  bool reached_visible = false;
  Tile* last_tile = nullptr;
  size_t distance_decreasing = 0;
  size_t distance_increasing = 0;
  queue = pending_layer_->CreateEvictionQueue(SAME_PRIORITY_FOR_BOTH_TREES);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();
    if (!last_tile)
      last_tile = tile;

    EXPECT_TRUE(tile);

    TilePriority priority = tile->priority(PENDING_TREE);

    if (priority.priority_bin == TilePriority::NOW) {
      reached_visible = true;
      last_tile = tile;
      break;
    }

    EXPECT_FALSE(tile->required_for_activation());

    while (std::abs(tile->contents_scale() - expected_scales[scale_index]) >
           std::numeric_limits<float>::epsilon()) {
      ++scale_index;
      ASSERT_LT(scale_index, arraysize(expected_scales));
    }

    EXPECT_FLOAT_EQ(tile->contents_scale(), expected_scales[scale_index]);
    unique_tiles.insert(tile);

    if (tile->required_for_activation() ==
            last_tile->required_for_activation() &&
        priority.priority_bin ==
            last_tile->priority(PENDING_TREE).priority_bin &&
        std::abs(tile->contents_scale() - last_tile->contents_scale()) <
            std::numeric_limits<float>::epsilon()) {
      if (priority.distance_to_visible <=
          last_tile->priority(PENDING_TREE).distance_to_visible)
        ++distance_decreasing;
      else
        ++distance_increasing;
    }

    last_tile = tile;
    queue->Pop();
  }

  // 4 high res tiles are inside the viewport, the rest are evicted.
  EXPECT_TRUE(reached_visible);
  EXPECT_EQ(12u, unique_tiles.size());
  EXPECT_EQ(1u, distance_increasing);
  EXPECT_EQ(11u, distance_decreasing);

  scale_index = 0;
  bool reached_required = false;
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();
    EXPECT_TRUE(tile);

    TilePriority priority = tile->priority(PENDING_TREE);
    EXPECT_EQ(TilePriority::NOW, priority.priority_bin);

    if (reached_required) {
      EXPECT_TRUE(tile->required_for_activation());
    } else if (tile->required_for_activation()) {
      reached_required = true;
      scale_index = 0;
    }

    while (std::abs(tile->contents_scale() - expected_scales[scale_index]) >
           std::numeric_limits<float>::epsilon()) {
      ++scale_index;
      ASSERT_LT(scale_index, arraysize(expected_scales));
    }

    EXPECT_FLOAT_EQ(tile->contents_scale(), expected_scales[scale_index]);
    unique_tiles.insert(tile);
    queue->Pop();
  }

  EXPECT_TRUE(reached_required);
  EXPECT_EQ(all_tiles_set.size(), unique_tiles.size());
}

TEST_F(PictureLayerImplTest, Occlusion) {
  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);
  gfx::Size viewport_size(1000, 1000);

  LayerTestCommon::LayerImplTest impl;
  host_impl_.SetViewportSize(viewport_size);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(layer_bounds, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());
  ActivateTree();

  std::vector<Tile*> tiles =
      active_layer_->HighResTiling()->AllTilesForTesting();
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(tiles);

  {
    SCOPED_TRACE("No occlusion");
    gfx::Rect occluded;
    impl.AppendQuadsWithOcclusion(active_layer_, occluded);

    LayerTestCommon::VerifyQuadsExactlyCoverRect(impl.quad_list(),
                                                 gfx::Rect(layer_bounds));
    EXPECT_EQ(100u, impl.quad_list().size());
  }

  {
    SCOPED_TRACE("Full occlusion");
    gfx::Rect occluded(active_layer_->visible_content_rect());
    impl.AppendQuadsWithOcclusion(active_layer_, occluded);

    LayerTestCommon::VerifyQuadsExactlyCoverRect(impl.quad_list(), gfx::Rect());
    EXPECT_EQ(impl.quad_list().size(), 0u);
  }

  {
    SCOPED_TRACE("Partial occlusion");
    gfx::Rect occluded(150, 0, 200, 1000);
    impl.AppendQuadsWithOcclusion(active_layer_, occluded);

    size_t partially_occluded_count = 0;
    LayerTestCommon::VerifyQuadsAreOccluded(
        impl.quad_list(), occluded, &partially_occluded_count);
    // The layer outputs one quad, which is partially occluded.
    EXPECT_EQ(100u - 10u, impl.quad_list().size());
    EXPECT_EQ(10u + 10u, partially_occluded_count);
  }
}

TEST_F(PictureLayerImplTest, RasterScaleChangeWithoutAnimation) {
  gfx::Size tile_size(host_impl_.settings().default_tile_size);
  SetupDefaultTrees(tile_size);

  ResetTilingsAndRasterScales();

  float contents_scale = 2.f;
  float device_scale = 1.f;
  float page_scale = 1.f;
  float maximum_animation_scale = 1.f;
  bool animating_transform = false;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 2.f);

  // Changing the source scale without being in an animation will cause
  // the layer to reset its source scale to 1.f.
  contents_scale = 3.f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 1.f);

  // Further changes to the source scale will no longer be reflected in the
  // contents scale.
  contents_scale = 0.5f;

  SetContentsScaleOnBothLayers(contents_scale,
                               device_scale,
                               page_scale,
                               maximum_animation_scale,
                               animating_transform);
  EXPECT_BOTH_EQ(HighResTiling()->contents_scale(), 1.f);
}

TEST_F(PictureLayerImplTest, LowResReadyToDrawNotEnoughToActivate) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  // Make sure some tiles are not shared.
  gfx::Rect invalidation(gfx::Point(50, 50), tile_size);
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, invalidation);

  // All pending layer tiles required are not ready.
  EXPECT_FALSE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());

  // Initialize all low-res tiles.
  pending_layer_->SetAllTilesReadyInTiling(pending_layer_->LowResTiling());

  // Low-res tiles should not be enough.
  EXPECT_FALSE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());

  // Initialize remaining tiles.
  pending_layer_->SetAllTilesReady();

  EXPECT_TRUE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());
}

TEST_F(PictureLayerImplTest, HighResReadyToDrawEnoughToActivate) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  // Make sure some tiles are not shared.
  gfx::Rect invalidation(gfx::Point(50, 50), tile_size);
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, invalidation);

  // All pending layer tiles required are not ready.
  EXPECT_FALSE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());

  // Initialize all high-res tiles.
  pending_layer_->SetAllTilesReadyInTiling(pending_layer_->HighResTiling());

  // High-res tiles should be enough, since they cover everything visible.
  EXPECT_TRUE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());
}

TEST_F(PictureLayerImplTest,
       SharedActiveHighResReadyAndPendingLowResReadyNotEnoughToActivate) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  // Make sure some tiles are not shared.
  gfx::Rect invalidation(gfx::Point(50, 50), tile_size);
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, invalidation);

  // Initialize all high-res tiles in the active layer.
  active_layer_->SetAllTilesReadyInTiling(active_layer_->HighResTiling());
  // And all the low-res tiles in the pending layer.
  pending_layer_->SetAllTilesReadyInTiling(pending_layer_->LowResTiling());

  // The unshared high-res tiles are not ready, so we cannot activate.
  EXPECT_FALSE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());

  // When the unshared pending high-res tiles are ready, we can activate.
  pending_layer_->SetAllTilesReadyInTiling(pending_layer_->HighResTiling());
  EXPECT_TRUE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());
}

TEST_F(PictureLayerImplTest, SharedActiveHighResReadyNotEnoughToActivate) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(1000, 1000);

  // Make sure some tiles are not shared.
  gfx::Rect invalidation(gfx::Point(50, 50), tile_size);
  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, invalidation);

  // Initialize all high-res tiles in the active layer.
  active_layer_->SetAllTilesReadyInTiling(active_layer_->HighResTiling());

  // The unshared high-res tiles are not ready, so we cannot activate.
  EXPECT_FALSE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());

  // When the unshared pending high-res tiles are ready, we can activate.
  pending_layer_->SetAllTilesReadyInTiling(pending_layer_->HighResTiling());
  EXPECT_TRUE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());
}

TEST_F(NoLowResPictureLayerImplTest, ManageTilingsCreatesTilings) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  EXPECT_LT(low_res_factor, 1.f);

  ResetTilingsAndRasterScales();

  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    6.f,  // ideal contents scale
                                    3.f,  // device scale
                                    2.f,  // page scale
                                    1.f,  // maximum animation scale
                                    false);
  ASSERT_EQ(1u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());

  // If we change the page scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    6.6f,  // ideal contents scale
                                    3.f,   // device scale
                                    2.2f,  // page scale
                                    1.f,   // maximum animation scale
                                    false);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.6f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());

  // If we change the device scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    7.26f,  // ideal contents scale
                                    3.3f,   // device scale
                                    2.2f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(3u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());

  // If we change the device scale factor, but end up at the same total scale
  // factor somehow, then we don't get new tilings.
  SetupDrawPropertiesAndUpdateTiles(active_layer_,
                                    7.26f,  // ideal contents scale
                                    2.2f,   // device scale
                                    3.3f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(3u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  active_layer_->tilings()->tiling_at(0)->contents_scale());
}

TEST_F(NoLowResPictureLayerImplTest, PendingLayerOnlyHasHighResTiling) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  EXPECT_LT(low_res_factor, 1.f);

  ResetTilingsAndRasterScales();

  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    6.f,  // ideal contents scale
                                    3.f,  // device scale
                                    2.f,  // page scale
                                    1.f,  // maximum animation scale
                                    false);
  ASSERT_EQ(1u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());

  // If we change the page scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    6.6f,  // ideal contents scale
                                    3.f,   // device scale
                                    2.2f,  // page scale
                                    1.f,   // maximum animation scale
                                    false);
  ASSERT_EQ(1u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(6.6f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());

  // If we change the device scale factor, then we should get new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    7.26f,  // ideal contents scale
                                    3.3f,   // device scale
                                    2.2f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(1u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());

  // If we change the device scale factor, but end up at the same total scale
  // factor somehow, then we don't get new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    7.26f,  // ideal contents scale
                                    2.2f,   // device scale
                                    3.3f,   // page scale
                                    1.f,    // maximum animation scale
                                    false);
  ASSERT_EQ(1u, pending_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(7.26f,
                  pending_layer_->tilings()->tiling_at(0)->contents_scale());
}

TEST_F(NoLowResPictureLayerImplTest, AllHighResRequiredEvenIfShared) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  SetupDefaultTreesWithFixedTileSize(layer_bounds, tile_size, Region());

  Tile* some_active_tile =
      active_layer_->HighResTiling()->AllTilesForTesting()[0];
  EXPECT_FALSE(some_active_tile->IsReadyToDraw());

  // All tiles shared (no invalidation), so even though the active tree's
  // tiles aren't ready, there is nothing required.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  if (host_impl_.settings().create_low_res_tiling)
    pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertAllTilesRequired(pending_layer_->HighResTiling());
  if (host_impl_.settings().create_low_res_tiling)
    AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(NoLowResPictureLayerImplTest, NothingRequiredIfActiveMissingTiles) {
  gfx::Size layer_bounds(400, 400);
  gfx::Size tile_size(100, 100);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  // This pile will create tilings, but has no recordings so will not create any
  // tiles.  This is attempting to simulate scrolling past the end of recorded
  // content on the active layer, where the recordings are so far away that
  // no tiles are created.
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateEmptyPileThatThinksItHasRecordings(
          tile_size, layer_bounds);

  SetupTreesWithFixedTileSize(pending_pile, active_pile, tile_size, Region());

  // Active layer has tilings, but no tiles due to missing recordings.
  EXPECT_TRUE(active_layer_->CanHaveTilings());
  EXPECT_EQ(active_layer_->tilings()->num_tilings(),
            host_impl_.settings().create_low_res_tiling ? 2u : 1u);
  EXPECT_EQ(active_layer_->HighResTiling()->AllTilesForTesting().size(), 0u);

  // Since the active layer has no tiles at all, the pending layer doesn't
  // need content in order to activate.
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();
  if (host_impl_.settings().create_low_res_tiling)
    pending_layer_->LowResTiling()->UpdateAllTilePrioritiesForTesting();

  AssertNoTilesRequired(pending_layer_->HighResTiling());
  if (host_impl_.settings().create_low_res_tiling)
    AssertNoTilesRequired(pending_layer_->LowResTiling());
}

TEST_F(NoLowResPictureLayerImplTest, InvalidViewportForPrioritizingTiles) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTreesWithInvalidation(pending_pile, active_pile, Region());

  SetupDrawPropertiesAndUpdateTiles(active_layer_, 1.f, 1.f, 1.f, 1.f, false);

  // UpdateTiles with valid viewport. Should update tile viewport.
  // Note viewport is considered invalid if and only if in resourceless
  // software draw.
  bool resourceless_software_draw = false;
  gfx::Rect viewport = gfx::Rect(layer_bounds);
  gfx::Transform transform;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);
  active_layer_->draw_properties().visible_content_rect = viewport;
  active_layer_->draw_properties().screen_space_transform = transform;
  active_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  gfx::Rect visible_rect_for_tile_priority =
      active_layer_->visible_rect_for_tile_priority();
  EXPECT_FALSE(visible_rect_for_tile_priority.IsEmpty());
  gfx::Transform screen_space_transform_for_tile_priority =
      active_layer_->screen_space_transform();

  // Expand viewport and set it as invalid for prioritizing tiles.
  // Should update viewport and transform, but not update visible rect.
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  resourceless_software_draw = true;
  viewport = gfx::ScaleToEnclosingRect(viewport, 2);
  transform.Translate(1.f, 1.f);
  active_layer_->draw_properties().visible_content_rect = viewport;
  active_layer_->draw_properties().screen_space_transform = transform;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);
  active_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  // Transform for tile priority is updated.
  EXPECT_TRANSFORMATION_MATRIX_EQ(transform,
                                  active_layer_->screen_space_transform());
  // Visible rect for tile priority retains old value.
  EXPECT_EQ(visible_rect_for_tile_priority,
            active_layer_->visible_rect_for_tile_priority());

  // Keep expanded viewport but mark it valid. Should update tile viewport.
  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  resourceless_software_draw = false;
  host_impl_.SetExternalDrawConstraints(transform,
                                        viewport,
                                        viewport,
                                        viewport,
                                        transform,
                                        resourceless_software_draw);
  active_layer_->UpdateTiles(Occlusion(), resourceless_software_draw);

  EXPECT_TRANSFORMATION_MATRIX_EQ(transform,
                                  active_layer_->screen_space_transform());
  EXPECT_EQ(viewport, active_layer_->visible_rect_for_tile_priority());
}

TEST_F(NoLowResPictureLayerImplTest, CleanUpTilings) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  std::vector<PictureLayerTiling*> used_tilings;

  SetupTrees(pending_pile, active_pile);

  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  EXPECT_LT(low_res_factor, 1.f);

  float device_scale = 1.7f;
  float page_scale = 3.2f;
  float scale = 1.f;

  ResetTilingsAndRasterScales();

  SetContentsScaleOnBothLayers(scale, device_scale, page_scale, 1.f, false);
  ASSERT_EQ(1u, active_layer_->tilings()->num_tilings());

  // We only have ideal tilings, so they aren't removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(1u, active_layer_->tilings()->num_tilings());

  host_impl_.PinchGestureBegin();

  // Changing the ideal but not creating new tilings.
  scale *= 1.5f;
  page_scale *= 1.5f;
  SetContentsScaleOnBothLayers(scale, device_scale, page_scale, 1.f, false);
  ASSERT_EQ(1u, active_layer_->tilings()->num_tilings());

  // The tilings are still our target scale, so they aren't removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(1u, active_layer_->tilings()->num_tilings());

  host_impl_.PinchGestureEnd();

  // Create a 1.2 scale tiling. Now we have 1.0 and 1.2 tilings. Ideal = 1.2.
  scale /= 4.f;
  page_scale /= 4.f;
  SetContentsScaleOnBothLayers(1.2f, device_scale, page_scale, 1.f, false);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());
  EXPECT_FLOAT_EQ(1.f,
                  active_layer_->tilings()->tiling_at(1)->contents_scale());

  // Mark the non-ideal tilings as used. They won't be removed.
  used_tilings.clear();
  used_tilings.push_back(active_layer_->tilings()->tiling_at(1));
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());

  // Now move the ideal scale to 0.5. Our target stays 1.2.
  SetContentsScaleOnBothLayers(0.5f, device_scale, page_scale, 1.f, false);

  // The high resolution tiling is between target and ideal, so is not
  // removed.  The low res tiling for the old ideal=1.0 scale is removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());

  // Now move the ideal scale to 1.0. Our target stays 1.2.
  SetContentsScaleOnBothLayers(1.f, device_scale, page_scale, 1.f, false);

  // All the tilings are between are target and the ideal, so they are not
  // removed.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());

  // Now move the ideal scale to 1.1 on the active layer. Our target stays 1.2.
  SetupDrawPropertiesAndUpdateTiles(
      active_layer_, 1.1f, device_scale, page_scale, 1.f, false);

  // Because the pending layer's ideal scale is still 1.0, our tilings fall
  // in the range [1.0,1.2] and are kept.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());

  // Move the ideal scale on the pending layer to 1.1 as well. Our target stays
  // 1.2 still.
  SetupDrawPropertiesAndUpdateTiles(
      pending_layer_, 1.1f, device_scale, page_scale, 1.f, false);

  // Our 1.0 tiling now falls outside the range between our ideal scale and our
  // target raster scale. But it is in our used tilings set, so nothing is
  // deleted.
  used_tilings.clear();
  used_tilings.push_back(active_layer_->tilings()->tiling_at(1));
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(2u, active_layer_->tilings()->num_tilings());

  // If we remove it from our used tilings set, it is outside the range to keep
  // so it is deleted.
  used_tilings.clear();
  active_layer_->CleanUpTilingsOnActiveLayer(used_tilings);
  ASSERT_EQ(1u, active_layer_->tilings()->num_tilings());
}

TEST_F(NoLowResPictureLayerImplTest, ReleaseResources) {
  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1300, 1900);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);
  EXPECT_EQ(1u, pending_layer_->tilings()->num_tilings());
  EXPECT_EQ(1u, active_layer_->tilings()->num_tilings());

  // All tilings should be removed when losing output surface.
  active_layer_->ReleaseResources();
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
  pending_layer_->ReleaseResources();
  EXPECT_EQ(0u, pending_layer_->tilings()->num_tilings());

  // This should create new tilings.
  SetupDrawPropertiesAndUpdateTiles(pending_layer_,
                                    1.3f,  // ideal contents scale
                                    2.7f,  // device scale
                                    3.2f,  // page scale
                                    1.f,   // maximum animation scale
                                    false);
  EXPECT_EQ(1u, pending_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, SharedQuadStateContainsMaxTilingScale) {
  scoped_ptr<RenderPass> render_pass = RenderPass::Create();

  gfx::Size tile_size(400, 400);
  gfx::Size layer_bounds(1000, 2000);

  host_impl_.SetViewportSize(gfx::Size(10000, 20000));

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  ResetTilingsAndRasterScales();
  SetupDrawPropertiesAndUpdateTiles(active_layer_, 2.5f, 1.f, 1.f, 1.f, false);

  float max_contents_scale = active_layer_->MaximumTilingContentsScale();
  EXPECT_EQ(2.5f, max_contents_scale);

  gfx::Transform scaled_draw_transform = active_layer_->draw_transform();
  scaled_draw_transform.Scale(SK_MScalar1 / max_contents_scale,
                              SK_MScalar1 / max_contents_scale);

  AppendQuadsData data;
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);

  // SharedQuadState should have be of size 1, as we are doing AppenQuad once.
  EXPECT_EQ(1u, render_pass->shared_quad_state_list.size());
  // The content_to_target_transform should be scaled by the
  // MaximumTilingContentsScale on the layer.
  EXPECT_EQ(scaled_draw_transform.ToString(),
            render_pass->shared_quad_state_list.front()
                ->content_to_target_transform.ToString());
  // The content_bounds should be scaled by the
  // MaximumTilingContentsScale on the layer.
  EXPECT_EQ(
      gfx::Size(2500u, 5000u).ToString(),
      render_pass->shared_quad_state_list.front()->content_bounds.ToString());
  // The visible_content_rect should be scaled by the
  // MaximumTilingContentsScale on the layer.
  EXPECT_EQ(gfx::Rect(0u, 0u, 2500u, 5000u).ToString(),
            render_pass->shared_quad_state_list.front()
                ->visible_content_rect.ToString());
}

class PictureLayerImplTestWithDelegatingRenderer : public PictureLayerImplTest {
 public:
  PictureLayerImplTestWithDelegatingRenderer() : PictureLayerImplTest() {}

  void InitializeRenderer() override {
    host_impl_.InitializeRenderer(FakeOutputSurface::CreateDelegating3d());
  }
};

TEST_F(PictureLayerImplTestWithDelegatingRenderer,
       DelegatingRendererWithTileOOM) {
  // This test is added for crbug.com/402321, where quad should be produced when
  // raster on demand is not allowed and tile is OOM.
  gfx::Size tile_size = host_impl_.settings().default_tile_size;
  gfx::Size layer_bounds(1000, 1000);

  // Create tiles.
  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTree(pending_pile);
  pending_layer_->SetBounds(layer_bounds);
  ActivateTree();
  host_impl_.active_tree()->UpdateDrawProperties();
  std::vector<Tile*> tiles =
      active_layer_->HighResTiling()->AllTilesForTesting();
  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(tiles);

  // Force tiles after max_tiles to be OOM. TileManager uses
  // GlobalStateThatImpactsTilesPriority from LayerTreeHostImpl, and we cannot
  // directly set state to host_impl_, so we set policy that would change the
  // state. We also need to update tree priority separately.
  GlobalStateThatImpactsTilePriority state;
  size_t max_tiles = 1;
  size_t memory_limit = max_tiles * 4 * tile_size.width() * tile_size.height();
  size_t resource_limit = max_tiles;
  ManagedMemoryPolicy policy(memory_limit,
                             gpu::MemoryAllocation::CUTOFF_ALLOW_EVERYTHING,
                             resource_limit);
  host_impl_.SetMemoryPolicy(policy);
  host_impl_.SetTreePriority(SAME_PRIORITY_FOR_BOTH_TREES);
  host_impl_.PrepareTiles();

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_HARDWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  // Even when OOM, quads should be produced, and should be different material
  // from quads with resource.
  EXPECT_LT(max_tiles, render_pass->quad_list.size());
  EXPECT_EQ(DrawQuad::Material::TILED_CONTENT,
            render_pass->quad_list.front()->material);
  EXPECT_EQ(DrawQuad::Material::SOLID_COLOR,
            render_pass->quad_list.back()->material);
}

class OcclusionTrackingSettings : public LowResTilingsSettings {
 public:
  OcclusionTrackingSettings() { use_occlusion_for_tile_prioritization = true; }
};

class OcclusionTrackingPictureLayerImplTest : public PictureLayerImplTest {
 public:
  OcclusionTrackingPictureLayerImplTest()
      : PictureLayerImplTest(OcclusionTrackingSettings()) {}

  void VerifyEvictionConsidersOcclusion(FakePictureLayerImpl* layer,
                                        FakePictureLayerImpl* twin_layer,
                                        WhichTree tree,
                                        size_t expected_occluded_tile_count) {
    WhichTree twin_tree = tree == ACTIVE_TREE ? PENDING_TREE : ACTIVE_TREE;
    for (int priority_count = 0; priority_count < NUM_TREE_PRIORITIES;
         ++priority_count) {
      TreePriority tree_priority = static_cast<TreePriority>(priority_count);
      size_t occluded_tile_count = 0u;
      Tile* last_tile = nullptr;
      std::set<Tile*> shared_tiles;

      scoped_ptr<TilingSetEvictionQueue> queue =
          layer->CreateEvictionQueue(tree_priority);
      while (!queue->IsEmpty()) {
        Tile* tile = queue->Top();
        if (!last_tile)
          last_tile = tile;
        if (tile->is_shared())
          EXPECT_TRUE(shared_tiles.insert(tile).second);

        // The only way we will encounter an occluded tile after an unoccluded
        // tile is if the priorty bin decreased, the tile is required for
        // activation, or the scale changed.
        bool tile_is_occluded = tile->is_occluded(tree);
        if (tile_is_occluded) {
          occluded_tile_count++;

          bool last_tile_is_occluded = last_tile->is_occluded(tree);
          if (!last_tile_is_occluded) {
            TilePriority::PriorityBin tile_priority_bin =
                tile->priority(tree).priority_bin;
            TilePriority::PriorityBin last_tile_priority_bin =
                last_tile->priority(tree).priority_bin;

            EXPECT_TRUE(
                (tile_priority_bin < last_tile_priority_bin) ||
                tile->required_for_activation() ||
                (tile->contents_scale() != last_tile->contents_scale()));
          }
        }
        last_tile = tile;
        queue->Pop();
      }
      // Count also shared tiles which are occluded in the tree but which were
      // not returned by the tiling set eviction queue. Those shared tiles
      // shall be returned by the twin tiling set eviction queue.
      queue = twin_layer->CreateEvictionQueue(tree_priority);
      while (!queue->IsEmpty()) {
        Tile* tile = queue->Top();
        if (tile->is_shared()) {
          EXPECT_TRUE(shared_tiles.insert(tile).second);
          if (tile->is_occluded(tree))
            ++occluded_tile_count;
          // Check the reasons why the shared tile was not returned by
          // the first tiling set eviction queue.
          switch (tree_priority) {
            case SAME_PRIORITY_FOR_BOTH_TREES: {
              const TilePriority& priority = tile->priority(tree);
              const TilePriority& priority_for_tree_priority =
                  tile->priority_for_tree_priority(tree_priority);
              const TilePriority& twin_priority = tile->priority(twin_tree);
              // Check if the shared tile was not returned by the first tiling
              // set eviction queue because it was out of order for the first
              // tiling set eviction queue but not for the twin tiling set
              // eviction queue.
              if (priority.priority_bin != twin_priority.priority_bin) {
                EXPECT_LT(priority_for_tree_priority.priority_bin,
                          priority.priority_bin);
                EXPECT_EQ(priority_for_tree_priority.priority_bin,
                          twin_priority.priority_bin);
                EXPECT_TRUE(priority_for_tree_priority.priority_bin <
                            priority.priority_bin);
              } else if (tile->is_occluded(tree) !=
                         tile->is_occluded(twin_tree)) {
                EXPECT_TRUE(tile->is_occluded(tree));
                EXPECT_FALSE(tile->is_occluded(twin_tree));
                EXPECT_FALSE(
                    tile->is_occluded_for_tree_priority(tree_priority));
              } else if (priority.distance_to_visible !=
                         twin_priority.distance_to_visible) {
                EXPECT_LT(priority_for_tree_priority.distance_to_visible,
                          priority.distance_to_visible);
                EXPECT_EQ(priority_for_tree_priority.distance_to_visible,
                          twin_priority.distance_to_visible);
                EXPECT_TRUE(priority_for_tree_priority.distance_to_visible <
                            priority.distance_to_visible);
              } else {
                // Shared tiles having the same active and pending priorities
                // should be returned only by a pending tree eviction queue.
                EXPECT_EQ(ACTIVE_TREE, tree);
              }
              break;
            }
            case SMOOTHNESS_TAKES_PRIORITY:
              // Shared tiles should be returned only by an active tree
              // eviction queue.
              EXPECT_EQ(PENDING_TREE, tree);
              break;
            case NEW_CONTENT_TAKES_PRIORITY:
              // Shared tiles should be returned only by a pending tree
              // eviction queue.
              EXPECT_EQ(ACTIVE_TREE, tree);
              break;
            case NUM_TREE_PRIORITIES:
              NOTREACHED();
              break;
          }
        }
        queue->Pop();
      }
      EXPECT_EQ(expected_occluded_tile_count, occluded_tile_count);
    }
  }
};

TEST_F(OcclusionTrackingPictureLayerImplTest,
       OccludedTilesSkippedDuringRasterization) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);
  gfx::Size viewport_size(500, 500);
  gfx::Point occluding_layer_position(310, 0);

  host_impl_.SetViewportSize(viewport_size);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());

  // No occlusion.
  int unoccluded_tile_count = 0;
  scoped_ptr<TilingSetRasterQueue> queue =
      pending_layer_->CreateRasterQueue(false);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();

    // Occluded tiles should not be iterated over.
    EXPECT_FALSE(tile->is_occluded(PENDING_TREE));

    // Some tiles may not be visible (i.e. outside the viewport). The rest are
    // visible and at least partially unoccluded, verified by the above expect.
    bool tile_is_visible =
        tile->content_rect().Intersects(pending_layer_->visible_content_rect());
    if (tile_is_visible)
      unoccluded_tile_count++;
    queue->Pop();
  }
  EXPECT_EQ(unoccluded_tile_count, 25);

  // Partial occlusion.
  pending_layer_->AddChild(LayerImpl::Create(host_impl_.pending_tree(), 1));
  LayerImpl* layer1 = pending_layer_->children()[0];
  layer1->SetBounds(layer_bounds);
  layer1->SetContentBounds(layer_bounds);
  layer1->SetDrawsContent(true);
  layer1->SetContentsOpaque(true);
  layer1->SetPosition(occluding_layer_position);

  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  unoccluded_tile_count = 0;
  queue = pending_layer_->CreateRasterQueue(false);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();

    EXPECT_FALSE(tile->is_occluded(PENDING_TREE));

    bool tile_is_visible =
        tile->content_rect().Intersects(pending_layer_->visible_content_rect());
    if (tile_is_visible)
      unoccluded_tile_count++;
    queue->Pop();
  }
  EXPECT_EQ(20, unoccluded_tile_count);

  // Full occlusion.
  layer1->SetPosition(gfx::Point(0, 0));

  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  unoccluded_tile_count = 0;
  queue = pending_layer_->CreateRasterQueue(false);
  while (!queue->IsEmpty()) {
    Tile* tile = queue->Top();

    EXPECT_FALSE(tile->is_occluded(PENDING_TREE));

    bool tile_is_visible =
        tile->content_rect().Intersects(pending_layer_->visible_content_rect());
    if (tile_is_visible)
      unoccluded_tile_count++;
    queue->Pop();
  }
  EXPECT_EQ(unoccluded_tile_count, 0);
}

TEST_F(OcclusionTrackingPictureLayerImplTest,
       OccludedTilesNotMarkedAsRequired) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);
  gfx::Size viewport_size(500, 500);
  gfx::Point occluding_layer_position(310, 0);

  host_impl_.SetViewportSize(viewport_size);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());

  // No occlusion.
  int occluded_tile_count = 0;
  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);

    occluded_tile_count = 0;
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             pending_layer_->contents_scale_x(),
             gfx::Rect(layer_bounds));
         iter;
         ++iter) {
      if (!*iter)
        continue;
      const Tile* tile = *iter;

      // Fully occluded tiles are not required for activation.
      if (tile->is_occluded(PENDING_TREE)) {
        EXPECT_FALSE(tile->required_for_activation());
        occluded_tile_count++;
      }
    }
    EXPECT_EQ(occluded_tile_count, 0);
  }

  // Partial occlusion.
  pending_layer_->AddChild(LayerImpl::Create(host_impl_.pending_tree(), 1));
  LayerImpl* layer1 = pending_layer_->children()[0];
  layer1->SetBounds(layer_bounds);
  layer1->SetContentBounds(layer_bounds);
  layer1->SetDrawsContent(true);
  layer1->SetContentsOpaque(true);
  layer1->SetPosition(occluding_layer_position);

  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    tiling->UpdateAllTilePrioritiesForTesting();

    occluded_tile_count = 0;
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             pending_layer_->contents_scale_x(),
             gfx::Rect(layer_bounds));
         iter;
         ++iter) {
      if (!*iter)
        continue;
      const Tile* tile = *iter;

      if (tile->is_occluded(PENDING_TREE)) {
        EXPECT_FALSE(tile->required_for_activation());
        occluded_tile_count++;
      }
    }
    switch (i) {
      case 0:
        EXPECT_EQ(occluded_tile_count, 5);
        break;
      case 1:
        EXPECT_EQ(occluded_tile_count, 2);
        break;
      default:
        NOTREACHED();
    }
  }

  // Full occlusion.
  layer1->SetPosition(gfx::PointF(0, 0));

  time_ticks += base::TimeDelta::FromMilliseconds(200);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  host_impl_.pending_tree()->UpdateDrawProperties();

  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    tiling->UpdateAllTilePrioritiesForTesting();

    occluded_tile_count = 0;
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             pending_layer_->contents_scale_x(),
             gfx::Rect(layer_bounds));
         iter;
         ++iter) {
      if (!*iter)
        continue;
      const Tile* tile = *iter;

      if (tile->is_occluded(PENDING_TREE)) {
        EXPECT_FALSE(tile->required_for_activation());
        occluded_tile_count++;
      }
    }
    switch (i) {
      case 0:
        EXPECT_EQ(25, occluded_tile_count);
        break;
      case 1:
        EXPECT_EQ(4, occluded_tile_count);
        break;
      default:
        NOTREACHED();
    }
  }
}

TEST_F(OcclusionTrackingPictureLayerImplTest, OcclusionForDifferentScales) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);
  gfx::Size viewport_size(500, 500);
  gfx::Point occluding_layer_position(310, 0);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  host_impl_.SetViewportSize(viewport_size);

  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, Region());
  ASSERT_TRUE(pending_layer_->CanHaveTilings());

  pending_layer_->AddChild(LayerImpl::Create(host_impl_.pending_tree(), 1));
  LayerImpl* layer1 = pending_layer_->children()[0];
  layer1->SetBounds(layer_bounds);
  layer1->SetContentBounds(layer_bounds);
  layer1->SetDrawsContent(true);
  layer1->SetContentsOpaque(true);
  layer1->SetPosition(occluding_layer_position);

  pending_layer_->tilings()->RemoveAllTilings();
  float low_res_factor = host_impl_.settings().low_res_contents_scale_factor;
  pending_layer_->AddTiling(low_res_factor);
  pending_layer_->AddTiling(0.3f);
  pending_layer_->AddTiling(0.7f);
  pending_layer_->AddTiling(1.0f);
  pending_layer_->AddTiling(2.0f);

  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  // UpdateDrawProperties with the occluding layer.
  host_impl_.pending_tree()->UpdateDrawProperties();

  EXPECT_EQ(5u, pending_layer_->num_tilings());

  int occluded_tile_count = 0;
  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    tiling->UpdateAllTilePrioritiesForTesting();
    std::vector<Tile*> tiles = tiling->AllTilesForTesting();

    occluded_tile_count = 0;
    for (size_t j = 0; j < tiles.size(); ++j) {
      if (tiles[j]->is_occluded(PENDING_TREE)) {
        gfx::Rect scaled_content_rect = ScaleToEnclosingRect(
            tiles[j]->content_rect(), 1.0f / tiles[j]->contents_scale());
        EXPECT_GE(scaled_content_rect.x(), occluding_layer_position.x());
        occluded_tile_count++;
      }
    }

    switch (i) {
      case 0:
        EXPECT_EQ(occluded_tile_count, 30);
        break;
      case 1:
        EXPECT_EQ(occluded_tile_count, 5);
        break;
      case 2:
        EXPECT_EQ(occluded_tile_count, 4);
        break;
      case 4:
      case 3:
        EXPECT_EQ(occluded_tile_count, 2);
        break;
      default:
        NOTREACHED();
    }
  }
}

TEST_F(OcclusionTrackingPictureLayerImplTest, DifferentOcclusionOnTrees) {
  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);
  gfx::Size viewport_size(1000, 1000);
  gfx::Point occluding_layer_position(310, 0);
  gfx::Rect invalidation_rect(230, 230, 102, 102);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  host_impl_.SetViewportSize(viewport_size);
  SetupPendingTree(active_pile);

  // Partially occlude the active layer.
  pending_layer_->AddChild(LayerImpl::Create(host_impl_.pending_tree(), 2));
  LayerImpl* layer1 = pending_layer_->children()[0];
  layer1->SetBounds(layer_bounds);
  layer1->SetContentBounds(layer_bounds);
  layer1->SetDrawsContent(true);
  layer1->SetContentsOpaque(true);
  layer1->SetPosition(occluding_layer_position);

  ActivateTree();

  // Partially invalidate the pending layer.
  SetupPendingTreeWithInvalidation(pending_pile, invalidation_rect);

  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    tiling->UpdateAllTilePrioritiesForTesting();

    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             pending_layer_->contents_scale_x(),
             gfx::Rect(layer_bounds));
         iter;
         ++iter) {
      if (!*iter)
        continue;
      const Tile* tile = *iter;

      // All tiles are unoccluded on the pending tree.
      EXPECT_FALSE(tile->is_occluded(PENDING_TREE));

      Tile* twin_tile = pending_layer_->GetPendingOrActiveTwinTiling(tiling)
                            ->TileAt(iter.i(), iter.j());
      gfx::Rect scaled_content_rect = ScaleToEnclosingRect(
          tile->content_rect(), 1.0f / tile->contents_scale());

      if (scaled_content_rect.Intersects(invalidation_rect)) {
        // Tiles inside the invalidation rect are only on the pending tree.
        EXPECT_NE(tile, twin_tile);

        // Unshared tiles should be unoccluded on the active tree by default.
        EXPECT_FALSE(tile->is_occluded(ACTIVE_TREE));
      } else {
        // Tiles outside the invalidation rect are shared between both trees.
        EXPECT_EQ(tile, twin_tile);
        // Shared tiles are occluded on the active tree iff they lie beneath the
        // occluding layer.
        EXPECT_EQ(tile->is_occluded(ACTIVE_TREE),
                  scaled_content_rect.x() >= occluding_layer_position.x());
      }
    }
  }

  for (size_t i = 0; i < active_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = active_layer_->tilings()->tiling_at(i);

    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             active_layer_->contents_scale_x(),
             gfx::Rect(layer_bounds));
         iter;
         ++iter) {
      if (!*iter)
        continue;
      const Tile* tile = *iter;

      Tile* twin_tile = active_layer_->GetPendingOrActiveTwinTiling(tiling)
                            ->TileAt(iter.i(), iter.j());
      gfx::Rect scaled_content_rect = ScaleToEnclosingRect(
          tile->content_rect(), 1.0f / tile->contents_scale());

      // Since we've already checked the shared tiles, only consider tiles in
      // the invalidation rect.
      if (scaled_content_rect.Intersects(invalidation_rect)) {
        // Tiles inside the invalidation rect are only on the active tree.
        EXPECT_NE(tile, twin_tile);

        // Unshared tiles should be unoccluded on the pending tree by default.
        EXPECT_FALSE(tile->is_occluded(PENDING_TREE));

        // Unshared tiles are occluded on the active tree iff they lie beneath
        // the occluding layer.
        EXPECT_EQ(tile->is_occluded(ACTIVE_TREE),
                  scaled_content_rect.x() >= occluding_layer_position.x());
      }
    }
  }
}

TEST_F(OcclusionTrackingPictureLayerImplTest,
       OccludedTilesConsideredDuringEviction) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);
  gfx::Size viewport_size(1000, 1000);
  gfx::Point pending_occluding_layer_position(310, 0);
  gfx::Point active_occluding_layer_position(0, 310);
  gfx::Rect invalidation_rect(230, 230, 102, 102);

  host_impl_.SetViewportSize(viewport_size);
  host_impl_.SetDeviceScaleFactor(2.f);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupPendingTreeWithFixedTileSize(active_pile, tile_size, Region());

  // Partially occlude the active layer.
  pending_layer_->AddChild(LayerImpl::Create(host_impl_.pending_tree(), 2));
  LayerImpl* active_occluding_layer = pending_layer_->children()[0];
  active_occluding_layer->SetBounds(layer_bounds);
  active_occluding_layer->SetContentBounds(layer_bounds);
  active_occluding_layer->SetDrawsContent(true);
  active_occluding_layer->SetContentsOpaque(true);
  active_occluding_layer->SetPosition(active_occluding_layer_position);

  ActivateTree();

  // Partially invalidate the pending layer. Tiles inside the invalidation rect
  // are not shared between trees.
  SetupPendingTreeWithFixedTileSize(pending_pile, tile_size, invalidation_rect);

  // Partially occlude the pending layer in a different way.
  pending_layer_->AddChild(LayerImpl::Create(host_impl_.pending_tree(), 3));
  LayerImpl* pending_occluding_layer = pending_layer_->children()[0];
  pending_occluding_layer->SetBounds(layer_bounds);
  pending_occluding_layer->SetContentBounds(layer_bounds);
  pending_occluding_layer->SetDrawsContent(true);
  pending_occluding_layer->SetContentsOpaque(true);
  pending_occluding_layer->SetPosition(pending_occluding_layer_position);

  EXPECT_EQ(2u, pending_layer_->num_tilings());
  EXPECT_EQ(2u, active_layer_->num_tilings());

  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));
  // UpdateDrawProperties with the occluding layer.
  host_impl_.pending_tree()->UpdateDrawProperties();

  // The expected number of occluded tiles on each of the 2 tilings for each of
  // the 3 tree priorities.
  size_t expected_occluded_tile_count_on_both[] = {9u, 1u};
  size_t expected_occluded_tile_count_on_active[] = {30u, 3u};
  size_t expected_occluded_tile_count_on_pending[] = {30u, 3u};

  size_t total_expected_occluded_tile_count_on_trees[] = {33u, 33u};

  // Verify number of occluded tiles on the pending layer for each tiling.
  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    tiling->UpdateAllTilePrioritiesForTesting();

    size_t occluded_tile_count_on_pending = 0u;
    size_t occluded_tile_count_on_active = 0u;
    size_t occluded_tile_count_on_both = 0u;
    for (PictureLayerTiling::CoverageIterator iter(tiling, 1.f,
                                                   gfx::Rect(layer_bounds));
         iter; ++iter) {
      Tile* tile = *iter;

      if (!tile)
        continue;
      if (tile->is_occluded(PENDING_TREE))
        occluded_tile_count_on_pending++;
      if (tile->is_occluded(ACTIVE_TREE))
        occluded_tile_count_on_active++;
      if (tile->is_occluded(PENDING_TREE) && tile->is_occluded(ACTIVE_TREE))
        occluded_tile_count_on_both++;
    }
    EXPECT_EQ(expected_occluded_tile_count_on_pending[i],
              occluded_tile_count_on_pending)
        << tiling->contents_scale();
    EXPECT_EQ(expected_occluded_tile_count_on_active[i],
              occluded_tile_count_on_active)
        << tiling->contents_scale();
    EXPECT_EQ(expected_occluded_tile_count_on_both[i],
              occluded_tile_count_on_both)
        << tiling->contents_scale();
  }

  // Verify number of occluded tiles on the active layer for each tiling.
  for (size_t i = 0; i < active_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = active_layer_->tilings()->tiling_at(i);
    tiling->UpdateAllTilePrioritiesForTesting();

    size_t occluded_tile_count_on_pending = 0u;
    size_t occluded_tile_count_on_active = 0u;
    size_t occluded_tile_count_on_both = 0u;
    for (PictureLayerTiling::CoverageIterator iter(
             tiling,
             pending_layer_->contents_scale_x(),
             gfx::Rect(layer_bounds));
         iter;
         ++iter) {
      Tile* tile = *iter;

      if (!tile)
        continue;
      if (tile->is_occluded(PENDING_TREE))
        occluded_tile_count_on_pending++;
      if (tile->is_occluded(ACTIVE_TREE))
        occluded_tile_count_on_active++;
      if (tile->is_occluded(PENDING_TREE) && tile->is_occluded(ACTIVE_TREE))
        occluded_tile_count_on_both++;
    }
    EXPECT_EQ(expected_occluded_tile_count_on_pending[i],
              occluded_tile_count_on_pending)
        << i;
    EXPECT_EQ(expected_occluded_tile_count_on_active[i],
              occluded_tile_count_on_active)
        << i;
    EXPECT_EQ(expected_occluded_tile_count_on_both[i],
              occluded_tile_count_on_both)
        << i;
  }

  std::vector<Tile*> all_tiles;
  for (size_t i = 0; i < pending_layer_->num_tilings(); ++i) {
    PictureLayerTiling* tiling = pending_layer_->tilings()->tiling_at(i);
    std::vector<Tile*> tiles = tiling->AllTilesForTesting();
    all_tiles.insert(all_tiles.end(), tiles.begin(), tiles.end());
  }

  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(all_tiles);

  VerifyEvictionConsidersOcclusion(
      pending_layer_, active_layer_, PENDING_TREE,
      total_expected_occluded_tile_count_on_trees[PENDING_TREE]);
  VerifyEvictionConsidersOcclusion(
      active_layer_, pending_layer_, ACTIVE_TREE,
      total_expected_occluded_tile_count_on_trees[ACTIVE_TREE]);

  // Repeat the tests without valid active tree priorities.
  active_layer_->set_has_valid_tile_priorities(false);
  VerifyEvictionConsidersOcclusion(
      pending_layer_, active_layer_, PENDING_TREE,
      total_expected_occluded_tile_count_on_trees[PENDING_TREE]);
  VerifyEvictionConsidersOcclusion(
      active_layer_, pending_layer_, ACTIVE_TREE, 0u);
  active_layer_->set_has_valid_tile_priorities(true);

  // Repeat the tests without valid pending tree priorities.
  pending_layer_->set_has_valid_tile_priorities(false);
  VerifyEvictionConsidersOcclusion(
      active_layer_, pending_layer_, ACTIVE_TREE,
      total_expected_occluded_tile_count_on_trees[ACTIVE_TREE]);
  VerifyEvictionConsidersOcclusion(
      pending_layer_, active_layer_, PENDING_TREE, 0u);
  pending_layer_->set_has_valid_tile_priorities(true);
}

TEST_F(PictureLayerImplTest, PendingOrActiveTwinLayer) {
  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);

  scoped_refptr<FakePicturePileImpl> pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTree(pile);
  EXPECT_FALSE(pending_layer_->GetPendingOrActiveTwinLayer());

  ActivateTree();
  EXPECT_FALSE(active_layer_->GetPendingOrActiveTwinLayer());

  SetupPendingTree(pile);
  EXPECT_TRUE(pending_layer_->GetPendingOrActiveTwinLayer());
  EXPECT_TRUE(active_layer_->GetPendingOrActiveTwinLayer());
  EXPECT_EQ(pending_layer_, active_layer_->GetPendingOrActiveTwinLayer());
  EXPECT_EQ(active_layer_, pending_layer_->GetPendingOrActiveTwinLayer());

  ActivateTree();
  EXPECT_FALSE(active_layer_->GetPendingOrActiveTwinLayer());

  // Make an empty pending tree.
  host_impl_.CreatePendingTree();
  host_impl_.pending_tree()->DetachLayerTree();
  EXPECT_FALSE(active_layer_->GetPendingOrActiveTwinLayer());
}

TEST_F(PictureLayerImplTest, RecycledTwinLayer) {
  gfx::Size tile_size(102, 102);
  gfx::Size layer_bounds(1000, 1000);

  scoped_refptr<FakePicturePileImpl> pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  SetupPendingTree(pile);
  EXPECT_FALSE(pending_layer_->GetRecycledTwinLayer());

  ActivateTree();
  EXPECT_TRUE(active_layer_->GetRecycledTwinLayer());
  EXPECT_EQ(old_pending_layer_, active_layer_->GetRecycledTwinLayer());

  SetupPendingTree(pile);
  EXPECT_FALSE(pending_layer_->GetRecycledTwinLayer());
  EXPECT_FALSE(active_layer_->GetRecycledTwinLayer());

  ActivateTree();
  EXPECT_TRUE(active_layer_->GetRecycledTwinLayer());
  EXPECT_EQ(old_pending_layer_, active_layer_->GetRecycledTwinLayer());

  // Make an empty pending tree.
  host_impl_.CreatePendingTree();
  host_impl_.pending_tree()->DetachLayerTree();
  EXPECT_FALSE(active_layer_->GetRecycledTwinLayer());
}

void PictureLayerImplTest::TestQuadsForSolidColor(bool test_for_solid) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);
  gfx::Rect layer_rect(layer_bounds);

  FakeContentLayerClient client;
  scoped_refptr<PictureLayer> layer = PictureLayer::Create(&client);
  FakeLayerTreeHostClient host_client(FakeLayerTreeHostClient::DIRECT_3D);
  scoped_ptr<FakeLayerTreeHost> host = FakeLayerTreeHost::Create(&host_client);
  host->SetRootLayer(layer);
  RecordingSource* recording_source = layer->GetRecordingSourceForTesting();

  int frame_number = 0;

  client.set_fill_with_nonsolid_color(!test_for_solid);

  Region invalidation(layer_rect);
  recording_source->UpdateAndExpandInvalidation(
      &client, &invalidation, false, layer_bounds, layer_rect, frame_number++,
      Picture::RECORD_NORMALLY);

  scoped_refptr<RasterSource> pending_raster_source =
      recording_source->CreateRasterSource();

  SetupPendingTreeWithFixedTileSize(pending_raster_source, tile_size, Region());
  ActivateTree();

  if (test_for_solid) {
    EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
  } else {
    ASSERT_TRUE(active_layer_->tilings());
    ASSERT_GT(active_layer_->tilings()->num_tilings(), 0u);
    std::vector<Tile*> tiles =
        active_layer_->tilings()->tiling_at(0)->AllTilesForTesting();
    EXPECT_FALSE(tiles.empty());
    host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(tiles);
  }

  scoped_ptr<RenderPass> render_pass = RenderPass::Create();
  AppendQuadsData data;
  active_layer_->WillDraw(DRAW_MODE_SOFTWARE, nullptr);
  active_layer_->AppendQuads(render_pass.get(), Occlusion(), &data);
  active_layer_->DidDraw(nullptr);

  DrawQuad::Material expected = test_for_solid
                                    ? DrawQuad::Material::SOLID_COLOR
                                    : DrawQuad::Material::TILED_CONTENT;
  EXPECT_EQ(expected, render_pass->quad_list.front()->material);
}

TEST_F(PictureLayerImplTest, DrawSolidQuads) {
  TestQuadsForSolidColor(true);
}

TEST_F(PictureLayerImplTest, DrawNonSolidQuads) {
  TestQuadsForSolidColor(false);
}

TEST_F(PictureLayerImplTest, NonSolidToSolidNoTilings) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(200, 200);
  gfx::Rect layer_rect(layer_bounds);

  FakeContentLayerClient client;
  scoped_refptr<PictureLayer> layer = PictureLayer::Create(&client);
  FakeLayerTreeHostClient host_client(FakeLayerTreeHostClient::DIRECT_3D);
  scoped_ptr<FakeLayerTreeHost> host = FakeLayerTreeHost::Create(&host_client);
  host->SetRootLayer(layer);
  RecordingSource* recording_source = layer->GetRecordingSourceForTesting();

  int frame_number = 0;

  client.set_fill_with_nonsolid_color(true);

  Region invalidation1(layer_rect);
  recording_source->UpdateAndExpandInvalidation(
      &client, &invalidation1, false, layer_bounds, layer_rect, frame_number++,
      Picture::RECORD_NORMALLY);

  scoped_refptr<RasterSource> raster_source1 =
      recording_source->CreateRasterSource();

  SetupPendingTree(raster_source1);
  ActivateTree();
  host_impl_.active_tree()->UpdateDrawProperties();

  // We've started with a solid layer that contains some tilings.
  ASSERT_TRUE(active_layer_->tilings());
  EXPECT_NE(0u, active_layer_->tilings()->num_tilings());

  client.set_fill_with_nonsolid_color(false);

  Region invalidation2(layer_rect);
  recording_source->UpdateAndExpandInvalidation(
      &client, &invalidation2, false, layer_bounds, layer_rect, frame_number++,
      Picture::RECORD_NORMALLY);

  scoped_refptr<RasterSource> raster_source2 =
      recording_source->CreateRasterSource();

  SetupPendingTree(raster_source2);
  ActivateTree();

  // We've switched to a solid color, so we should end up with no tilings.
  ASSERT_TRUE(active_layer_->tilings());
  EXPECT_EQ(0u, active_layer_->tilings()->num_tilings());
}

TEST_F(PictureLayerImplTest, ChangeInViewportAllowsTilingUpdates) {
  base::TimeTicks time_ticks;
  time_ticks += base::TimeDelta::FromMilliseconds(1);
  host_impl_.SetCurrentBeginFrameArgs(
      CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE, time_ticks));

  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 4000);

  scoped_refptr<FakePicturePileImpl> pending_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> active_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);

  SetupTrees(pending_pile, active_pile);

  Region invalidation;
  gfx::Rect viewport = gfx::Rect(0, 0, 100, 100);
  gfx::Transform transform;

  host_impl_.SetRequiresHighResToDraw();

  // Update tiles.
  pending_layer_->draw_properties().visible_content_rect = viewport;
  pending_layer_->draw_properties().screen_space_transform = transform;
  SetupDrawPropertiesAndUpdateTiles(pending_layer_, 1.f, 1.f, 1.f, 1.f, false);
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();

  // Ensure we can't activate.
  EXPECT_FALSE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());

  // Now in the same frame, move the viewport (this can happen during
  // animation).
  viewport = gfx::Rect(0, 2000, 100, 100);

  // Update tiles.
  pending_layer_->draw_properties().visible_content_rect = viewport;
  pending_layer_->draw_properties().screen_space_transform = transform;
  SetupDrawPropertiesAndUpdateTiles(pending_layer_, 1.f, 1.f, 1.f, 1.f, false);
  pending_layer_->HighResTiling()->UpdateAllTilePrioritiesForTesting();

  // Make sure all viewport tiles (viewport from the tiling) are ready to draw.
  std::vector<Tile*> tiles;
  for (PictureLayerTiling::CoverageIterator iter(
           pending_layer_->HighResTiling(),
           1.f,
           pending_layer_->HighResTiling()->GetCurrentVisibleRectForTesting());
       iter;
       ++iter) {
    if (*iter)
      tiles.push_back(*iter);
  }

  host_impl_.tile_manager()->InitializeTilesWithResourcesForTesting(tiles);

  // Ensure we can activate.
  EXPECT_TRUE(pending_layer_->AllTilesRequiredForActivationAreReadyToDraw());
}

TEST_F(PictureLayerImplTest, CloneMissingRecordings) {
  gfx::Size tile_size(100, 100);
  gfx::Size layer_bounds(400, 400);

  scoped_refptr<FakePicturePileImpl> filled_pile =
      FakePicturePileImpl::CreateFilledPile(tile_size, layer_bounds);
  scoped_refptr<FakePicturePileImpl> partial_pile =
      FakePicturePileImpl::CreateEmptyPile(tile_size, layer_bounds);
  for (int i = 1; i < partial_pile->tiling().num_tiles_x(); ++i) {
    for (int j = 1; j < partial_pile->tiling().num_tiles_y(); ++j)
      partial_pile->AddRecordingAt(i, j);
  }

  SetupPendingTreeWithFixedTileSize(filled_pile, tile_size, Region());
  ActivateTree();

  PictureLayerTiling* pending_tiling = old_pending_layer_->HighResTiling();
  PictureLayerTiling* active_tiling = active_layer_->HighResTiling();

  // We should have all tiles in both tile sets.
  EXPECT_EQ(5u * 5u, pending_tiling->AllTilesForTesting().size());
  EXPECT_EQ(5u * 5u, active_tiling->AllTilesForTesting().size());

  // Now put a partially-recorded pile on the pending tree (and invalidate
  // everything, since the main thread PicturePile will invalidate dropped
  // recordings). This will cause us to be missing some tiles.
  SetupPendingTreeWithFixedTileSize(partial_pile, tile_size,
                                    Region(gfx::Rect(layer_bounds)));
  EXPECT_EQ(3u * 3u, pending_tiling->AllTilesForTesting().size());
  EXPECT_FALSE(pending_tiling->TileAt(0, 0));
  EXPECT_FALSE(pending_tiling->TileAt(1, 1));
  EXPECT_TRUE(pending_tiling->TileAt(2, 2));

  // Active is not affected yet.
  EXPECT_EQ(5u * 5u, active_tiling->AllTilesForTesting().size());

  // Activate the tree. The same tiles go missing on the active tree.
  ActivateTree();
  EXPECT_EQ(3u * 3u, active_tiling->AllTilesForTesting().size());
  EXPECT_FALSE(active_tiling->TileAt(0, 0));
  EXPECT_FALSE(active_tiling->TileAt(1, 1));
  EXPECT_TRUE(active_tiling->TileAt(2, 2));

  // Now put a full recording on the pending tree again. We'll get all our tiles
  // back.
  SetupPendingTreeWithFixedTileSize(filled_pile, tile_size,
                                    Region(gfx::Rect(layer_bounds)));
  EXPECT_EQ(5u * 5u, pending_tiling->AllTilesForTesting().size());

  // Active is not affected yet.
  EXPECT_EQ(3u * 3u, active_tiling->AllTilesForTesting().size());

  // Activate the tree. The tiles are created and shared on the active tree.
  ActivateTree();
  EXPECT_EQ(5u * 5u, active_tiling->AllTilesForTesting().size());
  EXPECT_TRUE(active_tiling->TileAt(0, 0)->is_shared());
  EXPECT_TRUE(active_tiling->TileAt(1, 1)->is_shared());
  EXPECT_TRUE(active_tiling->TileAt(2, 2)->is_shared());
}

class TileSizeSettings : public ImplSidePaintingSettings {
 public:
  TileSizeSettings() {
    default_tile_size = gfx::Size(100, 100);
    max_untiled_layer_size = gfx::Size(200, 200);
  }
};

class TileSizeTest : public PictureLayerImplTest {
 public:
  TileSizeTest() : PictureLayerImplTest(TileSizeSettings()) {}
};

TEST_F(TileSizeTest, TileSizes) {
  host_impl_.CreatePendingTree();

  LayerTreeImpl* pending_tree = host_impl_.pending_tree();
  scoped_ptr<FakePictureLayerImpl> layer =
      FakePictureLayerImpl::Create(pending_tree, id_);

  host_impl_.SetViewportSize(gfx::Size(1000, 1000));
  gfx::Size result;

  host_impl_.SetUseGpuRasterization(false);

  // Default tile-size for large layers.
  result = layer->CalculateTileSize(gfx::Size(10000, 10000));
  EXPECT_EQ(result.width(), 100);
  EXPECT_EQ(result.height(), 100);
  // Don't tile and round-up, when under max_untiled_layer_size.
  result = layer->CalculateTileSize(gfx::Size(42, 42));
  EXPECT_EQ(result.width(), 64);
  EXPECT_EQ(result.height(), 64);
  result = layer->CalculateTileSize(gfx::Size(191, 191));
  EXPECT_EQ(result.width(), 192);
  EXPECT_EQ(result.height(), 192);
  result = layer->CalculateTileSize(gfx::Size(199, 199));
  EXPECT_EQ(result.width(), 200);
  EXPECT_EQ(result.height(), 200);

  // Gpu-rasterization uses 25% viewport-height tiles.
  // The +2's below are for border texels.
  host_impl_.SetUseGpuRasterization(true);
  host_impl_.SetViewportSize(gfx::Size(2000, 2000));
  result = layer->CalculateTileSize(gfx::Size(10000, 10000));
  EXPECT_EQ(result.width(), 2000);
  EXPECT_EQ(result.height(), 500 + 2);

  // Clamp and round-up, when smaller than viewport.
  // Tile-height doubles to 50% when width shrinks to <= 50%.
  host_impl_.SetViewportSize(gfx::Size(1000, 1000));
  result = layer->CalculateTileSize(gfx::Size(447, 10000));
  EXPECT_EQ(result.width(), 448);
  EXPECT_EQ(result.height(), 500 + 2);

  // Largest layer is 50% of viewport width (rounded up), and
  // 50% of viewport in height.
  result = layer->CalculateTileSize(gfx::Size(447, 400));
  EXPECT_EQ(result.width(), 448);
  EXPECT_EQ(result.height(), 448);
  result = layer->CalculateTileSize(gfx::Size(500, 499));
  EXPECT_EQ(result.width(), 512);
  EXPECT_EQ(result.height(), 500 + 2);
}

}  // namespace
}  // namespace cc
