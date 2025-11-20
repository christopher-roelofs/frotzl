/*
 * frotzl.c - Simple game launcher for Frotz
 *
 * Lists games in the local "games" directory and launches sfrotz when selected
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_GAMES 256
#define MAX_PATH_LEN 512

/* Colors */
#define COLOR_BG (SDL_Color){20, 20, 30, 255}
#define COLOR_TEXT (SDL_Color){220, 220, 220, 255}
#define COLOR_HIGHLIGHT (SDL_Color){100, 150, 200, 255}
#define COLOR_HIGHLIGHT_TEXT (SDL_Color){255, 255, 255, 255}
#define COLOR_TITLE (SDL_Color){120, 170, 220, 255}

typedef struct {
    char path[MAX_PATH_LEN];
    char name[256];
} GameEntry;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    TTF_Font *title_font;
    GameEntry games[MAX_GAMES];
    int game_count;
    int selected;
    int scroll_offset;
} Launcher;

/* Check if file has a valid game extension */
static int is_game_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;

    return (strcasecmp(ext, ".z3") == 0 ||
            strcasecmp(ext, ".z4") == 0 ||
            strcasecmp(ext, ".z5") == 0 ||
            strcasecmp(ext, ".z8") == 0 ||
            strcasecmp(ext, ".zblorb") == 0 ||
            strcasecmp(ext, ".zlb") == 0 ||
            strcasecmp(ext, ".dat") == 0);
}

/* Scan directory for game files */
static int scan_games(Launcher *launcher) {
    DIR *dir = opendir("games");
    if (!dir) {
        fprintf(stderr, "Cannot open games directory\n");
        return -1;
    }

    launcher->game_count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && launcher->game_count < MAX_GAMES) {
        if (entry->d_name[0] == '.') continue;

        if (is_game_file(entry->d_name)) {
            snprintf(launcher->games[launcher->game_count].path,
                    MAX_PATH_LEN, "games/%s", entry->d_name);

            /* Get display name (filename without extension) */
            strncpy(launcher->games[launcher->game_count].name,
                   entry->d_name, sizeof(launcher->games[0].name) - 1);
            char *dot = strrchr(launcher->games[launcher->game_count].name, '.');
            if (dot) *dot = '\0';

            launcher->game_count++;
        }
    }

    closedir(dir);
    return launcher->game_count;
}

/* Initialize SDL and fonts */
static int launcher_init(Launcher *launcher) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        return -1;
    }

    launcher->window = SDL_CreateWindow(
        "Frotz - Select Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN
    );
    if (!launcher->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return -1;
    }

    launcher->renderer = SDL_CreateRenderer(launcher->window, -1, SDL_RENDERER_ACCELERATED);
    if (!launcher->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(launcher->window);
        return -1;
    }

    /* Load fonts - try common Linux paths */
    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
        NULL
    };

    for (int i = 0; font_paths[i] != NULL; i++) {
        launcher->font = TTF_OpenFont(font_paths[i], 16);
        if (launcher->font) break;
    }

    for (int i = 0; font_paths[i] != NULL; i++) {
        launcher->title_font = TTF_OpenFont(font_paths[i], 24);
        if (launcher->title_font) break;
    }

    if (!launcher->font || !launcher->title_font) {
        fprintf(stderr, "Failed to load fonts\n");
        return -1;
    }

    launcher->selected = 0;
    launcher->scroll_offset = 0;

    return 0;
}

/* Draw text centered */
static void draw_text_centered(Launcher *launcher, const char *text, int y,
                       SDL_Color color, TTF_Font *font) {
    int w, h;
    SDL_GetRendererOutputSize(launcher->renderer, &w, &h);

    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(launcher->renderer, surface);
    if (texture) {
        SDL_Rect dest = {
            (w - surface->w) / 2,
            y,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(launcher->renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

/* Draw text at position */
static void draw_text(Launcher *launcher, const char *text, int x, int y,
              SDL_Color color, TTF_Font *font) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(launcher->renderer, surface);
    if (texture) {
        SDL_Rect dest = {x, y, surface->w, surface->h};
        SDL_RenderCopy(launcher->renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

/* Render the game list */
static void render_launcher(Launcher *launcher) {
    int w, h;
    SDL_GetRendererOutputSize(launcher->renderer, &w, &h);

    /* Clear screen */
    SDL_SetRenderDrawColor(launcher->renderer, COLOR_BG.r, COLOR_BG.g,
                          COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(launcher->renderer);

    /* Title */
    draw_text_centered(launcher, "FROTZ - SELECT GAME", 20, COLOR_TITLE,
                      launcher->title_font);

    /* Instructions */
    char count_str[64];
    snprintf(count_str, sizeof(count_str), "%d games found", launcher->game_count);
    draw_text_centered(launcher, count_str, 55, COLOR_TEXT, launcher->font);

    /* Game list */
    int y = 100;
    int visible_items = (h - 160) / 26;
    int start_idx = launcher->scroll_offset;
    int end_idx = start_idx + visible_items;
    if (end_idx > launcher->game_count) end_idx = launcher->game_count;

    for (int i = start_idx; i < end_idx; i++) {
        SDL_Color color = (i == launcher->selected) ? COLOR_HIGHLIGHT_TEXT : COLOR_TEXT;

        /* Highlight bar for selected item */
        if (i == launcher->selected) {
            SDL_SetRenderDrawColor(launcher->renderer, COLOR_HIGHLIGHT.r,
                                  COLOR_HIGHLIGHT.g, COLOR_HIGHLIGHT.b, 255);
            SDL_Rect highlight = {30, y - 2, w - 60, 24};
            SDL_RenderFillRect(launcher->renderer, &highlight);
        }

        /* Game name with selection indicator */
        char display[280];
        if (i == launcher->selected) {
            snprintf(display, sizeof(display), "> %s", launcher->games[i].name);
        } else {
            snprintf(display, sizeof(display), "  %s", launcher->games[i].name);
        }

        draw_text(launcher, display, 40, y, color, launcher->font);
        y += 26;
    }

    /* Controls help at bottom */
    SDL_SetRenderDrawColor(launcher->renderer, 60, 60, 80, 255);
    SDL_Rect help_bg = {0, h - 60, w, 60};
    SDL_RenderFillRect(launcher->renderer, &help_bg);

    draw_text_centered(launcher, "UP/DOWN - Navigate   ENTER - Play Game   ESC - Quit",
                      h - 40, COLOR_TEXT, launcher->font);

    SDL_RenderPresent(launcher->renderer);
}

/* Handle input */
static int handle_input(Launcher *launcher) {
    SDL_Event event;
    int w, h;
    SDL_GetRendererOutputSize(launcher->renderer, &w, &h);
    int visible_items = (h - 160) / 26;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return -1;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:
                    if (launcher->selected > 0) {
                        launcher->selected--;
                        if (launcher->selected < launcher->scroll_offset) {
                            launcher->scroll_offset = launcher->selected;
                        }
                    }
                    break;

                case SDLK_DOWN:
                    if (launcher->selected < launcher->game_count - 1) {
                        launcher->selected++;
                        if (launcher->selected >= launcher->scroll_offset + visible_items) {
                            launcher->scroll_offset = launcher->selected - visible_items + 1;
                        }
                    }
                    break;

                case SDLK_RETURN:
                case SDLK_SPACE:
                    return 1;

                case SDLK_ESCAPE:
                    return -1;

                default:
                    break;
            }
        }
    }

    return 0;
}

/* Cleanup */
static void launcher_cleanup(Launcher *launcher) {
    if (launcher->font) TTF_CloseFont(launcher->font);
    if (launcher->title_font) TTF_CloseFont(launcher->title_font);
    if (launcher->renderer) SDL_DestroyRenderer(launcher->renderer);
    if (launcher->window) SDL_DestroyWindow(launcher->window);
    TTF_Quit();
    SDL_Quit();
}

/* Launch sfrotz with the selected game */
static int launch_game(const char *game_path, int use_keyboard, int use_fullscreen) {
    char command[1024];
    int offset = 0;

    offset += snprintf(command + offset, sizeof(command) - offset, "./sfrotz");

    if (use_keyboard) {
        offset += snprintf(command + offset, sizeof(command) - offset, " -k");
    }

    if (use_fullscreen) {
        offset += snprintf(command + offset, sizeof(command) - offset, " -F");
    }

    snprintf(command + offset, sizeof(command) - offset, " \"%s\"", game_path);

    printf("Launching: %s\n", command);
    return system(command);
}

int main(int argc, char *argv[]) {
    Launcher launcher;
    memset(&launcher, 0, sizeof(Launcher));

    int use_keyboard = 0;
    int use_fullscreen = 0;

    /* Check for flags */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            use_keyboard = 1;
        } else if (strcmp(argv[i], "-F") == 0) {
            use_fullscreen = 1;
        }
    }

    if (launcher_init(&launcher) < 0) {
        fprintf(stderr, "Failed to initialize launcher\n");
        return 1;
    }

    if (scan_games(&launcher) <= 0) {
        fprintf(stderr, "No games found in games directory\n");
        fprintf(stderr, "Supported formats: .z3, .z4, .z5, .z8, .zblorb, .zlb, .dat\n");
        launcher_cleanup(&launcher);
        return 1;
    }

    /* Main loop */
    int running = 1;
    while (running) {
        render_launcher(&launcher);

        int result = handle_input(&launcher);
        if (result == 1) {
            /* Launch game */
            char *selected_path = launcher.games[launcher.selected].path;
            launcher_cleanup(&launcher);
            launch_game(selected_path, use_keyboard, use_fullscreen);
            return 0;
        } else if (result == -1) {
            /* Quit */
            running = 0;
        }

        SDL_Delay(16);
    }

    launcher_cleanup(&launcher);
    return 0;
}
