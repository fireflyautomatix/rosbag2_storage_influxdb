// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
// Copyright 2023 FireFly Automatix, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cmath>
#include <cstring>
#include <rosbag2_influxdb_converter/influxdb_converter.hpp>
#include <rosbag2_storage/ros_helper.hpp>
#include <rosidl_typesupport_introspection_cpp/field_types.hpp>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>
#include <sstream>
#include <vector>

namespace rosbag2_influxdb_converter {

// This is derived from an older version of rmw_fastrtps_cpp's align_ function
static inline void *align_(size_t __align, void *&__ptr) noexcept {
  const auto __intptr = reinterpret_cast<uintptr_t>(__ptr);
  const auto __aligned = (__intptr - 1u + __align) & ~(__align - 1);
  return __ptr = reinterpret_cast<void *>(__aligned);
}

static size_t calculate_max_align(
    const rosidl_typesupport_introspection_cpp::MessageMembers *members) {
  size_t max_align = 0;

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    size_t alignment = 0;
    const auto &member = members->members_[i];

    if (member.is_array_ && (!member.array_size_ || member.is_upper_bound_)) {
      alignment = alignof(std::vector<unsigned char>);
    } else {
      switch (member.type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
          alignment = alignof(bool);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
          alignment = alignof(uint8_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          alignment = alignof(char);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
          alignment = alignof(float);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
          alignment = alignof(double);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          alignment = alignof(int16_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          alignment = alignof(uint16_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          alignment = alignof(int32_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          alignment = alignof(uint32_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          alignment = alignof(int64_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          alignment = alignof(uint64_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          alignment = alignof(std::string);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE: {
          auto sub_members =
              (const rosidl_typesupport_introspection_cpp::MessageMembers *)
                  member.members_->data;
          alignment = calculate_max_align(sub_members);
        } break;
      }
    }

    if (alignment > max_align) {
      max_align = alignment;
    }
  }

  return max_align;
}

template <typename T>
void serialize_unsigned(void *value, std::stringstream &ser) {
  ser << std::to_string(*static_cast<T *>(value)) << "u";
}

template <typename T>
void serialize_signed(void *value, std::stringstream &ser) {
  ser << std::to_string(*static_cast<T *>(value)) << "i";
}

void serialize_string(void *value, std::stringstream &ser) {
  ser << "\"" << *static_cast<std::string *>(value) << "\"";
}

template <typename T>
void serialize_float(void *value, std::stringstream &ser) {
  T value_copy = *static_cast<T *>(value);
  if (!std::isfinite(value_copy)) {
    ser << "0";
  } else {
    ser << value_copy;
  }
}

void serialize_bool(void *value, std::stringstream &ser) {
  // don't cast to bool here because if the bool is
  // uninitialized the random value can't be deserialized
  ser << std::boolalpha << (*static_cast<uint8_t *>(value) ? true : false);
}

void serialize_value(
    const rosidl_typesupport_introspection_cpp::MessageMember *member,
    void *field, std::stringstream &ser) {
  switch (member->type_id_) {
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
      serialize_bool(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
      serialize_unsigned<uint8_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
      serialize_signed<char>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
      serialize_float<float>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
      serialize_float<double>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
      serialize_signed<int16_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
      serialize_unsigned<uint16_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
      serialize_signed<int32_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
      serialize_unsigned<uint32_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
      serialize_signed<int64_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
      serialize_unsigned<uint64_t>(field, ser);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
      serialize_string(field, ser);
      break;
  }
}

template <typename T>
void serialize_collection(
    const rosidl_typesupport_introspection_cpp::MessageMember *member,
    void *field, std::stringstream &ser, const std::string &field_name) {
  if (member->array_size_ && !member->is_upper_bound_) {
    auto array = static_cast<T *>(field);
    for (size_t index = 0; index < member->array_size_; ++index) {
      ser << field_name << "." << index << "=";
      serialize_value(member, array + index, ser);
      ser << ",";
    }
  } else {
    std::vector<T> &vector = *reinterpret_cast<std::vector<T> *>(field);
    for (size_t index = 0; index < vector.size(); ++index) {
      ser << field_name << "." << index << "=";
      serialize_value(member, &vector.data()[index], ser);
      ser << ",";
    }
  }
}

void serialize_collection_dispatch(
    const rosidl_typesupport_introspection_cpp::MessageMember *member,
    void *field, std::stringstream &ser, const std::string &field_name) {
  switch (member->type_id_) {
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
      // need to handle bools because vectors of bools are weird
      // TODO(adama): I think this only gives us the first element
      ser << field_name << "=";
      serialize_value(member, field, ser);
      ser << ",";
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
      serialize_collection<uint8_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
      serialize_collection<char>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
      serialize_collection<float>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
      serialize_collection<double>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
      serialize_collection<int16_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
      serialize_collection<uint16_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
      serialize_collection<int32_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
      serialize_collection<uint32_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
      serialize_collection<int64_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
      serialize_collection<uint64_t>(member, field, ser, field_name);
      break;
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
      serialize_collection<std::string>(member, field, ser, field_name);
      break;
  }
}

void serialize_field(
    const rosidl_typesupport_introspection_cpp::MessageMember *member,
    void *field, std::stringstream &ser, const std::string &field_name) {
  if (!member->is_array_) {
    ser << field_name << "=";
    serialize_value(member, field, ser);
    ser << ",";
  } else {
    serialize_collection_dispatch(member, field, ser, field_name);
  }
}

inline size_t get_array_size_and_assign_field(
    const rosidl_typesupport_introspection_cpp::MessageMember *member,
    void *field, void *&subros_message, size_t sub_members_size,
    size_t max_align) {
  auto vector = reinterpret_cast<std::vector<unsigned char> *>(field);
  void *ptr = reinterpret_cast<void *>(sub_members_size);
  size_t vsize =
      vector->size() / reinterpret_cast<size_t>(align_(max_align, ptr));
  if (member->is_upper_bound_ && vsize > member->array_size_) {
    throw std::runtime_error("vector overcomes the maximum length");
  }
  subros_message = reinterpret_cast<void *>(vector->data());
  return vsize;
}

void serializeROSmessage(
    std::stringstream &ser,
    const rosidl_typesupport_introspection_cpp::MessageMembers *members,
    const void *ros_message, const std::string &base_field_name = "") {
  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto member = members->members_ + i;
    std::string field_name;
    if (base_field_name == "") {
      field_name = std::string(member->name_);
    } else {
      field_name = base_field_name + "." + std::string(member->name_);
    }
    void *field = const_cast<char *>(static_cast<const char *>(ros_message)) +
                  member->offset_;
    if (member->type_id_ !=
        ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE) {
      serialize_field(member, field, ser, field_name);
    } else {
      auto sub_members = static_cast<
          const rosidl_typesupport_introspection_cpp::MessageMembers *>(
          member->members_->data);
      if (!member->is_array_) {
        serializeROSmessage(ser, sub_members, field, field_name);
      } else {
        void *subros_message = nullptr;
        size_t array_size = 0;
        size_t sub_members_size = sub_members->size_of_;
        size_t max_align = calculate_max_align(sub_members);

        if (member->array_size_ && !member->is_upper_bound_) {
          subros_message = field;
          array_size = member->array_size_;
        } else {
          array_size = get_array_size_and_assign_field(
              member, field, subros_message, sub_members_size, max_align);
        }

        for (size_t index = 0; index < array_size; ++index) {
          std::string new_base_field_name =
              field_name + "." + std::to_string(index);
          serializeROSmessage(ser, sub_members, subros_message,
                              new_base_field_name);
          subros_message =
              static_cast<char *>(subros_message) + sub_members_size;
          subros_message = align_(max_align, subros_message);
        }
      }
    }
  }
}

std::string normalize_message_namespace(const char *message_namespace) {
  std::string output(message_namespace);
  std::string delimiter("::");
  auto index = output.find(delimiter);
  output.replace(index, delimiter.size(), "/");
  return output;
}

void InfluxDBConverter::serialize(
    std::shared_ptr<const rosbag2_cpp::rosbag2_introspection_message_t>
        ros_message,
    const rosidl_message_type_support_t *type_support,
    std::shared_ptr<rosbag2_storage::SerializedBagMessage> serialized_message) {
  std::stringstream ss;

  ss << ros_message->topic_name;
  auto members = reinterpret_cast<
      const rosidl_typesupport_introspection_cpp::MessageMembers *>(
      type_support->data);

  ss << ",type=" << normalize_message_namespace(members->message_namespace_)
     << "/" << members->message_name_;

  ss << ",platform=" << std::getenv("PLATFORM_TOKEN")
     << ",machine=" << std::getenv("MACHINE_TOKEN");

  // End tags
  ss << " ";

  std::string base_field_name{""};
  // TODO(adama): do this with less copies
  serializeROSmessage(ss, members, ros_message->message);

  // rewind the stringstream to before the last comma
  ss.seekp(-1, std::ios_base::end);
  auto milliseconds = ros_message->time_stamp / 1000000;
  ss << " " << milliseconds << "\n";

  auto output = ss.str();

  serialized_message->serialized_data =
      rosbag2_storage::make_serialized_message(output.data(), output.size());
}

void InfluxDBConverter::deserialize(
    std::shared_ptr<
        const rosbag2_storage::SerializedBagMessage> /*serialized_message*/,
    const rosidl_message_type_support_t * /*type_support*/,
    std::shared_ptr<
        rosbag2_cpp::rosbag2_introspection_message_t> /*ros_message*/) {}

}  // namespace rosbag2_influxdb_converter

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
    rosbag2_influxdb_converter::InfluxDBConverter,
    rosbag2_cpp::converter_interfaces::SerializationFormatConverter)
