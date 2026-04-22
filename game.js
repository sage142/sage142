const canvas = document.getElementById("game");
const ctx = canvas.getContext("2d");

const keys = new Set();
const gravity = 0.75;
const friction = 0.84;

const world = {
  width: 3600,
  height: canvas.height,
  floorY: 460,
  clouds: Array.from({ length: 18 }, (_, i) => ({
    x: i * 240 + Math.random() * 120,
    y: 40 + Math.random() * 130,
    size: 26 + Math.random() * 34,
  })),
  hills: Array.from({ length: 12 }, (_, i) => ({
    x: i * 320,
    h: 120 + Math.random() * 120,
  })),
};

const player = {
  x: 80,
  y: world.floorY - 72,
  w: 46,
  h: 72,
  vx: 0,
  vy: 0,
  speed: 0.95,
  jumpPower: 15,
  grounded: false,
  facing: 1,
};

const goal = {
  x: world.width - 180,
  y: world.floorY - 180,
  w: 20,
  h: 180,
};

const enemies = [
  { x: 760, y: world.floorY - 46, w: 44, h: 42, dir: -1, speed: 1.4, alive: true },
  { x: 1410, y: world.floorY - 46, w: 44, h: 42, dir: 1, speed: 1.2, alive: true },
  { x: 2290, y: world.floorY - 46, w: 44, h: 42, dir: -1, speed: 1.5, alive: true },
];

const blocks = [
  { x: 330, y: 340, w: 70, h: 36, type: "brick" },
  { x: 400, y: 340, w: 70, h: 36, type: "question" },
  { x: 470, y: 340, w: 70, h: 36, type: "brick" },
  { x: 980, y: 300, w: 70, h: 36, type: "brick" },
  { x: 1050, y: 300, w: 70, h: 36, type: "question" },
  { x: 1610, y: 250, w: 70, h: 36, type: "question" },
  { x: 2140, y: 360, w: 70, h: 36, type: "brick" },
];

const platforms = [
  { x: 600, y: 410, w: 130, h: 20 },
  { x: 1220, y: 370, w: 180, h: 20 },
  { x: 1840, y: 330, w: 160, h: 20 },
  { x: 2610, y: 300, w: 140, h: 20 },
];

let coins = 0;
let won = false;
let message = "Reach the flag!";

const coinItems = [
  { x: 434, y: 290, r: 12, taken: false },
  { x: 1084, y: 250, r: 12, taken: false },
  { x: 1644, y: 200, r: 12, taken: false },
  { x: 1865, y: 285, r: 12, taken: false },
  { x: 2642, y: 255, r: 12, taken: false },
];

function reset() {
  player.x = 80;
  player.y = world.floorY - player.h;
  player.vx = 0;
  player.vy = 0;
  player.grounded = false;
  coins = 0;
  won = false;
  message = "Reach the flag!";
  coinItems.forEach((coin) => {
    coin.taken = false;
  });
  enemies.forEach((enemy, index) => {
    enemy.alive = true;
    enemy.x = [760, 1410, 2290][index];
  });
}

function intersects(a, b) {
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

function handleMovement() {
  const left = keys.has("ArrowLeft") || keys.has("a");
  const right = keys.has("ArrowRight") || keys.has("d");
  const jump = keys.has("ArrowUp") || keys.has("w") || keys.has(" ");

  if (left) {
    player.vx -= player.speed;
    player.facing = -1;
  }
  if (right) {
    player.vx += player.speed;
    player.facing = 1;
  }

  if (!left && !right) {
    player.vx *= friction;
  }

  player.vx = Math.max(-7, Math.min(7, player.vx));

  if (jump && player.grounded) {
    player.vy = -player.jumpPower;
    player.grounded = false;
  }
}

function stepPhysics() {
  handleMovement();

  player.vy += gravity;
  player.x += player.vx;

  if (player.x < 0) {
    player.x = 0;
    player.vx = 0;
  }
  if (player.x + player.w > world.width) {
    player.x = world.width - player.w;
    player.vx = 0;
  }

  player.y += player.vy;
  player.grounded = false;

  const solids = [
    { x: 0, y: world.floorY, w: world.width, h: world.height - world.floorY },
    ...blocks,
    ...platforms,
  ];

  for (const solid of solids) {
    if (!intersects(player, solid)) continue;

    const prevY = player.y - player.vy;
    const prevBottom = prevY + player.h;
    const prevTop = prevY;

    if (prevBottom <= solid.y + 2) {
      player.y = solid.y - player.h;
      player.vy = 0;
      player.grounded = true;
    } else if (prevTop >= solid.y + solid.h - 2) {
      player.y = solid.y + solid.h;
      player.vy = Math.max(0.5, player.vy);
    } else if (player.x + player.w / 2 < solid.x + solid.w / 2) {
      player.x = solid.x - player.w;
      player.vx = 0;
    } else {
      player.x = solid.x + solid.w;
      player.vx = 0;
    }
  }

  if (player.y > world.height + 100) {
    message = "You fell! Press R to restart.";
  }

  for (const enemy of enemies) {
    if (!enemy.alive) continue;
    enemy.x += enemy.speed * enemy.dir;

    const patrolMin = enemy === enemies[0] ? 700 : enemy === enemies[1] ? 1330 : 2200;
    const patrolMax = patrolMin + 260;
    if (enemy.x < patrolMin || enemy.x + enemy.w > patrolMax) {
      enemy.dir *= -1;
    }

    if (!intersects(player, enemy)) continue;

    if (player.vy > 2 && player.y + player.h - enemy.y < 24) {
      enemy.alive = false;
      player.vy = -10;
      message = "Nice stomp!";
    } else {
      message = "Ouch! Press R to restart.";
    }
  }

  for (const coin of coinItems) {
    if (coin.taken) continue;
    const hit =
      player.x < coin.x + coin.r &&
      player.x + player.w > coin.x - coin.r &&
      player.y < coin.y + coin.r &&
      player.y + player.h > coin.y - coin.r;
    if (hit) {
      coin.taken = true;
      coins += 1;
      message = "Coin collected!";
    }
  }

  if (player.x + player.w > goal.x && player.y < goal.y + goal.h) {
    won = true;
    message = "Course clear! Press R to play again.";
  }
}

function drawBackground(cameraX) {
  ctx.fillStyle = "#5ec9ff";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  for (const cloud of world.clouds) {
    const x = cloud.x - cameraX * 0.25;
    ctx.fillStyle = "rgba(255,255,255,0.9)";
    ctx.beginPath();
    ctx.arc(x, cloud.y, cloud.size, 0, Math.PI * 2);
    ctx.arc(x + cloud.size * 0.8, cloud.y + 6, cloud.size * 0.75, 0, Math.PI * 2);
    ctx.arc(x - cloud.size * 0.9, cloud.y + 8, cloud.size * 0.6, 0, Math.PI * 2);
    ctx.fill();
  }

  for (const hill of world.hills) {
    const x = hill.x - cameraX * 0.45;
    ctx.fillStyle = "#45ae5c";
    ctx.beginPath();
    ctx.moveTo(x, world.floorY);
    ctx.quadraticCurveTo(x + 120, world.floorY - hill.h, x + 240, world.floorY);
    ctx.closePath();
    ctx.fill();
  }
}

function drawWorld(cameraX) {
  ctx.fillStyle = "#8f5b2e";
  ctx.fillRect(-cameraX, world.floorY, world.width, world.height - world.floorY);

  blocks.forEach((block) => {
    const x = block.x - cameraX;
    ctx.fillStyle = block.type === "question" ? "#ffca48" : "#a55a2a";
    ctx.fillRect(x, block.y, block.w, block.h);
    ctx.strokeStyle = "#5a2b09";
    ctx.strokeRect(x + 2, block.y + 2, block.w - 4, block.h - 4);
    if (block.type === "question") {
      ctx.fillStyle = "#7b3f00";
      ctx.font = "bold 24px sans-serif";
      ctx.fillText("?", x + block.w / 2 - 7, block.y + 26);
    }
  });

  platforms.forEach((platform) => {
    const x = platform.x - cameraX;
    ctx.fillStyle = "#be8742";
    ctx.fillRect(x, platform.y, platform.w, platform.h);
  });

  coinItems.forEach((coin) => {
    if (coin.taken) return;
    const x = coin.x - cameraX;
    ctx.fillStyle = "#f8d530";
    ctx.beginPath();
    ctx.arc(x, coin.y, coin.r, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = "#c49707";
    ctx.stroke();
  });

  enemies.forEach((enemy) => {
    if (!enemy.alive) return;
    const x = enemy.x - cameraX;
    ctx.fillStyle = "#7a3d17";
    ctx.fillRect(x, enemy.y, enemy.w, enemy.h);
    ctx.fillStyle = "#f7ebcf";
    ctx.fillRect(x + 7, enemy.y + 10, 10, 8);
    ctx.fillRect(x + 26, enemy.y + 10, 10, 8);
  });

  const poleX = goal.x - cameraX;
  ctx.fillStyle = "#f0f0f0";
  ctx.fillRect(poleX, goal.y, goal.w, goal.h);
  ctx.fillStyle = "#5fd35f";
  ctx.fillRect(poleX + goal.w, goal.y + 10, 40, 28);
}

function drawPlayer(cameraX) {
  const x = player.x - cameraX;
  ctx.fillStyle = "#d3381c";
  ctx.fillRect(x + 6, player.y, player.w - 12, 20);
  ctx.fillStyle = "#2557d6";
  ctx.fillRect(x + 5, player.y + 20, player.w - 10, player.h - 20);
  ctx.fillStyle = "#f4c49a";
  ctx.fillRect(x + 10, player.y + 16, player.w - 20, 20);
  ctx.fillStyle = "#512203";
  const eyeX = player.facing > 0 ? x + 30 : x + 14;
  ctx.fillRect(eyeX, player.y + 24, 5, 5);
}

function drawHud() {
  ctx.fillStyle = "rgba(0,0,0,0.25)";
  ctx.fillRect(10, 10, 310, 88);

  ctx.fillStyle = "white";
  ctx.font = "bold 24px sans-serif";
  ctx.fillText(`COINS: ${coins}/${coinItems.length}`, 24, 44);
  ctx.font = "bold 20px sans-serif";
  ctx.fillText(message, 24, 78);

  if (won) {
    ctx.fillStyle = "rgba(20, 60, 20, 0.68)";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = "#fff";
    ctx.font = "bold 58px sans-serif";
    ctx.fillText("COURSE CLEAR!", canvas.width / 2 - 220, canvas.height / 2 - 10);
    ctx.font = "bold 26px sans-serif";
    ctx.fillText("Press R to restart", canvas.width / 2 - 120, canvas.height / 2 + 40);
  }
}

function loop() {
  if (!won && !message.startsWith("Ouch") && !message.startsWith("You fell")) {
    stepPhysics();
  }

  const cameraX = Math.max(0, Math.min(player.x - canvas.width / 2 + player.w / 2, world.width - canvas.width));

  drawBackground(cameraX);
  drawWorld(cameraX);
  drawPlayer(cameraX);
  drawHud();

  requestAnimationFrame(loop);
}

window.addEventListener("keydown", (event) => {
  const key = event.key.length === 1 ? event.key.toLowerCase() : event.key;
  if (["ArrowLeft", "ArrowRight", "ArrowUp", " ", "a", "d", "w"].includes(key)) {
    event.preventDefault();
  }
  if (key === "r") {
    reset();
  }
  keys.add(key);
});

window.addEventListener("keyup", (event) => {
  const key = event.key.length === 1 ? event.key.toLowerCase() : event.key;
  keys.delete(key);
});

reset();
loop();
