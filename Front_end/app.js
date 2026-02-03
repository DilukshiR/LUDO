let API = "http://localhost:8080";

const canvas = document.getElementById("board");
const ctx = canvas.getContext("2d");

const elState = document.getElementById("state");
const elLog = document.getElementById("log");
const elSubtitle = document.getElementById("subtitle");

const colors = [
  { name:"Yellow", fill:"#ffd84d" },
  { name:"Blue",   fill:"#4da3ff" },
  { name:"Red",    fill:"#ff4d4d" },
  { name:"Green",  fill:"#44dd77" }
];

// Ludo safe cells (L2 L15 L28 L41) are 0-based indices: 2, 15, 28, 41.
const SAFE = new Set([2, 15, 28, 41]);

let lastState = null;
let anim = null; // {startTime, duration, from, to, p, k}
let renderLoopRunning = false;

function setApiFromInput(){
  API = document.getElementById("api").value.trim();
}

document.getElementById("reconnect").onclick = () => {
  setApiFromInput();
  fetchState();
};

document.getElementById("roll").onclick = () => sendCmd("STEP");
document.getElementById("pause").onclick = () => sendCmd("PAUSE");
document.getElementById("resume").onclick = () => sendCmd("RESUME");

async function sendCmd(cmd){
  try{
    await fetch(API + "/cmd", {
      method: "POST",
      headers: { "Content-Type": "text/plain" },
      body: cmd
    });
    await fetchState();
  }catch(e){
    elSubtitle.textContent = "Server offline / command failed";
  }
}

async function fetchState(){
  try{
    const r = await fetch(API + "/state", { cache: "no-store" });
    const s = await r.json();
    elSubtitle.textContent = `Connected: ${API}`;
    onNewState(s);
  }catch(e){
    elSubtitle.textContent = "Not connected (start backend on localhost:8080)";
  }
}

// --- Coordinate system (simple but clear) ---
// We render a "ring" path around the board as 52 evenly spaced cells.

const CENTER = { x: 320, y: 320 };
const RING_R = 230;

function ringXY(i){
  const a = (Math.PI * 2) * (i / 52) - Math.PI/2;
  return { x: CENTER.x + Math.cos(a) * RING_R, y: CENTER.y + Math.sin(a) * RING_R };
}

function baseXY(player, pieceIndex){
  // 4 corner bases, each with 4 slots
  const pad = 90;
  const corners = [
    { x: pad, y: pad },                                 // P0
    { x: canvas.width - pad, y: pad },                  // P1
    { x: canvas.width - pad, y: canvas.height - pad },  // P2
    { x: pad, y: canvas.height - pad }                  // P3
  ];
  const c = corners[player];
  const offsets = [
    {x:-18,y:-18},{x:18,y:-18},{x:-18,y:18},{x:18,y:18}
  ];
  const o = offsets[pieceIndex % 4];
  return { x: c.x + o.x, y: c.y + o.y };
}

function homeXY(player, step){ // step 0..5
  // each player has a line towards center
  const dirs = [
    {x: 1, y: 1},   // P0 towards center
    {x: -1, y: 1},  // P1
    {x: -1, y: -1}, // P2
    {x: 1, y: -1}   // P3
  ];
  const d = dirs[player];
  const start = baseXY(player, 0);
  // start near ring, go inward
  const start2 = {
    x: (start.x + CENTER.x) / 2,
    y: (start.y + CENTER.y) / 2
  };
  const gap = 28;
  return {
    x: start2.x + d.x * gap * step,
    y: start2.y + d.y * gap * step
  };
}

function getPiecePositionValue(pieceCell){
  // backend sends objects like { position, homeStraight }
  if (pieceCell && typeof pieceCell === "object"){
    if (typeof pieceCell.position === "number") return pieceCell.position;
  }
  // or could be a plain number
  if (typeof pieceCell === "number") return pieceCell;
  return null;
}

function pieceXY(state, player, k){
  const cell = state.pieces?.[player]?.[k];
  let pos = getPiecePositionValue(cell);

  if (pos === -1) return baseXY(player, k);
  if (typeof pos === "number" && pos >= 0 && pos <= 51) return ringXY(pos);

  // Home path support:
  // 1) If backend uses 52..57 mapping, use pos-52.
  if (typeof pos === "number" && pos >= 52 && pos <= 57) return homeXY(player, pos - 52);

  // 2) If backend uses -2 for home path, look for a separate home step.
  if (pos === -2){
    const hs =
      state.home?.[player]?.[k] ??
      state.homeStraight?.[player]?.[k] ??
      (cell && typeof cell === "object" ? cell.homeStraight : undefined);

    if (typeof hs === "number") return homeXY(player, Math.min(5, Math.max(0, hs)));
  }

  // fallback
  return CENTER;
}

function onNewState(s){
  // detect one moved piece (compare VALUES, not object identity)
  if (lastState?.pieces && s?.pieces){
    let moved = null;

    for(let p=0;p<4;p++){
      for(let k=0;k<4;k++){
        const a = getPiecePositionValue(lastState.pieces?.[p]?.[k]);
        const b = getPiecePositionValue(s.pieces?.[p]?.[k]);

        // if either is null, skip (state not ready)
        if (a === null || b === null) continue;

        if (a !== b){
          moved = { p, k };
          break;
        }
      }
      if (moved) break;
    }

    if (moved){
      const from = pieceXY(lastState, moved.p, moved.k);
      const to = pieceXY(s, moved.p, moved.k);

      // only animate if it actually changed screen position
      if (from.x !== to.x || from.y !== to.y){
        anim = { startTime: performance.now(), duration: 220, from, to, ...moved };
      }
    }
  }

  lastState = s;
  updateSidePanel(s);

  // Start render loop once (avoid stacking multiple RAF loops)
  if (!renderLoopRunning){
    renderLoopRunning = true;
    requestAnimationFrame(draw);
  }
}

function updateSidePanel(s){
  const cp = s.current_player ?? 0;
  const dice = s.dice ?? 0;
  const round = s.round ?? 0;

  const mcell = (s.mystery_cell ?? -1);
  const mleft = (s.mystery_turns_left ?? 0);

  elState.textContent =
`Round: ${round}
Current Player: ${cp + 1} (${colors[cp].name})
Dice: ${dice}
Paused: ${s.paused ?? 0}
Winner: ${(s.winner ?? -1) === -1 ? "None" : "Player " + ((s.winner ?? 0) + 1)}
Mystery Cell: ${mcell === -1 ? "Inactive" : mcell}
Mystery Turns Left: ${mcell === -1 ? "-" : mleft}`;

  if (Array.isArray(s.log) && s.log.length){
    elLog.textContent = s.log.join("\n");
  } else {
    elLog.textContent = "(no log yet)";
  }
}

function lerp(a,b,t){ return a + (b-a)*t; }

function draw(now){
  if (!lastState){
    requestAnimationFrame(draw);
    return;
  }

  ctx.clearRect(0,0,canvas.width,canvas.height);

  // background
  ctx.fillStyle = "#0f1420";
  ctx.fillRect(0,0,canvas.width,canvas.height);

  // board border
  ctx.strokeStyle = "rgba(255,255,255,0.18)";
  ctx.lineWidth = 2;
  roundRect(18,18,604,604,18);
  ctx.stroke();

  // center home
  ctx.fillStyle = "rgba(255,255,255,0.05)";
  roundRect(260,260,120,120,14);
  ctx.fill();

  // draw ring cells
  for(let i=0;i<52;i++){
    const pt = ringXY(i);
    const isSafe = SAFE.has(i);
    const isMystery = (lastState.mystery_cell === i);

    ctx.beginPath();
    ctx.arc(pt.x, pt.y, 12, 0, Math.PI*2);

    if (isMystery){
      ctx.fillStyle = "rgba(255, 255, 255, 0.25)";
      ctx.fill();
    } else if (isSafe){
      ctx.fillStyle = "rgba(255, 255, 255, 0.12)";
      ctx.fill();
    } else {
      ctx.fillStyle = "rgba(255, 255, 255, 0.06)";
      ctx.fill();
    }

    ctx.strokeStyle = "rgba(255,255,255,0.12)";
    ctx.stroke();
  }

  // draw home paths (6 each)
  for(let p=0;p<4;p++){
    for(let step=0; step<6; step++){
      const pt = homeXY(p, step);
      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 10, 0, Math.PI*2);
      ctx.fillStyle = "rgba(255,255,255,0.07)";
      ctx.fill();
      ctx.strokeStyle = "rgba(255,255,255,0.12)";
      ctx.stroke();
    }
  }

  // draw bases
  for(let p=0;p<4;p++){
    for(let k=0;k<4;k++){
      const pt = baseXY(p,k);
      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 12, 0, Math.PI*2);
      ctx.fillStyle = "rgba(255,255,255,0.06)";
      ctx.fill();
      ctx.strokeStyle = "rgba(255,255,255,0.12)";
      ctx.stroke();
    }
  }

  // draw pieces (with animation on moved piece)
  const tNow = now ?? performance.now();
  for(let p=0;p<4;p++){
    for(let k=0;k<4;k++){
      let pt = pieceXY(lastState, p, k);

      // animate if this is the moved piece
      if (anim && anim.p === p && anim.k === k){
        const t = Math.min(1, (tNow - anim.startTime) / anim.duration);
        pt = {
          x: lerp(anim.from.x, anim.to.x, easeOut(t)),
          y: lerp(anim.from.y, anim.to.y, easeOut(t))
        };
        if (t >= 1) anim = null;
      }

      drawPiece(pt.x, pt.y, colors[p].fill, (p === (lastState.current_player ?? 0)));
    }
  }

  // current player indicator
  const cp = lastState.current_player ?? 0;
  ctx.font = "16px system-ui, Arial";
  ctx.fillStyle = "rgba(255,255,255,0.85)";
  ctx.fillText(`P${cp+1} turn`, 28, 44);

  // dice
  const dice = lastState.dice ?? 0;
  ctx.fillText(`Dice: ${dice}`, 28, 66);

  requestAnimationFrame(draw);
}

function drawPiece(x,y,color,isCurrent){
  ctx.beginPath();
  ctx.arc(x,y,9,0,Math.PI*2);
  ctx.fillStyle = color;
  ctx.fill();
  ctx.lineWidth = isCurrent ? 3 : 1.5;
  ctx.strokeStyle = isCurrent ? "rgba(255,255,255,0.9)" : "rgba(0,0,0,0.35)";
  ctx.stroke();
}

function roundRect(x,y,w,h,r){
  ctx.beginPath();
  ctx.moveTo(x+r, y);
  ctx.lineTo(x+w-r, y);
  ctx.quadraticCurveTo(x+w, y, x+w, y+r);
  ctx.lineTo(x+w, y+h-r);
  ctx.quadraticCurveTo(x+w, y+h, x+w-r, y+h);
  ctx.lineTo(x+r, y+h);
  ctx.quadraticCurveTo(x, y+h, x, y+h-r);
  ctx.lineTo(x, y+r);
  ctx.quadraticCurveTo(x, y, x+r, y);
  ctx.closePath();
}

function easeOut(t){ return 1 - Math.pow(1 - t, 3); }

// start
setApiFromInput();
fetchState();
// polling (simple real-time)
setInterval(fetchState, 300);
