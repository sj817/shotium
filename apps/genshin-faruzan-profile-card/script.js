'use strict';

/* ==========================================================================
 * 玩家资料配置 —— 修改这里即可更新卡片内容
 * 也支持 URL 参数覆盖,例如 index.html?nickname=旅行者&uid=123456789
 * ========================================================================== */
const profileData = {
  nickname: '知论派前辈',
  uid: '100000001',
  server: '天空岛',
  adventureRank: 60,
  worldLevel: 8,
  signature: '要称呼我为前辈,明白了吗?',
  achievements: 987,
  spiralAbyss: '12-3',
  activeDays: 1024,
  charLevel: 90,
  constellation: '满命 C6',
  friendship: 10,
  weapon: '终末嗟叹之诗',
  critRate: '62.4%',
  critDmg: '128.6%',
  energyRecharge: '241.3%',
  anemoDmg: '61.6%',
  talents: '9 / 12 / 12',
};

/* ---------- 数据渲染 ---------- */

/** 将 profileData 写入所有 [data-field] 节点 */
function renderProfile() {
  document.querySelectorAll('[data-field]').forEach((el) => {
    const key = el.dataset.field;
    if (key in profileData) el.textContent = String(profileData[key]);
  });
}

/** URL 参数覆盖默认资料 */
function applyUrlParams() {
  const params = new URLSearchParams(window.location.search);
  Object.keys(profileData).forEach((key) => {
    const value = params.get(key);
    if (value !== null && value !== '') profileData[key] = value;
  });
}

/* ---------- 卡片缩放 ---------- */

const CARD_WIDTH = 1600;
const CARD_HEIGHT = 900;
const STAGE_MARGIN = 88; // 卡片四周留白,让页面底色与卡片形成呼吸感

function fitCard() {
  const scale = Math.min(
    (window.innerWidth - STAGE_MARGIN) / CARD_WIDTH,
    (window.innerHeight - STAGE_MARGIN) / CARD_HEIGHT,
    1.1
  );
  document.getElementById('card').style.setProperty('--card-scale', String(Math.max(scale, 0.2)));
}

/* ---------- 背景粒子 ---------- */

function spawnParticles() {
  const host = document.getElementById('particles');
  const COUNT = 26;
  const frag = document.createDocumentFragment();
  for (let i = 0; i < COUNT; i += 1) {
    const p = document.createElement('span');
    p.className = 'particle';
    const size = 2 + Math.random() * 5;
    p.style.width = `${size.toFixed(1)}px`;
    p.style.height = `${size.toFixed(1)}px`;
    p.style.left = `${(Math.random() * 100).toFixed(2)}%`;
    p.style.top = `${(Math.random() * 100).toFixed(2)}%`;
    p.style.setProperty('--dx', `${(Math.random() * 90 - 45).toFixed(0)}px`);
    p.style.setProperty('--dy', `${(-30 - Math.random() * 90).toFixed(0)}px`);
    p.style.setProperty('--dur', `${(12 + Math.random() * 16).toFixed(1)}s`);
    p.style.setProperty('--delay', `${(-Math.random() * 20).toFixed(1)}s`);
    p.style.setProperty('--op', (0.25 + Math.random() * 0.5).toFixed(2));
    frag.appendChild(p);
  }
  host.appendChild(frag);
}

/* 机关圆盘外圈刻度 */
function drawMechanismTicks() {
  const group = document.querySelector('.mech-ticks');
  const NS = 'http://www.w3.org/2000/svg';
  for (let i = 0; i < 72; i += 1) {
    const angle = (i / 72) * Math.PI * 2;
    const long = i % 6 === 0;
    const r1 = long ? 352 : 360;
    const line = document.createElementNS(NS, 'line');
    line.setAttribute('x1', String(400 + Math.cos(angle) * r1));
    line.setAttribute('y1', String(400 + Math.sin(angle) * r1));
    line.setAttribute('x2', String(400 + Math.cos(angle) * 366));
    line.setAttribute('y2', String(400 + Math.sin(angle) * 366));
    line.setAttribute('stroke', 'currentColor');
    line.setAttribute('stroke-width', long ? '1' : '0.5');
    line.setAttribute('opacity', long ? '0.55' : '0.3');
    group.appendChild(line);
  }
}

/* ---------- 启动 ---------- */

applyUrlParams();
renderProfile();
fitCard();
spawnParticles();
drawMechanismTicks();

window.addEventListener('resize', fitCard);
