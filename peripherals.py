print("Starting Python peripheral mocks initialization...")
# BME280 Mock Logic
def bme280_handler(data):
    reg = data[0]
    print(f"BME280 I2C Read/Write at register: 0x{reg:02X}")
    if reg == 0xD0: # ID register
        bme280.EnqueueResponseBytes([0x60])
    elif reg == 0xFA: # Temperature MSB
        bme280.EnqueueResponseBytes([0x7E, 0xED, 0x00])

# SSD1306 Mock
def ssd1306_handler(data):
    pass

try:
    bme280 = machine["sysbus.i2c1.bme280"]
    bme280.DataReceived += bme280_handler
    print("BME280 mock attached.")

    ssd1306 = machine["sysbus.i2c1.ssd1306"]
    ssd1306.DataReceived += ssd1306_handler
    print("SSD1306 mock attached.")
except Exception as e:
    print(f"Error attaching mocks: {e}")

print("Python peripheral mocks initialized successfully.")
