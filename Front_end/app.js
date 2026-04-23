let API = "http://localhost:8080";

const canvas = document.getElementById("board");
const ctx = canvas.getContext("2d");

const elState = document.getElementById("state");
const elLog = document.getElementById("log");
const elSubtitle = document.getElementById("subtitle");
const elApi = document.getElementById("api");
const elReconnect = document.getElementById("reconnect");
const elRoll = document.getElementById("roll");
const elPause = document.getElementById("pause");
const elResume = document.getElementById("resume");

const colors = [
  { name:"Yellow", fill:"#ffd84d", dark:"#caa61d" },
  { name:"Blue",   fill:"#4da3ff", dark:"#2a6fbc" },
  { name:"Red",    fill:"#ff4d4d", dark:"#ba2d2d" },
  { name:"Green",  fill:"#44dd77", dark:"#239d4b" }
];

// Ludo safe cells (L2 L15 L28 L41) are 0-based indices: 2, 15, 28, 41.
const SAFE = new Set([1, 14, 27, 40]);
// Backend start positions are approachCell + 2 => 2, 15, 28, 41.
const START_INDEX_BY_PLAYER = [1, 14, 27, 40];
const START_PLAYER_BY_INDEX = new Map(START_INDEX_BY_PLAYER.map((idx, p) => [idx, p]));

let lastState = null;
let anim = null; // {startTime, duration, from, to, p, k}
let renderLoopRunning = false;

function setApiFromInput(){
  API = (elApi?.value || API).trim();
}

if (elReconnect){
  elReconnect.onclick = () => {
    setApiFromInput();
    fetchState();
  };
}

if (elRoll) elRoll.onclick = () => sendCmd("STEP");
if (elPause) elPause.onclick = () => sendCmd("PAUSE");
if (elResume) elResume.onclick = () => sendCmd("RESUME");

async function sendCmd(cmd){
  try{
    await fetch(API + "/cmd", {
      method: "POST",
      headers: { "Content-Type": "text/plain" },
      body: cmd
    });
    await fetchState();
  }catch(e){
    if (elSubtitle) elSubtitle.textContent = "Server offline / command failed";
  }
}

async function fetchState(){
  try{
    const r = await fetch(API + "/state", { cache: "no-store" });
    const s = await r.json();
    if (elSubtitle) elSubtitle.textContent = `Connected: ${API}`;
    onNewState(s);
  }catch(e){
    if (elSubtitle) elSubtitle.textContent = "Not connected (start backend on localhost:8080)";
  }
}

// --- Coordinate system (classic 15x15 Ludo grid) ---
const GRID = 15;
const CELL = 34; // 15 * 34 = 510
const OFFSET = 15;
const CENTER = { x: OFFSET + CELL * 7 + CELL / 2, y: OFFSET + CELL * 7 + CELL / 2 };

function gridXY(r, c){
  return { x: OFFSET + c * CELL + CELL / 2, y: OFFSET + r * CELL + CELL / 2 };
}

// 52-track coordinates (row, col) in clockwise order starting at left start.
const TRACK = [
  [6,0],[6,1],[6,2],[6,3],[6,4],[6,5],
  [5,6],[4,6],[3,6],[2,6],[1,6],
  [0,6],[0,7],[0,8],
  [1,8],[2,8],[3,8],[4,8],[5,8],
  [6,9],[6,10],[6,11],[6,12],[6,13],[6,14],
  [7,14],[8,14],
  [8,13],[8,12],[8,11],[8,10],[8,9],
  [9,8],[10,8],[11,8],[12,8],[13,8],
  [14,8],[14,7],[14,6],
  [13,6],[12,6],[11,6],[10,6],[9,6],
  [8,5],[8,4],[8,3],[8,2],[8,1],[8,0],
  [7,0]
];

const HOME_PATHS = [
  // Player 0 (Yellow - left) -> towards center along row 7
  [[7,1],[7,2],[7,3],[7,4],[7,5],[7,6]],
  // Player 1 (Blue - top) -> towards center along col 7
  [[1,7],[2,7],[3,7],[4,7],[5,7],[6,7]],
  // Player 2 (Red - right) -> towards center along row 7
  [[7,13],[7,12],[7,11],[7,10],[7,9],[7,8]],
  // Player 3 (Green - bottom) -> towards center along col 7
  [[13,7],[12,7],[11,7],[10,7],[9,7],[8,7]]
];

function baseXY(player, pieceIndex){
  // 4 corner bases, each with 4 slots in a 2x2 block
  const baseCells = [
    [[1,1],[1,3],[3,1],[3,3]],       // P0 top-left
    [[1,11],[1,13],[3,11],[3,13]],   // P1 top-right
    [[11,11],[11,13],[13,11],[13,13]], // P2 bottom-right
    [[11,1],[11,3],[13,1],[13,3]]    // P3 bottom-left
  ];
  const [r,c] = baseCells[player][pieceIndex % 4];
  return gridXY(r,c);
}

function homeXY(player, step){ // step 0..5
  const coord = HOME_PATHS[player][Math.min(5, Math.max(0, step))];
  return gridXY(coord[0], coord[1]);
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
  if (typeof pos === "number" && pos >= 0 && pos <= 51) {
    const coord = TRACK[pos];
    return gridXY(coord[0], coord[1]);
  }

  // Home path support:
  // 1) If backend uses 52..57 mapping, use pos-52.
  if (typeof pos === "number" && pos >= 52 && pos <= 57) return homeXY(player, pos - 52);

  // 2) If backend uses -2 for home path, look for a separate home step.
  if (pos === -2){
    const hs =
      state.home?.[player]?.[k] ??
      state.homeStraight?.[player]?.[k] ??
      (cell && typeof cell === "object" ? cell.homeStraight : undefined);

    // Backend homeStraight is 1..6, convert to 0..5 index for HOME_PATHS.
    if (typeof hs === "number") return homeXY(player, Math.min(5, Math.max(0, hs - 1)));
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
  if (!elState || !elLog) return;

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

function drawCellRect(r, c, color, alpha=1){
  ctx.save();
  ctx.globalAlpha = alpha;
  ctx.fillStyle = color;
  ctx.fillRect(OFFSET + c * CELL, OFFSET + r * CELL, CELL, CELL);
  ctx.restore();
}

function drawCellBorder(r, c, color, width=1){
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.strokeRect(OFFSET + c * CELL, OFFSET + r * CELL, CELL, CELL);
}

function drawGrid(){
  ctx.strokeStyle = "#d8d8d8";
  ctx.lineWidth = 1;
  for(let i=0;i<=GRID;i++){
    const p = OFFSET + i * CELL;
    ctx.beginPath();
    ctx.moveTo(OFFSET, p);
    ctx.lineTo(OFFSET + GRID * CELL, p);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(p, OFFSET);
    ctx.lineTo(p, OFFSET + GRID * CELL);
    ctx.stroke();
  }
}

function drawBoardLayout(){
  const boardStart = OFFSET;
  const boardSize = CELL * GRID;

  // white board base
  ctx.fillStyle = "#f8f8f8";
  ctx.fillRect(boardStart, boardStart, boardSize, boardSize);

  // corner home quadrants (classic bold ludo colors)
  const baseAreas = [
    { r0:0, c0:0, color: colors[0].fill },   // Yellow
    { r0:0, c0:9, color: colors[1].fill },   // Blue
    { r0:9, c0:9, color: colors[2].fill },   // Red
    { r0:9, c0:0, color: colors[3].fill }    // Green
  ];

  baseAreas.forEach((b, idx) => {
    for(let r=b.r0; r<b.r0+6; r++){
      for(let c=b.c0; c<b.c0+6; c++){
        drawCellRect(r, c, b.color, 0.95);
      }
    }

    // inner white house square
    for(let r=b.r0+1; r<b.r0+5; r++){
      for(let c=b.c0+1; c<b.c0+5; c++){
        drawCellRect(r, c, "#ffffff", 1);
      }
    }

    // Token nests are rendered in the main draw pass with piece slots.
  });

  // home paths to center
  for(let p=0; p<4; p++){
    HOME_PATHS[p].forEach(([r,c]) => drawCellRect(r, c, colors[p].fill, 0.92));
  }

  // center triangles (original ludo style)
  const centerX = OFFSET + CELL * 7.5;
  const centerY = OFFSET + CELL * 7.5;
  const span = CELL * 1.5;

  const tris = [
    // left, top, right, bottom (matching player index order)
    { color: colors[0].fill, pts: [[centerX - span, centerY - span], [centerX - span, centerY + span], [centerX, centerY]] },
    { color: colors[1].fill, pts: [[centerX - span, centerY - span], [centerX + span, centerY - span], [centerX, centerY]] },
    { color: colors[2].fill, pts: [[centerX + span, centerY - span], [centerX + span, centerY + span], [centerX, centerY]] },
    { color: colors[3].fill, pts: [[centerX - span, centerY + span], [centerX + span, centerY + span], [centerX, centerY]] }
  ];

  tris.forEach(t => {
    ctx.beginPath();
    ctx.moveTo(t.pts[0][0], t.pts[0][1]);
    ctx.lineTo(t.pts[1][0], t.pts[1][1]);
    ctx.lineTo(t.pts[2][0], t.pts[2][1]);
    ctx.closePath();
    ctx.fillStyle = t.color;
    ctx.fill();
    ctx.lineWidth = 1;
    ctx.strokeStyle = "#ffffff";
    ctx.stroke();
  });

  // board outline
  ctx.strokeStyle = "#2d2d2d";
  ctx.lineWidth = 2;
  ctx.strokeRect(boardStart, boardStart, boardSize, boardSize);

  // emphasize lane borders
  for(let i=0; i<GRID; i++){
    drawCellBorder(6, i, "#b8b8b8");
    drawCellBorder(8, i, "#b8b8b8");
    drawCellBorder(i, 6, "#b8b8b8");
    drawCellBorder(i, 8, "#b8b8b8");
  }
}

function draw(now){
  if (!lastState){
    requestAnimationFrame(draw);
    return;
  }

  ctx.clearRect(0,0,canvas.width,canvas.height);

  // table-like background
  ctx.fillStyle = "#d3b37a";
  ctx.fillRect(0,0,canvas.width,canvas.height);

  // board layout
  drawBoardLayout();
  drawGrid();

  // draw track cells (filled squares for classic board look)
  for(let i=0;i<52;i++){
    const [r,c] = TRACK[i];
    const isSafe = SAFE.has(i);
    const isMystery = (lastState.mystery_cell === i);
    const startPlayer = START_PLAYER_BY_INDEX.get(i);
    const isStartCell = startPlayer !== undefined;

    if (isMystery){
      drawCellRect(r, c, "#2b2b2b", 0.95);
    } else if (isStartCell){
      drawCellRect(r, c, colors[startPlayer].fill, 0.95);
    } else if (isSafe){
      drawCellRect(r, c, "#ece6a8", 1);
    } else {
      drawCellRect(r, c, "#ffffff", 1);
    }

    drawCellBorder(r, c, "#bebebe");

    if (isSafe){
      const pt = gridXY(r, c);
      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 6, 0, Math.PI * 2);
      ctx.fillStyle = isStartCell ? colors[startPlayer].dark : "#ad9d2b";
      ctx.fill();
    }

    if (isMystery){
      const pt = gridXY(r, c);
      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 5, 0, Math.PI * 2);
      ctx.fillStyle = "#ffffff";
      ctx.fill();
    }
  }

  // draw bases (piece slots)
  for(let p=0;p<4;p++){
    for(let k=0;k<4;k++){
      const pt = baseXY(p,k);
      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 15, 0, Math.PI*2);
      ctx.save();
      ctx.globalAlpha = 0.5;
      ctx.fillStyle = colors[p].fill;
      ctx.fill();
      ctx.restore();
      ctx.lineWidth = 2;
      ctx.strokeStyle = colors[p].dark;
      ctx.stroke();

      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 4, 0, Math.PI * 2);
      ctx.fillStyle = "rgba(255,255,255,0.95)";
      ctx.fill();
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
  ctx.font = "700 16px Trebuchet MS, Verdana, sans-serif";
  ctx.fillStyle = "#2f2f2f";
  ctx.fillText(`P${cp+1} turn`, 28, 44);

  // dice
  const dice = lastState.dice ?? 0;
  ctx.fillText(`Dice: ${dice}`, 28, 66);

  requestAnimationFrame(draw);
}

function drawPiece(x,y,color,isCurrent){
  ctx.beginPath();
  ctx.arc(x,y,11,0,Math.PI*2);
  ctx.fillStyle = color;
  ctx.fill();
  ctx.lineWidth = isCurrent ? 3 : 2;
  ctx.strokeStyle = isCurrent ? "#111111" : "rgba(0,0,0,0.4)";
  ctx.stroke();

  if (isCurrent){
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, Math.PI * 2);
    ctx.fillStyle = "#ffffff";
    ctx.fill();
  }
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
