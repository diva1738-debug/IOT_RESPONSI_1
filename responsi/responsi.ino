
#include <WiFi.h>
#include <WebServer.h>

// ===================================
// KONFIGURASI WIFI
// ===================================
const char* ssid = "dvanad";
const char* password = "12345678";

// ===================================
// KONFIGURASI PIN SENSOR
// ===================================
#define TRIG_PIN 5
#define ECHO_PIN 18
#define PIR_PIN 27

// ===================================
// WEB SERVER
// ===================================
WebServer server(80);

// ===================================
// VARIABEL SENSOR DAN WIFI
// ===================================
float distance = -1;
int motion = 0;

String securityStatus = "MEMULAI";
String wifiStatus = "MENGHUBUNGKAN";

// ===================================
// MEMBACA SENSOR ULTRASONIK
// ===================================
float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2;
}

// ===================================
// MEMPROSES DATA SENSOR
// ===================================
void processSensors() {
  distance = readUltrasonic();
  motion = digitalRead(PIR_PIN);

  if (motion == HIGH && distance >= 0 && distance < 100) {
    securityStatus = "OBJEK TERDETEKSI";
  }
  else if (motion == HIGH) {
    securityStatus = "GERAKAN TERDETEKSI";
  }
  else if (distance >= 0 && distance < 100) {
    securityStatus = "OBJEK DEKAT";
  }
  else {
    securityStatus = "AMAN";
  }
}

// ===================================
// HALAMAN WEB MONITORING
// ===================================
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Security System</title>

    <style>
      body {
        font-family: Arial;
        text-align: center;
        background: #eef3f8;
        padding: 20px;
      }

      .card {
        background: white;
        padding: 20px;
        margin: 15px auto;
        border-radius: 12px;
        max-width: 400px;
        box-shadow: 0 2px 10px #ccc;
      }

      h1 {
        color: #164e8a;
      }

      .value {
        font-size: 26px;
        font-weight: bold;
        color: #164e8a;
      }

      .connected {
        color: green;
      }

      .disconnected {
        color: red;
      }
    </style>
  </head>

  <body>

    <h1>Smart Security System</h1>
    <p>Monitoring Keamanan Berbasis IoT</p>

    <div class="card">
      <h3>Status Koneksi WiFi</h3>
      <div class="value" id="wifi">MEMERIKSA...</div>
      <p id="ip">IP Address: -</p>
      <p id="network">Nama WiFi: -</p>
    </div>

    <div class="card">
      <h3>Jarak Ultrasonik</h3>
      <div class="value" id="distance">Loading...</div>
    </div>

    <div class="card">
      <h3>Sensor PIR</h3>
      <div class="value" id="motion">Loading...</div>
    </div>

    <div class="card">
      <h3>Status Keamanan</h3>
      <div class="value" id="status">Loading...</div>
    </div>

    <p>Data diperbarui otomatis setiap detik.</p>

    <script>
      async function updateData() {
        try {
          const response = await fetch('/data');
          const data = await response.json();

          const wifi = document.getElementById('wifi');

          wifi.innerText = data.wifi;
          wifi.className = 'value ' +
            (data.wifi == 'TERHUBUNG'
              ? 'connected'
              : 'disconnected');

          document.getElementById('ip').innerText =
            'IP Address: ' + data.ip;

          document.getElementById('network').innerText =
            'Nama WiFi: ' + data.ssid;

          document.getElementById('distance').innerText =
            data.distance < 0
              ? 'Tidak terbaca'
              : data.distance + ' cm';

          document.getElementById('motion').innerText =
            data.motion == 1
              ? 'TERDETEKSI'
              : 'TIDAK TERDETEKSI';

          document.getElementById('status').innerText =
            data.status;

        } catch (error) {
          document.getElementById('status').innerText =
            'Koneksi terputus';
        }
      }

      updateData();
      setInterval(updateData, 1000);
    </script>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// ===================================
// MENGIRIM DATA SENSOR KE BROWSER
// ===================================
void handleData() {
  String ipAddress = "Belum tersedia";
  String networkName = "Tidak terhubung";

  if (WiFi.status() == WL_CONNECTED) {
    ipAddress = WiFi.localIP().toString();
    networkName = WiFi.SSID();
  }

  String json = "{";

  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"motion\":" + String(motion) + ",";
  json += "\"status\":\"" + securityStatus + "\",";
  json += "\"wifi\":\"" + wifiStatus + "\",";
  json += "\"ip\":\"" + ipAddress + "\",";
  json += "\"ssid\":\"" + networkName + "\"";

  json += "}";

  server.send(200, "application/json", json);
}

// ===================================
// SETUP
// ===================================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  Serial.println();
  Serial.println("SMART SECURITY SYSTEM");
  Serial.println("==============================");

  // Menghubungkan ESP32 ke WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Menghubungkan ke WiFi");

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiStatus = "TERHUBUNG";

    Serial.println();
    Serial.println("WIFI BERHASIL TERHUBUNG");
    Serial.print("Nama WiFi: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println("==============================");

  } else {
    wifiStatus = "TERPUTUS";

    Serial.println();
    Serial.println("WIFI GAGAL TERHUBUNG");
  }

  // Mengaktifkan web server
  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();

  Serial.println("Web Server dimulai!");
}

// ===================================
// LOOP
// ===================================
void loop() {

  // Memeriksa koneksi WiFi
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiStatus != "TERHUBUNG") {
      wifiStatus = "TERHUBUNG";

      Serial.println("WiFi berhasil terhubung kembali!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    }
  }
  else {
    if (wifiStatus != "TERPUTUS") {
      wifiStatus = "TERPUTUS";
      Serial.println("WiFi terputus!");
    }

    static unsigned long lastReconnect = 0;

    if (millis() - lastReconnect >= 5000) {
      lastReconnect = millis();

      Serial.println("Mencoba menghubungkan kembali...");
      WiFi.reconnect();
    }
  }

  // Membaca dan memproses sensor
  processSensors();

  // Menampilkan data di Serial Monitor
  Serial.println("==============================");

  Serial.print("Status WiFi: ");
  Serial.println(wifiStatus);

  Serial.print("Jarak: ");

  if (distance < 0) {
    Serial.println("Tidak terbaca");
  } else {
    Serial.print(distance);
    Serial.println(" cm");
  }

  Serial.print("Gerakan: ");
  Serial.println(
    motion == HIGH ? "TERDETEKSI" : "TIDAK TERDETEKSI"
  );

  Serial.print("Status Keamanan: ");
  Serial.println(securityStatus);

  // Melayani permintaan browser
  server.handleClient();

  delay(1000);
}
