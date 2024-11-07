#include <iostream>
#include <fstream>

#include <argparse/argparse.hpp>

// All DEBUG/TRACE statements will be removed by the pre-processor
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h> // support for basic file logging
#include <spdlog/sinks/stdout_color_sinks.h>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "NajaUtils.h"
#include "NajaPerf.h"

#include "SNLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLCapnP.h"
#include "SNLPyLoader.h"
#include "SNLLibertyConstructor.h"
#include "SNLVRLConstructor.h"
#include "SNLUtils.h"
#include "SNLException.h"

using namespace naja::SNL;

static void helloWorld() {
  ImGui::Begin("My DearImGui Window");
  ImGui::Text("hello, world");
  ImGui::End();
}

namespace {

const std::string NAJA_GUI_MAJOR("0");
const std::string NAJA_GUI_MINOR("1");
const std::string NAJA_GUI_REVISION("0");
const std::string NAJA_GUI_VERSION(
  NAJA_GUI_MAJOR + "." +
  NAJA_GUI_MINOR + "." +
  NAJA_GUI_REVISION);

enum class FormatType { NOT_PROVIDED, UNKNOWN, VERILOG, SNL, DOT, SVG};
FormatType argToFormatType(const std::string& inputFormat) {
  if (inputFormat.empty()) {
    return FormatType::NOT_PROVIDED;
  } else if (inputFormat == "verilog") {
    return FormatType::VERILOG;
  } else if (inputFormat == "snl") {
    return FormatType::SNL;
  } else if (inputFormat == "dot") {
    return FormatType::DOT;
  /*} else if (inputFormat == "svg") {
    return FormatType::SVG;*/
  } else {
    return FormatType::UNKNOWN;
  }
}

void showInstanceHierarchy(const naja::SNL::SNLDesign* design) {
  if (!design) return;

  auto nbTerms = design->getTerms().size();
  if (nbTerms > 0) {
    if (ImGui::TreeNode("Terms")) {
      for (const auto& term: design->getTerms()) {
        if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
          if (ImGui::TreeNode((void*)term, "%s", busTerm->getName().getString().c_str())) {
            for (const auto& bit: busTerm->getBits()) {
              ImGui::Text("%i", bit->getBit());
            }
            ImGui::TreePop();
          }
        } else { 
          auto scalarTerm = static_cast<SNLScalarTerm*>(term);
          ImGui::Text("%s", scalarTerm->getName().getString().c_str());
        }
      }
      ImGui::TreePop();
    }
  }

  auto nbNets = design->getNonAssignConstantNets().size();
  if (nbNets > 0) {
    if (ImGui::TreeNode("Nets")) {
      for (const auto& net: design->getNonAssignConstantNets()) {
        if (auto busNet = dynamic_cast<SNLBusNet*>(net)) {
          if (ImGui::TreeNode((void*)net, "%s", busNet->getName().getString().c_str())) {
            for (const auto& bit: busNet->getBusBits()) {
              ImGui::Text("%i", bit->getBit());
            }
            ImGui::TreePop();
          }
        } else { 
          auto scalarNet = static_cast<SNLScalarNet*>(net);
          ImGui::Text("%s", scalarNet->getName().getString().c_str());
        }
      }
      ImGui::TreePop();
    }
  }

  auto nbInstances = design->getInstances().size();
  if (nbInstances > 0) {
    if (ImGui::TreeNode("Instances")) {
      for (const auto& instance: design->getInstances()) {
        // Display the instance name
        std::string instanceName = instance->getName().getString();
        if (instance->isAnonymous()) {
            instanceName = std::to_string(instance->getID());
        }

        if (instance->isLeaf()) {
          ImGui::Text("%s", instanceName.c_str());
        } else {
          // Check if the instance has a model (another SNLDesign)
          const naja::SNL::SNLDesign* model = instance->getModel();
          // Create a tree node for the instance
          if (ImGui::TreeNode((void*)instance, "%s", instanceName.c_str())) {
            showInstanceHierarchy(model);
            ImGui::TreePop();
          }
        }
      }
      ImGui::TreePop();
    }
  }
}

void showTopHierarchy(const naja::SNL::SNLDesign* design) {
  if (!design) return;

  if (ImGui::TreeNode(design->getName().getString().c_str())) {
    showInstanceHierarchy(design);
    ImGui::TreePop();
  }
}


}

int main(int argc, char* argv[]) {
  const auto najaGUIStart{std::chrono::steady_clock::now()};
  argparse::ArgumentParser program("naja_gui");
  program.add_description("naja_gui");
  program.add_argument("-f", "--from_format").help("from/input format");
  program.add_argument("-i", "--input").append().help("input netlist paths");
  program.add_argument("-p", "--primitives")
    .nargs(argparse::nargs_pattern::at_least_one)
    .help("input primitives: list of path to primitives files (liberty format or Naja python format)");
  program.add_argument("-l", "--log")
    .default_value(std::string("naja_gui.log"))
    .help("Dump log file (default name: naja_gui.log)");
  program.add_argument("-s", "--stats")
    .default_value(std::string("naja_gui.stats"))
    .help("Dump stats log file named: naja_gui.stats");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  std::filesystem::path statsPath("naja_stats.log");
  if (program.is_used("--stats")) {
    statsPath = program.get<std::string>("--stats");
  }
  naja::NajaPerf::create(statsPath, "naja_edit");

  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  console_sink->set_pattern("[naja_gui] [%^%l%$] %v");
  sinks.push_back(console_sink);

  if (program.is_used("--log")) {
    auto logName = program.get<std::string>("--log");
    {
      std::ofstream logFile(logName, std::ios::out);
      if (logFile.is_open()) {
        std::string bannerTitle = "naja_gui " + NAJA_GUI_VERSION;
        std::ostringstream bannerStream;
        naja::NajaUtils::createBanner(logFile, bannerTitle, "#");
        logFile << std::endl;
        logFile.close();
      }
    }
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logName);
    file_sink->set_level(spdlog::level::trace);
    sinks.push_back(file_sink);
  }

  auto edit_logger =
      std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
  edit_logger->set_level(spdlog::level::trace);

  spdlog::set_default_logger(edit_logger);
  spdlog::flush_every(std::chrono::seconds(3));

  bool argError = false;

  std::string inputFormat;
  if (auto inputFormatArg = program.present("-f")) {
    inputFormat = *inputFormatArg;
  }
  FormatType inputFormatType = argToFormatType(inputFormat);
  if (inputFormatType == FormatType::UNKNOWN) {
    SPDLOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
    argError = true;
  }

  if (program.present("-i")) {
    if (inputFormatType == FormatType::NOT_PROVIDED) {
      SPDLOG_CRITICAL("output option (-f) is mandatory when the input is provided");
      std::exit(EXIT_FAILURE);
    }
  }

  using Paths = std::vector<std::filesystem::path>;

  Paths primitivesPaths;
  if (auto primitives = program.present("-p")) {
    if (inputFormatType == FormatType::SNL) {
      SPDLOG_CRITICAL("primitives option (-p) is incompatible with input format 'SNL'");
      argError = true;
    }
    auto primitivesPathsString = program.get<std::vector<std::string>>("-p");
    primitivesPaths.resize(primitivesPathsString.size());
    std::transform(primitivesPathsString.begin(),
      primitivesPathsString.end(),
      primitivesPaths.begin(),
      [](const std::string& str) {
        return std::filesystem::path(str);
      }
    );
  } else {
    if (inputFormatType != FormatType::SNL and inputFormatType != FormatType::NOT_PROVIDED) {
      SPDLOG_CRITICAL("primitives option (-p) is mandatory when the input format is not 'SNL'");
      argError = true;
    }
  }

  if (argError) {
    std::exit(-1);
  }

  using StringPaths = std::vector<std::string>;
  StringPaths inputStringPaths = program.get<StringPaths>("-i");

  Paths inputPaths;
  std::transform(inputStringPaths.begin(), inputStringPaths.end(),
                 std::back_inserter(inputPaths),
                 [](const std::string& sp) -> std::filesystem::path {
                   return std::filesystem::path(sp);
                 });

  try {
    SNLDB* db = nullptr;
    SNLLibrary* primitivesLibrary = nullptr;
    {
      naja::NajaPerf::Scope scope("SNL Creation");
      SNLUniverse::create();

      if (inputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Parsing SNL format");
        if (inputPaths.size() > 1) {
          SPDLOG_CRITICAL("Multiple input paths are not supported for SNL format");
          std::exit(EXIT_FAILURE);
        }
        const auto start{std::chrono::steady_clock::now()};
        auto inputPath = inputPaths[0];
        db = SNLCapnP::load(inputPath);
        SNLUniverse::get()->setTopDesign(db->getTopDesign());
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          SPDLOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Parsing verilog");
        db = SNLDB::create(SNLUniverse::get());
        primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives,
                                               SNLName("PRIMS"));
        for (const auto& path : primitivesPaths) {
          SPDLOG_INFO("Parsing primitives file: {}", path.string());
          auto extension = path.extension();
          if (extension.empty()) {
            SPDLOG_CRITICAL("Primitives path should end with an extension");
            std::exit(EXIT_FAILURE);
          } else if (extension == ".py") {
            SNLPyLoader::loadPrimitives(primitivesLibrary, path);
          } else if (extension == ".lib") {
            SNLLibertyConstructor constructor(primitivesLibrary);
            constructor.construct(path);
          } else {
            SPDLOG_CRITICAL("Unknow extension in Primitives path");
            std::exit(EXIT_FAILURE);
          }
        }

        auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
        SNLVRLConstructor constructor(designLibrary);
        const auto start{std::chrono::steady_clock::now()};
        {
          std::ostringstream oss;
          oss << "Parsing verilog file(s): ";
          size_t i = 0;
          for (auto path : inputPaths) {
            if (i++ >= 4) {
              oss << std::endl;
              i = 0;
            }
            oss << path << " ";
          }
          SPDLOG_INFO(oss.str());
        }
        constructor.construct(inputPaths);
        auto top = SNLUtils::findTop(designLibrary);
        if (top) {
          SNLUniverse::get()->setTopDesign(top);
          SPDLOG_INFO("Found top design: " + top->getString());
        } else {
          SPDLOG_ERROR("No top design was found after parsing verilog");
        }
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          SPDLOG_INFO(oss.str());
        }
    } else if (inputFormatType == FormatType::NOT_PROVIDED) {
      db = SNLDB::create(SNLUniverse::get());
      SNLUniverse::get()->setTopDB(db);
      } else {
        SPDLOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
        std::exit(EXIT_FAILURE);
      }
    }

    auto top = SNLUniverse::getTopDesign();
    if (not top) {
      SPDLOG_CRITICAL("No top to display");
      std::exit(EXIT_FAILURE);
    }

    // Setup window
    glfwSetErrorCallback([](int error, const char* description) {
      fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    });
    glfwInit();

    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(800, 600, "My GLFW Window", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    glewInit();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    while (!glfwWindowShouldClose(window)) {
      // Poll and handle events (inputs, window resize, etc.)
      glfwPollEvents();

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      showTopHierarchy(top);

      // Rendering
      ImGui::Render();
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

  } catch (const SNLException& e) {
    //SPDLOG_CRITICAL("Caught SNL error: {}\n{}",
    //  e.what(), e.trace().to_string()); 
    SPDLOG_CRITICAL("Caught SNL error: {}\n", e.what());
    std::exit(EXIT_FAILURE);
  }
  const auto najaGUIEnd{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> najaElapsedSeconds{najaGUIEnd - najaGUIStart};
  auto memInfo = naja::NajaPerf::getMemoryUsage();
  auto vmRSS = memInfo.first;
  auto vmPeak = memInfo.second;
  SPDLOG_INFO("########################################################");
  {
    std::ostringstream oss;
    oss << "naja_gui done in: " << najaElapsedSeconds.count() << "s";
    if (vmRSS != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(RSS): " << vmRSS / 1024.0 << "Mb";
    }
    if (vmPeak != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(Peak): " << vmPeak / 1024.0 << "Mb";
    }
    SPDLOG_INFO(oss.str());
  }
  SPDLOG_INFO("########################################################");
  std::exit(EXIT_SUCCESS);
}
