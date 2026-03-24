#pragma once

#include <string>
#include <vector>

namespace lamp::live::dsl {

struct SpriteDeclaration {
  std::string name;
  std::string bitmap;
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
  uint32_t colorLine = 0;
  uint32_t xLine = 0;
  uint32_t yLine = 0;
  uint32_t scaleLine = 0;
  uint32_t rotationLine = 0;
  uint32_t blendLine = 0;
  uint32_t visibleLine = 0;
};

struct Program {
  std::string effectName;
  std::vector<SpriteDeclaration> sprites;
  std::vector<LayerDeclaration> layers;
};

}  // namespace lamp::live::dsl