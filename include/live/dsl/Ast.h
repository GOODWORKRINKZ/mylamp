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
  std::string visibleExpression;
};

struct Program {
  std::string effectName;
  std::vector<SpriteDeclaration> sprites;
  std::vector<LayerDeclaration> layers;
};

}  // namespace lamp::live::dsl