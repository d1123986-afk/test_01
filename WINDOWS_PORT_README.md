# Адаптация SokolClient для Windows

## Обзор изменений

Данный проект был адаптирован для работы в операционной системе Windows. 
Основное изменение - замена Linux-специфичного драйвера `vhci_hcd` на его аналог для Windows.

## Изменённые файлы

### 1. qt_sokol_client_network.h
- Добавлена условная компиляция для Windows (`#ifdef _WIN32`)
- Заменены заголовочные файлы POSIX на Windows API:
  - `<sys/socket.h>` → `<winsock2.h>`
  - `<arpa/inet.h>` → `<ws2tcpip.h>`
  - `<netdb.h>` → `<ws2tcpip.h>`
  - `<unistd.h>` → удалён для Windows
- Добавлена библиотека `ws2_32.lib` для работы с сетью
- Макрос `close()` заменён на `closesocket()` для Windows

### 2. qt_sokol_client_network.cpp
- Добавлена инициализация WinSock через `WSAStartup()` в конструкторе соединения
- Добавлен вызов `WSACleanup()` в деструкторе
- Условная компиляция для `close()`/`closesocket()`

### 3. qt_sokol_client_driver.h
- Добавлены заголовочные файлы Windows:
  - `<windows.h>`
  - `<setupapi.h>`
  - `<cfgmgr32.h>`
- Определены константы для Windows USBIP VHCI драйвера:
  - `USBIP_VHCI_IOCTL_ATTACH`
  - `USBIP_VHCI_IOCTL_DETACH`
- Определены константы скоростей USB и статусов устройств для Windows
- Структура `sokol_vhci_driver` изменена для поддержки HANDLE в Windows

### 4. qt_sokol_client_driver_win_stub.cpp (новый файл)
- Реализация функций работы с VHCI драйвером для Windows
- Использование Windows API:
  - `CreateFile()` для открытия драйвера
  - `DeviceIoControl()` для управления устройствами
  - `CloseHandle()` для закрытия handle

### 5. SokolClient.pro
- Добавлена секция `win32` с настройками для Windows:
  - `DEFINES += _WIN32_WINNT=0x0601` (минимум Windows 7)
  - `LIBS += -lws2_32 -lsetupapi`
- Добавлен исходный файл `qt_sokol_client_driver_win_stub.cpp` только для Windows

## Необходимые компоненты для Windows

### 1. USBIP VHCI Driver для Windows
Для работы приложения необходимо установить драйвер USBIP VHCI для Windows:
- Репозиторий: https://github.com/cezanne/usbip-win
- Следуйте инструкциям по установке драйвера

### 2. Qt для Windows
- Установите Qt Framework для Windows
- Рекомендуется версия Qt 5.15 или выше

### 3. Компилятор
- MinGW-w64 или MSVC
- Убедитесь, что пути к компилятору добавлены в PATH

## Сборка проекта

### Для Windows (MinGW):
```bash
cd C:\path\to\SokolClient
qmake SokolClient.pro
mingw32-make
```

### Для Windows (MSVC):
```bash
cd C:\path\to\SokolClient
qmake SokolClient.pro
nmake
```

### Для Linux (без изменений):
```bash
cd /path/to/SokolClient
qmake SokolClient.pro
make
```

## Отличия реализации

### Linux
- Использует sysfs (`/sys/devices/platform/vhci_hcd`)
- Работа через udev для управления устройствами
- Драйвер ядра `vhci_hcd`

### Windows
- Использует Windows DeviceIoControl API
- Прямое взаимодействие с драйвером через HANDLE
- Драйвер usbip-win (сторонний)

## Известные ограничения

1. **Требуются права администратора** для установки и использования USBIP VHCI драйвера
2. **Совместимость драйверов**: Убедитесь, что версия драйвера usbip-win совместима с вашей версией Windows
3. **Производительность**: Производительность может отличаться от Linux-версии из-за различий в архитектуре драйверов

## Тестирование

После сборки проверьте:
1. Подключение к серверу Sokol
2. Отображение списка экспортированных устройств
3. Прикрепление (attach) USB-устройства
4. Открепление (detach) USB-устройства

## Поддержка

При возникновении проблем:
1. Проверьте, установлен ли драйвер usbip-win
2. Убедитесь, что служба USBIP запущена
3. Проверьте логи приложения через Qt Creator Console
