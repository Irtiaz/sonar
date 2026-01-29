# DYP-L08 Ultrasonic Underwater Sensor — ESP32 Interface

ESP-IDF project for reading distance measurements from a DYP-L08 (DYP-L081MTW) ultrasonic underwater obstacle avoidance sensor over UART.

## Wiring

| Sensor Wire | ESP32 Pin | Function |
|-------------|-----------|----------|
| Red | VIN (5V) | Power |
| Black | GND | Ground |
| White | GPIO16 | Sensor TX → ESP32 RX |
| Yellow | GPIO17 | ESP32 TX → Sensor RX |

The sensor requires 5V power. Use the VIN pin (USB voltage passthrough), not the 3.3V regulator output.

## Sensor Details

- **Model**: DYP-L081MTW (UART controlled mode)
- **Baud rate**: 115200, 8N1
- **Operating mode**: Command-triggered — the sensor does not output data on its own. It responds only after receiving a trigger byte (`0x01`) on its RX line.
- **Range**: 5–200 cm (underwater)
- **IP68 rated**: Operational up to 10 m depth

This sensor is designed for underwater use. In air, it will return `0xFFFD` (no target).

## Protocol

**Request**: Send `0x01` on UART TX.

**Response**: 4 bytes.

| Byte | Value |
|------|-------|
| 0 | `0xFF` (header) |
| 1 | Distance high byte |
| 2 | Distance low byte |
| 3 | Checksum: `(byte0 + byte1 + byte2) & 0xFF` |

Distance in millimeters = `byte1 * 256 + byte2`. A value of `0xFFFD` (65533) means no target detected.

## How the Code Works

The program configures UART1 at 115200 baud on GPIO16 (RX) and GPIO17 (TX). In a loop, it sends the trigger byte `0x01`, waits up to 500 ms for a 4-byte response, validates the checksum, and logs the distance. Readings are taken every 200 ms.
