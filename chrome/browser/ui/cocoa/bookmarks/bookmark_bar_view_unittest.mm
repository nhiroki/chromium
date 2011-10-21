// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_nsobject.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"
#include "chrome/browser/ui/cocoa/cocoa_profile_test.h"
#import "chrome/browser/ui/cocoa/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/url_drop_target.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/mozilla/NSPasteboard+Utils.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/ocmock_extensions.h"

namespace {
// Some values used for mocks and fakes.
const CGFloat kFakeIndicatorPos = 7.0;
const NSPoint kPoint = {10, 10};
};

// Fake DraggingInfo, fake BookmarkBarController, fake NSPasteboard...
@interface FakeBookmarkDraggingInfo : NSObject {
 @public
  BOOL dragButtonToPong_;
  BOOL dragButtonToShouldCopy_;
  BOOL dragURLsPong_;
  BOOL dragBookmarkDataPong_;
  BOOL dropIndicatorShown_;
  BOOL draggingEnteredCalled_;
  // Only mock one type of drag data at a time.
  NSString* dragDataType_;
  BookmarkButton* button_;  // weak
  BookmarkModel* bookmarkModel_;  // weak
  id draggingSource_;
}
@property (nonatomic) BOOL dropIndicatorShown;
@property (nonatomic) BOOL draggingEnteredCalled;
@property (nonatomic, copy) NSString* dragDataType;
@property (nonatomic, assign) BookmarkButton* button;
@property (nonatomic, assign) BookmarkModel* bookmarkModel;
@end

@implementation FakeBookmarkDraggingInfo

@synthesize dropIndicatorShown = dropIndicatorShown_;
@synthesize draggingEnteredCalled = draggingEnteredCalled_;
@synthesize dragDataType = dragDataType_;
@synthesize button = button_;
@synthesize bookmarkModel = bookmarkModel_;

- (id)init {
  if ((self = [super init])) {
    dropIndicatorShown_ = YES;
  }
  return self;
}

- (void)dealloc {
  [dragDataType_ release];
  [super dealloc];
}

- (void)reset {
  [dragDataType_ release];
  dragDataType_ = nil;
  dragButtonToPong_ = NO;
  dragURLsPong_ = NO;
  dragBookmarkDataPong_ = NO;
  dropIndicatorShown_ = YES;
  draggingEnteredCalled_ = NO;
  draggingSource_ = self;
}

- (void)setDraggingSource:(id)draggingSource {
  draggingSource_ = draggingSource;
}

// NSDragInfo mocking functions.

- (id)draggingPasteboard {
  return self;
}

// So we can look local.
- (id)draggingSource {
  return draggingSource_;
}

- (NSDragOperation)draggingSourceOperationMask {
  return NSDragOperationCopy | NSDragOperationMove;
}

- (NSPoint)draggingLocation {
  return kPoint;
}

// NSPasteboard mocking functions.

- (BOOL)containsURLData {
  NSArray* urlTypes = [URLDropTargetHandler handledDragTypes];
  if (dragDataType_)
    return [urlTypes containsObject:dragDataType_];
  return NO;
}

- (NSData*)dataForType:(NSString*)type {
  if (dragDataType_ && [dragDataType_ isEqualToString:type]) {
    if (button_)
      return [NSData dataWithBytes:&button_ length:sizeof(button_)];
    else
      return [NSData data];  // Return something, anything.
  }
  return nil;
}

// Fake a controller for callback ponging

- (BOOL)dragButton:(BookmarkButton*)button to:(NSPoint)point copy:(BOOL)copy {
  dragButtonToPong_ = YES;
  dragButtonToShouldCopy_ = copy;
  return YES;
}

- (BOOL)addURLs:(NSArray*)urls withTitles:(NSArray*)titles at:(NSPoint)point {
  dragURLsPong_ = YES;
  return YES;
}

- (void)getURLs:(NSArray**)outUrls
    andTitles:(NSArray**)outTitles
    convertingFilenames:(BOOL)convertFilenames {
}

- (BOOL)dragBookmarkData:(id<NSDraggingInfo>)info {
  dragBookmarkDataPong_ = YES;
  return NO;
}

- (BOOL)canEditBookmarks {
  return YES;
}

// Confirm the pongs.

- (BOOL)dragButtonToPong {
  return dragButtonToPong_;
}

- (BOOL)dragButtonToShouldCopy {
  return dragButtonToShouldCopy_;
}

- (BOOL)dragURLsPong {
  return dragURLsPong_;
}

- (BOOL)dragBookmarkDataPong {
  return dragBookmarkDataPong_;
}

- (CGFloat)indicatorPosForDragToPoint:(NSPoint)point {
  return kFakeIndicatorPos;
}

- (BOOL)shouldShowIndicatorShownForPoint:(NSPoint)point {
  return dropIndicatorShown_;
}

- (BOOL)draggingAllowed:(id<NSDraggingInfo>)info {
  return YES;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  draggingEnteredCalled_ = YES;
  return NSDragOperationNone;
}

- (void)setDropInsertionPos:(CGFloat)where {
}

- (void)clearDropInsertionPos {
}

@end

namespace {

class BookmarkBarViewTest : public CocoaProfileTest {
 public:
  virtual void SetUp() {
    CocoaProfileTest::SetUp();
    view_.reset([[BookmarkBarView alloc] init]);
    window_ = CreateBrowserWindow()->GetNativeHandle();
  }

  id GetMockController() {
    id mock_controller =
        [OCMockObject mockForClass:[BookmarkBarController class]];
    [[[mock_controller stub] andReturnBool:YES]
     draggingAllowed:OCMOCK_ANY];
    [[[mock_controller stub] andReturnBool:YES]
     shouldShowIndicatorShownForPoint:kPoint];
    [[[mock_controller stub] andReturnFloat:kFakeIndicatorPos]
     indicatorPosForDragToPoint:kPoint];
    [[[mock_controller stub] andReturn:window_] browserWindow];
    return mock_controller;
  }

  scoped_nsobject<BookmarkBarView> view_;
  NSWindow* window_;  // WEAK, owned by CocoaProfileTest
};

TEST_F(BookmarkBarViewTest, CanDragWindow) {
  EXPECT_FALSE([view_ mouseDownCanMoveWindow]);
}

TEST_F(BookmarkBarViewTest, BookmarkButtonDragAndDrop) {
  scoped_nsobject<FakeBookmarkDraggingInfo>
      info([[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  scoped_nsobject<BookmarkButton> dragged_button([[BookmarkButton alloc] init]);
  [dragged_button setDelegate:GetMockController()];
  [info setDraggingSource:dragged_button.get()];
  [info setDragDataType:kBookmarkButtonDragType];
  [info setButton:dragged_button.get()];
  [info setBookmarkModel:profile()->GetBookmarkModel()];
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_TRUE([view_ performDragOperation:(id)info.get()]);
  EXPECT_TRUE([info dragButtonToPong]);
  EXPECT_FALSE([info dragButtonToShouldCopy]);
  EXPECT_FALSE([info dragURLsPong]);
  EXPECT_TRUE([info dragBookmarkDataPong]);
}

// When dragging bookmarks across profiles, we should always copy, never move.
TEST_F(BookmarkBarViewTest, BookmarkButtonDragAndDropAcrossProfiles) {
  scoped_nsobject<FakeBookmarkDraggingInfo>
  info([[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  // |other_profile| is owned by the |testing_profile_manager|.
  TestingProfile* other_profile =
      testing_profile_manager()->CreateTestingProfile("other");
  other_profile->CreateBookmarkModel(true);
  other_profile->BlockUntilBookmarkModelLoaded();

  scoped_nsobject<BookmarkButton> dragged_button([[BookmarkButton alloc] init]);
  [dragged_button setDelegate:GetMockController()];
  [info setDraggingSource:dragged_button.get()];
  [info setDragDataType:kBookmarkButtonDragType];
  [info setButton:dragged_button.get()];
  [info setBookmarkModel:other_profile->GetBookmarkModel()];
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_TRUE([view_ performDragOperation:(id)info.get()]);
  EXPECT_TRUE([info dragButtonToPong]);
  EXPECT_TRUE([info dragButtonToShouldCopy]);
  EXPECT_FALSE([info dragURLsPong]);
  EXPECT_TRUE([info dragBookmarkDataPong]);
}

TEST_F(BookmarkBarViewTest, URLDragAndDrop) {
  scoped_nsobject<FakeBookmarkDraggingInfo>
      info([[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  NSArray* dragTypes = [URLDropTargetHandler handledDragTypes];
  for (NSString* type in dragTypes) {
    [info setDragDataType:type];
    EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationCopy);
    EXPECT_TRUE([view_ performDragOperation:(id)info.get()]);
    EXPECT_FALSE([info dragButtonToPong]);
    EXPECT_TRUE([info dragURLsPong]);
    EXPECT_TRUE([info dragBookmarkDataPong]);
    [info reset];
  }
}

TEST_F(BookmarkBarViewTest, BookmarkButtonDropIndicator) {
  scoped_nsobject<FakeBookmarkDraggingInfo>
      info([[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  scoped_nsobject<BookmarkButton> dragged_button([[BookmarkButton alloc] init]);
  [info setDraggingSource:dragged_button.get()];
  [info setDragDataType:kBookmarkButtonDragType];
  EXPECT_FALSE([info draggingEnteredCalled]);
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_TRUE([info draggingEnteredCalled]);  // Ensure controller pinged.
  EXPECT_TRUE([view_ dropIndicatorShown]);
  EXPECT_EQ([view_ dropIndicatorPosition], kFakeIndicatorPos);

  [info setDropIndicatorShown:NO];
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_FALSE([view_ dropIndicatorShown]);
}

}  // namespace
