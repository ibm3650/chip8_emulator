#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "SDL.h"

#define MEMORY_SIZE 4096
#define STACK_SIZE 64
#define REGISTERS_COUNT 16
#define FRAMEBUFFER_X   64
#define FRAMEBUFFER_Y   32
#define BASE_FONTSET_OFFSET  0x50
#define RESERVED_SPRITE_SIZE    5
#define PROGRAM_OFFSET  0x200


#define V0  0x0
#define V1  0x1
#define V2  0x2
#define V3  0x3
#define V4  0x4
#define V5  0x5
#define V6  0x6
#define V7  0x7
#define V8  0x8
#define V9  0x9
#define VA  0xA
#define VB  0xB
#define VC  0xC
#define VD  0xD
#define VE  0xE
#define VF  0xF


static uint8_t memory[MEMORY_SIZE];
static uint8_t REGISTERS[REGISTERS_COUNT];
static uint8_t framebuffer[FRAMEBUFFER_X * FRAMEBUFFER_Y];
static uint16_t STACK[STACK_SIZE / sizeof(uint16_t)];
static uint8_t SP = -1;
static uint8_t DT = 0;
static uint8_t ST = 0;
static uint16_t I = 0;
static uint16_t PC = PROGRAM_OFFSET;

SDL_Renderer* renderer;

void prinf_frame(){
    system("cls");
    for (int y = 0; y < FRAMEBUFFER_Y; ++y) {
        for (int x = 0; x < FRAMEBUFFER_X; ++x) {
            printf("%c", framebuffer[x+ FRAMEBUFFER_X * y] ? '*' : ' ');
        }
        printf("\r\n");
    }
}

void clear_display(){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void draw_display(){
    for (int y = 0; y < FRAMEBUFFER_Y; y++) {
        for (int x = 0; x < FRAMEBUFFER_X; x++) {
            if (framebuffer[y * FRAMEBUFFER_X + x] == 1) {
                SDL_Rect rect = { x * 10, y * 10, 10, 10 };
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }


}



static inline void opcode_sys(uint16_t opcode){}
//OK
static inline void opcode_cls(uint16_t opcode){
    //printf("CLS\r\n");
    //system("cls");
    clear_display();
}
//OK
static inline void opcode_ret(){
    PC = STACK[SP--];
}
//OK
static inline void opcode_jp(uint16_t opcode){
    //printf("JP %#05x\r\n", opcode & 0xFFF);
    //PC = opcode & 0xFFF;
    PC = (opcode & 0xFFF) - 2;
}
//OK
static inline void opcode_call(uint16_t opcode){
    STACK[++SP] = PC;
    //PC = opcode & 0xFFF;
    PC = (opcode & 0xFFF) - 2;
}
//OK
static inline void opcode_se_reg_val(uint16_t opcode){
    //printf("%#05x: SE V%d(%d), %d\r\n", PC,(opcode & 0x0F00) >> 8, REGISTERS[(opcode & 0x0F00) >> 8],(opcode & 0x00FF));
    if(REGISTERS[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        PC += 2;
}
//OK
static inline void opcode_sne_reg_val(uint16_t opcode){
    if(REGISTERS[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        PC += 2;
}
//OK
static inline void opcode_se_reg_reg(uint16_t opcode){
    if(REGISTERS[(opcode & 0x0F00) >> 8] == REGISTERS[(opcode & 0x00F0) >> 4])
        PC += 2;
}
//OK
static inline void opcode_ld_reg_val(uint16_t opcode){
    //printf("LD V%d, %#04x\r\n",(opcode & 0x0F00) >> 8, (opcode & 0x00FF));
    REGISTERS[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
}
//OK
static inline void opcode_add_reg_val(uint16_t opcode){
    REGISTERS[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
}
//OK
static inline void opcode_ld_reg_reg(uint16_t opcode){
    REGISTERS[(opcode & 0x0F00) >> 8] = REGISTERS[(opcode & 0x00F0) >> 4];
}
//OK
static inline void opcode_or_reg_reg(uint16_t opcode){
    REGISTERS[(opcode & 0x0F00) >> 8] |= REGISTERS[(opcode & 0x00F0) >> 4];
}
//OK
static inline void opcode_and_reg_reg(uint16_t opcode){
    REGISTERS[(opcode & 0x0F00) >> 8] &= REGISTERS[(opcode & 0x00F0) >> 4];
}
//OK
static inline void opcode_xor_reg_reg(uint16_t opcode){
    REGISTERS[(opcode & 0x0F00) >> 8] ^= REGISTERS[(opcode & 0x00F0) >> 4];
}
//OK
static inline void opcode_add_reg_reg(uint16_t opcode){
    uint16_t tmp = REGISTERS[(opcode & 0x0F00) >> 8] + REGISTERS[(opcode & 0x00F0) >> 4];
    REGISTERS[VF] = tmp > 0xFF;
    REGISTERS[(opcode & 0x0F00) >> 8] = tmp;
}
//OK
static inline void opcode_sub_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[(opcode & 0x0F00) >> 8] > REGISTERS[(opcode & 0x00F0) >> 4];
    REGISTERS[(opcode & 0x0F00) >> 8] -= REGISTERS[(opcode & 0x00F0) >> 4];
}
//OK
static inline void opcode_shr_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[(opcode & 0x0F00) >> 8] & 0x01;
    REGISTERS[(opcode & 0x0F00) >> 8] >>= 1;
}
//OK
static inline void opcode_subn_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[(opcode & 0x0F00) >> 8] < REGISTERS[(opcode & 0x00F0) >> 4];
    REGISTERS[(opcode & 0x0F00) >> 8] = REGISTERS[(opcode & 0x00F0) >> 4] - REGISTERS[(opcode & 0x0F00) >> 8];
}
//OK
static inline void opcode_shl_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[(opcode & 0x0F00) >> 8] >= 0x80;
    REGISTERS[(opcode & 0x0F00) >> 8] <<= 1;
}
//OK
static inline void opcode_sne_reg_reg(uint16_t opcode){
    if(REGISTERS[(opcode & 0x0F00) >> 8] != REGISTERS[(opcode & 0x00F0) >> 4])
        PC += 2;
}
//OK
static inline void opcode_ld_i_val(uint16_t opcode){
    //printf("LD I, %#05x\r\n", opcode & 0x0FFF);
    I = opcode & 0x0FFF;
}
//OK
static inline void opcode_jp_reg_val(uint16_t opcode){
   // printf("JP V0, %#05x\r\n", opcode & 0xFFF);
    PC = (opcode & 0x0FFF) + REGISTERS[V0];
}
//OK

unsigned char generate_random_byte() {
    HCRYPTPROV hCryptProv;
    unsigned char random_byte;

    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        fprintf(stderr, "Ошибка при инициализации криптопровайдера\n");
        exit(1);
    }

    if (!CryptGenRandom(hCryptProv, sizeof(random_byte), &random_byte)) {
        fprintf(stderr, "Ошибка генерации случайного числа\n");
        exit(1);
    }

    CryptReleaseContext(hCryptProv, 0);
    return random_byte;
}


static inline void opcode_rnd_reg_val(uint16_t opcode){
   REGISTERS[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
   //REGISTERS[(opcode & 0x0F00) >> 8] = generate_random_byte() & (opcode & 0x00FF);
  // printf("RND V%d, %#04x=%d\r\n",  (opcode & 0x0F00) >> 8,(opcode & 0x00FF), REGISTERS[(opcode & 0x0F00) >> 8]);
}

static inline void opcode_drw_reg_reg_val(uint16_t opcode){
    uint8_t x = REGISTERS[(opcode & 0x0F00) >> 8];
    uint8_t y = REGISTERS[(opcode & 0x00F0) >> 4];

    //if(x>=FRAMEBUFFER_X)
   //     x -= FRAMEBUFFER_X;
    //if(y>=FRAMEBUFFER_Y)
    //    y -= FRAMEBUFFER_Y;
    uint8_t n = opcode & 0x000F;
   // printf("DRW V%d, V%d, %d\r\n",  (opcode & 0x0F00) >> 8 ,(opcode & 0x00F0) >> 4 ,n);
    uint16_t index = x + y * FRAMEBUFFER_X;
    for(uint8_t y_pos = 0; y_pos < n; y_pos++){
        for(uint8_t x_pos = 0; x_pos < 8; x_pos++){
            framebuffer[index + x_pos + y_pos * FRAMEBUFFER_X] ^= (memory[I+y_pos] & (0x80 >> x_pos)) >> (7-x_pos);
        }
    }

    clear_display();
    draw_display();
    //prinf_frame();
}

static inline void opcode_skp_reg(uint16_t opcode){}

static inline void opcode_sknp_reg(uint16_t opcode){}
//OK
static inline void opcode_ld_reg_dt(uint16_t opcode){
   // printf("LD V%d(%d), DT(%d)\r\n", (opcode & 0x0F00) >> 8, REGISTERS[(opcode & 0x0F00) >> 8], DT);
    REGISTERS[(opcode & 0x0F00) >> 8] = DT;
}

static inline void opcode_ld_reg_k(uint16_t opcode){}
//OK
static inline void opcode_ld_dt_reg(uint16_t opcode){
     DT = REGISTERS[(opcode & 0x0F00) >> 8];
}
//OK
static inline void opcode_ld_st_reg(uint16_t opcode){
    ST = REGISTERS[(opcode & 0x0F00) >> 8];
}
//OK
static inline void opcode_add_i_reg(uint16_t opcode){
    I += REGISTERS[(opcode & 0x0F00) >> 8];
}
//OK
static inline void opcode_ld_f_reg(uint16_t opcode){
    I = REGISTERS[(opcode & 0x0F00) >> 8] * RESERVED_SPRITE_SIZE + BASE_FONTSET_OFFSET;
}
//OK
static inline void opcode_ld_b_reg(uint16_t opcode){
    const uint8_t reg_val = REGISTERS[(opcode & 0x0F00) >> 8];
    memory[I] = reg_val / 100;
    memory[I + 1] = (reg_val / 10) % 10;
    memory[I + 2] = (reg_val / 100) % 10;
}
//OK
static inline void opcode_ld_i_reg(uint16_t opcode){
    for (uint8_t i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
        memory[I + i] = REGISTERS[i];
    }
}
//OK
static  inline void opcode_ld_reg_i(uint16_t opcode){
    for (uint8_t i = 0; i <= ((opcode & 0x0F00) >> 8); ++i) {
        REGISTERS[i] = memory[I + i];
    }
}



static  inline void opcode_0(uint16_t opcode){
    switch (opcode & 0x000F) {
        case 0x0000:
            opcode_cls(opcode);
            break;
        case 0x000E:
            opcode_ret();
            break;
        default:
            opcode_sys(opcode);
    }
}

static  inline void opcode_8(uint16_t opcode){
    switch (opcode & 0x000F) {
        case 0x0000:
            opcode_ld_reg_reg(opcode);
            break;
        case 0x0001:
            opcode_or_reg_reg(opcode);
            break;
        case 0x0002:
            opcode_and_reg_reg(opcode);
            break;
        case 0x0003:
            opcode_xor_reg_reg(opcode);
            break;
        case 0x0004:
            opcode_add_reg_reg(opcode);
            break;
        case 0x0005:
            opcode_sub_reg_reg(opcode);
            break;
        case 0x0006:
            opcode_shl_reg_reg(opcode);
            break;
        case 0x0007:
            opcode_subn_reg_reg(opcode);
            break;
        case 0x000E:
            opcode_shr_reg_reg(opcode);
            break;
    }
}

static  inline void opcode_F(uint16_t opcode){
    switch (opcode & 0x00FF) {
        case 0x0007:
            opcode_ld_reg_dt(opcode);
            break;
        case 0x000A:
            opcode_ld_reg_k(opcode);
            break;
        case 0x0015:
            opcode_ld_dt_reg(opcode);
            break;
        case 0x0018:
            opcode_ld_st_reg(opcode);
            break;
        case 0x001E:
            opcode_add_i_reg(opcode);
            break;
        case 0x0029:
            opcode_ld_f_reg(opcode);
            break;
        case 0x0033:
            opcode_ld_b_reg(opcode);
            break;
        case 0x0055:
            opcode_ld_i_reg(opcode);
            break;
        case 0x0065:
            opcode_ld_reg_i(opcode);
            break;
    }
}


static  inline void opcode_E(uint16_t opcode){
    switch (opcode & 0x00FF) {
        case 0x009E:
            opcode_skp_reg(opcode);
            break;
        case 0x00A1:
            opcode_sknp_reg(opcode);
            break;
    }
}







#define TIMER_ID 1
#define TIMER_INTERVAL 1000 / 60 // Интервал в миллисекундах для частоты 60 Гц

VOID CALLBACK TimerDT(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (DT != 0) {
        InterlockedDecrement(&DT);
      //  printf("DT=%d\r\n",DT);
    }
}






int main() {
    const uint8_t base_fontset[80] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          FRAMEBUFFER_X * 10,
                                          FRAMEBUFFER_Y * 10,
                                          SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    memcpy(memory + BASE_FONTSET_OFFSET, base_fontset, sizeof(base_fontset));
    UINT_PTR timerId = SetTimer(NULL, TIMER_ID, TIMER_INTERVAL, TimerDT);
    if (timerId == 0) {
        printf("Ошибка при создании таймера\n");
        return 1;
    }
    srand(time(0));
//    for (int i = 0; i < 16; ++i) {
//        printf("%d\r\n", rand());
//    }
   //return 9;
    FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Astro Dodge Hires [Revival Studios, 2008] (1).ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Hires Worm V4 [RB-Revival Studios, 2007].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Maze [David Winter, 199x].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Trip8 Demo (2008) [Revival Studios].ch8", "rb");
    fread(memory + PROGRAM_OFFSET, MEMORY_SIZE - PROGRAM_OFFSET, sizeof(uint8_t), file);
    fclose(file);

    MSG msg;
    Uint32 next_time = SDL_GetTicks() + TIMER_INTERVAL;
    //while (GetMessage(&msg, NULL, 0, 0)) {
    while (1) {
        TranslateMessage(&msg);


        uint16_t opcode = memory[PC];
        opcode <<= 8;
        opcode |= memory[PC + 1];

        switch (opcode & 0xF000) {
            case 0x0000:
                opcode_0(opcode);
                break;
            case 0x1000:
                opcode_jp(opcode);
                break;
            case 0x2000:
                opcode_call(opcode);
                break;
            case 0x3000:
                opcode_se_reg_val(opcode);
                break;
            case 0x4000:
                opcode_sne_reg_val(opcode);
                break;
            case 0x5000:
                opcode_se_reg_reg(opcode);
                break;
            case 0x6000:
                opcode_ld_reg_val(opcode);
                break;
            case 0x7000:
                opcode_add_reg_val(opcode);
                break;
            case 0x8000:
                opcode_8(opcode);
                break;
            case 0x9000:
                opcode_sne_reg_reg(opcode);
                break;
            case 0xA000:
                opcode_ld_i_val(opcode);
                break;
            case 0xB000:
                opcode_jp_reg_val(opcode);
                break;
            case 0xC000:
                opcode_rnd_reg_val(opcode);
                break;
            case 0xD000:
                opcode_drw_reg_reg_val(opcode);
                break;
            case 0xE000:
                opcode_E(opcode);//ok
                break;
            case 0xF000:
                opcode_F(opcode);//ok
                break;
        }



        PC += 2;
       // DispatchMessage(&msg);
       //Sleep(15);
        // Отображение на экране
        SDL_RenderPresent(renderer);

        // Обработка событий SDL (например, выход из цикла при закрытии окна)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                break;
            }
        }
        Uint32 now = SDL_GetTicks();
        if (now >= next_time) {
            // Обработка кадра
            if (DT != 0) {
                DT--;
                //  printf("DT=%d\r\n",DT);
            }
            // Обновляем время для следующего кадра
            next_time += TIMER_INTERVAL;
        }
    }
//    I = 0x50;
//    REGISTERS[V0] = 0;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x55;
//    REGISTERS[V0] = 5;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x5A;
//    REGISTERS[V0] = 10;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x5F;
//    REGISTERS[V0] = 15;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x64;
//    REGISTERS[V0] = 20;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x69;
//    REGISTERS[V0] = 25;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x6E;
//    REGISTERS[V0] = 30;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x73;
//    REGISTERS[V0] = 35;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//
//    I = 0x78;
//    REGISTERS[V0] = 40;
//    REGISTERS[V1] = 0;
//    opcode_drw_reg_reg_val(0xD015);
//    prinf_frame();
    return 0;
}
