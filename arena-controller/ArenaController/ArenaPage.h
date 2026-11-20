#pragma once
#include <Arduino.h>

static const char arenaPage[] PROGMEM = R"ARENA(
<!doctype html>
<html lang="kk"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark"><title>Arena Controller</title>
<style>
:root{
 color-scheme:dark;--bg:#0b1018;--panel:#131c29;--line:#2a394b;--text:#f3f6fa;
 --muted:#a0b0c1;--lime:#d1fa70;--blue:#75b7ff;--red:#ff8890;
 font-family:system-ui,-apple-system,sans-serif;line-height:1.35;background:var(--bg);color:var(--text)
}
*{box-sizing:border-box}
body{max-width:1040px;margin:auto;padding:clamp(16px,4vw,36px);padding-bottom:44px;
 background:radial-gradient(ellipse at 80% 0%,#173145 0%,transparent 42%)}
header{display:flex;align-items:center;justify-content:space-between;gap:14px;padding:0 0 18px}
.brand{display:flex;align-items:center;gap:12px;font-weight:800;font-size:.88rem;letter-spacing:.055em}
.mark{width:36px;height:36px;display:grid;place-items:center;background:var(--lime);color:#141b0a;
 clip-path:polygon(16% 0,100% 0,100% 84%,84% 100%,0 100%,0 16%);font-size:1.25rem}
select,button{font:inherit;color:inherit}
select{max-width:100%;border:1px solid #3f5265;background:#162231;border-radius:10px;padding:9px 10px;min-height:44px}
.hero{position:relative;overflow:hidden;isolation:isolate;display:flex;align-items:flex-end;justify-content:space-between;
 flex-wrap:wrap;gap:14px;padding:clamp(18px,4vw,36px);min-height:132px;border:1px solid #35495b;
 border-radius:20px;background:linear-gradient(115deg,#162433,#121d2b 58%,#25364b)}
.hero::after{content:"";position:absolute;z-index:-1;right:0;top:0;bottom:0;width:38%;
 background:repeating-linear-gradient(130deg,transparent 0 20px,#a5daff13 20px 22px);
 mask-image:linear-gradient(90deg,transparent,#000)}
h1{font-size:clamp(1.75rem,5.5vw,3rem);line-height:1.05;letter-spacing:-.045em;margin:0;text-wrap:balance}
#connection{display:inline-flex;align-items:center;gap:7px;border:1px solid #536275;background:#121c29;
 border-radius:100px;padding:7px 10px;color:#e3ebf4;font-size:.77rem;white-space:nowrap}
#connection::before{content:"";width:7px;height:7px;flex:0 0 7px;border-radius:50%;background:#ffab74}
#connection[data-online=true]::before{background:var(--lime);box-shadow:0 0 12px #d1fa7066}
.jump{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;margin:14px 0}
.jump a{display:grid;place-items:center;text-align:center;min-height:44px;padding:5px;
 color:var(--text);background:#1a2737;border:1px solid #364a60;border-radius:9px;
 font-size:.85rem;font-weight:700;text-decoration:none;overflow-wrap:anywhere}
.jump a:focus-visible{outline:2px solid var(--lime);outline-offset:2px}
main{display:grid;gap:14px;margin-top:14px}
main section,.audio-zone{scroll-margin-top:12px}
.lights [data-id=blue]{order:1}.lights [data-id=red]{order:2}
.lights [data-id=blue-pulse]{order:3}.lights [data-id=red-pulse]{order:4}
.lights [data-id=blue-blink]{order:5}.lights [data-id=red-blink]{order:6}
.lights [data-id=alternate]{order:7}
section{padding:clamp(12px,2vw,20px);border:1px solid var(--line);border-radius:18px;background:var(--panel)}
h2{font-size:1rem;margin:0 0 12px;letter-spacing:.015em}
.commands{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
button{display:flex;align-items:center;gap:10px;text-align:left;min-width:0;
 min-height:52px;padding:10px 12px;background:#1a2737;border:1px solid #364a60;border-radius:11px;
 font-size:.88rem;font-weight:660;letter-spacing:-.012em;cursor:pointer;touch-action:manipulation;
 overflow-wrap:anywhere;transition:border-color .16s,background-color .16s}
button:hover:not(:disabled){border-color:#8297af}
button:active:not(:disabled){transform:translateY(1px)}
button:disabled{opacity:.49;cursor:default}
button:focus-visible,select:focus-visible{outline:2px solid var(--lime);outline-offset:3px}
.icon{flex:0 0 20px;text-align:center;color:var(--muted);font-family:system-ui,sans-serif;font-size:1rem}
.round button,.teams button{min-height:64px}
[data-id=start]{background:var(--lime);border-color:var(--lime);color:#151f0b}
[data-id=start] .icon{color:#32441a}
[data-id=stop]{background:#46252f;border-color:#8d4d59;color:#fff0f0}
[data-id=stop] .icon,[data-id^=red] .icon{color:var(--red)}
[data-id=winner] .icon{color:#ffd47c}
[data-id^=blue] .icon{color:var(--blue)}
[data-id=blue-intro]{background:#1b3350;border-color:#466f9d}
[data-id=red-intro]{background:#3c2633;border-color:#854d60}
[data-id=all-off]{grid-column:1/-1;background:#30252d;border-color:#765360}
[data-id=all-off] .icon{color:var(--red)}
[data-id=alternate]{grid-column:1/-1;background:linear-gradient(95deg,#3c2633,#202d49)}
.audio-zone{margin-top:14px;display:grid;gap:14px}
.mic{background:linear-gradient(130deg,#182637,#111924);display:grid;gap:9px;align-content:start}
.mic h2{margin-bottom:2px}#mic-button{justify-content:center;min-height:72px;background:#293b4b;border-color:#638298;
 color:#e6f2fc;touch-action:none;user-select:none;-webkit-touch-callout:none}
#mic-button[data-live=true]{color:#171014;background:#ff8890;border-color:#ff8890}
#mic-button .mic-dot{font-size:1.1rem}#mic-status{margin:0;color:#b5c6d7;font-size:.85rem}
#sounds{margin:0}#sounds form{display:grid;gap:8px}#sounds label,#sound-help,#sound-status{color:var(--muted);font-size:.85rem}
#sound-name,#sound-file{font:inherit;color:inherit;min-width:0;padding:8px;border:1px solid var(--line);border-radius:8px}
#sounds ul{list-style:none;margin:12px 0 0;padding:0;display:grid;gap:8px}
#sounds li{display:flex;flex-wrap:wrap;align-items:center;gap:8px;padding:8px;border:1px solid var(--line);border-radius:8px}
#sounds li strong{flex:1 1 9rem;min-width:0;overflow-wrap:anywhere}
#sounds li button{min-height:44px}#sound-status{min-height:1.2em;margin:8px 0 0}
#status{min-height:1.4em;margin:12px 4px 0;color:#b0bed0;font-size:.84rem}
@media(min-width:730px){
 main{grid-template-columns:1fr 1fr}.lights{grid-column:1/-1}
 .lights .commands{grid-template-columns:repeat(4,minmax(0,1fr))}
 .round button,.teams button{min-height:69px}
 .audio-zone{grid-template-columns:minmax(0,1fr) minmax(0,2fr)}
}
@media(max-width:365px){button{gap:6px;padding:8px;font-size:.82rem}.icon{flex-basis:15px}}
@media(prefers-reduced-motion:reduce){button{transition:none}}
</style></head><body>
<header><div class="brand"><span class="mark" aria-hidden="true">✦</span> BOX BATTLE OF BOTS</div>
<select id="lang" aria-label="Тіл / Язык / Language">
<option value="kk">Қазақша</option><option value="ru">Русский</option><option value="en">English</option>
</select></header>
<div class="hero"><h1 id="title"></h1><span id="connection" data-online="false"></span></div>
<nav class="jump" aria-label="Arena sections"><a href="#round"></a><a href="#teams"></a><a href="#lights"></a><a href="#audio"></a></nav>
<main id="commands"></main><p id="status" role="status"></p>
<div class="audio-zone" id="audio">
<section class="mic"><h2 id="mic-heading"></h2>
<button id="mic-button" type="button" aria-describedby="mic-status" aria-pressed="false" disabled>
<span class="mic-dot" aria-hidden="true">●</span><span id="mic-label"></span></button>
<p id="mic-status" role="status"></p></section>
<section id="sounds"><h2></h2>
<form><label for="sound-name" id="sound-name-label"></label>
<input id="sound-name" type="text" maxlength="60">
<input id="sound-file" type="file" accept="audio/mpeg,audio/wav,.mp3,.wav" required aria-describedby="sound-help">
<small id="sound-help"></small><button id="sound-add" type="submit"></button></form>
<ul></ul><p id="sound-status" role="status"></p></section></div>
<script>
const groups=[
 ['round',['start','stop','finish','winner']],
 ['teams',['blue-intro','red-intro']],
 ['lights',['toggle','full','blue','blue-pulse','blue-blink','red','red-pulse','red-blink','alternate','all-off']]
];
const strings={
 kk:['Арена басқаруы','Раунд','Командалар','Жарық','Раундты бастау','Жекпе-жекті тоқтату','Раундты аяқтау','Жеңімпазды жариялау','Көк команданы таныстыру','Қызыл команданы таныстыру','Ақ жарықты қосу / өшіру','Барлық жарықты 100% қосу','Көк жарық','Көк жарық пульсациясы','Көк жарықтың жылдам жыпылықтауы','Қызыл жарық','Қызыл жарық пульсациясы','Қызыл жарықтың жылдам жыпылықтауы','Қызыл мен көкті кезектестіру','Барлық жарықты өшіру','ESP32-ге қосылды','Алдымен раундты аяқта','Команда жіберілді','Байланыс үзілді','Қосылып жатыр…'],
 ru:['Управление ареной','Раунд','Команды','Освещение','Начать раунд','Остановить бой','Завершить раунд','Объявить победителя','Представить синюю команду','Представить красную команду','Включить / выключить белый свет','Включить все каналы на 100%','Синий свет','Пульсация синего','Быстрое мигание синего','Красный свет','Пульсация красного','Быстрое мигание красного','Чередовать красный и синий','Выключить всё освещение','ESP32 на связи','Сначала завершите раунд','Команда отправлена','Связь потеряна','Подключение…'],
 en:['Arena control','Round','Teams','Lighting','Start the round','Stop the battle','Finish the round','Announce the winner','Introduce the blue team','Introduce the red team','Toggle white light','Set all lights to 100%','Blue light','Pulse blue light','Flash blue light','Red light','Pulse red light','Flash red light','Alternate red and blue','Turn off all lights','Connected to ESP32','Finish the round first','Command sent','Connection lost','Connecting…']
};
const icons=['▶','■','⚑','★','●','●','⏻','☀','●','∿','✦','●','∿','✦','⇄','⏻'];
const picker=document.querySelector('#lang');
try{picker.value=localStorage.arenaLanguage||'kk'}catch{}
if(!strings[picker.value]) picker.value='kk';
let ready=false, online=false, busy=false, statusKey=24;
const labels=()=>strings[picker.value];
const show=(key,name='')=>{
 statusKey=key;
 const connection=document.querySelector('#connection');
 connection.dataset.online=String(online);
 connection.textContent=labels()[online?20:key===24?24:23];
 document.querySelector('#status').textContent=key===20||key>=23?'':labels()[key]+(name?' · '+name:'');
};
function updateButtons(){
 document.querySelectorAll('#commands button').forEach(b=>b.disabled=busy||!online||(b.dataset.id==='winner'&&!ready));
}
function render(){
 document.documentElement.lang=picker.value;
 document.title=document.querySelector('#title').textContent=labels()[0];
 const main=document.querySelector('#commands');main.replaceChildren();
 document.querySelectorAll('.jump a').forEach((link,i)=>{
  link.textContent=i<3?labels()[i+1]:({kk:'Дыбыс',ru:'Звук',en:'Audio'})[picker.value];
 });
 let offset=4;
 groups.forEach(([group,ids],i)=>{
  const section=document.createElement('section'), heading=document.createElement('h2'), list=document.createElement('div');
  section.id=group;section.className=group;heading.textContent=labels()[i+1];list.className='commands';
  ids.forEach(id=>{
   const b=document.createElement('button'), icon=document.createElement('span'), name=document.createElement('span');
   b.dataset.id=id; icon.className='icon'; icon.setAttribute('aria-hidden','true');
   icon.textContent=icons[offset-4]; name.textContent=labels()[offset++];
   b.append(icon,name); b.onclick=()=>send(id,name.textContent);list.append(b);
  });
  section.append(heading,list);main.append(section);
 });
 updateButtons();show(statusKey);
}
picker.onchange=()=>{try{localStorage.arenaLanguage=picker.value}catch{}render();renderSounds()};
async function refresh(){
 const response=await fetch('/api/status',{cache:'no-store',signal:AbortSignal.timeout(2500)});
 if(!response.ok) throw Error();
 ready=(await response.json()).winnerReady;
 const changed=!online;online=true;
 if(changed) renderSounds();
 if(statusKey===23||statusKey===24) show(20);
 updateButtons();
}
async function send(id,name){
 busy=true;updateButtons();
 try{
  const response=await fetch('/api/command',{method:'POST',headers:{'Content-Type':'text/plain','X-Arena-Command':'1'},body:id,signal:AbortSignal.timeout(2500)});
  if(response.status===409) show(21);
  else if(response.ok) show(22,name);
  else throw Error();
  await refresh();
 }catch{online=false;show(23);renderSounds()}
 finally{busy=false;updateButtons()}
}
async function poll(){
 if(!busy){try{await refresh()}catch{const changed=online;online=false;show(23);updateButtons();if(changed) renderSounds()}}
 setTimeout(poll,1000);
}
const soundText={
 kk:['Қосымша дыбыстар','Дыбыс қосу','Дыбыс атауы','Ойнату','Тоқтату','Өшіру','60 секундқа дейінгі MP3 немесе WAV','Әзірге дыбыс қосылмаған','Дыбыс дайындалып жатыр…','Дыбыс ойнатылып жатыр','Дыбыс тоқтады','Дыбысты ашу немесе жіберу сәтсіз болды','Файл 12 МБ-тан, дыбыс 60 секундтан аспасын','Браузерде файлды сақтау мүмкін болмады','Қосымша дыбыс аяқталды','Дыбысты файлдан таңдау'],
 ru:['Дополнительные звуки','Добавить звук','Название звука','Воспроизвести','Остановить','Удалить','MP3 или WAV до 60 секунд','Звуков пока нет','Подготовка звука…','Звук воспроизводится','Звук остановлен','Не удалось открыть или отправить звук','До 12 МБ и 60 секунд на файл','Браузер не смог сохранить файл','Воспроизведение завершено','Выбрать звуковой файл'],
 en:['Extra sounds','Add sound','Sound name','Play','Stop','Delete','MP3 or WAV up to 60 seconds','No sounds added yet','Preparing audio…','Playing audio','Audio stopped','Could not open or send audio','Maximum 12 MB and 60 seconds per file','Could not save the file in this browser','Playback ended','Choose an audio file']
};
const soundLabels=()=>soundText[picker.value];
const soundSection=document.querySelector('#sounds');
const soundName=document.querySelector('#sound-name');
const soundFile=document.querySelector('#sound-file');
const soundStatus=document.querySelector('#sound-status');
let library=[], soundMessage=-1, soundSession=null;
let database;
try{
 database=new Promise((resolve,reject)=>{
  const open=indexedDB.open('boxbot-sounds',1);
  open.onupgradeneeded=()=>open.result.createObjectStore('sounds',{keyPath:'id',autoIncrement:true});
  open.onsuccess=()=>resolve(open.result);open.onerror=()=>reject(open.error);
 });
}catch(error){database=Promise.reject(error)}
function soundNotice(id){
 soundMessage=id;soundStatus.textContent=id<0?'':soundLabels()[id];
}
async function soundStore(mode,operation){
 const db=await database;
 return new Promise((resolve,reject)=>{
  const tx=db.transaction('sounds',mode);
  let request;
  tx.oncomplete=()=>resolve(request.result);
  tx.onabort=()=>reject(tx.error);
  request=operation(tx.objectStore('sounds'));
 });
}
async function loadSounds(){
 try{library=await soundStore('readonly',store=>store.getAll())}
 catch{soundNotice(13)}
 renderSounds();
}
function renderSounds(){
 const names=soundLabels();
 soundSection.querySelector('h2').textContent=names[0];
 soundSection.querySelector('#sound-add').textContent=names[1];
 soundSection.querySelector('#sound-name-label').textContent=names[2];
 soundFile.setAttribute('aria-label',names[15]);
 soundSection.querySelector('#sound-help').textContent=names[6];
 const list=soundSection.querySelector('ul');list.replaceChildren();
 if(!library.length){const item=document.createElement('li');item.textContent=names[7];list.append(item)}
 for(const entry of library){
  const item=document.createElement('li'),name=document.createElement('strong');
  const play=document.createElement('button'),remove=document.createElement('button');
  name.textContent=entry.name;
  play.textContent=soundSession?.id===entry.id?names[4]:names[3];
  play.disabled=Boolean(micSession)||((!online||Boolean(soundSession&&soundSession.id!==entry.id))&&soundSession?.id!==entry.id);
  play.onclick=()=>soundSession?.id===entry.id?stopSound():playSound(entry);
  remove.textContent=names[5];
  remove.onclick=async()=>{
   try{
    if(soundSession?.id===entry.id) await stopSound();
    await soundStore('readwrite',store=>store.delete(entry.id));
    await loadSounds();
   }catch{soundNotice(13)}
  };
  item.append(name,play,remove);list.append(item);
 }
 soundNotice(soundMessage);renderMic();
}
soundSection.querySelector('form').onsubmit=async event=>{
 event.preventDefault();
 const file=soundFile.files[0];
 if(!file) return;
 if(file.size>12000000){soundNotice(12);return}
 try{
  const name=(soundName.value.trim()||file.name.replace(/\.[^.]+$/,'')).slice(0,60);
  await soundStore('readwrite',store=>store.add({name,file}));
  event.target.reset();
  soundNotice(-1);await loadSounds();
 }catch{soundNotice(13)}
};
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function audioRequest(action,key,body=''){
 return fetch('/api/audio?action='+action+'&key='+key,{
  method:'POST',headers:{'Content-Type':'text/plain','X-Arena-Command':'1'},
  body,signal:AbortSignal.timeout(2500)
 });
}
async function stopSound(){
 const session=soundSession;
 if(!session) return;
 soundSession=null;
 soundNotice(10);renderSounds();
 if(session.started){try{await audioRequest('stop',session.key)}catch{}}
}
async function playSound(entry){
 const token=crypto.getRandomValues(new Uint32Array(1))[0]||1;
 const session={id:entry.id,key:token.toString(16).padStart(8,'0'),started:false};
 soundSession=session;soundNotice(8);renderSounds();
 try{
  const Audio=window.AudioContext||window.webkitAudioContext;
  const Offline=window.OfflineAudioContext||window.webkitOfflineAudioContext;
  const context=new Audio();
  let decoded;
  try{decoded=await context.decodeAudioData(await entry.file.arrayBuffer())}
  finally{await context.close()}
  if(soundSession!==session) return;
  if(decoded.duration>60){soundNotice(12);return}
  const offline=new Offline(1,Math.min(960000,Math.ceil(decoded.duration*16000)),16000);
  const source=offline.createBufferSource();source.buffer=decoded;
  source.connect(offline.destination);source.start();
  const pcm=(await offline.startRendering()).getChannelData(0);
  if(soundSession!==session) return;
  const start=await audioRequest('start',session.key);
  if(!start.ok) throw Error();
  session.started=true;
  if(soundSession!==session){await audioRequest('stop',session.key);return}
  for(let i=0;i<pcm.length&&soundSession===session;){
   const length=Math.min(4096,pcm.length-i), bytes=new Array(length);
   for(let j=0;j<length;j++) bytes[j]=String.fromCharCode(Math.max(0,Math.min(255,Math.round(128+pcm[i+j]*127))));
   const response=await audioRequest('chunk',session.key,btoa(bytes.join('')));
   if(response.status===429){await pause(75);continue}
   if(!response.ok) throw Error();
   i+=length;
   if(i>=8192&&soundSession===session) soundNotice(9);
  }
  if(soundSession!==session) return;
  if(!(await audioRequest('end',session.key)).ok) throw Error();
  soundNotice(9);
  while(soundSession===session){
   await pause(400);
   if(soundSession!==session) return;
   const state=await audioRequest('status',session.key);
   if(state.status===200){soundNotice(14);break}
   if(state.status!==204) throw Error();
  }
 }catch{
  if(soundSession===session){
   soundNotice(11);
   if(session.started){try{await audioRequest('stop',session.key)}catch{}}
  }
 }finally{
  if(soundSession===session){soundSession=null;renderSounds()}
 }
}
const micText={
 kk:['Арена микрофоны','Басып тұрып сөйле','Тікелей эфир','Микрофонды қосып жатыр…','Микрофон өшірулі','Микрофон үшін сайтты HTTPS арқылы аш','Микрофонға рұқсат берілмеді немесе байланыс үзілді'],
 ru:['Микрофон арены','Удерживайте, чтобы говорить','В эфире','Подключение микрофона…','Микрофон выключен','Для микрофона откройте сайт через HTTPS','Нет доступа к микрофону или потеряна связь'],
 en:['Arena microphone','Hold to talk','On air','Connecting microphone…','Microphone off','Open the site over HTTPS to use the microphone','Microphone unavailable or connection lost']
};
const micButton=document.querySelector('#mic-button');
const micAvailable=Boolean(isSecureContext&&navigator.mediaDevices?.getUserMedia&&window.AudioContext);
let micSession=null,micStatus=4;
const micLabels=()=>micText[picker.value];
function renderMic(){
 const labels=micLabels();
 document.querySelector('#mic-heading').textContent=labels[0];
 document.querySelector('#mic-label').textContent=labels[1];
 document.querySelector('#mic-status').textContent=labels[micAvailable?micStatus:5];
 micButton.disabled=!micAvailable||!online||Boolean(soundSession);
 micButton.dataset.live=String(micStatus===2);
 micButton.setAttribute('aria-pressed',String(Boolean(micSession)));
}
const micWorklet=`
class ArenaMic extends AudioWorkletProcessor {
 constructor(){
  super();this.phase=0;this.packet=new Uint8Array(1536);this.index=0;
 }
 process(inputs){
  const input=inputs[0][0];if(!input)return true;
  for(const sample of input){
   this.phase+=16000;
   if(this.phase<sampleRate)continue;
   this.phase-=sampleRate;
   this.packet[this.index++]=Math.max(0,Math.min(255,Math.round(128+sample*127)));
   if(this.index===this.packet.length){
    this.port.postMessage(this.packet,[this.packet.buffer]);
    this.packet=new Uint8Array(1536);this.index=0;
   }
  }
  return true;
 }
}
registerProcessor('arena-mic',ArenaMic);
`;
function closeMicResources(session){
 session.stream?.getTracks().forEach(track=>track.stop());
 if(session.context&&session.context.state!=='closed') void session.context.close().catch(()=>{});
}
function stopMic(status=4){
 const session=micSession;
 if(!session) return;
 micSession=null;micStatus=status;
 closeMicResources(session);
 if(session.started) void audioRequest('stop',session.key).catch(()=>{});
 renderMic();renderSounds();
}
async function pumpMic(session){
 if(session.sending) return;
 session.sending=true;
 try{
  while(micSession===session&&session.packet){
   const packet=session.packet;session.packet=null;
   const response=await audioRequest('chunk',session.key,btoa(String.fromCharCode(...packet)));
   if(micSession!==session) return;
   if(!response.ok&&response.status!==429) throw Error();
   if(response.ok&&micStatus!==2){micStatus=2;renderMic()}
  }
 }catch{if(micSession===session)stopMic(6)}
 finally{session.sending=false}
}
async function startMic(){
 if(micSession||micButton.disabled) return;
 const key=(crypto.getRandomValues(new Uint32Array(1))[0]||1).toString(16).padStart(8,'0');
 const session={key,started:false,packet:null,sending:false};
 micSession=session;micStatus=3;renderMic();renderSounds();
 try{
  const context=session.context=new AudioContext();
  const resume=context.resume();
  const media=navigator.mediaDevices.getUserMedia({audio:{channelCount:1,echoCancellation:true},video:false})
   .then(stream=>{
    session.stream=stream;
    if(micSession!==session) closeMicResources(session);
   });
  await Promise.all([media,resume]);
  if(micSession!==session){closeMicResources(session);return}
  const url=URL.createObjectURL(new Blob([micWorklet],{type:'text/javascript'}));
  try{await context.audioWorklet.addModule(url)}
  finally{URL.revokeObjectURL(url)}
  if(micSession!==session) return;
  if(!(await audioRequest('mic',session.key)).ok) throw Error();
  session.started=true;
  if(micSession!==session){await audioRequest('stop',session.key);return}
  const source=context.createMediaStreamSource(session.stream);
  const processor=new AudioWorkletNode(context,'arena-mic');
  processor.port.onmessage=event=>{
   if(micSession!==session)return;
   session.packet=event.data;void pumpMic(session);
  };
  const silence=context.createGain();silence.gain.value=0;
  source.connect(processor).connect(silence).connect(context.destination);
 }catch{
  closeMicResources(session);
  if(micSession===session)stopMic(6);
 }
}
micButton.onpointerdown=event=>{
 if(event.button!==0||!event.isPrimary)return;
 event.preventDefault();micButton.setPointerCapture(event.pointerId);void startMic();
};
micButton.onpointerup=micButton.onpointercancel=micButton.onlostpointercapture=()=>stopMic();
micButton.onkeydown=event=>{
 if(event.code==='Space'||event.code==='Enter'){
  event.preventDefault();if(!event.repeat)void startMic();
 }
};
micButton.onkeyup=event=>{
 if(event.code==='Space'||event.code==='Enter'){event.preventDefault();stopMic()}
};
window.addEventListener('blur',()=>stopMic());
document.addEventListener('visibilitychange',()=>{if(document.hidden)stopMic()});

render();renderSounds();loadSounds();poll();
</script></body></html>
)ARENA";
