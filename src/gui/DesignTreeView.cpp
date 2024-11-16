#include "DesignTreeView.h"

#include "imgui.h"

#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
using namespace naja::SNL;

namespace {
  
void showInstanceHierarchy(const naja::SNL::SNLDesign* design) {
  if (!design) return;

  bool isLeaf = design->isLeaf();

  auto nbTerms = design->getTerms().size();
  if (nbTerms > 0) {
    bool expanded = isLeaf;
    if (not isLeaf) {
      expanded = ImGui::TreeNodeEx("Terms");
    }
    if (expanded) {
      for (const auto& term: design->getTerms()) {
        if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
          auto flags = ImGuiTreeNodeFlags_None;
          if (ImGui::TreeNodeEx((void*)term, flags, "%s[%i:%i]",
            busTerm->getName().getString().c_str(),
            busTerm->getMSB(),
            busTerm->getLSB())) {
            for (const auto& bit: busTerm->getBits()) {
              auto flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
              ImGui::TreeNodeEx((void*)bit, flags, "%i", bit->getBit());
            }
            ImGui::TreePop();
          }
        } else { 
          auto scalarTerm = static_cast<SNLScalarTerm*>(term);
          auto flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
          ImGui::TreeNodeEx((void*)term, flags, "%s", scalarTerm->getName().getString().c_str());
        }
      }
    }
    if (not isLeaf and expanded) {
      ImGui::TreePop();
    }
  }

  if (isLeaf) {
    return;
  }

  auto nbNets = design->getNonAssignConstantNets().size();
  if (nbNets > 0) {
    if (ImGui::TreeNodeEx("Nets")) {
      for (const auto& net: design->getNonAssignConstantNets()) {
        if (auto busNet = dynamic_cast<SNLBusNet*>(net)) {
          auto flags = ImGuiTreeNodeFlags_None;
          if (ImGui::TreeNodeEx((void*)net, flags, "%s[%i:%i]",
            busNet->getName().getString().c_str(),
            busNet->getMSB(),
            busNet->getLSB())) {
            for (auto bit: busNet->getBusBits()) {
              auto flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
              ImGui::TreeNodeEx((void*)bit, flags, "%i", bit->getBit());
            }
            ImGui::TreePop();
          }
        } else { 
          auto scalarNet = static_cast<SNLScalarNet*>(net);
          auto flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
          ImGui::TreeNodeEx((void*)net, flags, "%s", scalarNet->getName().getString().c_str());
        }
      }
      ImGui::TreePop();
    }
  }


  auto assignFilter = [](const SNLInstance* instance) { return not instance->getModel()->isAssign(); };
  auto primitives = design->getPrimitiveInstances().getSubCollection(assignFilter);
  auto nbPrimitives = primitives.size();
  if (nbPrimitives > 0) {
    if (ImGui::TreeNodeEx("Primitives")) {
      for (const auto& instance: primitives) {
        // Display the instance name
        std::string instanceName = instance->getName().getString();
        if (instance->isAnonymous()) {
            instanceName = std::to_string(instance->getID());
        }
        // Check if the instance has a model (another SNLDesign)
        const naja::SNL::SNLDesign* model = instance->getModel();
        if (model->isAnonymous()) {
          instanceName += " (" + std::to_string(model->getID()) + ")";
        } else {
          instanceName += " (" + model->getName().getString() + ")";
        }

        // Create a tree node for the instance
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        bool isExpanded = ImGui::TreeNodeEx((void*)instance, flags, "%s", instanceName.c_str());
        if (isExpanded) { 
          showInstanceHierarchy(model);
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  auto instances = design->getNonPrimitiveInstances();
  auto nbInstances = instances.size();
  if (nbInstances > 0) {
    if (ImGui::TreeNodeEx("Instances")) {
      for (const auto& instance: instances) {
        // Display the instance name
        std::string instanceName = instance->getName().getString();
        if (instance->isAnonymous()) {
            instanceName = std::to_string(instance->getID());
        }
        const naja::SNL::SNLDesign* model = instance->getModel();
        if (model->isAnonymous()) {
          instanceName += " (" + std::to_string(model->getID()) + ")";
        } else {
          instanceName += " (" + model->getName().getString() + ")";
        }

        // Create a tree node for the instance
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        bool isExpanded = ImGui::TreeNodeEx((void*)instance, flags, "%s", instanceName.c_str());
        if (isExpanded) { 
          showInstanceHierarchy(model);
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }
}

}

void DesignTreeView::render(const naja::SNL::SNLDesign* top) {
  if (!top) return;

  if (ImGui::TreeNodeEx(top->getName().getString().c_str())) {
    showInstanceHierarchy(top);
    ImGui::TreePop();
  }
}