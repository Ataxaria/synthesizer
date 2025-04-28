#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define _USE_MATH_DEFINES
#define TWO_PI M_PI * 2.0

typedef struct {
    double phase;
    double frequency;
    double amplitude;
    int sample_rate;
} SineWaveData;

void SquareWaveCallback(void*, Uint8*, int);
void DrawPianoKeys(SDL_Renderer*, TTF_Font*, const char*, int);
void DrawAdditionalInfo(SDL_Renderer*, TTF_Font*, double, int);

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_AudioSpec spec;
    SDL_AudioDeviceID device;
    SineWaveData sinusoid = {0.0, 440.0, 0.5, 44100};

    SDL_zero(spec);
    spec.freq = sinusoid.sample_rate;
    spec.format = AUDIO_F32SYS;
    spec.channels = 1;
    spec.samples = 4096;
    spec.callback = SquareWaveCallback;
    spec.userdata = &sinusoid;

    device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (device == 0) {
        printf("SDL_OpenAudio failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Synthesizer",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          637, 300, 0);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const char* keys = "zsxcfvgbnjmk,";
    int octave = 4;
    int currentKey = -1;
    bool quit = false;
    SDL_Event event;

    SDL_PauseAudioDevice(device, 1);

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP && octave < 7) {
                    octave++;
                } else if (event.key.keysym.sym == SDLK_DOWN && octave > 1) {
                    octave--;
                } else {
                    for (int k = 0; k < strlen(keys); k++) {
                        if (event.key.keysym.sym == (unsigned char)keys[k]) {
                            int keyNum = k + 1 + 12 * (octave - 1);
                            sinusoid.frequency = pow(2.0, (keyNum - 49) / 12.0) * 440;
                            SDL_PauseAudioDevice(device, 0);
                            currentKey = k;
                        }
                    }
                }
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == (unsigned char)keys[currentKey]) {
                    currentKey = -1;
                    SDL_PauseAudioDevice(device, 1);
                }
            }
        }

        SDL_RenderClear(renderer);
        DrawPianoKeys(renderer, font, keys, currentKey);
        DrawAdditionalInfo(renderer, font, sinusoid.frequency, octave);
        SDL_RenderPresent(renderer);

        SDL_Delay(10);
    }

    TTF_CloseFont(font);
    SDL_CloseAudioDevice(device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

void SquareWaveCallback(void *userdata, Uint8 *stream, int len)
{
    SineWaveData *data = (SineWaveData *)userdata;
    float *buffer = (float *)stream;
    len /= sizeof(float);

    for (int i = 0; i < len; i++) {
        if (sin(data->phase) >= 0) {
            buffer[i] = (float)(data->amplitude);
        } else {
            buffer[i] = (float)(-data->amplitude);
        }

        data->phase += TWO_PI * data->frequency / data->sample_rate;

        if (data->phase >= TWO_PI) {
            data->phase -= TWO_PI;
        }
    }
}

void DrawPianoKeys(SDL_Renderer* renderer, TTF_Font* font, const char* keys, int currentKey)
{
    int keyWidth = 637 / 13;
    int keyHeight = 240;
    SDL_Rect rect;
    
    for (int k = 0; k < 13; k++) {
        rect.x = k * keyWidth;
        rect.y = 0;
        rect.w = keyWidth;
        rect.h = keyHeight;

        if (k == currentKey) {
            SDL_SetRenderDrawColor(renderer, 69, 104, 120, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 137, 207, 240, 255);
        }
        SDL_RenderFillRect(renderer, &rect);
        SDL_RenderDrawRect(renderer, &rect);

        SDL_Color textColor = {0, 0, 0, 255};
        char text[2] = {keys[k], '\0'};
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect;
        textRect.x = rect.x + (keyWidth - textSurface->w) / 2;
        if (k == 1 || k == 4 || k == 6 || k == 9 || k == 11) {
            textRect.y = rect.y + (keyHeight - textSurface->h) - 60;
        } else {
            textRect.y = rect.y + (keyHeight - textSurface->h) - 20;
        }
        
        textRect.w = textSurface->w;
        textRect.h = textSurface->h;
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }
}

void DrawAdditionalInfo(SDL_Renderer* renderer, TTF_Font* font, double frequency, int octave)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = {0, 240, 637, 60};
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color textColor = {0, 0, 0, 255};
    char infoText[64];
    sprintf(infoText, "Frequency: %.2f Hz               Octave: %d", frequency, octave);
    SDL_Surface* infoSurface = TTF_RenderText_Solid(font, infoText, textColor);
    SDL_Texture* infoTexture = SDL_CreateTextureFromSurface(renderer, infoSurface);
    SDL_Rect infoRect = {20, 255, infoSurface->w, infoSurface->h};
    SDL_RenderCopy(renderer, infoTexture, NULL, &infoRect);
    SDL_FreeSurface(infoSurface);
    SDL_DestroyTexture(infoTexture);
}
