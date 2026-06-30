#include "srv_web.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../driver/drv_led.h"
#include "srv_wifi.h"
#include "srv_ir_ac.h"
#include "srv_ntp.h"
#include "srv_blinker.h"
#include "srv_mqtt.h"
#include "../app_task/task_headers.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

static ESP8266WebServer s_server(80);
static DNSServer s_dns_server;
static bool s_web_started = false;
static uint32_t s_request_count = 0;

/* ===== HTML页面 (PROGMEM) — 工业控制台风 · 合并状态 · 弹窗配置 ===== */
static const char AP_PANEL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
<title>开门猫 - 控制台</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
html,body{height:100%;width:100%}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
color:#c8c8c8;min-height:100vh;background:#141414;
background-image:
  linear-gradient(rgba(255,255,255,.015) 1px,transparent 1px),
  linear-gradient(90deg,rgba(255,255,255,.015) 1px,transparent 1px);
background-size:24px 24px}
.wrap{max-width:480px;margin:0 auto;padding:12px 12px 48px}
.hdr{border-bottom:1px solid #252525;padding:6px 0 14px;margin-bottom:14px}
.hdr h1{font-size:18px;font-weight:700;color:#fff;letter-spacing:2px;text-transform:uppercase}
.hdr .sub{font-size:10px;color:#555;margin-top:4px;letter-spacing:1px}
.card{background:#1a1a1a;border:1px solid #242424;border-radius:3px;margin-bottom:10px}
.card-hd{display:flex;align-items:center;justify-content:space-between;
padding:8px 12px;border-bottom:1px solid #222;background:#1e1e1e}
.card-hd .t{font-size:10px;font-weight:600;color:#888;letter-spacing:1px;text-transform:uppercase}
.card-hd .s{font-size:9px;color:#444;font-family:Consolas,monospace}
.card-bd{padding:10px 12px}

/* 状态行 */
.st-row{display:flex;gap:6px;margin-bottom:12px;flex-wrap:wrap}
.st-tag{display:inline-flex;align-items:center;gap:5px;
background:#1c1c1c;border:1px solid #2a2a2a;border-radius:2px;
padding:4px 8px;font-size:10px;color:#777;font-family:Consolas,Monaco,monospace;cursor:pointer;transition:border-color .15s}
.st-tag:hover{border-color:#ff6b35}
.st-tag .led{width:6px;height:6px;border-radius:1px;background:#444}
.st-tag.ok .led{background:#3bca3b;box-shadow:0 0 5px #3bca3b}
.st-tag.err .led{background:#e53e3e;box-shadow:0 0 5px #e53e3e}
.st-tag.warn .led{background:#e58a00;box-shadow:0 0 5px #e58a00}

/* 系统状态网格 */
.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.cell{background:#141414;border:1px solid #222;border-radius:2px;padding:8px 6px;text-align:center;cursor:pointer;transition:border-color .15s}
.cell:active{border-color:#ff6b35}
.cell .v{font-size:14px;font-weight:600;color:#fff;font-family:Consolas,monospace}
.cell .l{font-size:9px;color:#555;margin-top:3px;text-transform:uppercase;letter-spacing:.5px}
.cell.highlight{border-color:#ff6b35;background:rgba(255,107,53,.05)}
.cell.highlight .v{color:#ff6b35}

/* 门控制 - 含指示灯在右下角 */
.door-wrap{position:relative;text-align:center;padding:20px 0 16px}
.door-btn{width:130px;height:130px;border:2px solid #333;border-radius:4px;cursor:pointer;
font-size:42px;color:#fff;background:#1e1e1e;
box-shadow:0 4px 0 #0a0a0a,inset 0 1px 0 rgba(255,255,255,.03);
display:inline-flex;align-items:center;justify-content:center;
outline:none;-webkit-user-select:none;user-select:none;
-webkit-appearance:none;appearance:none;transition:all .1s;position:relative}
.door-btn:active{transform:translateY(3px);box-shadow:0 1px 0 #0a0a0a}
.door-btn .ic{display:inline-block;transition:transform .15s}
.door-btn.work{background:linear-gradient(180deg,#ff7b47 0%,#d94f15 100%);border-color:#ff6b35;
box-shadow:0 4px 0 #8a3410,inset 0 1px 0 rgba(255,255,255,.12)}
.door-btn.work:active{transform:translateY(3px);box-shadow:0 1px 0 #8a3410}
.door-btn.work .ic{animation:shake .35s ease-in-out infinite}
@keyframes shake{0%,100%{transform:rotate(0)}25%{transform:rotate(-4deg)}75%{transform:rotate(4deg)}}
.door-tx{margin-top:10px;font-size:11px;color:#666;font-weight:500}
.door-msg{margin-top:4px;font-size:10px;color:#444;font-family:Consolas,monospace;min-height:14px}
.door-msg.ok{color:#3bca3b}
.door-msg.fail{color:#e53e3e}

/* 指示灯在门控框右下角 */
.light-ctrl{position:absolute;bottom:16px;right:16px;display:flex;align-items:center;gap:6px}
.light-ctrl .ic{font-size:16px;opacity:.7}
.light-tgl{width:34px;height:18px;border:1px solid #2a2a2a;border-radius:2px;background:#141414;
cursor:pointer;position:relative;outline:none;-webkit-appearance:none;appearance:none;transition:all .15s}
.light-tgl::after{content:'';position:absolute;top:2px;left:2px;
width:10px;height:10px;border-radius:1px;background:#555;transition:all .15s}
.light-tgl.on{background:#0f2a0f;border-color:#1f5a1f}
.light-tgl.on::after{left:20px;background:#3bca3b}

/* 折叠面板 */
.collapse-hd{display:flex;align-items:center;justify-content:space-between;
padding:8px 12px;cursor:pointer;background:#1e1e1e;border-bottom:1px solid #222;
user-select:none;-webkit-user-select:none}
.collapse-hd .l{font-size:10px;font-weight:600;color:#888;letter-spacing:1px;text-transform:uppercase}
.collapse-hd .a{font-size:9px;color:#555;transition:transform .2s}
.collapse-hd.open .a{transform:rotate(180deg)}
.collapse-bd{max-height:0;overflow:hidden;transition:max-height .3s ease}
.collapse-bd.open{max-height:800px}
.collapse-in{padding:10px 12px}

/* 空调控制 */
.ac-ctrl{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-bottom:10px}
.ac-btn{background:#141414;border:1px solid #2a2a2a;border-radius:2px;
padding:8px 4px;font-size:10px;color:#888;cursor:pointer;text-align:center;
outline:none;font-family:inherit;transition:all .1s}
.ac-btn:active{background:#1e1e1e;transform:translateY(1px)}
.ac-btn.sel{border-color:#ff6b35;color:#ff6b35;background:rgba(255,107,53,.08)}
.ac-row{display:flex;align-items:center;justify-content:space-between;padding:6px 0}
.ac-label{font-size:10px;color:#666;text-transform:uppercase;letter-spacing:.5px}
.ac-temp{font-size:22px;font-weight:700;color:#fff;font-family:Consolas,monospace}
.ac-temp-unit{font-size:11px;color:#555;margin-left:2px}
.temp-ctrl{display:flex;align-items:center;gap:8px}
.temp-btn{width:28px;height:28px;border:1px solid #2a2a2a;border-radius:2px;
background:#141414;color:#aaa;font-size:16px;cursor:pointer;outline:none;
display:flex;align-items:center;justify-content:center}
.temp-btn:active{background:#1e1e1e}
.ac-fan{display:flex;gap:4px;margin-top:10px}
.fan-dot{width:10px;height:10px;border:1px solid #2a2a2a;border-radius:1px;background:#141414}
.fan-dot.sel{border-color:#ff6b35;background:#ff6b35}
.tgl{width:40px;height:22px;border:1px solid #2a2a2a;border-radius:2px;background:#141414;
cursor:pointer;position:relative;outline:none;flex-shrink:0;
-webkit-appearance:none;appearance:none;transition:all .15s}
.tgl::after{content:'';position:absolute;top:2px;left:2px;
width:14px;height:14px;border-radius:1px;background:#555;transition:all .15s}
.tgl.on{background:#0f2a0f;border-color:#1f5a1f}
.tgl.on::after{left:22px;background:#3bca3b}

/* 弹窗遮罩 */
.modal-mask{display:none;position:fixed;top:0;left:0;right:0;bottom:0;
background:rgba(0,0,0,.75);z-index:999;align-items:center;justify-content:center;padding:20px}
.modal-mask.show{display:flex}
.modal{background:#1a1a1a;border:1px solid #2a2a2a;border-radius:4px;width:100%;max-width:380px;
max-height:90vh;overflow-y:auto;-webkit-overflow-scrolling:touch}
.modal-hd{display:flex;align-items:center;justify-content:space-between;
padding:10px 14px;border-bottom:1px solid #222;background:#1e1e1e}
.modal-hd .t{font-size:12px;font-weight:600;color:#ccc;letter-spacing:.5px}
.modal-close{background:none;border:none;color:#666;font-size:18px;cursor:pointer;
padding:0 4px;line-height:1;outline:none;font-family:inherit}
.modal-bd{padding:14px}

.wifi-list{border:1px solid #222;border-radius:2px;height:220px;overflow-y:auto;
-webkit-overflow-scrolling:touch;margin-bottom:10px;background:#111}
.wifi-item{padding:8px 10px;border-bottom:1px solid #1a1a1a;cursor:pointer;
display:flex;justify-content:space-between;align-items:center;font-size:12px;color:#999;
transition:background .1s}
.wifi-item:last-child{border-bottom:none}
.wifi-item:active{background:#1e1e1e}
.wifi-item.sel{background:#1e2a1e;color:#3bca3b}
.rssi{font-size:10px;color:#555;font-family:Consolas,monospace}
.wifi-hint{text-align:center;font-size:11px;color:#555;padding:18px 0;font-family:Consolas,monospace}

.inp-grp{margin-bottom:8px}
.inp-grp label{font-size:10px;color:#666;text-transform:uppercase;letter-spacing:.5px;display:block;margin-bottom:4px}
.inp-grp input{width:100%;padding:10px 12px;background:#111;border:1px solid #2a2a2a;
border-radius:2px;font-size:12px;color:#ccc;outline:none;font-family:inherit;
transition:border-color .15s}
.inp-grp input:focus{border-color:#ff6b35}
.inp-grp input::placeholder{color:#444}
.btn{display:block;width:100%;padding:10px;border:1px solid #2a2a2a;border-radius:2px;
font-size:11px;font-weight:600;text-align:center;cursor:pointer;outline:none;
font-family:inherit;background:#1e1e1e;color:#aaa;transition:all .1s;letter-spacing:.5px}
.btn:active{background:#141414;transform:translateY(1px)}
.btn-pri{background:#ff6b35;border-color:#d94f15;color:#fff}
.btn-pri:active{background:#d94f15}
.btn-pri:disabled{background:#3a3a3a;border-color:#2a2a2a;color:#555;cursor:not-allowed}
.btn-dng{background:#1e0f0f;border-color:#3a1a1a;color:#e53e3e;margin-top:6px}
.btn-dng:active{background:#140a0a}
.scan-row{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px}
.scan-hint{font-size:10px;color:#555;font-family:Consolas,monospace}
.scan-btn{font-size:10px;color:#ff6b35;cursor:pointer;text-transform:uppercase;letter-spacing:.5px;
background:none;border:none;padding:0;outline:none;font-family:inherit}
.scan-btn:disabled{color:#444;cursor:not-allowed}

.ft{text-align:center;font-size:9px;color:#333;margin-top:14px;font-family:Consolas,monospace;letter-spacing:.5px}

.step-hint{font-size:10px;color:#555;margin-bottom:10px;padding:6px 8px;
background:#111;border:1px solid #222;border-radius:2px;font-family:Consolas,monospace}
.cur-info{margin-bottom:10px;padding:10px 12px;background:#0f1a0f;
border:1px solid #1a3a1a;border-radius:2px}
.cur-label{font-size:9px;color:#3bca3b;text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px}
.cur-val{font-size:13px;color:#fff;font-family:Consolas,monospace;word-break:break-all}
</style>
</head>
<body>
<div class="wrap">
  <div class="hdr">
    <h1>开门猫</h1>
    <div class="sub">OPEN-DOOR-CAT CONTROL TERMINAL v0.0.3</div>
  </div>
  <div class="card">
    <div class="card-hd"><span class="t">系统状态</span><span class="s" id="sysTime">--:--:--</span></div>
    <div class="card-bd">
      <div class="st-row" id="statusRow">
        <span class="st-tag" id="wifiTag" onclick="openWifiModal()"><span class="led"></span><span id="wifiTxt">WiFi</span></span>
        <span class="st-tag" id="mqttTag" onclick="openMqttModal()"><span class="led"></span><span id="mqttTxt">MQTT</span></span>
        <span class="st-tag" id="blinkerTag" onclick="openBlinkerModal()"><span class="led"></span><span id="blinkTxt">点灯</span></span>
        <span class="st-tag" id="ntpTag"><span class="led"></span><span id="ntpTxt">NTP</span></span>
      </div>
      <div class="grid">
        <div class="cell" onclick="openWifiModal()"><div class="v" id="valRssi">--</div><div class="l">信号dBm</div></div>
        <div class="cell" onclick="openWifiModal()"><div class="v" id="valIp">---.---.---.---</div><div class="l">IP地址</div></div>
        <div class="cell highlight"><div class="v" id="valHeartbeat">#--</div><div class="l">心跳</div></div>
        <div class="cell"><div class="v" id="valWebReq">--</div><div class="l">Web请求</div></div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="card-hd"><span class="t">门控制</span></div>
    <div class="card-bd door-wrap">
      <button class="door-btn" id="doorBtn" onclick="openDoor()"><span class="ic">&#128682;</span></button>
      <div class="door-tx">点击开门</div>
      <div class="door-msg" id="doorMsg"></div>
      <div class="light-ctrl">
        <span class="ic">&#128161;</span>
        <button class="light-tgl" id="lightTgl" onclick="toggleLight()"></button>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="collapse-hd open" id="acHd" onclick="toggleCollapse('ac')">
      <span class="l">空调控制</span>
      <span class="a">&#9660;</span>
    </div>
    <div class="collapse-bd open" id="acBd">
      <div class="collapse-in">
        <div class="ac-row">
          <span class="ac-label">电源</span>
          <button class="tgl" id="acPwr" onclick="toggleAcPower()"></button>
        </div>
        <div class="ac-ctrl">
          <button class="ac-btn" id="acCool" onclick="setAcMode('cool')">制冷</button>
          <button class="ac-btn" id="acDry" onclick="setAcMode('dry')">除湿</button>
          <button class="ac-btn" id="acHot" onclick="setAcMode('hot')">制热</button>
          <button class="ac-btn" id="acAuto" onclick="setAcMode('auto')">自动</button>
          <button class="ac-btn" id="acFan" onclick="setAcMode('fan')">送风</button>
        </div>
        <div class="ac-row">
          <span class="ac-label">温度</span>
          <div class="temp-ctrl">
            <button class="temp-btn" onclick="adjAcTemp(-1)">-</button>
            <span class="ac-temp"><span id="acTempVal">26</span><span class="ac-temp-unit">°C</span></span>
            <button class="temp-btn" onclick="adjAcTemp(1)">+</button>
          </div>
        </div>
        <div class="ac-fan" id="acFanLevel">
          <span class="fan-dot"></span><span class="fan-dot"></span><span class="fan-dot"></span><span class="fan-dot"></span><span class="fan-dot"></span>
        </div>
      </div>
    </div>
  </div>
  <div class="ft">ESP8266-01S | 1MB Flash</div>
</div>

<!-- WiFi配网弹窗 -->
<div class="modal-mask" id="wifiModal">
  <div class="modal">
    <div class="modal-hd">
      <span class="t">WiFi 配置</span>
      <button class="modal-close" onclick="closeWifiModal()">&times;</button>
    </div>
    <div class="modal-bd">
      <div class="cur-info" id="wifiCurInfo">
        <div class="cur-label">当前连接</div>
        <div class="cur-val" id="wifiCurSsid">--</div>
      </div>
      <div class="step-hint">点击扫描查找可用网络，或直接输入WiFi信息</div>
      <div class="scan-row">
        <span class="scan-hint">可用网络</span>
        <button class="scan-btn" id="scanBtn" onclick="scanWifi()">&#8635; 扫描</button>
      </div>
      <div class="wifi-list" id="wifiList"><div class="wifi-hint">点击上方扫描按钮搜索WiFi</div></div>
      <div class="inp-grp">
        <label>WiFi 名称</label>
        <input type="text" id="ssid" placeholder="SSID">
      </div>
      <div class="inp-grp">
        <label>WiFi 密码</label>
        <input type="password" id="pass" placeholder="留空表示无密码">
      </div>
      <button class="btn btn-pri" id="saveWifiBtn" onclick="saveWifi()">保存并连接</button>
      <button class="btn btn-dng" onclick="clearWifiConfig()">清除WiFi配置</button>
    </div>
  </div>
</div>

<!-- Blinker配置弹窗 -->
<div class="modal-mask" id="blinkerModal">
  <div class="modal">
    <div class="modal-hd">
      <span class="t">点灯科技 配置</span>
      <button class="modal-close" onclick="closeBlinkerModal()">&times;</button>
    </div>
    <div class="modal-bd">
      <div class="cur-info" id="blinkerCurInfo">
        <div class="cur-label">当前密钥</div>
        <div class="cur-val" id="blinkerCurAuth">--</div>
      </div>
      <div class="step-hint">请输入点灯科技APP中的设备密钥</div>
      <div class="inp-grp">
        <label>设备密钥 (Auth Key)</label>
        <input type="text" id="blinkerAuth" placeholder="请输入密钥" maxlength="32">
      </div>
      <button class="btn btn-pri" id="saveBlinkerBtn" onclick="saveBlinker()">保存并连接</button>
      <button class="btn btn-dng" onclick="clearBlinkerConfig()">清除点灯配置</button>
    </div>
  </div>
</div>

<!-- MQTT配置弹窗 -->
<div class="modal-mask" id="mqttModal">
  <div class="modal">
    <div class="modal-hd">
      <span class="t">MQTT 配置</span>
      <button class="modal-close" onclick="closeMqttModal()">&times;</button>
    </div>
    <div class="modal-bd">
      <div class="cur-info" id="mqttCurInfo">
        <div class="cur-label">当前服务器</div>
        <div class="cur-val" id="mqttCurHost">--</div>
      </div>
      <div class="step-hint">配置公共MQTT服务器，订阅主题接收开门指令</div>
      <div class="inp-grp">
        <label>服务器地址</label>
        <input type="text" id="mqttHost" placeholder="broker.emqx.io" maxlength="64">
      </div>
      <div class="inp-grp">
        <label>端口</label>
        <input type="text" id="mqttPort" placeholder="1883" maxlength="6">
      </div>
      <div class="inp-grp">
        <label>用户名（可选）</label>
        <input type="text" id="mqttUser" placeholder="留空表示无认证" maxlength="32">
      </div>
      <div class="inp-grp">
        <label>密码（可选）</label>
        <input type="password" id="mqttPass" placeholder="留空表示无密码" maxlength="32">
      </div>
      <div class="inp-grp">
        <label>订阅主题</label>
        <input type="text" id="mqttTopic" placeholder="esp8266/opendoor/door" maxlength="64">
      </div>
      <div class="inp-grp">
        <label>客户端ID</label>
        <input type="text" id="mqttClient" placeholder="留空自动生成" maxlength="32">
      </div>
      <button class="btn btn-pri" id="saveMqttBtn" onclick="saveMqtt()">保存并连接</button>
      <button class="btn btn-dng" onclick="clearMqttConfig()">清除MQTT配置</button>
    </div>
  </div>
</div>

<script>
var _poll=null,_hb=0,_needWifiModal=false,_needBlinkerModal=false,_status=null;

function poll(){
  fetch('/api/status').then(function(r){return r.json()}).then(function(d){
    _status=d;
    /* WiFi状态标签 */
    var wt=document.getElementById('wifiTag');
    if(d.wifi){wt.className='st-tag ok';document.getElementById('wifiTxt').textContent='WiFi已连'}
    else{wt.className='st-tag err';document.getElementById('wifiTxt').textContent='WiFi断开'}

    /* MQTT状态标签 */
    var mt=document.getElementById('mqttTag');
    if(!d.hasMqttConfig){mt.className='st-tag';document.getElementById('mqttTxt').textContent='MQTT'}
    else if(d.mqtt){mt.className='st-tag ok';document.getElementById('mqttTxt').textContent='MQTT已连'}
    else{mt.className='st-tag warn';document.getElementById('mqttTxt').textContent='MQTT连接中'}

    /* Blinker状态标签 */
    var bt=document.getElementById('blinkerTag');
    if(!d.hasBlinkerConfig){bt.className='st-tag warn';document.getElementById('blinkTxt').textContent='点灯未配置'}
    else if(d.blinker){bt.className='st-tag ok';document.getElementById('blinkTxt').textContent='点灯已连'}
    else{bt.className='st-tag warn';document.getElementById('blinkTxt').textContent='点灯连接中'}

    /* NTP状态 */
    var nt=document.getElementById('ntpTag');
    if(d.ntp){nt.className='st-tag ok';document.getElementById('ntpTxt').textContent='NTP同步'}
    else{nt.className='st-tag';document.getElementById('ntpTxt').textContent='NTP未同步'}

    /* 系统数值 */
    document.getElementById('valRssi').textContent=d.rssi||'--';
    document.getElementById('valIp').textContent=d.ip||'---.---.---.---';
    document.getElementById('valWebReq').textContent=d.webReq||'0';

    /* 心跳 */
    if(d.heartbeat&&d.heartbeat!==_hb){_hb=d.heartbeat;document.getElementById('valHeartbeat').textContent='#'+_hb}
    if(d.time)document.getElementById('sysTime').textContent=d.time;

    /* 灯光 */
    var lt=document.getElementById('lightTgl');
    lt.className=d.light?'light-tgl on':'light-tgl';

    /* 门状态 */
    var db=document.getElementById('doorBtn'),dm=document.getElementById('doorMsg');
    if(d.opening){db.className='door-btn work';dm.textContent='开门中...';dm.className='door-msg ok'}
    else if(db.className.indexOf('work')>=0){db.className='door-btn';dm.textContent=''}

    /* 空调 */
    document.getElementById('acPwr').className=d.acPower?'tgl on':'tgl';
    document.getElementById('acTempVal').textContent=d.acTemp||'26';
    var m=d.acMode||'auto';
    ['cool','dry','hot','auto','fan'].forEach(function(x){
      var b=document.getElementById('ac'+x.charAt(0).toUpperCase()+x.slice(1));
      b.className=x===m?'ac-btn sel':'ac-btn';
    });
    var fl=parseInt(d.acFan)||1;
    var fd=document.getElementById('acFanLevel').querySelectorAll('.fan-dot');
    fd.forEach(function(el,i){el.className=i<fl?'fan-dot sel':'fan-dot'});

    /* 首次加载时判断是否需要弹出配置弹窗 */
    if(_needWifiModal===false&&_needBlinkerModal===false){
      _needWifiModal=!d.wifi&&!d.hasWifiConfig;
      _needBlinkerModal=d.wifi&&!d.hasBlinkerConfig;
      if(_needWifiModal){openWifiModal()}
      else if(_needBlinkerModal){openBlinkerModal()}
    }
  }).catch(function(){})
}
poll();
_poll=setInterval(poll,2000);

/* ===== 门控制 ===== */
function openDoor(){
  var b=document.getElementById('doorBtn'),m=document.getElementById('doorMsg');
  if(b.className.indexOf('work')>=0)return;
  b.className='door-btn work';m.textContent='发送指令...';m.className='door-msg';
  fetch('/action_open').then(function(r){return r.text()}).then(function(){
    m.textContent='指令已执行';m.className='door-msg ok';
    setTimeout(function(){if(document.getElementById('doorBtn').className.indexOf('work')<0)m.textContent=''},4000);
  }).catch(function(){m.textContent='请求失败';m.className='door-msg fail';b.className='door-btn'})
}
function toggleLight(){
  fetch('/action_light').then(function(r){return r.text()}).then(function(){poll()}).catch(function(){})
}

/* ===== 折叠面板 ===== */
function toggleCollapse(id){
  var h=document.getElementById(id+'Hd'),b=document.getElementById(id+'Bd');
  if(h.className.indexOf('open')>=0){h.className='collapse-hd';b.className='collapse-bd'}
  else{h.className='collapse-hd open';b.className='collapse-bd open'}
}

/* ===== WiFi弹窗 ===== */
function openWifiModal(){
  document.getElementById('wifiModal').className='modal-mask show';
  _needWifiModal=false;
  // 显示当前配置
  var ssid=document.getElementById('wifiCurSsid');
  if(_status && _status.ssid){
    ssid.textContent=_status.ssid;
    document.getElementById('ssid').value=_status.ssid;
  } else {
    ssid.textContent='未配置';
  }
}
function closeWifiModal(){
  document.getElementById('wifiModal').className='modal-mask';
}
function selWifi(s){
  document.getElementById('ssid').value=s;
  document.querySelectorAll('.wifi-item').forEach(function(i){i.className='wifi-item'});
  if(event && event.target){
    var it=event.target.closest('.wifi-item');
    if(it)it.className='wifi-item sel';
  }
}
function scanWifi(){
  var e=document.getElementById('scanBtn'),l=document.getElementById('wifiList');
  e.disabled=true;e.textContent='扫描中...';
  l.innerHTML='<div class="wifi-hint">扫描中，请稍候...</div>';
  fetch('/api/scan').then(function(r){return r.json()}).then(function(d){
    if(!d||!d.length){l.innerHTML='<div class="wifi-hint">未发现WiFi信号</div>';e.disabled=false;e.textContent='&#8635; 重新扫描';return}
    var h='';for(var i=0;i<d.length;i++){
      var sf=d[i].s.replace(/'/g,"\\'");
      h+='<div class="wifi-item" onclick="selWifi(\''+sf+'\')"><span>'+d[i].s+'</span><span class="rssi">'+d[i].r+'dBm</span></div>';
    }
    l.innerHTML=h;e.disabled=false;e.textContent='&#8635; 重新扫描';
  }).catch(function(){l.innerHTML='<div class="wifi-hint">扫描失败</div>';e.disabled=false;e.textContent='&#8635; 重试'})
}
function saveWifi(){
  var s=document.getElementById('ssid').value;
  var p=document.getElementById('pass').value;
  if(!s){alert('请输入WiFi名称');return}
  var btn=document.getElementById('saveWifiBtn');
  btn.disabled=true;btn.textContent='保存中...';
  fetch('/save?ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)).then(function(){
    document.body.innerHTML='<div style="text-align:center;padding:80px 20px;color:#888;font-family:Consolas,monospace;font-size:14px"><div style="color:#ff6b35;font-size:20px;margin-bottom:16px">配置已保存</div><div>设备重启中...</div></div>';
  }).catch(function(){location.reload()})
}
function clearWifiConfig(){
  if(!confirm('确定清除WiFi配置？设备将重启。'))return;
  fetch('/clear').then(function(){
    document.body.innerHTML='<div style="text-align:center;padding:80px 20px;color:#888;font-family:Consolas,monospace;font-size:14px"><div style="color:#e53e3e;font-size:20px;margin-bottom:16px">配置已清除</div><div>设备重启中...</div></div>';
  }).catch(function(){location.reload()})
}

/* ===== Blinker弹窗 ===== */
function openBlinkerModal(){
  document.getElementById('blinkerModal').className='modal-mask show';
  _needBlinkerModal=false;
  // 显示当前配置
  var authEl=document.getElementById('blinkerCurAuth');
  if(_status && _status.blinkerAuth){
    authEl.textContent=_status.blinkerAuth;
    document.getElementById('blinkerAuth').value=_status.blinkerAuth;
  } else {
    authEl.textContent='未配置';
  }
}
function closeBlinkerModal(){
  document.getElementById('blinkerModal').className='modal-mask';
}
function saveBlinker(){
  var k=document.getElementById('blinkerAuth').value.trim();
  if(!k){alert('请输入设备密钥');return}
  var btn=document.getElementById('saveBlinkerBtn');
  btn.disabled=true;btn.textContent='保存中...';
  fetch('/save_blinker?auth='+encodeURIComponent(k)).then(function(){
    document.body.innerHTML='<div style="text-align:center;padding:80px 20px;color:#888;font-family:Consolas,monospace;font-size:14px"><div style="color:#ff6b35;font-size:20px;margin-bottom:16px">配置已保存</div><div>设备重启中...</div></div>';
  }).catch(function(){location.reload()})
}
function clearBlinkerConfig(){
  if(!confirm('确定清除点灯科技配置？设备将重启。'))return;
  fetch('/clear_blinker').then(function(){
    document.body.innerHTML='<div style="text-align:center;padding:80px 20px;color:#888;font-family:Consolas,monospace;font-size:14px"><div style="color:#e53e3e;font-size:20px;margin-bottom:16px">配置已清除</div><div>设备重启中...</div></div>';
  }).catch(function(){location.reload()})
}

/* ===== MQTT弹窗 ===== */
function openMqttModal(){
  document.getElementById('mqttModal').className='modal-mask show';
  var hostEl=document.getElementById('mqttCurHost');
  if(_status && _status.mqttHost){
    hostEl.textContent=_status.mqttHost+':'+(_status.mqttPort||'1883');
    document.getElementById('mqttHost').value=_status.mqttHost||'';
    document.getElementById('mqttPort').value=_status.mqttPort||'';
    document.getElementById('mqttTopic').value=_status.mqttTopic||'';
    document.getElementById('mqttClient').value=_status.mqttClient||'';
  } else {
    /* 显示默认配置 */
    hostEl.textContent='broker.emqx.io:1883 (默认)';
    document.getElementById('mqttHost').value='broker.emqx.io';
    document.getElementById('mqttPort').value='1883';
    document.getElementById('mqttTopic').value='esp8266/opendoor/door';
    document.getElementById('mqttClient').value='';
  }
}
function closeMqttModal(){
  document.getElementById('mqttModal').className='modal-mask';
}
function saveMqtt(){
  var h=document.getElementById('mqttHost').value.trim();
  if(!h){alert('请输入服务器地址');return}
  var p=document.getElementById('mqttPort').value.trim()||'1883';
  var u=document.getElementById('mqttUser').value.trim();
  var w=document.getElementById('mqttPass').value.trim();
  var t=document.getElementById('mqttTopic').value.trim()||'esp8266/opendoor/door';
  var c=document.getElementById('mqttClient').value.trim();
  var btn=document.getElementById('saveMqttBtn');
  btn.disabled=true;btn.textContent='保存中...';
  var url='/save_mqtt?host='+encodeURIComponent(h)+'&port='+encodeURIComponent(p)
    +'&user='+encodeURIComponent(u)+'&pass='+encodeURIComponent(w)
    +'&topic='+encodeURIComponent(t)+'&client='+encodeURIComponent(c);
  fetch(url).then(function(){
    document.body.innerHTML='<div style="text-align:center;padding:80px 20px;color:#888;font-family:Consolas,monospace;font-size:14px"><div style="color:#ff6b35;font-size:20px;margin-bottom:16px">配置已保存</div><div>设备重启中...</div></div>';
  }).catch(function(){location.reload()})
}
function clearMqttConfig(){
  if(!confirm('确定清除MQTT配置？设备将重启。'))return;
  fetch('/clear_mqtt').then(function(){
    document.body.innerHTML='<div style="text-align:center;padding:80px 20px;color:#888;font-family:Consolas,monospace;font-size:14px"><div style="color:#e53e3e;font-size:20px;margin-bottom:16px">配置已清除</div><div>设备重启中...</div></div>';
  }).catch(function(){location.reload()})
}

/* ===== 空调控制 ===== */
function toggleAcPower(){
  fetch('/action_ac_power').then(function(r){return r.text()}).then(function(){poll()}).catch(function(){})
}
function setAcMode(m){
  fetch('/action_ac_mode?m='+m).then(function(r){return r.text()}).then(function(){poll()}).catch(function(){})
}
function adjAcTemp(d){
  var cur=parseInt(document.getElementById('acTempVal').textContent)||26;
  var nv=cur+d;if(nv<16||nv>30)return;
  fetch('/action_ac_temp?t='+nv).then(function(r){return r.text()}).then(function(){poll()}).catch(function(){})
}
</script>
</body>
</html>
)rawliteral";

/* ===== API处理函数 ===== */

static void handle_root(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /");

    s_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    s_server.send(200, "text/html; charset=utf-8", "");
    s_server.sendContent_P(AP_PANEL_HTML);
}

static void handle_api_status(void)
{
    s_request_count++;

    String json = "{";
    /* WiFi状态 */
    json += "\"wifi\":" + String(srv_wifi_is_connected() ? "true" : "false");
    json += ",\"rssi\":\"" + String(srv_wifi_get_rssi()) + "\"";
    json += ",\"ip\":\"" + String(srv_wifi_get_ip()) + "\"";
    json += ",\"ssid\":\"" + String(srv_wifi_get_ssid()) + "\"";
    json += ",\"hasWifiConfig\":" + String(srv_wifi_has_config() ? "true" : "false");
    /* MQTT状态 */
    json += ",\"mqtt\":" + String(srv_mqtt_is_connected() ? "true" : "false");
    json += ",\"mqttHost\":\"" + String(srv_mqtt_get_host()) + "\"";
    json += ",\"mqttPort\":\"" + String(srv_mqtt_get_port()) + "\"";
    json += ",\"mqttTopic\":\"" + String(srv_mqtt_get_topic()) + "\"";
    json += ",\"mqttPubTopic\":\"" + String(srv_mqtt_get_pub_topic()) + "\"";
    json += ",\"mqttClient\":\"" + String(srv_mqtt_get_client_id()) + "\"";
    json += ",\"hasMqttConfig\":" + String(srv_mqtt_has_config() ? "true" : "false");
    /* 点灯状态 */
    json += ",\"blinker\":" + String(srv_blinker_is_ready() ? "true" : "false");
    json += ",\"blinkerAuth\":\"" + String(srv_blinker_get_auth()) + "\"";
    json += ",\"hasBlinkerConfig\":" + String(srv_blinker_has_config() ? "true" : "false");
    /* NTP状态 */
    json += ",\"ntp\":" + String(srv_ntp_is_synced() ? "true" : "false");
    /* 系统时间 */
    struct tm t;
    if (srv_ntp_get_time(&t)) {
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
        json += ",\"time\":\"" + String(time_str) + "\"";
    }
    /* 心跳计数 */
    json += ",\"heartbeat\":" + String(task_uart_get_heartbeat_count());
    /* Web请求计数 */
    json += ",\"webReq\":" + String(s_request_count);
    /* 灯光状态 */
    json += ",\"light\":" + String(drv_led_get_brightness() > 0 ? "true" : "false");
    /* 门状态 */
    json += ",\"opening\":" + String(task_door_is_opening() ? "true" : "false");
    /* 空调状态 */
    json += ",\"acPower\":" + String(srv_ir_ac_get_power() ? "true" : "false");
    json += ",\"acTemp\":" + String(srv_ir_ac_get_temp());
    const char *mode_en = "auto";
    switch (srv_ir_ac_get_mode()) {
        case AC_MODE_COOL: mode_en = "cool"; break;
        case AC_MODE_DRY:  mode_en = "dry"; break;
        case AC_MODE_HEAT: mode_en = "hot"; break;
        case AC_MODE_AUTO: mode_en = "auto"; break;
        case AC_MODE_FAN:  mode_en = "fan"; break;
    }
    json += ",\"acMode\":\"" + String(mode_en) + "\"";
    json += ",\"acFan\":\"" + String(srv_ir_ac_get_fan_level()) + "\"";
    json += "}";

    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "application/json; charset=utf-8", json);
}

static void handle_api_scan(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /api/scan (WiFi扫描)");
    srv_wifi_scan_start();
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "application/json; charset=utf-8", srv_wifi_get_scan_cache());
}

static void handle_action_open(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /action_open (开门)");
    door_send_cmd(DOOR_CMD_OPEN);
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "开门指令已执行");
}

static void handle_action_light(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /action_light (切换灯光)");
    task_led_manual_toggle();
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "灯光已切换");
}

static void handle_action_ac_power(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /action_ac_power (空调电源)");
    bool current = srv_ir_ac_get_power();
    srv_ir_ac_set_power(!current);
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", current ? "空调已关闭" : "空调已开启");
}

static void handle_action_ac_mode(void)
{
    s_request_count++;
    String m = s_server.arg("m");
    LOG_I("Web请求: GET /action_ac_mode?m=%s", m.c_str());

    ac_mode_t mode = AC_MODE_AUTO;
    if (m == "cool") mode = AC_MODE_COOL;
    else if (m == "dry") mode = AC_MODE_DRY;
    else if (m == "hot") mode = AC_MODE_HEAT;
    else if (m == "fan") mode = AC_MODE_FAN;
    else mode = AC_MODE_AUTO;

    srv_ir_ac_set_mode(mode);
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "模式已切换");
}

static void handle_action_ac_temp(void)
{
    s_request_count++;
    int t = s_server.arg("t").toInt();
    LOG_I("Web请求: GET /action_ac_temp?t=%d", t);
    if (t >= 16 && t <= 30) {
        srv_ir_ac_set_temp(t);
        s_server.sendHeader("Access-Control-Allow-Origin", "*");
        s_server.send(200, "text/plain; charset=utf-8", "温度已设置");
    } else {
        s_server.sendHeader("Access-Control-Allow-Origin", "*");
        s_server.send(400, "text/plain; charset=utf-8", "温度无效");
    }
}

static void handle_save(void)
{
    s_request_count++;
    String ssid = s_server.arg("ssid");
    String pass = s_server.arg("pass");
    LOG_I("Web请求: GET /save SSID=%s", ssid.c_str());

    if (ssid.length() == 0 || ssid.length() >= 32) {
        s_server.send(400, "text/plain", "SSID无效");
        return;
    }

    wifi_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ssid, ssid.c_str(), sizeof(cfg.ssid) - 1);
    strncpy(cfg.pass, pass.c_str(), sizeof(cfg.pass) - 1);
    cfg.magic = EEPROM_WIFI_MAGIC;

    srv_wifi_save_config(&cfg);
    LOG_I("WiFi配置已保存: %s", cfg.ssid);

    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "OK");
    delay(300);
    ESP.restart();
}

static void handle_clear(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /clear (清除WiFi配置)");
    srv_wifi_clear_config();
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "OK");
    delay(300);
    ESP.restart();
}

static void handle_save_blinker(void)
{
    s_request_count++;
    String auth = s_server.arg("auth");
    LOG_I("Web请求: GET /save_blinker (保存点灯配置)");

    if (auth.length() == 0 || auth.length() > EEPROM_BLINKER_AUTH_MAX) {
        s_server.send(400, "text/plain", "密钥无效");
        return;
    }

    srv_blinker_save_config(auth.c_str());

    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "OK");
    delay(300);
    ESP.restart();
}

static void handle_clear_blinker(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /clear_blinker (清除点灯配置)");
    srv_blinker_clear_config();
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "OK");
    delay(300);
    ESP.restart();
}

static void handle_save_mqtt(void)
{
    s_request_count++;
    String host = s_server.arg("host");
    String port = s_server.arg("port");
    String user = s_server.arg("user");
    String pass = s_server.arg("pass");
    String topic = s_server.arg("topic");
    String client = s_server.arg("client");
    LOG_I("Web请求: GET /save_mqtt (保存MQTT配置)");

    if (host.length() == 0 || host.length() > EEPROM_MQTT_HOST_MAX) {
        s_server.send(400, "text/plain", "服务器地址无效");
        return;
    }

    srv_mqtt_save_config(
        host.c_str(),
        port.c_str(),
        user.c_str(),
        pass.c_str(),
        topic.c_str(),
        client.c_str()
    );

    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "OK");
    delay(300);
    ESP.restart();
}

static void handle_clear_mqtt(void)
{
    s_request_count++;
    LOG_I("Web请求: GET /clear_mqtt (清除MQTT配置)");
    srv_mqtt_clear_config();
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "text/plain; charset=utf-8", "OK");
    delay(300);
    ESP.restart();
}

static void handle_not_found(void)
{
    s_request_count++;
    String uri = s_server.uri();
    String method = (s_server.method() == HTTP_GET) ? "GET" : "POST";
    LOG_I("Web请求: %s %s (404)", method.c_str(), uri.c_str());

    if (s_server.method() == HTTP_GET) {
        s_server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
        s_server.send(302, "text/plain", "");
    } else {
        s_server.send(404, "text/plain", "Not Found");
    }
}

/* ===== 公共接口 ===== */

err_code_t srv_web_init(void)
{
    s_server.on("/", HTTP_GET, handle_root);
    s_server.on("/api/status", HTTP_GET, handle_api_status);
    s_server.on("/api/scan", HTTP_GET, handle_api_scan);
    s_server.on("/action_open", HTTP_GET, handle_action_open);
    s_server.on("/action_light", HTTP_GET, handle_action_light);
    s_server.on("/action_ac_power", HTTP_GET, handle_action_ac_power);
    s_server.on("/action_ac_mode", HTTP_GET, handle_action_ac_mode);
    s_server.on("/action_ac_temp", HTTP_GET, handle_action_ac_temp);
    s_server.on("/save", HTTP_GET, handle_save);
    s_server.on("/clear", HTTP_GET, handle_clear);
    s_server.on("/save_blinker", HTTP_GET, handle_save_blinker);
    s_server.on("/clear_blinker", HTTP_GET, handle_clear_blinker);
    s_server.on("/save_mqtt", HTTP_GET, handle_save_mqtt);
    s_server.on("/clear_mqtt", HTTP_GET, handle_clear_mqtt);
    s_server.onNotFound(handle_not_found);

    s_server.begin();
    s_dns_server.setErrorReplyCode(DNSReplyCode::NoError);
    s_dns_server.start(53, "*", WiFi.softAPIP());

    s_web_started = true;
    LOG_I("Web服务器已启动, 端口80, DNS劫持已启用");
    return APP_OK;
}

void srv_web_run(void)
{
    if (s_web_started) {
        s_server.handleClient();
        s_dns_server.processNextRequest();
    }
}

uint32_t srv_web_get_request_count(void)
{
    return s_request_count;
}
