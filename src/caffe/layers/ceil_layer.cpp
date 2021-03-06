#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "caffe/layers/ceil_layer.hpp"

namespace caffe {
template <typename Dtype>
void CeilLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  const int count = bottom[0]->count();
  //CHECK_NE(top[0], bottom[0]) << this->type() << " Layer does not "
  //      "allow in-place computation.";
  top[0]->ReshapeLike(*bottom[0]);
  CHECK_EQ(count, top[0]->count());
}

template <typename Dtype>
void CeilLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
	  const Dtype* bottom_data = bottom[0]->cpu_data();
	  Dtype* top_data = top[0]->mutable_cpu_data();
	  const int count = bottom[0]->count();
	  for (int i = 0; i < count; ++i) {
	    top_data[i] = ceil(bottom_data[i]);
	  }
}

INSTANTIATE_CLASS(CeilLayer);
REGISTER_LAYER_CLASS(Ceil);

}  // namespace caffe
