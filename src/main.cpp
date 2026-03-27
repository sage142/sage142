#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

namespace {
constexpr int kScreenW = 1024;
constexpr int kScreenH = 576;
constexpr int kTile = 32;
constexpr float kGravity = 1500.0f;
constexpr float kRunSpeed = 260.0f;
constexpr float kJumpSpeed = -540.0f;
constexpr float kEnemySpeed = 90.0f;

struct Vec2 {
    float x = 0;
    float y = 0;
};

struct AABB {
    float x;
    float y;
    float w;
    float h;
};

struct Actor {
    Vec2 pos;
    Vec2 vel;
    int w;
    int h;
    bool onGround = false;
};

struct Coin {
    SDL_Rect rect;
    bool taken = false;
};

struct Goomba {
    Actor body;
    int dir = 1;
    bool alive = true;
};

bool Intersects(const AABB& a, const AABB& b) {
    return !(a.x + a.w <= b.x || a.x >= b.x + b.w || a.y + a.h <= b.y || a.y >= b.y + b.h);
}

AABB ToAABB(const Actor& actor) {
    return {actor.pos.x, actor.pos.y, static_cast<float>(actor.w), static_cast<float>(actor.h)};
}

bool SolidAt(int tx, int ty, const std::vector<std::string>& map) {
    if (ty < 0 || ty >= static_cast<int>(map.size())) {
        return false;
    }
    if (tx < 0 || tx >= static_cast<int>(map[ty].size())) {
        return true;
    }
    const char c = map[ty][tx];
    return c == '#' || c == 'B';
}

void ResolveMapCollisions(Actor& actor, float dt, const std::vector<std::string>& map) {
    actor.pos.x += actor.vel.x * dt;
    AABB box = ToAABB(actor);

    int left = static_cast<int>(std::floor(box.x / kTile));
    int right = static_cast<int>(std::floor((box.x + box.w - 1) / kTile));
    int top = static_cast<int>(std::floor(box.y / kTile));
    int bottom = static_cast<int>(std::floor((box.y + box.h - 1) / kTile));

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (!SolidAt(tx, ty, map)) continue;
            SDL_Rect tileRect{tx * kTile, ty * kTile, kTile, kTile};
            AABB t{static_cast<float>(tileRect.x), static_cast<float>(tileRect.y), static_cast<float>(tileRect.w), static_cast<float>(tileRect.h)};
            box = ToAABB(actor);
            if (!Intersects(box, t)) continue;
            if (actor.vel.x > 0) {
                actor.pos.x = t.x - box.w;
            } else if (actor.vel.x < 0) {
                actor.pos.x = t.x + t.w;
            }
            actor.vel.x = 0;
        }
    }

    actor.pos.y += actor.vel.y * dt;
    actor.onGround = false;
    box = ToAABB(actor);

    left = static_cast<int>(std::floor(box.x / kTile));
    right = static_cast<int>(std::floor((box.x + box.w - 1) / kTile));
    top = static_cast<int>(std::floor(box.y / kTile));
    bottom = static_cast<int>(std::floor((box.y + box.h - 1) / kTile));

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (!SolidAt(tx, ty, map)) continue;
            SDL_Rect tileRect{tx * kTile, ty * kTile, kTile, kTile};
            AABB t{static_cast<float>(tileRect.x), static_cast<float>(tileRect.y), static_cast<float>(tileRect.w), static_cast<float>(tileRect.h)};
            box = ToAABB(actor);
            if (!Intersects(box, t)) continue;
            if (actor.vel.y > 0) {
                actor.pos.y = t.y - box.h;
                actor.onGround = true;
            } else if (actor.vel.y < 0) {
                actor.pos.y = t.y + t.h;
            }
            actor.vel.y = 0;
        }
    }
}

void DrawRect(SDL_Renderer* renderer, SDL_Color color, const SDL_Rect& rect, const Vec2& camera) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r{rect.x - static_cast<int>(camera.x), rect.y - static_cast<int>(camera.y), rect.w, rect.h};
    SDL_RenderFillRect(renderer, &r);
}

}  // namespace

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Relocate Plumber", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          kScreenW, kScreenH, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<std::string> map = {
        "........................................................................................................",
        "........................................................................................................",
        "........................................................................................................",
        "........................................................................................................",
        ".................###....................................................................................",
        "...........................................................###..........................................",
        ".......###..............................................................................................",
        "......................................###...............................................................",
        ".............................###............................................###..........................",
        "........................................................................................................",
        "..................B.........................B.....................B.....................................",
        "........................................................................................................",
        "........................................................................................................",
        "........................................................................................................",
        "........................................................................................................",
        "########################################################################################################"
    };

    Actor mario{{80.0f, 200.0f}, {0.0f, 0.0f}, 24, 30, false};
    std::vector<Coin> coins;
    for (int i = 0; i < 12; ++i) {
        Coin c;
        c.rect = SDL_Rect{260 + i * 180, 300 - (i % 3) * 70, 14, 14};
        coins.push_back(c);
    }

    std::vector<Goomba> goombas;
    for (int i = 0; i < 6; ++i) {
        Goomba g;
        g.body = Actor{{460.0f + i * 250.0f, 440.0f}, {-kEnemySpeed, 0.0f}, 24, 24, false};
        g.dir = -1;
        goombas.push_back(g);
    }

    std::array<SDL_Rect, 4> relocationPortals{{
        SDL_Rect{320, 170, 26, 26},
        SDL_Rect{1180, 240, 26, 26},
        SDL_Rect{2020, 180, 26, 26},
        SDL_Rect{2870, 230, 26, 26},
    }};

    SDL_Rect flag{3320, 260, 24, 96};

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> portalPick(0, static_cast<int>(relocationPortals.size()) - 1);

    bool running = true;
    Uint64 prevCounter = SDL_GetPerformanceCounter();
    int score = 0;
    int lives = 3;
    bool won = false;

    while (running) {
        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt = static_cast<float>(now - prevCounter) / static_cast<float>(SDL_GetPerformanceFrequency());
        prevCounter = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        mario.vel.x = 0.0f;
        if (!won) {
            if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) mario.vel.x = -kRunSpeed;
            if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) mario.vel.x = kRunSpeed;
            if ((keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W]) && mario.onGround) {
                mario.vel.y = kJumpSpeed;
            }
        }

        mario.vel.y += kGravity * dt;
        ResolveMapCollisions(mario, std::min(dt, 1.0f / 30.0f), map);

        AABB marioBox = ToAABB(mario);
        for (Coin& coin : coins) {
            if (coin.taken) continue;
            AABB c{static_cast<float>(coin.rect.x), static_cast<float>(coin.rect.y), static_cast<float>(coin.rect.w), static_cast<float>(coin.rect.h)};
            if (Intersects(marioBox, c)) {
                coin.taken = true;
                score += 10;
            }
        }

        for (Goomba& g : goombas) {
            if (!g.alive) continue;
            g.body.vel.x = g.dir * kEnemySpeed;
            g.body.vel.y += kGravity * dt;
            const float beforeX = g.body.pos.x;
            ResolveMapCollisions(g.body, std::min(dt, 1.0f / 30.0f), map);
            if (std::abs(g.body.pos.x - beforeX) < 0.5f) g.dir *= -1;

            AABB gb = ToAABB(g.body);
            if (Intersects(marioBox, gb)) {
                if (mario.vel.y > 120.0f && mario.pos.y + mario.h - 4 < g.body.pos.y) {
                    g.alive = false;
                    mario.vel.y = -360.0f;
                    score += 100;
                } else {
                    lives -= 1;
                    mario.pos = {80.0f, 200.0f};
                    mario.vel = {0.0f, 0.0f};
                    if (lives <= 0) {
                        running = false;
                    }
                }
            }
        }

        for (const SDL_Rect& portal : relocationPortals) {
            AABB p{static_cast<float>(portal.x), static_cast<float>(portal.y), static_cast<float>(portal.w), static_cast<float>(portal.h)};
            if (Intersects(marioBox, p)) {
                int pick = portalPick(rng);
                SDL_Rect dst = relocationPortals[pick];
                if (dst.x == portal.x && dst.y == portal.y) {
                    dst = relocationPortals[(pick + 1) % relocationPortals.size()];
                }
                mario.pos.x = static_cast<float>(dst.x + dst.w + 8);
                mario.pos.y = static_cast<float>(dst.y - mario.h);
                mario.vel = {0.0f, -200.0f};
                score += 5;
                break;
            }
        }

        if (Intersects(marioBox, AABB{static_cast<float>(flag.x), static_cast<float>(flag.y), static_cast<float>(flag.w), static_cast<float>(flag.h)})) {
            won = true;
        }

        Vec2 camera;
        camera.x = std::max(0.0f, mario.pos.x - kScreenW * 0.35f);
        camera.y = 0.0f;

        SDL_SetRenderDrawColor(renderer, 121, 188, 255, 255);
        SDL_RenderClear(renderer);

        for (int y = 0; y < static_cast<int>(map.size()); ++y) {
            for (int x = 0; x < static_cast<int>(map[y].size()); ++x) {
                char t = map[y][x];
                if (t == '#' || t == 'B') {
                    SDL_Color c = (t == '#') ? SDL_Color{174, 114, 66, 255} : SDL_Color{224, 180, 87, 255};
                    DrawRect(renderer, c, SDL_Rect{x * kTile, y * kTile, kTile, kTile}, camera);
                }
            }
        }

        for (const Coin& coin : coins) {
            if (coin.taken) continue;
            DrawRect(renderer, SDL_Color{250, 222, 85, 255}, coin.rect, camera);
        }

        for (const Goomba& g : goombas) {
            if (!g.alive) continue;
            DrawRect(renderer, SDL_Color{153, 84, 52, 255}, SDL_Rect{static_cast<int>(g.body.pos.x), static_cast<int>(g.body.pos.y), g.body.w, g.body.h}, camera);
        }

        for (const SDL_Rect& portal : relocationPortals) {
            DrawRect(renderer, SDL_Color{154, 70, 255, 255}, portal, camera);
        }

        DrawRect(renderer, SDL_Color{20, 160, 50, 255}, flag, camera);
        DrawRect(renderer, SDL_Color{218, 53, 56, 255},
                 SDL_Rect{static_cast<int>(mario.pos.x), static_cast<int>(mario.pos.y), mario.w, mario.h}, camera);

        for (int i = 0; i < lives; ++i) {
            DrawRect(renderer, SDL_Color{218, 53, 56, 255}, SDL_Rect{20 + i * 24, 18, 16, 16}, {0, 0});
        }

        SDL_RenderPresent(renderer);

        static Uint64 hudTick = 0;
        if (now - hudTick > SDL_GetPerformanceFrequency() / 4) {
            hudTick = now;
            std::string title = "Relocate Plumber | Score: " + std::to_string(score) + " | Lives: " + std::to_string(lives);
            if (won) title += " | YOU RELOCATED TO THE GOAL!";
            SDL_SetWindowTitle(window, title.c_str());
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
