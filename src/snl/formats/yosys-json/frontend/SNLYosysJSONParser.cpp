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
    SNLYosysJSONSaxHandler(SNLLibrary* library): library_(library) {}
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
      contexts_.pop();
      #if 0
      if (not context_.empty()) {
        auto tag = context_.top().second;
        if (tag == Tag::Bits) {
          //port for the moment
          port_.bit_ = val;
        }
      }
      #endif
      std::cout << "unsigned: " << val << std::endl;
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
      } else if (val == "direction") {
        assert((not contexts_.empty()) and contexts_.top().first == Tag::Port);
        contexts_.push(Context(Tag::Direction, val));
        return true;
      } else if (not contexts_.empty()) {
        auto context = contexts_.top();
        auto tag = context.first;
        if (tag == Tag::Modules) {
          design_ = SNLDesign::create(library_, SNLName(val));
          std::cout << "CREATED " << design_->getString() << std::endl;
          contexts_.push(Context(Tag::Module, val));
          return true;
        } if (tag == Tag::Ports) {
          port_ = Port(val);
          contexts_.push(Context(Tag::Port, val));
          return true;
        }
      }
      contexts_.push(Context(Tag::Key, val));
      /*
        contexts_.top().first = Tag::Modules;
      } else if (val == "ports") {
        assert(not contexts_.empty());
        contexts_.top().first = Tag::Ports;
      } else if (val == "direction") {
        assert(not contexts_.empty());
        assert(contexts_.top().first == Tag::Port);
        contexts_.push(Context(Tag::Direction, ""));
      } else if (not contexts_.empty()) {
        auto tag = contexts_.top().first;
        if (tag == Tag::Module) {
          contexts_.top().second = val;
        } else if (tag == Tag::Port) {
          contexts_.top().second = val;
          port_ = Port(val);
        }
      }
      */
      /*
      if (not context_.empty()) {
        auto tag = context_.top().second;
        if (tag == Tag::Modules) {
          //create module
          auto design = SNLDesign::create(library_, SNLName(val));
          std::cout << "CREATED " << design->getString() << std::endl;
          detectedContext = true;
        } else if (tag == Tag::Ports) {
          port_ = Port(val);
          std::cout << "CREATED " << port_.getString() << std::endl;
          context_.push(Object(val, Tag::Port));
          detectedContext = true;
        } else {
          std::cout << "Not detected context: " << tag.getString() << std::endl;
        }
      }
      if (not detectedContext) {
        if (val == "attributes") {
          context_.push(Object(val, Tag::DontCare));
        } else if (val == "modules") {
          context_.push(Object(val, Tag::Modules));
        } else if (val == "ports") {
          context_.push(Object(val, Tag::Ports));
        } else if (val == "direction") {
          context_.push(Object(val, Tag::Direction));
        } else if (val == "bits") {
          context_.push(Object(val, Tag::Bits));
        } else {
          context_.push(Object(val, Tag::DontCare));
        }
      }
      std::cout << "Context " << context_.top().second.getString() << std::endl;
      startObject_ = false;
      */
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
        port_.name_ = std::string();
      }
      contexts_.pop();
      return true;
    }
    bool start_array(std::size_t elements) override {
      std::cout << "start array" << std::endl;
      contexts_.push(Context(Tag::Array, ""));
      #if 0
      if (not context_.empty()) {
        auto tag = context_.top().second;
        if (tag == Tag::Bits) {
          std::cout << "In Bits" << std::endl;
        }
      }
      #endif
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
          Top, Key, Array, Modules, Module, Ports, Port, Direction //, Bits
        };
        Tag(const TagEnum& tagEnum): tagEnum_(tagEnum) {}
        Tag(const Tag& tag) = default;
        operator const TagEnum&() const { return tagEnum_; }
        std::string getString() const {
          switch (tagEnum_) {
            case Top:
              return "Top";
            case Key:
              return "Key";
            case Array:
              return "Array";
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
            /*
            case Bits:
              return "Bits";
            */
          }
          return "ERROR";
        }
      private:
          TagEnum tagEnum_;
    };
    struct Port {
      Port() {}
      Port(const std::string& name): name_(name) {}
      std::string getString() const {
        return "<Port n:" + name_ + " d:" + direction_ + "b:" + std::to_string(bit_) + ">";
      }
      std::string name_;
      std::string direction_;
      unsigned bit_;
    };
    using Context = std::pair<Tag, std::string>;
    using Contexts = std::stack<Context>;
    Contexts    contexts_      {};
    SNLLibrary* library_      {nullptr}; 
    bool        inModules_    {false};
    SNLDesign*  design_       {nullptr};
    Port        port_         {};
};

}

namespace naja { namespace SNL {

void SNLYosysJSONParser::parse(std::istream& input, SNLLibrary* library) {
  json j;
  try {
    SNLYosysJSONSaxHandler handler(library);
    bool result = json::sax_parse(input, &handler);
    assert(handler.contexts_.empty());
  } catch (json::parse_error& ex) {
    std::stringstream stream;
    stream << "Parse error at byte " << ex.byte;
    core::NajaLog::error("YOSYS-JSON", stream.str());
  }
}

void SNLYosysJSONParser::parse(const std::filesystem::path& inputPath, SNLLibrary* library) {
  if (not std::filesystem::exists(inputPath)) {
    std::stringstream reason;
    reason << inputPath.string() << " is not a valid path";
    throw SNLException(reason.str());
  }
  std::ifstream input;
  input.open(inputPath);
  core::NajaLog::echo("YOSYS-JSON", "Starting Parsing of " + inputPath.string());
  parse(input, library);
}

}} // namespace SNL // namespace naja
