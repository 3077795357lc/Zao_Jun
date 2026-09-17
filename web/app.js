/* 灶君 Web 控制台：对应板端 ui_main.c + pages/
 * 侧栏导航表与 page_show() 的切换逻辑、设备控制、环境信息全部在此实现。
 * 数据来源统一走 MQTT（协议契约见 net/mqtt_client.c），视图只读 store。
 */
(() => {
  'use strict';

  /* ---------- 连接配置（默认与板端 mqtt_client.c 一致） ---------- */
  const cfg = {
    host: 'broker.hivemq.com',
    port: '8000',
    path: '/mqtt',
    prefix: 'home/zaojun_imx6ull'
  };

  /* ---------- 状态模型：对应板端 data/dev_status + env_status ---------- */
  const store = { connected: false, led: null, ac: null, temp: null, humi: null, time: '--' };

  /* 延迟测量：记录每次点击的时刻，等对应的 status 消息回来再算差值。
     t0 用 performance.now()，全程同一个浏览器时钟，所以不需要和板子对时。 */
  const pending = { led: null, ac: null };

  /* ---------- 导航表：顺序与板端 pages[] 完全一致 ---------- */
  const PAGES = [
    { id: 'dashboard', icon: '🏠', name: '主界面',     render: renderDashboard },
    { id: 'settings',  icon: '⚙️', name: '设置',       render: renderSettings },
    { id: 'log',       icon: '📜', name: '日志管理',   render: renderLog },
    { id: 'camera',    icon: '📷', name: '查看摄像头', render: renderStub },
    { id: 'audio',     icon: '🎵', name: '音频播放',   render: renderStub },
    { id: 'schedule',  icon: '⏰', name: '定时任务',   render: renderStub }
  ];

  let client = null;
  const $ = (id) => document.getElementById(id);

  /* ============================ 工具 ============================ */

  function fullTopic(sub) {
    const p = cfg.prefix.replace(/\/+$/, '');
    return sub.charAt(0) === '/' ? sub.slice(1) : (p + '/' + sub);
  }

  function log(cls, text) {
    if (!$('log')) return;
    const box = $('log');
    const d = document.createElement('div');
    const now = new Date();
    const nowStr = now.toLocaleTimeString('zh-CN', { hour12: false }) + '.' +
                   String(now.getMilliseconds()).padStart(3, '0');
    d.innerHTML = '<span class="t">' + nowStr + '</span> <span class="' + cls + '">' + esc(text) + '</span>';
    box.appendChild(d);
    while (box.childNodes.length > 200) box.removeChild(box.firstChild);
    box.scrollTop = box.scrollHeight;
  }
  function esc(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }
  /* 毫秒数去掉多余小数位，方便直接看 */
  function fmtMs(ms) { return Math.round(ms * 10) / 10; }

  /* ============================ 顶栏时钟（对应 clock_timer_cb） ============================ */

  function tickClock() {
    const now = new Date();
    const week = ['日', '一', '二', '三', '四', '五', '六'][now.getDay()];
    $('clockDate').textContent =
      now.getFullYear() + '年' + String(now.getMonth() + 1).padStart(2, '0') + '月' +
      String(now.getDate()).padStart(2, '0') + '日 星期' + week;
    $('clockTime').textContent = now.toLocaleTimeString('zh-CN', { hour12: false });
  }

  /* ============================ 路由（对应 page_show / nav_btn_cb） ============================ */

  function renderNav() {
    const sidebar = $('sidebar');
    sidebar.innerHTML = '';
    PAGES.forEach((p) => {
      const btn = document.createElement('button');
      btn.className = 'nav-btn';
      btn.dataset.id = p.id;
      btn.innerHTML = '<span class="ico">' + p.icon + '</span><span>' + p.name + '</span>';
      btn.onclick = () => { location.hash = '#/' + p.id; };
      sidebar.appendChild(btn);
    });
  }

  function route() {
    const id = (location.hash.replace(/^#\/?/, '') || 'dashboard');
    const page = PAGES.filter((p) => p.id === id)[0] || PAGES[0];

    document.querySelectorAll('.nav-btn').forEach((b) => {
      b.classList.toggle('active', b.dataset.id === page.id);
    });

    const content = $('content');
    content.innerHTML = '';
    const el = document.createElement('div');
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

    $('swLed').onclick = () => { clickCmd('led', 'cmd/led', store.led ? 'off' : 'on', '灯具'); };
    $('swAc').onclick  = () => { clickCmd('ac',  'cmd/ac',  store.ac  ? 'off' : 'on', '空调'); };
    paintDashboard();
  }

  function paintDashboard() {
    if (!$('swLed')) return;
    $('mTemp').textContent = store.temp === null ? '--' : store.temp;
    $('mHumi').textContent = store.humi === null ? '--' : store.humi;

    [['swLed', 'stLed', store.led], ['swAc', 'stAc', store.ac]].forEach((t) => {
      const sw = $(t[0]), st = $(t[1]), on = t[2];
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

    $('btnConn').onclick = () => {
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

    $('btnDbg').onclick = () => { publish($('dbgTopic').value.trim(), $('dbgPayload').value); };
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

  /* 用户点了一次开关：记下时刻，再发指令。
     key 用来把这次点击和后面收到的 status 消息配对，算出端到端延迟。 */
  function clickCmd(key, sub, payload, label) {
    pending[key] = { t0: performance.now(), wall: Date.now(), payload: payload, label: label };
    log('up', '👆 点击「' + label + '」 -> ' + payload);
    publish(sub, payload, key);
  }

  function publish(sub, payload, key) {
    if (!client || !client.connected) { log('sys', '未连接，发送失败'); return; }
    const topic = fullTopic(sub);
    const p = key ? pending[key] : null;
    /* qos=1 的回调在收到 Broker 的 PUBACK 时触发，代表上行这一段跑完了 */
    client.publish(topic, String(payload), { qos: 1, retain: false }, (err) => {
      if (err) { log('sys', '发布失败: ' + err.message); return; }
      log('down', '↑ ' + topic + '  ' + payload + ' 已确认（PUBACK）');
      if (p) {
        p.tPuback = performance.now();
        log('sys', '① Web->Broker 上行完成: 点击后 ' + fmtMs(p.tPuback - p.t0) + ' ms');
      }
    });
  }

  /* 板端状态回到网页：和这次点击的 t0 相减，得到"点击->状态刷新"的整段延迟 */
  function reportLatency(key, on) {
    const p = pending[key];
    if (!p) return;
    pending[key] = null;
    const total = performance.now() - p.t0;
    /* 超过 10s 说明不是这次点击引起的（比如重连后板端补发的 retain 状态），不算数 */
    if (total > 10000) return;
    let extra = '';
    if (p.tPuback) {
      extra = '（上行 ' + fmtMs(p.tPuback - p.t0) + ' ms / 板端+下行 ' +
              fmtMs(total - (p.tPuback - p.t0)) + ' ms）';
    }
    log('sys', '② ' + p.label + '状态刷新为「' + (on ? '开' : '关') + '」: 点击->刷新共 ' +
        fmtMs(total) + ' ms' + extra);
  }

  function connect() {
    if (typeof mqtt === 'undefined') {
      alert('mqtt.min.js 未加载，请确认 web/vendor/mqtt.min.js 存在');
      return;
    }
    if (client) { client.end(true); client = null; }

    const url = 'ws://' + cfg.host + ':' + cfg.port + cfg.path;
    const clientId = 'web_' + Math.random().toString(16).slice(2, 10);
    log('sys', '正在连接 ' + url + ' …');

    client = mqtt.connect(url, {
      clientId: clientId, clean: true, keepalive: 30,
      connectTimeout: 8000, reconnectPeriod: 4000
    });

    client.on('connect', () => {
      store.connected = true;
      paintConn();
      const sub = cfg.prefix.replace(/\/+$/, '') + '/status/#';
      client.subscribe(sub, { qos: 1 }, (err) => {
        log('sys', err ? ('订阅失败: ' + err.message) : ('已订阅 ' + sub));
      });
    });

    client.on('message', (topic, payload) => {
      const text = payload.toString();
      log('down', '↓ ' + topic + '  ' + text);
      const name = topic.split('/').slice(-2).join('/');
      if (name === 'status/led') { store.led = (text === 'on'); reportLatency('led', store.led); }
      else if (name === 'status/ac') { store.ac = (text === 'on'); reportLatency('ac', store.ac); }
      else if (name === 'status/env') {
        try { const o = JSON.parse(text);
          if (o.temp !== undefined) store.temp = o.temp;
          if (o.humi !== undefined) store.humi = o.humi;
        } catch (e) { /* 非 JSON 忽略 */ }
      }
      paintDashboard();
      paintConn();
    });

    client.on('error', (e) => { log('sys', '错误: ' + e.message); });
    client.on('close', () => {
      store.connected = false;
      pending.led = pending.ac = null;   /* 断线了，这次点击的测量作废 */
      $('dot').className = 'dot err';
      paintConn();
    });
    client.on('reconnect', () => { log('sys', '重连中…'); });
  }

  function disconnect() {
    if (client) { client.end(true); client = null; }
    store.connected = false;
    store.led = store.ac = store.temp = store.humi = null;
    pending.led = pending.ac = null;
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