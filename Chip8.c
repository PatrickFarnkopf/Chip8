
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

#include "Chip8.h"

typedef unsigned char byte;

#define PIXEL_WIDTH 64
#define PIXEL_HEIGHT 32

#define DELAY_TIMER 0
#define SOUND_TIMER 1
#define TIMER_COUNT 2

#define MAX_STACK_LEVELS 16

#define MAX_KEYS 16

unsigned short opcode;
unsigned short I; 
unsigned short pc; 

byte memory[4096];
byte V[16]; 
byte graphics[PIXEL_WIDTH * PIXEL_HEIGHT];
byte timer[TIMER_COUNT];
byte stack[MAX_STACK_LEVELS];
byte stackPtr; 
byte key[MAX_KEYS];

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Invalid arguments\n");
        return 0;
    }

    FILE *filePtr;
    long length;
    byte* buffer;

    initscr();
    curs_set(0);

    filePtr = fopen(argv[1], "rb");  
    fseek(filePtr, 0, SEEK_END);          
    length = ftell(filePtr);             
    rewind(filePtr);                      

    buffer = (byte*)malloc((length + 1)*sizeof(byte)); 
    fread(buffer, length, 1, filePtr); 
    fclose(filePtr); 

    initialize();

    for (unsigned int i = 0x200, j = 0x0; j < length; ++i, ++j)
        memory[i] = buffer[j];

    while (true)
    {
        step();
        refresh();
        usleep(16.6f);
    }

    endwin();
}

void initialize()
{
    pc          = 0x200;  
    opcode      = 0x0;
    I           = 0x0;
    stackPtr    = 0x0;

    for (int i = 0; i < 2048; ++i)
        graphics[i] = 0;

    for (int i = 0; i < 16; ++i)
        stack[i] = 0;

    for (int i = 0; i < 16; ++i)
        key[i] = V[i] = 0;

    for (int i = 0; i < 4096; ++i)
        memory[i] = 0;

    for (int i = 0; i < 80; ++i)
        memory[i] = chip8_fontset[i];

    for (int i = 0; i < 80; ++i)
        memory[i] = chip8_fontset[i];
}

void render()
{
    static int tick = 0;

    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            if (graphics[(y*64) + x] == 0) 
                mvprintw(y, x, " ");
            else 
                mvprintw(y, x, "#");
        }
    }

    mvprintw(0, 0, "Tick: %i", ++tick);
}

void step()
{
    opcode = memory[pc] << 8 | memory[pc + 1];  

    switch(opcode & 0xF000)
    {       
        case 0x0000:
            switch(opcode & 0x000F)
            {
                case 0x0000: 
                    for (int i = 0; i < 2048; ++i)
                    {
                        graphics[i] = 0x0;
                    }
                    pc += 2;
                break;

                case 0x000E: 
                    --stackPtr;           
                    pc = stack[stackPtr]; 
                    pc += 2;        
                break;

                default:;
                    
            }
        break;

        case 0x1000: 
            pc = opcode & 0x0FFF;
        break;

        case 0x2000: 
            stack[stackPtr] = pc;         
            ++stackPtr;                   
            pc = opcode & 0x0FFF;   
        break;
        
        case 0x3000:
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
        break;
        
        case 0x4000:
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
        break;
        
        case 0x5000:
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
        break;
        
        case 0x6000:
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
        break;
        
        case 0x7000:
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
        break;
        
        case 0x8000:
            switch(opcode & 0x000F)
            {
                case 0x0000:
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0001:
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0002:
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0003:
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0004:                
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) 
                        V[0xF] = 1;
                    else 
                        V[0xF] = 0;                 
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    pc += 2;                    
                break;

                case 0x0005:
                    if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) 
                        V[0xF] = 0;
                    else 
                        V[0xF] = 1;                 
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0006:
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                break;

                case 0x0007:
                    if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
                        V[0xF] = 0;
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];              
                    pc += 2;
                break;

                case 0x000E:
                    V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                break;

                default:;
            }
        break;
        
        case 0x9000:
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
        break;

        case 0xA000:
            I = opcode & 0x0FFF;
            pc += 2;
        break;
        
        case 0xB000: 
            pc = (opcode & 0x0FFF) + V[0];
        break;
        
        case 0xC000: 
            V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;
        break;
    
        case 0xD000: 
        {
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++)
            {
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++)
                {
                    if ((pixel & (0x80 >> xline)) != 0)
                    {
                        if (graphics[(x + xline + ((y + yline) * 64))] == 1)
                        {
                            V[0xF] = 1;                                    
                        }
                        graphics[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }     
            pc += 2;

            render();
        }
        break;
            
        case 0xE000:
            switch(opcode & 0x00FF)
            {
                case 0x009E:
                    if (key[V[(opcode & 0x0F00) >> 8]] != 0)
                        pc += 4;
                    else
                        pc += 2;
                break;
                
                case 0x00A1: 
                    if (key[V[(opcode & 0x0F00) >> 8]] == 0)
                        pc += 4;
                    else
                        pc += 2;
                break;

                default:;
            }
        break;
        
        case 0xF000:
            switch(opcode & 0x00FF)
            {
                case 0x0007:
                    V[(opcode & 0x0F00) >> 8] = timer[DELAY_TIMER];
                    pc += 2;
                break;

                case 0x000A:
                {
                    bool keyPress = false;

                    for (int i = 0; i < 16; ++i)
                    {
                        if (key[i] != 0)
                        {
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPress = true;
                        }
                    }

                    if (!keyPress)                       
                        return;

                    pc += 2;                    
                }
                break;
                
                case 0x0015:
                    timer[DELAY_TIMER] = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x0018:
                    timer[SOUND_TIMER] = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x001E:
                    if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x0029:
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                break;

                case 0x0033:
                    memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;                 
                    pc += 2;
                break;

                case 0x0055:          
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        memory[I + i] = V[i];   

                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                break;

                case 0x0065:           
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        V[i] = memory[I + i];           

                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                break;

                default:;
            }
        break;
 
        default:;
    }  
 
    if (timer[DELAY_TIMER] > 0)
      --timer[DELAY_TIMER];
 
    if (timer[SOUND_TIMER] > 0)
    {
        if (timer[SOUND_TIMER] == 1)
            printf("\a");
        --timer[SOUND_TIMER];
    }  
}