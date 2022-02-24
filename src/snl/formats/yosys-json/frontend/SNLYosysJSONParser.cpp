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
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
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
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "null" << std::endl;
#endif
      popContexts();
      return true;
    }
    bool boolean(bool val) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "bool: " << val << std::endl;
#endif
      popContexts();
      return true;
    }
    bool number_integer(number_integer_t val) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "integer: " << val << std::endl;
#endif
      popContexts();
      return true;
    }

    bool number_unsigned(number_unsigned_t val) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "unsigned: " << val << std::endl;
#endif
      assert(not contexts_.empty());
      auto tag = contexts_.top().first;
      if (tag == Tag::PortBitsArray) {
        port_.bits_.push_back(val);
        return true;
      } else if (tag == Tag::NetBitsArray) {
        assert(not netInfos_.undefined_);
        netInfos_.bits_.push_back(val);
        return true;
      } else if (tag == Tag::CellConnectionArray) {
        assert(not currentCellPort_.empty());
        auto it = cell_.ports_.find(currentCellPort_);
        assert(it != cell_.ports_.end());
        Port& port = it->second;
        port.bits_.push_back(val);
        return true;
      } else if (tag == Tag::CellHideName) {
        assert(not cell_.undefined_);
        if (val == 1) {
          cell_.anonymous_ = true;
        } else if (val == 0) {
          cell_.anonymous_ = false;
        } else {
          assert(false);
        }
      } else if (tag == Tag::NetHideName) {
        assert(not netInfos_.undefined_);
        if (val == 1) {
          netInfos_.anonymous_ = true;
        } else if (val == 0) {
          netInfos_.anonymous_ = false;
        } else {
          assert(false);
        }
      }
      popContexts();
      return true;
    }

    bool number_float(number_float_t val, const string_t& s) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "float: " << s << ":" << val << std::endl;
#endif
      popContexts();
      return true;
    }

    bool string(string_t& val) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "string: " << val << std::endl;
#endif
      assert(not contexts_.empty());
      auto tag = contexts_.top().first;
      if (tag == Tag::Direction) {
        port_.direction_ = val;
      } else if (tag == Tag::CellType) {
        cell_.type_ = val;
      } else if (tag == Tag::CellPortDirection) {
#ifdef YOSYS_JSON_PARSER_DEBUG
        std::cout << "cellport direction = " << val << std::endl;
#endif
        assert(not currentCellPort_.empty());
        auto it = cell_.ports_.find(currentCellPort_);
        assert(it != cell_.ports_.end());
        Port& port = it->second;
        port.direction_ = val;
        currentCellPort_ = std::string();
      } else if (tag == Tag::CellConnection) {
        auto it = cell_.ports_.find(val);
        if (it == cell_.ports_.end()) {
          std::ostringstream reason;
          reason << "cannot find " << val << " in cell ports, current ports are:" << std::endl;
          for (auto portIt: cell_.ports_) {
            reason << portIt.first << std::endl;
          }
          std::cerr << reason.str() << std::endl;
        }
        assert(it != cell_.ports_.end());
      }
      popContexts();
      return true;
    }

    bool binary(binary_t& val) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "binary: " << std::endl;
#endif
      popContexts();
      return true;
    }
    bool start_object(std::size_t elements) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "start obj: " << std::endl;
#endif
      if (contexts_.empty()) {
        contexts_.push(Context(Tag::Top, "Top"));
      }
      return true;
    }
    bool key(string_t& val) override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "key: " << val << std::endl;
#endif
      if (val == "modules") {
        contexts_.push(Context(Tag::Modules, val));
        return true;
      } else if (val == "ports") {
        contexts_.push(Context(Tag::Ports, val));
        return true;
      } else if (val == "port_directions") {
        contexts_.push(Context(Tag::CellPortDirections, val));
        return true;
      } else if (val == "connections") {
        contexts_.push(Context(Tag::CellConnections, val));
        return true;
      } else if (val == "direction") {
        assert((not contexts_.empty()) and contexts_.top().first == Tag::Port);
        contexts_.push(Context(Tag::Direction, val));
        return true;
      } else if (val == "bits") {
        if ((not contexts_.empty())) {
          if (contexts_.top().first == Tag::Port) {
            contexts_.push(Context(Tag::PortBits, val));
            return true;
          } else if (contexts_.top().first == Tag::Net) {
            contexts_.push(Context(Tag::NetBits, val));
            return true;
          }
        }
      } else if (val == "type") {
        if ((not contexts_.empty()) and contexts_.top().first == Tag::Cell) {
          contexts_.push(Context(Tag::CellType, val));
          return true;
        }
      } else if (val == "hide_name") {
        if ((not contexts_.empty())) {
          if  (contexts_.top().first == Tag::Cell) {
            contexts_.push(Context(Tag::CellHideName, val));
            return true;
          } else if (contexts_.top().first == Tag::Net) {
            contexts_.push(Context(Tag::NetHideName, val));
            return true;
          }
        }
      } else if (val == "cells") {
        contexts_.push(Context(Tag::Cells, val));
        return true;
      } else if (val == "netnames") {
        contexts_.push(Context(Tag::Nets, val));
        return true;
      } else if (not contexts_.empty()) {
        auto context = contexts_.top();
        auto tag = context.first;
        if (tag == Tag::Modules) {
          design_ = SNLDesign::create(designs_, SNLName(val));
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << "CREATED " << design_->getString() << std::endl;
#endif
          contexts_.push(Context(Tag::Module, val));
          return true;
        } if (tag == Tag::Ports) {
          port_ = Port(val);
          contexts_.push(Context(Tag::Port, val));
          return true;
        } if (tag == Tag::CellPortDirections) {
          currentCellPort_ = val;
          cell_.addPort(val);
          contexts_.push(Context(Tag::CellPortDirection, val));
          return true;
        } else if (tag == Tag::Cells) {
          cell_ = Cell(val);
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << "Start cell: " << cell_.getString() << std::endl;
#endif
          contexts_.push(Context(Tag::Cell, val));
          return true;
        } else if (tag == Tag::CellConnections) {
          //val here is cell port name
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << "Found cell connection: " << val << std::endl;
#endif
          contexts_.push(Context(Tag::CellConnection, val));
          currentCellPort_ = val;
          return true;
        } else if (tag == Tag::Nets) {
          netInfos_ = NetInfos(val);
          contexts_.push(Context(Tag::Net, val));
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
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "end obj: " << tag.getString() << ": " << key << std::endl;
#endif
      if (tag == Tag::Module) {
        createConnections();
        design_ = nullptr;
        connections_ = Connections();
      } else if (tag == Tag::Port) {
        createAndConnectTerm(design_, port_);
        port_ = Port();
      } else if (tag == Tag::Cell) {
        createInstance();
#ifdef YOSYS_JSON_PARSER_DEBUG
        std::cout << "End cell: " << cell_.getString() << std::endl;
#endif
        cell_ = Cell();
      } else if (tag == Tag::Net) {
        createNet();
        netInfos_ = NetInfos();
      }
      popContexts();
      return true;
    }
    bool start_array(std::size_t elements) override {
      if (not contexts_.empty()) {
        auto tag = contexts_.top().first;
        if (tag == Tag::PortBits) {
          contexts_.push(Context(Tag::PortBitsArray, ""));
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << "start " << contexts_.top().first.getString() << std::endl;
#endif
          return true;
        } else if (tag == Tag::NetBits) {
          contexts_.push(Context(Tag::NetBitsArray, ""));
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << "start " << contexts_.top().first.getString() << std::endl;
#endif
          return true;
        } else if (tag == Tag::CellConnection) {
          contexts_.push(Context(Tag::CellConnectionArray, ""));
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << "start " << contexts_.top().first.getString() << std::endl;
#endif
          return true;
        }
      }
      contexts_.push(Context(Tag::Array, ""));
      std::cout << "start array" << std::endl;
      return true;
    }
    bool end_array() override {
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "end " << contexts_.top().first.getString() << std::endl;
#endif
      if (not contexts_.empty()) {
        auto tag = contexts_.top().first;
        if (tag == Tag::PortBitsArray) {
          popContexts();
        } else if (tag == Tag::CellConnectionArray) {
          currentCellPort_ = std::string();
          popContexts();
        } else if (tag == Tag::NetBitsArray) {
          popContexts();
        }
      }
      popContexts();
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
          Cells, Cell, CellType, CellHideName, CellPortDirections, CellPortDirection,
          CellConnections, CellConnection, CellConnectionArray,
          Nets, Net, NetHideName, NetBits, NetBitsArray
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
            case CellHideName:
              return "CellHideName";
            case CellPortDirections:
              return "CellPortDirections";
            case CellPortDirection:
              return "CellPortDirection";
            case CellConnections:
              return "CellConnections";
            case CellConnection:
              return "CellConnection";
            case CellConnectionArray:
              return "CellConnectionArray";
            case Nets:
              return "Nets";
            case Net:
              return "Net";
            case NetBits:
              return "NetBits";
            case NetBitsArray:
              return "NetBitsArray";
            case NetHideName:
              return "NetHideName";
          }
          return "ERROR";
        }
      private:
          TagEnum tagEnum_;
    };
    using Bits = std::vector<unsigned>;
    struct Port {
      Port() = default;
      explicit Port(const std::string& name): undefined_(false), name_(name) {}
      std::string getString() const {
        std::ostringstream stream;
        stream << "<Port";
        if (undefined_) {
          stream << " undefined>";
        } else {
          stream << " n:" + name_ + " d:" + direction_;
          stream << " b:[";
          for (auto bit: bits_) {
            stream << " " << std::to_string(bit);
          }
          stream << "]>";
        }
        return stream.str();
      }
      bool definedBits() const { return not bits_.empty(); }
      bool        undefined_  {true};
      std::string name_       {};
      std::string direction_  {};
      Bits        bits_       {};
    };
    struct NetInfos {
      NetInfos() = default;
      explicit NetInfos(const std::string& name): undefined_(false), name_(name) {}
      std::string getString() const {
        std::ostringstream stream;
        stream << "<NetInfos";
        if (undefined_) {
          stream << " undefined>";
        } else {
          if (anonymous_) {
            stream << " anon";
          } else {
            stream << " n:" << name_;
          }
          stream << " b:[";
          for (auto bit: bits_) {
            stream << " " << std::to_string(bit);
          }
          stream << "]>";
        }
        return stream.str();
      }
      bool definedBits() const { return not bits_.empty(); }
      bool        undefined_  {true};
      bool        anonymous_  {false};
      std::string name_       {};
      Bits        bits_       {};
    };
    struct Cell {
      Cell() = default;
      explicit Cell(const std::string& name): undefined_(false), name_(name) {}
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
      void addPort(const std::string& name) {
        ports_[name] = Port(name);
      }
      using Ports = std::map<std::string, Port>;
      bool        undefined_  {true};
      bool        anonymous_  {false};
      std::string name_       {};
      std::string type_       {};
      Ports       ports_      {};
    };
    using Components = std::vector<SNLNetComponent*>;
    using Net = std::pair<NetInfos, Components>;
    using Connections = std::map<unsigned, Net>;

    void createConnections() {
      for (auto connection: connections_) {
        const NetInfos& netInfos = connection.second.first;
        const Components& components = connection.second.second;
        SNLBitNet* net = nullptr;
        if (netInfos.anonymous_) {
          net = SNLScalarNet::create(design_);
        } else {
          net = SNLScalarNet::create(design_, SNLName(netInfos.name_));
        }
        for (auto component: components) {
          component->setNet(net);
#ifdef YOSYS_JSON_PARSER_DEBUG
          std::cout << component->getString() << std::endl;
          std::cout << "setNet: " << net->getString() << std::endl;
#endif
        } 
      }
    }

    void createNet() {
      assert(not netInfos_.undefined_);
      assert(netInfos_.definedBits());
      if (netInfos_.bits_.size() == 1) {
        auto bit = netInfos_.bits_[0];
        auto it = connections_.find(bit);
        assert(it != connections_.end());
        it->second.first = netInfos_;
      }
    }

    void createInstance() {
      std::string type = cell_.type_;
      assert(not type.empty());
      SNLInstance* instance = nullptr;
      if (type.starts_with("$")) {
        //create primitive
        std::string primName = cell_.type_.substr(1, std::string::npos);
        auto prim = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, SNLName(primName));
        for (auto portIt: cell_.ports_) {
          const Port& port = portIt.second;
          assert(port.definedBits());
          if (port.bits_.size() == 1) {
            SNLScalarTerm::create(prim, getSNLDirection(port.direction_), SNLName(port.name_));
          } else {
            SNLBusTerm::create(prim, getSNLDirection(port.direction_), port.bits_.size()-1, 0, SNLName(port.name_));
          }
        }
        //now create instance
        if (cell_.anonymous_) {
          instance = SNLInstance::create(design_, prim);
        } else {
          instance = SNLInstance::create(design_, prim, SNLName(cell_.name_));
        }
      } else {
        auto model = designs_->getDesign(SNLName(type));
        assert(model);
        if (cell_.anonymous_) {
          instance = SNLInstance::create(design_, model);
        } else {
          instance = SNLInstance::create(design_, model, SNLName(cell_.name_));
        }
      }
      for (auto it: cell_.ports_) {
        const Port& port = it.second;
        connectInstTerms(instance, port);
      }
    }

    void connectInstTerms(SNLInstance* instance, const Port& port) { 
      SNLTerm* term = instance->getModel()->getTerm(SNLName(port.name_));
      assert(term);
      assert(port.definedBits());
      if (port.bits_.size() == 1) {
        SNLScalarTerm* scalarTerm = dynamic_cast<SNLScalarTerm*>(term);
        assert(scalarTerm);
        SNLInstTerm* instTerm = instance->getInstTerm(scalarTerm);
        assert(instTerm);
        auto bit = port.bits_[0];
        auto it = connections_.find(bit);
        if (it == connections_.end()) {
          connections_[bit] = Net(NetInfos(), {instTerm});
        } else {
          it->second.second.push_back(instTerm);
        }
      } else {
        SNLBusTerm* busTerm = dynamic_cast<SNLBusTerm*>(term);
        assert(busTerm);
        for (unsigned i=0; i<port.bits_.size(); ++i) {
          SNLBusTermBit* bitTerm = busTerm->getBit(port.bits_.size()-1-i);
          assert(bitTerm);
          SNLInstTerm* instTerm = instance->getInstTerm(bitTerm);
          assert(instTerm);
          auto bit = port.bits_[i];
          auto it = connections_.find(bit);
          if (it == connections_.end()) {
            connections_[bit] = Net(NetInfos(), {instTerm});
          } else {
            it->second.second.push_back(instTerm);
          }
        }
      }
    }

    void createAndConnectTerm(SNLDesign* design, const Port& port) {
      assert(port_.definedBits());
      if (port_.bits_.size() == 1) {
        SNLScalarTerm* term = SNLScalarTerm::create(design_, getSNLDirection(port_.direction_), SNLName(port_.name_));
        auto bit = port_.bits_[0];
        auto it = connections_.find(bit);
        if (it == connections_.end()) {
          connections_[bit] = Net(NetInfos(), {term});
        } else {
          it->second.second.push_back(term);
        }
      } else {
        SNLBusTerm* term = SNLBusTerm::create(design_, getSNLDirection(port_.direction_), port_.bits_.size()-1, 0, SNLName(port_.name_));
        for (unsigned i=0; i<port_.bits_.size(); ++i) {
          SNLBusTermBit* bitTerm = term->getBit(port_.bits_.size()-1-i);
          assert(bitTerm);
          auto bit = port_.bits_[i];
          auto it = connections_.find(bit);
          if (it == connections_.end()) {
            connections_[bit] = Net(NetInfos(), {bitTerm});
          } else {
            it->second.second.push_back(bitTerm);
          }
        }
#ifdef YOSYS_JSON_PARSER_DEBUG
        std::cout << "CREATED " << term->getString() << std::endl;
#endif
      }
    }

    void popContexts() {
      assert(not contexts_.empty());
#ifdef YOSYS_JSON_PARSER_DEBUG
      std::cout << "Pop: " << contexts_.top().first.getString() << std::endl;
#endif
      contexts_.pop();
    }

    using Context = std::pair<Tag, std::string>;
    using Contexts = std::stack<Context>;
    Contexts    contexts_         {};
    SNLLibrary* primitives_       {nullptr}; 
    SNLLibrary* designs_          {nullptr}; 
    SNLDesign*  design_           {nullptr};
    Port        port_             {};
    NetInfos    netInfos_         {};
    Cell        cell_             {};
    std::string currentCellPort_  {};
    Connections connections_      {};
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
