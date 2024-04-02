#include <stdio.h>
#include <stdint.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "SDL.h"

#define MEMORY_SIZE 4096
#define STACK_SIZE 64
#define REGISTERS_COUNT 16



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

//#define FRAMEBUFFER_X   64
uint8_t FRAMEBUFFER_X =  64;
uint8_t FRAMEBUFFER_Y =  32;
//#define FRAMEBUFFER_Y   32
static uint8_t memory[MEMORY_SIZE];
static uint8_t REGISTERS[REGISTERS_COUNT];
static uint8_t framebuffer[64*128];
static uint8_t keypad[16];
//static uint8_t framebuffer[FRAMEBUFFER_X * FRAMEBUFFER_Y];
static uint16_t STACK[STACK_SIZE / sizeof(uint16_t)];
static uint8_t SP = -1;
static uint8_t DT = 0;
static uint8_t ST = 0;
static uint16_t I = 0;
static uint16_t PC = PROGRAM_OFFSET;

SDL_Renderer* renderer;

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

#define NNN(x)  ((x) & 0xFFF)
#define X(x)  (((x) & 0x0F00) >> 8)
#define Y(x)  (((x) & 0x00F0) >> 4)
#define N(x)  ((x) & 0x000F)
#define KK(x)  ((x) & 0x00FF)
#define STACK_PUSH(x)   (STACK[++SP] = (x))
#define STACK_POP(x)   ((x) = STACK[SP--])

static inline void opcode_sys(uint16_t opcode){}

//OK
static inline void opcode_cls(uint16_t opcode){
   // clear_display();

    memset(framebuffer, 0, FRAMEBUFFER_X*FRAMEBUFFER_Y);
    //SDL_RenderPresent(renderer);
   // clear_display();
    //draw_display();
}

//OK
static inline void opcode_ret(){
    STACK_POP(PC);
}
//OK
static inline void opcode_jp(uint16_t opcode){
    PC = NNN(opcode) - 2;
}
//OK
static inline void opcode_call(uint16_t opcode){
    STACK_PUSH(PC);
    PC = NNN(opcode) - 2;
}

//OK
static inline void opcode_se_reg_val(uint16_t opcode){
    if(REGISTERS[X(opcode)] == KK(opcode))
        PC += 2;
}
//OK
static inline void opcode_sne_reg_val(uint16_t opcode){
    if(REGISTERS[X(opcode)] != KK(opcode))
        PC += 2;
}
//OK
static inline void opcode_se_reg_reg(uint16_t opcode){
    if(REGISTERS[X(opcode)] == REGISTERS[Y(opcode)])
        PC += 2;
}
//OK
static inline void opcode_ld_reg_val(uint16_t opcode){
    REGISTERS[X(opcode)] = KK(opcode);
}
//OK
static inline void opcode_add_reg_val(uint16_t opcode){
    REGISTERS[X(opcode)] += KK(opcode);
}
//OK
static inline void opcode_ld_reg_reg(uint16_t opcode){
    REGISTERS[X(opcode)] = REGISTERS[Y(opcode)];
}
//OK
static inline void opcode_or_reg_reg(uint16_t opcode){
    REGISTERS[X(opcode)] |= REGISTERS[Y(opcode)];
}
//OK
static inline void opcode_and_reg_reg(uint16_t opcode){
    REGISTERS[X(opcode)] &= REGISTERS[Y(opcode)];
}
//OK
static inline void opcode_xor_reg_reg(uint16_t opcode){
    REGISTERS[X(opcode)] ^= REGISTERS[Y(opcode)];
}
//OK
static inline void opcode_add_reg_reg(uint16_t opcode){
    uint16_t tmp = REGISTERS[X(opcode)] + REGISTERS[Y(opcode)];
    REGISTERS[VF] = tmp > 0xFF;
    REGISTERS[X(opcode)] = tmp & 0xFF;
}
//OK
static inline void opcode_sub_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[X(opcode)] > REGISTERS[Y(opcode)];
    REGISTERS[X(opcode)] -= REGISTERS[Y(opcode)];
}
//OK
static inline void opcode_shr_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[X(opcode)] & 0x01;
    REGISTERS[X(opcode)] >>= 1;
}
//OK
static inline void opcode_subn_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[X(opcode)] < REGISTERS[Y(opcode)];
    REGISTERS[X(opcode)] = REGISTERS[Y(opcode)] - REGISTERS[X(opcode)];
}
//OK
static inline void opcode_shl_reg_reg(uint16_t opcode){
    REGISTERS[VF] = REGISTERS[X(opcode)] >> 7;
    REGISTERS[X(opcode)] <<= 1;
}
//OK
static inline void opcode_sne_reg_reg(uint16_t opcode){
    if(REGISTERS[X(opcode)] != REGISTERS[Y(opcode)])
        PC += 2;
}
//OK
static inline void opcode_ld_i_val(uint16_t opcode){
    I = NNN(opcode);
}
//OK
static inline void opcode_jp_reg_val(uint16_t opcode){
    PC = NNN(opcode) + REGISTERS[V0] - 2;
}
//OK
static inline void opcode_rnd_reg_val(uint16_t opcode){
   REGISTERS[X(opcode)] = (rand() % 0xFF) & KK(opcode);
}
//OK
static inline void opcode_drw_reg_reg_val(uint16_t opcode){
    uint8_t x = REGISTERS[X(opcode)];
    uint8_t y = REGISTERS[Y(opcode)];
    uint8_t n = N(opcode);

    REGISTERS[VF] = 0;
    for (int yline = 0; yline < n; yline++) {
        uint8_t pixel = memory[I + yline];
        for(int xline = 0; xline < 8; xline++) {
            if((pixel & (0x80 >> xline)) != 0) {
                if(framebuffer[(x + xline + ((y + yline) * 64))] == 1){
                    REGISTERS[VF] = 1;
                }
                framebuffer[x + xline + ((y + yline) * 64)] ^= 1;
            }

        }

    }
//    uint16_t index = x + y * FRAMEBUFFER_X;
//    for(uint8_t y_pos = 0; y_pos < n; y_pos++){
//        for(uint8_t x_pos = 0; x_pos < 8; x_pos++){
//            uint8_t pixel = (memory[I+y_pos] & (0x80 >> x_pos)) >> (7-x_pos);
//            if(pixel) {
//                REGISTERS[VF] = framebuffer[index + x_pos + y_pos * FRAMEBUFFER_X] == 1;
//                framebuffer[index + x_pos + y_pos * FRAMEBUFFER_X] ^= 1;
//            }
//        }
//    }
}
//OK
static inline void opcode_skp_reg(uint16_t opcode){
    if(keypad[REGISTERS[X(opcode)]])
        PC+=2;
}
//OK
static inline void opcode_sknp_reg(uint16_t opcode){
    if(!keypad[REGISTERS[X(opcode)]])
        PC+=2;
}
//OK
static inline void opcode_ld_reg_dt(uint16_t opcode){
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
    uint8_t reg_val = REGISTERS[(opcode & 0x0F00) >> 8];
//    memory[I] = reg_val / 100;
//    memory[I + 1] = (reg_val / 10) % 10;
//    memory[I + 2] = (reg_val / 100) % 10;
    memory[I]     = (reg_val - (reg_val % 100)) / 100;
    reg_val -= memory[I] * 100;
    memory[I + 1] = (reg_val - (reg_val % 10)) / 10;
    reg_val -= memory[I+1] * 10;
    memory[I + 2] = reg_val;
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
            opcode_shr_reg_reg(opcode);
            break;
        case 0x0007:
            opcode_subn_reg_reg(opcode);
            break;
        case 0x000E:
            opcode_shl_reg_reg(opcode);
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


int run  =1;

_Noreturn int handler(void *data){
    while(1){
        Uint32 now = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                run = 0;
                break;
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_0:
                        keypad[0x0] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_1:
                        keypad[0x1] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_2:
                        keypad[0x2] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_3:
                        keypad[0x3] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_4:
                        keypad[0x4] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_5:
                        keypad[0x5] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_6:
                        keypad[0x6] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_7:
                        keypad[0x7] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_8:
                        keypad[0x8] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_9:
                        keypad[0x9] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_a:
                        keypad[0xA] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_b:
                        keypad[0xB] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_c:
                        keypad[0xC] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_d:
                        keypad[0xD] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_e:
                        keypad[0xE] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_f:
                        keypad[0xF] = event.type == SDL_KEYDOWN;
                        break;
                    default:
                        break;

                }
            }
        }
        while(now+100!=SDL_GetTicks());
    }
}




int main(int argc, char *argv[]) {
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
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          FRAMEBUFFER_X * 10,
                                          FRAMEBUFFER_X * 10,
                                          //FRAMEBUFFER_Y * 10,
                                          SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    memcpy(memory + BASE_FONTSET_OFFSET, base_fontset, sizeof(base_fontset));
    memset(keypad,0,16);
    memset(REGISTERS,0,16);
    memset(STACK,0,64);
    srand(time(0));

    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\3-corax+.ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Tic-Tac-Toe [David Winter].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Pong (1 player).ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Tetris [Fran Dachille, 1991].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Astro Dodge Hires [Revival Studios, 2008] (1).ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Hires Worm V4 [RB-Revival Studios, 2007].ch8", "rb");
   // FILE* file = fopen("C:\\Users\\kandu\\Downloads\\br8kout.ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Maze [David Winter, 199x].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Zero Demo [zeroZshadow, 2007].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Stars [Sergey Naydenov, 2010].ch8", "rb");
    //FILE* file = fopen("C:\\Users\\kandu\\Downloads\\Trip8 Demo (2008) [Revival Studios].ch8", "rb");
    fread(memory + PROGRAM_OFFSET, MEMORY_SIZE - PROGRAM_OFFSET, sizeof(uint8_t), file);
    fclose(file);
    //SDL_Thread *thread = SDL_CreateThread(handler, "ThreadName", NULL);
    Uint32 next_time = SDL_GetTicks() + 1000/60;
    SDL_Event event;
    while (run) {
//        while(SDL_PollEvent(&event))
//        {
//            switch(event.type)
//            {
//                case SDL_QUIT:
//                    run = 0;
//                    break;
//
//                case SDL_KEYDOWN:
//
//                    switch (event.key.keysym.sym)
//                    {
//
//
//                        case SDLK_x:keypad[0] = 1;break;
//                        case SDLK_1:keypad[1] = 1;break;
//                        case SDLK_2:keypad[2] = 1;break;
//                        case SDLK_3:keypad[3] = 1;break;
//                        case SDLK_q:keypad[4] = 1;break;
//                        case SDLK_w:keypad[5] = 1;break;
//                        case SDLK_e:keypad[6] = 1;break;
//                        case SDLK_a:keypad[7] = 1;break;
//                        case SDLK_s:keypad[8] = 1;break;
//                        case SDLK_d:keypad[9] = 1;break;
//                        case SDLK_z:keypad[0xA] = 1;break;
//                        case SDLK_c:keypad[0xB] = 1;break;
//                        case SDLK_4:keypad[0xC] = 1;break;
//                        case SDLK_r:keypad[0xD] = 1;break;
//                        case SDLK_f:keypad[0xE] = 1;break;
//                        case SDLK_v:keypad[0xF] = 1;break;
//                    }
//                    break;
//
//                case SDL_KEYUP:
//
//                    switch (event.key.keysym.sym)
//                    {
//                        case SDLK_x:keypad[0] = 0;break;
//                        case SDLK_1:keypad[1] = 0;break;
//                        case SDLK_2:keypad[2] = 0;break;
//                        case SDLK_3:keypad[3] = 0;break;
//                        case SDLK_q:keypad[4] = 0;break;
//                        case SDLK_w:keypad[5] = 0;break;
//                        case SDLK_e:keypad[6] = 0;break;
//                        case SDLK_a:keypad[7] = 0;break;
//                        case SDLK_s:keypad[8] = 0;break;
//                        case SDLK_d:keypad[9] = 0;break;
//                        case SDLK_z:keypad[0xA] = 0;break;
//                        case SDLK_c:keypad[0xB] = 0;break;
//                        case SDLK_4:keypad[0xC] = 0;break;
//                        case SDLK_r:keypad[0xD] = 0;break;
//                        case SDLK_f:keypad[0xE] = 0;break;
//                        case SDLK_v:keypad[0xF] = 0;break;
//                    }
//                    break;
//            }
//            break;
//        }
//        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_0:
                        keypad[0x0] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_1:
                        keypad[0x1] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_2:
                        keypad[0x2] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_3:
                        keypad[0x3] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_4:
                        keypad[0x4] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_5:
                        keypad[0x5] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_6:
                        keypad[0x6] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_7:
                        keypad[0x7] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_8:
                        keypad[0x8] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_9:
                        keypad[0x9] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_a:
                        keypad[0xA] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_b:
                        keypad[0xB] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_c:
                        keypad[0xC] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_d:
                        keypad[0xD] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_e:
                        keypad[0xE] = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_f:
                        keypad[0xF] = event.type == SDL_KEYDOWN;
                        break;
                    default:
                        break;

                }
                break;
            }
            else if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                run = 0;
                break;
            }
        }


        Uint32 now = SDL_GetTicks();


        uint16_t opcode = memory[PC];
        opcode <<= 8;
        opcode |= memory[PC + 1];
        //64x64 mode
        if ((PC==0x200) && (opcode==0x1260)) {
            // Init 64x64 hires mode
            FRAMEBUFFER_Y=64;
            opcode=0x12C0;  // Make the interperter jump to address 0x2c0
        }
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
        if (now >= next_time) {
            if (DT != 0)
                DT--;
            next_time += 1000/60;
        }
        SDL_RenderPresent(renderer);
        clear_display();
        draw_display();

        while(now== SDL_GetTicks());
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
