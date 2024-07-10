// Manage LED from terminal

int GREEN_LIGHT = 5; // Выход зеленый сигнал световора
int incomingByte = 0;

void setup()
{
    pinMode(GREEN_LIGHT, OUTPUT);   // объявляем как выход
    digitalWrite(GREEN_LIGHT, LOW); // сброс выходов после включения
    Serial.begin(9600);             // скорость связи
}

void loop()
{
    if (Serial.available() > 0)
        incomingByte = Serial.parseInt(); // читаем байт из буфера и преобразуем int

    if (incomingByte == 1)
        digitalWrite(GREEN_LIGHT, HIGH);
    if (incomingByte == 0)
        digitalWrite(GREEN_LIGHT, LOW);
}