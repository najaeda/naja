#include "SchematicView.h"

#include "Schematic.h"

#include "imgui.h"

void SchematicView::draw(const Schematic* schematic) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos(); // Top-left corner of the canvas
    ImVec2 canvasSize = ImGui::GetContentRegionAvail(); // Available size for the canvas

    // Draw canvas background
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(50, 50, 50, 255));

    // Draw nodes
    for (auto node: schematic->getNodes()) {
      const auto& position = node->getPosition();
      ImVec2 nodePos = ImVec2(canvasPos.x + position.x_, canvasPos.y + position.y_); // Adjust position to fit the canvas
      ImVec2 rectMin = nodePos;
      ImVec2 rectMax = ImVec2(nodePos.x + 150, nodePos.y + 50);

        // Draw the node rectangle
        drawList->AddRectFilled(rectMin, rectMax, IM_COL32(200, 200, 200, 255));
        drawList->AddRect(rectMin, rectMax, IM_COL32(0, 0, 0, 255)); // Border

        // Draw the node label
        ImVec2 textSize = ImGui::CalcTextSize(node->getName().c_str());
        ImVec2 textPos = ImVec2(rectMin.x + (150 - textSize.x) * 0.5f, rectMin.y + (50 - textSize.y) * 0.5f);
        drawList->AddText(textPos, IM_COL32(0, 0, 0, 255), node->getName().c_str());
    }
#if 0
    // Draw connections
    for (const Connection& connection : connections) {
        const Node& outputNode = nodes[connection.outputNodeId];
        const Node& inputNode = nodes[connection.inputNodeId];

        ImVec2 start = canvasPos + outputNode.position + ImVec2(150, 25); // Right-center of output node
        ImVec2 end = canvasPos + inputNode.position + ImVec2(0, 25);      // Left-center of input node

        // Draw a Bezier curve for the connection
        ImVec2 control1 = ImVec2(start.x + 50, start.y);
        ImVec2 control2 = ImVec2(end.x - 50, end.y);
        drawList->AddBezierCurve(start, control1, control2, end, IM_COL32(150, 150, 250, 255), 3.0f);
    }


  ImGui::Begin("Schematic View");

  // Setup canvas
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
  ImVec2 canvasSz = ImGui::GetContentRegionAvail();
  drawList->AddRectFilled(canvasP0, ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y), IM_COL32(50, 50, 50, 255));

    // Adjust pan and zoom
  ImVec2 offset = ImVec2(panOffset.x + canvasP0.x, panOffset.y + canvasP0.y);

  for (auto node: schematic->nodes_) {
    // Apply zoom and pan transformations
    ImVec2 nodePos = (node.position * zoomFactor) + offset;
    drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + NODE_WIDTH * zoomFactor, nodePos.y + NODE_HEIGHT * zoomFactor), IM_COL32(200, 200, 200, 255));
        
    for (const Pin& pin : node.inputs) {
      ImVec2 pinPos = (pin.position * zoomFactor) + nodePos;
      drawList->AddCircleFilled(pinPos, PIN_RADIUS * zoomFactor, IM_COL32(255, 100, 100, 255));
    }
    for (const Pin& pin : node.outputs) {
      ImVec2 pinPos = (pin.position * zoomFactor) + nodePos;
      drawList->AddCircleFilled(pinPos, PIN_RADIUS * zoomFactor, IM_COL32(100, 255, 100, 255));
    }
  }

  ImGui::End();
#endif
}