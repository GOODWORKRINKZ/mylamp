export type HelpItem = {
  term: string;
  description: string;
};

export type HelpSection = {
  title: string;
  items: HelpItem[];
};

export const editorHelpSections: HelpSection[] = [
  {
    title: "Переменные",
    items: [
      { term: "t", description: "Время в секундах с момента запуска эффекта." },
      { term: "dt", description: "Дельта времени между кадрами." },
      { term: "x / y", description: "Текущая координата пикселя на полотне." },
      { term: "nx / ny", description: "Нормализованные координаты от 0 до 1." },
    ],
  },
  {
    title: "Функции",
    items: [
      { term: "sin(value)", description: "Плавное колебание для движения и пульса." },
      { term: "cos(value)", description: "Сдвинутая синусоида для круговых траекторий." },
      { term: "abs(value)", description: "Убирает минус, удобно для вспышек и bounce-анимации." },
      { term: "min/max", description: "Ограничивают выражение сверху или снизу." },
      { term: "clamp(v, a, b)", description: "Жёстко держит значение в диапазоне." },
      { term: "temp() / humidity()", description: "Дают показания датчика температуры и влажности." },
    ],
  },
  {
    title: "Директивы",
    items: [
      { term: "effect \"name\"", description: "Имя текущего Lux-эффекта." },
      { term: "sprite ... bitmap", description: "Форма, которой потом пользуются слои." },
      { term: "text name \"...\"", description: "Текстовый спрайт из строки (лат. + кирилица)." },
      { term: "layer ... use", description: "Слой выбирает sprite и задаёт анимацию." },
      { term: "color rgb(...) / hsv(...)", description: "Цвет слоя через RGB или HSV." },
      { term: "rotation / blend", description: "Вращение и режим наложения (normal, add, multiply, screen)." },
      { term: "visible", description: "0 скрывает слой, 1 показывает, дробные значения пригодятся позже." },
    ],
  },
];