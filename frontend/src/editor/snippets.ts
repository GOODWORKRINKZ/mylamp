export type StarterSnippet = {
  id: string;
  name: string;
  description: string;
  source: string;
};

export const starterSnippets: StarterSnippet[] = [
  {
    id: "rainbow",
    name: "Радуга",
    description: "Плавный перелив по ширине лампы.",
    source: `effect "rainbow"

sprite dot {
  bitmap """
  #
  """
}

layer paint {
  use dot
  color hsv(nx * 360 + t * 70, 1, 1)
  x = x
  y = y
  scale = 1
  visible = 1
}`,
  },
  {
    id: "fire",
    name: "Огонь",
    description: "Тёплый пульс с движением вверх.",
    source: `effect "fire"

sprite flame {
  bitmap """
  .#.
  ###
  .#.
  """
}

layer core {
  use flame
  color rgb(255, 120 + 80 * sin(t * 4), 20)
  x = 12 + sin(t * 3)
  y = 10 - abs(sin(t * 2))
  scale = 2
  visible = 1
}`,
  },
  {
    id: "upside-down",
    name: "Изнанка",
    description: "Тёмная органика, багровые всполохи — как в Очень Странных Делах.",
    source: `effect "upside-down"

sprite fullscreen {
  bitmap """
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  """
}

layer upside {
  use fullscreen
  color hsv(15 + sin(nx * 8.0 + t * 0.5) * 4 + sin(ny * 10.0 + t * 0.4) * 5, 0.9, 0.03 + 0.97 * clamp(sin(nx * 15.7 + t * 0.7) * sin(ny * 12.6 + t * 0.6) * sin((nx + ny) * 9.4 + t * 0.55) * sin((nx - ny) * 7.9 + t * 0.45) * 0.6 + 0.5, 0, 1))
  x = 0
  y = 0
  visible = 1
}`,
  },
  {
    id: "heart",
    name: "Летающее сердечко",
    description: "Сердце плывёт и слегка дышит.",
    source: `effect "heart"

sprite heart {
  bitmap """
  .##.##.
  #######
  #######
  .#####.
  ..###..
  ...#...
  """
}

layer love {
  use heart
  color rgb(255, 30 + 20 * sin(t * 5), 80)
  x = 10 + sin(t) * 4
  y = 4 + cos(t * 1.2) * 2
  scale = 1 + abs(sin(t * 2))
  visible = 1
}`,
  },
  {
    id: "lightning",
    name: "Молния",
    description: "Короткие вспышки с холодным оттенком.",
    source: `effect "lightning"

sprite bolt {
  bitmap """
  .#.
  ##.
  .##
  .#.
  """
}

layer flash {
  use bolt
  color rgb(180 + 75 * abs(sin(t * 12)), 220, 255)
  x = 14
  y = 3
  scale = 2
  visible = max(0, sin(t * 12))
}`,
  },
  {
    id: "clock",
    name: "Часы",
    description: "Базовая заготовка под time-driven overlay.",
    source: `effect "clock"

sprite dot {
  bitmap """
  #
  """
}

layer indicator {
  use dot
  color rgb(80, 180, 255)
  x = 15 + sin(t) * 8
  y = 7 + cos(t) * 6
  scale = 2
  visible = 1
}`,
  },
];