#include <cstdint>
#include <iostream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <random>
#include <SDL2/SDL.h>

// using namespace std;

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int FONTSET_SIZE = 80;

uint8_t fontset[FONTSET_SIZE] = 
{
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

class Chip8 {
    public:
        uint8_t registers[16]{};
        uint8_t memory[4096]{};
        uint16_t index{};
        uint16_t pc{};
        uint16_t stack[16]{};
        uint8_t sp{};
        uint8_t delayTimers{};
        uint8_t soundTimers{};
        uint8_t keypad[16]{};
        uint32_t video[64*32]{};
        uint16_t opcode;
        bool running;

    Chip8();

    void LoadROM(const char* filename);
    void LogState();
    void EmulateCycle();
    void HandleInput();
    void Render(SDL_Renderer* renderer);
    void Run();
    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();
    void OP_4xkk();
    void OP_5xy0();
    void OP_6xkk();
    void OP_7xkk();
    void OP_8xy0();
    void OP_8xy1();
    void OP_8xy2();
    void OP_8xy3();
    void OP_8xy4();
    void OP_8xy5();
    void OP_8xy6();
    void OP_8xy7();
    void OP_8xyE();
    void OP_9xy0();
    void OP_Annn();
    void OP_Bnnn();
    void OP_Cxkk();
    void OP_Dxyn();
    void OP_Ex9E();
    void OP_ExA1();
    void OP_Fx07();
    void OP_Fx0A();
    void OP_Fx15();
    void OP_Fx18();
    void OP_Fx1E();
    void OP_Fx29();
    void OP_Fx33();
    void OP_Fx55();
    void OP_Fx65();

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;
};

Chip8::Chip8()
        : randGen(std::chrono::system_clock::now().time_since_epoch().count())
        , running(true)
    {
        randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
        pc = START_ADDRESS;

        for(unsigned int i = 0; i < FONTSET_SIZE; ++i) {
            memory[FONTSET_START_ADDRESS + i] = fontset[i];
        }
    }

void Chip8::LoadROM(const char* filename){
    // Open file as Binary type and move the pointer to the end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(file.is_open()){
        // Get the size of the file by getting the current position of pointer
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        // Go back to the beginning of the file
        file.seekg(0, std::ios::beg);
        
        // Store ROM data to buffer
        file.read(buffer, size);
        file.close();

        // Load the ROM data from buffer to memory, starting at 0x200
        for(long i = 0; i < size; ++i){
            memory[START_ADDRESS + i] = buffer[i];
        }

        // Free the buffer
        delete[] buffer;
    }
};

void Chip8::OP_00E0(){
    // Clear the display by setting all video memory to 0
    memset(video, 0, sizeof(video));
}

void Chip8::OP_00EE(){
    sp--;
    pc = stack[sp];
}

void Chip8::OP_1nnn(){
    // Decode instruction from opcode, in this case is JUMP
    uint16_t address = opcode & 0x0FFFu; // Extract "nnn" or the address to JUMP to
    pc = address;
}

void Chip8::OP_2nnn(){
    uint16_t address = opcode & 0x0FFFu;

    stack[sp] = pc; // Save the current PC 
    sp++; // Increment the SP
    pc = address; // JUMP to Address nnn
}

void Chip8::OP_3xkk(){
    // Compare if Vx == kk then skip
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FFu;

    if(registers[Vx] == kk){
        pc += 2;
    }
}

void Chip8::OP_4xkk(){
    // Compare if Vx != kk then skip
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FFu;

    if(registers[Vx] != kk){
        pc += 2;
    }
}

void Chip8::OP_5xy0(){
    // Compare if Vx == Vy then skip
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] == registers[Vy]){
        pc += 2;
    }
}

void Chip8::OP_6xkk(){
    // Set Vx = kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FFu;

    registers[Vx] = kk;
}

void Chip8::OP_7xkk(){
    // ADD kk to Vx or Vx = Vx + kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FFu;
     
    registers[Vx] += kk;
}

void Chip8::OP_8xy0(){
    // Set Vx = Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

void Chip8::OP_8xy1(){
    // Set Vx = Vx or Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vx] | registers[Vy];
}

void Chip8::OP_8xy2(){
    // Set Vx = Vx and Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vx] & registers[Vy];
}

void Chip8::OP_8xy3(){
    // Set Vx = Vx xor Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vx] ^ registers[Vy];
}

void Chip8::OP_8xy4(){
    // ADD Vx = Vx + Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    registers[Vx] = sum & 0xFF;
    registers[0xF] = (sum > 255) ? 1 : 0;
}

void Chip8::OP_8xy5(){
    // SUB Vx = Vx - Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[0xF] = (registers[Vx] >= registers[Vy]) ? 1 : 0;
    registers[Vx] = registers[Vx] - registers[Vy];
}

void Chip8::OP_8xy6(){
    // Set Vx = Vx SHR 1 in another word, DIV by 2
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    // uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[0xF] = registers[Vx] & 0x1u;
    registers[Vx] >>= 1;
}

void Chip8::OP_8xy7(){
    // SUB Vx = Vy - Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[0xF] = (registers[Vy] >= registers[Vx]) ? 1 : 0;
    registers[Vx] = registers[Vy] - registers[Vx];
}

void Chip8::OP_8xyE(){
    //Set Vx = Vx SHL 1 or multiple by 2
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[0xF] = (registers[Vx] & 0x80) >> 7u;
    registers[Vx] = registers[Vx] << 1;
}

void Chip8::OP_9xy0(){
    // Skip next instruction if Vx != Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] != registers[Vy]){
        pc += 2;
    }
}

void Chip8::OP_Annn(){
    // Set I = nnn
    uint16_t address = opcode & 0x0FFF;

    index = address;
}

void Chip8::OP_Bnnn(){
    // JUMP to location nnn + V0
    uint16_t address = (opcode & 0x0FFFu);
    pc = address + registers[0x0];
}

void Chip8::OP_Cxkk(){
    // Set Vx = random byte AND kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & kk;
}

void Chip8::OP_Dxyn(){
    uint8_t n = opcode & 0x000Fu;
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t xPos = registers[Vx] % 64; // Avoid going ouside of the screen
    uint8_t yPos = registers[Vy] % 32; // Avoid going ouside of the screen
    
    // Reset the VF 
    registers[0xF] = 0;

    for(unsigned int row = 0; row < n; row++){
        uint8_t spriteByte = memory[index + row]; // Get the data at that memory location (ex: 11110000)
        for(unsigned int col = 0; col < 8; col++){
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            uint32_t* screenPixel = &video[(yPos + row) * 64 + (xPos + col)]; // Point to a specific pixel in the video array
            
            if(spritePixel){
                if(*screenPixel == 0xFFFFFFFF){
                    registers[0xF] = 1;
                }

                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

void Chip8::OP_Ex9E(){
    // Check if the corresponding keyboard is pressed, then skip
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if(keypad[registers[Vx]] != 0) {
        pc += 2;
    }
}

void Chip8::OP_ExA1(){
    // Check if the corresponding keyboard is not pressed, then skip
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if(keypad[registers[Vx]] == 0) {
        pc += 2;
    }
}

void Chip8::OP_Fx07(){
    // Set Vx = delay timer value
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimers;
}

void Chip8::OP_Fx0A(){
    // Waits for a key press on the keyboard, stored it valued in Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    bool isPressed = false;
    
    // Iterate through 16 key in keypad
    for(uint8_t key = 0; key < 16; key++){
        if(keypad[key] != 0){
            registers[Vx] = key;
            isPressed = true;
            break;
        }
    }

    // If none of them pressed, repeat this instruction
    if (!isPressed)
    {
        pc -= 2;
    }
}

void Chip8::OP_Fx15(){
    // Set delayTimer equal to Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimers = registers[Vx];
}

void Chip8::OP_Fx18(){
    // Set soundTimer equal to Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimers = registers[Vx];
}

void Chip8::OP_Fx1E(){
    // Set index += Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}

void Chip8::OP_Fx29(){
    // Set index equal location of sprite for digit Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index = 0x50 + (Vx * 5); // FONTSET_START_ADDRESS = 0x50
}

void Chip8::OP_Fx33(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];
    
    for(int i = 3; i > 0; i--){
        memory[index + i - 1] = value % 10;
        value /= 10;
    }
}

void Chip8::OP_Fx55(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for(uint8_t V0 = 0x0; V0 <= Vx; V0++){
        memory[index + V0] = registers[V0];
    }
}

void Chip8::OP_Fx65(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for(uint8_t V0 = 0x0; V0 <= Vx; V0++){
        registers[V0] = memory[index + V0];
    }
}

void Chip8::EmulateCycle(){
    // Fetch opcode, each instruction is 2 bytes
    opcode = (memory[pc] << 8u | memory[pc + 1]);

    pc += 2;

    // Decode and execute the opcode
    switch (opcode & 0xF000u)
    {
    case 0x0000:
        switch (opcode & 0x00FFu)
        {
        case 0x00E0: OP_00E0(); break;
        case 0x00EE: OP_00EE(); break;
        default: std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl; break; // Error handling here
        }
        break;
    case 0x1000: OP_1nnn(); break;
    case 0x2000: OP_2nnn(); break;
    case 0x3000: OP_3xkk(); break;
    case 0x4000: OP_4xkk(); break;
    case 0x5000: OP_5xy0(); break;
    case 0x6000: OP_6xkk(); break;
    case 0x7000: OP_7xkk(); break;
    case 0x8000:
        switch (opcode & 0x000Fu)
        {
        case 0x0000: OP_8xy0(); break;
        case 0x0001: OP_8xy1(); break;
        case 0x0002: OP_8xy2(); break;
        case 0x0003: OP_8xy3(); break;
        case 0x0004: OP_8xy4(); break;
        case 0x0005: OP_8xy5(); break;
        case 0x0006: OP_8xy6(); break;
        case 0x0007: OP_8xy7(); break;
        case 0x000E: OP_8xyE(); break;
        default: std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl; break; // Error handling here
        }
        break;
    case 0x9000: OP_9xy0(); break;
    case 0xA000: OP_Annn(); break;
    case 0xB000: OP_Bnnn(); break;
    case 0xC000: OP_Cxkk(); break;
    case 0xD000: OP_Dxyn(); break;
    case 0xE000:
        switch (opcode & 0x00FFu)
        {
        case 0x009E: OP_Ex9E(); break;
        case 0x00A1: OP_ExA1(); break;
        default: std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl; break; // Error handling here
        }
        break;
    case 0xF000:
        switch (opcode & 0x00FFu)
        {
        case 0x0007: OP_Fx07(); break;
        case 0x000A: OP_Fx0A(); break;
        case 0x0015: OP_Fx15(); break;
        case 0x0018: OP_Fx18(); break;
        case 0x001E: OP_Fx1E(); break;
        case 0x0029: OP_Fx29(); break;
        case 0x0033: OP_Fx33(); break;
        case 0x0055: OP_Fx55(); break;
        case 0x0065: OP_Fx65(); break;
        default: std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl; break; // Error handling here
        }
        break;
    default: std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl; break; // Error handling here
    }

    // Update timers
    if(delayTimers > 0){ delayTimers--; }
    if(soundTimers > 0){ soundTimers--; }
}

void Chip8::HandleInput(){
    SDL_Event event;
    while (SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT){
            running = false;
            return;
        }
        if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP){
            bool isPressed = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE: running = false; break;
            case SDLK_1: keypad[0x1] = isPressed; break;
            case SDLK_2: keypad[0x2] = isPressed; break;
            case SDLK_3: keypad[0x3] = isPressed; break;
            case SDLK_4: keypad[0xC] = isPressed; break;
            case SDLK_q: keypad[0x4] = isPressed; break;
            case SDLK_w: keypad[0x5] = isPressed; break;
            case SDLK_e: keypad[0x6] = isPressed; break;
            case SDLK_r: keypad[0xD] = isPressed; break;
            case SDLK_a: keypad[0x7] = isPressed; break;
            case SDLK_s: keypad[0x8] = isPressed; break;
            case SDLK_d: keypad[0x9] = isPressed; break;
            case SDLK_f: keypad[0xE] = isPressed; break;
            case SDLK_z: keypad[0xA] = isPressed; break;
            case SDLK_x: keypad[0x0] = isPressed; break;
            case SDLK_c: keypad[0xB] = isPressed; break;
            case SDLK_v: keypad[0xF] = isPressed; break;
            default:
                break;
            }
        }
    }
}

void Chip8::LogState(){
    std::cout << "PC: " << std::hex << pc << " | I: " << index << std::endl;
    std::cout << "Registers: ";
    for (int i = 0; i < 16; ++i) std::cout << "V" << i << ": " << (int)registers[i] << " ";
    std::cout << std::endl;
}

void Chip8::Render(SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    for(int y = 0; y < 32; y++){
        for(int x = 0; x < 64; x++){
            if(video[y * 64 + x]) {
                SDL_Rect rect = {x * 10, y * 10, 10, 10};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void Chip8::Run(){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator",
                                            SDL_WINDOWPOS_CENTERED,
                                            SDL_WINDOWPOS_CENTERED,
                                            640, 320, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // bool running = true;
    while (running)
    {
        HandleInput();
        EmulateCycle();
        Render(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* argv[]){
    if(argc != 2){
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }

    const char* romFile = argv[1];

    Chip8 chip8;

    chip8.LoadROM(romFile);
    chip8.Run();

    return 0;
}
