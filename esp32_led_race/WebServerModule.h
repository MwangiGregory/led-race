#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "Config.h"
#include "PhysicsEngine.h"
#include "Logging.h"

/**
 * @class WebServerModule
 * @brief Manages the ESP32 access point (AP) hotspot and the web server,
 *        handling WebSocket telemetry broadcasts to client dashboards.
 */
class WebServerModule {
private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    uint32_t _lastBroadcastTime;

    // Embedded premium CSS/HTML/JS telemetry dashboard
    const char* INDEX_HTML PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Open LED Race v2 Dashboard</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
        <style>
            :root {
                --bg-color: #0b0c10;
                --card-bg: rgba(31, 40, 51, 0.65);
                --text-primary: #f5f5f7;
                --text-secondary: #c5c6c7;
                --accent-cyan: #66fcf1;
                --accent-blue: #1f2833;
                --neon-blue: #45a29e;
                --gold: #ffd700;
            }
            body {
                font-family: 'Outfit', sans-serif;
                background: var(--bg-color);
                background-image: radial-gradient(circle at 50% 20%, #1f2833 0%, #0b0c10 70%);
                color: var(--text-primary);
                margin: 0;
                padding: 10px;
                display: flex;
                flex-direction: column;
                align-items: center;
                min-height: 100vh;
                box-sizing: border-box;
            }
            h1 {
                font-size: 1.8rem;
                font-weight: 800;
                margin: 5px 0 12px 0;
                text-transform: uppercase;
                letter-spacing: 2px;
                background: linear-gradient(90deg, #66fcf1, #45a29e);
                -webkit-background-clip: text;
                -webkit-text-fill-color: transparent;
                text-shadow: 0 4px 10px rgba(102, 252, 241, 0.15);
            }
            .hud-container {
                width: 100%;
                max-width: 700px;
                background: var(--card-bg);
                backdrop-filter: blur(12px);
                border: 1px solid rgba(102, 252, 241, 0.2);
                border-radius: 16px;
                padding: 15px;
                box-shadow: 0 10px 35px rgba(0, 0, 0, 0.55);
                box-sizing: border-box;
                margin-bottom: 15px;
            }
            .stats-grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
                gap: 15px;
                margin-bottom: 5px;
            }
            .stat-box {
                background: rgba(11, 12, 16, 0.75);
                border: 1px solid rgba(102, 252, 241, 0.1);
                border-radius: 12px;
                padding: 15px;
                text-align: center;
            }
            .stat-label {
                font-size: 0.8rem;
                color: var(--text-secondary);
                text-transform: uppercase;
                letter-spacing: 1px;
                margin-bottom: 5px;
            }
            .stat-value {
                font-size: 1.4rem;
                font-weight: 600;
                color: var(--accent-cyan);
            }
            .status-badge {
                display: inline-block;
                padding: 4px 14px;
                border-radius: 30px;
                font-weight: 600;
                letter-spacing: 1px;
                font-size: 0.85rem;
                text-transform: uppercase;
            }
            .status-standby { background: #1a2f4c; color: #8ab4f8; border: 1px solid #3b66a6; }
            .status-countdown { background: #4a2828; color: #f28b82; border: 1px solid #a84444; animation: pulse 0.6s infinite alternate; }
            .status-racing { background: #133a1e; color: #81c995; border: 1px solid #247539; animation: pulse 0.8s infinite alternate; }
            .status-celebration { background: #3c1e4a; color: #d7aefb; border: 1px solid #7d3ca6; animation: rainbow-glow 3s infinite linear; }
            .status-menu { background: #2f2f35; color: #c5c6c7; border: 1px solid #5a5b5d; }
            
            .players-grid {
                width: 100%;
                max-width: 700px;
                display: grid;
                grid-template-columns: 1fr;
                gap: 15px;
                box-sizing: border-box;
            }
            .player-card {
                background: var(--card-bg);
                backdrop-filter: blur(12px);
                border: 1px solid rgba(255, 255, 255, 0.08);
                border-radius: 14px;
                padding: 18px 22px;
                display: flex;
                flex-direction: column;
                position: relative;
                overflow: hidden;
                transition: all 0.3s cubic-bezier(0.25, 0.8, 0.25, 1);
                box-shadow: 0 4px 15px rgba(0, 0, 0, 0.25);
            }
            .player-card:hover {
                transform: translateY(-2px);
                box-shadow: 0 8px 25px rgba(0, 0, 0, 0.4);
            }
            .card-row {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 10px;
            }
            .player-info {
                display: flex;
                align-items: center;
                gap: 10px;
                font-weight: 600;
                font-size: 1.15rem;
            }
            .player-indicator {
                width: 12px;
                height: 12px;
                border-radius: 50%;
                display: inline-block;
            }
            #pcard-0 .player-indicator { background: #dc3545; box-shadow: 0 0 8px #dc3545; }
            #pcard-1 .player-indicator { background: #007bff; box-shadow: 0 0 8px #007bff; }
            #pcard-2 .player-indicator { background: #28a745; box-shadow: 0 0 8px #28a745; }
            #pcard-3 .player-indicator { background: #ffc107; box-shadow: 0 0 8px #ffc107; }

            #pcard-0 { border-left: 5px solid #dc3545; }
            #pcard-1 { border-left: 5px solid #007bff; }
            #pcard-2 { border-left: 5px solid #28a745; }
            #pcard-3 { border-left: 5px solid #ffc107; }

            .leader-badge {
                background: linear-gradient(135deg, #ffd700, #ffa500);
                color: #111;
                padding: 2px 8px;
                border-radius: 20px;
                font-size: 0.72rem;
                font-weight: 800;
                text-transform: uppercase;
                display: none;
                align-items: center;
                gap: 3px;
            }
            .winner-badge {
                background: linear-gradient(135deg, #ffd700, #ff8c00);
                color: #111;
                padding: 4px 12px;
                border-radius: 20px;
                font-size: 0.8rem;
                font-weight: 800;
                text-transform: uppercase;
                display: none;
                align-items: center;
                gap: 4px;
                animation: bounce 0.5s infinite alternate;
            }
            .player-stats {
                display: flex;
                gap: 15px;
                color: var(--text-secondary);
                font-size: 0.95rem;
            }
            .stat-item span {
                font-weight: 600;
                color: var(--text-primary);
            }
            .progress-container {
                width: 100%;
                height: 8px;
                background: rgba(255, 255, 255, 0.05);
                border-radius: 10px;
                overflow: hidden;
                margin-top: 5px;
            }
            .progress-bar {
                height: 100%;
                width: 0%;
                border-radius: 10px;
                transition: width 0.15s ease-out;
            }
            #pcard-0 .progress-bar { background: #dc3545; }
            #pcard-1 .progress-bar { background: #007bff; }
            #pcard-2 .progress-bar { background: #28a745; }
            #pcard-3 .progress-bar { background: #ffc107; }

            .leader-highlight {
                border: 1px solid rgba(255, 215, 0, 0.45) !important;
                background: rgba(255, 215, 0, 0.06) !important;
            }
            .leader-highlight .leader-badge {
                display: flex;
            }
            .winner-highlight {
                border: 2px solid var(--gold) !important;
                background: rgba(255, 215, 0, 0.1) !important;
                box-shadow: 0 0 20px rgba(255, 215, 0, 0.35) !important;
                animation: victory-pulse 1.5s infinite ease-in-out;
            }
            .winner-highlight .winner-badge {
                display: flex;
            }

            @keyframes pulse {
                from { transform: scale(1); opacity: 0.95; }
                to { transform: scale(1.02); opacity: 1; }
            }
            @keyframes victory-pulse {
                0% { box-shadow: 0 0 10px rgba(255, 215, 0, 0.2); }
                50% { box-shadow: 0 0 25px rgba(255, 215, 0, 0.5); }
                100% { box-shadow: 0 0 10px rgba(255, 215, 0, 0.2); }
            }
            @keyframes rainbow-glow {
                0% { border-color: #ff007f; }
                33% { border-color: #00f0ff; }
                66% { border-color: #00ff7f; }
                100% { border-color: #ff007f; }
            }
            @keyframes bounce {
                from { transform: translateY(0); }
                to { transform: translateY(-3px); }
            }
            #winner-banner {
                margin-top: 15px;
                padding-top: 15px;
                border-top: 1px solid rgba(255, 255, 255, 0.08);
            }
        </style>
    </head>
    <body>
        <h1>Open LED Race</h1>
        
        <div class="hud-container">
            <div class="status-row" style="display: flex; justify-content: center; margin-bottom: 15px; border-bottom: 1px solid rgba(255, 255, 255, 0.08); padding-bottom: 12px;">
                <div id="status" class="status-badge status-standby" style="font-size: 1rem; padding: 5px 18px;">Standby</div>
            </div>
            
            <div id="countdown-overlay" style="display: none; text-align: center; margin-bottom: 15px;">
                <div id="countdown-number" style="font-size: 6rem; font-weight: 800; line-height: 1;">3</div>
            </div>

            <div class="stats-grid">
                <div class="stat-box">
                    <div class="stat-label">Track Length</div>
                    <div id="track-len" class="stat-value">5m</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">Difficulty</div>
                    <div id="difficulty" class="stat-value">Medium</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">Fastest Lap Record</div>
                    <div id="high-score" class="stat-value">--</div>
                </div>
            </div>
            <div id="winner-banner" style="display: none; text-align: center;">
                <h2 style="color: var(--gold); margin: 0; font-size: 1.35rem; text-transform: uppercase;">WINNER TIME: <span id="win-time">--</span>s</h2>
            </div>
            <div id="instruction-box" style="margin-top: 15px; text-align: center; font-size: 0.95rem; color: var(--text-secondary); border-top: 1px solid rgba(255, 255, 255, 0.08); padding-top: 15px; display: none;">
                Press the physical <strong>SELECT</strong> button or <strong>any player button</strong> to start the countdown!
            </div>
        </div>

        <div class="players-grid">
            <!-- P1 -->
            <div id="pcard-0" class="player-card">
                <div class="card-row">
                    <div class="player-info">
                        <span class="player-indicator"></span>
                        <span>Player 1 (Red)</span>
                        <span class="leader-badge">Lead</span>
                        <span class="winner-badge">Winner</span>
                    </div>
                    <div class="player-stats">
                        <div class="stat-item">Lap <span id="p0-lap">0</span></div>
                        <div class="stat-item">Spd <span id="p0-spd">0.0</span></div>
                    </div>
                </div>
                <div class="progress-container">
                    <div id="p0-progress" class="progress-bar"></div>
                </div>
            </div>
            
            <!-- P2 -->
            <div id="pcard-1" class="player-card">
                <div class="card-row">
                    <div class="player-info">
                        <span class="player-indicator"></span>
                        <span>Player 2 (Blue)</span>
                        <span class="leader-badge">Lead</span>
                        <span class="winner-badge">Winner</span>
                    </div>
                    <div class="player-stats">
                        <div class="stat-item">Lap <span id="p1-lap">0</span></div>
                        <div class="stat-item">Spd <span id="p1-spd">0.0</span></div>
                    </div>
                </div>
                <div class="progress-container">
                    <div id="p1-progress" class="progress-bar"></div>
                </div>
            </div>

            <!-- P3 -->
            <div id="pcard-2" class="player-card">
                <div class="card-row">
                    <div class="player-info">
                        <span class="player-indicator"></span>
                        <span>Player 3 (Green)</span>
                        <span class="leader-badge">Lead</span>
                        <span class="winner-badge">Winner</span>
                    </div>
                    <div class="player-stats">
                        <div class="stat-item">Lap <span id="p2-lap">0</span></div>
                        <div class="stat-item">Spd <span id="p2-spd">0.0</span></div>
                    </div>
                </div>
                <div class="progress-container">
                    <div id="p2-progress" class="progress-bar"></div>
                </div>
            </div>

            <!-- P4 -->
            <div id="pcard-3" class="player-card">
                <div class="card-row">
                    <div class="player-info">
                        <span class="player-indicator"></span>
                        <span>Player 4 (Yellow)</span>
                        <span class="leader-badge">Lead</span>
                        <span class="winner-badge">Winner</span>
                    </div>
                    <div class="player-stats">
                        <div class="stat-item">Lap <span id="p3-lap">0</span></div>
                        <div class="stat-item">Spd <span id="p3-spd">0.0</span></div>
                    </div>
                </div>
                <div class="progress-container">
                    <div id="p3-progress" class="progress-bar"></div>
                </div>
            </div>
        </div>

        <script>
            var gateway = `ws://${window.location.hostname}/ws`;
            var websocket = new WebSocket(gateway);
            
            websocket.onmessage = function(event) {
                var data = JSON.parse(event.data);
                
                // 1. Update Game Status Badge
                var statusEl = document.getElementById("status");
                statusEl.className = "status-badge";
                
                var stateStr = "Standby";
                var stateClass = "status-standby";
                switch(data.state) {
                    case 0: stateStr = "Standby"; stateClass = "status-standby"; break;
                    case 1: stateStr = "Countdown"; stateClass = "status-countdown"; break;
                    case 2: stateStr = "Racing"; stateClass = "status-racing"; break;
                    case 3: stateStr = "Celebration"; stateClass = "status-celebration"; break;
                    case 4: stateStr = "Settings"; stateClass = "status-menu"; break;
                }
                statusEl.innerText = stateStr;
                statusEl.classList.add(stateClass);

                // 2. Update Track Metrics & High Score
                document.getElementById("track-len").innerText = data.trackLen + "m";
                
                var diffStr = "Medium";
                switch(data.difficulty) {
                    case 0: diffStr = "Easy"; break;
                    case 1: diffStr = "Medium"; break;
                    case 2: diffStr = "Hard"; break;
                    case 3: diffStr = "Very Hard"; break;
                }
                document.getElementById("difficulty").innerText = diffStr;

                document.getElementById("high-score").innerText = (data.highScore > 900) ? "--" : data.highScore.toFixed(2) + "s";

                // 3. Handle Winner Banner & Standby Instructions
                var bannerEl = document.getElementById("winner-banner");
                if (data.state === 3 && data.winner !== 255) {
                    document.getElementById("win-time").innerText = data.winnerTime.toFixed(2);
                    bannerEl.style.display = "block";
                } else {
                    bannerEl.style.display = "none";
                }

                var instructionEl = document.getElementById("instruction-box");
                if (data.state === 0) { // STATE_STANDBY
                    instructionEl.style.display = "block";
                } else {
                    instructionEl.style.display = "none";
                }

                // Handle Countdown Display
                var countdownEl = document.getElementById("countdown-overlay");
                var countdownNumEl = document.getElementById("countdown-number");
                if (data.state === 1) { // STATE_COUNTDOWN
                    countdownEl.style.display = "block";
                    var elapsed = data.countdown;
                    if (elapsed < 1000) {
                        countdownNumEl.innerText = "3";
                        countdownNumEl.style.color = "#ff3333";
                        countdownNumEl.style.textShadow = "0 0 15px rgba(255, 51, 51, 0.6)";
                    } else if (elapsed >= 1000 && elapsed < 2000) {
                        countdownNumEl.innerText = "2";
                        countdownNumEl.style.color = "#ffaa00";
                        countdownNumEl.style.textShadow = "0 0 15px rgba(255, 170, 0, 0.6)";
                    } else if (elapsed >= 2000 && elapsed < 3000) {
                        countdownNumEl.innerText = "1";
                        countdownNumEl.style.color = "#ffff33";
                        countdownNumEl.style.textShadow = "0 0 15px rgba(255, 255, 51, 0.6)";
                    } else {
                        countdownNumEl.innerText = "GO!";
                        countdownNumEl.style.color = "#33ff33";
                        countdownNumEl.style.textShadow = "0 0 15px rgba(51, 255, 51, 0.6)";
                    }
                } else {
                    countdownEl.style.display = "none";
                }

                // 4. Update Player Cards
                var activeCount = data.activePlayers;
                var numLEDs = data.trackLeds;
                
                for (var i = 0; i < 4; i++) {
                    var card = document.getElementById(`pcard-${i}`);
                    if (i < activeCount) {
                        card.style.display = "flex";
                        
                        var pData = data.players[i];
                        document.getElementById(`p${i}-lap`).innerText = pData.lap;
                        document.getElementById(`p${i}-spd`).innerText = pData.spd.toFixed(1);
                        
                        // Progress Calculation
                        var totalLeds = numLEDs * data.maxLaps;
                        var progressPct = (pData.pos / totalLeds) * 100;
                        if (progressPct > 100) progressPct = 100;
                        if (progressPct < 0) progressPct = 0;
                        document.getElementById(`p${i}-progress`).style.width = progressPct + "%";
                        
                        // Reset styling
                        card.classList.remove("leader-highlight", "winner-highlight");
                        
                        // Leader Highlight (only during racing)
                        if (data.state === 2 && data.leader === i) {
                            card.classList.add("leader-highlight");
                        }
                        
                        // Winner Highlight (during celebration)
                        if (data.state === 3 && data.winner === i) {
                            card.classList.add("winner-highlight");
                        }
                    } else {
                        card.style.display = "none";
                    }
                }
            };
            
            websocket.onclose = function() {
                setTimeout(function() { location.reload(); }, 2000);
            };
        </script>
    </body>
    </html>
    )rawliteral";

public:
    /**
     * @brief Constructor initializing the async web server on port 80 and the WebSocket endpoint.
     */
    WebServerModule() : _server(80), _ws("/ws"), _lastBroadcastTime(0) {}

    /**
     * @brief Configures WiFi Access Point mode and starts the web server and WebSocket endpoint.
     */
    void begin() {
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
        SYS_LOG("[Network] Hotspot active. Connect to http://%s\n", WiFi.softAPIP().toString().c_str());

        if (MDNS.begin("ledrace")) {
            SYS_LOG("[Network] mDNS responder started. Access via http://ledrace.local\n");
            MDNS.addService("http", "tcp", 80);
        } else {
            SYS_LOG("[Network] Error setting up MDNS responder!\n");
        }

        _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
            if (type == WS_EVT_CONNECT) {
                SYS_LOG("[WebSocket] Browser socket client #%u connected\n", client->id());
            } else if (type == WS_EVT_DISCONNECT) {
                SYS_LOG("[WebSocket] Browser socket client #%u disconnected\n", client->id());
            }
        });
        _server.addHandler(&_ws);

        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            request->send_P(200, "text/html", INDEX_HTML);
        });

        _server.begin();
    }
    /**
     * @brief Broadcasts game telemetry data as a JSON packet to all connected WebSocket clients.
     * @param physics The physics engine state reference.
     * @param state The current global GameState.
     * @param winnerId The ID of the winner (255 if none).
     * @param winnerTime The winner's race time.
     * @param highScore The persistent high score time.
     */
    void broadcastTelemetry(const PhysicsEngine& physics, GameState state, uint8_t winnerId, float winnerTime, float highScore, uint32_t countdown = 0) {
        uint32_t now = millis();
        if (now - _lastBroadcastTime < 100) return;
        if (_ws.count() == 0) return;

        _lastBroadcastTime = now;

        String jsonPayload = "{";
        jsonPayload += "\"state\":" + String((int)state) + ",";
        jsonPayload += "\"trackLen\":" + String(physics.getTrackLengthMeters()) + ",";
        jsonPayload += "\"trackLeds\":" + String(physics.getTrackLength()) + ",";
        jsonPayload += "\"difficulty\":" + String(physics.getDifficulty()) + ",";
        jsonPayload += "\"maxLaps\":" + String(physics.getMaxLaps()) + ",";
        jsonPayload += "\"countdown\":" + String(countdown) + ",";
        jsonPayload += "\"activePlayers\":" + String(physics.getActivePlayers()) + ",";
        jsonPayload += "\"leader\":" + String(physics.getLeaderId()) + ",";
        jsonPayload += "\"winner\":" + String((int)winnerId) + ",";
        jsonPayload += "\"winnerTime\":" + String(winnerTime, 2) + ",";
        jsonPayload += "\"highScore\":" + String(highScore, 2) + ",";
        jsonPayload += "\"players\":[";

        uint8_t activeCount = physics.getActivePlayers();
        for (uint8_t i = 0; i < activeCount; i++) {
            const CarData& car = physics.getCarData(i);
            jsonPayload += "{";
            jsonPayload += "\"lap\":" + String(car.currentLap) + ",";
            jsonPayload += "\"spd\":" + String(car.speed, 2) + ",";
            jsonPayload += "\"px\":" + String(physics.getCarTrackPixelIndex(i)) + ",";
            jsonPayload += "\"pos\":" + String(car.position, 2);
            jsonPayload += "}";
            if (i < activeCount - 1) jsonPayload += ",";
        }
        jsonPayload += "]}";

        _ws.textAll(jsonPayload);
    }

    /**
     * @brief Performs cleanup of inactive WebSocket clients to manage system memory.
     */
    void cleanup() {
        _ws.cleanupClients();
    }
};

#endif // WEB_SERVER_MODULE_H