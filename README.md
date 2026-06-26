# Rocket

Rocket — это язык программирования и компилятор, разработанный для создания приложений с пользовательским интерфейсом. Язык компилируется в байткод и исполняется на собственной стековой виртуальной машине.

## Возможности

- **Декларативный UI** — компоненты описываются через вложенные структуры (`VBox`, `HBox`, `Button`, `TextField`), код отражает то, что отрисовывается
- **Реактивность** — поля, помеченные `trackable`, автоматически инициируют перерисовку интерфейса при изменении
- **Асинхронность** — `async func` и `await`, параллельное выполнение через `Async { ... }`
- **Управление памятью** — подсчёт ссылок (`shared_ptr`) с поддержкой слабых ссылок (`weak let`) для разрыва циклов
- **Опциональные типы** — `String?`, оператор `??` (nil coalescing), безопасные цепочки
- **Классы и структуры** — наследование через интерфейсы (`struct Foo : Bar`), `init`, `self`
- **Перечисления** — `enum` с `case`, сравнение через `==`
- **Лямбды и замыкания** — захват переменных по ссылке, трейлинг-синтаксис
- **Сеть** — встроенная библиотека `Net`: HTTP-запросы и парсинг JSON
- **Уровни доступа** — `public`, `private`

## Архитектура компилятора

```
Исходный код (.rocket)
       ↓
    Lexer          — токенизация
       ↓
  ASTBuilder       — рекурсивный спуск LL(1), построение AST
       ↓
    Compiler        — обход AST, генерация байткода
       ↓
     Chunk          — байткод + константный пул
       ↓
 VirtualMachine     — стековая VM, исполнение
```

## Синтаксис

### Переменные и типы
```swift
let name: String = "Rocket"
let count: Int = 42
let flag: Bool = true
let maybe: String? = null
```

### Классы
```swift
class TreeNode {
    let value: String
    weak let parent: TreeNode

    init(v: String) {
        self.value = v
    }
}

let root = TreeNode("root")
let child = TreeNode("child")
child.parent = root
print(child.parent.value)  // root
```

### Реактивность
```swift
class AppState {
    trackable let count: Int = 0
    trackable let message: String = ""
}

let state = AppState()
state.count = 1  // → перерисовка UI
```

### Асинхронность
```swift
async func fetchData() {
    let res = Http.get("https://api.example.com/data")
    if (res.isOk) {
        let parsed = Json.parse(res.value)
        print(parsed.title)
    }
}

await fetchData()

// Параллельно
let f1 = Async { doWork() }
let f2 = Async { doOtherWork() }
await f1
await f2
```

### Перечисления
```swift
enum TaskStatus {
    case todo
    case inProgress
    case done
}

let s = TaskStatus.todo
if (s == TaskStatus.todo) {
    print("not started")
}
```

### UI-компоненты
```swift
struct TaskManagerView : View {
    tracked let vm: TaskViewModel = TaskViewModel()

    let view: any View {
        return VBox(0.0, HAlign.start) {
            return [
                Text("Pipeline board", 21.0, Color.black),
                Button("Refresh", Color.blue) { self.vm.refresh() }
            ]
        }
    }
}
```

### Лямбды
```swift
let add = (a: Int, b: Int): Int {
    return a + b
}

let result = add(2, 3)
```

### Опциональные типы
```swift
let x: String? = null
let y: String = x ?? "default"
```

## Стандартные библиотеки

| Библиотека | Подключение | Возможности |
|---|---|---|
| UI | `import "libs/UI"` | `VBox`, `HBox`, `Button`, `TextField`, `Text`, `App` |
| Net | `import "libs/Net"` | `Http.get`, `Http.post`, `Json.parse` |

## Запуск

### Сборка
```bash
cmake -B build && cmake --build build
```

### Исполнение файла
```bash
./MyLanguage path/to/file.rocket
```

### Запуск приложения с UI
```bash
./MyLanguage taskManagerApp/main.rocket
```

## Пример приложения

В репозитории есть полноценное приложение — канбан-доска **Task Manager** (`taskManagerApp/`), построенное по архитектуре MVVM:

```
view/          — UI-компоненты
presentation/  — ViewModel, логика
model/         — модели данных
gateway/       — HTTP-запросы к API
```

## Тесты

```bash
ls test/
# array, async, class, features, func, if, lambda
# net, optional, reactive, struct, ui, weak, while ...
```

Каждая директория содержит `.rocket`-файлы с тестами конкретной возможности языка.

## Требования

- C++17
- CMake 3.15+
- OpenGL (для UI)
