// Copyright 2023 FireFly Automatix, Inc.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <iterator>
#include <rosbag2_storage_influxdb/influxdb_storage.hpp>
#include <sstream>
#include <utility>

namespace rosbag2_storage_plugins {

std::string get_env_or_throw(const char *env_var) {
  const char *value = std::getenv(env_var);
  if (value == nullptr) {
    throw std::runtime_error(std::string(env_var) + " not set");
  }
  return std::string{value};
}

std::string get_env_or_default(const char *env_var,
                               const std::string &default_value) {
  const char *value = std::getenv(env_var);
  if (value == nullptr) {
    return default_value;
  }
  return std::string{value};
}

InfluxDBStorage::InfluxDBStorage()
: host_(get_env_or_default("INFLUXDB_HOST", "localhost")),
  port_(get_env_or_default("INFLUXDB_PORT", "8086")),
  api_key_(get_env_or_throw("INFLUXDB_TOKEN")),
  org_name_(get_env_or_throw("INFLUXDB_ORG")),
  bucket_name_(get_env_or_default("INFLUXDB_BUCKET", "rosbag2")) {}

InfluxDBStorage::~InfluxDBStorage() {}

inline std::string_view get_data(
    const rosbag2_storage::SerializedBagMessage &msg) {
  return std::string_view{reinterpret_cast<char *>(msg.serialized_data->buffer),
                          msg.serialized_data->buffer_length};
}

void InfluxDBStorage::write_common(const std::string_view &line_protocol) {
  cpr::Response response =
      cpr::Post(cpr::Url{base_url() + "/api/v2/write"}, cpr::Bearer{api_key_},
                cpr::Header{{{"Content-Type", "text/plain"}}},
                cpr::Parameters{
                    {"org", org_name_},
                    {"bucket", bucket_name_},
                    {"precision", "ms"},
                },
                cpr::Body{line_protocol});

  if (response.error) {
    throw std::runtime_error(response.error.message);
  } else if (!cpr::status::is_success(response.status_code)) {
    throw std::runtime_error(response.text);
  }
}

std::string InfluxDBStorage::base_url() const {
  // TODO(adama): support https
  return std::string{"http://"} + host_ + ":" + port_;
}

void InfluxDBStorage::write(
    std::shared_ptr<const rosbag2_storage::SerializedBagMessage> msg) {
  write_common(get_data(*msg));
}

void InfluxDBStorage::write(
    const std::vector<
        std::shared_ptr<const rosbag2_storage::SerializedBagMessage>>
        &messages) {
  std::ostringstream os;
  std::for_each(
      messages.begin(), messages.end(),
      [&os](const std::shared_ptr<const rosbag2_storage::SerializedBagMessage>
                &msg) { os << get_data(*msg); });
  write_common(os.str());
}

void InfluxDBStorage::create_topic(
    const rosbag2_storage::TopicMetadata & /*topic*/) {}

void InfluxDBStorage::remove_topic(
    const rosbag2_storage::TopicMetadata & /*topic*/) {}

std::string InfluxDBStorage::create_bucket_request_body(
    const std::string &org_id) const {
  rapidjson::Document body;
  body.SetObject();
  body.AddMember("description", "Topics from ROS 2", body.GetAllocator());
  body.AddMember("name", bucket_name_, body.GetAllocator());
  body.AddMember("orgID", org_id, body.GetAllocator());
  body.AddMember("schemaType", "implicit", body.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  body.Accept(writer);
  return buffer.GetString();
}

void InfluxDBStorage::create_bucket(const std::string &org_id) const {
  cpr::Response response =
      cpr::Post(cpr::Url{base_url() + "/api/v2/buckets"}, cpr::Bearer{api_key_},
                cpr::Header{{{"Content-Type", "application/json"}}},
                cpr::Body{create_bucket_request_body(org_id)});

  if (response.error) {
    throw std::runtime_error(response.error.message);
  } else if (!cpr::status::is_success(response.status_code)) {
    throw std::runtime_error(response.text);
  }
}

std::string InfluxDBStorage::get_org_id() const {
  cpr::Response response =
      cpr::Get(cpr::Url{base_url() + "/api/v2/orgs"}, cpr::Bearer{api_key_},
               cpr::Header{{{"Content-Type", "application/json"}}},
               cpr::Parameters{{{"org", org_name_}}});

  if (response.error) {
    throw std::runtime_error(response.error.message);
  } else if (!cpr::status::is_success(response.status_code)) {
    throw std::runtime_error(response.text);
  } else {
    rapidjson::Document document;
    document.Parse(response.text);
    auto orgs = document.FindMember("orgs");
    if (orgs == document.MemberEnd()) {
      throw std::runtime_error(response.text);
    } else {
      for (const auto &org : orgs->value.GetArray()) {
        auto id = org.FindMember("id");
        if (id == org.MemberEnd()) {
          throw std::runtime_error(response.text);
        } else {
          return id->value.GetString();
        }
      }
      throw std::runtime_error(response.text);
    }
  }
}

bool InfluxDBStorage::bucket_exists() const {
  cpr::Response response =
      cpr::Get(cpr::Url{base_url() + "/api/v2/buckets"}, cpr::Bearer{api_key_},
               cpr::Header{{{"Content-Type", "application/json"}}},
               cpr::Parameters{{{"name", bucket_name_}}});

  if (response.error) {
    throw std::runtime_error(response.error.message);
  } else if (!cpr::status::is_success(response.status_code)) {
    throw std::runtime_error(response.text);
  } else {
    rapidjson::Document document;
    document.Parse(response.text);
    auto buckets = document.FindMember("buckets");
    if (buckets == document.MemberEnd()) {
      throw std::runtime_error(response.text);
    } else {
      return buckets->value.GetArray().Size() > 0;
    }
  }
}

void InfluxDBStorage::open(
    const rosbag2_storage::StorageOptions & /*storage_options*/,
    rosbag2_storage::storage_interfaces::IOFlag /*io_flag*/) {
  if (!bucket_exists()) {
    auto org_id = get_org_id();
    create_bucket(org_id);
  }
}

uint64_t InfluxDBStorage::get_bagfile_size() const { return 0u; }

std::string InfluxDBStorage::get_storage_identifier() const {
  return "influxdb";
}

uint64_t InfluxDBStorage::get_minimum_split_file_size() const { return 0u; }

void InfluxDBStorage::set_filter(
    const rosbag2_storage::StorageFilter & /*storage_filter*/) {}

void InfluxDBStorage::reset_filter() {}

void InfluxDBStorage::seek(const rcutils_time_point_value_t & /*timestamp*/) {}

bool InfluxDBStorage::has_next() { return true; }

std::shared_ptr<rosbag2_storage::SerializedBagMessage>
InfluxDBStorage::read_next() {
  return nullptr;
}

std::vector<rosbag2_storage::TopicMetadata>
InfluxDBStorage::get_all_topics_and_types() {
  return {};
}

rosbag2_storage::BagMetadata InfluxDBStorage::get_metadata() { return {}; }

std::string InfluxDBStorage::get_relative_file_path() const { return {}; }

}  // namespace rosbag2_storage_plugins

#include "pluginlib/class_list_macros.hpp"  // NOLINT
PLUGINLIB_EXPORT_CLASS(rosbag2_storage_plugins::InfluxDBStorage,
                       rosbag2_storage::storage_interfaces::ReadWriteInterface)
