import random
import time
import tkinter as tk
from dataclasses import dataclass

WIDTH, HEIGHT = 1100, 700
PLAY_W, PLAY_H = 760, 620
GRAVITY = 0.85
MOVE_SPEED = 5
JUMP_SPEED = -14


@dataclass
class RectObj:
    x: float
    y: float
    w: float
    h: float

    @property
    def left(self):
        return self.x

    @property
    def right(self):
        return self.x + self.w

    @property
    def top(self):
        return self.y

    @property
    def bottom(self):
        return self.y + self.h


class ObbyGame:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Square Obby Online [Fake Multiplayer]")
        self.root.geometry(f"{WIDTH}x{HEIGHT}")
        self.root.resizable(False, False)
        self.root.configure(bg="#1f2430")

        self.keys = set()
        self.chat_feed = ["System: Welcome to Square Obby Online!", "Tip: Reach the green finish platform."]
        self.fake_players = []
        self.last_checkpoint = (50, PLAY_H - 110)

        self._build_ui()
        self._build_level()
        self._spawn_fake_players()

        self.player = RectObj(*self.last_checkpoint, 34, 34)
        self.vel_x = 0.0
        self.vel_y = 0.0
        self.on_ground = False
        self.level_start = time.time()
        self.deaths = 0
        self.best_time = None

        self.root.bind("<KeyPress>", self._on_key_press)
        self.root.bind("<KeyRelease>", self._on_key_release)

        self.loop()

    def _build_ui(self):
        top = tk.Frame(self.root, bg="#171b25", height=50)
        top.pack(fill="x", side="top")
        tk.Label(top, text="◼ Square Obby Online", fg="white", bg="#171b25", font=("Segoe UI", 14, "bold")).pack(
            side="left", padx=12
        )
        self.stats_lbl = tk.Label(top, text="", fg="#cfd8ff", bg="#171b25", font=("Consolas", 11))
        self.stats_lbl.pack(side="left", padx=30)

        body = tk.Frame(self.root, bg="#1f2430")
        body.pack(fill="both", expand=True)

        self.canvas = tk.Canvas(body, width=PLAY_W, height=PLAY_H, bg="#95d0ff", highlightthickness=0)
        self.canvas.pack(side="left", padx=12, pady=12)

        right = tk.Frame(body, width=300, bg="#252b3a")
        right.pack(side="right", fill="y", padx=(0, 12), pady=12)
        right.pack_propagate(False)

        tk.Label(right, text="Players", fg="white", bg="#252b3a", font=("Segoe UI", 12, "bold")).pack(anchor="w", padx=10, pady=(8, 3))
        self.players_box = tk.Listbox(right, bg="#181c27", fg="#ebefff", borderwidth=0, highlightthickness=0, font=("Segoe UI", 10))
        self.players_box.pack(fill="x", padx=10, ipady=5)

        tk.Label(right, text="Chat", fg="white", bg="#252b3a", font=("Segoe UI", 12, "bold")).pack(anchor="w", padx=10, pady=(10, 3))
        self.chat_box = tk.Text(right, bg="#181c27", fg="#dbe0ff", borderwidth=0, highlightthickness=0, height=14, font=("Consolas", 10))
        self.chat_box.pack(fill="both", expand=True, padx=10)
        self.chat_box.configure(state="disabled")

        self.hint_lbl = tk.Label(
            right,
            text="Controls\nA/D or ←/→: move\nW/Space/↑: jump\nR: respawn",
            justify="left",
            fg="#c6ceef",
            bg="#252b3a",
            font=("Segoe UI", 10),
        )
        self.hint_lbl.pack(anchor="w", padx=10, pady=10)

    def _build_level(self):
        self.platforms = [
            RectObj(0, PLAY_H - 35, 120, 35),
            RectObj(170, PLAY_H - 80, 80, 20),
            RectObj(300, PLAY_H - 130, 70, 20),
            RectObj(430, PLAY_H - 190, 70, 20),
            RectObj(560, PLAY_H - 250, 90, 20),
            RectObj(700, PLAY_H - 180, 50, 20),
            RectObj(615, PLAY_H - 100, 80, 20),
            RectObj(480, PLAY_H - 40, 120, 20),
            RectObj(650, PLAY_H - 330, 95, 20),
            RectObj(420, PLAY_H - 380, 80, 20),
            RectObj(260, PLAY_H - 420, 80, 20),
            RectObj(120, PLAY_H - 470, 85, 20),
            RectObj(250, PLAY_H - 530, 95, 20),
            RectObj(450, PLAY_H - 560, 100, 20),
        ]
        self.hazards = [
            RectObj(120, PLAY_H - 30, 60, 10),
            RectObj(370, PLAY_H - 20, 100, 10),
            RectObj(530, PLAY_H - 270, 40, 10),
            RectObj(380, PLAY_H - 400, 30, 10),
            RectObj(220, PLAY_H - 550, 20, 10),
        ]
        self.finish = RectObj(620, PLAY_H - 590, 90, 20)

    def _spawn_fake_players(self):
        names = ["BuilderBro", "NoobMaster", "SpeedCube", "ParkourPro", "BrickQueen"]
        for n in names:
            self.fake_players.append({
                "name": n,
                "x": random.randint(20, 110),
                "y": PLAY_H - 90,
                "vx": random.choice([-2, -1, 1, 2]),
                "color": random.choice(["#ffb347", "#dca0ff", "#7ef5c6", "#f58db5"]),
                "msg_tick": random.randint(90, 260),
            })
        self._refresh_players_ui()
        self._append_chat("System", f"{len(self.fake_players)+1} players in server")

    def _refresh_players_ui(self):
        self.players_box.delete(0, tk.END)
        self.players_box.insert(tk.END, "You")
        for p in self.fake_players:
            self.players_box.insert(tk.END, p["name"])

    def _append_chat(self, who: str, msg: str):
        self.chat_feed.append(f"{who}: {msg}")
        self.chat_feed = self.chat_feed[-10:]
        self.chat_box.configure(state="normal")
        self.chat_box.delete("1.0", tk.END)
        self.chat_box.insert(tk.END, "\n".join(self.chat_feed))
        self.chat_box.configure(state="disabled")
        self.chat_box.see(tk.END)

    def _on_key_press(self, event):
        self.keys.add(event.keysym.lower())
        if event.keysym.lower() == "r":
            self.respawn("Respawned")

    def _on_key_release(self, event):
        self.keys.discard(event.keysym.lower())

    def _intersect(self, a: RectObj, b: RectObj):
        return a.right > b.left and a.left < b.right and a.bottom > b.top and a.top < b.bottom

    def respawn(self, reason="Oof"):
        self.player.x, self.player.y = self.last_checkpoint
        self.vel_x = 0
        self.vel_y = 0
        self.deaths += 1
        self._append_chat("System", reason)

    def _simulate_fake_players(self):
        barks = ["lol", "easy jump", "who touched lava", "gg", "follow me", "im lagging"]
        for p in self.fake_players:
            p["x"] += p["vx"]
            if p["x"] < 20 or p["x"] > PLAY_W - 70:
                p["vx"] *= -1

            p["msg_tick"] -= 1
            if p["msg_tick"] <= 0 and random.random() < 0.11:
                self._append_chat(p["name"], random.choice(barks))
                p["msg_tick"] = random.randint(120, 260)

    def _physics(self):
        moving_left = "a" in self.keys or "left" in self.keys
        moving_right = "d" in self.keys or "right" in self.keys
        jumping = "space" in self.keys or "w" in self.keys or "up" in self.keys

        self.vel_x = 0
        if moving_left:
            self.vel_x = -MOVE_SPEED
        if moving_right:
            self.vel_x = MOVE_SPEED

        if jumping and self.on_ground:
            self.vel_y = JUMP_SPEED
            self.on_ground = False

        # horizontal move
        self.player.x += self.vel_x
        self.player.x = max(0, min(PLAY_W - self.player.w, self.player.x))

        # vertical move
        self.vel_y += GRAVITY
        self.player.y += self.vel_y
        self.on_ground = False

        for plat in self.platforms + [self.finish]:
            if self._intersect(self.player, plat):
                if self.vel_y >= 0 and self.player.bottom - self.vel_y <= plat.top + 3:
                    self.player.y = plat.top - self.player.h
                    self.vel_y = 0
                    self.on_ground = True

        for hz in self.hazards:
            if self._intersect(self.player, hz):
                self.respawn("You touched lava")
                return

        if self.player.y > PLAY_H + 100:
            self.respawn("You fell")
            return

        if self._intersect(self.player, self.finish):
            elapsed = time.time() - self.level_start
            if self.best_time is None or elapsed < self.best_time:
                self.best_time = elapsed
            self._append_chat("System", f"Finish! Time: {elapsed:.2f}s")
            self.last_checkpoint = (50, PLAY_H - 110)
            self.player.x, self.player.y = self.last_checkpoint
            self.vel_y = 0
            self.level_start = time.time()

    def _draw(self):
        c = self.canvas
        c.delete("all")
        c.create_rectangle(0, 0, PLAY_W, PLAY_H, fill="#8fd2ff", width=0)

        # decorative stripes to mimic obby skybox style
        for i in range(0, PLAY_W, 50):
            c.create_rectangle(i, 0, i + 25, PLAY_H, fill="#9fddff", outline="")

        for p in self.platforms:
            c.create_rectangle(p.left, p.top, p.right, p.bottom, fill="#3c425c", outline="#272c41")
        for h in self.hazards:
            c.create_rectangle(h.left, h.top, h.right, h.bottom, fill="#ff4a4a", outline="#9c1010")
        c.create_rectangle(self.finish.left, self.finish.top, self.finish.right, self.finish.bottom, fill="#53e06f", outline="#2a913c")
        c.create_text(self.finish.left + 6, self.finish.top - 12, text="FINISH", anchor="w", font=("Segoe UI", 9, "bold"), fill="#1f5a2a")

        # fake players
        for p in self.fake_players:
            c.create_rectangle(p["x"], p["y"], p["x"] + 28, p["y"] + 28, fill=p["color"], outline="#000")
            c.create_text(p["x"] + 14, p["y"] - 9, text=p["name"], font=("Segoe UI", 8), fill="#222")

        c.create_rectangle(
            self.player.left,
            self.player.top,
            self.player.right,
            self.player.bottom,
            fill="#ffd041",
            outline="#4d3f00",
            width=2,
        )
        c.create_text(self.player.x + 17, self.player.y - 10, text="You", font=("Segoe UI", 8, "bold"), fill="#111")

        elapsed = time.time() - self.level_start
        best = f"{self.best_time:.2f}s" if self.best_time is not None else "--"
        self.stats_lbl.configure(text=f"Time: {elapsed:.2f}s   Best: {best}   Deaths: {self.deaths}")

    def loop(self):
        self._simulate_fake_players()
        self._physics()
        self._draw()
        self.root.after(16, self.loop)


if __name__ == "__main__":
    root = tk.Tk()
    ObbyGame(root)
    root.mainloop()
