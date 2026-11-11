#pragma once

#include "kitezh/vessel.hpp"

#include <stdexcept>
#include <vector>

namespace kitezh {

enum class Level {
    Low,
    High,
};

enum class Gate {
    Lower,
    Upper,
};

enum class GateState {
    Closed,
    Open,
};

class LockError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Камера шлюза. Снаружи нельзя нарушить правила присваиванием в поле:
// всё, что меняет смену, проходит через методы.
// Публичные сигнатуры не меняйте. Поля и приватные методы — можно.
class Lock {
public:
    Lock();

    [[nodiscard]] Level level() const;
    [[nodiscard]] GateState gate(Gate gate) const;
    [[nodiscard]] int pumpCycles() const;

    // Копия. Индекс 0 — судно, которое вошло раньше всех и выйдет первым.
    [[nodiscard]] std::vector<Vessel> inside() const;

    // Повторное открытие уже открытого затвора ничего не делает.
    // Повторное закрытие тоже. Любое недопустимое открытие бросает LockError
    // и оставляет камеру как была.
    void open(Gate gate);
    void close(Gate gate);

    // Переключает уровень. Оба затвора должны быть закрыты.
    // Если кому-то внутри не хватит глубины на новом уровне — LockError,
    // уровень и счётчик тактов не меняются.
    void pump();

    // Кладёт внутрь свою копию судна. Исходный Vessel после вызова можно менять.
    void enter(const Vessel& vessel);

    // Выпускает судно с позывным call_sign. Это должна быть голова очереди внутри.
    void exit(const std::string& call_sign);

private:
    Level level_ = Level::Low;
    GateState lower_gate_ = GateState::Closed;
    GateState upper_gate_ = GateState::Closed;
    int pump_cycles_ = 0;
    std::vector<Vessel> inside_;
    // Поля и приватные методы добавляйте здесь.
    // Публичные сигнатуры выше менять нельзя: тесты собираются против них.
};

}  // namespace kitezh
