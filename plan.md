# План и заметки: РСОИ, ЛР 1–5 на C++23

## Суть лабораторных

| ЛР | Что проверяют на самом деле | Нагрузка на код |
|----|-----------------------------|-----------------|
| 1 | CI/CD, Docker, деплой на Heroku | один CRUD-сервис, 5 эндпоинтов |
| 2 | микросервисы, docker-compose, Gateway | 3 CRUD-сервиса + оркестрация в Gateway, `/manage/health` |
| 3 | отказоустойчивость | Circuit Breaker, fallback, откат/компенсация, очередь повторов — самая «кодовая» ЛР |
| 4 | k8s, helm, ingress, registry | кода почти нет, конфиг через env |
| 5 | OAuth2 / OIDC, JWT + JWKS | фильтр проверки токена, проброс токена между сервисами |

Проверка везде — Postman/newman через HTTP. Внутреннюю архитектуру никто не смотрит.

## Решения

### Стек
- **C++23**, clang-20 + **libstdc++** + lld, clang-tidy, clang-format (всё из штатных репо Ubuntu 24.04).
- **userver v3.2** (Яндекс) — выбран вместо Drogon: stackful-корутины (код синхронный на вид, без `co_await`),
  HTTP-сервер + HTTP-клиент с таймаутами/ретраями/deadline propagation (Gateway, ЛР2–3), свой async-драйвер Postgres,
  YAML-конфиг с подстановкой env (`port#env: PORT`), встроенный ping-хендлер, логи/метрики/трейсинг,
  `UTEST` (gtest+gmock с корутинным окружением) и testsuite (pytest) для функциональных тестов.
- **Получение userver**: CPM (`CPMAddPackage`) из архива тега `v3.2` с проверкой SHA256, собирается как подпроект (`SYSTEM`).
  Системные зависимости — apt-пакеты из `deps/ubuntu-24.04.txt` (урезанный официальный список userver: без gRPC/Mongo/Kafka/...).
  Кодогенератору chaotic при configure нужен PyPI (ставит Jinja2/PyYAML/pydantic в venv).
- **libc++ → libstdc++**: apt-зависимости userver собраны под libstdc++, пересобирать всё под libc++ — не стоит того.
- **vcpkg убран**: userver в нём нет, остальные зависимости — из apt.
- **jwt-cpp** или `userver::crypto` — в ЛР5 (JWKS, RS256).
- Circuit Breaker и очередь повторов (ЛР3) — пишем сами; очередь in-memory (задание разрешает).
- Отказались: Drogon (был первым выбором, заменён на userver), Crow (нет HTTP-клиента), Boost.Beast (слишком низкоуровневый),
  oat++ (медленно развивается), cpprestsdk (заброшен).

### Сборка
- Собираем в **WSL2** (Ubuntu 24.04): тот же тулчейн, что в GitHub Actions и в Docker.
- Предупреждения — ошибки, clang-tidy `WarningsAsErrors: '*'` — **только для наших таргетов** (`rsoi_target_defaults()`),
  исходники userver не линтим. Что проходит локально, то проходит и в CI.
- Стандарты кода (для наших таргетов):
  - предупреждения `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Wold-style-cast -Wimplicit-fallthrough`;
  - clang-format (`.clang-format`), проверка в CI;
  - clang-tidy (`.clang-tidy`) с правилами именования как в userver: типы и функции `CamelCase`, переменные `lower_case`,
    приватные поля `member_`, константы/enum-значения `kCamelCase`, макросы `UPPER_CASE`;
  - ASan + UBSan в пресете `debug` (`USERVER_SANITIZE="addr;ub"`, флаги приходят через интерфейс userver) — в CI тоже;
  - unit-тесты (`UTEST`).
  - отключено в clang-tidy: `cppcoreguidelines-avoid-const-or-ref-data-members` (ссылки-члены для DI — норма, как в userver);
  - Не вводим (решили): pre-commit, .editorconfig, actionlint/hadolint/shellcheck, Conventional Commits, покрытие, IWYU.
- lld подхватывается userver автоматически (`userver_setup_environment()` ищет `lld-<версия clang>`), ccache — тоже.
- Первая сборка долгая (userver из исходников); CPM-кэш исходников — `~/.cache/CPM`, объектники — ccache.
- Проект лежит на диске Windows (`/mnt/d/...`); если сборка из WSL будет тормозить — перенести клон в файловую систему WSL (`~/...`).
- `.gitattributes` принудительно ставит LF (у `core.autocrlf=true` иначе в WSL приедут CRLF).

### Архитектура
Чистая архитектура в полном виде — оверкилл. Берём три слоя с одним правилом зависимостей: `api` и `storage` → `domain`, обратно нельзя.
- **Repository** за интерфейсом — ради unit-тестов (ЛР1) без БД.
- **API Gateway / API Composition** (ЛР2).
- **Circuit Breaker**, **Retry Queue**, компенсирующие действия (ЛР3) — клиенты сервисов за интерфейсом, CB как декоратор.
- **Middleware/Filter** для JWT (ЛР5).
- **Конфиг через env** (12-factor): `PORT`, `DATABASE_URL` — нужно для Heroku (ЛР1) и k8s (ЛР4).
- Корутины userver stackful → интерфейсы домена — обычные синхронные сигнатуры (`std::optional<Person> Find(int id)`), без зависимости от фреймворка.

### Структура
```
CMakeLists.txt, CMakePresets.json   # userver через CPM, компилятор clang-20
cmake/get_cpm.cmake
deps/ubuntu-24.04.txt               # apt-зависимости: одни и те же для WSL, CI и Docker
.clang-format, .clang-tidy
common/                      # появится в ЛР2: health, config, http-клиенты, circuit breaker, jwt filter
person-service/
  src/
    main.cpp
    domain/                  # сущности и логика, без HTTP и SQL → статическая библиотека person_domain
    api/                     # HTTP-ручки userver (person_handlers), JSON и валидация (person_json)
    storage/                 # Postgres-репозиторий, schema.sql
  configs/static_config.yaml # конфиг компонентов userver
  tests/
```
**Не называть папки** `include/`, `lib/`, `bin/`, `build/`, `scripts/` — их игнорирует `.gitignore` шаблона.

## Требования ЛР1 (из репозитория)
- Пути с префиксом **`/api/v1`**: `/api/v1/persons`, `/api/v1/persons/{id}`; `id` — int32.
- `PersonRequest`: обязателен только `name`; `age`, `address`, `work` — опциональны.
- `POST` → **201**, пустое тело, `Location: /api/v1/persons/{id}` (тест берёт последний сегмент как id).
- `GET` → 200 и `Content-Type: application/json`.
- **`PATCH` частичный**: тест шлёт только `name` и `address`, `work` и `age` должны сохраниться.
- `DELETE` → **204**.
- 404 → `{ "message": ... }`; 400 → `{ "message": ..., "errors": { поле: текст } }`.
- Локальный Postgres: `docker compose up -d` → БД `persons`, пользователь `program:test`.
- Решения по API: в PATCH все поля опциональны (частичное обновление, `name` не обязателен, но не пустой);
  `null` = поле не передано; нечисловой `{id}` → 400; неизвестные поля игнорируются.
- Heroku отдаёт `DATABASE_URL` как `postgres://...` — libpq понимает URI; добавить `sslmode=require`.

## План ЛР1
1. [x] **Окружение WSL2**: clang-20, lld, clang-tidy, clang-format + пакеты из `deps/ubuntu-24.04.txt`.
2. [x] **Скелет сборки**: presets, userver через CPM, clang-format/tidy, заглушки, `UTEST`-smoke, `/manage/health` (ping-хендлер userver) с логом.
   Проверка: `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`; сервис стартует, пишет лог, `curl /manage/health` → 200.
3. [x] **GitHub Actions** (`ubuntu-24.04`): apt из `deps/` → проверка форматирования → кэш CPM+ccache → сборка с clang-tidy → ctest.
4. [x] **HTTP-ручки на заглушках**: `api/person_handlers` (`handler-persons`: GET/POST, `handler-person`: GET/PATCH/DELETE),
   `api/person_json` (сериализация, валидация → 400). Заглушка: существует только `id = 1`. Проверено curl'ом.
5. [x] **Домен и сервис** (TDD): `PersonRepository` (интерфейс), `PersonService` (частичный PATCH через `ApplyPatch`),
   7 unit-тестов на GMock-моке: create, get (+nullopt), list, partial patch, patch не найден, patch удалённого, delete.
   Валидация формата — в `api`, домен получает корректные данные. Домен не зависит от userver.
6. [ ] **Postgres-репозиторий**; ручки переключаются с заглушек на сервис; таблица через `CREATE TABLE IF NOT EXISTS` при старте.
   Проверка: локально newman с `[inst][local]` окружением.
7. [ ] **Dockerfile**: multi-stage (builder: ubuntu:24.04 + `deps/` → runtime: ubuntu:24.04 + runtime-библиотеки), слушает `$PORT`.
   Проверка: `docker build` + `docker run` + newman.
8. [ ] **Деплой на Heroku из Actions без CLI**: `docker login registry.heroku.com` (API key) → `docker push registry.heroku.com/<app>/web`
   → релиз через Platform API (`PATCH /apps/<app>/formation`, curl). Секреты: `HEROKU_API_KEY`, `HEROKU_APP_NAME`.
9. [ ] Прописать `baseUrl` в `postman/[inst][heroku] Lab1.postman_environment.json`, PR `feat/initial` → `master`.

## Открытые вопросы
- Heroku: бесплатного плана нет → нужен аккаунт с Eco/Basic dyno и Essential Postgres.
- До деплоя шаг newman в CI будет падать — это ожидаемо.

## Локальная работа
```bash
sudo apt-get install -y $(cat deps/ubuntu-24.04.txt)   # один раз
cmake --preset debug                      # первый раз долго: CPM качает userver, venv для chaotic
cmake --build --preset debug              # первый раз долго: собирается userver
ctest --preset debug
cmake --build --preset debug --target format        # отформатировать
cmake --build --preset debug --target format-check  # проверить формат
./build/debug/person-service/person-service --config person-service/configs/static_config.yaml
curl -i localhost:8080/manage/health
```
CLion: Toolchain → WSL (Ubuntu); CMake → включить пресеты `debug`/`release`.
