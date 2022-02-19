/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLYosysJSONParser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stack>

#include <nlohmann/json.hpp>

#include "NajaLog.h"

#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLException.h"

using json = nlohmann::json;

namespace {

using namespace naja::SNL;

SNLTerm::Direction getSNLDirection(const std::string& direction) {
  if (direction == "input") {
    return SNLTerm::Direction::Input;
  } else if (direction == "output") {
    return SNLTerm::Direction::Output;
  } else {
    return SNLTerm::Direction::InOut;
  }
}

class SNLYosysJSONSaxHandler: public json::json_sax_t {
  public:
    explicit SNLYosysJSONSaxHandler(SNLLibrary* primitives, SNLLibrary* designs):
      primitives_(primitives),
      designs_(designs) {}
    bool null() override {
      std::cout << "null" << std::endl;
      assert(not contexts_.empty());
      contexts_.pop();
      return true;
    }
    bool boolean(bool val) override {
      std::cout << "bool: " << val << std::endl;
      assert(not contexts_.empty());
      contexts_.pop();
      return true;
    }
    bool number_integer(number_integer_t val) override {
      std::cout << "integer: " << val << std::endl;
      assert(not contexts_.empty());
      contexts_.pop();
      return true;
    }

    bool number_unsigned(number_unsigned_t val) override {
      std::cout << "unsigned: " << val << std::endl;
      assert(not contexts_.empty());
      auto tag = contexts_.top().first;
      port_.bit_ = val;

      contexts_.pop();
      return true;
    }

    bool number_float(number_float_t val, const string_t& s) override {
      std::cout << "float: " << s << ":" << val << std::endl;
      assert(not contexts_.empty());
      contexts_.pop();
      return true;
    }

    bool string(string_t& val) override {
      std::cout << "string: " << val << std::endl;
      assert(not contexts_.empty());
      auto tag = contexts_.top().first;
      if (tag == Tag::Direction) {
        port_.direction_ = val;
      } else if (tag == Tag::CellType) {
        cell_.type_ = val;
      } else if (tag == Tag::CellPort) {
        std::cout << "cellport direction = " << val << std::endl;
        assert(not cell_.ports_.empty());
        cell_.ports_.back().direction_ = val;
      }
      contexts_.pop();
      return true;
    }

    bool binary(binary_t& val) override {
      std::cout << "binary: " << std::endl;
      assert(not contexts_.empty());
      contexts_.pop();
      return true;
    }
    bool start_object(std::size_t elements) override {
      std::cout << "start obj: " << std::endl;
      if (contexts_.empty()) {
        contexts_.push(Context(Tag::Top, "Top"));
      }
      return true;
    }
    bool key(string_t& val) override {
      std::cout << "key: " << val << std::endl;
      if (val == "modules") {
        contexts_.push(Context(Tag::Modules, val));
        return true;
      } else if (val == "ports") {
        contexts_.push(Context(Tag::Ports, val));
        return true;
      } else if (val == "port_directions") {
        contexts_.push(Context(Tag::CellPorts, val));
        return true;
      } else if (val == "direction") {
        assert((not contexts_.empty()) and contexts_.top().first == Tag::Port);
        contexts_.push(Context(Tag::Direction, val));
        return true;
      } else if (val == "bits") {
        if ((not contexts_.empty()) and contexts_.top().first == Tag::Port) {
          contexts_.push(Context(Tag::PortBits, val));
          return true;
        }
      } else if (val == "type") {
        if ((not contexts_.empty()) and contexts_.top().first == Tag::Cell) {
          contexts_.push(Context(Tag::CellType, val));
          return true;
        }
      } else if (val == "cells") {
        contexts_.push(Context(Tag::Cells, val));
        return true;
      } else if (not contexts_.empty()) {
        auto context = contexts_.top();
        auto tag = context.first;
        if (tag == Tag::Modules) {
          design_ = SNLDesign::create(designs_, SNLName(val));
          std::cout << "CREATED " << design_->getString() << std::endl;
          contexts_.push(Context(Tag::Module, val));
          return true;
        } if (tag == Tag::Ports) {
          port_ = Port(val);
          contexts_.push(Context(Tag::Port, val));
          return true;
        } if (tag == Tag::CellPorts) {
          cell_.ports_.push_back(Port(val));
          contexts_.push(Context(Tag::CellPort, val));
          return true;
        } else if (tag == Tag::Cells) {
          cell_ = Cell(val);
          contexts_.push(Context(Tag::Cell, val));
          return true;
        }
      }
      contexts_.push(Context(Tag::Key, val));
      return true;
    }
    bool end_object() override {
      assert(not contexts_.empty());
      auto context = contexts_.top();
      auto tag = context.first;
      auto key = context.second;
      std::cout << "end obj: " << tag.getString() << ": " << key << std::endl;
      if (tag == Tag::Module) {
        design_ = nullptr;
      } else if (tag == Tag::Port) {
        auto term = SNLScalarTerm::create(design_, getSNLDirection(port_.direction_), SNLName(port_.name_));
        std::cout << "CREATED " << term->getString() << std::endl;
        port_ = Port();
      } else if (tag == Tag::Cell) {
        createInstance(cell_);
        cell_ = Cell();
      }
      contexts_.pop();
      return true;
    }
    bool start_array(std::size_t elements) override {
      std::cout << "start array" << std::endl;
      if (not contexts_.empty()) {
        auto tag = contexts_.top().first;
        if (tag == Tag::PortBits) {
          contexts_.push(Context(Tag::PortBitsArray, ""));
          return true;
        }
      }
      contexts_.push(Context(Tag::Array, ""));
      return true;
    }
    bool end_array() override {
      contexts_.pop();
      std::cout << "end array" << std::endl;
      return true;
    }
    bool parse_error(std::size_t position,
        const std::string& last_token,
        const nlohmann::detail::exception& ex) override {
      std::cout << "ERROR: " << last_token << std::endl;
      return true;
    }

    class Tag {
      public:
        enum TagEnum {
          Top, Key, Modules, Module, Ports, Port, Direction, PortBits,
          Array, PortBitsArray,
          Cells, Cell, CellType, CellPorts, CellPort
        };
        explicit Tag(const TagEnum& tagEnum): tagEnum_(tagEnum) {}
        Tag(const Tag& tag) = default;
        operator const TagEnum&() const { return tagEnum_; }
        std::string getString() const {
          switch (tagEnum_) {
            case Top:
              return "Top";
            case Key:
              return "Key";
            case Modules:
              return "Modules";
            case Module:
              return "Module";
            case Ports:
              return "Ports";
            case Port:
              return "Port";
            case Direction:
              return "Direction";
            case PortBits:
              return "PortBits";
            case Array:
              return "Array";
            case PortBitsArray:
              return "PortBitsArray";
            case Cells:
              return "Cells";
            case Cell:
              return "Cell";
            case CellType:
              return "CellType";
            case CellPorts:
              return "CellPorts";
            case CellPort:
              return "CellPort";
          }
          return "ERROR";
        }
      private:
          TagEnum tagEnum_;
    };
    struct Port {
      Port() = default;
      explicit Port(const std::string& name): undefined_(false), name_(name) {}
      std::string getString() const {
        return "<Port n:" + name_ + " d:" + direction_ + "b:" + std::to_string(bit_) + ">";
      }
      bool        undefined_  {true};
      std::string name_       {};
      std::string direction_;
      unsigned    bit_        {0};
    };
    struct Cell {
      Cell() = default;
      Cell(const std::string& name): name_(name) {}
      std::string getString() const {
        std::ostringstream stream;
        stream << "<Cell u:" << std::to_string(undefined_);
        if (anonymous_) {
          stream << " anon";
        } else {
          stream << " n:" << name_;
        }
        stream << " t:" + type_ + ">";
        return stream.str();
      }
      using Ports = std::vector<Port>;
      bool        undefined_  {true};
      bool        anonymous_  {false};
      std::string name_       {};
      std::string type_       {};
      Ports       ports_      {};
    };

    void createInstance(const Cell& cell) {
      std::string type = cell.type_;
      assert(not type.empty());
      if (type.starts_with("$")) {
        //create primitive
        std::string primName = cell.type_.substr(1, std::string::npos);
        auto prim = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, SNLName(primName));
        for (auto port: cell.ports_) {
          SNLScalarTerm::create(prim, getSNLDirection(port.direction_), SNLName(port.name_));
        }
        //now create instance
        //FIXME: manage anonymous
        SNLInstance::create(design_, prim, SNLName(cell.name_));
      } else {
        auto model = designs_->getDesign(SNLName(type));
        assert(model);
        SNLInstance::create(design_, model, SNLName(cell.name_));
      }
    }

    using Context = std::pair<Tag, std::string>;
    using Contexts = std::stack<Context>;
    Contexts    contexts_     {};
    SNLLibrary* primitives_   {nullptr}; 
    SNLLibrary* designs_       {nullptr}; 
    bool        inModules_    {false};
    SNLDesign*  design_       {nullptr};
    Port        port_         {};
    Cell        cell_         {};
};

}

namespace naja { namespace SNL {

void SNLYosysJSONParser::parse(
  std::istream& input,
  SNLLibrary* primitives,
  SNLLibrary* designs) {
  json j;
  try {
    SNLYosysJSONSaxHandler handler(primitives, designs);
    bool result = json::sax_parse(input, &handler);
    assert(handler.contexts_.empty());
  } catch (json::parse_error& ex) {
    std::stringstream stream;
    stream << "Parse error at byte " << ex.byte;
    core::NajaLog::error("YOSYS-JSON", stream.str());
  }
}

void SNLYosysJSONParser::parse(
  const std::filesystem::path& inputPath,
  SNLLibrary* primitives,
  SNLLibrary* designs) {
  if (not std::filesystem::exists(inputPath)) {
    std::stringstream reason;
    reason << inputPath.string() << " is not a valid path";
    throw SNLException(reason.str());
  }
  std::ifstream input;
  input.open(inputPath);
  core::NajaLog::echo("YOSYS-JSON", "Starting Parsing of " + inputPath.string());
  parse(input, primitives, designs);
}

}} // namespace SNL // namespace naja
