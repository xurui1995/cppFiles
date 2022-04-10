/*******************************************************************************
* Copyright 2020-2022 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/// @example cpu_get_started.cpp
/// @copybrief cpu_get_started_cpp
/// > Annotated version: @ref cpu_get_started_cpp

/// @page cpu_get_started_cpp Getting started on CPU
/// This is an example to demonstrate how to build a simple graph and run it on
/// CPU.
///
/// > Example code: @ref cpu_get_started.cpp
///
/// Some key take-aways included in this example:
///
/// * how to build a graph and get partitions from it
/// * how to create an engine, allocator and stream
/// * how to compile a partition
/// * how to execute a compiled partition
///
/// Some assumptions in this example:
///
/// * Only workflow is demonstrated without checking correctness
/// * Unsupported partitions should be handled by users themselves
///

/// @page cpu_get_started_cpp
/// @section cpu_get_started_cpp_headers Public headers
///
/// To start using oneDNN Graph, we must include the @ref dnnl_graph.hpp header
/// file in the application. All the C++ APIs reside in namespace `dnnl::graph`.
/// @page cpu_get_started_cpp
/// @snippet cpu_get_started.cpp Headers and namespace
//[Headers and namespace]
#include "oneapi/dnnl/dnnl_graph.hpp"
//[Headers and namespace]

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "common/example_utils.hpp"
#include "common/helpers_any_layout.hpp"

using namespace dnnl::graph;
using data_type = logical_tensor::data_type;
using layout_type = logical_tensor::layout_type;

/// @page cpu_get_started_cpp
/// @section cpu_get_started_cpp_tutorial cpu_get_started_tutorial() function
///
void cpu_get_started_tutorial() {
    // clang-format off
    std::vector<int64_t> conv0_input_dims {8, 3, 227, 227};
    std::vector<int64_t> conv0_weight_dims {96, 3, 11, 11};
    std::vector<int64_t> conv0_bias_dims {96};
    std::vector<int64_t> conv1_weight_dims {96, 96, 1, 1};
    std::vector<int64_t> conv1_bias_dims {96};
    std::vector<int64_t> dst_dims {-1, -1, -1, -1};

    /// @page cpu_get_started_cpp
    /// @subsection cpu_get_started_cpp_get_partition Build graph and get partitions
    ///
    /// In this section, we are trying to build a graph containing the pattern
    /// like `conv0->relu0->conv1->relu1`. After that, we can get all of
    /// partitions which are determined by backend.
    ///
    /// To create a graph, #dnnl::graph::engine::kind is needed because the
    /// returned partitions maybe vary on different devices.
    /// @snippet cpu_get_started.cpp Create graph
    //[Create graph]
    graph g(engine::kind::cpu);
    //[Create graph]

    /// To build a graph, the connection relationship of different ops must be
    /// known. In oneDNN Graph, #dnnl::graph::logical_tensor is used to express
    /// such relationship. So, next step is to create logical tensors for these
    /// ops including inputs and outputs.
    ///
    /// @note It's not necessary to provide concrete shape/layout information
    /// at graph partitioning stage. Users can provide these information till
    /// compilation stage.
    ///
    /// Create input/output #dnnl::graph::logical_tensor for first `Convolution` op.
    /// @snippet cpu_get_started.cpp Create conv's logical tensor
    //[Create conv's logical tensor]
    logical_tensor conv0_src_desc {0, data_type::f32, conv0_input_dims, layout_type::undef};
    logical_tensor conv0_weight_desc {1, data_type::f32, conv0_weight_dims, layout_type::undef};
    logical_tensor conv0_dst_desc {2, data_type::f32, dst_dims, layout_type::undef};
    //[Create conv's logical tensor]

    /// Create first `Convolution` op (#dnnl::graph::op) and attaches attributes
    /// to it, such as `strides`, `pads_begin`, `pads_end`, `data_format`, etc.
    /// @snippet cpu_get_started.cpp Create first conv
    //[Create first conv]
    op conv0(0, op::kind::Convolution, {conv0_src_desc, conv0_weight_desc}, {conv0_dst_desc}, "conv0");
    conv0.set_attr<std::vector<int64_t>>("strides", {4, 4});
    conv0.set_attr<std::vector<int64_t>>("pads_begin", {0, 0});
    conv0.set_attr<std::vector<int64_t>>("pads_end", {0, 0});
    conv0.set_attr<std::vector<int64_t>>("dilations", {1, 1});
    conv0.set_attr<int64_t>("groups", 1);
    conv0.set_attr<std::string>("data_format", "NCX");
    conv0.set_attr<std::string>("filter_format", "OIX");
    //[Create first conv]

    /// Create input/output logical tensors for first `BiasAdd` op.
    /// @snippet cpu_get_started.cpp Create biasadd's logical tensor
    //[Create biasadd's logical tensor]
    logical_tensor conv0_bias_desc {3, data_type::f32, conv0_bias_dims, layout_type::undef};
    logical_tensor conv0_bias_add_dst_desc {4, data_type::f32, dst_dims, layout_type::undef};
    //[Create biasadd's logical tensor]

    /// Create first `BiasAdd` op.
    /// @snippet cpu_get_started.cpp Create first bias_add
    //[Create first bias_add]
    op conv0_bias_add(1, op::kind::BiasAdd, {conv0_dst_desc, conv0_bias_desc}, {conv0_bias_add_dst_desc}, "conv0_bias_add");
    //[Create first bias_add]

    /// Create output logical tensors for first `Relu` op.
    /// @snippet cpu_get_started.cpp Create relu's logical tensor
    //[Create relu's logical tensor]
    logical_tensor relu0_dst_desc {5, data_type::f32, dst_dims, layout_type::undef};
    //[Create relu's logical tensor]

    /// Create first `Relu` op.
    /// @snippet cpu_get_started.cpp Create first relu
    //[Create first relu]
    op relu0(2, op::kind::ReLU, {conv0_bias_add_dst_desc}, {relu0_dst_desc}, "relu0");
    //[Create first relu]

    /// Create input/output logical tensors for second `Convolution` op.
    /// @snippet cpu_get_started.cpp Create conv's second logical tensor
    //[Create conv's second logical tensor]
    logical_tensor conv1_weight_desc {6, data_type::f32, conv1_weight_dims, layout_type::undef};
    logical_tensor conv1_dst_desc {7, data_type::f32, dst_dims, layout_type::undef};
    //[Create conv's second logical tensor]

    /// Create second `Convolution` op and also attaches required attributes to it.
    /// @snippet cpu_get_started.cpp Create second conv
    //[Create second conv]
    op conv1(3, op::kind::Convolution, {relu0_dst_desc, conv1_weight_desc}, {conv1_dst_desc}, "conv1");
    conv1.set_attr<std::vector<int64_t>>("strides", {1, 1});
    conv1.set_attr<std::vector<int64_t>>("pads_begin", {0, 0});
    conv1.set_attr<std::vector<int64_t>>("pads_end", {0, 0});
    conv1.set_attr<std::vector<int64_t>>("dilations", {1, 1});
    conv1.set_attr<int64_t>("groups", 1);
    conv1.set_attr<std::string>("data_format", "NCX");
    conv1.set_attr<std::string>("filter_format", "OIX");
    //[Create second conv]

    /// Create input/output logical tensors for second `BiasAdd` op.
    /// @snippet cpu_get_started.cpp Create biasadd's second logical tensor
    //[Create biasadd's second logical tensor]
    logical_tensor conv1_bias_desc {8, data_type::f32, conv1_bias_dims, layout_type::undef};
    logical_tensor conv1_bias_add_dst_desc {9, data_type::f32, dst_dims, layout_type::undef};
    //[Create biasadd's second logical tensor]

    /// Create second `BiasAdd` op.
    /// @snippet cpu_get_started.cpp Create second bias_add
    //[Create second bias_add]
    op conv1_bias_add(4, op::kind::BiasAdd, {conv1_dst_desc, conv1_bias_desc}, {conv1_bias_add_dst_desc}, "conv1_bias_add");
    //[Create second bias_add]

    /// Create output logical tensors for second `Relu` op.
    /// @snippet cpu_get_started.cpp Create relu's second logical tensor
    //[Create relu's second logical tensor]
    logical_tensor relu1_dst_desc {10, data_type::f32, dst_dims, layout_type::undef};
    //[Create relu's second logical tensor]

    /// Create second `Relu` op.
    /// @snippet cpu_get_started.cpp Create second relu
    //[Create second relu]
    op relu1(5, op::kind::ReLU, {conv1_bias_add_dst_desc}, {relu1_dst_desc}, "relu1");
    //[Create second relu]

    /// Finally, those created ops will be added into the graph. The graph
    /// inside will maintain a list to store all these ops.
    ///
    /// @note The order of adding op doesn't matter.
    ///
    /// @snippet cpu_get_started.cpp Add op
    //[Add op]
    g.add_op(conv0);
    g.add_op(conv0_bias_add);
    g.add_op(relu0);

    g.add_op(conv1);
    g.add_op(conv1_bias_add);
    g.add_op(relu1);
    //[Add op]

    /// After finished above operations, we can get partitions by calling
    /// #dnnl::graph::graph::get_partitions(). Here we can also specify the
    /// #dnnl::graph::partition::policy to get different partitions.
    ///
    /// In this example, the graph will be filtered into two partitions:
    /// `conv0+relu0` and `conv1+relu1`.
    ///
    /// @note
    ///     Setting `DNNL_GRAPH_DUMP=1` can save internal graphs into dot files
    ///     before/after graph fusion.
    ///
    /// @snippet cpu_get_started.cpp Get partition
    //[Get partition]
    auto partitions = g.get_partitions();
    //[Get partition]

    /// Contains the ids of logical tensors which will be set with any layout
    std::unordered_set<size_t> ids_with_any_layout;
    /// This is a helper function which helps decide which logical tensor is
    /// needed to be set with `dnnl::graph::logical_tensor::layout_type::any`
    /// layout. Typically, users need implement the similar logic in their code
    /// for best performance.
    set_any_layout(partitions, ids_with_any_layout);

    /// Create a #dnnl::graph::engine. Also, set a user-defined
    /// #dnnl::graph::allocator to this engine.
    ///
    /// @snippet cpu_get_started.cpp Create engine
    //[Create engine]
    engine eng {engine::kind::cpu, 0};
    //[Create engine]

    /// Create a #dnnl::graph::stream on a given engine
    ///
    /// @snippet cpu_get_started.cpp Create engine
    //[Create stream]
    stream strm {eng};
    //[Create stream]

    // mapping from logical tensor id to output tensors
    // used to the connection relationship between partitions (e.g partition 0's
    // output tensor is fed into partition 1)
    std::unordered_map<size_t, tensor> global_outputs_ts_map;
    // manage the lifetime of memory buffers binded to those input/output tensors
    std::vector<std::shared_ptr<void>> data_buffers;

    // mapping from id to queried logical tensor from compiled partition
    // used to record the logical tensors that are previously enabled with ANY layout
    std::unordered_map<size_t, logical_tensor> id_to_queried_logical_tensors;

    for (const auto &partition : partitions) {
        if (partition.is_supported()) {
            std::vector<logical_tensor> inputs = partition.get_in_ports();
            std::vector<logical_tensor> outputs = partition.get_out_ports();

            // update input logical tensors with concrete layout
            for (size_t idx = 0; idx < inputs.size(); ++idx) {
                size_t id = inputs[idx].get_id();
                std::cout << "1-" << "id = " << id << std::endl;
                // the tensor is an output of another partition
                if (id_to_queried_logical_tensors.find(id) != id_to_queried_logical_tensors.end()) {         
                    inputs[idx] = id_to_queried_logical_tensors[id];
                    std::cout << "2-" << "find " << std::endl;
                }
                else {
                    auto ori_lt = inputs[idx];
                    // create logical tensor with strided layout
                    std::cout << "3-" << "赋值 " << std::endl;
                    inputs[idx] = logical_tensor {ori_lt.get_id(), ori_lt.get_data_type(),
                        ori_lt.get_dims(), layout_type::strided};
                }
            }

            // update output logical tensors with concrete layout
            for (size_t idx = 0; idx < outputs.size(); ++idx) {
                size_t id = outputs[idx].get_id();
                std::cout << "4-" << "id = " << id << std::endl;
                layout_type ltype = layout_type::strided;

                
                if (ids_with_any_layout.count(id)) {

                    std::cout << "5-" <<  ids_with_any_layout.count(id)  << std::endl;
                    ltype = layout_type::any;
                }
                auto ori_lt = outputs[idx];
                // create logical tensor with strided/any layout
                 std::cout << "6-" <<  "赋值"  << std::endl;
                outputs[idx] = logical_tensor {ori_lt.get_id(), ori_lt.get_data_type(),
                    ori_lt.get_dims(), ltype};
            }

            /// Compile the partition to generate compiled partition with the
            /// input and output logical tensors.
            /// @snippet cpu_get_started.cpp Compile partition
            //[Compile partition]
            compiled_partition cp = partition.compile(inputs, outputs, eng);
            //[Compile partition]

            // update output logical tensors with queried one
            for (size_t idx = 0; idx < outputs.size(); ++idx) {
                size_t id = outputs[idx].get_id();
                std::cout << "7-" <<  id  << std::endl;
                outputs[idx] = cp.query_logical_tensor(id);
                std::cout << "8-" <<  idx  << std::endl;
               
                id_to_queried_logical_tensors[id] = outputs[idx];
            }

            // Binding data buffers with input and output logical tensors
            std::vector<tensor> inputs_ts, outputs_ts;
            inputs_ts.reserve(inputs.size());
            outputs_ts.reserve(outputs.size());
            for (const auto &in : inputs) {
                size_t id = in.get_id();
                size_t mem_size = in.get_mem_size();
                // check if the input is an output of another partition
                auto pos = global_outputs_ts_map.find(id);
                if (pos != global_outputs_ts_map.end()) {
                    inputs_ts.push_back(pos->second);
                    continue;
                }
                // memory allocation
                data_buffers.push_back({});
                data_buffers.back().reset(malloc(mem_size), cpu_deletor {});
                inputs_ts.push_back(tensor {in, eng, data_buffers.back().get()});
            }

            for (const auto &out : outputs) {
                size_t mem_size = out.get_mem_size();
                // memory allocation
                data_buffers.push_back({});
                data_buffers.back().reset(malloc(mem_size), cpu_deletor {});
                outputs_ts.push_back(tensor {out, eng, data_buffers.back().get()});
                global_outputs_ts_map[out.get_id()] = outputs_ts.back();
            }

            /// Execute the compiled partition on the specified stream.
            /// @snippet cpu_get_started.cpp Execute compiled partition
            //[Execute compiled partition]
            cp.execute(strm, inputs_ts, outputs_ts);
            //[Execute compiled partition]
        } else {
            std::cout << "cpu_get_started: got unsupported partition, users need "
                "handle the operators by themselves." << std::endl;
        }
    }
    // wait for all compiled partition's execution finished
    strm.wait();

    /// @page cpu_get_started_cpp Getting started on CPU
    // clang-format on
}

int main(int argc, char **argv) {
    cpu_get_started_tutorial();
    return 0;
}
