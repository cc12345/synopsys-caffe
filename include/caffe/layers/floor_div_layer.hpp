#ifndef CAFFE_FLOOR_DIV_LAYER_HPP_
#define CAFFE_FLOOR_DIV_LAYER_HPP_

#include <vector>

#include "caffe/blob.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"

namespace caffe {

/*
 * @brief Compute elementwise floor div (rounding toward the most negative integer).
 * Support broadcasting as numpy and TensorFlow do.
 *
 * Note: Two dimensions are compatible for broadcasting if both are the same or either is 1.
 * The rule starts with the right-most dimension, and works towards the left-most dimension.
 */

template <typename Dtype>
class FloorDivLayer : public Layer<Dtype> {
 public:
  explicit FloorDivLayer(const LayerParameter& param)
      : Layer<Dtype>(param) {}
  virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline const char* type() const { return "FloorDiv"; }
  virtual inline int MinBottomBlobs() const { return 1; }
  virtual inline int ExactNumTopBlobs() const { return 1; }

 protected:

  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual void Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom){
    NOT_IMPLEMENTED;
  };

  int dim_diff_;
  int dim_;
  bool is_scalar_;
};

}  // namespace caffe

#endif  // CAFFE_FLOOR_DIV_LAYER_HPP_
