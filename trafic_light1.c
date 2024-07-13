// Trafic_light
//
int RED_LIGHT = 5;              // Выход красный сигнал световора
int YELLOW_LIGHT = 6;           // Выход желтый сигнал световора
int GREEN_LIGHT = 7;            // Выход зеленый сигнал световора
int RED_CROSSWALK = 8;          // Выход красный сигнал пешехода
int GREEN_CROSSWALK = 9;        // Выход зеленый сигнал пешехода
int BUTTON = 2;                 // Вход кнопки
int TIME_ON_RED = 3000;         // Время включенного красного
int TIME_ON_YELL = 1000;        // Время включенного жултого
int TIME_ON_GREEN = 3000;       // Время включенного зеленого
int DELAY_FLASH = 500;          // Время задержки мигания зеленого
int DELAY_BETWEEN_SIGNAL = 200; // Задержка перед включением следующего сигнала
int DELAY_AFTER_BUTTON = 5000;  // Задержка после нажатия кнопки
int CYCLES_FLASH = 3;           // Количество циклов мигания зеленого

void setup()
{
    pinMode(RED_LIGHT, OUTPUT);
    pinMode(YELLOW_LIGHT, OUTPUT);
    pinMode(GREEN_LIGHT, OUTPUT);
    pinMode(RED_CROSSWALK, OUTPUT);
    pinMode(GREEN_CROSSWALK, OUTPUT);
    pinMode(BUTTON, INPUT);
    // сброс выходов после включения
    digitalWrite(RED_LIGHT, LOW);
    digitalWrite(YELLOW_LIGHT, LOW);
    digitalWrite(GREEN_LIGHT, LOW);
    digitalWrite(RED_CROSSWALK, LOW);
    digitalWrite(GREEN_CROSSWALK, LOW);
}

// Функция мигания
void flashLight(int color)
{
    for (int i = 0; i < CYCLES_FLASH; i++)
    {
        digitalWrite(color, LOW);
        delay(DELAY_FLASH);
        digitalWrite(color, HIGH);
        delay(DELAY_FLASH);
        digitalWrite(color, LOW);
    }
}

void loop()
{
    while (digitalRead(BUTTON)) // зелёный пока не нажата кнопка
    {
        digitalWrite(RED_CROSSWALK, HIGH);
        digitalWrite(GREEN_LIGHT, HIGH);
    }
    delay(DELAY_AFTER_BUTTON);
    flashLight(GREEN_LIGHT);
    digitalWrite(YELLOW_LIGHT, HIGH);
    delay(TIME_ON_YELL);
    digitalWrite(YELLOW_LIGHT, LOW);
    delay(DELAY_BETWEEN_SIGNAL);
    digitalWrite(RED_LIGHT, HIGH);       // включение красного для автомобилей
    digitalWrite(RED_CROSSWALK, LOW);    // выключение красного для пешехода
    digitalWrite(GREEN_CROSSWALK, HIGH); // включение зеленого для пешеходов
    delay(TIME_ON_RED);                  // время пешеходного перехода
    flashLight(GREEN_CROSSWALK);         // предупреждение о заканичивающемся переходе
    digitalWrite(RED_LIGHT, LOW);
}