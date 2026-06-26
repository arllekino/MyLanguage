# Лабораторная работа №3
## Декларативная вёрстка и UI-компоненты в Rocket

---

### Цель работы

Освоить создание графических приложений на языке Rocket с использованием декларативного подхода к описанию интерфейса: научиться компоновать элементы через `VBox`/`HBox`, применять модификаторы стиля, реагировать на действия пользователя.

---

### Теоретическая часть

#### Декларативный подход

В Rocket интерфейс описывается **декларативно** — вы указываете **что** должно отображаться, а не **как** это нарисовать. Структура кода отражает визуальную структуру экрана:

```swift
VBox(8.0, HAlign.center) {   // вертикальная колонка, выровнена по центру
    return [
        Text("Заголовок", 24.0, Color.white),
        Button("Нажми", Color.blue) { }
    ]
}
```

То, что вложено глубже — отображается внутри.

#### Запуск приложения

```swift
import "libs/UI"

let app = App()
app.run("Название окна", ширина, высота) {
    return КорневойВью()
}
```

#### Компоненты

| Компонент | Описание |
|-----------|----------|
| `Text(текст, размер, цвет)` | Текстовая метка |
| `Rect(ширина, высота, цвет)` | Цветной прямоугольник |
| `Button(текст, цвет) { }` | Кнопка с обработчиком нажатия |
| `TextField(id, значение, ширина) { }` | Поле ввода текста |
| `VBox(отступ, HAlign) { }` | Вертикальная колонка |
| `HBox(отступ, VAlign) { }` | Горизонтальная строка |
| `ScrollView(ш, в, scrollY, содержимое)` | Прокручиваемая область |

#### Выравнивание

```swift
VBox(10.0, HAlign.start)    // дочерние элементы — к левому краю
VBox(10.0, HAlign.center)   // по центру
VBox(10.0, HAlign.end)      // к правому краю

HBox(10.0, VAlign.top)      // к верхнему краю
HBox(10.0, VAlign.center)   // по центру
HBox(10.0, VAlign.bottom)   // к нижнему краю
```

#### Модификаторы стиля

Модификаторы применяются цепочкой к любому компоненту:

```swift
VBox(...) { ... }
    .padding(16.0)                    // внутренние отступы
    .background(Color.dark, 1.0)      // фон (цвет, прозрачность 0..1)
    .cornerRadius(10.0)               // скругление углов
```

#### Цвета

Встроенная палитра: `Color.white`, `Color.black`, `Color.red`, `Color.green`,
`Color.blue`, `Color.gray`, `Color.dark`, `Color.orange`, `Color.purple`, `Color.teal`

Произвольный цвет: `Color(r, g, b)` — значения от `0.0` до `1.0`

#### Реактивный View

Чтобы интерфейс обновлялся при изменении данных, нужны:

- `trackable let` в классе — поле, изменение которого вызывает перерисовку
- `tracked let` в структуре-вью — вью следит за этим объектом

```swift
class CounterViewModel {
    trackable let count: Int = 0

    func increment() {
        self.count = self.count + 1
    }
}

struct CounterView : View {
    tracked let vm: CounterViewModel = CounterViewModel()

    let view: any View {
        return VBox(16.0, HAlign.center) {
            return [
                Text(self.vm.count, 32.0, Color.white),
                Button("+", Color.blue) { self.vm.increment() }
            ]
        }
    }
}
```

---

### Задание 1. Первое окно

Создайте файл `ui_hello.rocket` — окно с текстом по центру:

```swift
import "libs/UI"

struct HelloView : View {
    let view: any View {
        return VBox(0.0, HAlign.center) {
            return [
                Text("Привет, Rocket UI!", 28.0, Color.white)
            ]
        }
    }
}

let app = App()
app.run("Первое окно", 800, 600) {
    return HelloView()
}
```

Запустите:
```bash
./build/MyLanguage ui_hello.rocket
```

Должно открыться окно с надписью на тёмном фоне.

---

### Задание 2. Компоновка VBox и HBox

Создайте файл `ui_layout.rocket` — несколько блоков, выстроенных в ряд:

```swift
import "libs/UI"

struct LayoutDemo : View {
    let view: any View {
        return VBox(20.0, HAlign.center) {
            return [
                Text("Строки (HBox)", 20.0, Color.gray),
                HBox(12.0, VAlign.center) {
                    return [
                        Rect(80.0, 80.0, Color.red),
                        Rect(80.0, 80.0, Color.green),
                        Rect(80.0, 80.0, Color.blue)
                    ]
                },

                Text("Колонки (VBox)", 20.0, Color.gray),
                HBox(12.0, VAlign.top) {
                    return [
                        VBox(8.0, HAlign.center) {
                            return [
                                Rect(60.0, 40.0, Color.orange),
                                Rect(60.0, 40.0, Color.orange),
                                Rect(60.0, 40.0, Color.orange)
                            ]
                        },
                        VBox(8.0, HAlign.center) {
                            return [
                                Rect(60.0, 60.0, Color.purple),
                                Rect(60.0, 60.0, Color.purple)
                            ]
                        },
                        VBox(8.0, HAlign.center) {
                            return [
                                Rect(60.0, 100.0, Color.teal)
                            ]
                        }
                    ]
                }
            ]
        }
    }
}

let app = App()
app.run("Layout Demo", 800, 600) {
    return LayoutDemo()
}
```

**Вопросы:**
- Как изменить ориентацию — поставить `VBox` внутрь `VBox`?
- Что изменится, если поменять `VAlign.top` на `VAlign.center`?

---

### Задание 3. Модификаторы стиля — карточка

Создайте файл `ui_card.rocket` — карточка с отступами, фоном и скруглёнными углами:

```swift
import "libs/UI"

struct CardView : View {
    let view: any View {
        return VBox(20.0, HAlign.center) {
            return [
                Text("Карточки", 22.0, Color.gray),
                HBox(16.0, VAlign.top) {
                    return [
                        VBox(8.0, HAlign.start) {
                            return [
                                Text("Задача 1", 18.0, Color.white),
                                Text("Исправить баг", 13.0, Color.gray),
                                Button("Готово", Color.green) { }
                            ]
                        }
                        .padding(16.0)
                        .background(Color.dark, 1.0)
                        .cornerRadius(10.0),

                        VBox(8.0, HAlign.start) {
                            return [
                                Text("Задача 2", 18.0, Color.white),
                                Text("Написать тесты", 13.0, Color.gray),
                                Button("В работу", Color.orange) { }
                            ]
                        }
                        .padding(16.0)
                        .background(Color.dark, 1.0)
                        .cornerRadius(10.0),

                        VBox(8.0, HAlign.start) {
                            return [
                                Text("Задача 3", 18.0, Color.white),
                                Text("Ревью кода", 13.0, Color.gray),
                                Button("Удалить", Color.red) { }
                            ]
                        }
                        .padding(16.0)
                        .background(Color.dark, 1.0)
                        .cornerRadius(10.0)
                    ]
                }
            ]
        }
    }
}

let app = App()
app.run("Cards", 900, 500) {
    return CardView()
}
```

Попробуйте изменить:
- `cornerRadius` с `10.0` до `0.0` и `30.0` — что происходит с углами?
- `padding` с `16.0` до `4.0` и `32.0` — как меняются отступы?
- цвет фона через `Color(r, g, b)` вместо `Color.dark`

---

### Задание 4. Интерактивность — счётчик

Создайте файл `ui_counter.rocket` — счётчик с кнопками `+` и `−`:

```swift
import "libs/UI"

class CounterVM {
    trackable let count: Int = 0

    func increment() {
        self.count = self.count + 1
    }

    func decrement() {
        if (self.count > 0) {
            self.count = self.count - 1
        }
    }

    func reset() {
        self.count = 0
    }
}

struct CounterView : View {
    tracked let vm: CounterVM = CounterVM()

    let view: any View {
        let color = Color.white
        if (self.vm.count > 10) {
            color = Color.orange
        }
        if (self.vm.count > 20) {
            color = Color.red
        }

        return VBox(20.0, HAlign.center) {
            return [
                Text("Счётчик", 18.0, Color.gray),
                Text(self.vm.count, 56.0, color),
                HBox(12.0, VAlign.center) {
                    return [
                        Button("-", Color.red)   { self.vm.decrement() },
                        Button("0", Color.gray)  { self.vm.reset() },
                        Button("+", Color.green) { self.vm.increment() }
                    ]
                }
            ]
        }
    }
}

let app = App()
app.run("Counter", 800, 500) {
    return CounterView()
}
```

**Обратите внимание:** `count` помечен `trackable` — при каждом изменении UI перерисовывается автоматически. Цвет числа меняется в зависимости от значения.

---

### Задание 5. Поле ввода

Создайте файл `ui_form.rocket` — форма с текстовым полем и кнопкой отправки:

```swift
import "libs/UI"

class FormVM {
    trackable let inputText: String = ""
    trackable let submitted: String = ""

    func onInput(text: String) {
        self.inputText = text
    }

    func submit() {
        if (self.inputText != "") {
            self.submitted = "Отправлено: " + self.inputText
            self.inputText = ""
        }
    }
}

struct FormView : View {
    tracked let vm: FormVM = FormVM()

    let view: any View {
        return VBox(16.0, HAlign.center) {
            return [
                Text("Форма", 22.0, Color.white),
                TextField("name_field", self.vm.inputText, 300.0) {
                    self.vm.onInput(it)
                },
                Button("Отправить", Color.blue) {
                    self.vm.submit()
                },
                Text(self.vm.submitted, 16.0, Color.green)
            ]
        }
    }
}

let app = App()
app.run("Form", 800, 500) {
    return FormView()
}
```

**Обратите внимание:** колбэк `TextField` принимает текущее значение поля через переменную `it`.

---

### Задание 6. Самостоятельное задание — профильная карточка

Реализуйте приложение `ui_profile.rocket` — карточку пользователя с редактированием имени.

Требования:
- Карточка с именем, должностью и аватаром (цветной круг — `Rect`)
- Кнопка «Редактировать» переключает режим: вместо текста появляется `TextField`
- Кнопка «Сохранить» применяет изменения и возвращает обычный вид
- Имя в карточке обновляется сразу после сохранения
- Карточка должна иметь фон, `padding` и `cornerRadius`

Минимальная структура:

```swift
class ProfileVM {
    trackable let name: String = "Иван Иванов"
    trackable let editing: Bool = false
    trackable let draft: String = ""

    func startEdit() { ... }
    func saveName() { ... }
    func onInput(text: String) { ... }
}
```

---

### Контрольные вопросы

1. Чем декларативный подход к вёрстке отличается от императивного?
2. Как `VBox` и `HBox` определяют направление расположения элементов?
3. Что означает `trackable let` у поля класса? Что происходит при его изменении?
4. Зачем нужно ключевое слово `tracked` у поля во View-структуре?
5. Какой синтаксис используется для обработки нажатия на кнопку?
6. Как передать текущее значение из `TextField` в ViewModel?
7. В каком порядке применяются модификаторы `.padding()`, `.background()`, `.cornerRadius()`? Имеет ли порядок значение?

---

### Содержание отчёта

1. Цель работы
2. Листинги программ заданий 1–5 с комментариями
3. Скриншоты запущенных окон для каждого задания
4. Листинг самостоятельного задания (задание 6)
5. Ответы на контрольные вопросы
