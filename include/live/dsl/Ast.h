#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lamp::live::dsl {

struct SpriteFrameDeclaration {
  std::string name;
  std::string bitmap;
};

struct PaletteEntry {
  char key;
  std::string colorExpression;
};

struct PaletteDeclaration {
  std::string name;
  std::vector<PaletteEntry> entries;
};

struct SpriteDeclaration {
  std::string name;
  std::string bitmap;
  std::string paletteName;
  std::vector<SpriteFrameDeclaration> frames;
};

struct TextDeclaration {
  std::string name;
  std::string content;
};

struct LayerDeclaration {
  std::string name;
  std::string spriteName;
  std::string colorExpression;
  std::string xExpression;
  std::string yExpression;
  std::string scaleExpression;
  std::string rotationExpression;
  std::string blendMode;
  std::string visibleExpression;
  std::string frameExpression;
  std::string zExpression;
  uint32_t colorLine = 0;
  uint32_t xLine = 0;
  uint32_t yLine = 0;
  uint32_t scaleLine = 0;
  uint32_t rotationLine = 0;
  uint32_t blendLine = 0;
  uint32_t visibleLine = 0;
  uint32_t frameLine = 0;
  uint32_t zLine = 0;
};

struct ForLoopStatement {
  std::string loopVariable;
  std::string startExpression;
  std::string endExpression;
  std::string stepExpression;
  std::string comparisonOperator;
  std::vector<LayerDeclaration> body;
};

struct ClockBlock {
  bool hasBlock = false;
  std::string enabledExpression;
  std::string zExpression;
  std::string blendMode;
  std::string alphaExpression;
  uint32_t enabledLine = 0;
  uint32_t zLine = 0;
  uint32_t blendLine = 0;
  uint32_t alphaLine = 0;
};

struct Program {
  std::string effectName;
  ClockBlock clockBlock;
  std::vector<PaletteDeclaration> palettes;
  std::vector<SpriteDeclaration> sprites;
  std::vector<TextDeclaration> texts;
  std::vector<LayerDeclaration> layers;
  std::vector<ForLoopStatement> forLoops;
};

}  // namespace lamp::live::dsl