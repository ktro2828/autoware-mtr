// Copyright 2024 TIER IV, Inc.
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

#ifndef COMMON__TRT_SERIALIZE_HPP_
#define COMMON__TRT_SERIALIZE_HPP_

#include <cassert>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <vector>

template <typename T>
inline void serialize_value(void ** buffer, T const & value);

template <typename T>
inline void deserialize_value(void const ** buffer, size_t * buffer_size, T * value);

namespace
{
template <typename T, class Enable = void>
struct Serializer
{
};

template <typename T>
struct Serializer<
  T, typename std::enable_if<
       std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_pod<T>::value>::type>
{
  static size_t serialized_size(T const & value) { return sizeof(T); }
  static void serialize(void ** buffer, T const & value)
  {
    ::memcpy(*buffer, &value, sizeof(T));
    reinterpret_cast<char *&>(*buffer) += sizeof(T);
  }
  static void deserialize(void const ** buffer, size_t * buffer_size, T * value)
  {
    assert(*buffer_size >= sizeof(T));
    ::memcpy(value, *buffer, sizeof(T));
    reinterpret_cast<char const *&>(*buffer) += sizeof(T);
    *buffer_size -= sizeof(T);
  }
};  // struct struct Serializer<T, ...>

template <>
struct Serializer<const char *>
{
  static size_t serialized_size(const char * value) { return strlen(value) + 1; }
  static void serialize(void ** buffer, const char * value)
  {
    constexpr size_t max_len = 10000;
    auto value_len = ::snprintf(static_cast<char *>(*buffer), max_len, "%s", value);
    reinterpret_cast<char *&>(*buffer) += value_len + 1;
  }
  static void deserialize(void const ** buffer, size_t * buffer_size, const char ** value)
  {
    *value = static_cast<char const *>(*buffer);
    size_t data_size = strnlen(*value, *buffer_size) + 1;
    assert(*buffer_size >= data_size);
    reinterpret_cast<char const *&>(*buffer) += data_size;
    *buffer_size -= data_size;
  }
};  // struct Serializer<const char *>

template <typename T>
struct Serializer<
  std::vector<T>,
  typename std::enable_if<
    std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_pod<T>::value>::type>
{
  static size_t serialized_size(std::vector<T> const & value)
  {
    return sizeof(value.size()) + value.size() * sizeof(T);
  }

  static void serialize(void ** buffer, std::vector<T> const & value)
  {
    serialize_value(buffer, value.size());
    size_t nbyte = value.size() * sizeof(T);
    ::memcpy(*buffer, value.data(), nbyte);
    reinterpret_cast<char *&>(*buffer) += nbyte;
  }

  static void deserialize(void const ** buffer, size_t * buffer_size, std::vector<T> * value)
  {
    size_t size;
    deserialize_value(buffer, buffer_size, &size);
    value->resize(size);
    size_t nbyte = value->size() * sizeof(T);
    assert(*buffer_size >= nbyte);
    ::memcpy(value->data(), *buffer, nbyte);
    reinterpret_cast<char const *&>(*buffer) += nbyte;
    *buffer_size -= nbyte;
  }
};  // struct Serializer<std::vector<T>, ...>
}  // namespace

template <typename T>
inline size_t serialized_size(T const & value)
{
  return Serializer<T>::serialized_size(value);
}

template <typename T>
inline void serialize_value(void ** buffer, T const & value)
{
  return Serializer<T>::serialize(buffer, value);
}

template <typename T>
inline void deserialize_value(void const ** buffer, size_t * buffer_size, T * value)
{
  return Serializer<T>::deserialize(buffer, buffer_size, value);
}
#endif  // COMMON__TRT_SERIALIZE_HPP_
