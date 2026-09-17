# ESP-IDF Ver 5.x Basic Example  
### Install PlatformIO with VSCode. 
```sh
https://youtu.be/tDh9iNSV2l0

```  
### ESP-IDF Extension for VS Code.  
```sh
https://github.com/espressif/vscode-esp-idf-extension
```

### idf.py basic command.  
| COMMAND                     | DESCRIPTION                     |
|-----------------------------|---------------------------------|
| `Ctrl+E release, then press T`   | Open ESP-IDF Terminal               |
| `Ctrl+E release, then press B`   | Quick build project                |
| `Ctrl+E release, then press F`   | Quick flash firmware                |
| `idf.py set-target esp32`   | Select ESP32 chip               |
| `idf.py menuconfig`         | Open configuration interface    |
| `idf.py build`              | Build project                   |
| `idf.py flash`              | Flash firmware                  |
| `idf.py flash -p COMx`      | Flash firmware with COM port    |
| `idf.py monitor`            | Serial Monitor                  |
| `idf.py monitor -b 115200`  | Select baudrate Serial Monitor  |
| `idf.py flash monitor`      | Build + Flash + Monitor         |
| `idf.py erase-flash`        | Erase all flash                 |
| `idf.py fullclean`          | Delete old build file, rebuild  |
| `idf.py chip_id`            | View chip ID                    |
| `idf.py flash_size`         | Check flash capacity            |  

### Create components folder in project (if not already exists)  .
```sh 
mkdir -p components
```
### Add this command if u are using platformio with esp-idf with C++
**CMakeList.txt** in project folder tree
```sh
 set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")
 include_directories(${CMAKE_CURRENT_LIST_DIR}/src) 
```
### ESP-IDF Sensor Components library  
```sh
git clone https://github.com/UncleRus/esp-idf-lib
```
### ESP-IDF Component Manager from Espressif
```sh
https://components.espressif.com/components/
```
### Create idf_component.yml download components
Example
```sh
idf.py add-dependency -p src "lvgl/lvgl^9.3.0"
```
Must RUN this command to download components
```sh
idf.py reconfigure
```

