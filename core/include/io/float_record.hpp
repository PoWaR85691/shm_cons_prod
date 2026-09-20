#pragma once

#include <cstddef>

namespace fast_channel {

constexpr std::size_t kFloatValueSize = sizeof(float);
constexpr char kFloatSeparator = '\n';
constexpr std::size_t kFloatRecordSize = kFloatValueSize + 1;

}
