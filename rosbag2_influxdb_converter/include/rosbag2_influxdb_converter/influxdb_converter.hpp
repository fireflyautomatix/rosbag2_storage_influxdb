// Copyright 2025 FireFly Automatix, Inc.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ROSBAG2_STORAGE_INFLUXDB__INFLUXDB_CONVERTER_HPP_
#define ROSBAG2_STORAGE_INFLUXDB__INFLUXDB_CONVERTER_HPP_

#include <rosbag2_cpp/converter_interfaces/serialization_format_converter.hpp>

namespace rosbag2_influxdb_converter {

class InfluxDBConverter
    : public rosbag2_cpp::converter_interfaces::SerializationFormatConverter {
 public:
  void serialize(
      std::shared_ptr<const rosbag2_cpp::rosbag2_introspection_message_t>
          ros_message,
      const rosidl_message_type_support_t* type_support,
      std::shared_ptr<rosbag2_storage::SerializedBagMessage> serialized_message)
      override;

  void deserialize(std::shared_ptr<const rosbag2_storage::SerializedBagMessage>
                       serialized_message,
                   const rosidl_message_type_support_t* type_support,
                   std::shared_ptr<rosbag2_cpp::rosbag2_introspection_message_t>
                       ros_message) override;
};

}  // namespace rosbag2_influxdb_converter

#endif  // ROSBAG2_STORAGE_INFLUXDB__INFLUXDB_CONVERTER_HPP_
