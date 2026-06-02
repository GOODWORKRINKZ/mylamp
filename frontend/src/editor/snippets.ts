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
  {
    id: "nyan-cat",
    name: "Нян Кот",
    description: "Котик с радужным шлейфом бежит по кругу.",
    source: `effect "nyan_cat"

palette nyan_pal {
  G = rgb(180, 180, 180)
  P = rgb(255, 150, 180)
  K = rgb(40, 40, 40)
  W = rgb(255, 255, 255)
}

sprite cat palette nyan_pal {
  frame f0 { bitmap """
.....GG.....
....GGGG....
...GGGGGG...
..GGGGGGGG..
.GGGGGGGGGG.
GGKGGKGGKGGG
GGGGGGGGGGGG
.GGWWWWWWGG.
.GGGGGGGGGG.
..GGGGGGGG..
.....PP.....
.....PP.....
""" }
  frame f1 { bitmap """
.....GG.....
....GGGG....
...GGGGGG...
..GGGGGGGG..
.GGGGGGGGGG.
GGKGGKGGKGGG
GGGGGGGGGGGG
.GGGGGGGGGG.
.GGWWWWWWGG.
..GGGGGGGG..
.....PP.....
.....PP.....
""" }
  frame f2 { bitmap """
.....GG.....
....GGGG....
...GGGGGG...
..GGGGGGGG..
.GGGGGGGGGG.
GGKGGKGGKGGG
GGGGGGGGGGGG
..GGGGGGGG..
.GGGGGGGGGG.
.GGWWWWWWGG.
.....PP.....
.....PP.....
""" }
  frame f3 { bitmap """
.....GG.....
....GGGG....
...GGGGGG...
..GGGGGGGG..
.GGGGGGGGGG.
GGKGGKGGKGGG
GGGGGGGGGGGG
....GGGGGG..
..GGGGGGGG..
.GGWWWWWWGG.
.....PP.....
.....PP.....
""" }
}

layer nyan {
  use cat
  x = (t * 8) % 32
  y = 2
  frame = (t * 4) % 4
  visible = 1
}`,
  },
  {
    id: "mario",
    name: "Марио",
    description: "8-битный Марио шагает по цилиндру — прямо из SMB1!",
    source: `effect "mario"

palette mario_pal {
  R = rgb(170, 85, 0)
  S = rgb(255, 0, 0)
  B = rgb(255, 170, 0)
  W = rgb(85, 85, 0)
  K = rgb(170, 170, 0)
  G = rgb(255, 85, 0)
}

sprite mario palette mario_pal {
  frame f0 {
    bitmap """
    ...SSSSS....
    ..SSSSSSSSS.
    ..RRRBBRB...
    .RBRBBBKBBB.
    .RBRRBBBRBBB
    .RRBBBBKRRR.
    ...BBBBBBB..
    ..RRSRRR....
    .RRRSRRSRWW.
    RRRRSSSSRRRR
    BBRGBSSBGRBB
    BBBSSSSSSBBB
    BBGSSSSSSGBB
    ..SSS..SSS..
    .WRR....RRW.
    RRRW....WRRR
    """
  }
  frame f1 {
    bitmap """
    .....SSSSS.....
    ....SSSSSSSSS..
    ....RRRBBRB....
    ...RBRBBBKBBB..
    ...RBKRBBBKBBB.
    ...RRBBBBKRRR..
    .....BBBBBBB...
    ..RRRRGSRR.....
    BBRRRRSSSRRRBBB
    BBB.RRSBSSSRRBB
    BB..SSSSSSS..R.
    ...SSSSSSSSSRW.
    ..SSSSSSSSSSRW.
    .RRSSS...SSSRW.
    .RRR...........
    ..RRW..........
    """
  }
  frame f2 {
    bitmap """
    ..SSSSS....
    .SSSSSSSSS.
    .RRRBBRB...
    RBRBBBKBBB.
    RBRRBBBRBBB
    RRBBBBKRRR.
    ..BBBBBBB..
    .RRGRRR....
    RRRRSSRR...
    RRRSSBSSB..
    RRRRSSSSS..
    SRRBBBSSS..
    .SRBBGSS...
    ..SSSRRR...
    ..RRRRRRR..
    ..RRRR.....
    """
  }
}

layer mario1 {
  use mario
  x = (t * 5) % 32
  y = 0
  frame = (t * 5) % 3
  visible = 1
}`,
  },
  {
    id: "plasma",
    name: "Плазма",
    description: "Абстрактные переливы — sin, cos и немного магии.",
    source: `effect "plasma"

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

layer plasma1 {
  use fullscreen
  color hsv(sin(nx * 8 + t) * 30 + cos(ny * 6 + t * 0.7) * 30 + t * 20, 1, 0.5 + 0.5 * sin(nx * 5 + ny * 4 + t))
  x = 0
  y = 0
  scale = 1
  visible = 1
}`,
  },
  {
    id: "scrolling-text",
    name: "Бегущая строка",
    description: "Текст движется по окружности, как на стадионе.",
    source: `effect "scrolling_text"

text msg "MYLAMP "

layer scroller {
  use msg
  color rgb(255, 200, 50)
  x = (t * 6) % 32
  y = 5
  scale = 1
  visible = 1
}`,
  },
  {
    id: "snake",
    name: "Змейка",
    description: "Волна из точек — классика LED-анимации.",
    source: `effect "snake"

sprite dot {
  bitmap """
#
"""
}

for j = 0; j < 10; j = j + 1 {
  layer seg {
    use dot
    color hsv(j * 30, 1, 1)
    x = sin(t * 2 + j * 0.6) * 12 + 16
    y = cos(t * 2 + j * 0.6) * 6 + 8
    scale = 1 + (j % 2)
    visible = 1
  }
}`,
  },
  {
    id: "fire-particles",
    name: "Огоньки",
    description: "Частицы пламени поднимаются вверх.",
    source: `effect "fire_particles"

sprite dot {
  bitmap """
#
"""
}

for j = 0; j < 12; j = j + 1 {
  layer p {
    use dot
    color hsv(15 + j * 2, 1, 1 - j * 0.08)
    x = sin(j * 2.7 + t) * 3 + 8
    y = (t * 3 + j * 2) % 32
    scale = 1 + (j % 2)
    blend = add
    visible = 1
  }
}`,
  },
  {
    id: "starfield",
    name: "Звёздное поле",
    description: "Пролетающие звёзды — как в космосе.",
    source: `effect "starfield"

sprite dot {
  bitmap """
#
"""
}

for j = 0; j < 20; j = j + 1 {
  layer s {
    use dot
    color rgb(200 + 55 * sin(t + j), 200 + 55 * cos(t + j * 0.7), 255)
    x = (j * 7 + 3) % 16
    y = (t * (3 + j % 4) + j * 11) % 32
    scale = 1
    blend = add
    visible = 1
  }
}`,
  },
  {
    id: "dna",
    name: "Спираль ДНК",
    description: "Двойная спираль крутится на матрице.",
    source: `effect "dna"

sprite dot {
  bitmap """
#
"""
}

for j = 0; j < 16; j = j + 1 {
  layer h1 {
    use dot
    color rgb(80, 200, 255)
    x = 8 + sin(t * 2 + j * 0.4) * 6
    y = j * 2
    scale = 1
    visible = 1
  }
  layer h2 {
    use dot
    color rgb(255, 80, 120)
    x = 8 + sin(t * 2 + j * 0.4 + 3.1415) * 6
    y = j * 2
    scale = 1
    visible = 1
  }
}`,
  },
];