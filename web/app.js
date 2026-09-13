/* 灶君 Web 控制台：对应板端 ui_main.c + pages/
 * 侧栏导航表与 page_show() 的切换逻辑、设备控制、环境信息全部在此实现。
 * 数据来源统一走 MQTT（协议契约见 net/mqtt_client.c），视图只读 store。
 */
(function () {
  'use strict';

  /* ---------- 连接配置（默认与板端 mqtt_client.c 一致） ---------- */
  var cfg = {
    host: 'broker.hivemq.com',
    port: '8000',
    path: '/mqtt',
    prefix: 'home/zaojun_imx6ull'
  };

  /* ---------- 状态模型：对应板端 data/dev_status + env_status ---------- */
  var store = { connected: false, led: null, ac: null, temp: null, humi: null, time: '--' };

  /* ---------- 导航表：顺序与板端 pages[] 完全一致 ---------- */
  var PAGES = [
    { id: 'dashboard', icon: '🏠', name: '主界面',     render: renderDashboard },
    { id: 'settings',  icon: '⚙️', name: '设置',       render: renderSettings },
    { id: 'log',       icon: '📜', name: '日志管理',   render: renderLog },
    { id: 'camera',    icon: '📷', name: '查看摄像头', render: renderStub },
    { id: 'audio',     icon: '🎵', name: '音频播放',   render: renderStub },
    { id: 'schedule',  icon: '⏰', name: '定时任务',   render: renderStub }
  ];

  var client = null;
  var $ = function (id) { return document.getElementById(id); };

  /* ============================ 工具 ============================ */

  function fullTopic(sub) {
    var p = cfg.prefix.replace(/\/+$/, '');
    return sub.charAt(0) === '/' ? sub.slice(1) : (p + '/' + sub);
  }

  function log(cls, text) {
    if (!$('log')) return;
    var box = $('log');
    var d = document.createElement('div');
    var now = new Date().toLocaleTimeString('zh-CN', { hour12: false });
    d.innerHTML = '<span class="t">' + now + '</span> <span class="' + cls + '">' + esc(text) + '</span>';
    box.appendChild(d);
    while (box.childNodes.length > 200) box.removeChild(box.firstChild);
    box.scrollTop = box.scrollHeight;
  }
  function esc(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  /* ============================ 顶栏时钟（对应 clock_timer_cb） ============================ */

  function tickClock() {
    var now = new Date();
    var week = ['日', '一', '二', '三', '四', '五', '六'][now.getDay()];
    $('clockDate').textContent =
      now.getFullYear() + '年' + String(now.getMonth() + 1).padStart(2, '0') + '月' +
      String(now.getDate()).padStart(2, '0') + '日 星期' + week;
    $('clockTime').textContent = now.toLocaleTimeString('zh-CN', { hour12: false });
  }

  /* ============================ 路由（对应 page_show / nav_btn_cb） ============================ */

  function renderNav() {
    var sidebar = $('sidebar');
    sidebar.innerHTML = '';
    PAGES.forEach(function (p) {
      var btn = document.createElement('button');
      btn.className = 'nav-btn';
      btn.dataset.id = p.id;
      btn.innerHTML = '<span class="ico">' + p.icon + '</span><span>' + p.name + '</span>';
      btn.onclick = function () { location.hash = '#/' + p.id; };
      sidebar.appendChild(btn);
    });
  }

  function route() {
    var id = (location.hash.replace(/^#\/?/, '') || 'dashboard');
    var page = PAGES.filter(function (p) { return p.id === id; })[0] || PAGES[0];

    document.querySelectorAll('.nav-btn').forEach(function (b) {
      b.classList.toggle('active', b.dataset.id === page.id);
    });

    var content = $('content');
    content.innerHTML = '';
    var el = document.createElement('div');
    el.className = 'page active';
    content.appendChild(el);
    page.render(el);
  }

  /* ============================ 页面：主界面 ============================ */

  function renderDashboard(root) {
    root.innerHTML =
      '<div class="grid2">' +
        '<div class="card"><h2>环境信息</h2>' +
          '<div class="metrics">' +
            '<div class="metric"><div class="k">温度 ℃</div><div class="v" id="mTemp">--</div></div>' +
            '<div class="metric"><div class="k">湿度 %</div><div class="v" id="mHumi">--</div></div>' +
          '</div>' +
          '<div class="hint">数据来自板端 DHT11，每 5 秒上报一次（主题 status/env）。</div>' +
        '</div>' +
        '<div class="card"><h2>设备控制</h2>' +
          '<div class="row"><span class="ico">💡</span><span class="name">灯具</span>' +
            '<span class="spacer"></span>' +
            '<button class="switch" id="swLed" disabled></button>' +
            '<span class="state" id="stLed">--</span></div>' +
          '<div class="row"><span class="ico">❄️</span><span class="name">空调</span>' +
            '<span class="spacer"></span>' +
            '<button class="switch" id="swAc" disabled></button>' +
            '<span class="state" id="stAc">--</span></div>' +
          '<div class="hint">开关发送到 <code>前缀/cmd/led</code> 与 <code>前缀/cmd/ac</code>，' +
          '状态以板端 retain 上报为准。</div>' +
        '</div>' +
      '</div>';

    $('swLed').onclick = function () { publish('cmd/led', store.led ? 'off' : 'on'); };
    $('swAc').onclick  = function () { publish('cmd/ac',  store.ac  ? 'off' : 'on'); };
    paintDashboard();
  }

  function paintDashboard() {
    if (!$('swLed')) return;
    $('mTemp').textContent = store.temp === null ? '--' : store.temp;
    $('mHumi').textContent = store.humi === null ? '--' : store.humi;

    [['swLed', 'stLed', store.led], ['swAc', 'stAc', store.ac]].forEach(function (t) {
      var sw = $(t[0]), st = $(t[1]), on = t[2];
      sw.classList.toggle('on', on === true);
      sw.disabled = !store.connected;
      st.textContent = on === null ? '--' : (on ? '开' : '关');
    });
  }

  /* ============================ 页面：设置 ============================ */

  function renderSettings(root) {
    root.innerHTML =
      '<div class="card"><h2>连接配置</h2>' +
        '<div class="field"><label>Broker 地址</label>' +
          '<input type="text" id="host" value="' + esc(cfg.host) + '"></div>' +
        '<div class="field"><label>WebSocket 端口</label>' +
          '<input type="text" id="port" value="' + esc(cfg.port) + '"></div>' +
        '<div class="field"><label>路径</label>' +
          '<input type="text" id="path" value="' + esc(cfg.path) + '"></div>' +
        '<div class="field"><label>主题前缀（需与板子一致）</label>' +
          '<input type="text" id="prefix" value="' + esc(cfg.prefix) + '"></div>' +
        '<div class="btns"><button class="btn primary" id="btnConn">连接</button>' +
          '<button class="btn" id="btnDisc" disabled>断开</button></div>' +
        '<div class="hint">公共测试 Broker 无需账号密码。主题前缀必须与开发板固件完全一致，' +
        '否则收不到消息。</div></div>';

    $('btnConn').onclick = function () {
      cfg.host = $('host').value.trim();
      cfg.port = $('port').value.trim();
      cfg.path = $('path').value.trim();
      cfg.prefix = $('prefix').value.trim();
      connect();
    };
    $('btnDisc').onclick = disconnect;
    paintConn();
  }

  /* ============================ 页面：日志管理 ============================ */

  function renderLog(root) {
    root.innerHTML =
      '<div class="card"><h2>消息日志</h2><div id="log"></div></div>' +
      '<div class="card"><h2>自定义发布（调试）</h2>' +
        '<div class="field"><label>主题（相对前缀或完整）</label>' +
          '<input type="text" id="dbgTopic" value="cmd/led"></div>' +
        '<div class="field"><label>载荷</label>' +
          '<input type="text" id="dbgPayload" value="on"></div>' +
        '<button class="btn" id="btnDbg" disabled>发送</button></div>';

    $('btnDbg').onclick = function () { publish($('dbgTopic').value.trim(), $('dbgPayload').value); };
    log('sys', '日志已就绪');
  }

  function renderStub(root) {
    root.innerHTML = '<div class="stub">该页面建设中…</div>';
  }

  /* ============================ 连接状态显示 ============================ */

  function paintConn() {
    $('dot').className = 'dot' + (store.connected ? ' on' : '');
    $('connState').textContent = store.connected ? ('已连接 ' + cfg.host) : '未连接';
    if ($('btnConn')) $('btnConn').disabled = store.connected;
    if ($('btnDisc')) $('btnDisc').disabled = !store.connected;
    if ($('btnDbg')) $('btnDbg').disabled = !store.connected;
    paintDashboard();
  }

  /* ============================ MQTT（对应 mqtt_client.c 的协议） ============================ */

  function publish(sub, payload) {
    if (!client || !client.connected) { log('sys', '未连接，发送失败'); return; }
    var topic = fullTopic(sub);
    client.publish(topic, String(payload), { qos: 1, retain: false });
    log('up', '↑ ' + topic + '  ' + payload);
  }

  function connect() {
    if (typeof mqtt === 'undefined') {
      alert('mqtt.min.js 未加载，请确认 web/vendor/mqtt.min.js 存在');
      return;
    }
    if (client) { client.end(true); client = null; }

    var url = 'ws://' + cfg.host + ':' + cfg.port + cfg.path;
    var clientId = 'web_' + Math.random().toString(16).slice(2, 10);
    log('sys', '正在连接 ' + url + ' …');

    client = mqtt.connect(url, {
      clientId: clientId, clean: true, keepalive: 30,
      connectTimeout: 8000, reconnectPeriod: 4000
    });

    client.on('connect', function () {
      store.connected = true;
      paintConn();
      var sub = cfg.prefix.replace(/\/+$/, '') + '/status/#';
      client.subscribe(sub, { qos: 1 }, function (err) {
        log('sys', err ? ('订阅失败: ' + err.message) : ('已订阅 ' + sub));
      });
    });

    client.on('message', function (topic, payload) {
      var text = payload.toString();
      log('down', '↓ ' + topic + '  ' + text);
      var name = topic.split('/').slice(-2).join('/');
      if (name === 'status/led') store.led = (text === 'on');
      else if (name === 'status/ac') store.ac = (text === 'on');
      else if (name === 'status/env') {
        try { var o = JSON.parse(text);
          if (o.temp !== undefined) store.temp = o.temp;
          if (o.humi !== undefined) store.humi = o.humi;
        } catch (e) { /* 非 JSON 忽略 */ }
      }
      paintDashboard();
      paintConn();
    });

    client.on('error', function (e) { log('sys', '错误: ' + e.message); });
    client.on('close', function () { store.connected = false; $('dot').className = 'dot err'; paintConn(); });
    client.on('reconnect', function () { log('sys', '重连中…'); });
  }

  function disconnect() {
    if (client) { client.end(true); client = null; }
    store.connected = false;
    store.led = store.ac = store.temp = store.humi = null;
    log('sys', '已断开');
    paintConn();
  }

  /* ============================ 启动 ============================ */

  renderNav();
  window.addEventListener('hashchange', route);
  route();
  tickClock();
  setInterval(tickClock, 1000);
  paintConn();
})();