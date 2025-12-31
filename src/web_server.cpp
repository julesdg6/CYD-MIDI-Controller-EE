// web_server.cpp
// WiFi web server for SD card file management
// Provides upload, download, delete, and list operations

#include "web_server.h"
#include "common_definitions.h"

WebServer server(WEB_SERVER_PORT);
bool wifiEnabled = false;
String wifiIPAddress = "";
String wifiMode = "AP"; // AP or STA
String currentPath = "/";

File uploadFile;

// WiFi config file path
const char* WIFI_CONFIG_FILE = "/wifi_config.txt";

// Minimal HTML page for file management
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>CYD Manager</title><style>
*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:#222;color:#fff;padding:10px}
h1{text-align:center;margin-bottom:10px;font-size:1.3em}button{padding:6px 10px;background:#28a;border:none;border-radius:4px;color:#fff;cursor:pointer;margin:2px;font-size:13px}
button:hover{background:#3ad}input[type='file'],input[type='text'],input[type='password']{padding:6px;border:1px solid #555;border-radius:4px;background:#333;color:#fff;margin:2px;font-size:13px;width:calc(100% - 4px)}
.breadcrumb{background:#333;padding:8px;border-radius:4px;margin-bottom:8px;font-size:14px}.breadcrumb a{color:#3ad;cursor:pointer;text-decoration:none}
.breadcrumb a:hover{text-decoration:underline}ul{list-style:none}li{padding:6px;margin:3px 0;background:#333;border-radius:4px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap}
.file-info{flex:1;min-width:120px}.file-name{font-weight:bold;cursor:pointer;color:#3ad}.file-name:hover{text-decoration:underline}.file-size{opacity:0.7;font-size:0.85em;margin-left:8px}
.folder{color:#fa0}.btn-delete{background:#c33}.btn-delete:hover{background:#e44}.status{padding:6px;margin:6px 0;border-radius:4px;display:none;font-size:13px}
.success{background:#2a5}.error{background:#c33}.section{margin:10px 0;padding:8px;background:#2a2a2a;border-radius:4px}.section h2{font-size:1.1em;margin-bottom:6px}
.wifi-form label{display:block;margin-top:6px;font-size:13px}.wifi-info{font-size:12px;opacity:0.8;margin-top:4px}
.gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:8px;margin-top:8px}
.gallery-item{background:#333;border-radius:4px;overflow:hidden;text-align:center;cursor:pointer;border:2px solid #444;transition:border 0.2s}
.gallery-item:hover{border-color:#3ad}.gallery-thumb{width:100%;height:120px;background:#111;display:flex;align-items:center;justify-content:center;font-size:2em}
.gallery-name{padding:6px;font-size:12px;word-break:break-word;overflow:hidden;text-overflow:ellipsis}
.gallery-controls{display:flex;gap:4px;padding:4px;justify-content:center}
.gallery-controls button{padding:4px 6px;font-size:11px}
</style></head><body>
<h1>🎹 CYD Manager</h1>
<div class='section'>
<h2>📸 Screenshots Gallery</h2>
<button onclick='loadScreenshots()'>Refresh Gallery</button>
<button onclick='downloadAllScreenshots()'>⬇️ Download All</button>
<div class='gallery' id='gallery'><div style='grid-column:1/-1;text-align:center;padding:20px'>Loading...</div></div>
</div>
<div class='section'>
<h2>📁 Files</h2>
<div class='breadcrumb' id='breadcrumb'>/</div>
<form id='up' enctype='multipart/form-data'>
<input type='file' name='file' id='fi' required>
<button type='submit'>Upload</button>
<button type='button' onclick='takeScreenshot()'>📸 Screenshot</button>
</form>
<div class='status' id='st'></div>
<ul id='fl'><li>Loading...</li></ul>
<button onclick='loadFiles()'>Refresh</button>
</div>
<div class='section'>
<h2>📶 WiFi Config</h2>
<form class='wifi-form' id='wifiForm'>
<label>SSID:<input type='text' id='ssid' placeholder='WiFi Network Name'></label>
<label>Password:<input type='password' id='pass' placeholder='WiFi Password'></label>
<button type='submit'>Save WiFi Config</button>
</form>
<div class='wifi-info' id='wifiInfo'>Current: AP Mode</div>
</div>
<script>
let curPath='/';
function fmt(b){if(b===0)return '0B';const k=1024,s=['B','KB','MB','GB'],i=Math.floor(Math.log(b)/Math.log(k));return Math.round(b/Math.pow(k,i)*100)/100+' '+s[i]}
function updateBreadcrumb(){const parts=curPath.split('/').filter(p=>p);let html='<a onclick="navTo(\'/\')">🏠</a>';let path='';parts.forEach(p=>{path+='/'+p;html+=' / <a onclick="navTo(\''+path+'\')">'+p+'</a>'});document.getElementById('breadcrumb').innerHTML=html}
function navTo(p){curPath=p;loadFiles()}
function loadFiles(){fetch('/list?path='+encodeURIComponent(curPath)).then(r=>r.json()).then(f=>{const l=document.getElementById('fl');if(f.length===0){l.innerHTML='<li>No items</li>';updateBreadcrumb();return}
l.innerHTML=f.map(item=>{if(item.isDir)return '<li><div class="file-info"><span class="file-name folder" onclick="navTo(\''+item.path+'\')">📁 '+item.name+'</span></div></li>';
return '<li><div class="file-info"><span class="file-name">'+item.name+'</span><span class="file-size">'+fmt(item.size)+'</span></div><div><button onclick="location.href=\'/download?file='+encodeURIComponent(item.path)+'\'">⬇️</button><button class="btn-delete" onclick="del(\''+item.path+'\')">🗑️</button></div></li>'}).join('');updateBreadcrumb()}).catch(e=>console.error(e))}
function del(n){if(!confirm('Delete '+n+'?'))return;fetch('/delete?file='+encodeURIComponent(n),{method:'DELETE'}).then(r=>{showSt(r.ok?'Deleted':'Failed',r.ok?'success':'error');if(r.ok)loadFiles()}).catch(e=>showSt('Error','error'))}
function loadScreenshots(){fetch('/screenshots').then(r=>r.json()).then(screenshots=>{const g=document.getElementById('gallery');if(!screenshots||screenshots.length===0){g.innerHTML='<div style="grid-column:1/-1;text-align:center;padding:20px">No screenshots found</div>';return}
g.innerHTML=screenshots.map(s=>'<div class="gallery-item"><div class="gallery-thumb">🖼️</div><div class="gallery-name">'+s.name+'</div><div class="gallery-controls"><button onclick="location.href=\'/screenshot?file='+encodeURIComponent(s.path)+'\'" style="flex:1">⬇️</button><button onclick="delScreenshot(\''+s.path+'\')">🗑️</button></div></div>').join('')}).catch(e=>{document.getElementById('gallery').innerHTML='<div style="grid-column:1/-1;text-align:center;padding:20px">Error loading screenshots</div>';console.error(e)})}
function delScreenshot(path){if(!confirm('Delete screenshot?'))return;fetch('/screenshot?file='+encodeURIComponent(path),{method:'DELETE'}).then(r=>{if(r.ok){loadScreenshots()}}).catch(e=>console.error(e))}
function downloadAllScreenshots(){showSt('Preparing download...','success');fetch('/screenshots').then(r=>r.json()).then(screenshots=>{if(!screenshots||screenshots.length===0){showSt('No screenshots','error');return}
screenshots.forEach((s,i)=>{setTimeout(()=>{const a=document.createElement('a');a.href='/screenshot?file='+encodeURIComponent(s.path);a.download=s.name;a.click()},i*500)})}).catch(e=>showSt('Error','error'))}
function takeScreenshot(){showSt('Taking screenshot...','success');fetch('/screenshot').then(r=>r.blob()).then(b=>{const url=URL.createObjectURL(b);const a=document.createElement('a');a.href=url;a.download='cyd_screen.bmp';a.click();showSt('Screenshot saved!','success');setTimeout(loadScreenshots,500)}).catch(e=>showSt('Screenshot failed','error'))}
document.getElementById('up').addEventListener('submit',e=>{e.preventDefault();const fd=new FormData(),fi=document.getElementById('fi');fd.append('file',fi.files[0]);fd.append('path',curPath);fetch('/upload',{method:'POST',body:fd}).then(r=>{showSt(r.ok?'Uploaded!':'Failed',r.ok?'success':'error');if(r.ok){fi.value='';loadFiles()}}).catch(e=>showSt('Error','error'))});
document.getElementById('wifiForm').addEventListener('submit',e=>{e.preventDefault();const ssid=document.getElementById('ssid').value,pass=document.getElementById('pass').value;fetch('/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,password:pass})}).then(r=>r.text()).then(t=>{showSt(t,'success');loadWifiInfo()}).catch(e=>showSt('WiFi config failed','error'))});
function loadWifiInfo(){fetch('/wifi').then(r=>r.text()).then(t=>document.getElementById('wifiInfo').innerHTML='Current: '+t).catch(e=>{})}
function showSt(m,t){const s=document.getElementById('st');s.textContent=m;s.className='status '+t;s.style.display='block';setTimeout(()=>s.style.display='none',3000)}
loadFiles();loadWifiInfo();loadScreenshots()
</script></body></html>
)rawliteral";

// Load WiFi config from SD card
bool loadWiFiConfig(String &ssid, String &password) {
  if (!SD.begin(SD_CS, sdSPI)) return false;
  
  if (!SD.exists(WIFI_CONFIG_FILE)) {
    SD.end();
    return false;
  }
  
  File file = SD.open(WIFI_CONFIG_FILE, FILE_READ);
  if (!file) {
    SD.end();
    return false;
  }
  
  ssid = file.readStringUntil('\n');
  password = file.readStringUntil('\n');
  ssid.trim();
  password.trim();
  
  file.close();
  SD.end();
  
  return ssid.length() > 0;
}

// Save WiFi config to SD card
bool saveWiFiConfig(const String &ssid, const String &password) {
  if (!SD.begin(SD_CS, sdSPI)) return false;
  
  File file = SD.open(WIFI_CONFIG_FILE, FILE_WRITE);
  if (!file) {
    SD.end();
    return false;
  }
  
  file.println(ssid);
  file.println(password);
  file.close();
  SD.end();
  
  return true;
}

void initializeWebServer() {
  if (!sdCardAvailable) {
    Serial.println("Cannot start web server: SD card not available");
    return;
  }
  
  // Try to load WiFi config from SD card
  String savedSSID, savedPassword;
  if (loadWiFiConfig(savedSSID, savedPassword)) {
    // Try to connect to saved WiFi
    Serial.printf("Connecting to WiFi: %s\n", savedSSID.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    int timeout = 20; // 10 seconds
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(500);
      Serial.print(".");
      timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiMode = "STA";
      wifiIPAddress = WiFi.localIP().toString();
      Serial.printf("\nConnected to WiFi! IP: %s\n", wifiIPAddress.c_str());
    } else {
      Serial.println("\nFailed to connect, starting AP mode");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
      wifiMode = "AP";
      wifiIPAddress = WiFi.softAPIP().toString();
    }
  } else {
    // Start WiFi in AP mode
    Serial.println("Starting WiFi Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    wifiMode = "AP";
    wifiIPAddress = WiFi.softAPIP().toString();
  }
  
  Serial.printf("WiFi Mode: %s\n", wifiMode.c_str());
  Serial.printf("IP Address: %s\n", wifiIPAddress.c_str());
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/list", HTTP_GET, handleFileList);
  server.on("/upload", HTTP_POST, []() {
    server.send(200);
  }, handleFileUpload);
  server.on("/download", HTTP_GET, handleFileDownload);
  server.on("/delete", HTTP_DELETE, handleFileDelete);
  server.on("/screenshot", HTTP_GET, handleScreenshot);
  server.on("/screenshot", HTTP_DELETE, handleScreenshot);
  server.on("/screenshots", HTTP_GET, handleScreenshots);
  server.on("/wifi", HTTP_GET, handleWiFiGet);
  server.on("/wifi", HTTP_POST, handleWiFiPost);
  server.onNotFound(handleNotFound);
  
  server.begin();
  wifiEnabled = true;
  
  Serial.println("Web server started on port 80");
  Serial.printf("Visit http://%s in your browser\n", wifiIPAddress.c_str());
}

void handleWebServer() {
  if (wifiEnabled) {
    server.handleClient();
  }
}

void stopWebServer() {
  if (wifiEnabled) {
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiEnabled = false;
    Serial.println("Web server stopped");
  }
}

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleFileList() {
  String path = server.hasArg("path") ? server.arg("path") : "/";
  
  if (!SD.begin(SD_CS, sdSPI)) {
    server.send(500, "application/json", "[]");
    return;
  }
  
  File root = SD.open(path);
  if (!root || !root.isDirectory()) {
    SD.end();
    server.send(404, "application/json", "[]");
    return;
  }
  
  String json = "[";
  bool first = true;
  
  File file = root.openNextFile();
  while (file) {
    if (!first) json += ",";
    
    String fullPath = path;
    if (!fullPath.endsWith("/")) fullPath += "/";
    fullPath += String(file.name());
    
    json += "{\"name\":\"" + String(file.name()) + "\",";
    json += "\"path\":\"" + fullPath + "\",";
    json += "\"isDir\":" + String(file.isDirectory() ? "true" : "false");
    
    if (!file.isDirectory()) {
      json += ",\"size\":" + String(file.size());
    }
    
    json += "}";
    first = false;
    file = root.openNextFile();
  }
  
  json += "]";
  root.close();
  SD.end();
  
  server.send(200, "application/json", json);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    if (!SD.begin(SD_CS, sdSPI)) {
      Serial.println("SD card mount failed during upload");
      return;
    }
    
    String path = server.hasArg("path") ? server.arg("path") : "/";
    if (!path.endsWith("/")) path += "/";
    String filename = path + upload.filename;
    
    Serial.printf("Upload Start: %s\n", filename.c_str());
    uploadFile = SD.open(filename, FILE_WRITE);
    
    if (!uploadFile) {
      Serial.println("Failed to open file for writing");
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("Upload Complete: %d bytes\n", upload.totalSize);
    }
    SD.end();
  }
}

void handleFileDownload() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file parameter");
    return;
  }
  
  String filename = "/" + server.arg("file");
  
  if (!SD.begin(SD_CS, sdSPI)) {
    server.send(500, "text/plain", "SD card mount failed");
    return;
  }
  
  if (!SD.exists(filename)) {
    server.send(404, "text/plain", "File not found");
    SD.end();
    return;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    server.send(500, "text/plain", "Failed to open file");
    SD.end();
    return;
  }
  
  server.streamFile(file, "application/octet-stream");
  file.close();
  SD.end();
}

void handleFileDelete() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file parameter");
    return;
  }
  
  String filename = "/" + server.arg("file");
  
  if (!SD.begin(SD_CS, sdSPI)) {
    server.send(500, "text/plain", "SD card mount failed");
    return;
  }
  
  if (SD.remove(filename)) {
    server.send(200, "text/plain", "File deleted");
  } else {
    server.send(500, "text/plain", "Failed to delete file");
  }
  
  SD.end();
}

void handleScreenshot() {
  // If file parameter provided, download that screenshot
  if (server.hasArg("file")) {
    String filename = "/" + server.arg("file");
    
    // Check if this is a DELETE request
    if (server.method() == HTTP_DELETE) {
      if (!SD.begin(SD_CS, sdSPI)) {
        server.send(500, "text/plain", "SD card mount failed");
        return;
      }
      
      if (SD.remove(filename)) {
        server.send(200, "text/plain", "Screenshot deleted");
      } else {
        server.send(500, "text/plain", "Failed to delete screenshot");
      }
      
      SD.end();
      return;
    }
    
    // Download screenshot
    if (!SD.begin(SD_CS, sdSPI)) {
      server.send(500, "text/plain", "SD card mount failed");
      return;
    }
    
    if (!SD.exists(filename)) {
      server.send(404, "text/plain", "Screenshot not found");
      SD.end();
      return;
    }
    
    File file = SD.open(filename, FILE_READ);
    if (!file) {
      server.send(500, "text/plain", "Failed to open screenshot");
      SD.end();
      return;
    }
    
    server.streamFile(file, "image/bmp");
    file.close();
    SD.end();
    return;
  }
  
  // Otherwise, capture new screenshot
  // Create BMP header (480x320, 16-bit RGB565)
  const int width = 480;
  const int height = 320;
  const int rowSize = ((width * 2 + 3) / 4) * 4; // Row must be multiple of 4 bytes
  const int imageSize = rowSize * height;
  const int fileSize = 54 + imageSize;
  
  uint8_t bmpHeader[54] = {
    'B', 'M',                       // BM magic
    fileSize & 0xFF, (fileSize >> 8) & 0xFF, (fileSize >> 16) & 0xFF, (fileSize >> 24) & 0xFF,
    0, 0, 0, 0,                     // Reserved
    54, 0, 0, 0,                    // Pixel data offset
    40, 0, 0, 0,                    // DIB header size
    width & 0xFF, (width >> 8) & 0xFF, 0, 0,
    height & 0xFF, (height >> 8) & 0xFF, 0, 0,
    1, 0,                           // Color planes
    16, 0,                          // Bits per pixel (16-bit)
    0, 0, 0, 0,                     // No compression
    imageSize & 0xFF, (imageSize >> 8) & 0xFF, (imageSize >> 16) & 0xFF, (imageSize >> 24) & 0xFF,
    0, 0, 0, 0,                     // X pixels per meter
    0, 0, 0, 0,                     // Y pixels per meter
    0, 0, 0, 0,                     // Colors in palette
    0, 0, 0, 0                      // Important colors
  };
  
  server.setContentLength(fileSize);
  server.send(200, "image/bmp", "");
  
  // Send header
  server.sendContent_P((const char*)bmpHeader, 54);
  
  // Send pixel data (bottom to top for BMP format)
  uint16_t rowBuffer[width];
  for (int y = height - 1; y >= 0; y--) {
    tft.readRect(0, y, width, 1, rowBuffer);
    server.sendContent_P((const char*)rowBuffer, width * 2);
    
    // Add padding if needed
    int padding = rowSize - (width * 2);
    if (padding > 0) {
      uint8_t pad[4] = {0, 0, 0, 0};
      server.sendContent_P((const char*)pad, padding);
    }
  }
  
  Serial.println("Screenshot sent");
}

void handleScreenshots() {
  if (!SD.begin(SD_CS, sdSPI)) {
    server.send(500, "application/json", "[]");
    return;
  }
  
  File root = SD.open("/");
  if (!root) {
    SD.end();
    server.send(500, "application/json", "[]");
    return;
  }
  
  String json = "[";
  bool first = true;
  
  File file = root.openNextFile();
  while (file) {
    String filename = String(file.name());
    
    // Check if it's a .bmp file
    if (!file.isDirectory() && filename.endsWith(".bmp")) {
      if (!first) json += ",";
      
      json += "{\"name\":\"" + filename + "\",";
      json += "\"path\":\"" + filename + "\",";
      json += "\"size\":" + String(file.size()) + "}";
      
      first = false;
    }
    
    file = root.openNextFile();
  }
  
  json += "]";
  root.close();
  SD.end();
  
  server.send(200, "application/json", json);
}

void handleWiFiGet() {
  String info = wifiMode;
  if (wifiMode == "STA") {
    info += " - " + WiFi.SSID() + " (" + wifiIPAddress + ")";
  } else {
    info += " - " + String(WIFI_SSID) + " (" + wifiIPAddress + ")";
  }
  server.send(200, "text/plain", info);
}

void handleWiFiPost() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing JSON body");
    return;
  }
  
  String body = server.arg("plain");
  
  // Simple JSON parsing (no library needed for this simple case)
  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidStart);
  int passStart = body.indexOf("\"password\":\"") + 12;
  int passEnd = body.indexOf("\"", passStart);
  
  if (ssidStart < 8 || ssidEnd < 0 || passStart < 12 || passEnd < 0) {
    server.send(400, "text/plain", "Invalid JSON format");
    return;
  }
  
  String ssid = body.substring(ssidStart, ssidEnd);
  String password = body.substring(passStart, passEnd);
  
  if (saveWiFiConfig(ssid, password)) {
    server.send(200, "text/plain", "WiFi config saved! Restart device to connect.");
    Serial.printf("WiFi config saved: %s\n", ssid.c_str());
  } else {
    server.send(500, "text/plain", "Failed to save WiFi config");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}
