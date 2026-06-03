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
    description: "Две синусоиды в противофазе крутятся навстречу.",
    source: `effect "double_rainbow"

palette rainbow_pal {
  K = rgb(255, 0, 0)
  O = rgb(255, 127, 0)
  G = rgb(255, 255, 0)
  Z = rgb(0, 255, 0)
  g = rgb(0, 255, 255)
  S = rgb(0, 0, 255)
  F = rgb(127, 0, 255)
}

sprite fwd_strip palette rainbow_pal {
  bitmap """
  K
  O
  G
  Z
  g
  S
  F
  """
}

sprite rev_strip palette rainbow_pal {
  bitmap """
  F
  S
  g
  Z
  G
  O
  K
  """
}

for k = 0; k < 32; k = k + 1 {
  layer wave_up {
    use fwd_strip
    color rgb(255, 255, 255)
    x = k
    y = 4 + 4 * sin(k * 0.19635 + t * 3)
    visible = 1
    z = 1
  }
  layer wave_down {
    use rev_strip
    color rgb(255, 255, 255)
    x = k
    y = 4 - 4 * sin(k * 0.19635 - t * 3)
    visible = 1
    z = 1
  }
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
    id: "mario",
    name: "Марио",
    description: "8-битный Марио шагает по цилиндру.",
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
];
