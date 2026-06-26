# Лабораторная работа №2
## Функции, классы и объектно-ориентированное программирование в Rocket

---

### Цель работы

Изучить объявление функций, работу с классами и структурами, инкапсуляцию данных через модификаторы доступа, а также рекурсию на примерах в языке Rocket.

---

### Теоретическая часть

#### Функции

```swift
func имя(параметр: Тип): ТипВозврата {
    return значение
}
```

Функция без возвращаемого значения:
```swift
func greet(name: String): Void {
    print("Hello, " + name)
}
```

#### Классы

Класс объявляется ключевым словом `class`. Поля инициализируются через `init`. Внутри методов доступ к полям — через `self`.

```swift
class Robot {
    let name: String
    let id: Int

    init(name: String, id: Int) {
        self.name = name
        self.id = id
    }

    func identify(): Void {
        print("Я робот " + self.name)
    }
}
```

#### Модификаторы доступа

| Модификатор | Описание |
|-------------|----------|
| `public` | Доступно снаружи класса (по умолчанию) |
| `private` | Доступно только внутри класса |

```swift
class BankAccount {
    private let balance: Double

    init(balance: Double) {
        self.balance = balance
    }

    public func deposit(amount: Double): Void {
        self.balance = self.balance + amount
    }
}
```

---

### Задание 1. Функции

Создайте файл `functions.rocket`:

```swift
func add(a: Int, b: Int): Int {
    return a + b
}

func multiply(a: Int, b: Int): Int {
    return a * b
}

func greet(name: String): Void {
    print("Привет, " + name + "!")
}

greet("Ракета")

let sum = add(5, 3)
print("5 + 3 = " + sum)

let product = multiply(4, 7)
print("4 * 7 = " + product)
```

**Вопросы:**
- Что указывается после двоеточия в сигнатуре функции?
- Как вызвать функцию, не возвращающую значение?

---

### Задание 2. Рекурсия

Создайте файл `recursion.rocket`:

```swift
func fibonacci(n: Int): Int {
    if (n <= 1) {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

func factorial(n: Int): Int {
    if (n <= 1) {
        return 1
    }
    return n * factorial(n - 1)
}

print("Числа Фибоначчи:")
let i = 0
while (i < 10) {
    print(fibonacci(i))
    i = i + 1
}

print("Факториал 5 = " + factorial(5))   // 120
print("Факториал 10 = " + factorial(10)) // 3628800
```

---

### Задание 3. Первый класс

Создайте файл `robot.rocket`:

```swift
class Robot {
    let name: String
    let id: Int

    init(name: String, id: Int) {
        self.name = name
        self.id = id
    }

    func identify(): Void {
        print("Я робот " + self.name + ", ID: " + self.id)
    }

    func greet(other: Robot): Void {
        print(self.name + " приветствует " + other.name)
    }
}

let r1 = Robot("R2-D2", 101)
let r2 = Robot("C-3PO", 102)

r1.identify()
r2.identify()
r1.greet(r2)
```

---

### Задание 4. Инкапсуляция

Создайте файл `bank.rocket`:

```swift
class BankAccount {
    private let balance: Double
    private let owner: String

    init(owner: String, balance: Double) {
        self.owner = owner
        self.balance = balance
    }

    public func deposit(amount: Double): Void {
        self.balance = self.balance + amount
        print("Пополнение: " + amount)
        self.printBalance()
    }

    public func withdraw(amount: Double): Void {
        if (amount > self.balance) {
            print("Недостаточно средств")
            return
        }
        self.balance = self.balance - amount
        print("Снятие: " + amount)
        self.printBalance()
    }

    private func printBalance(): Void {
        print("Баланс " + self.owner + ": " + self.balance)
    }
}

let account = BankAccount("Иван", 1000.0)
account.deposit(500.0)
account.withdraw(200.0)
account.withdraw(9999.0)
```

Попробуйте обратиться к `account.balance` напрямую. Что произошло?

---

### Задание 5. Несколько объектов

Создайте файл `fleet.rocket` — реализуйте класс `Car` с полями `brand` (String), `speed` (Int) и методами `accelerate(by: Int)` и `info()`:

```swift
class Car {
    let brand: String
    let speed: Int

    init(brand: String) {
        self.brand = brand
        self.speed = 0
    }

    func accelerate(by: Int): Void {
        self.speed = self.speed + by
    }

    func info(): Void {
        print(self.brand + " едет со скоростью " + self.speed + " км/ч")
    }
}

let car1 = Car("Tesla")
let car2 = Car("BMW")

car1.accelerate(60)
car2.accelerate(90)

car1.info()
car2.info()

car1.accelerate(40)
car1.info()
```

---

### Задание 6. Самостоятельное задание

Реализуйте класс `Stack` — стек целых чисел:

- `push(value: Int)` — добавить элемент
- `pop()` — удалить и вернуть верхний элемент
- `peek()` — просмотреть верхний элемент без удаления
- `size()` — вернуть количество элементов
- поле `items: [Int]` — внутренний массив (приватный)

Напишите программу, которая:
1. Создаёт стек
2. Добавляет числа 10, 20, 30
3. Выводит верхний элемент (`peek`)
4. Удаляет элемент (`pop`)
5. Проверяет размер стека

---

### Контрольные вопросы

1. Как объявить функцию, принимающую два параметра и возвращающую значение?
2. Что такое `self` внутри метода класса?
3. Как работает модификатор `private`? Что произойдёт при обращении к приватному полю снаружи класса?
4. В чём разница между функцией и методом класса?
5. Что такое рекурсия? Какое условие обязательно должно быть в рекурсивной функции?

---

### Содержание отчёта

1. Цель работы
2. Листинги всех программ с комментариями
3. Скриншоты вывода для каждого задания
4. Листинг самостоятельного задания (класс `Stack`)
5. Ответы на контрольные вопросы
