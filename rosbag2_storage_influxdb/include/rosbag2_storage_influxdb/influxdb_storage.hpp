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

#ifndef ROSBAG2_STORAGE_INFLUXDB__INFLUXDB_STORAGE_HPP_
#define ROSBAG2_STORAGE_INFLUXDB__INFLUXDB_STORAGE_HPP_

#include <rosbag2_storage/storage_interfaces/read_write_interface.hpp>
#include <string_view>

namespace rosbag2_storage_plugins {

// TODO(adama): implement ReadWriteInterface
class InfluxDBStorage
    : public rosbag2_storage::storage_interfaces::ReadWriteInterface {
 public:
  InfluxDBStorage();
  ~InfluxDBStorage() override;

  // BaseWriteInterface

  void write(std::shared_ptr<const rosbag2_storage::SerializedBagMessage> msg)
      override;

  void write(const std::vector<
             std::shared_ptr<const rosbag2_storage::SerializedBagMessage>>& msg)
      override;

  void create_topic(const rosbag2_storage::TopicMetadata& topic) override;

  void remove_topic(const rosbag2_storage::TopicMetadata& topic) override;

  // ReadWriteInterface

  void open(
      const rosbag2_storage::StorageOptions& storage_options,
      rosbag2_storage::storage_interfaces::IOFlag io_flag =
          rosbag2_storage::storage_interfaces::IOFlag::READ_WRITE) override;

  uint64_t get_bagfile_size() const override;

  std::string get_storage_identifier() const override;

  virtual uint64_t get_minimum_split_file_size() const override;

  void set_filter(
      const rosbag2_storage::StorageFilter& storage_filter) override;

  void reset_filter() override;

  void seek(const rcutils_time_point_value_t& timestamp) override;

  // BaseReadInterface

  bool has_next() override;

  std::shared_ptr<rosbag2_storage::SerializedBagMessage> read_next() override;

  std::vector<rosbag2_storage::TopicMetadata> get_all_topics_and_types()
      override;

  // BaseInfoInterface

  rosbag2_storage::BagMetadata get_metadata() override;

  std::string get_relative_file_path() const override;

 private:
  void write_common(const std::string_view& line_protocol);

  std::string base_url() const;
  std::string create_bucket_request_body(const std::string& org_id) const;
  bool bucket_exists() const;
  std::string get_org_id() const;
  void create_bucket(const std::string& org_id) const;

  const std::string host_;
  const std::string port_;
  const std::string api_key_;
  const std::string org_name_;
  const std::string bucket_name_;
};

}  // namespace rosbag2_storage_plugins

#endif  // ROSBAG2_STORAGE_INFLUXDB__INFLUXDB_STORAGE_HPP_
