#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "config.h"
#include "sensors.h"
#include "storage.h"
#include "hotspot.h"

// the web server runs on port 80
static WebServer server(80);

// time of the last request or connection (used to know when to shut down)
static uint32_t lastActivity = 0;

// touch --> resets the idle timer
static void touch() { lastActivity = millis(); }

// the web page, %INFO% gets replaced with the current status
static const char PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1"><title>River Monitor</title>
<style>body{font-family:sans-serif;max-width:480px;margin:1.5em auto;padding:0 1em}
a.btn,button{display:block;width:100%;padding:.8em;margin:.6em 0;font-size:1em;text-align:center;
box-sizing:border-box;border:1px solid #888;border-radius:6px;background:#eee;color:#000;text-decoration:none}
pre{white-space:pre-wrap}</style></head><body>
<h2>River Monitor</h2><p>%INFO%</p>
<a class="btn" href="/log.csv">Download log (CSV)</a>
<a class="btn" href="/log_old.csv">Download older log (CSV)</a>
<button onclick="live()">Take live reading</button><pre id="live"></pre>
<button onclick="if(confirm('Delete ALL stored data?'))location='/clear'">Clear stored data</button>
<script>
fetch('/settime?t='+Math.floor(Date.now()/1000));
function live(){var auto=false;
function live(){var e=document.getElementById('live');if(!auto)e.textContent='Reading...';
fetch('/live').then(function(r){return r.text()}).then(function(t){e.textContent=t;if(auto)setTimeout(live,2000)})}
if(location.search.indexOf('auto')>-1){auto=true;live()}}
</script></body></html>)rawliteral";

// sendFile --> sends a log file to the phone as a download
static void sendFile(const char *path, const char *downloadName) {
  touch();

  // tell the user if the file doesn't exist yet
  if (!LittleFS.exists(path)) { server.send(404, "text/plain", "No data yet"); return; }

  // send the file as a download
  File f = LittleFS.open(path, FILE_READ);
  server.sendHeader("Content-Disposition", String("attachment; filename=") + downloadName);
  server.streamFile(f, "text/csv");
  f.close();
}

// setupRoutes --> sets what happens for each web address
static void setupRoutes() {
  // main page: shows the status and the buttons
  server.on("/", []() {
    touch();
    String info = "Readings logged since power-up: " + String(readingsLogged()) +
                  "<br>Log size: " + String(logSizeBytes() / 1024.0, 1) + " KB" +
                  "<br>Clock: " + String(timeIsSet() ? "set" : "NOT set") +
                  " (set automatically when you open this page)";
    String page = FPSTR(PAGE);
    page.replace("%INFO%", info);
    server.send(200, "text/html", page);
  });

  // downloads the current log and the older log
  server.on("/log.csv",     []() { sendFile(LOG_FILE, "river_log.csv"); });
  server.on("/log_old.csv", []() { sendFile(LOG_OLD,  "river_log_old.csv"); });

  // sets the clock using the time from the phone
  server.on("/settime", []() {
    touch();
    setClock(server.arg("t").toInt());
    server.send(200, "text/plain", "ok");
  });

  // takes a reading right now and shows it on the page
  server.on("/live", []() {
    touch();
    Reading r = takeReading();
    char buf[200];
    snprintf(buf, sizeof buf,
             "Temp: %.2f C\nTDS: %.0f ppm\nTurbidity: %.0f NTU (%.3f V)\nTilt: %.1f deg\nLevel: %.1f cm",
             r.tempC, r.tds_ppm, r.turb_ntu, r.turb_v, r.tilt_deg, r.level_cm);
    server.send(200, "text/plain", buf);
  });

  // deletes all the stored data
  server.on("/clear", []() {
    touch();
    clearLog();
    server.send(200, "text/html", "<p>Stored data cleared.</p><a href='/'>Back</a>");
  });
}

// hotspotStart --> starts the wifi hotspot and the web server
void hotspotStart() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  setupRoutes();
  server.begin();
  touch();
  Serial.printf("Hotspot '%s' up - open http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

// hotspotHandle --> checks for and answers any web page requests
void hotspotHandle() { server.handleClient(); }

// hotspotStop --> turns off the web server and wifi
void hotspotStop() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

// hotspotRunUntilIdle --> keeps the hotspot on until no one is connected for a while
void hotspotRunUntilIdle() {
  hotspotStart();
  uint32_t start = millis();

  // keep going until the max time is reached
  while (millis() - start < AP_MAX_MS) {
    server.handleClient();

    // someone is connected so reset the idle timer
    if (WiFi.softAPgetStationNum() > 0) touch();

    // no one has been connected for too long so stop
    if (millis() - lastActivity > AP_IDLE_MS) break;
    delay(5);
  }
  hotspotStop();
}
