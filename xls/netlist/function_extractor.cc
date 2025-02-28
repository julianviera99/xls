// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/netlist/function_extractor.h"

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/types/variant.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/ret_check.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist.pb.h"

namespace xls {
namespace netlist {
namespace function {
namespace {

constexpr const char kDirectionKey[] = "direction";
constexpr const char kFunctionKey[] = "function";
constexpr const char kNextStateKey[] = "next_state";
constexpr const char kInputValue[] = "input";
constexpr const char kOutputValue[] = "output";
constexpr const char kPinKind[] = "pin";
constexpr const char kFfKind[] = "ff";

// Gets input and output pin names and output pin functions and adds them to the
// entry proto.
absl::Status ExtractFromPin(const cell_lib::Block& pin,
                            CellLibraryEntryProto* entry_proto) {
  // I've yet to see an example where this isn't the case.
  std::string name = pin.args[0];

  absl::optional<bool> is_output;
  std::string function;
  for (const cell_lib::BlockEntry& entry : pin.entries) {
    const auto* kv_entry = absl::get_if<cell_lib::KVEntry>(&entry);
    if (kv_entry) {
      if (kv_entry->key == kDirectionKey) {
        if (kv_entry->value == kOutputValue) {
          is_output = true;
        } else if (kv_entry->value == kInputValue) {
          is_output = false;
        } else {
          // "internal" is at least one add'l direction.
          // We don't care about such pins.
          return absl::OkStatus();
        }
      } else if (kv_entry->key == kFunctionKey) {
        function = kv_entry->value;
      }
    }
  }

  if (is_output == absl::nullopt) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Pin %s has no direction entry!", name));
  }

  if (is_output.value()) {
    OutputPinProto* output_pin = entry_proto->add_output_pins();
    output_pin->set_name(name);
    if (!function.empty()) {
      // Some output pins lack associated functions.
      // Ignore them for now (during baseline dev). If they turn out to be
      // important, I'll circle back.
      output_pin->set_function(function);
    }
  } else {
    entry_proto->add_input_names(name);
  }

  return absl::OkStatus();
}

// If we see a ff (flop-flop) block, it contains a "next_state" field, with the
// function calculating the output value of the cell after the next clock.
// Since all our current (logic-checking) purposes are clockless, this is
// equivalent to being the "function" of an output pin. All known FF cells have
// a single output pin, so we sanity check for that.
// If so, then we replace that function with the one from the next_state field.
absl::Status ExtractFromFf(const cell_lib::Block& ff,
                           CellLibraryEntryProto* entry_proto) {
  entry_proto->set_kind(netlist::CellKindProto::FLOP);
  for (const cell_lib::BlockEntry& entry : ff.entries) {
    const auto* kv_entry = absl::get_if<cell_lib::KVEntry>(&entry);
    if (kv_entry && kv_entry->key == kNextStateKey) {
      std::string next_state_function = kv_entry->value;
      XLS_RET_CHECK(entry_proto->output_pins_size() == 1);
      entry_proto->mutable_output_pins(0)->set_function(next_state_function);
    }
  }

  return absl::OkStatus();
}

absl::Status ExtractFromCell(const cell_lib::Block& cell,
                             CellLibraryEntryProto* entry_proto) {
  // I've yet to see an example where this isn't the case.
  entry_proto->set_name(cell.args[0]);

  // Default kind; overridden only if necessary.
  entry_proto->set_kind(netlist::CellKindProto::OTHER);

  for (const cell_lib::BlockEntry& entry : cell.entries) {
    if (absl::holds_alternative<std::unique_ptr<cell_lib::Block>>(entry)) {
      auto& block_entry = absl::get<std::unique_ptr<cell_lib::Block>>(entry);
      if (block_entry->kind == kPinKind) {
        XLS_RETURN_IF_ERROR(ExtractFromPin(*block_entry.get(), entry_proto));
      } else if (block_entry->kind == kFfKind) {
        // If it's a flip-flop, we need to replace the pin's output function
        // with it's next_state function.
        XLS_RETURN_IF_ERROR(ExtractFromFf(*block_entry.get(), entry_proto));
      }
    }
  }

  return absl::OkStatus();
}

}  // namespace

xabsl::StatusOr<CellLibraryProto> ExtractFunctions(
    cell_lib::CharStream* stream) {
  cell_lib::Scanner scanner(stream);
  absl::flat_hash_set<std::string> kind_allowlist(
      {"library", "cell", "pin", "direction", "function", "ff", "next_state"});
  cell_lib::Parser parser(&scanner);

  XLS_ASSIGN_OR_RETURN(std::unique_ptr<cell_lib::Block> block,
                       parser.ParseLibrary());
  CellLibraryProto proto;
  for (const cell_lib::BlockEntry& entry : block->entries) {
    if (absl::holds_alternative<std::unique_ptr<cell_lib::Block>>(entry)) {
      auto& block_entry = absl::get<std::unique_ptr<cell_lib::Block>>(entry);
      if (block_entry->kind == "cell") {
        CellLibraryEntryProto* entry_proto = proto.add_entries();
        XLS_RETURN_IF_ERROR(ExtractFromCell(*block_entry.get(), entry_proto));
      }
    }
  }

  return proto;
}

}  // namespace function
}  // namespace netlist
}  // namespace xls
