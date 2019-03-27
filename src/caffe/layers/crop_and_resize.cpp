#include <algorithm>

#include "caffe/layers/crop_and_resize.hpp"

namespace caffe
{

template <typename Dtype>
void CropAndResizeLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype> *> &bottom,
                                      const vector<Blob<Dtype> *> &top)
{
	CropAndResizeParameter crop_and_resize_param = this->layer_param_.crop_and_resize_param();
    CHECK_GT(crop_and_resize_param.crop_h(), 0)
        << "crop_h must be > 0";
    CHECK_GT(crop_and_resize_param.crop_w(), 0)
        << "crop_w must be > 0";
    crop_height_ = crop_and_resize_param.crop_h();
    crop_width_ = crop_and_resize_param.crop_w();
    extrapolation_value_ = crop_and_resize_param.extrapolation_value();
}

template <typename Dtype>
void CropAndResizeLayer<Dtype>::Reshape(const vector<Blob<Dtype> *> &bottom,
                                   const vector<Blob<Dtype> *> &top)
{
	// bottom[0] should be input image with 4D
	CHECK_EQ(bottom[0]->num_axes(), 4) << "bottom[0] must have 4 axes.";
	// bottom[2] should be box coordinates with shape [num_boxes, 4]
	CHECK_EQ(bottom[2]->num_axes(), 2) << "bottom[2] must have 2 axes.";
	CHECK_EQ(bottom[2]->shape(1), 4) << "Box coordinates axis must have shape 4.";
	// bottom[1] should be box index with shape [num_boxes]
	CHECK_EQ(bottom[1]->num_axes(), 1) << "bottom[1] must have 1 axis.";
	CHECK_EQ(bottom[1]->shape(0), bottom[1]->shape(0))
		<< "box index and box coordinates should have equal count.";
    batch_size_ = bottom[0]->num();
    // TODO: only implement NHWC data format here
	channels_ = bottom[0]->shape(3);
    image_height_ = bottom[0]->shape(1);
    image_width_ = bottom[0]->shape(2);
    CHECK_GT(image_height_, 0)<< "image height must be > 0";
    CHECK_GT(image_width_, 0)<< "image width must be > 0";
    num_boxes_ = bottom[2]->num();
    // TODO: only implement NHWC data format here
    top[0]->Reshape(num_boxes_, crop_height_, crop_width_, channels_);
}

template <typename Dtype>
void CropAndResizeLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype> *> &bottom,
                                       const vector<Blob<Dtype> *> &top)
{
    const Dtype *bottom_data = bottom[0]->cpu_data();
    const Dtype *bottom_index = bottom[1]->cpu_data();
    const Dtype *bottom_rois = bottom[2]->cpu_data();
    Dtype *top_data = top[0]->mutable_cpu_data();
    for (int b = 0; b < num_boxes_; ++b)
    {
      const float y1 = bottom_rois[b*num_boxes_+0];
      const float x1 = bottom_rois[b*num_boxes_+1];
      const float y2 = bottom_rois[b*num_boxes_+2];
      const float x2 = bottom_rois[b*num_boxes_+3];

      // FastBoundsCheck, box_index's values should be in range [0, batch_size)
      const int b_in = bottom_index[b];
      if (b_in >= batch_size_ || b_in < 0)
        continue;

      const float height_scale =
          (crop_height_ > 1) ? (y2 - y1) * (image_height_ - 1) / (crop_height_ - 1) : 0;
      const float width_scale =
          (crop_width_ > 1) ? (x2 - x1) * (image_width_ - 1) / (crop_width_ - 1) : 0;

      for (int y = 0; y < crop_height_; ++y) {
        const float in_y = (crop_height_ > 1)
                               ? y1 * (image_height_ - 1) + y * height_scale
                               : 0.5 * (y1 + y2) * (image_height_ - 1);
        if (in_y < 0 || in_y > image_height_ - 1) {
          for (int x = 0; x < crop_width_; ++x) {
            for (int d = 0; d < channels_; ++d) {
              top_data[((b*crop_height_+y)*crop_width_+x)*channels_+d] = extrapolation_value_;
              //crops(b, y, x, d) = extrapolation_value;
            }
          }
          continue;
        }

        const int top_y_index = floorf(in_y);
        const int bottom_y_index = ceilf(in_y);
        const float y_lerp = in_y - top_y_index;

        for (int x = 0; x < crop_width_; ++x) {
             const float in_x = (crop_width_ > 1)
                                    ? x1 * (image_width_ - 1) + x * width_scale
                                    : 0.5 * (x1 + x2) * (image_width_ - 1);
             if (in_x < 0 || in_x > image_width_ - 1) {
               for (int d = 0; d < channels_; ++d) {
            	 top_data[((b*crop_height_+y)*crop_width_+x)*channels_+d] = extrapolation_value_;
                 //crops(b, y, x, d) = extrapolation_value;
               }
               continue;
             }
             const int left_x_index = floorf(in_x);
             const int right_x_index = ceilf(in_x);
             const float x_lerp = in_x - left_x_index;

             for (int d = 0; d < channels_; ++d) {
               int index = ((b_in*image_height_+top_y_index)*image_width_+left_x_index)*channels_+d;
               //(image(b_in, top_y_index, left_x_index, d));
               const float top_left = static_cast<float>(bottom_data[index]);

               index = ((b_in*image_height_+top_y_index)*image_width_+right_x_index)*channels_+d;
               //(image(b_in, top_y_index, right_x_index, d));
               const float top_right = static_cast<float>(bottom_data[index]);

               index = ((b_in*image_height_+bottom_y_index)*image_width_+left_x_index)*channels_+d;
               //(image(b_in, bottom_y_index, left_x_index, d));
               const float bottom_left = static_cast<float>(bottom_data[index]);

               index = ((b_in*image_height_+bottom_y_index)*image_width_+right_x_index)*channels_+d;
               //(image(b_in, bottom_y_index, right_x_index, d));
               const float bottom_right = static_cast<float>(bottom_data[index]);

               const float top = top_left + (top_right - top_left) * x_lerp;
               const float bottom = bottom_left + (bottom_right - bottom_left) * x_lerp;

               top_data[((b*crop_height_+y)*crop_width_+x)*channels_+d] = top + (bottom - top) * y_lerp;
               //crops(b, y, x, d) = top + (bottom - top) * y_lerp;
             }
           }
       }
    }

}


INSTANTIATE_CLASS(CropAndResizeLayer);
REGISTER_LAYER_CLASS(CropAndResize);

} // namespace caffe
