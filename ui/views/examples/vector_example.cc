// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/vector_example.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icons_public2.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace views {
namespace examples {

namespace {

class VectorIconGallery : public View, public TextfieldController {
 public:
  VectorIconGallery()
      : image_view_(new ImageView()),
        size_input_(new Textfield()),
        color_input_(new Textfield()),
        vector_id_(0),
        size_(32),
        color_(SK_ColorRED) {
    AddChildView(size_input_);
    AddChildView(color_input_);
    AddChildView(image_view_);

    size_input_->set_placeholder_text(base::ASCIIToUTF16("Size in dip"));
    size_input_->set_controller(this);
    color_input_->set_placeholder_text(base::ASCIIToUTF16("Color (AARRGGBB)"));
    color_input_->set_controller(this);

    BoxLayout* box = new BoxLayout(BoxLayout::kVertical, 10, 10, 10);
    SetLayoutManager(box);
    box->SetFlexForView(image_view_, 1);
    UpdateImage();
  }

  ~VectorIconGallery() override {}

  // View implementation.
  bool OnMousePressed(const ui::MouseEvent& event) override {
    if (GetEventHandlerForPoint(event.location()) == image_view_) {
      vector_id_ = ((vector_id_ + 1) %
                    static_cast<int>(gfx::VectorIconId::VECTOR_ICON_NONE));
      UpdateImage();
      return true;
    }
    return false;
  }

  // TextfieldController implementation.
  void ContentsChanged(Textfield* sender,
                       const base::string16& new_contents) override {
    if (sender == size_input_) {
      if (base::StringToSizeT(new_contents, &size_))
        UpdateImage();
      else
        size_input_->SetText(base::string16());

      return;
    }

    DCHECK_EQ(color_input_, sender);
    if (new_contents.size() != 8u)
      return;
    unsigned new_color =
        strtoul(base::UTF16ToASCII(new_contents).c_str(), nullptr, 16);
    if (new_color <= 0xffffffff) {
      color_ = new_color;
      UpdateImage();
    }
  }

  void UpdateImage() {
    image_view_->SetImage(gfx::CreateVectorIcon(
        static_cast<gfx::VectorIconId>(vector_id_), size_, color_));
  }

 private:
  ImageView* image_view_;
  Textfield* size_input_;
  Textfield* color_input_;

  int vector_id_;
  size_t size_;
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(VectorIconGallery);
};

}  // namespace

VectorExample::VectorExample() : ExampleBase("Vector Icon") {}

VectorExample::~VectorExample() {}

void VectorExample::CreateExampleView(View* container) {
  container->SetLayoutManager(new FillLayout());
  container->AddChildView(new VectorIconGallery());
}

}  // namespace examples
}  // namespace views
