// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/core/nn/NanoFlann.h"

#include "open3d/utility/Console.h"

namespace open3d {
namespace core {
namespace nn {

NanoFlann::NanoFlann(){};
NanoFlann::NanoFlann(const core::Tensor &tensor) { SetTensorData(tensor); };
NanoFlann::~NanoFlann(){};

bool NanoFlann::SetTensorData(const core::Tensor &data) {
    core::SizeVector shape = data.GetShape();
    core::Dtype dtype = data.GetDtype();
    if (shape.size() != 2) {
        utility::LogWarning(
                "[NanoFlann::SetTensorData] Tensor must be two-dimenional");
        return false;
    }
    if (dtype != core::Dtype::Float64) {
        utility::LogWarning(
                "[NanoFlann::SetTensorData] Tensor with dtype other than "
                "float64 is not supported currently");
        return false;
    }

    dataset_size_ = shape[0];
    dimension_ = shape[1];
    data_.resize(dataset_size_ * dimension_);
    memcpy(data_.data(), data.ToFlatVector<double>().data(),
           dataset_size_ * dimension_ * sizeof(double));
    adaptor_.reset(
            new Adaptor<double>((size_t)dataset_size_, dimension_, data_.data()));

    index_.reset(new KDTree_t((size_t)dimension_, *adaptor_.get()));
    index_->buildIndex();
    return true;
};

std::pair<core::Tensor, core::Tensor> NanoFlann::SearchKnn(
        const core::Tensor &query, int knn) {
    core::SizeVector shape = query.GetShape();
    if (shape.size() != 2) {
        utility::LogError(
                "[NanoFlann::SearchKnn] query tensor must be 2 dimensional "
                "matrix");
    }
    if ((size_t)shape[1] != dimension_) {
        utility::LogError(
                "[NanoFlann::SearchKnn] query tensor has different dimension "
                "with reference tensor");
    }
    std::vector<size_t> result_indices;
    std::vector<double> result_distances;
    int num_query = (int)shape[0];
    int num_results = 0;

    for (int i = 0; i < num_query; i++) {
        core::Tensor query_point = query[i];
        std::vector<double> query_vector = query_point.ToFlatVector<double>();

        std::vector<size_t> _indices(knn);
        std::vector<double> _distances(knn);

        num_results = index_->knnSearch(query_vector.data(), (size_t)knn,
                                        _indices.data(), _distances.data());

        _indices.resize(num_results);
        _distances.resize(num_results);
        result_indices.insert(result_indices.end(), _indices.begin(),
                              _indices.end());
        result_distances.insert(result_distances.end(), _distances.begin(),
                                _distances.end());
    }
    if (num_results < 1) {
        return std::make_pair(core::Tensor(), core::Tensor());
    }

    std::vector<int64_t> result_indices2(result_indices.begin(),
                                         result_indices.end());
    core::Tensor indices(result_indices2, {num_query, num_results},
                         core::Dtype::Int64);
    core::Tensor distances(result_distances, {num_query, num_results},
                           core::Dtype::Float64);
    return std::make_pair(indices, distances);
};

std::tuple<core::Tensor, core::Tensor, core::Tensor> NanoFlann::SearchRadius(
        const core::Tensor &query, double *radii) {
    core::SizeVector shape = query.GetShape();
    if (shape.size() != 2) {
        utility::LogError(
                "[NanoFlann::SearchRadius] query tensor must be 2 dimensional "
                "matrix");
    }
    if ((size_t)shape[1] != dimension_) {
        utility::LogError(
                "[NanoFlann::SearchRadius] query tensor has different "
                "dimension with reference tensor");
    }
    std::vector<size_t> result_indices;
    std::vector<double> result_distances;
    std::vector<size_t> result_nums;
    int num_query = (int)shape[0];

    for (int i = 0; i < num_query; i++) {
        core::Tensor query_point = query[i];
        std::vector<double> query_vector = query_point.ToFlatVector<double>();
        double radius = radii[i];

        nanoflann::SearchParams params;
        std::vector<std::pair<size_t, double>> ret_matches;
        size_t num_matches = index_->radiusSearch(
                query_vector.data(), radius * radius, ret_matches, params);

        ret_matches.resize(num_matches);
        result_nums.push_back(num_matches);
        for (auto ret = ret_matches.begin(); ret < ret_matches.end(); ret++) {
            result_indices.push_back(ret->first);
            result_distances.push_back(ret->second);
        }
    }
    int size = 0;
    for (auto &s : result_nums) {
        size += s;
    }
    std::vector<int64_t> result_nums2(result_nums.begin(), result_nums.end());
    std::vector<int64_t> result_indices2(result_indices.begin(),
                                         result_indices.end());
    core::Tensor indices(result_indices2, {size}, core::Dtype::Int64);
    core::Tensor distances(result_distances, {size}, core::Dtype::Float64);
    core::Tensor nums(result_nums2, {(int)result_nums2.size()},
                      core::Dtype::Int64);
    return std::make_tuple(indices, distances, nums);

    return std::make_tuple(core::Tensor(), core::Tensor(), core::Tensor());
};

std::tuple<core::Tensor, core::Tensor, core::Tensor> NanoFlann::SearchRadius(
        const core::Tensor &query, double radius) {
    core::SizeVector shape = query.GetShape();
    int num_query = shape[0];
    double radii[num_query];
    for (int i = 0; i < num_query; i++) {
        radii[i] = radius;
    }
    return SearchRadius(query, radii);
};

}  // namespace nn
}  // namespace core
}  // namespace open3d