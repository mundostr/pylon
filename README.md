# CONTROL LINE F2A/VFS TIMMING 2025
## ESPNOW PYLON SENSOR

## This proyect is related to Aeromodelling, Control Line Speed classes F2A and VFS (Speed Southamerican Formula).

### Base info:
- Current CIAM Transitrace
- https://www.go-cl.se/f2atimer.html
- https://go-cl.se/tt/tt2016/Transitrace_Feb_2016.htm
- https://fai.org/sites/default/files/sc4_vol_f2_controlline_25v1.3_0.pdf

### Elements needed:
- ESP32 based board.
- hall effect sensor module (with amplifier).
- 3 cells lithium battery with stepdown module.
- external flexible 2.4 antenna to mount outside the pylon.

### Features:
- Hall sensor based pylon position reading.
- Ability to calculate and send elapsed time and speed in km/h lap by lap.
- Ability to communicate via wifi UDP to an app for reporting.
- Ability to modify laps limits between 9 for F2A FAI Speed, or 10 for VFS (Speed Southamerican Formula).

### Installation:
- The system is designed as a PlatformIO project. You need to create a data folder and a credentials.txt file inside, with this format:

```
SSID=wifi_ssid
PASS=wifi_pass
PYLON_IP=fixed_ip_for_this_device
GATEWAY_IP=gateway_ip
NETMASK_IP=gateway_netmask
LAPTOP_IP=laptop_ip (where app is running)
LAPTOP_PORT=laptop_port (ie 3333)
```

### Operation:
- The system connects to a WiFi AP and establishes an UDP socket.
- When it receives an ACT message from a laptop app via socket, it starts to count laps up, taking into consideration the rules specs (2 complete laps must elapse until start timming). From that moment counts up to 9 or 10 (depending on the current active class F2A or VFS).
- Every time the sensor readings reports a new lap, the system sends an UDP message (PARTIAL) to the app with the current lap, elapsed time for that lap, and speed in km/h.
- After complete the total laps, it sends a FINAL message with the total time and the speed, and enters the inactive mode (sensor still works but no report is send via UDP).
- At any moment during timming, if a DES (desactivate) message is received, it cleans up variables and also returns to inactive mode.
- At any moment when inactive, it accepts to switch between F2A or VFS class, according to the UDP message received.