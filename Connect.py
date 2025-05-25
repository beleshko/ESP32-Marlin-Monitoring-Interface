#!/usr/bin/env python3
"""
ESP32 UART Bridge - Virtual COM Port
Создает виртуальный COM-порт для работы с ESP32 UART Bridge
Работает на Linux и Windows
"""

import socket
import threading
import time
import sys
import os
import argparse
from typing import Optional

# Импорты для разных ОС
if sys.platform.startswith('win'):
    import serial
    import serial.tools.list_ports
    try:
        import com0com
        HAS_COM0COM = True
    except ImportError:
        HAS_COM0COM = False
        print("Предупреждение: com0com не найден. Для Windows рекомендуется установить com0com")
else:
    import pty
    import serial
    import fcntl
    import termios

class ESP32UARTBridge:
    def __init__(self, esp32_ip: str, esp32_port: int = 23, virtual_port: Optional[str] = None):
        self.esp32_ip = esp32_ip
        self.esp32_port = esp32_port
        self.virtual_port = virtual_port
        self.socket = None
        self.running = False
        self.threads = []
        
        # Для Linux - pty
        self.master_fd = None
        self.slave_fd = None
        self.slave_name = None
        
        # Для Windows - serial
        self.serial_port = None
        
    def create_virtual_port_linux(self):
        """Создает виртуальный порт в Linux используя pty"""
        try:
            self.master_fd, self.slave_fd = pty.openpty()
            self.slave_name = os.ttyname(self.slave_fd)
            
            # Настройка терминала
            attrs = termios.tcgetattr(self.slave_fd)
            attrs[0] = 0  # iflag
            attrs[1] = 0  # oflag
            attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # cflag
            attrs[3] = 0  # lflag
            attrs[4] = termios.B115200  # ispeed
            attrs[5] = termios.B115200  # ospeed
            termios.tcsetattr(self.slave_fd, termios.TCSANOW, attrs)
            
            print(f"Виртуальный порт создан: {self.slave_name}")
            return True
            
        except Exception as e:
            print(f"Ошибка создания виртуального порта Linux: {e}")
            return False
    
    def create_virtual_port_windows(self):
        """Создает виртуальный порт в Windows"""
        if self.virtual_port:
            try:
                # Пытаемся открыть указанный порт
                self.serial_port = serial.Serial(
                    port=self.virtual_port,
                    baudrate=115200,
                    timeout=0.1
                )
                print(f"Используется порт: {self.virtual_port}")
                return True
            except Exception as e:
                print(f"Не удалось открыть порт {self.virtual_port}: {e}")
        
        # Ищем доступные порты
        available_ports = [port.device for port in serial.tools.list_ports.comports()]
        
        if HAS_COM0COM:
            print("com0com найден. Рекомендуется настроить пару портов через com0com.")
        
        print("Доступные COM-порты:")
        for port in available_ports:
            print(f"  {port}")
        
        if not available_ports:
            print("Доступных COM-портов не найдено.")
            print("Для Windows рекомендуется:")
            print("1. Установить com0com")
            print("2. Создать пару виртуальных портов")
            print("3. Указать один из портов через параметр --port")
            return False
        
        return False
    
    def connect_to_esp32(self) -> bool:
        """Подключается к ESP32"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5)
            self.socket.connect((self.esp32_ip, self.esp32_port))
            print(f"Подключено к ESP32: {self.esp32_ip}:{self.esp32_port}")
            return True
        except Exception as e:
            print(f"Ошибка подключения к ESP32: {e}")
            return False
    
    def esp32_to_virtual_thread(self):
        """Поток для передачи данных от ESP32 к виртуальному порту"""
        while self.running:
            try:
                data = self.socket.recv(1024)
                if not data:
                    break
                
                if sys.platform.startswith('win'):
                    if self.serial_port and self.serial_port.is_open:
                        self.serial_port.write(data)
                else:
                    if self.master_fd:
                        os.write(self.master_fd, data)
                        
            except Exception as e:
                if self.running:
                    print(f"Ошибка при получении данных от ESP32: {e}")
                break
    
    def virtual_to_esp32_thread(self):
        """Поток для передачи данных от виртуального порта к ESP32"""
        while self.running:
            try:
                data = None
                
                if sys.platform.startswith('win'):
                    if self.serial_port and self.serial_port.is_open:
                        if self.serial_port.in_waiting > 0:
                            data = self.serial_port.read(self.serial_port.in_waiting)
                else:
                    if self.master_fd:
                        try:
                            data = os.read(self.master_fd, 1024)
                        except OSError:
                            time.sleep(0.01)
                            continue
                
                if data and self.socket:
                    self.socket.send(data)
                    
                time.sleep(0.01)
                
            except Exception as e:
                if self.running:
                    print(f"Ошибка при отправке данных в ESP32: {e}")
                break
    
    def status_thread(self):
        """Поток для вывода статуса соединения"""
        while self.running:
            try:
                # Проверяем соединение с ESP32
                self.socket.send(b'')  # Пустой пакет для проверки
                time.sleep(5)
            except:
                if self.running:
                    print("Потеряно соединение с ESP32")
                    self.stop()
                break
    
    def start(self) -> bool:
        """Запускает мост"""
        print("Запуск ESP32 UART Bridge...")
        
        # Создаем виртуальный порт
        if sys.platform.startswith('win'):
            if not self.create_virtual_port_windows():
                return False
        else:
            if not self.create_virtual_port_linux():
                return False
        
        # Подключаемся к ESP32
        if not self.connect_to_esp32():
            return False
        
        self.running = True
        
        # Запускаем потоки
        self.threads = [
            threading.Thread(target=self.esp32_to_virtual_thread, daemon=True),
            threading.Thread(target=self.virtual_to_esp32_thread, daemon=True),
            threading.Thread(target=self.status_thread, daemon=True)
        ]
        
        for thread in self.threads:
            thread.start()
        
        if sys.platform.startswith('win'):
            if self.virtual_port:
                print(f"Мост активен. Виртуальный порт: {self.virtual_port}")
            else:
                print("Мост активен. Настройте виртуальный порт через com0com")
        else:
            print(f"Мост активен. Виртуальный порт: {self.slave_name}")
        
        print("Нажмите Ctrl+C для остановки")
        return True
    
    def stop(self):
        """Останавливает мост"""
        print("\nОстановка моста...")
        self.running = False
        
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        
        if sys.platform.startswith('win'):
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
        else:
            if self.master_fd:
                os.close(self.master_fd)
            if self.slave_fd:
                os.close(self.slave_fd)
        
        # Ждем завершения потоков
        for thread in self.threads:
            if thread.is_alive():
                thread.join(timeout=1)

def scan_esp32_devices(start_ip: str = "192.168.1.", port: int = 23) -> list:
    """Сканирует сеть на наличие ESP32 устройств"""
    print(f"Сканирование сети {start_ip}1-254 на порту {port}...")
    found_devices = []
    
    def check_device(ip):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((ip, port))
            sock.close()
            if result == 0:
                found_devices.append(ip)
                print(f"Найдено устройство: {ip}")
        except:
            pass
    
    threads = []
    for i in range(1, 255):
        ip = f"{start_ip}{i}"
        thread = threading.Thread(target=check_device, args=(ip,))
        thread.start()
        threads.append(thread)
    
    for thread in threads:
        thread.join()
    
    return found_devices

def main():
    parser = argparse.ArgumentParser(description='ESP32 UART Bridge - Virtual COM Port')
    parser.add_argument('--ip', '-i', help='IP адрес ESP32', required=False)
    parser.add_argument('--port', '-p', help='Виртуальный COM-порт (только для Windows)', required=False)
    parser.add_argument('--esp-port', '-e', type=int, default=23, help='Порт ESP32 (по умолчанию 23)')
    parser.add_argument('--scan', '-s', action='store_true', help='Сканировать сеть на наличие ESP32')
    
    args = parser.parse_args()
    
    if args.scan or not args.ip:
        devices = scan_esp32_devices()
        if not devices:
            print("ESP32 устройства не найдены")
            return
        
        if not args.ip:
            if len(devices) == 1:
                esp32_ip = devices[0]
                print(f"Автоматически выбран: {esp32_ip}")
            else:
                print("Найдено несколько устройств:")
                for i, device in enumerate(devices):
                    print(f"{i+1}. {device}")
                
                while True:
                    try:
                        choice = int(input("Выберите устройство (номер): ")) - 1
                        if 0 <= choice < len(devices):
                            esp32_ip = devices[choice]
                            break
                        else:
                            print("Неверный номер")
                    except ValueError:
                        print("Введите число")
        else:
            esp32_ip = args.ip
    else:
        esp32_ip = args.ip
    
    # Создаем и запускаем мост
    bridge = ESP32UARTBridge(esp32_ip, args.esp_port, args.port)
    
    try:
        if bridge.start():
            while bridge.running:
                time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        bridge.stop()

if __name__ == "__main__":
    main()
